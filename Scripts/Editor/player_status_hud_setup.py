"""Approved TODO6 exact-four-package setup after the user's native build.

Separate UnrealEditor-Cmd -run=pythonscript -unattended -EnablePlugins=UMGToolSet
-SCCProvider=None -DCPlayerStatusSetupMode=inspect|apply|verify
-DCPlayerStatusSetupRun=<fresh> -DCPlayerStatusSetupReference=<inspect/apply run>
-abslog=<project>/Saved/Diagnostics/PlayerStatus20261020/<fresh>/Editor.log
No C++ build, PIE, map/material save, binary copy or unknown-outcome overwrite.
"""
import importlib.util
import json
import os
from pathlib import Path
import re
import sys
import traceback

import unreal as ue

sys.dont_write_bytecode = True
PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/PlayerStatus20261020'
ASSETS = {
    'health': '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_Healthbar',
    'hud': '/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout',
    'experience': '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience',
    'wrapper': '/Game/DreamCatcher/UI/HUD/W_DC_PlayerStatus',
}
EXPECTED_BEFORE = {
    'health': '90e011b8cbdee6de68a63b789f283799449000399a7b2b61c2b96ebdf283c0a3',
    'hud': '85d2bafb914f28abab5ccb9ac326e91aca73bfdeafce8f855f2cbaf9769c95cd',
    'experience': '724c3e5917b1c300dc5e2acf42f304a23cdaf146ecb2de64a79ab9336a67551b',
}
STYLE_VALUES = dict(BarWidth=440., ArmorHeight=24., ArmorHealthGap=8., HealthHeight=32.,
    BottomMargin=72., HorizontalOffset=0., ArmorSegmentGap=8., StarSize=80., StarGap=40.,
    StarIconScale=.6, FontSize=18, ReadyGlowAlpha=.3, InactiveGlowAlpha=0., RefreshInterval=.05)
COLORS = dict(TextColor=(1, 1, 1, 1), ArmorColor=(1, 1, 1, 1), ReadyColor=(1, 1, 1, 1),
    ActiveColor=(.6, .85, 1, 1), CooldownColor=(.35, .35, .35, 1), UnavailableColor=(.15, .15, .15, 1))
EXPERIENCE_VALUES = dict(bUsePlayerStatusDefaults=True, PlayerMaxHealth=100., PlayerMaxArmor=150., PlayerInitialArmor=150.)
STAR = '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/Art/MI_UI_AccoladeStar'
RING = '/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/HUD/M_UI_RadialProgress_Ability'
MAX_FUNCTION = '/Script/DreamCatcher.DCPlayerStatusWidget.GetSafeMaxHealth'
DENOMINATORS = {'EventGraph': ('K2Node_PromotableOperator_1', 'K2Node_PromotableOperator_2'),
                'InitializeBarVisuals': ('K2Node_PromotableOperator_0',)}
NEW_WIDGETS = ('DCHealthTextRow', 'DCMaxHealthText')
TOLERANCE_PATHS = tuple(ASSETS['health'] + '.W_Healthbar:EventGraph.K2Node_PromotableOperator_' + str(index)
                       for index in (3, 4, 6, 9))
EMPTY_TOLERANCE_TAIL = (',PinName="ErrorTolerance",PinType.PinCategory="",PinType.PinSubCategory="",'
    'PinType.PinSubCategoryObject=None,PinType.PinSubCategoryMemberReference=(),PinType.PinValueType=(),'
    'PinType.ContainerType=None,PinType.bIsReference=False,PinType.bIsConst=False,PinType.bIsWeakPointer=False,'
    'PinType.bIsUObjectWrapper=False,PinType.bSerializeAsSinglePrecisionFloat=False,'
    'PersistentGuid=00000000000000000000000000000000,bHidden=True,bNotConnectable=False,'
    'bDefaultValueIsReadOnly=False,bDefaultValueIsIgnored=False,bAdvancedView=False,bOrphanedPin=False,)')
# These compiler-only graphs are present in the reviewed original A61 export.
# BlueprintEditorLibrary.ListGraphs filters RF_Transient (cpp:417+); preserve
# every graph it returns. Unknown unlisted graph names are refused.
COMPILER_NAMES = {
    'health': {'EdGraph_0', 'EdGraph_1', 'ExecuteUbergraph_W_Healthbar', 'EventOnEliminated',
               'UpdateHealthbar', 'OnPosessedPawnChanged', 'Construct', 'Health Changed',
               'SetDynamicMaterials_MERGED', 'InitializeBarVisuals_MERGED', 'ResetAnimatedState_MERGED'},
    'hud': {'ExecuteUbergraph_W_ShooterHUDLayout'}, 'experience': set(),
    'wrapper': {'ExecuteUbergraph_W_DC_PlayerStatus'},
}
spec = importlib.util.spec_from_file_location('dc_player_status_evidence', PROJECT / 'Scripts/Editor/ingame_hud_layout_setup.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def disk(key):
    return PROJECT / ('Content/' + ASSETS[key][6:] + '.uasset')


def inventory():
    result = hud.inventory()
    for key in ASSETS:
        result.pop(str(disk(key).relative_to(PROJECT)), None)
    return result


def hashes():
    result = hud.protected_hashes()
    for key in ASSETS:
        result.pop(str(disk(key).relative_to(PROJECT)).replace('\\', '/'), None)
    # Preserve bytes of the submitted map, boss/Haru/SMG/rifle source art and all
    # text/configuration files, in addition to the full-scope size/time inventory.
    for group in ('Source', 'Config', 'Scripts', 'docs'):
        for path in (PROJECT / group).rglob('*'):
            if path.is_file(): result[str(path.relative_to(PROJECT))] = hud.sha(path)
    for path in (PROJECT / 'Content').rglob('*'):
        if path.is_file() and path not in [disk(key) for key in ASSETS] and (
            path.suffix == '.umap' or any(word in path.as_posix().lower() for word in (
                '/boss/', '/haru', '/smg', '/weapons/rifle/', '/damagetaken/', '/hud/art/', '/crosshair/', '/foundation/fonts/'))):
            result[str(path.relative_to(PROJECT))] = hud.sha(path)
    for asset in (STAR, RING):
        path = PROJECT / ('Content/' + asset[6:] + '.uasset')
        result[str(path.relative_to(PROJECT))] = hud.sha(path)
    return result


def package_hashes():
    return {key: hud.sha(disk(key)) if disk(key).is_file() else None for key in ASSETS}


def object_path(package):
    return package + '.' + package.rsplit('/', 1)[1]


def flat_objects(text):
    """Exact own-property records; child objects remain independent records."""
    stack, rows = [], {}
    for line in text.splitlines(keepends=True):
        if line.strip().startswith('Begin Object '):
            found = re.findall(r'\bExportPath="[^\']*\'([^\']+)\'"', line)
            assert len(found) == 1, 'Unidentified exported object'
            stack.append((found[0], [line]))
        elif line.strip() == 'End Object':
            assert stack
            path, own = stack.pop()
            own.append(line)
            rows.setdefault(path, []).append(''.join(own))
        else:
            assert stack or not line.strip(), 'Text outside object export'
            if stack: stack[-1][1].append(line)
    assert not stack
    return rows


def strip_fields(text, names):
    return ''.join(line for line in text.splitlines(keepends=True)
                   if not any(re.match(r'^\s*' + re.escape(name) + '=', line) for name in names))


def strip_guid_entries(text, names):
    output = []
    for line in text.splitlines(keepends=True):
        if line.strip().startswith('WidgetVariableNameToGuidMap='):
            for name in names:
                pattern = r'\("' + re.escape(name) + r'", [0-9A-F]{32}\)'
                assert len(re.findall(pattern, line)) <= 1
                line, count = re.subn(',' + pattern, '', line)
                if not count: line, count = re.subn(pattern + ',', '', line)
        output.append(line)
    return ''.join(output)


def strip_pin_links(text, names):
    output = []
    for line in text.splitlines(keepends=True):
        if line.strip().startswith('CustomProperties Pin '):
            found = re.search(r'\bPinName="([^\"]+)"', line)
            assert found
            if found[1] in names:
                line = re.sub(r'LinkedTo=\([^)]*\),', '', line)
        output.append(line)
    return ''.join(output)


def pin(node, name):
    values = [value for value in ue.BlueprintEditorLibrary.list_all_pins(node) if str(value.get_pin_name()) == name]
    assert len(values) == 1, (node.get_name(), name)
    return values[0]


def links(value):
    return sorted((other.get_owning_node().get_name(), str(other.get_pin_name())) for other in value.list_connected_pins())


def tree(umg, bp):
    rows = list(umg.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets'))
    names = [str(row.get_editor_property('widget_name')) for row in rows]
    assert len(names) == len(set(names))
    return rows, {name: row.get_editor_property('widget') for name, row in zip(names, rows)}


def snapshot(key, bp, umg, directory, label):
    folder = directory / (label + '_' + key)
    folder.mkdir()
    full = hud.export(bp, folder / 'blueprint.t3d')
    cdo = hud.export(ue.get_default_object(bp.generated_class()), folder / 'cdo.t3d')
    records = flat_objects(full)
    authored, compiler = {}, {}
    for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp)):
        authored[graph.get_path_name()] = hud.export(graph, folder / ('graph_%02d.t3d' % index))
    for path, values in records.items():
        if any('Begin Object Class=/Script/Engine.EdGraph ' in value for value in values) and path not in authored:
            assert path.rsplit(':', 1)[-1] in COMPILER_NAMES[key], 'Unreviewed compiler graph: ' + path
            compiler[path] = values
    widgets = tree(umg, bp)[1] if key != 'experience' else {}
    hierarchy, widget_records = {}, {}
    for index, (name, widget) in enumerate(widgets.items()):
        parent, slot = widget.get_parent(), widget.get_editor_property('slot')
        hierarchy[name] = dict(path=widget.get_path_name(), cls=widget.get_class().get_path_name(),
            parent=parent.get_name() if parent else None, slot=slot.get_path_name() if slot else None)
        widget_records[name] = hud.export(widget, folder / ('widget_%02d.t3d' % index))
        if slot: hud.export(slot, folder / ('slot_%02d.t3d' % index))
    return dict(parent=ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
        cdo_path=ue.get_default_object(bp.generated_class()).get_path_name(),
        records=records, cdo=cdo, authored=authored, compiler=compiler,
        hierarchy=hierarchy, widgets=widget_records, full_raw=full)


def canonical_hidden_tolerance(path, text):
    """Only four reconstructed, empty/hidden/unlinked tolerance pin IDs.

    K2Node_PromotableOperator.cpp:621-636 calls UpdatePinsFromFunction; 1430-1439
    creates a missing comparison tolerance pin even when its type is empty, then
    hides it if the function has no tolerance parameter. EdGraphPin.cpp:510 gives
    a newly created pin FGuid::NewGuid(). All other GUIDs/properties stay exact.
    """
    if path not in TOLERANCE_PATHS:
        return text
    result, found, has_pins = [], 0, False
    pattern = r'CustomProperties Pin \(PinId=([0-9A-F]{32})' + re.escape(EMPTY_TOLERANCE_TAIL)
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('CustomProperties Pin '):
            has_pins = True
            if 'PinName="ErrorTolerance"' in stripped:
                match = re.fullmatch(pattern, stripped)
                assert match, 'Changed hidden tolerance pin shape: ' + path
                found += 1
                line = line.replace('PinId=' + match[1] + ',', 'PinId=<reconstructed-hidden-tolerance>,', 1)
        result.append(line)
    assert found == (1 if has_pins else 0), 'Missing/duplicate hidden tolerance pin: ' + path
    return ''.join(result)


def canonical_denominator_defaults(path, text):
    allowed = {object_path(ASSETS['health']) + ':' + graph + '.' + node
               for graph, nodes in DENOMINATORS.items() for node in nodes}
    if path not in allowed:
        return text
    output = []
    for line in text.splitlines(keepends=True):
        if line.strip().startswith('CustomProperties Pin ') and 'PinName="B"' in line:
            for name, value in re.findall(r',((?:Autogenerated)?DefaultValue)="([^\"]*)",', line):
                assert value == '' or (name == 'DefaultValue' and value == '100.000000'), 'Unexpected denominator default'
            line = re.sub(r',(?:Autogenerated)?DefaultValue="[^\"]*"(?=,)', '', line)
        output.append(line)
    return ''.join(output)


def canonical_health_variable_type(text):
    """The replaced HUD member keeps its name/GUID and changes only its class."""
    old = '"/Script/UMG.WidgetBlueprintGeneratedClass\'' + object_path(ASSETS['health']) + '_C\'"'
    new = '"/Script/UMG.WidgetBlueprintGeneratedClass\'' + object_path(ASSETS['wrapper']) + '_C\'"'
    output, found = [], 0
    for line in text.splitlines(keepends=True):
        if re.match(r'^\s*GeneratedVariables\([0-9]+\)=', line) and 'VarName="BW_Healthbar"' in line:
            found += 1
            assert 'VarGuid=0441E4B59E2C31CBA254FB3B4DE790D9,' in line, 'Replaced HUD variable GUID changed'
            assert line.count('VarName="BW_Healthbar"') == 1
            old_field, new_field = 'PinSubCategoryObject=' + old, 'PinSubCategoryObject=' + new
            assert line.count(old_field) + line.count(new_field) == 1
            assert 'VarType=(PinCategory="object",' in line
            line = line.replace(new_field, old_field, 1)
        output.append(line)
    assert found <= 1, 'Duplicate BW_Healthbar generated variable'
    return ''.join(output)


def persistent_records(snapshot):
    transient = tuple(snapshot['compiler'])
    return {path: [canonical_denominator_defaults(path, canonical_hidden_tolerance(path, value)) for value in values] for path, values in snapshot['records'].items()
            if not any(path == graph or path.startswith(graph + '.') for graph in transient)}


def unchanged_snapshot(before, after):
    assert before['parent'] == after['parent'] and before['hierarchy'] == after['hierarchy']
    assert before['cdo_path'] == after['cdo_path']
    assert before['cdo'] == after['cdo']
    assert set(before['authored']) == set(after['authored'])
    assert persistent_records(before) == persistent_records(after), 'Persistent object/pin/default/GUID data changed'


def common_hud(widgets):
    assert {'BW_Healthbar', 'VerticalBox_0', 'Radar', 'CanvasPanel_0', 'ExtensionPoint_ModeStatus', 'ExtensionPoint_Reticle'} <= set(widgets)
    slot = widgets['VerticalBox_0'].get_editor_property('slot')
    assert (slot.get_position().x, slot.get_position().y) == (-64, -48)
    radar = widgets['Radar'].get_editor_property('slot')
    assert (radar.get_position().x, radar.get_position().y, radar.get_size().x, radar.get_size().y) == (64, 48, 240, 240)
    assert widgets['BW_Healthbar'].get_parent() == widgets['CanvasPanel_0']


def baseline_health(widgets):
    required = {'AnimBoundSB', 'Overlay_0', 'Overlay_3', 'NumberBorderSB', 'HealthNumber', 'BarBorder', 'BarFill'}
    assert required <= set(widgets) and not set(NEW_WIDGETS).intersection(widgets)
    assert widgets['AnimBoundSB'].get_parent() is None
    assert widgets['HealthNumber'].get_parent() == widgets['Overlay_3']
    assert widgets['AnimBoundSB'].get_editor_property('WidthOverride') == 456
    assert widgets['AnimBoundSB'].get_editor_property('HeightOverride') == 32


def graph_contract(bp, changed=None):
    result = {}
    for graph_name, denominators in DENOMINATORS.items():
        editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, graph_name)
        assert editor
        nodes = {node.get_name(): node for node in editor.list_all_nodes()}
        for name in denominators:
            assert name in nodes
            value = pin(nodes[name], 'B')
            if changed is None:
                assert float(value.get_pin_value()) == 100
                assert not links(value)
        if graph_name == 'EventGraph':
            event = nodes.get('K2Node_CustomEvent_0')
            assert event
            if changed is None: assert not links(pin(event, 'HealthComponent'))
        result[graph_name] = dict(editor=editor, nodes=nodes)
    return result


def edit_graphs(bp, graph_data):
    names = {}
    event = graph_data['EventGraph']
    safe = event['editor'].add_call_function_node(MAX_FUNCTION)
    assert safe
    assert pin(event['nodes']['K2Node_CustomEvent_0'], 'HealthComponent').try_create_connection(pin(safe, 'HealthComponent'))
    for name in DENOMINATORS['EventGraph']:
        assert pin(safe, 'ReturnValue').try_create_connection(pin(event['nodes'][name], 'B'))
    names['EventGraph'] = [safe.get_name()]
    init = graph_data['InitializeBarVisuals']
    pawn = init['editor'].add_call_function_node('/Script/UMG.UserWidget.GetOwningPlayerPawn')
    health = init['editor'].add_call_function_node('/Script/DreamCatcher.DCLyraHealthComponent.FindHealthComponent')
    safe_init = init['editor'].add_call_function_node(MAX_FUNCTION)
    assert pawn and health and safe_init
    assert pin(pawn, 'ReturnValue').try_create_connection(pin(health, 'Actor'))
    assert pin(health, 'ReturnValue').try_create_connection(pin(safe_init, 'HealthComponent'))
    assert pin(safe_init, 'ReturnValue').try_create_connection(pin(init['nodes']['K2Node_PromotableOperator_0'], 'B'))
    names['InitializeBarVisuals'] = [pawn.get_name(), health.get_name(), safe_init.get_name()]
    check_graphs(bp, names)
    return names


def check_graphs(bp, names):
    data = graph_contract(bp, names)
    event, init = data['EventGraph']['nodes'], data['InitializeBarVisuals']['nodes']
    safe = event[names['EventGraph'][0]]
    assert links(pin(event['K2Node_CustomEvent_0'], 'HealthComponent')) == [(safe.get_name(), 'HealthComponent')]
    assert links(pin(safe, 'ReturnValue')) == sorted((name, 'B') for name in DENOMINATORS['EventGraph'])
    pawn, health, safe_init = [init[name] for name in names['InitializeBarVisuals']]
    for first, first_pin, last, last_pin in ((pawn, 'ReturnValue', health, 'Actor'),
        (health, 'ReturnValue', safe_init, 'HealthComponent'),
        (safe_init, 'ReturnValue', init['K2Node_PromotableOperator_0'], 'B')):
        assert links(pin(first, first_pin)) == [(last.get_name(), last_pin)]


def source_appearance(health_widgets):
    brushes = {name: health_widgets[source].get_editor_property('Brush')
        for name, source in (('ArmorBorderBrush', 'BarBorder'), ('ArmorFillBrush', 'BarFill'))}
    for name, brush in brushes.items():
        resource = ue.WidgetLibrary.get_brush_resource(brush)
        assert resource and resource.get_name() == ('MI_UI_HealthBar_BarBorder' if name == 'ArmorBorderBrush' else 'MI_UI_HealthBar_Fill')
    for name, asset in (('StarBrush', STAR), ('SignatureRingBrush', RING)):
        material = ue.load_asset(asset); assert material, asset
        brushes[name] = ue.WidgetLibrary.make_brush_from_material(material, 80, 80)
    font = health_widgets['HealthNumber'].get_editor_property('Font')
    assert font.get_editor_property('FontObject'), 'Original HealthNumber font is required'
    return brushes, font


def material_info(brushes):
    result = {}
    for name, brush in brushes.items():
        material = ue.WidgetLibrary.get_brush_resource(brush)
        scalar = sorted(str(value) for value in ue.MaterialEditingLibrary.get_scalar_parameter_names(material))
        vector = sorted(str(value) for value in ue.MaterialEditingLibrary.get_vector_parameter_names(material))
        result[name] = dict(path=material.get_path_name(), scalar_parameters=scalar, vector_parameters=vector)
    for name in ('ArmorBorderBrush', 'ArmorFillBrush'):
        assert {'Health_Current', 'Health_Updated'} <= set(result[name]['scalar_parameters'])
    assert 'Inner_RGBA' in result['StarBrush']['vector_parameters']
    assert 'EliminationGlowAlpha' in result['StarBrush']['scalar_parameters']
    assert {'BarColor', 'ActiveRange'} <= set(result['SignatureRingBrush']['vector_parameters'])
    return result


def create_wrapper(umg, native_class, health, brushes, font):
    package = ASSETS['wrapper']
    bp = umg.call_method('CreateWidgetBlueprint', args=(package.rsplit('/', 1)[0], package.rsplit('/', 1)[1], native_class))
    assert bp and bp.get_path_name() == object_path(package)
    rows, _ = tree(umg, bp)
    roots = [row.get_editor_property('widget') for row in rows if not row.get_editor_property('parent')]
    assert len(roots) <= 1
    if roots:
        assert roots[0].get_outer().get_outer() == bp, 'Refuse inherited/foreign root removal'
        assert umg.call_method('RemoveWidget', args=(bp, roots[0]))
    cdo = ue.get_default_object(bp.generated_class())
    cdo.set_editor_property('HealthWidgetClass', health.generated_class())
    style = cdo.get_editor_property('StatusStyle')
    for name, value in STYLE_VALUES.items(): style.set_editor_property(name, value)
    for name, value in COLORS.items(): style.set_editor_property(name, ue.LinearColor(*value))
    for name, brush in brushes.items(): style.set_editor_property(name, brush)
    style.set_editor_property('TextFont', font)
    cdo.set_editor_property('StatusStyle', style)
    assert umg.call_method('CompileWidgetBlueprint', args=(bp,))
    check_wrapper(bp, native_class, health, umg, brushes, font)
    return bp


def check_wrapper(bp, native_class, health, umg, brushes, font):
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == native_class
    assert not tree(umg, bp)[0]
    cdo = ue.get_default_object(bp.generated_class())
    assert cdo.get_editor_property('HealthWidgetClass') == health.generated_class()
    assert not cdo.get_editor_property('bIsFocusable')
    style = cdo.get_editor_property('StatusStyle')
    for name, expected in STYLE_VALUES.items(): assert abs(float(style.get_editor_property(name)) - expected) < .000001, name
    for name, expected in COLORS.items():
        value = style.get_editor_property(name)
        assert max(abs(a-b) for a, b in zip((value.r, value.g, value.b, value.a), expected)) < .000001
    for name, expected in brushes.items(): assert style.get_editor_property(name).export_text() == expected.export_text(), name
    assert style.get_editor_property('TextFont').export_text() == font.export_text()


def edit_health_tree(bp, umg, widgets):
    original_number = widgets['HealthNumber']
    old_slot = original_number.get_editor_property('slot').get_path_name()
    widgets['AnimBoundSB'].set_width_override(440)
    widgets['AnimBoundSB'].set_height_override(32)
    widgets['NumberBorderSB'].set_visibility(ue.SlateVisibility.COLLAPSED)
    info = umg.call_method('AddWidget', args=(bp, ue.HorizontalBox, NEW_WIDGETS[0], widgets['Overlay_0'], -1))
    row = info.get_editor_property('widget'); assert row
    umg.call_method('ToggleWidgetAsVariable', args=(bp, row, False))
    overlay_slot = row.get_editor_property('slot')
    overlay_slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_CENTER)
    overlay_slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_CENTER)
    moved = umg.call_method('MoveWidget', args=(bp, original_number, row, -1))
    assert moved.get_editor_property('widget') == original_number
    original_number.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_CENTER)
    info = umg.call_method('AddWidget', args=(bp, ue.TextBlock, NEW_WIDGETS[1], row, -1))
    maximum = info.get_editor_property('widget'); assert maximum
    umg.call_method('ToggleWidgetAsVariable', args=(bp, maximum, False))
    maximum.set_text('/100')
    maximum.set_font(original_number.get_editor_property('Font'))
    maximum.set_color_and_opacity(original_number.get_editor_property('ColorAndOpacity'))
    maximum.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    maximum.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_CENTER)
    return dict(old_number_slot=old_slot, new_number_slot=original_number.get_editor_property('slot').get_path_name(),
                new_overlay_slot=overlay_slot.get_path_name())


def check_health_tree(widgets):
    assert widgets['AnimBoundSB'].get_editor_property('WidthOverride') == 440
    assert widgets['AnimBoundSB'].get_editor_property('HeightOverride') == 32
    assert widgets['NumberBorderSB'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert widgets['HealthNumber'].get_parent() == widgets[NEW_WIDGETS[0]]
    assert widgets[NEW_WIDGETS[1]].get_parent() == widgets[NEW_WIDGETS[0]]
    assert widgets[NEW_WIDGETS[0]].get_parent() == widgets['Overlay_0']
    assert list(widgets[NEW_WIDGETS[0]].get_all_children()) == [widgets['HealthNumber'], widgets[NEW_WIDGETS[1]]]


def edit_hud(bp, umg, widgets, wrapper):
    common = {name: widgets['BW_Healthbar'].get_editor_property(name) for name in (
        'RenderTransform', 'RenderTransformPivot', 'RenderOpacity', 'Clipping', 'FlowDirectionPreference', 'Visibility')}
    result = umg.call_method('ReplaceWidgetWithTemplate', args=(bp, widgets['BW_Healthbar'], wrapper.generated_class()))
    assert result.get_editor_property('bSuccess')
    assert not result.get_editor_property('UnmatchedReferencedProperties')
    assert not result.get_editor_property('UnmatchedReferencedFunctions')
    _, current = tree(umg, bp)
    widget = current['BW_Healthbar']
    assert widget.get_class() == wrapper.generated_class()
    for name, value in common.items(): widget.set_editor_property(name, value)
    slot = widget.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot) and slot.get_name() == 'CanvasPanelSlot_2'
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(1, 1)))
    slot.set_alignment(ue.Vector2D(0, 0))
    slot.set_offsets(ue.Margin(0, 0, 0, 0))
    slot.set_auto_size(False)
    def value_text(value):
        return value.export_text() if hasattr(value, 'export_text') else str(value)
    assert all(value_text(widget.get_editor_property(name)) == value_text(value) for name, value in common.items())
    return dict(widget=widget.get_path_name(), slot=slot.get_path_name(), replacement_report=result.export_text(),
                preserved_common_properties={name: value_text(value) for name, value in common.items()})


def check_hud(widgets, wrapper):
    common_hud(widgets)
    widget = widgets['BW_Healthbar']; assert widget.get_class() == wrapper.generated_class()
    slot = widget.get_editor_property('slot')
    anchors, alignment, offsets = slot.get_anchors(), slot.get_alignment(), slot.get_offsets()
    assert (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y) == (0, 0, 1, 1)
    assert (alignment.x, alignment.y, offsets.left, offsets.top, offsets.right, offsets.bottom) == (0, 0, 0, 0, 0, 0)
    assert not slot.get_auto_size() and slot.get_name() == 'CanvasPanelSlot_2'


def compare_approved(before, after, key, changes):
    assert before['parent'] == after['parent']
    assert before['cdo_path'] == after['cdo_path']
    old, new = persistent_records(before), persistent_records(after)
    base = object_path(ASSETS[key])
    added_nodes = {base + ':' + graph + '.' + name for graph, names in changes.get('nodes', {}).items() for name in names} if key == 'health' else set()
    new_names = NEW_WIDGETS if key == 'health' else ()
    added_prefixes = [base + ':WidgetTree.' + name for name in new_names] + list(added_nodes)
    if key == 'health': added_prefixes.append(changes['health_tree']['new_overlay_slot'])
    removed = {changes['health_tree']['old_number_slot']} if key == 'health' else set()
    for records in (old, new):
        for path in list(records):
            if path in removed or any(path == prefix or path.startswith(prefix + '.') for prefix in added_prefixes):
                records.pop(path)
            elif key == 'hud' and path.startswith(changes['hud']['widget'] + '.'):
                # Only the replaced UserWidget's owned template subtree changes.
                # The original health BP itself is independently fully compared.
                records.pop(path)
    assert set(old) == set(new), 'Unexpected object addition/removal: ' + key
    for path in old:
        first, second = old[path], new[path]
        assert len(first) == len(second), 'Changed object declaration count: ' + path
        transformed = []
        for text in (first, second):
            values = []
            for value in text:
                if key == 'experience' and path == before['cdo_path']:
                    value = strip_fields(value, EXPERIENCE_VALUES)
                if key == 'health':
                    if path == base: value = strip_guid_entries(value, NEW_WIDGETS)
                    if path.endswith(':WidgetTree.AnimBoundSB'): value = strip_fields(value, {'WidthOverride'})
                    if path.endswith(':WidgetTree.NumberBorderSB'): value = strip_fields(value, {'Visibility'})
                    if path.endswith(':WidgetTree.HealthNumber'): value = strip_fields(value, {'Slot'})
                    if path.endswith(':WidgetTree.Overlay_3'):
                        value = ''.join(line for line in value.splitlines(keepends=True) if not (line.strip().startswith('Slots(') and changes['health_tree']['old_number_slot'].rsplit('.', 1)[1] in line))
                    if path.endswith(':WidgetTree.Overlay_0'):
                        value = ''.join(line for line in value.splitlines(keepends=True) if not (line.strip().startswith('Slots(') and changes['health_tree']['new_overlay_slot'].rsplit('.', 1)[1] in line))
                    for graph, names in changes['nodes'].items():
                        if path == base + ':' + graph:
                            value = ''.join(line for line in value.splitlines(keepends=True) if not (line.strip().startswith('Nodes(') and any("'" + name + "'" in line for name in names)))
                    for graph, nodes in DENOMINATORS.items():
                        if path in {base + ':' + graph + '.' + node for node in nodes}: value = strip_pin_links(value, {'B'})
                    if path == base + ':EventGraph.K2Node_CustomEvent_0': value = strip_pin_links(value, {'HealthComponent'})
                if key == 'hud':
                    if path == changes['hud']['slot']: value = strip_fields(value, {'LayoutData', 'Content', 'bAutoSize'})
                    if path == changes['hud']['widget']:
                        # This one object is intentionally replaced with the approved wrapper.
                        value = '<approved BW_Healthbar class replacement>'
                    if path == base:
                        value = canonical_health_variable_type(value)
                        value = strip_guid_entries(value, ('BW_Healthbar',))
                values.append(value)
            transformed.append(values)
        assert transformed[0] == transformed[1], 'Unexpected authored property/pin/GUID change: ' + path
    if key == 'experience':
        assert strip_fields(before['cdo'], EXPERIENCE_VALUES) == strip_fields(after['cdo'], EXPERIENCE_VALUES)
    else:
        assert before['cdo'] == after['cdo'], 'Unexpected widget CDO defaults: ' + key
    assert set(before['authored']) == set(after['authored']), 'Authored graph set changed'
    for name in before['hierarchy']:
        if key == 'health' and name == 'HealthNumber':
            assert before['hierarchy'][name]['path'] == after['hierarchy'][name]['path']
            assert before['hierarchy'][name]['cls'] == after['hierarchy'][name]['cls']
        elif key == 'hud' and name == 'BW_Healthbar':
            assert before['hierarchy'][name]['path'] == after['hierarchy'][name]['path']
            assert before['hierarchy'][name]['parent'] == after['hierarchy'][name]['parent']
        else: assert before['hierarchy'][name] == after['hierarchy'][name], name
    assert set(after['hierarchy']) - set(before['hierarchy']) == (set(NEW_WIDGETS) if key == 'health' else set())


def main():
    command = ue.SystemLibrary.get_command_line()
    def arg(name, choices):
        values = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
        assert len(values) == 1, 'Explicit unique -' + name + ' required'
        return values[0]
    mode = arg('DCPlayerStatusSetupMode', 'inspect|apply|verify').lower()
    run = arg('DCPlayerStatusSetupRun', '[A-Za-z0-9_]{1,64}')
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert all(flag in command.lower() for flag in ('-run=pythonscript', '-unattended', '-enableplugins=umgtoolset', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor(); hud.clean()
    directory = REPORTS / run; log = directory / 'Editor.log'
    values = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(values) == 1 and Path(values[0][0] or values[0][1]).resolve() == log.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Use a fresh journal'
    before_files, protected, before_hashes = inventory(), hashes(), package_hashes()
    report = dict(schema=1, mode=mode, run=run, pid=os.getpid(), complete=False, saved=[], asset_saves=0,
        cpp_build=False, PIE=False, protected_hashes=protected, before_hashes=before_hashes,
        hidden_tolerance_pin_policy=dict(paths=list(TOLERANCE_PATHS), field='PinId only',
            required_shape=EMPTY_TOLERANCE_TAIL, raw_exports_preserved=True,
            source='K2Node_PromotableOperator.cpp:621-636,1430-1439; EdGraphPin.cpp:510'))
    try:
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'); assert settings
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', hud.export(ue.get_default_object(settings), directory / 'settings.t3d'), re.M) == ['SoC_Never']
        native = ue.load_class(None, '/Script/DreamCatcher.DCPlayerStatusWidget')
        assert native, 'BUILD_REQUIRED: DCPlayerStatusWidget is not registered'
        cdo = ue.get_default_object(native)
        try:
            assert cdo.call_method('GetSafeMaxHealth', args=(None,)) == 100.
            cdo.get_editor_property('HealthWidgetClass'); cdo.get_editor_property('StatusStyle')
        except Exception as exc: raise AssertionError('BUILD_REQUIRED: approved player-status native schema/helper missing') from exc
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_class
        umg = ue.get_default_object(umg_class)
        assets = {key: ue.load_asset(ASSETS[key]) for key in ('health', 'hud', 'experience')}
        assert all(assets.values())
        experience_cdo = ue.get_default_object(assets['experience'].generated_class())
        pawn_data = experience_cdo.get_editor_property('DefaultPawnData')
        pawn_class = pawn_data.get_editor_property('PawnClass') if pawn_data else None
        report['authoring_usage'] = dict(experience=ASSETS['experience'],
            pawn_data=pawn_data.get_path_name() if pawn_data else None,
            pawn_class=pawn_class.get_path_name() if pawn_class else None,
            hud_layout=ASSETS['hud'], original_health_widget=ASSETS['health'], runtime_actor_verified=False)
        _, hp_widgets = tree(umg, assets['health']); _, hud_widgets = tree(umg, assets['hud'])
        common_hud(hud_widgets)
        brushes, font = source_appearance(hp_widgets)
        report['material_parameters'] = material_info(brushes)
        report['source_font'] = font.export_text()
        loaded = {key: snapshot(key, bp, umg, directory, 'loaded') for key, bp in assets.items()}
        report['loaded'] = loaded
        if mode == 'inspect':
            assert all(before_hashes[key] == expected for key, expected in EXPECTED_BEFORE.items())
            assert not disk('wrapper').exists() and not ue.EditorAssetLibrary.does_asset_exist(ASSETS['wrapper'])
            baseline_health(hp_widgets); graph_contract(assets['health'])
            assert hud_widgets['BW_Healthbar'].get_class() == assets['health'].generated_class()
            report['baseline'] = loaded
        else:
            reference = arg('DCPlayerStatusSetupReference', '[A-Za-z0-9_]{1,64}')
            assert reference != run
            previous = json.loads((REPORTS / reference / 'report.json').read_text(encoding='utf-8'))
            assert previous['schema'] == 1 and previous['complete'] and previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['pid'] != report['pid'] and previous['protected_hashes'] == protected
            assert previous['after_hashes'] == before_hashes
            assert previous['material_parameters'] == report['material_parameters'] and previous['source_font'] == report['source_font']
            baseline = previous['baseline']; report.update(reference=reference, baseline=baseline)
            if mode == 'apply':
                assert not disk('wrapper').exists() and not ue.EditorAssetLibrary.does_asset_exist(ASSETS['wrapper']), 'Unknown/partial destination requires review'
                for key in assets: unchanged_snapshot(baseline[key], loaded[key])
                baseline_health(hp_widgets)
                wrapper = create_wrapper(umg, native, assets['health'], brushes, font)
                changes = dict(nodes=edit_graphs(assets['health'], graph_contract(assets['health'])))
                changes['health_tree'] = edit_health_tree(assets['health'], umg, hp_widgets)
                changes['hud'] = edit_hud(assets['hud'], umg, hud_widgets, wrapper)
                exp = ue.get_default_object(assets['experience'].generated_class())
                for name, value in EXPERIENCE_VALUES.items(): exp.set_editor_property(name, value)
                for key in ('health', 'hud', 'experience'):
                    assert (umg.call_method('CompileWidgetBlueprint', args=(assets[key],)) if key != 'experience' else ue.BlueprintEditorLibrary.compile_blueprint(assets[key]))
            else:
                assert previous['asset_saves'] == 4 and set(previous['saved']) == set(ASSETS.values())
                wrapper = ue.load_asset(ASSETS['wrapper']); assert wrapper
                changes = previous['changes']
            report['changes'] = changes
            check_graphs(assets['health'], changes['nodes'])
            check_health_tree(tree(umg, assets['health'])[1])
            check_wrapper(wrapper, native, assets['health'], umg, brushes, font)
            check_hud(tree(umg, assets['hud'])[1], wrapper)
            exp = ue.get_default_object(assets['experience'].generated_class())
            for name, value in EXPERIENCE_VALUES.items(): assert exp.get_editor_property(name) == value
            configured = {key: snapshot(key, bp, umg, directory, 'configured') for key, bp in assets.items()}
            report['configured'] = configured
            for key in assets: compare_approved(baseline[key], configured[key], key, changes)
            report['wrapper_cdo'] = hud.export(ue.get_default_object(wrapper.generated_class()), directory / 'wrapper_cdo.t3d')
            if mode == 'verify':
                assert report['wrapper_cdo'] == previous['wrapper_cdo']
                for key in assets: unchanged_snapshot(previous['configured'][key], configured[key])
            hud.clean(tuple(ASSETS.values())); hud.log_check(log)
            assert inventory() == before_files and hashes() == protected and package_hashes() == before_hashes
            if mode == 'apply':
                hud.assert_no_other_editor()
                for key in ('wrapper', 'health', 'hud', 'experience'):
                    report['save_intent'] = ASSETS[key]; hud.persist(directory / 'report.json', report)
                    obj = wrapper if key == 'wrapper' else assets[key]
                    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
                    report['saved'].append(ASSETS[key]); report['asset_saves'] += 1
                    report['after_hashes'] = package_hashes(); hud.persist(directory / 'report.json', report)
                report.pop('save_intent', None); hud.clean()
        report['after_hashes'] = package_hashes()
        if mode != 'apply': assert report['after_hashes'] == before_hashes
        hud.clean(tuple(ASSETS.values()))
        hud.log_check(log); report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        if mode == 'apply' and 'assets' in locals() and 'umg' in locals():
            try:
                report['failed_health_memory'] = snapshot('health', assets['health'], umg, directory, 'failed')
            except Exception:
                report['failure_snapshot_error'] = traceback.format_exc()
        raise
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_files
        report['protected_hashes_unchanged'] = hashes() == protected
        report['after_hashes'] = package_hashes()
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Player-status setup protection failed'
    print('PLAYER_STATUS_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'run', 'complete', 'asset_saves', 'saved')}))


if __name__ == '__main__':
    main()
