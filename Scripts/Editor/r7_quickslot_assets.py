"""Approved R7 source assets only: prepare/verify/migrate the four fixed packages.

No original asset saves, no dependency expansion or existing target overwrite.
Uses the separately verified QuickSlot bridge; no new native gameplay logic.
"""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unreal as ue
import r7_quickslot_transfer_probe as probe
import r7_quickbar_setup as recipe

PROJECT, SOURCE, REPORT = probe.PROJECT, probe.SOURCE, probe.REPORT
ROOT = "/Game/LyraMigration/UI/R7/Equipment"
GA = "/ShooterCore/Game/GA_QuickbarSlots"
MAPPING = {GA: ROOT + "/Abilities/GA_QuickbarSlots"}
ACTION_MAP = {"/ShooterCore/Input/Actions/IA_QuickSlot" + suffix:
              ROOT + "/Input/Actions/IA_QuickSlot" + suffix
              for suffix in ("1", "2", "3", "_CycleForward", "_CycleBackward")}
MAPPING.update({p: d for p, d in ACTION_MAP.items() if "Cycle" in p})
PACKAGES = list(MAPPING.values()) + [probe.STAGING]
require = probe.require


def policy(bp):
    obj = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    return {key: str(obj.get_editor_property(key)) for key in
            ("ActivationPolicy", "ActivationGroup", "InstancingPolicy", "NetExecutionPolicy")}


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR7SlotAssets=(prepare|verify|migrate)(?:\s|$)", cmd)
    run = re.search(r"-DCR7SlotRun=([A-Za-z0-9_]{1,36})(?:\s|$)", cmd)
    require(mode and run, "Explicit mode/run required")
    mode, run = mode.group(1), run.group(1)
    baseline_run = re.search(r"-DCR7SlotBaseline=([A-Za-z0-9_]{1,36})(?:\s|$)", cmd)
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == SOURCE.resolve(), "Source project only")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(guard and (not isinstance(guard, tuple) or guard[0]), "Unsafe context")
    recipe.helper.LOG = PROJECT / "Saved/Logs" / ("R7Slot-" + run + ".log")
    probe.helper.LOG = recipe.helper.LOG
    receipt = REPORT / ("slot_assets_" + run + ".json")
    require(recipe.helper.LOG.is_file() and not receipt.exists(), "Fresh log/receipt required")
    require(json.loads((REPORT / "slot_bridge_G.json").read_text())["complete"], "Successful bridge memory test required")
    original_files = [p for package in list(MAPPING) + ["/ShooterCore/Input/Mappings/IMC_ShooterGame", "/ShooterCore/Game/AbilitySet_ShooterHero"]
                      for p in recipe.files(SOURCE, package)]
    protected = {str(p): recipe.digest(p) for p in original_files}
    protected.update(json.loads((REPORT / "sender_read_B.json").read_text())["protected"])
    for package in ACTION_MAP.values():
        if "Cycle" not in package:
            protected.update({str(p): recipe.digest(p) for project in (SOURCE, PROJECT)
                              for p in recipe.files(project, package)})
    recipe.unchanged(protected)
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    result = dict(mode=mode, mapping=MAPPING, action_map=ACTION_MAP, packages=PACKAGES,
                  protected=protected, complete=False, asset_saves=0)
    try:
        if mode == "prepare":
            require(hasattr(ue.DCRifleMigrationLibrary, "retarget_quick_slot_input_actions"), "User build with fixed Action-reference bridge required")
            require(all(not p.exists() for package in PACKAGES for root in (SOURCE, PROJECT)
                        for p in recipe.files(root, package)), "New-only preparation destinations required")
            target, memory = probe.main()
            result["memory_probe"] = "slot_" + run + ".json"
            originals = {p: ue.load_asset(p) for p in MAPPING}
            require(all(originals.values()), "Original GA/actions missing")
            source_ga = recipe.snapshot(originals[GA])
            result["source_ga"] = source_ga
            result["ga_policy"] = policy(originals[GA])
            copies = {p: assets.duplicate_loaded_asset(obj, MAPPING[p]) for p, obj in originals.items()}
            require(all(copies.values()), "Scoped duplicate failed")
            require(policy(copies[GA]) == result["ga_policy"], "Copied activation policy mismatch")
            actions = {p: ue.load_asset(d) for p, d in ACTION_MAP.items()}
            require(all(actions.values()), "Approved action identity missing")
            detail = ue.DCRifleMigrationLibrary.retarget_quick_slot_input_actions(target)
            result["action_retarget_report"] = detail
            require(isinstance(detail, str) and detail.startswith("Retargeted five"), "Action identity remap failed: " + repr(detail))
            require(ue.BlueprintEditorLibrary.compile_blueprint(target), "Prepared sender Compile failed")
            require(ue.BlueprintEditorLibrary.compile_blueprint(copies[GA]), "Copied GA Compile failed")
            ue.collect_garbage()
            recipe.helper.clean()
            expected = recipe.comparable(memory["expected"], ACTION_MAP)
            actual = [g for g in recipe.snapshot(target)["graphs"] if g["name"] in ("QuickSlotInput", "ChangeQuickbarSlot")]
            result.update(sender_expected=expected, sender_actual=actual)
            require(actual == expected, "Prepared sender graph differs from mapped original")
            require(recipe.snapshot(copies[GA]) == recipe.comparable(source_ga, MAPPING), "GA graph differs from original")
            refs = ue.DCRifleMigrationLibrary.inspect_blueprint_type_references(target)
            require(refs.succeeded and not any(probe.HERO in str(r) for r in refs.references), "Sender retains original Hero reference")
            recipe.unchanged(protected)
            for obj in list(copies.values()) + [target]:
                recipe.helper.clean()
                require(assets.save_loaded_asset(obj, False), "New asset save failed")
                result["asset_saves"] += 1
            result["saved_hashes"] = {str(p): recipe.digest(p) for package in PACKAGES for p in recipe.files(SOURCE, package)}
        else:
            require(baseline_run, "Explicit successful -DCR7SlotBaseline required")
            baseline = json.loads((REPORT / ("slot_assets_" + baseline_run.group(1) + ".json")).read_text())
            require(baseline["complete"] and baseline["mode"] == "prepare", "Complete prepare receipt required")
            recipe.unchanged(baseline["saved_hashes"])
            loaded = {p: ue.load_asset(p) for p in PACKAGES}
            require(all(loaded.values()), "Prepared asset missing")
            for p in (probe.STAGING, MAPPING[GA]):
                require(ue.BlueprintEditorLibrary.compile_blueprint(loaded[p]), "Reload Compile failed")
            recipe.helper.clean()
            require(policy(loaded[MAPPING[GA]]) == baseline["ga_policy"], "Reload GA policy mismatch")
            require(recipe.snapshot(loaded[MAPPING[GA]]) == recipe.comparable(baseline["source_ga"], MAPPING), "Reload GA graph mismatch")
            actual = [g for g in recipe.snapshot(loaded[probe.STAGING])["graphs"] if g["name"] in ("QuickSlotInput", "ChangeQuickbarSlot")]
            require(actual == baseline["sender_expected"], "Reload sender mismatch")
            context = ue.load_asset("/ShooterCore/Input/Mappings/IMC_ShooterGame")
            require(context, "Original mapping context missing")
            result["input_rows"] = []
            for row in context.get_editor_property("DefaultKeyMappings").get_editor_property("Mappings"):
                action = row.get_editor_property("Action")
                if not action or action.get_path_name().split(".")[0] not in ACTION_MAP: continue
                result["input_rows"].append(dict(text=row.export_text(), triggers=[dict(
                    path=t.get_path_name(), cls=t.get_class().get_path_name(),
                    threshold=t.get_editor_property("ActuationThreshold"),
                    always_tick=t.get_editor_property("bShouldAlwaysTick")) for t in row.get_editor_property("Triggers")]))
            require(len(result["input_rows"]) == 7, "Original QuickSlot mapping count changed")
            ability_set = ue.load_asset("/ShooterCore/Game/AbilitySet_ShooterHero")
            entries = [entry.export_text() for entry in ability_set.get_editor_property("GrantedGameplayAbilities")
                       if GA + ".GA_QuickbarSlots_C" in entry.export_text()]
            require(len(entries) == 1, "Expected one original QuickSlot ability grant")
            result["ability_entry"] = entries[0]
            registry = ue.AssetRegistryHelpers.get_asset_registry()
            result["dependencies"] = {p: [str(d) for d in registry.get_dependencies(p, ue.AssetRegistryDependencyOptions())] for p in PACKAGES}
            allowed = set(PACKAGES) | set(ACTION_MAP.values())
            require(all(d in allowed or d.startswith(("/Script/", "/Engine/")) for ds in result["dependencies"].values() for d in ds), "Unapproved content dependency")
            if mode == "migrate":
                require(ue.SystemLibrary.get_console_variable_int_value("AssetTools.UseNewPackageMigration") == 0, "Use isolated legacy engine migration")
                require(all(not p.exists() for package in PACKAGES for p in recipe.files(PROJECT, package)), "Target overwrite refused")
                options = ue.MigrationOptions()
                options.set_editor_property("prompt", False)
                options.set_editor_property("ignore_dependencies", True)
                options.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
                ue.AssetToolsHelpers.get_asset_tools().migrate_packages(PACKAGES, str(PROJECT / "Content"), options)
                require(all(recipe.files(PROJECT, p)[0].is_file() for p in PACKAGES), "Migration incomplete")
                result["migrated"] = len(PACKAGES)
            recipe.unchanged(baseline["saved_hashes"])
        recipe.unchanged(protected)
        recipe.helper.clean()
        result["complete"] = True
        print("DC_R7_SLOT_ASSETS " + json.dumps(dict(mode=mode, complete=True, asset_saves=result["asset_saves"])))
    finally:
        result["protected_unchanged"] = all(recipe.digest(Path(p)) == h for p, h in protected.items())
        receipt.write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
