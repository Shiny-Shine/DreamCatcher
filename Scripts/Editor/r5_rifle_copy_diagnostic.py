"""Bounded source-project diagnostics for the previously approved Rifle staging.

No C++ build, original save, consolidation or migration. -DCRifleDiagnostic=duplicate
creates at most two new copies; verify only reloads them in a fresh process.
"""
import json
import os
from pathlib import Path
import re
import unreal

ROOT = "/Game/LyraMigration/Rifle/Diagnostics/SingleAsset"
SOURCES = ["/Game/Audio/Blueprints/B_MusicManagerComponent_Base", "/Game/Audio/Blueprints/WeaponAudioMacros"]

def report(event, **values):
    print("DC_RIFLE_DIAGNOSTIC " + json.dumps({"event": event, **values}, ensure_ascii=False))

def require(ok, reason):
    if not ok:
        raise RuntimeError(reason)

def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate source-project commandlet required")
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    actual = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(expected)) == os.path.normcase(str(actual)), "Wrong project")
    match = re.search(r"(?:^|\s)-DCRifleDiagnostic=(\S+)", command)
    mode = match.group(1) if match else "verify"
    require(mode in ("duplicate", "verify"), "Unknown mode")
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    for source in SOURCES:
        name = source.rsplit("/", 1)[1]
        dest = ROOT + "/" + name
        if mode == "duplicate":
            require(not assets.does_asset_exist(dest), "Do not overwrite diagnostic copy: " + dest)
            report("before_source_load", asset=source)
            original = unreal.load_asset(source)
            require(original is not None, "Missing source")
            report("before_duplicate", source=source, dest=dest)
            copy = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, ROOT, original)
            require(copy is not None, "Duplicate failed")
            report("before_save", asset=dest)
            require(assets.save_loaded_asset(copy, False), "Save failed")
            report("saved", asset=dest)
        else:
            report("before_reload", asset=dest)
            copy = unreal.load_asset(dest)
            require(copy is not None, "Missing diagnostic copy")
            report("reloaded", asset=dest, parent=unreal.BlueprintEditorLibrary.get_blueprint_parent_class(copy).get_path_name())
    report("complete", mode=mode, original_writes=0, cpp_build=False, migration=False)

main()
