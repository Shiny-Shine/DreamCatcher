"""Arm a bounded real-PIE menu/story/return check; no asset or config saves.

Run before Project.Maps.PIE in a separate offscreen editor on L_DC_FrontEnd.
This invokes actual widget button delegates, but is not physical-input or art QA.
The existing combat is not replayed: FailStage drives the accepted result path.
"""
import hashlib
import json
import re
import time
import traceback
from pathlib import Path

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/Frontend'
run = re.search(r'-DCFrontendProbe=([A-Za-z0-9_]+)', ue.SystemLibrary.get_command_line()).group(1)
out = REPORTS / ('pie_' + run + '.json')
assert not out.exists()
MENU = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'
STORY = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
state = dict(step='menu_initial', wall=time.monotonic(), busy=False, done=False)
result = dict(complete=False, run=run, events=[], failures=[], asset_saves=0,
              physical_input_verified=False, visual_acceptance=False, forced_result=True,
              loading_screen_verified=False)
protected_paths = [PROJECT / 'Config/DefaultEngine.ini', PROJECT / 'Config/DefaultGame.ini',
                   PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
                   PROJECT / 'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
                   PROJECT / 'Content/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode.uasset',
                   PROJECT / 'Content/DreamCatcher/Blueprints/Core/BP_DC_FrontEndGameMode.uasset',
                   PROJECT / 'Content/DreamCatcher/Experiences/B_DC_FrontEndExperience.uasset',
                   PROJECT / 'Content/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle.uasset']
for root in ['Content/DreamCatcher/UI/Frontend', 'Content/DreamCatcher/Blueprints/UI', 'Content/DreamCatcher/Maps']:
    protected_paths.extend((PROJECT / root).glob('*.uasset'))
    protected_paths.extend((PROJECT / root).glob('*.umap'))


def hashes(paths):
    return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()}


before = hashes(protected_paths)
save_before = hashes((PROJECT / 'Saved/SaveGames').rglob('*'))


def native(name):
    return ue.load_class(None, '/Script/DreamCatcher.' + name)


def bp(path):
    return ue.load_class(None, path + '.' + path.rsplit('/', 1)[-1] + '_C')


screen_cls = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
settings_cls = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Settings/W_LyraSettingScreen')
game_menu_cls = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu')
dialog_cls = bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
result_cls = native('DCStageResultLayer')
ammo_cls = bp('/Game/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle')
director_cls = native('DCStageDirector')
bot_cls = native('DCLyraPlayerBotController')
boss_cls = native('DCBossCharacter')
loading_cls = ue.load_class(None, '/Script/CommonLoadingScreen.LoadingScreenManager')
ui = ue.get_default_object(ue.load_class(None, '/Script/CommonGame.CommonUIExtensions'))
menu_tag = ue.GameplayTag()
assert menu_tag.import_text('(TagName="UI.Layer.Menu")')
assert all([screen_cls, settings_cls, game_menu_cls, dialog_cls, result_cls])
ammo_tag = ue.GameplayTag()
capacity_tag = ue.GameplayTag()
assert ammo_tag.import_text('(TagName="Lyra.ShooterGame.Weapon.MagazineAmmo")')
assert capacity_tag.import_text('(TagName="Lyra.ShooterGame.Weapon.MagazineSize")')


def event(name, **data):
    row = dict(event=name, elapsed=round(time.monotonic() - state['wall'], 2), **data)
    result['events'].append(row)
    print('DC_FRONTEND_PIE_EVENT ' + json.dumps(row, ensure_ascii=False))


def active(pc, cls):
    return [w for w in ue.ObjectIterator(ue.CommonActivatableWidget)
            if w.get_class() == cls and w.get_owning_player() == pc and w.is_activated()]


def one(pc, cls):
    values = active(pc, cls)
    assert len(values) == 1, (cls.get_name(), len(values))
    return values[0]


def child(owner, name):
    found = []
    for widget in ue.ObjectIterator(ue.Widget):
        if widget.get_name() != name:
            continue
        outer = widget.get_outer()
        while outer:
            if outer == owner:
                found.append(widget)
                break
            outer = outer.get_outer()
    assert len(found) == 1, (owner.get_name(), name, len(found))
    return found[0]


def click(widget):
    if isinstance(widget, ue.CommonButtonBase):
        widget.call_method('HandleButtonClicked')
    else:
        widget.get_editor_property('OnClicked').broadcast()


def choose_dialog(pc, index):
    dialog = one(pc, dialog_cls)
    buttons = list(child(dialog, 'EntryBox_Buttons').get_all_entries())
    assert len(buttons) == 2
    click(buttons[index])


def menu_ready(world, pc):
    assert not pc.get_controlled_pawn(), 'Menu spawned a pawn'
    assert not ue.GameplayStatics.get_all_actors_of_class(world, director_cls)
    assert not ue.GameplayStatics.get_all_actors_of_class(world, bot_cls)
    assert not ue.GameplayStatics.get_all_actors_of_class(world, boss_cls)
    assert not active(pc, result_cls) and not active(pc, game_menu_cls)
    return one(pc, screen_cls)


def finish(ok):
    if state['done']:
        return
    state['done'] = True
    result['protected_changes'] = [p for p, digest in before.items()
                                   if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest() != digest]
    result['user_savegames_unchanged'] = save_before == hashes((PROJECT / 'Saved/SaveGames').rglob('*'))
    log = (REPORTS / ('pie_' + run + '.log')).read_text(encoding='utf-8-sig', errors='replace')
    result['ensure_count'] = log.count('Ensure condition failed')
    result['engine_errors'] = [line for line in log.splitlines() if ': Error:' in line or 'Fatal:' in line]
    result['complete'] = ok and not result['failures'] and not result['protected_changes'] and result['user_savegames_unchanged'] and not result['ensure_count'] and not result['engine_errors']
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not result['complete']:
        ue.log_error('Frontend PIE probe failed; see ' + str(out))
    print('DC_FRONTEND_PIE_RESULT ' + json.dumps({k: v for k, v in result.items() if k != 'events'}, ensure_ascii=False))


def tick(delta):
    if state['done'] or state['busy']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['wall'] < 140, 'Frontend PIE timed out at ' + state['step']
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            return
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc:
            return
        level = ue.GameplayStatics.get_current_level_name(world, True)
        t = ue.GameplayStatics.get_time_seconds(world)
        mode = ue.GameplayStatics.get_game_mode(world)
        step = state['step']
        if step == 'stage_after_retry' and level == 'Level_1' and t < state['travel_at']:
            state['restart_seen'] = True
        if step in ['menu_initial', 'menu_after_escape', 'menu_final']:
            if level != 'L_DC_FrontEnd' or not active(pc, screen_cls):
                return
            screen = menu_ready(world, pc)
            if t < 2:
                return
            game_instance = ue.GameplayStatics.get_game_instance(world)
            loading = [obj for obj in ue.ObjectIterator(loading_cls) if obj.get_outer() == game_instance]
            assert len(loading) <= 1
            if loading:
                reason = loading[0].get_debug_reason_for_showing_or_hiding_loading_screen()
                if reason != '(nothing wants to show it anymore)':
                    return
            else:
                # Existing plugin gate is false in normal PIE and packaged builds.
                # Verify the requested navigation path without claiming loading-screen activation.
                reason = 'disabled_by_existing_diagnostic_gate'
            assert pc.get_editor_property('bShowMouseCursor'), 'Menu cursor is hidden'
            event(step, pawn=False, screen=screen.get_class().get_name(), loading_reason=reason)
            if step == 'menu_initial':
                click(child(screen, 'SettingsButton'))
                state.update(step='settings', at=t)
            elif step == 'menu_after_escape':
                click(child(screen, 'StoryButton'))
                assert mode.is_story_travel_pending()
                state.update(step='stage_for_result')
            else:
                finish(True)
        elif step == 'settings':
            if not active(pc, settings_cls):
                return
            assert not pc.get_controlled_pawn()
            one(pc, settings_cls).deactivate_widget()
            event('existing_settings_opened_and_closed_without_edits')
            state.update(step='quit_prompt', at=t)
        elif step == 'quit_prompt':
            if not active(pc, screen_cls) or t - state['at'] < .5:
                return
            click(child(one(pc, screen_cls), 'QuitButton'))
            state.update(step='quit_cancel', at=t)
        elif step == 'quit_cancel':
            if not active(pc, dialog_cls) or t - state['at'] < .5:
                return
            choose_dialog(pc, 1)
            event('quit_confirmation_canceled')
            state.update(step='story_click', at=t)
        elif step == 'story_click':
            if active(pc, dialog_cls) or t - state['at'] < .5:
                return
            click(child(one(pc, screen_cls), 'StoryButton'))
            assert mode.is_story_travel_pending()
            event('story_card_requested_level1')
            state.update(step='stage_for_escape')
        elif step in ['stage_for_escape', 'stage_for_result', 'stage_after_retry']:
            if level != 'Level_1' or t < 2.5 or not pc.get_controlled_pawn():
                return
            if step == 'stage_after_retry' and not state.get('restart_seen'):
                return
            assert not mode.has_run_ended()
            director = mode.get_stage_director()
            if not director or director.get_current_phase_index() != 0:
                return
            assert not active(pc, screen_cls), 'Frontend remains above stage'
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
            counters = [widget for widget in ue.ObjectIterator(ue.UserWidget)
                        if widget.get_class() == ammo_cls and widget.get_owning_player() == pc
                        and widget.get_editor_property('InventoryInstance')]
            if not counters:
                return
            assert len(counters) == 1
            counter = counters[0]
            item = counter.get_editor_property('InventoryInstance')
            ammo = item.get_stat_tag_stack_count(ammo_tag)
            capacity = item.get_stat_tag_stack_count(capacity_tag)
            for name in ['AmmoCounter_Bar', 'AmmoCounter_Glow']:
                material = child(counter, name).get_dynamic_material()
                assert material.get_scalar_parameter_value('Rounds_Current') == ammo
                assert material.get_scalar_parameter_value('Rounds_Max') == capacity
                assert material.get_scalar_parameter_value('IsLowAmmo') == (1 if ammo <= 4 else 0)
            event(step, pawn=pc.get_controlled_pawn().get_class().get_name(), phase=0,
                  ammo=ammo, magazine_capacity=capacity, ammo_display_matches_inventory=True)
            if step == 'stage_for_escape':
                local_player = ui.call_method('GetLocalPlayerFromController', args=(pc,))
                assert ui.call_method('PushContentToLayer_ForPlayer', args=(local_player, menu_tag, game_menu_cls))
                state.update(step='escape_return', at=t)
            else:
                director.fail_stage()
                state.update(step='retry_result' if step == 'stage_for_result' else 'return_result', at=t)
        elif step == 'escape_return':
            if not active(pc, game_menu_cls) or t - state['at'] < .5:
                return
            click(child(one(pc, game_menu_cls), 'ReturnButton'))
            state.update(step='escape_confirm', at=t)
        elif step == 'escape_confirm':
            if not active(pc, dialog_cls) or t - state['at'] < .5:
                return
            choose_dialog(pc, 0)
            assert mode.is_stage_travel_pending()
            event('existing_escape_menu_confirmed_return')
            state.update(step='menu_after_escape')
        elif step in ['retry_result', 'return_result']:
            if not active(pc, result_cls) or t - state['at'] < .5:
                return
            layer = one(pc, result_cls)
            assert mode.has_run_ended() and not mode.was_run_successful()
            if step == 'retry_result':
                click(child(layer, 'RetryButton'))
                assert mode.is_stage_travel_pending()
                event('result_retry_requested_same_level')
                state.update(step='stage_after_retry', travel_at=t)
            else:
                click(child(layer, 'ExitButton'))
                assert mode.is_stage_travel_pending()
                event('result_return_requested_frontend')
                state.update(step='menu_final')
    except Exception:
        result['failures'].append(traceback.format_exc())
        finish(False)
    finally:
        state['busy'] = False


state['callback'] = ue.register_slate_post_tick_callback(tick)
print('DC_FRONTEND_PIE_ARMED')
