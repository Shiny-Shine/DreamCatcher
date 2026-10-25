"""After apply/reload, observe menu BGM gating in a separate real PIE session.

Arm before Project.Maps.PIE on L_DC_FrontEnd, -DCFrontendMusicProbe=<id>, audio
enabled (no -nosound). Optional -DCFrontendMusicFailure uses a transient invalid
PV source. Checks component playback/ownership, not human audio acceptance.
Skip/settings actions use widget APIs, not physical key input. No asset saves.
After process exit: --finalize REPORT LOG AUTOMATION_INDEX EXIT_CODE.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import traceback


def finalize():
    report_path, log_path, index_path, exit_code = sys.argv[2:]
    report = json.loads(Path(report_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    report['engine_errors'] = [line for line in log.splitlines() if ': Error:' in line or 'Fatal:' in line or 'Ensure condition failed' in line]
    report['automation_states'] = [t.get('state') for t in index.get('tests', [])]
    report['process_exit_code'] = int(exit_code)
    report['complete'] = bool(report.get('observations_complete') and not report['engine_errors']
                              and int(exit_code) == 0 and report['automation_states']
                              and all(s in ('Success', 'SuccessWithWarnings') for s in report['automation_states']))
    Path(report_path).write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not report['complete']:
        raise SystemExit('Frontend music observations/process/Automation failed; inspect report')
    print('Frontend music report finalized')


if __name__ == '__main__' and '--finalize' in sys.argv:
    finalize()
    raise SystemExit(0)

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/FrontendMusic20261025'
MENU = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'
command = ue.SystemLibrary.get_command_line()
match = re.search(r'-DCFrontendMusicProbe=([A-Za-z0-9_]+)\b', command)
assert match and '-unattended' in command.lower()
assert '-nosound' not in command.lower(), 'Audio must be enabled for the BGM playing-state check'
FAILURE = '-dcfrontendmusicfailure' in command.lower()
OUT = REPORTS / ('pie_' + match.group(1) + '.json')
assert not OUT.exists()


def bp(path):
    return ue.load_class(None, path + '.' + path.rsplit('/', 1)[1] + '_C')


intro_cls = bp('/Game/DreamCatcher/UI/Frontend/W_DC_FrontendIntro')
menu_cls = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
settings_cls = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Settings/W_LyraSettingScreen')
component_cls = ue.load_class(None, '/Script/DreamCatcher.DCLyraFrontendStateComponent')
cue = ue.load_asset('/Game/DreamCatcher/Audio/선택화면_Cue')
assert intro_cls and menu_cls and settings_cls and component_cls and cue
cdo = ue.get_default_object(intro_cls)
original_source = cdo.get_editor_property('IntroMediaSource')
if FAILURE:
    invalid = ue.FileMediaSource()
    invalid.set_file_path(str(PROJECT / 'Content/Movies/__missing_music_gate_probe__.mp4'))
    assert not invalid.validate()
    cdo.set_editor_property('IntroMediaSource', invalid)


def staged_entries():
    value = subprocess.run(['git', 'ls-files', '--stage', '-z'], cwd=PROJECT,
                           env=dict(os.environ, GIT_OPTIONAL_LOCKS='0'), capture_output=True, check=True).stdout
    return hashlib.sha256(value).hexdigest()


protected_paths = ['.git/HEAD', 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
    'Config/DefaultGame.ini', 'Config/DefaultEngine.ini',
    'Source/DreamCatcher/UI/Frontend/DCFrontendIntroScreen.cpp',
    'Content/DreamCatcher/Maps/L_DC_FrontEnd.umap',
    'Content/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent.uasset',
    'Content/DreamCatcher/Audio/선택화면_Cue.uasset', 'Content/DreamCatcher/Audio/선택화면.uasset',
    'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
    'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_2.umap',
    'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_3.umap']
before = {p: hashlib.sha256((PROJECT / p).read_bytes()).hexdigest() for p in protected_paths}
staged_before = staged_entries()
state = dict(step='pv', started=time.monotonic(), step_started=time.monotonic(), done=False, busy=False)
result = dict(complete=False, observations_complete=False, mode='failure' if FAILURE else 'normal',
              events=[], failures=[], asset_saves=0, physical_input_verified=False,
              audio_acceptance=False, visual_acceptance=False, skip_uses_widget_api=True)


def event(name, **values):
    result['events'].append(dict(event=name, elapsed=time.monotonic() - state['started'], **values))


def step(name):
    state['step'] = name
    state['step_started'] = time.monotonic()


def active(pc, cls):
    return [w for w in ue.ObjectIterator(ue.CommonActivatableWidget)
            if w.get_class() == cls and w.get_owning_player() == pc and w.is_activated()]


def playing_music():
    # This separate editor opens no audio-preview asset editors. The legacy
    # fire-and-forget BeginPlay path is independently rejected by graph verify.
    return [a for a in ue.ObjectIterator(ue.AudioComponent)
            if a.get_editor_property('Sound') == cue and a.is_playing()]


def finish(ok):
    if state['done']:
        return
    state['done'] = True
    cdo.set_editor_property('IntroMediaSource', original_source)
    result['protected_changes'] = [p for p, h in before.items() if hashlib.sha256((PROJECT / p).read_bytes()).hexdigest() != h]
    result['staged_entries_unchanged'] = staged_entries() == staged_before
    result['cdo_source_restored'] = cdo.get_editor_property('IntroMediaSource') == original_source
    result['observations_complete'] = bool(ok and not result['failures'] and not result['protected_changes'] and result['staged_entries_unchanged'])
    OUT.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(handle)
    if result['observations_complete']:
        ue.log('DC_FRONTEND_MUSIC_OBSERVATIONS_COMPLETE ' + str(OUT))
    else:
        ue.log_error('DC_FRONTEND_MUSIC_PROBE_FAILED ' + str(result['failures']))


def tick(_delta):
    if state['done'] or state['busy']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['step_started'] < 50, 'Timeout at ' + state['step']
        worlds = ue.EditorLevelLibrary.get_pie_worlds(False)
        if not worlds:
            if state['step'] == 'ending':
                assert not playing_music(), 'BGM survived frontend world teardown'
                event('bgm_stopped_after_world_end')
                finish(True)
            return
        assert len(worlds) == 1
        world = worlds[0]
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        game_state = ue.GameplayStatics.get_game_state(world)
        if not pc or not game_state:
            return
        component = game_state.get_component_by_class(component_cls)
        if not component:
            return
        intros, menus = active(pc, intro_cls), active(pc, menu_cls)
        music = component.get_menu_music_component()
        if state['step'] in ('pv', 'reentry_pv'):
            if FAILURE:
                if menus:
                    event('invalid_pv_reached_modes')
                    step('menu')
                return
            if not intros:
                assert not menus, 'Mode menu appeared before a valid PV'
                return
            assert len(intros) == 1 and not menus and not music and not playing_music()
            player = intros[0].get_intro_player()
            if not player or not intros[0].is_intro_playing() or ue.MathLibrary.get_total_seconds(player.get_time()) < .25:
                return
            assert player.is_playing() and not player.has_error()
            event('pv_only_audio_phase', visit=1 if state['step'] == 'pv' else 2)
            intros[0].skip_intro()
            step('menu' if state['step'] == 'pv' else 'second_menu')
        elif state['step'] in ('menu', 'second_menu'):
            if not menus or not music or not music.is_playing():
                return
            assert len(menus) == 1 and not intros and playing_music() == [music]
            event('one_bgm_after_pv', component=repr(music), visit=2 if state['step'] == 'second_menu' else 1)
            if FAILURE or state['step'] == 'second_menu':
                step('ending')
                ue.EditorLevelLibrary.editor_end_play()
            else:
                state['music_token'] = repr(music)
                menus[0].open_settings()
                step('settings')
        elif state['step'] == 'settings':
            settings = active(pc, settings_cls)
            if not settings:
                return
            assert music and music.is_playing() and repr(music) == state['music_token'] and playing_music() == [music]
            event('settings_kept_same_bgm')
            settings[0].deactivate_widget()
            step('settings_closed')
        elif state['step'] == 'settings_closed':
            if not menus or active(pc, settings_cls):
                return
            assert music and music.is_playing() and repr(music) == state['music_token'] and playing_music() == [music]
            event('settings_close_did_not_restart_bgm')
            step('reentry_pv')
            ue.GameplayStatics.open_level(world, MENU)
    except Exception:
        result['failures'].append(traceback.format_exc())
        finish(False)
    finally:
        state['busy'] = False


handle = ue.register_slate_post_tick_callback(tick)
