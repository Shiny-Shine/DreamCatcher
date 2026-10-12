"""Post-build real PIE checks; no editor graph/CDO readers or asset saves.

-DCR11Probe=<fresh receipt> -DCR11Scenario=clear|fail-retry-return
Use >=90s Project.Maps.PIE duration. Controlled GE damage is not a weapon-kill test.
Do not retain World/Actor/widget wrappers across OpenLevel; globals contain plain data only.
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
run=re.search(r'-DCR11Probe=([A-Za-z0-9_]+)',cmd).group(1)
scenario=re.search(r'-DCR11Scenario=(clear|fail-retry-return)(?:\s|$)',cmd).group(1)
out=P/'Saved/Diagnostics/R11'/('pie_'+run+'.json');assert not out.exists()
ROOT='/Game/DreamCatcher/GAS/Test/R11'
RETURN_ROOT='/Game/DreamCatcher/GAS/Test/R10/LyraEncounter'
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
           (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/DreamCatcher/DreamCatcher_ENV/Map/Main')
           for p in root.rglob('*') if p.is_file()}
protected[str(P/'.git/index')]=hashlib.sha256((P/'.git/index').read_bytes()).hexdigest()
result=dict(complete=False,scenario=scenario,events=[],samples=[],failures=[],asset_saves=0,
            physical_input_verified=False,replication_tested=False,visual_quality_verified=False)
state=dict(wall=time.monotonic(),last=-1,done=False,step='first',immune=False,seen={},phases=[])


def cls(name):
    value=ue.load_class(None,'/Script/DreamCatcher.'+name);assert value,name;return value


def tag(name):
    value=ue.GameplayTag();assert value.import_text('(TagName="'+name+'")');return value


mode_class=cls('DCStageRunGameMode')
bot_class=cls('DCLyraPlayerBotController')
health_class=cls('DCLyraHealthComponent')
inventory_class=cls('DCInventoryManagerComponent')
quickbar_class=cls('DCLyraQuickBarComponent')
widget_class=cls('DCStageResultWidget')
spawner_class=cls('DCEnemySpawner')
library=ue.get_default_object(ue.load_class(None,'/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
widget_library=ue.get_default_object(ue.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
damage_class=ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
assert damage_class
immunity=ue.GameplayTagContainer();assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
self_destruct=ue.GameplayTagContainer();assert self_destruct.import_text('(GameplayTags=((TagName="Gameplay.Damage.SelfDestruct")))')
damage_tag=tag('SetByCaller.Damage');block_tag=tag('Gameplay.AbilityInputBlocked')
move=ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput').find_native_input_action_for_tag(tag('InputTag.Move'),True)


def event(name,t,**data): result['events'].append(dict(event=name,time=t,**data))
def actors(world,class_):return list(ue.GameplayStatics.get_all_actors_of_class(world,class_))
def widgets(world):return list(widget_library.call_method('GetAllWidgetsOfClass',args=(world,widget_class,True)))
def health(pawn):return pawn.get_component_by_class(health_class) if pawn else None


def damage(pawn):
    asc=library.call_method('GetAbilitySystemComponent',args=(pawn,));assert asc
    spec=asc.make_outgoing_spec(damage_class,1,asc.make_effect_context())
    spec=library.call_method('AssignTagSetByCallerMagnitude',args=(spec,damage_tag,health(pawn).get_max_health()))
    spec=library.call_method('AddAssetTags',args=(spec,self_destruct))
    asc.apply_gameplay_effect_spec_to_self(spec)


def immune(pawn,enabled):
    if state['immune']==enabled:return
    assert pawn
    assert library.call_method('AddLooseGameplayTags' if enabled else 'RemoveLooseGameplayTags',args=(pawn,immunity,False))
    state['immune']=enabled


def subsystem(pc):
    return ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method(
        'GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))


def player_ready(pc,pawn):
    if not pc or not pawn or not health(pawn) or health(pawn).get_max_health()<=0:return False
    inv=pc.get_component_by_class(inventory_class)
    qb=pc.get_component_by_class(quickbar_class)
    return inv and qb and len(inv.get_all_items())==1 and qb.get_active_slot_item() is not None


def finish(ok):
    if state['done']:return
    state['done']=True
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    pc=ue.GameplayStatics.get_player_controller(world,0) if world else None
    pawn=pc.get_controlled_pawn() if pc else None
    if pc and pawn:
        subsystem(pc).inject_input_vector_for_action(move,ue.Vector(),[],[])
        if state['immune']:immune(pawn,False)
    result['protected_changed']=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
    result['complete']=ok and not result['failures'] and not result['protected_changed'] and result['ensure_count']==0
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not result['complete']:ue.log_error('R11 stage probe failed: '+str(out))
    print('R11_STAGE_PIE '+json.dumps({k:result[k] for k in ['complete','scenario','protected_changed','failures']}))


def tick(delta):
    if state['done']:return
    try:
        assert time.monotonic()-state['wall']<240,'PIE/travel timeout'
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():return
        t=ue.GameplayStatics.get_time_seconds(world)
        if state['step']=='reload_wait':
            if t>=state['travel_at']-1:return
            state.update(step='retry_ready',last=-1,immune=False)
            event('reload_world',t)
        if state['step']=='return_wait':
            if not world.get_path_name().startswith(RETURN_ROOT+'/UEDPIE_'):return
            state.update(step='return_ready',last=-1,immune=False)
        if t-state['last']<.1:return
        state['last']=t
        pc=ue.GameplayStatics.get_player_controller(world,0)
        pawn=pc.get_controlled_pawn() if pc else None
        mode=ue.GameplayStatics.get_game_mode(world)
        if state['step']=='return_ready':
            if pawn and health(pawn) and health(pawn).get_max_health()>0:immune(pawn,True)
            if not player_ready(pc,pawn):return
            assert not widgets(world)
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
            asc=library.call_method('GetAbilitySystemComponent',args=(pawn,))
            assert not asc.has_matching_gameplay_tag(block_tag)
            event('return_map_ready',t,map=world.get_path_name(),health=health(pawn).get_health())
            finish(True);return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_')
        if not mode:return
        director=mode.get_stage_director()
        if not director:return
        phase=director.get_current_phase_index()
        ended=mode.has_run_ended()
        widget=mode.get_stage_widget()
        result['samples'].append(dict(time=t,step=state['step'],phase=phase,ended=ended,success=mode.was_run_successful(),bots=len(actors(world,bot_class))))
        if state['step']=='first' and player_ready(pc,pawn) and phase>=0:
            assert pc.get_editor_property('PlayerState').get_team_id()==1
            assert len(widgets(world))==1
            event('first_ready',t,health=health(pawn).get_health())
            state['step']='clear' if scenario=='clear' else 'fail'
        if state['step']=='clear':
            if not ended:
                assert player_ready(pc,pawn)
                immune(pawn,True)
                if not state['phases'] or state['phases'][-1]!=phase:state['phases'].append(phase)
                for bot in actors(world,bot_class):
                    enemy=bot.get_controlled_pawn()
                    if not enemy or health(enemy).is_dead_or_dying():continue
                    key=enemy.get_name();row=state['seen'].setdefault(key,dict(first=t,phase=phase,path=False,loadout=False))
                    assert bot.get_editor_property('PlayerState').get_team_id()==2
                    inv=bot.get_component_by_class(inventory_class)
                    row['loadout']|=bool(inv and len(inv.get_all_items())==1)
                    if not row['path']:
                        systems=[n for n in ue.ObjectIterator(ue.NavigationSystemV1) if n.get_outer()==world]
                        assert len(systems)==1
                        nav=systems[0]
                        # Calling the static Python wrapper on a Within=World CDO makes
                        # the editor script guard ask that CDO for its World (ensure).
                        path=nav.call_method('FindPathToLocationSynchronously',args=(world,pawn.get_actor_location(),enemy.get_actor_location(),pawn,None))
                        row['path']=bool(path and path.is_valid() and not path.is_partial())
                    if t-row['first']>2 and row['loadout'] and row['path']:
                        event('kill_phase_enemy',t,phase=phase,enemy=key)
                        damage(enemy)
                    assert t-row['first']<15,'Enemy loadout/navigation did not become ready'
            else:
                assert mode.was_run_successful()
                state.setdefault('ended_at',t)
                if not widget or not widget.is_result_visible():
                    assert t-state['ended_at']<3,'Result UI did not appear'
                    return
                assert state['phases']==[0,1,2] and len(state['seen'])==3
                assert all(r['path'] and r['loadout'] for r in state['seen'].values())
                spawners=actors(world,spawner_class)
                assert len(spawners)==3 and all(s.get_spawned_count()==1 and s.is_spawner_cleared() for s in spawners)
                if not actors(world,bot_class):
                    event('clear_and_cleanup',t,phases=state['phases'],enemies=3)
                    finish(True);return
        elif state['step']=='fail' and player_ready(pc,pawn) and t>2:
            event('player_gas_death',t)
            damage(pawn);state.update(step='failed_wait',death_at=t)
        elif state['step']=='failed_wait' and ended and t-state['death_at']>8.3:
            assert not mode.was_run_successful() and widget and widget.is_result_visible()
            assert not actors(world,bot_class) and not actors(world,spawner_class)
            assert pawn is None,'Original player death cleanup should finish without automatic respawn'
            assert mode.request_restart_stage()
            assert not mode.request_restart_stage(),'Duplicate travel request accepted'
            event('explicit_retry',t)
            state.update(step='reload_wait',travel_at=t,last=-1,immune=False)
            return # No World/Actor/widget references escape this callback.
        elif state['step']=='retry_ready' and player_ready(pc,pawn) and phase>=0:
            assert not ended and health(pawn).get_health()==health(pawn).get_max_health()
            assert not pc.is_move_input_ignored() and not pc.is_look_input_ignored()
            assert not library.call_method('GetAbilitySystemComponent',args=(pawn,)).has_matching_gameplay_tag(block_tag)
            assert len(widgets(world))==1
            immune(pawn,True)
            p=pawn.get_actor_location();state.update(step='retry_move',move_at=t,position=[p.x,p.y,p.z])
            event('retry_player_ready',t,phase=phase,health=health(pawn).get_health())
        elif state['step']=='retry_move':
            subsystem(pc).inject_input_vector_for_action(move,ue.Vector(0,1,0),[],[])
            if t-state['move_at']>.5:
                subsystem(pc).inject_input_vector_for_action(move,ue.Vector(),[],[])
                distance=(pawn.get_actor_location()-ue.Vector(*state['position'])).length()
                assert distance>20,'Input/movement did not recover after retry'
                immune(pawn,False)
                event('retry_movement',t,distance_cm=distance)
                director.fail_stage();state['step']='exit_ready'
        elif state['step']=='exit_ready' and ended and widget and widget.is_result_visible():
            assert mode.request_exit_stage()
            event('explicit_return',t)
            state.update(step='return_wait',last=-1)
            return
    except Exception:
        result['failures'].append(traceback.format_exc());finish(False)


state['callback']=ue.register_slate_post_tick_callback(tick)
print('R11_STAGE_PIE_ARMED')
