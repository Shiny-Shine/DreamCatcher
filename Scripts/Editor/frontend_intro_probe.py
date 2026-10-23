"""Bounded PIE PV observations after apply/reload; never saves assets or builds.

Arm before Project.Maps.PIE on L_DC_FrontEnd, -DCFrontendIntroProbe=<id>.
Optional -DCFrontendIntroFailure checks an invalid media source in memory.
Normal mode seeks near the actual clip end, observes the loop, dismisses via the
widget API, reloads the menu map, and observes fresh playback. Physical input and
audio/visual acceptance remain user checks; native tests cover key-up retention.
Finalize only after process exit:
python frontend_intro_probe.py --finalize REPORT LOG AUTOMATION_INDEX EXIT_CODE
"""
import hashlib
import json
from pathlib import Path
import re
import sys
import time
import traceback


def finalize():
    report_path, log_path, index_path, exit_code = sys.argv[2:]
    report = json.loads(Path(report_path).read_text(encoding='utf-8'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    report['engine_errors'] = [x for x in log.splitlines() if ': Error:' in x or 'Fatal:' in x or 'Ensure condition failed' in x]
    report['automation_states'] = [x.get('state') for x in index.get('tests', [])]
    report['process_exit_code'] = int(exit_code)
    report['complete'] = bool(report.get('observations_complete') and not report['engine_errors'] and int(exit_code) == 0
                              and report['automation_states'] and all(x in ('Success', 'SuccessWithWarnings') for x in report['automation_states']))
    Path(report_path).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not report['complete']:
        raise SystemExit('PV observation/process/Automation checks failed; inspect report')
    print('PV report finalized successfully')


if __name__ == '__main__' and '--finalize' in sys.argv:
    finalize()
    raise SystemExit(0)

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/FrontendPV20261024'
MENU = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'
match = re.search(r'-DCFrontendIntroProbe=([A-Za-z0-9_]+)\b', ue.SystemLibrary.get_command_line())
assert match, 'Explicit probe run id required'
FAILURE = '-dcfrontendintrofailure' in ue.SystemLibrary.get_command_line().lower()
OUT = REPORTS / ('pie_' + match.group(1) + '.json')
assert not OUT.exists(), 'Preserve existing probe evidence'
intro_class = ue.load_class(None, '/Game/DreamCatcher/UI/Frontend/W_DC_FrontendIntro.W_DC_FrontendIntro_C')
menu_class = ue.load_class(None, '/Game/DreamCatcher/UI/Frontend/W_DC_Frontend.W_DC_Frontend_C')
assert intro_class and menu_class
cdo = ue.get_default_object(intro_class)
original_source = cdo.get_editor_property('IntroMediaSource')
assert original_source
if FAILURE:
    invalid = ue.FileMediaSource()
    invalid.set_file_path(str(PROJECT / 'Content/Movies/__missing_frontend_intro_probe__.mp4'))
    assert not invalid.validate()
    cdo.set_editor_property('IntroMediaSource', invalid)

protected = [PROJECT / p for p in ('Config/DefaultGame.ini', 'Config/DefaultEngine.ini', '.git/HEAD', '.git/index',
    'Binaries/Win64/UnrealEditor-DreamCatcher.dll', 'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
    'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_2.umap', 'Content/DreamCatcher/Screens/DREAMCATCHER_PV.uasset',
    'Content/DreamCatcher/UI/Frontend/W_DC_FrontendIntro.uasset',
    'Content/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent.uasset')]
before = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in protected}
state = dict(step='opening', started=time.monotonic(), step_started=time.monotonic(), busy=False, done=False)
result = dict(complete=False, observations_complete=False, mode='failure' if FAILURE else 'loop_reentry',
              events=[], failures=[], asset_saves=0, physical_input_verified=False, visual_acceptance=False,
              audio_acceptance=False, skip_uses_widget_api=True, loop_uses_near_end_seek=True)


def event(name, **data):
    result['events'].append(dict(event=name, elapsed=time.monotonic() - state['started'], **data))


def active(pc, cls):
    return [w for w in ue.ObjectIterator(ue.CommonActivatableWidget)
            if w.get_class() == cls and w.get_owning_player() == pc and w.is_activated()]


def step(name):
    state['step'] = name
    state['step_started'] = time.monotonic()


def finish(ok):
    if state['done']:
        return
    state['done'] = True
    cdo.set_editor_property('IntroMediaSource', original_source)
    result['protected_changes'] = [p for p, digest in before.items() if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest() != digest]
    result['observations_complete'] = ok and not result['failures'] and not result['protected_changes']
    result['cdo_source_restored'] = cdo.get_editor_property('IntroMediaSource') == original_source
    OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(handle)
    if not result['observations_complete']:
        ue.log_error('DC_FRONTEND_INTRO_PROBE_FAILED ' + str(result['failures']))
    else:
        ue.log('DC_FRONTEND_INTRO_OBSERVATIONS_COMPLETE ' + str(OUT))


def tick(_delta):
    if state['done'] or state['busy']:
        return
    state['busy'] = True
    try:
        worlds = ue.EditorLevelLibrary.get_pie_worlds(False)
        if not worlds:
            assert time.monotonic() - state['started'] < 50, 'PIE did not start'
            return
        assert len(worlds) == 1
        world = worlds[0]
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc:
            return
        assert time.monotonic() - state['step_started'] < 50, 'Timeout at ' + state['step']
        intros, menus = active(pc, intro_class), active(pc, menu_class)
        if FAILURE:
            if menus:
                assert len(menus) == 1 and not intros and not pc.get_controlled_pawn()
                event('invalid_media_continued_to_modes')
                finish(True)
            return
        if state['step'] in ('opening', 'reentering'):
            if not intros:
                assert not menus, 'Menu appeared instead of PV'
                return
            assert len(intros) == 1 and not menus
            intro = intros[0]
            player = intro.get_intro_player()
            if not player or not intro.is_intro_playing():
                return
            seconds = ue.MathLibrary.get_total_seconds(player.get_time())
            duration = ue.MathLibrary.get_total_seconds(player.get_duration())
            if seconds <= .1 or duration <= 0:
                return
            assert player.is_looping() and not player.has_error() and not pc.get_controlled_pawn()
            if state['step'] == 'reentering':
                assert repr(world) != state['prior_world'], 'The frontend world did not reload'
                assert seconds < min(duration, 5), 'Reentry did not start a fresh presentation'
                event('frontend_map_reentry_replayed_pv', seconds=seconds)
                intro.skip_intro()
                step('second_menu')
                return
            assert duration > 1, 'Clip must be longer than one second for the loop observation'
            event('initial_pv_playing', seconds=seconds, duration=duration)
            near_end = ue.MathLibrary.from_seconds(duration - .5)
            assert player.seek(near_end), 'Actual media backend rejected the bounded near-end seek'
            state['duration'] = duration
            state['saw_near_end'] = False
            step('looping')
        elif state['step'] == 'looping':
            assert len(intros) == 1 and not menus
            player = intros[0].get_intro_player()
            assert player and player.is_looping() and not player.has_error()
            seconds = ue.MathLibrary.get_total_seconds(player.get_time())
            if seconds >= state['duration'] - .8:
                state['saw_near_end'] = True
            if state['saw_near_end'] and .05 < seconds < min(2, state['duration'] / 2):
                event('actual_media_loop_observed', seconds=seconds)
                intros[0].skip_intro()
                assert not intros[0].get_intro_player()
                step('first_menu')
        elif state['step'] == 'first_menu':
            if menus:
                assert len(menus) == 1 and not intros
                assert not ue.GameplayStatics.get_game_mode(world).is_story_travel_pending()
                event('skip_revealed_modes')
                state['prior_world'] = repr(world)
                step('reentering')
                ue.GameplayStatics.open_level(world, MENU)
        elif state['step'] == 'second_menu':
            if menus:
                assert len(menus) == 1 and not intros
                event('second_skip_revealed_modes')
                finish(True)
    except Exception:
        result['failures'].append(traceback.format_exc())
        finish(False)
    finally:
        state['busy'] = False


handle = ue.register_slate_post_tick_callback(tick)
