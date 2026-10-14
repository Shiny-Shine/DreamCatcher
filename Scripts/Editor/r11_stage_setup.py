"""Post-user-build R11 Level 1 copy. No existing map/asset/config is saved.

-DCR11StageSetup=apply|resume|verify -DCR11StageRun=<fresh receipt>
apply/resume: ticking unattended editor with -DCR11StageEditor; import this module
and call main() from a Python file (ExecCmds splits inline argument commas).
verify: Python commandlet. Resume accepts only the recorded template-only partial save.
The placeholder result UI is the native UDCStageResultWidget; no duplicate HUD assets.
"""
import hashlib
import json
import re
import sys
import time
import traceback
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue

P = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(P/'Scripts/Editor'))
from r10_enemy_setup import cdo, generated, load, encode
from r9_signature_foundation import blueprint
from r10_lyra_ai_map import clean

ROOT = '/Game/DreamCatcher/GAS/Test/R11'
SOURCE_MAP = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
MAP = ROOT+'/L_DC_Level1Flow'
MODE = ROOT+'/BP_DC_StageRunGameMode'
AI_ROOT = '/Game/DreamCatcher/GAS/Test/R10/LyraAI'
RETURN_MAP = '/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/L_DC_LyraFiniteEncounter'
SPAWNER = '/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/BP_DC_LyraFiniteSpawner'
EXPERIENCE = AI_ROOT+'/FiniteEncounter/B_DC_LyraFiniteExperience'


def cls(name):
    obj = ue.load_class(None, '/Script/DreamCatcher.'+name)
    assert obj, 'User C++ build required: '+name
    return obj


def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
def world(): return ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
def actors(class_): return list(ue.GameplayStatics.get_all_actors_of_class(world(), class_))


def environment():
    # Record only plain data, never keep source World/Actor Python references across map loading.
    excluded = set(actors(ue.PlayerStart)+actors(cls('DCEnemyCharacter'))+actors(cls('DCStageDirector'))
                   +actors(cls('DCEncounterController'))+actors(cls('DCEnemySpawner'))
                   +actors(ue.NavMeshBoundsVolume)+actors(ue.RecastNavMesh))
    return {a.get_name():dict(cls=a.get_class().get_path_name(), transform=a.get_actor_transform().export_text())
            for a in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors() if a not in excluded}


def trace_ground(point):
    raw = ue.SystemLibrary.line_trace_single(world(), point, point-ue.Vector(0, 0, 2000), ue.TraceTypeQuery.cast(0),
                                            False, [], ue.DrawDebugTrace.NONE, True, ue.LinearColor(), ue.LinearColor(), 0.0)
    if raw is None: return None
    values = raw if isinstance(raw, tuple) else (raw,)
    hit = next((v for v in values if isinstance(v, ue.HitResult)), None)
    assert hit is not None, repr(raw)
    text = hit.export_text()
    if 'bBlockingHit=True' not in text: return None
    def vector(field):
        m = re.search(field+r'=\(X=([-+\d.eE]+),Y=([-+\d.eE]+),Z=([-+\d.eE]+)\)', text)
        assert m, text
        return ue.Vector(*(float(v) for v in m.groups()))
    if vector('ImpactNormal').z < .7: return None
    return vector('ImpactPoint')


def spawn_positions(anchor):
    capsule = cdo(load(AI_ROOT+'/BP_DC_LyraInventoryPawn')).get_component_by_class(ue.CapsuleComponent)
    assert capsule
    half_height, radius = capsule.get_scaled_capsule_half_height(), capsule.get_scaled_capsule_radius()
    print('R11_PLACEMENT_CAPSULE '+str(dict(anchor=encode(anchor), half_height=half_height, radius=radius)))
    # Keep initial enemies away from the player start, on the inspected street axis.
    candidates = [(400,0),(700,0),(1100,0),(-400,0),(-700,0),(0,400),(0,-400),(400,400),(-400,-400),(0,700),(0,-700)]
    result = []
    for dx,dy in candidates:
        ground = trace_ground(anchor+ue.Vector(dx,dy,0))
        if ground is None:
            print('R11_PLACEMENT_NO_FLOOR '+str((dx,dy)))
            continue
        point = ground+ue.Vector(0,0,half_height+5)
        overlaps = ue.SystemLibrary.capsule_overlap_actors(world(), point, radius, half_height,
                         [ue.ObjectTypeQuery.OBJECT_TYPE_QUERY1], None, []) or []
        print('R11_PLACEMENT_CANDIDATE '+str(dict(point=encode(point), overlaps=[a.get_path_name() for a in overlaps])))
        if overlaps: continue
        result.append(point)
        if len(result) == 3: break
    assert len(result) == 3, 'Need three clear floor positions near the original enemy; inspect geometry before placing'
    return result


def add_navigation(points):
    api = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    assert not actors(ue.NavMeshBoundsVolume) and not actors(ue.RecastNavMesh), 'Unexpected source navigation; inspect before editing'
    minimum = ue.Vector(min(p.x for p in points)-1000, min(p.y for p in points)-1000, min(p.z for p in points)-500)
    maximum = ue.Vector(max(p.x for p in points)+1000, max(p.y for p in points)+1000, max(p.z for p in points)+500)
    center, extent = (minimum+maximum)*.5, (maximum-minimum)*.5
    volume = api.spawn_actor_from_class(ue.NavMeshBoundsVolume, center)
    assert volume
    volume.set_actor_label('R11 Navigation Bounds')
    _, initial = volume.get_actor_bounds(False)
    assert min(initial.x,initial.y,initial.z) > 0
    volume.set_actor_scale3d(ue.Vector(extent.x/initial.x,extent.y/initial.y,extent.z/initial.z))
    nav = ue.NavigationSystemV1.get_navigation_system(world())
    assert nav
    nav.on_navigation_bounds_updated(volume)
    data = actors(ue.RecastNavMesh)
    if not data:
        library = ue.get_default_object(ue.GameplayStatics.static_class())
        actor = library.call_method('BeginDeferredActorSpawnFromClass', args=(world(), ue.RecastNavMesh.static_class(), ue.Transform(), ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        assert actor
        data = [library.call_method('FinishSpawningActor', args=(actor, ue.Transform(), ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))]
    for actor in data: actor.set_editor_property('RuntimeGeneration', ue.RuntimeGenerationType.DYNAMIC)
    return dict(center=encode(center), extent=encode(extent), runtime='Dynamic', pathfinding_verified=False)


def inspect(mode_bp):
    assert world().get_world_settings().get_editor_property('DefaultGameMode') == generated(mode_bp)
    defaults = cdo(mode_bp)
    assert defaults.get_editor_property('PlayerControllerClass') == generated(load(AI_ROOT+'/BP_DC_LyraAIPlayerController'))
    assert RETURN_MAP in str(encode(defaults.get_editor_property('ReturnMap')))
    assert defaults.get_editor_property('PlayerTeamId') == 1 and defaults.get_editor_property('EnemyTeamId') == 2
    assert defaults.get_editor_property('ResultWidgetClass') == cls('DCStageResultWidget')
    asset_id = defaults.get_editor_property('DefaultExperience')
    expected_experience = generated(load(EXPERIENCE))
    library = ue.get_default_object(ue.SystemLibrary.static_class())
    assert library.call_method('GetClassFromPrimaryAssetId', args=(asset_id,)) == expected_experience
    directors, encounters, spawners = (actors(cls(n)) for n in ('DCStageDirector','DCEncounterController','DCEnemySpawner'))
    assert len(directors) == 1 and len(encounters) == len(spawners) == 3
    assert not actors(cls('DCEnemyCharacter'))
    assert len(actors(cls('DCLyraPlayerStart'))) == 1
    assert not directors[0].get_editor_property('bAutoStartStage')
    phases = list(directors[0].get_editor_property('Phases'))
    assert [p.get_editor_property('PhaseType') for p in phases] == [ue.DCStagePhaseType.ENCOUNTER, ue.DCStagePhaseType.ENCOUNTER, ue.DCStagePhaseType.BOSS_ENCOUNTER, ue.DCStagePhaseType.COMPLETE]
    assert {p.get_editor_property('Encounter') for p in phases[:3]} == set(encounters)
    assert {e.get_editor_property('Spawners')[0] for e in encounters if len(e.get_editor_property('Spawners')) == 1} == set(spawners)
    for encounter in encounters:
        assert not encounter.get_editor_property('bAutoStartOnBeginPlay')
        assert len(encounter.get_editor_property('Spawners')) == 1
    for spawner in spawners:
        assert spawner.get_editor_property('SpawnCount') == spawner.get_editor_property('MaxAliveAtOnce') == 1
        assert not spawner.get_editor_property('bActivateOnBeginPlay')
    return dict(phases=[p.export_text() for p in phases], environment=environment(),
                spawns=[dict(label=s.get_actor_label(), location=encode(s.get_actor_location())) for s in spawners])


def steps():
    cmd = ue.SystemLibrary.get_command_line()
    operation = re.search(r'-DCR11StageSetup=(apply|resume|verify)(?:\s|$)',cmd).group(1)
    run = re.search(r'-DCR11StageRun=([A-Za-z0-9_]+)',cmd).group(1)
    result_path = P/'Saved/Diagnostics/R11'/('setup_'+run+'.json')
    assert not result_path.exists()
    log = P/'Saved/Logs'/('R11StageSetup-'+run+'.log')
    source_receipt = json.loads((P/'Saved/Diagnostics/R11/inspect_stage_A.json').read_text(encoding='utf-8'))
    files = [p for root in (P/'Source', P/'Config', P/'Content/DreamCatcher/GAS/Test', P/'Content/Blueprint/Stage', P/'Content/Blueprint/AI', P/'Content/DreamCatcher/DreamCatcher_ENV/Map/Main', P/'Content/LyraMigration/AI') for p in root.rglob('*') if p.is_file()]
    files += [P/'.git/index', P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll', P/'DreamCatcher.uproject']
    protected = {str(p):digest(p) for p in files}
    if operation == 'resume':
        # Resume only our known template-only partial save; reject user edits or other assets.
        prior = json.loads((P/'Saved/Diagnostics/R11/setup_Apply_A.json').read_text(encoding='utf-8'))
        current = {str(p):digest(p) for p in (P/'Content/DreamCatcher/GAS/Test/R11').rglob('*') if p.is_file()}
        assert not prior['complete'] and prior['saved'] == [MAP] and not prior['protected_changed']
        assert current == prior['output_hashes'] and len(current) == 1, 'Partial R11 output changed; inspect before resume'
        for path in current: protected.pop(path, None)
    result = dict(complete=False, operation=operation, saved=[], protected=protected, PIE=False)
    try:
        # These new classes cannot load from the previous R10 DLL. Check before any writes.
        cls('DCStageRunGameMode'); cls('DCStageResultWidget')
        levels = ue.get_editor_subsystem(ue.LevelEditorSubsystem)
        if operation in ('apply', 'resume'):
            if operation == 'apply':
                assert not (P/'Content/DreamCatcher/GAS/Test/R11').exists(), 'Fresh R11 destination required'
            source_file = P/'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap'
            assert digest(source_file) == source_receipt['protected'][str(source_file)], 'Designer map changed; inspect the new state first'
            assert levels.load_level(SOURCE_MAP)
            source_environment = environment()
            if operation == 'apply':
                assert levels.new_level_from_template(MAP, SOURCE_MAP)
                result['saved'].append(MAP)
            else:
                assert levels.load_level(MAP)
            # Scene queries need the regular editor to tick after loading the world.
            yield
            assert environment() == source_environment, 'Template copy altered existing environment actor transforms/classes'
            api = ue.get_editor_subsystem(ue.EditorActorSubsystem)
            enemies, starts = actors(cls('DCEnemyCharacter')), actors(ue.PlayerStart)
            assert len(enemies) == len(starts) == 1
            anchor = enemies[0].get_actor_location()
            start_location, start_rotation = starts[0].get_actor_location(), starts[0].get_actor_rotation()
            for actor in enemies+starts:
                assert actor.get_path_name().startswith(MAP+'.')
                assert api.destroy_actor(actor)
            start = api.spawn_actor_from_class(cls('DCLyraPlayerStart'),start_location,start_rotation)
            assert start
            start.set_actor_label('R11 Player Start')
            positions = spawn_positions(anchor)
            result['navigation'] = add_navigation(positions+[start_location])
            stage = api.spawn_actor_from_class(generated(load('/Game/Blueprint/Stage/BP_StageDirector')), start_location)
            assert stage
            stage.set_actor_label('R11 Stage Director')
            stage.set_editor_property('bAutoStartStage',False)
            phases = []
            for index,(point,title) in enumerate(zip(positions,['전투 1','전투 2','보스 (임시)'])):
                spawner = api.spawn_actor_from_class(generated(load(SPAWNER)),point)
                encounter = api.spawn_actor_from_class(generated(load('/Game/Blueprint/Stage/BP_EncounterController')),point)
                assert spawner and encounter
                spawner.set_actor_label('R11 Spawner '+str(index+1))
                spawner.set_editor_property('SpawnCount',1)
                spawner.set_editor_property('MaxAliveAtOnce',1)
                spawner.set_editor_property('bActivateOnBeginPlay',False)
                encounter.set_actor_label('R11 Encounter '+str(index+1))
                encounter.set_editor_property('bAutoStartOnBeginPlay',False)
                encounter.set_editor_property('Spawners',[spawner])
                phase = ue.DCStagePhaseDefinition()
                phase.set_editor_property('PhaseType',ue.DCStagePhaseType.BOSS_ENCOUNTER if index == 2 else ue.DCStagePhaseType.ENCOUNTER)
                phase.set_editor_property('PhaseTitle',ue.Text(title))
                phase.set_editor_property('Encounter',encounter)
                phases.append(phase)
            last = ue.DCStagePhaseDefinition()
            last.set_editor_property('PhaseType',ue.DCStagePhaseType.COMPLETE)
            last.set_editor_property('PhaseTitle',ue.Text('완료'))
            stage.set_editor_property('Phases',phases+[last])
            mode_bp = blueprint(MODE,cls('DCStageRunGameMode'))
            defaults = cdo(mode_bp)
            defaults.set_editor_property('PlayerControllerClass',generated(load(AI_ROOT+'/BP_DC_LyraAIPlayerController')))
            asset_id = defaults.get_editor_property('DefaultExperience')
            assert asset_id.import_text('DCLyraExperienceDefinition:B_DC_LyraFiniteExperience')
            defaults.set_editor_property('DefaultExperience',asset_id)
            defaults.set_editor_property('ReturnMap',load(RETURN_MAP))
            assert ue.BlueprintEditorLibrary.compile_blueprint(mode_bp)
            world().get_world_settings().set_editor_property('DefaultGameMode',generated(mode_bp))
            clean(log)
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(mode_bp,False)
            result['saved'].append(MODE)
            assert environment() == source_environment
            assert levels.save_current_level()
            if MAP not in result['saved']: result['saved'].append(MAP)
            result['source_environment']=source_environment
        else:
            mode_bp=load(MODE)
            assert levels.load_level(MAP)
        result['assembly']=inspect(mode_bp)
        clean(log)
        result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        result['output_hashes']={str(p):digest(p) for p in (P/'Content/DreamCatcher/GAS/Test/R11').rglob('*') if p.is_file()}
        result_path.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R11_STAGE_SETUP '+json.dumps({k:result[k] for k in ('complete','operation','saved','protected_changed')}))


def main():
    cmd = ue.SystemLibrary.get_command_line()
    if '-DCR11StageSetup=verify' in cmd:
        for _ in steps(): pass
        return
    assert '-DCR11StageEditor' in cmd, 'Use a ticking unattended editor for placement queries'
    iterator = steps()
    state = dict(next_at=0, busy=False)
    def tick(delta):
        if state['busy'] or time.monotonic() < state['next_at']: return
        state['busy'] = True
        try:
            next(iterator)
            state['next_at'] = time.monotonic()+1
        except StopIteration:
            ue.unregister_slate_post_tick_callback(state['handle'])
            ue.SystemLibrary.quit_editor()
        except Exception:
            ue.log_error(traceback.format_exc())
            ue.unregister_slate_post_tick_callback(state['handle'])
            ue.SystemLibrary.quit_editor()
        finally:
            state['busy'] = False
    state['handle'] = ue.register_slate_post_tick_callback(tick)


if __name__=='__main__': main()
