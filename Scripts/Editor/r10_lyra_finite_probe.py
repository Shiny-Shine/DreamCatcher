"""Post-build actual PIE lifecycle check for the new finite encounter map.

Use -DCR10FiniteProbe=<fresh receipt>, Project.Maps.PIE and >=35s PIE duration.
Runtime-only player immunity and an existing damage GE with the original self-destruct tag exercise GAS death.
This verifies finite spawning/cleanup, not aim, weapon lethality or visual quality.
"""
import hashlib
import json
import re
import time
import traceback
from pathlib import Path
import unreal as ue

P = Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT = '/Game/DreamCatcher/GAS/Test/R10/LyraEncounter'
run = re.search(r'-DCR10FiniteProbe=([A-Za-z0-9_]+)', ue.SystemLibrary.get_command_line()).group(1)
out = P/'Saved/Diagnostics/R10'/('finite_pie_'+run+'.json')
assert not out.exists()
protected = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
             (P/'Source', P/'Config', P/'Content/DreamCatcher/GAS/Test', P/'Content/LyraMigration/AI')
             for p in root.rglob('*') if p.is_file()}
protected[str(P/'.git/index')] = hashlib.sha256((P/'.git/index').read_bytes()).hexdigest()
result = dict(complete=False, samples=[], events=[], failures=[], asset_saves=0,
              death_trigger='GE_Damage_Basic_SetByCaller + Gameplay.Damage.SelfDestruct', player_runtime_immunity=True,
              replication_tested=False, visual_quality_verified=False)
state = dict(wall=time.monotonic(), last=-1, done=False, tracked={}, callbacks=[], clears={}, encounters=0)


def cls(name):
    obj = ue.load_class(None, '/Script/DreamCatcher.'+name)
    assert obj, name
    return obj


bot_class, spawner_class, encounter_class = [cls(n) for n in ('DCLyraPlayerBotController','DCEnemySpawner','DCEncounterController')]
health_class, inventory_class, equipment_class = [cls(n) for n in ('DCLyraHealthComponent','DCInventoryManagerComponent','DCLyraEquipmentManagerComponent')]
equipment_instance_class = cls('DCLyraEquipmentInstance')
immunity = ue.GameplayTagContainer()
assert immunity.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
library = ue.get_default_object(ue.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
damage_class = ue.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
assert library and damage_class
self_destruct_tags = ue.GameplayTagContainer()
assert self_destruct_tags.import_text('(GameplayTags=((TagName="Gameplay.Damage.SelfDestruct")))')
damage_tag = ue.GameplayTag()
assert damage_tag.import_text('(TagName="SetByCaller.Damage")')


def now(): return ue.GameplayStatics.get_time_seconds(state['world'])
def actors(class_): return list(ue.GameplayStatics.get_all_actors_of_class(state['world'], class_))


def event(kind, **data):
    if not state['done']: result['events'].append(dict(event=kind, time=now(), **data))


def bind(obj, name, callback):
    obj.get_editor_property(name).add_callable(callback)
    state['callbacks'].append(callback)


def lifecycle_callback(kind, key):
    # Unreal validates the callable's total argument count, including defaults.
    # Capture metadata in a closure while exposing exactly the delegate's one argument.
    def callback(actor):
        event(kind, pawn=key)
    return callback


def finish(ok):
    if state['done']: return
    state['done'] = True
    player = state.get('immune_player')
    if player and ue.SystemLibrary.is_valid(player):
        library.call_method('RemoveLooseGameplayTags', args=(player, immunity, False))
    result['protected_changed'] = [p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest() != h]
    result['complete'] = ok and not result['failures'] and not result['protected_changed']
    out.write_text(json.dumps(result, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not result['complete']: ue.log_error('R10 finite encounter check failed; see '+str(out))
    print('R10_FINITE_PIE '+json.dumps({k:result[k] for k in ('complete','protected_changed','failures')}))


def tick(delta):
    if state['done']: return
    try:
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            if time.monotonic()-state['wall'] > 180: raise AssertionError('No PIE world')
            return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_')
        state['world'] = world
        t = now()
        if t-state['last'] < .05: return
        state['last'] = t
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        player = pc.get_controlled_pawn() if pc else None
        if player and 'immune_player' not in state:
            health = player.get_component_by_class(health_class)
            if health and health.get_max_health() > 0:
                assert library.call_method('AddLooseGameplayTags', args=(player, immunity, False))
                state['immune_player'] = player
        spawners = actors(spawner_class)
        encounters = actors(encounter_class)
        assert len(spawners) == 2 and len(encounters) == 1
        if 'bound' not in state:
            state['bound'] = True
            def cleared(s):
                key = s.get_name()
                state['clears'][key] = state['clears'].get(key, 0)+1
                event('spawner_cleared', spawner=key)
            def completed(e):
                state['encounters'] += 1
                event('encounter_completed')
            for s in spawners: bind(s, 'OnSpawnerCleared', cleared)
            bind(encounters[0], 'OnEncounterCompleted', completed)
        bots = actors(bot_class)
        for bot in bots:
            pawn = bot.get_controlled_pawn()
            if not pawn: continue
            key = pawn.get_path_name()
            health = pawn.get_component_by_class(health_class)
            assert health
            if key not in state['tracked']:
                record = dict(pawn=pawn, health=health, born=t, loadout=False, self_destruct=False)
                state['tracked'][key] = record
                bot_name = bot.get_name()
                ps = bot.get_editor_property('PlayerState')
                assert ps
                event('spawned', pawn=key, controller=bot_name)
                bind(health, 'OnDeathStarted', lifecycle_callback('death_started', key))
                bind(health, 'OnDeathFinished', lifecycle_callback('death_finished', key))
                bind(pawn, 'OnDestroyed', lifecycle_callback('pawn_destroyed', key))
                bind(bot, 'OnDestroyed', lifecycle_callback('controller_destroyed', key))
                bind(ps, 'OnDestroyed', lifecycle_callback('player_state_destroyed', key))
            record = state['tracked'][key]
            inv = bot.get_component_by_class(inventory_class)
            eq = pawn.get_component_by_class(equipment_class)
            if inv and eq and not health.is_dead_or_dying():
                n_items = len(inv.get_all_items())
                n_equipped = len(eq.get_equipment_instances_of_type(equipment_instance_class))
                assert n_items <= 1 and n_equipped <= 1, 'Duplicate initial loadout'
                record['loadout'] |= n_items == n_equipped == 1
            if t-record['born'] > 3 and not health.is_dead_or_dying() and not record['self_destruct']:
                assert record['loadout'], 'Initial rifle not ready'
                record['self_destruct'] = True
                event('requested_gas_death', pawn=key)
                # DamageSelfDestruct is native-only. Reuse the already verified R9
                # diagnostic GE recipe through exposed GAS APIs, without a new C++ hook.
                asc = library.call_method('GetAbilitySystemComponent', args=(pawn,))
                assert asc
                spec = asc.make_outgoing_spec(damage_class, 1, asc.make_effect_context())
                spec = library.call_method('AssignTagSetByCallerMagnitude', args=(spec, damage_tag, health.get_max_health()))
                spec = library.call_method('AddAssetTags', args=(spec, self_destruct_tags))
                asc.apply_gameplay_effect_spec_to_self(spec)
        rows = [dict(name=s.get_name(), spawned=s.get_spawned_count(), alive=s.get_alive_count(),
                     pending_cleanup=s.get_pending_lyra_cleanup_count(), cleared=s.is_spawner_cleared(),
                     failed=s.has_spawn_failure(), budget=s.get_editor_property('SpawnCount')) for s in spawners]
        assert all(not r['failed'] and r['alive'] <= 1 and r['spawned'] <= r['budget'] for r in rows)
        assert not encounters[0].has_encounter_failure()
        result['samples'].append(dict(time=t, spawners=rows, controllers=len(bots), completed=encounters[0].is_encounter_completed()))
        if all(r['cleared'] and r['pending_cleanup'] == 0 for r in rows) and len(bots) == 0:
            state.setdefault('clean_since', t)
            if t-state['clean_since'] > 2:
                assert sum(r['spawned'] for r in rows) == 3 and len(state['tracked']) == 3
                assert state['encounters'] == 1 and len(state['clears']) == 2 and all(v == 1 for v in state['clears'].values())
                for key, record in state['tracked'].items():
                    events = [e for e in result['events'] if e.get('pawn') == key]
                    times = {}
                    for kind in ('death_started','death_finished','pawn_destroyed','controller_destroyed','player_state_destroyed'):
                        found = [e['time'] for e in events if e['event'] == kind]
                        assert len(found) == 1, (key, kind, found)
                        times[kind] = found[0]
                    assert record['loadout'] and times['death_started'] <= times['death_finished'] <= times['controller_destroyed']
                    assert times['death_finished'] <= times['pawn_destroyed'] and times['death_finished'] <= times['player_state_destroyed']
                assert player and ue.SystemLibrary.is_valid(player) and not player.get_component_by_class(health_class).is_dead_or_dying()
                result['checks'] = dict(total_spawned=3, spawner_clears=2, encounter_completions=1, remaining_bot_controllers=0,
                                        loadout_each_once=True, death_cleanup_order=True, spawn_limits=True)
                finish(True)
        if t > 30 and not state['done']: raise AssertionError('Finite lifecycle did not finish within 30 seconds')
    except Exception:
        result['failures'].append(traceback.format_exc())
        finish(False)


state['callback'] = ue.register_slate_post_tick_callback(tick)
print('R10_FINITE_PIE_ARMED')
