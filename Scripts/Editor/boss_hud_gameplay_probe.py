"""Bounded boss HUD PIE verification, armed before Project.Maps.PIE on Frontend.

Separate rendered/offscreen unattended editor, -SCCProvider=None
-DCBossHUDProbe=<fresh> -DCBossHUDPaintDiagnostics -forcelogflush
-abslog=<project>/Saved/Diagnostics/BossHUD20261019/<fresh>/Editor.log
-GameUserSettingsINI=<same fresh folder>/GameUserSettings.ini
No asset/map/config saves. Original boss data, health, phase/death and stage travel
are used; temporary bosses and diagnostic GE damage are not weapon/balance proof.
After editor exit: python boss_hud_gameplay_probe.py --verify-report REPORT LOG INDEX EXIT_CODE
"""
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import time
import traceback

sys.dont_write_bytecode = True


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.is_relative_to((Path(__file__).resolve().parents[2] / 'Saved/Diagnostics/BossHUD20261019').resolve())
    report = json.loads(path.read_text(encoding='utf-8'))
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    errors = [line for line in log.splitlines() if re.search(
        r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    passed = (int(index.get('failed', -1)) == 0 and int(index.get('notRun', -1)) == 0
              and int(index.get('inProcess', -1)) == 0 and bool(tests) and all(row.get('state') == 'Success' for row in tests))
    matching = any('BOSS_HUD_PIE ' in line and '"event": "armed"' in line
                   and '"run": "' + report['run'] + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=passed,
        engine_errors=errors, matching_run_log=matching, log=str(Path(log_path).resolve()),
        automation_index=str(Path(index_path).resolve()))
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
match = re.search(r'-DCBossHUDProbe=([A-Za-z0-9_]{1,64})(?=\s|$)', COMMAND)
assert match and all(flag in COMMAND.lower() for flag in
    ('-unattended', '-sccprovider=none', '-dcbosshudpaintdiagnostics', '-forcelogflush'))
assert '-nullrhi' not in COMMAND.lower(), 'Actual child paint geometry requires a rendered viewport'
assert not ue.SourceControl.is_enabled()
RUN = match.group(1)
DIRECTORY = PROJECT / 'Saved/Diagnostics/BossHUD20261019' / RUN
DIRECTORY.mkdir(parents=True, exist_ok=True)
OUT = DIRECTORY / 'pie.json'; assert not OUT.exists()
log_match = re.search(r'-abslog=(?:"([^"]+)"|(\S+))', COMMAND, re.I); assert log_match
PAINT_LOG = Path(log_match.group(1) or log_match.group(2)).resolve()
assert PAINT_LOG.is_relative_to(DIRECTORY.resolve())
settings = re.search(r'-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
assert settings and Path(settings.group(1) or settings.group(2)).resolve().is_relative_to(DIRECTORY.resolve())
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Arm before Project.Maps.PIE starts'
# Editor-only world access is deliberately completed before PIE, as in the
# preceding radar probe. No Editor-world actor is spawned or modified.
EDITOR_WORLD = editor.get_editor_world(); assert EDITOR_WORLD


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot():
    return {str(p): [p.stat().st_size, p.stat().st_mtime_ns] for name in
        ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
        for p in (PROJECT / name).rglob('*') if p.is_file()}


before_files = snapshot()
protected_names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
    'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
    'Content/DreamCatcher/Data/Boss/DA_DC_Boss.uasset',
    'Saved/Config/WindowsEditor/GameUserSettings.ini', 'Saved/Config/Windows/GameUserSettings.ini']
protected = {str(PROJECT / name): sha(PROJECT / name) if (PROJECT / name).is_file() else None for name in protected_names}
state = dict(step='menu_initial', started=time.monotonic(), at=0., busy=False, done=False,
             temporary=[], volatile_widgets=[])
report = dict(run=RUN, complete=False, probe_passed=False, events=[], failures=[],
    asset_saves=0, cpp_build=False, physical_input_verified=False, visual_acceptance=False,
    weapon_or_balance_verified=False, full_stage_or_packaging_verified=False,
    diagnostic_damage_effect='/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')


def native(name):
    cls = ue.load_class(None, '/Script/DreamCatcher.' + name); assert cls, name
    return cls


def bp(path):
    cls = ue.load_class(None, path + '.' + path.rsplit('/', 1)[1] + '_C'); assert cls, path
    return cls


boss_class = bp('/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss')
health_class = native('DCLyraHealthComponent')
phase_class = native('DCBossPhaseComponent')
widget_class = native('DCBossHealthWidget')
overlay_class = native('DCStageOverlayWidget')
result_class = native('DCStageResultLayer')
hud_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout')
frontend_class = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
menu_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu')
dialog_class = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
damage_class = bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
ui = ue.get_default_object(ue.load_class(None, '/Script/CommonGame.CommonUIExtensions'))
gas = ue.get_default_object(ue.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
game = ue.get_default_object(ue.load_class(None, '/Script/Engine.GameplayStatics'))
menu_tag = ue.GameplayTag(); assert menu_tag.import_text('(TagName="UI.Layer.Menu")')
damage_tag = ue.GameplayTag(); assert damage_tag.import_text('(TagName="SetByCaller.Damage")')
immunity = ue.GameplayTagContainer(); assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
data = ue.load_asset('/Game/DreamCatcher/Data/Boss/DA_DC_Boss'); assert data
assert str(data.get_editor_property('DisplayName')) == '춘식이'
assert str(data.get_editor_property('Subtitle')) == '나스닥의 수호자'
gameplay_fields = ('MaxHealth', 'PhaseTwoHealthRatio', 'OpeningDelay', 'TransitionDuration', 'PhaseOneGap', 'PhaseTwoGap')
gameplay_baseline = {name: data.get_editor_property(name) for name in gameplay_fields}


def event(name, **values):
    row = dict(event=name, run=RUN, elapsed=round(time.monotonic() - state['started'], 3), **values)
    report['events'].append(row); print('BOSS_HUD_PIE ' + json.dumps(row, ensure_ascii=False))


def valid(obj):
    if obj is None: return False
    if not isinstance(obj, ue.Object):
        raise TypeError('Expected an unreal.Object wrapper or None')
    try:
        # PyWrapperObject.cpp ValidateInternalState (lines 81-88) raises this
        # exact native-instance error for a wrapper surviving its UObject/GC.
        # Check the built-in object method before UFunction argument marshaling,
        # which otherwise reports an ambiguous "Cannot nativize" TypeError.
        obj.get_path_name()
    except Exception as exc:
        if re.fullmatch(r'(?:[^:\r\n]+: )?Internal Error - ObjectInstance is null!', str(exc)):
            return False
        raise
    return ue.SystemLibrary.is_valid(obj)


def widgets(pc, world, cls, active=False):
    return [w for w in ue.ObjectIterator(ue.UserWidget) if w.get_class() == cls
        and w.get_owning_player() == pc and w.get_world() == world and not w.get_name().startswith('Default__')
        and (not active or w.is_activated())]


def one(pc, world, cls, active=False):
    values = widgets(pc, world, cls, active); assert len(values) == 1, (cls.get_name(), len(values))
    return values[0]


def owned_by(obj, owner):
    outer = obj.get_outer()
    while outer:
        if outer == owner: return True
        outer = outer.get_outer()
    return False


def child(owner, name):
    values = [w for w in ue.ObjectIterator(ue.Widget) if w.get_name() == name and owned_by(w, owner)]
    assert len(values) == 1, (owner.get_name(), name, len(values))
    return values[0]


def click(widget):
    if isinstance(widget, ue.CommonButtonBase): widget.call_method('HandleButtonClicked')
    else: widget.get_editor_property('OnClicked').broadcast()


def active_boss_entries(pc, world):
    # Only the currently activated HUD's registered entries constitute screen
    # ownership. An old overlay can retain its WidgetTree and internal Canvas
    # parents after extension removal; those UObjects are not active entries.
    layouts = widgets(pc, world, hud_class, True)
    assert len(layouts) <= 1, 'Multiple gameplay HUD layouts are activated'
    if not layouts: return []
    point = child(layouts[0], 'ExtensionPoint_ModeStatus')
    # UIExtensionPointWidget.h:20 derives from UDynamicEntryBoxBase. Its
    # inherited BlueprintCallable GetAllEntries (base .h:89-90) returns the
    # pool's active widgets (base .cpp:64-66), excluding released/pool objects.
    assert isinstance(point, ue.DynamicEntryBoxBase), point.get_class().get_path_name()
    entries = list(point.get_all_entries())
    overlays = [entry for entry in entries if valid(entry) and entry.get_class() == overlay_class]
    assert len(overlays) <= 1, 'Duplicate StageOverlay entries are registered on the active HUD'
    result = []
    for overlay in overlays:
        values = [w for w in widgets(pc, world, widget_class) if owned_by(w, overlay)]
        assert len(values) <= 1, 'Duplicate boss HUD children belong to an active StageOverlay'
        result.extend((overlay, widget) for widget in values)
    return result


def visible_boss_widgets(pc, world):
    hidden = (ue.SlateVisibility.COLLAPSED, ue.SlateVisibility.HIDDEN)
    return [widget for overlay, widget in active_boss_entries(pc, world)
        if overlay.get_visibility() not in hidden and widget.get_visibility() not in hidden]


def retained_boss_objects(pc, world):
    active = {widget.get_path_name() for _, widget in active_boss_entries(pc, world)}
    result = []
    for widget in ue.ObjectIterator(ue.UserWidget):
        if not valid(widget) or widget.get_class() != widget_class or widget.get_name().startswith('Default__'): continue
        owner = widget.get_owning_player(); obj_world = widget.get_world()
        result.append(dict(widget=widget.get_path_name(),
            world=obj_world.get_path_name() if valid(obj_world) else None,
            owning_player=owner.get_path_name() if valid(owner) else None,
            current_world=obj_world == world, current_player=owner == pc,
            registered_on_active_hud=widget.get_path_name() in active,
            own_visibility=str(widget.get_visibility())))
    return result


def observe_widget(widget):
    if not any(old == widget for old, _ in state['volatile_widgets']):
        # UWidget.h:387-388 is the reflected bIsVolatile property;
        # :619-620 is the actual BlueprintCallable ForceVolatile setter.
        previous = bool(widget.get_editor_property('bIsVolatile'))
        state['volatile_widgets'].append((widget, previous)); widget.force_volatile(True)
    texts = [str(obj.get_text()) for obj in ue.ObjectIterator(ue.TextBlock) if owned_by(obj, widget)]
    assert sorted(texts) == sorted(['춘식이', '나스닥의 수호자']), ('Unexpected phase/health/other text', texts)
    return texts


def native_paints(first_frame):
    ue.log_flush()
    if not PAINT_LOG.is_file(): return []
    with PAINT_LOG.open('rb') as handle:
        handle.seek(0, 2); handle.seek(max(0, handle.tell() - 131072))
        tail = handle.read(131072).decode('utf-8-sig', errors='replace')
    frame = int(ue.SystemLibrary.get_frame_count()); values = {}
    for line in tail.splitlines():
        if 'DCBOSS_HUD_PAINT ' not in line: continue
        try: value = json.loads(line.split('DCBOSS_HUD_PAINT ', 1)[1])
        except json.JSONDecodeError: continue
        painted = value.get('frame')
        if not isinstance(painted, int) or not first_frame <= painted <= frame: continue
        value['read_at_frame'] = frame
        values[(value.get('world'), value.get('widget'), painted)] = value
    return sorted(values.values(), key=lambda row: row['frame'])


def wait_for_paint(widget, world, fraction, label):
    observe_widget(widget)
    pending = state.get('paint_pending')
    if not pending or pending['label'] != label or pending['widget'] != widget:
        pending = dict(label=label, widget=widget, first_frame=int(ue.SystemLibrary.get_frame_count()), at=time.monotonic())
        state['paint_pending'] = pending
    values = [row for row in native_paints(pending['first_frame']) if row['world'] == world.get_path_name()
              and row['read_at_frame'] - row['frame'] <= 16]
    assert {row['widget'] for row in values} <= {widget.get_path_name()}, 'More than one boss HUD actually painted'
    for row in reversed(values):
        if abs(row['fraction'] - fraction) > .0001: continue
        for key in ('root_size', 'panel_position', 'panel_size', 'bar_position', 'bar_size', 'border_size', 'fill_size'):
            assert len(row[key]) == 2 and all(math.isfinite(n) for n in row[key]), (key, row)
        assert min(row['root_size']) > 0
        for key, expected in [('panel_size', (720, 86)), ('bar_size', (640, 18)),
                              ('border_size', (640, 18)), ('fill_size', (640, 18)), ('bar_position', (40, 62))]:
            assert max(abs(a-b) for a, b in zip(row[key], expected)) < .1, (key, row)
        expected = ((row['root_size'][0] - 720) / 2, 88)
        assert max(abs(a-b) for a, b in zip(row['panel_position'], expected)) < .1, row
        assert row['name'] == '춘식이' and row['subtitle'] == '나스닥의 수호자'
        assert row['border_material'].endswith('MI_UI_HealthBar_BarBorder.MI_UI_HealthBar_BarBorder')
        assert row['fill_material'].endswith('MI_UI_HealthBar_Fill.MI_UI_HealthBar_Fill')
        report.setdefault('actual_native_geometry', {})[label] = dict(
            source='actual_NativePaint_and_native_child_paint_geometry', native_paint=row)
        state.pop('paint_pending', None); return row
    assert time.monotonic() - pending['at'] < 4, 'No fresh actual boss HUD paint for ' + label
    return None


def observe_hidden(pc, world, label):
    assert not visible_boss_widgets(pc, world), ('Boss HUD remains visible', label)
    observation = state.get('hidden_pending')
    if not observation or observation['label'] != label:
        observation = dict(label=label, first_frame=int(ue.SystemLibrary.get_frame_count()), at=time.monotonic())
        state['hidden_pending'] = observation
    assert not native_paints(observation['first_frame'] + 1), ('Hidden boss HUD still paints', label)
    frame = int(ue.SystemLibrary.get_frame_count())
    if frame - observation['first_frame'] < 24 or time.monotonic() - observation['at'] < .1: return False
    report.setdefault('hidden_observations', {})[label] = dict(first_frame=observation['first_frame'],
        last_frame=frame, current_visible_count=0, new_native_paints=0,
        active_hud_count=len(widgets(pc, world, hud_class, True)),
        retained_objects=retained_boss_objects(pc, world))
    state.pop('hidden_pending', None); return True


def spawn_boss(world, pawn):
    assert world == editor.get_game_world() and 'UEDPIE_' in world.get_path_name()
    transform = ue.Transform(location=pawn.get_actor_location() + ue.Vector(1200, 0, 0))
    actor = game.call_method('BeginDeferredActorSpawnFromClass', args=(world, boss_class, transform,
        ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor and actor.get_world() == world; state['temporary'].append(actor)
    actor = game.call_method('FinishSpawningActor', args=(actor, transform, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor.get_world() == world
    return actor


def damage(source, target, amount):
    assert valid(source) and valid(target) and source.get_world() == target.get_world()
    source_asc = gas.call_method('GetAbilitySystemComponent', args=(source,))
    target_asc = gas.call_method('GetAbilitySystemComponent', args=(target,))
    assert source_asc and target_asc
    spec = source_asc.make_outgoing_spec(damage_class, 1, source_asc.make_effect_context())
    spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, damage_tag, float(amount)))
    source_asc.apply_gameplay_effect_spec_to_target(spec, target_asc)


def own_immunity(pawn):
    assert valid(pawn), 'Current immunity target must be a live UObject'
    previous = state.get('immune_pawn')
    if valid(previous):
        if previous == pawn: return
        # Remove only the single contribution previously added by this probe.
        gas.call_method('RemoveLooseGameplayTags', args=(previous, immunity, False))
    state.pop('immune_pawn', None)
    gas.call_method('AddLooseGameplayTags', args=(pawn, immunity, False)); state['immune_pawn'] = pawn


def finish(ok):
    if state['done']: return
    state['done'] = True
    try:
        immune_pawn = state.pop('immune_pawn', None)
        if valid(immune_pawn):
            gas.call_method('RemoveLooseGameplayTags', args=(immune_pawn, immunity, False))
    except Exception: report['failures'].append(traceback.format_exc())
    # Each owned restoration is independent. One expired/broken wrapper must
    # never prevent later widgets or temporary actors from being cleaned up.
    for widget, previous in state['volatile_widgets']:
        try:
            if valid(widget): widget.force_volatile(previous)
        except Exception: report['failures'].append(traceback.format_exc())
    for actor in state['temporary']:
        try:
            if valid(actor): actor.destroy_actor()
        except Exception: report['failures'].append(traceback.format_exc())
    after = snapshot()
    report['protected_changes'] = [p for p, value in before_files.items() if after.get(p) != value]
    report['new_protected_files'] = sorted(set(after) - set(before_files))
    report['protected_hash_changes'] = [p for p, value in protected.items()
        if (sha(Path(p)) if Path(p).is_file() else None) != value]
    report['gameplay_data_unchanged'] = all(data.get_editor_property(name) == value for name, value in gameplay_baseline.items())
    report['screenshots'] = []
    for path in DIRECTORY.glob('boss_hud*.png'):
        with path.open('rb') as handle: signature = handle.read(8)
        if path.stat().st_size > 32 and signature == b'\x89PNG\r\n\x1a\n': report['screenshots'].append(str(path))
    if ok and not report['screenshots']: report['failures'].append('SHOWUI screenshot produced no valid PNG')
    report['probe_passed'] = bool(ok and report['gameplay_data_unchanged'] and not report['failures']
        and not report['protected_changes'] and not report['new_protected_files'] and not report['protected_hash_changes'])
    report['final_verification_pending'] = True
    event('finished', probe_passed=report['probe_passed'], final_verification_pending=True)
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not report['probe_passed']: ue.log_error('Boss HUD PIE verification failed: ' + str(OUT))


def tick(delta):
    if state['done'] or state['busy']: return
    state['busy'] = True
    try:
        assert time.monotonic() - state['started'] < 180, 'Boss HUD probe timeout: ' + state['step']
        world = editor.get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name(): return
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc: return
        now = ue.GameplayStatics.get_time_seconds(world)
        level = ue.GameplayStatics.get_current_level_name(world, True)
        step = state['step']
        if step in ('menu_initial', 'menu_return', 'menu_final'):
            if level != 'L_DC_FrontEnd' or not widgets(pc, world, frontend_class, True) or now < 2: return
            assert not widgets(pc, world, hud_class, True) and not pc.get_controlled_pawn()
            if not observe_hidden(pc, world, step): return
            if step == 'menu_final': finish(True); return
            click(child(one(pc, world, frontend_class, True), 'StoryButton'))
            event(step, visible_boss_hud_count=0)
            state.update(step='initial' if step == 'menu_initial' else 'reentry'); return
        if level != 'Level_1': return
        pawn = pc.get_controlled_pawn(); mode = ue.GameplayStatics.get_game_mode(world)
        if step == 'death_wait':
            # Existing StageOverlay refreshes at .1s. Allow that original
            # presentation cadence before requiring the display to disappear.
            if now - state['at'] < .3: return
            if not observe_hidden(pc, world, 'player_death'): return
            if pawn or not widgets(pc, world, result_class, True): return
            assert state.get('death_started_seen') and mode.has_run_ended()
            click(child(one(pc, world, result_class, True), 'RetryButton'))
            assert mode.is_stage_travel_pending()
            event('original_death_and_retry', boss_hidden=True)
            state.update(step='restart', travel_at=now); return
        if step == 'restart' and now >= state['travel_at'] - .5: return
        if not widgets(pc, world, hud_class, True) or now < 2 or not pawn: return
        if step in ('initial', 'restart', 'reentry'):
            health = pawn.get_component_by_class(health_class)
            if not health or health.get_max_health() <= 0 or health.is_dead_or_dying(): return
            own_immunity(pawn)
            if not observe_hidden(pc, world, step): return
            if step == 'restart':
                event('restart', fresh_player=pawn.get_path_name(), visible_boss_hud_count=0)
                player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(player, menu_tag, menu_class))
                state.update(step='esc_menu', at=now); return
            boss = spawn_boss(world, pawn)
            state.update(step='full' if step == 'initial' else 'reentry_full', at=now, boss=boss)
            return
        if step in ('esc_menu', 'esc_final'):
            if now - state['at'] < .4 or not widgets(pc, world, menu_class, True): return
            click(child(one(pc, world, menu_class, True), 'ReturnButton'))
            state.update(step='confirm' if step == 'esc_menu' else 'final_confirm', at=now); return
        if step in ('confirm', 'final_confirm'):
            if now - state['at'] < .4 or not widgets(pc, world, dialog_class, True): return
            buttons = list(child(one(pc, world, dialog_class, True), 'EntryBox_Buttons').get_all_entries())
            assert len(buttons) == 2; click(buttons[0]); assert mode.is_stage_travel_pending()
            event('original_menu_return', iteration=1 if step == 'confirm' else 2)
            state.update(step='menu_return' if step == 'confirm' else 'menu_final'); return
        boss = state['boss']
        if step == 'boss_death':
            if now - state['at'] < .3: return
            if not observe_hidden(pc, world, 'boss_death'): return
            health = boss.get_component_by_class(health_class) if valid(boss) else None
            if valid(boss) and health and 'DEATH_FINISHED' not in str(health.get_death_state()).upper(): return
            event('original_boss_death', death_finished_or_removed=True)
            if valid(boss): return  # wait for original Pawn removal, never shorten Death
            state['boss'] = spawn_boss(world, pawn); state.update(step='player_death_ready', at=now); return
        if not valid(boss): raise AssertionError('Actual boss disappeared before ' + step)
        health = boss.get_component_by_class(health_class); phase = boss.get_component_by_class(phase_class)
        assert health and phase and health.get_max_health() > 0
        visible = visible_boss_widgets(pc, world)
        if not visible:
            assert now - state['at'] < 3, 'Boss data is live but HUD never became visible'; return
        assert len(visible) == 1, 'Duplicate boss HUD display'
        widget = visible[0]
        fraction = health.get_health() / health.get_max_health()
        if step in ('full', 'reentry_full', 'player_death_ready'):
            painted = wait_for_paint(widget, world, 1.0, step)
            if painted is None: return
            event(step, actual_health=health.get_health(), max_health=health.get_max_health(), native_paint=painted)
            if step == 'reentry_full':
                player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(player, menu_tag, menu_class))
                state.update(step='esc_final', at=now); return
            if step == 'player_death_ready':
                sources = [a for a in ue.GameplayStatics.get_all_actors_of_class(world, ue.Pawn)
                    if a != pawn and a != boss and a.get_class() != boss_class
                    and a.get_component_by_class(health_class)
                    and not a.get_component_by_class(health_class).is_dead_or_dying()
                    and gas.call_method('GetAbilitySystemComponent', args=(a,))]
                assert sources, 'Original enemy source is unavailable for player death verification'
                gas.call_method('RemoveLooseGameplayTags', args=(pawn, immunity, False)); state.pop('immune_pawn', None)
                damage(sources[0], pawn, 10000)
                state['death_started_seen'] = pawn.get_component_by_class(health_class).is_dead_or_dying()
                assert state['death_started_seen']
                state.update(step='death_wait', at=now); return
            damage(pawn, boss, health.get_max_health() * .25)
            state.update(step='damaged', at=now); return
        if step == 'damaged':
            assert abs(fraction - .75) < .001 and phase.get_boss_phase() == 1
            painted = wait_for_paint(widget, world, .75, step)
            if painted is None: return
            event('actual_health_change', native_paint=painted, actual_health=health.get_health())
            damage(pawn, boss, health.get_max_health() * .30)
            assert phase.get_boss_phase() == 2 and 'TRANSITIONING' in str(phase.get_boss_run_state()).upper()
            event('actual_phase_threshold', phase=2, state=str(phase.get_boss_run_state()))
            state.update(step='transition', at=now); return
        if step == 'transition':
            assert abs(fraction - .45) < .001 and phase.get_boss_phase() == 2
            painted = wait_for_paint(widget, world, .45, step)
            if painted is None: return
            report['phase_transition_paint'] = painted
            if 'FIGHTING' not in str(phase.get_boss_run_state()).upper(): return
            event('actual_phase_transition_finished', phase=2, native_paint=painted,
                transition_duration=gameplay_baseline['TransitionDuration'])
            ue.SystemLibrary.execute_console_command(world,
                'Shot SHOWUI nosuffix filename="' + (DIRECTORY / 'boss_hud.png').as_posix() + '"', pc)
            state.update(step='screenshot_wait', at=now); return
        if step == 'screenshot_wait':
            # Shot captures a later render frame; keep the boss alive and HUD
            # visible until that capture has had time to complete.
            if now - state['at'] < .6: return
            damage(pawn, boss, 10000)
            assert health.is_dead_or_dying()
            state.update(step='boss_death', at=now); return
    except Exception:
        report['failures'].append(traceback.format_exc()); finish(False)
    finally:
        state['busy'] = False


state['callback'] = ue.register_slate_post_tick_callback(tick)
event('armed')
