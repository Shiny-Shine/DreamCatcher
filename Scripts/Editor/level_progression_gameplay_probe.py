"""Rendered PIE: natural clear-movie completion/travel, death and media-error recovery.

No asset/config saves, C++ build, packaging or physical-input/audio acceptance.
Diagnostic movement input, proximity teleports, temporary immunity and GAS damage
drive the existing encounters. Movie decoding/time/end/travel are never mocked.
--commands RUN prints isolated PIE commands; --verify-report REPORT LOG INDEX EXIT
checks final process/Automation/log evidence after the editor has exited.
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
ROOT = PROJECT / 'Saved/Diagnostics/LevelProgression20261023'


def support_module():
    spec = importlib.util.spec_from_file_location('dc_progression_probe_support', Path(__file__).with_name('level_progression_setup.py'))
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value)
    return value


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve(); assert path.is_relative_to(ROOT.resolve())
    report = json.loads(path.read_text(encoding='utf-8'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    automation_ok = bool(tests) and all(row.get('state') == 'Success' for row in tests)
    automation_ok &= all(int(index.get(key, -1)) == 0 for key in ('failed', 'notRun', 'inProcess'))
    errors = [line for line in log.splitlines() if re.search(r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    end_count = log.count('[MissionClear] Movie ended; requesting configured next screen.')
    required_ends = 4 - report.get('start_level', 1) if report['case'] == 'chain' else 0
    screenshots = []
    for requested in report.get('screenshots', []):
        requested_path = Path(requested).resolve()
        assert requested_path.is_relative_to(path.parent)
        # UE's Shot command appends a numeric suffix even with nosuffix.
        found = sorted(requested_path.parent.glob(requested_path.stem + '*.png'))
        screenshots.extend(str(item) for item in found if item.stat().st_size > 0)
    screenshots_ok = len(screenshots) >= len(report.get('screenshots', []))
    decoded_frame_guard_ok = report['case'] != 'decode_error' or '클리어 영상의 화면을 재생하지 못했습니다.' in log
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=automation_ok,
        natural_media_end_events=end_count, engine_errors=errors,
        screenshot_files=screenshots, screenshots_present=screenshots_ok,
        decoded_frame_failure_observed=report['case'] == 'decode_error' and decoded_frame_guard_ok,
        engine_warnings=[line for line in log.splitlines() if ': Warning:' in line])
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0 and automation_ok and not errors
                              and end_count == required_ends and screenshots_ok and decoded_frame_guard_ok)
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], end_events=end_count, errors=len(errors))))
    return 0 if report['complete'] else 1


def arm():
    import unreal as u
    support = support_module()
    command = u.SystemLibrary.get_command_line()
    run = re.search(r'-DCProgressionProbe=([A-Za-z0-9_]+)\b', command).group(1)
    case = re.search(r'-DCProgressionCase=(chain|death|media_error|decode_error)\b', command).group(1)
    start_match = re.search(r'-DCProgressionStartLevel=([123])\b', command)
    start_level = int(start_match.group(1)) if start_match and case == 'chain' else (1 if case == 'chain' else 2)
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert '-nullrhi' not in command.lower(), 'Real movie decoding/presentation requires a renderer'
    assert not u.SourceControl.is_enabled()
    support.helpers().hud.assert_no_other_editor()
    settings = re.search(r'"-GameUserSettingsINI=([^"]+)"|-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', command, re.I)
    assert settings and Path(next(part for part in settings.groups() if part)).resolve().is_relative_to(ROOT.resolve())
    directory = ROOT / ('PIE_' + run); directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'probe.json'; assert not output.exists()
    protected = support.protected_hashes()
    asset_hashes = {str(support.disk(package)): support.sha(support.disk(package)) for package in
                    set(support.ASSET_WRITES + support.MAPS + [support.FRONTEND, support.EXPERIENCE])}
    asset_hashes[str(support.CONFIG)] = support.sha(support.CONFIG)
    asset_hashes[str(support.MOVIE)] = support.sha(support.MOVIE)
    before_inventory = support.inventory()
    report = dict(run=run, case=case, start_level=start_level, complete=False, probe_passed=False, events=[], failures=[], movies={},
                  screenshots=[], asset_saves=0, physical_input_or_audio_acceptance=False,
                  tick_counts=dict(callbacks=0, no_world=0, no_controller=0, movie_lookup_misses=0, media_frames=0))
    state = dict(step='ready', wall=time.monotonic(), busy=False, done=False, index=start_level - 1,
                 killed=set(), phase=None, immunity_pawn=None, navigation_verified=set())
    editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
    assert not editor.get_game_world(), 'Arm before Project.Maps.PIE starts'
    cls = lambda name: u.load_class(None, '/Script/DreamCatcher.' + name)
    gas = u.get_default_object(u.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
    subs = u.get_default_object(u.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
    input_class = u.load_class(None, '/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')
    damage_class = u.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
    assert damage_class and cls('DCMissionClearWidget')
    input_data = u.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')

    def tag(name):
        value = u.GameplayTag(); assert value.import_text('(TagName="' + name + '")'); return value

    move = input_data.find_native_input_action_for_tag(tag('InputTag.Move'), True)
    fire = input_data.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'), True)
    blocked_tag = tag('Gameplay.AbilityInputBlocked')
    immunity_tags = u.GameplayTagContainer(); assert immunity_tags.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
    error_defaults = u.get_default_object(u.load_asset(support.MODES[1]).generated_class()) if case in ('media_error', 'decode_error') else None
    original_movie = error_defaults.get_editor_property('MissionClearSource') if error_defaults else None

    def event(name, **values):
        row = dict(event=name, wall=time.monotonic() - state['wall'], **values)
        report['events'].append(row); print('DC_PROGRESSION_PIE ' + json.dumps(row, ensure_ascii=False))

    def actors(world, name):
        return list(u.GameplayStatics.get_all_actors_of_class(world, cls(name)))

    def health(pawn):
        return pawn.get_component_by_class(cls('DCLyraHealthComponent')) if pawn else None

    def asc(pawn):
        return gas.call_method('GetAbilitySystemComponent', args=(pawn,))

    def inject(pc, action, x, y=0):
        system = subs.call_method('GetLocalPlayerSubSystemFromPlayerController', args=(pc, input_class))
        assert system; system.inject_input_vector_for_action(action, u.Vector(x, y, 0), [], [])

    def immunity(pawn, enable):
        assert gas.call_method('AddLooseGameplayTags' if enable else 'RemoveLooseGameplayTags', args=(pawn, immunity_tags, False))

    def damage(target, source):
        source_asc = asc(source); target_asc = asc(target)
        assert source_asc and target_asc
        spec = source_asc.make_outgoing_spec(damage_class, 1, source_asc.make_effect_context())
        spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, tag('SetByCaller.Damage'), 1000000.0))
        source_asc.apply_gameplay_effect_spec_to_target(spec, target_asc)

    def widgets(pc, name):
        kind = cls(name)
        return [value for value in u.ObjectIterator(u.UserWidget) if value.get_class() == kind
                and value.get_owning_player() == pc and not value.get_name().startswith('Default__')]

    def finish(ok):
        if state['done']:
            return
        state['done'] = True
        if error_defaults:
            error_defaults.set_editor_property('MissionClearSource', original_movie)
            report['memory_override_restored'] = error_defaults.get_editor_property('MissionClearSource') == original_movie
        report['asset_hashes_unchanged'] = all(Path(path).is_file() and support.sha(Path(path)) == value for path, value in asset_hashes.items())
        report['protected_inventory_unchanged'] = support.inventory() == before_inventory
        report['protected_hashes_unchanged'] = support.protected_hashes() == protected
        report['probe_passed'] = bool(ok and not report['failures'] and report['asset_hashes_unchanged']
                                      and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged'])
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        u.unregister_slate_post_tick_callback(state['callback'])
        if not report['probe_passed']:
            u.log_error('Level progression PIE failed: ' + str(output))
        print('DC_PROGRESSION_PROBE_RESULT ' + json.dumps(dict(passed=report['probe_passed'], report=str(output))))

    def tick(delta):
        if state['done'] or state['busy']:
            return
        state['busy'] = True
        report['tick_counts']['callbacks'] += 1
        try:
            assert time.monotonic() - state['wall'] < 220, 'PIE progression probe timed out'
            world = editor.get_game_world()
            if not world or 'UEDPIE_' not in world.get_path_name():
                report['tick_counts']['no_world'] += 1
                return
            now = u.GameplayStatics.get_time_seconds(world)
            level = u.GameplayStatics.get_current_level_name(world, True)
            pc = u.GameplayStatics.get_player_controller(world, 0)
            mode = u.GameplayStatics.get_game_mode(world)
            if not pc or not mode:
                report['tick_counts']['no_controller'] += 1
                return
            if level == 'L_DC_FrontEnd':
                assert state['step'] in ('travel_wait', 'movie', 'error_return_wait')
                if not isinstance(mode, u.DCFrontendGameMode):
                    return
                screens = widgets(pc, 'DCFrontendScreen')
                # The actual screen is a Blueprint subclass; use reflected inheritance.
                screens = [value for value in u.ObjectIterator(u.UserWidget) if value.get_owning_player() == pc
                           and u.MathLibrary.class_is_child_of(value.get_class(), cls('DCFrontendScreen')) and value.is_activated()]
                if not screens:
                    return
                assert not pc.get_controlled_pawn()
                assert case in ('media_error', 'decode_error') or (state['index'] == 2 and len(report['movies'].get('Level_3', {}).get('times', [])) >= 2)
                event('mode_selection_ready', map=world.get_path_name()); finish(True); return
            expected = 'Level_' + str(state['index'] + 1)
            if state['step'] in ('travel_wait', 'movie') and level != expected:
                assert case == 'chain' and level == 'Level_' + str(state['index'] + 2), (expected, level)
                assert len(report['movies'].get(expected, {}).get('times', [])) >= 2
                state.update(index=state['index'] + 1, step='ready', killed=set(), phase=None, immunity_pawn=None)
                expected = level
                event('next_map_loaded', level=level)
            assert level == expected, (level, expected)
            if not isinstance(mode, u.DCStageRunGameMode):
                return
            pawn = pc.get_controlled_pawn()
            if state['step'] == 'death_wait':
                if not mode.has_run_ended() or mode.was_run_successful() or pawn or not widgets(pc, 'DCStageResultLayer'):
                    return
                if now - state['at'] < 3:
                    return
                assert not widgets(pc, 'DCMissionClearWidget')
                event('death_kept_existing_result_and_no_clear_movie')
                assert mode.request_restart_stage()
                state.update(step='restart_wait'); return
            if state['step'] == 'restart_wait':
                if not pawn or not health(pawn) or health(pawn).is_dead_or_dying() or mode.has_run_ended():
                    return
                if not mode.get_stage_director() or not actors(world, 'DCLyraPlayerBotController'):
                    return
                event('death_retry_new_run_ready', level=level)
                finish(True); return
            if mode.has_run_ended():
                assert mode.was_run_successful()
                movies = widgets(pc, 'DCMissionClearWidget')
                movies = [value for value in movies if value.is_activated()]
                if not movies:
                    report['tick_counts']['movie_lookup_misses'] += 1
                    return
                assert len(movies) == 1 and not widgets(pc, 'DCStageResultLayer')
                movie = movies[0]
                if not (pc.is_move_input_ignored() and pc.is_look_input_ignored()):
                    # The UI and GameMode next-tick callbacks can run on opposite
                    # sides of this observer. Require blocking before fading/video.
                    assert movie.get_blackout_opacity() <= .001 and not movie.get_mission_clear_player()
                    return
                assert asc(pawn).get_gameplay_tag_count(blocked_tag) > 0
                if case in ('media_error', 'decode_error'):
                    if not movie.is_showing_failure():
                        return
                    assert not movie.get_mission_clear_player() and not mode.should_play_mission_clear()
                    event('missing_media_error_controls_ready' if case == 'media_error' else 'undecoded_media_error_controls_ready')
                    movie.call_method('ReturnToModes')
                    assert mode.is_stage_travel_pending()
                    state.update(step='error_return_wait'); return
                player = movie.get_mission_clear_player()
                if player:
                    report['tick_counts']['media_frames'] += 1
                    assert abs(movie.get_blackout_opacity() - 1) < .001
                    seconds = u.MathLibrary.get_total_seconds(player.get_time())
                    duration = u.MathLibrary.get_total_seconds(player.get_duration())
                    if duration <= 0:
                        return
                    entry = report['movies'].setdefault(level, dict(times=[], duration=duration))
                    elapsed = time.monotonic() - entry.setdefault('first_observed_wall', time.monotonic())
                    if elapsed - entry.get('last_state_log', -1) >= .5:
                        entry['last_state_log'] = elapsed
                        event('media_state', level=level, seconds=seconds, rate=player.get_rate(), playing=player.is_playing(),
                              paused=player.is_paused(), buffering=player.is_buffering(), game_time=now,
                              display_seconds=u.MathLibrary.get_total_seconds(player.get_display_time()),
                              world_paused=u.GameplayStatics.is_game_paused(world))
                    if not entry['times'] or seconds - entry['times'][-1] > .05:
                        entry['times'].append(seconds)
                    assert duration > 0 and not mode.is_stage_travel_pending(), 'Cannot travel while the movie is still playing'
                    if elapsed > 2 and level not in state.setdefault('screenshots', set()):
                        filename = str(directory / ('movie_' + level + '.png'))
                        # HighResShot excludes UI and can stall the seven-second
                        # media sequence. Capture the ordinary viewport with UI.
                        u.SystemLibrary.execute_console_command(world, 'Shot showui filename="' + filename.replace('\\', '/') + '" nosuffix')
                        report['screenshots'].append(filename); state['screenshots'].add(level)
                    state.update(step='movie')
                elif mode.is_stage_travel_pending():
                    entry = report['movies'].get(level)
                    assert entry and len(entry['times']) >= 2, 'No actual advancing media clock observed'
                    event('natural_movie_completion_before_travel', level=level, last_time=entry['times'][-1])
                    state.update(step='travel_wait')
                return
            if not pawn or not health(pawn) or health(pawn).get_max_health() <= 0 or not mode.get_stage_director():
                return
            quickbar = pc.get_component_by_class(cls('DCLyraQuickBarComponent'))
            if not quickbar or not quickbar.get_active_slot_item():
                return
            if state['step'] == 'ready':
                if now <= .5:
                    return
                assert mode.get_class().get_path_name() == support.MODES[state['index']] + '.' + support.MODES[state['index']].rsplit('/', 1)[-1] + '_C'
                if not widgets(pc, 'DCStageOverlayWidget'):
                    return
                state.update(step='move', at=now, origin=pawn.get_actor_location(), ammo=quickbar.get_active_slot_item().get_stat_tag_stack_count(tag('Lyra.ShooterGame.Weapon.MagazineAmmo')))
                assert state['ammo'] > 0
                assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
            if state['step'] == 'move':
                inject(pc, move, 0, 1)
                if now - state['at'] < .5:
                    return
                inject(pc, move, 0, 0)
                assert pawn.get_actor_location().distance(state['origin']) > 5, 'New map movement did not recover'
                state.update(step='encounters' if case == 'death' else 'shoot', at=now)
                if case == 'death':
                    event('play_setup_ready', level=level, pawn=pawn.get_class().get_path_name(), ammo_used=0)
            if state['step'] == 'shoot':
                ammo = quickbar.get_active_slot_item().get_stat_tag_stack_count(tag('Lyra.ShooterGame.Weapon.MagazineAmmo'))
                if ammo >= state['ammo']:
                    assert now - state['at'] < 5, 'New map fire input did not consume ammo'
                    inject(pc, fire, 1)
                    return
                inject(pc, fire, 0)
                event('play_setup_ready', level=level, pawn=pawn.get_class().get_path_name(), ammo_used=state['ammo'] - ammo)
                state.update(step='encounters')
            if case != 'death' and state['immunity_pawn'] != pawn.get_path_name():
                immunity(pawn, True); state['immunity_pawn'] = pawn.get_path_name()
            director = mode.get_stage_director()
            phase_index = director.get_current_phase_index()
            phases = list(director.get_editor_property('Phases'))
            if phase_index < 0 or phase_index >= len(phases):
                return
            encounter = phases[phase_index].get_editor_property('Encounter')
            if not encounter:
                return
            for spawner in encounter.get_editor_property('Spawners'):
                if spawner.get_spawned_count() == 0 and spawner.get_editor_property('bRequirePlayerProximity'):
                    position = spawner.get_editor_property('SpawnPoint').get_world_location()
                    pawn.set_actor_location(position + u.Vector(-350, 0, 200), False, True)
            bot_pawns = [bot.get_controlled_pawn() for bot in actors(world, 'DCLyraPlayerBotController')]
            targets = bot_pawns + actors(world, 'DCBossCharacter')
            live = [target for target in targets if target and health(target) and health(target).get_max_health() > 0
                    and asc(target) and not health(target).is_dead_or_dying()
                    and target.get_path_name() not in state['killed']]
            if case == 'death':
                if live:
                    damage(pawn, live[0]); state.update(step='death_wait', at=now)
                return
            if case == 'chain' and state['index'] > 0 and level not in state['navigation_verified'] and live:
                # Reuse the existing Level_1 probe's live AI MoveTo trial. Static
                # NavigationSystem wrappers can resolve a CDO world in PIE.
                bot = next(bot for bot in actors(world, 'DCLyraPlayerBotController') if bot.get_controlled_pawn() in live)
                bot_pawn = bot.get_controlled_pawn()
                if state['step'] == 'encounters':
                    bot.get_editor_property('BrainComponent').stop_logic('Approved sample encounter navigation probe')
                    bot.stop_movement()
                    spawn_goal = next(iter(encounter.get_editor_property('Spawners'))).get_editor_property('SpawnPoint').get_world_location()
                    start_goal = next(iter(u.GameplayStatics.get_all_actors_of_class(world, u.PlayerStart))).get_actor_location()
                    # Both endpoints are empty. A goal on the living player's
                    # capsule can be held outside reach by crowd avoidance.
                    goals = [spawn_goal, (spawn_goal + start_goal) * .5]
                    goal = max(goals, key=lambda point: (point - bot_pawn.get_actor_location()).length())
                    state.update(step='nav_pause', at=now, nav_deadline=now + 75,
                                 nav_goal=[goal.x, goal.y, goal.z], nav_origin=bot_pawn.get_actor_location(),
                                 last_nav_request=-10, last_nav_state=-10)
                    return
                goal = u.Vector(*state['nav_goal'])
                assert now < state['nav_deadline'], 'Sample enemy never followed a complete navigation path: ' + level
                if state['step'] == 'nav_pause':
                    if now - state['at'] < .5 or now - state['last_nav_request'] < .5:
                        return
                    state['last_nav_request'] = now
                    request = bot.move_to_location(goal, 10, True, True, True, True, None, False)
                    if request == u.PathFollowingRequestResult.FAILED:
                        return
                    state.update(step='nav_move')
                    event('complete_navigation_path_requested', level=level, request=str(request), goal=state['nav_goal'])
                    return
                if state['step'] == 'nav_move':
                    displacement = (bot_pawn.get_actor_location() - state['nav_origin']).length()
                    remaining = (bot_pawn.get_actor_location() - goal).length()
                    if now - state['last_nav_state'] > 2:
                        state['last_nav_state'] = now
                        movement = bot_pawn.get_component_by_class(u.CharacterMovementComponent)
                        event('navigation_state', level=level, position=bot_pawn.get_actor_location().export_text(),
                              displacement_cm=displacement, remaining_cm=remaining, velocity=bot_pawn.get_velocity().export_text(),
                              move_status=str(bot.get_move_status()), mode=str(movement.get_editor_property('MovementMode')),
                              max_walk_speed=movement.get_editor_property('MaxWalkSpeed'),
                              movement_stopped_tags=asc(bot_pawn).get_gameplay_tag_count(tag('Gameplay.MovementStopped')))
                    if remaining >= 100:
                        return
                    assert displacement > 100, 'Navigation trial did not produce actual enemy movement'
                    bot.stop_movement()
                    state['navigation_verified'].add(level)
                    state.update(step='encounters')
                    event('enemy_followed_complete_navigation_path', level=level, displacement_cm=displacement, remaining_cm=remaining)
            for target in live:
                state['killed'].add(target.get_path_name()); damage(target, pawn)
                event('existing_enemy_death', level=level, phase=phase_index, target=target.get_class().get_path_name())
        except Exception:
            report['failures'].append(traceback.format_exc()); finish(False)
        finally:
            state['busy'] = False

    if error_defaults:
        # EditDefaultsOnly is authored on its CDO, before PIE creates the mode.
        # This process-only override is restored even when the probe fails.
        error_source = None
        if case == 'decode_error':
            applied = json.loads((ROOT / 'Apply_A/report.json').read_text(encoding='utf-8'))
            original_path = Path(applied['movie']['source'])
            assert support.sha(original_path) == applied['movie']['sha256']
            error_source = u.FileMediaSource()
            error_source.set_file_path(str(original_path))
            assert error_source.validate()
        error_defaults.set_editor_property('MissionClearSource', error_source)
    state['callback'] = u.register_slate_post_tick_callback(tick)
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    event('armed', run=run, case=case, PIE=True, diagnostic_damage_and_teleports=True)


if __name__ == '__main__':
    if '--verify-report' in sys.argv:
        offset = sys.argv.index('--verify-report'); assert len(sys.argv[offset + 1:]) == 4
        raise SystemExit(verify_report(*sys.argv[offset + 1:]))
    elif len(sys.argv) == 3 and sys.argv[1] == '--commands':
        run = sys.argv[2]; assert re.fullmatch(r'[A-Za-z0-9_]+', run)
        for case in ('chain', 'death', 'media_error', 'decode_error'):
            directory = ROOT / ('PIE_' + run + '_' + case)
            assert not directory.exists()
            print(subprocess.list2cmdline([
                'E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe', str(PROJECT / 'DreamCatcher.uproject'),
                '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_' + ('1' if case == 'chain' else '2'),
                '-unattended', '-SCCProvider=None', '-RenderOffscreen', '-NoSplash', '-NoAutoSave', '-NoSaveConfig',
                '-DCProgressionProbe=' + run + '_' + case, '-DCProgressionCase=' + case,
                '-ExecutePythonScript=' + str(Path(__file__).resolve()),
                '-ExecCmds=t.MaxFPS 60,Automation RunTests Project.Maps.PIE', '-TestExit=Automation Test Queue Empty',
                '-ini:Engine:[/Script/Engine.AutomationTestSettings]:PIETestDuration=' + ('180' if case == 'chain' else '60'),
                '-ReportExportPath=' + str(directory / 'Automation'), '-GameUserSettingsINI=' + str(directory / 'GameUserSettings.ini'),
                '-abslog=' + str(directory / 'Editor.log')]))
    else:
        arm()
