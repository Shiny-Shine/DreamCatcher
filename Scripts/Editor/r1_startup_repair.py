"""R1 bounded GameData metadata resave / read-only verification. Never builds C++.

Separate commandlet with -DCRifleCopyDiagnostics -DCR1StartupMode=resave|verify
and -DCR1StartupRun=<fresh_name>, matching Saved/Logs/R1Startup-<fresh_name>.log.
The resave writes only /Game/DefaultGameData; gameplay-effect references must not change.
"""
import hashlib
import json
from pathlib import Path
import re
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
DATA = "/Game/DefaultGameData"
FIELDS = ("DamageGameplayEffect_SetByCaller", "HealGameplayEffect_SetByCaller", "DynamicTagGameplayEffect")
FAILURES = {"ActivateFailCooldownTag": "Ability.ActivateFail.Cooldown", "ActivateFailCostTag": "Ability.ActivateFail.Cost",
            "ActivateFailNetworkingTag": "Ability.ActivateFail.Networking", "ActivateFailTagsBlockedTag": "Ability.ActivateFail.TagsBlocked",
            "ActivateFailTagsMissingTag": "Ability.ActivateFail.TagsMissing"}


def require(ok, message):
    if not ok: raise RuntimeError(message)


def emit(event, **values):
    print("R1_STARTUP " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def encode(value):
    if value is None: return None
    if isinstance(value, ue.Object): return value.get_path_name()
    return value.export_text() if hasattr(value, "export_text") else str(value)


def signature(asset):
    return {field: encode(asset.get_editor_property(field)) for field in FIELDS}


def protected_state():
    files = [PROJECT / "Config/DefaultGame.ini", PROJECT / "Config/DefaultEngine.ini", PROJECT / ".git/index"]
    # All other uassets/sidecars directly in this data asset's directory stay untouched.
    files += [p for p in (PROJECT / "Content").iterdir() if p.is_file() and p.stem != "DefaultGameData"]
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}


def main():
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR1StartupMode=(resave|verify)(?:\s|$)", command)
    run = re.search(r"-DCR1StartupRun=([A-Za-z0-9_]{1,40})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit isolated commandlet required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context/SCC/SaveOnCompile guard rejected")
    logfile = PROJECT / "Saved/Logs" / ("R1Startup-" + run.group(1) + ".log")
    before_files = protected_state()
    try:
        asset = ue.load_asset(DATA)
        require(asset is not None and asset.get_class().get_path_name() == "/Script/DreamCatcher.DCGameData", "Expected existing DCGameData")
        before = signature(asset)
        require(all(before.values()), "GameData GE references must remain populated")
        settings = ue.get_default_object(ue.load_class(None, "/Script/GameplayAbilities.GameplayAbilitiesDeveloperSettings"))
        tags = {name: encode(settings.get_editor_property(name)) for name in FAILURES}
        emit("failure_settings_read", values=tags)
        require(all(tags[name] == '(TagName="' + value + '")' for name, value in FAILURES.items()), "Failure tags not restored")
        emit("before", cls=asset.get_class().get_path_name(), effects=before, failure_tags=tags)
        require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure")
        errors = [line[:1400] for line in logfile.read_text(encoding="utf-8-sig", errors="replace").splitlines()
                  if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
        require(not errors, "Engine errors; refuse resave: " + repr(errors[:3]))
        if mode.group(1) == "resave":
            require(ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(asset, False), "GameData resave failed")
        require(signature(asset) == before, "Gameplay-effect references changed")
        registry = ue.AssetRegistryHelpers.get_asset_registry()
        data = registry.get_asset_by_object_path(DATA + ".DefaultGameData", True)
        emit("complete", mode=mode.group(1), effects=signature(asset), asset_type=encode(data.get_tag_value("PrimaryAssetType")),
             asset_name=encode(data.get_tag_value("PrimaryAssetName")), asset_saves=1 if mode.group(1) == "resave" else 0,
             standalone_verified=False)
    finally:
        require(protected_state() == before_files, "Protected Config/index/other root assets changed")
        emit("protected_unchanged", files=len(before_files))


if __name__ == "__main__":
    main()
