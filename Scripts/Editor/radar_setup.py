"""Bounded HUD radar inspect/apply/verify after the user's native build.

UnrealEditor-Cmd -run=pythonscript -unattended -EnablePlugins=UMGToolSet
-SCCProvider=None -DCRadarSetupMode=inspect|apply|verify -DCRadarSetupRun=<fresh>
-DCRadarSetupReference=<inspect run for apply, completed apply run for verify>
-abslog=<project>/Saved/Diagnostics/Radar20261019/<fresh>/Editor.log
Only W_DC_Radar (new) and the existing W_ShooterHUDLayout can be saved.
Existing destinations are refused; a partial saved attempt requires review.
"""
import importlib.util
import json
from pathlib import Path
import re
import traceback

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/Radar20261019'
NEW = '/Game/DreamCatcher/UI/HUD/W_DC_Radar'
NEW_DISK = PROJECT / ('Content/' + NEW[6:] + '.uasset')
ART_ROOT = '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Menu/Art/'
ART = {'PlayerBrush': ART_ROOT + 'MI_UI_Settings_ArrowIcon',
       'ObjectiveBrush': ART_ROOT + 'MI_UI_Settings_DiamondIcon',
       'BackgroundBrush': '/Game/DreamCatcher/UI/Textures/Crosshair/T_Circle_Filled'}
VALUES = dict(RangeMeters=50.0, InnerPadding=4.0, CandidateRefreshInterval=.2,
              PlayerIconSize=24.0, EnemyIconSize=12.0, ObjectiveIconSize=24.0,
              PlayerIconRotationDegrees=90.0)
# Reuse the already reviewed scoped export/process/compile-evidence helpers.
# This file is never modified, and its main() is guarded on import.
spec = importlib.util.spec_from_file_location('dc_hud_layout_evidence', PROJECT / 'Scripts/Editor/ingame_hud_layout_setup.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def inventory():
    values = hud.inventory()
    values.pop(str(NEW_DISK.relative_to(PROJECT)), None)
    return values


def hashes():
    result = hud.protected_hashes()
    for asset in ART.values():
        name = 'Content/' + asset[6:] + '.uasset'
        result[name] = hud.sha(PROJECT / name)
    return result


def near(value, expected):
    return abs(float(value) - expected) < .000001


def native():
    cls = ue.load_class(None, '/Script/DreamCatcher.DCRadarWidget')
    assert cls, 'BUILD_REQUIRED: DCRadarWidget is not registered; no substitute parent is allowed'
    cdo = ue.get_default_object(cls)
    for name, value in VALUES.items():
        try:
            assert near(cdo.get_editor_property(name), value), name
        except Exception as exc:
            raise AssertionError('BUILD_REQUIRED: radar native schema/default differs: ' + name) from exc
    assert not cdo.get_editor_property('bIsFocusable')
    assert not cdo.call_method('IsRadarActive') and not cdo.call_method('GetRadarMarkers')
    assert cdo.call_method('GetObjectiveComponent') is None
    return cls


def configure_defaults(bp):
    cdo = ue.get_default_object(bp.generated_class())
    for name, value in VALUES.items():
        cdo.set_editor_property(name, value)
    cdo.set_editor_property('BackgroundUVMin', ue.Vector2D(.046875, .046875))
    cdo.set_editor_property('BackgroundUVMax', ue.Vector2D(.953125, .953125))
    for name, path in ART.items():
        asset = ue.load_asset(path)
        assert asset, path
        brush = (ue.WidgetLibrary.make_brush_from_texture(asset, 240, 240)
                 if name == 'BackgroundBrush' else ue.WidgetLibrary.make_brush_from_material(asset, 24, 24))
        brush.set_editor_property('TintColor', ue.SlateColor(specified_color=(
            ue.LinearColor(.16, .16, .16, .65) if name == 'BackgroundBrush' else ue.LinearColor(1, 1, 1, 1))))
        cdo.set_editor_property(name, brush)
    # An untextured white image is the FSlateColorBrush equivalent. NativePaint
    # explicitly supports resource-less brushes, and never replaces source art.
    enemy = ue.SlateBrush()
    assert enemy.import_text('(DrawAs=Image,ImageSize=(X=12.000000,Y=12.000000),ResourceObject=None,TintColor=(SpecifiedColor=(R=1.000000,G=1.000000,B=1.000000,A=1.000000),ColorUseRule=UseColor_Specified))')
    cdo.set_editor_property('EnemyBrush', enemy)


def check_new(bp, cls, directory, label):
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == cls
    cdo = ue.get_default_object(bp.generated_class())
    for name, value in VALUES.items():
        assert near(cdo.get_editor_property(name), value), name
    assert not cdo.get_editor_property('bIsFocusable')
    assert cdo.get_editor_property('Visibility') == ue.SlateVisibility.HIT_TEST_INVISIBLE
    for name, expected in [('BackgroundUVMin', .046875), ('BackgroundUVMax', .953125)]:
        uv = cdo.get_editor_property(name)
        assert (uv.x, uv.y) == (expected, expected)
    for name, path in ART.items():
        brush = cdo.get_editor_property(name)
        assert ue.WidgetLibrary.get_brush_resource(brush).get_path_name().split('.')[0] == path
    assert ue.WidgetLibrary.get_brush_resource(cdo.get_editor_property('EnemyBrush')) is None
    full = hud.export(bp, directory / (label + '_radar_blueprint.t3d'))
    assert 'TickPrediction=WillTick' in full, 'Radar must retain native tick'
    return hud.export(cdo, directory / (label + '_radar_cdo.t3d'))


def check_child(umg, bp, radar_class):
    rows, widgets = hud.tree(umg, bp)
    radar = widgets.get('Radar')
    assert radar and radar.get_class() == radar_class and radar.get_parent() == widgets['CanvasPanel_0']
    assert len(rows) == 31, 'Only one child may be added to the existing 30-widget HUD'
    assert radar.get_visibility() == ue.SlateVisibility.HIT_TEST_INVISIBLE
    assert not radar.get_editor_property('bIsFocusable')
    row = next(row for row in rows if str(row.get_editor_property('widget_name')) == 'Radar')
    assert not row.get_editor_property('bIsVariable'), 'Do not add a HUD CDO property'
    slot = radar.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot)
    anchors, align = slot.get_anchors(), slot.get_alignment()
    assert (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y, align.x, align.y) == (0, 0, 0, 0, 0, 0)
    assert (slot.get_position().x, slot.get_position().y, slot.get_size().x, slot.get_size().y) == (64, 48, 240, 240)
    assert not slot.get_auto_size() and slot.get_z_order() == 0
    hud.check_layout(umg, bp, 'after')
    return radar.get_path_name(), slot.get_path_name(), row.export_text()


def strip_addition(text, widget_path, slot_path):
    """Remove only the new subobjects/references/map entry from raw comparison."""
    blocks = hud.object_blocks(text, widget_path) + hud.object_blocks(text, slot_path)
    lines = text.splitlines(keepends=True)
    output, previous = [], 0
    for start, end, _ in sorted(blocks):
        assert start >= previous, 'New subobjects must not overlap'
        output.extend(lines[previous:start]); previous = end
    output.extend(lines[previous:])
    text = ''.join(output)
    slot_name = slot_path.rsplit('.', 1)[1]
    text, count = re.subn(r'^\s*Slots\(\d+\)="/Script/UMG.CanvasPanelSlot\'' + re.escape(slot_name) + r'\'"\n', '', text, flags=re.M)
    assert count <= 1
    # UMGToolSet registers one GUID even for a non-variable child. Preserve all
    # other map entries, their order, authored node/pin GUIDs and numeric data.
    map_pattern = r'\("Radar", [0-9A-F]{32}\)'
    lines = text.splitlines(keepends=True)
    for index, line in enumerate(lines):
        if not line.strip().startswith('WidgetVariableNameToGuidMap='): continue
        map_count = len(re.findall(map_pattern, line)); assert map_count <= 1
        if map_count:
            changed, count = re.subn(',' + map_pattern, '', line)
            if not count: changed, count = re.subn(map_pattern + ',', '', line)
            assert count == 1, 'Expected one Radar entry in the existing GUID map'
            lines[index] = changed
    text = ''.join(lines)
    return text


def compare_existing(before, after, addition, report):
    widget_path, slot_path, row_text = addition
    assert before['parent_class'] == after['parent_class']
    assert after['hierarchy'].count(row_text) == 1
    assert before['hierarchy'] == [row for row in after['hierarchy'] if row != row_text], 'Existing hierarchy changed'
    assert set(after['texts']) - set(before['texts']) == {'widget:' + widget_path, 'slot:' + slot_path}
    evidence = {}
    for key, old in before['texts'].items():
        new = strip_addition(after['texts'][key], widget_path, slot_path)
        if key == 'blueprint':
            old, evidence['before'] = hud.separate_generated_graph(old)
            new, evidence['after'] = hud.separate_generated_graph(new)
            if evidence['before']['present'] and evidence['after']['present']:
                assert evidence['before']['canonical_blocks'] == evidence['after']['canonical_blocks']
        assert old == new, 'Unexpected existing HUD data change: ' + key
    report['existing_hud_comparison'] = dict(equal=True, transient_graph_evidence=evidence,
        only_added_widget=widget_path, only_added_slot=slot_path, only_added_guid_key='Radar')


def main():
    command = ue.SystemLibrary.get_command_line()
    def arg(name, choices):
        matches = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
        assert len(matches) == 1, 'Explicit unique -' + name + ' required'
        return matches[0]
    mode, run = arg('DCRadarSetupMode', 'inspect|apply|verify').lower(), arg('DCRadarSetupRun', '[A-Za-z0-9_]{1,64}')
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert all(flag in command.lower() for flag in ('-run=pythonscript', '-unattended', '-enableplugins=umgtoolset', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor(); hud.clean()
    directory = REPORTS / run; log = directory / 'Editor.log'
    match = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(match) == 1 and Path(match[0][0] or match[0][1]).resolve() == log.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Use a fresh evidence directory'
    before_files, protected = inventory(), hashes()
    report = dict(schema=1, mode=mode, run=run, pid=__import__('os').getpid(), complete=False, saved=[],
                  asset_saves=0, native_build_executed=False, PIE=False, protected_hashes=protected,
                  hud_before_sha256=hud.sha(hud.DISK), new_existed_before=NEW_DISK.exists())
    try:
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings_text = hud.export(ue.get_default_object(settings), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        cls = native()
        umg_cls = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_cls
        umg = ue.get_default_object(umg_cls)
        bp = ue.load_asset(hud.ASSET); assert bp
        hud.check_layout(umg, bp, 'after')
        loaded = hud.snapshot(umg, bp, directory, 'loaded')
        report['loaded'] = loaded
        if mode == 'inspect':
            assert not NEW_DISK.exists() and not ue.EditorAssetLibrary.does_asset_exist(NEW), 'New destination already exists'
            rows, widgets = hud.tree(umg, bp)
            assert len(rows) == 30 and 'Radar' not in widgets
        else:
            reference = arg('DCRadarSetupReference', '[A-Za-z0-9_]{1,64}')
            assert reference != run
            previous = json.loads((REPORTS / reference / 'report.json').read_text(encoding='utf-8'))
            assert previous['schema'] == 1 and previous['complete'] and previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['pid'] != report['pid'] and previous['protected_hashes'] == protected
            assert report['hud_before_sha256'] == previous['hud_after_sha256'], 'HUD changed after reference'
            baseline = previous['loaded'] if mode == 'apply' else previous['baseline']
            report.update(reference=reference, baseline=baseline)
            if mode == 'apply':
                assert not NEW_DISK.exists() and not ue.EditorAssetLibrary.does_asset_exist(NEW), 'Existing/partial destination requires review'
                hud.same(baseline, loaded, 'after', 'after')
                radar_bp = umg.call_method('CreateWidgetBlueprint', args=(NEW.rsplit('/', 1)[0], 'W_DC_Radar', cls)); assert radar_bp
                # Only the newly created Blueprint's factory root can be removed.
                for row in list(umg.call_method('GetWidgets', args=(radar_bp,)).get_editor_property('widgets')):
                    if not row.get_editor_property('parent'):
                        assert umg.call_method('RemoveWidget', args=(radar_bp, row.get_editor_property('widget')))
                        break
                configure_defaults(radar_bp)
                assert umg.call_method('CompileWidgetBlueprint', args=(radar_bp,))
                report['radar_cdo'] = check_new(radar_bp, cls, directory, 'created')
                _, widgets = hud.tree(umg, bp)
                row = umg.call_method('AddWidget', args=(bp, radar_bp.generated_class(), 'Radar', widgets['CanvasPanel_0'], -1))
                radar = row.get_editor_property('widget'); assert radar
                umg.call_method('ToggleWidgetAsVariable', args=(bp, radar, False))
                radar.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
                slot = radar.get_editor_property('slot')
                slot.set_position(ue.Vector2D(64, 48)); slot.set_size(ue.Vector2D(240, 240))
                assert umg.call_method('CompileWidgetBlueprint', args=(bp,))
            else:
                assert NEW_DISK.exists() and hud.sha(NEW_DISK) == previous['radar_sha256']
                radar_bp = ue.load_asset(NEW); assert radar_bp
                assert umg.call_method('CompileWidgetBlueprint', args=(radar_bp,))
                report['radar_cdo'] = check_new(radar_bp, cls, directory, 'reloaded')
                assert report['radar_cdo'] == previous['radar_cdo'], 'Radar CDO changed after save/reload'
                assert umg.call_method('CompileWidgetBlueprint', args=(bp,))
            addition = check_child(umg, bp, radar_bp.generated_class())
            compiled = hud.snapshot(umg, bp, directory, 'compiled')
            report['compiled'] = compiled
            compare_existing(baseline, compiled, addition, report)
            hud.clean((hud.ASSET, NEW)); hud.log_check(log)
            assert inventory() == before_files and hashes() == protected
            assert hud.sha(hud.DISK) == report['hud_before_sha256'], 'Implicit save is not allowed'
            if mode == 'apply':
                hud.assert_no_other_editor()
                # All comparisons finish before either approved explicit save.
                for target, obj in [(NEW, radar_bp), (hud.ASSET, bp)]:
                    report['save_intent'] = target; hud.persist(directory / 'report.json', report)
                    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), target
                    report['saved'].append(target); report['asset_saves'] += 1
                    if target == NEW: report['radar_sha256'] = hud.sha(NEW_DISK)
                    hud.persist(directory / 'report.json', report)
                report.pop('save_intent', None); hud.clean()
            report['radar_sha256'] = hud.sha(NEW_DISK)
        report['hud_after_sha256'] = hud.sha(hud.DISK)
        if mode != 'apply': assert report['hud_after_sha256'] == report['hud_before_sha256']
        hud.log_check(log)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc(); raise
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_files
        report['protected_hashes_unchanged'] = hashes() == protected
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Protection/verification failed'
    print('RADAR_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'run', 'complete', 'saved', 'asset_saves')}))


if __name__ == '__main__':
    main()
