"""Approved R6 audio-only change. Does not change firing/reload/tag rules.

-DCR6AudioMode=inspect|prepare|verify -DCR6AudioRun=<fresh_name>
Matching log: Saved/Logs/R6RifleAudio-<fresh_name>.log.
Writes only a new MetaSound copy and the imported Fire Cue's one Sound pin.
"""
import hashlib
import json
from pathlib import Path
import re
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
CORE = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247"
SOURCE = "/Game/Audio/Sounds/Weapons/Rifle2/MSS_Weapons_Rifle2_Fire"
DEST = "/Game/DreamCatcher/GAS/Test/R5/Integration/Audio/MSS_DC_Rifle_Fire"
CUE = CORE + "/GCN_Weapon_Rifle_Fire"
PROTECTED = [SOURCE, CORE + "/GA_Weapon_Fire_Rifle_Auto", CORE + "/GA_Weapon_ReloadMagazine",
             CORE + "/GA_Weapon_Reload_Rifle", CORE + "/GA_Weapon_AutoReload",
             "/Game/LyraMigration/AbilitySystem/TagRelationships_ShooterHero",
             "/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RiflePawn"]
LOG = None


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def emit(event, **values):
    print("R6_AUDIO " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def fingerprint():
    files = [PROJECT / "Config/DefaultGame.ini", PROJECT / "Config/DefaultEngine.ini", PROJECT / ".git/index"]
    for package in PROTECTED:
        files.extend(PROJECT / "Content" / (package[6:] + ext) for ext in (".uasset", ".uexp", ".ubulk", ".uptnl"))
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None for p in files}


def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; refuse save")
    errors = [line[:1400] for line in LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
              if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine error; refuse save: " + repr(errors[:3]))


def success(result):
    return str(result).endswith("SUCCEEDED: 0>") or str(result).split(":")[0].endswith("SUCCEEDED")


def builder_for(asset):
    cls = ue.load_class(None, "/Script/MetasoundEditor.MetaSoundEditorSubsystem")
    require(cls is not None, "MetaSound Editor subsystem missing")
    builder, result = ue.get_editor_subsystem(cls).find_or_begin_building(asset)
    require(builder is not None and success(result), "Could not open MetaSound builder: " + str(result))
    return builder


def interval(builder):
    literal, result = builder.get_graph_input_default("ShotInterval")
    require(success(result), "ShotInterval read failed: " + str(result))
    text = literal.export_text()
    found = re.search(r"AsFloat=\(([-0-9.eE]+)\)", text)
    require(found is not None, "Expected float-backed Time literal: " + text)
    return float(found.group(1)), literal


def cue_snapshot(bp):
    graph_paths = {g.get_path_name() for g in ue.BlueprintEditorLibrary.list_graphs(bp)}
    records, sound_pins = {}, []
    for node in ue.ObjectIterator(ue.EdGraphNode):
        outer = node.get_outer()
        if not outer or outer.get_path_name() not in graph_paths or "K2Node" not in node.get_class().get_name():
            continue
        title = ue.BlueprintEditorLibrary.get_node_title(node)
        for pin in ue.BlueprintEditorLibrary.list_all_pins(node):
            key = outer.get_name() + "." + node.get_name() + "." + str(pin.get_pin_name())
            records[key] = (pin.get_pin_value(), sorted((p.get_owning_node().get_name(), str(p.get_pin_name())) for p in pin.list_connected_pins()))
            if outer.get_name() == "OnBurst" and title == "TriggerFireAudio" and str(pin.get_pin_name()) == "Sound":
                sound_pins.append((key, pin))
    require(len(sound_pins) == 1, "Expected exactly one TriggerFireAudio Sound pin")
    return records, sound_pins[0]


def export_audio(obj, run, suffix):
    path = PROJECT / "Saved/Diagnostics/R6ReloadAudio" / (run + "-" + suffix + ".t3d")
    require(not path.exists(), "Fresh diagnostic export required")
    task = ue.AssetExportTask()
    task.object = obj
    task.exporter = ue.ObjectExporterT3D()
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    task.replace_identical = False
    require(ue.Exporter.run_asset_export_task(task), "MetaSound text export failed")
    return str(path)


def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR6AudioMode=(inspect|prepare|verify)(?:\s|$)", command)
    run = re.search(r"-DCR6AudioRun=([A-Za-z0-9_]{1,40})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit isolated commandlet required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    LOG = PROJECT / "Saved/Logs" / ("R6RifleAudio-" + run.group(1) + ".log")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context guard rejected")
    ue.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
    before = fingerprint()
    try:
        source = ue.load_asset(SOURCE)
        fire_bp = ue.load_asset(CORE + "/GA_Weapon_Fire_Rifle_Auto")
        require(source is not None and fire_bp is not None, "Required source missing")
        fire_interval = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(fire_bp)).get_editor_property("FireDelayTimeSecs")
        old_interval, literal = interval(builder_for(source))
        require(abs(old_interval - 0.15) < 0.0001 and abs(fire_interval - 0.12) < 0.0001, "Defaults changed; inspect user changes before editing")
        cue = ue.load_asset(CUE)
        require(cue is not None, "Fire Cue missing")
        graph_before, (key, pin) = cue_snapshot(cue)
        emit("inspected", source_interval=old_interval, fire_interval=fire_interval, sound_pin=pin.get_pin_value())
        if mode.group(1) == "inspect":
            clean()
            return
        assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        if mode.group(1) == "prepare":
            require(not assets.does_asset_exist(DEST), "New audio copy only; refusing overwrite")
            require(pin.get_pin_value() == source.get_path_name(), "Unexpected Cue Sound; refusing overwrite")
            copy = assets.duplicate_asset(SOURCE, DEST)
            require(copy is not None, "MetaSound duplication failed")
            builder = builder_for(copy)
            require(literal.import_text("(Type=Float,AsFloat=(0.120000))"), "Time literal creation failed")
            result = builder.set_graph_input_default("ShotInterval", literal)
            require(success(result), "ShotInterval edit failed: " + str(result))
            require(abs(interval(builder)[0] - fire_interval) < 0.0001, "Audio interval did not match firing")
            clean()
            require(assets.save_loaded_asset(copy, False), "New audio save failed")
            require(pin.set_pin_value(copy.get_path_name()), "Cue Sound pin update failed")
            graph_after, _ = cue_snapshot(cue)
            changed = [k for k in graph_before if graph_before[k] != graph_after.get(k)]
            require(changed == [key] and graph_after.keys() == graph_before.keys(), "Unapproved Cue graph change")
            require(ue.BlueprintEditorLibrary.compile_blueprint(cue), "Fire Cue Compile failed")
            clean()
            require(assets.save_loaded_asset(cue, False), "Fire Cue save failed")
            emit("prepared", sound=DEST, interval=interval(builder)[0], changed_cue_pin=key, asset_saves=2)
        else:
            copy = ue.load_asset(DEST)
            require(copy is not None, "Audio copy not saved")
            require(abs(interval(builder_for(copy))[0] - fire_interval) < 0.0001, "Reloaded interval mismatch")
            require(pin.get_pin_value() == copy.get_path_name(), "Reloaded Cue Sound mismatch")
            clean()
            emit("verified", interval=interval(builder_for(copy))[0], asset_saves=0, heard_output_verified=False,
                 source_export=export_audio(source, run.group(1), "source"), copy_export=export_audio(copy, run.group(1), "copy"))
    finally:
        require(fingerprint() == before, "Protected original/reload/rules/Config/index changed")
        emit("protected_unchanged", file_states=len(before))


if __name__ == "__main__":
    main()
