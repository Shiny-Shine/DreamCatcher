"""Bounded real player/bot Rifle shots after the approved BaseDamage asset setup.

Load BEFORE Project.Maps.PIE in an isolated unattended editor, with source control
disabled and a Saved/Diagnostics GameUserSettingsINI. Flags:
  -DCBaseDamageProbe=<unique_run> -DCBaseDamageValues=defaults|equal8|zero_enemy
The two non-default cases temporarily change only the loaded Experience CDO,
restore it on completion, and never save it. Player fire uses Enhanced Input;
bot fire uses the original BTS_Shoot InputTag.Weapon.Fire gameplay event.
The defaults case then uses the existing fail/restart API and checks the fresh
player/bot attributes after re-entering Level_1. Other cases finish after shots.

After the editor exits, finalize the report (the in-editor probe is NOT final):
  python base_damage_gameplay_probe.py --verify-report REPORT LOG INDEX EXIT_CODE
This checks PIE Automation success, process exit and Error/ensure/Fatal together.
No manufactured HitResult, alternate damage GE or SetByCaller test damage is used.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
import time
import traceback


def verify_report(report_path, log_path, index_path, exit_code):
    report_path = Path(report_path).resolve()
    root = Path(__file__).resolve().parents[2] / 'Saved/Diagnostics/BaseDamage20261018'
    assert report_path.is_relative_to(root.resolve()), 'Unexpected report destination'
    report = json.loads(report_path.read_text(encoding='utf-8'))
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    errors = [line for line in log.splitlines() if re.search(
        r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    pie_tests = [row for row in index.get('tests', [])
                 if row.get('fullTestPath') == 'Project.Maps.PIE']
    automation_ok = (int(index.get('failed', -1)) == 0 and int(index.get('notRun', -1)) == 0
                     and int(index.get('inProcess', -1)) == 0 and bool(pie_tests)
                     and all(row.get('state') == 'Success' for row in pie_tests))
    matching_run = any('BASE_DAMAGE_PIE ' in line and '"event": "armed"' in line
                       and '"run": "' + str(report.get('run')) + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(
        editor_exit_code=int(exit_code), engine_errors=errors,
        automation_passed=automation_ok, matching_run_log=matching_run, log=str(Path(log_path).resolve()),
        automation_index=str(Path(index_path).resolve()))
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0
                              and automation_ok and matching_run and not errors)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], engine_errors=len(errors),
                         automation_passed=automation_ok)))
    return 0 if report['complete'] else 1


if '--verify-report' in sys.argv:
    pos = sys.argv.index('--verify-report')
    assert len(sys.argv[pos + 1:]) == 4
    raise SystemExit(verify_report(*sys.argv[pos + 1:]))

import unreal as u

PROJECT = Path(u.Paths.project_dir()).resolve()
OUTPUT = PROJECT / 'Saved/Diagnostics/BaseDamage20261018'
COMMAND = u.SystemLibrary.get_command_line()
RUN_MATCH = re.search(r'-DCBaseDamageProbe=([A-Za-z0-9_]+)', COMMAND)
MODE_MATCH = re.search(r'-DCBaseDamageValues=(defaults|equal8|zero_enemy)(?:\s|$)', COMMAND)
assert RUN_MATCH and MODE_MATCH, 'Explicit unique run and case flags required'
assert re.search(r'(?:^|\s)-unattended(?:\s|$)', COMMAND, re.I)
assert not u.SourceControl.is_enabled(), 'Run with -SCCProvider=None'
settings_arg = re.search(r'-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
assert settings_arg, 'Use an isolated settings INI'
settings_path = Path(settings_arg.group(1) or settings_arg.group(2)).resolve()
assert settings_path.is_relative_to((PROJECT / 'Saved/Diagnostics').resolve()), 'Settings INI must be inside Saved/Diagnostics'
assert not u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world(), 'Arm before PIE'
RUN, MODE = RUN_MATCH.group(1), MODE_MATCH.group(1)
OUTPUT.mkdir(parents=True, exist_ok=True)
OUT = OUTPUT / ('pie_' + RUN + '.json')
assert not OUT.exists(), 'Do not overwrite a previous run'
MAP = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
EXPERIENCE = '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience'
EXPECTED = {'defaults': (0.0, 12.0), 'equal8': (8.0, 8.0), 'zero_enemy': (8.0, 0.0)}[MODE]
state = dict(step='ready', wall=time.monotonic(), done=False, busy=False, index=0, inputs={})
report = dict(complete=False, probe_passed=False, run=RUN, mode=MODE, shots=[], events=[], failures=[],
              asset_saves=0, user_visual_audio_verified=False, runtime_reentry_verified=False,
              network_replication_verified=False,
              scope='Fresh PIE initialization and actual shots; defaults also checks attributes after same-Level_1 restart. Native tests cover duplicate callbacks.')


def snapshot():
    return {str(p): [p.stat().st_size, p.stat().st_mtime_ns]
            for folder in ['Content', 'Source', 'Config', 'Plugins/GameFeatures/DCStageUI/Content', 'Saved/SaveGames']
            for p in (PROJECT / folder).rglob('*') if p.is_file()}


protected = snapshot()
protected_hashes = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in [PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
                                 PROJECT / '.git/HEAD', PROJECT / '.git/index'] if path.is_file()}


def cls(name):
    return u.load_class(None, '/Script/DreamCatcher.' + name)


def tag(name):
    value = u.GameplayTag()
    assert value.import_text('(TagName="' + name + '")')
    return value


def event(event_name, **values):
    row = dict(event=event_name, **values)
    report['events'].append(row)
    print('BASE_DAMAGE_PIE ' + json.dumps(row, ensure_ascii=False))


def health(actor):
    return actor.get_component_by_class(cls('DCLyraHealthComponent'))


def attribute(controller):
    player_state = controller.get_editor_property('PlayerState')
    combat = [v for v in u.ObjectIterator(cls('DCCombatSet')) if v.get_outer() == player_state]
    assert len(combat) == 1, 'Expected the existing PlayerState CombatSet'
    text = combat[0].get_editor_property('BaseDamage').export_text()
    base = re.search(r'BaseValue=([^,)]+)', text)
    current = re.search(r'CurrentValue=([^,)]+)', text)
    assert base and current, text
    asc = player_state.get_dc_ability_system_component()
    tags = u.GameplayTagLibrary.get_owned_gameplay_tags(asc)
    return dict(base=float(base.group(1)), current=float(current.group(1)),
                ignore_bonus='Gameplay.Damage.IgnoreWeaponBonus' in tags.export_text(),
                player_state=player_state.get_path_name(), asc=asc.get_path_name())


def inject(action, on):
    state['inputs'][action] = u.Vector(1 if on else 0, 0, 0)
    state['input'].inject_input_vector_for_action(action, state['inputs'][action], [], [])


def item(controller, pawn):
    equip = pawn.get_component_by_class(cls('DCLyraEquipmentManagerComponent'))
    weapon = equip.get_first_instance_of_type(cls('DCLyraRangedWeaponInstance')) if equip else None
    return weapon.get_instigator() if weapon else None


def ammo():
    return state['item'].get_stat_tag_stack_count(ammo_tag)


math = u.get_default_object(u.load_class(None, '/Script/Engine.KismetMathLibrary'))
gas = u.get_default_object(u.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
subs = u.get_default_object(u.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
input_data = u.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')
fire = input_data.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'), True)
aim = input_data.find_native_input_action_for_tag(tag('InputTag.Aim'), True)
ammo_tag = tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
bot_fire_tag = tag('InputTag.Weapon.Fire')
startup_immunity = u.GameplayTagContainer()
assert startup_immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
cases = [('player_body', 'player', 'spine_02', EXPECTED[0] + 12),
         ('player_head', 'player', 'head', (EXPECTED[0] + 12) * 1.5),
         ('enemy_body', 'enemy', 'spine_02', EXPECTED[1]),
         ('enemy_head', 'enemy', 'head', EXPECTED[1] * 1.5)]


def begin(now):
    name, role, bone, expected = cases[state['index']]
    if role == 'player':
        shooter, target, controller = state['pawn'], state['bot_pawn'], state['pc']
    else:
        shooter, target, controller = state['bot_pawn'], state['pawn'], state['bot']
    state.update(shooter=shooter, target=target, controller=controller,
                 item=item(controller, shooter), step='settle', at=now)
    assert state['item'], 'Missing equipped weapon inventory instance'
    mesh = target.get_component_by_class(u.SkeletalMeshComponent)
    physics = mesh.get_editor_property('PhysicsAssetOverride') or mesh.get_skinned_asset().get_editor_property('PhysicsAsset')
    bodies = [b for b in u.ObjectIterator(u.load_class(None, '/Script/Engine.SkeletalBodySetup'))
              if b.get_outer() == physics and str(b.get_editor_property('BoneName')) == bone]
    assert len(bodies) == 1, (name, bone)
    capsules = list(bodies[0].get_editor_property('AggGeom').get_editor_property('SphylElems'))
    assert capsules, (name, bone)
    state['body_center'] = capsules[0].get_editor_property('Center')
    inject(aim, role == 'player')
    event('case', name=name, bone=bone, expected=expected,
          activation='EnhancedInput' if role == 'player' else 'BTS_Shoot gameplay event InputTag.Weapon.Fire')


def aimpoint():
    mesh = state['target'].get_component_by_class(u.SkeletalMeshComponent)
    transform = mesh.get_socket_transform(cases[state['index']][2], u.RelativeTransformSpace.RTS_WORLD)
    return math.call_method('TransformLocation', args=(transform, state['body_center']))


def request_reentry(now):
    """Schedule native restart, then discard every reference into the outgoing world."""
    inject(fire, False)
    inject(aim, False)
    mode = u.GameplayStatics.get_game_mode(state['world'])
    director = mode.get_stage_director()
    assert director, 'Current stage director missing'
    director.fail_stage()
    assert mode.has_run_ended(), 'Stage must finish before requesting restart'
    assert mode.request_restart_stage(), 'Native restart request rejected'
    event('reentry_requested', previous_world_time=now, expected_player=0.0, expected_enemy=12.0)
    # Only Python scalars, strings, the callback handle and time limits survive.
    # The caller returns immediately, releasing its local world/actor references too.
    retained = dict(step='reentry', travel_at=now, wall=state['wall'], callback=state['callback'],
                    done=False, busy=True, restarted=False)
    state.clear()
    state.update(retained)


def check_reentry(world, now):
    mode = u.GameplayStatics.get_game_mode(world)
    # The old world cannot pass: its clock only increases and its run is ended.
    if not mode:
        return
    if not state['restarted']:
        if now >= state['travel_at'] or mode.has_run_ended():
            return
        state['restarted'] = True
    assert not mode.has_run_ended(), 'New run ended before initialization could be checked'
    pc = u.GameplayStatics.get_player_controller(world, 0)
    pawn = pc.get_controlled_pawn() if pc else None
    if not pawn or not health(pawn) or not item(pc, pawn):
        return
    bots = [bot for bot in u.GameplayStatics.get_all_actors_of_class(world, cls('DCLyraPlayerBotController'))
            if bot.get_controlled_pawn() and health(bot.get_controlled_pawn())
            and item(bot, bot.get_controlled_pawn())]
    if not bots:
        return
    values = {'player': attribute(pc), 'enemy': attribute(bots[0])}
    for role, expected in [('player', 0.0), ('enemy', 12.0)]:
        value = values[role]
        assert value['base'] == expected and value['current'] == expected, (role, value)
        assert value['ignore_bonus'] == (role == 'enemy'), (role, value)
    report['reentry_attributes'] = values
    report['runtime_reentry_verified'] = True
    event('reentry_verified', world_time=now, player_base=0.0, enemy_base=12.0)
    finish(True)


def finish(passed):
    if state['done']:
        return
    state['done'] = True
    if 'input' in state:
        inject(fire, False)
        inject(aim, False)
    if state.get('startup_immunity_added'):
        gas.call_method('RemoveLooseGameplayTags', args=(state['startup_pawn'], startup_immunity, False))
        state['startup_immunity_added'] = False
    try:
        for key, value in saved_settings.items():
            experience.set_editor_property(key, value)
        report['experience_cdo_restored'] = all(experience.get_editor_property(k) == v for k, v in saved_settings.items())
    except Exception:
        report['failures'].append(traceback.format_exc())
    after = snapshot()
    report['protected_changes'] = [p for p, value in protected.items() if after.get(p) != value]
    report['new_protected_files'] = sorted(set(after) - set(protected))
    report['protected_hash_changes'] = [p for p, expected in protected_hashes.items()
                                         if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest() != expected]
    report['probe_passed'] = bool(passed and not report['failures'] and report.get('experience_cdo_restored')
                                  and not report['protected_changes'] and not report['new_protected_files']
                                  and not report['protected_hash_changes']
                                  and (MODE != 'defaults' or report['runtime_reentry_verified']))
    # The process has not exited, so engine and Automation results are necessarily pending.
    event('probe_finished', passed=report['probe_passed'], final_verification_pending=True)
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    u.unregister_slate_post_tick_callback(state['callback'])
    state.clear()
    state.update(done=True, busy=False)
    if not report['probe_passed']:
        u.log_error('BaseDamage PIE probe failed: ' + str(report['failures']))


def tick(delta):
    if state['done'] or state['busy']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['wall'] < 100, 'Probe timed out'
        world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            return
        assert world.get_path_name().startswith(MAP.rsplit('/', 1)[0] + '/UEDPIE_')
        now = u.GameplayStatics.get_time_seconds(world)
        if state['step'] == 'reentry':
            check_reentry(world, now)
            return
        if state['step'] == 'ready':
            pc = u.GameplayStatics.get_player_controller(world, 0)
            pawn = pc.get_controlled_pawn() if pc else None
            if not pawn or not health(pawn):
                return
            if not state.get('startup_immunity_added'):
                ps = pc.get_editor_property('PlayerState')
                if not ps:
                    return
                owned = u.GameplayTagLibrary.get_owned_gameplay_tags(ps.get_dc_ability_system_component()).export_text()
                assert 'Gameplay.DamageImmunity' not in owned, 'Preexisting immunity would invalidate actual damage checks'
                assert gas.call_method('AddLooseGameplayTags', args=(pawn, startup_immunity, False))
                state.update(startup_pawn=pawn, startup_immunity_added=True)
            if now < 3:
                return
            bots = [b for b in u.GameplayStatics.get_all_actors_of_class(world, cls('DCLyraPlayerBotController'))
                    if b.get_controlled_pawn() and health(b.get_controlled_pawn())]
            if not bots or not item(pc, pawn):
                return
            bot = next((b for b in bots if item(b, b.get_controlled_pawn())), None)
            if not bot:
                return
            # Stop behavior-tree decisions only; the actual equipped weapon GA still performs its trace/GE.
            for other in bots:
                other.get_editor_property('BrainComponent').stop_logic('bounded BaseDamage probe')
                other.stop_movement()
            bot_pawn = bot.get_controlled_pawn()
            for actor in [pawn, bot_pawn]:
                actor.get_movement_component().stop_movement_immediately()
                actor.get_movement_component().disable_movement()
            origin = pawn.get_actor_location() + u.Vector(0, 0, 6000)
            pawn.set_actor_location(origin, False, True)
            bot_pawn.set_actor_location(origin + u.Vector(650, 0, 0), False, True)
            bot_pawn.set_actor_rotation(u.Rotator(yaw=180), True)
            input_subsystem = subs.call_method('GetLocalPlayerSubSystemFromPlayerController', args=(
                pc, u.load_class(None, '/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
            assert input_subsystem
            state.update(world=world, pc=pc, pawn=pawn, bot=bot, bot_pawn=bot_pawn,
                         input=input_subsystem, camera=u.GameplayStatics.get_player_camera_manager(world, 0))
            assert gas.call_method('RemoveLooseGameplayTags', args=(pawn, startup_immunity, False))
            state['startup_immunity_added'] = False
            report['initialization'] = {'player': attribute(pc), 'enemy': attribute(bot)}
            for role, expected in zip(['player', 'enemy'], EXPECTED):
                value = report['initialization'][role]
                assert value['base'] == expected and value['current'] == expected, (role, value)
                assert value['ignore_bonus'] == (role == 'enemy'), (role, value)
            begin(now)
        point = aimpoint()
        role = cases[state['index']][1]
        if role == 'player':
            start = state['camera'].get_camera_location()
            rotation = math.call_method('FindLookAtRotation', args=(start, point))
            state['pc'].set_control_rotation(rotation)
            direction = (point - start).normal()
            start = start + direction * ((state['pawn'].get_actor_location() - start).dot(direction))
        else:
            start = state['bot_pawn'].get_actor_location() + u.Vector(0, 0, state['bot_pawn'].get_editor_property('BaseEyeHeight'))
            state['bot'].set_focal_point(point)
            state['bot'].set_control_rotation(math.call_method('FindLookAtRotation', args=(start, point)))
        elapsed = now - state['at']
        if state['step'] == 'settle' and elapsed > 3:
            direction = (point - start).normal()
            hit = u.SystemLibrary.line_trace_single(world, start, point + direction * 100,
                u.TraceTypeQuery.cast(2), True, [state['shooter']], u.DrawDebugTrace.NONE, True,
                u.LinearColor(), u.LinearColor(), 0)
            trace = hit.export_text() if isinstance(hit, u.HitResult) else str(hit)
            assert state['target'].get_name() in trace, trace
            assert ('WeakSpot' in trace) == (cases[state['index']][2] == 'head'), trace
            state.update(step='shoot', at=now, before_health=health(state['target']).get_health(),
                         before_ammo=ammo(), trace=trace)
            if role == 'player':
                inject(fire, True)
            else:
                # Exactly the original BTS_Shoot event entry point, not direct damage injection.
                gas.call_method('SendGameplayEventToActor', args=(state['shooter'], bot_fire_tag, u.GameplayEventData()))
        elif state['step'] == 'shoot' and ammo() < state['before_ammo']:
            inject(fire, False)
            state.update(step='observe', at=now)
        elif state['step'] == 'shoot' and elapsed > 4:
            raise AssertionError('Original weapon activation did not consume ammo')
        elif state['step'] == 'observe' and elapsed > 0.1:
            damage = state['before_health'] - health(state['target']).get_health()
            row = dict(case=cases[state['index']][0], damage=damage, expected=cases[state['index']][3],
                       ammo_used=state['before_ammo'] - ammo(), weapon_channel_trace=state['trace'])
            report['shots'].append(row)
            event('shot', **row)
            assert abs(damage - row['expected']) < 0.01 and row['ammo_used'] == 1, row
            state.update(step='cooldown', at=now)
        elif state['step'] == 'cooldown' and elapsed > 1:
            state['index'] += 1
            if state['index'] == len(cases):
                report['final_attributes'] = {'player': attribute(state['pc']), 'enemy': attribute(state['bot'])}
                assert report['final_attributes'] == report['initialization'], 'Shots altered initial attributes/source tags'
                if MODE == 'defaults':
                    request_reentry(now)
                else:
                    finish(True)
                return
            begin(now)
        for action, value in state['inputs'].items():
            state['input'].inject_input_vector_for_action(action, value, [], [])
    except Exception:
        report['failures'].append(traceback.format_exc())
        finish(False)
    finally:
        state['busy'] = False


# No saves: in-memory overrides exist only for this process and are restored even on probe failure.
experience_bp = u.load_asset(EXPERIENCE)
experience = u.get_default_object(experience_bp.generated_class())
saved_settings = {name: experience.get_editor_property(name) for name in [
    'bUseRoleBasedBaseDamage', 'PlayerBaseDamage', 'EnemyBaseDamage']}
assert saved_settings == dict(bUseRoleBasedBaseDamage=True, PlayerBaseDamage=0.0, EnemyBaseDamage=12.0), saved_settings
state['callback'] = u.register_slate_post_tick_callback(tick)
try:
    experience.set_editor_property('PlayerBaseDamage', EXPECTED[0])
    experience.set_editor_property('EnemyBaseDamage', EXPECTED[1])
    event('armed', run=RUN, mode=MODE, expected_player=EXPECTED[0], expected_enemy=EXPECTED[1], PIE=True)
except Exception:
    report['failures'].append(traceback.format_exc())
    finish(False)
