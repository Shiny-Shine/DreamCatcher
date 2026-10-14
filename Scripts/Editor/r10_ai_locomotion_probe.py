"""Observe locomotion, with an optional single-runtime-bot navigation A/B trial.

No CDO/asset saves. The player receives a short Move input for comparison.
"""
import hashlib,json,re,time,traceback,sys
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher')
sys.dont_write_bytecode=True
ROOT='/Game/DreamCatcher/GAS/Test/R10/LyraAI'
cmd=ue.SystemLibrary.get_command_line()
run=re.search(r'-DCR10LocoRun=([A-Za-z0-9_]+)',cmd).group(1)
trial='-DCR10LocoRuntimeTrial' in cmd
out=P/'Saved/Diagnostics/R10'/('locomotion_pie_'+run+'.json');assert not out.exists()
protected={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for root in
    (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI',P/'Content/Characters/Heroes/Mannequin/Animations')
    for p in root.rglob('*') if p.is_file()}
protected[str(P/'.git/index')]=hashlib.sha256((P/'.git/index').read_bytes()).hexdigest()
def cls(name):return ue.load_class(None,'/Script/DreamCatcher.'+name)
bot_class=cls('DCLyraPlayerBotController')
layer_class=ue.load_class(None,'/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Rifle/ABP_RifleAnimLayers.ABP_RifleAnimLayers_C')
move_tag=ue.GameplayTag();assert move_tag.import_text('(TagName="InputTag.Move")')
move=ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput').find_native_input_action_for_tag(move_tag,True)
result=dict(complete=False,asset_saves=0,runtime_trial=trial,samples=[],failures=[],machine_descriptions={})
state=dict(wall=time.monotonic(),last=-1,finished=False,overridden=False)
if '-DCR10LocoSavedFix' in cmd:
    assert not trial
    fixed=json.loads((P/'Saved/Diagnostics/R10/locomotion_Apply_C.json').read_text())
    assert fixed['complete'] and fixed['asset_saves']==1
    pawn_file=P/'Content/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn.uasset'
    assert hashlib.sha256(pawn_file.read_bytes()).hexdigest()==fixed['pawn_after_hash']
    # Keep editor graph/default-object inspection in the separate commandlet.
    # PIE observes runtime components without invoking Blueprint editor readers.
    result['saved_asset_hash_checked']=True
    result['expected_saved_nav']=fixed['after_nav']
def encode(v):
    if v is None or isinstance(v,(str,int,float,bool)):return v
    if isinstance(v,ue.Object):return v.get_path_name()
    return v.export_text() if hasattr(v,'export_text') else str(v)
def fields(obj,names):
    values={}
    for name in names:
        try:values[name]=encode(obj.get_editor_property(name))
        except Exception as error:values[name]={'unavailable':str(error)}
    return values
def sample(controller,kind,now):
    pawn=controller.get_controlled_pawn()
    if not pawn:return None
    movement=pawn.get_component_by_class(ue.CharacterMovementComponent)
    mesh=pawn.get_component_by_class(ue.SkeletalMeshComponent)
    anim=mesh.get_anim_instance()
    assert movement and mesh and anim
    nav=movement.get_editor_property('NavMovementProperties')
    if '-DCR10LocoSavedFix' in cmd:assert nav.get_editor_property('bUseAccelerationForPaths')
    row=dict(time=now,controller=controller.get_name(),pawn=pawn.get_name(),kind=kind,
        speed=pawn.get_velocity().length(),acceleration=movement.get_current_acceleration().length(),
        nav=nav.export_text(),anim_class=anim.get_class().get_path_name(),
        anim=fields(anim,('HasVelocity','HasAcceleration','IsOnGround','LocalVelocity2D','LocalAcceleration2D','GroundDistance','LastLinkedLayer')),
        rifle_layer=bool(anim.get_linked_anim_layer_instance_by_class(layer_class)),bones={})
    row['dead']=pawn.get_component_by_class(cls('DCLyraHealthComponent')).is_dead_or_dying()
    equipment=pawn.get_component_by_class(cls('DCLyraEquipmentManagerComponent'))
    row['equipment_count']=len(equipment.get_equipment_instances_of_type(cls('DCLyraEquipmentInstance'))) if equipment else 0
    for name in ('foot_l','foot_r','calf_l','calf_r'):
        assert mesh.does_socket_exist(name),name
        t=mesh.get_socket_transform(name,ue.RelativeTransformSpace.RTS_COMPONENT)
        row['bones'][name]=dict(position=[t.translation.x,t.translation.y,t.translation.z],rotation=[t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w])
    key=anim.get_class().get_path_name()
    if key not in result['machine_descriptions']:
        try:
            machines=anim.get_class().get_editor_property('BakedStateMachines')
            result['machine_descriptions'][key]=[m.export_text() for m in machines]
        except Exception as error:result['machine_descriptions'][key]={'unavailable':str(error)}
    desc=result['machine_descriptions'][key]
    if isinstance(desc,list):row['states']=[str(anim.call_method('GetCurrentStateName',args=(i,))) for i in range(len(desc))]
    return row
def finish(reason):
    state['finished']=True
    if state.get('subsystem'):state['subsystem'].inject_input_vector_for_action(move,ue.Vector(),[],[])
    result['reason']=reason
    result['protected_changed']=[p for p,h in protected.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    result['complete']=reason=='observed' and not result['failures'] and not result['protected_changed']
    out.write_text(json.dumps(result,indent=2),encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    print('R10_LOCOMOTION_PIE '+json.dumps({k:result[k] for k in ('complete','reason','protected_changed')}))
def tick(delta):
    if state['finished']:return
    try:
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            if time.monotonic()-state['wall']>180:finish('no_pie_world')
            return
        assert world.get_path_name().startswith(ROOT+'/UEDPIE_')
        now=ue.GameplayStatics.get_time_seconds(world)
        pc=ue.GameplayStatics.get_player_controller(world,0)
        if pc and pc.get_controlled_pawn() and 'subsystem' not in state:
            state['subsystem']=ue.get_default_object(ue.load_class(None,'/Script/Engine.SubsystemBlueprintLibrary')).call_method('GetLocalPlayerSubSystemFromPlayerController',args=(pc,ue.load_class(None,'/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
        if state.get('subsystem'):state['subsystem'].inject_input_vector_for_action(move,ue.Vector(0,1 if 1.5<now<2.2 else 0,0),[],[])
        if now-state['last']<.10:return
        state['last']=now
        bots=list(ue.GameplayStatics.get_all_actors_of_class(world,bot_class))
        rows=[r for c in bots if c.get_controlled_pawn() for r in [sample(c,'bot',now)] if r]
        if pc and pc.get_controlled_pawn():rows.append(sample(pc,'player',now))
        result['samples'].extend(rows)
        if trial and now>4 and not state['overridden']:
            controller=next(c for c in bots if c.get_controlled_pawn() and c.get_controlled_pawn().get_velocity().length()>100)
            movement=controller.get_controlled_pawn().get_component_by_class(ue.CharacterMovementComponent)
            nav=movement.get_editor_property('NavMovementProperties').copy()
            result['trial']=dict(time=now,controller=controller.get_name(),before=nav.export_text())
            # Runtime experiment only: suppress Editor reconstruction of the live
            # Pawn, which would reset its linked layer and equipment components.
            nav.set_editor_property('bUseAccelerationForPaths',True,notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
            movement.set_editor_property('NavMovementProperties',nav,notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
            result['trial']['after']=movement.get_editor_property('NavMovementProperties').export_text()
            state['overridden']=True
        if now>11:finish('observed')
    except Exception:
        details=traceback.format_exc();result['failures'].append(details)
        ue.log_error('R10 locomotion '+details);finish('failed')
state['callback']=ue.register_slate_post_tick_callback(tick)
print('R10_LOCOMOTION_PIE_ARMED')
