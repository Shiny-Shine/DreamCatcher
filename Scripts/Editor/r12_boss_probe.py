"""Actual standalone PIE after build/setup; use >=120s Project.Maps.PIE.
-DCR12Probe=<fresh receipt> -DCR12Scenario=combat|fail-retry-return
Controlled GE damage/immunity/input are diagnostics, not user weapon/visual acceptance.
Keep only plain data across OpenLevel; no world/actor/widget globals or bound actor delegates.
"""
import hashlib
import json
import re
import time
import traceback
from pathlib import Path
import unreal as ue

P=Path('E:/Epic Games/UEProjects/DreamCatcher')
cmd=ue.SystemLibrary.get_command_line()
run=re.search(r'-DCR12Probe=([A-Za-z0-9_]+)',cmd).group(1)
scenario=re.search(r'-DCR12Scenario=(combat|fail-retry-return)(?:\s|$)',cmd).group(1)
out=P/'Saved/Diagnostics/R12'/('pie_'+run+'.json');assert not out.exists()
ROOT='/Game/DreamCatcher/GAS/Test/R12'
RETURN='/Game/DreamCatcher/GAS/Test/R10/LyraEncounter'
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
    ('Source','Config','Content/DreamCatcher/GAS/Test','Content/DreamCatcher/DreamCatcher_ENV/Map/Main')
    for p in (P/root).rglob('*') if p.is_file()}
protected[str(P/'.git/index')]=hashlib.sha256((P/'.git/index').read_bytes()).hexdigest()
result=dict(complete=False,scenario=scenario,events=[],failures=[],asset_saves=0,physical_input_verified=False,visual_quality_verified=False,replication_tested=False)
state=dict(step='opening',last=-1,wall=time.monotonic(),immune=False,done=False,seen={1:set(),2:set()},recovered=set(),busy=False)


def cls(name):
    value=ue.load_class(None,'/Script/DreamCatcher.'+name);assert value;return value
def tag(name):
    value=ue.GameplayTag();assert value.import_text('(TagName="'+name+'")');return value
def container(name):
    value=ue.GameplayTagContainer();assert value.import_text('(GameplayTags=((TagName="'+name+'")))');return value
def actors(w,c):return list(ue.GameplayStatics.get_all_actors_of_class(w,c))
def event(name,t,**data):result['events'].append(dict(event=name,time=t,**data))


boss_class=cls('DCBossCharacter');bot_class=cls('DCLyraPlayerBotController')
health_class=cls('DCLyraHealthComponent');hud_class=cls('DCBossHealthWidget');stage_ui=cls('DCStageResultWidget')
warning_class=cls('DCBossTelegraph');spawner_class=cls('DCEnemySpawner')
inventory_class=cls('DCInventoryManagerComponent');quickbar_class=cls('DCLyraQuickBarComponent')
line_class=cls('DCGameplayAbility_BossLineShot');ground_class=cls('DCGameplayAbility_BossGroundStrike')
gas=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
ui=ue.get_default_object(ue.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
subsystems=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary'))
input_class=ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')
damage_class=ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
immune_tags=container('Gameplay.DamageImmunity');self_tags=container('Gameplay.Damage.SelfDestruct')
damage_tag=tag('SetByCaller.Damage');blocked=tag('Gameplay.AbilityInputBlocked')
move=ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput').find_native_input_action_for_tag(tag('InputTag.Move'),True)


def health(p):return p.get_component_by_class(health_class) if p else None
def asc(p):return gas.call_method('GetAbilitySystemComponent',args=(p,))
def widgets(w,c):return list(ui.call_method('GetAllWidgetsOfClass',args=(w,c,True)))
def input_(pc):return subsystems.call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,input_class))
def ready(pc,p):
    if not pc or not p or not health(p) or health(p).get_max_health()<=0:return False
    inv=pc.get_component_by_class(inventory_class);qb=pc.get_component_by_class(quickbar_class)
    return inv and qb and len(inv.get_all_items())==1 and qb.get_active_slot_item() is not None
def immune(p,value):
    if state['immune']==value:return
    assert gas.call_method('AddLooseGameplayTags' if value else 'RemoveLooseGameplayTags',args=(p,immune_tags,False))
    state['immune']=value
def damage(p,amount=None,source=None):
    # NPC-owned ASC has no PlayerState. Attribute boss kills to the real player,
    # as a weapon hit does, rather than sending an NPC self-elimination to Lyra's reticle.
    component=asc(source if source is not None else p);assert component
    spec=component.make_outgoing_spec(damage_class,1,component.make_effect_context())
    spec=gas.call_method('AssignTagSetByCallerMagnitude',args=(spec,damage_tag,amount if amount is not None else health(p).get_max_health()))
    if source is None:
        spec=gas.call_method('AddAssetTags',args=(spec,self_tags))
    component.apply_gameplay_effect_spec_to_target(spec,asc(p))
def attacks(b,c):
    return [a for a in ue.ObjectIterator(c) if not a.get_name().startswith('Default__') and a.get_avatar_actor_from_actor_info()==b]


def finish(ok):
    if state['done']:return
    state['done']=True
    w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    pc=ue.GameplayStatics.get_player_controller(w,0) if w else None
    p=pc.get_controlled_pawn() if pc else None
    if pc and p:
        input_(pc).inject_input_vector_for_action(move,ue.Vector(),[],[])
        if state['immune']:immune(p,False)
    result['protected_changed']=[p for p,h in protected.items() if not Path(p).is_file() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
    result['complete']=ok and not result['failures'] and not result['protected_changed'] and result['ensure_count']==0
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not result['complete']:ue.log_error('R12 boss probe failed: '+str(out))
    print('R12_BOSS_PIE '+str({k:result[k] for k in ('complete','scenario','ensure_count','failures')}))


def tick(delta):
    if state['done'] or state['busy']:return
    state['busy']=True
    try:
        assert time.monotonic()-state['wall']<280,'PIE/travel timeout'
        w=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not w or 'UEDPIE_' not in w.get_path_name():return
        t=ue.GameplayStatics.get_time_seconds(w)
        if state['step']=='reload':
            if t>=state['travel_at']-1:return
            state.update(step='retry',last=-1,immune=False);event('world_reloaded',t)
        if state['step']=='return':
            if not w.get_path_name().startswith(RETURN+'/UEDPIE_'):return
            state.update(step='returned',last=-1,immune=False)
        if t-state['last']<.05:return
        state['last']=t
        pc=ue.GameplayStatics.get_player_controller(w,0);p=pc.get_controlled_pawn() if pc else None
        mode=ue.GameplayStatics.get_game_mode(w)
        if state['step']=='returned':
            if p and health(p) and health(p).get_max_health()>0:immune(p,True)
            if not ready(pc,p):return
            assert not widgets(w,hud_class) and not widgets(w,stage_ui)
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored() and not asc(p).has_matching_gameplay_tag(blocked)
            event('returned_to_r10',t,health=health(p).get_health());finish(True);return
        assert w.get_path_name().startswith(ROOT+'/UEDPIE_')
        if not mode or not mode.get_stage_director():return
        director=mode.get_stage_director();phase=director.get_current_phase_index()
        if state['step'] in ('opening','boss','transition','phase2') and ready(pc,p):immune(p,True)
        if state['step']=='opening':
            if not ready(pc,p):return
            assert not mode.has_run_ended(),'Run ended before boss setup'
            if phase<2:
                for bot in actors(w,bot_class):
                    pawn=bot.get_controlled_pawn()
                    if pawn and health(pawn) and not health(pawn).is_dead_or_dying() and t>1:
                        damage(pawn,source=p)
                        event('player_credited_opening_kill',t,enemy=pawn.get_name())
                return
            bosses=actors(w,boss_class)
            if len(bosses)!=1:return
            b=bosses[0]
            assert b.is_gas_ready() and b.get_editor_property('TeamId')==2
            assert len(widgets(w,hud_class))==1
            event('boss_ready',t,health=health(b).get_health())
            state.update(step='boss',boss_at=t)
        if state['step'] in ('boss','transition','phase2'):
            bosses=actors(w,boss_class);assert len(bosses)==1
            b=bosses[0];component=b.get_boss_phase_component();boss_phase=component.get_boss_phase()
            assert t-state['boss_at']<65,'Boss patterns stalled'
            line=attacks(b,line_class);ground=attacks(b,ground_class)
            for label,abilities in [('line',line),('ground',ground)]:
                for a in abilities:
                    attack_phase=a.get_attack_phase()
                    if attack_phase==ue.DCEnemyAttackPhase.WINDUP:
                        assert actors(w,warning_class),'Visible warning actor missing'
                        if label not in state['seen'][boss_phase]:
                            state['seen'][boss_phase].add(label);event('pattern_windup',t,pattern=label,phase=boss_phase)
                    if boss_phase==2 and attack_phase==ue.DCEnemyAttackPhase.RECOVERY:state['recovered'].add(label)
            if state['step']=='boss' and state['seen'][1]=={'line','ground'}:
                if scenario=='combat':
                    damage(b,health(b).get_max_health()*.51,source=p)
                    event('phase_threshold_damage',t);state['step']='transition';return
                immune(p,False);damage(p,source=b);event('player_death_during_boss',t)
                state.update(step='death_wait',death_at=t);return
            if state['step']=='transition':
                assert boss_phase==2
                if component.get_boss_run_state()==ue.DCBossRunState.TRANSITIONING:
                    assert not actors(w,warning_class);state['transition_seen']=True
                else:
                    assert state.get('transition_seen') and component.get_boss_run_state()==ue.DCBossRunState.FIGHTING
                    event('phase_two_ready',t);state['step']='phase2'
            if state['step']=='phase2' and state['recovered']=={'line','ground'}:
                damage(b,source=p);event('boss_lethal_damage',t);state['step']='clear_wait';return
        elif state['step']=='clear_wait':
            assert mode.has_run_ended() and mode.was_run_successful()
            if actors(w,boss_class) or actors(w,bot_class) or actors(w,warning_class):return
            assert not widgets(w,hud_class) and mode.get_stage_widget().is_result_visible()
            assert all(s.is_spawner_cleared() and s.get_spawned_count()==1 for s in actors(w,spawner_class))
            event('boss_clear_and_cleanup',t);finish(True)
        elif state['step']=='death_wait' and t-state['death_at']>8.3:
            assert mode.has_run_ended() and not mode.was_run_successful()
            assert not actors(w,boss_class) and not actors(w,bot_class) and not actors(w,warning_class) and not actors(w,spawner_class)
            assert not widgets(w,hud_class) and p is None
            assert mode.request_restart_stage() and not mode.request_restart_stage()
            event('explicit_retry',t);state.update(step='reload',travel_at=t,last=-1,immune=False);return
        elif state['step']=='retry' and ready(pc,p) and phase>=0:
            assert not mode.has_run_ended() and health(p).get_health()==health(p).get_max_health()
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored() and not asc(p).has_matching_gameplay_tag(blocked)
            assert not widgets(w,hud_class) and len(widgets(w,stage_ui))==1
            immune(p,True);v=p.get_actor_location()
            state.update(step='move',move_at=t,position=[v.x,v.y,v.z]);event('retry_ready',t)
        elif state['step']=='move':
            input_(pc).inject_input_vector_for_action(move,ue.Vector(0,1,0),[],[])
            if t-state['move_at']>.5:
                input_(pc).inject_input_vector_for_action(move,ue.Vector(),[],[])
                distance=(p.get_actor_location()-ue.Vector(*state['position'])).length();assert distance>20
                event('retry_move',t,distance_cm=distance)
                immune(p,False);director.fail_stage();state['step']='exit'
        elif state['step']=='exit' and mode.has_run_ended() and mode.get_stage_widget().is_result_visible():
            assert mode.request_exit_stage();event('explicit_return',t);state.update(step='return',last=-1);return
    except Exception:
        result['failures'].append(traceback.format_exc());finish(False)
    finally:state['busy']=False


state['callback']=ue.register_slate_post_tick_callback(tick)
print('R12_BOSS_PIE_ARMED')
