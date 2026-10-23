"""After the user's C++ build, apply/verify ONE approved spawner Blueprint.

Separate unattended Python commandlet, -SCCProvider=None, with
-DCSpawnerProximityMode=apply|verify -DCSpawnerProximityRun=<fresh journal id>.
Apply may compile/save only BP_DC_LyraFiniteSpawner. Verify saves no assets.
Level_1 is loaded for inspection only. No build, PIE, map save or Config edit.
Outside Unreal: --commands RUN prints apply/verify commands without running them.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / 'Saved/Diagnostics/SpawnerProximity'
TARGET = '/Game/DreamCatcher/Blueprints/AI/BP_DC_LyraFiniteSpawner'
MAP = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
FIELDS = dict(bRequirePlayerProximity=True, PlayerTriggerRadius=2000.0,
              ProximityCheckInterval=0.1, SpawnCount=1)
TARGET_FILE = PROJECT / ('Content/' + TARGET[6:] + '.uasset')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(exclude_target=False):
    return {path.relative_to(PROJECT).as_posix(): [path.stat().st_size, path.stat().st_mtime_ns]
            for top in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
            for path in (PROJECT / top).rglob('*')
            if path.is_file() and (not exclude_target or path != TARGET_FILE)}


def protected_hashes():
    names = ('.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'AGENTS.md',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
             'Content/' + MAP[6:] + '.umap',
             'Content/DreamCatcher/Experiences/B_DC_StageUIExperience.uasset',
             'Content/DreamCatcher/Blueprints/AI/Boss/BP_DC_BossSpawner.uasset',
             'Content/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle.uasset')
    return {name: sha(PROJECT / name) for name in names}


def editor_support():
    # Reuse the existing no-concurrent-editor guard and Unreal text exporter.
    spec = importlib.util.spec_from_file_location('dc_proximity_export_support',
                                                  Path(__file__).with_name('base_damage_setup.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def values(obj):
    return {name: obj.get_editor_property(name) for name in FIELDS}


def desired(obj):
    actual = values(obj)
    assert actual['bRequirePlayerProximity'] is True and actual['SpawnCount'] == 1, actual
    assert abs(actual['PlayerTriggerRadius'] - 2000.0) < 0.001, actual
    assert abs(actual['ProximityCheckInterval'] - 0.1) < 0.000001, actual
    assert obj.get_editor_property('MaxAliveAtOnce') == 1
    assert obj.get_editor_property('bActivateOnBeginPlay') is False
    return actual


def cdo_signature(text, ignore_fields=()):
    """Compare every exported value; Compile can reorder these two native inners.

    Assets_B's exports show only Root/SpawnPoint declaration/definition order
    changing. Keep each complete block and require exactly those four blocks.
    This is a comparison of diagnostics, never an import or asset rewrite.
    """
    lines = text.splitlines()
    blocks, scalars, names = [], [], []
    index = 0
    while index < len(lines):
        line = lines[index]
        if line.startswith('   Begin Object '):
            name = re.search(r'\bName="(Root|SpawnPoint)"', line)
            assert name, 'Unexpected CDO inner object: ' + line
            names.append(name.group(1))
            end = index + 1
            while end < len(lines) and lines[end] != '   End Object':
                end += 1
            assert end < len(lines), 'Unterminated CDO inner object'
            blocks.append('\n'.join(lines[index:end + 1]))
            index = end + 1
        else:
            if not any(line.strip().startswith(name + '=') for name in ignore_fields):
                scalars.append(line)
            index += 1
    assert sorted(names) == ['Root', 'Root', 'SpawnPoint', 'SpawnPoint'], names
    return sorted(blocks), scalars


def inspect_map(u, require_proximity=False):
    world = u.EditorLoadingAndSavingUtils.load_map(MAP)
    assert world, 'Level_1 did not load'
    cls = lambda name: u.load_class(None, '/Script/DreamCatcher.' + name)
    spawners = {a.get_name(): a for a in u.GameplayStatics.get_all_actors_of_class(world, cls('DCEnemySpawner'))}
    expected = {'BP_DC_LyraFiniteSpawner_C_0', 'BP_DC_LyraFiniteSpawner_C_1', 'BP_DC_BossSpawner_C_0'}
    assert set(spawners) == expected, 'Map placement changed; review before applying: ' + str(sorted(spawners))
    facts = {}
    for name, actor in spawners.items():
        is_boss = name == 'BP_DC_BossSpawner_C_0'
        assert actor.get_editor_property('SpawnCount') == 1
        assert actor.get_editor_property('MaxAliveAtOnce') == 1
        assert actor.get_editor_property('bActivateOnBeginPlay') is False
        if is_boss:
            assert actor.get_editor_property('bRequirePlayerProximity') is False
        elif require_proximity:
            desired(actor)
        point = actor.get_editor_property('SpawnPoint').get_world_location()
        facts[name] = dict(label=actor.get_actor_label(), position=[point.x, point.y, point.z], settings=values(actor))
    encounters = {a.get_name(): a for a in u.GameplayStatics.get_all_actors_of_class(world, cls('DCEncounterController'))}
    assert len(encounters) == 3
    ordered = ['BP_DC_LyraFiniteSpawner_C_0', 'BP_DC_LyraFiniteSpawner_C_1', 'BP_DC_BossSpawner_C_0']
    for index, spawner_name in enumerate(ordered):
        encounter = encounters['BP_EncounterController_C_' + str(index)]
        assert list(encounter.get_editor_property('Spawners')) == [spawners[spawner_name]]
        assert encounter.get_editor_property('bAutoStartOnBeginPlay') is False
        assert encounter.get_editor_property('StartDelay') == 0.0
    directors = list(u.GameplayStatics.get_all_actors_of_class(world, cls('DCStageDirector')))
    assert len(directors) == 1 and directors[0].get_editor_property('bAutoStartStage') is False
    phases = list(directors[0].get_editor_property('Phases'))
    assert len(phases) == 4
    for index in range(3):
        assert phases[index].get_editor_property('Encounter') == encounters['BP_EncounterController_C_' + str(index)]
        expected_type = u.DCStagePhaseType.BOSS_ENCOUNTER if index == 2 else u.DCStagePhaseType.ENCOUNTER
        assert phases[index].get_editor_property('PhaseType') == expected_type
    assert phases[3].get_editor_property('PhaseType') == u.DCStagePhaseType.COMPLETE
    return facts


def main():
    import unreal as u
    command = u.SystemLibrary.get_command_line()
    mode_match = re.search(r'-DCSpawnerProximityMode=(apply|verify)\b', command)
    run_match = re.search(r'-DCSpawnerProximityRun=([A-Za-z0-9_]+)\b', command)
    assert mode_match and run_match, 'Explicit mode and run id required'
    mode, run = mode_match.group(1), run_match.group(1)
    attempt_match = re.search(r'-DCSpawnerProximityAttempt=([A-Za-z0-9_]+)\b', command)
    label = mode + ('_' + attempt_match.group(1) if attempt_match else '')
    assert Path(u.Paths.project_dir()).resolve() == PROJECT.resolve()
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert not u.SourceControl.is_enabled()
    support = editor_support()
    support.assert_no_other_editor()
    assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    assert not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    directory = ROOT / run
    state_path = directory / 'state.json'
    if mode == 'apply':
        assert not directory.exists(), 'Preserve existing run; inspect it before retrying'
        directory.mkdir(parents=True)
    else:
        assert state_path.is_file(), 'Verify requires the apply journal'
    report_path = directory / (label + '_report.json')
    assert not report_path.exists(), 'Do not overwrite previous evidence'
    before_inventory, hashes = inventory(exclude_target=True), protected_hashes()
    target_before = sha(TARGET_FILE)
    report = dict(mode=mode, run=run, complete=False, saved=[], cpp_build=False, PIE=False)
    try:
        settings = u.get_default_object(u.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
        settings_text = support.export(settings, directory / (label + '_settings.t3d'))
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        native = u.get_default_object(u.load_class(None, '/Script/DreamCatcher.DCEnemySpawner'))
        assert native.get_editor_property('bRequirePlayerProximity') is False, 'New user C++ build required'
        assert abs(native.get_editor_property('PlayerTriggerRadius') - 2000.0) < 0.001
        bp = u.load_asset(TARGET)
        assert bp and u.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name() == '/Script/DreamCatcher.DCEnemySpawner'
        registry = u.AssetRegistryHelpers.get_asset_registry()
        registry.wait_for_completion()
        options = u.AssetRegistryDependencyOptions(include_hard_package_references=True, include_soft_package_references=True)
        assert {str(x) for x in registry.get_referencers(TARGET, options)} == {MAP}, 'New BP consumers require review'
        cdo = u.get_default_object(bp.generated_class())
        before = support.export(cdo, directory / (label + '_before.t3d'))
        graphs = support.graph_snapshot(bp, directory, label + '_graph_before')
        if mode == 'apply':
            assert values(cdo)['bRequirePlayerProximity'] is False
            assert cdo.get_editor_property('SpawnCount') == 2, 'BP baseline changed since approval'
            before_map = inspect_map(u)
            assert u.BlueprintEditorLibrary.compile_blueprint(bp) is True
            assert bp.get_editor_property('Status') == u.BlueprintStatus.BS_UP_TO_DATE
            cdo = u.get_default_object(bp.generated_class())
            for name, value in FIELDS.items():
                cdo.set_editor_property(name, value)
            desired(cdo)
            after = support.export(cdo, directory / 'apply_after.t3d')
            assert cdo_signature(before, FIELDS) == cdo_signature(after, FIELDS), 'Unexpected CDO change'
            assert support.graph_snapshot(bp, directory, 'apply_graph_after') == graphs, 'Graph changed'
            support.assert_no_other_editor()
            dirty = {p.get_name() for p in u.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
            assert dirty <= {TARGET}, 'Unexpected dirty dependencies; no broad save permitted: ' + str(dirty)
            assert inventory(exclude_target=True) == before_inventory and protected_hashes() == hashes
            journal = dict(complete=False, target=TARGET, before_sha256=sha(TARGET_FILE), after_cdo=after,
                           graphs=graphs, map_before=before_map, save_intent=True)
            state_path.write_text(json.dumps(journal, ensure_ascii=False, indent=2), encoding='utf-8')
            assert u.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
            report['saved'].append(TARGET)
            journal.update(complete=True, save_intent=False, disk_sha256=sha(TARGET_FILE))
            state_path.write_text(json.dumps(journal, ensure_ascii=False, indent=2), encoding='utf-8')
            report['map_before'] = before_map
            report['fresh_map_inheritance_verified'] = False
        else:
            journal = json.loads(state_path.read_text(encoding='utf-8'))
            assert journal['complete'] and journal['target'] == TARGET and not journal['save_intent']
            assert sha(TARGET_FILE) == journal['disk_sha256']
            desired(cdo)
            assert cdo_signature(before) == cdo_signature(journal['after_cdo']) and graphs == journal['graphs']
            report['map'] = inspect_map(u, require_proximity=True)
            for name, record in report['map'].items():
                assert record['position'] == journal['map_before'][name]['position']
            report['fresh_map_inheritance_verified'] = True
        report['complete'] = True
    except Exception:
        report['failure'] = traceback.format_exc()
        raise
    finally:
        after_inventory = inventory(exclude_target=True)
        report['protected_inventory_changes'] = sorted(name for name, prior in before_inventory.items() if after_inventory.get(name) != prior)
        report['new_protected_files'] = sorted(set(after_inventory) - set(before_inventory))
        report['protected_inventory_unchanged'] = after_inventory == before_inventory
        report['protected_hashes_unchanged'] = protected_hashes() == hashes
        report['verify_target_unchanged'] = mode == 'apply' or sha(TARGET_FILE) == target_before
        report['complete'] = (report['complete'] and report['protected_inventory_unchanged']
                              and report['protected_hashes_unchanged'] and report['verify_target_unchanged'])
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('SPAWNER_PROXIMITY_SETUP ' + json.dumps(report, ensure_ascii=False))
    assert report['complete'], 'Apply/verification failed'


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        run = sys.argv[2]
        assert re.fullmatch(r'[A-Za-z0-9_]+', run)
        assert not (ROOT / run).exists(), 'Print commands with a fresh run id'
        for mode in ('apply', 'verify'):
            print(subprocess.list2cmdline([
                'E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe',
                str(PROJECT / 'DreamCatcher.uproject'), '-run=pythonscript', '-script=' + str(Path(__file__).resolve()),
                '-unattended', '-SCCProvider=None', '-NullRHI', '-NoSound',
                '-DCSpawnerProximityMode=' + mode, '-DCSpawnerProximityRun=' + run,
                '-abslog=' + str(ROOT / (run + '_' + mode + '.log'))]))
    else:
        main()
