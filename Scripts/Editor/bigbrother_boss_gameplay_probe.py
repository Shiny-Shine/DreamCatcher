"""Rendered, bounded BigBrother PIE; no asset saves, builds or physical-input claim.

-DCBigBrotherProbe=<fresh> -DCBigBrotherCase=combat|death_retry
Uses real weapon input for body/head shots, diagnostic immunity/teleports and
GAS damage for phase/death progression. Mission-clear video ends naturally.
The existing frontend PV is dismissed through its normal widget API.
--finalize REPORT LOG AUTOMATION_INDEX EXIT_CODE validates process evidence.
"""
import importlib.util
import json
import math
from pathlib import Path
import re
import sys
import time
import traceback
sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT = P / 'Saved/Diagnostics/BigBrotherBoss20261024'


def finalize():
    path, log_path, index_path, code = sys.argv[2:]
    path = Path(path).resolve(); assert path.is_relative_to(ROOT.resolve())
    r = json.loads(path.read_text(encoding='utf-8'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    errors = [line for line in log.splitlines() if re.search(r': Error:|: Fatal:|Ensure condition failed:|Assertion failed:', line)]
    ends = log.count('[MissionClear] Movie ended; requesting configured next screen.')
    tests = [row for row in index['tests'] if row['fullTestPath'] == 'Project.Maps.PIE']
    ok = bool(tests) and all(row['state'] == 'Success' for row in tests) and all(index[k] == 0 for k in ('failed', 'notRun', 'inProcess'))
    screenshots = []
    for name in r['screenshots']:
        q = Path(name); screenshots += [str(v) for v in q.parent.glob(q.stem + '*.png') if v.stat().st_size > 0]
    r['final_verification'] = dict(editor_exit_code=int(code), automation_passed=ok, engine_errors=errors,
        engine_warnings=[line for line in log.splitlines() if ': Warning:' in line], natural_mission_clear_ends=ends, screenshot_files=screenshots)
    r['complete'] = bool(r['probe_passed'] and int(code) == 0 and ok and not errors
                          and ends == (1 if r['case'] == 'combat' else 0) and len(screenshots) >= len(r['screenshots']))
    path.write_text(json.dumps(r, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=r['complete'], engine_errors=len(errors), natural_ends=ends)))
    return 0 if r['complete'] else 1


def arm():
    import unreal as u
    spec = importlib.util.spec_from_file_location('dc_bigbrother_setup_support', P / 'Scripts/Editor/bigbrother_boss_setup.py')
    setup = importlib.util.module_from_spec(spec); spec.loader.exec_module(setup)
    support = setup.support_module(); support.helpers().hud.assert_no_other_editor()
    command = u.SystemLibrary.get_command_line()
    run = re.search(r'-DCBigBrotherProbe=([A-Za-z0-9_]+)\b', command).group(1)
    case = re.search(r'-DCBigBrotherCase=(combat|death_retry)\b', command).group(1)
    assert '-unattended' in command.lower() and '-nullrhi' not in command.lower() and not u.SourceControl.is_enabled()
    directory = ROOT / ('PIE_' + run); directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'probe.json'; assert not output.exists()
    before, protected = setup.inventory(), support.protected_hashes()
    hashes = {str(setup.disk(p)): setup.sha(setup.disk(p)) for p in list(setup.PATHS.values()) + [setup.MAP]}
    r = dict(complete=False, probe_passed=False, case=case, events=[], failures=[], shots=[], screenshots=[],
             pattern_starts=[], facing_samples=0, max_facing_error_degrees=0, movie_times=[], asset_saves=0,
             diagnostic_damage_immunity_teleports=True, physical_input_audio_visual_acceptance=False)
    s = dict(step='ready', wall=time.monotonic(), busy=False, done=False, inputs={}, seen=set(), fired=set(), previous_patterns={}, locked={}, damage_before={})
    editor = u.get_editor_subsystem(u.UnrealEditorSubsystem)
    assert not editor.get_game_world()
    cls = lambda name: u.load_class(None, '/Script/DreamCatcher.' + name)
    gas = u.get_default_object(u.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
    game = u.get_default_object(u.load_class(None, '/Script/Engine.GameplayStatics'))
    subs = u.get_default_object(u.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
    boss_class = u.load_asset(setup.PATHS['boss']).generated_class()
    boss_data = u.load_asset(setup.PATHS['data'])
    maximum_health = boss_data.get_editor_property('MaxHealth')
    phase_threshold = maximum_health * boss_data.get_editor_property('PhaseTwoHealthRatio')
    r['maximum_health'] = maximum_health
    attack_damage = {key:u.get_default_object(u.load_asset(setup.PATHS[key]).generated_class()).get_editor_property('DamageAmount') for key in ('line','ground')}
    damage_class = u.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
    data = u.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')
    def tag(name):
        t = u.GameplayTag(); assert t.import_text('(TagName="' + name + '")'); return t
    fire = data.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'), True)
    aim = data.find_native_input_action_for_tag(tag('InputTag.Aim'), True)
    ammo_tag = tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
    immunity = u.GameplayTagContainer(); assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
    def health(actor): return actor.get_component_by_class(cls('DCLyraHealthComponent')) if actor else None
    def asc(actor): return gas.call_method('GetAbilitySystemComponent', args=(actor,))
    def all_actors(world, kind): return list(u.GameplayStatics.get_all_actors_of_class(world, kind))
    def event(name, **fields):
        row = dict(event=name, elapsed=time.monotonic() - s['wall'], **fields); r['events'].append(row)
        print('DC_BIGBROTHER_PIE ' + json.dumps(row, ensure_ascii=False))
    def inject(action, on):
        s['inputs'][action] = u.Vector(1 if on else 0, 0, 0)
        s['input'].inject_input_vector_for_action(action, s['inputs'][action], [], [])
    def damage(target, source, amount):
        source_asc = asc(source)
        effect = source_asc.make_outgoing_spec(damage_class, 1, source_asc.make_effect_context())
        effect = gas.call_method('AssignTagSetByCallerMagnitude', args=(effect, tag('SetByCaller.Damage'), float(amount)))
        source_asc.apply_gameplay_effect_spec_to_target(effect, asc(target))
    def active_widgets(pc, native):
        kind = cls(native)
        return [obj for obj in u.ObjectIterator(u.CommonActivatableWidget) if obj.get_owning_player() == pc
                and u.MathLibrary.class_is_child_of(obj.get_class(), kind) and obj.is_activated()]
    def shot(world, name):
        filename = str(directory / (name + '.png'))
        u.SystemLibrary.execute_console_command(world, 'Shot showui filename="' + filename.replace('\\', '/') + '" nosuffix')
        r['screenshots'].append(filename)
    def finish(ok):
        if s['done']: return
        s['done'] = True
        if 'input' in s:
            inject(fire, False); inject(aim, False)
        r['asset_hashes_unchanged'] = all(setup.sha(Path(p)) == h for p, h in hashes.items())
        r['protected_inventory_unchanged'] = setup.inventory() == before
        r['protected_hashes_unchanged'] = support.protected_hashes() == protected
        r['probe_passed'] = bool(ok and not r['failures'] and r['asset_hashes_unchanged'] and r['protected_inventory_unchanged'] and r['protected_hashes_unchanged'])
        output.write_text(json.dumps(r, ensure_ascii=False, indent=2), encoding='utf-8')
        u.unregister_slate_post_tick_callback(s['handle'])
        if not r['probe_passed']: u.log_error('BigBrother PIE probe failed: ' + str(output))
        print('DC_BIGBROTHER_PROBE_RESULT ' + json.dumps(dict(passed=r['probe_passed'], report=str(output))))
    def parts(hit): return game.call_method('BreakHitResult', args=(hit,)) if hit else (False,)
    def visible_trace(world, start, end, ignore):
        return parts(u.SystemLibrary.line_trace_single(world, start, end, u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, ignore, u.DrawDebugTrace.NONE, True))
    def find_ground_positions(world, pawn, boss):
        points = []; origin = boss.get_actor_location(); fire_origin = boss.get_ranged_attack_origin()
        for angle in (-105, -90, -65, -135, -40, 0, 45, 90, 180):
            for radius in (6500, 7500, 5500, 8500):
                radians = math.radians(angle)
                xy = origin + u.Vector(math.cos(radians) * radius, math.sin(radians) * radius, 0)
                hit = visible_trace(world, u.Vector(xy.x, xy.y, 4000), u.Vector(xy.x, xy.y, -1000), [pawn, boss])
                if not hit[0] or hit[7].z < .7 or not -300 < hit[5].z < 1000: continue
                point = hit[5] + u.Vector(0, 0, 100)
                if visible_trace(world, fire_origin, point + u.Vector(0, 0, 50), [pawn, boss])[0]: continue
                capsule_hit = u.SystemLibrary.capsule_trace_single(world, point, point + u.Vector(0, 0, .1), 42, 96,
                    u.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [pawn, boss], u.DrawDebugTrace.NONE, True)
                if parts(capsule_hit)[0]: continue
                points.append(point); break
            if len(points) == 3: return points
        assert len(points) >= 2, 'No two verified floor/visibility positions in this saved arena'
        return points
    def watch_patterns(now, boss, pawn):
        phase = boss.get_boss_phase_component().get_boss_phase()
        for key, kind in (('line', 'DCGameplayAbility_BossLineShot'), ('ground', 'DCGameplayAbility_BossGroundStrike')):
            instances = [a for a in u.ObjectIterator(cls(kind)) if a.get_outer() in (boss, asc(boss))]
            if not instances: continue
            assert len(instances) == 1
            ability = instances[0]; status = str(ability.get_attack_phase())
            if 'WINDUP' in status:
                point = ability.get_locked_aim_point() if key == 'line' else ability.get_locked_ground_point()
                if 'WINDUP' not in s['previous_patterns'].get(key, ''):
                    s['seen'].add((phase, key)); s['locked'][key] = point
                    row = dict(phase=phase, pattern=key, point=point.export_text(), player=pawn.get_actor_location().export_text())
                    r['pattern_starts'].append(row); event('pattern_windup', **row)
                    if phase == 1 and (phase,key) not in s['fired']:
                        s['damage_before'][key] = health(pawn).get_health() + health(pawn).get_armor()
                    assert all_actors(s['world'], cls('DCBossTelegraph')), 'No pattern warning actor'
                else: assert point.distance(s['locked'][key]) < .01, 'Warning followed the moving player'
            if 'RECOVERY' in status and 'RECOVERY' not in s['previous_patterns'].get(key, ''):
                if phase == 1 and (phase,key) not in s['fired']:
                    applied = s['damage_before'][key] - health(pawn).get_health() - health(pawn).get_armor()
                    assert abs(applied - attack_damage[key]) < .1, (key, applied, attack_damage[key])
                    event('boss_attack_real_damage', pattern=key, damage=applied)
                s['fired'].add((phase,key)); event('pattern_fired', phase=phase, pattern=key)
            s['previous_patterns'][key] = status
        if now > s.get('teleported_at', -1) + .1:
            direction = pawn.get_actor_location() - boss.get_actor_location()
            expected = math.degrees(math.atan2(direction.y, direction.x))
            error = abs((boss.get_actor_rotation().yaw - expected + 180) % 360 - 180)
            r['max_facing_error_degrees'] = max(r['max_facing_error_degrees'], error)
            assert error < 2.0, 'Boss stopped facing the stage player'
            r['facing_samples'] += 1
        assert boss.get_actor_location().distance(s['boss_origin']) < .01, 'Boss moved from its authored position'
        assert boss.get_ranged_attack_origin().distance(s['attack_origin']) < .01, 'Axis-aligned fire origin moved with yaw'
    def tick(dt):
        if s['done'] or s['busy']: return
        s['busy'] = True
        try:
            assert time.monotonic() - s['wall'] < 220, 'BigBrother PIE timed out at ' + s['step']
            world = editor.get_game_world()
            if not world or 'UEDPIE_' not in world.get_path_name(): return
            pc = u.GameplayStatics.get_player_controller(world, 0)
            if not pc: return
            mode = u.GameplayStatics.get_game_mode(world); now = u.GameplayStatics.get_time_seconds(world)
            level = u.GameplayStatics.get_current_level_name(world, True)
            if level == 'L_DC_FrontEnd':
                assert case == 'combat' and s['step'] in ('movie', 'frontend') and len(r['movie_times']) >= 2
                s['step'] = 'frontend'
                intros = active_widgets(pc, 'DCFrontendIntroScreen')
                if intros and intros[0].is_intro_playing():
                    player = intros[0].get_intro_player()
                    if player and u.MathLibrary.get_total_seconds(player.get_time()) > .1:
                        event('existing_frontend_pv_played_then_api_skip'); intros[0].skip_intro()
                if active_widgets(pc, 'DCFrontendScreen'):
                    assert not pc.get_controlled_pawn(); event('mode_selection_ready'); finish(True)
                return
            assert level == 'Level_3'
            pawn = pc.get_controlled_pawn()
            bosses = all_actors(world, cls('DCBossCharacter'))
            if s['step'] == 'death_wait':
                if pawn or bosses or not active_widgets(pc, 'DCStageResultLayer'): return
                assert mode.has_run_ended() and not mode.was_run_successful() and not active_widgets(pc, 'DCMissionClearWidget')
                event('death_cleanup_and_existing_failure_result')
                assert mode.request_restart_stage(); s['step'] = 'restart_wait'; return
            if s['step'] == 'restart_wait':
                if not pawn or not health(pawn) or mode.has_run_ended() or len(bosses) != 1 or not bosses[0].is_gas_ready(): return
                assert health(bosses[0]).get_health() == maximum_health and bosses[0].get_boss_phase_component().get_boss_phase() == 1
                assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
                event('new_run_has_one_fresh_bigbrother'); finish(True); return
            if mode.has_run_ended():
                assert case == 'combat' and mode.was_run_successful()
                movies = active_widgets(pc, 'DCMissionClearWidget')
                if not movies: return
                movie = movies[0]; player = movie.get_mission_clear_player()
                assert not movie.is_showing_failure()
                if player and u.MathLibrary.get_total_seconds(player.get_duration()) > 0:
                    assert movie.get_blackout_opacity() >= .999 and pc.is_move_input_ignored() and pc.is_look_input_ignored()
                    seconds = u.MathLibrary.get_total_seconds(player.get_time())
                    if not r['movie_times'] or seconds - r['movie_times'][-1] > .05: r['movie_times'].append(seconds)
                    assert not mode.is_stage_travel_pending()
                    if seconds > 2 and not s.get('movie_shot'): shot(world, 'mission_clear'); s['movie_shot'] = True
                s['step'] = 'movie'; return
            if not pawn or not health(pawn) or len(bosses) != 1: return
            boss = bosses[0]
            if not boss.is_gas_ready() or not mode.get_stage_director(): return
            assert boss.get_class() == boss_class and not all_actors(world, cls('DCLyraPlayerBotController'))
            if s['step'] == 'ready':
                quickbar = pc.get_component_by_class(cls('DCLyraQuickBarComponent'))
                if not quickbar or not quickbar.get_active_slot_item(): return
                assert health(boss).get_health() == health(boss).get_max_health() == maximum_health
                mesh = boss.get_component_by_class(u.SkeletalMeshComponent)
                assert mesh.get_editor_property('SkeletalMesh') == u.load_asset(setup.MESH) and mesh.get_editor_property('bPauseAnims')
                assert mesh.get_editor_property('AnimClass') is None and mesh.get_editor_property('PhysicsAssetOverride') is None
                assert boss.get_velocity().length() < .01
                s.update(world=world, pc=pc, pawn=pawn, boss=boss, boss_origin=boss.get_actor_location(),
                         attack_origin=boss.get_ranged_attack_origin(), item=quickbar.get_active_slot_item(), at=now)
                s['input'] = subs.call_method('GetLocalPlayerSubSystemFromPlayerController', args=(pc, u.load_class(None, '/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
                event('bigbrother_spawned_once', health=health(boss).get_health(), location=s['boss_origin'].export_text(), origin=s['attack_origin'].export_text())
                if case == 'death_retry':
                    damage(pawn, boss, 1000000); s['step'] = 'death_wait'; return
                assert health(pawn).get_health() + health(pawn).get_armor() > sum(attack_damage.values())
                s['ground_positions'] = find_ground_positions(world, pawn, boss)
                pawn.set_actor_location(s['ground_positions'][0], False, True)
                s.update(step='phase_one', teleported_at=now, next_move=now + .7, move_index=0)
            watch_patterns(now, boss, pawn)
            if s['step'] in ('phase_one', 'phase_two'):
                if s['step'] == 'phase_two' and now > s['next_move']:
                    s['move_index'] = (s['move_index'] + 1) % len(s['ground_positions'])
                    pawn.set_actor_location(s['ground_positions'][s['move_index']], False, True)
                    s.update(next_move=now + .7, teleported_at=now)
                phase = 1 if s['step'] == 'phase_one' else 2
                if not {(phase, 'line'), (phase, 'ground')} <= s['fired']: return
                if phase == 2:
                    assert r['facing_samples'] > 20
                    event('both_phases_patterns_and_facing_passed', samples=r['facing_samples'])
                    damage(boss, pawn, 1000000); s['step'] = 'clear_wait'; return
                # A diagnostic elevated firing position avoids the arena cover.
                assert gas.call_method('AddLooseGameplayTags', args=(pawn, immunity, False))
                pawn.get_movement_component().stop_movement_immediately(); pawn.get_movement_component().disable_movement()
                pawn.set_actor_location(s['boss_origin'] + u.Vector(0, -6000, 5500), False, True)
                s.update(step='shot_settle', teleported_at=now, at=now, shot_index=0)
                s['camera'] = u.GameplayStatics.get_player_camera_manager(world, 0)
                inject(aim, True)
            if s['step'] in ('shot_settle', 'shoot', 'shot_observe', 'shot_cooldown'):
                local_point = u.Vector(0, 0, 4650) if s['shot_index'] == 0 else u.Vector(*setup.HIT_SHAPES['BossHeadHit']['center'])
                point = u.MathLibrary.transform_location(boss.get_actor_transform(), local_point)
                cam = s['camera'].get_camera_location()
                pc.set_control_rotation(u.MathLibrary.find_look_at_rotation(cam, point))
                if s['step'] == 'shot_settle' and now - s['at'] > 2:
                    reticles = [w for w in u.ObjectIterator(u.UserWidget) if w.get_class().get_name() == 'W_Reticle_Rifle_C' and w.get_owning_player() == pc]
                    if not reticles or not reticles[0].has_first_shot_accuracy(): return
                    direction = (point - cam).normal(); start = cam + direction * ((pawn.get_actor_location() - cam).dot(direction))
                    hit = u.SystemLibrary.line_trace_single(world, start, point + direction * 100, u.TraceTypeQuery.cast(2), True, [pawn], u.DrawDebugTrace.NONE, True)
                    h = parts(hit); assert h[0] and h[9] == boss, 'Weapon trace did not reach BigBrother'
                    text = hit.export_text(); assert ('WeakSpot' in text) == (s['shot_index'] == 1), text
                    s.update(step='shoot', at=now, before_ammo=s['item'].get_stat_tag_stack_count(ammo_tag), before_health=health(boss).get_health(), trace=text)
                    inject(fire, True)
                elif s['step'] == 'shoot' and s['item'].get_stat_tag_stack_count(ammo_tag) < s['before_ammo']:
                    inject(fire, False); s.update(step='shot_observe', at=now)
                elif s['step'] == 'shot_observe' and now - s['at'] > .1:
                    row = dict(zone='body' if s['shot_index'] == 0 else 'head', damage=s['before_health'] - health(boss).get_health(),
                               ammo_used=s['before_ammo'] - s['item'].get_stat_tag_stack_count(ammo_tag), trace=s['trace'])
                    assert row['damage'] > 0 and row['ammo_used'] == 1 and health(boss).get_health() > phase_threshold, row
                    r['shots'].append(row); event('real_weapon_shot', **row)
                    if s['shot_index'] == 1:
                        assert r['shots'][1]['damage'] > r['shots'][0]['damage'], 'Head weak-spot did not increase damage'
                        shot(world, 'BigBrother_gameplay')
                    s.update(step='shot_cooldown', at=now)
                elif s['step'] == 'shot_cooldown' and now - s['at'] > 1:
                    if s['shot_index'] == 0: s.update(shot_index=1, step='shot_settle', at=now)
                    else:
                        inject(aim, False)
                        pawn.set_actor_location(s['ground_positions'][0], False, True)
                        pawn.get_movement_component().set_movement_mode(u.MovementMode.MOVE_WALKING)
                        damage(boss, pawn, health(boss).get_health() - (phase_threshold - 1))
                        assert 0 < health(boss).get_health() <= phase_threshold
                        assert boss.get_boss_phase_component().get_boss_phase() == 2
                        event('second_phase_transition_started')
                        s.update(step='phase_two', teleported_at=now, next_move=now + .2)
            for action, value in s['inputs'].items(): s['input'].inject_input_vector_for_action(action, value, [], [])
        except Exception:
            r['failures'].append(traceback.format_exc()); finish(False)
        finally: s['busy'] = False
    s['handle'] = u.register_slate_post_tick_callback(tick)
    u.EditorPythonScripting.set_keep_python_script_alive(True)
    event('armed', case=case)


if __name__ == '__main__':
    if '--finalize' in sys.argv: raise SystemExit(finalize())
    arm()
