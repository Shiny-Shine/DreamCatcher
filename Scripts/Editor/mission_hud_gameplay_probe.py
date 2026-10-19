"""TODO4 real-flow PIE probe, armed on Frontend before Project.Maps.PIE.

Separate rendered/offscreen unattended editor, -SCCProvider=None
-DCMissionHUDProbe=<fresh> -DCMissionHUDProbeResolution=1920x1080|1280x720
-DCMissionHUDPaintDiagnostics -DCRadarPaintDiagnostics -DCBossHUDPaintDiagnostics
-forcelogflush -abslog=<project>/Saved/Diagnostics/MissionHUD20261019/<fresh>/Editor.log
-GameUserSettingsINI=<same fresh folder>/GameUserSettings.ini
Allow at least 300 seconds for Project.Maps.PIE. Run separately for each requested
resolution. The requested size is checked against the actual player viewport;
this script does not pretend r.SetRes resizes an embedded editor viewport.
It records a size mismatch while continuing the remaining feature checks.
In-panel hold/fade evidence uses actual DCMISSION_ROW_PAINT inherited alpha and
text, separately from feature-flow/resolution results. Sparse frames are reported
as sampling limits. SHOWUI PNGs are auxiliary captures, never fade/UI proof.

Only temporary PIE state and transient objects are changed. No asset/config/save
writes, native build, physical input, subjective visual or weapon balance proof.
After editor exit: python mission_hud_gameplay_probe.py --verify-report REPORT LOG INDEX EXIT_CODE
"""
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys
import time
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
REPORT_ROOT = PROJECT / 'Saved/Diagnostics/MissionHUD20261019'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    return {str(path.relative_to(PROJECT)): [path.stat().st_size, path.stat().st_mtime_ns]
        for root in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
        for path in (PROJECT / root).rglob('*') if path.is_file()}


def protected_hashes():
    names = ('.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'AGENTS.md',
        'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
        'Content/DreamCatcher/Data/Boss/DA_DC_Boss.uasset',
        'Content/DreamCatcher/Data/Stage/DA_DC_MissionList_Example.uasset',
        'Content/DreamCatcher/UI/HUD/W_DC_MissionEntry.uasset',
        'Content/DreamCatcher/UI/HUD/W_DC_Radar.uasset',
        'Saved/Config/WindowsEditor/GameUserSettings.ini', 'Saved/Config/Windows/GameUserSettings.ini')
    return {name: sha(PROJECT / name) if (PROJECT / name).is_file() else None for name in names}


def log_observations(log):
    errors = [line for line in log.splitlines() if re.search(
        r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    warnings = {}
    for line in log.splitlines():
        if ': Warning:' not in line: continue
        if 'TapDelay' in line or 'WeaponAudioFunctions' in line:
            category = 'weapon_audio_tap_delay'
        elif 'PoseAsset' in line:
            category = 'pose_asset'
        elif 'Navigation' in line or 'Crowd' in line:
            category = 'navigation_crowd'
        else:
            category = 'other_requires_review'
        bucket = warnings.setdefault(category, dict(count=0, lines=[]))
        bucket['count'] += 1
        if line not in bucket['lines']: bucket['lines'].append(line)
    return errors, warnings


def native_paint_rows(log, prefix, first_frame, last_frame):
    values = {}
    for line in log.splitlines():
        if prefix + ' ' not in line: continue
        try: value = json.loads(line.split(prefix + ' ', 1)[1])
        except json.JSONDecodeError: continue  # Bounded tails can start within a line.
        frame = value.get('frame')
        if not isinstance(frame, int) or not first_frame <= frame <= last_frame: continue
        values[(value.get('widget'), frame)] = value
    return sorted(values.values(), key=lambda row: (row['frame'], row.get('widget', '')))


def transition_evidence(observation, row_samples, hud_samples):
    """Only actual native row paint establishes text, counters and fade alpha."""
    first, last = observation['first_frame'], observation['last_frame']
    rows = [row for row in row_samples if first <= row['frame'] <= last
        and row.get('world') == observation['world'] and row.get('source') == observation['source']
        and row.get('serial') == observation['serial']]
    hud = [row for row in hud_samples if first <= row['frame'] <= last
        and row.get('world') == observation['world'] and row.get('source') == observation['source']
        and row.get('widget') == observation['widget'] and row.get('serial') == observation['serial']]
    old = [row for row in rows if row.get('phase_index') == observation['phase']
        and row.get('page') == observation['old_page']]
    incoming = [row for row in rows if row.get('phase_index') == observation.get('incoming_phase')
        and row.get('page') == observation.get('incoming_page')]
    problems = []
    for row in old + incoming:
        if not isinstance(row.get('alpha'), (float, int)) or not math.isfinite(row['alpha']) or not -.001 <= row['alpha'] <= 1.001:
            problems.append('invalid_actual_row_alpha'); continue
        expected = observation['outgoing'] if row in old else observation.get('incoming', [])
        wanted = next((value for value in expected if value['id'] == row.get('id')), None)
        if not wanted or any(row.get(key) != wanted[key] for key in ('current', 'target', 'completed')):
            problems.append('native_row_counter_or_identity_mismatch'); continue
        text = row.get('text', '')
        if wanted['text'] not in text: problems.append('native_row_text_mismatch')
        if 'GUIDANCE' in wanted['source'].upper():
            if text != wanted['text']: problems.append('guidance_text_changed')
        elif not text.startswith(str(wanted['current']) + '/' + str(wanted['target'])):
            problems.append('native_counter_text_mismatch')
        if wanted['completed'] and 'DEFEAT' in wanted['source'].upper() and not text.endswith(' · 완료'):
            problems.append('completed_defeat_suffix_missing')
    pages_by_frame = {}
    visible_ids = set()
    expected_pages = {(observation['phase'], observation['old_page']),
        (observation.get('incoming_phase'), observation.get('incoming_page'))}
    for row in rows:
        if isinstance(row.get('alpha'), (float, int)) and row['alpha'] > .02:
            pages_by_frame.setdefault(row['frame'], set()).add((row.get('phase_index'), row.get('page')))
            if (row.get('phase_index'), row.get('page')) not in expected_pages:
                problems.append('unexpected_phase_or_page_actually_painted')
            key = (row['frame'], row.get('phase_index'), row.get('page'), row.get('id'))
            if key in visible_ids: problems.append('duplicate_visible_mission_row')
            visible_ids.add(key)
    if any(len(pages) > 1 for pages in pages_by_frame.values()): problems.append('two_body_pages_actually_painted_together')
    geometry = [row for row in hud if row.get('transition_state') in ('fade_out', 'fade_in')]
    geometry_base = None
    for row in geometry:
        if any(len(row.get(key, [])) != 2 or not all(math.isfinite(value) for value in row[key])
                for key in ('panel_position', 'panel_size', 'list_size')):
            problems.append('invalid_native_panel_geometry'); continue
        if geometry_base and any(abs(a - b) > .2 for key in ('panel_position', 'panel_size')
                for a, b in zip(row[key], geometry_base[key])): problems.append('panel_changed_during_fade')
        if geometry_base is None: geometry_base = row
        drawn = sorted(row.get('painted_rows', []), key=lambda value: value['position'][1])
        if any(value['position'][1] < -.2 or value['position'][1] + value['size'][1] > row['list_size'][1] + .5
                for value in drawn): problems.append('native_transition_row_clipped')
        if any(second['position'][1] < first_row['position'][1] + first_row['size'][1] - .2
                for first_row, second in zip(drawn, drawn[1:])): problems.append('native_rows_overlap')
    old_count = [row for row in old if row.get('id') == 'DefeatNightmares' and row.get('completed')]
    new_count = [row for row in incoming if row.get('id') == 'DefeatNightmares']
    bands = dict(completed_full=[row for row in old_count if row.get('alpha', -1) >= .98],
        outgoing_partial=[row for row in old_count if 0 < row.get('alpha', -1) < 1],
        incoming_partial=[row for row in new_count if 0 < row.get('alpha', -1) < 1],
        incoming_full=[row for row in new_count if row.get('alpha', -1) >= .98])
    central = dict(outgoing_partial=[row for row in old_count if .02 < row.get('alpha', -1) < .98],
        incoming_partial=[row for row in new_count if .02 < row.get('alpha', -1) < .98])
    sequence = [values[0] for values in bands.values() if values]
    if len(sequence) == 4 and [row['frame'] for row in sequence] != sorted(row['frame'] for row in sequence):
        problems.append('actual_fade_band_order_mismatch')
    missing = [name for name, values in bands.items() if not values]
    geometry_states = {row['transition_state'] for row in geometry}
    for name in ('fade_out', 'fade_in'):
        if name not in geometry_states: missing.append('native_panel_geometry_' + name)
    injected = observation.get('midpoint_edit')
    first_incoming_frame = min((row['frame'] for row in incoming), default=None)
    first_incoming = [row for row in incoming if row['frame'] == first_incoming_frame]
    first_by_id = {}
    for row in sorted(incoming, key=lambda value: (value['frame'], value.get('widget', ''))):
        first_by_id.setdefault(row['id'], row)
    midpoint_checks = []
    for wanted in injected['expected'] if injected else []:
        first = first_by_id.get(wanted['id'])
        expected_text = wanted['text'] if 'GUIDANCE' in wanted['source'].upper() else (
            str(wanted['current']) + '/' + str(wanted['target']) + '  ' + wanted['text']
            + (' · 완료' if wanted['completed'] and 'DEFEAT' in wanted['source'].upper() else ''))
        check = dict(id=wanted['id'], first_frame=first['frame'] if first else None,
            injection_frame=injected['frame'], after_injection=bool(first and first['frame'] > injected['frame']),
            expected_text=expected_text, actual_text=first.get('text') if first else None,
            text_matches=bool(first and first.get('text') == expected_text),
            counters_match=bool(first and all(first.get(key) == wanted[key] for key in ('current', 'target', 'completed'))))
        check['passed'] = check['after_injection'] and check['text_matches'] and check['counters_match']
        midpoint_checks.append(check)
        if first and not (check['text_matches'] and check['counters_match']):
            problems.append('midpoint_first_incoming_text_or_counter_mismatch')
    midpoint_verified = bool(midpoint_checks and all(check['passed'] for check in midpoint_checks)
        and not any(problem.startswith('native_row_') or problem == 'native_counter_text_mismatch' for problem in problems))
    if not observation.get('next_phase_and_radar_while_holding'): missing.append('next_phase_and_radar_while_holding')
    if not observation.get('radar_native_during_hold'): missing.append('new_objective_native_radar_paint_while_holding')
    geometry_problems = {'invalid_native_panel_geometry', 'panel_changed_during_fade', 'native_rows_overlap', 'native_transition_row_clipped'}
    return dict(status='failed' if problems else 'sampling_limit' if missing else 'passed',
        problems=sorted(set(problems)), missing_samples=missing, row_paints=len(rows), hud_paints=len(hud),
        actual_row_alpha_verified=all(bands.values()) and not any(problem in ('invalid_actual_row_alpha', 'actual_fade_band_order_mismatch') for problem in problems),
        native_panel_geometry_verified=geometry_states == {'fade_out', 'fade_in'} and not geometry_problems.intersection(problems),
        native_panel_geometry_states=sorted(geometry_states),
        band_counts={name: len(values) for name, values in bands.items()},
        representative_actual_paints={name: values[0] for name, values in bands.items() if values},
        partial_definition='0 < actual NativePaint alpha < 1; central interval is separate sampling coverage',
        central_partial_counts={name: len(values) for name, values in central.items()},
        representative_central_partial_paints={name: values[0] for name, values in central.items() if values},
        central_partial_coverage='passed' if all(central.values()) else 'sampling_limit',
        central_partial_missing_samples=[name for name, values in central.items() if not values],
        panel_geometry_samples=len(geometry), no_two_page_overlap_observed=bool(rows) and not any(len(pages) > 1 for pages in pages_by_frame.values()),
        midpoint_latest_read='verified' if midpoint_verified else 'not_injected_or_sampling_limit',
        first_incoming_frame=first_incoming_frame, first_incoming_actual_paints=first_incoming,
        first_incoming_by_id=first_by_id, midpoint_first_paint_checks=midpoint_checks,
        header_limit='Header is outside the body switcher in source; no separate header alpha/geometry diagnostic exists',
        evidence='NativePaint inherited style alpha/text only; no renderOpacity, Python geometry, slot or PNG inference')


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.name == 'pie.json' and path.is_relative_to(REPORT_ROOT.resolve())
    result = json.loads(path.read_text(encoding='utf-8'))
    assert Path(log_path).resolve().is_relative_to(path.parent)
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    errors, warnings = log_observations(log)
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    passed = (int(index.get('failed', -1)) == 0 and int(index.get('notRun', -1)) == 0
        and int(index.get('inProcess', -1)) == 0 and bool(tests)
        and all(row.get('state') == 'Success' for row in tests))
    matching = any('MISSION_HUD_PIE ' in line and '"event": "armed"' in line
        and '"run": "' + result['run'] + '"' in line for line in log.splitlines())
    baseline = json.loads((path.parent / 'protection_before.json').read_text(encoding='utf-8'))
    disk_ok = inventory() == baseline['inventory'] and protected_hashes() == baseline['hashes']
    result['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=passed,
        engine_errors=errors, warnings=warnings, matching_run_log=matching,
        protection_after_editor_exit=disk_ok, log=str(Path(log_path).resolve()),
        automation_index=str(Path(index_path).resolve()),
        warning_context='-nosound may produce existing TapDelay/WeaponAudioFunctions warnings; '
            'navigation/crowd and PoseAsset warnings remain separately recorded, with other warnings requiring review')
    result['complete'] = bool(result.get('probe_passed') and int(exit_code) == 0
        and passed and matching and not errors and disk_ok)
    result['final_verification_pending'] = False
    path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=result['complete'], automation_passed=passed,
        engine_errors=len(errors), warning_counts={key: value['count'] for key, value in warnings.items()},
        requested_resolution=result['requested_resolution'], resolution_verified=result.get('resolution_verified', False))))
    return 0 if result['complete'] else 1


if '--verify-report' in sys.argv:
    position = sys.argv.index('--verify-report')
    assert len(sys.argv[position + 1:]) == 4
    raise SystemExit(verify_report(*sys.argv[position + 1:]))

import unreal as ue

COMMAND = ue.SystemLibrary.get_command_line()
match = re.search(r'-DCMissionHUDProbe=([A-Za-z0-9_]{1,64})(?=\s|$)', COMMAND)
size_match = re.search(r'-DCMissionHUDProbeResolution=(1920x1080|1280x720)(?=\s|$)', COMMAND)
assert match and size_match, 'Use fresh run and explicit requested resolution flags'
assert all(flag in COMMAND.lower() for flag in ('-unattended', '-sccprovider=none',
    '-dcmissionhudpaintdiagnostics', '-dcradarpaintdiagnostics', '-dcbosshudpaintdiagnostics', '-forcelogflush'))
assert '-nullrhi' not in COMMAND.lower() and not ue.SourceControl.is_enabled()
RUN = match.group(1)
EXPECTED_SIZE = [int(value) for value in size_match.group(1).split('x')]
DIRECTORY = REPORT_ROOT / RUN
DIRECTORY.mkdir(parents=True, exist_ok=True)
OUT = DIRECTORY / 'pie.json'
assert not OUT.exists() and not (DIRECTORY / 'protection_before.json').exists()
log_match = re.search(r'-abslog=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
settings = re.search(r'-GameUserSettingsINI=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
assert log_match and settings
PAINT_LOG = Path(log_match.group(1) or log_match.group(2)).resolve()
assert PAINT_LOG.is_relative_to(DIRECTORY.resolve())
assert Path(settings.group(1) or settings.group(2)).resolve().is_relative_to(DIRECTORY.resolve())
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Arm before Project.Maps.PIE; do not inspect Editor world during PIE'
before = dict(inventory=inventory(), hashes=protected_hashes())
(DIRECTORY / 'protection_before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
state = dict(step='menu_initial', started=time.monotonic(), busy=False, done=False,
    volatile=[], mutations=[], seen_missions=[], killed=set(), visited_phases=set())
report = dict(run=RUN, complete=False, probe_passed=False, events=[], failures=[], asset_saves=0,
    cpp_build=False, physical_input_verified=False, visual_acceptance=False, packaging_verified=False,
    weapon_or_balance_verified=False, requested_resolution=EXPECTED_SIZE, resolution_verified=False,
    two_resolution_coverage='Each run verifies one measured resolution; parent must collect both isolated reports',
    offscreen_render='-renderoffscreen' in COMMAND.lower(),
    transition_observations=[], fade_visual_verified=False,
    fade_visual_scope='Actual native row paint is separate from flow/resolution/normal-exit verification',
    latest_wins_overlap_runtime='Not exercised here; retry covers cancellation during an actual outgoing transition',
    diagnostic_damage_effect='/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')


def native(name):
    cls = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert cls, name
    return cls


def bp(path):
    cls = ue.load_class(None, path + '.' + path.rsplit('/', 1)[1] + '_C')
    assert cls, path
    return cls


CLASSES = {name: native(name) for name in ('DCStageOverlayWidget', 'DCMissionObjectiveWidget',
    'DCMissionListView', 'DCMissionListData', 'DCBossHealthWidget', 'DCLyraHealthComponent',
    'DCStageResultLayer', 'DCBossCharacter', 'DCBossPhaseComponent', 'DCEnemySpawner', 'DCEncounterController')}
HUD = bp('/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout')
FRONTEND = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
MENU = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu')
DIALOG = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
DAMAGE = bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
ui = ue.get_default_object(ue.load_class(None, '/Script/CommonGame.CommonUIExtensions'))
gas = ue.get_default_object(ue.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
math_library = ue.get_default_object(ue.load_class(None, '/Script/Engine.KismetMathLibrary'))
immunity = ue.GameplayTagContainer()
assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
damage_tag = ue.GameplayTag(); assert damage_tag.import_text('(TagName="SetByCaller.Damage")')
menu_tag = ue.GameplayTag(); assert menu_tag.import_text('(TagName="UI.Layer.Menu")')
boss_data = ue.load_asset('/Game/DreamCatcher/Data/Boss/DA_DC_Boss')
mission_data = ue.load_asset('/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example')
assert boss_data and mission_data
boss_fields = ('MaxHealth', 'PhaseTwoHealthRatio', 'OpeningDelay', 'TransitionDuration', 'PhaseOneGap', 'PhaseTwoGap')
boss_baseline = {name: boss_data.get_editor_property(name) for name in boss_fields}


def definition_snapshot(data):
    return dict(missions=[value.export_text() for value in data.get_editor_property('Missions')],
        default=str(data.get_editor_property('DefaultTrackedMissionId')),
        style=data.get_editor_property('HUDStyle').export_text())


original_definition = definition_snapshot(mission_data)


def event(name, **values):
    value = dict(event=name, run=RUN, elapsed=round(time.monotonic() - state['started'], 3), **values)
    report['events'].append(value)
    print('MISSION_HUD_PIE ' + json.dumps(value, ensure_ascii=False))


def valid(obj):
    if obj is None: return False
    if not isinstance(obj, ue.Object): raise TypeError('Expected unreal.Object or None')
    try:
        obj.get_path_name()
    except Exception as exc:
        if re.fullmatch(r'(?:[^:\r\n]+: )?Internal Error - ObjectInstance is null!', str(exc)): return False
        raise
    return ue.SystemLibrary.is_valid(obj)


def widgets(pc, world, cls, active=False):
    return [widget for widget in ue.ObjectIterator(ue.UserWidget) if valid(widget)
        and widget.get_class() == cls and not widget.get_name().startswith('Default__')
        and widget.get_world() == world and widget.get_owning_player() == pc
        and (not active or widget.is_activated())]


def one(pc, world, cls, active=False):
    values = widgets(pc, world, cls, active)
    assert len(values) == 1, (cls.get_name(), len(values))
    return values[0]


def owned_by(obj, owner):
    outer = obj.get_outer()
    while outer:
        if outer == owner: return True
        outer = outer.get_outer()
    return False


def child(owner, name):
    values = [widget for widget in ue.ObjectIterator(ue.Widget) if valid(widget)
        and widget.get_name() == name and owned_by(widget, owner)]
    assert len(values) == 1, (owner.get_path_name(), name, len(values))
    return values[0]


def owned_class(owner, cls):
    values = [widget for widget in ue.ObjectIterator(ue.UserWidget) if valid(widget)
        and widget.get_class() == cls and owned_by(widget, owner)]
    assert len(values) == 1, (owner.get_path_name(), cls.get_name(), len(values))
    return values[0]


def click(widget):
    if isinstance(widget, ue.CommonButtonBase): widget.call_method('HandleButtonClicked')
    else: widget.get_editor_property('OnClicked').broadcast()


def active_hud(pc, world, required=True):
    layouts = widgets(pc, world, HUD, True)
    assert len(layouts) <= 1, 'Duplicate activated shooter HUD'
    if not layouts:
        assert not required, 'No active gameplay HUD'
        return None
    layout = layouts[0]
    point = child(layout, 'ExtensionPoint_ModeStatus')
    assert isinstance(point, ue.DynamicEntryBoxBase)
    overlays = [entry for entry in point.get_all_entries() if valid(entry)
        and entry.get_class() == CLASSES['DCStageOverlayWidget']]
    assert len(overlays) <= 1, 'Duplicate active stage overlays'
    if not overlays:
        assert not required, 'Active ModeStatus has no stage overlay'
        return None
    mission = owned_class(overlays[0], CLASSES['DCMissionObjectiveWidget'])
    boss = owned_class(overlays[0], CLASSES['DCBossHealthWidget'])
    if not any(previous == mission for previous in state['seen_missions']): state['seen_missions'].append(mission)
    return dict(layout=layout, overlay=overlays[0], mission=mission, boss=boss, radar=child(layout, 'Radar'))


def volatile(widget):
    if not any(previous == widget for previous, _ in state['volatile']):
        state['volatile'].append((widget, bool(widget.get_editor_property('bIsVolatile'))))
        widget.force_volatile(True)


def paints(prefix, first_frame):
    ue.log_flush()
    if not PAINT_LOG.is_file(): return []
    with PAINT_LOG.open('rb') as handle:
        handle.seek(0, 2); handle.seek(max(0, handle.tell() - 262144))
        tail = handle.read(262144).decode('utf-8-sig', errors='replace')
    frame = int(ue.SystemLibrary.get_frame_count())
    values = native_paint_rows(tail, prefix, first_frame, frame)
    for value in values:
        value['read_at_frame'] = frame
    return values


def progress(values):
    return [dict(id=str(row.get_editor_property('MissionId')), text=str(row.get_editor_property('DisplayText')),
        source=str(row.get_editor_property('ProgressSource')), current=int(row.get_editor_property('CurrentCount')),
        target=int(row.get_editor_property('TargetCount')), completed=bool(row.get_editor_property('bCompleted')),
        tracked=bool(row.get_editor_property('bTracked'))) for row in values]


def frame_wait(label, widget):
    current = state.get('paint_wait')
    if not current or current['label'] != label or current['widget'] != widget:
        current = dict(label=label, widget=widget, frame=int(ue.SystemLibrary.get_frame_count()) + 1, at=time.monotonic())
        state['paint_wait'] = current
    assert time.monotonic() - current['at'] < 5, 'Fresh paint unavailable: ' + label
    return current


def mission_paint(context, world, mode, director, label, expected_ids=None):
    widget = context['mission']; volatile(widget)
    if widget.is_mission_transition_active(): return None
    pending = frame_wait(label, widget)
    expected = progress(director.get_mission_progress())
    if progress(widget.get_displayed_missions()) != expected: return None
    if expected_ids is not None: assert [row['id'] for row in expected] == expected_ids, expected
    assert not widget.does_content_overflow(), 'Main HUD list exceeds configured bounds'
    values = [row for row in paints('DCMISSION_HUD_PAINT', pending['frame'])
        if row['world'] == world.get_path_name() and row.get('source') == director.get_path_name()
        and row['read_at_frame'] - row['frame'] <= 16]
    assert {row['widget'] for row in values} <= {widget.get_path_name()}, 'Multiple mission widgets actually painted'
    for row in reversed(values):
        if row.get('transition_state') != 'live' or row.get('phase_index') != director.get_current_phase_index(): continue
        if row.get('serial') != int(widget.get_animation_serial()) or row.get('active_page') != widget.get_active_body_page_index(): continue
        if row['phase'] != str(mode.get_stage_title()): continue
        if [value['id'] for value in row['rows']] != [value['id'] for value in expected]: continue
        if any(any(actual[key] != wanted[key] for key in ('current', 'target', 'completed', 'tracked'))
            for actual, wanted in zip(row['rows'], expected)): continue
        assert not row['overflow'] and row['rows_count'] == len(expected)
        for key in ('root_size', 'panel_position', 'panel_size', 'list_position', 'list_size'):
            assert len(row[key]) == 2 and all(math.isfinite(value) for value in row[key]), (key, row)
        assert min(row['root_size']) > 0
        assert abs(row['panel_size'][0] - 360) < .1 and 132 <= row['panel_size'][1] <= 392.1
        assert abs(row['panel_position'][0] - (row['root_size'][0] - 64 - 360)) < .1
        assert abs(row['panel_position'][1] - 288) < .1
        assert len(row['painted_rows']) == len(expected), 'Rows were clipped or not actually generated'
        # GetDisplayedEntryWidgets is the pool's active array, not a promised
        # visual-order array. Inspect the actual painted Y coordinates instead.
        drawn_rows = sorted(row['painted_rows'], key=lambda value: value['position'][1])
        assert [value['id'] for value in drawn_rows] == [value['id'] for value in expected], 'Pooled stale row/visual order'
        for actual, drawn, wanted in zip(row['rows'], drawn_rows, expected):
            assert wanted['text'] in actual['text'] and drawn['text'] == actual['text'], (actual, drawn, wanted)
            if 'GUIDANCE' in wanted['source'].upper(): assert actual['text'] == wanted['text']
            else: assert actual['text'].startswith(str(wanted['current']) + '/' + str(wanted['target']))
            assert abs(drawn['size'][0] - 328) < .2 and drawn['size'][1] >= 47.9
            assert drawn['position'][1] >= -.2 and drawn['position'][1] + drawn['size'][1] <= row['list_size'][1] + .5
        assert all(second['position'][1] >= first['position'][1] + first['size'][1] - .2
            for first, second in zip(drawn_rows, drawn_rows[1:])), 'Painted mission rows overlap'
        report.setdefault('native_mission_paints', {})[label] = row
        state.pop('paint_wait', None)
        return row
    return None


def phase_context(director):
    index = director.get_current_phase_index()
    phases = list(director.get_editor_property('Phases'))
    assert 0 <= index < len(phases)
    phase = phases[index]
    encounter = phase.get_editor_property('Encounter')
    spawners = []
    for spawner in encounter.get_editor_property('Spawners'):
        if valid(spawner) and spawner not in spawners: spawners.append(spawner)
    assert spawners and not encounter.has_encounter_failure() and not encounter.is_encounter_aborted()
    return index, phase, encounter, spawners


def counts(director):
    index, _, _, spawners = phase_context(director)
    defeated = sum(spawner.get_defeated_count() for spawner in spawners)
    planned = sum(spawner.get_planned_enemy_count() for spawner in spawners)
    row = next(row for row in progress(director.get_mission_progress()) if row['id'] == 'DefeatNightmares')
    assert (row['current'], row['target']) == (defeated, planned)
    assert all(not spawner.has_spawn_failure() for spawner in spawners)
    return dict(phase=index, defeated=defeated, planned=planned,
        spawners=[dict(path=spawner.get_path_name(), defeated=spawner.get_defeated_count(),
            planned=spawner.get_planned_enemy_count(), spawned=spawner.get_spawned_count(), alive=spawner.get_alive_count())
            for spawner in spawners])


def radar_target(context, world, director, expected=True):
    radar = context['radar']; target = director.get_tracked_mission_target_component()
    if not expected:
        assert target is None and radar.get_objective_component() is None
        assert not [row for row in radar.get_radar_markers() if 'OBJECTIVE' in str(row.get_editor_property('Kind')).upper()]
        return None
    assert valid(target) and target.get_world() == world and radar.get_objective_component() == target
    _, phase, _, _ = phase_context(director)
    rows = progress(director.get_mission_progress())
    tracked = next(row for row in rows if row['tracked'])
    definition = next(value for value in director.get_current_mission_list().get_editor_property('Missions')
        if str(value.get_editor_property('MissionId')) == tracked['id'])
    bindings = [value for value in phase.get_editor_property('MissionTargets')
        if value.get_editor_property('TargetKey') == definition.get_editor_property('TargetKey')]
    assert len(bindings) == 1
    actor = bindings[0].get_editor_property('TargetActor')
    assert valid(actor) and actor.get_world() == world and actor.call_method('K2_GetRootComponent') == target
    return target


def projection_paint(context, world, pawn, director, label, clamped):
    radar = context['radar']; target = radar_target(context, world, director)
    pending = frame_wait(label, radar)
    camera = ue.GameplayStatics.get_player_camera_manager(world, 0)
    yaw = camera.get_camera_rotation().yaw
    delta = target.get_world_location() - pawn.get_actor_location()
    limit = 120 - radar.get_editor_property('InnerPadding') - max(radar.get_editor_property(name)
        for name in ('PlayerIconSize', 'EnemyIconSize', 'ObjectiveIconSize')) * math.sqrt(2) / 2
    distance = radar.get_editor_property('RangeMeters') * 100
    x = (-math.sin(math.radians(yaw)) * delta.x + math.cos(math.radians(yaw)) * delta.y) / distance * limit
    y = -(math.cos(math.radians(yaw)) * delta.x + math.sin(math.radians(yaw)) * delta.y) / distance * limit
    radius = math.hypot(x, y)
    if radius > limit: x, y = x * limit / radius, y * limit / radius
    for row in reversed(paints('DCRADAR_PAINT', pending['frame'])):
        if row['world'] != world.get_path_name() or row['widget'] != radar.get_path_name() or row['read_at_frame'] - row['frame'] > 12: continue
        if abs((row['camera_yaw'] - yaw + 180) % 360 - 180) > .25: continue
        markers = [value for value in row['markers'] if value['kind'] == 2]
        if len(markers) != 1: continue
        marker = markers[0]
        if marker['component'] != target.get_path_name() or marker['actor'] != target.get_owner().get_path_name(): continue
        if marker['clamped'] != clamped or max(abs(a - b) for a, b in zip(marker['center'], (120 + x, 120 + y))) > .2: continue
        assert max(abs(value - 240) for value in row['local_size']) < .1
        assert math.hypot(marker['center'][0] - 120, marker['center'][1] - 120) <= limit + .1
        report.setdefault('native_radar_paints', {})[label] = row
        state.pop('paint_wait', None)
        return row
    return None


def clone(value):
    result = type(value)()
    assert result.import_text(value.export_text())
    return result


def use_temporary_list(director, phase_index=0):
    assert 'phase_override' not in state, 'Restore previous transient phase list first'
    phases = [clone(value) for value in director.get_editor_property('Phases')]
    original = phases[phase_index].get_editor_property('MissionList')
    assert original == mission_data and director.get_current_phase_index() == phase_index
    temporary = ue.new_object(CLASSES['DCMissionListData'])
    assert temporary.get_outer().get_path_name() == '/Engine/Transient'
    temporary.set_editor_property('Missions', [clone(value) for value in original.get_editor_property('Missions')])
    temporary.set_editor_property('HUDStyle', clone(original.get_editor_property('HUDStyle')))
    temporary.set_editor_property('DefaultTrackedMissionId', original.get_editor_property('DefaultTrackedMissionId'))
    phases[phase_index].set_editor_property('MissionList', temporary)
    state['phase_override'] = (director, phase_index, original)
    state['temporary_list'] = temporary
    director.set_editor_property('Phases', phases)
    assert director.get_current_mission_list() == temporary
    return temporary


def restore_list():
    restoration = state.pop('phase_override', None)
    if restoration:
        director, phase_index, original = restoration
        if valid(director):
            phases = [clone(value) for value in director.get_editor_property('Phases')]
            phases[phase_index].set_editor_property('MissionList', original)
            director.set_editor_property('Phases', phases)


def death_before(context, world, director):
    widget = context['mission']; volatile(widget)
    style = director.get_current_mission_list().get_editor_property('HUDStyle')
    target = director.get_tracked_mission_target_component()
    return dict(frame=int(ue.SystemLibrary.get_frame_count()), worldtime=ue.GameplayStatics.get_time_seconds(world),
        world=world.get_path_name(), source=director.get_path_name(), widget=widget.get_path_name(),
        phase=director.get_current_phase_index(), old_page=widget.get_active_body_page_index(),
        body_before=widget.get_active_body_page().get_path_name(),
        serial_before=int(widget.get_animation_serial()), outgoing=progress(director.get_mission_progress()),
        objective_before=target.get_path_name() if valid(target) else None,
        hold=float(style.get_editor_property('CompletedHoldDuration')),
        fade=float(style.get_editor_property('MissionTransitionDuration')))


def begin_transition(context, before_death, actual, label):
    widget = context['mission']
    assert widget.is_mission_transition_active() and str(widget.get_body_transition_state()) == 'holding'
    assert widget.get_displayed_phase_index() == before_death['phase']
    assert widget.get_active_body_page_index() == before_death['old_page']
    serial = int(widget.get_animation_serial()); assert serial > before_death['serial_before']
    outgoing = [dict(row) for row in before_death['outgoing']]
    for row in outgoing:
        if 'DEFEAT' in row['source'].upper():
            row.update(current=actual, completed=row['target'] > 0 and actual >= row['target'])
    assert progress(widget.get_displayed_missions()) == outgoing, 'Completed outgoing list differs from actual final encounter count'
    observation = dict(before_death, label=label, first_frame=before_death['frame'] + 1,
        serial=serial, outgoing=outgoing, observed_states=[], at=time.monotonic())
    report['transition_observations'].append(observation)
    state['pending_transition'] = observation
    event('in_panel_completed_snapshot', phase=observation['phase'], serial=serial,
        frame=before_death['frame'], displayed=outgoing, original_asset_unchanged=definition_snapshot(mission_data) == original_definition)
    return observation


def observe_transition(context, world, mode, director):
    observation = state['pending_transition']; widget = context['mission']
    assert time.monotonic() - observation['at'] < 12, 'In-panel transition did not settle'
    assert director.get_path_name() == observation['source'] and world.get_path_name() == observation['world']
    frame = int(ue.SystemLibrary.get_frame_count()); index = director.get_current_phase_index()
    body_state = str(widget.get_body_transition_state())
    sample = dict(frame=frame, worldtime=ue.GameplayStatics.get_time_seconds(world), live_phase=index,
        displayed_phase=widget.get_displayed_phase_index(), page=widget.get_active_body_page_index(),
        active_body=widget.get_active_body_page().get_path_name(),
        serial=int(widget.get_animation_serial()), state=body_state)
    if not observation['observed_states'] or any(sample[key] != observation['observed_states'][-1][key]
            for key in ('live_phase', 'displayed_phase', 'page', 'serial', 'state')):
        observation['observed_states'].append(sample)
    if index <= observation['phase']: return False
    target = radar_target(context, world, director)
    if 'first_next_phase' not in observation:
        observation['first_next_phase'] = dict(sample, counters=counts(director), objective=target.get_path_name())
        event('gameplay_advanced_during_mission_transition', **observation['first_next_phase'])
    if body_state == 'holding':
        assert widget.get_displayed_phase_index() == observation['phase']
        assert progress(widget.get_displayed_missions()) == observation['outgoing']
        observation['next_phase_and_radar_while_holding'] = True
        for painted in paints('DCRADAR_PAINT', observation['first_next_phase']['frame']):
            if painted['world'] == observation['world'] and painted['widget'] == context['radar'].get_path_name() and any(
                    marker.get('component') == target.get_path_name() and marker.get('kind') == 2 for marker in painted['markers']):
                observation['radar_native_during_hold'] = painted
    # Change only a transient next-phase clone, after fade-out starts but before
    # its midpoint. Keep two short rows and the actual TargetKey/radar binding.
    if body_state == 'fade_out' and observation['phase'] == 0 and 'midpoint_edit' not in observation:
        temporary = use_temporary_list(director, index)
        values = [clone(value) for value in temporary.get_editor_property('Missions')]
        for offset, definition in enumerate(values):
            name = str(definition.get_editor_property('MissionId'))
            if name == 'DefeatNightmares': definition.set_editor_property('DisplayText', ue.Text('전환 최신 처치 목표'))
            elif name == 'ReachPlaza':
                replacement = ue.DCMissionDefinition(); assert replacement.import_text('(ProgressSource=External)')
                for key in ('MissionId', 'TargetKey', 'bEnabled'): replacement.set_editor_property(key, definition.get_editor_property(key))
                replacement.set_editor_property('DisplayText', ue.Text('전환 최신 안내 목표'))
                replacement.set_editor_property('TargetCount', 3); values[offset] = replacement
        temporary.set_editor_property('Missions', values)
        assert director.update_external_mission_progress(ue.Name('ReachPlaza'), 2)
        observation['midpoint_edit'] = dict(frame=frame, phase=index, worldtime=sample['worldtime'],
            expected=progress(director.get_mission_progress()), transient_only=True)
        assert definition_snapshot(mission_data) == original_definition
        event('fade_out_transient_latest_data_edit', **observation['midpoint_edit'])
    if body_state != 'live' or widget.is_mission_transition_active() or widget.get_displayed_phase_index() != index: return False
    painted = mission_paint(context, world, mode, director, observation['label'] + '_incoming_live')
    if not painted: return False
    observation.update(last_frame=int(ue.SystemLibrary.get_frame_count()), incoming_phase=index,
        incoming_page=widget.get_active_body_page_index(), incoming=progress(director.get_mission_progress()))
    if observation.get('midpoint_edit'):
        assert observation['incoming'] == observation['midpoint_edit']['expected'], 'Midpoint clone changed before incoming live verification'
    assert int(widget.get_animation_serial()) == observation['serial']
    event('in_panel_transition_settled', phase=index, frame=observation['last_frame'],
        serial=observation['serial'], incoming=observation['incoming'])
    restore_list(); state.pop('pending_transition', None)
    return True


def own_immunity(pawn):
    previous = state.get('immune_pawn')
    if valid(previous):
        if previous == pawn: return
        gas.call_method('RemoveLooseGameplayTags', args=(previous, immunity, False))
    state.pop('immune_pawn', None)
    assert gas.call_method('AddLooseGameplayTags', args=(pawn, immunity, False))
    state['immune_pawn'] = pawn


def remove_immunity():
    pawn = state.pop('immune_pawn', None)
    if valid(pawn): gas.call_method('RemoveLooseGameplayTags', args=(pawn, immunity, False))


def damage(source, target):
    assert valid(source) and valid(target) and source.get_world() == target.get_world()
    source_asc = gas.call_method('GetAbilitySystemComponent', args=(source,))
    target_asc = gas.call_method('GetAbilitySystemComponent', args=(target,))
    assert source_asc and target_asc
    spec = source_asc.make_outgoing_spec(DAMAGE, 1, source_asc.make_effect_context())
    spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, damage_tag, 10000.0))
    source_asc.apply_gameplay_effect_spec_to_target(spec, target_asc)


def living_enemies(world, pawn):
    result = []
    for actor in ue.GameplayStatics.get_all_actors_of_class(world, ue.Pawn):
        if actor == pawn: continue
        health = actor.get_component_by_class(CLASSES['DCLyraHealthComponent'])
        if health and health.get_max_health() > 0 and not health.is_dead_or_dying() and health.get_health() > 0:
            result.append(actor)
    return result


def hidden(pc, world, label):
    context = active_hud(pc, world, False)
    if context:
        assert not context['mission'].get_displayed_missions()
        assert not context['mission'].is_mission_transition_active()
        assert child(context['mission'], 'MissionPanel').get_visibility() == ue.SlateVisibility.COLLAPSED
    layouts = widgets(pc, world, HUD, True)
    for layout in layouts:
        radar = child(layout, 'Radar')
        assert radar.get_objective_component() is None and not radar.get_radar_markers()
    waiting = state.get('hidden_wait')
    if not waiting or waiting['label'] != label:
        waiting = dict(label=label, frame=int(ue.SystemLibrary.get_frame_count()) + 1, at=time.monotonic())
        state['hidden_wait'] = waiting
    assert not paints('DCMISSION_HUD_PAINT', waiting['frame']), 'Mission HUD painted after terminal/menu state'
    assert not paints('DCMISSION_ROW_PAINT', waiting['frame']), 'Mission row actually painted after terminal/menu state'
    assert not paints('DCRADAR_PAINT', waiting['frame']), 'Radar painted after terminal/menu state'
    frame = int(ue.SystemLibrary.get_frame_count())
    if frame - waiting['frame'] < 24 or time.monotonic() - waiting['at'] < .2: return False
    retained = []
    for widget in state['seen_missions']:
        if valid(widget):
            retained.append(dict(path=widget.get_path_name(), displayed_rows=len(widget.get_displayed_missions()),
                transition_active=widget.is_mission_transition_active(), serial=int(widget.get_animation_serial()),
                registered_on_active_hud=bool(context and widget == context['mission'])))
    report.setdefault('hidden_observations', {})[label] = dict(first_frame=waiting['frame'], last_frame=frame,
        active_gameplay_huds=len(layouts), retained_mission_objects=retained,
        new_mission_paints=0, new_radar_paints=0)
    cancelled = state.get('cancel_transition')
    if cancelled and label == 'death_result': cancelled['cancel_check_last_frame'] = frame
    state.pop('hidden_wait', None)
    return True


def restore_mutations():
    for callback in reversed(state['mutations']):
        try: callback()
        except Exception: report['failures'].append(traceback.format_exc())
    state['mutations'].clear()


def restore_transform(actor, transform):
    if valid(actor):
        actor.set_actor_transform(transform, False, True)
        assert actor.get_actor_transform().export_text() == transform.export_text(), 'Temporary goal transform was not restored'


def restore_mobility(component, mobility):
    if valid(component):
        component.set_mobility(mobility)
        assert component.get_editor_property('Mobility') == mobility


def finish(ok):
    if state['done']: return
    state['done'] = True
    for callback in (restore_list, restore_mutations, remove_immunity):
        try: callback()
        except Exception: report['failures'].append(traceback.format_exc())
    for widget, previous in state['volatile']:
        try:
            if valid(widget): widget.force_volatile(previous)
        except Exception: report['failures'].append(traceback.format_exc())
    report['definition_asset_memory_unchanged'] = definition_snapshot(mission_data) == original_definition
    report['boss_combat_data_unchanged'] = all(boss_data.get_editor_property(name) == value for name, value in boss_baseline.items())
    after = inventory()
    report['protected_metadata_changes'] = [name for name, value in before['inventory'].items() if after.get(name) != value]
    report['new_protected_files'] = sorted(set(after) - set(before['inventory']))
    after_hashes = protected_hashes()
    report['protected_hash_changes'] = [name for name, value in before['hashes'].items() if after_hashes.get(name) != value]
    report['screenshots'] = []
    for path in DIRECTORY.glob('mission_*.png'):
        header = path.read_bytes()[:24]
        if len(header) == 24 and header[:8] == b'\x89PNG\r\n\x1a\n':
            report['screenshots'].append(dict(path=str(path), size=list(struct.unpack('>II', header[16:24]))))
    report['screenshots_are_ui_evidence'] = False
    report['screenshot_limit'] = 'SHOWUI can capture an offscreen black/grid image; PNG existence/dimensions do not prove HUD or fade rendering'
    ue.log_flush()
    paint_log = PAINT_LOG.read_text(encoding='utf-8-sig', errors='replace') if PAINT_LOG.is_file() else ''
    last_frame = int(ue.SystemLibrary.get_frame_count())
    all_rows = native_paint_rows(paint_log, 'DCMISSION_ROW_PAINT', 0, last_frame)
    all_hud = native_paint_rows(paint_log, 'DCMISSION_HUD_PAINT', 0, last_frame)
    evidence = []
    for observation in report['transition_observations']:
        if observation.get('cancelled'):
            after_cancel = [row for row in all_rows if observation['cancel_frame'] < row['frame'] <= observation.get('cancel_check_last_frame', observation['cancel_frame'])
                and row.get('world') == observation['world'] and row.get('source') == observation['source']]
            cancel_status = 'failed' if after_cancel else 'cancelled_intentionally' if observation['transition_at_death']['active'] else 'sampling_limit'
            evidence.append(dict(label=observation['label'], status=cancel_status,
                actual_transition_at_death=observation['transition_at_death'], actual_paints_after_cancel=after_cancel,
                cancellation_window=[observation['cancel_frame'] + 1, observation.get('cancel_check_last_frame')]))
        elif 'last_frame' in observation:
            evidence.append(dict(label=observation['label'], **transition_evidence(observation, all_rows, all_hud)))
        else:
            evidence.append(dict(label=observation['label'], status='sampling_limit', missing_samples=['flow_ended_before_incoming_live']))
    normal = [value for value in evidence if value['status'] != 'cancelled_intentionally']
    status = 'failed' if any(value['status'] == 'failed' for value in evidence) else (
        'passed' if len(normal) >= 2 and all(value['status'] == 'passed' for value in normal) else 'sampling_limit')
    report['fade_visual_verification'] = dict(status=status, transitions=evidence,
        scope='Actual row text/counters/inherited alpha, one body page per frame and sampled native panel bounds; header alpha is not separately instrumented',
        requested_resolution_verified=report['resolution_verified'])
    report['fade_visual_verified'] = status == 'passed'
    report['feature_flow_passed'] = bool(ok and not report['failures'])
    report['probe_passed'] = bool(report['feature_flow_passed'] and report['resolution_verified']
        and report['definition_asset_memory_unchanged'] and report['boss_combat_data_unchanged']
        and not report['protected_metadata_changes'] and not report['new_protected_files'] and not report['protected_hash_changes'])
    report['final_verification_pending'] = True
    event('finished', feature_flow_passed=report['feature_flow_passed'], probe_passed=report['probe_passed'],
        resolution_verified=report['resolution_verified'], fade_visual_verification=status, final_verification_pending=True)
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not report['feature_flow_passed']: ue.log_error('Mission HUD feature PIE verification failed: ' + str(OUT))


def tick(delta):
    if state['done'] or state['busy']: return
    state['busy'] = True
    try:
        assert time.monotonic() - state['started'] < 300, 'Mission HUD probe timeout: ' + state['step']
        world = editor.get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name(): return
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc: return
        level = ue.GameplayStatics.get_current_level_name(world, True)
        now = ue.GameplayStatics.get_time_seconds(world)
        step = state['step']
        if step in ('menu_initial', 'menu_return', 'menu_final'):
            if level != 'L_DC_FrontEnd' or now < 2 or not widgets(pc, world, FRONTEND, True): return
            assert not pc.get_controlled_pawn() and not widgets(pc, world, HUD, True)
            if not hidden(pc, world, step): return
            if step == 'menu_final': finish(True); return
            click(child(one(pc, world, FRONTEND, True), 'StoryButton'))
            state.update(step='initial' if step == 'menu_initial' else 'reentry')
            event(step, mission_hud_visible=False, radar_objective=False)
            return
        if level != 'Level_1': return
        mode = ue.GameplayStatics.get_game_mode(world)
        pawn = pc.get_controlled_pawn()
        if step in ('clear_result', 'death_result'):
            if now - state['at'] < .4 or not widgets(pc, world, CLASSES['DCStageResultLayer'], True): return
            assert mode.has_run_ended()
            if not hidden(pc, world, step): return
            result = one(pc, world, CLASSES['DCStageResultLayer'], True)
            if step == 'clear_result':
                assert mode.was_run_successful()
                assert state['visited_phases'] == {0, 1, 2}
                click(child(result, 'RetryButton')); assert mode.is_stage_travel_pending()
                event('actual_stage_success_and_retry', phases=sorted(state['visited_phases']))
                state.update(step='restart', travel_at=now)
            else:
                assert state['death_seen'] and not mode.was_run_successful()
                click(child(result, 'ExitButton')); assert mode.is_stage_travel_pending()
                event('actual_player_death_and_result_return')
                state.update(step='menu_return')
            return
        if step == 'restart' and now >= state['travel_at'] - .5: return
        if not pawn or now < 2 or not widgets(pc, world, HUD, True): return
        health = pawn.get_component_by_class(CLASSES['DCLyraHealthComponent'])
        if not health or health.get_max_health() <= 0 or health.is_dead_or_dying(): return
        own_immunity(pawn)
        context = active_hud(pc, world)
        director = mode.get_stage_director()
        assert valid(director) and not mode.has_run_ended()
        if step in ('initial', 'restart', 'restart_clear', 'reentry'):
            if director.get_current_phase_index() != 0: return
            observation = counts(director)
            assert (observation['defeated'] == 0 or step == 'restart_clear') and observation['planned'] > 0
            if not context['radar'].is_radar_active() or not context['radar'].get_objective_component(): return
            target = radar_target(context, world, director)
            painted = mission_paint(context, world, mode, director, step, ['DefeatNightmares', 'ReachPlaza'])
            if not painted: return
            event(step, counters=observation, target=target.get_path_name(), fresh_runtime_rows=True)
            if step in ('restart', 'restart_clear'):
                enemies = living_enemies(world, pawn)
                if not enemies: return
                before_death = death_before(context, world, director)
                damage(pawn, enemies[0])
                _, _, _, spawners = phase_context(director)
                actual = sum(value.get_defeated_count() for value in spawners)
                assert actual == observation['defeated'] + 1
                if actual >= observation['planned']:
                    begin_transition(context, before_death, actual, 'retry_death_cancel')
                    state.update(step='restart_transition_death', at=now)
                else: state.update(step='restart_clear', at=now)
                return
            if step == 'reentry':
                player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(player, menu_tag, MENU))
                state.update(step='esc_final', at=now)
                return
            # Actual viewport size, plus independent actual native radar scale.
            radar_paints = [row for row in paints('DCRADAR_PAINT', painted['frame'] - 16)
                if row['world'] == world.get_path_name() and row['widget'] == context['radar'].get_path_name()]
            if not radar_paints: return
            native_radar = radar_paints[-1]
            actual_size = list(pc.get_viewport_size())
            scale = [a / b for a, b in zip(native_radar['pixel_size'], native_radar['local_size'])]
            assert abs(scale[0] - scale[1]) < .001 and min(scale) > 0
            physical_root = [value * scale[0] for value in painted['root_size']]
            assert max(abs(a - b) for a, b in zip(physical_root, actual_size)) < 2, (physical_root, actual_size)
            report['resolution_verified'] = actual_size == EXPECTED_SIZE
            report['resolution_observation'] = dict(expected=EXPECTED_SIZE, actual=actual_size,
                native_root=painted['root_size'], actual_scale=scale, physical_root=physical_root,
                matches=report['resolution_verified'],
                limitation=None if report['resolution_verified'] else 'Embedded Project.Maps.PIE viewport did not match requested size; no resolution success is claimed')
            ue.SystemLibrary.execute_console_command(world,
                'Shot SHOWUI nosuffix filename="' + (DIRECTORY / ('mission_' + size_match.group(1) + '.png')).as_posix() + '"', pc)
            state.update(step='screenshot_wait', at=now)
            return
        if step == 'screenshot_wait':
            if now - state['at'] < .5: return
            state['ordinary_serial'] = int(context['mission'].get_animation_serial())
            temporary = use_temporary_list(director)
            added = ue.DCMissionDefinition()
            assert added.import_text('(ProgressSource=External)')
            for name, value in dict(MissionId=ue.Name('ProbeCollect'), DisplayText=ue.Text('단서 수집하기'),
                TargetCount=3, bEnabled=True).items(): added.set_editor_property(name, value)
            values = [clone(value) for value in temporary.get_editor_property('Missions')]
            temporary.set_editor_property('Missions', values + [added])
            state.update(step='edit_added', at=now)
            return
        if step.startswith('edit_'):
            assert director.get_current_phase_index() == 0, 'Mission edits advanced gameplay'
            assert not context['mission'].is_mission_transition_active()
            assert int(context['mission'].get_animation_serial()) == state['ordinary_serial'], 'Ordinary same-phase edits started an animation'
            temporary = state['temporary_list']
            if step == 'edit_added':
                if not mission_paint(context, world, mode, director, step, ['DefeatNightmares', 'ReachPlaza', 'ProbeCollect']): return
                values = [clone(value) for value in temporary.get_editor_property('Missions')]
                values[2].set_editor_property('DisplayText', ue.Text('수정한 단서 수집 목표'))
                temporary.set_editor_property('Missions', [values[2], values[1], values[0]])
                assert director.update_external_mission_progress(ue.Name('ProbeCollect'), 2)
                state.update(step='edit_reordered', at=now)
            elif step == 'edit_reordered':
                if not mission_paint(context, world, mode, director, step, ['ProbeCollect', 'ReachPlaza', 'DefeatNightmares']): return
                row = progress(director.get_mission_progress())[0]
                assert row['current'] == 2 and row['target'] == 3 and not row['completed'] and row['text'] == '수정한 단서 수집 목표'
                assert director.set_external_mission_completed(ue.Name('ProbeCollect'), True)
                state.update(step='edit_completed', at=now)
            elif step == 'edit_completed':
                if not mission_paint(context, world, mode, director, step, ['ProbeCollect', 'ReachPlaza', 'DefeatNightmares']): return
                assert progress(director.get_mission_progress())[0]['completed']
                temporary.set_editor_property('Missions', [clone(value) for value in temporary.get_editor_property('Missions')
                    if str(value.get_editor_property('MissionId')) == 'DefeatNightmares'])
                state.update(step='edit_deleted', at=now)
            elif step == 'edit_deleted':
                if not mission_paint(context, world, mode, director, step, ['DefeatNightmares']): return
                radar_target(context, world, director, False)
                temporary.set_editor_property('Missions', [clone(value) for value in mission_data.get_editor_property('Missions')])
                assert director.set_tracked_mission(ue.Name('ReachPlaza'))
                state.update(step='edit_restored', at=now)
            elif step == 'edit_restored':
                if not mission_paint(context, world, mode, director, step, ['DefeatNightmares', 'ReachPlaza']): return
                radar_target(context, world, director)
                assert definition_snapshot(mission_data) == original_definition
                restore_list()
                assert director.get_current_mission_list() == mission_data
                assert director.set_tracked_mission(ue.Name('ReachPlaza'))
                target = radar_target(context, world, director)
                actor = target.get_owner(); location = actor.get_actor_transform()
                # Reverse cleanup restores transform while the component is
                # still movable. SetMobility(Movable) also changes descendants
                # (SceneComponent.cpp:3105); restore every original child value.
                for component in [target] + list(target.get_children_components(True)):
                    mobility = component.get_editor_property('Mobility')
                    state['mutations'].append(lambda component=component, mobility=mobility: restore_mobility(component, mobility))
                state['mutations'].append(lambda actor=actor, location=location: restore_transform(actor, location))
                rotation = pc.get_control_rotation()
                state['mutations'].append(lambda pc=pc, rotation=rotation: pc.set_control_rotation(rotation) if valid(pc) else None)
                target.set_mobility(ue.ComponentMobility.MOVABLE)
                actor.set_actor_location(pawn.get_actor_location() + ue.Vector(2500, 0, 0), False, True)
                state.update(step='projection_near', at=now, target_actor=actor, target_component=target, control_rotation=rotation)
                event('editable_list_add_modify_reorder_complete_delete_restore', original_asset_unchanged=True,
                    completion_did_not_advance_phase=True, ordinary_updates_did_not_animate=True,
                    animation_serial=state['ordinary_serial'], deleted_tracked_mission_cleared_radar=True)
            return
        if step == 'transition_observe':
            if observe_transition(context, world, mode, director): state.update(step='clear_fights', at=now)
            return
        if step == 'restart_transition_death':
            observation = state['pending_transition']
            assert time.monotonic() - observation['at'] < 12
            if director.get_current_phase_index() <= observation['phase']: return
            radar_target(context, world, director)
            enemies = living_enemies(world, pawn)
            if not enemies: return
            observation['transition_at_death'] = dict(active=context['mission'].is_mission_transition_active(),
                state=str(context['mission'].get_body_transition_state()),
                displayed_phase=context['mission'].get_displayed_phase_index(), live_phase=director.get_current_phase_index(),
                serial=int(context['mission'].get_animation_serial()))
            observation.update(cancelled=True, cancel_frame=int(ue.SystemLibrary.get_frame_count()))
            state['cancel_transition'] = observation; state.pop('pending_transition', None)
            remove_immunity(); damage(enemies[0], pawn)
            state['death_seen'] = health.is_dead_or_dying(); assert state['death_seen']
            event('actual_transition_cancelled_by_player_death', **observation['transition_at_death'],
                frame=observation['cancel_frame'])
            state.update(step='death_result', at=now)
            return
        if step in ('projection_near', 'projection_rotated', 'projection_far', 'arrival'):
            assert director.get_current_phase_index() == 0
            if now - state['at'] < .4: return
            projected = projection_paint(context, world, pawn, director, step, step == 'projection_far')
            if not projected: return
            if step == 'projection_near':
                state['initial_camera_yaw'] = projected['camera_yaw']
                rotation = state['control_rotation']
                pc.set_control_rotation(ue.Rotator(pitch=rotation.pitch, yaw=rotation.yaw + 90, roll=rotation.roll))
                state.update(step='projection_rotated', at=now)
            elif step == 'projection_rotated':
                turn = abs((projected['camera_yaw'] - state['initial_camera_yaw'] + 180) % 360 - 180)
                assert 75 <= turn <= 105, ('Actual camera did not turn with the controller', turn)
                state['target_actor'].set_actor_location(pawn.get_actor_location() + ue.Vector(100000, 100000, 0), False, True)
                state.update(step='projection_far', at=now)
            elif step == 'projection_far':
                state['target_actor'].set_actor_location(pawn.get_actor_location(), False, True)
                state.update(step='arrival', at=now)
            else:
                assert director.get_current_phase_index() == 0, 'Goal/player co-location became an arrival gate'
                restore_mutations()
                state.update(step='clear_fights', at=now)
                event('actual_bound_target_projection_and_no_arrival_gate', player_goal_colocation_fixture=True,
                    camera_rotation=True, beyond_50m_clamped=True)
            return
        if step == 'clear_fights':
            index = director.get_current_phase_index()
            observation = counts(director)
            if index not in state['visited_phases']:
                if not mission_paint(context, world, mode, director, 'phase_' + str(index), ['DefeatNightmares', 'ReachPlaza']): return
                radar_target(context, world, director)
                assert observation['defeated'] == 0
                state['visited_phases'].add(index)
                event('actual_encounter_mission_initial', counters=observation)
            enemies = living_enemies(world, pawn)
            if not enemies: return
            target = next((actor for actor in enemies if actor.get_path_name() not in state['killed']), None)
            if not target: return
            if index == 2:
                assert math_library.call_method('ClassIsChildOf', args=(target.get_class(), CLASSES['DCBossCharacter']))
                boss_phase = target.get_component_by_class(CLASSES['DCBossPhaseComponent'])
                assert boss_phase
                if 'FIGHTING' not in str(boss_phase.get_boss_run_state()).upper(): return
                volatile(context['boss'])
                pending = frame_wait('boss_regression', context['boss'])
                samples = [value for value in paints('DCBOSS_HUD_PAINT', pending['frame'])
                    if value['world'] == world.get_path_name() and value['widget'] == context['boss'].get_path_name()
                    and value['read_at_frame'] - value['frame'] <= 16]
                if not samples: return
                value = samples[-1]
                assert value['name'] == '춘식이' and value['subtitle'] == '나스닥의 수호자'
                for key, expected in (('panel_size', [720, 86]), ('bar_size', [640, 18]), ('bar_position', [40, 62])):
                    assert max(abs(a - b) for a, b in zip(value[key], expected)) < .1
                assert abs(value['panel_position'][1] - 48) < .1
                assert abs(value['panel_position'][0] - (value['root_size'][0] - 720) / 2) < .1
                report['boss_hud_regression'] = value; state.pop('paint_wait', None)
            _, _, _, spawners = phase_context(director)
            previous = sum(value.get_defeated_count() for value in spawners)
            before_death = death_before(context, world, director)
            state['killed'].add(target.get_path_name())
            damage(pawn, target)
            assert target.get_component_by_class(CLASSES['DCLyraHealthComponent']).is_dead_or_dying()
            actual = sum(value.get_defeated_count() for value in spawners)
            assert actual == previous + 1, 'Actual death did not increment its owning encounter counter once'
            event('actual_spawned_enemy_death', phase=index, target=target.get_path_name(), defeated_before=previous,
                defeated_after=actual, planned=observation['planned'], frame=before_death['frame'],
                source=before_death['source'], serial_before=before_death['serial_before'], old_page=before_death['old_page'])
            if index < 2 and actual >= observation['planned']:
                begin_transition(context, before_death, actual, 'phase_' + str(index))
                state.update(step='transition_observe', at=now)
            if index == 2 and actual >= observation['planned']: state.update(step='clear_result', at=now)
            return
        if step == 'esc_final':
            if now - state['at'] < .4 or not widgets(pc, world, MENU, True): return
            click(child(one(pc, world, MENU, True), 'ReturnButton'))
            state.update(step='confirm_final', at=now)
            return
        if step == 'confirm_final':
            if now - state['at'] < .4 or not widgets(pc, world, DIALOG, True): return
            buttons = list(child(one(pc, world, DIALOG, True), 'EntryBox_Buttons').get_all_entries())
            assert len(buttons) == 2
            click(buttons[0]); assert mode.is_stage_travel_pending()
            state.update(step='menu_final'); event('original_menu_return_after_fresh_reentry')
            return
    except Exception:
        report['failures'].append(traceback.format_exc()); finish(False)
    finally:
        state['busy'] = False


state['callback'] = ue.register_slate_post_tick_callback(tick)
event('armed', expected_resolution=EXPECTED_SIZE)
