"""One-off approved R5/R6 asset migration. No C++ build or binary file copying.

Original Lyra: -DCRifleIntegrationMode=migrate, using the saved exact closure plan.
DreamCatcher: -DCRifleIntegrationMode=verify (read/Compile imported core only).
Existing destination assets are SKIPPED, never overwritten by migration.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
import unreal as ue

TARGET = Path(__file__).resolve().parents[2]
SOURCE = TARGET.parent / "LyraStarterGame"
PREPARED = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247"
REPORT = TARGET / "Saved/Diagnostics/R5RifleIntegration"
EXTENSIONS = (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl")
VERIFY_LOG = None


def require(value, message):
    if not value:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_RIFLE_INTEGRATION " + json.dumps({"event": event, **values}, ensure_ascii=False))


def digest(path):
    if not path.is_file():
        return None
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def path_for(root, package):
    require(re.fullmatch(r"/(Game|ShooterCore)/[A-Za-z0-9_+/-]+", package), "Invalid package: " + package)
    require("//" not in package, "Empty package segment")
    mount, relative = package[1:].split("/", 1)
    content = root / ("Content" if mount == "Game" else "Plugins/GameFeatures/ShooterCore/Content")
    result = (content / relative).resolve()
    require(result.is_relative_to(content.resolve()), "Package escaped content")
    return result


def states(root, packages):
    return {str(path_for(root, p).with_suffix(ext)): digest(path_for(root, p).with_suffix(ext))
            for p in packages for ext in EXTENSIONS}


def unchanged(before):
    changed = [path for path, value in before.items() if digest(Path(path)) != value]
    require(not changed, "Protected files changed: " + repr(changed[:20]))


def saved_plan():
    records = []
    for line in (SOURCE / "Saved/Logs/R5Closure18-dependency-plan.log").read_text(encoding="utf-8-sig").splitlines():
        if "DC_RIFLE_DEPENDENCIES " in line:
            item = json.loads(line.split("DC_RIFLE_DEPENDENCIES ", 1)[1])
            if item["event"] == "plan":
                records.append(item)
    require(len(records) == 1 and records[0]["saved_run"] == "RifleCore_Prepared_0247", "Expected verified closure plan")
    return records[0]


def clean(phase):
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure: " + phase)
    if VERIFY_LOG is not None:
        require(VERIFY_LOG.is_file(), "Use the expected fresh per-run verify -abslog")
        errors = [line[:2048] for line in VERIFY_LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
                  if re.search(r": Error:|Fatal:|Ensure condition failed", line)]
        require(not errors, "Actual asset load/Compile errors at " + phase + ": " + repr(errors[:3]))


def main():
    global VERIFY_LOG
    command = ue.SystemLibrary.get_command_line()
    match = re.search(r"(?:^|\s)-DCRifleIntegrationMode=(migrate|migrate-channel|migrate-tags|verify)(?:\s|$)", command)
    require(match and "-run=pythonscript" in command.lower(), "Explicit Python commandlet mode required")
    require("-dcriflecopydiagnostics" in command.lower(), "Diagnostic guard opt-in required")
    require(not ue.SourceControl.is_enabled(), "Source control must be disabled")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context rejected")
    mode = match.group(1)
    if mode == "verify":
        run = re.search(r"(?:^|\s)-DCRifleIntegrationRun=([A-Za-z0-9_]{1,32})(?:\s|$)", command)
        require(run, "Use a fresh DCRifleIntegrationRun for post-build verification")
        VERIFY_LOG = TARGET / "Saved/Logs" / ("R5RifleIntegration-verify-" + run.group(1) + ".log")
    project = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    require(project == (TARGET if mode == "verify" else SOURCE).resolve(), "Wrong project for operation")
    plan = saved_plan()
    selected = sorted(p for p in plan["package_edges"] if p.startswith(("/Game/", "/ShooterCore/")))
    require(len(selected) == 969, "Approved Game967+ShooterCore2 closure changed")
    require(sorted(p for p in selected if p.startswith("/ShooterCore/")) == [
        "/ShooterCore/Input/Abilities/Struct_UIMessaging", "/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto"], "Unexpected plugin assets")
    mapped = {p: "/Game/" + p.split("/", 2)[2] for p in selected}
    require(len(set(mapped.values())) == len(mapped), "Mount remapping collision")
    core = sorted(p for p in selected if p.startswith(PREPARED + "/"))
    require(len(core) == 18, "Exact prepared18 required")
    protect = {str(TARGET / p): digest(TARGET / p) for p in (
        "Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultInput.ini", "DreamCatcher.uproject", ".git/index")}
    if mode == "migrate-tags":
        tables = ["/Game/ContextEffects/DT_AnimEffectTags", "/Game/ContextEffects/DT_SurfaceTypes"]
        require(all(not path_for(TARGET, p).with_suffix(".uasset").exists() for p in tables), "Do not overwrite tag tables")
        before = states(SOURCE, tables)
        opts = ue.MigrationOptions()
        opts.set_editor_property("prompt", False)
        opts.set_editor_property("ignore_dependencies", True)
        opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
        ue.AssetToolsHelpers.get_asset_tools().migrate_packages(tables, str(TARGET / "Content"), opts)
        unchanged(before)
        unchanged(protect)
        require(all(path_for(TARGET, p).with_suffix(".uasset").is_file() for p in tables), "Tag tables missing")
        clean("tag table migration")
        emit("tag_tables_migrated", packages=tables, existing_assets_overwritten=0)
    elif mode == "migrate-channel":
        # UE5.8 new migration rejects this one legacy Niagara package as asset-less.
        # The engine's existing file migration preserves it without custom binary I/O.
        channel = "/Game/Effects/Particles/Impacts/NS_ImpactDataChannel"
        previous = json.loads((REPORT / "migration-after.json").read_text(encoding="utf-8"))
        require(previous["missing"] == [channel] and previous["new_packages_present"] == 854, "Only the recorded missing channel can be retried")
        require(not path_for(TARGET, channel).with_suffix(".uasset").exists(), "Channel destination must be absent")
        require(ue.SystemLibrary.get_console_variable_int_value("AssetTools.UseNewPackageMigration") == 0,
                "Use a process-only ConsoleVariables override for the engine legacy migration")
        checkpoint = json.loads((REPORT / "migration-before.json").read_text(encoding="utf-8"))
        target_before = states(TARGET, mapped.values())
        unchanged(checkpoint["source"])
        opts = ue.MigrationOptions()
        opts.set_editor_property("prompt", False)
        opts.set_editor_property("ignore_dependencies", True)
        opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
        try:
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages([channel], str(TARGET / "Content"), opts)
        finally:
            unchanged(checkpoint["source"])
            unchanged({p: h for p, h in target_before.items() if h is not None})
            unchanged(protect)
        require(digest(path_for(TARGET, channel).with_suffix(".uasset")) == digest(path_for(SOURCE, channel).with_suffix(".uasset")),
                "Channel did not migrate byte-identically")
        clean("channel migration")
        emit("channel_migrated", source_equal=True, all_855_present=all(path_for(TARGET, p).with_suffix(".uasset").is_file() for p in mapped.values()))
    elif mode == "migrate":
        require(not REPORT.exists(), "New migration report required; inspect any earlier partial run before retry")
        require(not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages(), "Initial dirty packages refused")
        source_before = states(SOURCE, selected)
        target_before = states(TARGET, mapped.values())
        existing = sorted(p for p in mapped.values() if path_for(TARGET, p).with_suffix(".uasset").is_file())
        require(existing == sorted(item["package"] for item in plan["collisions"]), "Existing target set differs from audited114")
        require(len(existing) == 114 and all(not path_for(TARGET, p).with_suffix(".uasset").exists() for p in core), "No replacement/resume of core")
        require(all(path_for(SOURCE, p).with_suffix(".uasset").is_file() for p in selected), "Missing source package")
        REPORT.mkdir(parents=True, exist_ok=False)
        with (REPORT / "migration-before.json").open("x", encoding="utf-8") as stream:
            json.dump({"source": source_before, "destination": target_before, "mapping": mapped,
                       "existing_skipped": existing, "core": core, "protect": protect}, stream, indent=2)
        emit("migration_begin", core=18, explicit_packages=len(selected), skip_existing=len(existing), new_packages=len(selected)-len(existing))
        opts = ue.MigrationOptions()
        opts.set_editor_property("prompt", False)
        opts.set_editor_property("ignore_dependencies", True)  # The bounded full Game/Shooter closure is explicit.
        opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
        try:
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(selected, str(TARGET / "Content"), opts)
        finally:
            unchanged(source_before)
            unchanged({p: h for p, h in target_before.items() if h is not None})
            unchanged(protect)
        clean("migration")
        missing = [p for p in mapped.values() if not path_for(TARGET, p).with_suffix(".uasset").is_file()]
        after = {"missing": missing, "new_packages_present": len(selected)-len(existing)-len(missing),
                 "old_files_unchanged": True, "source_files_unchanged": True, "cpp_build": False, "packaging": False}
        with (REPORT / "migration-after.json").open("x", encoding="utf-8") as stream:
            json.dump(after, stream, indent=2)
        emit("migration_result", **after)
        require(not missing, "Migration incomplete; preserve partial output and inspect log")
    else:
        records = []
        for package in core + [mapped[p] for p in selected if p.startswith("/ShooterCore/")]:
            clean("before load")
            asset = ue.load_asset(package)
            require(asset is not None, "Load failed: " + package)
            row = {"package": package, "class": asset.get_class().get_path_name()}
            if isinstance(asset, ue.Blueprint):
                row["parent"] = ue.BlueprintEditorLibrary.get_blueprint_parent_class(asset).get_path_name()
                ue.BlueprintEditorLibrary.compile_blueprint(asset)
                row["generated"] = str(ue.BlueprintEditorLibrary.generated_class(asset))
            records.append(row)
            emit("core_loaded", **row)
            clean("after compile")
        unchanged(protect)
        emit("verify_complete", count=len(records), asset_saves=0, runtime_verified=False)


if __name__ == "__main__":
    main()
