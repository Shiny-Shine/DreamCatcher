"""Bounded real PIE verification for Level_1 flow/navigation/same-map return.

Uses injected input, a temporary AI MoveTo trial, and diagnostic GAS damage.
Also checks crouch, scope, player death/cleanup and explicit restart after native cleanup.
No asset writes or C++ build; not physical-input or visual acceptance.
"""
import hashlib,json,re,time,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/Level1Integration'
run=re.search(r'-DCLevel1Probe=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out=R/('pie_'+run+'.json');assert not out.exists()
MAP='/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
state=dict(step='ready',wall=time.monotonic(),done=False,busy=False,immune=False,killed=set(),bot_immune=False)
result=dict(complete=False,events=[],failures=[],asset_saves=0,physical_input_or_visual_verified=False)
protected={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for root in ['Source','Config','Content/DreamCatcher/GAS','Content/DreamCatcher/Lyra','Content/DreamCatcher/Data','Content/DreamCatcher/Input','Content/DreamCatcher/Experiences','Content/DreamCatcher/Blueprints','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
for f in [P/'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']:protected[str(f)]=hashlib.sha256(f.read_bytes()).hexdigest()
savefiles={str(f) for f in (P/'Saved/SaveGames').rglob('*') if f.is_file()}
def cls(n):return ue.load_class(None,'/Script/DreamCatcher.'+n)
def bp(p):return ue.load_class(None,p+'.'+p.rsplit('/',1)[-1]+'_C')
def tag(n):
 value=ue.GameplayTag();assert value.import_text('(TagName="'+n+'")');return value
def tags(n):
 value=ue.GameplayTagContainer();assert value.import_text('(GameplayTags=((TagName="'+n+'")))');return value
def event(n,t,**data):result['events'].append(dict(event=n,time=t,**data));print('LEVEL1_PIE_EVENT '+str(result['events'][-1]))
def actors(w,c):return list(ue.GameplayStatics.get_all_actors_of_class(w,c))
def widgets(pc,c):return [x for x in ue.ObjectIterator(ue.UserWidget) if x.get_class()==c and x.get_owning_player()==pc and not x.get_name().startswith('Default__')]
def one(pc,c):
 values=widgets(pc,c);assert len(values)==1,(c.get_name(),len(values));return values[0]
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
ui=ue.get_default_object(ue.load_class(None,'/Script/CommonGame.CommonUIExtensions'))
subs=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
health_cls=cls('DCLyraHealthComponent');bot_cls=cls('DCLyraPlayerBotController');boss_cls=cls('DCBossCharacter')
overlay_cls=cls('DCStageOverlayWidget');boss_ui_cls=cls('DCBossHealthWidget');result_cls=cls('DCStageResultLayer')
input_cls=ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')
menu_cls=bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_LyraGameMenu')
confirm_cls=bp('/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Dialogs/W_ConfirmationDefault')
damage_cls=bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
data=ue.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')
fire=data.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'),True)
aim=data.find_native_input_action_for_tag(tag('InputTag.Aim'),True)
move=data.find_native_input_action_for_tag(tag('InputTag.Move'),True)
ammo_tag=tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
map_settings=ue.get_default_object(ue.load_class(None,'/Script/EngineSettings.GameMapsSettings'))
packaging=ue.get_default_object(ue.load_class(None,'/Script/DeveloperToolSettings.ProjectPackagingSettings'))
result['effective_game_default_map']=map_settings.get_editor_property('GameDefaultMap').export_text()
result['effective_maps_to_cook']=[x.get_editor_property('FilePath') for x in packaging.get_editor_property('MapsToCook')]
assert MAP+'.Level_1' in result['effective_game_default_map']
assert result['effective_maps_to_cook']==[MAP]
def health(p):return p.get_component_by_class(health_cls) if p else None
def asc(p):return gas.call_method('GetAbilitySystemComponent',args=(p,))
def input_system(pc):return subs.call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,input_cls))
def inject(pc,action,on):input_system(pc).inject_input_vector_for_action(action,ue.Vector(1 if on else 0,0,0),[],[])
def immunity(p,on):assert gas.call_method('AddLooseGameplayTags' if on else 'RemoveLooseGameplayTags',args=(p,tags('Gameplay.DamageImmunity'),False))
def damage(target,source):
 a=asc(source);spec=a.make_outgoing_spec(damage_cls,1,a.make_effect_context())
 spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),10000.0))
 a.apply_gameplay_effect_spec_to_target(spec,asc(target))
def finish(ok):
 if state['done']:return
 state['done']=True
 result['protected_changes']=[p for p,h in protected.items() if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
 result['new_user_savefiles']=sorted({str(f) for f in (P/'Saved/SaveGames').rglob('*') if f.is_file()}-savefiles)
 log=(P/'Saved/Logs'/('Level1Flow-PIE-'+run+'.log')).read_text(encoding='utf-8-sig',errors='replace')
 result['ensure_count']=log.count('Ensure condition failed')
 result['engine_errors']=[line for line in log.splitlines() if ': Error:' in line or 'Fatal:' in line]
 result['complete']=ok and not result['failures'] and not result['protected_changes'] and not result['new_user_savefiles'] and result['ensure_count']==0 and not result['engine_errors']
 out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 ue.unregister_slate_post_tick_callback(state['callback'])
 if not result['complete']:ue.log_error('Level_1 PIE verification failed: '+str(out))
 print('LEVEL1_PIE_RESULT '+str({k:v for k,v in result.items() if k!='events'}))
def tick(delta):
 if state['done'] or state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['wall']<220,'Level_1 PIE timeout'
  w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
  if not w or 'UEDPIE_' not in w.get_path_name():return
  assert w.get_path_name().startswith(MAP.rsplit('/',1)[0]+'/UEDPIE_'),w.get_path_name()
  t=ue.GameplayStatics.get_time_seconds(w)
  if state['step'] in ['result_return_wait','menu_return_wait','death_restart_wait']:
   if t>=state['travel_at']-1:return
   old_step=state['step']
   event({'result_return_wait':'result_return_reloaded_same_map','menu_return_wait':'menu_return_reloaded_same_map','death_restart_wait':'death_restart_reloaded_same_map'}[old_step],t,map=w.get_path_name())
   state.update(step={'result_return_wait':'returned','menu_return_wait':'menu_returned','death_restart_wait':'death_restarted'}[old_step],immune=False)
  pc=ue.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
  if state['step']=='death_wait':
   assert t-state['at']<25,'Player death or failed-run cleanup did not finish'
   mode=ue.GameplayStatics.get_game_mode(w);qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent'))
   assert mode.has_run_ended() and not mode.was_run_successful()
   if p or qb.get_active_slot_item() or actors(w,bot_cls) or actors(w,boss_cls):return
   # Lyra detaches first, then destroys the avatar via its 0.1-second lifespan.
   if any(a.get_name()==state['dead_pawn_name'] for a in actors(w,ue.Character)):return
   layer=one(pc,result_cls)
   if not layer.is_activated():return
   if 'dead_empty_at' not in state:
    state['dead_empty_at']=t;event('player_death_avatar_loadout_and_ai_cleanup',t)
   if t-state['dead_empty_at']<1.5:return
   event('no_automatic_respawn_after_death',t)
   assert mode.request_restart_stage() and mode.is_stage_travel_pending()
   state.update(step='death_restart_wait',travel_at=t);return
  if not p or not health(p) or health(p).get_health()<=0:return
  if not state['immune']:immunity(p,True);state['immune']=True
  mode=ue.GameplayStatics.get_game_mode(w);director=mode.get_stage_director() if isinstance(mode,ue.DCStageRunGameMode) else None
  qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent'))
  if not director or not qb or not qb.get_active_slot_item() or not widgets(pc,overlay_cls) or t<1.5:return
  item=qb.get_active_slot_item();phase=director.get_current_phase_index()
  if state['step']=='ready':
   assert mode.get_class().get_path_name()=='/Game/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode.BP_DC_Level1GameMode_C'
   assert phase==0
   bots=[b for b in actors(w,bot_cls) if b.get_controlled_pawn() and health(b.get_controlled_pawn()) and not health(b.get_controlled_pawn()).is_dead_or_dying()]
   if len(bots)!=1:return
   immunity(bots[0].get_controlled_pawn(),True);state.update(bot_name=bots[0].get_name(),bot_immune=True,ammo=item.get_stat_tag_stack_count(ammo_tag),hip=p.get_gas_aim_mode(),step='fire',at=t)
   event('level1_experience_hud_loadout_ready',t,phase=phase,ammo=state['ammo']);inject(pc,fire,True)
  elif state['step']=='fire':
   inject(pc,fire,True)
   if t-state['at']>.35:
    inject(pc,fire,False);count=item.get_stat_tag_stack_count(ammo_tag);assert 0<count<state['ammo']
    event('rifle_fired',t,before=state['ammo'],after=count);state.update(step='aim',at=t);inject(pc,aim,True)
  elif state['step']=='aim':
   inject(pc,aim,True)
   if t-state['at']>.7:
    assert p.get_gas_aim_mode()!=state['hip'];state['shoulder']=p.get_gas_aim_mode();inject(pc,aim,False);state.update(step='aim_release',at=t)
  elif state['step']=='aim_release' and t-state['at']>.7:
   assert p.get_gas_aim_mode()==state['hip'];event('aim_and_hip_return',t)
   p.toggle_crouch();state.update(step='crouch_wait',at=t)
  elif state['step']=='crouch_wait' and t-state['at']>.3:
   assert p.get_movement_component().is_crouching();event('character_crouched',t)
   p.toggle_crouch();state.update(step='uncrouch_wait',at=t)
  elif state['step']=='uncrouch_wait' and t-state['at']>.3:
   assert not p.get_movement_component().is_crouching();event('character_uncrouched',t)
   inject(pc,aim,True);state.update(step='scope_tap_release',at=t)
  elif state['step']=='scope_tap_release':
   inject(pc,aim,False);state.update(step='scope_wait',at=t)
  elif state['step']=='scope_wait' and t-state['at']>.4:
   assert p.get_gas_aim_mode() not in [state['hip'],state['shoulder']]
   event('scope_tap_activated',t);inject(pc,aim,True);state.update(step='scope_off_release',at=t)
  elif state['step']=='scope_off_release':
   inject(pc,aim,False);state.update(step='scope_off_wait',at=t)
  elif state['step']=='scope_off_wait' and t-state['at']>.4:
   assert p.get_gas_aim_mode()==state['hip'];event('scope_tap_returned_to_hip',t);state.update(step='nav_ready',at=t)
  elif state['step']=='nav_ready':
   assert t-state['at']<75,'Navigation did not become ready'
   nav_data=actors(w,ue.RecastNavMesh);assert len(nav_data)==1
   assert nav_data[0].get_editor_property('RuntimeGeneration')==ue.RuntimeGenerationType.DYNAMIC
   bot=next(b for b in actors(w,bot_cls) if b.get_name()==state['bot_name']);pawn=bot.get_controlled_pawn()
   points=[s.get_actor_location() for s in actors(w,cls('DCEnemySpawner'))]
   # Avoid NavigationSystem static UFunction wrappers in PIE: they resolve a CDO world.
   # Instead prove reachability through the live AIController, with partial paths forbidden.
   goals=sorted(points,key=lambda q:(q-pawn.get_actor_location()).length(),reverse=True)
   bot.get_editor_property('BrainComponent').stop_logic('Level_1 navigation probe');bot.stop_movement()
   state.update(step='nav_pause',at=t,nav_deadline=t+75,goals=[[q.x,q.y,q.z] for q in goals],goal_index=0,total_distance=0)
   event('navigation_move_trial_to_all_encounters',t,goals=state['goals'])
  elif state['step']=='nav_pause' and t-state['at']>.5:
   bot=next(b for b in actors(w,bot_cls) if b.get_name()==state['bot_name'])
   state['goal']=state['goals'][state['goal_index']]
   request=bot.move_to_location(ue.Vector(*state['goal']),40,True,True,True,True,None,False)
   if request==ue.PathFollowingRequestResult.FAILED:
    if t-state.get('last_nav_diagnostic',-10)>10:
     state['last_nav_diagnostic']=t
     pawn=bot.get_controlled_pawn()
     event('navigation_request_failed',t,goal=state['goal'],pawn_position=pawn.get_actor_location().export_text(),nav_data=[dict(name=n.get_name(),main=n.get_editor_property('bCanBeMainNavData'),runtime=str(n.get_editor_property('RuntimeGeneration')),radius=n.get_editor_property('AgentRadius'),height=n.get_editor_property('AgentHeight')) for n in actors(w,ue.RecastNavMesh)],bounds=[v.get_actor_transform().export_text() for v in actors(w,ue.NavMeshBoundsVolume)])
    assert t<state['nav_deadline'],'Navigation never accepted a complete path'
    return
   v=bot.get_controlled_pawn().get_actor_location();state['bot_start']=[v.x,v.y,v.z]
   state.update(step='nav_move',at=t);event('navigation_move_requested',t,request=str(request),goal=state['goal'])
  elif state['step']=='nav_move':
   bot=next(b for b in actors(w,bot_cls) if b.get_name()==state['bot_name']);pawn=bot.get_controlled_pawn()
   distance=(pawn.get_actor_location()-ue.Vector(*state['bot_start'])).length();remaining=(pawn.get_actor_location()-ue.Vector(*state['goal'])).length()
   assert t-state['at']<25,dict(distance=distance,remaining=remaining)
   if remaining<100:
    bot.stop_movement();state['total_distance']+=distance
    event('navigation_goal_reached',t,index=state['goal_index'],distance_cm=distance,remaining_cm=remaining)
    state['goal_index']+=1
    if state['goal_index']<len(state['goals']):state.update(step='nav_pause',at=t)
    else:
     assert state['total_distance']>180
     bot.get_editor_property('BrainComponent').restart_logic();immunity(pawn,False);state['bot_immune']=False
     event('ai_followed_navigation_path',t,distance_cm=state['total_distance'],encounter_points_reached=3);state.update(step='clear_fights',at=t)
  elif state['step']=='clear_fights':
   for bot in actors(w,bot_cls):
    pawn=bot.get_controlled_pawn()
    if pawn and health(pawn) and not health(pawn).is_dead_or_dying() and pawn.get_name() not in state['killed']:
     state['killed'].add(pawn.get_name());damage(pawn,p);event('encounter_completed_by_diagnostic_damage',t,phase=phase);return
   bosses=actors(w,boss_cls)
   if phase==2 and len(bosses)==1 and bosses[0].is_gas_ready():
    if not widgets(pc,boss_ui_cls):return
    event('boss_and_health_hud_ready',t);damage(bosses[0],p);state.update(step='clear_result',at=t)
  elif state['step']=='clear_result':
   assert t-state['at']<25,'Clear result or owned-object cleanup did not finish'
   if not mode.has_run_ended() or not mode.was_run_successful() or actors(w,boss_cls) or actors(w,bot_cls):return
   layer=one(pc,result_cls)
   if not layer.is_activated():return
   event('level1_clear_and_cleanup',t,killed_bots=len(state['killed']))
   assert mode.request_exit_stage() and mode.is_stage_travel_pending()
   state.update(step='result_return_wait',travel_at=t)
  elif state['step']=='returned':
   assert not mode.has_run_ended() and phase==0 and health(p).get_health()==health(p).get_max_health()
   assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
   v=p.get_actor_location();state.update(step='returned_move',at=t,position=[v.x,v.y,v.z]);event('result_return_started_fresh_run',t)
  elif state['step']=='returned_move':
   input_system(pc).inject_input_vector_for_action(move,ue.Vector(0,1,0),[],[])
   if t-state['at']>.6:
    input_system(pc).inject_input_vector_for_action(move,ue.Vector(),[],[])
    distance=(p.get_actor_location()-ue.Vector(*state['position'])).length();assert distance>20
    lp=ui.call_method('GetLocalPlayerFromController',args=(pc,));assert ui.call_method('PushContentToLayer_ForPlayer',args=(lp,tag('UI.Layer.Menu'),menu_cls))
    state.update(step='return_menu',at=t);event('movement_restored_after_return',t,distance_cm=distance)
  elif state['step']=='return_menu' and t-state['at']>.6:
   menu=one(pc,menu_cls);assert menu.is_activated();menu.get_editor_property('ReturnButton').call_method('HandleButtonClicked')
   state.update(step='confirmation',at=t)
  elif state['step']=='confirmation' and t-state['at']>.5:
   dialog=one(pc,confirm_cls);assert dialog.is_activated()
   boxes=[x for x in ue.ObjectIterator(ue.DynamicEntryBox) if x.get_name()=='EntryBox_Buttons' and x.get_outer().get_outer()==dialog]
   assert len(boxes)==1
   buttons=list(boxes[0].get_all_entries());assert len(buttons)==2
   buttons[0].call_method('HandleButtonClicked');assert mode.is_stage_travel_pending()
   event('menu_confirmation_requested_same_map_return',t);state.update(step='menu_return_wait',travel_at=t)
  elif state['step']=='menu_returned':
   assert not mode.has_run_ended() and phase==0 and not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
   event('menu_return_started_fresh_run',t)
   bots=[b for b in actors(w,bot_cls) if b.get_controlled_pawn()]
   if len(bots)!=1:return
   immunity(p,False);state['immune']=False
   state.update(step='death_wait',at=t,dead_pawn_name=p.get_name())
   damage(p,bots[0].get_controlled_pawn());event('player_received_diagnostic_lethal_damage',t)
  elif state['step']=='death_restarted':
   assert not mode.has_run_ended() and phase==0 and health(p).get_health()==health(p).get_max_health()
   assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
   event('explicit_restart_after_death_started_fresh_run',t,ammo=item.get_stat_tag_stack_count(ammo_tag))
   immunity(p,False);state['immune']=False;finish(True)
 except Exception:result['failures'].append(traceback.format_exc());finish(False)
 finally:state['busy']=False
state['callback']=ue.register_slate_post_tick_callback(tick)
print('LEVEL1_PIE_ARMED')
