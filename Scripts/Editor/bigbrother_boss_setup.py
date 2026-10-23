"""Approved BigBrother-only asset setup. Unreal APIs only; no C++ build or PIE.

Ordinary unattended editor, -DCBigBrotherMode=inspect|apply|verify,
-DCBigBrotherRun=<fresh>, -DCBigBrotherReference=<inspect/apply run>.
Only seven new assets and the inspected Level_3 map may be saved. Partial writes
are journaled and require review, never an automatic retry over existing assets.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import time
import traceback

sys.dont_write_bytecode = True
P = Path(__file__).resolve().parents[2]
ROOT = P / 'Saved/Diagnostics/BigBrotherBoss20261024'
MAP = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_3'
PATHS = {
    'boss': '/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_BigBrotherBoss',
    'spawner': '/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_BigBrotherBossSpawner',
    'data': '/Game/DreamCatcher/Data/Boss/DA_DC_BigBrotherBoss',
    'line': '/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BigBrotherLineShot',
    'ground': '/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BigBrotherGroundStrike',
    'set': '/Game/DreamCatcher/GAS/AbilitySets/AS_DC_BigBrotherBoss',
    'mission': '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Level3Boss',
}
SOURCE = {
    'boss': '/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss',
    'spawner': '/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_BossSpawner',
    'data': '/Game/DreamCatcher/Data/Boss/DA_DC_Boss',
    'line': '/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BossLineShot',
    'ground': '/Game/DreamCatcher/GAS/Abilities/Boss/GA_DC_BossGroundStrike',
    'set': '/Game/DreamCatcher/GAS/AbilitySets/AS_DC_Boss',
    'mission': '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_LevelExample',
}
MESH = '/Game/DreamCatcher/Main_Charcher/Big_Brother/BigBrother'
MATERIAL = '/Game/DreamCatcher/Main_Charcher/Big_Brother/Material_tripo_8ee5710f_9813_4036_8bd6_534cf8d40cce'
BODY_MATERIAL = '/Game/Characters/Heroes/PhysMat_Player'
HEAD_MATERIAL = '/Game/Characters/Heroes/PhysMat_Player_WeakSpot'
EXCLUDED = {'SkeletalMeshActor_1', 'DC_ExampleSpawner', 'DC_ExampleEncounter', 'DC_ExampleStageDirector',
            'DC_ExamplePlayerStart', 'DC_ExampleNavBounds', 'DC_ExampleRecast'}
SPECIAL_PRESERVED = ('DC_ExamplePlayerStart', 'DC_ExampleNavBounds', 'DC_ExampleRecast')
# Rigid presentation-space shapes; whole-body yaw carries them together. The
# head remains a separate weak-spot component, using the accepted Lyra materials.
HIT_SHAPES = {
    'BossBodyHit': dict(kind='capsule', center=(0, 0, 2800), radius=1500, half_height=2800, weak=False),
    'BossLeftArmHit': dict(kind='box', center=(0, -2100, 5350), extent=(900, 900, 650), weak=False),
    'BossRightArmHit': dict(kind='box', center=(0, 1500, 3700), extent=(800, 600, 1500), weak=False),
    'BossHeadHit': dict(kind='box', center=(500, -450, 5750), extent=(500, 650, 550), rotation=(0, 0, -25), weak=True),
}


def read(path): return json.loads(Path(path).read_text(encoding='utf-8-sig'))
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def disk(package): return P / ('Content/' + package[6:] + ('.umap' if package == MAP else '.uasset'))


def support_module():
    spec = importlib.util.spec_from_file_location('dc_bigbrother_existing_support', Path(__file__).with_name('level_progression_setup.py'))
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


def inventory():
    allowed = {disk(p).resolve() for p in list(PATHS.values()) + [MAP]}
    result = {}
    for name in ('Content', 'Source', 'Scripts', 'Config', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (P / name).rglob('*'):
            if path.is_file() and path.resolve() not in allowed:
                info = path.stat(); result[str(path.relative_to(P))] = [info.st_size, info.st_mtime_ns]
    return result


def build_ready():
    frozen = read(ROOT / 'CppFreeze_B.json'); built = read(ROOT / 'UserBuild_B.json')
    assert built['user_reported_success'] and frozen['approved_cpp'] == built['approved_cpp']
    assert all(sha(P / name) == value for name, value in frozen['approved_cpp'].items())
    assert sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll') == built['dll_sha256']
    for name, count in (('Native_A', 7), ('Native_B', 6)):
        result = read(ROOT / name / 'Automation/index.json')
        assert result['succeeded'] == count and all(result[key] == 0 for key in ('failed', 'notRun', 'inProcess', 'succeededWithWarnings'))
    return built


def load(package):
    value = u.load_asset(package); assert value, package
    return value


def cdo(bp): return u.get_default_object(bp.generated_class())
def vec(values): return u.Vector(*values)
def actors(): return list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
def actor_named(name):
    found = [a for a in actors() if a.get_name() == name]
    assert len(found) == 1, name
    return found[0]


def components(bp):
    sub = u.get_engine_subsystem(u.SubobjectDataSubsystem)
    lib = u.SubobjectDataBlueprintFunctionLibrary
    result = []
    for handle in sub.k2_gather_subobject_data_for_blueprint(bp):
        obj = lib.get_object_for_blueprint(lib.get_data(handle), bp)
        if isinstance(obj, u.ActorComponent): result.append((handle, obj))
    return result


def owned_component_targets(bp, component_name):
    values = {obj.get_path_name(): obj for _, obj in components(bp) if obj.get_name() == component_name}
    for obj in cdo(bp).get_components_by_class(u.ActorComponent):
        if obj.get_name() == component_name: values[obj.get_path_name()] = obj
    assert values
    assert all(obj.get_path_name().startswith(bp.get_path_name().split('.')[0] + '.') for obj in values.values()), 'Shared parent component edit refused'
    return list(values.values())


def add_component(bp, cls, name, parent_name):
    sub = u.get_engine_subsystem(u.SubobjectDataSubsystem)
    lib = u.SubobjectDataBlueprintFunctionLibrary
    parents = [(handle, obj) for handle, obj in components(bp) if obj.get_name() == parent_name]
    assert len(parents) == 1, (parent_name, [o.get_name() for _, o in components(bp)])
    handle, reason = sub.add_new_subobject(u.AddNewSubobjectParams(parent_handle=parents[0][0], new_class=cls, blueprint_context=bp))
    assert not str(reason), str(reason)
    assert sub.rename_subobject(handle, name)
    obj = lib.get_object_for_blueprint(lib.get_data(handle), bp)
    assert obj and obj.get_outer() == bp.generated_class()
    return obj


def setup_mesh(obj, scale, preview=False):
    obj.set_editor_property('PhysicsAssetOverride', None)
    obj.set_skeletal_mesh_asset(load(MESH))
    obj.set_editor_property('AnimClass', None)
    obj.set_editor_property('AnimationMode', u.AnimationMode.ANIMATION_SINGLE_NODE)
    animation = obj.get_editor_property('AnimationData')
    animation.set_editor_property('AnimToPlay', None)
    animation.set_editor_property('bSavedPlaying', False)
    animation.set_editor_property('bSavedLooping', False)
    obj.set_editor_property('AnimationData', animation)
    obj.set_editor_property('bPauseAnims', True)
    obj.set_editor_property('RelativeLocation', vec((0, 0, 0)))
    obj.set_editor_property('RelativeRotation', u.Rotator(0, 0, 0))
    obj.set_editor_property('RelativeScale3D', vec(scale))
    obj.set_material(0, load(MATERIAL))
    obj.set_simulate_physics(False)
    obj.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    obj.set_editor_property('bHiddenInGame', preview)
    obj.set_editor_property('ComponentTags', [u.Name('DC.BigBrother.Preview' if preview else 'DC.BigBrother.Visual')])


def setup_hit_shape(obj, shape):
    obj.set_editor_property('RelativeLocation', vec(shape['center']))
    rotation = shape.get('rotation', (0, 0, 0))
    obj.set_editor_property('RelativeRotation', u.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]))
    if shape['kind'] == 'capsule': obj.set_capsule_size(shape['radius'], shape['half_height'])
    else: obj.set_box_extent(vec(shape['extent']))
    obj.set_collision_enabled(u.CollisionEnabled.QUERY_ONLY)
    obj.set_collision_object_type(u.CollisionChannel.cast(2))  # ECC_Pawn
    obj.set_collision_response_to_all_channels(u.CollisionResponseType.cast(0))
    for channel in (3, 15, 16, 17):  # Visibility, original Lyra Weapon/Weapon_Capsule/Weapon_Multi
        obj.set_collision_response_to_channel(u.CollisionChannel.cast(channel), u.CollisionResponseType.cast(2))
    obj.call_method('SetGenerateOverlapEvents', args=(False,))
    obj.set_phys_material_override(load(HEAD_MATERIAL if shape['weak'] else BODY_MATERIAL))
    obj.set_editor_property('ComponentTags', [u.Name('DC.BigBrother.Head' if shape['weak'] else 'DC.BigBrother.Body')])


def new_child(key):
    bp = u.BlueprintEditorLibrary.create_blueprint_asset_with_parent(PATHS[key], load(SOURCE[key]).generated_class())
    assert bp
    return bp


def duplicate(key):
    name, folder = PATHS[key].rsplit('/', 1)[1], PATHS[key].rsplit('/', 1)[0]
    obj = u.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, folder, load(SOURCE[key])); assert obj
    return obj


def make_assets(plan):
    assets = {}
    for key in ('line', 'ground'):
        assets[key] = new_child(key); cdo(assets[key]).set_editor_property('AttackRange', 10000.0)
        assert u.BlueprintEditorLibrary.compile_blueprint(assets[key])
    assets['set'] = duplicate('set')
    grants = list(assets['set'].get_editor_property('GrantedGameplayAbilities')); assert len(grants) == 2
    for row, key in zip(grants, ('line', 'ground')):
        # EditDefaultsOnly rejects a setter on a Python struct copy. Import the
        # existing Unreal struct text with just its approved class reference changed.
        original_class = load(SOURCE[key]).generated_class().get_path_name()
        text = row.export_text(); assert original_class in text
        assert row.import_text(text.replace(original_class, assets[key].generated_class().get_path_name()))
    assets['set'].set_editor_property('GrantedGameplayAbilities', grants)
    assets['data'] = duplicate('data')
    assets['data'].set_editor_property('DisplayName', u.Text('BigBrother'))
    assets['data'].set_editor_property('Subtitle', u.Text(''))
    assets['mission'] = duplicate('mission')
    mission = u.DCMissionDefinition()
    assert mission.import_text('(MissionId=DefeatBigBrother,ProgressSource=CurrentEncounterDefeats,TargetCount=1,bEnabled=True)')
    mission.set_editor_property('DisplayText', u.Text('BigBrother 처치하기'))
    assets['mission'].set_editor_property('Missions', [mission])
    assets['mission'].set_editor_property('DefaultTrackedMissionId', u.Name('DefeatBigBrother'))
    assets['boss'] = new_child('boss')
    defaults = cdo(assets['boss'])
    for name, value in dict(BossData=assets['data'], RangedAttackAbilityClass=assets['line'].generated_class(),
            GroundStrikeAbilityClass=assets['ground'].generated_class(), AbilitySets=[assets['set']],
            bAlwaysFacePlayer=True, bUseCustomAttackOrigin=True, AttackOriginOffset=vec((0, 0, 5500))).items():
        defaults.set_editor_property(name, value)
    for mesh in owned_component_targets(assets['boss'], 'CharacterMesh0'): setup_mesh(mesh, plan['scale'])
    for name, shape in HIT_SHAPES.items():
        component = add_component(assets['boss'], u.CapsuleComponent if shape['kind'] == 'capsule' else u.BoxComponent, name, 'CollisionCylinder')
        setup_hit_shape(component, shape)
    assert u.BlueprintEditorLibrary.compile_blueprint(assets['boss'])
    assets['spawner'] = new_child('spawner')
    cdo(assets['spawner']).set_editor_property('GASEnemyClass', assets['boss'].generated_class())
    preview = add_component(assets['spawner'], u.SkeletalMeshComponent, 'BigBrotherPreview', 'Root')
    setup_mesh(preview, plan['scale'], preview=True)
    assert u.BlueprintEditorLibrary.compile_blueprint(assets['spawner'])
    return assets


def verify_assets(plan, directory):
    assets = {key: load(path) for key, path in PATHS.items()}
    for key in ('boss', 'spawner', 'line', 'ground'): assert u.BlueprintEditorLibrary.compile_blueprint(assets[key])
    boss, spawner = cdo(assets['boss']), cdo(assets['spawner'])
    assert boss.get_editor_property('bAlwaysFacePlayer') and boss.get_editor_property('bUseCustomAttackOrigin')
    assert boss.get_editor_property('AttackOriginOffset').distance(vec((0, 0, 5500))) < .001
    for field, key in (('BossData', 'data'), ('RangedAttackAbilityClass', 'line'), ('GroundStrikeAbilityClass', 'ground')):
        assert boss.get_editor_property(field) == (assets[key].generated_class() if key in ('line', 'ground') else assets[key])
    assert list(boss.get_editor_property('AbilitySets')) == [assets['set']]
    assert spawner.get_editor_property('GASEnemyClass') == assets['boss'].generated_class()
    assert not spawner.get_editor_property('LyraBotControllerClass') and spawner.get_editor_property('bCleanupOwnedGASNPCs')
    assert spawner.get_editor_property('SpawnCount') == spawner.get_editor_property('MaxAliveAtOnce') == 1
    assert not spawner.get_editor_property('bRequirePlayerProximity') and not spawner.get_editor_property('bActivateOnBeginPlay')
    source_data = load(SOURCE['data'])
    for field in ('MaxHealth', 'PhaseTwoHealthRatio', 'OpeningDelay', 'TransitionDuration', 'PhaseOneGap', 'PhaseTwoGap', 'HUDStyle', 'WarningMaterial', 'ImpactMaterial'):
        left, right = assets['data'].get_editor_property(field), source_data.get_editor_property(field)
        assert (left.export_text() == right.export_text()) if hasattr(left, 'export_text') else left == right, field
    for key in ('line', 'ground'):
        current, original = cdo(assets[key]), cdo(load(SOURCE[key]))
        assert current.get_editor_property('AttackRange') == 10000.0
        for field in ('DamageAmount', 'WindupTime', 'RecoveryTime', 'DamageEffectClass') + (('Radius', 'VerticalTolerance') if key == 'ground' else ()):
            assert current.get_editor_property(field) == original.get_editor_property(field), (key, field)
    for mesh in owned_component_targets(assets['boss'], 'CharacterMesh0'):
        assert mesh.get_editor_property('SkeletalMesh') == load(MESH) and mesh.get_editor_property('PhysicsAssetOverride') is None
        assert mesh.get_editor_property('AnimClass') is None and mesh.get_editor_property('bPauseAnims')
        assert mesh.get_editor_property('RelativeLocation').length() < .001
        assert mesh.get_editor_property('RelativeScale3D').distance(vec(plan['scale'])) < .001
        assert mesh.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
    shapes = {obj.get_name().removesuffix('_GEN_VARIABLE'): obj for _, obj in components(assets['boss']) if obj.get_name().removesuffix('_GEN_VARIABLE') in HIT_SHAPES}
    assert set(shapes) == set(HIT_SHAPES)
    for name, obj in shapes.items():
        shape = HIT_SHAPES[name]
        assert obj.get_editor_property('RelativeLocation').distance(vec(shape['center'])) < .001
        rotation = obj.get_editor_property('RelativeRotation')
        assert max(abs(a - b) for a, b in zip((rotation.pitch, rotation.yaw, rotation.roll), shape.get('rotation', (0, 0, 0)))) < .001
        if shape['kind'] == 'box': assert obj.call_method('GetUnscaledBoxExtent').distance(vec(shape['extent'])) < .001
        else:
            assert abs(obj.get_scaled_capsule_radius() - shape['radius']) < .001
            assert abs(obj.get_scaled_capsule_half_height() - shape['half_height']) < .001
        assert obj.get_collision_enabled() == u.CollisionEnabled.QUERY_ONLY
        assert obj.get_collision_response_to_channel(u.CollisionChannel.cast(15)) == u.CollisionResponseType.cast(2)
        assert obj.get_collision_response_to_channel(u.CollisionChannel.cast(2)) == u.CollisionResponseType.cast(0)
        serialized = evidence.hud.export(obj, directory / (name + '_hit_shape.t3d'))
        expected_material = HEAD_MATERIAL if shape['weak'] else BODY_MATERIAL
        assert 'PhysMaterialOverride=' in serialized and expected_material + '.' in serialized
    definitions = list(assets['mission'].get_editor_property('Missions')); assert len(definitions) == 1
    assert definitions[0].get_editor_property('TargetCount') == 1 and str(definitions[0].get_editor_property('MissionId')) == 'DefeatBigBrother'
    return assets


def map_plan(world):
    big = actor_named('SkeletalMeshActor_1')
    assert big.get_actor_label(False) == 'BigBrother' and big.get_class() == u.SkeletalMeshActor.static_class()
    mesh = big.get_component_by_class(u.SkeletalMeshComponent)
    assert mesh.get_editor_property('SkeletalMesh') == load(MESH)
    assert mesh.get_material(0) == load(MATERIAL)
    assert not big.get_attach_parent_actor(), 'Review an attached presentation before replacing it'
    rotation = big.get_actor_rotation(); assert abs(rotation.pitch) < .001 and abs(rotation.roll) < .001
    scale = big.get_actor_scale3d(); assert min(scale.x, scale.y, scale.z) > 0
    spawner, encounter, director = (actor_named(name) for name in ('DC_ExampleSpawner', 'DC_ExampleEncounter', 'DC_ExampleStageDirector'))
    assert spawner.get_editor_property('LyraBotControllerClass') and not spawner.get_editor_property('GASEnemyClass')
    assert spawner.get_editor_property('SpawnCount') == spawner.get_editor_property('MaxAliveAtOnce') == 1
    assert list(encounter.get_editor_property('Spawners')) == [spawner]
    phases = list(director.get_editor_property('Phases'))
    assert len(phases) == 2 and phases[0].get_editor_property('Encounter') == encounter
    assert phases[0].get_editor_property('PhaseType') == u.DCStagePhaseType.ENCOUNTER
    assert phases[1].get_editor_property('PhaseType') == u.DCStagePhaseType.COMPLETE
    position = big.get_actor_location()
    return dict(position=[position.x, position.y, position.z], rotation=[0, rotation.yaw, 0],
                scale=[scale.x, scale.y, scale.z], visual_transform=mesh.get_world_transform().export_text(),
                actor_count=len(actors()), old_visual=big.get_path_name(), old_spawner=spawner.get_path_name())


def environment(world, directory, label, expected=None):
    selected = expected or {a.get_path_name() for a in actors() if a.get_name() not in EXCLUDED and a.get_name() != 'DC_BigBrotherSpawner'}
    return support.snapshot(world, directory, label, None, expected_paths=selected)


def special_exports(directory, label):
    return {name: evidence.hud.export(actor_named(name), directory / (label + '_' + name + '.t3d')) for name in SPECIAL_PRESERVED}


def check_map(plan, assets):
    assert not any(a.get_name() in ('SkeletalMeshActor_1', 'DC_ExampleSpawner') for a in actors())
    spawner = actor_named('DC_BigBrotherSpawner')
    assert spawner.get_class() == assets['spawner'].generated_class()
    assert spawner.get_actor_location().distance(vec(plan['position'])) < .001
    assert spawner.get_actor_scale3d().distance(vec((1, 1, 1))) < .001
    preview = spawner.get_component_by_class(u.SkeletalMeshComponent); assert preview
    # Equivalent quaternions can export signed zero differently after replacing
    # a SkeletalMeshActor with the spawner. Compare the actual transform values.
    assert preview.get_world_location().distance(vec(plan['position'])) < .001
    assert preview.get_editor_property('RelativeLocation').length() < .001
    assert preview.get_editor_property('RelativeScale3D').distance(vec(plan['scale'])) < .001
    local_rotation = preview.get_editor_property('RelativeRotation')
    assert max(abs(local_rotation.pitch), abs(local_rotation.yaw), abs(local_rotation.roll)) < .001
    rotation = spawner.get_actor_rotation()
    assert abs(rotation.pitch) < .001 and abs(rotation.roll) < .001
    assert abs((rotation.yaw - plan['rotation'][1] + 180) % 360 - 180) < .001
    assert preview.get_editor_property('bHiddenInGame') and preview.get_collision_enabled() == u.CollisionEnabled.NO_COLLISION
    encounter, director = actor_named('DC_ExampleEncounter'), actor_named('DC_ExampleStageDirector')
    assert list(encounter.get_editor_property('Spawners')) == [spawner]
    phases = list(director.get_editor_property('Phases'))
    assert len(phases) == 2 and phases[0].get_editor_property('PhaseType') == u.DCStagePhaseType.BOSS_ENCOUNTER
    assert phases[0].get_editor_property('Encounter') == encounter and phases[0].get_editor_property('MissionList') == assets['mission']
    assert phases[1].get_editor_property('PhaseType') == u.DCStagePhaseType.COMPLETE
    assert len(actors()) == plan['actor_count'] - 1
    return dict(actor_count=len(actors()), boss_spawner=spawner.get_path_name(), original_visual_transform_preserved=True,
                original_visual_transform=plan['visual_transform'], actual_visual_transform=preview.get_world_transform().export_text())


def main():
    global u, support, evidence
    import unreal as u
    command = u.SystemLibrary.get_command_line()
    mode = re.search(r'-DCBigBrotherMode=(inspect|apply|verify)\b', command).group(1)
    run = re.search(r'-DCBigBrotherRun=([A-Za-z0-9_]+)\b', command).group(1)
    reference_match = re.search(r'-DCBigBrotherReference=([A-Za-z0-9_]+)\b', command)
    resume_saved = '-DCBigBrotherResumeSavedF' in command
    fit_match = re.search(r'-DCBigBrotherHitboxFit=([A-Za-z0-9_]+)\b', command)
    assert not resume_saved or mode == 'apply'
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert Path(u.Paths.project_dir()).resolve() == P.resolve() and not u.SourceControl.is_enabled()
    support = support_module(); support.ue = u
    evidence = support.helpers(); support.evidence = evidence
    evidence.hud.assert_no_other_editor()
    built = build_ready()
    directory = ROOT / run; directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'report.json'; assert not output.exists(), 'Preserve an existing run journal'
    protected, before = support.protected_hashes(), inventory()
    originals = {p: sha(disk(p)) for p in list(SOURCE.values()) + [MESH, MATERIAL, BODY_MATERIAL, HEAD_MATERIAL]}
    original_map = sha(disk(MAP))
    reference = read(ROOT / reference_match.group(1) / 'report.json') if reference_match else None
    report = dict(complete=False, mode=mode, saved=[], asset_writes=0, PIE=False, cpp_build=False,
                  dll_sha256=built['dll_sha256'], source_hashes=originals, map_input_sha256=original_map)
    def persist(): output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    def save(obj, package):
        assert package in list(PATHS.values()) + [MAP] and package not in report['saved']
        if package == MAP: assert u.EditorLoadingAndSavingUtils.save_map(obj, package)
        else: assert u.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
        report['saved'].append(package); report['asset_writes'] += 1; persist()
    state = dict(start=time.monotonic(), done=False)
    def tick(dt):
        if state['done'] or time.monotonic() - state['start'] < 2: return
        state['done'] = True
        try:
            settings = u.get_default_object(u.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
            text = evidence.hud.export(settings, directory / 'compile_settings.t3d')
            assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', text, re.M) == ['SoC_Never']
            world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
            assert world.get_path_name() == MAP + '.Level_3'
            if mode != 'verify' and not resume_saved:
                assert all(not disk(p).exists() and not u.EditorAssetLibrary.does_asset_exist(p) for p in PATHS.values()), 'New destination already exists; inspect any partial write first'
            if mode == 'inspect':
                report['plan'] = map_plan(world)
                report['environment'] = environment(world, directory, 'before')
                report['special'] = special_exports(directory, 'before')
                serialized = json.dumps(report['environment']['actors'])
                assert report['plan']['old_visual'] not in serialized and report['plan']['old_spawner'] not in serialized, 'An environment actor references a replaced actor'
            elif mode == 'apply':
                assert reference and reference['mode'] == 'inspect' and reference['complete']
                assert reference['source_hashes'] == originals and reference['map_input_sha256'] == original_map
                plan = map_plan(world); assert plan == reference['plan']
                report['plan'] = plan; report['environment'] = reference['environment']; report['special'] = reference['special']
                source_defaults = {key: evidence.hud.export(cdo(load(SOURCE[key])), directory / ('source_' + key + '_before.t3d')) for key in ('boss', 'spawner', 'line', 'ground')}
                if resume_saved:
                    partial = read(ROOT / 'Partial_F_Assets.json')
                    assert set(partial['saved_hashes']) == set(PATHS.values())
                    assert partial['map_sha256_unchanged'] == original_map and partial['source_hashes'] == originals
                    assert all(sha(disk(p)) == h for p, h in partial['saved_hashes'].items())
                    assets = verify_assets(plan, directory)
                    report['reused_saved_assets'] = partial['saved_hashes']
                    report['resume_source'] = 'Partial_F_Assets.json'
                else:
                    assets = make_assets(plan)
                    verify_assets(plan, directory)
                for key, text in source_defaults.items():
                    assert evidence.hud.export(cdo(load(SOURCE[key])), directory / ('source_' + key + '_after.t3d')) == text, 'Source CDO changed: ' + key
                if not resume_saved:
                    for key in ('line', 'ground', 'set', 'data', 'mission', 'boss', 'spawner'): save(assets[key], PATHS[key])
                spawner = support.spawn(world, assets['spawner'].generated_class(), 'DC_BigBrotherSpawner', plan['position'], plan['rotation'])
                spawner.set_actor_label('BigBrother')
                encounter, director = actor_named('DC_ExampleEncounter'), actor_named('DC_ExampleStageDirector')
                encounter.set_editor_property('Spawners', [spawner])
                phases = list(director.get_editor_property('Phases'))
                phases[0].set_editor_property('PhaseType', u.DCStagePhaseType.BOSS_ENCOUNTER)
                phases[0].set_editor_property('PhaseTitle', u.Text('BigBrother 보스전'))
                phases[0].set_editor_property('MissionList', assets['mission'])
                director.set_editor_property('Phases', phases)
                editor = u.get_editor_subsystem(u.EditorActorSubsystem)
                assert editor.destroy_actor(actor_named('DC_ExampleSpawner'))
                assert editor.destroy_actor(actor_named('SkeletalMeshActor_1'))
                report['preview_transform_before_check'] = spawner.get_component_by_class(u.SkeletalMeshComponent).get_world_transform().export_text()
                persist()
                report['map_check'] = check_map(plan, assets)
                after = environment(world, directory, 'after', set(reference['environment']['actors']) | {world.get_world_settings().get_path_name()})
                report['environment_comparison'] = evidence.compare_maps(reference['environment'], after, cross_process=True)
                assert reference['environment']['world_settings'] == after['world_settings']
                assert special_exports(directory, 'after') == reference['special']
                save(world, MAP)
                report['saved_hashes'] = {p: sha(disk(p)) for p in list(PATHS.values()) + [MAP]}
            else:
                assert reference and reference['mode'] == 'apply' and reference['complete']
                expected_hashes = dict(reference['saved_hashes'])
                if fit_match:
                    fit = read(ROOT / fit_match.group(1) / 'report.json')
                    assert fit['complete'] and fit['saved'] == [PATHS['boss']]
                    assert fit['before_sha256'] == expected_hashes[PATHS['boss']] and fit['map_sha256'] == expected_hashes[MAP]
                    expected_hashes[PATHS['boss']] = fit['after_sha256']
                    report['hitbox_fit_journal'] = fit_match.group(1)
                assert all(sha(disk(p)) == h for p, h in expected_hashes.items())
                assets = verify_assets(reference['plan'], directory)
                report['map_check'] = check_map(reference['plan'], assets)
                after = environment(world, directory, 'reload', set(reference['environment']['actors']) | {world.get_world_settings().get_path_name()})
                report['environment_comparison'] = evidence.compare_maps(reference['environment'], after, cross_process=True)
                assert reference['environment']['world_settings'] == after['world_settings']
                assert special_exports(directory, 'reload') == reference['special']
                assert all(sha(disk(p)) == h for p, h in expected_hashes.items())
            report['complete'] = True
        except Exception:
            report['failure'] = traceback.format_exc(); u.log_error(report['failure'])
        finally:
            after_files = inventory()
            report['protected_inventory_changes'] = [name for name in sorted(set(before) | set(after_files)) if before.get(name) != after_files.get(name)]
            report['protected_hashes_unchanged'] = support.protected_hashes() == protected
            report['original_assets_unchanged'] = all(sha(disk(p)) == h for p, h in originals.items())
            report['read_only_map_unchanged'] = mode == 'apply' or sha(disk(MAP)) == original_map
            report['complete'] &= not report['protected_inventory_changes'] and report['protected_hashes_unchanged'] and report['original_assets_unchanged'] and report['read_only_map_unchanged']
            persist()
            print('DC_BIGBROTHER_SETUP ' + json.dumps({key: report[key] for key in ('complete', 'mode', 'saved')}))
            u.unregister_slate_post_tick_callback(state['handle'])
            u.EditorPythonScripting.set_keep_python_script_alive(False)
            u.SystemLibrary.quit_editor()
    state['handle'] = u.register_slate_post_tick_callback(tick)
    u.EditorPythonScripting.set_keep_python_script_alive(True)


if __name__ == '__main__': main()
