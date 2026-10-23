"""Final-approved real PIE registration checks for Level_1/2.
No saves/build/config writes. Diagnostic teleports, immunity and existing GAS GE
exercise 75m/one-shot/mission counts, actual Level_2 AI movement, natural clear
movie completion and existing next-map travel. A separate abort/restart case
checks pending-spawn cancellation and per-run reset. Physical acceptance remains
with the user. Finalize --verify-report REPORT LOG INDEX EXIT after editor exit.
"""
import importlib.util
import json
from pathlib import Path
import re
import sys
import time
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / 'Saved/Diagnostics/SpawnerRegistration20261024'


def support_module():
    spec = importlib.util.spec_from_file_location('registration_probe_support', Path(__file__).with_name('spawner_registration_setup.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.is_relative_to(ROOT.resolve())
    report = json.loads(path.read_text(encoding='utf-8'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    automation_ok = bool(tests) and all(row.get('state') == 'Success' for row in tests)
    automation_ok &= all(int(index.get(k, -1)) == 0 for k in ('failed', 'notRun', 'inProcess'))
    errors = [line for line in log.splitlines() if re.search(r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    media_ends = log.count('[MissionClear] Movie ended; requesting configured next screen.')
    matching_run = any('REGISTRATION_PIE ' in line and '"event": "armed"' in line and '"run": "' + report['run'] + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=automation_ok,
        matching_run=matching_run, natural_media_end_events=media_ends, engine_errors=errors,
        engine_warnings=[line for line in log.splitlines() if ': Warning:' in line],
        automation_index=str(Path(index_path).resolve()), log=str(Path(log_path).resolve()))
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0 and automation_ok
        and matching_run and not errors and media_ends == (2 if report['case'] == 'chain' else 0))
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], errors=len(errors), media_end_events=media_ends)))
    return 0 if report['complete'] else 1


def arm():
    import unreal as u
    s = support_module()
    s.ue = u
    s.actors = lambda world: list(u.GameplayStatics.get_all_actors_of_class(world, u.Actor))
    command = u.SystemLibrary.get_command_line()
    run = re.search(r'-DCRegistrationProbe=([A-Za-z0-9_]+)\b', command).group(1)
    case = re.search(r'-DCRegistrationCase=(chain|abort)\b', command).group(1)
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert '-nullrhi' not in command.lower() and not u.SourceControl.is_enabled()
    s.module('base_damage_setup').assert_no_other_editor()
    setting = re.search(r'"-GameUserSettingsINI=([^"]+)"|-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', command, re.I)
    assert setting and Path(next(x for x in setting.groups() if x)).resolve().is_relative_to(ROOT.resolve())
    directory = ROOT / ('PIE_' + run)
    directory.mkdir(parents=True, exist_ok=True)
    out = directory / 'probe.json'
    assert not out.exists()
    before = s.inventory()
    protected = s.protected_hashes()
    targets = s.inputs()
    editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
    assert not editor.get_game_world()
    cls = lambda name: u.load_class(None, '/Script/DreamCatcher.' + name)
    gas = u.get_default_object(u.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
    damage_class = u.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
    assert damage_class
    state = dict(done=False, busy=False, wall=time.monotonic(), level=None, step='ready', at=0,
                 immunity=None, phase=None, phase_kills=0, nav_done=False, cleared={}, last_event=-10)
    report = dict(run=run, case=case, complete=False, probe_passed=False, events=[], failures=[], movies={},
                  asset_saves=0, cpp_build=False, physical_input_visual_audio_verified=False,
                  real_weapon_damage_verified=False, network_replication_verified=False)

    def event(name, **values):
        row = dict(event=name, **values)
        report['events'].append(row)
        print('REGISTRATION_PIE ' + json.dumps(row, ensure_ascii=False))

    def actors(world, name):
        return list(u.GameplayStatics.get_all_actors_of_class(world, cls(name)))

    def health(pawn):
        return pawn.get_component_by_class(cls('DCLyraHealthComponent')) if pawn else None

    def asc(pawn):
        return gas.call_method('GetAbilitySystemComponent', args=(pawn,))

    def tag(name):
        value = u.GameplayTag()
        assert value.import_text('(TagName="' + name + '")')
        return value

    immunity_tags = u.GameplayTagContainer()
    assert immunity_tags.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')

    def add_immunity(pawn):
        if state['immunity'] is None:
            assert gas.call_method('AddLooseGameplayTags', args=(pawn, immunity_tags, False))
            state['immunity'] = pawn

    def remove_immunity():
        if state['immunity'] is not None:
            assert gas.call_method('RemoveLooseGameplayTags', args=(state['immunity'], immunity_tags, False))
            state['immunity'] = None

    def move(pawn, point):
        movement = pawn.get_movement_component()
        movement.stop_movement_immediately()
        movement.disable_movement()
        pawn.set_actor_location(point, False, True)
        assert pawn.get_actor_location().distance(point) < .1

    def point(spawner):
        return spawner.get_editor_property('SpawnPoint').get_world_location()

    def bots(world, live=True):
        result = []
        for bot in actors(world, 'DCLyraPlayerBotController'):
            pawn = bot.get_controlled_pawn()
            if pawn and health(pawn) and (not live or not health(pawn).is_dead_or_dying()):
                result.append(bot)
        return result

    def stop_bots(world):
        for bot in bots(world):
            brain = bot.get_editor_property('BrainComponent')
            if brain:
                brain.stop_logic('bounded registration diagnostics')
            bot.stop_movement()

    def damage(target, source):
        source_asc = asc(source)
        spec = source_asc.make_outgoing_spec(damage_class, 1, source_asc.make_effect_context())
        spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, tag('SetByCaller.Damage'), 1000000.0))
        source_asc.apply_gameplay_effect_spec_to_target(spec, asc(target))

    def equipped(pawn):
        equipment = pawn.get_component_by_class(cls('DCLyraEquipmentManagerComponent'))
        weapon = equipment.get_first_instance_of_type(cls('DCLyraRangedWeaponInstance')) if equipment else None
        return weapon and weapon.get_instigator()

    def movie_widgets(pc):
        return [x for x in u.ObjectIterator(u.UserWidget) if x.get_owning_player() == pc
                and u.MathLibrary.class_is_child_of(x.get_class(), cls('DCMissionClearWidget')) and x.is_activated()]

    def finish(ok):
        if state['done']:
            return
        state['done'] = True
        try:
            remove_immunity()
        except Exception:
            report['failures'].append(traceback.format_exc())
        report['assets_unchanged'] = s.inputs() == targets
        report['protected_hashes_unchanged'] = s.protected_hashes() == protected
        report['protected_inventory_unchanged'] = s.inventory() == before
        report['probe_passed'] = bool(ok and not report['failures'] and report['assets_unchanged']
            and report['protected_hashes_unchanged'] and report['protected_inventory_unchanged'])
        event('probe_finished', passed=report['probe_passed'], final_verification_pending=True)
        out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        u.unregister_slate_post_tick_callback(state['callback'])
        state.clear()
        state.update(done=True, busy=False)
        if not report['probe_passed']:
            u.log_error('Registration PIE failed: ' + str(report['failures']))

    def tick(delta):
        if state['done'] or state['busy']:
            return
        state['busy'] = True
        try:
            assert time.monotonic() - state['wall'] < 240, 'Registration PIE timeout'
            world = editor.get_game_world()
            if not world:
                return
            level = u.GameplayStatics.get_current_level_name(world, True)
            now = u.GameplayStatics.get_time_seconds(world)
            if level == 'Level_3':
                assert case == 'chain' and state['level'] == 'Level_2'
                assert len(report['movies'].get('Level_2', [])) >= 2
                assert state['cleared'] == {'Level_1': [15, 9], 'Level_2': [17]}
                event('existing_next_map_Level_3_loaded')
                finish(True)
                return
            assert level in ('Level_1', 'Level_2')
            index = 0 if level == 'Level_1' else 1
            pc = u.GameplayStatics.get_player_controller(world, 0)
            pawn = pc.get_controlled_pawn() if pc else None
            mode = u.GameplayStatics.get_game_mode(world)
            if not pc or not mode or not pawn or not health(pawn) or health(pawn).get_health() <= 0:
                return
            director = mode.get_stage_director()
            if not director:
                return
            if level != state['level']:
                if state['level'] is not None:
                    assert case == 'chain' and level == 'Level_2' and len(report['movies'].get('Level_1', [])) >= 2
                    assert state['immunity'] is None
                state.update(level=level, step='ready', phase=None, phase_kills=0, at=now)
                event('map_ready', level=level)
            if mode.has_run_ended() and case == 'chain':
                assert mode.was_run_successful()
                remove_immunity()
                movies = movie_widgets(pc)
                if not movies:
                    return
                assert len(movies) == 1
                player = movies[0].get_mission_clear_player()
                if player:
                    seconds = u.MathLibrary.get_total_seconds(player.get_time())
                    times = report['movies'].setdefault(level, [])
                    if not times or seconds - times[-1] > .1:
                        times.append(seconds)
                return
            if state['step'] == 'abort_wait':
                assert mode.has_run_ended() and not mode.was_run_successful()
                if now - state['at'] < 1:
                    return
                assert not actors(world, 'DCEnemySpawner') and not actors(world, 'DCLyraPlayerBotController')
                assert not movie_widgets(pc)
                remove_immunity()
                assert mode.request_restart_stage()
                event('waiting_spawners_cancelled_and_restart_requested')
                state.update(step='restart_wait', previous_time=now)
                return
            if state['step'] == 'restart_wait':
                if now >= state['previous_time'] - .5 or mode.has_run_ended():
                    return
                table = s.spawner_table(world, 0)
                assert all(a.get_spawned_count() == 0 for a in table.values())
                add_immunity(pawn)
                selected = table[s.label(23)]
                move(pawn, point(selected) + u.Vector(-100, 0, 200))
                state.update(step='restarted_spawn', at=now)
                event('new_run_spawn_counters_reset')
                return
            if state['step'] == 'restarted_spawn':
                table = s.spawner_table(world, 0)
                if table[s.label(23)].get_spawned_count() != 1:
                    return
                assert table[s.label(23)].get_alive_count() == 1
                if now - state['at'] < 1.0:
                    return
                initialized = bots(world)
                assert now - state['at'] < 10, 'Restarted bots did not initialize'
                if not initialized or any(not bot.get_editor_property('BrainComponent') or not equipped(bot.get_controlled_pawn()) for bot in initialized):
                    return
                event('new_run_bots_initialized', count=len(initialized))
                event('new_run_can_spawn_once_again')
                director.fail_stage()
                finish(True)
                return
            if case == 'abort' and state['step'] == 'abort_waiting':
                if now - state['at'] < .6:
                    return
                table = s.spawner_table(world, 0)
                assert all(a.get_spawned_count() == 0 for a in table.values())
                near = point(table[s.label(23)]) + u.Vector(-100, 0, 200)
                director.fail_stage()
                move(pawn, near)
                state.update(step='abort_wait', at=now)
                event('aborted_while_waiting_then_entered_radius')
                return
            if state['step'] == 'ready':
                if director.get_current_phase_index() != 0 or now < .3:
                    return
                s.map_facts(world, index)
                add_immunity(pawn)
                if case == 'abort':
                    move(pawn, u.Vector(-20000, -20000, 300))
                    state.update(step='abort_waiting', at=now)
                    return
                if index == 0:
                    selected = s.spawner_table(world, 0)[s.label(23)]
                    move(pawn, point(selected) + u.Vector(-7501, 0, 0))
                    state.update(step='outside', at=now)
                else:
                    state.update(step='encounters')
                return
            table = s.spawner_table(world, index)
            if state['step'] == 'outside' and now - state['at'] > .4:
                selected = table[s.label(23)]
                assert selected.get_spawned_count() == 0
                move(pawn, point(selected) + u.Vector(-7490, 0, 0))
                state.update(step='inside', at=now)
                event('outside_75m_waits')
                return
            if state['step'] == 'inside':
                selected = table[s.label(23)]
                if selected.get_spawned_count() != 1:
                    return
                assert selected.get_alive_count() == 1
                stop_bots(world)
                move(pawn, point(selected) + u.Vector(-7501, 0, 0))
                state.update(step='reentry_far', at=now)
                event('inside_75m_spawned_once')
                return
            if state['step'] == 'reentry_far' and now - state['at'] > .25:
                move(pawn, point(table[s.label(23)]) + u.Vector(-7490, 0, 0))
                state.update(step='reentry_near', at=now)
                return
            if state['step'] == 'reentry_near' and now - state['at'] > .25:
                assert table[s.label(23)].get_spawned_count() == 1
                state.update(step='encounters')
                event('reentry_kept_single_spawn')
            if state['step'] in ('outside', 'inside', 'reentry_far', 'reentry_near'):
                return
            add_immunity(pawn)
            assert all(a.get_spawned_count() <= 1 and a.get_alive_count() <= 1 for a in table.values())
            if state['step'] != 'nav_move':
                stop_bots(world)
            phase_index = director.get_current_phase_index()
            groups = s.GROUPS if index == 0 else [list(range(1, 18))]
            if not 0 <= phase_index < len(groups):
                return
            phases = list(director.get_editor_property('Phases'))
            encounter = phases[phase_index].get_editor_property('Encounter')
            registered = list(encounter.get_editor_property('Spawners'))
            expected = len(groups[phase_index])
            rows = [row for row in director.get_mission_progress()
                    if row.get_editor_property('ProgressSource') == u.DCMissionProgressSource.CURRENT_ENCOUNTER_DEFEATS]
            if not rows:
                return
            assert len(rows) == 1 and rows[0].get_editor_property('TargetCount') == expected
            if state['phase'] != phase_index:
                state.update(phase=phase_index, phase_kills=0)
                event('mission_total', level=level, phase=phase_index, target=expected)
            for spawner in registered:
                if spawner.get_spawned_count() == 0:
                    move(pawn, point(spawner) + u.Vector(-100, 0, 200))
                    return
            living = [bot for bot in bots(world) if equipped(bot.get_controlled_pawn())]
            if not living:
                return
            if index == 1 and not state['nav_done']:
                bot = next((b for b in living if b.get_path_name() == state.get('nav_bot')), living[0])
                bot_pawn = bot.get_controlled_pawn()
                if state['step'] != 'nav_move':
                    origin = bot_pawn.get_actor_location()
                    state.update(step='nav_move', nav_bot=bot.get_path_name(), nav_origin=[origin.x, origin.y, origin.z],
                                 nav_candidate=0, nav_started=now, nav_last=-10, nav_goal=None)
                offsets = [(400, 0), (-400, 0), (0, 400), (0, -400), (300, 300), (-300, 300), (300, -300), (-300, -300)]
                assert now - state['nav_started'] < 35, 'Level_2 AI did not follow a complete local NavMesh path'
                if state['nav_goal'] is None or now - state['nav_last'] > 3:
                    dx, dy = offsets[state['nav_candidate'] % len(offsets)]
                    state['nav_candidate'] += 1
                    origin = u.Vector(*state['nav_origin'])
                    goal = origin + u.Vector(dx, dy, 0)
                    result = bot.move_to_location(goal, 20, True, True, True, True, None, False)
                    state.update(nav_last=now, nav_goal=[goal.x, goal.y, goal.z])
                    event('navigation_request', result=str(result))
                position = bot_pawn.get_actor_location()
                displacement = position.distance(u.Vector(*state['nav_origin']))
                remaining = position.distance(u.Vector(*state['nav_goal']))
                if displacement > 100 and remaining < 125:
                    state.update(nav_done=True, step='encounters')
                    event('Level_2_AI_followed_complete_local_path', displacement_cm=displacement, remaining_cm=remaining)
                return
            for future in range(phase_index + 1, len(groups)):
                assert all(table[s.label(i)].get_spawned_count() == 0 for i in groups[future])
            defeated = sum(a.get_defeated_count() for a in registered)
            assert rows[0].get_editor_property('CurrentCount') == defeated
            damage(living[0].get_controlled_pawn(), pawn)
            after_defeated = sum(a.get_defeated_count() for a in registered)
            assert after_defeated == defeated + 1
            if after_defeated == expected:
                assert encounter.is_encounter_completed()
                state['cleared'].setdefault(level, []).append(expected)
                event('encounter_count_completed', level=level, phase=phase_index, defeated=after_defeated)
        except Exception:
            report['failures'].append(traceback.format_exc())
            finish(False)
        finally:
            state['busy'] = False

    state['callback'] = u.register_slate_post_tick_callback(tick)
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    event('armed', run=run, case=case, diagnostic_teleports_and_damage=True)


if __name__ == '__main__':
    if '--verify-report' in sys.argv:
        pos = sys.argv.index('--verify-report')
        assert len(sys.argv[pos + 1:]) == 4
        raise SystemExit(verify_report(*sys.argv[pos + 1:]))
    arm()

