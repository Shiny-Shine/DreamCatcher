"""R13 UI-only combat integration: original killfeed/accolade and boss/result HUD.
Diagnostic damage/immunity, no repeated R12 pattern validation or asset saves.
"""
import json,re,time,hashlib,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
run=re.search(r'-DCR13CombatUI=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out=R/('combat_ui_'+run+'.json');assert not out.exists()
state=dict(step='ready',last=-1,wall=time.monotonic(),done=False,immune=False,busy=False,killed=set(),kills=0,accolade_seen=False)
result=dict(complete=False,events=[],failures=[],asset_saves=0,physical_weapon_or_visual_verified=False)
protected={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
def cls(n):return ue.load_class(None,'/Script/DreamCatcher.'+n)
def bp(p):return ue.load_class(None,p+'.'+p.rsplit('/',1)[1]+'_C')
def tag(n):
 v=ue.GameplayTag();assert v.import_text('(TagName="'+n+'")');return v
def tags(n):
 v=ue.GameplayTagContainer();assert v.import_text('(GameplayTags=((TagName="'+n+'")))');return v
def widgets(pc,c):return [x for x in ue.ObjectIterator(ue.UserWidget) if x.get_class()==c and x.get_owning_player()==pc and not x.get_name().startswith('Default__')]
def event(n,t,**data):result['events'].append(dict(event=n,time=t,**data));print('R13_COMBAT_UI_EVENT '+str(result['events'][-1]))
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
health_cls=cls('DCLyraHealthComponent');bot_cls=cls('DCLyraPlayerBotController');boss_cls=cls('DCBossCharacter')
overlay_cls=cls('DCStageOverlayWidget');boss_ui_cls=cls('DCBossHealthWidget');result_cls=cls('DCStageResultLayer')
feed_cls=bp('/Game/LyraMigration/UI/R13/ShooterCore/UserInterface/Notifications/EliminationFeed/W_EliminationFeedEntryWidget')
toast_cls=bp('/Game/LyraMigration/UI/R13/ShooterCore/UserInterface/Notifications/Accolades/W_AccoladeToast')
damage_cls=bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
def health(p):return p.get_component_by_class(health_cls) if p else None
def asc(p):return gas.call_method('GetAbilitySystemComponent',args=(p,))
def finish(ok):
 if state['done']:return
 state['done']=True
 w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world();pc=ue.GameplayStatics.get_player_controller(w,0) if w else None
 p=pc.get_controlled_pawn() if pc else None
 if p and state['immune']:gas.call_method('RemoveLooseGameplayTags',args=(p,tags('Gameplay.DamageImmunity'),False))
 result['protected_changed']=[f for f,h in protected.items() if not Path(f).is_file() or hashlib.sha256(Path(f).read_bytes()).hexdigest()!=h]
 result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count();result['complete']=ok and not result['failures'] and not result['protected_changed'] and result['ensure_count']==0
 out.write_text(json.dumps(result,indent=2),encoding='utf-8');ue.unregister_slate_post_tick_callback(state['callback'])
 if not result['complete']:ue.log_error('R13 combat UI failed: '+str(out))
 print('R13_COMBAT_UI '+str({k:result[k] for k in ['complete','failures','ensure_count']}))
def hit(target,source):
 a=asc(source);spec=a.make_outgoing_spec(damage_cls,1,a.make_effect_context())
 spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),10000.0))
 a.apply_gameplay_effect_spec_to_target(spec,asc(target))
def tick(delta):
 if state['done'] or state['busy']:return
 state['busy']=True
 try:
  assert time.monotonic()-state['wall']<200,'UI combat timeout'
  w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
  if not w or 'UEDPIE_' not in w.get_path_name():return
  t=ue.GameplayStatics.get_time_seconds(w)
  if t-state['last']<.08:return
  state['last']=t;pc=ue.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
  if not p or not health(p) or health(p).get_health()<=0:return
  mode=ue.GameplayStatics.get_game_mode(w)
  if not isinstance(mode,ue.DCStageRunGameMode) or not widgets(pc,overlay_cls):return
  if not state['immune']:
   assert gas.call_method('AddLooseGameplayTags',args=(p,tags('Gameplay.DamageImmunity'),False));state['immune']=True
  if widgets(pc,toast_cls):state['accolade_seen']=True
  if state['step']=='ready' and t>2:
   for bot in ue.GameplayStatics.get_all_actors_of_class(w,bot_cls):
    pawn=bot.get_controlled_pawn()
    if pawn and health(pawn) and health(pawn).get_health()>0 and pawn.get_path_name() not in state['killed']:
     state['killed'].add(pawn.get_path_name());hit(pawn,p);state['kills']+=1;event('player_killed_bot',t,index=state['kills']);return
   bosses=list(ue.GameplayStatics.get_all_actors_of_class(w,boss_cls))
   if bosses and state['kills']==2:
    assert len(widgets(pc,feed_cls))>=2
    state.update(step='boss',at=t);event('two_killfeed_entries',t,accolade_seen=state['accolade_seen'])
  elif state['step']=='boss' and t-state['at']>1.0:
   bosses=list(ue.GameplayStatics.get_all_actors_of_class(w,boss_cls));assert len(bosses)==1
   displays=widgets(pc,boss_ui_cls);assert len(displays)==1 and displays[0].is_visible()
   assert state['accolade_seen'],'Original chain accolade was not displayed'
   event('boss_health_and_original_accolade_displayed',t,health=health(bosses[0]).get_health())
   hit(bosses[0],p);state.update(step='result',at=t)
  elif state['step']=='result' and t-state['at']>3:
   assert mode.has_run_ended() and mode.was_run_successful()
   assert any(x.is_activated() for x in widgets(pc,result_cls))
   assert all(not x.is_visible() for x in widgets(pc,boss_ui_cls))
   assert not list(ue.GameplayStatics.get_all_actors_of_class(w,boss_cls))
   event('NPC_boss_elimination_clear_UI',t,killfeed_widgets=len(widgets(pc,feed_cls)));finish(True)
 except Exception:result['failures'].append(traceback.format_exc());finish(False)
 finally:state['busy']=False
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R13_COMBAT_UI_ARMED')
