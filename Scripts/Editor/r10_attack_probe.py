"""One-map R10-2 live attack checks. Temporary world edits only; no asset saves."""
import hashlib
import json
import re
import traceback
from pathlib import Path
import unreal as ue

P=Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT='/Game/DreamCatcher/GAS/Test/R10/Attack'
run=re.search(r'-DCR10Run=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
trace_only='-DCR10AttackTraceOnly' in ue.SystemLibrary.get_command_line()
out=P/'Saved/Diagnostics/R10'/('attack_pie_'+run+'.json')
assert not out.exists()
scope={'__name__':'r10_attack_existing_rifle_helpers'}
exec(compile((P/'Scripts/Editor/r7_reticle_host_probe.py').read_text(encoding='utf-8-sig'),'existing_helpers','exec'),scope)
state=scope['state']
ue.unregister_slate_post_tick_callback(state['callback'])
cls,tag,library=scope['cls'],scope['tag'],scope['ability_library']
enemy_class=ue.load_class(None,ROOT+'/BP_DC_GASEnemyRanged.BP_DC_GASEnemyRanged_C')
attack_class=ue.load_class(None,ROOT+'/GA_DC_EnemyRangedAttack.GA_DC_EnemyRangedAttack_C')
damage_class=ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
dash=scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ability.Dash'),True)
block_tags=ue.GameplayTagContainer()
assert block_tags.import_text('(GameplayTags=((TagName="Gameplay.AbilityInputBlocked")))')
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
    (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test/R10',P/'Content/DreamCatcher/GAS/Test/R5/Integration')
    for p in root.rglob('*') if p.is_file()}
result=dict(complete=False,failures=[],events=[],samples=[],asset_saves=0,deaths=0,clears=0,encounters=0,
    avoided_damage=0,physical_inputs_tested=False,replication_tested=False,visual_quality_verified=False,audio_verified=False)
state.update(phase='waiting',inputs={},last_sample=-1,defense_tasks=set(),blocked=False)


def now(): return ue.GameplayStatics.get_time_seconds(state['world'])


def record(event,**values):
    row=dict(event=event,time=now(),**values)
    result['events'].append(row)
    print('R10_ATTACK_PIE '+json.dumps(row))


def stage(name):
    state.update(phase=name,since=now())
    record('stage',name=name)


def counts():
    a=state['attack']
    return dict(intent=a.get_editor_property('TestIntentCount'),shot=a.get_editor_property('TestShotCount'),end=a.get_editor_property('TestEndCount'))


def hp(): return state['health'].get_health()


def block(on):
    if state['blocked']==on: return
    assert library.call_method('AddLooseGameplayTags' if on else 'RemoveLooseGameplayTags',args=(state['enemy'],block_tags,False))
    state['blocked']=on


def request(name):
    state.update(before=counts(),hp_before=hp())
    block(False)
    stage(name+'_intent')


def reset_player():
    assert not state['asc'].has_matching_gameplay_tag(tag('Event.Movement.Dash'))
    state['pawn'].set_actor_location(state['origin'],False,True)
    state['pawn'].get_component_by_class(ue.CharacterMovementComponent).stop_movement_immediately()
    state['pc'].set_control_rotation(ue.Rotator())


def cover():
    transform=ue.Transform(location=ue.Vector(325,0,150),scale=ue.Vector(.2,4,4))
    game=ue.get_default_object(ue.load_class(None,'/Script/Engine.GameplayStatics'))
    scale=ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT
    actor=game.call_method('BeginDeferredActorSpawnFromClass',args=(state['world'],ue.StaticMeshActor,transform,ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,scale))
    assert actor
    mesh=actor.get_component_by_class(ue.StaticMeshComponent)
    mesh.set_mobility(ue.ComponentMobility.MOVABLE)
    mesh.set_static_mesh(ue.load_asset('/Engine/BasicShapes/Cube'))
    mesh.set_collision_response_to_all_channels(ue.CollisionResponseType.cast(2))
    mesh.set_collision_enabled(ue.CollisionEnabled.QUERY_AND_PHYSICS)
    game.call_method('FinishSpawningActor',args=(actor,transform,scale))
    state['wall']=actor


def damage_enemy(amount):
    source=state['asc']
    target=library.call_method('GetAbilitySystemComponent',args=(state['enemy'],))
    spec=source.make_outgoing_spec(damage_class,1,source.make_effect_context())
    spec=library.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),float(amount)))
    source.apply_gameplay_effect_spec_to_target(spec,target)


def on_avoided(data):
    result['avoided_damage']+=1
    record('dash_blocked_real_damage',magnitude=data.get_editor_property('EventMagnitude'))


def on_death(actor):
    result['deaths']+=1
    state['death_time']=now()
    record('enemy_death',counts=counts())


def on_finished(actor):
    state['finish_time']=now()
    record('enemy_death_finished',delay=now()-state['death_time'])


def on_destroyed(actor):
    state['destroy_time']=now()
    record('enemy_destroyed')


def on_clear(actor):
    result['clears']+=1
    record('spawner_cleared')


def on_encounter(actor):
    result['encounters']+=1
    record('encounter_completed')


def trace_diagnostics():
    pawn,enemy=state['pawn'],state['enemy']
    start=enemy.get_actor_location()+ue.Vector(0,0,60)
    delta=state['attack'].get_locked_aim_point()-start
    end=start+delta*(1400.0/delta.length())
    entries=[]
    for actor in ue.GameplayStatics.get_all_actors_of_class(state['world'],ue.Actor):
        parent=actor
        for _ in range(6):
            if parent==pawn: break
            parent=parent.get_attach_parent_actor() if parent else None
        if parent!=pawn: continue
        for component in actor.get_components_by_class(ue.PrimitiveComponent):
            entries.append(dict(actor=actor.get_path_name(),component=component.get_path_name(),
                parent=actor.get_attach_parent_actor().get_path_name() if actor.get_attach_parent_actor() else None,
                profile=str(component.get_collision_profile_name()),enabled=str(component.get_collision_enabled()),
                visibility=str(component.get_collision_response_to_channel(ue.CollisionChannel.cast(3))),
                weapon=str(component.get_collision_response_to_channel(ue.CollisionChannel.cast(15))),
                weapon_capsule=str(component.get_collision_response_to_channel(ue.CollisionChannel.cast(16)))))
    result['collision_components']=entries
    result['trace_geometry']=dict(start=str(start),end=str(end),locked_aim=str(state['attack'].get_locked_aim_point()),
        player_location=str(pawn.get_actor_location()),health=hp(),counts=counts())
    result['traces']=[]
    # Query 0 is the engine's Visibility trace; compare exactly the attack's current ray.
    for index in (0,):
        raw=ue.SystemLibrary.line_trace_single(state['world'],start,end,ue.TraceTypeQuery.cast(index),False,
            [enemy],ue.DrawDebugTrace.NONE,True,ue.LinearColor(),ue.LinearColor(),0.0)
        if raw is None:
            result['traces'].append(dict(query_index=index,hit=False))
            continue
        values=raw if isinstance(raw,tuple) else (raw,)
        hit=next((v for v in values if isinstance(v,ue.HitResult)),None)
        assert hit is not None,repr(raw)
        result['traces'].append(dict(query_index=index,hit=True,hit_result=hit.export_text()))
    result['diagnostic_complete']=True


def finish(reason):
    for component,profile in state.get('temporary_collision',[]): component.set_collision_profile_name(profile)
    if state.get('temporary_collision'): result['temporary_collision_restored']=True
    if 'subsystem' in state:
        for action in (scope['fire'],dash): state['subsystem'].inject_input_vector_for_action(action,ue.Vector(),[],[])
    result.update(reason=reason,complete=reason=='passed',protected_changed=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h])
    if result['protected_changed']: result['complete']=False
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    print('R10_ATTACK_FINISH '+json.dumps(dict(complete=result['complete'],reason=reason)))


def tick(delta):
    try:
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name(): return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_') and 'L_DC_EnemyAttackIntegration.' in world.get_path_name()
        if state['phase']=='waiting':
            pc=ue.GameplayStatics.get_player_controller(world,0)
            pawn=ue.GameplayStatics.get_player_pawn(world,0)
            qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent')) if pc else None
            enemies=list(ue.GameplayStatics.get_all_actors_of_class(world,enemy_class))
            if not pawn or not qb or not qb.get_active_slot_item() or len(enemies)!=1: return
            enemy=enemies[0]
            found=[a for a in ue.ObjectIterator(ue.GameplayAbility) if a.get_class()==attack_class
                and not a.get_name().startswith('Default__') and a.get_avatar_actor_from_actor_info()==enemy]
            if not found: return
            assert len(found)==1 and enemy.is_gas_ready()
            subsystem=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method(
                'GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
            state.update(world=world,pc=pc,qb=qb,subsystem=subsystem,enemy=enemy,attack=found[0],started=ue.GameplayStatics.get_time_seconds(world))
            scope['bind_pawn'](pawn)
            state['health']=pawn.get_component_by_class(cls('DCLyraHealthComponent'))
            state['origin']=pawn.get_actor_location()
            state['spawner']=list(ue.GameplayStatics.get_all_actors_of_class(world,cls('DCEnemySpawner')))[0]
            encounter=list(ue.GameplayStatics.get_all_actors_of_class(world,cls('DCEncounterController')))[0]
            state['spawner'].get_editor_property('OnSpawnerCleared').add_callable(on_clear)
            encounter.get_editor_property('OnEncounterCompleted').add_callable(on_encounter)
            enemy.get_editor_property('OnEnemyDeath').add_callable(on_death)
            enemy.get_health_component().get_editor_property('OnDeathFinished').add_callable(on_finished)
            enemy.get_editor_property('OnDestroyed').add_callable(on_destroyed)
            block(True)
            stage('settle')
        assert now()-state['started']<35,'Runtime timeout: '+state['phase']
        for task in ue.ObjectIterator(ue.DCAbilityTask_DashDefenseWindow):
            if not task.get_name().startswith('Default__') and task.is_defense_window_active() and task.get_path_name() not in state['defense_tasks']:
                task.get_editor_property('OnDamageAvoided').add_callable(on_avoided)
                state['defense_tasks'].add(task.get_path_name())
        phase,elapsed=state['phase'],now()-state['since']
        c=counts()
        if phase=='settle' and elapsed>.4:
            assert state['attack'].get_attack_phase()==ue.DCEnemyAttackPhase.IDLE
            state['origin']=state['pawn'].get_actor_location()
            scope['inspect_ui']('r10_attack_initial_ui')
            request('hit')
        elif phase.endswith('_intent') and c['intent']>state['before']['intent']:
            state.update(intent_time=now(),locked=state['attack'].get_locked_aim_point())
            assert hp()==state['hp_before']
            kind=phase[:-7]
            if kind=='dodge': state['pawn'].set_actor_location(state['origin']+ue.Vector(0,250,0),False,True)
            elif kind=='cover': cover()
            elif kind=='cancel':
                state['attack'].call_method('K2_CancelAbility',args=())
                assert state['attack'].get_attack_phase()==ue.DCEnemyAttackPhase.IDLE
                block(True)
            elif kind=='kill':
                camera=state['pc'].player_camera_manager.get_camera_location()
                state['pc'].set_control_rotation(ue.MathLibrary.find_look_at_rotation(camera,state['enemy'].get_actor_location()+ue.Vector(0,0,20)))
                state['inputs'][scope['fire']]=ue.Vector(1,0,0)
            stage(kind+'_observe')
        elif phase=='hit_observe':
            if elapsed<.3: assert hp()==state['hp_before'],'Damage arrived during windup'
            if c['shot']>state['before']['shot'] and 'shot_time' not in state:
                if trace_only:
                    trace_diagnostics()
                    result['original_trace']={k:result[k] for k in ('collision_components','trace_geometry','traces')}
                    block(True)
                    state['temporary_collision']=[]
                    for component in state['pawn'].get_components_by_class(ue.PrimitiveComponent):
                        if isinstance(component,(ue.CapsuleComponent,ue.SkeletalMeshComponent)):
                            state['temporary_collision'].append((component,component.get_collision_profile_name()))
                            component.set_collision_response_to_channel(ue.CollisionChannel.cast(3),ue.CollisionResponseType.cast(2))
                    request('collision_confirm');return
                state['shot_time']=now()
                assert c['shot']==state['before']['shot']+1 and hp()==state['hp_before']-8
                assert .45 < now()-state['intent_time'] < .7
                record('windup_and_hit',before=state['hp_before'],after=hp(),delay=now()-state['intent_time'])
            if c['end']>state['before']['end']:
                assert 'shot_time' in state and now()-state['shot_time']>=.70
                assert hp()==state['hp_before']-8 and c['shot']==state['before']['shot']+1
                record('recovery',duration=now()-state['shot_time'])
                block(True);stage('prepare_dodge')
        elif phase=='prepare_dodge' and elapsed>.2: request('dodge')
        elif phase=='dodge_observe' and c['shot']>state['before']['shot']:
            assert hp()==state['hp_before'] and state['attack'].get_locked_aim_point()==state['locked']
            record('locked_aim_missed',health=hp())
            block(True);stage('prepare_cover')
        elif phase=='prepare_cover' and elapsed>.2:
            reset_player();request('cover')
        elif phase=='cover_observe' and elapsed>.7:
            assert c['end']>state['before']['end'] and c['shot']==state['before']['shot'] and hp()==state['hp_before']
            record('cover_cancelled',health=hp())
            block(True)
            state['wall'].destroy_actor()
            stage('prepare_cancel')
        elif phase=='prepare_cancel' and elapsed>.2: request('cancel')
        elif phase=='cancel_observe' and elapsed>.7:
            assert c['shot']==state['before']['shot'] and hp()==state['hp_before']
            record('external_cancel_no_late_damage',health=hp())
            stage('prepare_dash')
        elif phase=='prepare_dash' and elapsed>.2:
            reset_player();request('dash')
        elif phase=='dash_observe':
            if elapsed>.38 and not state.get('dash_pressed'):
                state['inputs'][dash]=ue.Vector(1,0,0)
                state['dash_pressed']=True
            if c['shot']>state['before']['shot']:
                assert hp()==state['hp_before'] and result['avoided_damage']==1
                record('dash_resisted_enemy_shot',health=hp(),avoided=result['avoided_damage'])
                state['inputs'][dash]=ue.Vector()
                block(True);stage('prepare_kill')
        elif phase=='prepare_kill' and elapsed>2.0:
            reset_player()
            damage_enemy(88.0)
            assert state['enemy'].get_health_component().get_health()==12.0
            request('kill')
        elif phase=='kill_observe' and result['deaths']==1:
            state['inputs'][scope['fire']]=ue.Vector()
            assert c['shot']==state['before']['shot'] and hp()==state['hp_before']
            assert state['attack'].get_attack_phase()==ue.DCEnemyAttackPhase.IDLE
            record('rifle_kill_cancelled_windup',health=hp())
            stage('cleanup')
        elif phase=='cleanup' and elapsed>2.5:
            assert result['deaths']==result['clears']==result['encounters']==1
            assert 1.95<=state['finish_time']-state['death_time']<=2.25
            assert state['destroy_time']>=state['finish_time']
            assert hp()==state['hp_before'] and state['spawner'].get_alive_count()==0
            scope['inspect_ui']('r10_attack_final_ui')
            finish('passed');return
        elif phase=='collision_confirm_observe' and c['shot']>state['before']['shot']:
            result['temporary_visibility_block_damage']=state['hp_before']-hp()
            trace_diagnostics()
            result['temporary_trace']={k:result[k] for k in ('collision_components','trace_geometry','traces')}
            assert result['temporary_visibility_block_damage']==8
            block(True)
            finish('trace_confirmed');return
        if now()-state['last_sample']>.04:
            result['samples'].append(dict(time=now(),stage=state['phase'],hp=hp(),phase=str(state['attack'].get_attack_phase()),
                counts=c,immunity=state['asc'].get_gameplay_tag_count(tag('Gameplay.DamageImmunity'))))
            state['last_sample']=now()
        for action,value in state['inputs'].items(): state['subsystem'].inject_input_vector_for_action(action,value,[],[])
    except Exception:
        details=traceback.format_exc()
        result['failures'].append(details)
        ue.log_error('R10 attack PIE: '+details)
        finish('failed')


state['callback']=ue.register_slate_post_tick_callback(tick)
print('R10_ATTACK_PIE_ARMED saves=0')
