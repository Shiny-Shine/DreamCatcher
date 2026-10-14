"""R7 QuickSlot bridge acceptance probe, run AFTER the user's Editor build.

Source Lyra only; -DCRifleCopyDiagnostics -DCR7QuickSlotTransfer
-DCR7SlotRun=<fresh_name>. Uses an unsaved staging Pawn. Never saves assets.
"""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unreal as ue
import r7_root_ui_setup as helper

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / "LyraStarterGame"
REPORT = PROJECT / "Saved/Diagnostics/R7QuickBar"
HERO = "/ShooterCore/Game/B_Hero_ShooterMannequin"
STAGING = "/Game/LyraMigration/UI/R7/Equipment/Diagnostics/BP_QuickSlotInputSource"
KEEP = {"K2Node_EnhancedInputAction_" + str(i) for i in range(1, 6)} | {
    "K2Node_CallFunction_19", "K2Node_CallFunction_25", "K2Node_CallFunction_26",
    "K2Node_CallFunction_39", "K2Node_CallFunction_41"}
require = helper.require


def live_snapshot(blueprint, raw):
    """Exclude compiler leftovers only when absent from actual Graph.Nodes."""
    live = {}
    for graph in ue.BlueprintEditorLibrary.list_graphs(blueprint):
        editor = ue.BlueprintGraphEditor.get_graph_editor(graph)
        live[graph.get_name()] = {n.get_name() for n in list(editor.list_all_nodes()) + list(editor.list_comment_nodes())}
    result = json.loads(json.dumps(raw))
    excluded = []
    for graph in result["graphs"]:
        require(graph["name"] in live, "Snapshot graph is not registered")
        for node in graph["nodes"]:
            if node["name"] not in live[graph["name"]]:
                require(not any(p["links"] for p in node.get("pins", [])), "Omitted node has live links; investigate")
                excluded.append(dict(graph=graph["name"], node=node["name"], cls=node["class"]))
        graph["nodes"] = [n for n in graph["nodes"] if n["name"] in live[graph["name"]]]
        require({n["name"] for n in graph["nodes"]} == live[graph["name"]], "Reader missed a registered node")
    return result, excluded


def main():
    command = ue.SystemLibrary.get_command_line()
    run = re.search(r"-DCR7SlotRun=([A-Za-z0-9_]{1,36})(?:\s|$)", command)
    require(run and "-run=pythonscript" in command.lower()
            and "-DCR7QuickSlotTransfer" in command, "Explicit isolated probe required")
    run = run.group(1)
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == SOURCE.resolve(), "Wrong project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(guard and (not isinstance(guard, tuple) or guard[0]), "Unsafe Editor context")
    require(hasattr(ue.DCRifleMigrationLibrary, "copy_quick_slot_input_graphs"), "User Editor build required; bridge is not loaded")
    helper.LOG = PROJECT / "Saved/Logs" / ("R7Slot-" + run + ".log")
    receipt = REPORT / ("slot_" + run + ".json")
    require(helper.LOG.is_file() and not receipt.exists(), "Fresh matching log/receipt required")
    source_audit = json.loads((REPORT / "sender_read_B.json").read_text(encoding="utf-8"))
    target_audit = json.loads((REPORT / "input_target_read_B.json").read_text(encoding="utf-8"))
    protected = dict(source_audit["protected"])
    # Preserve current target files, allowing the user's intervening DLL rebuild.
    protected.update({p: helper.digest(Path(p)) for p in target_audit["protected"]})
    protected.update({str(PROJECT / p): helper.digest(PROJECT / p) for p in (
        "Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultGameplayTags.ini",
        ".git/index", "Binaries/Win64/UnrealEditor-DreamCatcher.dll")})
    staging_files = [(SOURCE / "Content" / STAGING[6:]).with_suffix(ext)
                     for ext in (".uasset", ".uexp", ".ubulk", ".uptnl")]
    require(all(not p.exists() for p in staging_files), "Probe must not touch existing staging assets")
    result = dict(mode="memory_probe", asset_saves=0, protected=protected, complete=False)
    try:
        require(all(helper.digest(Path(p)) == h for p, h in protected.items()), "Protected source changed")
        original = ue.load_asset(HERO)
        require(original, "Original Hero missing")
        loaded = helper.snapshot(original)
        require(loaded == source_audit["inspected"][HERO]["snapshot"], "Original graph changed since approval")
        before, excluded = live_snapshot(original, loaded)
        result["non_graph_compiler_objects_before"] = excluded
        helper.clean()
        assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        require(not assets.does_asset_exist(STAGING), "Staging destination already exists")
        target = ue.BlueprintEditorLibrary.create_blueprint_asset_with_parent(STAGING, ue.Pawn)
        require(target, "New staging Pawn failed")
        # Leave a pre-call record even if a native crash prevents Python finally.
        result["stage"] = "before_native_copy"
        receipt.write_text(json.dumps(result, indent=2), encoding="utf-8")
        # Unreal converts bool-success + one out parameter to str on success,
        # None on failure, not a (bool, str) tuple.
        detail = ue.DCRifleMigrationLibrary.copy_quick_slot_input_graphs(original, target)
        result["bridge_report"] = detail
        require(isinstance(detail, str) and detail.startswith("Copied original"), "Bridge rejected: " + repr(detail))
        # CloneGraph is also used for compiler intermediates and can generate
        # object names. Identify our two exact node sets, then restore the public
        # function/graph names using the engine's normal graph registration path.
        original_function = next(g for g in before["graphs"] if g["name"] == "ChangeQuickbarSlot")
        function_nodes = {n["name"] for n in original_function["nodes"]}
        copied = {}
        result["post_copy_graphs"] = []
        for graph in ue.BlueprintEditorLibrary.list_graphs(target):
            names = {n.get_name() for n in ue.BlueprintGraphEditor.get_graph_editor(graph).list_all_nodes()}
            result["post_copy_graphs"].append(dict(name=graph.get_name(), nodes=sorted(names)))
            if names == function_nodes: copied["ChangeQuickbarSlot"] = graph
            if names == KEEP: copied["QuickSlotInput"] = graph
        require(set(copied) == {"ChangeQuickbarSlot", "QuickSlotInput"}, "Copied graph node sets missing")
        for name, graph in copied.items():
            ue.BlueprintEditorLibrary.rename_graph(graph, name)
        result["finalization"] = "restore original function/input graph names and engine registration"
        result["stage"] = "before_destination_compile"
        receipt.write_text(json.dumps(result, indent=2), encoding="utf-8")
        require(ue.BlueprintEditorLibrary.compile_blueprint(target), "Copied graph Blueprint Compile failed")
        ue.collect_garbage()
        helper.clean()
        wanted = [g for g in before["graphs"] if g["name"] == "ChangeQuickbarSlot"]
        event = next(g for g in before["graphs"] if g["name"] == "EventGraph")
        wanted.append(dict(name="QuickSlotInput", nodes=[n for n in event["nodes"] if n["name"] in KEEP]))
        result["expected"] = sorted(wanted, key=lambda g: g["name"])
        result["actual"] = [g for g in helper.snapshot(target)["graphs"]
                            if g["name"] in ("QuickSlotInput", "ChangeQuickbarSlot")]
        require(result["expected"] == result["actual"], "Original node/pin/type/default comparison failed")
        # The exact same target must reject a second call rather than duplicate/overwrite graphs.
        before_retry = helper.snapshot(target)
        retry_result = ue.DCRifleMigrationLibrary.copy_quick_slot_input_graphs(original, target)
        result["retry_result"] = retry_result
        require(retry_result is None and helper.snapshot(target) == before_retry, "Duplicate transfer was not rejected without mutation")
        result["original_before"] = before
        result["original_after"], result["non_graph_compiler_objects_after"] = live_snapshot(original, helper.snapshot(original))
        require(result["original_after"] == before, "Original Blueprint changed")
        helper.clean()
        require(all(helper.digest(Path(p)) == h for p, h in protected.items()), "Protected file changed")
        require(all(not p.exists() for p in staging_files), "Memory probe unexpectedly wrote an asset")
        result["complete"] = True
        result["stage"] = "memory_probe_complete"
        print("DC_R7_SLOTS memory_complete=true original_graph_equal=true retry_rejected=true asset_saves=0")
    finally:
        result["protected_unchanged"] = all(helper.digest(Path(p)) == h for p, h in protected.items())
        result["staging_files_absent"] = all(not p.exists() for p in staging_files)
        receipt.write_text(json.dumps(result, indent=2), encoding="utf-8")
    return target, result


if __name__ == "__main__":
    main()
