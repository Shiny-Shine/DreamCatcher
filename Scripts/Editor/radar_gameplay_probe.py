"""One bounded Project.Maps.PIE radar lifecycle probe, armed before PIE.

Start on L_DC_FrontEnd in a separate unattended rendered/offscreen editor with
-SCCProvider=None -DCRadarProbe=<fresh_run> and an isolated GameUserSettingsINI
inside Saved/Diagnostics/Radar20261019/<fresh_run>/. No map/asset/config saves.
Uses actual HUD data, original Death/retry/menu callbacks and temporary PIE actors.
After editor exit: python radar_gameplay_probe.py --verify-report REPORT LOG INDEX EXIT_CODE
This is automated geometry/data evidence, not physical-input or visual acceptance.
"""
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import time
import traceback


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.is_relative_to((Path(__file__).resolve().parents[2] / 'Saved/Diagnostics/Radar20261019').resolve())
    report = json.loads(path.read_text(encoding='utf-8'))
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    errors = [line for line in log.splitlines() if re.search(r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    passed = (int(index.get('failed', -1)) == 0 and int(index.get('notRun', -1)) == 0
              and int(index.get('inProcess', -1)) == 0 and bool(tests) and all(row.get('state') == 'Success' for row in tests))
    matching = any('RADAR_PIE ' in line and '"event": "armed"' in line and '"run": "' + report['run'] + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=passed,
        engine_errors=errors, matching_run_log=matching, log=str(Path(log_path).resolve()), automation_index=str(Path(index_path).resolve()))
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0 and passed and matching and not errors)
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], automation_passed=passed, engine_errors=len(errors))))
    return 0 if report['complete'] else 1


if '--verify-report' in sys.argv:
    position = sys.argv.index('--verify-report')
    assert len(sys.argv[position + 1:]) == 4
    raise SystemExit(verify_report(*sys.argv[position + 1:]))

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
COMMAND = ue.SystemLibrary.get_command_line()
match = re.search(r'-DCRadarProbe=([A-Za-z0-9_]{1,64})(?=\s|$)', COMMAND)
assert match and '-unattended' in COMMAND.lower() and '-sccprovider=none' in COMMAND.lower()
assert '-nullrhi' not in COMMAND.lower(), 'Use a rendered/offscreen viewport; zero geometry is not a pass'
assert not ue.SourceControl.is_enabled()
RUN = match.group(1)
DIRECTORY = PROJECT / 'Saved/Diagnostics/Radar20261019' / RUN
DIRECTORY.mkdir(parents=True, exist_ok=True)
OUT = DIRECTORY / 'pie.json'; assert not OUT.exists()
assert '-dcradarpaintdiagnostics' in COMMAND.lower(), 'Opt-in actual NativePaint evidence is required'
assert '-forcelogflush' in COMMAND.lower(), 'Fresh-frame paint checks require immediate log flushing'
log_match = re.search(r'-abslog=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
assert log_match
PAINT_LOG = Path(log_match.group(1) or log_match.group(2)).resolve()
assert PAINT_LOG.is_relative_to(DIRECTORY.resolve()), 'Read only this run\'s native paint log'
settings = re.search(r'-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
assert settings and Path(settings.group(1) or settings.group(2)).resolve().is_relative_to(DIRECTORY.resolve())
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Arm before Project.Maps.PIE starts'
# GetEditorWorld is editor-only and is deliberately captured before PIE starts;
# UnrealEditorSubsystem rejects that API while the editor is playing.
EDITOR_WORLD = editor.get_editor_world()
assert EDITOR_WORLD and ue.SystemLibrary.is_valid(EDITOR_WORLD), 'No Editor world available for the foreign-world fixture before PIE'
assert 'UEDPIE_' not in EDITOR_WORLD.get_path_name()
STORY = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
MENU = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot():
    return {str(p): [p.stat().st_size, p.stat().st_mtime_ns] for name in
            ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
            for p in (PROJECT / name).rglob('*') if p.is_file()}


before_files = snapshot()
protected_names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
                   'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
                   'Saved/Config/WindowsEditor/GameUserSettings.ini', 'Saved/Config/Windows/GameUserSettings.ini']
protected = {str(PROJECT / name): sha(PROJECT / name) if (PROJECT / name).is_file() else None for name in protected_names}
state = dict(step='menu_initial', started=time.monotonic(), at=0., busy=False, done=False, temporary=[], mutations=[])
report = dict(run=RUN, complete=False, probe_passed=False, events=[], failures=[], asset_saves=0,
              cpp_build=False, physical_input_verified=False, visual_acceptance=False,
              full_combat_or_packaging_verified=False, offscreen_render='-renderoffscreen' in COMMAND.lower(),
              objective_selection_connected=False,
              diagnostic_lethal_effect='/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller',
              diagnostic_damage_is_weapon_or_balance_verification=False,
              visibility_trace_mapping=dict(index=0, channel='ECC_Visibility', collision_channel_index=3,
                  blocking_response_index=2,
                  evidence='Engine/EngineTypes.h:1103 then CollisionProfile.cpp:373-377 append Visibility before Camera; custom channels append later'))


def native(name):
    cls = ue.load_class(None, '/Script/DreamCatcher.' + name); assert cls, name
    return cls


def bp(path):
    cls = ue.load_class(None, path + '.' + path.rsplit('/', 1)[1] + '_C'); assert cls, path
    return cls


radar_class = bp('/Game/DreamCatcher/UI/HUD/W_DC_Radar')
hud_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout')
frontend_class = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
menu_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu')
dialog_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
boss_class = bp('/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss')
damage_class = bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
health_class = native('DCLyraHealthComponent')
result_class = native('DCStageResultLayer')
ui = ue.get_default_object(ue.load_class(None, '/Script/CommonGame.CommonUIExtensions'))
gas = ue.get_default_object(ue.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
game = ue.get_default_object(ue.load_class(None, '/Script/Engine.GameplayStatics'))
menu_tag = ue.GameplayTag(); assert menu_tag.import_text('(TagName="UI.Layer.Menu")')
immunity = ue.GameplayTagContainer(); assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
damage_tag = ue.GameplayTag(); assert damage_tag.import_text('(TagName="SetByCaller.Damage")')


def event(name, **values):
    row = dict(event=name, run=RUN, elapsed=round(time.monotonic() - state['started'], 3), **values)
    report['events'].append(row); print('RADAR_PIE ' + json.dumps(row, ensure_ascii=False))


def valid(obj):
    return obj is not None and ue.SystemLibrary.is_valid(obj)


def widgets(pc, cls, active=False):
    return [w for w in ue.ObjectIterator(ue.UserWidget) if w.get_class() == cls and w.get_owning_player() == pc
            and (not active or w.is_activated()) and not w.get_name().startswith('Default__')]


def one(pc, cls, active=False):
    values = widgets(pc, cls, active); assert len(values) == 1, (cls.get_name(), len(values))
    return values[0]


def child(owner, name):
    values = []
    for widget in ue.ObjectIterator(ue.Widget):
        if widget.get_name() != name: continue
        outer = widget.get_outer()
        while outer:
            if outer == owner: values.append(widget); break
            outer = outer.get_outer()
    assert len(values) == 1, (owner.get_name(), name, len(values))
    return values[0]


def click(widget):
    if isinstance(widget, ue.CommonButtonBase): widget.call_method('HandleButtonClicked')
    else: widget.get_editor_property('OnClicked').broadcast()


def markers(radar):
    values = []
    for marker in radar.call_method('GetRadarMarkers'):
        label = str(marker.get_editor_property('Kind')).upper()
        kind = next((name for name in ('player', 'enemy', 'objective') if name.upper() in label), None)
        assert kind, label
        center = marker.get_editor_property('Center')
        values.append(dict(kind=kind, center=[center.x, center.y], size=marker.get_editor_property('SizePixels'),
                           clamped=marker.get_editor_property('bClamped'), actor=marker.get_editor_property('ActorPath'),
                           component=marker.get_editor_property('ComponentPath')))
    return values


def marker_for(radar, actor, kind):
    values = [row for row in markers(radar) if row['kind'] == kind and row['actor'] == actor.get_path_name()]
    assert len(values) <= 1
    return values[0] if values else None


def geometry(radar, world):
    g = radar.get_cached_geometry()
    size = ue.SlateLibrary.get_local_size(g)
    assert size.x > 0 and size.y > 0, 'Radar geometry is zero; layout verification is unavailable'
    assert abs(size.x - 240) < .1 and abs(size.y - 240) < .1, (size.x, size.y)
    pixel, viewport = ue.SlateLibrary.local_to_viewport(world, g, ue.Vector2D())
    slot = radar.get_editor_property('slot')
    assert (slot.get_position().x, slot.get_position().y, slot.get_size().x, slot.get_size().y) == (64, 48, 240, 240)
    assert radar.get_parent().get_name() == 'CanvasPanel_0'
    return dict(local_size=[size.x, size.y], pixel_origin=[pixel.x, pixel.y], viewport_origin=[viewport.x, viewport.y])


def geometry_diagnostics(radar, layout, pc):
    """Actual Slate measurements only; never replace a zero with desired/slot size."""
    def measure(widget):
        row = dict(path=widget.get_path_name(), visibility=str(widget.get_visibility()))
        for label, reader in [('tick', widget.get_tick_space_geometry), ('paint', widget.get_paint_space_geometry)]:
            try:
                value = reader()
                size = ue.SlateLibrary.get_local_size(value)
                origin = ue.SlateLibrary.local_to_absolute(value, ue.Vector2D())
                row[label] = dict(local_size=[size.x, size.y], absolute_origin=[origin.x, origin.y])
            except Exception:
                row[label] = dict(read_error=traceback.format_exc())
        desired = widget.get_desired_size()
        row['desired_size'] = [desired.x, desired.y]
        row['render_opacity'] = widget.get_render_opacity()
        if isinstance(widget, ue.CommonActivatableWidget): row['activated'] = widget.is_activated()
        return row
    values = dict(radar=measure(radar), hud=measure(layout), parents=[],
                  radar_active=radar.call_method('IsRadarActive'))
    parent = radar.get_parent()
    for _ in range(6):
        if not parent: break
        values['parents'].append(measure(parent)); parent = parent.get_parent()
    try: values['viewport_size'] = list(pc.get_viewport_size())
    except Exception: values['viewport_read_error'] = traceback.format_exc()
    return values


def native_paint_rows(minimum_frame):
    """Bounded raw paint observations, deduplicated against Automation replay."""
    ue.log_flush()
    if not PAINT_LOG.is_file(): return []
    with PAINT_LOG.open('rb') as handle:
        handle.seek(0, 2)
        handle.seek(max(0, handle.tell() - 65536))
        tail = handle.read(65536).decode('utf-8-sig', errors='replace')
    frame = int(ue.SystemLibrary.get_frame_count())  # KismetSystemLibrary.cpp:367 returns GFrameCounter.
    values = {}
    for line in tail.splitlines():
        if 'DCRADAR_PAINT ' not in line: continue
        try: value = json.loads(line.split('DCRADAR_PAINT ', 1)[1])
        except json.JSONDecodeError: continue  # A bounded tail can start/end within a line.
        painted_frame = value.get('frame')
        if not isinstance(painted_frame, int) or not minimum_frame <= painted_frame <= frame: continue
        value['read_at_frame'] = frame
        values[(value.get('world'), value.get('widget'), painted_frame)] = value
    return sorted(values.values(), key=lambda row: row['frame'])


def recent_native_paint(radar, world, minimum_frame):
    """Only current-instance paint within eight actual engine frames."""
    for value in reversed(native_paint_rows(minimum_frame)):
        if value.get('widget') != radar.get_path_name() or value.get('world') != world.get_path_name(): continue
        if value['read_at_frame'] - value['frame'] > 8: continue
        return value
    return None


def dormant_radars(pc, active_radar=None):
    values = []
    for radar in widgets(pc, radar_class):
        if radar == active_radar: continue
        row = dict(widget=radar.get_path_name(), active=radar.call_method('IsRadarActive'),
                   markers=markers(radar), objective=radar.call_method('GetObjectiveComponent') is not None)
        values.append(row)
        assert not row['active'] and not row['markers'] and not row['objective'], ('Non-screen radar is still active', row)
    return values


def observe_frontend_no_radar(pc, phase):
    assert not widgets(pc, hud_class, True), 'An active gameplay HUD remains in Frontend'
    assert not pc.get_controlled_pawn(), 'Frontend spawned a gameplay Pawn'
    dormant = dormant_radars(pc)
    frame = int(ue.SystemLibrary.get_frame_count())
    observation = state.get('menu_observation')
    if not observation or observation['phase'] != phase or observation['pc'] != pc:
        observation = dict(phase=phase, pc=pc, first_frame=frame, started=time.monotonic())
        state['menu_observation'] = observation
    drawn = native_paint_rows(observation['first_frame'])
    assert not drawn, ('Radar actually painted after Frontend became active', drawn)
    if frame - observation['first_frame'] < 16 or time.monotonic() - observation['started'] < .1: return False
    report.setdefault('frontend_radar_observations', {})[phase] = dict(
        first_frame=observation['first_frame'], last_frame=frame, observed_new_frames=frame - observation['first_frame'],
        active_hud_count=0, controlled_pawn=False, dormant_count=len(dormant), dormant_widgets=dormant, native_draw_count=0)
    state.pop('menu_observation', None)
    return True


def assert_single_screen_paint(radar, world, minimum_frame):
    values = [row for row in native_paint_rows(minimum_frame)
              if row['world'] == world.get_path_name() and row['read_at_frame'] - row['frame'] <= 8]
    paths = {row['widget'] for row in values}
    assert paths <= {radar.get_path_name()}, ('Multiple radar widget instances actually painted', paths)


def validate_native_geometry(value):
    for key in ('local_size', 'parent_canvas_size', 'canvas_position', 'absolute_origin', 'pixel_size'):
        assert len(value[key]) == 2 and all(math.isfinite(n) for n in value[key]), (key, value)
    assert all(abs(n - 240) < .1 for n in value['local_size']), value
    assert all(n > 0 for n in value['parent_canvas_size']), 'Native parent Canvas paint geometry is unavailable'
    assert max(abs(a - b) for a, b in zip(value['canvas_position'], (64, 48))) < .1, value
    assert all(n > 0 for n in value['pixel_size']) and value['draw_count'] > 0
    sx, sy = [pixels / local for pixels, local in zip(value['pixel_size'], value['local_size'])]
    assert abs(sx - sy) < .001, 'Actual radar paint scale is not uniform'


def wait_for_geometry(radar, layout, pc, world, step):
    pending = state.get('geometry_pending')
    frame = int(ue.SystemLibrary.get_frame_count())
    if not pending or pending['widget'] != radar or pending['step'] != step:
        pending = dict(widget=radar, step=step, started=time.monotonic(), first_frame=frame, frames=0)
        state['geometry_pending'] = pending
        report.setdefault('geometry_samples', {})[step] = geometry_diagnostics(radar, layout, pc)
    pending['frames'] += 1
    assert_single_screen_paint(radar, world, pending['first_frame'])
    native_paint = recent_native_paint(radar, world, pending['first_frame'])
    if native_paint is None:
        # The native logger samples every fourth frame. Allow several samples,
        # but never accept the previous stage/instance or an unchanging old row.
        if frame - pending['first_frame'] <= 16 and time.monotonic() - pending['started'] < 2: return None
        report.setdefault('native_paint_failures', {})[step] = geometry_diagnostics(radar, layout, pc)
        ue.SystemLibrary.execute_console_command(world, 'Shot SHOWUI nosuffix filename="' + (DIRECTORY / ('radar_paint_failure_' + step + '.png')).as_posix() + '"', pc)
        raise AssertionError('No current-instance NativePaint log within 16 new frames/2s (8-frame freshness required)')
    validate_native_geometry(native_paint)
    size = ue.SlateLibrary.get_local_size(radar.get_cached_geometry())
    reflected = (geometry(radar, world) if size.x > 0 and size.y > 0 else
                 dict(available=False, local_size=[size.x, size.y], reason='Reflected FGeometry returned zero; root cause unconfirmed'))
    measured = dict(source='actual_NativePaint_AllottedGeometry', native_paint=native_paint, reflected_geometry=reflected)
    report.setdefault('actual_native_geometry', {})[step] = measured
    state['paint_required_frame'] = native_paint['frame']
    state.pop('geometry_pending', None)
    return measured


def native_marker_agreement(radar, world):
    """Require a fresh paint after the current camera/actor change, then compare it."""
    first = state.setdefault('paint_agreement_wait', dict(frame=int(ue.SystemLibrary.get_frame_count()), at=time.monotonic()))
    value = recent_native_paint(radar, world, state['paint_required_frame'])
    equal = False
    if value:
        assert_single_screen_paint(radar, world, state['paint_required_frame'])
        validate_native_geometry(value)
        current = markers(radar)
        kinds = {0: 'player', 1: 'enemy', 2: 'objective'}
        painted = {(kinds[row['kind']], row['actor'], row['component']): row for row in value['markers']}
        equal = len(current) == len(painted)
        for row in current:
            actual = painted.get((row['kind'], row['actor'], row['component']))
            equal = bool(equal and actual and max(abs(a - b) for a, b in zip(row['center'], actual['center'])) < .1
                         and abs(row['size'] - actual['size']) < .001 and row['clamped'] == actual['clamped'])
        yaw = ue.GameplayStatics.get_player_camera_manager(world, 0).get_camera_rotation().yaw
        equal = equal and abs((value['camera_yaw'] - yaw + 180) % 360 - 180) < .25
    if equal:
        state.pop('paint_agreement_wait', None)
        return value
    frame = int(ue.SystemLibrary.get_frame_count())
    if frame - first['frame'] <= 16 and time.monotonic() - first['at'] < 2: return None
    report['native_marker_disagreement'] = dict(native_paint=value, current_markers=markers(radar), current_frame=frame)
    raise AssertionError('GetRadarMarkers and fresh NativePaint did not converge within 16 new frames/2s')


def assert_pie(world):
    assert world == editor.get_game_world() and 'UEDPIE_' in world.get_path_name()
    assert ue.GameplayStatics.get_current_level_name(world, True) == 'Level_1', world.get_path_name()


def spawn(world, cls, location, scale=None):
    # Never use EditorLevelLibrary/EditorActorSubsystem: both operations target
    # the explicitly verified current PIE world through GameplayStatics.
    assert_pie(world)
    transform = ue.Transform(location=location, scale=scale or ue.Vector(1, 1, 1))
    actor = game.call_method('BeginDeferredActorSpawnFromClass', args=(world, cls, transform,
        ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor and actor.get_world() == world
    state['temporary'].append(actor)
    mesh = actor.get_component_by_class(ue.StaticMeshComponent)
    if mesh: mesh.set_mobility(ue.ComponentMobility.MOVABLE)
    actor = game.call_method('FinishSpawningActor', args=(actor, transform, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor.get_world() == world
    return actor


def stop_actor(actor):
    if not valid(actor): return
    controller = actor.get_controller() if isinstance(actor, ue.Pawn) else None
    if valid(controller) and isinstance(controller, ue.AIController):
        brain = controller.get_editor_property('BrainComponent')
        if brain: brain.stop_logic('bounded radar diagnostic')
        controller.stop_movement()
    movement = actor.get_movement_component() if isinstance(actor, ue.Pawn) else None
    state['mutations'].append((actor, actor.get_actor_location(), actor.get_actor_rotation(),
                               movement.get_editor_property('MovementMode') if movement else None))
    if movement: movement.stop_movement_immediately(); movement.disable_movement()


def cleanup_runtime():
    for actor in state['temporary']:
        if valid(actor): actor.destroy_actor()
    state['temporary'].clear()
    for actor, location, rotation, mode in state['mutations']:
        if valid(actor):
            actor.set_actor_location(location, False, True); actor.set_actor_rotation(rotation, True)
            if mode is not None: actor.get_movement_component().set_movement_mode(mode, 0)
            controller = actor.get_controller()
            if valid(controller) and isinstance(controller, ue.AIController):
                brain = controller.get_editor_property('BrainComponent')
                if brain: brain.restart_logic()
    state['mutations'].clear()


def finish(ok):
    if state['done']: return
    state['done'] = True
    try:
        cleanup_runtime()
        if valid(state.get('immune_pawn')):
            gas.call_method('RemoveLooseGameplayTags', args=(state['immune_pawn'], immunity, False))
            state.pop('immune_pawn', None)
        if valid(state.get('control_pc')):
            state['control_pc'].set_control_rotation(state['original_control_rotation'])
    except Exception: report['failures'].append(traceback.format_exc())
    after = snapshot()
    report['protected_changes'] = [p for p, value in before_files.items() if after.get(p) != value]
    report['new_protected_files'] = sorted(set(after) - set(before_files))
    report['protected_hash_changes'] = [p for p, value in protected.items()
        if (sha(Path(p)) if Path(p).is_file() else None) != value]
    screenshots = list(DIRECTORY.glob('radar*.png'))
    report['screenshots'] = []
    for path in screenshots:
        with path.open('rb') as handle: signature = handle.read(8)
        if path.stat().st_size > 32 and signature == b'\x89PNG\r\n\x1a\n': report['screenshots'].append(str(path))
    report['invalid_screenshots'] = [str(p) for p in screenshots if str(p) not in report['screenshots']]
    report['screenshot_captured'] = bool(report['screenshots'])
    if ok and not report['screenshot_captured']: report['failures'].append('Requested SHOWUI screenshot produced no valid PNG')
    report['probe_passed'] = bool(ok and not report['failures'] and not report['protected_changes']
                                  and not report['new_protected_files'] and not report['protected_hash_changes'])
    report['final_verification_pending'] = True
    event('finished', probe_passed=report['probe_passed'], final_verification_pending=True)
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not report['probe_passed']: ue.log_error('Radar PIE verification failed: ' + str(OUT))


def tick(delta):
    if state['done'] or state['busy']: return
    state['busy'] = True
    try:
        assert time.monotonic() - state['started'] < 200, 'Radar probe timeout: ' + state['step']
        world = editor.get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name(): return
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc: return
        level = ue.GameplayStatics.get_current_level_name(world, True)
        now = ue.GameplayStatics.get_time_seconds(world)
        step = state['step']
        if step in ('menu_initial', 'menu_return', 'menu_final'):
            if level != 'L_DC_FrontEnd' or not widgets(pc, frontend_class, True) or now < 2: return
            if not observe_frontend_no_radar(pc, step): return
            if step == 'menu_final': finish(True); return
            click(child(one(pc, frontend_class, True), 'StoryButton'))
            event(step, radar_count=0)
            state.update(step='initial' if step == 'menu_initial' else 'reentry'); return
        if level != 'Level_1': return
        assert_pie(world)
        pawn = pc.get_controlled_pawn()
        mode = ue.GameplayStatics.get_game_mode(world)
        if step == 'death_wait':
            if valid(state.get('death_health')):
                death = str(state['death_health'].get_death_state())
                if 'NOT_DEAD' not in death.upper(): state['death_started_seen'] = True
            if valid(state.get('death_radar')): assert not markers(state['death_radar']), 'Dead player still has radar markers'
            if pawn or not widgets(pc, result_class, True): return
            assert state.get('death_started_seen'), 'Actual original DeathStarted not observed'
            state.pop('death_health', None); state.pop('death_radar', None)
            click(child(one(pc, result_class, True), 'RetryButton'))
            assert mode.is_stage_travel_pending()
            event('original_death_and_retry', radar_hidden=True)
            state.update(step='restart', previous_world=world.get_path_name(), travel_at=now); return
        # Same-map restart can reuse the UEDPIE package name. Observe the actual
        # world clock reset rather than comparing that name as object identity.
        if step == 'restart' and now >= state['travel_at'] - .5: return
        if not widgets(pc, hud_class, True) or now < 2: return
        layout = one(pc, hud_class, True)
        radar = child(layout, 'Radar')
        assert radar.get_class() == radar_class and radar.get_owning_player() == pc
        dormant = dormant_radars(pc, radar)
        if step not in ('initial', 'restart', 'reentry') and (not pawn or not radar.call_method('IsRadarActive')): return
        if step in ('initial', 'restart', 'reentry'):
            if step == 'initial' and valid(pawn):
                health = pawn.get_component_by_class(health_class)
                if health and health.get_health() > 0 and health.get_max_health() > 0 and state.get('immune_pawn') != pawn:
                    if valid(state.get('immune_pawn')):
                        gas.call_method('RemoveLooseGameplayTags', args=(state['immune_pawn'], immunity, False))
                    gas.call_method('AddLooseGameplayTags', args=(pawn, immunity, False))
                    state['immune_pawn'] = pawn
                    event('diagnostic_immunity_owned_before_geometry', pawn=pawn.get_path_name())
            measured = wait_for_geometry(radar, layout, pc, world, step)
            if measured is None: return
            assert pawn and radar.call_method('IsRadarActive'), 'Player/view became inactive while waiting for actual Slate geometry'
            assert radar.call_method('GetObjectiveComponent') is None
            event(step, radar_count=1, geometry=measured, world=world.get_path_name())
            report.setdefault('stage_radar_observations', {})[step] = dict(active_hud_count=1,
                active_radar=radar.get_path_name(), dormant_count=len(dormant), dormant_widgets=dormant)
            if step == 'restart':
                player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(player, menu_tag, menu_class))
                state.update(step='esc_menu', at=now); return
            if step == 'reentry':
                player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(player, menu_tag, menu_class))
                state.update(step='esc_final', at=now); return
            rows = markers(radar)
            enemies = [actor for actor in ue.GameplayStatics.get_all_actors_of_class(world, ue.Pawn)
                       if actor != pawn and any(row['kind'] == 'enemy' and row['actor'] == actor.get_path_name() for row in rows)]
            if not enemies: return
            target = enemies[0]
            for actor in ue.GameplayStatics.get_all_actors_of_class(world, ue.Pawn): stop_actor(actor)
            asc = gas.call_method('GetAbilitySystemComponent', args=(pawn,))
            assert state.get('immune_pawn') == pawn, 'Geometry warmup must own exactly one diagnostic immunity contribution'
            state['control_pc'], state['original_control_rotation'] = pc, pc.get_control_rotation()
            pawn.set_actor_location(ue.Vector(0, 0, 2000), False, True)
            goal = spawn(world, ue.StaticMeshActor, ue.Vector())
            component = goal.get_component_by_class(ue.SceneComponent)
            assert component
            component.set_mobility(ue.ComponentMobility.MOVABLE)
            assert component.get_editor_property('Mobility') == ue.ComponentMobility.MOVABLE
            radar.call_method('SetObjectiveComponent', args=(component,))
            assert radar.call_method('GetObjectiveComponent') == component
            objective = marker_for(radar, goal, 'objective')
            assert objective and max(abs(value - 120) for value in objective['center']) < .01, 'World-origin/zero-XY objective invalid'
            radar.call_method('ClearObjective')
            target.set_actor_location(ue.Vector(2500, 0, 2000), False, True)
            pc.set_control_rotation(ue.Rotator(yaw=0))
            state['paint_required_frame'] = int(ue.SystemLibrary.get_frame_count()) + 1
            state.update(step='yaw', at=now, index=0, target=target, goal=goal, component=component)
            return
        if step == 'yaw':
            if now - state['at'] < .4: return
            camera = ue.GameplayStatics.get_player_camera_manager(world, 0)
            yaw = camera.get_camera_rotation().yaw
            requested_yaw = [0, 45, 90, 180][state['index']]
            assert abs((yaw - requested_yaw + 180) % 360 - 180) < 2, ('camera yaw not reached', yaw, requested_yaw)
            marker = marker_for(radar, state['target'], 'enemy'); assert marker
            expected = [120 - 49.5147186258 * math.sin(math.radians(yaw)), 120 - 49.5147186258 * math.cos(math.radians(yaw))]
            assert max(abs(a - b) for a, b in zip(marker['center'], expected)) < .1, (marker, expected, yaw)
            painted = native_marker_agreement(radar, world)
            if painted is None: return
            event('actual_camera_projection', yaw=yaw, marker=marker, native_paint=painted)
            if state['index'] == 0:
                ue.SystemLibrary.execute_console_command(world, 'Shot SHOWUI nosuffix filename="' + (DIRECTORY / 'radar.png').as_posix() + '"', pc)
            state['index'] += 1
            if state['index'] < 4:
                pc.set_control_rotation(ue.Rotator(yaw=[0, 45, 90, 180][state['index']]))
                state['paint_required_frame'] = int(ue.SystemLibrary.get_frame_count()) + 1
                state['at'] = now; return
            state['target'].set_actor_location(ue.Vector(5000, 0, 2000), False, True)
            state.update(step='boundary', at=now); return
        if step == 'boundary':
            if now - state['at'] < .2: return
            assert marker_for(radar, state['target'], 'enemy'), '50m boundary should be included'
            state['target'].set_actor_location(ue.Vector(5001, 0, 2000), False, True)
            assert marker_for(radar, state['target'], 'enemy') is None, 'Beyond 50m should be hidden'
            state['target'].set_actor_location(ue.Vector(4999, 0, 2000), False, True)
            assert marker_for(radar, state['target'], 'enemy'), 'Inside 50m should be visible'
            wall = spawn(world, ue.StaticMeshActor, ue.Vector(1250, 0, 2000), ue.Vector(.1, 10, 10))
            mesh = wall.get_component_by_class(ue.StaticMeshComponent)
            mesh.set_mobility(ue.ComponentMobility.MOVABLE); mesh.set_static_mesh(ue.load_asset('/Engine/BasicShapes/Cube'))
            mesh.set_collision_enabled(ue.CollisionEnabled.QUERY_AND_PHYSICS)
            # Fixed EngineTypes.h enum values: ECC_Visibility=3, ECR_Block=2;
            # avoid assuming editor-dependent Python enum display-name aliases.
            mesh.set_collision_response_to_channel(ue.CollisionChannel.cast(3), ue.CollisionResponseType.cast(2))
            state['target'].set_actor_location(ue.Vector(2500, 0, 2000), False, True)
            boss = spawn(world, boss_class, ue.Vector(0, 1000, 2000))
            state.update(step='wall_boss_objective', at=now, boss=boss, wall=wall); return
        if step == 'wall_boss_objective':
            if now - state['at'] < .4: return
            # Visibility is mapping index zero, not the weapon channel index two
            # used by the hit-zone probe. Require actual current-PIE occlusion.
            trace = ue.SystemLibrary.line_trace_single(world, pawn.get_actor_location(), state['target'].get_actor_location(),
                ue.TraceTypeQuery.cast(0), False, [pawn, state['target']], ue.DrawDebugTrace.NONE, True,
                ue.LinearColor(), ue.LinearColor(), 0)
            hits = [trace] if isinstance(trace, ue.HitResult) else [value for value in trace if isinstance(value, ue.HitResult)]
            assert len(hits) == 1, 'Visibility trace returned no hit result'
            # NativeBreakFunc is also hidden from direct Python aliases by
            # PyGenUtil::IsScriptExposedFunction, but the reflected UFunction exists.
            parts = game.call_method('BreakHitResult', args=(hits[0],))
            assert parts[0] and parts[9] == state['wall'], 'Diagnostic wall did not actually occlude the target'
            event('actual_visibility_occlusion', wall=state['wall'].get_path_name(), hit=hits[0].export_text())
            assert marker_for(radar, state['target'], 'enemy'), 'Wall must not hide enemy'
            assert marker_for(radar, state['boss'], 'enemy'), 'Real boss must use the same team/health filter'
            state['boss'].destroy_actor()
            state['goal'].set_actor_location(ue.Vector(100000, 100000, 2000), False, True)
            location = state['goal'].get_actor_location()
            assert (location.x, location.y, location.z) == (100000, 100000, 2000), 'Movable objective did not move'
            radar.call_method('SetObjectiveComponent', args=(state['component'],))
            objective = marker_for(radar, state['goal'], 'objective'); assert objective and objective['clamped']
            assert math.hypot(objective['center'][0] - 120, objective['center'][1] - 120) + 24 * math.sqrt(2) / 2 <= 116.01
            foreign_world = EDITOR_WORLD
            assert foreign_world and foreign_world != world and 'UEDPIE_' not in foreign_world.get_path_name()
            foreign_owners = list(ue.GameplayStatics.get_all_actors_of_class(foreign_world, ue.WorldSettings))
            assert foreign_owners, ('Foreign-world fixture has no existing WorldSettings owner', foreign_world.get_path_name())
            foreign_owner = foreign_owners[0]
            assert foreign_owner.get_world() == foreign_world, ('Foreign fixture owner world mismatch', foreign_owner.get_path_name())
            foreign = None
            try:
                # Empty Frontend worlds need not contain an existing SceneComponent.
                # Create only an unregistered memory object under an existing actor;
                # never spawn an Editor actor, add an instance component or save it.
                foreign = ue.new_object(ue.SceneComponent, outer=foreign_owner)
                assert valid(foreign), ('Foreign fixture component creation failed', foreign_owner.get_path_name())
                owner = foreign.get_owner()
                report['foreign_world_fixture'] = dict(editor_world=foreign_world.get_path_name(),
                    owner=foreign_owner.get_path_name(), component=foreign.get_path_name(),
                    component_owner=owner.get_path_name() if owner else None,
                    component_world=foreign.get_world().get_path_name() if foreign.get_world() else None,
                    registered_by_probe=False, editor_actor_spawned=False)
                assert owner == foreign_owner, ('Foreign component has no expected owner', report['foreign_world_fixture'])
                assert foreign.get_world() == foreign_world, ('Foreign component has no expected world', report['foreign_world_fixture'])
                radar.call_method('SetObjectiveComponent', args=(foreign,))
                assert radar.call_method('GetObjectiveComponent') is None, 'Radar accepted an actual foreign-world component'
            finally:
                if valid(foreign):
                    # K2_DestroyComponent checks its explicit caller against Owner.
                    foreign.call_method('K2_DestroyComponent', args=(foreign_owner,))
                    assert not valid(foreign), 'Foreign memory component was not destroyed by its owner'
            radar.call_method('SetObjectiveComponent', args=(state['component'],))
            state['goal'].destroy_actor()
            assert radar.call_method('GetObjectiveComponent') is None
            event('range_wall_boss_and_objective', objective=objective, foreign_world_rejected=True, deleted_owner_hidden=True)
            source = state['target']
            source_asc = gas.call_method('GetAbilitySystemComponent', args=(source,))
            assert valid(source) and source_asc, 'Keep the actual enemy source alive through diagnostic cleanup'
            cleanup_runtime()
            pc.set_control_rotation(state['original_control_rotation'])
            state.pop('control_pc', None)
            gas.call_method('RemoveLooseGameplayTags', args=(pawn, immunity, False)); state.pop('immune_pawn', None)
            for key in ('target', 'goal', 'component', 'boss', 'wall'): state.pop(key, None)
            health = pawn.get_component_by_class(health_class)
            state.update(step='death_wait', death_health=health, death_radar=radar)
            assert valid(source), 'Cleanup must not destroy the real enemy source'
            spec = source_asc.make_outgoing_spec(damage_class, 1, source_asc.make_effect_context())
            spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, damage_tag, 10000.0))
            source_asc.apply_gameplay_effect_spec_to_target(spec, gas.call_method('GetAbilitySystemComponent', args=(pawn,)))
            event('original_diagnostic_GE_triggered_death', source=source.get_path_name(), amount=10000,
                  weapon_or_balance_verification=False)
            assert not markers(radar), 'Original lethal damage must stop radar display'
            return
        if step in ('esc_menu', 'esc_final'):
            if now - state['at'] < .4 or not widgets(pc, menu_class, True): return
            click(child(one(pc, menu_class, True), 'ReturnButton'))
            state.update(step='esc_confirm' if step == 'esc_menu' else 'final_confirm', at=now); return
        if step in ('esc_confirm', 'final_confirm'):
            if now - state['at'] < .4 or not widgets(pc, dialog_class, True): return
            buttons = list(child(one(pc, dialog_class, True), 'EntryBox_Buttons').get_all_entries())
            assert len(buttons) == 2; click(buttons[0])
            assert mode.is_stage_travel_pending()
            event('original_esc_return', iteration=1 if step == 'esc_confirm' else 2)
            state.update(step='menu_return' if step == 'esc_confirm' else 'menu_final'); return
    except Exception:
        report['failures'].append(traceback.format_exc()); finish(False)
    finally:
        state['busy'] = False


state['callback'] = ue.register_slate_post_tick_callback(tick)
event('armed')
