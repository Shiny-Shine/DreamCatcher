"""Scoped R5-1 editor automation. Never builds C++, packages, or edits existing game assets.

Run with UnrealEditor-Cmd and -run=pythonscript, then select
-DCAutomationMode=audit|prepare|verify (default: audit).
Only prepare may save one new Blueprint at ASSET_PATH. Existing assets are
validated, never overwritten. The smoke test uses an unsaved, transient editor
world; it is NOT a PIE, network, Equipment, or QuickBar test.
"""

import json
import os
from pathlib import Path
import re
import traceback

import unreal


ASSET_PATH = "/Game/DreamCatcher/GAS/Test/R5/Automation/ID_DC_R5_InventorySmoke"
STAT_TAG_NAME = "Test.R2.CostCharge"  # Existing project tag, only on the new item instance.
INITIAL_COUNT = 7
CLASS_PATHS = {
    "definition": "/Script/DreamCatcher.DCInventoryItemDefinition",
    "stats": "/Script/DreamCatcher.DCInventoryFragment_SetStats",
    "manager": "/Script/DreamCatcher.DCInventoryManagerComponent",
}


def report(event, **values):
    print("DC_AUTOMATION " + json.dumps({"event": event, **values}, ensure_ascii=False))


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def stat_tag():
    tag = unreal.GameplayTag()
    require(tag.import_text('(TagName="' + STAT_TAG_NAME + '")'), "Cannot import existing test tag")
    return tag


def audit():
    classes = {}
    for key, path in CLASS_PATHS.items():
        cls = unreal.load_class(None, path)
        require(cls is not None, "Class not loaded: " + path)
        classes[key] = cls
    definition = unreal.get_default_object(classes["definition"])
    stats = unreal.get_default_object(classes["stats"])
    manager = unreal.get_default_object(classes["manager"])
    report(
        "audit",
        classes=CLASS_PATHS,
        default_fragment_count=len(definition.get_editor_property("fragments")),
        default_stat_count=len(stats.get_editor_property("InitialItemStats")),
        manager_methods=[name for name in ("add_item_definition", "get_all_items", "remove_item_instance")
                         if callable(getattr(manager, name, None))],
        can_create_blueprint=hasattr(unreal.BlueprintEditorLibrary, "create_blueprint_asset_with_parent"),
        can_compile_blueprint=hasattr(unreal.BlueprintEditorLibrary, "compile_blueprint"),
        new_object_doc=unreal.new_object.__doc__,
        tag_text=stat_tag().export_text(),
    )
    return classes


def validate_asset(blueprint):
    require(blueprint is not None, "Missing test Blueprint")
    require(blueprint.get_path_name().split(".")[0] == ASSET_PATH, "Unexpected asset path")
    item_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    require(item_class is not None, "Missing generated class")
    cdo = unreal.get_default_object(item_class)
    fragments = cdo.get_editor_property("fragments")
    require(len(fragments) == 1, "Expected exactly one SetStats fragment")
    require(fragments[0].get_class().get_path_name() == CLASS_PATHS["stats"], "Unexpected fragment class")
    values = fragments[0].get_editor_property("InitialItemStats")
    require(len(values) == 1 and values[stat_tag()] == INITIAL_COUNT, "Unexpected initial item stats")
    report("asset_validated", asset=ASSET_PATH, initial_count=INITIAL_COUNT)
    return item_class


def prepare_asset(classes):
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    if assets.does_asset_exist(ASSET_PATH):
        blueprint = assets.load_asset(ASSET_PATH)
        validate_asset(blueprint)
        report("existing_asset_preserved", asset=ASSET_PATH)
        return blueprint

    blueprint = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(ASSET_PATH, classes["definition"])
    require(blueprint is not None, "Could not create test Blueprint")
    require(unreal.BlueprintEditorLibrary.compile_blueprint(blueprint), "Initial Blueprint compile failed")
    cdo = unreal.get_default_object(unreal.BlueprintEditorLibrary.generated_class(blueprint))
    fragment = unreal.new_object(classes["stats"], cdo, "InitialStats")
    fragment.set_editor_property("InitialItemStats", {stat_tag(): INITIAL_COUNT})
    cdo.set_editor_property("display_name", unreal.Text("R5 Inventory Automation Test"))
    cdo.set_editor_property("fragments", [fragment])
    require(unreal.BlueprintEditorLibrary.compile_blueprint(blueprint), "Configured Blueprint compile failed")
    validate_asset(blueprint)
    require(assets.save_loaded_asset(blueprint, False), "Saving the allowlisted test asset failed")
    report("asset_saved", asset=ASSET_PATH)
    return blueprint


def smoke_test(classes, item_class):
    # Isolated commandlet only. Never run this against the user's active editor map.
    world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    require(world is not None, "Could not create transient editor world")
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actor = actors.spawn_actor_from_class(unreal.Actor, unreal.Vector(), unreal.Rotator(), True)
    require(actor is not None, "Could not spawn transient test owner")
    try:
        # AddComponentByClass is ScriptNoExport in this engine. An actor-owned
        # transient UObject component is sufficient for these inventory unit calls.
        manager = unreal.new_object(classes["manager"], actor, "InventorySmokeComponent")
        require(manager.get_owner() == actor, "Inventory component has the wrong owner")
        require(actor.has_authority(), "Test owner must have authority")
        require(len(manager.get_all_items()) == 0, "Test inventory was not empty")
        item = manager.add_item_definition(item_class, 1)
        require(item is not None and len(manager.get_all_items()) == 1, "Item creation failed")
        tag = stat_tag()
        require(item.get_stat_tag_stack_count(tag) == INITIAL_COUNT, "SetStats did not initialize the item")
        item.add_stat_tag_stack(tag, 3)
        require(item.get_stat_tag_stack_count(tag) == 10, "Stat addition failed")
        item.remove_stat_tag_stack(tag, 4)
        require(item.get_stat_tag_stack_count(tag) == 6, "Stat removal failed")
        require(manager.find_first_item_stack_by_definition(item_class) == item, "Item lookup failed")
        manager.remove_item_instance(item)
        require(len(manager.get_all_items()) == 0, "Item removal failed")
        report("smoke_pass", context="transient_editor_world_not_PIE", checks=[
            "create", "initial_stats_7", "add_stats_10", "remove_stats_6", "lookup", "remove_item"
        ])
    finally:
        require(actors.destroy_actor(actor), "Transient test actor cleanup failed")


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command_line.lower(), "Use a separate Python commandlet, not an interactive editor")
    expected_root = Path(__file__).resolve().parents[2]
    project_root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(expected_root)) == os.path.normcase(str(project_root)), "Wrong project workspace")
    match = re.search(r"(?:^|\s)-DCAutomationMode=(audit|prepare|verify)(?:\s|$)", command_line)
    mode = match.group(1) if match else "audit"
    classes = audit()
    if mode == "audit":
        report("complete", mode=mode, asset_writes=0)
        return
    if mode == "prepare":
        blueprint = prepare_asset(classes)
    else:
        blueprint = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).load_asset(ASSET_PATH)
    item_class = validate_asset(blueprint)
    smoke_test(classes, item_class)
    report("complete", mode=mode, allowed_asset=ASSET_PATH, cpp_build=False, packaging=False, PIE=False)


try:
    main()
except Exception:
    report("failed", traceback=traceback.format_exc())
    raise
