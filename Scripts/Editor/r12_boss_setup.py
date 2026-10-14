"""Approved R12 assets only, AFTER the user's C++ build.

Commandlet: -DCR12BossSetup=apply|verify -DCR12BossRun=<fresh receipt>
No external binary copying. Existing R11/player/config/assets are read-only.
Partial output requires inspection; apply never overwrites an existing R12 folder.
"""
import hashlib
import json
import re
import sys
import traceback
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue

P = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(P/'Scripts/Editor'))
from r10_enemy_setup import load, cdo, generated, components
from r9_signature_foundation import blueprint
from r10_lyra_ai_map import clean
from r11_stage_setup import environment

ROOT = '/Game/DreamCatcher/GAS/Test/R12'
SOURCE = '/Game/DreamCatcher/GAS/Test/R11/L_DC_Level1Flow'
NPC = '/Game/DreamCatcher/GAS/Test/R10/BP_DC_GASEnemy'
DEATH = '/Game/DreamCatcher/GAS/Test/R10/GA_DC_EnemyDeath'
DAMAGE = '/Game/DreamCatcher/GAS/Test/R10/Attack/GE_DC_EnemyRangedDamage'
PATHS = {k:ROOT+'/'+n for k,n in dict(map='L_DC_Level1Boss', boss='BP_DC_Boss', data='DA_DC_Boss',
    line='GA_DC_BossLineShot', ground='GA_DC_BossGroundStrike', aset='AS_DC_Boss',
    spawner='BP_DC_BossSpawner', warning='M_DC_BossWarning', impact='M_DC_BossImpact').items()}


def cls(name):
    value = ue.load_class(None, '/Script/DreamCatcher.'+name)
    assert value, 'User C++ build required: '+name
    return value


def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
def world(): return ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
def actors(name): return list(ue.GameplayStatics.get_all_actors_of_class(world(), cls(name)))


def data_asset(path, class_):
    factory = ue.DataAssetFactory(); factory.set_editor_property('DataAssetClass', class_)
    value = ue.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/',1)[1], ROOT, class_, factory)
    assert value
    return value


def material(path, color):
    value = ue.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/',1)[1], ROOT, ue.Material, ue.MaterialFactoryNew())
    assert value
    value.set_editor_property('ShadingModel', ue.MaterialShadingModel.MSM_UNLIT)
    value.set_editor_property('TwoSided', True)
    ue.MaterialEditingLibrary.set_material_usage(value, ue.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    assert value.get_editor_property('bUsedWithInstancedStaticMeshes')
    expression = ue.MaterialEditingLibrary.create_material_expression(value, ue.MaterialExpressionConstant3Vector, -200, 0)
    assert expression
    expression.set_editor_property('Constant', ue.LinearColor(*color, 1))
    assert ue.MaterialEditingLibrary.connect_material_property(expression, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
    errors = ue.MaterialEditingLibrary.recompile_material(value)
    assert not errors, errors
    return value


def build_assets():
    values = {}
    values['warning'] = material(PATHS['warning'], (6, 1.8, .06))
    values['impact'] = material(PATHS['impact'], (8, .15, .02))
    values['data'] = data_asset(PATHS['data'], cls('DCBossData'))
    values['data'].set_editor_property('WarningMaterial', values['warning'])
    values['data'].set_editor_property('ImpactMaterial', values['impact'])
    for key, parent in [('line','DCGameplayAbility_BossLineShot'),('ground','DCGameplayAbility_BossGroundStrike')]:
        bp = blueprint(PATHS[key], cls(parent))
        bp.set_editor_property('bGenerateAbstractClass', False)
        cdo(bp).set_editor_property('DamageEffectClass', generated(load(DAMAGE)))
        assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
        values[key] = bp
    values['aset'] = data_asset(PATHS['aset'], cls('DCAbilitySet'))
    template = load('/Game/DreamCatcher/GAS/Test/R10/Attack/AS_DC_EnemyRangedAttack').get_editor_property('GrantedGameplayAbilities')[0]
    grants = []
    for key in ('line','ground'):
        grant = type(template)()
        assert grant.import_text('(Ability="/Script/Engine.BlueprintGeneratedClass\''+generated(values[key]).get_path_name()+'\'",AbilityLevel=1,InputTag=(TagName=""))')
        grants.append(grant)
    values['aset'].set_editor_property('GrantedGameplayAbilities', grants)
    boss = blueprint(PATHS['boss'], cls('DCBossCharacter'))
    for key,value in dict(BossData=values['data'], DeathAbilityClass=generated(load(DEATH)),
        RangedAttackAbilityClass=generated(values['line']), GroundStrikeAbilityClass=generated(values['ground']),
        AbilitySets=[values['aset']], bAutoRangedAttack=False, TeamId=2).items(): cdo(boss).set_editor_property(key, value)
    source_components = components(load(NPC))
    for component in components(boss):
        assert component.get_path_name().startswith(PATHS['boss']+'.')
        if isinstance(component, ue.SkeletalMeshComponent):
            source = next(x for x in source_components if isinstance(x, ue.SkeletalMeshComponent))
            for name in ('SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation','RelativeScale3D'):
                component.set_editor_property(name, source.get_editor_property(name))
        elif isinstance(component, ue.CapsuleComponent):
            source = next(x for x in source_components if isinstance(x, ue.CapsuleComponent))
            for name in ('CapsuleRadius','CapsuleHalfHeight'): component.set_editor_property(name, source.get_editor_property(name))
    assert ue.BlueprintEditorLibrary.compile_blueprint(boss)
    values['boss'] = boss
    spawner = blueprint(PATHS['spawner'], cls('DCEnemySpawner'))
    for key,value in dict(GASEnemyClass=generated(boss), LyraBotControllerClass=None, EnemyClass=None,
        bCleanupOwnedGASNPCs=True, SpawnCount=1, MaxAliveAtOnce=1, bActivateOnBeginPlay=False).items():
        cdo(spawner).set_editor_property(key, value)
    assert ue.BlueprintEditorLibrary.compile_blueprint(spawner)
    values['spawner'] = spawner
    return values


def verify(values):
    director = actors('DCStageDirector'); assert len(director)==1
    phases = list(director[0].get_editor_property('Phases'))
    assert len(phases)==4 and phases[2].get_editor_property('PhaseType')==ue.DCStagePhaseType.BOSS_ENCOUNTER
    spawners = actors('DCEnemySpawner'); assert len(spawners)==3
    final = phases[2].get_editor_property('Encounter').get_editor_property('Spawners')
    assert len(final)==1 and final[0].get_class()==generated(values['spawner'])
    assert final[0].get_editor_property('bCleanupOwnedGASNPCs')
    assert final[0].get_editor_property('GASEnemyClass')==generated(values['boss'])
    assert final[0].get_editor_property('LyraBotControllerClass') is None
    for phase in phases[:2]:
        entries = phase.get_editor_property('Encounter').get_editor_property('Spawners'); assert len(entries)==1
        assert entries[0].get_editor_property('LyraBotControllerClass') is not None
        assert not entries[0].get_editor_property('bCleanupOwnedGASNPCs')
    expected_mode = generated(load('/Game/DreamCatcher/GAS/Test/R11/BP_DC_StageRunGameMode'))
    assert world().get_world_settings().get_editor_property('DefaultGameMode')==expected_mode
    defaults = cdo(values['boss'])
    assert defaults.get_editor_property('BossData')==values['data']
    assert defaults.get_editor_property('DeathAbilityClass')==generated(load(DEATH))
    assert defaults.get_editor_property('TeamId')==2 and not defaults.get_editor_property('bAutoRangedAttack')
    assert list(defaults.get_editor_property('AbilitySets'))==[values['aset']]
    for key in ('line','ground'):
        assert cdo(values[key]).get_editor_property('DamageEffectClass')==generated(load(DAMAGE))
    assert values['data'].get_editor_property('WarningMaterial')==values['warning']
    assert values['data'].get_editor_property('ImpactMaterial')==values['impact']
    assert values['data'].get_editor_property('PhaseTwoHealthRatio')==.5
    for key in ('warning','impact'):
        assert values[key].get_editor_property('bUsedWithInstancedStaticMeshes')
    return dict(environment=environment(), boss_spawn=final[0].get_actor_location().export_text(),
                phase_titles=[str(p.get_editor_property('PhaseTitle')) for p in phases])


def main():
    cmd = ue.SystemLibrary.get_command_line()
    operation = re.search(r'-DCR12BossSetup=(apply|verify)(?:\s|$)',cmd).group(1)
    run = re.search(r'-DCR12BossRun=([A-Za-z0-9_]+)',cmd).group(1)
    out = P/'Saved/Diagnostics/R12'/('setup_'+run+'.json'); assert not out.exists()
    log = P/'Saved/Logs'/('R12BossSetup-'+run+'.log')
    files = [f for root in ('Source','Config','Content/DreamCatcher/GAS/Test','Content/Blueprint/AI',
        'Content/Blueprint/Stage','Content/DreamCatcher/DreamCatcher_ENV/Map/Main','Content/LyraMigration/AI')
        for f in (P/root).rglob('*') if f.is_file()]
    files += [P/'.git/index',P/'DreamCatcher.uproject',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    protected = {str(f):digest(f) for f in files}
    result = dict(complete=False, operation=operation, saved=[], protected=protected, PIE=False)
    try:
        for name in ('DCBossCharacter','DCBossData','DCBossPhaseComponent','DCBossHealthWidget','DCGameplayAbility_BossLineShot','DCGameplayAbility_BossGroundStrike'): cls(name)
        levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
        if operation=='apply':
            assert not (P/'Content/DreamCatcher/GAS/Test/R12').exists(), 'Fresh R12 destination required; inspect partial output before resuming'
            values = build_assets()
            clean(log)
            for key,value in values.items():
                assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(value,False)
                result['saved'].append(PATHS[key])
            assert levels.load_level(SOURCE)
            source_environment = environment()
            assert levels.new_level_from_template(PATHS['map'], SOURCE)
            result['saved'].append(PATHS['map'])
            director = actors('DCStageDirector'); assert len(director)==1
            phases = list(director[0].get_editor_property('Phases'))
            encounter = phases[2].get_editor_property('Encounter')
            old = list(encounter.get_editor_property('Spawners')); assert len(old)==1
            api = ue.get_editor_subsystem(ue.EditorActorSubsystem)
            point = old[0].get_actor_location()
            # Reuse R11's verified floor/clearance, adjusting only for this NPC capsule.
            old_capsule = cdo(load('/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn')).get_component_by_class(ue.CapsuleComponent)
            new_capsule = cdo(values['boss']).get_component_by_class(ue.CapsuleComponent)
            offset = new_capsule.get_scaled_capsule_half_height()-old_capsule.get_scaled_capsule_half_height()
            point.z += offset
            result['boss_capsule_height_offset'] = offset
            boss_spawner = api.spawn_actor_from_class(generated(values['spawner']),point,old[0].get_actor_rotation())
            assert boss_spawner
            boss_spawner.set_actor_label('R12 Boss Spawner')
            encounter.set_editor_property('Spawners',[boss_spawner])
            assert old[0].get_path_name().startswith(PATHS['map']+'.') and api.destroy_actor(old[0])
            phases[2].set_editor_property('PhaseTitle',ue.Text('보스전'))
            director[0].set_editor_property('Phases',phases)
            assert environment()==source_environment
            result['source_environment']=source_environment
            clean(log)
            assert levels.save_current_level()
        else:
            values={k:load(v) for k,v in PATHS.items() if k!='map'}
            assert levels.load_level(SOURCE); source_environment=environment()
            assert levels.load_level(PATHS['map'])
            assert environment()==source_environment
        result['assembly']=verify(values)
        clean(log); result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc(); raise
    finally:
        result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
        result['output_hashes']={str(f):digest(f) for f in (P/'Content/DreamCatcher/GAS/Test/R12').rglob('*') if f.is_file()}
        out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R12_BOSS_SETUP '+str({k:result[k] for k in ('complete','operation','saved','protected_changed')}))


if __name__=='__main__': main()
