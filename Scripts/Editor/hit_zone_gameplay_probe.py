"""Bounded PIE shots at current HARU bot and BP_DC_Boss; no asset saves.

Moves runtime actors into free space, stops movement/AI, and injects real fire input.
Records a matching weapon-channel trace, real health/ammo and Cue Niagara instance.
Does not claim manual input, sound perception, rendered marker or network acceptance.
"""
import hashlib,json,re,time,traceback
from pathlib import Path
import unreal as u
P=Path('E:/Epic Games/UEProjects/DreamCatcher');D=P/'Saved/Diagnostics/HitZones20261018'
RUN=re.search(r'-DCHitZoneProbe=([A-Za-z0-9_]+)',u.SystemLibrary.get_command_line()).group(1)
OUT=D/('pie_'+RUN+'.json');assert not OUT.exists()
MAP='/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
state=dict(step='ready',wall=time.monotonic(),done=False,busy=False,index=0,inputs={})
result=dict(complete=False,shots=[],events=[],failures=[],asset_saves=0,physical_input_visual_audio_verified=False)
protected={str(p):[p.stat().st_size,p.stat().st_mtime_ns] for root in ['Content','Source','Config','Plugins/GameFeatures/DCStageUI/Content','Saved/SaveGames'] for p in (P/root).rglob('*') if p.is_file()}

def cls(n):return u.load_class(None,'/Script/DreamCatcher.'+n)
def tag(n):
 v=u.GameplayTag();assert v.import_text('(TagName="'+n+'")');return v
def health(a):return a.get_component_by_class(cls('DCLyraHealthComponent'))
def actors(w,c):return list(u.GameplayStatics.get_all_actors_of_class(w,c))
def event(n,**kw):
 result['events'].append(dict(event=n,**kw));print('HIT_ZONE_PIE '+json.dumps(result['events'][-1],ensure_ascii=False))
def inject(action,on):
 state['inputs'][action]=u.Vector(1 if on else 0,0,0)
 state['input'].inject_input_vector_for_action(action,state['inputs'][action],[],[])
data=u.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')
fire=data.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'),True)
aim=data.find_native_input_action_for_tag(tag('InputTag.Aim'),True)
ammo_tag=tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
math=u.get_default_object(u.load_class(None,'/Script/Engine.KismetMathLibrary'))
gas=u.get_default_object(u.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
game=u.get_default_object(u.load_class(None,'/Script/Engine.GameplayStatics'))
subs=u.get_default_object(u.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
cases=[('haru_body','spine_02',12),('haru_head','head',18),('miss','head',0),('boss_body','spine_03',12),('boss_head','head',18)]
result['raw_number_payload_verified']=False # The live Niagara graph consumes DamageInfo before the Python post-tick callback.

def ammo():return state['item'].get_stat_tag_stack_count(ammo_tag)
def numbers():
 comp=state['number'].get_editor_property('NiagaraComp')
 return list(u.NiagaraDataInterfaceArrayFunctionLibrary.get_niagara_array_vector4(comp,'DamageInfo')) if comp else []
def aimpoint():
 if state['index']==2:return state['pawn'].get_actor_location()+u.Vector(4000,0,8000)
 mesh=state['target'].get_component_by_class(u.SkeletalMeshComponent)
 return math.call_method('TransformLocation',args=(mesh.get_socket_transform(cases[state['index']][1],u.RelativeTransformSpace.RTS_WORLD),state['body_center']))
def begin(now):
 name,bone,expected=cases[state['index']]
 mesh=state['target'].get_component_by_class(u.SkeletalMeshComponent)
 pa=mesh.get_editor_property('PhysicsAssetOverride') or mesh.get_skinned_asset().get_editor_property('PhysicsAsset')
 bs=[b for b in u.ObjectIterator(u.load_class(None,'/Script/Engine.SkeletalBodySetup')) if b.get_outer()==pa and str(b.get_editor_property('BoneName'))==bone];assert len(bs)==1,(name,bone)
 geom=bs[0].get_editor_property('AggGeom');caps=list(geom.get_editor_property('SphylElems'));assert caps
 state.update(step='settle',at=now,body_center=caps[0].get_editor_property('Center'))
 event('case',name=name,bone=bone,body_material=str(bs[0].get_editor_property('PhysMaterial')),expected=expected)
def finish(ok):
 if state['done']:return
 state['done']=True
 if 'input' in state:
  inject(fire,False);inject(aim,False)
 result['protected_changes']=[p for p,s in protected.items() if not Path(p).exists() or [Path(p).stat().st_size,Path(p).stat().st_mtime_ns]!=s]
 result['new_savefiles']=[str(p) for p in (P/'Saved/SaveGames').rglob('*') if p.is_file() and str(p) not in protected]
 result['complete']=ok and not result['failures'] and not result['protected_changes'] and not result['new_savefiles']
 OUT.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 u.unregister_slate_post_tick_callback(state['callback'])
 if not result['complete']:u.log_error('Hit-zone PIE failed: '+str(result['failures']))
 event('complete',passed=result['complete'])
def tick(delta):
 if state['done'] or state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['wall']<150,'probe timeout'
  w=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_game_world()
  if not w or 'UEDPIE_' not in w.get_path_name():return
  assert w.get_path_name().startswith(MAP.rsplit('/',1)[0]+'/UEDPIE_')
  now=u.GameplayStatics.get_time_seconds(w)
  if state['step']=='ready':
   pc=u.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
   if not p or not health(p):return
   if not state.get('immune'):
    immune=u.GameplayTagContainer();assert immune.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
    assert gas.call_method('AddLooseGameplayTags',args=(p,immune,False));state['immune']=True
   if now<3:return
   qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent'));number=pc.get_component_by_class(cls('DCLyraNumberPopComponent_NiagaraText'))
   bots=[b for b in actors(w,cls('DCLyraPlayerBotController')) if b.get_controlled_pawn() and health(b.get_controlled_pawn())]
   reticles=[x for x in u.ObjectIterator(u.UserWidget) if x.get_class().get_name()=='W_Reticle_Rifle_C' and x.get_owning_player()==pc and not x.get_name().startswith('Default__')]
   if not qb or not qb.get_active_slot_item() or not bots or not reticles:
    if now-state.get('last_wait',0)>5:
     event('waiting',quickbar=bool(qb),bots=len(bots),reticles=len(reticles),number=bool(number));state['last_wait']=now
    return
   assert number,'NumberPop missing on current stage controller'
   assert number.get_editor_property('Style') is not None
   for bot in bots:
    bot.get_editor_property('BrainComponent').stop_logic('bounded hit-zone probe');bot.stop_movement()
   target=bots[0].get_controlled_pawn()
   for a in [p,target]:a.get_movement_component().stop_movement_immediately();a.get_movement_component().disable_movement()
   origin=p.get_actor_location()+u.Vector(0,0,6000)
   p.set_actor_location(origin,False,True);target.set_actor_location(origin+u.Vector(650,0,0),False,True);target.set_actor_rotation(u.Rotator(yaw=180),True)
   inp=subs.call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,u.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
   state.update(world=w,pc=pc,pawn=p,target=target,origin=origin,item=qb.get_active_slot_item(),input=inp,reticle=reticles[0],number=number,camera=u.GameplayStatics.get_player_camera_manager(w,0))
   hitwidget=reticles[0].get_editor_property('HitMarkerConfirmations')
   result['marker_zone_brushes']={k.export_text():v.export_text() for k,v in hitwidget.get_editor_property('PerHitMarkerZoneOverrideImages').items()}
   inject(aim,True);begin(now)
  point=aimpoint();cam=state['camera'].get_camera_location()
  rotation=math.call_method('FindLookAtRotation',args=(cam,point));state['pc'].set_control_rotation(rotation)
  elapsed=now-state['at']
  if state['step']=='settle' and elapsed>2 and state['reticle'].has_first_shot_accuracy():
   direction=(point-cam).normal();weaponloc=state['pawn'].get_actor_location()
   start=cam+direction*((weaponloc-cam).dot(direction))
   hit=u.SystemLibrary.line_trace_single(w,start,point+direction*100,u.TraceTypeQuery.cast(2),True,[state['pawn']],u.DrawDebugTrace.NONE,True,u.LinearColor(),u.LinearColor(),0)
   hittext=hit.export_text() if isinstance(hit,u.HitResult) else str(hit)
   name,bone,expected=cases[state['index']]
   if expected:assert ('WeakSpot' in hittext)==(expected==18),hittext
   state.update(step='shoot',at=now,before_health=health(state['target']).get_health(),before_ammo=ammo(),before_numbers=[v.export_text() for v in numbers()],trace=hittext)
   event('fire',case=name,trace=hittext);inject(fire,True)
  elif state['step']=='shoot' and ammo()<state['before_ammo']:
   inject(fire,False);state.update(step='observe',at=now)
  elif state['step']=='observe' and elapsed>.05:
   name,bone,expected=cases[state['index']];damage=state['before_health']-health(state['target']).get_health();ns=numbers()
   values=[dict(x=v.x,y=v.y,z=v.z,w=v.w) for v in ns]
   row=dict(case=name,damage=damage,expected=expected,ammo_used=state['before_ammo']-ammo(),numbers=values,weapon_channel_trace=state['trace'])
   result['shots'].append(row);event('shot_result',**row)
   assert abs(damage-expected)<.01 and row['ammo_used']==1,row
   comp=state['number'].get_editor_property('NiagaraComp')
   if expected:
    assert comp and comp.is_active()
    row['number_system']=comp.get_asset().get_path_name()
    assert row['number_system'].endswith('/NS_DamageNumbers.NS_DamageNumbers')
    location=comp.get_world_location();row['number_location']=location.export_text()
    match=re.search(r'ImpactPoint=\(X=([^,]+),Y=([^,]+),Z=([^\)]+)\)',state['trace']);assert match
    row['number_to_trace_distance']=(location-u.Vector(*map(float,match.groups()))).length()
    assert row['number_to_trace_distance']<2,row
   else:assert [v.export_text() for v in ns]==state['before_numbers'],row
   state.update(step='cooldown',at=now)
  elif state['step']=='cooldown' and elapsed>1:
   state['index']+=1
   if state['index']==len(cases):finish(True);return
   if state['index']==3:
    state['target'].set_actor_location(state['origin']+u.Vector(-3000,0,0),False,True)
    bp=u.load_asset('/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss')
    transform=u.Transform(location=state['origin']+u.Vector(650,0,0),rotation=u.Rotator(yaw=180))
    boss=game.call_method('BeginDeferredActorSpawnFromClass',args=(w,bp.generated_class(),transform,u.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,u.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    boss=game.call_method('FinishSpawningActor',args=(boss,transform,u.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT));assert boss.is_gas_ready()
    state['target']=boss
   begin(now)
  for action,value in state['inputs'].items():state['input'].inject_input_vector_for_action(action,value,[],[])
 except Exception:result['failures'].append(traceback.format_exc());finish(False)
 finally:state['busy']=False
state['callback']=u.register_slate_post_tick_callback(tick)
event('armed',PIE=True)
