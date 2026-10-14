"""R13 standalone PIE: actual Experience/HUD/menu/settings/result/travel lifecycle.

Native layer pushes, delegate handlers and diagnostic damage are automation, not
physical input or visual acceptance. No assets or user settings are saved.
"""
import json,re,time,hashlib,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
cmd=ue.SystemLibrary.get_command_line();run=re.search(r'-DCR13UIProbe=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('pie_'+run+'.json');assert not out.exists()
ROOT='/Game/DreamCatcher/GAS/Test/R13';UI='/Game/LyraMigration/UI/R13'
RETURN='/Game/DreamCatcher/GAS/Test/R10/LyraEncounter'
state=dict(step='ready',wall=time.monotonic(),last=-1,done=False,busy=False,immune=False)
result=dict(complete=False,events=[],failures=[],asset_saves=0,settings_saves=0,physical_input_verified=False,visual_quality_verified=False)
protected={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
savefiles=set(str(f) for f in (P/'Saved/SaveGames').rglob('*') if f.is_file())

def cls(name):
    value=ue.load_class(None,'/Script/DreamCatcher.'+name);assert value,name;return value
def assetclass(path):
    value=ue.load_class(None,path+'.'+path.rsplit('/',1)[1]+'_C');assert value,path;return value
def tag(name):
    v=ue.GameplayTag();assert v.import_text('(TagName="'+name+'")');return v
def tags(name):
    v=ue.GameplayTagContainer();assert v.import_text('(GameplayTags=((TagName="'+name+'")))');return v
def event(name,t,**data):result['events'].append(dict(event=name,time=t,**data));print('R13_UI_EVENT '+str(result['events'][-1]))
def widgets(pc,c):return [x for x in ue.ObjectIterator(ue.UserWidget) if x.get_class()==c and x.get_owning_player()==pc and not x.get_name().startswith('Default__')]
def one(pc,c):
    rows=widgets(pc,c);assert len(rows)==1,(c.get_name(),len(rows));return rows[0]
def actors(w,c):return list(ue.GameplayStatics.get_all_actors_of_class(w,c))
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
ui=ue.get_default_object(ue.load_class(None,'/Script/CommonGame.CommonUIExtensions'))
health_class=cls('DCLyraHealthComponent');overlay_class=cls('DCStageOverlayWidget');result_class=cls('DCStageResultLayer')
menu_class=assetclass(UI+'/Game/UI/Hud/W_LyraGameMenu');settings_class=assetclass(UI+'/Game/UI/Settings/W_LyraSettingScreen')
confirmation_class=assetclass(UI+'/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
layout_class=assetclass(UI+'/ShooterCore/UserInterface/W_ShooterHUDLayout')
quick_class=assetclass('/Game/LyraMigration/UI/R7/Equipment/W_QuickBar')
reticle_class=assetclass('/Game/LyraMigration/UI/R7/W_WeaponReticleHost')
damage_class=assetclass('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
root_class=assetclass('/Game/LyraMigration/UI/R7/Foundation/W_OverallUILayout')
blocked=tag('Gameplay.AbilityInputBlocked');immune_tags=tags('Gameplay.DamageImmunity')

def health(p):return p.get_component_by_class(health_class) if p else None
def asc(p):return gas.call_method('GetAbilitySystemComponent',args=(p,))
def immune(p,value):
    if state['immune']==value:return
    assert gas.call_method('AddLooseGameplayTags' if value else 'RemoveLooseGameplayTags',args=(p,immune_tags,False));state['immune']=value
def push(pc,c):
    lp=ui.call_method('GetLocalPlayerFromController',args=(pc,))
    value=ui.call_method('PushContentToLayer_ForPlayer',args=(lp,tag('UI.Layer.Menu'),c));assert value;return value
def damage(target,source=None):
    component=asc(source or target);assert component
    spec=component.make_outgoing_spec(damage_class,1,component.make_effect_context())
    spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),10000.0))
    if source is None:spec=gas.call_method('AddAssetTags',args=(spec,tags('Gameplay.Damage.SelfDestruct')))
    component.apply_gameplay_effect_spec_to_target(spec,asc(target))

def finish(ok):
    if state['done']:return
    state['done']=True
    w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    pc=ue.GameplayStatics.get_player_controller(w,0) if w else None
    p=pc.get_controlled_pawn() if pc else None
    if p and state['immune']:immune(p,False)
    result['protected_changed']=[f for f,h in protected.items() if not Path(f).is_file() or hashlib.sha256(Path(f).read_bytes()).hexdigest()!=h]
    result['new_user_savefiles']=sorted(set(str(f) for f in (P/'Saved/SaveGames').rglob('*') if f.is_file())-savefiles)
    result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
    result['complete']=ok and not result['failures'] and not result['protected_changed'] and not result['new_user_savefiles'] and result['ensure_count']==0
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not result['complete']:ue.log_error('R13 UI probe failed: '+str(out))
    print('R13_UI_PIE '+str({k:result[k] for k in ['complete','failures','ensure_count']}))

def tick(delta):
    if state['done'] or state['busy']:return
    state['busy']=True
    try:
        assert time.monotonic()-state['wall']<230,'PIE timeout'
        w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not w or 'UEDPIE_' not in w.get_path_name():return
        t=ue.GameplayStatics.get_time_seconds(w)
        if state['step']=='reloading':
            if t>=state['travel_at']-1:return
            state.update(step='retry',last=-1,immune=False)
        if state['step']=='returning':
            if not w.get_path_name().startswith(RETURN+'/UEDPIE_'):return
            state.update(step='returned',last=-1,immune=False)
        if t-state['last']<.15:return
        state['last']=t
        pc=ue.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
        mode=ue.GameplayStatics.get_game_mode(w)
        ready=bool(p and health(p) and health(p).get_health()>0 and pc.get_component_by_class(cls('DCLyraQuickBarComponent')))
        if state['step']=='returned':
            if not ready:return
            assert not any(x.is_activated() for x in widgets(pc,layout_class)+widgets(pc,result_class))
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored() and not asc(p).has_matching_gameplay_tag(blocked)
            event('returned_to_R10_without_R13_UI_or_input',t);finish(True);return
        if not isinstance(mode,ue.DCStageRunGameMode):return
        if state['step'] in ('ready','retry'):
            if not ready or not mode.get_stage_director() or not widgets(pc,overlay_class) or t<1.0:return
            assert mode.uses_experience_ui() and mode.get_stage_widget() is None
            for c in [root_class,layout_class,overlay_class,quick_class,reticle_class]:one(pc,c)
            assert pc.get_component_by_class(cls('DCStageUIComponent'))
            active_results=[x for x in widgets(pc,result_class) if x.is_activated()]
            if (active_results or asc(p).has_matching_gameplay_tag(blocked)) and t<6.0:return
            assert not active_results and not asc(p).has_matching_gameplay_tag(blocked),(len(active_results),'input still blocked after readiness')
            immune(p,True);event('hud_ready' if state['step']=='ready' else 'retry_hud_ready',t)
            push(pc,menu_class);state.update(step='menu' if state['step']=='ready' else 'return_menu',at=t);return
        if state['step']=='menu' and t-state['at']>.6:
            assert one(pc,menu_class).is_activated() and pc.is_move_input_ignored() and pc.is_look_input_ignored()
            assert asc(p).has_matching_gameplay_tag(blocked)
            push(pc,settings_class);state.update(step='settings',at=t);event('menu_blocks_combat_input',t)
        elif state['step']=='settings' and t-state['at']>1.5:
            screen=one(pc,settings_class);assert screen.is_activated()
            setting_objects=[x for x in ue.ObjectIterator(ue.GameSetting) if not x.get_name().startswith('Default__')]
            names=sorted(set(str(x.get_dev_name()) for x in setting_objects))
            assert 'OverallVolume' in names and len(names)>20,names
            result['settings_dev_names']=names
            screen.call_method('CancelChanges');screen.deactivate_widget()
            state.update(step='close_menu',at=t);event('settings_registry_constructed_and_cancelled',t,count=len(names))
        elif state['step']=='close_menu' and t-state['at']>.5:
            one(pc,menu_class).deactivate_widget();state.update(step='unblocked',at=t)
        elif state['step']=='unblocked' and t-state['at']>.6:
            assert not asc(p).has_matching_gameplay_tag(blocked) and not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
            push(pc,menu_class);state.update(step='kill_in_menu',at=t);event('menu_close_releases_own_input',t)
        elif state['step']=='kill_in_menu' and t-state['at']>.5:
            immune(p,False);damage(p);state.update(step='death_result',at=t);event('player_death_with_menu_open',t)
        elif state['step']=='death_result' and t-state['at']>8.5:
            assert mode.has_run_ended() and not mode.was_run_successful()
            assert one(pc,result_class).is_activated()
            assert all(not x.is_activated() for x in widgets(pc,menu_class)+widgets(pc,settings_class))
            assert not actors(w,cls('DCBossCharacter')) and not actors(w,cls('DCLyraPlayerBotController'))
            assert mode.request_restart_stage() and not mode.request_restart_stage()
            event('result_replaces_menu_and_retry_once',t);state.update(step='reloading',travel_at=t,last=-1,immune=False);return
        elif state['step']=='return_menu' and t-state['at']>.6:
            assert one(pc,menu_class).is_activated()
            button=one(pc,menu_class).get_editor_property('ReturnButton');assert button
            button.call_method('HandleButtonClicked')
            event('original_return_confirmation_requested',t);state.update(step='confirmation',at=t)
        elif state['step']=='confirmation' and t-state['at']>.5:
            dialog=one(pc,confirmation_class);assert dialog.is_activated()
            boxes=[x for x in ue.ObjectIterator(ue.DynamicEntryBox) if x.get_name()=='EntryBox_Buttons' and x.get_outer().get_outer()==dialog]
            assert len(boxes)==1
            buttons=list(boxes[0].get_all_entries());assert len(buttons)==2
            buttons[0].call_method('HandleButtonClicked')
            assert mode.is_stage_travel_pending()
            event('original_confirmation_calls_stage_return',t);state.update(step='returning',last=-1);return
    except Exception:
        result['failures'].append(traceback.format_exc());finish(False)
    finally:state['busy']=False

state['callback']=ue.register_slate_post_tick_callback(tick)
print('R13_UI_PIE_ARMED')
