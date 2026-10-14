"""Approved single-map Rifle integration recipe; no generic migration tooling or C++ build.

Separate DreamCatcher Python commandlet, -DCRiflePlayerMode=inspect|prepare|verify
and -DCRiflePlayerRun=<fresh_name>, with matching R5RiflePlayer-<fresh_name>.log.
Creates ONLY /Game/DreamCatcher/GAS/Test/R5/Integration assets, never overwrites a prior run.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue

ROOT = "/Game/DreamCatcher/GAS/Test/R5/Integration"
CORE = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247"
PROJECT = Path(__file__).resolve().parents[2]
PAWN_SOURCE = "/Game/DreamCatcher/Blueprints/Player/Test/BP_DC_Player_GAS_Test"
DATA_SOURCE = "/Game/DreamCatcher/GAS/Pawn/DA_DC_PlayerPawn"
PC_SOURCE = "/Game/DreamCatcher/Blueprints/Core/Test/BP_DC_PlayerController_GAS_Test"
PATHS = {"pawn": ROOT + "/BP_DC_RiflePawn", "data": ROOT + "/DA_DC_RiflePawn",
         "input": ROOT + "/DA_DC_RifleInput", "controller": ROOT + "/BP_DC_RifleController",
         "mode": ROOT + "/BP_DC_RifleGameMode", "reload_mapping": ROOT + "/IMC_DC_RifleReload",
         "map": ROOT + "/L_DC_RifleIntegration"}
LOG = None


def require(value, message):
    if not value:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_RIFLE_PLAYER " + json.dumps({"event": event, **values}, ensure_ascii=False))


def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; do not save")
    errors = [line[:1600] for line in LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
              if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine diagnostics; do not save: " + repr(errors[:3]))


def asset(path):
    result = ue.load_asset(path)
    require(result is not None, "Missing asset " + path)
    clean()
    return result


def generated(bp):
    result = ue.BlueprintEditorLibrary.generated_class(bp)
    require(result is not None, "Missing generated class " + bp.get_path_name())
    return result


def cdo(bp):
    return ue.get_default_object(generated(bp))


def encode(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, ue.Object):
        return value.get_path_name()
    if isinstance(value, (list, tuple, ue.Array)):
        return [encode(x) for x in value]
    return value.export_text() if hasattr(value, "export_text") else str(value)


def components(bp, editable=False):
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    result = []
    for handle in handles:
        data = ue.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = (ue.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp) if editable
               else ue.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data))
        if isinstance(obj, ue.ActorComponent):
            if editable:
                require(obj.get_path_name().startswith(ROOT + "/"), "Refusing to edit inherited original template " + obj.get_path_name())
            result.append((handle, obj))
    return handles, result


def add_component(bp, class_name):
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    handles, objects = components(bp)
    found = [obj for _, obj in objects if obj.get_class().get_name() == class_name]
    require(len(found) <= 1, "Duplicate component " + class_name)
    if found:
        return
    native = ue.load_class(None, "/Script/DreamCatcher." + class_name)
    require(native is not None, "Missing built class " + class_name)
    params = ue.AddNewSubobjectParams(parent_handle=handles[0], new_class=native, blueprint_context=bp)
    handle, reason = subsystem.add_new_subobject(params)
    require(not str(reason), "Component creation: " + str(reason))
    clean()


def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode_match = re.search(r"-DCRiflePlayerMode=(inspect|prepare|verify)(?:\s|$)", command)
    run_match = re.search(r"-DCRiflePlayerRun=([A-Za-z0-9_]{1,32})(?:\s|$)", command)
    require(mode_match and run_match and "-run=pythonscript" in command.lower(), "Explicit commandlet recipe required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    LOG = PROJECT / "Saved/Logs" / ("R5RiflePlayer-" + run_match.group(1) + ".log")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context rejected")
    clean()
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    source_data = asset(DATA_SOURCE)
    source_pawn = asset(PAWN_SOURCE)
    source_pc = asset(PC_SOURCE)
    source_input = source_data.get_editor_property("input_config")
    require(source_input is not None, "Current PawnData lacks input config")
    protected = {}
    for path in (PAWN_SOURCE, DATA_SOURCE, PC_SOURCE, source_input.get_path_name().split(".")[0]):
        file = PROJECT / "Content" / (path[6:] + ".uasset")
        protected[str(file)] = hashlib.sha256(file.read_bytes()).hexdigest()
    if mode_match.group(1) == "inspect":
        _, objects = components(source_pawn)
        emit("source", pawn_parent=encode(ue.BlueprintEditorLibrary.get_blueprint_parent_class(source_pawn)),
             pawn_data={name: encode(source_data.get_editor_property(name)) for name in ("pawn_class", "input_config", "ability_sets", "default_weapon_definition")},
             input_actions=encode(source_input.get_editor_property("ability_input_actions")),
             controller_mappings=encode(cdo(source_pc).get_editor_property("default_mapping_contexts")),
             components=[{"path": obj.get_path_name(), "class": obj.get_class().get_name()} for _, obj in objects])
        for _, obj in objects:
            if obj.get_class().get_name() == "DCHeroComponent":
                emit("hero_defaults", values={name: encode(obj.get_editor_property(name)) for name in (
                    "bUseLegacyPlayerInput", "bUseLegacyCameraMode", "bUseOriginalADSInputRouting", "DefaultInputMappings")})
                for mapping in obj.get_editor_property("default_input_mappings"):
                    emit("mapping", value=encode(mapping))
            if obj.get_name() == "CharacterMesh0":
                emit("mesh_defaults", mesh=encode(obj.get_editor_property("skeletal_mesh_asset")), anim=encode(obj.get_editor_property("anim_class")))
        return
    if mode_match.group(1) == "verify":
        global_data = asset("/Game/DefaultGameData")
        registry_data = ue.AssetRegistryHelpers.get_asset_registry().get_asset_by_object_path("/Game/DefaultGameData.DefaultGameData")
        emit("game_data_registration", loaded_class=global_data.get_class().get_path_name(),
             stored_type=registry_data.get_tag_value("PrimaryAssetType"), stored_name=registry_data.get_tag_value("PrimaryAssetName"))
        loaded = {key: asset(path) for key, path in PATHS.items()}
        data = loaded["data"]
        require(data.get_editor_property("default_weapon_definition") is None, "Legacy default weapon is still configured")
        require(data.get_editor_property("pawn_class") == generated(loaded["pawn"]), "PawnData class mismatch")
        require(cdo(loaded["mode"]).get_editor_property("integration_pawn_data") == data, "GameMode PawnData mismatch")
        require(cdo(loaded["controller"]).get_editor_property("starting_rifle") == generated(asset(CORE + "/ID_Rifle")), "Rifle item mismatch")
        for cue_path in assets.list_assets("/Game/DreamCatcher/GAS/GameplayCues", recursive=True, include_folder=False) + [CORE + "/GCN_Weapon_Impact", CORE + "/GCN_Weapon_Rifle_Fire"]:
            cue = asset(cue_path)
            if isinstance(cue, ue.Blueprint):
                default = cdo(cue)
                try:
                    emit("cue_tag", path=cue.get_path_name(), tag=encode(default.get_editor_property("gameplay_cue_tag")))
                except Exception:
                    pass  # Non-Cue Blueprints are not this comparison's subjects.
        emit("verified", assets=7, asset_saves=0, runtime_verified=False)
        return
    require(all(not assets.does_asset_exist(path) for path in PATHS.values()), "Fresh integration paths required; do not overwrite partial/user assets")
    # New copies only. The old player/map and input data remain a recovery/comparison path.
    pawn = assets.duplicate_asset(PAWN_SOURCE, PATHS["pawn"])
    data = assets.duplicate_asset(DATA_SOURCE, PATHS["data"])
    inputs = assets.duplicate_asset(source_input.get_path_name().split(".")[0], PATHS["input"])
    require(pawn and data and inputs, "Could not create independent copies")
    actions = []
    changed = 0
    for entry in source_input.get_editor_property("ability_input_actions"):
        text = entry.export_text()
        if 'InputTag.Weapon.Fire"' in text:
            text = text.replace('InputTag.Weapon.Fire"', 'InputTag.Weapon.FireAuto"')
            changed += 1
        copy = type(entry)()
        require(copy.import_text(text), "Input entry import failed")
        actions.append(copy)
    require(changed == 1, "Expected exactly one current fire input; inspect before changing mapping")
    reload_action = asset("/Game/LyraMigration/BaseInput/Actions/IA_Weapon_Reload")
    reload_entry = type(actions[0])()
    require(reload_entry.import_text('(InputAction="' + reload_action.get_path_name() + '",InputTag=(TagName="InputTag.Weapon.Reload"))'), "Reload input entry failed")
    actions.append(reload_entry)
    mapping_class = ue.load_class(None, "/Script/EnhancedInput.InputMappingContext")
    factory = ue.DataAssetFactory()
    factory.set_editor_property("data_asset_class", mapping_class)
    reload_mapping = ue.AssetToolsHelpers.get_asset_tools().create_asset("IMC_DC_RifleReload", ROOT, mapping_class, factory)
    require(reload_mapping is not None, "Reload mapping creation failed")
    reload_key = ue.Key()
    require(reload_key.import_text("R"), "R key import failed")
    reload_mapping.map_key(reload_action, reload_key)
    inputs.set_editor_property("ability_input_actions", actions)
    data.set_editor_property("input_config", inputs)
    data.set_editor_property("default_weapon_definition", None)
    add_component(pawn, "DCLyraEquipmentManagerComponent")
    add_component(pawn, "DCLyraPawnComponent_CharacterParts")
    ue.BlueprintEditorLibrary.compile_blueprint(pawn)
    clean()
    _, objects = components(pawn, editable=True)
    mesh = next((obj for _, obj in objects if isinstance(obj, ue.SkeletalMeshComponent) and obj.get_name() == "CharacterMesh0"), None)
    require(mesh is not None, "Character main mesh template missing")
    # The old test uses the template mannequin skeleton. Keep this change in the new
    # Pawn only and use the original mesh+anim pair, rather than assuming compatibility.
    base_anim = asset("/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base")
    mesh.set_editor_property("skeletal_mesh_asset", asset("/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny"))
    mesh.set_editor_property("anim_class", generated(base_anim))
    for _, obj in objects:
        name = obj.get_class().get_name()
        if name == "DCPawnExtensionComponent":
            obj.set_editor_property("pawn_data", None)  # Integration GameMode supplies it before construction.
        elif name == "DCHeroComponent":
            mappings = list(obj.get_editor_property("default_input_mappings"))
            require(mappings, "Expected verified Hero mapping defaults")
            new_mapping = type(mappings[0])()
            require(new_mapping.import_text('(InputMapping="' + reload_mapping.get_path_name() + '",Priority=1,bRegisterWithSettings=True)'), "Hero reload mapping import failed")
            mappings.append(new_mapping)
            obj.set_editor_property("default_input_mappings", mappings)
        elif name == "DCLyraPawnComponent_CharacterParts":
            body = obj.get_editor_property("body_meshes")
            body.set_editor_property("default_mesh", mesh.get_editor_property("skeletal_mesh_asset"))
            obj.set_editor_property("body_meshes", body)
        elif obj.get_name() == "WeaponMesh":
            obj.set_editor_property("visible", False)  # Do not display a second legacy gun.
    ue.BlueprintEditorLibrary.compile_blueprint(pawn)
    clean()
    data.set_editor_property("pawn_class", generated(pawn))
    controller = ue.BlueprintEditorLibrary.create_blueprint_asset_with_parent(PATHS["controller"], ue.load_class(None, "/Script/DreamCatcher.DCRifleIntegrationController"))
    mode = ue.BlueprintEditorLibrary.create_blueprint_asset_with_parent(PATHS["mode"], ue.load_class(None, "/Script/DreamCatcher.DCRifleIntegrationGameMode"))
    require(controller and mode, "Integration Blueprint creation failed")
    for bp in (controller, mode):
        require(ue.BlueprintEditorLibrary.compile_blueprint(bp), "Integration BP compile failed")
    cdo(controller).set_editor_property("starting_rifle", generated(asset(CORE + "/ID_Rifle")))
    cdo(controller).set_editor_property("default_mapping_contexts", cdo(source_pc).get_editor_property("default_mapping_contexts"))
    cdo(mode).set_editor_property("integration_pawn_data", data)
    cdo(mode).set_editor_property("player_controller_class", generated(controller))
    for bp in (controller, mode):
        require(ue.BlueprintEditorLibrary.compile_blueprint(bp), "Configured BP compile failed")
    clean()
    for obj in (inputs, reload_mapping, pawn, data, controller, mode):
        require(assets.save_loaded_asset(obj, False), "Save failed " + obj.get_path_name())
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    require(levels.new_level(PATHS["map"]), "Could not create new integration map")
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", generated(mode))
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    floor = actors.spawn_actor_from_class(ue.StaticMeshActor, ue.Vector(0, 0, -10))
    floor.static_mesh_component.set_static_mesh(asset("/Engine/BasicShapes/Cube"))
    floor.set_actor_scale3d(ue.Vector(40, 40, 0.2))
    floor.set_actor_label("Rifle Test Floor")
    actors.spawn_actor_from_class(ue.PlayerStart, ue.Vector(0, 0, 110))
    actors.spawn_actor_from_class(ue.DirectionalLight, ue.Vector(0, 0, 500), ue.Rotator(-55, 35, 0))
    target_bp = asset("/Game/DreamCatcher/Blueprints/AI/Test/BP_DC_GASTestTarget")
    actors.spawn_actor_from_class(generated(target_bp), ue.Vector(650, 0, 100))
    clean()
    require(levels.save_current_level(), "Could not save integration map")
    for file, expected in protected.items():
        require(hashlib.sha256(Path(file).read_bytes()).hexdigest() == expected, "Original changed: " + file)
    emit("prepared", paths=PATHS, original_player_files_unchanged=True, runtime_verified=False)


if __name__ == "__main__":
    main()
