"""R10-1 bounded NPC assets. UE API only; no build, existing-asset save or global changes."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r7_root_ui_setup as reader
import r9_dash_assets as audit
from r9_signature_foundation import blueprint

P = Path(__file__).resolve().parents[2]
R = P / 'Saved/Diagnostics/R10'
ROOT = '/Game/DreamCatcher/GAS/Test/R10'
OLD = '/Game/Blueprint/AI/BP_EnemyCharacter'
DEATH_SOURCE = '/Game/Characters/Heroes/Abilities/GA_Hero_Death'
MODE = '/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleGameMode'
PATHS = {k: ROOT+'/'+v for k,v in dict(death='GA_DC_EnemyDeath', enemy='BP_DC_GASEnemy',
    spawner='BP_DC_GASEnemySpawner', map='L_DC_EnemyIntegration').items()}


def load(path):
    obj = ue.load_asset(path)
    assert obj, path
    return obj


def generated(bp):
    return ue.BlueprintEditorLibrary.generated_class(bp)


def cdo(bp):
    return ue.get_default_object(generated(bp))


def encode(value):
    if value is None or isinstance(value, (str, float, int, bool)): return value
    if isinstance(value, ue.Object): return value.get_path_name()
    if isinstance(value, (list, tuple, ue.Array)): return [encode(x) for x in value]
    return value.export_text() if hasattr(value, 'export_text') else str(value)


def fields(obj, names):
    return {n: encode(obj.get_editor_property(n)) for n in names}


def components(bp):
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    objects = []
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = ue.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = ue.SubobjectDataBlueprintFunctionLibrary.get_object_for_blueprint(data, bp)
        if isinstance(obj, ue.ActorComponent): objects.append(obj)
    return objects


def clean():
    assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0
    errors = [line for line in LOG.read_text(encoding='utf-8-sig',errors='replace').splitlines()
              if re.search(r': Error:|Fatal:|Ensure condition failed|Invalid GameplayTag',line)]
    assert not errors, errors[:3]


def camera_value(obj, value=None, write=False):
    # Blueprint variable names retain spaces, unlike their Python display aliases.
    if write: obj.set_editor_property('Camera Mode', value)
    return obj.get_editor_property('Camera Mode')


def create_assets(source_death, source_enemy):
    death = blueprint(PATHS['death'], generated(source_death))
    camera_value(cdo(death), write=True)
    cdo(death).set_editor_property('Duration', cdo(source_enemy).get_editor_property('DestroyDelay'))
    assert ue.BlueprintEditorLibrary.compile_blueprint(death)
    enemy = blueprint(PATHS['enemy'], ue.load_class(None,'/Script/DreamCatcher.DCGASEnemyCharacter'))
    default = cdo(enemy)
    default.set_editor_property('DeathAbilityClass', generated(death))
    default.set_editor_property('InitialHealth',100.0)
    default.set_editor_property('TeamId',1)
    default.set_editor_property('AbilitySets',[])
    old_components = components(source_enemy)
    for obj in components(enemy):
        assert obj.get_path_name().startswith(PATHS['enemy']+'.'), obj.get_path_name()
        if isinstance(obj, ue.SkeletalMeshComponent):
            old = next(c for c in old_components if isinstance(c,ue.SkeletalMeshComponent))
            for name in ('SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation','RelativeScale3D'):
                obj.set_editor_property(name,old.get_editor_property(name))
        elif isinstance(obj, ue.CapsuleComponent):
            old = next(c for c in old_components if isinstance(c,ue.CapsuleComponent))
            for name in ('CapsuleRadius','CapsuleHalfHeight'): obj.set_editor_property(name,old.get_editor_property(name))
        elif isinstance(obj, ue.CharacterMovementComponent):
            obj.set_editor_property('MaxWalkSpeed',cdo(source_enemy).get_editor_property('MoveSpeed'))
    assert ue.BlueprintEditorLibrary.compile_blueprint(enemy)
    spawner = blueprint(PATHS['spawner'],ue.load_class(None,'/Script/DreamCatcher.DCEnemySpawner'))
    for name,value in dict(GASEnemyClass=generated(enemy), EnemyClass=None, SpawnCount=3,
                           MaxAliveAtOnce=1,SpawnInterval=1.0,bActivateOnBeginPlay=False).items():
        cdo(spawner).set_editor_property(name,value)
    assert ue.BlueprintEditorLibrary.compile_blueprint(spawner)
    return dict(death=death,enemy=enemy,spawner=spawner)


def verify_assets(objects, source_death, source_enemy):
    death, enemy, spawner = (objects[k] for k in ('death','enemy','spawner'))
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(death) == generated(source_death)
    assert camera_value(cdo(death)) is None
    assert cdo(death).get_editor_property('Duration') == cdo(source_enemy).get_editor_property('DestroyDelay') == 2.0
    assert fields(cdo(death),('AbilityTriggers','bAutoStartDeath','NetExecutionPolicy','CancelAbilitiesWithTag','BlockAbilitiesWithTag')) == fields(cdo(source_death),('AbilityTriggers','bAutoStartDeath','NetExecutionPolicy','CancelAbilitiesWithTag','BlockAbilitiesWithTag'))
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(enemy).get_path_name() == '/Script/DreamCatcher.DCGASEnemyCharacter'
    assert cdo(enemy).get_editor_property('DeathAbilityClass') == generated(death)
    assert cdo(enemy).get_editor_property('InitialHealth') == 100.0 and cdo(enemy).get_editor_property('TeamId') == 1
    assert not cdo(enemy).get_editor_property('AbilitySets')
    old = components(source_enemy)
    actual = components(enemy)
    for cls,names in ((ue.SkeletalMeshComponent,('SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation','RelativeScale3D')),
                      (ue.CapsuleComponent,('CapsuleRadius','CapsuleHalfHeight'))):
        assert fields(next(c for c in actual if isinstance(c,cls)),names) == fields(next(c for c in old if isinstance(c,cls)),names)
    assert len([c for c in actual if isinstance(c,ue.AbilitySystemComponent)]) == 1
    assert not any(c.get_class().get_path_name() == '/Script/DreamCatcher.DCHealthComponent' for c in actual)
    expected = dict(GASEnemyClass=generated(enemy).get_path_name(),EnemyClass=None,SpawnCount=3,MaxAliveAtOnce=1,SpawnInterval=1.0,bActivateOnBeginPlay=False)
    assert fields(cdo(spawner),expected) == expected
    return dict(death_parent=encode(generated(source_death)),death_duration=2.0,camera=None,
        enemy=fields(cdo(enemy),('DeathAbilityClass','InitialHealth','TeamId','AbilitySets')),spawner=expected,
        original_death_graph_inherited=True,legacy_mesh_and_transform_preserved=True,animation_class=None)


def create_map(objects):
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.new_level(PATHS['map'])
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('DefaultGameMode',generated(load(MODE)))
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    floor = actors.spawn_actor_from_class(ue.StaticMeshActor,ue.Vector(0,0,-10))
    floor.static_mesh_component.set_static_mesh(load('/Engine/BasicShapes/Cube'))
    floor.set_actor_scale3d(ue.Vector(40,40,.2))
    floor.set_actor_label('R10 Test Floor')
    actors.spawn_actor_from_class(ue.PlayerStart,ue.Vector(0,0,110))
    actors.spawn_actor_from_class(ue.DirectionalLight,ue.Vector(0,0,500),ue.Rotator(-55,35,0))
    spawner = actors.spawn_actor_from_class(generated(objects['spawner']),ue.Vector(650,0,88),ue.Rotator(0,180,0))
    spawner.set_actor_label('R10 GAS Enemy Spawner - 3 sequential')
    encounter = actors.spawn_actor_from_class(ue.load_class(None,'/Script/DreamCatcher.DCEncounterController'),ue.Vector(0,300,0))
    encounter.set_actor_label('R10 Encounter')
    encounter.set_editor_property('Spawners',[spawner])
    encounter.set_editor_property('bAutoStartOnBeginPlay',True)
    clean()
    assert levels.save_current_level()


def verify_map(objects):
    levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.load_level(PATHS['map'])
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    assert world.get_world_settings().get_editor_property('DefaultGameMode') == generated(load(MODE))
    actors = ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors()
    spawners = [a for a in actors if a.get_class() == generated(objects['spawner'])]
    encounters = [a for a in actors if a.get_class().get_path_name() == '/Script/DreamCatcher.DCEncounterController']
    assert len(spawners) == len(encounters) == 1
    assert list(encounters[0].get_editor_property('Spawners')) == spawners
    assert encounters[0].get_editor_property('bAutoStartOnBeginPlay')
    assert (spawners[0].get_actor_location()-ue.Vector(650,0,88)).length() < .01
    return dict(map=PATHS['map'],game_mode=MODE,spawners=1,encounters=1)


def main():
    global LOG
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR10Mode=(inspect|apply|verify)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR10Run=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    LOG = P/'Saved/Logs'/('R10EnemyAssets-'+run+'.log')
    out = R/('assets_'+run)
    out.mkdir(parents=True, exist_ok=False)
    protected_paths = [p for root in (P/'Source', P/'Config', P/'Content/Blueprint/AI',
        P/'Content/Blueprint/Stage', P/'Content/DreamCatcher/GAS/Test/R5/Integration',
        P/'Content/DreamCatcher/GAS/Test/R9', P/'Content/Characters/Heroes/Abilities')
        for p in root.rglob('*') if p.is_file()]
    protected_paths += [P/'DreamCatcher.uproject', P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    if mode == 'verify': protected_paths += [p for p in (P/'Content/DreamCatcher/GAS/Test/R10').rglob('*') if p.is_file()]
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    protected = {str(p): digest(p) for p in protected_paths}
    result = dict(complete=False, mode=mode, asset_saves=[], protected=protected)
    try:
        death = load(DEATH_SOURCE)
        enemy = load(OLD)
        result['source_death'] = dict(parent=encode(ue.BlueprintEditorLibrary.get_blueprint_parent_class(death)),
            defaults=audit.scope['reflected_fields'](cdo(death))['values'],
            graphs=reader.snapshot(death))
        result['legacy_enemy'] = fields(cdo(enemy), ('MoveSpeed','DestroyDelay','AutoPossessAI'))
        result['components'] = []
        for obj in components(enemy):
            names = []
            if isinstance(obj, ue.SkeletalMeshComponent): names = ['SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation','RelativeScale3D']
            elif isinstance(obj, ue.CapsuleComponent): names = ['CapsuleRadius','CapsuleHalfHeight']
            if names: result['components'].append(dict(path=obj.get_path_name(), cls=obj.get_class().get_path_name(), fields=fields(obj,names)))
        assert load(MODE)
        if mode != 'inspect':
            source_graphs = reader.snapshot(death)
            # Fail before saving if source Blueprint variables or API contracts differ.
            result['source_parameters'] = dict(camera=encode(camera_value(cdo(death))),duration=cdo(death).get_editor_property('Duration'))
            if mode == 'apply':
                assert all(not ue.EditorAssetLibrary.does_asset_exist(p) for p in PATHS.values()), 'Fresh destinations only'
                assert not list((P/'Content/DreamCatcher/GAS/Test/R10').glob('*')), 'Refuse preexisting/partial output'
                objects = create_assets(death,enemy)
            else: objects = {k:load(p) for k,p in PATHS.items() if k != 'map'}
            for bp in objects.values(): assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            result['verified'] = verify_assets(objects,death,enemy)
            assert reader.snapshot(death) == source_graphs
            clean()
            if mode == 'apply':
                editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
                for bp in objects.values():
                    assert bp.get_path_name().split('.')[0] in PATHS.values()
                    assert editor.save_loaded_asset(bp,False)
                    result['asset_saves'].append(bp.get_path_name())
                create_map(objects)
                result['asset_saves'].append(PATHS['map'])
            result['map'] = verify_map(objects)
        clean()
        result['complete'] = True
    finally:
        result['protected_changed'] = [p for p,h in protected.items() if digest(Path(p)) != h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_ASSETS '+json.dumps({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))


if __name__ == '__main__': main()
