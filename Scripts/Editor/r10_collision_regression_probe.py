"""Reuse the accepted R8 input/combat sequence; verify player collision and camera too."""
import json
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher')
scope={'__name__':'r10_collision_existing_jump_regression'}
exec(compile((P/'Saved/Diagnostics/R8/jump_pie_probe.py').read_text(encoding='utf-8-sig'),'existing_jump_regression','exec'),scope)
scene=scope['scope']
state,result=scope['state'],scope['result']
ue.unregister_slate_post_tick_callback(state['callback'])
scene['out']=P/'Saved/Diagnostics/R10'/('collision_pie_'+scope['run']+'.json')
assert not scene['out'].exists()
base_sample=scene['sample']


def sample():
    row=base_sample()
    capsule=state['pawn'].get_component_by_class(ue.CapsuleComponent)
    mesh=state['pawn'].get_component_by_class(ue.SkeletalMeshComponent)
    row['collision_profiles']=[str(capsule.get_collision_profile_name()),str(mesh.get_collision_profile_name())]
    row['collision_enabled']=[str(capsule.get_collision_enabled()),str(mesh.get_collision_enabled())]
    row['camera_fov']=state['pc'].player_camera_manager.get_fov_angle()
    row['view_target_is_pawn']=state['pc'].get_view_target()==state['pawn']
    return row


scene['sample']=sample
base_finish=scene['finish']


def finish(reason):
    if reason=='observations_complete':
        by={n:[r for r in result['samples'] if r['stage']==n] for n in {r['stage'] for r in result['samples']}}
        last=lambda n:by[n][-1]
        checks=dict(
            original_profiles_while_alive=all(r['collision_profiles']==['LyraPawnCapsule','LyraPawnMesh'] for r in result['samples'] if r['tags']['Status.Death.Dying']==0),
            movement_in_four_directions=all(any(r['speed']>1 and r['anim']['HasAcceleration'] for r in by[n]) for n in ('forward','right','left_transition','backward')),
            stop_returns_idle=last('stop')['speed']<1,
            crouch_releases=last('crouched')['anim']['IsCrouching'] and not last('uncrouched')['anim']['IsCrouching'],
            ads_changes_fov=last('aim_hold')['camera_fov']<last('idle')['camera_fov']-1,
            ads_release_restores_fov=abs(last('aim_release')['camera_fov']-last('idle')['camera_fov'])<.1,
            view_target_preserved=all(r['view_target_is_pawn'] for n in ('idle','forward','jump_start','crouched','aim_hold','aim_release') for r in by[n]),
            manual_reload_full=last('manual_R_wait')['items'][0]['ammo']==30,
            auto_refill_resumed=state.get('saw_auto_refill',False) and 0<last('held_fire_through_auto')['items'][0]['ammo']<30,
            reload_after_auto=last('post_auto_R_wait')['items'][0]['ammo']==30,
            weapon_switch_returns=last('second_settle')['slot']==1 and last('first_settle')['slot']==0,
            death_cancels_reload=last('death_R_start')['items']==last('death_during_reload')['items'] and last('death_during_reload')['tags']['Status.Death.Dying']>0)
        result['collision_regression_checks']=checks
        print('R10_COLLISION_REGRESSION '+json.dumps(checks))
        if not all(checks.values()): ue.log_error('Collision regression failed: '+repr([n for n,v in checks.items() if not v]))
    base_finish(reason)


scene['finish']=finish
state['callback']=ue.register_slate_post_tick_callback(scene['tick'])
print('R10_COLLISION_REGRESSION_ARMED saves=0')
