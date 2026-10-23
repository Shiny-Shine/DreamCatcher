"""Real Level_1 PIE probe after user build + spawner setup/fresh verify.

Arm before Project.Maps.PIE in an isolated unattended editor. Teleports, temporary
player immunity and diagnostic GAS damage test spawn/lifecycle conditions, not
physical input, real weapon hits, visuals, audio or network replication.
No asset/config saves or C++ build. Native tests cover multiple pending spawners.
Outside Unreal:
  --commands RUN: print an editor command without executing it.
  --verify-report REPORT LOG AUTOMATION_INDEX EXIT_CODE: finalize after exit.
"""
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import time
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / 'Saved/Diagnostics/SpawnerProximity'
MAP = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'


def setup_support():
    spec = importlib.util.spec_from_file_location('dc_proximity_setup_support',
                                                  Path(__file__).with_name('spawner_proximity_setup.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.is_relative_to(ROOT.resolve()), 'Unexpected report destination'
    report = json.loads(path.read_text(encoding='utf-8'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    tests = [test for test in index.get('tests', []) if test.get('fullTestPath') == 'Project.Maps.PIE']
    automation_ok = bool(tests) and all(test.get('state') == 'Success' for test in tests)
    automation_ok = automation_ok and all(int(index.get(key, -1)) == 0 for key in ('failed', 'notRun', 'inProcess'))
    errors = [line for line in log.splitlines() if re.search(
        r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    matching_run = any('SPAWNER_PROXIMITY_PIE ' in line and '"event": "armed"' in line
                       and '"run": "' + report['run'] + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=automation_ok,
        matching_run_log=matching_run, engine_errors=errors,
        engine_warnings=[line for line in log.splitlines() if ': Warning:' in line],
        log=str(Path(log_path).resolve()), automation_index=str(Path(index_path).resolve()))
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0
                              and automation_ok and matching_run and not errors)
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], errors=len(errors), automation_passed=automation_ok)))
    return 0 if report['complete'] else 1


def arm():
    import unreal as u
    support = setup_support()
    command = u.SystemLibrary.get_command_line()
    run_match = re.search(r'-DCSpawnerProximityProbe=([A-Za-z0-9_]+)\b', command)
    assert run_match and '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert not u.SourceControl.is_enabled()
    support.editor_support().assert_no_other_editor()
    settings = re.search(r'"-GameUserSettingsINI=([^"]+)"|-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', command, re.I)
    assert settings, 'Use an isolated settings INI'
    assert Path(next(value for value in settings.groups() if value)).resolve().is_relative_to(ROOT.resolve())
    editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
    assert not editor.get_game_world(), 'Arm before PIE'
    run = run_match.group(1)
    directory = ROOT / ('PIE_' + run)
    directory.mkdir(parents=True, exist_ok=True)
    out = directory / 'probe.json'
    assert not out.exists(), 'Do not overwrite previous evidence'
    before, hashes = support.inventory(), support.protected_hashes()
    bp_hash = support.sha(support.TARGET_FILE)
    support.inspect_map(u, require_proximity=True)
    cls = lambda name: u.load_class(None, '/Script/DreamCatcher.' + name)
    gas = u.get_default_object(u.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
    damage_class = u.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
    assert damage_class
    experience = u.get_default_object(u.load_asset('/Game/DreamCatcher/Experiences/B_DC_StageUIExperience').generated_class())
    pawn_data = experience.get_editor_property('BotPawnData') or experience.get_editor_property('DefaultPawnData')
    expected_pawn_class = pawn_data.get_editor_property('PawnClass')
    state = dict(step='ready', at=0.0, wall=time.monotonic(), done=False, busy=False, immune_pawn=None)
    report = dict(run=run, complete=False, probe_passed=False, events=[], failures=[], asset_saves=0,
                  cpp_build=False, user_visual_audio_verified=False, physical_input_verified=False,
                  real_weapon_damage_verified=False, network_replication_verified=False,
                  expected_bot_pawn_class=expected_pawn_class.get_path_name())

    def event(name, **data):
        row = dict(event=name, **data)
        report['events'].append(row)
        print('SPAWNER_PROXIMITY_PIE ' + json.dumps(row, ensure_ascii=False))

    def actors(world, name):
        return list(u.GameplayStatics.get_all_actors_of_class(world, cls(name)))

    def health(pawn):
        return pawn.get_component_by_class(cls('DCLyraHealthComponent')) if pawn else None

    def tags(name):
        value = u.GameplayTagContainer()
        assert value.import_text('(GameplayTags=((TagName="' + name + '")))')
        return value

    def remove_immunity():
        if state['immune_pawn'] is not None:
            assert gas.call_method('RemoveLooseGameplayTags', args=(state['immune_pawn'], tags('Gameplay.DamageImmunity'), False))
            state['immune_pawn'] = None

    def damage(target, source):
        asc = gas.call_method('GetAbilitySystemComponent', args=(source,))
        spec = asc.make_outgoing_spec(damage_class, 1.0, asc.make_effect_context())
        tag = u.GameplayTag()
        assert tag.import_text('(TagName="SetByCaller.Damage")')
        spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, tag, 100000.0))
        asc.apply_gameplay_effect_spec_to_target(spec, gas.call_method('GetAbilitySystemComponent', args=(target,)))

    def move_player(pawn, point):
        pawn.get_movement_component().stop_movement_immediately()
        pawn.get_movement_component().disable_movement()
        pawn.set_actor_location(point, False, True)
        assert (pawn.get_actor_location() - point).length() < 0.1

    def point(spawner):
        return spawner.get_editor_property('SpawnPoint').get_world_location()

    def live_bots(world):
        return [bot for bot in actors(world, 'DCLyraPlayerBotController')
                if bot.get_controlled_pawn() and health(bot.get_controlled_pawn())
                and not health(bot.get_controlled_pawn()).is_dead_or_dying()]

    def equipped_bot(world):
        bots = live_bots(world)
        if len(bots) != 1:
            return None
        pawn = bots[0].get_controlled_pawn()
        equipment = pawn.get_component_by_class(cls('DCLyraEquipmentManagerComponent'))
        weapon = equipment.get_first_instance_of_type(cls('DCLyraRangedWeaponInstance')) if equipment else None
        if not weapon or not weapon.get_instigator():
            return None
        assert pawn.get_class() == expected_pawn_class, 'Existing BotPawnData path changed'
        brain = bots[0].get_editor_property('BrainComponent')
        if brain:
            brain.stop_logic('bounded proximity spawn probe')
        bots[0].stop_movement()
        return pawn

    def finish(passed):
        if state['done']:
            return
        state['done'] = True
        try:
            remove_immunity()
        except Exception:
            report['failures'].append(traceback.format_exc())
        report['protected_inventory_unchanged'] = support.inventory() == before
        report['protected_hashes_unchanged'] = support.protected_hashes() == hashes
        report['spawner_bp_unchanged'] = support.sha(support.TARGET_FILE) == bp_hash
        report['probe_passed'] = bool(passed and not report['failures'] and report['protected_inventory_unchanged']
                                      and report['protected_hashes_unchanged'] and report['spawner_bp_unchanged'])
        event('probe_finished', passed=report['probe_passed'], final_verification_pending=True)
        out.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        u.unregister_slate_post_tick_callback(state['callback'])
        state.clear()
        state.update(done=True, busy=False)
        if not report['probe_passed']:
            u.log_error('Spawner proximity PIE probe failed: ' + str(report['failures']))

    def tick(delta):
        if state['done'] or state['busy']:
            return
        state['busy'] = True
        try:
            assert time.monotonic() - state['wall'] < 180, 'Proximity probe timed out'
            world = editor.get_game_world()
            if not world:
                return
            assert world.get_path_name().startswith(MAP.rsplit('/', 1)[0] + '/UEDPIE_')
            now = u.GameplayStatics.get_time_seconds(world)
            pc = u.GameplayStatics.get_player_controller(world, 0)
            pawn = pc.get_controlled_pawn() if pc else None
            mode = u.GameplayStatics.get_game_mode(world)
            director = mode.get_stage_director() if mode else None
            if not pawn or not health(pawn) or health(pawn).get_health() <= 0 or not director:
                return
            if state['step'] == 'restart_wait':
                if now >= state['previous_world_time'] - 1.0 or director.get_current_phase_index() != 0:
                    return
                current = {a.get_name(): a for a in actors(world, 'DCEnemySpawner')}
                first = current['BP_DC_LyraFiniteSpawner_C_0']
                assert first.get_spawned_count() == 0 and not first.is_spawner_cleared()
                origin = point(first)
                state.update(step='fresh_wait', at=now, near=[origin.x + 1990, origin.y, origin.z])
                event('fresh_run_resets_single_use', time=now)
                return
            if state['step'] == 'fresh_wait' and now - state['at'] > 0.6:
                assert not live_bots(world)
                director.fail_stage()
                move_player(pawn, u.Vector(*state['near']))
                state.update(step='aborted_wait', at=now)
                event('aborted_while_waiting_then_entered_radius', time=now)
                return
            if state['step'] == 'aborted_wait' and now - state['at'] > 1.0:
                assert mode.has_run_ended() and not mode.was_run_successful()
                assert not actors(world, 'DCLyraPlayerBotController') and not actors(world, 'DCBossCharacter')
                assert not actors(world, 'DCEnemySpawner')
                event('no_delayed_spawn_after_abort', time=now)
                finish(True)
                return
            if state['step'] in ('fresh_wait', 'aborted_wait'):
                return
            if state['step'] == 'cleanup':
                if actors(world, 'DCLyraPlayerBotController') or actors(world, 'DCBossCharacter'):
                    return
                remove_immunity()
                assert mode.has_run_ended() and mode.request_restart_stage()
                event('explicit_restart_requested', time=now)
                state.update(step='restart_wait', previous_world_time=now)
                return
            spawners = {a.get_name(): a for a in actors(world, 'DCEnemySpawner')}
            first = spawners['BP_DC_LyraFiniteSpawner_C_0']
            second = spawners['BP_DC_LyraFiniteSpawner_C_1']
            boss_spawner = spawners['BP_DC_BossSpawner_C_0']
            if state['step'] == 'ready':
                if director.get_current_phase_index() != 0:
                    return
                assert first.get_spawned_count() == second.get_spawned_count() == 0
                assert gas.call_method('AddLooseGameplayTags', args=(pawn, tags('Gameplay.DamageImmunity'), False))
                state['immune_pawn'] = pawn
                move_player(pawn, point(first) + u.Vector(2200, 0, 0))
                state.update(step='outside', at=now)
            elif state['step'] == 'outside' and now - state['at'] > 0.6:
                assert first.get_spawned_count() == second.get_spawned_count() == 0
                assert not first.is_spawner_cleared() and director.get_current_phase_index() == 0
                event('outside_waits_without_advancing', time=now)
                move_player(pawn, point(first) + u.Vector(1990, 0, 0))
                state.update(step='first_spawn', at=now)
            elif state['step'] == 'first_spawn':
                bot_pawn = equipped_bot(world)
                if bot_pawn is None:
                    return
                assert first.get_spawned_count() == first.get_alive_count() == 1
                assert second.get_spawned_count() == 0, 'Future encounter spawned early'
                event('first_approach_spawned_one_equipped_bot', time=now, pawn=bot_pawn.get_class().get_path_name())
                move_player(pawn, point(first) + u.Vector(2200, 0, 0))
                state.update(step='reenter_far', at=now)
            elif state['step'] == 'reenter_far' and now - state['at'] > 0.4:
                move_player(pawn, point(first) + u.Vector(1990, 0, 0))
                state.update(step='reenter_near', at=now)
            elif state['step'] == 'reenter_near' and now - state['at'] > 0.4:
                assert first.get_spawned_count() == first.get_alive_count() == 1
                assert len(live_bots(world)) == 1
                event('reentry_did_not_spawn_again', time=now)
                move_player(pawn, point(second) + u.Vector(1990, 0, 0))
                damage(live_bots(world)[0].get_controlled_pawn(), pawn)
                state.update(step='second_spawn', at=now)
            elif state['step'] == 'second_spawn':
                if director.get_current_phase_index() != 1 or second.get_spawned_count() != 1:
                    return
                bot_pawn = equipped_bot(world)
                if bot_pawn is None:
                    return
                assert first.is_spawner_cleared() and first.get_spawned_count() == 1
                assert second.get_alive_count() == 1
                event('already_near_at_next_phase_spawned_once', time=now)
                move_player(pawn, point(boss_spawner) + u.Vector(5000, 0, 1000))
                damage(bot_pawn, pawn)
                state.update(step='boss', at=now)
            elif state['step'] == 'boss':
                bosses = actors(world, 'DCBossCharacter')
                if director.get_current_phase_index() != 2 or len(bosses) != 1 or not bosses[0].is_gas_ready():
                    return
                assert second.is_spawner_cleared() and second.get_spawned_count() == 1
                assert boss_spawner.get_spawned_count() == 1
                assert boss_spawner.get_editor_property('bRequirePlayerProximity') is False
                event('boss_still_spawns_outside_player_radius', time=now)
                director.fail_stage()
                state.update(step='cleanup', at=now)
        except Exception:
            report['failures'].append(traceback.format_exc())
            finish(False)
        finally:
            state['busy'] = False

    state['callback'] = u.register_slate_post_tick_callback(tick)
    # -ExecutePythonScript otherwise queues QUIT_EDITOR on the next frame.
    # Let Automation's -TestExit close the editor after exporting its results.
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    event('armed', run=run, PIE=True, diagnostic_teleports_and_damage=True)


if __name__ == '__main__':
    if '--verify-report' in sys.argv:
        pos = sys.argv.index('--verify-report')
        assert len(sys.argv[pos + 1:]) == 4
        raise SystemExit(verify_report(*sys.argv[pos + 1:]))
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        run = sys.argv[2]
        assert re.fullmatch(r'[A-Za-z0-9_]+', run)
        folder = ROOT / ('PIE_' + run)
        assert not folder.exists(), 'Print commands with a fresh run id'
        print(subprocess.list2cmdline([
            'E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',
            str(PROJECT / 'DreamCatcher.uproject'), MAP, '-unattended', '-SCCProvider=None',
            '-RenderOffscreen', '-NoSound', '-NoSplash', '-DCSpawnerProximityProbe=' + run,
            '-ExecutePythonScript=' + str(Path(__file__).resolve()),
            '-ExecCmds=t.MaxFPS 60,Automation RunTests Project.Maps.PIE', '-TestExit=Automation Test Queue Empty',
            '-ini:Engine:[/Script/Engine.AutomationTestSettings]:PIETestDuration=60',
            '-ReportExportPath=' + str(folder / 'Automation'),
            '-GameUserSettingsINI=' + str(folder / 'GameUserSettings.ini'),
            '-abslog=' + str(folder / 'Editor.log')]))
    else:
        arm()
