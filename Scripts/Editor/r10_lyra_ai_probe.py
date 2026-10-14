"""Observe the isolated saved R10 AI map in actual PIE. No asset/config writes.

Start with Project.Maps.PIE and -DCR10AIProbe=<fresh receipt name>.
This deliberately does not grant inventory, force targets, move actors or fire bots.
"""
import hashlib
import json
import re
import time
import traceback
from pathlib import Path
import unreal as ue

P=Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT='/Game/DreamCatcher/GAS/Test/R10/LyraAI'
run=re.search(r'-DCR10AIProbe=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
out=P/'Saved/Diagnostics/R10'/('ai_pie_'+run+'.json');assert not out.exists()
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
           (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI')
           for p in root.rglob('*') if p.is_file()}
protected[str(P/'.git/index')]=hashlib.sha256((P/'.git/index').read_bytes()).hexdigest()
result=dict(complete=False,asset_saves=0,samples=[],failures=[],checks={},
            events=[],initial_loadout_tested=False,replication_tested=False,visual_audio_verified=False)
state=dict(wall=time.monotonic(),last=-1,done=False,first={},inventory={},ammo={},health={},targets=set(),tracked={})
duration_match=re.search(r'-DCR10AIProbeSeconds=(\d+)',ue.SystemLibrary.get_command_line())
duration=int(duration_match.group(1)) if duration_match else 32
assert 20<=duration<=90
restart_after_death='-DCR10AIRestartAfterDeath' in ue.SystemLibrary.get_command_line()
result['restart_requested_by_probe']=restart_after_death
result['automatic_respawn_tested']=False
def cls(name): return ue.load_class(None,'/Script/DreamCatcher.'+name)
bot_class=cls('DCLyraPlayerBotController')
qb_class=cls('DCLyraQuickBarComponent')
health_class=cls('DCLyraHealthComponent')
equipment_class=cls('DCLyraEquipmentManagerComponent')
inventory_class=cls('DCInventoryManagerComponent')
pickup_class=cls('DCLyraWeaponSpawner')
def tag(name):
    value=ue.GameplayTag();assert value.import_text('(TagName="'+name+'")');return value
magazine=tag('Lyra.ShooterGame.Weapon.MagazineAmmo')
spare=tag('Lyra.ShooterGame.Weapon.SpareAmmo')
def name(obj): return obj.get_name() if obj else None
def vector(value): return [value.x,value.y,value.z]
def track_lifecycle(pawn,health,controller,kind):
    key=pawn.get_path_name()
    if key in state['tracked'] or not health:return
    def record(event,actor):
        if state['done']:return
        inventory=controller.get_component_by_class(inventory_class)
        qb=controller.get_component_by_class(qb_class)
        equipment=actor.get_component_by_class(equipment_class)
        result['events'].append(dict(event=event,time=ue.GameplayStatics.get_time_seconds(state['world']),
            pawn=key,controller=controller.get_name(),kind=kind,
            inventory_count=len(inventory.get_all_items()) if inventory else None,
            occupied_slots=sum(item is not None for item in qb.get_slots()) if qb else None,
            equipment_count=len(equipment.get_equipment_instances_of_type(cls('DCLyraEquipmentInstance'))) if equipment else 0))
        if restart_after_death and event=='destroyed' and kind=='bot' and 'restart_controller' not in state:
            state.update(restart_controller=controller,restart_old_pawn=key,restart_team=controller.get_editor_property('PlayerState').get_team_id())
    callbacks=[lambda actor:record('death_started',actor),lambda actor:record('death_finished_cleanup_queued',actor),lambda actor:record('destroyed',actor)]
    health.get_editor_property('OnDeathStarted').add_callable(callbacks[0])
    health.get_editor_property('OnDeathFinished').add_callable(callbacks[1])
    pawn.get_editor_property('OnDestroyed').add_callable(callbacks[2])
    state['tracked'][key]=callbacks
def snapshot(controller,kind):
    pawn=controller.get_controlled_pawn()
    qb=controller.get_component_by_class(qb_class)
    item=qb.get_active_slot_item() if qb else None
    inventory=controller.get_component_by_class(inventory_class)
    ps=controller.get_editor_property('PlayerState')
    row=dict(controller=name(controller),kind=kind,pawn=name(pawn),team=ps.get_team_id() if ps else None,
             quickbar=bool(qb),slots=len(qb.get_slots()) if qb else 0,item=item.get_path_name() if item else None,
             inventory_count=len(inventory.get_all_items()) if inventory else None)
    if pawn:
        location=pawn.get_actor_location()
        row['position']=vector(location)
        key=pawn.get_name()
        state['first'].setdefault(key,row['position'])
        row['travel']=(location-ue.Vector(*state['first'][key])).length()
        health=pawn.get_component_by_class(health_class)
        track_lifecycle(pawn,health,controller,kind)
        row['hp']=health.get_health() if health else None
        row['dead']=health.is_dead_or_dying() if health else None
        equip=pawn.get_component_by_class(equipment_class)
        instances=list(equip.get_equipment_instances_of_type(cls('DCLyraEquipmentInstance'))) if equip else []
        row['equipment']=len(instances)
        row['equipment_item_link']=len(instances)==1 and instances[0].get_instigator()==item if item else not instances
        state['health'].setdefault(key,[]).append(row['hp'])
    if item:
        row['magazine']=item.get_stat_tag_stack_count(magazine)
        row['spare']=item.get_stat_tag_stack_count(spare)
        state['ammo'].setdefault(item.get_path_name(),[]).append(row['magazine'])
    state['inventory'].setdefault(controller.get_name(),[]).append(bool(item))
    if kind=='bot':
        bb=controller.get_editor_property('Blackboard')
        if bb:
            enemy=bb.get_value_as_object('TargetEnemy')
            row.update(target=name(enemy),out_of_ammo=bb.get_value_as_bool('OutOfAmmo'),move_goal=vector(bb.get_value_as_vector('MoveGoal')))
            if enemy:state['targets'].add(controller.get_name())
        row['move_status']=str(controller.get_move_status())
    return row
def finish(reason):
    state['done']=True
    result['reason']=reason
    bots=[row for sample in result['samples'] for row in sample['actors'] if row['kind']=='bot']
    result['checks']=dict(
        experience_and_three_bots=any(sum(r['kind']=='bot' for r in s['actors'])==3 for s in result['samples']),
        teams_one_and_two=any({r['team'] for r in s['actors']}=={1,2} and len(s['actors'])==4 for s in result['samples']),
        bot_moved=any(r.get('travel',0)>150 for r in bots),
        bot_acquired_target=bool(state['targets']),
        pickup_consumed=any(len(s.get('available_pickups',[]))==4 and not all(s['available_pickups']) for s in result['samples']),
        bot_fired=any(any(b<a for a,b in zip(v,v[1:])) for key,v in state['ammo'].items() if any(r.get('item')==key for r in bots)),
        ammo_decreased=any(any(b<a for a,b in zip(v,v[1:])) for v in state['ammo'].values()),
        ammo_increased=any(any(b>a for a,b in zip(v,v[1:])) for v in state['ammo'].values()),
        damage_observed=any(any(h is not None and h<100 for h in v) for v in state['health'].values()),
        death_observed=any(r.get('dead') for r in bots),
        bot_death_inventory_cleanup=any(e['event']=='destroyed' and e['kind']=='bot' and e['inventory_count']==0 and e['occupied_slots']==0 and e['equipment_count']==0 for e in result['events']))
    early=[s for s in result['samples'] if s['time']<2 and sum(r['kind']=='bot' for r in s['actors'])==3]
    result['initial_loadout_tested']=any(all(r.get('inventory_count')==1 and r.get('equipment')==1 and r.get('equipment_item_link') for r in s['actors']) and len(s.get('available_pickups',[]))==4 and all(s['available_pickups']) for s in early)
    result['restart_contract_tested']=any(e['event']=='restart_complete' for e in result['events'])
    result['protected_changed']=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    result['complete']=reason=='observed' and not result['failures'] and not result['protected_changed']
    result['all_behavior_checks_passed']=all(result['checks'].values())
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    print('R10_AI_PIE_FINISH '+json.dumps({k:result[k] for k in ('complete','checks','reason')}))
def tick(delta):
    if state['done']:return
    try:
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            if time.monotonic()-state['wall']>150:finish('no_pie_world')
            return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_')
        state['world']=world
        now=ue.GameplayStatics.get_time_seconds(world)
        if now-state['last']<.20:return
        state['last']=now
        controllers=list(ue.GameplayStatics.get_all_actors_of_class(world,bot_class))
        if restart_after_death and 'restart_controller' in state and not state.get('restart_issued'):
            controller=state['restart_controller']
            assert controller.get_controlled_pawn() is None
            ue.GameplayStatics.get_game_mode(world).request_player_restart_next_frame(controller,False)
            state['restart_issued']=True
            result['events'].append(dict(event='restart_requested',time=now,controller=controller.get_name(),caller='probe -> original RequestPlayerRestartNextFrame'))
        rows=[snapshot(c,'bot') for c in controllers]
        pc=ue.GameplayStatics.get_player_controller(world,0)
        if pc:rows.append(snapshot(pc,'player'))
        if state.get('restart_issued') and not state.get('restart_confirmed'):
            controller=state['restart_controller']
            pawn=controller.get_controlled_pawn()
            row=next((r for r in rows if r['controller']==controller.get_name()),None)
            if pawn and row and row.get('item'):
                assert pawn.get_path_name()!=state['restart_old_pawn']
                assert row['inventory_count']==row['equipment']==1 and row['equipment_item_link']
                assert row['team']==state['restart_team'] and row['hp']==100 and not row['dead']
                state['restart_confirmed']=True
                result['events'].append(dict(event='restart_complete',time=now,controller=controller.get_name(),pawn=pawn.get_path_name(),team=row['team'],health=row['hp'],inventory_count=row['inventory_count'],equipment_count=row['equipment'],equipment_item_link=row['equipment_item_link']))
        pickups=list(ue.GameplayStatics.get_all_actors_of_class(world,pickup_class))
        result['samples'].append(dict(time=now,actors=rows,available_pickups=[p.get_editor_property('bIsWeaponAvailable') for p in pickups]))
        if now>duration:finish('observed')
    except Exception:
        details=traceback.format_exc();result['failures'].append(details)
        ue.log_error('R10 AI PIE '+details);finish('failed')
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R10_AI_PIE_ARMED')
