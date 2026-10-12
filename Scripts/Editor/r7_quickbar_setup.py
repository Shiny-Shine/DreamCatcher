"""Scoped QuickBar asset recipe; no runtime C++/Config edits or existing asset writes.

Source commandlets: -DCR7QuickMode=memory|prepare|migrate -DCR7QuickRun=<fresh>.
Uses the engine's memory Advanced Copy only after checking its complete 66-package
dependency closure. Saves ONLY new copies after original/graph/diagnostic guards.
"""
import importlib.util
import json
import os
from pathlib import Path
import re
import sys
import unreal as ue

sys.dont_write_bytecode = True

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / "LyraStarterGame"
ROOT = "/Game/LyraMigration/UI/R7/Equipment"
SEED = "/ShooterCore/UserInterface/HUD/W_QuickBar"
WIDGETS = (SEED, SEED + "Slot", "/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName")
REPORT = PROJECT / "Saved/Diagnostics/R7QuickBar"
helper_spec = importlib.util.spec_from_file_location("r7_root_recipe", PROJECT / "Scripts/Editor/r7_root_ui_setup.py")
helper = importlib.util.module_from_spec(helper_spec)
helper_spec.loader.exec_module(helper)
require, digest, snapshot = helper.require, helper.digest, helper.snapshot


def emit(event, **values):
    print("DC_R7_QUICK " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def files(project, package):
    require(package.startswith(("/Game/", "/ShooterCore/")) and ".." not in package, "Unexpected package")
    base = project / ("Content" if package.startswith("/Game/") else "Plugins/GameFeatures/ShooterCore/Content")
    relative = package.split("/", 2)[2]
    return [(base / relative).with_suffix(ext) for ext in (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl")]


def closure(seed, query):
    pending, visited = [seed], {}
    while pending:
        p = pending.pop()
        if p in visited or p.startswith(("/Engine/", "/Script/")):
            continue
        require(p.startswith(("/Game/", "/ShooterCore/")), "Unexpected non-engine mount: " + p)
        require(len(visited) < 66, "Closure exceeds the approved candidate bundle")
        visited[p] = sorted(str(d) for d in query(p))
        pending.extend(visited[p])
    require(len(visited) == 66, "Source closure changed; review before copy")
    return visited


def destination(package):
    # Matches UE5.8 GenerateAdvancedCopyDestinations for this single seed only.
    parent = os.path.commonprefix((SEED + "/", package)).rsplit("/", 1)[0]
    if parent:
        return ROOT + package[len(parent):]
    return ROOT + "/LyraStarterGame/" + package[6:] if package.startswith("/Game/") else ROOT + package


def dirty():
    return {p.get_path_name() for p in list(ue.EditorLoadingAndSavingUtils.get_dirty_content_packages())
            + list(ue.EditorLoadingAndSavingUtils.get_dirty_map_packages())}


def comparable(value, mapping):
    text = json.dumps(value, sort_keys=True)
    for old, new in sorted(mapping.items(), key=lambda item: -len(item[0])):
        text = text.replace(old, new)
    return json.loads(text)


def unchanged(before):
    require(all(digest(Path(p)) == h for p, h in before.items()), "Protected file changed")


def remap_reload_literal(bp, old_object_path, new_object_path):
    # UE Advanced Copy leaves this soft literal on the InjectInput node unchanged.
    nodes = [n for n in ue.ObjectIterator(ue.K2Node)
             if n.get_outer().get_outer() == bp and n.get_name() == "K2Node_CallFunction_7"]
    require(len(nodes) == 1 and ue.BlueprintEditorLibrary.get_node_title(nodes[0]) == "InjectInputVectorForAction", "Unexpected reload graph")
    pins = [p for p in ue.BlueprintEditorLibrary.list_all_pins(nodes[0]) if str(p.get_pin_name()) == "Action"]
    require(len(pins) == 1 and pins[0].get_pin_value() in (old_object_path, new_object_path), "Unexpected Reload Action literal")
    if pins[0].get_pin_value() != new_object_path:
        require(pins[0].set_pin_value(new_object_path), "Reload Action reference remap failed")
        ue.BlueprintEditorLibrary.compile_blueprint(bp)


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR7QuickMode=(memory|prepare|migrate)(?:\s|$)", cmd)
    run = re.search(r"-DCR7QuickRun=([A-Za-z0-9_]{1,40})(?:\s|$)", cmd)
    require(mode and run and "-run=pythonscript" in cmd.lower(), "Explicit source commandlet required")
    mode, run = mode.group(1), run.group(1)
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == SOURCE.resolve(), "Wrong source project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0]), "Context refused")
    helper.LOG = PROJECT / "Saved/Logs" / ("R7Quick-" + run + ".log")
    require(helper.LOG.is_file(), "Fresh matching log required")
    REPORT.mkdir(parents=True, exist_ok=True)
    receipt = REPORT / (run + ".json")
    require(not receipt.exists(), "Fresh receipt required")
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = ue.AssetRegistryDependencyOptions()
    current = closure(SEED, lambda p: registry.get_dependencies(p, options))
    audit = json.loads((PROJECT / "Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/source.json").read_text(encoding="utf-8-sig"))
    cached = closure(SEED, lambda p: audit["dependencies"][p])
    require(current == cached, "Dependency metadata changed since proposal")
    mapping = {p: destination(p) for p in sorted(current)}
    require(len(set(mapping.values())) == 66 and all(p.startswith(ROOT + "/") for p in mapping.values()), "Destination collision/escape")
    protected_paths = [p for src in current for p in files(SOURCE, src)]
    # Keep original native APIs, input identity, and the existing active path intact.
    protected_paths += [PROJECT / p for p in (
        "Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultGameplayTags.ini", "DreamCatcher.uproject", ".git/index",
        "Binaries/Win64/UnrealEditor-DreamCatcher.dll", "Source/DreamCatcher/DreamCatcherPlayerController.h",
        "Source/DreamCatcher/Development/DCRifleIntegrationActors.cpp", "Source/DreamCatcher/Development/DCRifleIntegrationActors.h",
        "Content/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleController.uasset",
        "Content/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput.uasset",
        "Content/DreamCatcher/GAS/Test/R5/Integration/L_DC_RifleIntegration.umap",
        "Content/LyraMigration/UI/R7/W_WeaponReticleHost.uasset", "Content/LyraMigration/BaseInput/Actions/IA_Weapon_Reload.uasset")]
    before = {str(p): digest(p) for p in protected_paths}
    expected = {str(Path(p)): h for p, h in audit["source_file_states"].items()}
    require(all(before[str(p)] == expected[str(p)] for p in protected_paths[:330]), "Source bytes changed since approved audit")
    helper.clean()
    editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    result = dict(mode=mode, mapping=mapping, protected=before)
    try:
        if mode == "migrate":
            prepared = json.loads((REPORT / "prepare_A.json").read_text(encoding="utf-8"))
            require(prepared["mapping"] == mapping, "Different prepared manifest")
            require(ue.SystemLibrary.get_console_variable_int_value("AssetTools.UseNewPackageMigration") == 0, "Use isolated engine legacy migration")
            require(all(not p.exists() for dest in mapping.values() for p in files(PROJECT, dest)), "Existing target refused")
            require(all(files(SOURCE, d)[0].is_file() for d in mapping.values()), "Prepared source incomplete")
            opts = ue.MigrationOptions()
            opts.set_editor_property("prompt", False)
            opts.set_editor_property("ignore_dependencies", True)
            opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping.values()), str(PROJECT / "Content"), opts)
            require(all(files(PROJECT, d)[0].is_file() for d in mapping.values()), "Target migration incomplete")
            result["migrated"] = 66
        else:
            require(all(not p.exists() for project in (SOURCE, PROJECT) for dest in mapping.values() for p in files(project, dest)), "New-only destination required")
            require(not editor.does_directory_exist(ROOT), "Existing source destination refused")
            originals = {p: ue.load_asset(p) for p in mapping}
            require(all(originals.values()), "Source asset load failed")
            # Loaded-node enumeration otherwise sees unlinked compiler intermediates.
            # Strong Python references retain the real assets through engine GC.
            ue.collect_garbage()
            blueprint_originals = {p: bp for p, bp in originals.items() if isinstance(bp, ue.Blueprint)}
            source_snapshots = {p: snapshot(bp) for p, bp in blueprint_originals.items()}
            source_states, source_types = {}, {}
            for p, bp in blueprint_originals.items():
                state = ue.DCRifleMigrationLibrary.inspect_blueprint_type_references(bp)
                require(state.succeeded and state.blueprint_state, "Original type read failed")
                source_states[p] = state.blueprint_state
                source_types[p] = list(state.references)
            require(not dirty(), "Source load dirty; AdvancedCopy save preamble refused")
            settings = ue.get_default_object(ue.load_class(None, "/Script/AssetTools.AssetToolsSettings"))
            require(not settings.get_editor_property("AdvancedCopyCustomizations"), "Custom copy policy requires review")
            require(ue.SystemLibrary.get_console_variable_int_value("AssetTools.UseHeaderPatchingAdvancedCopy") == 0, "Header patching forbidden")
            helper.clean()
            completed, held = [], {}
            def on_copied(success, records):
                completed.append(bool(success))
                for record in records:
                    obj = record.asset
                    if obj:
                        path = ue.Object.get_path_name(obj)
                        package = path.split(".")[0]
                        if path == package + "." + package.rsplit("/", 1)[1]:
                            require(package in mapping.values(), "Copy callback escaped exact manifest")
                            held[package] = obj
            callback = ue.AdvancedCopyCompletedEvent()
            callback.bind_callable(on_copied)
            ue.AssetToolsHelpers.get_asset_tools().begin_advanced_copy_packages([SEED], ROOT, callback)
            require(completed == [True] and set(held) == set(mapping.values()), "Copy callback/manifest mismatch")
            remap_reload_literal(held[mapping[WIDGETS[2]]], "/Game/Input/Actions/IA_Weapon_Reload.IA_Weapon_Reload",
                                 mapping["/Game/Input/Actions/IA_Weapon_Reload"] + ".IA_Weapon_Reload")
            ue.collect_garbage()
            helper.clean()
            require(dirty().issubset(set(mapping.values())), "Original/unapproved package became dirty")
            copy_differences = {}
            for p, bp in blueprint_originals.items():
                current_snapshot = snapshot(bp)
                if current_snapshot != source_snapshots[p]:
                    (REPORT / (run + "_original_difference.json")).write_text(json.dumps(dict(
                        package=p, before=source_snapshots[p], after=current_snapshot,
                        before_state=source_states[p],
                        after_state=ue.DCRifleMigrationLibrary.inspect_blueprint_type_references(bp).blueprint_state),
                        ensure_ascii=False, indent=2), encoding="utf-8")
                require(current_snapshot == source_snapshots[p], "Original Blueprint structure changed: " + p)
                state = ue.DCRifleMigrationLibrary.inspect_blueprint_type_references(bp)
                require(state.succeeded and state.blueprint_state == source_states[p]
                        and list(state.references) == source_types[p], "Original Blueprint state or Graph.Nodes types changed")
                copy = held[mapping[p]]
                expected_copy, actual_copy = comparable(source_snapshots[p], mapping), snapshot(copy)
                if expected_copy != actual_copy:
                    copy_differences[p] = dict(before=expected_copy, after=actual_copy)
            if copy_differences:
                (REPORT / (run + "_copy_difference.json")).write_text(
                    json.dumps(copy_differences, ensure_ascii=False, indent=2), encoding="utf-8")
            require(not copy_differences, "Copied Blueprint graph/tree mismatch: " + repr(list(copy_differences)))
            unchanged(before)
            result["source_snapshots"] = source_snapshots
            result["asset_saves"] = 0
            if mode == "prepare":
                for obj in held.values():
                    helper.clean()
                    require(editor.save_loaded_asset(obj, False), "New copied asset save failed")
                result["asset_saves"] = len(held)
            emit("memory_verified", packages=len(held), blueprints=len(blueprint_originals), saved=result["asset_saves"])
        helper.clean()
        unchanged(before)
        receipt.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        emit("complete", mode=mode, report=str(receipt), protected_unchanged=True)
    finally:
        unchanged(before)


if __name__ == "__main__":
    main()
