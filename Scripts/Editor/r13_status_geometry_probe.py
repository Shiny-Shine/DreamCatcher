"""Measure only R13 status layout; diagnostic bot damage advances to the boss."""
import hashlib,json,re,time,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
run=re.search(r'-DCR13Geometry=(Before_[A-Z]|After_[A-Z])',ue.SystemLibrary.get_command_line()).group(1)
out=R/('status_geometry_'+run+'.json');assert not out.exists()
result=dict(complete=False,measurements={},failures=[],asset_saves=0,physical_input_or_visual_verified=False)
state=dict(done=False,busy=False,last=-1,wall=time.monotonic(),immune=False,killed=set(),boss_at=None)
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
protected={str(f):digest(f) for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Plugins/GameFeatures/DCStageUI/Content','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
def cls(n):return ue.load_class(None,'/Script/DreamCatcher.'+n)
def tag(n):
 v=ue.GameplayTag();assert v.import_text('(TagName="'+n+'")');return v
def tags(n):
 v=ue.GameplayTagContainer();assert v.import_text('(GameplayTags=((TagName="'+n+'")))');return v
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
def asc(p):return gas.call_method('GetAbilitySystemComponent',args=(p,))
def health(p):return p.get_component_by_class(cls('DCLyraHealthComponent'))
def widgets(pc,c):return [x for x in ue.ObjectIterator(ue.UserWidget) if x.get_class()==c and x.get_owning_player()==pc and not x.get_name().startswith('Default__')]
def belongs(obj,owner):
 while obj:
  if obj==owner:return True
  obj=obj.get_outer()
 return False
slate=ue.get_default_object(ue.load_class(None,'/Script/UMG.SlateBlueprintLibrary'))
def local_size(g):return slate.call_method('GetLocalSize',args=(g,))
def measure(widget,reference):
 g=widget.get_cached_geometry();size=local_size(g)
 points=[slate.call_method('AbsoluteToLocal',args=(reference.get_cached_geometry(),slate.call_method('LocalToAbsolute',args=(g,v)))) for v in [ue.Vector2D(0,0),size]]
 return dict(size=[size.x,size.y],bounds=[points[0].x,points[0].y,points[1].x,points[1].y])
def finish(ok):
 if state['done']:return
 state['done']=True
 w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world();pc=ue.GameplayStatics.get_player_controller(w,0) if w else None
 p=pc.get_controlled_pawn() if pc else None
 if p and state['immune']:gas.call_method('RemoveLooseGameplayTags',args=(p,tags('Gameplay.DamageImmunity'),False))
 result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
 result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
 result['complete']=ok and not result['failures'] and not result['protected_changed'] and result['ensure_count']==0
 out.write_text(json.dumps(result,indent=2),encoding='utf-8');ue.unregister_slate_post_tick_callback(state['callback'])
 if not result['complete']:ue.log_error('R13 geometry probe failed: '+str(out))
 print('R13_STATUS_GEOMETRY '+str(result))
def tick(delta):
 if state['done'] or state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['wall']<150,'Geometry timeout'
  w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
  if not w or 'UEDPIE_' not in w.get_path_name():return
  t=ue.GameplayStatics.get_time_seconds(w)
  if t-state['last']<.1:return
  state['last']=t;pc=ue.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
  if not p or not health(p) or health(p).get_health()<=0:return
  overlays=widgets(pc,cls('DCStageOverlayWidget'))
  if not overlays:return
  if not state['immune']:
   assert gas.call_method('AddLooseGameplayTags',args=(p,tags('Gameplay.DamageImmunity'),False));state['immune']=True
  if t<2:return
  for bot in ue.GameplayStatics.get_all_actors_of_class(w,cls('DCLyraPlayerBotController')):
   pawn=bot.get_controlled_pawn()
   if pawn and health(pawn) and health(pawn).get_health()>0 and pawn.get_path_name() not in state['killed']:
    state['killed'].add(pawn.get_path_name());a=asc(p)
    damage=ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
    spec=a.make_outgoing_spec(damage,1,a.make_effect_context())
    spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),10000.0))
    a.apply_gameplay_effect_spec_to_target(spec,asc(pawn));return
  bosses=widgets(pc,cls('DCBossHealthWidget'))
  if not bosses or not bosses[0].is_visible():return
  if state['boss_at'] is None:state['boss_at']=t;return
  if t-state['boss_at']<.5:return
  layouts=[x for x in ue.ObjectIterator(ue.UserWidget) if x.get_owning_player()==pc and x.get_class().get_name()=='W_ShooterHUDLayout_C'];assert len(layouts)==1
  layout=layouts[0];overlay=overlays[0]
  phase=[x for x in ue.ObjectIterator(ue.Border) if x.get_name()=='PhasePanel' and belongs(x,overlay)];assert len(phase)==1
  boss_panels=[x for x in ue.ObjectIterator(ue.Border) if belongs(x,bosses[0])];assert len(boss_panels)==1
  m={n:measure(obj,layout) for n,obj in [('layout',layout),('overlay',overlay),('phase_panel',phase[0]),('boss_panel',boss_panels[0])]};result['measurements']=m
  width,height=m['layout']['size'];assert width>600 and height>300
  if run.startswith('Before_'):
   assert m['overlay']['size'][0]<1 and m['boss_panel']['bounds'][0]<0 and m['phase_panel']['bounds'][0]<0,'Reported clipping not reproduced'
  else:
   assert abs(m['overlay']['size'][0]-width)<1 and abs(m['overlay']['size'][1]-height)<1
   for n in ['phase_panel','boss_panel']:
    left,top,right,bottom=m[n]['bounds'];assert 0<=left<right<=width and 0<=top<bottom<=height,(n,m[n])
    assert abs((left+right)*.5-width*.5)<1,(n,'not centered')
  finish(True)
 except Exception:result['failures'].append(traceback.format_exc());finish(False)
 finally:state['busy']=False
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R13_STATUS_GEOMETRY_ARMED')
