"""Read-only macro/parent audit after core memory-copy rejection. No Compile/save/copy."""
import json
import os
from pathlib import Path
import unreal


def emit(event, **values):
    print("DC_RIFLE_REFERENCES " + json.dumps({"event": event, **values}, ensure_ascii=False))


def require(value, message):
    if not value:
        raise RuntimeError(message)


def audit_blueprint(package):
    asset = unreal.load_asset(package)
    require(asset is not None, "Missing original: " + package)
    if not isinstance(asset, unreal.Blueprint):
        return
    parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
    graphs = unreal.BlueprintEditorLibrary.list_graphs(asset)
    emit("blueprint", package=package, parent=parent.get_path_name() if parent else None,
         graphs=[graph.get_name() for graph in graphs])
    graph_paths = {graph.get_path_name() for graph in graphs}
    for node in unreal.ObjectIterator(unreal.K2Node):
        if not node.get_outer() or node.get_outer().get_path_name() not in graph_paths:
            continue
        title = unreal.BlueprintEditorLibrary.get_node_title(node)
        if "MacroInstance" in node.get_class().get_name():
            try:
                reference = node.get_editor_property("MacroGraphReference").export_text()
                emit("macro_reference", package=package, node=node.get_name(), title=title, reference=reference)
            except Exception as error:
                emit("macro_reference_unverified", package=package, node=node.get_name(), reason=str(error))
        if "weapon" in title.lower() or "Weapon" in node.get_outer().get_name():
            pin_data = []
            for pin in unreal.BlueprintEditorLibrary.list_all_pins(node):
                data = {"name": str(pin.get_pin_name()), "value": pin.get_pin_value(),
                        "type": str(pin.get_pin_type_display_string()), "schema": pin.get_pin_type_as_json_schema()}
                try:
                    subtype = pin.get_pin_type().get_editor_property("PinSubCategoryObject")
                    data["subtype_object"] = subtype.get_path_name() if subtype else None
                except Exception as error:
                    data["subtype_unverified"] = str(error)
                pin_data.append(data)
            emit("weapon_node", package=package, graph=node.get_outer().get_name(), node=node.get_name(), title=title,
                 pins=pin_data)


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate commandlet required")
    expected = Path(__file__).resolve().parents[2].parent / "LyraStarterGame"
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(project)) == os.path.normcase(str(expected)), "Original project required")
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
    cue = "/ShooterCore/Weapons/Rifle/GCN_Weapon_Rifle_Fire"
    deps = registry.get_dependencies(cue, options)
    require(deps is not None, "Cue dependencies unavailable")
    emit("cue_dependencies", package=cue, dependencies=sorted(str(item) for item in deps))
    audit_blueprint(cue)
    for dependency in deps:
        package = str(dependency)
        if any(word in package.lower() for word in ("macro", "functions")) and package.startswith(("/Game/", "/ShooterCore/")):
            references = registry.get_dependencies(package, options)
            emit("library_dependencies", package=package, dependencies=None if references is None else sorted(str(item) for item in references))
            audit_blueprint(package)
    for package in ("/ShooterCore/Weapons/Pistol/B_Pistol", "/ShooterCore/Weapons/Pistol/B_WeaponInstance_Pistol",
                    "/ShooterCore/Weapons/Pistol/GA_Weapon_Fire_Pistol", "/Game/Weapons/Pistol/GA_Weapon_Reload_Pistol"):
        asset = unreal.load_asset(package)
        require(asset is not None, "Missing comparison parent: " + package)
        parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
        emit("original_parent", package=package, parent=parent.get_path_name())
    emit("complete", asset_writes=0, compile=False, copy=False)


main()
