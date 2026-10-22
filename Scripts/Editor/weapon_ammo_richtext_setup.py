"""Approved ammo-only HUD edit, via Unreal Editor APIs; no C++ build or PIE.

Run separate inspect, apply, verify commandlets with -DCAmmoRichMode=<mode>,
-DCAmmoRichRun=<unique_id>, and (after inspect) -DCAmmoRichReference=<run>.
Use -unattended -run=pythonscript -SCCProvider=None -EnablePlugins=UMGToolSet.
The absolute log must be Saved/Diagnostics/AmmoSpacing20261022/<run>/Editor.log.
Only W_DC_WeaponRow and the new DT_DC_AmmoTextStyle may be saved. Existing
user changes are the baseline; the original HUD generator is never run.
"""
import hashlib
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
R = P / 'Saved/Diagnostics/AmmoSpacing20261022'
SCRIPT = P / 'Scripts/Editor/weapon_ammo_richtext_setup.py'
ROW = '/Game/DreamCatcher/UI/HUD/W_DC_WeaponRow'
TABLE = '/Game/DreamCatcher/UI/HUD/DT_DC_AmmoTextStyle'
BASE_SHA = '6bfe59266daed28a21958186645eb3441eb3aa97bdf451b9d00d10a7ebd338b7'
SAVED_APPLY_SCRIPT_SHA = '62fa5574a1c0d7f8a023c03e450dbec046a652fba7bb1f5268e1d083956d8a16'
SAVED_APPLY_REPORT_SHA = 'c09c6e5967a1062ba6129fbdf6eac50281b37dd29044f1d3c04267514df2e937'
FORMAT = '<Magazine>{Magazine}</><Slash>/</><Spare>{Spare}</>'
OLD = ('Magazine', 'Slash', 'Spare')


def module(name):
    spec = importlib.util.spec_from_file_location('ammo_rich_' + name, SCRIPT.parent / (name + '.py'))
    result = importlib.util.module_from_spec(spec); spec.loader.exec_module(result)
    return result


def sha(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''): result.update(block)
    return result.hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def disk(package):
    assert package in (ROW, TABLE)
    return P / ('Content/' + package[6:] + '.uasset')


def target_hashes():
    return {key: sha(disk(value)) if disk(value).exists() else None for key, value in {'row': ROW, 'table': TABLE}.items()}


def protection():
    result = {}
    allowed = {disk(ROW), disk(TABLE), SCRIPT}
    for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (P / top).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat()
                result[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    hashes = {name: sha(P / name) for name in ('.git/HEAD', '.git/index', 'DreamCatcher.uproject',
        'Binaries/Win64/UnrealEditor-DreamCatcher.dll', 'Content/DreamCatcher/UI/HUD/W_DC_WeaponPanel.uasset',
        'Content/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD.uasset')}
    return {'inventory': result, 'sha256': hashes}


def initialize():
    global ue, screens, hud, UMG
    import unreal as ue
    screens = module('weapon_screens_hud_setup'); screens.ue = ue
    screens.hud = module('ingame_hud_layout_setup'); hud = screens.hud
    screens.helpers = module('weapon_ammo_hud_setup')
    screens.old_art = module('ingame_hud_art_setup'); screens.old_art.ue = ue
    screens.ordered = module('pistol_slot_setup'); screens.fields = module('mission_hud_setup')
    UMG = ue.get_default_object(ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
    assert UMG; screens.UMG = UMG


def snapshot(bp, folder, label):
    return screens.snapshot_widgets({'row': bp}, folder, label)


def normalized(value):
    # Preserve authored nodes/links and every scalar, normalize numeric defaults
    # and subobject export ordering exactly as the existing HUD helper does.
    result = json.loads(json.dumps(value))
    for item in result.values():
        item.pop('blueprint', None)
        item['cdo'] = screens.ordered.item_subobject_order(item['cdo'])
        for widget in item['widgets'].values():
            widget['raw'] = screens.ordered.item_subobject_order(widget['raw'])
            if widget['slot']: widget['slot'] = screens.ordered.item_subobject_order(widget['slot'])
        if 'AmmoText' in item['widgets']:
            # RemoveWidget detaches these three slots, but the pre-save export
            # still includes their empty orphan UObjects until native saving/GC.
            # Never ignore a connected slot or any other canvas child/property.
            raw = item['widgets']['RowCanvas']['raw']
            for index, offsets in ((4, 'Left=43.000000,Top=27.000000,Right=60.000000,Bottom=44.000000'),
                                   (5, 'Left=113.000000,Top=44.000000,Right=10.000000,Bottom=24.000000'),
                                   (6, 'Left=130.000000,Top=46.000000,Right=40.000000,Bottom=20.000000')):
                name = 'CanvasPanelSlot_' + str(index)
                assert not re.search(r'^   Slots\(\d+\)=.*\x27' + name + r'\x27', raw, re.M)
                pattern = r'^   Begin Object (?:Class=/Script/UMG.CanvasPanelSlot )?Name="' + name + r'" ExportPath="[^"\n]+"\n(.*?)^   End Object\n'
                def remove_orphan(match):
                    assert match[1] in ('', '      LayoutData=(Offsets=(' + offsets + '))\n')
                    return ''
                raw, count = re.subn(pattern, remove_orphan, raw, flags=re.M | re.S)
                assert count in (0, 2), (name, count)
            item['widgets']['RowCanvas']['raw'] = raw
        for graph in item['graphs'].values():
            for node in graph.values():
                for pin in node.get('pins', {}).values():
                    if pin['type'] in ('Float (double-precision)', 'Float (single-precision)', 'Integer'):
                        pin['value'] = str(screens.Decimal(pin['value'] or '0').normalize())
                if node['class'] == '/Script/BlueprintGraph.K2Node_FormatText':
                    literal = node['pins']['Format']['value']
                    assert literal == FORMAT or re.fullmatch(r'NSLOCTEXT\("", "[A-Fa-f0-9]{32}", "' + re.escape(FORMAT) + r'"\)', literal)
                    node['pins']['Format']['value'] = FORMAT
    return result


def inspect(bp, report):
    _, widgets = hud.tree(UMG, bp)
    assert set(widgets) == {'RowSize', 'RowCanvas', 'OnCard', 'OffCard', 'NumberButton', 'WeaponName', *OLD}
    report['styles'] = {}
    for name in OLD:
        value = widgets[name]; assert isinstance(value, ue.TextBlock)
        style = ue.TextBlockStyle()
        for target, source in (('Font', 'Font'), ('ColorAndOpacity', 'ColorAndOpacity'),
                               ('ShadowColorAndOpacity', 'ShadowColorAndOpacity')):
            style.set_editor_property(target, value.get_editor_property(source))
        # FTextBlockStyle uses the float Slate wrapper in UE 5.8, while
        # UTextBlock exposes the double CoreUObject vector.
        offset = style.get_editor_property('ShadowOffset')
        original = value.get_editor_property('ShadowOffset')
        offset.set_editor_property('X', original.x); offset.set_editor_property('Y', original.y)
        style.set_editor_property('ShadowOffset', offset)
        report['styles'][name] = style.export_text()
    report['styles']['Default'] = report['styles']['Magazine']
    e = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'UpdateData')
    report['format_actions'] = [str(name) for name in e.list_available_nodes([]) if 'formattext' in str(name).lower().replace(' ', '')]
    assert len(report['format_actions']) == 1, report['format_actions']
    report['table_api'] = [name for name in dir(ue.DataTableFunctionLibrary) if any(x in name for x in ('json', 'row'))]
    report['runtime_api'] = {'widget_library': ue.load_class(None, '/Script/UMG.WidgetBlueprintLibrary').get_path_name()}
    slot = widgets['Magazine'].get_editor_property('slot')
    report['magazine_layout'] = slot.get_editor_property('LayoutData').export_text()
    report['preview'] = FORMAT.replace('{Magazine}', str(widgets['Magazine'].get_text())).replace('{Spare}', str(widgets['Spare'].get_text()))


def graph_context(bp, name):
    g = screens.Graph.__new__(screens.Graph)
    g.e = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, name); assert g.e
    return g, {node.get_name(): node for node in g.e.list_all_nodes()}


def replace_graphs(bp, format_action, report):
    pin, wire = screens.pin, screens.wire
    g, nodes = graph_context(bp, 'UpdateData')
    for index in (10, 13):
        assert {str(value.get_pin_name()) for value in ue.BlueprintEditorLibrary.list_all_pins(nodes['K2Node_CallFunction_' + str(index)])} == {'self', 'Tag', 'ReturnValue'}
    removed = ['K2Node_VariableGet_' + str(i) for i in (2, 3, 9, 10)]
    removed += ['K2Node_CallFunction_' + str(i) for i in (2, 3, 12, 15)]
    # Remove only the two former text setters in each branch, retaining the
    # existing inventory access, integer conversions, item/name/selection flow.
    g.e.remove_nodes([nodes[name] for name in removed])
    fmt = g.e.create_node_from_name(format_action, ue.Vector2D(1400, 900), [])
    assert fmt and fmt.get_class().get_name() == 'K2Node_FormatText'
    screens.literal(fmt, 'Format', FORMAT)
    assert {str(value.get_pin_name()) for value in ue.BlueprintEditorLibrary.list_all_pins(fmt)} == {'Format', 'Result', 'Magazine', 'Spare'}
    wire(pin(nodes['K2Node_CallFunction_11'], 'ReturnValue'), pin(fmt, 'Magazine'))
    wire(pin(nodes['K2Node_CallFunction_14'], 'ReturnValue'), pin(fmt, 'Spare'))
    g.tail = pin(nodes['K2Node_CallFunction_1'], 'then')
    empty = g.widget('AmmoText', 'RichTextBlock.SetText', InText='')
    wire(g.tail, pin(nodes['K2Node_FunctionResult_0'], 'execute'))
    g.tail = [pin(nodes['K2Node_CallFunction_' + str(i)], 'then') for i in (8, 9)]
    filled = g.widget('AmmoText', 'RichTextBlock.SetText', InText=pin(fmt, 'Result'))
    wire(g.tail, pin(nodes['K2Node_FunctionResult_0'], 'execute'))
    report['graph_patch'] = {'UpdateData': {'removed': removed, 'format_node': fmt.get_name(),
        'clear_node': empty.get_name(), 'update_node': filled.get_name()}}
    g, nodes = graph_context(bp, 'ApplyVisuals')
    removed = ['K2Node_VariableGet_' + str(i) for i in (8, 9, 10)]
    removed += ['K2Node_CallFunction_' + str(i) for i in (17, 18, 19, 20)]
    g.e.remove_nodes([nodes[name] for name in removed])
    wire(g.get('AmmoText'), pin(nodes['K2Node_CallFunction_15'], 'Widget'))
    wire(pin(nodes['K2Node_CallFunction_16'], 'then'), pin(nodes['K2Node_IfThenElse_0'], 'execute'))
    report['graph_patch']['ApplyVisuals'] = {'removed': removed}


def make_table(styles):
    assert not ue.EditorAssetLibrary.does_asset_exist(TABLE) and not disk(TABLE).exists()
    struct = ue.load_object(None, '/Script/UMG.RichTextStyleRow'); assert struct
    factory = ue.DataTableFactory(); factory.set_editor_property('Struct', struct)
    table = ue.AssetToolsHelpers.get_asset_tools().create_asset(TABLE.rsplit('/', 1)[1], TABLE.rsplit('/', 1)[0], ue.DataTable, factory)
    assert table
    stream = io.StringIO(newline='')
    writer = csv.writer(stream); writer.writerow(['Name', 'TextStyle'])
    for name in ('Default', *OLD): writer.writerow([name, styles[name]])
    assert ue.DataTableFunctionLibrary.fill_data_table_from_csv_string(table, stream.getvalue())
    return table


def table_state(table):
    assert ue.DataTableFunctionLibrary.get_data_table_row_struct(table).get_path_name() == '/Script/UMG.RichTextStyleRow'
    raw = ue.DataTableFunctionLibrary.export_data_table_to_json_string(table)
    result = {value['Name']: value for value in json.loads(raw)}
    assert set(result) == {'Default', *OLD}
    return result


def preserve(before, after, patch):
    a, b = normalized(before)['row'], normalized(after)['row']
    assert a['parent'] == b['parent']
    cdo_a, cdo_b = screens.root_properties(a['cdo']), screens.root_properties(b['cdo'])
    for name in OLD: assert cdo_a.pop(name) == 'None'
    assert cdo_b.pop('AmmoText') == 'None'
    assert cdo_a == cdo_b, 'Other row defaults changed'
    for name in ('RowSize', 'OnCard', 'OffCard', 'NumberButton', 'WeaponName'):
        assert a['widgets'][name] == b['widgets'][name], 'Other widget changed: ' + name
    # RowCanvas differs only in child slots; retain all its own root properties.
    canvas_a = screens.root_properties(a['widgets']['RowCanvas']['raw'])
    canvas_b = screens.root_properties(b['widgets']['RowCanvas']['raw'])
    assert canvas_a == canvas_b
    assert a['widgets']['RowCanvas']['slot'] == b['widgets']['RowCanvas']['slot']
    assert set(a['graphs']) == set(b['graphs'])
    for name in a['graphs']:
        if name not in patch:
            assert a['graphs'][name] == b['graphs'][name], 'Other graph changed: ' + name
            continue
        old_graph, new_graph = a['graphs'][name], b['graphs'][name]
        removed = set(patch[name]['removed'])
        preserved = set(old_graph) - removed
        assert preserved <= set(new_graph)
        # Removed/new nodes only change their immediate connections. For every
        # preserved node compare class, title, values, types and remaining links.
        for key in preserved:
            old, new = json.loads(json.dumps(old_graph[key])), json.loads(json.dumps(new_graph[key]))
            for value in (old, new):
                for pin_name, value_pin in value.get('pins', {}).items():
                    value_pin['links'] = [link for link in value_pin['links'] if link[0] in preserved]
                    # The last magazine position now feeds HasItem directly.
                    if name == 'ApplyVisuals' and (key, pin_name) in (
                            ('K2Node_CallFunction_16', 'then'), ('K2Node_IfThenElse_0', 'execute')):
                        value_pin['links'] = []
            assert old == new, 'Unexpected graph change: ' + name + '/' + key


def validate(bp, table, previous, report):
    _, widgets = hud.tree(UMG, bp)
    assert set(widgets) == {'RowSize', 'RowCanvas', 'OnCard', 'OffCard', 'NumberButton', 'WeaponName', 'AmmoText'}
    rich = widgets['AmmoText']; assert isinstance(rich, ue.RichTextBlock)
    assert rich.get_text_style_set() == table
    assert rich.get_editor_property('Justification') == ue.TextJustify.LEFT
    assert rich.get_editor_property('MinDesiredWidth') == 0
    assert not rich.get_editor_property('AutoWrapText') and rich.get_editor_property('WrapTextAt') == 0
    assert rich.get_editor_property('Margin').export_text() == ue.Margin(0).export_text()
    assert not rich.get_editor_property('bOverrideDefaultStyle')
    assert rich.get_editor_property('slot').get_auto_size()
    assert rich.get_editor_property('slot').get_alignment().export_text() == ue.Vector2D(0, 0).export_text()
    assert rich.get_editor_property('slot').get_editor_property('LayoutData').export_text() == previous['magazine_layout']
    assert str(rich.get_text()) == previous['preview']
    report['table_state'] = table_state(table)
    for name in ('Default', *OLD):
        style = ue.TextBlockStyle(); assert style.import_text(previous['styles'][name])
        # CSV round-trip must preserve every authored TextStyle field. Re-export
        # each row's struct value through the native CSV serializer.
    csv_text = ue.DataTableFunctionLibrary.export_data_table_to_csv_string(table)
    rows = list(csv.reader(io.StringIO(csv_text)))
    assert rows[0][1] == 'TextStyle'
    for row in rows[1:]:
        if not row: continue
        style = ue.TextBlockStyle(); assert style.import_text(row[1])
        expected = ue.TextBlockStyle(); assert expected.import_text(previous['styles'][row[0]])
        assert style.export_text() == expected.export_text(), 'Style changed: ' + row[0]
    graphs = screens.old_art.graph_state(bp)
    formats = [value for graph in graphs.values() for value in graph.values() if value['class'] == '/Script/BlueprintGraph.K2Node_FormatText']
    assert len(formats) == 1
    fmt = formats[0]
    assert FORMAT in fmt['pins']['Format']['value']
    assert set(fmt['pins']) == {'Format', 'Result', 'Magazine', 'Spare'}
    for index, role in ((11, 'Magazine'), (14, 'Spare')):
        assert fmt['pins'][role]['links'] == [['K2Node_CallFunction_' + str(index), 'ReturnValue']]
        assert graphs['UpdateData']['K2Node_CallFunction_' + str(index)]['pins']['bUseGrouping']['value'] == 'false'
    report['validation'] = {'one_rich_text': True, 'actual_format_text_node': True, 'styles_32_16_14_preserved': True,
        'left_aligned_auto_width_no_wrap': True, 'existing_inventory_conversions_retained': True}


def object_tests(bp, report):
    # Existing compiled math tests include 0.25s transition and reversal. The
    # protected panel is loaded for read-only math/template checks, never saved.
    assets = {'row': bp, 'panel': ue.load_asset(screens.ASSETS['panel'])}
    screens.object_transition_tests(assets, report)
    library = ue.get_default_object(ue.load_class(None, '/Script/UMG.WidgetBlueprintLibrary'))
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    row = library.call_method('Create', args=(world, bp.generated_class(), None))
    assert row and row != ue.get_default_object(bp.generated_class())
    rich = row.get_editor_property('AmmoText'); assert rich and isinstance(rich, ue.RichTextBlock)
    item = ue.new_object(ue.load_class(None, '/Script/DreamCatcher.DCInventoryItemInstance'))
    tags = []
    for name in ('MagazineAmmo', 'SpareAmmo'):
        tag = ue.GameplayTag(); assert tag.import_text('(TagName="Lyra.ShooterGame.Weapon.' + name + '")')
        tags.append(tag)
    values = []
    for magazine, spare in ((30, 120), (10, 120), (9, 120), (0, 120), (30, 90), (9, 1234)):
        for tag, desired in zip(tags, (magazine, spare)):
            current = item.call_method('GetStatTagStackCount', args=(tag,))
            if current: item.call_method('RemoveStatTagStack', args=(tag, current))
            if desired: item.call_method('AddStatTagStack', args=(tag, desired))
        row.call_method('UpdateData', args=(item, True)); row.call_method('ApplyVisuals')
        actual = str(rich.get_text())
        assert actual == FORMAT.replace('{Magazine}', str(magazine)).replace('{Spare}', str(spare)), actual
        values.append(actual)
    row.call_method('UpdateData', args=(None, False)); row.call_method('ApplyVisuals')
    assert str(rich.get_text()) == ''
    assert not row.get_editor_property('HasItem') and not row.get_editor_property('HasState')
    assert row.get_editor_property('RowSize').get_visibility() == ue.SlateVisibility.COLLAPSED
    row.call_method('UpdateData', args=(item, True)); row.call_method('ApplyVisuals')
    assert str(rich.get_text()) == values[-1] and row.get_editor_property('HasItem')
    assert row.get_editor_property('RowSize').get_visibility() == ue.SlateVisibility.VISIBLE
    positions = []
    row.call_method('UpdateData', args=(item, False))
    for step in (0, .125, .125):
        row.call_method('AdvanceTransition', args=(step,)); row.call_method('ApplyVisuals')
        alpha = row.get_editor_property('CurrentAlpha')
        actual = rich.get_editor_property('slot').get_position()
        expected = row.call_method('ComputeLayout', args=(alpha,))[6]
        assert actual.export_text() == expected.export_text()
        positions.append({'alpha': alpha, 'position': [actual.x, actual.y]})
    row.call_method('PreparePreview')
    assert row.get_editor_property('HasItem')
    report['ammo_object_tests'] = {'actual_compiled_UpdateData': values, 'empty_clears_and_collapses': True,
        'rebind_restores': True, 'combined_transition_positions': positions, 'preview_function_valid': True,
        'PIE': False, 'Slate_geometry_or_visual_validation': False}


def apply(bp, previous, folder, report):
    _, widgets = hud.tree(UMG, bp)
    table = make_table(previous['styles'])
    rich = screens.widget(bp, ue.RichTextBlock, 'AmmoText', widgets['RowCanvas'])
    rich.set_text_style_set(table); rich.set_text(previous['preview'])
    rich.set_editor_property('Justification', ue.TextJustify.LEFT)
    rich.set_min_desired_width(0); rich.set_auto_wrap_text(False)
    rich.set_editor_property('WrapTextAt', 0); rich.set_editor_property('Margin', ue.Margin(0))
    rich.set_visibility(widgets['Magazine'].get_visibility())
    layout = rich.get_editor_property('slot').get_editor_property('LayoutData')
    assert layout.import_text(previous['magazine_layout'])
    rich.get_editor_property('slot').set_editor_property('LayoutData', layout)
    rich.get_editor_property('slot').set_alignment(ue.Vector2D(0, 0))
    rich.get_editor_property('slot').set_auto_size(True)
    replace_graphs(bp, previous['format_actions'][0], report)
    for name in OLD: assert UMG.call_method('RemoveWidget', args=(bp, widgets[name]))
    screens.compile_widget(bp, report)
    report['after'] = snapshot(bp, folder, 'after')
    preserve(report['before'], report['after'], report['graph_patch'])
    validate(bp, table, previous, report)
    object_tests(bp, report)
    # Transient tests must not leak into CDOs/templates or any saved value.
    tested = snapshot(bp, folder, 'tested')
    assert normalized(tested) == normalized(report['after'])
    hud.clean((ROW, TABLE)); hud.log_check(folder / 'Editor.log')
    assert protection() == report['protected_before'] and target_hashes() == report['before_sha256']
    for package, obj in ((TABLE, table), (ROW, bp)):
        hud.assert_no_other_editor()
        assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
        report['asset_saves'].append(package)
    report['contract'] = {key: previous[key] for key in ('styles', 'magazine_layout', 'preview')}


def verify(bp, previous, folder, report):
    table = ue.load_asset(TABLE); assert table
    report['after'] = snapshot(bp, folder, 'reloaded')
    assert normalized(report['after']) == normalized(previous['after']), 'Reload changed widget/graph/default data'
    validate(bp, table, previous['contract'], report)
    assert report['table_state'] == previous['table_state']
    screens.compile_widget(bp, report)
    object_tests(bp, report)
    tested = snapshot(bp, folder, 'tested')
    assert normalized(tested) == normalized(report['after']), 'Reload compile/test changed authored state'
    # Compile can dirty only the already verified target in memory. Do not save.
    hud.clean((ROW,)); hud.log_check(folder / 'Editor.log')


def main():
    initialize()
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = screens.arg(command, 'DCAmmoRichMode', 'inspect|apply|verify')
    run = screens.arg(command, 'DCAmmoRichRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'; assert not output.exists(), 'Use a fresh run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    protected, before_sha = protection(), target_hashes()
    report = {'mode': mode, 'run': run, 'complete': False, 'asset_saves': [], 'compile_requested': [],
        'PIE': False, 'cpp_build': False, 'runtime_visual_verified': False, 'script_sha256': sha(SCRIPT),
        'before_sha256': before_sha, 'protected_before': protected}
    try:
        settings = hud.export(ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')), folder / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        bp = ue.load_asset(ROW); assert bp
        report['before'] = snapshot(bp, folder, 'before')
        if mode == 'inspect':
            assert before_sha == {'row': BASE_SHA, 'table': None}, 'Reviewed baseline changed'
            inspect(bp, report)
        else:
            reference = screens.arg(command, 'DCAmmoRichReference', '[A-Za-z0-9_]{1,64}')
            previous = read(R / reference / 'report.json')
            assert previous['complete'] and previous['protection_passed']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == report['script_sha256'] or (
                mode == 'verify' and reference == 'Rich_D_apply' and previous['script_sha256'] == SAVED_APPLY_SCRIPT_SHA
                and sha(R / reference / 'report.json') == SAVED_APPLY_REPORT_SHA)
            assert previous['protected_before'] == protected
            assert before_sha == previous['after_sha256']
            report['reference'] = reference
            if mode == 'apply':
                assert before_sha == {'row': BASE_SHA, 'table': None}
                assert normalized(report['before']) == normalized(previous['before'])
                apply(bp, previous, folder, report)
            else:
                verify(bp, previous, folder, report)
        hud.clean((ROW,) if mode == 'verify' else ()); hud.log_check(folder / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = target_hashes()
        report['protection_passed'] = protection() == protected
        if not report['protection_passed']: report['complete'] = False
        if mode != 'apply' and report['after_sha256'] != before_sha: report['complete'] = False
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.log('DC_AMMO_RICHTEXT ' + json.dumps({'mode': mode, 'complete': report['complete'], 'saves': report['asset_saves']}))


if __name__ == '__main__':
    main()
