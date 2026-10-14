"""Post-build, bounded R10 finite-spawner map setup. Unreal asset APIs only.

-DCR10FiniteSetup=apply|verify|relocate-experience -DCR10FiniteRun=<fresh receipt>
Requires the user's new C++ build. Existing assets are never saved/overwritten.
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
from r10_enemy_setup import cdo, generated, load
from r9_signature_foundation import blueprint
import r10_lyra_ai_map as previous

ROOT = '/Game/DreamCatcher/GAS/Test/R10/LyraEncounter'
BASE = '/Game/DreamCatcher/GAS/Test/R10/LyraAI'
BOT = '/Game/LyraMigration/AI/ShooterCore/Bot/B_AI_Controller_LyraShooter'
AUTO = '/Game/LyraMigration/AI/ShooterCore/Bot/B_ShooterBotSpawner.B_ShooterBotSpawner_C'
OLD_EXPERIENCE = ROOT+'/B_DC_LyraFiniteExperience'
# The existing AssetManager scan includes LyraAI recursively. Keep Config unchanged.
PATHS = dict(experience=BASE+'/FiniteEncounter/B_DC_LyraFiniteExperience', mode=ROOT+'/BP_DC_LyraFiniteGameMode',
             spawner=ROOT+'/BP_DC_LyraFiniteSpawner', map=ROOT+'/L_DC_LyraFiniteEncounter')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def cls(name):
    value = ue.load_class(None, '/Script/DreamCatcher.'+name)
    assert value, name
    return value


def entries(bp):
    actions = cdo(bp).get_editor_property('Actions')
    assert len(actions) == 1, 'Unexpected Experience shape; inspect before changing'
    return actions[0], list(actions[0].get_editor_property('ComponentList'))


def filtered_source():
    _, rows = entries(load(BASE+'/B_DC_LyraAIExperience'))
    keep = [row for row in rows if AUTO not in row.export_text()]
    assert len(rows) == 6 and len(keep) == 5, 'Exactly one original automatic bot spawner expected'
    return keep


def save(asset, result, log):
    assert asset.get_path_name().split('.')[0] in PATHS.values()
    previous.clean(log)
    assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(asset, False)
    result['saved'].append(asset.get_path_name().split('.')[0])


def inspect(objects, check_registration=False):
    _, rows = entries(objects['experience'])
    assert [x.export_text() for x in rows] == [x.export_text() for x in filtered_source()]
    assert cdo(objects['experience']).get_editor_property('DefaultPawnData') == load(BASE+'/DA_DC_LyraAIPawn')
    assert cdo(objects['mode']).get_editor_property('DefaultExperience').export_text() == 'DCLyraExperienceDefinition:B_DC_LyraFiniteExperience'
    defaults = cdo(objects['spawner'])
    assert defaults.get_editor_property('LyraBotControllerClass') == generated(load(BOT))
    assert defaults.get_editor_property('EnemyClass') is None and defaults.get_editor_property('GASEnemyClass') is None
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    assert world.get_world_settings().get_editor_property('DefaultGameMode') == generated(objects['mode'])
    spawners = sorted(ue.GameplayStatics.get_all_actors_of_class(world, cls('DCEnemySpawner')), key=lambda a:a.get_actor_label())
    encounters = list(ue.GameplayStatics.get_all_actors_of_class(world, cls('DCEncounterController')))
    assert len(spawners) == 2 and len(encounters) == 1
    assert [s.get_editor_property('SpawnCount') for s in spawners] == [2, 1]
    for spawner in spawners:
        assert spawner.get_editor_property('MaxAliveAtOnce') == 1
        assert spawner.get_editor_property('SpawnInterval') == 1.0
        assert not spawner.get_editor_property('bActivateOnBeginPlay')
    assert set(encounters[0].get_editor_property('Spawners')) == set(spawners)
    assert encounters[0].get_editor_property('bAutoStartOnBeginPlay')
    primary_class = None
    if check_registration:
        library = ue.get_default_object(ue.SystemLibrary.static_class())
        primary_class = library.call_method('GetClassFromPrimaryAssetId', args=(cdo(objects['mode']).get_editor_property('DefaultExperience'),))
        assert primary_class == generated(objects['experience']), 'Experience must resolve through AssetManager, not just direct load'
    return dict(component_entries=[r.export_text() for r in rows], total_spawns=3, per_spawner=[2, 1],
                max_alive_per_spawner=1, original_team_policy=True, automatic_respawn=False,
                asset_manager_class=primary_class.get_path_name() if primary_class else None)


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR10FiniteSetup=(apply|verify|relocate-experience)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR10FiniteRun=([A-Za-z0-9_]+)', cmd).group(1)
    out = P/'Saved/Diagnostics/R10'/('finite_setup_'+run+'.json')
    assert not out.exists()
    log = P/'Saved/Logs'/('R10FiniteSetup-'+run+'.log')
    files = [p for root in (P/'Source', P/'Config', P/'Content/DreamCatcher/GAS/Test', P/'Content/LyraMigration/AI')
             for p in root.rglob('*') if p.is_file()]
    files += [P/'.git/index', P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll', P/'DreamCatcher.uproject']
    protected = {str(p):digest(p) for p in files}
    if mode == 'relocate-experience':
        prior = json.loads((P/'Saved/Diagnostics/R10/finite_setup_Apply_A.json').read_text())
        assert prior['complete'] and not prior['protected_changed']
        assert all(digest(Path(p)) == h for p,h in prior['output_hashes'].items()), 'New assembly changed since creation'
        # Only the Experience created by this task may be renamed (Unreal keeps a redirector).
        editable = P/'Content'/Path(OLD_EXPERIENCE[6:]+'.uasset')
        protected.pop(str(editable))
    result = dict(complete=False, mode=mode, created=[], saved=[], protected=protected, PIE=False)
    try:
        # Fail before writing anything if the editor is still using the old DLL.
        assert ue.get_default_object(cls('DCEnemySpawner')).get_editor_property('LyraBotControllerClass') is None
        assert ue.get_default_object(cls('DCEncounterController')).get_remaining_spawner_count() == 0
        assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        if mode == 'apply':
            assert not (P/'Content/DreamCatcher/GAS/Test/R10/LyraEncounter').exists(), 'Fresh destination required'
            assert all(not assets.does_asset_exist(path) for path in PATHS.values())
            keep = filtered_source()
            experience = assets.duplicate_loaded_asset(load(BASE+'/B_DC_LyraAIExperience'), PATHS['experience'])
            assert experience
            action, _ = entries(experience)
            assert action.get_path_name().startswith(PATHS['experience']+'.'), 'Action must belong to new copy'
            action.set_editor_property('ComponentList', keep)
            assert ue.BlueprintEditorLibrary.compile_blueprint(experience)
            mode_bp = blueprint(PATHS['mode'], generated(load(BASE+'/BP_DC_LyraAIGameMode')))
            asset_id = cdo(mode_bp).get_editor_property('DefaultExperience')
            assert asset_id.import_text('DCLyraExperienceDefinition:B_DC_LyraFiniteExperience')
            cdo(mode_bp).set_editor_property('DefaultExperience', asset_id)
            assert ue.BlueprintEditorLibrary.compile_blueprint(mode_bp)
            spawner = blueprint(PATHS['spawner'], cls('DCEnemySpawner'))
            for key, value in dict(LyraBotControllerClass=generated(load(BOT)), EnemyClass=None, GASEnemyClass=None,
                                   SpawnCount=2, MaxAliveAtOnce=1, SpawnInterval=1.0, bActivateOnBeginPlay=False).items():
                cdo(spawner).set_editor_property(key, value)
            assert ue.BlueprintEditorLibrary.compile_blueprint(spawner)
            objects = dict(experience=experience, mode=mode_bp, spawner=spawner)
            for asset in objects.values(): save(asset, result, log)
            # Reuse the completed one-shot floor/cover/starts/pickups/NavMesh construction.
            # Only its destination map changes; it does not edit any existing map.
            previous.PATHS = dict(previous.PATHS, map=PATHS['map'])
            previous.make_map(objects, result, log)
            actor_api = ue.get_editor_subsystem(ue.EditorActorSubsystem)
            spawned = []
            for i, (location, count) in enumerate((((-1000, 700, 100), 2), ((1000, -700, 100), 1))):
                actor = actor_api.spawn_actor_from_class(generated(spawner), ue.Vector(*location))
                assert actor
                actor.set_actor_label('R10 Finite Spawner '+('A' if i == 0 else 'B'))
                actor.set_editor_property('SpawnCount', count)
                spawned.append(actor)
            encounter = actor_api.spawn_actor_from_class(cls('DCEncounterController'), ue.Vector(0, 0, 100))
            assert encounter
            encounter.set_actor_label('R10 Finite Encounter')
            encounter.set_editor_property('Spawners', spawned)
            encounter.set_editor_property('bAutoStartOnBeginPlay', True)
            previous.clean(log)
            assert ue.get_editor_subsystem(ue.LevelEditorSubsystem).save_current_level()
        elif mode == 'relocate-experience':
            assert not assets.does_asset_exist(PATHS['experience']), 'Fresh scan-covered destination required'
            experience = load(OLD_EXPERIENCE)
            assert assets.rename_loaded_asset(experience, PATHS['experience'])
            save(experience, result, log)
            result['relocated_from'] = OLD_EXPERIENCE
            objects = {key:load(path) for key,path in PATHS.items() if key != 'map'}
            assert ue.get_editor_subsystem(ue.LevelEditorSubsystem).load_level(PATHS['map'])
        else:
            objects = {key:load(path) for key,path in PATHS.items() if key != 'map'}
            assert ue.get_editor_subsystem(ue.LevelEditorSubsystem).load_level(PATHS['map'])
        result['assembly'] = inspect(objects, check_registration=mode == 'verify')
        previous.clean(log)
        result['complete'] = True
    except Exception:
        result['failure'] = traceback.format_exc()
        raise
    finally:
        result['protected_changed'] = [p for p,h in protected.items() if digest(Path(p)) != h]
        result['output_hashes'] = {str(p):digest(p) for root in (P/'Content/DreamCatcher/GAS/Test/R10/LyraEncounter', P/'Content/DreamCatcher/GAS/Test/R10/LyraAI/FiniteEncounter') for p in root.rglob('*') if p.is_file()}
        out.write_text(json.dumps(result, indent=2), encoding='utf-8')
        assert not result['protected_changed'], result['protected_changed']
        print('R10_FINITE_SETUP '+json.dumps({key:result[key] for key in ('complete','mode','saved','protected_changed')}))


if __name__ == '__main__': main()
