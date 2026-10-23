"""Approved Kill Z edits for Level_2/3, after the user's native build.

Run in a separate unattended Editor with -ExecutePythonScript=<this file>,
-DCFallDeathMode=apply|verify, -DCFallDeathRun=<fresh id>, and (verify only)
-DCFallDeathReference=<successful apply id>. No PIE, C++ build or Config writes.
--commands <id> prints the apply/verify commands without importing Unreal.
Only the two maps' WorldSettings.KillZ may change; all UE saves use UE APIs.
"""
import gzip
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
REPORTS = PROJECT / 'Saved/Diagnostics/FallDeath20261024'
TARGETS = {
    '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_2': -1500.0,
    '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_3': -6000.0,
}
FROZEN_SOURCE = {
    'Source/DreamCatcher/DreamCatcherCharacter.h': '2d44a1b2f8651ac641b67aac08ba512b74f17f69ef943e0078f710ecb0dc741d',
    'Source/DreamCatcher/DreamCatcherCharacter.cpp': 'd38732f682c7d1c72b27bed653a680d145fea9413a8c961c9b2cdc2e56632c04',
}
DLL = PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'
# Observed concurrent frontend work during Fall_A_apply. Its text edits do not
# affect the loaded DLL; retain/report them, never restore or overwrite them.
EXTERNAL_SOURCE_EDITS = {'Source/DreamCatcher/UI/Frontend/DCFrontendIntroScreen.cpp'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def disk(package):
    assert package in TARGETS
    return PROJECT / 'Content' / (package[6:] + '.umap')


def evidence_module():
    # Reuse the established actor/component/brush preservation checks.
    # LevelExporterT3D is intentionally not used: it can detach child actors.
    path = PROJECT / 'Scripts/Editor/mission_hud_setup.py'
    spec = importlib.util.spec_from_file_location('dc_fall_map_evidence', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def inventory():
    allowed = {disk(package) for package in TARGETS}
    return {path.relative_to(PROJECT).as_posix(): [path.stat().st_size, path.stat().st_mtime_ns]
            for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames')
            for path in (PROJECT / top).rglob('*') if path.is_file() and path not in allowed
            and path.relative_to(PROJECT).as_posix() not in EXTERNAL_SOURCE_EDITS}


def protected_hashes():
    names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
             'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap']
    paths = [PROJECT / name for name in names] + [DLL]
    paths += [p for top in ('Source', 'Config') for p in (PROJECT / top).rglob('*') if p.is_file()]
    return {path.relative_to(PROJECT).as_posix(): sha(path) for path in paths
            if path.relative_to(PROJECT).as_posix() not in EXTERNAL_SOURCE_EDITS}


def external_source_hashes():
    # Edits are allowed only to this already observed file, not deletion.
    return {name: sha(PROJECT / name) for name in EXTERNAL_SOURCE_EDITS}


def settings_neutral(snapshot):
    # Strip only this exact WorldSettings scalar, preserving every other field.
    result = dict(snapshot)
    # The shared helper's original mission-only exemption must not apply here.
    result['director'] = None
    result['actors'] = dict(snapshot['actors'])
    key = snapshot['settings']
    row = dict(result['actors'][key])
    row['text'], removed = re.subn(r'^   KillZ=[^\r\n]*(?:\r?\n|$)', '', row['text'], flags=re.M)
    assert removed <= 1, 'Unexpected WorldSettings export layout'
    result['actors'][key] = row
    return result


def write_snapshot(path, snapshot):
    assert not path.exists()
    with gzip.open(path, 'wt', encoding='utf-8') as handle:
        json.dump(snapshot, handle, ensure_ascii=False)


def read_snapshot(path):
    with gzip.open(path, 'rt', encoding='utf-8') as handle:
        return json.load(handle)


def run():
    global ue
    import unreal as ue

    command = ue.SystemLibrary.get_command_line()
    mode_match = re.search(r'-DCFallDeathMode=(apply|verify)\b', command)
    run_match = re.search(r'-DCFallDeathRun=([A-Za-z0-9_]+)\b', command)
    reference_match = re.search(r'-DCFallDeathReference=([A-Za-z0-9_]+)\b', command)
    assert '-unattended' in command.lower() and mode_match and run_match
    assert '-run=pythonscript' not in command.lower(), 'Use full Editor context for comparable component exports'
    assert '-noautosave' in command.lower() and '-nosaveconfig' in command.lower()
    assert not ue.SourceControl.is_enabled(), 'Use -SCCProvider=None'
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT
    assert not ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world(), 'Never run during PIE'
    mode = mode_match.group(1)
    directory = REPORTS / run_match.group(1)
    directory.mkdir(parents=True, exist_ok=True)
    report_path = directory / 'report.json'
    assert not report_path.exists(), 'Preserve existing/partial run; inspect it before retrying'
    evidence = evidence_module()
    evidence.hud.assert_no_other_editor()
    report = dict(mode=mode, complete=False, saved=[], saved_hashes={}, map_checks={}, snapshots={},
                  asset_writes_allowed=list(TARGETS), PIE=False, cpp_build=False)

    def persist():
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')

    reference = None
    if mode == 'verify':
        assert reference_match, 'Verify requires the successful apply journal'
    if reference_match:
        reference = json.loads((REPORTS / reference_match.group(1) / 'report.json').read_text(encoding='utf-8'))
        assert reference['mode'] == 'apply' and not reference.get('pending_save')
        if mode == 'verify':
            assert reference['complete']
        else:
            assert not reference['complete'] and 'External protected changes; review before saving' in reference.get('error', '')
            assert reference['saved'] == [next(iter(TARGETS))], 'Only the inspected partial Level_2 save may resume'
            report['resumed_from'] = reference_match.group(1)
    baseline_inventory, baseline_hashes = inventory(), protected_hashes()
    baseline_maps = {package: sha(disk(package)) for package in TARGETS}
    report['before_map_hashes'] = baseline_maps
    report['source_hashes'] = {name: sha(PROJECT / name) for name in FROZEN_SOURCE}
    report['dll_hash'] = sha(DLL)
    report['external_source_before'] = external_source_hashes()
    manifest_path = directory / 'protected_baseline.json.gz'
    write_snapshot(manifest_path, dict(inventory=baseline_inventory, hashes=baseline_hashes))
    report['protected_baseline'] = str(manifest_path)
    persist()
    try:
        assert report['source_hashes'] == FROZEN_SOURCE, 'Reviewed C++ changed; review before applying'
        assert DLL.stat().st_mtime_ns >= max((PROJECT / name).stat().st_mtime_ns for name in FROZEN_SOURCE), 'User C++ build is pending'
        if reference:
            assert reference['dll_hash'] == report['dll_hash'], 'DLL changed since apply'
            known_maps = reference['saved_hashes'] if mode == 'verify' else reference['observed_map_hashes']
            assert known_maps == baseline_maps, 'Maps changed since the reviewed apply outcome'
        evidence.clean()
        settings = ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
        settings_text = evidence.hud.export(settings, directory / 'compile_settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        for index, (package, target) in enumerate(TARGETS.items()):
            evidence.clean()
            world = ue.EditorLoadingAndSavingUtils.load_map(package)
            assert world and world.get_path_name() == package + '.' + package.rsplit('/', 1)[-1]
            world_settings = world.get_world_settings()
            before_z = float(world_settings.get_editor_property('KillZ'))
            assert world_settings.get_editor_property('bEnableWorldBoundsChecks'), 'Review disabled world bounds checks'
            assert before_z in (-1048575.0, target), 'Unreviewed Kill Z value: ' + str(before_z)
            actors = list(ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors())
            for actor in actors:
                class_name = actor.get_class().get_name()
                if 'Spawner' in class_name or 'PlayerStart' in class_name:
                    assert actor.get_actor_location().z > target, 'Start/spawner below proposed Kill Z: ' + actor.get_name()
                    for component in actor.get_components_by_class(ue.ArrowComponent):
                        if component.get_name() == 'SpawnPoint':
                            assert component.get_world_location().z > target, 'SpawnPoint below proposed Kill Z: ' + actor.get_name()
            proof = {}
            before = evidence.map_snapshot(world, actors, world_settings, directory, 'level%d_before' % (index + 2), proof)
            evidence.clean()
            if mode == 'apply':
                if reference and package in reference['snapshots']:
                    assert before_z == target and sha(disk(package)) == reference['saved_hashes'][package]
                    previous = read_snapshot(Path(reference['snapshots'][package]))
                    evidence.compare_maps(dict(previous, director=None), dict(before, director=None), cross_process=True)
                if before_z != target:
                    world_settings.set_editor_property('KillZ', target, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
                    after = evidence.map_snapshot(world, actors, world_settings, directory, 'level%d_after' % (index + 2), proof)
                else:
                    after = before
                evidence.compare_maps(settings_neutral(before), settings_neutral(after))
                assert float(world_settings.get_editor_property('KillZ')) == target
                assert world_settings.get_editor_property('bEnableWorldBoundsChecks')
                evidence.clean(maps=(package,))
                evidence.hud.log_check(directory / 'Editor.log')
                evidence.hud.assert_no_other_editor()
                assert sha(disk(package)) == baseline_maps[package], 'Map changed on disk while being inspected'
                assert inventory() == baseline_inventory and protected_hashes() == baseline_hashes, 'External protected changes; review before saving'
                if before_z != target:
                    report['pending_save'] = package; persist()
                    assert ue.EditorLoadingAndSavingUtils.save_map(world, package), 'Map save failed'
                    report['saved'].append(package)
                    report.pop('pending_save')
                report['saved_hashes'][package] = sha(disk(package))
                snapshot_path = directory / ('level%d_applied.json.gz' % (index + 2))
                write_snapshot(snapshot_path, after)
                report['snapshots'][package] = str(snapshot_path)
                persist()
            else:
                assert before_z == target
                applied = read_snapshot(Path(reference['snapshots'][package]))
                evidence.compare_maps(dict(applied, director=None), dict(before, director=None), cross_process=True)
                assert sha(disk(package)) == baseline_maps[package]
            report['map_checks'][package] = dict(before_kill_z=before_z, kill_z=target, world_bounds_checks=True,
                                                actor_count=len(actors), other_map_data_preserved=True)
            evidence.clean(); persist()
        evidence.hud.log_check(directory / 'Editor.log')
        assert inventory() == baseline_inventory and protected_hashes() == baseline_hashes, 'Protected file changes detected'
        report['after_map_hashes'] = {package: sha(disk(package)) for package in TARGETS}
        if mode == 'verify':
            assert report['after_map_hashes'] == baseline_maps and not report['saved']
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        report['observed_map_hashes'] = {package: sha(disk(package)) for package in TARGETS}
        current_inventory = inventory()
        report['protected_changes'] = [name for name in sorted(set(baseline_inventory) | set(current_inventory))
                                       if baseline_inventory.get(name) != current_inventory.get(name)]
        ue.log_error(report['error'])
    finally:
        report['external_source_after'] = external_source_hashes()
        persist()
        ue.log('DC_FALL_DEATH_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'complete', 'saved', 'map_checks')}))
        ue.SystemLibrary.quit_editor()


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        run_id = sys.argv[2]
        assert re.fullmatch(r'[A-Za-z0-9_]+', run_id)
        for mode in ('apply', 'verify'):
            args = ['E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe', str(PROJECT / 'DreamCatcher.uproject'),
                    '-ExecutePythonScript=' + str(Path(__file__).resolve()), '-unattended', '-nop4', '-NoAutoSave',
                    '-NoSaveConfig', '-SCCProvider=None', '-NullRHI', '-NoSound',
                    '-DCFallDeathMode=' + mode, '-DCFallDeathRun=' + run_id + '_' + mode,
                    '-abslog=' + str(REPORTS / (run_id + '_' + mode) / 'Editor.log')]
            if mode == 'verify': args.append('-DCFallDeathReference=' + run_id + '_apply')
            print(subprocess.list2cmdline(args))
    else:
        run()
