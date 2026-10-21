"""Approved Screens texture settings only; no PIE, build, import or resize.

--commands <prefix> prints separate apply/fresh-verify launches, never runs UE.
The fixed 42 assets get the UI LOD group; only Shield_Bar and Button_Again
also get uncompressed RGBA8 and sRGB. Native source PNG exports are diagnostic
files: the parent must compare decoded RGBA pixels, including alpha, across
before/after/fresh exports before declaring the resource step fully verified.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/UITextureSetup20261021'
SCRIPT = PROJECT / 'Scripts/Editor/ui_screen_texture_setup.py'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
MANIFEST = PROJECT / 'Saved/Diagnostics/UIScreenReview20261021_C/settings.json'
MANIFEST_SHA = '0b0afb42b986ac07a8d91ec92908eacf97d20719037f1a869101315815d87403'
NORMALS = {'/Game/DreamCatcher/Screens/InGame_UI/Shield_Bar',
           '/Game/DreamCatcher/Screens/Pause/Button_Again'}


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def asset_file(package):
    assert package.startswith('/Game/DreamCatcher/')
    return PROJECT / ('Content/' + package[6:] + '.uasset')


def local_module(name):
    spec = importlib.util.spec_from_file_location('ui_texture_' + name, SCRIPT.parent / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def manifest():
    assert sha(MANIFEST) == MANIFEST_SHA, 'Approved manifest changed'
    data = read(MANIFEST)
    assert data['count'] == len(data['rows']) == 42 and data['source_assets_unchanged']
    rows = {row['path'].split('.')[0]: row for row in data['rows']}
    assert len(rows) == 42 and NORMALS <= rows.keys()
    assert all(package.startswith('/Game/DreamCatcher/Screens/') for package in rows)
    files = {asset_file(package) for package in rows}
    actual = {path for path in (PROJECT / 'Content/DreamCatcher/Screens').rglob('*') if path.is_file()}
    assert actual == files, 'Screens file set changed; no extra/new assets are permitted'
    expected_dirs = {parent for path in files for parent in path.parents
                     if parent.is_relative_to(PROJECT / 'Content/DreamCatcher/Screens')}
    actual_dirs = {PROJECT / 'Content/DreamCatcher/Screens'} | {
        path for path in (PROJECT / 'Content/DreamCatcher/Screens').rglob('*') if path.is_dir()}
    assert actual_dirs == expected_dirs, 'Screens folder structure changed'
    return rows


def protection(rows):
    allowed = {asset_file(package) for package in rows}
    inventory = {}
    for directory in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for path in (PROJECT / directory).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat()
                inventory[path.relative_to(PROJECT).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    old = PROJECT / 'Saved/Diagnostics/Pistol20261020'
    pistol = read(old / 'Port_U_verify/report.json')
    assert pistol['complete'] and len(pistol['final_sha256']) == 197
    frozen = read(old / 'frozen_cpp_config.json')['files']
    assert all(sha(PROJECT / name) == value for name, value in frozen.items())
    paths = [asset_file(package) for package in pistol['final_sha256']]
    paths += [PROJECT / name for name in frozen]
    paths += [PROJECT / name for name in ('.git/HEAD', '.git/index', 'DreamCatcher.uproject',
              'Binaries/Win64/UnrealEditor-DreamCatcher.dll')]
    paths += [MANIFEST, old / 'frozen_cpp_config.json', old / 'Port_U_verify/report.json']
    return {'inventory': inventory, 'sha256': {path.as_posix(): sha(path) for path in paths}}


def print_commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    manifest()
    commands = {}
    for mode in ('apply', 'verify'):
        run = prefix + '_' + mode
        directory = REPORTS / run
        args = [ENGINE.as_posix(), (PROJECT / 'DreamCatcher.uproject').as_posix(),
                '-run=pythonscript', '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4',
                '-SCCProvider=None', '-nullrhi', '-nosound', '-DCUITextureMode=' + mode,
                '-DCUITextureRun=' + run, '-abslog=' + (directory / 'Editor.log').as_posix()]
        if mode == 'verify':
            args.append('-DCUITextureReference=' + prefix + '_apply')
        commands[mode] = {'args': args, 'create_log_directory_first': directory.as_posix(),
                         'windows_command_line': ' '.join('"' + arg + '"' if ' ' in arg else arg for arg in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False, 'commands': commands}, indent=2))


def settings(texture, original):
    result = {}
    for name in original['settings']:
        value = texture.get_editor_property(name)
        result[name] = value.export_text() if hasattr(value, 'export_text') else (
            str(value) if isinstance(original['settings'][name], str) else value)
    return result


def desired(original, package):
    result = dict(original['settings'])
    result['LODGroup'] = str(ue.TextureGroup.TEXTUREGROUP_UI)
    if package in NORMALS:
        result['CompressionSettings'] = str(ue.TextureCompressionSettings.TC_EDITOR_ICON)
        result['SRGB'] = True
    return result


def capture(texture, original, package, directory, label, index):
    assert texture.get_class().get_name() == 'Texture2D' and texture.get_path_name() == original['path']
    width, height = texture.blueprint_get_size_x(), texture.blueprint_get_size_y()
    assert (width, height) == (original['width'], original['height']), 'Texture dimensions changed'
    stem = label + '_' + str(index).zfill(2)
    raw = hud.export(texture, directory / (stem + '.t3d'))
    fields = {'LODGroup', 'LightingGuid'}
    if package in NORMALS:
        fields |= {'CompressionSettings', 'SRGB'}
    other = re.sub(r'^   (?:' + '|'.join(sorted(fields)) + r')=[^\n]*\n?', '', raw, flags=re.M)
    png = directory / (stem + '.png')
    assert not png.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter, task.filename = texture, ue.TextureExporterPNG(), png.as_posix()
    task.automated, task.prompt, task.replace_identical = True, False, True
    assert ue.Exporter.run_asset_export_task(task) and png.is_file(), 'Source PNG export failed'
    return {'path': texture.get_path_name(), 'width': width, 'height': height,
            'settings': settings(texture, original), 'other_properties': pistol.item_subobject_order(other),
            'source_identity_export': [line for line in raw.splitlines() if line.startswith('   Source=')],
            'lighting_guid_export': [line for line in raw.splitlines() if line.startswith('   LightingGuid=')],
            'source_png': png.as_posix(), 'png_file_sha256': sha(png)}


def same_preserved(before, after):
    for name in ('path', 'width', 'height', 'other_properties', 'source_identity_export'):
        assert before[name] == after[name], 'Unapproved texture change: ' + name + ' ' + before['path']


def run_editor():
    global ue, hud, pistol
    import unreal as ue
    hud, pistol = local_module('ingame_hud_layout_setup'), local_module('pistol_slot_setup')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    hud.assert_no_other_editor()
    mode = re.search(r'-DCUITextureMode=(apply|verify)\b', command).group(1)
    run = re.search(r'-DCUITextureRun=([A-Za-z0-9_]{1,64})\b', command).group(1)
    directory = REPORTS / run
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'report.json'
    assert not output.exists(), 'Use a fresh report run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (directory / 'Editor.log').resolve()
    rows = manifest()
    approved_hashes = {(PROJECT / name.replace('\\', '/')).resolve(): value
                       for name, value in read(MANIFEST)['source_sha256'].items()}
    assert set(approved_hashes) == {asset_file(package).resolve() for package in rows}
    before = protection(rows)
    report = {'mode': mode, 'run': run, 'complete': False, 'PIE': False, 'automatic_PIE': False,
              'manifest_sha256': MANIFEST_SHA, 'script_sha256': sha(SCRIPT), 'asset_saves': [],
              'compile_requested': [], 'protection_before': before, 'before': {}, 'after': {},
              'before_sha256': {p: sha(asset_file(p)) for p in rows},
              'pixel_verification': {'required_external_decoded_RGBA_comparison': True, 'verified': False,
                  'contract': 'Parent compares every before/after/fresh source PNG width, height and decoded RGBA bytes including alpha; PNG file hashes alone are not pixel proof.'},
              'derived_metadata': {'LightingGuid': 'Engine Texture.cpp:822 calls SetLightingGuid on every PostEditChangeProperty; recorded separately, never restored.'}}
    try:
        hud.clean()
        listed = set(ue.EditorAssetLibrary.list_assets('/Game/DreamCatcher/Screens', recursive=True, include_folder=False))
        assert listed == {row['path'] for row in rows.values()}, 'Asset registry differs from fixed42 manifest'
        previous = None
        if mode == 'verify':
            ref = re.search(r'-DCUITextureReference=([A-Za-z0-9_]{1,64})\b', command).group(1)
            path = REPORTS / ref / 'report.json'
            previous = read(path)
            assert previous['mode'] == 'apply' and previous['complete'] and previous['protection_passed']
            assert previous['manifest_sha256'] == MANIFEST_SHA and previous['script_sha256'] == sha(SCRIPT)
            assert previous['protection_before'] == before
            assert previous['after_sha256'] == report['before_sha256']
            report['reference'], report['reference_sha256'] = ref, sha(path)
        changed = []
        for index, (package, original) in enumerate(sorted(rows.items())):
            texture = ue.load_asset(package)
            assert texture
            initial = capture(texture, original, package, directory, 'before' if mode == 'apply' else 'verify', index)
            report['before'][package] = initial
            expected = desired(original, package)
            if mode == 'apply':
                assert initial['settings'] in (original['settings'], expected), 'Unexpected initial settings: ' + package
                if initial['settings'] != expected:
                    assert report['before_sha256'][package] == approved_hashes[asset_file(package).resolve()], 'Original asset differs from approved snapshot: ' + package
                    texture.set_editor_property('LODGroup', ue.TextureGroup.TEXTUREGROUP_UI)
                    if package in NORMALS:
                        texture.set_editor_property('CompressionSettings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
                        texture.set_editor_property('SRGB', True)
                    changed.append((package, texture))
                final = capture(texture, original, package, directory, 'after', index)
                same_preserved(initial, final)
            else:
                final = initial
                same_preserved(previous['after'][package], final)
            assert final['settings'] == expected, 'Final texture settings differ: ' + package
            report['after'][package] = final
        hud.clean([package for package, _ in changed])
        hud.log_check(directory / 'Editor.log')
        assert protection(rows) == before
        for package, texture in changed:
            assert package in rows
            hud.assert_no_other_editor()
            assert ue.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False), package
            report['asset_saves'].append(package)
        report['skipped_compliant_assets'] = sorted(set(rows) - set(report['asset_saves']))
        hud.clean()
        hud.log_check(directory / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = {p: sha(asset_file(p)) for p in rows}
        try:
            manifest()
            report['protection_passed'] = protection(rows) == before
            assert report['protection_passed'], 'Protected inventory/hash changed'
            for package in rows:
                if package not in report['asset_saves']:
                    assert report['after_sha256'][package] == report['before_sha256'][package], 'Unsaved texture bytes changed'
        except Exception:
            report['protection_passed'], report['complete'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_UI_TEXTURE_SETUP ' + json.dumps({key: report.get(key) for key in
              ('mode', 'run', 'complete', 'protection_passed', 'asset_saves', 'error', 'protection_error')}))
        assert report['complete'], 'UI texture native phase incomplete; inspect report'


if __name__ == '__main__':
    if '--commands' in sys.argv:
        print_commands(sys.argv[sys.argv.index('--commands') + 1])
    else:
        run_editor()
