"""Read-only verification of the approved R6 Fire/Impact Cue path switch.

Run in a separate Python commandlet with -DCRifleCopyDiagnostics and
-DCR6CueRun=<name>, matching Saved/Logs/R6RifleCue-<name>.log.
Use process-only -ini:Game overrides for preflight; omit them after Config switch.
No asset saving, C++ build, Cue invocation, or generic auditing framework.
"""
import hashlib
import json
from pathlib import Path
import re
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
CORE = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247"
DEATH = "/Game/GameplayCueNotifies/DCMigration/Death"
OLD = "/Game/DreamCatcher/GAS/GameplayCues"
CUES = {CORE + "/GCN_Weapon_Rifle_Fire": "GameplayCue.Weapon.Rifle.Fire",
        CORE + "/GCN_Weapon_Impact": "GameplayCue.Weapon.Rifle.Impact"}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def emit(event, **values):
    print("R6_CUE " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def fingerprint(packages):
    files = [PROJECT / "Config/DefaultGame.ini", PROJECT / ".git/index"]
    for package in packages:
        if package.startswith("/Game/"):
            files.extend(PROJECT / "Content" / (package[6:] + ext)
                         for ext in (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl"))
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None for p in files}


def main():
    command = ue.SystemLibrary.get_command_line()
    run = re.search(r"-DCR6CueRun=([A-Za-z0-9_]{1,40})(?:\s|$)", command)
    require(run and "-run=pythonscript" in command.lower(), "Explicit commandlet run required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Diagnostic guard rejected")
    logfile = PROJECT / "Saved/Logs" / ("R6RifleCue-" + run.group(1) + ".log")
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    # The manager creates GlobalGameplayCueSet for runtime and EditorGameplayCueSet
    # for preview (GameplayCueManager.cpp). Read the runtime set's public entries;
    # Globals' Config field and the manager's library field are protected in Python.
    manager_type = ue.load_class(None, "/Script/GameplayAbilities.GameplayCueManager")
    cue_set_type = ue.load_class(None, "/Script/GameplayAbilities.GameplayCueSet")
    require(manager_type is not None and cue_set_type is not None, "Native Cue classes not found")
    managers = [obj for obj in ue.ObjectIterator(manager_type) if "Default__" not in obj.get_path_name()]
    require(len(managers) == 1, "Expected one live CueManager: " + repr([x.get_path_name() for x in managers]))
    cue_sets = [obj for obj in ue.ObjectIterator(cue_set_type)
                if obj.get_name() == "GlobalGameplayCueSet" and obj.get_outer() == managers[0]]
    require(len(cue_sets) == 1, "Expected exactly one initialized runtime CueSet")
    cue_set = cue_sets[0]
    options = ue.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
    dependencies = {p: sorted(str(x) for x in registry.get_dependencies(p, options)) for p in CUES}
    packages = sorted(set(CUES) | {d for ds in dependencies.values() for d in ds if not d.startswith("/Script/")})
    protected = packages + [OLD + "/GCN_DC_Weapon_Rifle_Fire"]
    before = fingerprint(protected)
    try:
        for package in packages:
            require(not package.startswith("/ShooterCore/"), "Original plugin dependency remains: " + package)
            require(ue.load_asset(package) is not None, "Missing direct dependency: " + package)
        for package, tag in CUES.items():
            bp = ue.load_asset(package)
            require(ue.BlueprintEditorLibrary.compile_blueprint(bp), "Cue Compile failed: " + package)
            cls = ue.BlueprintEditorLibrary.generated_class(bp)
            require(cls is not None, "Missing Cue generated class")
            actual = ue.get_default_object(cls).get_editor_property("gameplay_cue_tag").export_text()
            require(actual == '(TagName="' + tag + '")', "Cue tag mismatch: " + actual)
            emit("cue_loaded", asset=package, tag=tag, generated_class=cls.get_path_name(), direct_dependencies=dependencies[package])
        # Engine's built-in command prints the actual runtime CueSet, not just CDO tags.
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
        require(world is not None, "No editor world for the engine Cue diagnostic")
        ue.SystemLibrary.execute_console_command(world, "GameplayCue.PrintGameplayCueNotifyMap")
        registrations = []
        for entry in cue_set.get_editor_property("GameplayCueData"):
            registrations.append({"tag": entry.get_editor_property("GameplayCueTag").export_text(),
                                  "class": entry.get_editor_property("GameplayCueNotifyObj").export_text()})
        emit("runtime_registrations", manager=managers[0].get_path_name(), entries=registrations)
        for package, tag in CUES.items():
            matches = [r for r in registrations if r["tag"] == '(TagName="' + tag + '")']
            require(len(matches) == 1 and package in matches[0]["class"], "Runtime Cue registration mismatch: " + tag)
        require(all("GCN_DC_Weapon_Rifle_Fire" not in r["class"] for r in registrations), "Old test Cue still registered")
        require(any('GameplayCue.Character.Death' in r["tag"] and DEATH in r["class"] for r in registrations), "Death registration lost")
        require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure")
        lines = logfile.read_text(encoding="utf-8-sig", errors="replace").splitlines()
        errors = [line[:1200] for line in lines if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
        require(not errors, "Engine diagnostic failure: " + repr(errors[:3]))
        emit("read_complete", direct_packages_loaded=len(packages), asset_saves=0, visual_runtime_verified=False)
    finally:
        require(fingerprint(protected) == before, "Protected file changed")
        emit("protected_unchanged", file_states=len(before))


if __name__ == "__main__":
    main()
