"""Prepare only four isolated R5 Equipment fixtures, after the user's C++ build.

Run in a separate UnrealEditor-Cmd Python commandlet with
-DCAutomationMode=audit|prepare|verify (default audit). No C++ build, packaging,
map edits, redirects, production asset changes, or gameplay test execution.
Run verify in a fresh process, then run DreamCatcher.R5.Equipment.QuickBarLifecycle
with the Unreal automation framework in another process using -DCR5IsolatedAutomation.
Fixture validation is NOT a gameplay pass.
Existing fixtures are validated, never overwritten or deleted.
"""

import json
import os
from pathlib import Path
import re
import traceback

import unreal


ROOT = "/Game/DreamCatcher/GAS/Test/R5/Automation/Equipment"
ASSETS = {
    "ability": ROOT + "/GA_DC_R5_EquipmentSmoke",
    "ability_set": ROOT + "/AS_DC_R5_EquipmentSmoke",
    "equipment": ROOT + "/WID_DC_R5_EquipmentSmoke",
    "item": ROOT + "/ID_DC_R5_EquipmentSmoke",
}
CLASS_PATHS = {
    "ability": "/Script/DreamCatcher.DCLyraGameplayAbility_FromEquipment",
    "ability_set": "/Script/DreamCatcher.DCAbilitySet",
    "equipment": "/Script/DreamCatcher.DCLyraEquipmentDefinition",
    "weapon": "/Script/DreamCatcher.DCLyraWeaponInstance",
    "item": "/Script/DreamCatcher.DCInventoryItemDefinition",
    "equippable": "/Script/DreamCatcher.DCInventoryFragment_EquippableItem",
    "stats": "/Script/DreamCatcher.DCInventoryFragment_SetStats",
    "equipment_actor": "/Script/Engine.SkeletalMeshActor",
    "manager": "/Script/DreamCatcher.DCLyraEquipmentManagerComponent",
    "quick_bar": "/Script/DreamCatcher.DCLyraQuickBarComponent",
    "quick_bar_icon": "/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon",
    "pickup_icon": "/Script/DreamCatcher.DCInventoryFragment_PickupIcon",
    "character_parts": "/Script/DreamCatcher.DCLyraPawnComponent_CharacterParts",
}
STAT_TAG_NAME = "Test.R2.CostCharge"  # Only a test item stack, not player costs/ammo balance.


def report(event, **values):
    print("DC_R5_EQUIPMENT " + json.dumps({"event": event, **values}, ensure_ascii=False))


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def tag():
    value = unreal.GameplayTag()
    require(value.import_text('(TagName="' + STAT_TAG_NAME + '")'), "Existing test tag unavailable")
    return value


def generated_class(blueprint):
    result = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    require(result is not None, "Blueprint has no generated class")
    return result


def make_struct(native_name):
    native = unreal.load_object(None, "/Script/DreamCatcher." + native_name)
    require(native is not None, "Missing native struct: " + native_name)
    wrapper = unreal.get_type_from_struct(native)
    require(wrapper is not None, "Python wrapper unavailable: " + native_name)
    return wrapper()


def validate(key, asset, classes, prepared):
    require(asset is not None, "Missing fixture: " + ASSETS[key])
    require(asset.get_path_name().split(".")[0] == ASSETS[key], "Fixture outside exact allowlist")
    if key == "ability_set":
        require(asset.get_class() == classes[key], "Unexpected AbilitySet class")
        entries = asset.get_editor_property("GrantedGameplayAbilities")
        require(len(entries) == 1, "Expected exactly one granted Ability")
        require(entries[0].get_editor_property("Ability") == generated_class(prepared["ability"]), "Wrong granted Ability")
        require(entries[0].get_editor_property("AbilityLevel") == 1, "Wrong Ability level")
        # GameplayTag wrappers do not provide value equality; compare Unreal's serialized value.
        require(entries[0].get_editor_property("InputTag").export_text() == unreal.GameplayTag().export_text(), "Unexpected input binding")
        require(len(asset.get_editor_property("GrantedGameplayEffects")) == 0, "Unexpected effects")
        require(len(asset.get_editor_property("GrantedAttributes")) == 0, "Unexpected attributes")
    else:
        # ParentClass is protected; use the UE 5.8 editor API rather than reflected property access.
        require(unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset) == classes[key], "Unexpected Blueprint parent")
        cdo = unreal.get_default_object(generated_class(asset))
        if key == "ability":
            require(cdo.get_editor_property("InstancingPolicy") == unreal.GameplayAbilityInstancingPolicy.INSTANCED_PER_ACTOR,
                    "Fixture Ability must be instanced per actor")
            require(cdo.get_editor_property("ActivationPolicy") == unreal.DCAbilityActivationPolicy.ON_INPUT_TRIGGERED,
                    "Fixture must not auto-activate during grant")
        elif key == "equipment":
            require(cdo.get_editor_property("InstanceType") == classes["weapon"], "Wrong WeaponInstance hierarchy")
            require(list(cdo.get_editor_property("AbilitySetsToGrant")) == [prepared["ability_set"]], "Wrong AbilitySet")
            actors = cdo.get_editor_property("ActorsToSpawn")
            require(len(actors) == 1, "Expected one equipment Actor")
            require(actors[0].get_editor_property("ActorToSpawn") == classes["equipment_actor"], "Wrong Actor class")
            require(str(actors[0].get_editor_property("AttachSocket")) == "None", "Unexpected socket")
            transform = actors[0].get_editor_property("AttachTransform")
            require(transform.translation == unreal.Vector(10.0, 20.0, 30.0), "Wrong test attachment offset")
            require(transform.scale3d == unreal.Vector(1.0, 1.0, 1.0), "Wrong test attachment scale")
        elif key == "item":
            fragments = cdo.get_editor_property("Fragments")
            require(len(fragments) == 2, "Expected EquippableItem and SetStats only")
            require(fragments[0].get_class() == classes["equippable"], "Wrong equipment fragment")
            require(fragments[0].get_editor_property("EquipmentDefinition") == generated_class(prepared["equipment"]),
                    "Wrong EquipmentDefinition")
            require(fragments[1].get_class() == classes["stats"], "Wrong stats fragment")
            stats = fragments[1].get_editor_property("InitialItemStats")
            require(len(stats) == 1 and stats[tag()] == 7, "Wrong item stat initialization")
    report("fixture_validated", asset=ASSETS[key])


def create_fixture(key, classes, prepared, assets):
    path = ASSETS[key]
    if assets.does_asset_exist(path):
        asset = assets.load_asset(path)
        validate(key, asset, classes, prepared)
        report("existing_fixture_preserved", asset=path)
        return asset

    if key == "ability_set":
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", classes[key])
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit("/", 1)[1], ROOT, classes[key], factory)
        require(asset is not None, "AbilitySet creation failed")
        entry = make_struct("DCAbilitySet_GameplayAbility")
        # Python array access returns a detached struct, whose EditDefaultsOnly setters reject edits.
        # Import defaults through Unreal's struct API, then assign to the new DataAsset and validate.
        ability_path = generated_class(prepared["ability"]).get_path_name()
        require(entry.import_text('(Ability="' + ability_path + '",AbilityLevel=1)'), "Ability entry import failed")
        asset.set_editor_property("GrantedGameplayAbilities", [entry])
    else:
        asset = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(path, classes[key])
        require(asset is not None, "Blueprint creation failed: " + path)
        require(unreal.BlueprintEditorLibrary.compile_blueprint(asset), "Initial Blueprint compile failed")
        cdo = unreal.get_default_object(generated_class(asset))
        if key == "equipment":
            cdo.set_editor_property("InstanceType", classes["weapon"])
            cdo.set_editor_property("AbilitySetsToGrant", [prepared["ability_set"]])
            actor = make_struct("DCLyraEquipmentActorToSpawn")
            actor.set_editor_property("ActorToSpawn", classes["equipment_actor"])
            actor.set_editor_property("AttachTransform", unreal.Transform(location=unreal.Vector(10.0, 20.0, 30.0)))
            cdo.set_editor_property("ActorsToSpawn", [actor])
        elif key == "item":
            equipment = unreal.new_object(classes["equippable"], cdo, "Equipment")
            equipment.set_editor_property("EquipmentDefinition", generated_class(prepared["equipment"]))
            stats = unreal.new_object(classes["stats"], cdo, "InitialStats")
            stats.set_editor_property("InitialItemStats", {tag(): 7})
            cdo.set_editor_property("DisplayName", unreal.Text("R5 Equipment Automation Test"))
            cdo.set_editor_property("Fragments", [equipment, stats])
        require(unreal.BlueprintEditorLibrary.compile_blueprint(asset), "Configured Blueprint compile failed")

    validate(key, asset, classes, prepared)
    require(assets.save_loaded_asset(asset, False), "Saving fixture failed: " + path)
    report("fixture_saved", asset=path)
    return asset


def main():
    command_line = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command_line.lower(), "Separate commandlet required; never run in the interactive editor")
    expected_root = Path(__file__).resolve().parents[2]
    project_root = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(expected_root)) == os.path.normcase(str(project_root)), "Wrong project workspace")
    match = re.search(r"(?:^|\s)-DCAutomationMode=(\S+)", command_line)
    mode = match.group(1) if match else "audit"
    require(mode in ("audit", "prepare", "verify"), "Unknown automation mode")
    classes = {key: unreal.load_class(None, path) for key, path in CLASS_PATHS.items()}
    for key, cls in classes.items():
        require(cls is not None, "Class unavailable; user C++ build required: " + CLASS_PATHS[key])
    part_types = {}
    for name in ("DCLyraCharacterPart", "DCLyraCharacterPartHandle", "DCLyraAppliedCharacterPartEntry",
                 "DCLyraCharacterPartList", "EDCLyraCharacterCustomizationCollisionMode"):
        native_type = unreal.load_object(None, "/Script/DreamCatcher." + name)
        require(native_type is not None, "Missing CharacterParts reflection type: " + name)
        part_types[name] = native_type.get_class().get_name()
    parts_cdo = unreal.get_default_object(classes["character_parts"])
    empty_tags = parts_cdo.get_combined_tags(unreal.GameplayTag())
    require(len(parts_cdo.get_character_part_actors()) == 0, "CharacterParts CDO unexpectedly has spawned actors")
    require(empty_tags.export_text() == unreal.GameplayTagContainer().export_text(), "CharacterParts CDO tags are not empty")
    report("character_parts_types", types=part_types, empty_cdo_tags=True, spawned_actors=0,
           pawn_attached=False, lifecycle_test_run=False, replication_test_run=False)
    report("audit", classes=CLASS_PATHS, allowlist=list(ASSETS.values()))
    if mode == "audit":
        report("complete", mode=mode, asset_writes=0, gameplay_test_run=False)
        return
    assets = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    prepared = {}
    for key in ASSETS:  # Dependency order: Ability -> AbilitySet -> Equipment -> Item.
        asset = create_fixture(key, classes, prepared, assets) if mode == "prepare" else assets.load_asset(ASSETS[key])
        validate(key, asset, classes, prepared)
        prepared[key] = asset
    report("complete", mode=mode, cpp_build=False, packaging=False, PIE=False, gameplay_test_run=False,
           next_test="DreamCatcher.R5.Equipment.QuickBarLifecycle")


try:
    main()
except Exception:
    report("failed", traceback=traceback.format_exc())
    raise
