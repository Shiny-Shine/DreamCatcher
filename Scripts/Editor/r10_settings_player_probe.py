"""Focused old-player regression for the approved global SettingsLocal registration."""
import hashlib,json,re,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher')
run=re.search(r'-DCR10SettingsProbe=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out=P/'Saved/Diagnostics/R10'/('settings_player_'+run+'.json');assert not out.exists()
scope={'__name__':'r10_reuse_player_observers'}
exec(compile((P/'Scripts/Editor/r7_reticle_host_probe.py').read_text(encoding='utf-8-sig'),'r7_existing_helpers','exec'),scope)
state=scope['state'];ue.unregister_slate_post_tick_callback(state['callback'])
cls,tag=scope['cls'],scope['tag']
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
           (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI')
           for p in root.rglob('*') if p.is_file()}
result=dict(complete=False,events=[],failures=[],asset_saves=0,physical_inputs_tested=False,screen_quality_verified=False)
move=scope['config'].find_native_input_action_for_tag(tag('InputTag.Move'),True)
look=scope['config'].find_native_input_action_for_tag(tag('InputTag.Look'),False) or scope['config'].find_native_input_action_for_tag(tag('InputTag.Look.Mouse'),True)
assert move and look
new_ai_map='-DCR10SettingsNewAIMap' in ue.SystemLibrary.get_command_line()
expected_root='/Game/DreamCatcher/GAS/Test/R10/LyraAI' if new_ai_map else scope['ROOT']
def event(label,**data):
    result['events'].append(dict(event=label,time=ue.GameplayStatics.get_time_seconds(state['world']),**data))
def stage(label):state.update(phase=label,since=ue.GameplayStatics.get_time_seconds(state['world']))
def finish(reason):
    for action in state['inputs']:state['subsystem'].inject_input_vector_for_action(action,ue.Vector(),[],[])
    result['reason']=reason
    result['protected_changed']=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    result['complete']=reason=='passed' and not result['protected_changed']
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    print('R10_SETTINGS_PLAYER '+json.dumps({k:result[k] for k in ('complete','reason','protected_changed')}))
def tick(delta):
    try:
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():return
        assert world.get_path_name().startswith(expected_root+'/UEDPIE_')
        now=ue.GameplayStatics.get_time_seconds(world)
        if state['phase']=='waiting':
            pc=ue.GameplayStatics.get_player_controller(world,0)
            pawn=ue.GameplayStatics.get_player_pawn(world,0)
            qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent')) if pc else None
            if not pawn or not qb or not qb.get_active_slot_item():return
            subsystem=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
            state.update(world=world,pc=pc,qb=qb,subsystem=subsystem,started=now)
            scope['bind_pawn'](pawn)
            if new_ai_map:
                assert not ue.GameplayStatics.get_all_actors_of_class(world,cls('DCLyraPlayerBotController')),'This isolated player regression requires zero bots through the original developer setting'
                assert len(pc.get_component_by_class(cls('DCInventoryManagerComponent')).get_all_items())==1
                event('new_map_player',pawn=pawn.get_class().get_path_name(),inventory_count=1)
            settings=ue.GameUserSettings.get_game_user_settings()
            assert settings.get_class().get_path_name()=='/Script/DreamCatcher.DCLyraSettingsLocal'
            event('settings_class',class_path=settings.get_class().get_path_name(),resolution=str(settings.get_screen_resolution()),fullscreen=str(settings.get_fullscreen_mode()))
            stage('settle')
        assert now-state['started']<22,'Player regression timeout: '+state['phase']
        phase,elapsed=state['phase'],now-state['since']
        if phase=='settle' and elapsed>1:
            scope['inspect_ui']('r10_settings_initial_ui')
            state.update(position=state['pawn'].get_actor_location(),rotation=state['pc'].get_control_rotation(),full=scope['ammo'](),aim_mode=state['pawn'].get_gas_aim_mode())
            state['inputs'][move]=ue.Vector(0,1,0)
            state['inputs'][look]=ue.Vector(.5,0,0)
            stage('move_look')
        elif phase=='move_look' and elapsed>.5:
            state['inputs'][move]=ue.Vector();state['inputs'][look]=ue.Vector()
            distance=(state['pawn'].get_actor_location()-state['position']).length()
            yaw=state['pc'].get_control_rotation().yaw-state['rotation'].yaw
            assert distance>30 and abs(yaw)>1,(distance,yaw)
            event('movement_look',distance_cm=distance,yaw_delta=yaw)
            scope['input_value'](scope['aim'],True);stage('aim')
        elif phase=='aim' and elapsed>.8:
            mode=state['pawn'].get_gas_aim_mode()
            assert mode!=state['aim_mode'],str(mode)
            event('aim_enter',mode=str(mode),fov=state['pc'].player_camera_manager.get_fov_angle())
            scope['input_value'](scope['aim'],False);stage('aim_exit')
        elif phase=='aim_exit' and elapsed>.8:
            assert state['pawn'].get_gas_aim_mode()==state['aim_mode']
            event('aim_exit',mode=str(state['pawn'].get_gas_aim_mode()))
            scope['input_value'](scope['fire'],True);stage('fire')
        elif phase=='fire' and scope['ammo']()<=state['full']-3:
            scope['input_value'](scope['fire'],False);stage('release')
            event('fired',before=state['full'],after=scope['ammo']())
        elif phase=='release' and elapsed>.35:
            scope['input_value'](scope['reload_action'],True);stage('reload_press')
        elif phase=='reload_press' and elapsed>.1:
            scope['input_value'](scope['reload_action'],False);stage('reload_wait')
        elif phase=='reload_wait' and scope['ammo']()==state['full']:
            scope['inspect_ui']('r10_settings_final_ui')
            event('reloaded',ammo=scope['ammo'](),reticle_in_viewport=scope['host']().is_in_viewport())
            finish('passed');return
        for action,value in state['inputs'].items():state['subsystem'].inject_input_vector_for_action(action,value,[],[])
    except Exception:
        details=traceback.format_exc();result['failures'].append(details);ue.log_error(details);finish('failed')
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R10_SETTINGS_PLAYER_ARMED')
