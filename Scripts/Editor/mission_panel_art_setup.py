"""Approved mission panel typography/art only; no PIE, build or Blueprint compile.

Only DA_DC_MissionList_Example may be saved. Existing images, fonts, mission
definitions, stage flow, weapon HUD and all other assets remain protected.
Use --commands <prefix> outside Unreal to print separate inspect/apply/verify
commands. A parent-reviewed user_build_success.json is required for execution.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/MissionPanelArt20261023'
SCRIPT = P / 'Scripts/Editor/mission_panel_art_setup.py'
BASELINE = R / 'baseline.json'
FROZEN = R / 'cpp_frozen.json'
BUILD = R / 'user_build_success.json'
DLL = 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'
ENGINE = P.parent.parent / 'UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
ASSET = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example'
RESOURCES = {'panel': '/Game/DreamCatcher/Screens/InGame_UI/Mission_UI',
             'unchecked': '/Game/DreamCatcher/Screens/InGame_UI/Check_False',
             'completed': '/Game/DreamCatcher/Screens/InGame_UI/Check_True',
             'font': '/Game/DreamCatcher/UI/Font/Paperlogy-6SemiBold_Font'}
VALUES = {'PanelWidth': 373.0, 'MinHeight': 205.0, 'MaxHeight': 392.0,
          'Padding': 21.0, 'HeaderHeight': 43.0, 'RowMinHeight': 40.0,
          'CheckboxSize': 20.0, 'CheckboxTextGap': 15.0, 'FontSize': 16,
          'bFontSizeInPixels': True, 'bShowStageTitle': False,
          'bHeaderIncludedInPanel': True}
STYLE_FIELDS = set(VALUES) | {'MissionFont', 'TextColor', 'PanelBrush', 'CheckboxBrush', 'CompletedBrush'}
TOPS = ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def disk(package):
    assert package.startswith('/Game/DreamCatcher/')
    return P / ('Content/' + package[6:] + '.uasset')


def module(name):
    spec = importlib.util.spec_from_file_location('mission_panel_' + name, SCRIPT.parent / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    result = {}
    for mode in ('inspect', 'apply', 'verify'):
        run = prefix + '_' + mode
        folder = R / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-run=pythonscript',
                '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
                '-nullrhi', '-nosound', '-DCMissionArtMode=' + mode, '-DCMissionArtRun=' + run,
                '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode != 'inspect':
            args.append('-DCMissionArtReference=' + prefix + ('_inspect' if mode == 'apply' else '_apply'))
        result[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': ' '.join('"' + value + '"' if ' ' in value else value for value in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False,
                      'cpp_build': False, 'commands': result}, ensure_ascii=False, indent=2))


def argument(command, name, pattern):
    found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
    assert len(found) == 1, 'Use one explicit ' + name
    return found[0]


def relative_inventory():
    values = {}
    for top in TOPS:
        for path in (P / top).rglob('*'):
            if path.is_file():
                info = path.stat()
                values[path.relative_to(P).as_posix()] = [info.st_size, info.st_mtime_ns]
    return values


def build_output(name):
    # Only a reviewed user-build receipt can admit changed generated output.
    return bool(re.fullmatch(r'Binaries/[^:]+|Plugins/(?:[^/]+/)+(?:Binaries|Intermediate/Build)/[^:]+', name))


def protection():
    baseline, frozen, build = read(BASELINE), read(FROZEN), read(BUILD)
    cpp_names = set(baseline['cpp_scope'])
    asset_names = set(baseline['asset_scope'])
    script_names = set(baseline['script_scope'])
    assert asset_names == {disk(ASSET).relative_to(P).as_posix()}
    assert script_names == {SCRIPT.relative_to(P).as_posix()}
    assert set(frozen['files']) == cpp_names and len(cpp_names) == 6
    assert build['user_reported_build_success'] is True
    assert build['cpp_frozen_sha256'] == sha(FROZEN)
    dll_info = (P / DLL).stat()
    assert build['dll'] == {'sha256': sha(P / DLL), 'size': dll_info.st_size, 'mtime_ns': dll_info.st_mtime_ns}
    assert all(sha(P / name) == expected for name, expected in frozen['files'].items()), 'Frozen C++ changed'
    overrides = build.get('baseline_overrides', {})
    assert all(build_output(name) and '\\' not in name and '..' not in Path(name).parts for name in overrides)
    for name, value in overrides.items():
        path = P / name
        if value is None:
            assert not path.exists(), 'Reviewed removed build output reappeared: ' + name
        else:
            info = path.stat()
            assert value == {'sha256': sha(path), 'size': info.st_size, 'mtime_ns': info.st_mtime_ns}, name
    legacy_files = build.get('legacy_excluded_files', {})
    assert set(legacy_files) <= {'Saved/SaveGames/EnhancedInputUserSettings.sav', 'Saved/SaveGames/SharedGameSettings.sav'}
    for name, value in legacy_files.items():
        assert name not in baseline['inventory'] and name not in baseline['files']
        path = P / name; info = path.stat()
        assert value == {'sha256': sha(path), 'size': info.st_size, 'mtime_ns': info.st_mtime_ns}, name
    excluded = cpp_names | asset_names | script_names | {DLL} | set(overrides) | set(legacy_files)
    protected = {}
    for name, expected in baseline['files'].items():
        if name in excluded:
            continue
        assert sha(P / name) == expected, 'Existing user file changed: ' + name
        protected[name] = expected
    current = relative_inventory()
    original = {name.replace('\\', '/'): value for name, value in baseline['inventory'].items()}
    assert {name: value for name, value in current.items() if name not in excluded} == {
        name: value for name, value in original.items() if name not in excluded}, 'Existing file inventory changed outside approved scope/build receipt'
    # This is the post-user-build baseline. No further build writes are admitted
    # between inspect, apply and fresh verify, including the reviewed overrides.
    current.pop(next(iter(asset_names)), None)
    protected.update(frozen['files'])
    protected[DLL] = build['dll']['sha256']
    protected[SCRIPT.relative_to(P).as_posix()] = sha(SCRIPT)
    protected.update({name: value['sha256'] for name, value in overrides.items() if value is not None})
    protected.update({name: value['sha256'] for name, value in legacy_files.items()})
    return {'initial_baseline_sha256': sha(BASELINE), 'cpp_frozen_sha256': sha(FROZEN),
            'build_receipt_sha256': sha(BUILD), 'sha256': protected, 'inventory': current,
            'dll': build['dll']}


def copy_struct(value):
    result = type(value)()
    assert result.import_text(value.export_text())
    assert result.export_text() == value.export_text()
    return result


def snapshot(data, folder, label):
    return {'raw': hud.export(data, folder / (label + '.t3d')),
            'style': data.get_editor_property('HUDStyle').export_text(),
            'missions': [value.export_text() for value in data.get_editor_property('Missions')],
            'tracked': str(data.get_editor_property('DefaultTrackedMissionId'))}


def strip_style(raw):
    result, count = re.subn(r'^   HUDStyle=[^\n]*\n?', '', raw, flags=re.M)
    assert count == 1, 'Expected one root HUDStyle property'
    return result


def validate_preserved(before, after):
    assert strip_style(before['raw']) == strip_style(after['raw']), 'Non-style mission data changed'
    assert before['missions'] == after['missions'] and before['tracked'] == after['tracked']
    assert fields.without_fields(before['style'], STYLE_FIELDS) == fields.without_fields(after['style'], STYLE_FIELDS), 'Other style settings changed'
    old = dict(part.split('=', 1) for part in fields.split_fields(before['style']))
    new = dict(part.split('=', 1) for part in fields.split_fields(after['style']))
    assert fields.without_fields(old['PanelBrush'], {'ResourceObject'}) == fields.without_fields(
        new['PanelBrush'], {'ResourceObject'}), 'Original panel size/nine-slice settings changed'


def brush_image_size(brush):
    properties = dict(part.split('=', 1) for part in fields.split_fields(brush.export_text()))
    size = dict(part.split('=', 1) for part in fields.split_fields(properties['ImageSize']))
    assert set(size) == {'X', 'Y'}, 'Expected exactly two native brush image-size coordinates'
    return [float(size['X']), float(size['Y'])]


def unchecked_brush(texture):
    brush = ue.WidgetLibrary.make_brush_from_texture(texture, 20, 20)
    brush.set_editor_property('DrawAs', ue.SlateBrushDrawType.IMAGE)
    brush.set_editor_property('Tiling', ue.SlateBrushTileType.NO_TILE)
    brush.set_editor_property('Margin', ue.Margin(0))
    brush.set_editor_property('TintColor', ue.SlateColor(specified_color=ue.LinearColor(1, 1, 1, 1)))
    return brush


def apply_style(data, resources):
    style = copy_struct(data.get_editor_property('HUDStyle'))
    for name, value in VALUES.items():
        style.set_editor_property(name, value)
    style.set_editor_property('MissionFont', resources['font'])
    style.set_editor_property('TextColor', ue.LinearColor(1, 1, 1, 1))
    panel = copy_struct(style.get_editor_property('PanelBrush'))
    assert panel.get_editor_property('DrawAs') == ue.SlateBrushDrawType.BOX, 'Preserve the existing panel nine-slice contract'
    assert brush_image_size(panel) == [373, 205], 'Preserve the existing native panel image size'
    panel.set_editor_property('ResourceObject', resources['panel'])
    style.set_editor_property('PanelBrush', panel)
    style.set_editor_property('CheckboxBrush', unchecked_brush(resources['unchecked']))
    style.set_editor_property('CompletedBrush', unchecked_brush(resources['completed']))
    data.set_editor_property('HUDStyle', style)


def validate_style(data, resources):
    style = data.get_editor_property('HUDStyle')
    for name, expected in VALUES.items():
        assert style.get_editor_property(name) == expected, name
    assert style.get_editor_property('MissionFont') == resources['font']
    color = style.get_editor_property('TextColor')
    assert [color.r, color.g, color.b, color.a] == [1, 1, 1, 1]
    for name, role in (('CheckboxBrush', 'unchecked'), ('CompletedBrush', 'completed')):
        brush = style.get_editor_property(name)
        assert brush.get_editor_property('ResourceObject') == resources[role]
        assert brush.get_editor_property('DrawAs') == ue.SlateBrushDrawType.IMAGE
        assert brush.get_editor_property('Tiling') == ue.SlateBrushTileType.NO_TILE
        assert brush_image_size(brush) == [20, 20]
        margin = brush.get_editor_property('Margin')
        assert [margin.left, margin.top, margin.right, margin.bottom] == [0, 0, 0, 0]
        tint = brush.get_editor_property('TintColor').get_editor_property('SpecifiedColor')
        assert [tint.r, tint.g, tint.b, tint.a] == [1, 1, 1, 1]
    panel = style.get_editor_property('PanelBrush')
    assert panel.get_editor_property('ResourceObject') == resources['panel']
    assert panel.get_editor_property('DrawAs') == ue.SlateBrushDrawType.BOX
    assert brush_image_size(panel) == [373, 205]
    # Validate only saved configuration and the agreed local coordinates.
    # This does not stand in for a rendered/PIE comparison to the attachment.
    assert 21 + 43 + (40 - 20) / 2 == 74
    assert 21 + 20 + 15 == 56
    assert 2 * 21 + 43 + 2 * 40 == 165
    return {'panel_minimum': [373, 205], 'checkbox_positions': [[21, 74], [21, 114]],
            'text_x': 56, 'row_pitch': 40, 'design_font_size': 16, 'native_font_points': 12,
            'wrap_text_width': 296, 'height_for_two_rows_before_minimum': 165,
            'dynamic_height_examples': {'two_rows': 205, 'three_rows': 205, 'four_rows': 245},
            'runtime_and_visual_verified': False}


def inspect_resources(folder):
    resources, report = {}, {}
    for role, package in RESOURCES.items():
        value = ue.load_asset(package)
        expected_class = 'Font' if role == 'font' else 'Texture2D'
        assert value and value.get_class().get_name() == expected_class, package
        item = {'path': value.get_path_name(), 'sha256': sha(disk(package)),
                'raw': hud.export(value, folder / ('resource_' + role + '.t3d'))}
        if role != 'font':
            item['native_size'] = [value.blueprint_get_size_x(), value.blueprint_get_size_y()]
            assert all(number > 0 for number in item['native_size'])
        if role == 'panel':
            assert item['native_size'] == [373, 205]
        resources[role], report[role] = value, item
    return resources, report


def main():
    global ue, hud, fields
    import unreal as ue
    hud = module('ingame_hud_layout_setup')
    fields = module('mission_hud_setup')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-nop4', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = argument(command, 'DCMissionArtMode', 'inspect|apply|verify')
    run = argument(command, 'DCMissionArtRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'; assert not output.exists(), 'Use a fresh report run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    protected = protection()
    report = {'mode': mode, 'run': run, 'complete': False, 'PIE': False, 'automatic_PIE': False,
              'cpp_build': False, 'compile_requested': [], 'asset_saves': [], 'scope': [ASSET],
              'script_sha256': sha(SCRIPT), 'protection_before': protected,
              'before_sha256': sha(disk(ASSET)), 'runtime_and_visual_verified': False,
              'limits': 'Saved configuration comparison only. User performs PIE/render/play verification.'}
    try:
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'); assert settings_class
        settings = hud.export(ue.get_default_object(settings_class), folder / 'blueprint_editor_settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        native_class = ue.load_class(None, '/Script/DreamCatcher.DCMissionListData'); assert native_class
        defaults = ue.get_default_object(native_class).get_editor_property('HUDStyle')
        assert defaults.get_editor_property('CheckboxSize') == 24
        assert defaults.get_editor_property('CheckboxTextGap') == 16
        assert defaults.get_editor_property('bShowStageTitle') is True
        assert defaults.get_editor_property('bFontSizeInPixels') is False
        assert defaults.get_editor_property('MissionFont') is None
        report['native_schema_verified'] = True
        previous = None
        if mode != 'inspect':
            reference = argument(command, 'DCMissionArtReference', '[A-Za-z0-9_]{1,64}')
            prior_path = R / reference / 'report.json'; previous = read(prior_path)
            assert previous['complete'] and previous['protection_passed']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == sha(SCRIPT)
            assert previous['protection_before'] == protected, 'Post-build baseline changed since the previous process'
            assert report['before_sha256'] == previous['before_sha256' if mode == 'apply' else 'after_sha256']
            report['reference'], report['reference_sha256'] = reference, sha(prior_path)
        elif disk(ASSET).relative_to(P).as_posix() in read(BASELINE)['files']:
            assert report['before_sha256'] == read(BASELINE)['files'][disk(ASSET).relative_to(P).as_posix()], 'Mission DA changed since approved baseline'
        data = ue.load_asset(ASSET); assert data and data.get_class() == native_class
        resources, report['resources'] = inspect_resources(folder)
        report['before'] = snapshot(data, folder, 'before')
        if mode == 'apply':
            assert report['before'] == previous['before'], 'Mission data changed since inspect'
            apply_style(data, resources)
            report['after'] = snapshot(data, folder, 'after')
            validate_preserved(report['before'], report['after'])
            report['layout_contract'] = validate_style(data, resources)
            hud.clean([ASSET]); hud.log_check(folder / 'Editor.log')
            assert protection() == protected
            hud.assert_no_other_editor()
            assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False)
            report['asset_saves'].append(ASSET)
        elif mode == 'verify':
            assert report['before'] == previous['after'], 'Fresh saved mission data differs'
            validate_preserved(previous['before'], report['before'])
            report['layout_contract'] = validate_style(data, resources)
        hud.clean(); hud.log_check(folder / 'Editor.log')
        assert len(report['asset_saves']) == (1 if mode == 'apply' else 0)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = sha(disk(ASSET))
        try:
            report['protection_passed'] = protection() == protected
            assert report['protection_passed'], 'Protected content/source/config/scripts/Git/build output changed'
            if mode != 'apply' or not report['asset_saves']:
                assert report['before_sha256'] == report['after_sha256']
        except Exception:
            report['complete'], report['protection_passed'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_MISSION_ART_SETUP ' + json.dumps({name: report.get(name) for name in
              ('mode', 'run', 'complete', 'asset_saves', 'error', 'protection_error')}, ensure_ascii=False))
        assert report['complete'], 'Mission art run incomplete; inspect report'


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        commands(sys.argv[2])
    else:
        main()
