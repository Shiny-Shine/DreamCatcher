"""Approved HUD artwork only: five existing assets and seven new materials.

Separate unattended inspect/apply/fresh-verify commandlets; never PIE or build.
Keep the Lyra inventory, quickbar, HP/armor and mission behavior. New materials
extend a duplicate of the original health master; shared source assets stay intact.
--commands <prefix> prints launches only. No overwrite or partial-run recovery.
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
R = P / 'Saved/Diagnostics/InGameHUDArt20261021'
SCRIPT = P / 'Scripts/Editor/ingame_hud_art_setup.py'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
EXISTING = {
    'row': '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/W_QuickBarSlot',
    'panel': '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/W_QuickBar',
    'health': '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_Healthbar',
    'status': '/Game/DreamCatcher/UI/HUD/W_DC_PlayerStatus',
    'mission': '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example'}
ART = '/Game/DreamCatcher/UI/HUD/Art/'
NEW = {key: ART + name for key, name in {
    'master': 'M_DC_HUD_HealthArt', 'fill': 'MI_DC_HUD_HealthArt_Fill',
    'border': 'MI_DC_HUD_HealthArt_Border', 'glow': 'MI_DC_HUD_HealthArt_Glow',
    'left': 'MI_DC_HUD_ArmorArt_Left', 'middle': 'MI_DC_HUD_ArmorArt_Middle',
    'right': 'MI_DC_HUD_ArmorArt_Right'}.items()}
SHARED = '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/LyraStarterGame/UI/Hud/Art/'
SOURCE = {key: SHARED + name for key, name in {
    'master': 'M_UI_Base_HealthBar', 'fill': 'MI_UI_HealthBar_Fill',
    'border': 'MI_UI_HealthBar_BarBorder', 'glow': 'MI_UI_HealthBar_Glow'}.items()}
SCREENS = '/Game/DreamCatcher/Screens/InGame_UI/'
TEXTURES = {name: SCREENS + name for name in ('HP_Bar', 'Shield_Bar', 'Mission_UI',
    'checked', 'Not_checked', 'Main_GUN_Card', 'Main_GUN_OFF', 'SUB_GUN_Card', 'Sub_GUN_OFF')}
BRUSH_VARS = ('DCSelectedCardBrush', 'DCUnselectedCardBrush')
ROWS = ('W_QuickBarSlot_1', 'W_QuickBarSlot_2', 'W_QuickBarSlot_3')
WHITE_PARAMS = {'Bar_Health_Fill_RGBA', 'Bar_Health_Highlight_RGBA'}
UV_RECTS = {'left': (0, 0, 141 / 440, 1), 'middle': (149 / 440, 0, 142 / 440, 1),
            'right': (299 / 440, 0, 141 / 440, 1)}


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def disk(package):
    assert package.startswith('/Game/DreamCatcher/')
    return P / ('Content/' + package[6:] + '.uasset')


def module(name):
    spec = importlib.util.spec_from_file_location('hud_art_' + name, SCRIPT.parent / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def export(obj, folder, name):
    return hud.export(obj, folder / (name + '.t3d'))


def path(obj):
    return obj.get_path_name() if obj else None


def protection(only_assets=None):
    allowed = {disk(value) for value in (only_assets if only_assets is not None else
               list(EXISTING.values()) + list(NEW.values()))}
    inventory = {}
    for top in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for item in (P / top).rglob('*'):
            if item.is_file() and item not in allowed:
                stat = item.stat()
                inventory[item.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    frozen = read(R / 'cpp_frozen.json')
    assert all(sha(P / name) == value for name, value in frozen['files'].items())
    pistol = read(P / 'Saved/Diagnostics/Pistol20261020/Port_U_verify/report.json')
    assert pistol['complete'] and len(pistol['final_sha256']) == 197
    important = [disk(value) for value in list(SOURCE.values()) + list(pistol['final_sha256'])]
    important += list((P / 'Content/DreamCatcher/Screens').rglob('*.uasset'))
    assert len(list((P / 'Content/DreamCatcher/Screens').rglob('*.uasset'))) == 42
    important += [P / name for name in frozen['files']]
    important += [P / name for name in ('.git/HEAD', '.git/index', 'DreamCatcher.uproject',
        'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Content/DreamCatcher/Input/DA_DC_RifleInput.uasset',
        'Content/DreamCatcher/Input/Mappings/IMC_DC_RifleBase.uasset')]
    important += [R / 'cpp_frozen.json', R / 'user_build_success.json']
    if only_assets is not None:
        important += [disk(value) for value in list(NEW.values()) +
                      [EXISTING[key] for key in ('health', 'status', 'mission')]]
    return {'inventory': inventory, 'sha256': {item.as_posix(): sha(item) for item in important}}


def print_commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    result = {}
    for mode in ('inspect', 'apply', 'verify'):
        run = prefix + '_' + mode
        folder = R / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-run=pythonscript',
            '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
            '-EnablePlugins=UMGToolSet', '-nullrhi', '-nosound', '-DCHUDArtMode=' + mode,
            '-DCHUDArtRun=' + run, '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode != 'inspect':
            args.append('-DCHUDArtReference=' + prefix + ('_inspect' if mode == 'apply' else '_apply'))
        result[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': ' '.join('"' + value + '"' if ' ' in value else value for value in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False, 'commands': result}, indent=2))


def copy_struct(value):
    result = type(value)()
    assert result.import_text(value.export_text())
    assert result.export_text() == value.export_text()
    return result


def brush_resource(original, resource):
    result = copy_struct(original)
    result.set_editor_property('ResourceObject', resource)
    assert mission.without_fields(result.export_text(), {'ResourceObject'}) == mission.without_fields(
        original.export_text(), {'ResourceObject'})
    return result


def texture_brush(texture, width, height, margins=None):
    value = ue.WidgetLibrary.make_brush_from_texture(texture, width, height)
    value.set_editor_property('TintColor', ue.SlateColor(specified_color=ue.LinearColor(1, 1, 1, 1)))
    if margins:
        value.set_editor_property('DrawAs', ue.SlateBrushDrawType.BOX)
        value.set_editor_property('Margin', ue.Margin(*margins))
    return value


def graph_state(bp):
    result = {}
    for graph in ue.BlueprintEditorLibrary.list_graphs(bp):
        editor = ue.BlueprintGraphEditor.get_graph_editor(graph)
        assert editor
        nodes = {}
        for node in editor.list_all_nodes():
            item = {'class': node.get_class().get_path_name()}
            if 'K2Node' in node.get_class().get_name():
                item['title'] = ue.BlueprintEditorLibrary.get_node_title(node)
                item['pins'] = {str(pin.get_pin_name()): {
                    'type': str(pin.get_pin_type_display_string()), 'value': pin.get_pin_value(),
                    'links': sorted([other.get_owning_node().get_name(), str(other.get_pin_name())]
                        for other in pin.list_connected_pins())}
                    for pin in ue.BlueprintEditorLibrary.list_all_pins(node)}
            nodes[node.get_name()] = item
        result[graph.get_name()] = nodes
    return result


def snapshot(assets, folder, label):
    result = {}
    for key, asset in assets.items():
        target = folder / (label + '_' + key)
        target.mkdir()
        cdo = asset if key == 'mission' else ue.get_default_object(asset.generated_class())
        item = {'cdo': export(cdo, target, 'cdo')}
        if key != 'mission':
            item['graphs'] = graph_state(asset)
            item['blueprint_export'] = export(asset, target, 'blueprint')
            if key == 'status':
                authored = UMG.call_method('GetWidgets', args=(asset,)).get_editor_property('widgets')
                assert len(authored) == 0, 'Native status wrapper must have no authored widgets'
                widgets = {}
            else:
                _, widgets = hud.tree(UMG, asset)
            item['widgets'] = {name: {'class': widget.get_class().get_path_name(), 'parent': path(widget.get_parent()),
                'raw': export(widget, target, 'widget_' + str(index).zfill(2)),
                'slot': export(widget.get_editor_property('slot'), target, 'slot_' + str(index).zfill(2))
                    if widget.get_editor_property('slot') else None}
                for index, (name, widget) in enumerate(sorted(widgets.items()))}
        if key in ('status', 'mission'):
            item['style'] = cdo.get_editor_property('StatusStyle' if key == 'status' else 'HUDStyle').export_text()
        result[key] = item
    return result


def strip_root(text, fields):
    return re.sub(r'^   (?:' + '|'.join(re.escape(field) for field in fields) +
                  r')=[^\n]*\n?', '', text, flags=re.M)


def validate_preserved(before, after, additions):
    for key in EXISTING:
        cdo_fields = set(BRUSH_VARS) if key == 'row' else ({'StatusStyle'} if key == 'status' else
            ({'HUDStyle'} if key == 'mission' else set()))
        assert pistol.item_subobject_order(strip_root(before[key]['cdo'], cdo_fields)) == pistol.item_subobject_order(
            strip_root(after[key]['cdo'], cdo_fields)), 'Unapproved CDO change: ' + key
        if key in ('status', 'mission'):
            fields = {'ArmorSegmentFillBrushes', 'TextColor'} if key == 'status' else {
                'PanelBrush', 'CheckboxBrush', 'CompletedBrush', 'bHeaderIncludedInPanel'}
            assert mission.without_fields(before[key]['style'], fields) == mission.without_fields(
                after[key]['style'], fields), 'Other style/mission properties changed'
        if key == 'mission':
            continue
        assert set(before[key]['graphs']) == set(after[key]['graphs'])
        for name, old_nodes in before[key]['graphs'].items():
            new_nodes = after[key]['graphs'][name]
            allowed = additions.get('nodes', []) if key == 'row' and name == 'UpdateTotalAmmoLeft' else []
            assert set(new_nodes) == set(old_nodes) | set(allowed), 'Unexpected graph nodes: ' + key + ':' + name
            for node_name, old in old_nodes.items():
                new = json.loads(json.dumps(new_nodes[node_name]))
                old = json.loads(json.dumps(old))
                if allowed:
                    expected_links = additions['links']
                    for pin_name in old.get('pins', {}):
                        token = node_name + '.' + pin_name
                        if token in expected_links:
                            assert new['pins'][pin_name]['links'] == sorted(old['pins'][pin_name]['links'] + expected_links[token])
                            new['pins'][pin_name]['links'] = old['pins'][pin_name]['links']
                assert new == old, 'Original graph changed: ' + key + ':' + name + ':' + node_name
        assert set(before[key]['widgets']) == set(after[key]['widgets'])
        for name, old_widget in before[key]['widgets'].items():
            new_widget = after[key]['widgets'][name]
            assert old_widget['class'] == new_widget['class'] and old_widget['parent'] == new_widget['parent']
            assert old_widget['slot'] == new_widget['slot'], 'Widget geometry changed: ' + key + ':' + name
            fields = {'Background', 'BrushColor'} if key == 'row' and name == 'SelectionBorder' else (
                set(BRUSH_VARS) if key == 'panel' and name in ROWS else (
                {'Brush'} if key == 'health' and name in ('BarFill', 'BarBorder', 'BarGlow') else set()))
            assert pistol.item_subobject_order(strip_root(old_widget['raw'], fields)) == pistol.item_subobject_order(
                strip_root(new_widget['raw'], fields)), 'Other widget properties changed: ' + key + ':' + name
        old_objects, new_objects = status.flat_objects(before[key]['blueprint_export']), status.flat_objects(after[key]['blueprint_export'])
        def animations(objects):
            return {name for name, records in objects.items() if any(
                re.search(r'\bClass=/Script/UMG\.WidgetAnimation(?:\s|$)', value.splitlines()[0]) for value in records)}
        animation_roots = animations(old_objects)
        assert animation_roots == animations(new_objects), 'Widget animation set changed'
        for name, records in old_objects.items():
            if any(name == animation or name.startswith(animation + '.') for animation in animation_roots):
                assert new_objects.get(name) == records, 'Original animation changed: ' + name


def material_parameters(material):
    result = {}
    for kind in ('scalar', 'vector', 'texture', 'static_switch'):
        names = sorted(set(str(value) for value in getattr(MEL, 'get_' + kind + '_parameter_names')(material)))
        values = {}
        for name in names:
            value = getattr(MEL, 'get_material_instance_' + kind + '_parameter_value')(material, name)
            values[name] = value.export_text() if hasattr(value, 'export_text') else (
                path(value) if kind == 'texture' else value)
        result[kind] = values
    return result


def source_material_state(sources, folder):
    root = sources['master']
    result = {'expressions': {}, 'instances': {}}
    for index, expression in enumerate(MEL.get_material_expressions(root)):
        result['expressions'][expression.get_name()] = {'class': expression.get_class().get_path_name(),
            'raw': export(expression, folder, 'source_expression_' + str(index).zfill(3))}
    for key in ('fill', 'border', 'glow'):
        result['instances'][key] = {'parent': path(sources[key].get_editor_property('parent')),
            'parameters': material_parameters(sources[key]), 'raw': export(sources[key], folder, 'source_' + key)}
    result['outputs'] = {kind: [MEL.get_material_property_input_node(root, prop).get_name(),
        MEL.get_material_property_input_node_output_name(root, prop)] for kind, prop in
        (('color', ue.MaterialProperty.MP_EMISSIVE_COLOR), ('alpha', ue.MaterialProperty.MP_OPACITY))}
    assert result['outputs']['color'][0] == 'MaterialExpressionStaticSwitchParameter_2'
    assert result['outputs']['alpha'][0] == 'MaterialExpressionStaticSwitchParameter_1'
    return result


def duplicate(source, key):
    destination = NEW[key]
    assert not disk(destination).exists() and not ue.EditorAssetLibrary.does_asset_exist(destination)
    result = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset(destination.rsplit('/', 1)[1], ART.rstrip('/'), source)
    assert result and result.get_path_name().split('.')[0] == destination
    return result


def add_artwork(root, textures, report):
    expressions = MEL.get_material_expressions(root)
    original_names = {expression.get_name() for expression in expressions}
    uv = [expression for expression in expressions if expression.get_class().get_name() == 'MaterialExpressionMaterialFunctionCall'
          and path(expression.get_editor_property('material_function')) == '/Engine/Functions/UserInterface/GetUserInterfaceUV.GetUserInterfaceUV']
    assert len(uv) == 1 and 'Normalized UV' in MEL.get_material_expression_output_names(uv[0])
    old_outputs = {kind: (MEL.get_material_property_input_node(root, prop),
        MEL.get_material_property_input_node_output_name(root, prop)) for kind, prop in
        (('color', ue.MaterialProperty.MP_EMISSIVE_COLOR), ('alpha', ue.MaterialProperty.MP_OPACITY))}
    created = []
    def node(cls, **properties):
        obj = MEL.create_material_expression(root, getattr(ue, 'MaterialExpression' + cls), 1600, 100 * len(created))
        assert obj
        for name, value in properties.items(): obj.set_editor_property(name, value)
        created.append(obj.get_name())
        return obj
    def connect(start, output, end, input_name):
        assert MEL.connect_material_expressions(start, output, end, input_name)
    rect = node('VectorParameter', parameter_name='SourceArtUVRect', default_value=ue.LinearColor(0, 0, 1, 1))
    offset = node('ComponentMask', r=True, g=True, b=False, a=False)
    scale = node('AppendVector')
    connect(rect, '', offset, ''); connect(rect, 'B', scale, 'A'); connect(rect, 'A', scale, 'B')
    multiply = node('Multiply'); connect(uv[0], 'Normalized UV', multiply, 'A'); connect(scale, '', multiply, 'B')
    add = node('Add'); connect(multiply, '', add, 'A'); connect(offset, '', add, 'B')
    sample = node('TextureSampleParameter2D', parameter_name='ArtworkTexture', texture=textures['HP_Bar'])
    sample_inputs = list(MEL.get_material_expression_input_names(sample))
    assert sample_inputs and sample_inputs[0] in ('UVs', 'Coordinates'), sample_inputs
    connect(add, '', sample, sample_inputs[0])
    for kind, output, prop in (('color', 'RGB', ue.MaterialProperty.MP_EMISSIVE_COLOR),
                               ('alpha', 'A', ue.MaterialProperty.MP_OPACITY)):
        sample_output = output if output in MEL.get_material_expression_output_names(sample) else ''
        assert output != 'A' or sample_output == 'A'
        mul = node('Multiply'); connect(old_outputs[kind][0], old_outputs[kind][1], mul, 'A'); connect(sample, sample_output, mul, 'B')
        switch = node('StaticSwitchParameter', parameter_name='ArtEnabled', default_value=True)
        connect(mul, '', switch, 'True'); connect(old_outputs[kind][0], old_outputs[kind][1], switch, 'False')
        assert MEL.connect_material_property(switch, '', prop)
    assert {expression.get_name() for expression in MEL.get_material_expressions(root)} == original_names | set(created)
    report['new_material_expressions'] = created


def create_materials(sources, textures, report):
    result = {'master': duplicate(sources['master'], 'master')}
    add_artwork(result['master'], textures, report)
    errors = list(MEL.recompile_material(result['master']))
    report['compile_requested'].append(NEW['master'])
    assert not errors, errors
    for key in ('fill', 'border', 'glow', 'left', 'middle', 'right'):
        source_key = key if key in ('fill', 'border', 'glow') else 'fill'
        instance = duplicate(sources[source_key], key)
        MEL.set_material_instance_parent(instance, result['master'] if key == 'fill' else result['fill'])
        artwork = textures['Shield_Bar' if key in UV_RECTS else 'HP_Bar']
        MEL.set_material_instance_texture_parameter_value(instance, 'ArtworkTexture', artwork)
        assert MEL.get_material_instance_texture_parameter_value(instance, 'ArtworkTexture') == artwork
        rect = ue.LinearColor(*(UV_RECTS.get(key, (0, 0, 1, 1))))
        MEL.set_material_instance_vector_parameter_value(instance, 'SourceArtUVRect', rect)
        assert MEL.get_material_instance_vector_parameter_value(instance, 'SourceArtUVRect').export_text() == rect.export_text()
        MEL.set_material_instance_static_switch_parameter_value(instance, 'ArtEnabled', key != 'glow')
        assert MEL.get_material_instance_static_switch_parameter_value(instance, 'ArtEnabled') == (key != 'glow')
        for parameter in WHITE_PARAMS:
            color = MEL.get_material_instance_vector_parameter_value(sources[source_key], parameter) if key == 'glow' else ue.LinearColor(1, 1, 1, 1)
            MEL.set_material_instance_vector_parameter_value(instance, parameter, color)
            assert MEL.get_material_instance_vector_parameter_value(instance, parameter).export_text() == color.export_text()
        MEL.update_material_instance(instance)
        report['compile_requested'].append(NEW[key] + ':UpdateMaterialInstance')
        result[key] = instance
    return result


def canonical_master(text):
    old, new = SOURCE['master'], NEW['master']
    old_name, new_name = old.rsplit('/', 1)[1], new.rsplit('/', 1)[1]
    return text.replace(old + '.' + old_name, new + '.' + new_name).replace(
        "'" + old_name + ':', "'" + new_name + ':').replace("'" + old_name + "'", "'" + new_name + "'")


def validate_materials(materials, source_state, folder, report):
    expressions = {obj.get_name(): obj for obj in MEL.get_material_expressions(materials['master'])}
    old_names = set(source_state['expressions'])
    assert old_names <= expressions.keys()
    assert set(expressions) - old_names == set(report['new_material_expressions'])
    for index, (name, original) in enumerate(sorted(source_state['expressions'].items())):
        obj = expressions[name]
        assert obj.get_class().get_path_name() == original['class']
        assert export(obj, folder, 'preserved_expression_' + str(index).zfill(3)) == canonical_master(original['raw']), 'Original material expression changed: ' + name
    result = {}
    for key in ('fill', 'border', 'glow', 'left', 'middle', 'right'):
        instance = materials[key]
        assert instance.get_editor_property('parent') == materials['master' if key == 'fill' else 'fill']
        actual = material_parameters(instance)
        source_key = key if key in ('fill', 'border', 'glow') else 'fill'
        for kind, parameters in source_state['instances'][source_key]['parameters'].items():
            missing = parameters.keys() - actual[kind].keys()
            if missing:
                # UpdateMaterialInstance refreshes parameter names. This exact
                # legacy zero override has no declaration in the UI master.
                assert kind == 'scalar' and missing == {'RefractionDepthBias'}
                assert parameters['RefractionDepthBias'] == 0
                assert not any('ParameterName="RefractionDepthBias"' in value['raw']
                               for value in source_state['expressions'].values())
                report.setdefault('pruned_unused_zero_parameter_cache', []).append(key + ':RefractionDepthBias')
            for name, old_value in parameters.items():
                if name in missing:
                    continue
                expected = ue.LinearColor(1, 1, 1, 1).export_text() if kind == 'vector' and name in WHITE_PARAMS and key != 'glow' else old_value
                assert actual[kind][name] == expected, 'Original material parameter changed: ' + key + ':' + name
        assert actual['texture']['ArtworkTexture'] == path(TEXTURE_OBJECTS['Shield_Bar' if key in UV_RECTS else 'HP_Bar'])
        assert actual['static_switch']['ArtEnabled'] == (key != 'glow')
        vector = MEL.get_material_instance_vector_parameter_value(instance, 'SourceArtUVRect')
        assert max(abs(a - b) for a, b in zip((vector.r, vector.g, vector.b, vector.a), UV_RECTS.get(key, (0, 0, 1, 1)))) < 1e-6
        result[key] = {'parent': path(instance.get_editor_property('parent')), 'parameters': actual,
                       'raw': export(instance, folder, 'new_' + key)}
    report['material_state'] = result


def compile_widget(bp, report):
    report['compile_requested'].append(bp.get_path_name())
    assert UMG.call_method('CompileWidgetBlueprint', args=(bp,)), 'Widget compile failed'
    assert str(bp.get_editor_property('status')) in ('<BlueprintStatus.BS_UP_TO_DATE: 3>',
        '<BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS: 4>'), str(bp.get_editor_property('status'))


def card_brushes(textures, main):
    names = ('Main_GUN_Card', 'Main_GUN_OFF') if main else ('SUB_GUN_Card', 'Sub_GUN_OFF')
    return [texture_brush(textures[name], 316, height, (8 / 316, 8 / height, 8 / 316, 8 / height))
            for name, height in zip(names, (76, 36))]


def apply_weapons(assets, textures, report):
    bp = assets['row']
    selected, unselected = card_brushes(textures, True)
    editor, nodes = weapon.editor_nodes(bp, 'UpdateTotalAmmoLeft')
    assert nodes['K2Node_IfThenElse_4'] and weapon.pin(nodes['K2Node_IfThenElse_4'], 'Condition').list_connected_pins()[0].get_owning_node() == nodes['K2Node_VariableGet_7']
    struct = ue.load_object(None, '/Script/SlateCore.SlateBrush'); assert struct
    typ = ue.BlueprintEditorLibrary.get_struct_type(struct)
    for name, value in zip(BRUSH_VARS, (selected, unselected)):
        assert editor.add_member_variable(name, typ, value.export_text())
        ue.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, True)
    additions = {'nodes': [], 'links': {}}
    for end_name, getter_name, variable in (('K2Node_CallFunction_24', 'K2Node_VariableGet_15', BRUSH_VARS[0]),
                                           ('K2Node_CallFunction_35', 'K2Node_VariableGet_23', BRUSH_VARS[1])):
        end = weapon.pin(nodes[end_name], 'then'); assert not end.list_connected_pins()
        setter = weapon.call(editor, '/Script/UMG.Border.SetBrush')
        value = editor.add_get_member_variable_node(variable); assert value
        weapon.wire(end, weapon.pin(setter, 'execute'))
        weapon.wire(weapon.pin(nodes[getter_name], 'SelectionBorder'), weapon.pin(setter, 'self'))
        weapon.wire(weapon.pin(value, variable), weapon.pin(setter, 'InBrush'))
        additions['nodes'] += [setter.get_name(), value.get_name()]
        additions['links'][end_name + '.then'] = [[setter.get_name(), 'execute']]
        additions['links'][getter_name + '.SelectionBorder'] = [[setter.get_name(), 'self']]
    _, widgets = hud.tree(UMG, bp)
    widgets['SelectionBorder'].set_brush(selected)
    widgets['SelectionBorder'].set_brush_color(ue.LinearColor(1, 1, 1, 1))
    compile_widget(bp, report)
    # Reacquire row templates after reinstancing the modified row class.
    _, widgets = hud.tree(UMG, assets['panel'])
    for index, name in enumerate(ROWS):
        for variable, value in zip(BRUSH_VARS, card_brushes(textures, index == 0)):
            widgets[name].set_editor_property(variable, value)
    compile_widget(assets['panel'], report)
    return additions


def apply_other(assets, materials, textures, report):
    _, widgets = hud.tree(UMG, assets['health'])
    for widget, material in (('BarFill', 'fill'), ('BarBorder', 'border'), ('BarGlow', 'glow')):
        widgets[widget].set_brush(brush_resource(widgets[widget].get_editor_property('Brush'), materials[material]))
    compile_widget(assets['health'], report)
    cdo = ue.get_default_object(assets['status'].generated_class())
    style = copy_struct(cdo.get_editor_property('StatusStyle'))
    assert not style.get_editor_property('ArmorSegmentFillBrushes')
    style.set_editor_property('ArmorSegmentFillBrushes', [brush_resource(style.get_editor_property('ArmorFillBrush'), materials[key])
        for key in ('left', 'middle', 'right')])
    style.set_editor_property('TextColor', ue.LinearColor(.025, .04, .08, 1))
    cdo.set_editor_property('StatusStyle', style)
    compile_widget(assets['status'], report)
    data = assets['mission']
    style = copy_struct(data.get_editor_property('HUDStyle'))
    assert not style.get_editor_property('bHeaderIncludedInPanel')
    style.set_editor_property('PanelBrush', texture_brush(textures['Mission_UI'], 373, 205,
        (128 / 373, 48 / 205, 16 / 373, 16 / 205)))
    style.set_editor_property('bHeaderIncludedInPanel', True)
    for field, texture in (('CheckboxBrush', 'Not_checked'), ('CompletedBrush', 'checked')):
        style.set_editor_property(field, texture_brush(textures[texture], 20, 20))
    data.set_editor_property('HUDStyle', style)


def validate_bindings(assets, materials, textures):
    _, row = hud.tree(UMG, assets['row'])
    _, panel = hud.tree(UMG, assets['panel'])
    assert row['SizeBox_274'].get_editor_property('WidthOverride') == 352
    assert row['SizeBox_274'].get_editor_property('HeightOverride') == 64
    assert row['SelectionBorder'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert [child.get_name() for child in panel['DCWeaponRows'].get_all_children()] == list(ROWS)
    padding = panel['DCWeaponPanel'].get_editor_property('Padding')
    assert (padding.left, padding.top, padding.right, padding.bottom) == (0, 12, 0, 12)
    assert panel['DCWeaponPanelSize'].get_editor_property('WidthOverride') == 352
    for index, name in enumerate(ROWS):
        assert panel[name].get_editor_property('SlotIndex') == index
        for field, value in zip(BRUSH_VARS, card_brushes(textures, index == 0)):
            assert panel[name].get_editor_property(field).export_text() == value.export_text()
    cdo = ue.get_default_object(assets['row'].generated_class())
    for field, value in zip(BRUSH_VARS, card_brushes(textures, True)):
        assert cdo.get_editor_property(field).export_text() == value.export_text()
    assert row['SelectionBorder'].get_editor_property('Background').export_text() == card_brushes(textures, True)[0].export_text()
    _, widgets = hud.tree(UMG, assets['health'])
    for widget, key in (('BarFill', 'fill'), ('BarBorder', 'border'), ('BarGlow', 'glow')):
        assert ue.WidgetLibrary.get_brush_resource(widgets[widget].get_editor_property('Brush')) == materials[key]
    style = ue.get_default_object(assets['status'].generated_class()).get_editor_property('StatusStyle')
    fills = list(style.get_editor_property('ArmorSegmentFillBrushes'))
    assert len(fills) == 3
    assert [ue.WidgetLibrary.get_brush_resource(value) for value in fills] == [materials[key] for key in ('left', 'middle', 'right')]
    assert style.get_editor_property('TextColor').export_text() == ue.LinearColor(.025, .04, .08, 1).export_text()
    style = assets['mission'].get_editor_property('HUDStyle')
    assert style.get_editor_property('bHeaderIncludedInPanel')
    for field, key in (('PanelBrush', 'Mission_UI'), ('CheckboxBrush', 'Not_checked'), ('CompletedBrush', 'checked')):
        assert ue.WidgetLibrary.get_brush_resource(style.get_editor_property(field)) == textures[key]


def argument(command, name, pattern):
    found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
    assert len(found) == 1, 'Use one explicit -' + name
    return found[0]


def comparable_state(value):
    # Full exports remain in the report and their animation records are checked
    # separately. Authored Graph.Nodes/pins and every widget/CDO are compared;
    # raw Blueprint exports can also include non-authored compiler helper nodes.
    result = json.loads(json.dumps(value))
    for item in result.values():
        item.pop('blueprint_export', None)
        item['cdo'] = pistol.item_subobject_order(item['cdo'])
        for widget in item.get('widgets', {}).values():
            widget['raw'] = pistol.item_subobject_order(widget['raw'])
            if widget['slot']:
                widget['slot'] = pistol.item_subobject_order(widget['slot'])
    return result


def main():
    global ue, hud, pistol, weapon, status, mission, MEL, UMG, TEXTURE_OBJECTS
    import unreal as ue
    hud = module('ingame_hud_layout_setup'); pistol = module('pistol_slot_setup')
    weapon = module('weapon_ammo_hud_setup'); status = module('player_status_hud_setup'); mission = module('mission_hud_setup')
    MEL = ue.MaterialEditingLibrary
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = argument(command, 'DCHUDArtMode', 'inspect|apply|verify')
    run = argument(command, 'DCHUDArtRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run
    folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'
    assert not output.exists(), 'Use a fresh report run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    build = read(R / 'user_build_success.json')
    assert build['user_reported_build_success'] and build['native_schema_verified']
    assert sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll') == build['dll_sha256']
    frozen = read(R / 'cpp_frozen.json')
    if mode != 'verify':
        assert all(sha(P / name) == value for name, value in frozen['protected_existing_assets'].items())
        assert all(not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package) for package in NEW.values()), 'New asset already exists; no overwrite/resume'
    else:
        assert all(disk(package).is_file() for package in NEW.values())
    protected = protection()
    report = {'mode': mode, 'run': run, 'complete': False, 'PIE': False, 'automatic_PIE': False,
        'cpp_build': False, 'asset_saves': [], 'compile_requested': [], 'existing_assets': EXISTING, 'new_assets': NEW,
        'script_sha256': sha(SCRIPT), 'frozen_sha256': sha(R / 'cpp_frozen.json'), 'protection_before': protected,
        'before_sha256': {key: sha(disk(value)) for key, value in EXISTING.items()},
        'runtime_and_visual_verified': False, 'limits': 'User manual play only; no automatic PIE/render/input test.'}
    try:
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'); assert settings_class
        settings_text = export(ue.get_default_object(settings_class), folder, 'blueprint_editor_settings')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_class
        UMG = ue.get_default_object(umg_class)
        assets = {key: ue.load_asset(value) for key, value in EXISTING.items()}
        sources = {key: ue.load_asset(value) for key, value in SOURCE.items()}
        TEXTURE_OBJECTS = {key: ue.load_asset(value) for key, value in TEXTURES.items()}
        assert all(list(assets.values()) + list(sources.values()) + list(TEXTURE_OBJECTS.values()))
        native = ue.get_default_object(ue.load_class(None, '/Script/DreamCatcher.DCMissionListData'))
        assert not native.get_editor_property('HUDStyle').get_editor_property('bHeaderIncludedInPanel')
        native = ue.get_default_object(ue.load_class(None, '/Script/DreamCatcher.DCPlayerStatusWidget'))
        assert not native.get_editor_property('StatusStyle').get_editor_property('ArmorSegmentFillBrushes')
        previous = None
        if mode != 'inspect':
            ref = argument(command, 'DCHUDArtReference', '[A-Za-z0-9_]{1,64}')
            ref_path = R / ref / 'report.json'
            previous = read(ref_path)
            assert previous['complete'] and previous['protection_passed']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == sha(SCRIPT) and previous['frozen_sha256'] == report['frozen_sha256']
            assert previous['protection_before'] == protected
            expected_sha = previous['before_sha256'] if mode == 'apply' else {key: previous['after_sha256'][key] for key in EXISTING}
            assert expected_sha == report['before_sha256']
            report['reference'], report['reference_sha256'] = ref, sha(ref_path)
        report['before'] = snapshot(assets, folder, 'before')
        source_folder = folder / 'source_materials'; source_folder.mkdir()
        source_state = source_material_state(sources, source_folder)
        report['source_materials'] = source_state
        if mode == 'inspect':
            report['complete'] = True
        elif mode == 'apply':
            assert comparable_state(report['before']) == comparable_state(previous['before']), 'Current asset data differs from inspect'
            materials = create_materials(sources, TEXTURE_OBJECTS, report)
            report['weapon_additions'] = apply_weapons(assets, TEXTURE_OBJECTS, report)
            apply_other(assets, materials, TEXTURE_OBJECTS, report)
            report['after'] = snapshot(assets, folder, 'after')
            validate_preserved(report['before'], report['after'], report['weapon_additions'])
            validate_bindings(assets, materials, TEXTURE_OBJECTS)
            material_folder = folder / 'material_validation'; material_folder.mkdir()
            validate_materials(materials, source_state, material_folder, report)
            hud.clean(list(EXISTING.values()) + list(NEW.values()))
            hud.log_check(folder / 'Editor.log')
            assert protection() == protected
            # All twelve assets pass data/compile checks before the first explicit save.
            for key in ('master', 'fill', 'border', 'glow', 'left', 'middle', 'right'):
                hud.assert_no_other_editor()
                assert ue.EditorAssetLibrary.save_loaded_asset(materials[key], only_if_is_dirty=False)
                report['asset_saves'].append(NEW[key])
            for key in ('row', 'panel', 'health', 'status', 'mission'):
                hud.assert_no_other_editor()
                assert ue.EditorAssetLibrary.save_loaded_asset(assets[key], only_if_is_dirty=False)
                report['asset_saves'].append(EXISTING[key])
            report['complete'] = True
        else:
            materials = {key: ue.load_asset(value) for key, value in NEW.items()}
            assert all(materials.values())
            assert comparable_state(report['before']) == comparable_state(previous['after']), 'Fresh saved widget/CDO/graph data differs'
            report['weapon_additions'] = previous['weapon_additions']
            report['new_material_expressions'] = previous['new_material_expressions']
            validate_preserved(previous['before'], report['before'], report['weapon_additions'])
            validate_bindings(assets, materials, TEXTURE_OBJECTS)
            material_folder = folder / 'material_validation'; material_folder.mkdir()
            validate_materials(materials, source_state, material_folder, report)
            assert {key: sha(disk(value)) for key, value in NEW.items()} == {key: previous['after_sha256'][key] for key in NEW}
            report['complete'] = True
        hud.clean()
        hud.log_check(folder / 'Editor.log')
    except Exception:
        report['complete'] = False
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = {key: sha(disk(value)) if disk(value).exists() else None
                                 for key, value in dict(EXISTING, **NEW).items()}
        try:
            report['protection_passed'] = protection() == protected
            assert report['protection_passed'], 'Protected content/source/config/script/Git state changed'
            if mode != 'apply' or not report['asset_saves']:
                assert report['before_sha256'] == {key: report['after_sha256'][key] for key in EXISTING}
        except Exception:
            report['complete'], report['protection_passed'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_HUD_ART_SETUP ' + json.dumps({key: report.get(key) for key in
            ('mode', 'run', 'complete', 'asset_saves', 'error', 'protection_error')}))
        assert report['complete'], 'HUD art run incomplete; inspect report'


SHAPE_REPORTS = R / 'WeaponShape20261022'
SHAPE_TARGETS = {key: EXISTING[key] for key in ('row', 'panel')}
SHAPE_BODY = 'DCWeaponCardBody'
SHAPE_BADGE = 'DCWeaponNumberBadge'
SHAPE_POSITIONS = {14: (40, 34), 16: (92, 44), 21: (106, 44),
                   28: (212, 6), 30: (266, 8), 32: (278, 8)}
SHAPE_MOVED = {'DCWeaponDisplayName': (40, 10, 160, 20), 'WeaponCard': (238, 10, 106, 56),
    'AmmoIcon': (166, 44, 12, 16), 'DCWeaponMagazineFit': (40, 34, 50, 32),
    'DCWeaponSlashFit': (92, 44, 12, 16), 'DCWeaponSpareFit': (106, 44, 50, 16)}


def weapon_shape_commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    commands = {}
    for mode in ('inspect', 'apply', 'verify'):
        run = prefix + '_' + mode
        folder = SHAPE_REPORTS / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-run=pythonscript',
            '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
            '-EnablePlugins=UMGToolSet', '-nullrhi', '-nosound', '-DCHUDWeaponShapeMode=' + mode,
            '-DCHUDWeaponShapeRun=' + run, '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode != 'inspect':
            args.append('-DCHUDWeaponShapeReference=' + prefix + ('_inspect' if mode == 'apply' else '_apply'))
        commands[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': ' '.join('"' + value + '"' if ' ' in value else value for value in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False, 'commands': commands}, indent=2))


def shape_brushes(textures, main_card):
    names = ('Main_GUN_Card', 'Main_GUN_OFF') if main_card else ('SUB_GUN_Card', 'Sub_GUN_OFF')
    values = [texture_brush(textures[name], 316, height) for name, height in zip(names, (76, 36))]
    for value in values:
        value.set_editor_property('DrawAs', ue.SlateBrushDrawType.IMAGE)
        value.set_editor_property('Margin', ue.Margin(0))
        value.set_editor_property('Tiling', ue.SlateBrushTileType.NO_TILE)
    return values


def no_draw_brush():
    value = ue.SlateBrush()
    assert value.import_text('(DrawAs=NoDrawType,ResourceObject=None)')
    return value


def shape_apply(assets, textures, report):
    bp = assets['row']
    _, widgets = hud.tree(UMG, bp)
    assert not ({SHAPE_BODY, SHAPE_BADGE} & widgets.keys()), 'Shape widgets already exist; use verify, no reapply'
    canvas = widgets['DCWeaponRowCanvas']
    body = weapon.add_widget(UMG, bp, ue.Border, SHAPE_BODY, canvas)
    body.set_padding(ue.Margin(0)); body.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    body.set_brush(shape_brushes(textures, True)[0]); body.set_brush_color(ue.LinearColor(1, 1, 1, 1))
    slot = body.get_editor_property('slot')
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(1, 1)))
    slot.set_alignment(ue.Vector2D(0, 0)); slot.set_offsets(ue.Margin(32, 0, 4, 0))
    slot.set_auto_size(False); slot.set_z_order(-1)
    badge = weapon.add_widget(UMG, bp, ue.Border, SHAPE_BADGE, canvas, False)
    badge.set_padding(ue.Margin(0)); badge.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    brush = ue.SlateBrush()
    assert brush.import_text('(DrawAs=RoundedBox,ImageSize=(X=24,Y=24),ResourceObject=None,'
        'TintColor=(SpecifiedColor=(R=0.025,G=0.04,B=0.10,A=1),ColorUseRule=UseColor_Specified),'
        'OutlineSettings=(CornerRadii=(X=2,Y=2,Z=2,W=2),Color=(SpecifiedColor=(R=1,G=1,B=1,A=1),'
        'ColorUseRule=UseColor_Specified),Width=1,RoundingType=FixedRadius))')
    badge.set_brush(brush)
    weapon.canvas_slot(badge, 4, 6, 24, 24)
    weapon.move_widget(UMG, bp, widgets['DCWeaponSlotFit'], badge)
    number_slot = widgets['DCWeaponSlotFit'].get_editor_property('slot')
    number_slot.set_padding(ue.Margin(0)); number_slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    number_slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    widgets['DCWeaponSlotNumber'].set_color_and_opacity(ue.SlateColor(specified_color=ue.LinearColor(1, 1, 1, 1)))
    widgets['SelectionBorder'].set_brush(no_draw_brush())
    widgets['SizeBox_274'].set_height_override(76)
    for name, bounds in SHAPE_MOVED.items(): weapon.canvas_slot(widgets[name], *bounds)
    for name in ('ItemGlow', 'ItemGlow_Boost'): widgets[name].set_visibility(ue.SlateVisibility.COLLAPSED)
    editor, nodes = weapon.editor_nodes(bp, 'UpdateTotalAmmoLeft')
    body_getter = editor.add_get_member_variable_node(SHAPE_BODY); assert body_getter
    glow_getters = {name: editor.add_get_member_variable_node(name) for name in ('ItemGlow', 'ItemGlow_Boost')}
    assert all(glow_getters.values())
    added = [body_getter.get_name()] + [node.get_name() for node in glow_getters.values()]
    hidden = {}
    for setter_name, old_getter in (('K2Node_CallFunction_36', 'K2Node_VariableGet_15'),
                                    ('K2Node_CallFunction_37', 'K2Node_VariableGet_23')):
        setter = nodes[setter_name]
        target = weapon.pin(setter, 'self')
        assert [(pin.get_owning_node().get_name(), str(pin.get_pin_name())) for pin in target.list_connected_pins()] == [(old_getter, 'SelectionBorder')]
        assert target.break_pin_links()
        weapon.wire(weapon.pin(body_getter, SHAPE_BODY), target)
        previous = weapon.pin(setter, 'then'); assert not previous.list_connected_pins()
        hidden[setter_name] = []
        for name, getter in glow_getters.items():
            node = weapon.call(editor, '/Script/UMG.Widget.SetVisibility')
            weapon.wire(previous, weapon.pin(node, 'execute'))
            weapon.wire(weapon.pin(getter, name), weapon.pin(node, 'self'))
            weapon.literal(node, 'InVisibility', 'Collapsed')
            previous = weapon.pin(node, 'then')
            added.append(node.get_name()); hidden[setter_name].append(node.get_name())
    for node, height in ((11, 76), (25, 36)):
        weapon.literal(nodes['K2Node_CallFunction_' + str(node)], 'InHeightOverride', height)
    for node, (x, y) in SHAPE_POSITIONS.items():
        weapon.literal(nodes['K2Node_CallFunction_' + str(node)], 'InPosition', '(X=%.6f,Y=%.6f)' % (x, y))
    compile_widget(bp, report)
    cdo = ue.get_default_object(bp.generated_class())
    for field, value in zip(BRUSH_VARS, shape_brushes(textures, True)): cdo.set_editor_property(field, value)
    _, panel = hud.tree(UMG, assets['panel'])
    for index, name in enumerate(ROWS):
        for field, value in zip(BRUSH_VARS, shape_brushes(textures, index == 0)):
            panel[name].set_editor_property(field, value)
    panel['DCWeaponPanel'].set_padding(ue.Margin(0)); panel['DCWeaponPanel'].set_brush(no_draw_brush())
    # Existing panel graph has only 0/8 row-gap padding. Refuse a new runtime
    # outer-padding writer rather than leaving a hidden 12-pixel override.
    for graph in graph_state(assets['panel']).values():
        for node in graph.values():
            if node.get('title') == 'SetPadding':
                value = node['pins']['InPadding']['value']
                fields = {part.split('=', 1)[0]: float(part.split('=', 1)[1]) for part in mission.split_fields(value)}
                assert fields['Top'] in (0, 8) and all(fields[key] == 0 for key in ('Left', 'Right', 'Bottom'))
    compile_widget(assets['panel'], report)
    return {'nodes': added, 'body_getter': body_getter.get_name(),
        'glow_getters': {name: node.get_name() for name, node in glow_getters.items()}, 'hidden': hidden}


def shape_validate(assets, textures, additions):
    _, widgets = hud.tree(UMG, assets['row'])
    _, panel = hud.tree(UMG, assets['panel'])
    assert widgets['SizeBox_274'].get_editor_property('WidthOverride') == 352
    assert widgets['SizeBox_274'].get_editor_property('HeightOverride') == 76
    assert widgets['SelectionBorder'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert widgets['SelectionBorder'].get_editor_property('Background').export_text() == no_draw_brush().export_text()
    body, badge = widgets[SHAPE_BODY], widgets[SHAPE_BADGE]
    assert body.get_parent() == widgets['DCWeaponRowCanvas'] and badge.get_parent() == widgets['DCWeaponRowCanvas']
    slot = body.get_editor_property('slot')
    anchors = slot.get_anchors()
    assert (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y) == (0, 0, 1, 1)
    offsets = slot.get_offsets()
    assert (offsets.left, offsets.top, offsets.right, offsets.bottom) == (32, 0, 4, 0)
    assert slot.get_z_order() == -1 and body.get_visibility() == ue.SlateVisibility.HIT_TEST_INVISIBLE
    assert body.get_editor_property('Background').export_text() == shape_brushes(textures, True)[0].export_text()
    badge_slot = badge.get_editor_property('slot')
    assert (badge_slot.get_position().x, badge_slot.get_position().y, badge_slot.get_size().x, badge_slot.get_size().y) == (4, 6, 24, 24)
    assert widgets['DCWeaponSlotFit'].get_parent() == badge
    assert widgets['DCWeaponSlotNumber'].get_editor_property('ColorAndOpacity').export_text() == ue.SlateColor(specified_color=ue.LinearColor(1, 1, 1, 1)).export_text()
    for name, expected in SHAPE_MOVED.items():
        current = widgets[name].get_editor_property('slot')
        assert (current.get_position().x, current.get_position().y, current.get_size().x, current.get_size().y) == expected
    for name in ('ItemGlow', 'ItemGlow_Boost'): assert widgets[name].get_visibility() == ue.SlateVisibility.COLLAPSED
    cdo = ue.get_default_object(assets['row'].generated_class())
    for field, expected in zip(BRUSH_VARS, shape_brushes(textures, True)):
        assert cdo.get_editor_property(field).export_text() == expected.export_text()
    for index, name in enumerate(ROWS):
        assert panel[name].get_editor_property('SlotIndex') == index
        for field, expected in zip(BRUSH_VARS, shape_brushes(textures, index == 0)):
            assert panel[name].get_editor_property(field).export_text() == expected.export_text()
    padding = panel['DCWeaponPanel'].get_editor_property('Padding')
    assert (padding.left, padding.top, padding.right, padding.bottom) == (0, 0, 0, 0)
    assert panel['DCWeaponPanel'].get_editor_property('Background').export_text() == no_draw_brush().export_text()
    assert panel['DCWeaponPanelSize'].get_editor_property('WidthOverride') == 352
    assert [child.get_name() for child in panel['DCWeaponRows'].get_all_children()] == list(ROWS)
    graph = graph_state(assets['row'])['UpdateTotalAmmoLeft']
    for name, height in ((11, 76), (25, 36)):
        assert float(graph['K2Node_CallFunction_' + str(name)]['pins']['InHeightOverride']['value']) == height
    for number, position in SHAPE_POSITIONS.items():
        value = graph['K2Node_CallFunction_' + str(number)]['pins']['InPosition']['value']
        fields = dict(part.split('=', 1) for part in mission.split_fields(value))
        assert (float(fields['X']), float(fields['Y'])) == position
    for setter_name in ('K2Node_CallFunction_36', 'K2Node_CallFunction_37'):
        assert graph[setter_name]['pins']['self']['links'] == [[additions['body_getter'], SHAPE_BODY]]
        expected_brush = 'K2Node_VariableGet_31' if setter_name.endswith('_36') else 'K2Node_VariableGet_32'
        assert graph[setter_name]['pins']['InBrush']['links'] == [[expected_brush, BRUSH_VARS[0 if setter_name.endswith('_36') else 1]]]
        previous = setter_name
        for widget, node_name in zip(('ItemGlow', 'ItemGlow_Boost'), additions['hidden'][setter_name]):
            assert graph[previous]['pins']['then']['links'] == [[node_name, 'execute']]
            assert graph[node_name]['title'] == 'SetVisibility'
            assert graph[node_name]['pins']['InVisibility']['value'] == 'Collapsed'
            assert graph[node_name]['pins']['self']['links'] == [[additions['glow_getters'][widget], widget]]
            previous = node_name
        assert not graph[previous]['pins']['then']['links']
    for name, visibility in (('K2Node_VariableGet_15', 'K2Node_CallFunction_24'),
                             ('K2Node_VariableGet_23', 'K2Node_CallFunction_35')):
        assert graph[name]['pins']['SelectionBorder']['links'] == [[visibility, 'self']]


def shape_preserved(before, after, additions):
    for key in SHAPE_TARGETS:
        fields = set(BRUSH_VARS) | {SHAPE_BODY, SHAPE_BADGE} if key == 'row' else set()
        assert pistol.item_subobject_order(strip_root(before[key]['cdo'], fields)) == pistol.item_subobject_order(strip_root(after[key]['cdo'], fields)), 'Other CDO properties changed'
        assert before[key]['graphs'].keys() == after[key]['graphs'].keys()
        for name, old_nodes in before[key]['graphs'].items():
            new_nodes = after[key]['graphs'][name]
            active = key == 'row' and name == 'UpdateTotalAmmoLeft'
            assert set(new_nodes) == set(old_nodes) | (set(additions['nodes']) if active else set())
            for node_name, old in old_nodes.items():
                old, new = json.loads(json.dumps(old)), json.loads(json.dumps(new_nodes[node_name]))
                if active:
                    changing_values = {('K2Node_CallFunction_' + str(number), 'InPosition') for number in SHAPE_POSITIONS}
                    changing_values |= {('K2Node_CallFunction_11', 'InHeightOverride'), ('K2Node_CallFunction_25', 'InHeightOverride')}
                    changing_links = {('K2Node_CallFunction_36', 'self'), ('K2Node_CallFunction_37', 'self'),
                        ('K2Node_CallFunction_36', 'then'), ('K2Node_CallFunction_37', 'then'),
                        ('K2Node_VariableGet_15', 'SelectionBorder'), ('K2Node_VariableGet_23', 'SelectionBorder')}
                    for pin_name in old.get('pins', {}):
                        if (node_name, pin_name) in changing_values: new['pins'][pin_name]['value'] = old['pins'][pin_name]['value']
                        if (node_name, pin_name) in changing_links: new['pins'][pin_name]['links'] = old['pins'][pin_name]['links']
                assert old == new, 'Functional graph changed: ' + key + ':' + name + ':' + node_name
        old_widgets, new_widgets = before[key]['widgets'], after[key]['widgets']
        assert set(new_widgets) == set(old_widgets) | ({SHAPE_BODY, SHAPE_BADGE} if key == 'row' else set())
        for name, original in old_widgets.items():
            actual = new_widgets[name]
            assert original['class'] == actual['class']
            if not (key == 'row' and name == 'DCWeaponSlotFit'): assert original['parent'] == actual['parent']
            raw_fields = set()
            if key == 'row':
                if name == 'SelectionBorder': raw_fields = {'Background'}
                elif name == 'SizeBox_274': raw_fields = {'HeightOverride'}
                elif name == 'DCWeaponSlotFit': raw_fields = {'Slot'}
                elif name == 'DCWeaponSlotNumber': raw_fields = {'ColorAndOpacity'}
                elif name in ('ItemGlow', 'ItemGlow_Boost'): raw_fields = {'Visibility'}
            elif name in ROWS: raw_fields = set(BRUSH_VARS)
            elif name == 'DCWeaponPanel': raw_fields = {'Padding', 'Background'}
            if key == 'row' and name == 'DCWeaponRowCanvas':
                def own(text):
                    objects = status.flat_objects(text)
                    return re.sub(r'^\s*Slots\(\d+\)=[^\n]*\n?', '', objects[min(objects, key=len)][0], flags=re.M)
                assert own(original['raw']) == own(actual['raw'])
            else:
                assert pistol.item_subobject_order(status.strip_fields(original['raw'], raw_fields)) == pistol.item_subobject_order(status.strip_fields(actual['raw'], raw_fields)), 'Other widget property changed: ' + name
            if key == 'row' and name == 'DCWeaponSlotFit':
                continue  # The new fixed 24px badge owns this existing ScaleBox.
            if key == 'row' and name in SHAPE_MOVED:
                assert strip_root(original['slot'], {'LayoutData'}) == strip_root(actual['slot'], {'LayoutData'})
            else:
                assert original['slot'] == actual['slot'], 'Other widget slot changed: ' + name
        old_objects = status.flat_objects(before[key]['blueprint_export'])
        new_objects = status.flat_objects(after[key]['blueprint_export'])
        roots = {name for name, records in old_objects.items() if any(re.search(r'\bClass=/Script/UMG\.WidgetAnimation(?:\s|$)', record.splitlines()[0]) for record in records)}
        for name, records in old_objects.items():
            if any(name == root or name.startswith(root + '.') for root in roots):
                assert new_objects.get(name) == records, 'Original animation data changed'


def weapon_shape_main():
    global ue, hud, pistol, weapon, status, mission, UMG
    import unreal as ue
    hud = module('ingame_hud_layout_setup'); pistol = module('pistol_slot_setup')
    weapon = module('weapon_ammo_hud_setup'); status = module('player_status_hud_setup'); mission = module('mission_hud_setup')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = argument(command, 'DCHUDWeaponShapeMode', 'inspect|apply|verify')
    run = argument(command, 'DCHUDWeaponShapeRun', '[A-Za-z0-9_]{1,64}')
    folder = SHAPE_REPORTS / run; folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'; assert not output.exists(), 'Use a fresh shape run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    baseline_path = R / 'HUDArt_D_verify/report.json'
    baseline = read(baseline_path)
    assert baseline['complete'] and baseline['protection_passed']
    protected = protection(list(SHAPE_TARGETS.values()))
    current_hashes = {key: sha(disk(package)) for key, package in SHAPE_TARGETS.items()}
    if mode != 'verify': assert current_hashes == {key: baseline['after_sha256'][key] for key in SHAPE_TARGETS}, 'Current pair differs from reviewed D state'
    report = {'mode': 'weapon_shape_' + mode, 'run': run, 'complete': False, 'PIE': False,
        'automatic_PIE': False, 'asset_saves': [], 'compile_requested': [], 'allowed_assets': SHAPE_TARGETS,
        'script_sha256': sha(SCRIPT), 'reviewed_D_sha256': sha(baseline_path), 'protection_before': protected,
        'before_sha256': current_hashes, 'layout_contract': {'outer_width': 352, 'body_x': 32, 'body_width': 316,
            'active_height': 76, 'inactive_height': 36, 'row_gap': 8, 'panel_padding': 0, 'two_rows_height': 120,
            'number_badge': [4, 6, 24, 24], 'three_populated_rows_height': 164,
            'DPI': 'Existing viewport scaling; native-size artwork in Slate units, no texture resize or nine-slice.'},
        'limits': 'No PIE/render/input. User manual shape, icon, ammo, switching and resolution confirmation pending.'}
    try:
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'); assert settings_class
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', export(ue.get_default_object(settings_class), folder, 'settings'), re.M) == ['SoC_Never']
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_class
        UMG = ue.get_default_object(umg_class)
        assets = {key: ue.load_asset(package) for key, package in SHAPE_TARGETS.items()}; assert all(assets.values())
        textures = {key: ue.load_asset(TEXTURES[key]) for key in ('Main_GUN_Card', 'Main_GUN_OFF', 'SUB_GUN_Card', 'Sub_GUN_OFF')}
        assert all(textures.values())
        report['before'] = snapshot(assets, folder, 'before')
        reference = None
        if mode != 'inspect':
            name = argument(command, 'DCHUDWeaponShapeReference', '[A-Za-z0-9_]{1,64}')
            reference_path = SHAPE_REPORTS / name / 'report.json'; reference = read(reference_path)
            assert reference['complete'] and reference['protection_passed']
            assert reference['mode'] == ('weapon_shape_inspect' if mode == 'apply' else 'weapon_shape_apply')
            assert reference['script_sha256'] == sha(SCRIPT) and reference['reviewed_D_sha256'] == sha(baseline_path)
            assert reference['protection_before'] == protected
            expected_hashes = reference['before_sha256'] if mode == 'apply' else reference['after_sha256']
            assert expected_hashes == current_hashes
            report['reference'], report['reference_sha256'] = name, sha(reference_path)
        if mode == 'inspect':
            expected = {key: baseline['before'][key] for key in SHAPE_TARGETS}
            assert comparable_state(report['before']) == comparable_state(expected)
            _, widgets = hud.tree(UMG, assets['row'])
            assert not ({SHAPE_BODY, SHAPE_BADGE} & widgets.keys())
        elif mode == 'apply':
            assert comparable_state(report['before']) == comparable_state(reference['before'])
            report['additions'] = shape_apply(assets, textures, report)
            report['after'] = snapshot(assets, folder, 'after')
            shape_preserved(report['before'], report['after'], report['additions'])
            shape_validate(assets, textures, report['additions'])
            hud.clean(list(SHAPE_TARGETS.values())); hud.log_check(folder / 'Editor.log')
            assert protection(list(SHAPE_TARGETS.values())) == protected
            for key in ('row', 'panel'):
                hud.assert_no_other_editor()
                assert ue.EditorAssetLibrary.save_loaded_asset(assets[key], only_if_is_dirty=False)
                report['asset_saves'].append(SHAPE_TARGETS[key])
        else:
            report['additions'] = reference['additions']
            assert comparable_state(report['before']) == comparable_state(reference['after']), 'Fresh saved shape differs'
            shape_preserved(reference['before'], report['before'], report['additions'])
            shape_validate(assets, textures, report['additions'])
        hud.clean(); hud.log_check(folder / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = {key: sha(disk(package)) for key, package in SHAPE_TARGETS.items()}
        try:
            report['protection_passed'] = protection(list(SHAPE_TARGETS.values())) == protected
            assert report['protection_passed']
            if mode != 'apply' or not report['asset_saves']: assert report['after_sha256'] == report['before_sha256']
        except Exception:
            report['complete'], report['protection_passed'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_WEAPON_SHAPE ' + json.dumps({key: report.get(key) for key in ('mode', 'run', 'complete', 'asset_saves', 'error', 'protection_error')}))
        assert report['complete'], 'Weapon shape correction incomplete; inspect report'


if __name__ == '__main__':
    if '--weapon-shape-commands' in sys.argv:
        weapon_shape_commands(sys.argv[sys.argv.index('--weapon-shape-commands') + 1])
    elif '--commands' in sys.argv:
        print_commands(sys.argv[sys.argv.index('--commands') + 1])
    else:
        import unreal as entry_unreal
        if '-DCHUDWeaponShapeMode=' in entry_unreal.SystemLibrary.get_command_line():
            weapon_shape_main()
        else:
            main()
