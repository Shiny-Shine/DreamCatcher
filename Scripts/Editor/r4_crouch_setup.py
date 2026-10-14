"""Approved Integration-only crouch settings/input. No C++ build or gameplay rule edits.

-DCCrouchMode=inspect|prepare|verify -DCCrouchRun=<name>
verify also requires -DCCrouchBaseline=<successful prepare run>.
Requires -DCRifleCopyDiagnostics for the existing isolated-context guard.
Only BP_DC_RiflePawn, DA_DC_RifleInput and a fresh IMC_DC_RifleCrouch may be saved.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
import unreal as ue
from r5_rifle_player_setup import components

PROJECT = Path(__file__).resolve().parents[2]
ROOT = "/Game/DreamCatcher/GAS/Test/R5/Integration"
PAWN = ROOT + "/BP_DC_RiflePawn"
INPUT = ROOT + "/DA_DC_RifleInput"
CONTEXT = ROOT + "/IMC_DC_RifleCrouch"
ACTION = "/Game/LyraMigration/BaseInput/Actions/IA_Crouch"
SOURCE_CONTEXT = "/Game/LyraMigration/BaseInput/IMC_Default"
PROTECTED = [ACTION, SOURCE_CONTEXT, "/Game/Input/IMC_Player",
             "/Game/DreamCatcher/Blueprints/Player/Test/BP_DC_Player_GAS_Test",
             "/Game/DreamCatcher/GAS/Pawn/DA_DC_PlayerPawn", ROOT + "/DA_DC_RiflePawn",
             ROOT + "/BP_DC_RifleController", ROOT + "/L_DC_RifleIntegration",
             "/Game/DreamCatcher/GAS/Abilities/Aim/GA_DC_ShoulderADS",
             "/Game/DreamCatcher/GAS/Abilities/Aim/GA_DC_ScopeADS"]
LOG = None

def require(ok, message):
    if not ok:
        raise RuntimeError(message)
def emit(event, **data):
    print("R4_CROUCH_SETUP " + json.dumps(dict(event=event, **data), ensure_ascii=False))
def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; refuse saving")
    errors = [line[:1200] for line in LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
              if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine diagnostics; refuse saving: " + repr(errors[:3]))
def load(path):
    obj = ue.load_asset(path)
    require(obj is not None, "Missing asset " + path)
    clean()
    return obj
def cdo(bp):
    return ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
def fingerprint():
    paths = [PROJECT / "Config/DefaultGame.ini", PROJECT / "Config/DefaultEngine.ini", PROJECT / ".git/index"]
    for package in PROTECTED:
        paths.extend(PROJECT / "Content" / (package[6:] + ext) for ext in (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl"))
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None for p in paths}
def mapping_data(context):
    return context.get_editor_property("DefaultKeyMappings")
def rows(context, action):
    return [entry for entry in mapping_data(context).get_editor_property("Mappings") if entry.get_editor_property("Action") == action]
def mapping_signature(entry):
    return dict(action=entry.get_editor_property("Action").get_path_name(), key=entry.get_editor_property("Key").export_text(),
                triggers=[dict(kind=t.get_class().get_path_name(), threshold=t.get_editor_property("ActuationThreshold"),
                               always_tick=t.get_editor_property("bShouldAlwaysTick")) for t in entry.get_editor_property("Triggers")],
                modifiers=[m.get_class().get_path_name() for m in entry.get_editor_property("Modifiers")],
                behavior=str(entry.get_editor_property("SettingBehavior")),
                player_settings=entry.get_editor_property("PlayerMappableKeySettings") is not None)
def snapshot(bp, inputs):
    default = cdo(bp)
    movement = default.get_component_by_class(ue.CharacterMovementComponent)
    # Blueprint-added SCS components are templates, not instantiated CDO components.
    _, template_objects = components(bp)
    heroes = [obj for _, obj in template_objects if obj.get_class().get_name() == "DCHeroComponent"]
    require(len(heroes) == 1, "Ambiguous Hero template")
    hero = heroes[0]
    capsule = default.get_component_by_class(ue.CapsuleComponent)
    require(movement and hero and capsule, "Missing required components")
    return dict(eyes=[default.get_editor_property("BaseEyeHeight"), default.get_editor_property("CrouchedEyeHeight")],
                nav=movement.get_editor_property("NavAgentProps").export_text(),
                ledge=movement.get_editor_property("bCanWalkOffLedgesWhenCrouching"),
                half_height=movement.get_editor_property("CrouchedHalfHeight"),
                movement={name: movement.get_editor_property(name) for name in ("MaxWalkSpeed", "MaxWalkSpeedCrouched", "MaxAcceleration", "BrakingDecelerationWalking", "AirControl")},
                capsule=[capsule.get_editor_property("CapsuleRadius"), capsule.get_editor_property("CapsuleHalfHeight")],
                hero_flags={name: hero.get_editor_property(name) for name in ("bUseLegacyPlayerInput", "bUseLegacyCameraMode", "bUseOriginalADSInputRouting")},
                contexts=[entry.export_text() for entry in hero.get_editor_property("DefaultInputMappings")],
                native_actions=[entry.export_text() for entry in inputs.get_editor_property("NativeInputActions")],
                ability_actions=[entry.export_text() for entry in inputs.get_editor_property("AbilityInputActions")])
def validate(snapshot_after, before):
    require(snapshot_after["eyes"] == [80.0, 50.0] and snapshot_after["ledge"] is True and snapshot_after["half_height"] == 65.0,
            "Original crouch defaults not preserved")
    require(snapshot_after["nav"] == before["nav"].replace("bCanCrouch=False", "bCanCrouch=True"), "Unexpected nav-agent changes")
    for name in ("movement", "capsule", "hero_flags", "ability_actions"):
        require(snapshot_after[name] == before[name], "Unrelated property changed: " + name)
    require(snapshot_after["native_actions"][:-1] == before["native_actions"] and len(snapshot_after["native_actions"]) == len(before["native_actions"])+1,
            "Existing native inputs changed")
    require('TagName="InputTag.Crouch"' in snapshot_after["native_actions"][-1] and ACTION in snapshot_after["native_actions"][-1], "Wrong crouch action")
    require(snapshot_after["contexts"][:-1] == before["contexts"] and len(snapshot_after["contexts"]) == len(before["contexts"])+1,
            "Existing input contexts changed")
    require(CONTEXT in snapshot_after["contexts"][-1] and "Priority=1" in snapshot_after["contexts"][-1], "Wrong crouch context")

def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCCrouchMode=(inspect|prepare|verify)(?:\s|$)", command)
    run = re.search(r"-DCCrouchRun=([A-Za-z0-9_]{1,32})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit commandlet arguments required")
    require(Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT, "Wrong project")
    LOG = PROJECT / "Saved/Logs" / ("R4Crouch-" + run.group(1) + ".log")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    emit("context_guard", result=guard)
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context/SCC/SaveOnCompile guard rejected")
    ue.AssetRegistryHelpers.get_asset_registry().search_all_assets(True)
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    protected = fingerprint()
    try:
        bp, inputs, action, source = load(PAWN), load(INPUT), load(ACTION), load(SOURCE_CONTEXT)
        source_rows = rows(source, action)
        signatures = [mapping_signature(entry) for entry in source_rows]
        require(len(signatures) == 2 and {entry["key"] for entry in signatures} == {"LeftControl", "Gamepad_FaceButton_Right"}, "Source keys changed")
        require(all(not entry["modifiers"] and not entry["player_settings"] for entry in signatures), "Source has unhandled mapping overrides")
        require(all(t["kind"] == "/Script/EnhancedInput.InputTriggerPressed" for entry in signatures for t in entry["triggers"]), "Unexpected trigger type")
        before = snapshot(bp, inputs)
        emit("before", values=before, source_mappings=signatures, protected=protected)
        if mode.group(1) == "inspect":
            clean()
            return
        if mode.group(1) == "prepare":
            require(not assets.does_asset_exist(CONTEXT), "Fresh context only; refusing overwrite")
            require(before["eyes"] == [64.0, 32.0] and not before["ledge"] and before["half_height"] == 40.0 and "bCanCrouch=False" in before["nav"], "Pawn defaults changed since approval")
            require(not any('TagName="InputTag.Crouch"' in entry for entry in before["native_actions"]), "Crouch already bound; inspect rather than duplicate")
            _, objects = components(bp, editable=True)
            movements = [obj for _, obj in objects if isinstance(obj, ue.CharacterMovementComponent)]
            heroes = [obj for _, obj in objects if obj.get_class().get_name() == "DCHeroComponent"]
            require(len(movements) == len(heroes) == 1, "Ambiguous editable templates")
            movement, hero = movements[0], heroes[0]
            factory = ue.DataAssetFactory()
            factory.set_editor_property("DataAssetClass", ue.load_class(None, "/Script/EnhancedInput.InputMappingContext"))
            mapping = ue.AssetToolsHelpers.get_asset_tools().create_asset("IMC_DC_RifleCrouch", ROOT, factory.get_editor_property("DataAssetClass"), factory)
            require(mapping is not None, "Could not create scoped context")
            copied_rows = []
            for source_entry in source_rows:
                text = source_entry.export_text()
                for trigger in source_entry.get_editor_property("Triggers"):
                    copied = ue.new_object(trigger.get_class(), outer=mapping)
                    copied.set_editor_property("ActuationThreshold", trigger.get_editor_property("ActuationThreshold"))
                    require(copied.get_editor_property("bShouldAlwaysTick") == trigger.get_editor_property("bShouldAlwaysTick"), "Unexpected trigger tick flag")
                    text = text.replace(trigger.get_path_name(), copied.get_path_name())
                entry = type(source_entry)()
                require(entry.import_text(text), "Mapping copy failed")
                copied_rows.append(entry)
            data = mapping_data(mapping)
            data.set_editor_property("Mappings", copied_rows)
            mapping.set_editor_property("DefaultKeyMappings", data)
            native = list(inputs.get_editor_property("NativeInputActions"))
            require(native, "Expected existing native action schema")
            added = type(native[0])()
            require(added.import_text('(InputAction="' + action.get_path_name() + '",InputTag=(TagName="InputTag.Crouch"))'), "Input-tag entry failed")
            inputs.set_editor_property("NativeInputActions", native + [added])
            nav = movement.get_editor_property("NavAgentProps")
            nav.set_editor_property("bCanCrouch", True)
            movement.set_editor_property("NavAgentProps", nav)
            movement.set_editor_property("bCanWalkOffLedgesWhenCrouching", True)
            movement.set_editor_property("CrouchedHalfHeight", 65.0)
            cdo(bp).set_editor_property("BaseEyeHeight", 80.0)
            cdo(bp).set_editor_property("CrouchedEyeHeight", 50.0)
            contexts = list(hero.get_editor_property("DefaultInputMappings"))
            require(contexts, "Expected existing Hero mapping schema")
            context_entry = type(contexts[0])()
            require(context_entry.import_text('(InputMapping="' + mapping.get_path_name() + '",Priority=1,bRegisterWithSettings=True)'), "Hero mapping entry failed")
            hero.set_editor_property("DefaultInputMappings", contexts + [context_entry])
            require(ue.BlueprintEditorLibrary.compile_blueprint(bp), "Pawn Blueprint Compile failed")
            clean()
            emit("memory_after", values=snapshot(bp, inputs))
            validate(snapshot(bp, inputs), before)
            require([mapping_signature(entry) for entry in rows(mapping, action)] == signatures, "Source mapping semantics changed")
            require(all(t.get_outer() == mapping for entry in rows(mapping, action) for t in entry.get_editor_property("Triggers")), "Trigger still references source context")
            require(fingerprint() == protected, "Protected files changed before save")
            for obj in (mapping, inputs, bp):
                clean()
                require(assets.save_loaded_asset(obj, False), "Save failed " + obj.get_path_name())
            emit("prepared", asset_saves=3, new_native_code_executed=False)
        else:
            baseline = re.search(r"-DCCrouchBaseline=([A-Za-z0-9_]{1,32})(?:\s|$)", command)
            require(baseline, "Successful prepare baseline required")
            text = (PROJECT / "Saved/Logs" / ("R4Crouch-" + baseline.group(1) + ".log")).read_text(encoding="utf-8-sig")
            records = [json.loads(line.split("R4_CROUCH_SETUP ", 1)[1]) for line in text.splitlines() if "LogPython: R4_CROUCH_SETUP " in line]
            require(any(entry["event"] == "prepared" for entry in records), "Baseline did not save successfully")
            baseline_record = next(entry for entry in records if entry["event"] == "before")
            validate(before, baseline_record["values"])
            require(protected == baseline_record["protected"], "Protected files changed since prepare")
            mapping = load(CONTEXT)
            require([mapping_signature(entry) for entry in rows(mapping, action)] == baseline_record["source_mappings"], "Reloaded keys/triggers changed")
            require(len(mapping_data(mapping).get_editor_property("Mappings")) == 2, "Unexpected extra mappings")
            require(all(t.get_outer() == mapping for entry in rows(mapping, action) for t in entry.get_editor_property("Triggers")), "Reloaded trigger outer mismatch")
            emit("verified", values=before, mappings=[mapping_signature(entry) for entry in rows(mapping, action)], asset_saves=0,
                 runtime_crouch_tag_verified=False)
        clean()
    finally:
        require(fingerprint() == protected, "Protected asset/config/index changed")

if __name__ == "__main__":
    main()
