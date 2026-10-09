"""Exercise the approved, bounded Editor copy bridge; never build or package.

Default mode is read-only preflight. memory/save require explicit command-line
mode and a fresh run name. verify only reloads/compiles copies in memory.
All asset writes are made by Unreal's native bridge, never Python file I/O.
Pass -DCRiflePreserveOverrides to opt into SCS snapshot/restore/compare before save.
With the original weapon_actor pair only, -DCRifleAllowKnownLoadDirty opts into
four fixed unsaved Niagara dependencies, guarded by pre/post file fingerprints.
compare mode reports the comparison without treating an expected mismatch as a script failure.
"""
import json
import os
from pathlib import Path
import re

import unreal


ROOT = "/Game/LyraMigration/Rifle/Diagnostics/Explicit"
GROUPS = {
    "audio": (
        "/Game/Audio/Blueprints/B_MusicManagerComponent_Base",
        "/Game/Audio/Blueprints/WeaponAudioMacros",
    ),
    "weapon_actor": (
        "/Game/Weapons/B_Weapon",
        "/ShooterCore/Weapons/Rifle/B_Rifle",
    ),
    # Read-only expansion of the already inventoried core roots. Never passed to copy.
    "rifle_core_audit": (
        "/ShooterCore/Weapons/B_WeaponInstance_Base",
        "/ShooterCore/Weapons/Rifle/B_WeaponInstance_Rifle",
        "/Game/Weapons/B_Weapon",
        "/ShooterCore/Weapons/Rifle/B_Rifle",
        "/Game/Weapons/GA_Weapon_Fire",
        "/ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto",
        "/Game/Weapons/GA_Weapon_ReloadMagazine",
        "/ShooterCore/Weapons/Rifle/GA_Weapon_Reload_Rifle",
        "/Game/Weapons/GA_Weapon_AutoReload",
        "/ShooterCore/Weapons/Rifle/W_Reticle_Rifle",
        "/ShooterCore/Weapons/Rifle/W_AmmoCounter_Rifle",
        "/ShooterCore/Weapons/Rifle/AbilitySet_ShooterRifle",
        "/ShooterCore/Weapons/Rifle/WID_Rifle",
        "/ShooterCore/Weapons/Rifle/ID_Rifle",
        "/ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire",
        "/Game/GameplayCueNotifies/GCN_Weapon_Impact",
    ),
}


def emit(event, **values):
    print("DC_RIFLE_EXPLICIT " + json.dumps({"event": event, **values}, ensure_ascii=False))


def require(value, message):
    if not value:
        raise RuntimeError(message)


def option(command, name, default):
    match = re.search(r"(?:^|\s)-" + re.escape(name) + r"=(\S+)", command)
    return match.group(1) if match else default


def inspect_component_templates(packages):
    # Enumerate already-loaded templates. Do not use GetObjectForBlueprint:
    # the editor's implementation can create an override even for a getter.
    for package in packages:
        require(unreal.load_asset(package) is not None, "Missing audit package: " + package)
    for package in packages:
        templates = []
        for component in unreal.ObjectIterator(unreal.SkeletalMeshComponent):
            if not component.get_path_name().startswith(package + "."):
                continue
            values = {}
            for name in ("skeletal_mesh_asset", "anim_class", "override_materials", "relative_location", "relative_rotation", "relative_scale3d"):
                try:
                    value = component.get_editor_property(name)
                    if isinstance(value, unreal.Object):
                        value = value.get_path_name()
                    elif isinstance(value, unreal.Array):
                        value = [item.get_path_name() if isinstance(item, unreal.Object) else str(item) for item in value]
                    elif value is not None and not isinstance(value, (str, bool, float, int)):
                        value = str(value)
                    values[name] = value
                except Exception as error:
                    values[name] = {"unverified": str(error)}
            templates.append({"object": component.get_path_name(), "values": values})
        emit("component_templates", package=package, templates=templates)


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    actual = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(actual)) == os.path.normcase(str(expected)), "Original Lyra project required")
    mode = option(command, "DCRifleExplicitMode", "preflight")
    run = option(command, "DCRifleExplicitRun", "")
    group = option(command, "DCRifleExplicitGroup", "audio")
    preserve = bool(re.search(r"(?:^|\s)-DCRiflePreserveOverrides(?:\s|$)", command))
    allow_load_dirty = bool(re.search(r"(?:^|\s)-DCRifleAllowKnownLoadDirty(?:\s|$)", command))
    require(group in GROUPS, "Only a fixed approved diagnostic group is permitted")
    require(not allow_load_dirty or (preserve and group == "weapon_actor"),
            "Known load-dirty policy requires override preservation and the original weapon_actor pair")
    sources = GROUPS[group]
    require(mode in ("preflight", "memory", "save", "verify", "inspect", "compare", "dirty_audit"), "Unknown mode")
    require(group != "rifle_core_audit" or mode == "dirty_audit", "Core group is read-only; copy is not authorized by this script")
    require(re.fullmatch(r"[A-Za-z0-9_]{1,48}", run), "Explicit safe run name required")
    plan = {source: ROOT + "/" + run + "/" + source.rsplit("/", 1)[1] for source in sources}
    require(hasattr(unreal, "DCRifleMigrationLibrary"), "Compiled Editor bridge did not load")
    bridge = unreal.DCRifleMigrationLibrary
    if mode in ("memory", "save"):
        require("allow_known_load_dirty_dependencies" in (bridge.copy_package_subset.__doc__ or ""),
                "Rebuild the Editor plugin before running this updated copy script")
    emit("begin", mode=mode, run=run, group=group, plan=plan, preserve_overrides=preserve,
         allow_known_load_dirty=allow_load_dirty, cpp_build=False, packaging=False)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    dependency_options = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False,
        include_hard_management_references=False,
    )

    if mode == "dirty_audit":
        def dirty_packages():
            return sorted(package.get_path_name() for package in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        before = dirty_packages()
        emit("dirty_before_load", packages=before)
        for source in sources:
            asset = unreal.load_asset(source)
            require(asset is not None, "Missing original: " + source)
            after = dirty_packages()
            emit("dirty_after_load", source=source, asset_class=asset.get_class().get_path_name(), packages=after,
                 explicit_sources_dirty=sorted(set(sources) & set(after)),
                 newly_dirty=sorted(set(after) - set(before)))
        if group == "rifle_core_audit":
            pending, visited, external, unavailable = list(sources), set(), set(), []
            while pending:
                package = pending.pop()
                if package in visited or package in external:
                    continue
                if package.startswith(("/Script/", "/Engine/")):
                    external.add(package)
                    continue
                visited.add(package)
                deps = registry.get_dependencies(package, dependency_options)
                if deps is None:
                    unavailable.append(package)
                else:
                    pending.extend(str(dep) for dep in deps)
            mounts = {}
            for package in visited:
                mount = package.split("/", 2)[1]
                mounts[mount] = mounts.get(mount, 0) + 1
            emit("core_dependency_closure", roots=len(sources), packages=len(visited), mounts=mounts,
                 engine_script_externals=len(external), unavailable=sorted(unavailable),
                 includes_original_hero="/ShooterCore/Game/B_Hero_ShooterMannequin" in visited,
                 note="Original roots, not patched staging; package references only, no save or migration")
        emit("complete", mode=mode, asset_writes=0, clear_dirty_flags=False, copy_invoked=False)
        return

    if mode == "inspect":
        inspect_component_templates(list(plan) + list(plan.values()))
        emit("complete", mode=mode, asset_writes=0)
        return

    if mode == "compare":
        comparison = bridge.compare_inherited_overrides(run, plan)
        emit("override_comparison", success=comparison.get_editor_property("succeeded"),
             expected=comparison.get_editor_property("expected_overrides"),
             compared=comparison.get_editor_property("compared_overrides"),
             messages=list(comparison.get_editor_property("messages")))
        emit("complete", mode=mode, asset_writes=0, comparison_report_only=True)
        return

    if mode == "verify":
        # Existing saved run folders must never be accepted for another copy.
        rejected = bridge.validate_copy_plan(run, plan)
        require(rejected is None, "Existing diagnostic folder unexpectedly accepted")
        emit("existing_destination_rejected", run=run)

    if mode != "verify":
        emit("api", validate_doc=bridge.validate_copy_plan.__doc__, copy_doc=bridge.copy_package_subset.__doc__)
        # UE Python folds a bool return + one out parameter into str/None.
        report = bridge.validate_copy_plan(run, plan)
        valid = isinstance(report, str)
        emit("preflight", valid=valid, report=report)
        require(valid, report or "Preflight rejected; inspect LogDCRifleMigration")
        for source in sources:
            dependencies = registry.get_dependencies(source, dependency_options)
            require(dependencies is not None, "Source registry data unavailable: " + source)
            emit("source_dependencies", source=source, packages=sorted(str(p) for p in dependencies))
        if mode == "preflight":
            emit("complete", mode=mode, asset_writes=0)
            return
        result = bridge.copy_package_subset(run, plan, mode == "save", preserve, allow_load_dirty)
        fields = {name: result.get_editor_property(name) for name in (
            "preflight_passed", "copy_invoked", "native_copy_reported_success",
            "source_files_unchanged", "destinations_verified", "saved_to_disk", "succeeded",
            "inherited_overrides_requested", "inherited_overrides_verified", "inherited_override_count",
            "known_load_dirty_policy_requested", "known_load_dirty_policy_verified",
        )}
        emit("copy_result", **fields, messages=list(result.get_editor_property("messages")),
             observed_load_dirty_dependencies=list(result.get_editor_property("observed_load_dirty_dependencies")))
        require(fields["succeeded"], "Native bridge output checks failed; preserve partial diagnostics")
        require(fields["saved_to_disk"] == (mode == "save"), "Unexpected save state")
        if preserve:
            require(fields["inherited_overrides_verified"], "Override preservation failed; no migration-ready output")
        if allow_load_dirty:
            require(fields["known_load_dirty_policy_verified"], "Known load-dirty policy check failed")

    # Inspection and Compile here never save. The native copy already compiled
    # before its optional save; verify is a separate-process load/compile check.
    for source, destination in plan.items():
        emit("before_inspect", destination=destination)
        asset = unreal.load_asset(destination)
        require(asset is not None, "Destination load failed: " + destination)
        parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
        compile_ok = unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        require(compile_ok, "Destination Blueprint compile failed: " + destination)
        dependencies = registry.get_dependencies(destination, dependency_options)
        if mode == "verify":
            require(dependencies is not None, "Reloaded destination registry data missing")
        emit("inspected", destination=destination, parent=parent.get_path_name() if parent else None,
             compile_ok=compile_ok, dependencies=None if dependencies is None else sorted(str(p) for p in dependencies))
        if mode == "verify":
            original = unreal.load_asset(source)
            require(original is not None, "Original unavailable for read-only comparison")
            original_parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(original)
            expected_parent = original_parent.get_path_name() if original_parent else None
            for old, new in plan.items():
                if expected_parent and expected_parent.startswith(old + "."):
                    expected_parent = new + expected_parent[len(old):]
                    break
            require((parent.get_path_name() if parent else None) == expected_parent, "Parent class remap mismatch")
            original_deps = registry.get_dependencies(source, dependency_options)
            require(original_deps is not None, "Original dependency data unavailable")
            expected_deps = {plan.get(str(dep), str(dep)) for dep in original_deps}
            actual_deps = {str(dep) for dep in dependencies}
            missing = sorted(expected_deps - actual_deps)
            unexpected = sorted(actual_deps - expected_deps)
            emit("reference_comparison", source=source, destination=destination,
                 expected_parent=expected_parent, missing=missing, unexpected=unexpected)
            require(not missing and not unexpected, "Dependency remap differs from original; retain diagnostics, do not activate")
    if preserve:
        comparison = bridge.compare_inherited_overrides(run, plan)
        success = comparison.get_editor_property("succeeded")
        count = comparison.get_editor_property("expected_overrides")
        emit("override_comparison", success=success, expected=count,
             compared=comparison.get_editor_property("compared_overrides"),
             messages=list(comparison.get_editor_property("messages")))
        require(success, "Full inherited override comparison failed")
        if group == "weapon_actor":
            require(count > 0, "Expected a Rifle inherited override, not an empty comparison")
    emit("complete", mode=mode, original_saves=0, migration=False,
         fresh_process_reload=(mode == "verify"), gameplay_verified=False)


main()
