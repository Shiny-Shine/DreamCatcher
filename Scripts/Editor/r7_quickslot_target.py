"""Approved QuickSlot integration only. No native build or final HUD replacement.

-DCR7SlotTarget=prepare|preview|apply|verify -DCR7SlotRun=<fresh>
prepare saves only two newly migrated BPs. preview never saves. apply saves only
the integration Pawn/PawnData and new IMC/AbilitySet after a successful preview.
"""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unreal as ue
import r7_quickslot_assets as source
from r7_quickbar_target import normalize as normalize_ui
from r7_quickslot_transfer_probe import live_snapshot, KEEP

PROJECT, REPORT, STAGING = source.PROJECT, source.REPORT, source.probe.STAGING
ROOT = "/Game/DreamCatcher/GAS/Test/R5/Integration"
PAWN, DATA = ROOT + "/BP_DC_RiflePawn", ROOT + "/DA_DC_RiflePawn"
CONTEXT, ABILITIES = ROOT + "/IMC_DC_QuickSlots", ROOT + "/AS_DC_QuickSlots"
PREVIEW = "/Game/LyraMigration/UI/R7/Equipment/Diagnostics/BP_DC_RiflePawn_QuickSlotPreview"
LEGACY = {"K2Node_InputKey_4": "1", "K2Node_InputKey_1": "2", "K2Node_InputKey_2": "3"}
require, snapshot, digest = source.require, source.recipe.snapshot, source.recipe.digest


def normalize(value, mapping):
    # Same native class redirect already in DefaultEngine, including K2 display spelling.
    return json.loads(json.dumps(normalize_ui(value, mapping)).replace("Lyra Gameplay Ability", "DCGameplay Ability"))


def load(path):
    obj = ue.load_asset(path)
    require(obj, "Missing asset " + path)
    return obj


def hero(bp):
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    found = []
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = ue.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = ue.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        if obj and ue.Object.get_class(obj).get_name() == "DCHeroComponent": found.append(obj)
    require(len(found) == 1, "Expected one editable Hero template")
    require(found[0].get_path_name().startswith(bp.get_path_name().split(".")[0] + "."), "Inherited/shared Hero template refused")
    return found[0]


def mapped_text(text):
    for old, new in dict(source.MAPPING, **source.ACTION_MAP).items(): text = text.replace(old, new)
    return text


def create_data(path, class_path):
    cls = ue.load_class(None, class_path)
    factory = ue.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", cls)
    obj = ue.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit("/", 1)[1], path.rsplit("/", 1)[0], cls, factory)
    require(obj, "Data asset creation failed")
    return obj


def create_input_data(original):
    context = create_data(CONTEXT, "/Script/EnhancedInput.InputMappingContext")
    template = load(ROOT + "/IMC_DC_RifleReload").get_editor_property("DefaultKeyMappings").get_editor_property("Mappings")[0]
    rows = []
    for record in original["input_rows"]:
        text = mapped_text(record["text"])
        for trigger in record["triggers"]:
            require(trigger["cls"] == "/Script/EnhancedInput.InputTriggerPressed", "Unexpected trigger type")
            obj = ue.new_object(ue.load_class(None, trigger["cls"]), outer=context)
            obj.set_editor_property("ActuationThreshold", trigger["threshold"])
            require(obj.get_editor_property("bShouldAlwaysTick") == trigger["always_tick"], "Trigger tick mismatch")
            text = text.replace(trigger["path"], obj.get_path_name())
        row = type(template)()
        require(row.import_text(text), "Mapping row import failed")
        require(row.export_text() == text, "Mapping row values changed on import")
        rows.append(row)
    data = context.get_editor_property("DefaultKeyMappings")
    data.set_editor_property("Mappings", rows)
    context.set_editor_property("DefaultKeyMappings", data)
    abilities = create_data(ABILITIES, "/Script/DreamCatcher.DCAbilitySet")
    sample = load("/Game/DreamCatcher/GAS/Test/AS_DC_FoundationTest").get_editor_property("GrantedGameplayAbilities")[0]
    entry = type(sample)()
    require(entry.import_text(mapped_text(original["ability_entry"])), "Original grant entry import failed")
    require(entry.export_text() == mapped_text(original["ability_entry"]), "Original ability grant changed")
    abilities.set_editor_property("GrantedGameplayAbilities", [entry])
    return context, abilities


def copy_sender(donor, pawn, expected):
    detail = ue.DCRifleMigrationLibrary.copy_quick_slot_input_graphs(donor, pawn)
    require(isinstance(detail, str) and detail.startswith("Copied original"), "Graph bridge rejected: " + repr(detail))
    function_names = {n["name"] for g in expected if g["name"] == "ChangeQuickbarSlot" for n in g["nodes"]}
    matched = {}
    for graph in ue.BlueprintEditorLibrary.list_graphs(pawn):
        names = {n.get_name() for n in ue.BlueprintGraphEditor.get_graph_editor(graph).list_all_nodes()}
        if names == function_names: matched["ChangeQuickbarSlot"] = graph
        if names == KEEP: matched["QuickSlotInput"] = graph
    require(set(matched) == {"ChangeQuickbarSlot", "QuickSlotInput"}, "Copied graph selection ambiguous")
    for name, graph in matched.items(): ue.BlueprintEditorLibrary.rename_graph(graph, name)
    event = ue.BlueprintEditorLibrary.find_event_graph(pawn)
    editor = ue.BlueprintGraphEditor.get_graph_editor(event)
    keys = [n for n in editor.list_all_nodes() if n.get_name() in LEGACY]
    require(len(keys) == 3 and all(ue.Object.get_class(n).get_name() == "K2Node_InputKey"
            and ue.BlueprintEditorLibrary.get_node_title(n) == LEGACY[n.get_name()] for n in keys), "Legacy key set changed")
    editor.remove_nodes(keys)
    return detail


def compare_pawn(bp, before, expected, result=None):
    actual, _ = live_snapshot(bp, snapshot(bp))
    pairs = [g for g in actual["graphs"] if g["name"] in ("ChangeQuickbarSlot", "QuickSlotInput")]
    require(pairs == expected, "Input graphs differ from mapped original")
    old = json.loads(json.dumps(before["graphs"]))
    for graph in old:
        if graph["name"] == "EventGraph":
            graph["nodes"] = [n for n in graph["nodes"] if n["name"] not in LEGACY]
            for node in graph["nodes"]:
                for pin in node.get("pins", []):
                    pin["links"] = [link for link in pin["links"] if link["node"] not in LEGACY]
    remaining = [g for g in actual["graphs"] if g["name"] not in ("ChangeQuickbarSlot", "QuickSlotInput")]
    if result is not None:
        result["existing_graphs_expected"] = old
        result["existing_graphs_actual"] = remaining
    require(remaining == old,
            "Unapproved original graph change")
    return actual


def preview_comparable(value):
    # UE gives an unsaved duplicate its own localization namespace/key. Only
    # this existing debug format is allowed to differ in identity, never text.
    value = json.loads(json.dumps(value))
    for graph in value["graphs"]:
        if graph["name"] != "EventGraph": continue
        for node in graph["nodes"]:
            if node["name"] != "K2Node_FormatText_0": continue
            for pin in node.get("pins", []):
                if pin["name"] != "Format": continue
                match = re.fullmatch(r'NSLOCTEXT\("[^"]*", "[^"]*", "(.*)"\)', pin["value"])
                require(match and match.group(1) == "Heat: {Heat}, Spread: {Spread}", "Debug display text changed")
                pin["value"] = match.group(1)
    return value


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR7SlotTarget=(prepare|preview|apply|verify)(?:\s|$)", cmd)
    run = re.search(r"-DCR7SlotRun=([A-Za-z0-9_]{1,36})(?:\s|$)", cmd)
    require(mode and run and "-run=pythonscript" in cmd.lower(), "Explicit mode/run required")
    mode, run = mode.group(1), run.group(1)
    prior_run = re.search(r"-DCR7SlotTargetBaseline=([A-Za-z0-9_]{1,36})(?:\s|$)", cmd)
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve(), "Target project only")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(guard and (not isinstance(guard, tuple) or guard[0]), "Unsafe context")
    source.recipe.helper.LOG = PROJECT / "Saved/Logs" / ("R7Slot-" + run + ".log")
    clean = source.recipe.helper.clean
    receipt = REPORT / ("slot_target_" + run + ".json")
    require(source.recipe.helper.LOG.is_file() and not receipt.exists(), "Fresh log/receipt required")
    baseline = json.loads((REPORT / "slot_assets_prepare_B.json").read_text())
    migration = json.loads((REPORT / "slot_assets_migrate_C.json").read_text())
    original_input = json.loads((REPORT / "slot_assets_verify_D.json").read_text())
    require(all(r["complete"] for r in (baseline, migration, original_input)), "Source/migration evidence incomplete")
    source.recipe.unchanged(baseline["protected"])
    source.recipe.unchanged(baseline["saved_hashes"])
    old_target = json.loads((REPORT / "input_target_read_B.json").read_text())
    protected = {p: digest(Path(p)) for p in old_target["protected"]}
    protected.update({str(PROJECT / p): digest(PROJECT / p) for p in (
        "Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultGameplayTags.ini", ".git/index",
        "Binaries/Win64/UnrealEditor-DreamCatcher.dll", "Source/DreamCatcher/Development/DCRifleIntegrationActors.cpp")})
    if mode == "apply":
        for package in (PAWN, DATA):
            for p in source.recipe.files(PROJECT, package): protected.pop(str(p), None)
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    result = dict(mode=mode, protected=protected, complete=False, asset_saves=0)
    if mode == "verify":
        protected.update({str(p): digest(p) for package in source.PACKAGES + [PAWN, DATA, CONTEXT, ABILITIES]
                          for p in source.recipe.files(PROJECT, package)})
    try:
        loaded = {p: load(p) for p in source.PACKAGES}
        for p in source.ACTION_MAP.values(): load(p)
        ga = loaded[source.MAPPING[source.GA]]
        donor = loaded[STAGING]
        expected = normalize(baseline["sender_expected"], {})
        if mode == "prepare":
            for package in source.PACKAGES:
                require(digest(source.recipe.files(PROJECT, package)[0]) == digest(source.recipe.files(source.SOURCE, package)[0]), "Target changed since migration")
            require(isinstance(ue.DCRifleMigrationLibrary.retarget_quick_slot_input_actions(donor), str), "Migrated binding identity mismatch")
            for obj in (donor, ga): require(ue.BlueprintEditorLibrary.compile_blueprint(obj), "Migrated BP Compile failed")
            ue.collect_garbage()
            clean()
            require([g for g in snapshot(donor)["graphs"] if g["name"] in ("ChangeQuickbarSlot", "QuickSlotInput")] == expected, "Target sender mismatch")
            result["ga_expected"] = normalize(baseline["source_ga"], source.MAPPING)
            result["ga_actual"] = snapshot(ga)
            result["ga_policy_actual"] = source.policy(ga)
            require(result["ga_actual"] == result["ga_expected"], "Target GA graph mismatch")
            require({k: v.replace("<Lyra", "<DC") for k, v in baseline["ga_policy"].items()} == result["ga_policy_actual"], "Target GA policy mismatch")
            for obj in (donor, ga):
                require(assets.save_loaded_asset(obj, False), "New target BP save failed")
                result["asset_saves"] += 1
        elif mode in ("preview", "apply"):
            require(all(not source.recipe.files(PROJECT, p)[0].exists() for p in (CONTEXT, ABILITIES, PREVIEW)), "New setup destinations required")
            if mode == "apply":
                require(prior_run, "Explicit successful preview baseline required")
                prior = json.loads((REPORT / ("slot_target_" + prior_run.group(1) + ".json")).read_text())
                require(prior["complete"] and prior["mode"] == "preview" and prior["asset_saves"] == 0, "Successful memory preview required")
                source.recipe.unchanged(prior["protected"])
            original = load(PAWN)
            before, _ = live_snapshot(original, snapshot(original))
            pawn = assets.duplicate_loaded_asset(original, PREVIEW) if mode == "preview" else original
            require(pawn, "Pawn preview failed")
            if mode == "preview":
                duplicate_before, _ = live_snapshot(pawn, snapshot(pawn))
                require(preview_comparable(duplicate_before) == preview_comparable(before), "Preview duplication changed existing gameplay graphs")
                result["preview_only_localization_identity"] = duplicate_before != before
                before = duplicate_before
            result["before"] = before
            result["bridge_report"] = copy_sender(donor, pawn, expected)
            context, abilities = create_input_data(original_input)
            hero_obj = hero(pawn)
            mappings = list(hero_obj.get_editor_property("DefaultInputMappings"))
            result["mappings_before"] = [m.export_text() for m in mappings]
            entry = type(mappings[0])()
            require(entry.import_text('(InputMapping="' + context.get_path_name() + '",Priority=1,bRegisterWithSettings=True)'), "Hero mapping import failed")
            hero_obj.set_editor_property("DefaultInputMappings", mappings + [entry])
            require(ue.BlueprintEditorLibrary.compile_blueprint(pawn), "Integrated Pawn Compile failed")
            ue.collect_garbage()
            clean()
            result["after"] = compare_pawn(pawn, before, expected, result)
            require([m.export_text() for m in hero(pawn).get_editor_property("DefaultInputMappings")][:-1] == result["mappings_before"], "Existing mappings changed")
            data = load(DATA)
            sets = list(data.get_editor_property("AbilitySets"))
            result["sets_before"] = [s.get_path_name() for s in sets]
            require(len(sets) == 4 and abilities not in sets, "Ability grant list changed")
            if mode == "apply":
                data.set_editor_property("AbilitySets", sets + [abilities])
                for obj in (context, abilities, pawn, data):
                    clean()
                    require(assets.save_loaded_asset(obj, False), "Integration save failed")
                    result["asset_saves"] += 1
        else:
            require(prior_run, "Explicit successful apply baseline required")
            prior = json.loads((REPORT / ("slot_target_" + prior_run.group(1) + ".json")).read_text())
            require(prior["complete"] and prior["mode"] == "apply" and prior["asset_saves"] == 4, "Complete apply evidence required")
            pawn = load(PAWN)
            require(ue.BlueprintEditorLibrary.compile_blueprint(pawn), "Reload Pawn Compile failed")
            ue.collect_garbage()
            compare_pawn(pawn, prior["before"], expected, result)
            data, context, abilities = load(DATA), load(CONTEXT), load(ABILITIES)
            require([s.get_path_name() for s in data.get_editor_property("AbilitySets")] == prior["sets_before"] + [abilities.get_path_name()], "Grant list mismatch")
            rows = list(context.get_editor_property("DefaultKeyMappings").get_editor_property("Mappings"))
            require(len(rows) == 7, "Mapping count mismatch")
            for row, record in zip(rows, original_input["input_rows"]):
                text = mapped_text(record["text"])
                triggers = list(row.get_editor_property("Triggers"))
                require(len(triggers) == len(record["triggers"]), "Trigger count mismatch")
                for trigger, saved in zip(triggers, record["triggers"]):
                    require(trigger.get_outer() == context and trigger.get_class().get_path_name() == saved["cls"]
                            and trigger.get_editor_property("ActuationThreshold") == saved["threshold"]
                            and trigger.get_editor_property("bShouldAlwaysTick") == saved["always_tick"], "Original trigger settings changed")
                    text = text.replace(saved["path"], trigger.get_path_name())
                require(row.export_text() == text, "Original mapping row changed")
            require([e.export_text() for e in abilities.get_editor_property("GrantedGameplayAbilities")] == [mapped_text(original_input["ability_entry"])], "Original grant entry changed")
            contexts = list(hero(pawn).get_editor_property("DefaultInputMappings"))
            require([m.export_text() for m in contexts][:-1] == prior["mappings_before"], "Original mappings changed")
            require(contexts[-1].get_editor_property("InputMapping") == context
                    and contexts[-1].get_editor_property("Priority") == 1
                    and contexts[-1].get_editor_property("bRegisterWithSettings"), "QuickSlot context is not registered as approved")
        clean()
        source.recipe.unchanged(protected)
        source.recipe.unchanged(baseline["protected"])
        result["complete"] = True
        print("DC_R7_SLOT_TARGET " + json.dumps(dict(mode=mode, complete=True, asset_saves=result["asset_saves"])))
    finally:
        result["protected_unchanged"] = all(digest(Path(p)) == h for p, h in protected.items())
        receipt.write_text(json.dumps(result, indent=2), encoding="utf-8")


if __name__ == "__main__": main()
