"""R14-2A affected-path regression only: equipment/fire, camera and player death.
Uses existing R7 input and R13 diagnostic damage patterns; never saves assets.
"""
import hashlib,json,re,time,traceback,math
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R14'
run=re.search(r'-DCR14Probe=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out=R/('cleanup_pie_'+run+'.json');assert not out.exists()
state=dict(step='ready',done=False,busy=False,wall=time.monotonic(),immune=False,inputs={},callbacks=[])
result=dict(complete=False,events=[],failures=[],asset_saves=0,physical_input_or_visual_verified=False)
protected={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
def cls(n):return ue.load_class(None,'/Script/DreamCatcher.'+n)
def tag(n):
 v=ue.GameplayTag();assert v.import_text('(TagName="'+n+'")');return v
def tags(n):
 v=ue.GameplayTagContainer();assert v.import_text('(GameplayTags=((TagName="'+n+'")))');return v
def event(n,**values):
 result['events'].append(dict(event=n,**values));print('R14_CLEANUP_PIE_EVENT '+str(result['events'][-1]))
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
config=ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput')
fire=config.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'),True)
aim=config.find_native_input_action_for_tag(tag('InputTag.Aim'),True);assert fire and aim
ammo_tag=tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
def set_input(action,on):state['inputs'][action]=ue.Vector(1 if on else 0,0,0)
def next_step(n,t):state.update(step=n,at=t)
def finish(ok):
 if state['done']:return
 state['done']=True
 if state.get('subsystem'):
  for action in state['inputs']:state['subsystem'].inject_input_vector_for_action(action,ue.Vector(),[],[])
 result['protected_changes']=[p for p,h in protected.items() if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
 result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
 result['complete']=ok and not result['failures'] and not result['protected_changes'] and result['ensure_count']==0
 out.write_text(json.dumps(result,indent=2),encoding='utf-8');ue.unregister_slate_post_tick_callback(state['callback'])
 if not result['complete']:ue.log_error('R14 cleanup PIE failed: '+str(out))
def tick(delta):
 if state['done'] or state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['wall']<160,'Cleanup PIE timeout'
  w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
  if not w or 'UEDPIE_' not in w.get_path_name():return
  t=ue.GameplayStatics.get_time_seconds(w)
  if state['step']=='ready':
   pc=ue.GameplayStatics.get_player_controller(w,0);pawn=pc.get_controlled_pawn() if pc else None
   qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent')) if pc else None
   if not pawn or not qb or not qb.get_active_slot_item() or t<1.5:return
   health=pawn.get_component_by_class(cls('DCLyraHealthComponent'))
   if not health or health.is_dead_or_dying():return
   asc=gas.call_method('GetAbilitySystemComponent',args=(pawn,))
   assert gas.call_method('AddLooseGameplayTags',args=(pawn,tags('Gameplay.DamageImmunity'),False));state['immune']=True
   equipment=pawn.get_component_by_class(cls('DCLyraEquipmentManagerComponent'))
   weapons=equipment.get_equipment_instances_of_type(cls('DCLyraWeaponInstance'));assert len(weapons)==1
   legacy=pawn.get_component_by_class(cls('DCEquipmentManagerComponent'));assert legacy and legacy.get_equipment_count()==0
   item=qb.get_active_slot_item();assert weapons[0].get_instigator()==item
   subsystem=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
   state.update(world=w,pc=pc,pawn=pawn,qb=qb,item=item,health=health,asc=asc,subsystem=subsystem,ammo=item.get_stat_tag_stack_count(ammo_tag),hip=pawn.get_gas_aim_mode())
   camera=pawn.get_component_by_class(cls('DCCameraComponent'));assert camera and camera.is_active()
   fov=pc.player_camera_manager.get_fov_angle();assert math.isfinite(fov) and 10<fov<180
   event('original_loadout_and_camera',pawn=pawn.get_class().get_path_name(),equipment_count=1,legacy_equipment_count=0,ammo=state['ammo'],fov=fov)
   def death_started(actor):event('original_death_started',time=ue.GameplayStatics.get_time_seconds(w))
   def death_finished(actor):event('original_death_finished',time=ue.GameplayStatics.get_time_seconds(w))
   def destroyed(actor):event('player_pawn_destroyed',time=ue.GameplayStatics.get_time_seconds(w))
   state['callbacks']=[death_started,death_finished,destroyed]
   health.get_editor_property('OnDeathStarted').add_callable(death_started)
   health.get_editor_property('OnDeathFinished').add_callable(death_finished)
   pawn.get_editor_property('OnDestroyed').add_callable(destroyed)
   set_input(fire,True);next_step('fire',t)
  elif state['step']=='fire' and t-state['at']>.4:
   ammo=state['item'].get_stat_tag_stack_count(ammo_tag);assert 0<ammo<state['ammo'],ammo
   set_input(fire,False);event('original_fire_consumed_ammo',before=state['ammo'],after=ammo);next_step('release',t)
  elif state['step']=='release' and t-state['at']>.3:
   set_input(aim,True);next_step('aim',t)
  elif state['step']=='aim' and t-state['at']>.7:
   assert state['pawn'].get_gas_aim_mode()!=state['hip'];event('aim_enter',mode=str(state['pawn'].get_gas_aim_mode()))
   set_input(aim,False);next_step('aim_release',t)
  elif state['step']=='aim_release' and t-state['at']>.7:
   assert state['pawn'].get_gas_aim_mode()==state['hip'];event('aim_return_to_hip')
   pawn=state['pawn'];assert gas.call_method('RemoveLooseGameplayTags',args=(pawn,tags('Gameplay.DamageImmunity'),False));state['immune']=False
   damage=ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
   a=state['asc'];spec=a.make_outgoing_spec(damage,1,a.make_effect_context())
   spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),10000.0))
   spec=gas.call_method('AddAssetTags',args=(spec,tags('Gameplay.Damage.SelfDestruct')))
   a.apply_gameplay_effect_spec_to_target(spec,a);next_step('death',t)
  elif state['step']=='death' and t-state['at']>10:
   names={e['event'] for e in result['events']}
   assert {'original_death_started','original_death_finished','player_pawn_destroyed'}<=names,names
   mode=ue.GameplayStatics.get_game_mode(w);assert mode.has_run_ended() and not mode.was_run_successful()
   assert not state['pc'].get_controlled_pawn()
   assert not state['qb'].get_active_slot_item()
   event('failed_result_and_equipment_cleanup');finish(True);return
  if state.get('subsystem'):
   for action,value in state['inputs'].items():state['subsystem'].inject_input_vector_for_action(action,value,[],[])
 except Exception:result['failures'].append(traceback.format_exc());finish(False)
 finally:state['busy']=False
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R14_CLEANUP_PIE_ARMED')
