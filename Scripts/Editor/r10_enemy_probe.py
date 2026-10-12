"""R10-only PIE observations with real rifle input; no asset/config saves."""
import hashlib
import json
import re
import traceback
from pathlib import Path
import unreal as ue

P = Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT = '/Game/DreamCatcher/GAS/Test/R10'
run = re.search(r'-DCR10Run=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out = P/'Saved/Diagnostics/R10'/('pie_'+run+'.json')
assert not out.exists()
scope = {'__name__':'r10_existing_rifle_helpers'}
exec(compile((P/'Scripts/Editor/r7_reticle_host_probe.py').read_text(encoding='utf-8-sig'),'existing_rifle_helpers','exec'),scope)
state = scope['state']
ue.unregister_slate_post_tick_callback(state['callback'])
cls,tag,library = scope['cls'],scope['tag'],scope['ability_library']
enemy_class = ue.load_class(None,ROOT+'/BP_DC_GASEnemy.BP_DC_GASEnemy_C')
spawner_class = ue.load_class(None,ROOT+'/BP_DC_GASEnemySpawner.BP_DC_GASEnemySpawner_C')
actions = {k:scope[k] for k in ('fire','reload_action','aim')}
actions['dash'] = scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ability.Dash'),True)
actions['signature'] = scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ultimate'),True)
signature_class = ue.load_class(None,'/Game/DreamCatcher/GAS/Test/R9/Signature/GA_DC_SignatureSmoke.GA_DC_SignatureSmoke_C')
damage_class = ue.load_class(None,'/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
assert all(actions.values()) and enemy_class and spawner_class and signature_class and damage_class
protected = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
    (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test/R10',P/'Content/DreamCatcher/GAS/Test/R5/Integration')
    for p in root.rglob('*') if p.is_file()}
result = dict(complete=False,asset_saves=0,failures=[],events=[],samples=[],deaths={},
              clears=0,encounters=0,physical_inputs_tested=False,replication_tested=False,
              visual_quality_verified=False,audio_verified=False)
state.update(tracked={},phase='waiting',inputs={},last_sample=-1)


def now(): return ue.GameplayStatics.get_time_seconds(state['world'])


def record(event, **values):
    result['events'].append(dict(event=event,time=now(),**values))
    print('R10_PIE_EVENT '+json.dumps(result['events'][-1]))


def inp(name,on):
    state['inputs'][actions[name]] = ue.Vector(float(on),0,0)


def stage(name):
    state.update(phase=name,since=now())
    record('stage',name=name)


def actors(cls): return list(ue.GameplayStatics.get_all_actors_of_class(state['world'],cls))


def health(enemy): return enemy.get_health_component().get_health()


def on_death(enemy):
    key=enemy.get_name()
    result['deaths'][key]=result['deaths'].get(key,0)+1
    state['tracked'][key]['death_time']=now()
    record('death_started',enemy=key,health=health(enemy),collision=enemy.get_actor_enable_collision())


def on_finish(enemy):
    key=enemy.get_name()
    state['tracked'][key]['finish_time']=now()
    record('death_finished',enemy=key,delay=now()-state['tracked'][key]['death_time'])


def on_destroyed(enemy):
    state['tracked'][enemy.get_name()]['destroy_time']=now()
    record('destroyed',enemy=enemy.get_name())


def on_clear(spawner):
    result['clears']+=1
    record('spawner_cleared',spawned=spawner.get_spawned_count(),alive=spawner.get_alive_count())


def on_encounter(encounter):
    result['encounters']+=1
    record('encounter_completed')


def track():
    for enemy in actors(enemy_class):
        key=enemy.get_name()
        if key in state['tracked']: continue
        assert enemy.is_gas_ready() and not enemy.is_dead_or_dying()
        assert health(enemy)==100
        enemy.get_editor_property('OnEnemyDeath').add_callable(on_death)
        enemy.get_health_component().get_editor_property('OnDeathFinished').add_callable(on_finish)
        enemy.get_editor_property('OnDestroyed').add_callable(on_destroyed)
        state['tracked'][key]=dict(actor=enemy,spawn_time=now())
        record('spawned',enemy=key,health=health(enemy),team=enemy.get_editor_property('TeamId'))


def living(): return [a for a in actors(enemy_class) if not a.is_dead_or_dying()]


def aim_at_enemy():
    live=living()
    if live:
        camera=state['pc'].player_camera_manager.get_camera_location()
        point=live[0].get_actor_location()+ue.Vector(0,0,20)
        state['pc'].set_control_rotation(ue.MathLibrary.find_look_at_rotation(camera,point))


def damage_player(amount):
    asc=state['asc']
    spec=asc.make_outgoing_spec(damage_class,1,asc.make_effect_context())
    spec=library.call_method('AssignTagSetByCallerMagnitude',args=(spec,tag('SetByCaller.Damage'),float(amount)))
    asc.apply_gameplay_effect_spec_to_self(spec)


def signature_count():
    found=[a for a in ue.ObjectIterator(ue.GameplayAbility) if a.get_class()==signature_class
           and not a.get_name().startswith('Default__') and a.get_avatar_actor_from_actor_info()==state['pawn']]
    assert len(found)==1
    return found[0].get_editor_property('TestCommitCount')


def finish(reason):
    state['finished']=True
    if 'subsystem' in state:
        for action in actions.values(): state['subsystem'].inject_input_vector_for_action(action,ue.Vector(),[],[])
    for entry in state['tracked'].values(): entry.pop('actor',None)
    result.update(reason=reason,complete=reason=='passed',tracked=state['tracked'],
        protected_changed=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h])
    if result['protected_changed']: result['complete']=False
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    print('R10_PIE_FINISH '+json.dumps(dict(complete=result['complete'],reason=reason)))


def tick(delta):
    try:
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name(): return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_') and 'L_DC_EnemyIntegration.' in world.get_path_name()
        if state['phase']=='waiting':
            pc=ue.GameplayStatics.get_player_controller(world,0)
            pawn=ue.GameplayStatics.get_player_pawn(world,0)
            qb=pc.get_component_by_class(cls('DCLyraQuickBarComponent')) if pc else None
            if not pawn or not qb or not qb.get_active_slot_item(): return
            subsystem=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method(
                'GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
            state.update(world=world,pc=pc,qb=qb,subsystem=subsystem,started=ue.GameplayStatics.get_time_seconds(world))
            scope['bind_pawn'](pawn)
            state['spawner']=actors(spawner_class)[0]
            state['encounter']=actors(cls('DCEncounterController'))[0]
            state['spawner'].get_editor_property('OnSpawnerCleared').add_callable(on_clear)
            state['encounter'].get_editor_property('OnEncounterCompleted').add_callable(on_encounter)
            state['health']=pawn.get_component_by_class(cls('DCLyraHealthComponent'))
            state['pressed']=ue.new_object(ue.InputTriggerPressed,pc,'R10SignaturePressed')
            track()
            state['first']=living()[0]
            assert state['spawner'].get_alive_count()==1
            stage('settle')
        assert now()-state['started']<45,'Runtime timeout: '+state['phase']
        track()
        phase,elapsed=state['phase'],now()-state['since']
        if phase in ('settle','friendly','friendly_release','kills'): aim_at_enemy()
        if phase=='settle' and elapsed>.8:
            scope['inspect_ui']('r10_initial_ui')
            # Runtime-only fixture adjustment; do not alter CDOs or saved team data.
            state['first'].set_editor_property('TeamId',0)
            state['ammo_before']=scope['ammo']()
            inp('fire',True);stage('friendly')
        elif phase=='friendly' and elapsed>.5:
            inp('fire',False)
            assert scope['ammo']()<state['ammo_before'] and health(state['first'])==100
            record('same_team_blocked',ammo=scope['ammo'](),health=health(state['first']))
            stage('friendly_release')
        elif phase=='friendly_release' and elapsed>.4:
            state['first'].set_editor_property('TeamId',1)
            inp('fire',True);stage('kills')
        elif phase=='kills' and state['spawner'].is_spawner_cleared():
            inp('fire',False)
            assert result['clears']==result['encounters']==1 and state['spawner'].get_spawned_count()==3
            assert len(result['deaths'])==3 and all(v==1 for v in result['deaths'].values())
            stage('cleanup')
        elif phase=='cleanup' and elapsed>2.5:
            assert not actors(enemy_class) and state['spawner'].get_alive_count()==0
            for entry in state['tracked'].values():
                assert 1.95 <= entry['finish_time']-entry['death_time'] <= 2.25
                assert entry['destroy_time']>=entry['finish_time']
            assert not state['asc'].has_matching_gameplay_tag(tag('Status.Death'))
            assert state['pc'].get_view_target()==state['pawn']
            scope['inspect_ui']('r10_after_kills_ui')
            record('three_deaths_cleaned',player_camera_unchanged=True)
            inp('reload_action',True);stage('reload_press')
        elif phase=='reload_press' and elapsed>.1:
            inp('reload_action',False);stage('reload_wait')
        elif phase=='reload_wait' and elapsed>2.3:
            assert scope['ammo']()==30
            state['hp_before']=state['health'].get_health()
            inp('dash',True);stage('dash')
        elif phase=='dash' and elapsed>.07:
            assert state['asc'].has_matching_gameplay_tag(tag('Gameplay.DamageImmunity'))
            damage_player(20)
            assert state['health'].get_health()==state['hp_before']
            inp('dash',False);stage('dash_end')
        elif phase=='dash_end' and elapsed>1.2:
            assert not state['asc'].has_matching_gameplay_tag(tag('Gameplay.DamageImmunity'))
            state['q_before']=signature_count()
            inp('signature',True);stage('signature_hold')
        elif phase=='signature_hold' and elapsed>2.6:
            assert signature_count()==state['q_before']+1
            inp('signature',False);stage('signature_release')
        elif phase=='signature_release' and elapsed>.2:
            inp('signature',True);stage('signature_again')
        elif phase=='signature_again' and elapsed>.2:
            inp('signature',False)
            assert signature_count()==state['q_before']+2
            damage_player(10000)
            stage('player_death')
        elif phase=='player_death' and elapsed>.3:
            assert state['asc'].has_matching_gameplay_tag(tag('Status.Death.Dying'))
            record('player_regression',ammo_reloaded=30,dash_immunity=True,signature_commits=2,death=True)
            finish('passed');return
        if now()-state['last_sample']>.05:
            result['samples'].append(dict(time=now(),phase=state['phase'],ammo=scope['ammo'](),
                enemies={a.get_name():health(a) for a in actors(enemy_class)},alive=state['spawner'].get_alive_count(),spawned=state['spawner'].get_spawned_count()))
            state['last_sample']=now()
        for action,value in state['inputs'].items():
            triggers=[state['pressed']] if action==actions['signature'] else []
            state['subsystem'].inject_input_vector_for_action(action,value,[],triggers)
    except Exception:
        error=traceback.format_exc()
        result['failures'].append(error)
        ue.log_error('R10 PIE: '+error)
        finish('failed')


state['callback']=ue.register_slate_post_tick_callback(tick)
print('R10_PIE_ARMED saves=0')
