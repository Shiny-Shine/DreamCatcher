"""One-map R7 PIE observation: actual Host creation, weapon data and lifecycle.

Run with the stock Project.Maps.PIE automation test, never saves assets/config.
NullRHI/no-sound results do not validate rendered appearance, audio or networking.
"""
import json
import traceback
import unreal as ue

ROOT = "/Game/DreamCatcher/GAS/Test/R5/Integration"
HOST = "/Game/LyraMigration/UI/R7/W_WeaponReticleHost.W_WeaponReticleHost_C"


def cls(name):
    return ue.load_class(None, "/Script/DreamCatcher." + name)


def tag(name):
    value = ue.GameplayTag()
    assert value.import_text('(TagName="' + name + '")')
    return value


def emit(event, **values):
    print("DC_R7_PIE " + json.dumps(dict(event=event, **values), ensure_ascii=False))


config = ue.load_asset(ROOT + "/DA_DC_RifleInput")
fire = config.find_ability_input_action_for_tag(tag("InputTag.Weapon.FireAuto"), True)
reload_action = config.find_ability_input_action_for_tag(tag("InputTag.Weapon.Reload"), True)
aim = config.find_native_input_action_for_tag(tag("InputTag.Aim"), True)
ammo_tag = tag("Lyra.ShooterGame.Weapon.MagazineAmmo")
ability_library = ue.get_default_object(ue.load_class(None, "/Script/GameplayAbilities.AbilitySystemBlueprintLibrary"))
host_class = ue.load_class(None, HOST)
state = dict(phase="waiting", inputs={})


def input_value(action, enabled):
    state["inputs"][action] = ue.Vector(1 if enabled else 0, 0, 0)
    state["subsystem"].inject_input_vector_for_action(action, state["inputs"][action], [], [])


def ammo():
    return state["item"].get_stat_tag_stack_count(ammo_tag)


def host():
    return state["pc"].get_editor_property("WeaponReticleHost")


def live_hosts():
    return [w for w in ue.ObjectIterator(ue.UserWidget) if w.get_class() == host_class
            and w.get_owning_player() == state["pc"] and w.is_in_viewport()]


def bind_pawn(pawn):
    state["pawn"] = pawn
    state["item"] = state["qb"].get_active_slot_item()
    state["weapon"] = pawn.get_component_by_class(cls("DCLyraEquipmentManagerComponent")).get_first_instance_of_type(cls("DCLyraWeaponInstance"))
    state["asc"] = ability_library.call_method("GetAbilitySystemComponent", args=(pawn,))
    assert state["weapon"].get_instigator() == state["item"]


def inspect_ui(event):
    widget = host()
    assert widget and widget.is_in_viewport() and widget.get_owning_player() == state["pc"]
    assert len(live_hosts()) == 1, "Duplicate viewport Hosts"
    children = list(widget.get_editor_property("SpawnedWidgets"))
    result = dict(host=widget.get_path_name(), child_classes=[c.get_class().get_path_name() for c in children],
                  ammo=ammo(), viewport_hosts=len(live_hosts()), children=[],
                  top_level_widgets=[w.get_class().get_path_name() for w in ue.ObjectIterator(ue.UserWidget)
                                     if w.get_owning_player() == state["pc"] and w.is_in_viewport()])
    for child in children:
        assert child.get_editor_property("WeaponInstance") == state["weapon"], "Wrong weapon binding"
        assert child.get_editor_property("InventoryInstance") == state["item"], "Wrong item binding"
        texts = {text.get_name(): str(text.get_text()) for text in ue.ObjectIterator(ue.TextBlock)
                 if text.get_path_name().startswith(child.get_path_name() + ":")}
        result["children"].append(dict(name=child.get_class().get_name(), texts=texts,
                                       spread=child.compute_spread_angle(), first_shot=child.has_first_shot_accuracy()))
    emit(event, **result)
    assert sorted(c.get_class().get_name() for c in children) == ["W_AmmoCounter_Rifle_C", "W_Reticle_Rifle_C"], "Native Host Tick has not populated both widgets"
    return widget


def stage(name, now):
    state.update(phase=name, since=now)
    emit("stage", name=name, time=now)


def finish(reason):
    if "subsystem" in state:
        for action in (fire, reload_action, aim):
            input_value(action, False)
    emit("finished", reason=reason, asset_saves=0, visible_appearance_audio_network_verified=False)
    ue.unregister_slate_post_tick_callback(state["callback"])


def tick(delta):
    try:
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or "UEDPIE_" not in world.get_path_name():
            return
        assert world.get_path_name().startswith(ROOT + "/UEDPIE_") and "L_DC_RifleIntegration." in world.get_path_name(), "R7 probe is restricted to the integration map"
        now = ue.GameplayStatics.get_time_seconds(world)
        if state["phase"] == "waiting":
            pc = ue.GameplayStatics.get_player_controller(world, 0)
            pawn = ue.GameplayStatics.get_player_pawn(world, 0)
            if not pc or not pawn:
                return
            qb = pc.get_component_by_class(cls("DCLyraQuickBarComponent"))
            if not qb or not qb.get_active_slot_item():
                return
            library = ue.get_default_object(ue.load_class(None, "/Script/Engine.SubsystemBlueprintLibrary"))
            subsystem = library.call_method("GetLocalPlayerSubSystemFromPlayerController", args=(pc, ue.load_class(None, "/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem")))
            state.update(world=world, pc=pc, qb=qb, subsystem=subsystem, started=now)
            bind_pawn(pawn)
            stage("settle", now)
        if now - state["started"] > 23:
            raise AssertionError("R7 probe timeout in " + state["phase"])
        phase, elapsed = state["phase"], now - state["since"]
        if phase == "settle" and elapsed > 2:
            state["first_host"] = inspect_ui("initial_ui")
            state["full_ammo"] = ammo()
            stage("fire", now)
            input_value(fire, True)
        elif phase == "fire" and ammo() <= state["full_ammo"] - 3:
            input_value(fire, False)
            stage("fire_settle", now)
        elif phase == "fire_settle" and elapsed > 0.3:
            inspect_ui("ammo_after_shots")
            assert host() == state["first_host"], "Unexpected per-tick Host recreation"
            input_value(reload_action, True)
            stage("reload_press", now)
        elif phase == "reload_press" and elapsed > 0.1:
            input_value(reload_action, False)
            stage("reload_wait", now)
        elif phase == "reload_wait" and ammo() == state["full_ammo"]:
            inspect_ui("ammo_reloaded")
            input_value(aim, True)
            stage("aim", now)
        elif phase == "aim" and elapsed > 1:
            inspect_ui("aimed_ui")
            input_value(aim, False)
            state["old_host"] = host()
            state["qb"].set_active_slot_index(1)
            stage("empty", now)
        elif phase == "empty" and elapsed > 0.3:
            assert host() is None and not state["old_host"].is_in_viewport() and len(live_hosts()) == 0
            emit("empty_slot_clean", viewport_hosts=0)
            state["qb"].set_active_slot_index(0)
            bind_pawn(state["pawn"])
            stage("reequip", now)
        elif phase == "reequip" and elapsed > 0.5:
            assert inspect_ui("reequipped_ui") != state["old_host"]
            state["old_host"] = host()
            state["old_pawn"] = state["pawn"]
            state["pc"].un_possess()
            stage("unpossessed", now)
        elif phase == "unpossessed" and elapsed > 0.3:
            assert host() is None and not state["old_host"].is_in_viewport() and len(live_hosts()) == 0
            emit("unpossess_clean", viewport_hosts=0)
            state["old_pawn"].call_method("K2_DestroyActor", args=())
            stage("old_cleanup", now)
        elif phase == "old_cleanup" and elapsed > 0.3:
            assert not ue.SystemLibrary.is_valid(state["old_pawn"])
            ue.GameplayStatics.get_game_mode(world).restart_player(state["pc"])
            stage("restart", now)
        elif phase == "restart":
            pawn = ue.GameplayStatics.get_player_pawn(world, 0)
            if pawn and state["qb"].get_active_slot_item():
                bind_pawn(pawn)
                stage("new_pawn", now)
        elif phase == "new_pawn" and elapsed > 0.5:
            state["old_host"] = inspect_ui("new_pawn_ui")
            effect = ue.load_class(None, "/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C")
            asc = state["asc"]
            spec = asc.make_outgoing_spec(effect, 1.0, asc.make_effect_context())
            spec = ability_library.call_method("AssignTagSetByCallerMagnitude", args=(spec, tag("SetByCaller.Damage"), 10000.0))
            asc.apply_gameplay_effect_spec_to_self(spec)
            stage("death", now)
        elif phase == "death" and elapsed > 0.3:
            assert host() is None and not state["old_host"].is_in_viewport() and len(live_hosts()) == 0
            assert state["pawn"].get_component_by_class(cls("DCLyraHealthComponent")).is_dead_or_dying()
            emit("death_clean", viewport_hosts=0)
            finish("host_data_and_lifecycle_pass")
            return
        for action, value in state["inputs"].items():
            state["subsystem"].inject_input_vector_for_action(action, value, [], [])
    except Exception:
        details = traceback.format_exc()
        emit("probe_error", details=details)
        ue.log_error("R7 Host probe failed: " + details)
        finish("failed")


state["callback"] = ue.register_slate_post_tick_callback(tick)
emit("armed", asset_saves=0)
