"""Approved direct 352x120 weapon HUD resize; no ScaleBox substitute.

Separate inspect/apply/verify Unreal Python commandlets; no C++ build or PIE.
Required: -unattended -run=pythonscript -SCCProvider=None -EnablePlugins=UMGToolSet
-DCPanelSizeMode=<mode> -DCPanelSizeRun=<fresh_run> and, after inspect,
-DCPanelSizeReference=<previous_run>. Log must be the run folder's Editor.log.
Save only the existing panel, row and ammo style table. The previous wrapper
authoring path is removed. Preserve current user values; no C++ build or PIE.
"""
import importlib.util
import csv
import io
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/WeaponPanelSize20261022'
SCRIPT = P / 'Scripts/Editor/weapon_panel_size_setup.py'
PANEL = '/Game/DreamCatcher/UI/HUD/W_DC_WeaponPanel'
DISK = P / 'Content/DreamCatcher/UI/HUD/W_DC_WeaponPanel.uasset'
ASSETS = {'panel': PANEL, 'row': '/Game/DreamCatcher/UI/HUD/W_DC_WeaponRow',
          'style': '/Game/DreamCatcher/UI/HUD/DT_DC_AmmoTextStyle'}
BASE = {'panel': '17a8fcc339c9adc893a4a83382df50f3e7bce31a7eae063965f5f14e13ea5863',
        'row': '0732e25b55b0e455953f41af3848f0777584dac41b5168e9343e94f04f20354b',
        'style': '08f75db29a3c46dc4d7d0fb250958e8b7c1b70373e8d7660103d2eac800fbd0f'}
OUTPUT, DESIGN = (352, 120), (347, 109)
NEW = ('PanelOutputSize', 'PanelOutputScale', 'PanelDesignBounds')
SX, SY = 352 / 347, 120 / 109


def initialize():
    global ue, rich, screens, hud, UMG
    spec = importlib.util.spec_from_file_location('panel_size_rich_helpers', SCRIPT.with_name('weapon_ammo_richtext_setup.py'))
    rich = importlib.util.module_from_spec(spec); spec.loader.exec_module(rich); rich.initialize()
    ue, screens, hud, UMG = rich.ue, rich.screens, rich.hud, rich.UMG


def protection():
    allowed = {SCRIPT, *(disk(key) for key in ASSETS)}
    state = {}
    for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (P / top).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat(); state[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    names = ('.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Content/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD.uasset')
    return {'inventory': state, 'sha256': {name: rich.sha(P / name) for name in names}}


def disk(key):
    return P / ('Content/' + ASSETS[key][6:] + '.uasset')


def hashes():
    return {key: rich.sha(disk(key)) for key in ASSETS}


def styles(table):
    rows = list(csv.reader(io.StringIO(ue.DataTableFunctionLibrary.export_data_table_to_csv_string(table))))
    assert rows[0][1] == 'TextStyle'
    return {row[0]: row[1] for row in rows[1:] if row}


def snapshot(assets, folder, label):
    value = screens.snapshot_widgets({key: assets[key] for key in ('panel', 'row')}, folder, label)
    value['styles'] = styles(assets['style'])
    return value


def normalized(value):
    result = json.loads(json.dumps(value))
    for name in ('panel', 'row'):
        item = result[name]
        item.pop('blueprint', None)
        item['cdo'] = screens.ordered.item_subobject_order(item['cdo'])
        for widget in item['widgets'].values():
            widget['raw'] = screens.ordered.item_subobject_order(widget['raw'])
            if widget['slot']: widget['slot'] = screens.ordered.item_subobject_order(widget['slot'])
        for graph in item['graphs'].values():
            for node in graph.values():
                for pin in node.get('pins', {}).values():
                    if pin['type'] in ('Float (double-precision)', 'Float (single-precision)', 'Integer'):
                        pin['value'] = str(screens.Decimal(pin['value'] or '0').normalize())
    return result


def inspect(assets, report):
    report['measured'] = {}
    for key in ('panel', 'row'):
        _, widgets = hud.tree(UMG, assets[key])
        entries = {}
        for name, value in widgets.items():
            entry = {'class': value.get_class().get_name(), 'parent': value.get_parent().get_name() if value.get_parent() else None,
                     'transform': value.get_editor_property('RenderTransform').export_text()}
            slot = value.get_editor_property('slot')
            if isinstance(slot, ue.CanvasPanelSlot):
                p, s = slot.get_position(), slot.get_size()
                entry.update(position=[p.x, p.y], size=[s.x, s.y], auto_size=slot.get_auto_size(), layout=slot.get_editor_property('LayoutData').export_text())
            if isinstance(value, ue.SizeBox):
                entry.update(width=value.get_editor_property('WidthOverride'), height=value.get_editor_property('HeightOverride'))
            if isinstance(value, ue.TextBlock): entry.update(font=value.get_editor_property('Font').export_text(), text=str(value.get_text()))
            if isinstance(value, ue.RichTextBlock): entry.update(text=str(value.get_text()), style=value.get_text_style_set().get_path_name())
            entries[name] = entry
        report['measured'][key] = {'widgets': entries, 'cdo': screens.root_properties(report['before'][key]['cdo'])}
    report['measured']['fonts'] = {}
    for name, raw in report['before']['styles'].items():
        style = ue.TextBlockStyle(); assert style.import_text(raw)
        font = style.get_editor_property('Font')
        report['measured']['fonts'][name] = {'size': font.get_editor_property('Size'), 'font': font.export_text()}
    assert set(report['measured']['panel']['widgets']) == {'PanelSize', 'PanelCanvas', 'Row1', 'Row2', *NEW}
    assert all('Scale=(X=1.000000,Y=1.000000)' in value['transform'] for key in ('panel', 'row') for value in report['measured'][key]['widgets'].values())


SCALED_FLOATS = {'AllocatedHeight': SY, 'OnHeight': SY, 'OffHeight': SY,
                 'OnCardX': SX, 'OffCardX': SX, 'OnButtonY': SY, 'OffButtonY': SY}
VECTORS = tuple(screens_name for screens_name in ('OnNamePosition', 'OffNamePosition',
    'OnMagazinePosition', 'OffMagazinePosition', 'OnSlashPosition', 'OffSlashPosition', 'OnSparePosition', 'OffSparePosition'))
STATE = ('CurrentAlpha', 'StartAlpha', 'TargetAlpha', 'TransitionElapsed', 'SelectedState', 'HasItem', 'HasState', 'DesignPreview')


def near(a, b, epsilon=2e-4):
    assert abs(float(a) - float(b)) < epsilon, (a, b)


def vec(value):
    return ue.Vector2D(value.x * SX, value.y * SY)


def read_row_values(obj):
    return {**{name: float(obj.get_editor_property(name)) for name in SCALED_FLOATS},
            **{name: obj.get_editor_property(name).export_text() for name in VECTORS},
            **{name: obj.get_editor_property(name) for name in STATE}}


def write_row_values(obj, before):
    for name, ratio in SCALED_FLOATS.items(): obj.set_editor_property(name, before[name] * ratio)
    for name in VECTORS:
        value = ue.Vector2D(); assert value.import_text(before[name])
        obj.set_editor_property(name, vec(value))
    for name in STATE: assert obj.get_editor_property(name) == before[name], 'Unrelated selection state changed: ' + name


def resize_row(bp, source):
    _, widgets = hud.tree(UMG, bp)
    widgets['RowSize'].set_width_override(352)
    widgets['RowSize'].set_height_override(source['row']['widgets']['RowSize']['height'] * SY)
    for name, value in widgets.items():
        slot = value.get_editor_property('slot')
        if isinstance(slot, ue.CanvasPanelSlot):
            slot.set_position(vec(slot.get_position())); slot.set_size(vec(slot.get_size()))
    font = widgets['WeaponName'].get_editor_property('Font')
    font.set_editor_property('Size', font.get_editor_property('Size') * SY)
    widgets['WeaponName'].set_font(font)
    cdo = ue.get_default_object(bp.generated_class())
    write_row_values(cdo, read_row_values(cdo))


def resize_styles(table, before):
    stream = io.StringIO(newline=''); writer = csv.writer(stream)
    writer.writerow(['Name', 'TextStyle'])
    for name, raw in before.items():
        style = ue.TextBlockStyle(); assert style.import_text(raw)
        font = style.get_editor_property('Font')
        font.set_editor_property('Size', font.get_editor_property('Size') * SY)
        style.set_editor_property('Font', font)
        writer.writerow([name, style.export_text()])
    assert ue.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, stream.getvalue())


def unwrap_panel(bp):
    _, widgets = hud.tree(UMG, bp)
    inner = widgets['PanelSize']
    assert inner.get_parent() == widgets['PanelDesignBounds']
    # Native UMG unwrapping promotes the existing child, preserving the entire
    # original subtree; no root property write or replacement widget is needed.
    for name in NEW:
        assert widgets[name].get_children_count() == 1
        assert UMG.call_method('ReplaceWidgetWithChild', args=(bp, widgets[name]))
    _, current = hud.tree(UMG, bp)
    assert set(current) == {'PanelSize', 'PanelCanvas', 'Row1', 'Row2'}
    assert current['PanelSize'] == inner and inner.get_child_at(0) == widgets['PanelCanvas']
    inner.set_width_override(352); inner.set_height_override(120)
    slot = current['PanelCanvas'].get_editor_property('slot')
    slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    slot.set_padding(ue.Margin(0))
    return current


def graph_context(bp, name):
    g = screens.Graph.__new__(screens.Graph)
    g.e = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, name); assert g.e
    return g, {node.get_name(): node for node in g.e.list_all_nodes()}


def patch_panel_graphs(bp, report):
    pin, wire, literal = screens.pin, screens.wire, screens.literal
    g, nodes = graph_context(bp, 'ComputePanelLayout')
    for name, ratio in (('A', SY), ('B', SY)):
        value = float(pin(nodes['K2Node_CallFunction_0'], name).get_pin_value())
        literal(nodes['K2Node_CallFunction_0'], name, value * ratio)
    for name in ('FirstHeight', 'SecondHeight'):
        value = float(pin(nodes['K2Node_FunctionEntry_0'], name).get_pin_value())
        literal(nodes['K2Node_FunctionEntry_0'], name, value * SY)
    g, nodes = graph_context(bp, 'ApplyPanelLayout')
    layout = nodes['K2Node_CallFunction_0']
    for name in ('FirstHeight', 'SecondHeight'):
        literal(layout, name, float(pin(layout, name).get_pin_value()) * SY)
    height = pin(nodes['K2Node_CallFunction_1'], 'InHeightOverride')
    assert height.break_pin_links(); literal(nodes['K2Node_CallFunction_1'], 'InHeightOverride', 120)
    gap = g.pure('Subtract_DoubleDouble', A=120, B=pin(layout, 'Height'))
    row2_y = g.pure('Add_DoubleDouble', A=gap, B=pin(layout, 'Row2Y'))
    assert pin(nodes['K2Node_CallFunction_2'], 'Y').break_pin_links()
    wire(row2_y, pin(nodes['K2Node_CallFunction_2'], 'Y'))
    g.tail = pin(nodes['K2Node_CallFunction_1'], 'then'); assert g.tail.break_pin_links()
    screens.canvas_position(g, g.get('Row1'), g.pure('MakeVector2D', X=0, Y=gap))
    wire(g.tail, pin(nodes['K2Node_CallFunction_4'], 'execute'))
    report['graph_changes'] = {'ComputePanelLayout': 'Scale existing spacing/input defaults only; keep three outputs.',
        'ApplyPanelLayout': 'Constant height120; Row1 Y=120-occupiedHeight, Row2 Y=Row1Y+spacing.'}


def root_without(raw, names):
    value = screens.root_properties(raw)
    for name in names: value.pop(name, None)
    return value


def preserve(before, after):
    a, b = normalized(before), normalized(after)
    assert a['row']['graphs'] == b['row']['graphs'], 'Row logic or user Format Text changed'
    assert a['panel']['cdo'] == b['panel']['cdo'], 'Panel behavior defaults changed'
    assert root_without(a['row']['cdo'], set(SCALED_FLOATS) | set(VECTORS)) == root_without(b['row']['cdo'], set(SCALED_FLOATS) | set(VECTORS))
    for name in a['panel']['graphs']:
        if name not in ('ComputePanelLayout', 'ApplyPanelLayout'):
            assert a['panel']['graphs'][name] == b['panel']['graphs'][name], name
    # Exact old graph projection outside the reviewed spacing/default changes.
    ga, gb = a['panel']['graphs']['ComputePanelLayout'], b['panel']['graphs']['ComputePanelLayout']
    expected = json.loads(json.dumps(ga))
    for node, names in (('K2Node_CallFunction_0', ('A', 'B')), ('K2Node_FunctionEntry_0', ('FirstHeight', 'SecondHeight'))):
        for name in names:
            near(gb[node]['pins'][name]['value'], float(ga[node]['pins'][name]['value']) * SY)
            expected[node]['pins'][name]['value'] = gb[node]['pins'][name]['value']
    assert expected == gb
    ga, gb = a['panel']['graphs']['ApplyPanelLayout'], b['panel']['graphs']['ApplyPanelLayout']
    for name, old in ga.items():
        old, new = json.loads(json.dumps(old)), json.loads(json.dumps(gb[name]))
        for pin_name in old['pins']:
            allowed = ((name == 'K2Node_CallFunction_0' and pin_name in ('Height', 'Row2Y', 'FirstHeight', 'SecondHeight')) or
                (name == 'K2Node_CallFunction_1' and pin_name in ('InHeightOverride', 'then')) or
                (name == 'K2Node_CallFunction_2' and pin_name == 'Y') or
                (name == 'K2Node_CallFunction_4' and pin_name == 'execute'))
            if allowed:
                assert old['pins'][pin_name]['type'] == new['pins'][pin_name]['type']
                old['pins'][pin_name] = new['pins'][pin_name]
        assert old == new, 'Unexpected panel node change: ' + name
    for key in ('panel', 'row'):
        for name, old in a[key]['widgets'].items():
            if name in NEW: continue
            new = b[key]['widgets'][name]
            assert old['class'] == new['class']
            ignore = set()
            if name in ('PanelSize', 'RowSize'): ignore |= {'WidthOverride', 'HeightOverride', 'Slot'}
            if name in ('Row1', 'Row2'): ignore |= set(SCALED_FLOATS) | set(VECTORS)
            if name == 'WeaponName': ignore.add('Font')
            assert root_without(old['raw'], ignore) == root_without(new['raw'], ignore), key + '/' + name
            if old['slot'] and new['slot']:
                assert root_without(old['slot'], {'LayoutData'}) == root_without(new['slot'], {'LayoutData'}), 'Slot behavior changed: ' + name


def validate(assets, contract, report):
    _, panel = hud.tree(UMG, assets['panel']); _, row = hud.tree(UMG, assets['row'])
    assert set(panel) == {'PanelSize', 'PanelCanvas', 'Row1', 'Row2'}
    assert not panel['PanelSize'].get_parent() and panel['PanelCanvas'].get_parent() == panel['PanelSize']
    assert (panel['PanelSize'].get_editor_property('WidthOverride'), panel['PanelSize'].get_editor_property('HeightOverride')) == (352, 120)
    assert row['RowSize'].get_editor_property('WidthOverride') == 352
    near(row['RowSize'].get_editor_property('HeightOverride'), contract['row']['widgets']['RowSize']['height'] * SY)
    for collection in (panel, row):
        for value in collection.values():
            assert not isinstance(value, ue.ScaleBox)
            assert value.get_editor_property('RenderTransform').export_text() == ue.WidgetTransform().export_text()
    for collection, name in ((panel, 'PanelCanvas'), (row, 'RowCanvas')):
        slot = collection[name].get_editor_property('slot')
        assert slot.get_editor_property('HorizontalAlignment') == ue.HorizontalAlignment.H_ALIGN_FILL
        assert slot.get_editor_property('VerticalAlignment') == ue.VerticalAlignment.V_ALIGN_FILL
    cdo = ue.get_default_object(assets['row'].generated_class())
    for name, ratio in SCALED_FLOATS.items(): near(cdo.get_editor_property(name), float(contract['row']['cdo'][name]) * ratio)
    for name in VECTORS:
        old = ue.Vector2D(); assert old.import_text(contract['row']['cdo'][name])
        actual = cdo.get_editor_property(name); near(actual.x, old.x * SX); near(actual.y, old.y * SY)
    for name, value in row.items():
        old = contract['row']['widgets'][name]
        if 'position' in old:
            slot = value.get_editor_property('slot'); position, size = slot.get_position(), slot.get_size()
            for actual, expected in zip((position.x, position.y, size.x, size.y),
                                       (old['position'][0]*SX, old['position'][1]*SY, old['size'][0]*SX, old['size'][1]*SY)): near(actual, expected)
            assert slot.get_auto_size() == old['auto_size']
    expected_font = ue.SlateFontInfo(); assert expected_font.import_text(contract['row']['widgets']['WeaponName']['font'])
    expected_font.set_editor_property('Size', expected_font.get_editor_property('Size') * SY)
    assert row['WeaponName'].get_editor_property('Font').export_text() == expected_font.export_text()
    actual_styles = styles(assets['style'])
    for name, raw in actual_styles.items():
        expected = ue.TextBlockStyle(); assert expected.import_text(contract['styles'][name])
        font = expected.get_editor_property('Font'); font.set_editor_property('Size', font.get_editor_property('Size') * SY)
        expected.set_editor_property('Font', font)
        actual = ue.TextBlockStyle(); assert actual.import_text(raw)
        assert actual.export_text() == expected.export_text(), 'Other style data changed: ' + name
    assert str(row['AmmoText'].get_text()) == contract['row']['widgets']['AmmoText']['text']
    report['validation'] = {'native_panel': [352, 120], 'native_row_width': 352, 'no_scalebox_or_render_scale': True,
        'current_fonts_resized_proportionally': True, 'Format_Text_and_user_spacing_preserved': True}


def object_tests(assets, contract, report):
    library = ue.get_default_object(ue.load_class(None, '/Script/UMG.WidgetBlueprintLibrary'))
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    panel = library.call_method('Create', args=(world, assets['panel'].generated_class(), None)); assert panel
    first, second = [panel.get_editor_property(name) for name in ('Row1', 'Row2')]
    root, canvas = panel.get_editor_property('PanelSize'), panel.get_editor_property('PanelCanvas')
    assert canvas.get_parent() == root and not root.get_parent()
    item = ue.new_object(ue.load_class(None, '/Script/DreamCatcher.DCInventoryItemInstance'))
    results = []
    def measure(label):
        for child in (first, second): child.call_method('ApplyVisuals')
        panel.call_method('ApplyPanelLayout')
        args = (first.get_editor_property('AllocatedHeight'), second.get_editor_property('AllocatedHeight'),
                first.get_editor_property('HasItem'), second.get_editor_property('HasItem'), first.get_editor_property('CurrentAlpha'))
        relative_y, occupied, opacity = panel.call_method('ComputePanelLayout', args=args)
        near(root.get_editor_property('WidthOverride'), 352); near(root.get_editor_property('HeightOverride'), 120)
        near(first.get_editor_property('slot').get_position().y, 120 - occupied)
        near(second.get_editor_property('slot').get_position().y, 120 - occupied + relative_y)
        near(panel.get_render_opacity(), opacity)
        for child in (first, second):
            child_root = child.get_editor_property('RowSize')
            near(child_root.get_editor_property('WidthOverride'), 352)
            near(child_root.get_editor_property('HeightOverride'), child.get_editor_property('AllocatedHeight'))
        panel.force_layout_prepass(); desired = panel.get_desired_size()
        if desired.x or desired.y: near(desired.x, 352); near(desired.y, 120)
        results.append({'state': label, 'occupied_height': occupied, 'row_y': [first.get_editor_property('slot').get_position().y, second.get_editor_property('slot').get_position().y],
            'alphas': [first.get_editor_property('CurrentAlpha'), second.get_editor_property('CurrentAlpha')], 'opacity': opacity, 'desired_size': [desired.x, desired.y]})
    def update(a, b, selected=True):
        first.call_method('UpdateData', args=(item if a else None, selected))
        second.call_method('UpdateData', args=(item if b else None, not selected))
    def advance(dt):
        for child in (first, second): child.call_method('AdvanceTransition', args=(dt,))
    update(True, True); measure('first_selected')
    update(True, True, False); advance(.125); measure('switch_half')
    update(True, True); advance(.125); measure('reverse_half'); advance(.125); measure('first_restored')
    update(True, True, False); advance(.25); measure('second_selected')
    update(False, False); measure('empty')
    update(True, False); measure('only_first')
    update(False, True, False); measure('only_second')
    for index, expected in ((0,107*SY), (1,108*SY), (2,107.5*SY), (4,120), (5,1), (6,76*SY), (7,76*SY)):
        near(results[index]['occupied_height'], expected)
    assert results[1]['alphas'] == [.5,.5] and results[2]['alphas'] == [.75,.25]
    tags = []
    for suffix in ('MagazineAmmo', 'SpareAmmo'):
        tag = ue.GameplayTag(); assert tag.import_text('(TagName="Lyra.ShooterGame.Weapon.' + suffix + '")'); tags.append(tag)
    fmt = contract['format_text']
    ammo = []
    for magazine, spare in ((30,120), (10,120), (9,120), (0,120), (30,90), (9,1234)):
        for tag, desired in zip(tags, (magazine,spare)):
            existing = item.call_method('GetStatTagStackCount', args=(tag,))
            if existing: item.call_method('RemoveStatTagStack', args=(tag,existing))
            if desired: item.call_method('AddStatTagStack', args=(tag,desired))
        first.call_method('UpdateData', args=(item,True))
        actual = str(first.get_editor_property('AmmoText').get_text())
        assert actual == fmt.replace('{Magazine}',str(magazine)).replace('{Spare}',str(spare)), actual
        ammo.append(actual)
    first.call_method('UpdateData', args=(None,False)); assert str(first.get_editor_property('AmmoText').get_text()) == ''
    first.call_method('UpdateData', args=(item,True)); assert str(first.get_editor_property('AmmoText').get_text()) == ammo[-1]
    report['object_tests'] = {'states':results, 'ammo':ammo, 'empty_rebind':True, 'PIE':False,
        'Slate_desired_size_available': all(value['desired_size']==[352,120] for value in results), 'visual_or_click_verified':False}


def apply(assets, previous, folder, report):
    contract = json.loads(json.dumps(previous['measured'])); contract['styles'] = previous['before']['styles']
    formats = [node for node in previous['before']['row']['graphs']['UpdateData'].values() if node['class'].endswith('K2Node_FormatText')]
    assert len(formats) == 1
    value = formats[0]['pins']['Format']['value']
    match = re.fullmatch(r'NSLOCTEXT\("[^"]*", "[^"]*", ("(?:\\.|[^"])*")\)', value)
    contract['format_text'] = json.loads(match[1]) if match else value
    _, panel_widgets = hud.tree(UMG, assets['panel'])
    template_values = {name:read_row_values(panel_widgets[name]) for name in ('Row1','Row2')}
    resize_styles(assets['style'], contract['styles']); resize_row(assets['row'], contract)
    screens.compile_widget(assets['row'],report)
    panel = unwrap_panel(assets['panel'])
    for name in ('Row1','Row2'):
        write_row_values(panel[name],template_values[name])
        slot=panel[name].get_editor_property('slot'); slot.set_size(vec(slot.get_size()))
    panel['Row1'].get_editor_property('slot').set_position(ue.Vector2D(0,120-107*SY))
    panel['Row2'].get_editor_property('slot').set_position(ue.Vector2D(0,120-107*SY+71*SY))
    patch_panel_graphs(assets['panel'],report); screens.compile_widget(assets['panel'],report)
    report['after'] = snapshot(assets,folder,'after')
    preserve(report['before'],report['after']); validate(assets,contract,report); object_tests(assets,contract,report)
    assert normalized(snapshot(assets,folder,'tested')) == normalized(report['after']), 'Transient tests altered persistent data'
    hud.clean(ASSETS.values()); hud.log_check(folder/'Editor.log')
    assert protection()==report['protected_before'] and hashes()==report['before_sha256']
    for key in ('style','row','panel'):
        hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(assets[key],only_if_is_dirty=False)
        report['asset_saves'].append(ASSETS[key])
    report['contract'] = contract


def verify(assets, previous, folder, report):
    report['after'] = snapshot(assets,folder,'reloaded')
    assert normalized(report['after']) == normalized(previous['after']), 'Saved native data differs'
    validate(assets,previous['contract'],report)
    for key in ('row','panel'): screens.compile_widget(assets[key],report)
    object_tests(assets,previous['contract'],report)
    assert normalized(snapshot(assets,folder,'tested')) == normalized(report['after'])


def main():
    initialize()
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = screens.arg(command, 'DCPanelSizeMode', 'inspect|apply|verify')
    run = screens.arg(command, 'DCPanelSizeRun', 'Direct_[A-Za-z0-9_]{1,56}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'; assert not output.exists(), 'Use a fresh run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    protected, before_sha = protection(), hashes()
    report = {'mode': mode, 'run': run, 'complete': False, 'asset_saves': [], 'compile_requested': [],
        'PIE': False, 'cpp_build': False, 'visual_verified': False, 'script_sha256': rich.sha(SCRIPT),
        'before_sha256': before_sha, 'protected_before': protected}
    try:
        settings = hud.export(ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')), folder / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        assets = {key: ue.load_asset(package) for key, package in ASSETS.items()}; assert all(assets.values())
        report['before'] = snapshot(assets, folder, 'before')
        if mode == 'inspect':
            assert before_sha == BASE, 'Latest user baseline changed'
            inspect(assets, report)
        else:
            reference = screens.arg(command, 'DCPanelSizeReference', 'Direct_[A-Za-z0-9_]{1,56}')
            previous = rich.read(R / reference / 'report.json')
            assert previous['complete'] and previous['protection_passed']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == report['script_sha256']
            assert previous['protected_before'] == protected
            assert before_sha == previous['after_sha256']
            report['reference'] = reference
            if mode == 'apply':
                assert before_sha == BASE
                assert normalized(report['before']) == normalized(previous['before'])
                apply(assets, previous, folder, report)
            else:
                verify(assets, previous, folder, report)
        hud.clean(ASSETS.values() if mode == 'verify' else ()); hud.log_check(folder / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = hashes()
        report['protection_passed'] = protection() == protected
        if not report['protection_passed']: report['complete'] = False
        if mode != 'apply' and report['after_sha256'] != before_sha: report['complete'] = False
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.log('DC_PANEL_DIRECT_SIZE ' + json.dumps({'mode': mode, 'complete': report['complete'], 'saves': report['asset_saves']}))


if __name__ == '__main__':
    main()
