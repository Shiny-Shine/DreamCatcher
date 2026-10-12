"""Approved integration-map PIE damage/window/cancel/death checks; saves nothing."""
import hashlib
import json
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
P = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
assert P == Path('E:/Epic Games/UEProjects/DreamCatcher').resolve()
R = P / 'Saved/Diagnostics/R9'
scope = {'__name__': 'r9_defense_existing_pie'}
exec(compile((P/'Saved/Diagnostics/R8/pie.py').read_text(encoding='utf-8-sig'), 'existing_pie', 'exec'), scope)
state, result = scope['state'], scope['result']
ue.unregister_slate_post_tick_callback(state['callback'])
scope['out'] = R / ('pie_' + scope['run'] + '.json')
assert not scope['out'].exists()
tag = scope['tag']
library = scope['scope']['ability_library']
dash = scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ability.Dash'), True)
scope['actions']['dash'] = dash
for name in ('Gameplay.DamageImmunity', 'Event.Movement.Dash', 'GameplayCue.Character.Dash.Cooldown'):
    scope['track'][name] = tag(name)
ga = ue.load_class(None, '/Game/LyraMigration/Dash/GA_Hero_Dash.GA_Hero_Dash_C')
damage_ge = ue.load_class(None, '/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller.GE_Damage_Basic_SetByCaller_C')
assert ga and damage_ge and dash
protected = {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
             for root in (P/'Config', P/'Content/LyraMigration/Dash', P/'Content/DreamCatcher/GAS/Effects', P/'Content/DreamCatcher/GAS/Test/R5/Integration')
             for p in root.rglob('*') if p.is_file()}
base_sample = scope['sample']


def health_component():
    return state['pawn'].get_component_by_class(scope['cls']('DCLyraHealthComponent'))


def immunity():
    return state['asc'].get_gameplay_tag_count(tag('Gameplay.DamageImmunity'))


def sample():
    row = base_sample()
    row['health'] = health_component().get_health()
    row['active_defense_tasks'] = sum(1 for t in ue.ObjectIterator(ue.DCAbilityTask_DashDefenseWindow)
                                    if not t.get_name().startswith('Default__') and t.is_defense_window_active())
    return row


scope['sample'] = sample


def damage(amount, expected_delta, self_destruct=False):
    asc = state['asc']
    before = health_component().get_health()
    spec = asc.make_outgoing_spec(damage_ge, 1, asc.make_effect_context())
    spec = library.call_method('AssignTagSetByCallerMagnitude', args=(spec, tag('SetByCaller.Damage'), float(amount)))
    if self_destruct:
        tags = ue.GameplayTagContainer()
        assert tags.import_text('(GameplayTags=((TagName="Gameplay.Damage.SelfDestruct")))')
        spec = library.call_method('AddAssetTags', args=(spec, tags))
    asc.apply_gameplay_effect_spec_to_self(spec)
    after = health_component().get_health()
    result.setdefault('damage_checks', []).append(dict(stage=state['phase'], before=before, after=after,
        amount=amount, expected_delta=expected_delta, self_destruct=self_destruct, immunity_after=immunity()))
    assert abs(after - before - expected_delta) < .001, result['damage_checks'][-1]
    sample()


def inp(on):
    scope['set_input']('dash', on)


def begin():
    assert immunity() == 0
    state['pawn'].get_component_by_class(ue.CharacterMovementComponent).stop_movement_immediately()
    state['pc'].set_control_rotation(ue.Rotator(pitch=0, yaw=0, roll=0))
    state['pawn'].set_actor_rotation(ue.Rotator(pitch=0, yaw=0, roll=0), True)
    inp(True)


def during():
    assert immunity() == 1
    assert sample()['active_defense_tasks'] == 1
    damage(20, 0)
    damage(15, 0)


def after_window():
    assert immunity() == 0
    assert sample()['active_defense_tasks'] == 0
    damage(10, -10)


def cancel():
    assert immunity() == 1
    abilities = [o for o in ue.ObjectIterator(ue.GameplayAbility) if o.get_class() == ga
                 and not o.get_name().startswith('Default__') and o.get_avatar_actor_from_actor_info() == state['pawn']]
    assert len(abilities) == 1
    abilities[0].call_method('K2_CancelAbility', args=())
    assert immunity() == 0 and sample()['active_defense_tasks'] == 0
    damage(5, -5)


def blocked_retry():
    assert immunity() == 0
    assert state['asc'].get_gameplay_tag_count(tag('GameplayCue.Character.Dash.Cooldown')) > 0
    inp(True)


def self_destruct():
    assert immunity() == 1
    damage(10000, -health_component().get_health(), True)
    assert immunity() == 0 and health_component().is_dead_or_dying()
    assert sample()['active_defense_tasks'] == 0


scope['steps'] = [('settle', 1.0, lambda: None), ('damage_before', .1, lambda: damage(25, -25)),
    ('normal_press', .065, begin), ('damage_during', .065, during), ('normal_release', .35, lambda: inp(False)),
    ('damage_after', .15, after_window), ('cooldown_retry_press', .08, blocked_retry),
    ('cooldown_retry_release', 1.5, lambda: (inp(False), after_window())),
    ('cancel_press', .065, begin), ('cancel_window', .04, cancel), ('cancel_release', 1.7, lambda: inp(False)),
    ('death_press', .065, begin), ('self_destruct_during', .1, self_destruct),
    ('death_release', .25, lambda: inp(False)), ('death_retry', .15, lambda: inp(True)),
    ('death_done', .25, lambda: inp(False))]
base_finish = scope['finish']


def finish(reason):
    result['protected_changed'] = [p for p, h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest() != h]
    result.update(input_to_original_dash=True, damage_via_existing_set_by_caller_GE=True,
                  feedback_visual_audio_verified=False, replication_tested=False)
    if reason == 'observations_complete':
        assert not result['protected_changed'] and immunity() == 0
        assert sample()['active_defense_tasks'] == 0
    base_finish(reason)


scope['finish'] = finish
state['callback'] = ue.register_slate_post_tick_callback(scope['tick'])
print('R9_DEFENSE_PIE_ARMED asset_saves=0')
