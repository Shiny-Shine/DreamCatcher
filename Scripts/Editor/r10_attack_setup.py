"""Approved R10-2 five new assets, isolated from the accepted passive enemy/map."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r10_enemy_setup as base
from r9_signature_foundation import blueprint
from r9_dash_noinput_fix import pin

P,R = base.P,base.R
ROOT = '/Game/DreamCatcher/GAS/Test/R10/Attack'
PATHS = {k:ROOT+'/'+n for k,n in dict(ga='GA_DC_EnemyRangedAttack', ge='GE_DC_EnemyRangedDamage',
    aset='AS_DC_EnemyRangedAttack',enemy='BP_DC_GASEnemyRanged',map='L_DC_EnemyAttackIntegration').items()}
GE_SOURCE='/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller'


def add_probe_event(bp, event_name, count_name, label):
    editor=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'EventGraph')
    y_base={'BP_OnAttackIntent':0,'BP_OnAttackShot':420,'BP_OnAttackEnded':840}[event_name]
    event=ue.BlueprintEditorLibrary.add_event_override(bp,event_name,ue.IntPoint(0,y_base))
    get=editor.add_get_member_variable_node(count_name)
    add=editor.add_call_function_node('/Script/Engine.KismetMathLibrary.Add_IntInt')
    setter=editor.add_set_member_variable_node(count_name)
    log=editor.add_call_function_node('/Script/Engine.KismetSystemLibrary.PrintString')
    assert all((event,get,add,setter,log))
    for node,x,y in ((get,230,-100),(add,430,-100),(setter,650,0),(log,880,0)): node.set_node_pos(ue.IntPoint(x,y+y_base))
    assert pin(event,'then').try_create_connection(pin(setter,'execute'))
    assert pin(get,count_name).try_create_connection(pin(add,'A'))
    assert pin(add,'ReturnValue').try_create_connection(pin(setter,count_name))
    assert pin(add,'B').set_pin_value('1')
    assert pin(setter,'then').try_create_connection(pin(log,'execute'))
    assert pin(log,'InString').set_pin_value('[R10-2 TEST] '+label)
    assert pin(log,'Duration').set_pin_value('0.4')


def create():
    ge=blueprint(PATHS['ge'],base.generated(base.load(GE_SOURCE)))
    assert ue.BlueprintEditorLibrary.compile_blueprint(ge)
    ga=blueprint(PATHS['ga'],ue.load_class(None,'/Script/DreamCatcher.DCGameplayAbility_EnemyRangedAttack'))
    ga.set_editor_property('bGenerateAbstractClass',False)
    default=base.cdo(ga)
    for name,value in dict(DamageEffectClass=base.generated(ge),DamageAmount=8.0,AttackRange=1400.0,WindupTime=.5,RecoveryTime=.75).items():
        default.set_editor_property(name,value)
    for name in ('TestIntentCount','TestShotCount','TestEndCount'):
        assert ue.BlueprintEditorLibrary.add_member_variable(ga,name,ue.BlueprintEditorLibrary.get_basic_type_by_name('int'))
    assert ue.BlueprintEditorLibrary.compile_blueprint(ga)
    for event,count,label in (('BP_OnAttackIntent','TestIntentCount','WINDUP 0.5s'),
            ('BP_OnAttackShot','TestShotCount','SHOT; recovery 0.75s'),('BP_OnAttackEnded','TestEndCount','ENDED')):
        add_probe_event(ga,event,count,label)
    assert ue.BlueprintEditorLibrary.compile_blueprint(ga)
    cls=ue.load_class(None,'/Script/DreamCatcher.DCAbilitySet')
    factory=ue.DataAssetFactory()
    factory.set_editor_property('DataAssetClass',cls)
    aset=ue.AssetToolsHelpers.get_asset_tools().create_asset(PATHS['aset'].rsplit('/',1)[1],ROOT,cls,factory)
    assert aset
    sample=base.load('/Game/DreamCatcher/GAS/Test/R5/Integration/AS_DC_RifleDash').get_editor_property('GrantedGameplayAbilities')[0]
    grant=type(sample)()
    assert grant.import_text('(Ability="/Script/Engine.BlueprintGeneratedClass\''+base.generated(ga).get_path_name()+'\'",AbilityLevel=1,InputTag=(TagName=""))')
    aset.set_editor_property('GrantedGameplayAbilities',[grant])
    enemy=blueprint(PATHS['enemy'],base.generated(base.load(base.PATHS['enemy'])))
    default=base.cdo(enemy)
    default.set_editor_property('AbilitySets',[aset])
    default.set_editor_property('RangedAttackAbilityClass',base.generated(ga))
    default.set_editor_property('bAutoRangedAttack',True)
    assert ue.BlueprintEditorLibrary.compile_blueprint(enemy)
    return dict(ga=ga,ge=ge,aset=aset,enemy=enemy)


def verify(objects):
    ga,ge,aset,enemy=(objects[k] for k in ('ga','ge','aset','enemy'))
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(ge)==base.generated(base.load(GE_SOURCE))
    expected=base.fields(base.cdo(base.load(GE_SOURCE)),('Executions','Modifiers','DurationPolicy','GameplayCues'))
    assert base.fields(base.cdo(ge),expected)==expected
    assert len(base.cdo(ge).get_editor_property('Executions'))==1
    assert not base.cdo(ge).get_editor_property('Modifiers')
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(ga).get_path_name()=='/Script/DreamCatcher.DCGameplayAbility_EnemyRangedAttack'
    assert not ga.get_editor_property('bGenerateAbstractClass')
    assert base.fields(base.cdo(ga),('DamageAmount','AttackRange','WindupTime','RecoveryTime'))==dict(DamageAmount=8.0,AttackRange=1400.0,WindupTime=.5,RecoveryTime=.75)
    assert base.cdo(ga).get_editor_property('DamageEffectClass')==base.generated(ge)
    assert base.cdo(ga).get_editor_property('NetExecutionPolicy')==ue.GameplayAbilityNetExecutionPolicy.SERVER_ONLY
    assert all(base.cdo(ga).get_editor_property(n)==0 for n in ('TestIntentCount','TestShotCount','TestEndCount'))
    grants=list(aset.get_editor_property('GrantedGameplayAbilities'))
    assert len(grants)==1 and grants[0].get_editor_property('Ability')==base.generated(ga)
    assert grants[0].get_editor_property('InputTag').export_text()=='(TagName="")'
    assert not aset.get_editor_property('GrantedGameplayEffects') and not aset.get_editor_property('GrantedAttributes')
    parent=base.load(base.PATHS['enemy'])
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(enemy)==base.generated(parent)
    assert base.fields(base.cdo(enemy),('InitialHealth','TeamId','DeathAbilityClass'))==base.fields(base.cdo(parent),('InitialHealth','TeamId','DeathAbilityClass'))
    assert list(base.cdo(enemy).get_editor_property('AbilitySets'))==[aset]
    assert base.cdo(enemy).get_editor_property('RangedAttackAbilityClass')==base.generated(ga)
    assert base.cdo(enemy).get_editor_property('bAutoRangedAttack')
    assert not base.cdo(parent).get_editor_property('bAutoRangedAttack')
    for cls,names in ((ue.SkeletalMeshComponent,('SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation')),
                      (ue.CapsuleComponent,('CapsuleRadius','CapsuleHalfHeight'))):
        original=next(c for c in base.components(parent) if isinstance(c,cls))
        copied=next(c for c in base.components(enemy) if isinstance(c,cls))
        assert base.fields(original,names)==base.fields(copied,names)
    return dict(damage_contract=expected,ability=base.fields(base.cdo(ga),('DamageAmount','AttackRange','WindupTime','RecoveryTime')),
                attack_parent=base.PATHS['enemy'],test_only_logging=True,original_passive_enemy_unchanged=True)


def make_map(objects):
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.new_level(PATHS['map'])
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('DefaultGameMode',base.generated(base.load(base.MODE)))
    actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
    floor=actors.spawn_actor_from_class(ue.StaticMeshActor,ue.Vector(0,0,-10))
    floor.static_mesh_component.set_static_mesh(base.load('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(ue.Vector(40,40,.2))
    floor.set_actor_label('R10 Attack Test Floor')
    actors.spawn_actor_from_class(ue.PlayerStart,ue.Vector(0,0,110))
    actors.spawn_actor_from_class(ue.DirectionalLight,ue.Vector(0,0,500),ue.Rotator(-55,35,0))
    spawner=actors.spawn_actor_from_class(base.generated(base.load(base.PATHS['spawner'])),ue.Vector(650,0,88),ue.Rotator(0,180,0))
    spawner.set_editor_property('GASEnemyClass',base.generated(objects['enemy']))
    spawner.set_editor_property('SpawnCount',1)
    spawner.set_actor_label('R10 Ranged Enemy - One Stationary Attacker')
    encounter=actors.spawn_actor_from_class(ue.load_class(None,'/Script/DreamCatcher.DCEncounterController'),ue.Vector(0,300,0))
    encounter.set_editor_property('Spawners',[spawner])
    encounter.set_editor_property('bAutoStartOnBeginPlay',True)
    encounter.set_actor_label('R10 Attack Encounter')
    base.clean()
    assert levels.save_current_level()


def verify_map(objects):
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.load_level(PATHS['map'])
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    assert world.get_world_settings().get_editor_property('DefaultGameMode')==base.generated(base.load(base.MODE))
    actors=ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors()
    spawners=[a for a in actors if a.get_class()==base.generated(base.load(base.PATHS['spawner']))]
    encounters=[a for a in actors if a.get_class().get_path_name()=='/Script/DreamCatcher.DCEncounterController']
    assert len(spawners)==len(encounters)==1
    assert spawners[0].get_editor_property('GASEnemyClass')==base.generated(objects['enemy'])
    assert spawners[0].get_editor_property('SpawnCount')==1
    assert list(encounters[0].get_editor_property('Spawners'))==spawners
    assert encounters[0].get_editor_property('bAutoStartOnBeginPlay')


def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR10AttackMode=(apply|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR10Run=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    base.LOG=P/'Saved/Logs'/('R10AttackAssets-'+run+'.log')
    out=R/('attack_assets_'+run)
    out.mkdir(parents=True,exist_ok=False)
    mutable=P/'Content'/ROOT[6:]
    protected_paths=[p for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS',P/'Content/Blueprint/AI',
        P/'Content/Blueprint/Stage',P/'Content/GameplayEffects/Damage') for p in root.rglob('*')
        if p.is_file() and (mode=='verify' or not p.is_relative_to(mutable))]
    protected_paths += [P/'DreamCatcher.uproject',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    protected={str(p):digest(p) for p in protected_paths}
    result=dict(complete=False,mode=mode,asset_saves=[],protected=protected)
    try:
        base.clean()
        if mode=='apply':
            assert all(not ue.EditorAssetLibrary.does_asset_exist(p) for p in PATHS.values())
            assert not list(mutable.glob('*')), 'Refuse existing/partial output'
            objects=create()
        else: objects={k:base.load(p) for k,p in PATHS.items() if k!='map'}
        for obj in objects.values():
            if isinstance(obj,ue.Blueprint): assert ue.BlueprintEditorLibrary.compile_blueprint(obj)
        result['verified']=verify(objects)
        base.clean()
        if mode=='apply':
            editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
            for obj in objects.values():
                assert obj.get_path_name().split('.')[0] in PATHS.values()
                assert editor.save_loaded_asset(obj,False)
                result['asset_saves'].append(obj.get_path_name())
            make_map(objects)
            result['asset_saves'].append(PATHS['map'])
        verify_map(objects)
        base.clean()
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_ATTACK_ASSETS '+json.dumps({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))


if __name__=='__main__': main()
