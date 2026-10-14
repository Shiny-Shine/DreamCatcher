"""Approved R5 rifle staging, only under /Game/LyraMigration/Rifle.

Separate LyraStarterGame Python commandlet. Never builds C++, modifies original
assets, consolidates assets, or edits active DreamCatcher gameplay.
Modes: seeds (owned core copies), inspect/plan (read only), copy (experimental).
IMPORTANT: both full-load save attempts crashed and header patching was incomplete.
Existing Content/Payload/Prepared folders are diagnostic partial output, NOT migration-ready.
The script deliberately refuses to rerun copy over an existing destination.
"""
import json
import os
import re
from collections import deque
from pathlib import Path
import unreal

ROOT = "/Game/LyraMigration/Rifle"
SEED = ROOT + "/Seed"
SOURCES = [
    "/ShooterCore/Weapons/B_WeaponInstance_Base",
    "/ShooterCore/Weapons/Rifle/B_WeaponInstance_Rifle",
    "/Game/Weapons/B_Weapon", "/ShooterCore/Weapons/Rifle/B_Rifle",
    "/Game/Weapons/GA_Weapon_Fire", "/ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto",
    "/Game/Weapons/GA_Weapon_ReloadMagazine", "/ShooterCore/Weapons/Rifle/GA_Weapon_Reload_Rifle",
    "/Game/Weapons/GA_Weapon_AutoReload",
    "/ShooterCore/Weapons/Rifle/W_Reticle_Rifle", "/ShooterCore/Weapons/Rifle/W_AmmoCounter_Rifle",
    "/ShooterCore/Weapons/Rifle/AbilitySet_ShooterRifle", "/ShooterCore/Weapons/Rifle/WID_Rifle",
    "/ShooterCore/Weapons/Rifle/ID_Rifle",
    "/ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire", "/Game/GameplayCueNotifies/GCN_Weapon_Impact",
]
DEST = {source: SEED + "/" + source.rsplit("/", 1)[1] for source in SOURCES}
# Reparenting an Actor Blueprint drops inherited SCS component overrides in this engine.
# Keep a fresh B_Rifle copy's original parent until Advanced Copy remaps the whole hierarchy.
DEST["/ShooterCore/Weapons/Rifle/B_Rifle"] = SEED + "/Preserved/B_Rifle"
OWNER_KEY = "DC.R5.RifleSource"

def emit(event, **values):
    print("DC_RIFLE_STAGE " + json.dumps({"event": event, **values}, ensure_ascii=False))

def require(value, message):
    if not value:
        raise RuntimeError(message)

def generated(asset):
    value = unreal.BlueprintEditorLibrary.generated_class(asset)
    require(value is not None, "No generated class: " + asset.get_path_name())
    return value

def replace_paths(text):
    text = text.replace(SEED + "/B_Rifle.", DEST["/ShooterCore/Weapons/Rifle/B_Rifle"] + ".")
    for source in sorted(DEST, key=len, reverse=True):
        text = text.replace(source + ".", DEST[source] + ".")
    return text

def translated(value):
    if isinstance(value, unreal.Object):
        path = value.get_path_name()
        updated = replace_paths(path)
        if path != updated:
            result = unreal.load_object(None, updated)
            require(result is not None, "Missing staged reference: " + updated)
            return result
    if callable(getattr(value, "export_text", None)):
        text = value.export_text()
        updated = replace_paths(text)
        if text != updated:
            result = type(value)()
            require(result.import_text(updated), "Struct reference remap failed")
            return result
    if isinstance(value, (list, tuple, unreal.Array)):
        return [translated(item) for item in value]
    return value

def patch_hero_pin(asset):
    graph = unreal.BlueprintEditorLibrary.find_event_graph(asset)
    old = "/ShooterCore/Game/B_Hero_ShooterMannequin.B_Hero_ShooterMannequin_C"
    new = "/Script/LyraGame.LyraCharacter"
    found = 0
    for node in unreal.ObjectIterator(unreal.K2Node):
        if node.get_outer() != graph:
            continue
        if unreal.BlueprintEditorLibrary.get_node_title(node) != "GetTypedPawn":
            continue
        pin = unreal.BlueprintEditorLibrary.find_input_pin(node, "PawnType")
        if pin.is_valid() and pin.get_pin_value() in (old, new):
            require(pin.set_pin_value(new), "Could not set native Character type")
            require(pin.get_pin_value() == new, "Hero pin did not change")
            found += 1
    require(found == 1, "Expected exactly one approved Hero type pin")
    emit("hero_pin", asset=asset.get_path_name(), old=old, new=new, changed_pins=found)

def inspect(registry):
    options = unreal.AssetRegistryDependencyOptions()
    for source, dest in DEST.items():
        asset = unreal.load_asset(dest)
        require(asset is not None, "Missing seed: " + dest)
        require(unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).get_metadata_tag(asset, OWNER_KEY) == source,
                "Seed is not owned by this migration: " + dest)
        parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset) if isinstance(asset, unreal.Blueprint) else None
        deps = registry.get_dependencies(dest, options)
        emit("seed", source=source, dest=dest, parent=parent.get_path_name() if parent else None,
             dependencies=sorted(str(p) for p in deps) if deps is not None else None,
             requires_fresh_registry_scan=deps is None)

def plan(registry):
    pending, seen, project = deque(DEST.values()), set(), []
    options = unreal.AssetRegistryDependencyOptions()
    while pending:
        package = str(pending.popleft())
        if package in seen or package.startswith(("/Engine/", "/Script/")):
            continue
        seen.add(package)
        if package.startswith(("/Game/", "/ShooterCore/")):
            project.append(package)
        deps = registry.get_dependencies(package, options)
        require(deps is not None, "Dependency registry not ready: " + package)
        pending.extend(deps)
    for package in sorted(project):
        emit("planned_package", package=package)
    emit("plan", project_packages=len(project), total_including_engine_plugins=len(seen),
         original_hero_referenced="/ShooterCore/Game/B_Hero_ShooterMannequin" in seen)

def copy_dependencies(assets, registry):
    target = ROOT + "/Prepared"
    require(not assets.does_directory_exist(target), "Copy destination must be new: " + target)
    dirty = list(unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    dirty += list(unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())
    require(not dirty, "Refusing Advanced Copy while packages are dirty: " + str([p.get_path_name() for p in dirty]))
    # A fresh process reaches this without loading original Blueprints. Advanced Copy's save-dirty
    # preamble has nothing to save, and its entire generated destination map is under target.
    completion = []
    held_assets = {}
    def finished(success, copied):
        completion.append(bool(success))
        for record in copied:
            obj = record.asset
            if obj is None:
                continue
            path = obj.get_path_name()
            package = path.split(".")[0]
            if path == package + "." + package.rsplit("/", 1)[1]:
                require(package.startswith(target + "/"), "Copy callback escaped root")
                held_assets[package] = obj
        emit("copy_callback", success=bool(success), object_records=len(copied))
    callback = unreal.AdvancedCopyCompletedEvent()
    callback.bind_callable(finished)
    unreal.SystemLibrary.execute_console_command(None, "AssetTools.UseHeaderPatchingAdvancedCopy 0")
    unreal.AssetToolsHelpers.get_asset_tools().begin_advanced_copy_packages([SEED], target, callback)
    require(completion == [True], "Advanced Copy did not report successful completion")
    require(held_assets, "No strong asset references received from Advanced Copy")
    emit("batch_save_begin", assets=len(held_assets))
    require(assets.save_loaded_assets(list(held_assets.values()), False), "Batch saving copied assets failed")
    registry.scan_paths_synchronous([target], True)
    copied_paths = assets.list_assets(target, True, False)
    require(copied_paths, "No copied assets found")
    for path in copied_paths:
        require(str(path).startswith(target + "/"), "Copy outside permitted root")
    emit("copied", root=target, assets=len(copied_paths))

def seeds(assets):
    loaded = {}
    # Duplicate first, then remap, so every required replacement exists.
    for source, dest in DEST.items():
        if assets.does_asset_exist(dest):
            asset = assets.load_asset(dest)
            require(assets.get_metadata_tag(asset, OWNER_KEY) == source, "Refusing to overwrite unowned asset: " + dest)
        else:
            original = unreal.load_asset(source)
            require(original is not None, "Source unavailable: " + source)
            asset = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(dest.rsplit("/", 1)[1], dest.rsplit("/", 1)[0], original)
            require(asset is not None, "Duplicate failed: " + source)
            assets.set_metadata_tag(asset, OWNER_KEY, source)
        loaded[source] = asset
    for source in SOURCES:
        asset = loaded[source]
        name = source.rsplit("/", 1)[1]
        if isinstance(asset, unreal.Blueprint):
            parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
            original_parent = parent.get_path_name().split(".")[0] if parent else ""
            if original_parent in loaded and name != "B_Rifle":
                unreal.BlueprintEditorLibrary.reparent_blueprint(asset, generated(loaded[original_parent]))
            if name == "B_WeaponInstance_Base":
                patch_hero_pin(asset)
            require(unreal.BlueprintEditorLibrary.compile_blueprint(asset), "Seed compile failed: " + name)
            cdo = unreal.get_default_object(generated(asset))
            if name == "WID_Rifle":
                for field in ("InstanceType", "AbilitySetsToGrant", "ActorsToSpawn"):
                    cdo.set_editor_property(field, translated(cdo.get_editor_property(field)))
            elif name == "ID_Rifle":
                for fragment in cdo.get_editor_property("Fragments"):
                    kind = fragment.get_class().get_name()
                    if "EquippableItem" in kind:
                        fragment.set_editor_property("EquipmentDefinition", translated(fragment.get_editor_property("EquipmentDefinition")))
                    elif "ReticleConfig" in kind:
                        fragment.set_editor_property("ReticleWidgets", translated(fragment.get_editor_property("ReticleWidgets")))
            require(unreal.BlueprintEditorLibrary.compile_blueprint(asset), "Configured seed compile failed: " + name)
        else:
            for field in ("GrantedGameplayAbilities", "GrantedGameplayEffects", "GrantedAttributes"):
                asset.set_editor_property(field, translated(asset.get_editor_property(field)))
        require(asset.get_path_name().startswith(SEED + "/"), "Save outside allowlist")
        require(assets.save_loaded_asset(asset, False), "Seed save failed: " + name)
        emit("saved", asset=asset.get_path_name())

def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    actual = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(expected)) == os.path.normcase(str(actual)), "Source project required")
    match = re.search(r"(?:^|\s)-DCRifleStage=(\S+)", command)
    mode = match.group(1) if match else "inspect"
    require(mode in ("seeds", "inspect", "plan", "copy"), "Unknown mode")
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    if mode == "seeds":
        seeds(assets)
    if mode == "copy":
        copy_dependencies(assets, registry)
    else:
        inspect(registry)
        if mode == "plan":
            plan(registry)
    emit("complete", mode=mode, allowed_root=ROOT, original_asset_writes=0, cpp_build=False, packaging=False)

main()
