"""Read-only post-build native/Spatialization check. Never saves or compiles assets."""
import hashlib
import json
from pathlib import Path
import re

import unreal


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_R5_UI_DEPENDENCY " + json.dumps({"event": event, **values}))


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    for match in re.finditer(r"-EnablePlugins=([^\s]+)", command, re.IGNORECASE):
        require("spatialization" not in match.group(1).lower(), "Project registration must not be masked by an override")
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(project == Path(__file__).resolve().parents[2], "DreamCatcher project only")
    require(project.name == "DreamCatcher", "Unexpected project")
    package = "/Game/Audio/AttenuationPresets/TempITDSpatializationSourceSettings"
    base = project / "Content/Audio/AttenuationPresets/TempITDSpatializationSourceSettings"

    def snapshot():
        result = {}
        for suffix in (".uasset", ".uexp", ".ubulk", ".uptnl"):
            path = base.with_suffix(suffix)
            require(suffix != ".uasset" or path.is_file(), "Expected existing settings asset")
            result[str(path)] = None
            if path.is_file():
                with path.open("rb") as stream:
                    result[str(path)] = hashlib.file_digest(stream, "sha256").hexdigest()
        return result

    before = snapshot()
    bridge = unreal.DCRifleMigrationLibrary
    require(not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(), "Unexpected initial dirty content")
    emit("begin", package=package, cpp_build=False, packaging=False)
    try:
        for native in ("/Script/DreamCatcher.DCLyraCircumferenceMarkerWidget",
                       "/Script/Spatialization.ITDSpatializationSourceSettings"):
            cls = unreal.load_class(None, native)
            require(cls is not None and cls.get_path_name() == native, "Native class unavailable: " + native)
            emit("native_loaded", path=cls.get_path_name())
        entry = unreal.load_object(None, "/Script/DreamCatcher.DCLyraCircumferenceMarkerEntry")
        require(entry is not None, "Marker entry struct unavailable")
        asset = unreal.load_asset(package)
        require(asset is not None, "Settings asset failed to load")
        require(asset.get_class().get_path_name() == "/Script/Spatialization.ITDSpatializationSourceSettings",
                "Unexpected asset class")
        emit("asset_loaded", path=asset.get_path_name(), asset_class=asset.get_class().get_path_name())
        require(bridge.get_engine_ensure_failure_count() == 0, "Engine ensure encountered")
        require(not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(), "Load dirtied content; do not save")
    finally:
        require(snapshot() == before, "Asset/companion file state changed")
        emit("file_states_unchanged", count=len(before))
    emit("complete", asset_writes=0, explicit_compile=False, ensure_failures=0, dirty_packages=[])


main()
