"""Approved exact-18 core/library preparation inside original Lyra, never C++ build/Migrate.

-DCRifleCoreMode=preflight (default), inspect, memory, prepare, verify, or plan
-DCRifleCoreRun=RifleCore_<fresh ASCII name>
prepare: native memory copy -> comparisons -> one approved Hero pin edit ->
comparisons/guards -> save only this invocation's new copies. No overwrite/resume.
verify/plan are asset-read-only; use a separate process after prepare.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import sys

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from r5_rifle_core_contract import (
    BASE, EXTENSIONS, HERO_NEW, HERO_OLD, HERO_PACKAGE, ROOT,
    build_plan, check_dirty_names, expected_dependencies, expected_graph, expected_type_references, require, translate,
)

DATA_FIELDS = {
    "ID_Rifle": ("DisplayName", "Fragments"),
    "WID_Rifle": ("InstanceType", "AbilitySetsToGrant", "ActorsToSpawn"),
    "AbilitySet_ShooterRifle": ("GrantedGameplayAbilities", "GrantedGameplayEffects", "GrantedAttributes"),
    "B_WeaponInstance_Rifle": (
        "SpreadRecoveryCooldownDelay", "HeatToSpreadCurve", "HeatToHeatPerShotCurve", "HeatToCoolDownPerSecondCurve",
        "bAllowFirstShotAccuracy", "SpreadExponent", "SpreadAngleMultiplier_Aiming", "SpreadAngleMultiplier_StandingStill",
        "SpreadAngleMultiplier_Crouching", "SpreadAngleMultiplier_JumpingOrFalling", "BulletsPerCartridge",
        "MaxDamageRange", "BulletTraceSweepRadius", "DistanceDamageFalloff", "MaterialDamageMultiplier",
        "EquippedAnimSet", "UneuippedAnimSet", "ApplicableDeviceProperties",
    ),
}
ABILITY_FIELDS = ("ActivationPolicy", "AdditionalCosts", "CostGameplayEffectClass", "CooldownGameplayEffectClass",
                  "InstancingPolicy", "NetExecutionPolicy", "AbilityTriggers", "ActivationOwnedTags")
OWNED_FIELDS = {
    "InventoryFragment_EquippableItem": ("EquipmentDefinition",),
    "InventoryFragment_SetStats": ("InitialItemStats",),
    "InventoryFragment_ReticleConfig": ("ReticleWidgets",),
    "InventoryFragment_QuickBarIcon": ("Brush", "AmmoBrush", "DisplayNameWhenEquipped"),
    "InventoryFragment_PickupIcon": ("SkeletalMesh", "DisplayName", "PadColor"),
    "LyraAbilityCost_ItemTagStack": ("Quantity", "Tag", "FailureTag"),
}


def emit(event, **values):
    print("DC_RIFLE_CORE " + json.dumps({"event": event, **values}, ensure_ascii=False))


def option(command, name, default):
    match = re.search(r"(?:^|\s)-" + re.escape(name) + r"=(\S+)", command)
    return match.group(1) if match else default


def file_path(project, package, extension=".uasset"):
    if package.startswith("/Game/"):
        root, relative = project / "Content", package[len("/Game/"):]
    elif package.startswith("/ShooterCore/"):
        root = project / "Plugins/GameFeatures/ShooterCore/Content"
        relative = package[len("/ShooterCore/"):]
    else:
        raise RuntimeError("Unapproved file mapping: " + package)
    path = (root / (relative + extension)).resolve()
    require(path.is_relative_to(root.resolve()), "Package path escapes mounted content")
    return path


def fingerprints(project, packages):
    result = {}
    for package in packages:
        for extension in EXTENSIONS:
            path = file_path(project, package, extension)
            require(extension != ".uasset" or path.is_file(), "Missing package file: " + str(path))
            digest = None
            if path.exists():
                with path.open("rb") as stream:
                    digest = hashlib.file_digest(stream, "sha256").hexdigest()
            result[str(path)] = digest
    return result


def check_files(project, packages, before):
    after = fingerprints(project, packages)
    changed = [path for path in before if before[path] != after.get(path)]
    require(not changed, "Original/saved file state changed: " + repr(changed))
    emit("file_states_unchanged", checked=len(before))


def dirty_names():
    return {package.get_path_name() for package in (
        list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        + list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()))}


def check_engine_clean(bridge, phase):
    require(bridge.get_engine_ensure_failure_count() == 0,
            "Engine ensure detected at " + phase + "; stop mutation/save and inspect the log")


def save_guard(project, plan, allowed, siblings, before, bridge):
    check_engine_clean(bridge, "save_guard")
    require(not unreal.SourceControl.is_enabled(), "Source control must remain disabled")
    check_dirty_names(dirty_names(), plan, plan.values(), allowed)
    check_files(project, list(plan) + allowed + siblings, before)
    for destination in plan.values():
        for extension in EXTENSIONS + (".umap",):
            require(not file_path(project, destination, extension).exists(), "Never overwrite: " + destination)


def encode(value):
    if value is None or isinstance(value, (str, bool, int, float)):
        return value
    if isinstance(value, unreal.Object):
        name = value.get_class().get_name()
        if name in OWNED_FIELDS:
            return {"class": value.get_class().get_path_name(),
                    "fields": {field: encode(value.get_editor_property(field)) for field in OWNED_FIELDS[name]}}
        require("InventoryFragment" not in name, "Unverified fragment fields: " + name)
        return value.get_path_name()
    if isinstance(value, (list, tuple, unreal.Array)):
        return [encode(item) for item in value]
    if isinstance(value, (dict, unreal.Map)):
        return {str(encode(key)): encode(item) for key, item in value.items()}
    if callable(getattr(value, "export_text", None)):
        return value.export_text()
    return str(value)


def owner(asset):
    if isinstance(asset, unreal.Blueprint):
        generated = unreal.BlueprintEditorLibrary.generated_class(asset)
        require(generated is not None, "Missing generated class: " + asset.get_path_name())
        return unreal.get_default_object(generated)
    return asset


def graph_index():
    result = {}
    for node in unreal.ObjectIterator(unreal.EdGraphNode):
        outer = node.get_outer()
        if outer:
            result.setdefault(outer.get_path_name(), []).append(node)
    return result


def graph_signature(asset, index):
    # Structural/default-value comparison, not a Widget Designer or all-pin-type audit.
    result = {}
    for graph in unreal.BlueprintEditorLibrary.list_graphs(asset):
        nodes = {}
        for node in index.get(graph.get_path_name(), []):
            data = {"class": node.get_class().get_path_name()}
            if isinstance(node, unreal.K2Node):
                data["title"] = unreal.BlueprintEditorLibrary.get_node_title(node)
                pins = {}
                for pin in unreal.BlueprintEditorLibrary.list_all_pins(node):
                    name, direction = str(pin.get_pin_name()), str(pin.get_pin_direction())
                    key = name + "|" + direction
                    require(key not in pins, "Ambiguous pin name/direction")
                    pins[key] = {"name": name, "direction": direction, "value": pin.get_pin_value(),
                                 "links": sorted(other.get_owning_node().get_name() + "|" + str(other.get_pin_name())
                                                 + "|" + str(other.get_pin_direction()) for other in pin.list_connected_pins())}
                data["pins"] = pins
            nodes[node.get_name()] = data
        result[graph.get_name()] = nodes
    return result


def type_report(asset, bridge):
    report = bridge.inspect_blueprint_type_references(asset)
    check_engine_clean(bridge, "type_report")
    require(report.get_editor_property("succeeded"), "Unverified C++ type references: " + repr(list(report.get_editor_property("messages"))))
    return list(report.get_editor_property("references")), report.get_editor_property("blueprint_state")


def original_snapshot(sources, bridge):
    result = {}
    for source in sources:
        asset = unreal.load_asset(source)
        require(asset is not None, "Original/sibling failed to load: " + source)
        if isinstance(asset, unreal.Blueprint):
            result[source] = type_report(asset, bridge)
    return result


def check_original_snapshot(before, bridge):
    require(original_snapshot(before, bridge) == before, "Original parent/class identity/status/dirty/type references changed")


def patch_hero(copies, plan, bridge):
    check_engine_clean(bridge, "before_hero_edit")
    asset = copies[BASE]
    require(asset.get_path_name() == plan[BASE] + ".B_WeaponInstance_Base", "Hero edit must target this invocation's copy")
    graph = unreal.BlueprintEditorLibrary.find_event_graph(asset)
    pins = []
    for node in unreal.ObjectIterator(unreal.K2Node):
        if node.get_outer() == graph and unreal.BlueprintEditorLibrary.get_node_title(node) == "GetTypedPawn":
            pin = unreal.BlueprintEditorLibrary.find_input_pin(node, "PawnType")
            if pin.is_valid() and pin.get_pin_value() == HERO_OLD:
                pins.append(pin)
    require(len(pins) == 1, "Expected one unmodified original Hero pin; no resume/second patch")
    require(pins[0].set_pin_value(HERO_NEW), "Hero pin setter failed")
    check_engine_clean(bridge, "after_hero_edit")
    require(pins[0].get_pin_value() == HERO_NEW, "Hero pin value mismatch")
    # Base first, then the derived weapon instance; no reparenting.
    for source in (BASE, "/ShooterCore/Weapons/Rifle/B_WeaponInstance_Rifle"):
        require(unreal.BlueprintEditorLibrary.compile_blueprint(copies[source]), "Hero-connected Blueprint compile failed")
        check_engine_clean(bridge, "after_hero_compile:" + source)
    emit("hero_pin_changed", asset=plan[BASE], old=HERO_OLD, new=HERO_NEW, changed_pins=1)


def check_bundle(copies, plan, bridge, run, registry, options, patched, fresh):
    check_engine_clean(bridge, "before_bundle")
    originals = {source: unreal.load_asset(source) for source in plan}
    require(all(originals.values()), "An original core asset did not load")
    for source, asset in copies.items():
        require(asset.get_path_name() == plan[source] + "." + source.rsplit("/", 1)[1], "Copy identity mismatch")
        require(asset.get_class() == originals[source].get_class(), "Copy asset class mismatch")
        if isinstance(asset, unreal.Blueprint):
            require(unreal.BlueprintEditorLibrary.compile_blueprint(asset), "Copy compile failed: " + plan[source])
            check_engine_clean(bridge, "after_bundle_compile:" + source)
    index = graph_index()
    for source, asset in copies.items():
        original = originals[source]
        fields = DATA_FIELDS.get(source.rsplit("/", 1)[1], ())
        if source.rsplit("/", 1)[1].startswith("GA_"):
            fields += ABILITY_FIELDS
        before = {name: encode(owner(original).get_editor_property(name)) for name in fields}
        after = {name: encode(owner(asset).get_editor_property(name)) for name in fields}
        require(translate(before, plan) == after, "Selected data/fragment/cost mismatch: " + source)
        if isinstance(asset, unreal.Blueprint):
            original_parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(original)
            actual_parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
            require(translate(original_parent.get_path_name(), plan) == actual_parent.get_path_name(), "Parent remap mismatch: " + source)
            original_graph = graph_signature(original, index)
            expected = expected_graph(source, original_graph, plan, patched)
            actual = graph_signature(asset, index)
            require(expected == actual, "Graph node/pin/default/link mismatch: " + source)
            original_types, _ = type_report(original, bridge)
            copied_types, _ = type_report(asset, bridge)
            expected_types = expected_type_references(source, original_types, plan, patched, original_graph)
            if expected_types != sorted(copied_types):
                emit("type_reference_difference", source=source, patched=patched,
                     missing=sorted(set(expected_types) - set(copied_types)),
                     unexpected=sorted(set(copied_types) - set(expected_types)))
                raise RuntimeError("Macro graph/cast/pin type mismatch: " + source)
        if fresh:
            before_deps = registry.get_dependencies(source, options)
            after_deps = registry.get_dependencies(plan[source], options)
            require(before_deps is not None and after_deps is not None, "Registry dependencies unavailable")
            expected = expected_dependencies(source, before_deps, plan, patched)
            actual = {str(package) for package in after_deps}
            emit("reference_comparison", source=source, missing=sorted(expected - actual), unexpected=sorted(actual - expected))
            require(expected == actual, "Fresh package reference mismatch: " + source)
        emit("core_checked", source=source, destination=plan[source], selected_fields=len(fields), patched=patched, fresh=fresh)
    comparison = bridge.compare_inherited_overrides(run, plan)
    check_engine_clean(bridge, "after_override_comparison")
    emit("override_comparison", success=comparison.get_editor_property("succeeded"),
         expected=comparison.get_editor_property("expected_overrides"), compared=comparison.get_editor_property("compared_overrides"),
         messages=list(comparison.get_editor_property("messages")))
    require(comparison.get_editor_property("succeeded") and comparison.get_editor_property("expected_overrides") > 0,
            "Core inherited override comparison failed or did not inspect the Rifle")


def dependency_plan(registry, options, plan):
    pending, visited, external, unavailable = list(plan.values()), set(), set(), []
    while pending:
        package = pending.pop()
        if package in visited or package in external:
            continue
        if package.startswith(("/Engine/", "/Script/")):
            external.add(package)
            continue
        visited.add(package)
        dependencies = registry.get_dependencies(package, options)
        if dependencies is None:
            unavailable.append(package)
        else:
            pending.extend(str(item) for item in dependencies)
    mounts = {}
    collisions = []
    target = Path(__file__).resolve().parents[2]
    for package in visited:
        mount = package.split("/", 2)[1]
        mounts[mount] = mounts.get(mount, 0) + 1
        if mount == "Game" and package not in plan.values():
            if any(file_path(target, package, ext).exists() for ext in (".uasset", ".umap")):
                collisions.append(package)
    emit("dependency_plan", package_count=len(visited), mounts=mounts, engine_script_externals=len(external),
         unavailable=sorted(unavailable), original_core_references=sorted(set(plan) & visited),
         original_hero_reachable=HERO_PACKAGE in visited, existing_target_game_paths=sorted(collisions),
         note="Read-only package-reference plan; no dependency copies, overwrites, redirects, or Migrate")
    require(not unavailable, "Dependency plan contains unreadable registry entries")


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    require(os.path.normcase(str(project)) == os.path.normcase(str(expected.resolve())), "Original Lyra project required")
    mode = option(command, "DCRifleCoreMode", "preflight")
    require(mode in ("preflight", "inspect", "memory", "prepare", "verify", "plan"), "Unknown core mode")
    run = option(command, "DCRifleCoreRun", "")
    bridge = unreal.DCRifleMigrationLibrary
    require(all(hasattr(bridge, name) for name in ("get_approved_rifle_closure_packages", "get_known_rifle_load_dirty_packages",
                "get_protected_rifle_sibling_packages", "inspect_blueprint_type_references", "validate_diagnostic_context",
                "get_engine_ensure_failure_count")),
            "Rebuild the lifecycle/diagnostic-guard Editor plugin first")
    require(bridge.validate_diagnostic_context() is not None, "Diagnostic context rejected before loading")
    check_engine_clean(bridge, "startup")
    sources = list(bridge.get_approved_rifle_closure_packages())
    allowed = list(bridge.get_known_rifle_load_dirty_packages())
    siblings = list(bridge.get_protected_rifle_sibling_packages())
    require(len(allowed) == 4, "Known dependency policy changed; review before running")
    require(len(siblings) == 4 and not set(siblings).intersection(allowed), "Siblings must be protected, not dirty exceptions")
    plan = build_plan(sources, run)
    emit("begin", mode=mode, run=run, plan=plan, cpp_build=False, packaging=False, migration=False)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
    if mode == "preflight":
        require(bridge.validate_copy_plan(run, plan) is not None, "Preflight rejected")
        check_engine_clean(bridge, "preflight_complete")
        emit("complete", mode=mode, asset_writes=0)
        return
    watch = sources + allowed + siblings
    before = fingerprints(project, watch)
    if mode == "inspect":
        require(bridge.validate_copy_plan(run, plan) is not None, "Inspection preflight rejected")
        for source, (references, state) in original_snapshot(sources + siblings, bridge).items():
            emit("original_types", source=source, references=references, state=state)
        check_dirty_names(dirty_names(), sources + siblings, (), allowed)
        check_files(project, watch, before)
        check_engine_clean(bridge, "inspect_complete")
        emit("complete", mode=mode, asset_writes=0, compile=False, copy=False)
        return
    if mode in ("verify", "plan"):
        require(not dirty_names(), "Fresh verify process must start clean")
        saved_before = fingerprints(project, plan.values())
        original_before = original_snapshot(sources + siblings, bridge)
        check_dirty_names(dirty_names(), sources + siblings, (), allowed)
        copies = {source: unreal.load_asset(destination) for source, destination in plan.items()}
        require(all(copies.values()), "Saved core bundle is incomplete")
        check_bundle(copies, plan, bridge, run, registry, options, patched=True, fresh=True)
        dependency_plan(registry, options, plan)
        check_files(project, watch, before)
        check_files(project, plan.values(), saved_before)
        check_original_snapshot(original_before, bridge)
        check_dirty_names(dirty_names(), sources + siblings, plan.values(), allowed)
        check_engine_clean(bridge, "verify_complete")
        emit("complete", mode=mode, asset_writes=0, gameplay_verified=False)
        return

    for flag in ("DCRifleCopyDiagnostics", "DCRiflePreserveOverrides", "DCRifleAllowKnownLoadDirty"):
        require(re.search(r"(?:^|\s)-" + flag + r"(?:\s|$)", command), "Explicit flag required: " + flag)
    require(not dirty_names(), "Initial dirty packages are never allowed")
    require(bridge.validate_copy_plan(run, plan) is not None, "Fresh run preflight rejected")
    result = bridge.copy_package_subset(run, plan, False, True, True)
    emit("memory_copy_result", success=result.get_editor_property("succeeded"),
         engine_diagnostics_clean=result.get_editor_property("engine_diagnostics_clean"),
         ensure_failures=bridge.get_engine_ensure_failure_count(), messages=list(result.get_editor_property("messages")))
    check_engine_clean(bridge, "after_native_memory_copy")
    require(result.get_editor_property("succeeded") and result.get_editor_property("inherited_overrides_verified")
            and result.get_editor_property("engine_diagnostics_clean")
            and result.get_editor_property("scoped_copy_used") and result.get_editor_property("original_blueprints_unchanged")
            and result.get_editor_property("known_load_dirty_policy_verified") and not result.get_editor_property("saved_to_disk"),
            "Native guarded memory copy failed; do not save")
    require(set(result.get_editor_property("destinations")) == set(plan.values()), "Unexpected native output set")
    copies = {source: unreal.load_asset(destination) for source, destination in plan.items()}
    require(all(copies.values()), "Missing in-memory copy")
    # C++ already guarded all pre-existing loaded BPs through copy/repair. Extend protection
    # over Python's later validation/Hero edit/save, including the four original siblings.
    original_before = original_snapshot(sources + siblings, bridge)
    check_bundle(copies, plan, bridge, run, registry, options, patched=False, fresh=False)
    patch_hero(copies, plan, bridge)
    check_bundle(copies, plan, bridge, run, registry, options, patched=True, fresh=False)
    check_original_snapshot(original_before, bridge)
    save_guard(project, plan, allowed, siblings, before, bridge)
    if mode == "prepare":
        # No resume mode: these objects were created by this invocation after fresh-folder preflight.
        assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
        check_engine_clean(bridge, "immediately_before_python_save")
        require(assets.save_loaded_assets(list(copies.values()), False), "Explicit core save failed; retain partial output")
        check_engine_clean(bridge, "after_python_save")
        for destination in plan.values():
            require(file_path(project, destination).stat().st_size > 0, "Saved file missing or empty")
        check_dirty_names(dirty_names(), plan, (), allowed)
    check_files(project, watch, before)
    check_original_snapshot(original_before, bridge)
    check_engine_clean(bridge, "memory_or_prepare_complete")
    emit("complete", mode=mode, new_saved_assets=len(plan) if mode == "prepare" else 0,
         original_saves=0, fresh_process_reload=False, gameplay_verified=False,
         ensure_failures=bridge.get_engine_ensure_failure_count())


main()
