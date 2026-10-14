"""Read-only original Lyra rifle audit. No save, compile, migrate, reparent or property edits.

Run in LyraStarterGame with a separate Python commandlet. Output goes to the
Unreal log, not to assets. Unreadable fields/graphs are reported as unverified.
"""

import json
import os
from collections import deque
from pathlib import Path
import re

import unreal


RIFLE_ROOT = "/ShooterCore/Weapons/Rifle/"
ROOTS = [RIFLE_ROOT + name for name in (
    "ID_Rifle", "WID_Rifle", "AbilitySet_ShooterRifle", "B_WeaponInstance_Rifle",
    "B_Rifle", "GA_Weapon_Fire_Rifle_Auto", "GA_Weapon_Reload_Rifle",
)]
ROOTS += ["/Game/Weapons/GA_Weapon_AutoReload", RIFLE_ROOT + "W_Reticle_Rifle", RIFLE_ROOT + "W_AmmoCounter_Rifle"]
FIELDS = {
    "ID_Rifle": ["DisplayName", "Fragments"],
    "WID_Rifle": ["InstanceType", "AbilitySetsToGrant", "ActorsToSpawn"],
    "AbilitySet_ShooterRifle": ["GrantedGameplayAbilities", "GrantedGameplayEffects", "GrantedAttributes"],
    "B_WeaponInstance_Rifle": [
        "SpreadRecoveryCooldownDelay", "HeatToSpreadCurve", "HeatToHeatPerShotCurve", "HeatToCoolDownPerSecondCurve",
        "bAllowFirstShotAccuracy", "SpreadExponent", "SpreadAngleMultiplier_Aiming", "SpreadAngleMultiplier_StandingStill",
        "SpreadAngleMultiplier_Crouching", "SpreadAngleMultiplier_JumpingOrFalling", "BulletsPerCartridge",
        "MaxDamageRange", "BulletTraceSweepRadius", "DistanceDamageFalloff", "MaterialDamageMultiplier",
        "EquippedAnimSet", "UneuippedAnimSet", "ApplicableDeviceProperties",
    ],
}
ABILITY_FIELDS = ["ActivationPolicy", "AdditionalCosts", "CostGameplayEffectClass", "CooldownGameplayEffectClass",
                  "InstancingPolicy", "NetExecutionPolicy", "AbilityTriggers", "ActivationOwnedTags"]


def emit(event, **values):
    print("DC_RIFLE_AUDIT " + json.dumps({"event": event, **values}, ensure_ascii=False))


def serialize(value):
    if value is None or isinstance(value, (str, bool, int, float)):
        return value
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if callable(getattr(value, "export_text", None)):
        return value.export_text()
    if isinstance(value, (list, tuple, unreal.Array)):
        return [serialize(item) for item in value]
    if isinstance(value, (dict, unreal.Map)):
        return {str(serialize(key)): serialize(item) for key, item in value.items()}
    return str(value)


def read_fields(owner, names):
    fields, unavailable = {}, {}
    for name in dict.fromkeys(names):
        try:
            fields[name] = serialize(owner.get_editor_property(name))
        except Exception as error:
            unavailable[name] = str(error)
    return fields, unavailable


def graphs(blueprint, package):
    try:
        # Graph.Nodes is not readable through Python reflection. Enumerate loaded node objects
        # by their graph outer instead; this does not edit the graph or change access flags.
        by_outer = {}
        for node in unreal.ObjectIterator(unreal.EdGraphNode):
            outer = node.get_outer()
            if outer:
                by_outer.setdefault(outer.get_path_name(), []).append(node)
        for graph in unreal.BlueprintEditorLibrary.list_graphs(blueprint):
            nodes = []
            for node in by_outer.get(graph.get_path_name(), []):
                data = {"name": node.get_name(), "class": node.get_class().get_path_name()}
                if "K2Node" in node.get_class().get_name():
                    try:
                        data["title"] = unreal.BlueprintEditorLibrary.get_node_title(node)
                        data["pins"] = []
                        for pin in unreal.BlueprintEditorLibrary.list_all_pins(node):
                            data["pins"].append({
                                "name": str(pin.get_pin_name()), "direction": str(pin.get_pin_direction()),
                                "value": pin.get_pin_value(), "type": str(pin.get_pin_type_display_string()),
                                "links": [{"node": other.get_owning_node().get_name(), "pin": str(other.get_pin_name())}
                                          for other in pin.list_connected_pins()],
                            })
                    except Exception as error:
                        data["unverified"] = str(error)
                nodes.append(data)
            emit("graph", asset=package, name=graph.get_name(), enumeration="loaded_nodes_by_graph_outer", nodes=nodes)
    except Exception as error:
        emit("graph_unverified", asset=package, reason=str(error))


def dependency_plan(registry, options):
    # Package-reference closure only. Engine and script packages are external; no migration is performed.
    # The original Hero reference is deliberately not removed from this inventory.
    seeds = ROOTS + [RIFLE_ROOT + "GCN_Weapon_Rifle_Fire"]
    pending, visited, external = deque(seeds), set(), set()
    while pending:
        package = pending.popleft()
        if package in visited:
            continue
        if package.startswith(("/Engine/", "/Script/")):
            external.add(package)
            continue
        if len(visited) >= 12000:
            raise RuntimeError("Dependency limit reached; inventory incomplete")
        visited.add(package)
        dependencies = sorted(str(value) for value in registry.get_dependencies(package, options))
        asset_data = registry.get_assets_by_package_name(package)
        records = []
        for data in asset_data:
            records.append({"name": str(data.asset_name), "class": str(data.asset_class_path),
                            "native_parent": serialize(data.get_tag_value("NativeParentClass")),
                            "parent": serialize(data.get_tag_value("ParentClass"))})
        emit("dependency_package", package=package, assets=records, dependencies=dependencies)
        pending.extend(dependencies)
    emit("dependency_complete", roots=seeds, packages=len(visited), external_packages=len(external),
         asset_writes=0, is_migration_manifest=False,
         scope="hard_and_soft_package_references; original_Hero_still_referenced; dynamic_tag_lookup_not_exhaustive")


def main():
    if "-run=pythonscript" not in unreal.SystemLibrary.get_command_line().lower():
        raise RuntimeError("Separate read-only Python commandlet required")
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    actual = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    if os.path.normcase(str(expected)) != os.path.normcase(str(actual)):
        raise RuntimeError("Run this source audit only in the sibling LyraStarterGame project")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions()
    if "-DCDependencyPlan" in unreal.SystemLibrary.get_command_line():
        dependency_plan(registry, options)
        return
    pending, visited = list(ROOTS), set()
    while pending:
        package = pending.pop(0)
        if package in visited:
            continue
        if len(visited) >= 24:
            emit("unverified", reason="Parent traversal limit", remaining=pending + [package])
            break
        visited.add(package)
        asset = unreal.load_asset(package)
        if asset is None:
            emit("unverified", asset=package, reason="Asset did not load")
            continue
        dependencies = sorted(str(value) for value in registry.get_dependencies(package, options))
        blueprint = asset if isinstance(asset, unreal.Blueprint) else None
        owner = asset
        parent = None
        names = list(FIELDS.get(asset.get_name(), []))
        if blueprint:
            cls = unreal.BlueprintEditorLibrary.generated_class(blueprint)
            parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(blueprint)
            owner = unreal.get_default_object(cls)
            for name in unreal.BlueprintEditorLibrary.list_member_variable_names(blueprint, True):
                name = str(name)
                if not name.startswith("/Script/"):
                    names.append(re.split(r"[:.]", name)[-1])
            if asset.get_name().startswith("GA_"):
                names += ABILITY_FIELDS
            if parent and parent.get_path_name().startswith(("/Game/", "/ShooterCore/")):
                pending.append(parent.get_path_name().split(".")[0])
        fields, unavailable = read_fields(owner, names)
        emit("asset", asset=package, native_type=asset.get_class().get_path_name(),
             parent=serialize(parent), values=fields, unverified_fields=unavailable, dependencies=dependencies)
        if "Fragments" in names:
            for fragment in owner.get_editor_property("Fragments"):
                props, missing = read_fields(fragment, ["InitialItemStats", "EquipmentDefinition", "ReticleWidgets",
                                                       "Brush", "AmmoBrush", "DisplayNameWhenEquipped",
                                                       "SkeletalMesh", "DisplayName", "PadColor"])
                emit("fragment", asset=package, type=fragment.get_class().get_path_name(), values=props)
        if "AdditionalCosts" in names:
            for cost in owner.get_editor_property("AdditionalCosts"):
                props, missing = read_fields(cost, ["Quantity", "Tag", "FailureTag", "bOnlyApplyCostOnHit"])
                emit("cost", asset=package, type=cost.get_class().get_path_name(), values=props, unverified_fields=missing)
        if blueprint:
            graphs(blueprint, package)
    emit("complete", inspected=len(visited), asset_writes=0, cpp_build=False, packaging=False,
         runtime_validation=False)


main()
