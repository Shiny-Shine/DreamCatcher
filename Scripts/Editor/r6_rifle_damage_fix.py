"""Scoped R6 GE/input repair and post-build death-target setup, through Unreal only.

-DCR6FixMode=inspect-config|repair-ge|verify-ge|prepare-target|verify-target
-DCR6FixRun=<fresh_name>, matching Saved/Logs/R6RifleFix-<fresh_name>.log.
Never builds C++, overwrites the common target, or changes damage math.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
ROOT = "/Game/DreamCatcher/GAS/Test/R5/Integration"
FIRE = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/GA_Weapon_Fire_Rifle_Auto"
DAMAGE = "/Game/Weapons/Rifle/GE_Damage_RifleAuto"
DEATH = "/Game/Characters/Heroes/Abilities/GA_Hero_Death"
OLD_TARGET = "/Game/DreamCatcher/Blueprints/AI/Test/BP_DC_GASTestTarget"
NEW_TARGET = ROOT + "/BP_DC_RifleDeathTarget"
MAP = ROOT + "/L_DC_RifleIntegration"
LOG = None


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def emit(event, **values):
    print("R6_RIFLE_FIX " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; refuse saving")
    errors = [line[:1500] for line in LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
              if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine diagnostics; refuse saving: " + repr(errors[:3]))


def load(path):
    obj = ue.load_asset(path)
    require(obj is not None, "Missing asset: " + path)
    clean()
    return obj


def generated(bp):
    cls = ue.BlueprintEditorLibrary.generated_class(bp)
    require(cls is not None, "Missing generated class: " + bp.get_path_name())
    return cls


def cdo(bp):
    return ue.get_default_object(generated(bp))


def fingerprint(package):
    require(package.startswith("/Game/"), "Only Game assets allowed")
    result = {}
    for suffix in (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl"):
        file = PROJECT / "Content" / (package[6:] + suffix)
        result[str(file)] = hashlib.sha256(file.read_bytes()).hexdigest() if file.is_file() else None
    return result


def unchanged(before):
    for name, expected in before.items():
        file = Path(name)
        actual = hashlib.sha256(file.read_bytes()).hexdigest() if file.is_file() else None
        require(actual == expected, "Protected asset changed: " + name)


def input_scales():
    controller = cdo(load(ROOT + "/BP_DC_RifleController"))
    scales = (controller.get_deprecated_input_yaw_scale(), controller.get_deprecated_input_pitch_scale(),
              controller.get_deprecated_input_roll_scale())
    require(all(abs(x - 1.0) < 0.0001 for x in scales), "Integration Controller config not neutral: " + repr(scales))
    emit("input_scales", yaw=scales[0], pitch=scales[1], roll=scales[2], old_maps_unchanged=True)


def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR6FixMode=(inspect-config|repair-ge|verify-ge|prepare-target|verify-target)(?:\s|$)", command)
    run = re.search(r"-DCR6FixRun=([A-Za-z0-9_]{1,40})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit isolated commandlet required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    LOG = PROJECT / "Saved/Logs" / ("R6RifleFix-" + run.group(1) + ".log")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context/SCC/SaveOnCompile guard rejected")
    # Complete the startup disk gather before replacing an existing package file.
    # Never work around a sharing violation with raw binary file moves/deletes.
    ue.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    protected = {}
    for path in (OLD_TARGET, DAMAGE, DEATH, "/Game/DreamCatcher/Blueprints/Player/Test/BP_DC_Player_GAS_Test"):
        protected.update(fingerprint(path))
    try:
        input_scales()
        death_bp = load(DEATH)
        require(ue.BlueprintEditorLibrary.get_blueprint_parent_class(death_bp).get_path_name() ==
                "/Script/DreamCatcher.DCLyraGameplayAbility_Death", "Death asset must retain its original-derived parent")
        emit("death_reference", asset=DEATH, parent=ue.BlueprintEditorLibrary.get_blueprint_parent_class(death_bp).get_path_name(),
             duration=cdo(death_bp).get_editor_property("Duration"), auto_start=cdo(death_bp).get_editor_property("bAutoStartDeath"))
        if mode.group(1) == "inspect-config":
            emit("config_verified", asset_saves=0, channels=[n for n in dir(ue.CollisionChannel) if "WEAPON" in n])
            return
        fire = load(FIRE)
        damage = generated(load(DAMAGE))
        current = cdo(fire).get_editor_property("GE_Damage")
        if mode.group(1) == "repair-ge":
            require(current is None or current == damage, "Unexpected GE override: don't replace user changes")
            if current is None:
                cdo(fire).set_editor_property("GE_Damage", damage)
                require(ue.BlueprintEditorLibrary.compile_blueprint(fire), "Fire Blueprint Compile failed")
                clean()
                require(cdo(fire).get_editor_property("GE_Damage") == damage, "GE assignment lost at Compile")
                require(assets.save_loaded_asset(fire, False), "Could not save repaired fire ability")
                emit("ge_repaired", asset=FIRE, damage=damage.get_path_name())
        require(cdo(fire).get_editor_property("GE_Damage") == damage, "Rifle GE reference is not restored")
        if mode.group(1) in ("repair-ge", "verify-ge"):
            emit("verified_ge", ge=damage.get_path_name(), asset_saves=1 if current is None else 0, gameplay_verified=False)
            return
        native = ue.load_class(None, "/Script/DreamCatcher.DCRifleIntegrationTarget")
        require(native is not None, "User C++ build is required before target setup")
        levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
        require(levels.load_level(MAP), "Could not load integration map")
        actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
        old_class_path = generated(load(OLD_TARGET)).get_path_name()
        old_actors = [a for a in actors.get_all_level_actors() if a.get_class().get_path_name() == old_class_path]
        if mode.group(1) == "prepare-target":
            require(not assets.does_asset_exist(NEW_TARGET), "Fresh target Blueprint required; no overwrite")
            require(len(old_actors) == 1, "Expected exactly the one old integration dummy")
            old_actor = old_actors[0]
            old_mesh = old_actor.get_editor_property("target_mesh")
            bp = ue.BlueprintEditorLibrary.create_blueprint_asset_with_parent(NEW_TARGET, native)
            require(bp is not None and ue.BlueprintEditorLibrary.compile_blueprint(bp), "Target BP creation failed")
            defaults = cdo(bp)
            defaults.set_editor_property("death_ability_class", generated(death_bp))
            defaults.set_editor_property("integration_team_id", 1)
            defaults.set_editor_property("initial_health", old_actor.get_editor_property("initial_health"))
            # New native target defaults own collision. Copy visuals only, not legacy Ignore responses.
            target_mesh = defaults.get_editor_property("target_mesh")
            target_mesh.set_editor_property("static_mesh", old_mesh.get_editor_property("static_mesh"))
            target_mesh.set_editor_property("override_materials", old_mesh.get_editor_property("override_materials"))
            require(ue.BlueprintEditorLibrary.compile_blueprint(bp), "Configured target Compile failed")
            clean()
            require(assets.save_loaded_asset(bp, False), "Target save failed")
            replacement = actors.spawn_actor_from_class(generated(bp), old_actor.get_actor_location(), old_actor.get_actor_rotation())
            require(replacement is not None, "Could not spawn replacement target")
            replacement.set_actor_scale3d(old_actor.get_actor_scale3d())
            replacement.set_actor_label("R6 Rifle Death Target")
            # The user approved replacing this one actor on this integration map, not deleting its BP.
            require(actors.destroy_actor(old_actor), "Could not replace the old placed dummy")
            clean()
            require(levels.save_current_level(), "Integration map save failed")
            emit("target_prepared", blueprint=NEW_TARGET, map=MAP, replaced_placed_actors=1, deleted_asset_files=0)
        else:
            bp = load(NEW_TARGET)
            require(ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == native, "Target parent mismatch")
            require(cdo(bp).get_editor_property("death_ability_class") == generated(death_bp), "Death Ability mismatch")
            require(cdo(bp).get_editor_property("integration_team_id") == 1, "Target team mismatch")
            placed = [a for a in actors.get_all_level_actors() if a.get_class() == generated(bp)]
            require(len(placed) == 1 and not old_actors, "Expected one new target and no legacy dummy on integration map")
            mesh = placed[0].get_editor_property("target_mesh")
            # User-defined enum names become available after the original channel config is registered.
            channel = ue.CollisionChannel.ECC_LYRA_TRACE_CHANNEL_WEAPON
            # EngineTypes.h exposes ECollisionResponse as CollisionResponseType in UE 5.8;
            # CollisionResponse is the separate FCollisionResponse struct wrapper.
            response = mesh.get_collision_response_to_channel(channel)
            emit("weapon_collision", response=str(response), enum_entries=[n for n in dir(ue.CollisionResponseType) if n.isupper()])
            require(response == ue.CollisionResponseType.ECR_BLOCK, "Target still ignores weapon trace")
            emit("target_verified", target=placed[0].get_path_name(), weapon_response="Block", asset_saves=0, death_runtime_verified=False)
    finally:
        unchanged(protected)
        emit("protected_assets_unchanged", file_states=len(protected))


if __name__ == "__main__":
    main()
