"""Cooldown-only signature smoke in the integration PIE; no saved gameplay writes."""
import hashlib
import json
import re
import sys
import traceback
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
P = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
assert P == Path('E:/Epic Games/UEProjects/DreamCatcher').resolve()
R = P/'Saved/Diagnostics/R9'
scope = {'__name__':'r9_signature_existing_pie'}
exec(compile((P/'Saved/Diagnostics/R8/pie.py').read_text(encoding='utf-8-sig'),'existing_pie','exec'),scope)
state,result = scope['state'],scope['result']
ue.unregister_slate_post_tick_callback(state['callback'])
scope['out'] = R/('pie_'+scope['run']+'.json')
assert not scope['out'].exists()
tag = scope['tag']
library = scope['scope']['ability_library']
action = scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ultimate'),True)
assert action.get_path_name() == '/Game/Input/Actions/IA_Ultimate.IA_Ultimate'
context = ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/IMC_DC_RifleBase')
q_rows = [r for r in context.get_editor_property('DefaultKeyMappings').get_editor_property('Mappings')
          if r.get_editor_property('Action') == action and r.get_editor_property('Key').export_text() == 'Q']
assert len(q_rows) == 1
q_triggers = list(q_rows[0].get_editor_property('Triggers'))
assert len(q_triggers) == 1 and q_triggers[0].get_class() == ue.InputTriggerPressed.static_class()
scope['actions']['signature'] = action
scope['actions']['dash'] = scope['config'].find_ability_input_action_for_tag(tag('InputTag.Ability.Dash'),True)
for name in ('Cooldown.Ultimate','Gameplay.AbilityInputBlocked'):
    scope['track'][name] = tag(name)
GA = ue.load_class(None,'/Game/DreamCatcher/GAS/Test/R9/Signature/GA_DC_SignatureSmoke.GA_DC_SignatureSmoke_C')
assert GA
protected = {str(p):hashlib.sha256(p.read_bytes()).hexdigest()
    for root in (P/'Config',P/'Source',P/'Content/DreamCatcher/GAS',P/'Content/LyraMigration/Dash')
    for p in root.rglob('*') if p.is_file()}
base_sample = scope['sample']


def ability():
    found = [a for a in ue.ObjectIterator(ue.GameplayAbility) if a.get_class() == GA
             and not a.get_name().startswith('Default__') and a.get_avatar_actor_from_actor_info() == state['pawn']]
    assert len(found) == 1, [a.get_path_name() for a in found]
    return found[0]


def sample():
    row = base_sample()
    a = ability()
    pc_state = state['pc'].get_editor_property('PlayerState')
    resources = [r for r in ue.ObjectIterator(ue.DCResourceSet) if r.get_outer() == pc_state]
    assert len(resources) == 1
    charge = resources[0].get_editor_property('UltimateCharge').export_text()
    value = float(re.search(r'CurrentValue=([-0-9.]+)',charge).group(1))
    assert value == 0, 'No gauge charge/consumption should be needed'
    row.update(signature_commits=a.get_editor_property('TestCommitCount'),
        signature_active=library.call_method('IsGameplayAbilityActive',args=(a,)),
        cooldown_remaining=a.get_cooldown_time_remaining(),legacy_resource_value=value)
    return row


scope['sample'] = sample


def check(count, cooldown, active=None):
    row = sample()
    assert row['signature_commits'] == count, row
    assert row['tags']['Cooldown.Ultimate'] == (1 if cooldown else 0), row
    assert (row['cooldown_remaining'] > 0) == cooldown, row
    if active is not None:
        assert row['signature_active'] == active, row
    result.setdefault('checks',[]).append(dict(stage=state['phase'],count=count,cooldown=cooldown,active=active,time=row['time']))


def inp(value):
    # Action injection does NOT run key-mapping triggers automatically. Replay the
    # actual private Q mapping's trigger with a persistent runtime instance below.
    state['signature_value'] = ue.Vector(1 if value else 0,0,0)


def cancel():
    check(2,True,True)
    ability().call_method('K2_CancelAbility',args=())
    check(2,True,False)


block_tags = ue.GameplayTagContainer()
assert block_tags.import_text('(GameplayTags=((TagName="Gameplay.AbilityInputBlocked")))')


def block(on):
    assert library.call_method('AddLooseGameplayTags' if on else 'RemoveLooseGameplayTags',args=(state['pawn'],block_tags,False))
    state['signature_probe_blocked'] = on


def hold_start():
    state['before_hold'] = ability().get_editor_property('TestCommitCount')
    inp(True)


def hold_end():
    inp(False)
    state['after_hold'] = ability().get_editor_property('TestCommitCount')
    result['held_input_observation'] = dict(before=state['before_hold'],after=state['after_hold'],seconds=2.6,
        shared_action_has_pressed_trigger=False,private_q_mapping_has_pressed_trigger=True,policy_changed=True,
        mapping_trigger_replayed_through_engine=True,physical_key_tested=False)
    assert state['after_hold'] == state['before_hold']+1, 'Holding Q must not retrigger after cooldown'
    check(state['after_hold'],False,False)


def death():
    check(state['after_hold']+1,True,True)
    scope['kill']()
    check(state['after_hold']+1,True,False)


def combat_start():
    state['signature_start_ammo'] = scope['scope']['ammo']()
    scope['set_input']('fire',True)


def combat_stop():
    scope['set_input']('fire',False)
    result['combat_ammo_after_fire'] = scope['scope']['ammo']()
    assert result['combat_ammo_after_fire'] < state['signature_start_ammo']


def combat_reloaded():
    result['combat_ammo_after_reload'] = scope['scope']['ammo']()
    assert result['combat_ammo_after_reload'] == state['signature_start_ammo']


scope['steps'] = [
    ('settle',1.0,lambda:None),('initial',.05,lambda:check(0,False,False)),
    ('first_press',.08,lambda:inp(True)),('first_release',.45,lambda:(check(1,True,True),inp(False))),
    ('cooldown_retry_press',.08,lambda:(check(1,True,False),inp(True))),
    ('cooldown_retry_release',2.1,lambda:(inp(False),check(1,True,False))),
    ('cooldown_expired',.06,lambda:check(1,False,False)),
    ('second_press',.08,lambda:inp(True)),('cancel_after_commit',.04,cancel),
    ('cancel_release',2.1,lambda:inp(False)),('after_cancel_expiry',.05,lambda:check(2,False,False)),
    ('dash_press',.08,lambda:scope['set_input']('dash',True)),
    ('dash_blocks_signature',.10,lambda:inp(True)),
    ('dash_release',2.0,lambda:(inp(False),scope['set_input']('dash',False),check(2,False,False))),
    ('hold_observe',2.6,hold_start),('hold_release',2.1,hold_end),
    ('combat_fire',.35,combat_start),('combat_release',.25,combat_stop),
    ('combat_reload_press',.08,lambda:scope['set_input']('reload_action',True)),
    ('combat_reload_wait',2.2,lambda:scope['set_input']('reload_action',False)),
    ('combat_reload_done',.1,combat_reloaded),
    ('input_block_press',.10,lambda:(block(True),inp(True))),
    ('input_block_release',.10,lambda:(inp(False),check(state['after_hold'],False,False))),
    ('unblock',.10,lambda:block(False)),
    ('death_press',.08,lambda:inp(True)),('death_during_active',.08,death),
    ('death_release',2.1,lambda:inp(False)),('death_retry_press',.10,lambda:inp(True)),
    ('death_retry_release',.15,lambda:(inp(False),check(state['after_hold']+1,False,False))),
]
base_finish = scope['finish']


def finish(reason):
    state['signature_probe_finished'] = True
    if state.get('signature_probe_blocked'):
        block(False)
    result['protected_changed'] = [p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest() != h]
    result.update(input_to_ability=True,production_effect_implemented=False,test_cooldown_seconds=2,
        cancellation_refund_policy_finalized=False,death_respawn_policy_finalized=False)
    if reason == 'observations_complete':
        assert not result['protected_changed']
    base_finish(reason)


scope['finish'] = finish


def tick(delta):
    try:
        scope['tick'](delta)
        if state.get('signature_probe_finished') or 'subsystem' not in state:
            return
        if 'signature_runtime_triggers' not in state:
            pressed = ue.new_object(ue.InputTriggerPressed,state['pc'],'R9SignatureProbePressed')
            for field in ('ActuationThreshold','bShouldAlwaysTick'):
                pressed.set_editor_property(field,q_triggers[0].get_editor_property(field))
            state['signature_runtime_triggers'] = [pressed]
        state['subsystem'].inject_input_vector_for_action(action,state.get('signature_value',ue.Vector()),[],state['signature_runtime_triggers'])
    except Exception:
        details = traceback.format_exc()
        result['failures'].append(details)
        ue.log_error('R9 signature injection failed: '+details)
        finish('probe_failed')


state['callback'] = ue.register_slate_post_tick_callback(tick)
print('R9_SIGNATURE_PIE_ARMED saves=0')
