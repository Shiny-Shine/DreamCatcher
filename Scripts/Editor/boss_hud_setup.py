"""Approved boss HUD data only, after the user's native build.

Separate unattended Python commandlet, -SCCProvider=None:
-DCBossHUDSetupMode=inspect|apply|verify -DCBossHUDSetupRun=<fresh>
-DCBossHUDSetupReference=<completed inspect for apply / apply for verify>
-abslog=<project>/Saved/Diagnostics/BossHUD20261019/<fresh>/Editor.log
Only DA_DC_Boss may be saved. No Blueprint compile, map, source or material save.
"""
import importlib.util
import json
from pathlib import Path
import re
import sys
import traceback

import unreal as ue

sys.dont_write_bytecode = True
PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/BossHUD20261019'
ASSET = '/Game/DreamCatcher/Data/Boss/DA_DC_Boss'
DISK = PROJECT / ('Content/' + ASSET[6:] + '.uasset')
HEALTHBAR = '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_Healthbar'
ART = '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/LyraStarterGame/UI/Hud/Art/'
BRUSHES = {'BarBorderBrush': ('BarBorder', ART + 'MI_UI_HealthBar_BarBorder'),
           'BarFillBrush': ('BarFill', ART + 'MI_UI_HealthBar_Fill')}
VECTORS = {'PanelSize': (720, 86), 'PanelOffset': (0, 88), 'NameOffset': (0, 0),
           'SubtitleOffset': (0, 32), 'BarOffset': (0, 62), 'BarSize': (640, 18)}
NUMBERS = {'NameFontSize': 24, 'SubtitleFontSize': 12}
NAME, SUBTITLE = '춘식이', '나스닥의 수호자'
spec = importlib.util.spec_from_file_location('dc_boss_hud_evidence', PROJECT / 'Scripts/Editor/ingame_hud_layout_setup.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def inventory():
    result = hud.inventory()
    # The shared helper excludes its own allowed HUD asset. This task does not.
    value = hud.DISK.stat()
    result[str(hud.DISK.relative_to(PROJECT))] = [value.st_size, value.st_mtime_ns]
    result.pop(str(DISK.relative_to(PROJECT)), None)
    return result


def hashes():
    result = hud.protected_hashes()
    for path in [hud.DISK, PROJECT / ('Content/' + HEALTHBAR[6:] + '.uasset'),
                 PROJECT / 'Source/DreamCatcher/AI/Boss/DCBossData.cpp'] + [
                 PROJECT / ('Content/' + path[6:] + '.uasset') for _, path in BRUSHES.values()]:
        result[str(path.relative_to(PROJECT))] = hud.sha(path)
    return result


def strip_approved(text):
    # Native data asset fields export on single lines, including nested structs.
    # Refuse multiline additions rather than normalizing unrelated data away.
    for name in ('DisplayName', 'Subtitle', 'HUDStyle'):
        lines = [line for line in text.splitlines() if re.match(r'^\s+' + name + '=', line)]
        assert len(lines) <= 1, (name, len(lines))
        text = re.sub(r'^\s+' + name + r'=[^\r\n]*\n?', '', text, flags=re.M)
    return text


def native():
    cls = ue.load_class(None, '/Script/DreamCatcher.DCBossData')
    widget = ue.load_class(None, '/Script/DreamCatcher.DCBossHealthWidget')
    assert cls and widget, 'BUILD_REQUIRED: approved boss native types are missing'
    try:
        cdo = ue.get_default_object(cls)
        assert str(cdo.get_editor_property('DisplayName')) == NAME
        assert str(cdo.get_editor_property('Subtitle')) == SUBTITLE
        style = cdo.get_editor_property('HUDStyle')
        check_geometry(style)
        assert hasattr(ue.get_default_object(widget), 'set_boss_status')
    except Exception as exc:
        raise AssertionError('BUILD_REQUIRED: approved boss HUD schema/defaults are not loaded') from exc
    return cls


def check_geometry(style):
    for name, expected in VECTORS.items():
        value = style.get_editor_property(name)
        assert (value.x, value.y) == expected, (name, value)
    for name, expected in NUMBERS.items():
        assert style.get_editor_property(name) == expected, name


def source_brushes():
    source = ue.load_asset(HEALTHBAR); assert source, HEALTHBAR
    result = {}
    for name, (widget_name, expected) in BRUSHES.items():
        path = source.get_path_name() + ':WidgetTree.' + widget_name
        images = [obj for obj in ue.ObjectIterator(ue.Image) if obj.get_path_name() == path]
        assert len(images) == 1, (path, len(images))
        brush = images[0].get_editor_property('Brush')
        resource = ue.WidgetLibrary.get_brush_resource(brush)
        assert resource and resource.get_path_name().split('.')[0] == expected
        result[name] = brush
    return result


def configure(data, brushes):
    data.set_editor_property('DisplayName', ue.Text(NAME))
    data.set_editor_property('Subtitle', ue.Text(SUBTITLE))
    style = data.get_editor_property('HUDStyle')
    for name, value in VECTORS.items(): style.set_editor_property(name, ue.Vector2D(*value))
    for name, value in NUMBERS.items(): style.set_editor_property(name, value)
    style.set_editor_property('NameColor', ue.LinearColor(1, 1, 1, 1))
    style.set_editor_property('SubtitleColor', ue.LinearColor(.8, .8, .8, 1))
    for name, brush in brushes.items(): style.set_editor_property(name, brush)
    data.set_editor_property('HUDStyle', style)


def check(data, brushes):
    assert str(data.get_editor_property('DisplayName')) == NAME
    assert str(data.get_editor_property('Subtitle')) == SUBTITLE
    style = data.get_editor_property('HUDStyle'); check_geometry(style)
    for name, expected in brushes.items():
        assert style.get_editor_property(name).export_text() == expected.export_text(), name
    for name, expected in [('NameColor', (1, 1, 1, 1)), ('SubtitleColor', (.8, .8, .8, 1))]:
        value = style.get_editor_property(name)
        assert max(abs(a-b) for a, b in zip((value.r, value.g, value.b, value.a), expected)) < .000001


def main():
    command = ue.SystemLibrary.get_command_line()
    def arg(name, choices):
        matches = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
        assert len(matches) == 1, 'Explicit unique -' + name + ' required'
        return matches[0]
    mode = arg('DCBossHUDSetupMode', 'inspect|apply|verify').lower()
    run = arg('DCBossHUDSetupRun', '[A-Za-z0-9_]{1,64}')
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert all(flag in command.lower() for flag in ('-run=pythonscript', '-unattended', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor(); hud.clean()
    directory = REPORTS / run; log = directory / 'Editor.log'
    matches = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(matches) == 1 and Path(matches[0][0] or matches[0][1]).resolve() == log.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Use a fresh evidence directory'
    before_files, protected = inventory(), hashes()
    report = dict(schema=1, mode=mode, run=run, pid=__import__('os').getpid(), complete=False,
                  saved=[], asset_saves=0, explicit_compile=0, PIE=False, cpp_build=False,
                  asset_before_sha256=hud.sha(DISK), protected_hashes=protected)
    try:
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings_text = hud.export(ue.get_default_object(settings), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        cls = native()
        data = ue.load_asset(ASSET); assert data and data.get_class() == cls
        brushes = source_brushes()
        loaded = hud.export(data, directory / 'loaded_data.t3d')
        report['loaded'] = loaded
        report['source_brushes'] = {key: value.export_text() for key, value in brushes.items()}
        if mode == 'inspect':
            report['baseline'] = loaded
        else:
            reference = arg('DCBossHUDSetupReference', '[A-Za-z0-9_]{1,64}')
            assert reference != run
            previous = json.loads((REPORTS / reference / 'report.json').read_text(encoding='utf-8'))
            assert previous['schema'] == 1 and previous['complete']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['pid'] != report['pid'] and previous['protected_hashes'] == protected
            assert report['asset_before_sha256'] == previous['asset_after_sha256'], 'Boss data changed after reference'
            assert previous['source_brushes'] == report['source_brushes']
            baseline = previous['baseline']; report.update(reference=reference, baseline=baseline)
            assert loaded == previous['loaded' if mode == 'apply' else 'configured'], 'Fresh-process data differs'
            if mode == 'apply': configure(data, brushes)
            check(data, brushes)
            configured = hud.export(data, directory / 'configured_data.t3d')
            assert strip_approved(baseline) == strip_approved(configured), 'Unapproved boss data changed'
            report['configured'] = configured
            report['preserved_non_hud_fields'] = True
            hud.clean((ASSET,)); hud.log_check(log)
            assert inventory() == before_files and hashes() == protected
            assert hud.sha(DISK) == report['asset_before_sha256'], 'Implicit asset save detected'
            if mode == 'apply':
                hud.assert_no_other_editor()
                report['save_intent'] = ASSET; hud.persist(directory / 'report.json', report)
                assert ue.EditorAssetLibrary.save_loaded_asset(data, only_if_is_dirty=False)
                report['saved'] = [ASSET]; report['asset_saves'] = 1
                report.pop('save_intent'); hud.persist(directory / 'report.json', report)
            hud.clean()
        report['asset_after_sha256'] = hud.sha(DISK)
        if mode != 'apply': assert report['asset_after_sha256'] == report['asset_before_sha256']
        hud.log_check(log); report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc(); raise
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_files
        report['protected_hashes_unchanged'] = hashes() == protected
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Boss HUD protection/verification failed'
    print('BOSS_HUD_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'run', 'complete', 'saved', 'asset_saves')}))


if __name__ == '__main__':
    main()
