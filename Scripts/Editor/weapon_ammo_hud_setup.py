"""TODO7: inspect three equipment widgets; apply/verify only QuickBar and its row.

Separate UnrealEditor-Cmd Python commandlet; close other editors first:
-unattended -run=pythonscript -EnablePlugins=UMGToolSet -SCCProvider=None
-DCWeaponAmmoSetupMode=inspect|apply|verify -DCWeaponAmmoSetupRun=<fresh_id>
-DCWeaponAmmoSetupReference=<completed inspect/apply id> for apply/verify
-abslog=<project>/Saved/Diagnostics/WeaponAmmo20261020/Inspect_A/Editor.log
Apply compiles/checks both changed BPs before saving either. Verify never compiles
or saves. No PIE, gameplay changes, source-project load or binary file copying.
Font-only repair: -DCWeaponAmmoSetupMode=font_apply|font_verify, fresh run, and
-abslog=<project>/Saved/Diagnostics/WeaponAmmo20261020/FontFix_Report/<run>/Editor.log.
font_verify references a completed font_apply run; only the existing row is writable.
"""
import sys

sys.dont_write_bytecode = True

import importlib.util
import json
from pathlib import Path
import re
import traceback

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/WeaponAmmo20261020'
BASELINE = REPORTS / 'Before_A/baseline.json'
FONT_BASELINE = REPORTS / 'FontFix_Before_A/baseline.json'
FONT_REPORTS = REPORTS / 'FontFix_Report'
SELF = 'Scripts/Editor/weapon_ammo_hud_setup.py'
PROBE = 'Scripts/Editor/weapon_ammo_hud_gameplay_probe.py'
SOURCE_AUDIT = PROJECT / 'Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/source.json'
SOURCE_PROJECT = Path('E:/Epic Games/UEProjects/LyraStarterGame')
ASSETS = {
    name: '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/' + name
    for name in ('W_QuickBar', 'W_QuickBarSlot', 'W_WeaponAmmoAndName')
}
CHANGED = ('W_QuickBarSlot', 'W_QuickBar')
WRITABLE = {'Content/' + ASSETS[name].removeprefix('/Game/') + '.uasset' for name in CHANGED}
ROW_FUNCTION = 'DCRefreshWeaponRow'
CLEAR_FUNCTION = 'DCClearWeaponRow'
PANEL_FUNCTION = 'DCRefreshWeaponPanel'
ROWS = ('W_QuickBarSlot_1', 'W_QuickBarSlot_2', 'W_QuickBarSlot_3')
FONT_PROFILE = 'font_fit'
FONT_WRAPPERS = {
    'WeaponAmmoCount': ('DCWeaponMagazineFit', (40, 28, 50, 32)),
    'DCWeaponAmmoSlash': ('DCWeaponSlashFit', (92, 38, 12, 16)),
    'DCWeaponSpareAmmo': ('DCWeaponSpareFit', (106, 38, 50, 16)),
    'DCWeaponSlotNumber': ('DCWeaponSlotFit', (4, 2, 24, 22))}
FONT_GETTERS = ((13, 10, 'WeaponAmmoCount'), (15, 11, 'DCWeaponAmmoSlash'),
                (20, 12, 'DCWeaponSpareAmmo'), (27, 18, 'WeaponAmmoCount'),
                (29, 19, 'DCWeaponAmmoSlash'), (31, 20, 'DCWeaponSpareAmmo'))

spec = importlib.util.spec_from_file_location(
    'dc_weapon_ammo_inspect_evidence', PROJECT / 'Scripts/Editor/ingame_hud_layout_setup.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def normalized(values):
    return {str(key).replace('\\', '/'): value for key, value in values.items()}


def inventory():
    # The reused helper excludes its writable HUD asset; inspect permits no such exclusion.
    values = normalized(hud.inventory())
    info = hud.DISK.stat()
    values[hud.DISK.relative_to(PROJECT).as_posix()] = [info.st_size, info.st_mtime_ns]
    return values


def hashes(names):
    return {name: hud.sha(PROJECT / name) for name in names}


def check_parent_baseline(baseline, changed=False):
    assert hashes(baseline['protected']) == baseline['protected'], 'Protected baseline hash changed'
    expected = normalized(baseline['approved_assets'])
    exact = {'Content/' + package.removeprefix('/Game/') + '.uasset' for package in ASSETS.values()}
    assert set(expected) == exact, 'Unexpected target asset baseline'
    assert all(hud.sha(PROJECT / key) == value for key, value in expected.items()
               if not changed or key not in WRITABLE), 'Target asset baseline hash changed'
    expected_inventory = normalized(baseline['inventory'])
    # Before_A records exactly these six roots. SaveGames has a separate run hash guard.
    parent_roots = {'Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs'}
    actual_inventory = {key: value for key, value in inventory().items()
                        if key.split('/', 1)[0] in parent_roots}
    for key in {SELF, PROBE} | (WRITABLE if changed else set()):
        expected_inventory.pop(key, None)
        actual_inventory.pop(key, None)
    assert actual_inventory == expected_inventory, 'Before_A file inventory changed outside this script'


def reference_source():
    audit = json.loads(SOURCE_AUDIT.read_text(encoding='utf-8-sig'))
    result = {'audit': str(SOURCE_AUDIT), 'audit_sha256': hud.sha(SOURCE_AUDIT), 'assets': {}}
    for name in ASSETS:
        disk = SOURCE_PROJECT / ('Plugins/GameFeatures/ShooterCore/Content/UserInterface/HUD/' + name + '.uasset')
        entries = [(key, value) for key, value in audit['source_file_states'].items()
                   if Path(key).resolve() == disk.resolve()]
        assert len(entries) == 1, 'Missing exact original source audit entry: ' + name
        current = hud.sha(disk)
        assert current == entries[0][1], 'Original source asset changed since its graph audit: ' + name
        source_package = '/ShooterCore/UserInterface/HUD/' + name
        original = audit['assets'][source_package]
        assert original['path'] == source_package + '.' + name
        result['assets'][name] = {
            'disk': str(disk), 'sha256': current, 'audit_parent': original.get('parent'),
            'audit_graph_count': len(original.get('graphs', [])), 'unreal_load_requested': False}
    return result


def raw_export(obj, folder, filename):
    path = folder / filename
    text = hud.export(obj, path)
    return {'file': str(path.relative_to(REPORTS)), 'sha256': hud.sha(path),
            'text_characters': len(text), 'object': obj.get_path_name()}


def graph_snapshot(bp, folder):
    result = []
    for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp)):
        record = {'path': graph.get_path_name(), 'name': graph.get_name(),
                  'raw': raw_export(graph, folder, 'graph_%02d.t3d' % index), 'nodes': []}
        nodes = [node for node in ue.ObjectIterator(ue.EdGraphNode) if node.get_outer() == graph]
        for node in sorted(nodes, key=lambda item: item.get_path_name()):
            item = {'path': node.get_path_name(), 'name': node.get_name(),
                    'class': node.get_class().get_path_name()}
            if 'K2Node' in node.get_class().get_name():
                item['title'] = ue.BlueprintEditorLibrary.get_node_title(node)
                item['pins'] = [
                    {'name': str(pin.get_pin_name()), 'direction': str(pin.get_pin_direction()),
                     'value': pin.get_pin_value(), 'type': str(pin.get_pin_type_display_string()),
                     'links': sorted([other.get_owning_node().get_name(), str(other.get_pin_name())]
                                     for other in pin.list_connected_pins())}
                    for pin in ue.BlueprintEditorLibrary.list_all_pins(node)]
            record['nodes'].append(item)
        result.append(record)
    return result


def snapshot(name, bp, umg, directory, label=''):
    folder = directory / (label + name)
    folder.mkdir()
    cdo = ue.get_default_object(bp.generated_class())
    assert cdo, 'Missing generated CDO: ' + name
    result = {'asset': bp.get_path_name(),
              'parent': ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
              'generated_class': bp.generated_class().get_path_name(),
              'blueprint_raw': raw_export(bp, folder, 'blueprint.t3d'),
              'cdo_raw': raw_export(cdo, folder, 'cdo.t3d'),
              'graphs': graph_snapshot(bp, folder), 'widgets': []}
    rows, widgets = hud.tree(umg, bp)
    row_by_name = {str(row.get_editor_property('widget_name')): row for row in rows}
    for index, (widget_name, widget) in enumerate(sorted(widgets.items())):
        parent = widget.get_parent()
        slot = widget.get_editor_property('slot')
        named_host = row_by_name[widget_name].get_editor_property('named_slot_host')
        record = {'name': widget_name, 'path': widget.get_path_name(),
                  'class': widget.get_class().get_path_name(),
                  'parent': hud.path_of(parent), 'slot': hud.path_of(slot),
                  'named_slot_host': hud.path_of(named_host),
                  'raw': raw_export(widget, folder, 'widget_%02d.t3d' % index)}
        if slot:
            record['slot_raw'] = raw_export(slot, folder, 'slot_%02d.t3d' % index)
        result['widgets'].append(record)
    return result


def pin(node, name):
    values = [value for value in ue.BlueprintEditorLibrary.list_all_pins(node)
              if str(value.get_pin_name()) == name]
    assert len(values) == 1, (node.get_name(), name)
    return values[0]


def wire(start, end):
    if isinstance(start, (tuple, list)):
        for value in start:
            wire(value, end)
        return
    assert start.try_create_connection(end), (start.get_pin_name(), end.get_pin_name())


def literal(node, name, value):
    assert pin(node, name).set_pin_value(str(value)), (node.get_name(), name, value)


def call(editor, function):
    node = editor.add_call_function_node(function)
    assert node, 'Unavailable reflected function: ' + function
    return node


def member(editor, name, class_path=''):
    node = editor.add_get_member_variable_node(name, class_path)
    assert node, 'Unavailable member: ' + name
    return pin(node, name)


def set_widget(editor, previous, widget, function, values):
    node = call(editor, '/Script/UMG.' + function)
    wire(previous, pin(node, 'execute'))
    wire(member(editor, widget), pin(node, 'self'))
    for name, value in values.items():
        if isinstance(value, ue.BlueprintGraphPin):
            wire(value, pin(node, name))
        else:
            literal(node, name, value)
    return pin(node, 'then')


def visibility(editor, previous, widget, value):
    return set_widget(editor, previous, widget, 'Widget.SetVisibility', {'InVisibility': value})


def editor_nodes(bp, name):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, name)
    assert editor
    return editor, {node.get_name(): node for node in editor.list_all_nodes()}


def change_link(node, name, destination, changes, graph):
    value = pin(node, name)
    changes.setdefault(graph, []).append([node.get_name(), name])
    # Reciprocal ends are also approved precisely, never all links in the graph.
    for other in value.list_connected_pins():
        changes[graph].append([other.get_owning_node().get_name(), str(other.get_pin_name())])
    assert value.break_pin_links()
    if destination is not None:
        wire(value, destination)


def add_widget(umg, bp, cls, name, parent, variable=True):
    widget = umg.call_method('AddWidget', args=(bp, cls, name, parent, -1)).get_editor_property('widget')
    assert widget and widget.get_name() == name
    umg.call_method('ToggleWidgetAsVariable', args=(bp, widget, variable))
    return widget


def move_widget(umg, bp, widget, parent):
    assert umg.call_method('MoveWidget', args=(bp, widget, parent, -1)).get_editor_property('widget') == widget


def canvas_slot(widget, x, y, width, height):
    slot = widget.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot)
    slot.set_position(ue.Vector2D(x, y))
    slot.set_size(ue.Vector2D(width, height))
    slot.set_anchors(ue.Anchors(minimum=ue.Vector2D(0, 0), maximum=ue.Vector2D(0, 0)))
    slot.set_alignment(ue.Vector2D(0, 0))
    slot.set_auto_size(False)


def edit_row_tree(bp, umg, widgets, source_name):
    size = widgets['SizeBox_274']
    umg.call_method('ToggleWidgetAsVariable', args=(bp, size, True))
    size.set_width_override(352)
    size.set_height_override(64)
    widgets['SelectionBorder'].set_visibility(ue.SlateVisibility.COLLAPSED)
    widgets['SelectionBorder'].set_padding(ue.Margin(0))
    widgets['SelectionBorder'].set_brush_color(ue.LinearColor(1, 1, 1, 1))
    canvas = add_widget(umg, bp, ue.CanvasPanel, 'DCWeaponRowCanvas', widgets['Overlay_0'], False)
    canvas.get_editor_property('slot').set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    canvas.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    name = add_widget(umg, bp, ue.TextBlock, 'DCWeaponDisplayName', canvas)
    number = add_widget(umg, bp, ue.TextBlock, 'DCWeaponSlotNumber', canvas)
    slash = add_widget(umg, bp, ue.TextBlock, 'DCWeaponAmmoSlash', canvas)
    spare = add_widget(umg, bp, ue.CommonNumericTextBlock, 'DCWeaponSpareAmmo', canvas)
    for widget in (name, number):
        widget.set_font(source_name.get_editor_property('Font'))
        widget.set_font_size(14 if widget == name else 18)
        widget.set_color_and_opacity(widgets['WeaponAmmoCount'].get_editor_property('ColorAndOpacity'))
        widget.set_text('')
    name.set_text_overflow_policy(ue.TextOverflowPolicy.ELLIPSIS)
    name.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS)
    for widget in (slash, spare):
        widget.set_font(widgets['WeaponAmmoCount'].get_editor_property('Font'))
        widget.set_font_size(12)
        widget.set_color_and_opacity(widgets['WeaponAmmoCount'].get_editor_property('ColorAndOpacity'))
    slash.set_text('/')
    spare.set_current_value(0)
    widgets['WeaponAmmoCount'].set_current_value(0)
    widgets['WeaponAmmoCount'].set_font_size(28)
    for widget in (widgets['WeaponCard'], widgets['WeaponAmmoCount'], widgets['AmmoIcon']):
        move_widget(umg, bp, widget, canvas)
    canvas_slot(name, 40, 4, 190, 20)
    canvas_slot(number, 4, 2, 24, 22)
    canvas_slot(widgets['WeaponCard'], 238, 4, 106, 56)
    canvas_slot(widgets['WeaponAmmoCount'], 40, 28, 50, 32)
    canvas_slot(slash, 92, 38, 12, 16)
    canvas_slot(spare, 106, 38, 50, 16)
    canvas_slot(widgets['AmmoIcon'], 166, 38, 12, 16)
    widgets['AmmoCounterHB'].set_visibility(ue.SlateVisibility.COLLAPSED)
    widgets['EmptyText'].set_visibility(ue.SlateVisibility.COLLAPSED)
    # Preserve every original animation and its named binding; those still control material feedback.


def position_node(editor, previous, widget, x, y):
    slot = call(editor, '/Script/UMG.WidgetLayoutLibrary.SlotAsCanvasSlot')
    wire(member(editor, widget), pin(slot, 'Widget'))
    node = call(editor, '/Script/UMG.CanvasPanelSlot.SetPosition')
    wire(previous, pin(node, 'execute'))
    wire(pin(slot, 'ReturnValue'), pin(node, 'self'))
    literal(node, 'InPosition', '(X=%s,Y=%s)' % (x, y))
    return pin(node, 'then')


def layout_row_nodes(editor, previous, active):
    previous = set_widget(editor, previous, 'SizeBox_274', 'SizeBox.SetHeightOverride',
                          {'InHeightOverride': 64 if active else 24})
    previous = set_widget(editor, previous, 'WeaponAmmoCount', 'TextBlock.SetFontSize',
                          {'DisplayFontSize': 28 if active else 14})
    positions = [('WeaponAmmoCount', 40, 28), ('DCWeaponAmmoSlash', 92, 38), ('DCWeaponSpareAmmo', 106, 38)] if active else [
        ('WeaponAmmoCount', 240, 2), ('DCWeaponAmmoSlash', 266, 2), ('DCWeaponSpareAmmo', 278, 2)]
    for name, x, y in positions:
        previous = position_node(editor, previous, name, x, y)
    for name in ('WeaponCard', 'AmmoIcon'):
        previous = visibility(editor, previous, name, 'HitTestInvisible' if active else 'Collapsed')
    # Keep the original Border mouse binding hit-testable over the whole row.
    return visibility(editor, previous, 'SelectionBorder', 'Visible')


def create_clear_row(bp):
    editor = ue.BlueprintGraphEditor.create_and_edit_function_graph(bp, CLEAR_FUNCTION)
    previous = editor.find_graph_entry_pin()
    empty = call(editor, 'UpdateSlot')
    wire(previous, pin(empty, 'execute'))
    setter = editor.add_set_member_variable_node('QuickBarComponent')
    assert setter
    wire(pin(empty, 'then'), pin(setter, 'execute'))
    selected = call(editor, 'UpdateIsSelected')
    wire(pin(setter, 'then'), pin(selected, 'execute'))
    previous = visibility(editor, pin(selected, 'then'), 'SelectionBorder', 'Collapsed')
    for widget in ('DCWeaponDisplayName', 'DCWeaponSlotNumber'):
        previous = set_widget(editor, previous, widget, 'TextBlock.SetText', {'InText': ''})
    for widget in ('WeaponAmmoCount', 'DCWeaponSpareAmmo'):
        node = call(editor, '/Script/CommonUI.CommonNumericTextBlock.SetCurrentValue')
        wire(previous, pin(node, 'execute'))
        wire(member(editor, widget), pin(node, 'self'))
        literal(node, 'NewValue', '0')
        previous = pin(node, 'then')


def guard_valid(editor, previous, value, invalid):
    check = call(editor, '/Script/Engine.KismetSystemLibrary.IsValid')
    branch = editor.add_branch_node()
    assert branch
    wire(value, pin(check, 'Object'))
    wire(pin(check, 'ReturnValue'), pin(branch, 'Condition'))
    wire(previous, pin(branch, 'execute'))
    wire(pin(branch, 'else'), invalid)
    return pin(branch, 'then')


def edit_row_graphs(bp):
    changes = {}
    # Same-component refresh must also query the current item (SlotsChanged may be the only event).
    editor, nodes = editor_nodes(bp, 'UpdateComponentRef')
    change_link(nodes['K2Node_FunctionEntry_0'], 'then', pin(nodes['K2Node_VariableSet_1'], 'execute'), changes, 'UpdateComponentRef')
    changes['UpdateComponentRef'].append(['K2Node_VariableSet_1', 'execute'])
    clear = call(editor, CLEAR_FUNCTION)
    wire(pin(nodes['K2Node_MacroInstance_0'], 'Is Not Valid'), pin(clear, 'execute'))
    changes['UpdateComponentRef'].append(['K2Node_MacroInstance_0', 'Is Not Valid'])

    editor, nodes = editor_nodes(bp, 'UpdateTotalAmmoLeft')
    clear = call(editor, CLEAR_FUNCTION)
    change_link(nodes['K2Node_IfThenElse_1'], 'else', pin(clear, 'execute'), changes, 'UpdateTotalAmmoLeft')
    wire(pin(nodes['K2Node_IfThenElse_2'], 'then'), pin(clear, 'execute'))
    changes['UpdateTotalAmmoLeft'].append(['K2Node_IfThenElse_2', 'then'])
    old_exec = pin(nodes['K2Node_CallFunction_18'], 'execute')
    change_link(nodes['K2Node_IfThenElse_2'], 'else', None, changes, 'UpdateTotalAmmoLeft')
    success = guard_valid(editor, pin(nodes['K2Node_IfThenElse_2'], 'else'),
                          pin(nodes['K2Node_GetArrayItem_1'], 'Output'), pin(clear, 'execute'))
    wire(success, old_exec)
    change_link(nodes['K2Node_CallFunction_19'], 'InInt', pin(nodes['K2Node_CallFunction_8'], 'ReturnValue'), changes, 'UpdateTotalAmmoLeft')
    changes['UpdateTotalAmmoLeft'].append(['K2Node_CallFunction_8', 'ReturnValue'])
    previous = pin(nodes['K2Node_CallFunction_18'], 'then')
    changes['UpdateTotalAmmoLeft'].append(['K2Node_CallFunction_18', 'then'])
    convert = call(editor, '/Script/Engine.KismetMathLibrary.Conv_IntToDouble')
    wire(pin(nodes['K2Node_CallFunction_17'], 'ReturnValue'), pin(convert, 'InInt'))
    changes['UpdateTotalAmmoLeft'].append(['K2Node_CallFunction_17', 'ReturnValue'])
    spare = call(editor, '/Script/CommonUI.CommonNumericTextBlock.SetCurrentValue')
    wire(previous, pin(spare, 'execute'))
    wire(member(editor, 'DCWeaponSpareAmmo'), pin(spare, 'self'))
    wire(pin(convert, 'ReturnValue'), pin(spare, 'NewValue'))
    fragment = call(editor, '/Script/DreamCatcher.DCInventoryItemInstance.FindFragmentByClass')
    wire(pin(spare, 'then'), pin(fragment, 'execute'))
    wire(pin(nodes['K2Node_GetArrayItem_1'], 'Output'), pin(fragment, 'self'))
    changes['UpdateTotalAmmoLeft'].append(['K2Node_GetArrayItem_1', 'Output'])
    literal(fragment, 'FragmentClass', '/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon')
    previous = guard_valid(editor, pin(fragment, 'then'), pin(fragment, 'ReturnValue'), pin(clear, 'execute'))
    display = editor.add_get_member_variable_node('DisplayNameWhenEquipped', '/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon')
    assert display
    wire(pin(fragment, 'ReturnValue'), pin(display, 'self'))
    previous = set_widget(editor, previous, 'DCWeaponDisplayName', 'TextBlock.SetText',
                          {'InText': pin(display, 'DisplayNameWhenEquipped')})
    add = call(editor, '/Script/Engine.KismetMathLibrary.Add_IntInt')
    wire(member(editor, 'SlotIndex'), pin(add, 'A'))
    literal(add, 'B', '1')
    text = call(editor, '/Script/Engine.KismetTextLibrary.Conv_IntToText')
    wire(pin(add, 'ReturnValue'), pin(text, 'Value'))
    previous = set_widget(editor, previous, 'DCWeaponSlotNumber', 'TextBlock.SetText', {'InText': pin(text, 'ReturnValue')})
    branch = editor.add_branch_node()
    wire(previous, pin(branch, 'execute'))
    wire(member(editor, 'IsSelected'), pin(branch, 'Condition'))
    layout_row_nodes(editor, pin(branch, 'then'), True)
    layout_row_nodes(editor, pin(branch, 'else'), False)

    refresh = ue.BlueprintGraphEditor.create_and_edit_function_graph(bp, ROW_FUNCTION)
    result = refresh.add_graph_output_parameter('IsVisible', ue.BlueprintEditorLibrary.get_basic_type_by_name('bool'))
    clear = call(refresh, CLEAR_FUNCTION)
    wire(pin(clear, 'then'), pin(result, 'execute'))
    invalid = pin(clear, 'execute')
    previous = refresh.find_graph_entry_pin()
    enabled = refresh.add_branch_node()
    wire(previous, pin(enabled, 'execute'))
    wire(member(refresh, 'DCAllowUpdates'), pin(enabled, 'Condition'))
    wire(pin(enabled, 'else'), invalid)
    owner = call(refresh, '/Script/UMG.Widget.GetOwningPlayer')
    previous = guard_valid(refresh, pin(enabled, 'then'), pin(owner, 'ReturnValue'), invalid)
    pawn = call(refresh, '/Script/UMG.UserWidget.GetOwningPlayerPawn')
    previous = guard_valid(refresh, previous, pin(pawn, 'ReturnValue'), invalid)
    health = call(refresh, '/Script/DreamCatcher.DCLyraHealthComponent.FindHealthComponent')
    wire(pin(pawn, 'ReturnValue'), pin(health, 'Actor'))
    previous = guard_valid(refresh, previous, pin(health, 'ReturnValue'), invalid)
    maximum = call(refresh, '/Script/DreamCatcher.DCLyraHealthComponent.GetMaxHealth')
    wire(pin(health, 'ReturnValue'), pin(maximum, 'self'))
    ready = call(refresh, '/Script/Engine.KismetMathLibrary.Greater_DoubleDouble')
    wire(pin(maximum, 'ReturnValue'), pin(ready, 'A'))
    literal(ready, 'B', '0')
    ready_branch = refresh.add_branch_node()
    wire(previous, pin(ready_branch, 'execute'))
    wire(pin(ready, 'ReturnValue'), pin(ready_branch, 'Condition'))
    wire(pin(ready_branch, 'else'), invalid)
    previous = pin(ready_branch, 'then')
    death = call(refresh, '/Script/DreamCatcher.DCLyraHealthComponent.IsDeadOrDying')
    wire(previous, pin(death, 'execute'))
    wire(pin(health, 'ReturnValue'), pin(death, 'self'))
    wire(pin(death, 'True'), invalid)
    component = call(refresh, '/Script/Engine.Actor.GetComponentByClass')
    wire(pin(owner, 'ReturnValue'), pin(component, 'self'))
    literal(component, 'ComponentClass', '/Script/DreamCatcher.DCLyraQuickBarComponent')
    previous = guard_valid(refresh, pin(death, 'False'), pin(component, 'ReturnValue'), invalid)
    update = call(refresh, 'UpdateComponentRef')
    wire(previous, pin(update, 'execute'))
    wire(pin(component, 'ReturnValue'), pin(update, 'QuickBarComponent'))
    ammo = call(refresh, 'UpdateTotalAmmoLeft')
    wire(pin(update, 'then'), pin(ammo, 'execute'))
    wire(pin(component, 'ReturnValue'), pin(ammo, 'QuickBarComponent'))
    success = refresh.add_return_node()
    inverse = call(refresh, '/Script/Engine.KismetMathLibrary.Not_PreBool')
    wire(member(refresh, 'IsEmpty'), pin(inverse, 'A'))
    wire(pin(ammo, 'then'), pin(success, 'execute'))
    wire(pin(inverse, 'ReturnValue'), pin(success, 'IsVisible'))

    event, nodes = editor_nodes(bp, 'EventGraph')
    # All own-owner message filtering nodes/listeners and the click/input function remain original.
    for source, source_pin in [('K2Node_Event_2', 'then'), ('K2Node_IfThenElse_0', 'then'), ('K2Node_IfThenElse_1', 'then')]:
        node = call(event, ROW_FUNCTION)
        change_link(nodes[source], source_pin, pin(node, 'execute'), changes, 'EventGraph')
    init = call(event, ROW_FUNCTION)
    change_link(nodes['K2Node_CallFunction_3'], 'then', pin(init, 'execute'), changes, 'EventGraph')
    wire(pin(init, 'then'), pin(nodes['K2Node_ExecutionSequence_0'], 'execute'))
    construct = event.add_set_member_variable_node('DCAllowUpdates')
    literal(construct, 'DCAllowUpdates', 'true')
    wire(pin(nodes['K2Node_Event_1'], 'then'), pin(construct, 'execute'))
    changes['EventGraph'].append(['K2Node_Event_1', 'then'])
    update = call(event, ROW_FUNCTION)
    wire(pin(construct, 'then'), pin(update, 'execute'))
    destruct = ue.BlueprintEditorLibrary.add_event_override(bp, 'Destruct', ue.IntPoint(1800, 1800))
    assert destruct
    disable = event.add_set_member_variable_node('DCAllowUpdates')
    literal(disable, 'DCAllowUpdates', 'false')
    wire(pin(destruct, 'then'), pin(disable, 'execute'))
    clear = call(event, CLEAR_FUNCTION)
    wire(pin(disable, 'then'), pin(clear, 'execute'))
    return changes


def edit_panel_tree(bp, umg, widgets):
    overrides = {name: {'index': int(widgets[name].get_editor_property('SlotIndex')),
                        'input': hud.path_of(widgets[name].get_editor_property('SlotInputAction'))} for name in ROWS}
    assert [overrides[name]['index'] for name in ROWS] == [0, 1, 2]
    widgets['WeaponQuickBar'].set_visibility(ue.SlateVisibility.COLLAPSED)
    size = add_widget(umg, bp, ue.SizeBox, 'DCWeaponPanelSize', widgets['Overlay_0'], False)
    size.set_width_override(352)
    panel = add_widget(umg, bp, ue.Border, 'DCWeaponPanel', size)
    panel.set_brush(widgets['Border_1'].get_editor_property('Background'))
    panel.set_padding(ue.Margin(left=0, top=12, right=0, bottom=12))
    panel.set_visibility(ue.SlateVisibility.COLLAPSED)
    vertical = add_widget(umg, bp, ue.VerticalBox, 'DCWeaponRows', panel, False)
    for name in ROWS:
        umg.call_method('ToggleWidgetAsVariable', args=(bp, widgets[name], True))
        move_widget(umg, bp, widgets[name], vertical)
        widgets[name].get_editor_property('slot').set_padding(ue.Margin(0))
    return overrides


def edit_panel_graphs(bp, row_class):
    editor = ue.BlueprintGraphEditor.create_and_edit_function_graph(bp, PANEL_FUNCTION)
    previous = editor.find_graph_entry_pin()
    any_visible = None
    for name in ROWS:
        row = member(editor, name)
        # UObjectBaseUtility::GetPathName uses ':' for a field directly inside a package-owned class.
        update = call(editor, row_class.get_path_name() + ':' + ROW_FUNCTION)
        wire(previous, pin(update, 'execute'))
        wire(row, pin(update, 'self'))
        current = pin(update, 'IsVisible')
        previous = pin(update, 'then')
        slot = call(editor, '/Script/UMG.WidgetLayoutLibrary.SlotAsVerticalBoxSlot')
        wire(row, pin(slot, 'Widget'))
        if any_visible is None:
            pad = call(editor, '/Script/UMG.VerticalBoxSlot.SetPadding')
            wire(previous, pin(pad, 'execute'))
            wire(pin(slot, 'ReturnValue'), pin(pad, 'self'))
            literal(pad, 'InPadding', '(Left=0,Top=0,Right=0,Bottom=0)')
            previous = pin(pad, 'then')
            any_visible = current
        else:
            branch = editor.add_branch_node()
            wire(previous, pin(branch, 'execute'))
            gap = call(editor, '/Script/Engine.KismetMathLibrary.BooleanAND')
            wire(any_visible, pin(gap, 'A'))
            wire(current, pin(gap, 'B'))
            wire(pin(gap, 'ReturnValue'), pin(branch, 'Condition'))
            # A pure OR carries cumulative occupancy; separate setter branches rejoin at next call.
            combined = call(editor, '/Script/Engine.KismetMathLibrary.BooleanOR')
            wire(any_visible, pin(combined, 'A'))
            wire(current, pin(combined, 'B'))
            previous_outputs = []
            for output, top in [('then', 8), ('else', 0)]:
                pad = call(editor, '/Script/UMG.VerticalBoxSlot.SetPadding')
                wire(pin(branch, output), pin(pad, 'execute'))
                wire(pin(slot, 'ReturnValue'), pin(pad, 'self'))
                literal(pad, 'InPadding', '(Left=0,Top=%s,Right=0,Bottom=0)' % top)
                previous_outputs.append(pin(pad, 'then'))
            previous = previous_outputs
            any_visible = pin(combined, 'ReturnValue')
    branch = editor.add_branch_node()
    wire(previous, pin(branch, 'execute'))
    wire(any_visible, pin(branch, 'Condition'))
    visibility(editor, pin(branch, 'then'), 'DCWeaponPanel', 'SelfHitTestInvisible')
    visibility(editor, pin(branch, 'else'), 'DCWeaponPanel', 'Collapsed')
    for event_name in ('Tick', 'Construct'):
        event = ue.BlueprintEditorLibrary.add_event_override(bp, event_name, ue.IntPoint(100, 300 if event_name == 'Tick' else 0))
        assert event
        event_editor = ue.BlueprintGraphEditor.get_graph_editor(event.get_outer())
        refresh = call(event_editor, PANEL_FUNCTION)
        wire(pin(event, 'then'), pin(refresh, 'execute'))


def legacy_split_helper(graph, node):
    expected_graph = ASSETS['W_QuickBarSlot'] + '.W_QuickBarSlot:EventGraph'
    if graph['path'] != expected_graph:
        return False
    if node['name'] not in ('K2Node_BreakStruct_0', 'K2Node_BreakStruct_1'):
        return False
    contracts = {
        'Break DCLyra Quick Bar Active Index Changed Message': ('DCLyraQuickBarActiveIndexChangedMessage',
            'DCLyra Quick Bar Active Index Changed Message', 'ActiveIndex', 'Integer'),
        'Break DCLyra Quick Bar Slots Changed Message': ('DCLyraQuickBarSlotsChangedMessage',
            'DCLyra Quick Bar Slots Changed Message', 'Slots',
            'Array of DCInventory Item Instance Object References')}
    contract = contracts.get(node.get('title')) if graph['name'] == 'EventGraph' else None
    assert contract, ('Unexpected exact-name split helper type', node['name'])
    message, title, output, output_type = contract
    expected = {
        'path': ASSETS['W_QuickBarSlot'] + '.W_QuickBarSlot:EventGraph.' + node['name'],
        'name': node['name'], 'class': '/Script/BlueprintGraph.K2Node_BreakStruct',
        'title': 'Break ' + title,
        'pins': [{'name': name, 'direction': '<EdGraphPinDirection.EGPD_' + direction + '>',
                  'value': '', 'type': pin_type, 'links': []}
                 for name, direction, pin_type in (
                     (message, 'INPUT: 0', title + ' Structure (by ref)'),
                     ('Owner', 'OUTPUT: 1', 'Actor Object Reference'),
                     (output, 'OUTPUT: 1', output_type))]}
    assert node == expected, ('Split helper differs from exact disconnected contract', node['name'])
    # Inspect_A / Apply_B / Verify_A raw exports confirm these inner split-pin helpers
    # are not in Graph.Nodes. ObjectIterator sees them on some loads; authored nodes
    # are not being deleted. Their immutable original source/raw evidence is retained.
    assert not re.search(r'^\s*Nodes\(\d+\)=.*\b' + re.escape(node['name']) + r'\b',
                         exported_text(graph['raw']), re.M), 'Helper unexpectedly became an authored graph node'
    return True


def legacy_split_helpers(graph):
    helpers = [node for node in graph['nodes'] if legacy_split_helper(graph, node)]
    assert len(helpers) <= 2
    assert len({node['name'] for node in helpers}) == len(helpers)
    if len(helpers) == 2:
        # Their allocator names swap across loads; both exact message kinds must still occur once.
        assert len({node['pins'][0]['name'] for node in helpers}) == 2, 'Duplicate split helper message kind'
    return {node['name'] for node in helpers}


def canonical_reload_pin(graph, node, item, contract):
    title, cls, pin_name, pin_type, values, canonical = contract
    assert node['title'] == title and node['class'] == '/Script/BlueprintGraph.' + cls
    assert node['path'].endswith(':' + graph['name'] + '.' + node['name'])
    assert item['name'] == pin_name and item['type'] == pin_type
    assert item['direction'] == '<EdGraphPinDirection.EGPD_INPUT: 0>' and item['links'] == []
    assert item['value'] in values, ('Unapproved reload value', graph['name'], node['name'], item)
    if graph['name'] == ROW_FUNCTION and pin_name in ('B', 'IsVisible'):
        records = flat_objects(exported_text(graph['raw']))
        raw_pins = [line for line in records[node['path']].splitlines()
                    if line.startswith('CustomProperties Pin (') and 'PinName="' + pin_name + '"' in line]
        assert len(raw_pins) == 1 and 'LinkedTo=' not in raw_pins[0]
        raw = raw_pins[0]
        defaults = re.findall(r'(?<![A-Za-z])DefaultValue="([^"]*)"', raw)
        autogenerated = re.findall(r'AutogeneratedDefaultValue="([^"]*)"', raw)
        if pin_name == 'B':
            assert 'PinType.PinCategory="real",PinType.PinSubCategory="double",' in raw
            assert defaults == (['0'] if item['value'] == '0' else []) and autogenerated == []
            # KismetCompilerVMBackend.cpp:759-772 emits this double through FCString::Atod;
            # its missing/empty numeric literal and literal "0" both denote zero.
        else:
            assert 'PinType.PinCategory="bool",PinType.PinSubCategory="",' in raw
            expected = ['false'] if item['value'] == 'false' else []
            assert defaults == expected and autogenerated == expected
    item['value'] = canonical


def signature(snapshot_data, profile='legacy'):
    assert profile in ('legacy', FONT_PROFILE)
    # Only these observed NEW node pins receive typed-value spelling normalization.
    row = ASSETS['W_QuickBarSlot'] + '.W_QuickBarSlot'
    panel = ASSETS['W_QuickBar'] + '.W_QuickBar'
    contracts = {}
    inactive = (212, 0) if profile == FONT_PROFILE else (240, 2)
    for number, x, y in ((14, 40, 28), (16, 92, 38), (21, 106, 38),
                          (28, *inactive), (30, 266, 2), (32, 278, 2)):
        compact, expanded = '(X=%s,Y=%s)' % (x, y), '(X=%.6f,Y=%.6f)' % (x, y)
        contracts[(row, 'UpdateTotalAmmoLeft', 'K2Node_CallFunction_' + str(number))] = (
            'SetPosition', 'K2Node_CallFunction', 'InPosition', 'Vector 2D Structure', (compact, expanded), compact)
    for number in (5, 6):
        contracts[(row, CLEAR_FUNCTION, 'K2Node_CallFunction_' + str(number))] = (
            'SetCurrentValue', 'K2Node_CallFunction', 'NewValue', 'Float (single-precision)', ('0', '0.0'), '0.0')
    contracts[(row, ROW_FUNCTION, 'K2Node_FunctionResult_0')] = (
        'Return Node', 'K2Node_FunctionResult', 'IsVisible', 'Boolean', ('', 'false'), 'false')
    contracts[(row, ROW_FUNCTION, 'K2Node_PromotableOperator_0')] = (
        'float > float', 'K2Node_PromotableOperator', 'B', 'Float (double-precision)', ('0', ''), '0')
    if profile == FONT_PROFILE:
        contracts[(row, 'UpdateTotalAmmoLeft', 'K2Node_CallFunction_26')] = (
            'SetFontSize', 'K2Node_CallFunction', 'DisplayFontSize', 'Float (single-precision)', ('12', '12.0'), '12')
    for number, top in ((2, 0), (5, 8), (6, 0), (9, 8), (10, 0)):
        compact = '(Left=0,Top=%s,Right=0,Bottom=0)' % top
        expanded = '(Left=0.000000,Top=%.6f,Right=0.000000,Bottom=0.000000)' % top
        contracts[(panel, PANEL_FUNCTION, 'K2Node_CallFunction_' + str(number))] = (
            'SetPadding', 'K2Node_CallFunction', 'InPadding', 'Margin Structure', (compact, expanded), compact)
    result = []
    for graph in snapshot_data['graphs']:
        nodes = []
        helpers = legacy_split_helpers(graph)
        for original in graph['nodes']:
            if original['name'] in helpers:
                continue
            node = json.loads(json.dumps(original))
            contract = contracts.get((snapshot_data['asset'], graph['name'], node['name']))
            if contract:
                assert node['path'] == snapshot_data['asset'] + ':' + graph['name'] + '.' + node['name']
                pins = [item for item in node.get('pins', []) if item['name'] == contract[2]]
                assert len(pins) == 1
                canonical_reload_pin(graph, node, pins[0], contract)
            nodes.append(node)
        result.append({'name': graph['name'], 'nodes': nodes})
    return result


def preserve_original_graphs(before, after, changes):
    final = {graph['name']: graph for graph in after['graphs']}
    for graph in before['graphs']:
        assert graph['name'] in final
        helpers = legacy_split_helpers(graph)
        legacy_split_helpers(final[graph['name']])
        actual = {node['name']: node for node in final[graph['name']]['nodes']}
        mutable = {tuple(value) for value in changes.get(graph['name'], [])}
        for original in graph['nodes']:
            if original['name'] in helpers:
                continue
            if original['name'] not in actual:
                raise AssertionError(('Original node removed', graph['name'], original['name']))
            left, right = json.loads(json.dumps(original)), json.loads(json.dumps(actual[original['name']]))
            for value in (left, right):
                for item in value.get('pins', []):
                    if (original['name'], item['name']) in mutable:
                        item['links'] = []
            assert left == right, ('Original node changed outside approved pins', graph['name'], original['name'])


def check_layout(assets, umg, overrides, profile='legacy'):
    _, row = hud.tree(umg, assets['W_QuickBarSlot'])
    _, panel = hud.tree(umg, assets['W_QuickBar'])
    assert row['SizeBox_274'].get_editor_property('WidthOverride') == 352
    assert row['SizeBox_274'].get_editor_property('HeightOverride') == 64
    assert row['SelectionBorder'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert row['DCWeaponDisplayName'].get_editor_property('TextOverflowPolicy') == ue.TextOverflowPolicy.ELLIPSIS
    assert row['DCWeaponDisplayName'].get_editor_property('Clipping') == ue.WidgetClipping.CLIP_TO_BOUNDS
    assert panel['WeaponQuickBar'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert panel['DCWeaponPanelSize'].get_editor_property('WidthOverride') == 352
    padding = panel['DCWeaponPanel'].get_editor_property('Padding')
    assert (padding.left, padding.top, padding.right, padding.bottom) == (0, 12, 0, 12)
    assert [child.get_name() for child in panel['DCWeaponRows'].get_all_children()] == list(ROWS)
    for name, original in overrides.items():
        assert panel[name].get_editor_property('SlotIndex') == original['index']
        assert hud.path_of(panel[name].get_editor_property('SlotInputAction')) == original['input']
    assert set((ROW_FUNCTION, CLEAR_FUNCTION)) <= {graph.get_name() for graph in ue.BlueprintEditorLibrary.list_graphs(assets['W_QuickBarSlot'])}
    assert PANEL_FUNCTION in {graph.get_name() for graph in ue.BlueprintEditorLibrary.list_graphs(assets['W_QuickBar'])}
    if profile == FONT_PROFILE:
        check_font_row(row)
    return {'width': 352, 'active_row': 64, 'inactive_row': 24, 'gap': 8,
            'vertical_padding': 12, 'occupied_heights': [88, 120, 152],
            'original_slot_overrides': overrides, 'pixel_geometry_verified': False,
            'gameplay_verified': False, 'basis': 'Compiled graph literals and template layout; user checks actual gameplay/pixels.'}


def flat_objects(text):
    stack, records = [], {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            match = re.search(r'ExportPath="[^\']*\'([^\']+)\'"', line)
            assert match, 'Exporter omitted object identity'
            stack.append([match.group(1), []])
        elif stripped == 'End Object':
            name, lines = stack.pop()
            if lines:
                assert name not in records
                records[name] = '\n'.join(lines)
        elif stack:
            stack[-1][1].append(line.strip())
    assert not stack
    return records


def exported_text(record):
    path = REPORTS / record['file']
    assert hud.sha(path) == record['sha256'], 'Diagnostic evidence changed'
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')


def preserve_animation_and_defaults(before, after, name, profile='legacy'):
    left, right = [flat_objects(exported_text(item['blueprint_raw'])) for item in (before, after)]
    animations = {key: value for key, value in left.items()
                  if any(':' + token in key for token in ('InactiveToActive', 'ActiveToInactive', 'ToEmpty'))}
    assert all(right.get(key) == value for key, value in animations.items()), 'Original animation data changed'
    additions = {'DCAllowUpdates', 'DCWeaponDisplayName', 'DCWeaponSlotNumber', 'DCWeaponAmmoSlash',
                 'DCWeaponSpareAmmo', 'SizeBox_274', 'DCWeaponPanel'} | set(ROWS)
    if profile == FONT_PROFILE and name == 'W_QuickBarSlot':
        for wrapper, _ in FONT_WRAPPERS.values():
            additions.add(wrapper)
            for item, required in ((before, False), (after, True)):
                fields = re.findall(r'^\s*' + re.escape(wrapper) + r'=(.*)$', exported_text(item['cdo_raw']), re.M)
                assert (fields == ['None'] if required else fields in ([], ['None'])), 'Unexpected fit-wrapper CDO value'
    def cdo_lines(item):
        records = flat_objects(exported_text(item['cdo_raw']))
        assert len(records) == 1
        return [line for line in next(iter(records.values())).splitlines()
                if line.split('=', 1)[0] not in additions]
    old_cdo, new_cdo = cdo_lines(before), cdo_lines(after)
    if name == 'W_QuickBar':
        assert old_cdo.count('bHasScriptImplementedTick=False') == 1
        assert new_cdo.count('bHasScriptImplementedTick=True') == 1
        new_cdo = ['bHasScriptImplementedTick=False' if line == 'bHasScriptImplementedTick=True' else line
                   for line in new_cdo]
    assert old_cdo == new_cdo, 'Original CDO defaults changed: ' + name
    return {'unchanged_animation_records': len(animations), 'original_cdo_defaults_unchanged': True,
            'accepted_cdo_change': {'bHasScriptImplementedTick': [False, True]} if name == 'W_QuickBar' else {}}


def compile_widget(umg, bp, report):
    report['compile_requested'] += 1
    assert umg.call_method('CompileWidgetBlueprint', args=(bp,)), 'Widget compile failed: ' + bp.get_name()
    report['compile'] += 1


def fit_font_tree(bp, umg):
    _, widgets = hud.tree(umg, bp)
    assert not {wrapper for wrapper, _ in FONT_WRAPPERS.values()} & widgets.keys(), 'Font fit is already present'
    canvas = widgets['DCWeaponRowCanvas']
    for original, (wrapper_name, bounds) in FONT_WRAPPERS.items():
        widget = widgets[original]
        assert widget.get_parent() == canvas and isinstance(widget.get_editor_property('slot'), ue.CanvasPanelSlot)
        wrapper = add_widget(umg, bp, ue.ScaleBox, wrapper_name, canvas)
        wrapper.set_stretch(ue.Stretch.SCALE_TO_FIT)
        wrapper.set_stretch_direction(ue.StretchDirection.DOWN_ONLY)
        wrapper.set_ignore_inherited_scale(False)
        wrapper.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS)
        canvas_slot(wrapper, *bounds)
        move_widget(umg, bp, widget, wrapper)
        slot = widget.get_editor_property('slot')
        assert isinstance(slot, ue.ScaleBoxSlot)
        slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_LEFT)
        slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_TOP)
    name = widgets['DCWeaponDisplayName']
    name.set_font_size(12)
    canvas_slot(name, 40, 4, 160, 20)
    name.set_text_overflow_policy(ue.TextOverflowPolicy.ELLIPSIS)
    name.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS)


def fit_font_graphs(bp):
    editor, nodes = editor_nodes(bp, 'UpdateTotalAmmoLeft')
    changes = {}
    for slot_number, getter_number, original in FONT_GETTERS:
        slot = nodes['K2Node_CallFunction_' + str(slot_number)]
        assert ue.BlueprintEditorLibrary.get_node_title(slot) == 'SlotAsCanvasSlot'
        linked = [(p.get_owning_node().get_name(), str(p.get_pin_name())) for p in pin(slot, 'Widget').list_connected_pins()]
        assert linked == [('K2Node_VariableGet_' + str(getter_number), original)], 'Unexpected original canvas getter'
        wrapper = FONT_WRAPPERS[original][0]
        change_link(slot, 'Widget', member(editor, wrapper), changes, 'UpdateTotalAmmoLeft')
    inactive_font = nodes['K2Node_CallFunction_26']
    assert ue.BlueprintEditorLibrary.get_node_title(inactive_font) == 'SetFontSize'
    assert pin(inactive_font, 'DisplayFontSize').get_pin_value() in ('14', '14.0')
    literal(inactive_font, 'DisplayFontSize', '12')
    inactive_position = nodes['K2Node_CallFunction_28']
    assert ue.BlueprintEditorLibrary.get_node_title(inactive_position) == 'SetPosition'
    assert pin(inactive_position, 'InPosition').get_pin_value() in ('(X=240,Y=2)', '(X=240.000000,Y=2.000000)')
    literal(inactive_position, 'InPosition', '(X=212,Y=0)')
    return changes


def check_font_row(widgets):
    assert widgets['SizeBox_274'].get_editor_property('WidthOverride') == 352
    assert widgets['SizeBox_274'].get_editor_property('HeightOverride') == 64
    for original, (wrapper_name, bounds) in FONT_WRAPPERS.items():
        wrapper, widget = widgets[wrapper_name], widgets[original]
        assert isinstance(wrapper, ue.ScaleBox) and wrapper.get_parent() == widgets['DCWeaponRowCanvas']
        assert wrapper.get_editor_property('Stretch') == ue.Stretch.SCALE_TO_FIT
        assert wrapper.get_editor_property('StretchDirection') == ue.StretchDirection.DOWN_ONLY
        assert not wrapper.get_editor_property('IgnoreInheritedScale')
        assert wrapper.get_editor_property('Clipping') == ue.WidgetClipping.CLIP_TO_BOUNDS
        assert wrapper.get_children_count() == 1 and wrapper.get_child_at(0) == widget
        slot = widget.get_editor_property('slot')
        assert isinstance(slot, ue.ScaleBoxSlot)
        assert slot.get_editor_property('HorizontalAlignment') == ue.HorizontalAlignment.H_ALIGN_LEFT
        assert slot.get_editor_property('VerticalAlignment') == ue.VerticalAlignment.V_ALIGN_TOP
        parent_slot = wrapper.get_editor_property('slot')
        position, size = parent_slot.get_position(), parent_slot.get_size()
        assert (position.x, position.y, size.x, size.y) == bounds
        anchors, alignment = parent_slot.get_anchors(), parent_slot.get_alignment()
        assert (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y,
                alignment.x, alignment.y) == (0, 0, 0, 0, 0, 0) and not parent_slot.get_auto_size()
    name = widgets['DCWeaponDisplayName']
    position, size = name.get_editor_property('slot').get_position(), name.get_editor_property('slot').get_size()
    assert (position.x, position.y, size.x, size.y) == (40, 4, 160, 20)
    assert name.get_editor_property('Font').get_editor_property('Size') == 12
    assert name.get_editor_property('TextOverflowPolicy') == ue.TextOverflowPolicy.ELLIPSIS
    assert name.get_editor_property('Clipping') == ue.WidgetClipping.CLIP_TO_BOUNDS
    expected_children = ['DCWeaponDisplayName', 'WeaponCard', 'AmmoIcon'] + [value[0] for value in FONT_WRAPPERS.values()]
    assert [child.get_name() for child in widgets['DCWeaponRowCanvas'].get_all_children()] == expected_children


def font_graph_contract(data, profile):
    assert profile in ('legacy', FONT_PROFILE)
    normalized_graphs = signature(data, profile)  # Exact typed defaults and split-helper checks.
    graph = next(graph for graph in data['graphs'] if graph['name'] == 'UpdateTotalAmmoLeft')
    nodes = {node['name']: node for node in graph['nodes']}
    for slot_number, getter_number, original in FONT_GETTERS:
        slot = nodes['K2Node_CallFunction_' + str(slot_number)]
        assert slot['title'] == 'SlotAsCanvasSlot' and slot['class'] == '/Script/BlueprintGraph.K2Node_CallFunction'
        value = next(value for value in slot['pins'] if value['name'] == 'Widget')
        assert value['type'] == 'Widget Object Reference' and value['direction'] == '<EdGraphPinDirection.EGPD_INPUT: 0>'
        if profile == 'legacy':
            assert value['links'] == [['K2Node_VariableGet_' + str(getter_number), original]]
        else:
            assert len(value['links']) == 1
            getter_name, output = value['links'][0]
            assert output == FONT_WRAPPERS[original][0]
            getter = nodes[getter_name]
            assert getter['class'] == '/Script/BlueprintGraph.K2Node_VariableGet'
            assert getter['path'] == data['asset'] + ':UpdateTotalAmmoLeft.' + getter_name
            assert len(getter['pins']) == 1
            assert getter['pins'][0] == {'name': output, 'direction': '<EdGraphPinDirection.EGPD_OUTPUT: 1>',
                'value': '', 'type': 'Scale Box Object Reference', 'links': [[slot['name'], 'Widget']]}
    font = next(value for value in nodes['K2Node_CallFunction_26']['pins'] if value['name'] == 'DisplayFontSize')
    assert nodes['K2Node_CallFunction_26']['title'] == 'SetFontSize'
    assert font['type'] == 'Float (single-precision)' and font['links'] == []
    assert font['value'] in (('14', '14.0') if profile == 'legacy' else ('12', '12.0'))
    return normalized_graphs


def preserve_font_graphs(before, after, changes):
    old_signature = {graph['name']: graph['nodes'] for graph in font_graph_contract(before, 'legacy')}
    new_signature = {graph['name']: graph['nodes'] for graph in font_graph_contract(after, FONT_PROFILE)}
    original = json.loads(json.dumps(before))
    adjusted = json.loads(json.dumps(after))
    # Reuse only the previously observed exact typed-default spelling contracts.
    for graph in original['graphs']:
        graph['nodes'] = old_signature[graph['name']]
    for graph in adjusted['graphs']:
        graph['nodes'] = new_signature[graph['name']]
    for graph in original['graphs']:
        old_names = {node['name'] for node in graph['nodes']}
        new_nodes = new_signature[graph['name']]
        added = {node['name'] for node in new_nodes} - old_names
        if graph['name'] == 'UpdateTotalAmmoLeft':
            getter_nodes = {link[0] for node in new_nodes
                            if node['name'] in {'K2Node_CallFunction_' + str(value[0]) for value in FONT_GETTERS}
                            for value in node['pins'] if value['name'] == 'Widget' for link in value['links']}
            assert len(getter_nodes) == 6 and added == getter_nodes, 'Only six fit-wrapper getters may be added'
        else:
            assert not added, 'Unexpected added node outside the font patch'
    old_graph = next(graph for graph in original['graphs'] if graph['name'] == 'UpdateTotalAmmoLeft')
    new_graph = next(graph for graph in adjusted['graphs'] if graph['name'] == 'UpdateTotalAmmoLeft')
    for number, field in ((26, 'DisplayFontSize'), (28, 'InPosition')):
        name = 'K2Node_CallFunction_' + str(number)
        old = next(pin for node in old_graph['nodes'] if node['name'] == name for pin in node['pins'] if pin['name'] == field)
        new = next(pin for node in new_graph['nodes'] if node['name'] == name for pin in node['pins'] if pin['name'] == field)
        assert {key: value for key, value in old.items() if key != 'value'} == {
            key: value for key, value in new.items() if key != 'value'}
        new['value'] = old['value']  # Exactly the two prevalidated approved scalar/layout changes.
    preserve_original_graphs(original, adjusted, changes)


def preserve_font_widgets(before, after):
    old = {widget['name']: widget for widget in before['widgets']}
    new = {widget['name']: widget for widget in after['widgets']}
    assert set(new) - set(old) == {value[0] for value in FONT_WRAPPERS.values()}
    assert not set(old) - set(new)
    for name in old:
        if name == 'DCWeaponRowCanvas':
            continue  # Its exact seven children and their bounds are checked by check_font_row.
        left, right = exported_text(old[name]['raw']), exported_text(new[name]['raw'])
        if name in FONT_WRAPPERS:
            assert new[name]['parent'].endswith('.' + FONT_WRAPPERS[name][0])
            left = re.sub(r'^\s*Slot=.*\n?', '', left, flags=re.M)
            right = re.sub(r'^\s*Slot=.*\n?', '', right, flags=re.M)
        elif name == 'DCWeaponDisplayName':
            old_font = re.findall(r'^\s*Font=(.*)$', left, re.M)
            new_font = re.findall(r'^\s*Font=(.*)$', right, re.M)
            assert len(old_font) == len(new_font) == 1
            expected, count = re.subn(r'(?<![A-Za-z])Size=14\.000000(?=[,)])', 'Size=12.000000', old_font[0])
            assert count == 1 and new_font == [expected], 'Name font changed beyond approved size'
            left = re.sub(r'^\s*Font=.*\n?', '', left, flags=re.M)
            right = re.sub(r'^\s*Font=.*\n?', '', right, flags=re.M)
        assert left == right, 'Unapproved existing widget property change: ' + name
        if name not in FONT_WRAPPERS and name != 'DCWeaponDisplayName' and old[name].get('slot_raw'):
            assert exported_text(old[name]['slot_raw']) == exported_text(new[name]['slot_raw']), 'Unapproved existing slot change: ' + name
    return {'old_widget_objects_preserved': len(old), 'new_fit_wrappers': 4,
            'name_font': 12, 'name_width': 160, 'pixel_geometry_verified': False}


def font_protection(baseline, allow_row=False):
    row_file = 'Content/' + ASSETS['W_QuickBarSlot'].removeprefix('/Game/') + '.uasset'
    ignored = {SELF} | ({row_file} if allow_row else set())
    expected = {key: value for key, value in baseline['hashes'].items() if key not in ignored}
    assert hashes(expected) == expected, 'Font-fix protected file hash changed'
    assert {key: value for key, value in inventory().items() if key not in ignored} == {
        key: value for key, value in normalized(baseline['inventory']).items() if key not in ignored}, 'Font-fix baseline inventory changed'


def font_main(command, mode):
    assert mode in ('font_apply', 'font_verify')
    run = argument(command, 'DCWeaponAmmoSetupRun', '[A-Za-z0-9_]+')
    for flag in ('unattended', 'run=pythonscript', 'SCCProvider=None'):
        assert re.search(r'(?i)(?:^|\s)-' + re.escape(flag) + r'(?=\s|$)', command)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor()
    hud.clean()
    directory = FONT_REPORTS / run
    assert not (directory / 'report.json').exists(), 'Existing font-fix report'
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / 'Editor.log'
    args = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(args) == 1 and Path(next(value for value in args[0] if value)).resolve() == log.resolve()
    baseline = json.loads(FONT_BASELINE.read_text(encoding='utf-8-sig'))
    row_file = 'Content/' + ASSETS['W_QuickBarSlot'].removeprefix('/Game/') + '.uasset'
    before_inventory = inventory()
    before_hashes = hashes(baseline['hashes'])
    savegames = hashes([name for name in before_inventory if name.startswith('Saved/SaveGames/')])
    before_hashes.update(savegames)
    report = {'schema': 3, 'mode': mode, 'run': run, 'complete': False, 'font_profile': FONT_PROFILE,
              'baseline': str(FONT_BASELINE), 'baseline_sha256': hud.sha(FONT_BASELINE),
              'script_sha256': hud.sha(PROJECT / SELF), 'compile': 0, 'compile_requested': 0,
              'asset_saves': 0, 'PIE': False, 'cpp_build': False, 'assets': {}, 'failures': [],
              'protected_before': before_hashes, 'savegames_before': savegames,
              'scope': 'Only W_QuickBarSlot font fit tree, six canvas-slot getters, inactive font and one coordinate.',
              'pixel_geometry_verified': False, 'gameplay_verified': False}
    try:
        font_protection(baseline, allow_row=(mode == 'font_verify'))
        if mode == 'font_apply':
            reference_path = REPORTS / baseline['previous_validation']
            reference = json.loads(reference_path.read_text(encoding='utf-8-sig'))
            assert reference['complete'] and reference['mode'] == 'verify'
        else:
            previous_run = argument(command, 'DCWeaponAmmoSetupReference', '[A-Za-z0-9_]+')
            assert previous_run != run
            reference_path = FONT_REPORTS / previous_run / 'report.json'
            reference = json.loads(reference_path.read_text(encoding='utf-8-sig'))
            assert reference['complete'] and reference['mode'] == 'font_apply'
            assert reference['baseline_sha256'] == report['baseline_sha256']
            assert hashes(reference['asset_sha256']) == reference['asset_sha256']
        report['reference'] = {'report': str(reference_path), 'sha256': hud.sha(reference_path)}
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings_class
        settings = hud.export(ue.get_default_object(settings_class), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet')
        assert umg_class
        umg = ue.get_default_object(umg_class)
        bp = ue.load_asset(ASSETS['W_QuickBarSlot'])
        assert bp and bp.get_path_name() == ASSETS['W_QuickBarSlot'] + '.W_QuickBarSlot'
        before = snapshot('W_QuickBarSlot', bp, umg, directory, 'before_' if mode == 'font_apply' else '')
        report['assets']['W_QuickBarSlot'] = before
        hud.clean()
        if mode == 'font_apply':
            assert signature(before) == signature(reference['assets']['W_QuickBarSlot'])
            font_graph_contract(before, 'legacy')
            fit_font_tree(bp, umg)
            compile_widget(umg, bp, report)  # Expose the four wrapper member variables first.
            changes = fit_font_graphs(bp)
            compile_widget(umg, bp, report)
            after = snapshot('W_QuickBarSlot', bp, umg, directory, 'compiled_')
            report['after'] = {'W_QuickBarSlot': after}
            report['approved_pin_changes'] = changes
            preserve_font_graphs(before, after, changes)
            report['widget_preservation'] = preserve_font_widgets(before, after)
            report['preservation'] = preserve_animation_and_defaults(before, after, 'W_QuickBarSlot', FONT_PROFILE)
            check_font_row(hud.tree(umg, bp)[1])
            hud.clean([ASSETS['W_QuickBarSlot']])
            font_protection(baseline)
            assert hashes(before_hashes) == before_hashes
            hud.log_check(log)
            assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
            report['asset_saves'] = 1
            report['asset_sha256'] = hashes([row_file])
        else:
            assert signature(before, FONT_PROFILE) == signature(reference['after']['W_QuickBarSlot'], FONT_PROFILE)
            preserve_font_graphs(reference['assets']['W_QuickBarSlot'], before, reference['approved_pin_changes'])
            report['widget_preservation'] = preserve_font_widgets(reference['assets']['W_QuickBarSlot'], before)
            report['preservation'] = preserve_animation_and_defaults(reference['assets']['W_QuickBarSlot'], before,
                                                                    'W_QuickBarSlot', FONT_PROFILE)
            check_font_row(hud.tree(umg, bp)[1])
            report['asset_sha256'] = hashes([row_file])
            assert report['asset_sha256'] == reference['asset_sha256']
        hud.clean()
        hud.log_check(log)
    except Exception:
        report['failures'].append(traceback.format_exc())
    finally:
        try:
            hud.clean([ASSETS['W_QuickBarSlot']]) if mode == 'font_apply' and report['failures'] else hud.clean()
            ignored = {row_file} if mode == 'font_apply' else set()
            assert {key: value for key, value in inventory().items() if key not in ignored} == {
                key: value for key, value in before_inventory.items() if key not in ignored}
            expected = {key: value for key, value in before_hashes.items() if key not in ignored}
            assert hashes(expected) == expected
            font_protection(baseline, allow_row=(mode == 'font_verify' or report['asset_saves'] == 1))
            assert hud.sha(FONT_BASELINE) == report['baseline_sha256']
            report['protection_unchanged'] = True
        except Exception:
            report['protection_unchanged'] = False
            report['failures'].append(traceback.format_exc())
        report['complete'] = not report['failures'] and len(report['assets']) == 1
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Weapon font-fix failed; see ' + str(directory / 'report.json')
    ue.log('Weapon font-fix complete: ' + mode + '; ' + str(directory / 'report.json'))


def argument(command, name, choices):
    matches = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
    assert len(matches) == 1, 'One -' + name + ' argument is required'
    return matches[0]


def main():
    command = ue.SystemLibrary.get_command_line()
    mode = argument(command, 'DCWeaponAmmoSetupMode', 'inspect|apply|verify|font_apply|font_verify').lower()
    if mode in ('font_apply', 'font_verify'):
        font_main(command, mode)
        return
    run = argument(command, 'DCWeaponAmmoSetupRun', '[A-Za-z0-9_]+')
    assert re.search(r'(?i)(?:^|\s)-unattended(?=\s|$)', command)
    assert re.search(r'(?i)(?:^|\s)-run=pythonscript(?=\s|$)', command)
    assert re.search(r'(?i)(?:^|\s)-SCCProvider=None(?=\s|$)', command)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert not ue.SourceControl.is_enabled(), 'Source control must be disabled'
    hud.assert_no_other_editor()
    hud.clean()
    directory = REPORTS / run
    # -abslog can create the directory before Python starts; never overwrite an existing report.
    assert not (directory / 'report.json').exists(), 'Existing diagnostic report'
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / 'Editor.log'
    log_args = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(log_args) == 1 and Path(next(value for value in log_args[0] if value)).resolve() == log.resolve()
    baseline = json.loads(BASELINE.read_text(encoding='utf-8-sig'))
    before_inventory = inventory()
    before_hashes = hashes(list(baseline['protected']) + list(baseline['approved_assets']))
    savegame_hashes = hashes([name for name in before_inventory if name.startswith('Saved/SaveGames/')])
    before_hashes.update(savegame_hashes)
    report = {'schema': 2, 'mode': mode, 'run': run, 'complete': False,
              'asset_saves': 0, 'compile': 0, 'compile_requested': 0, 'cpp_build': False, 'PIE': False,
              'baseline': str(BASELINE), 'baseline_sha256': hud.sha(BASELINE),
              'script_sha256': hud.sha(PROJECT / SELF), 'assets': {}, 'failures': [],
              'protected_before': before_hashes, 'savegames_before': savegame_hashes,
              'inventory_file_count': len(before_inventory),
              'scope': 'Inspect three; mutate/save only W_QuickBarSlot and W_QuickBar. W_WeaponAmmoAndName remains unchanged.',
              'graph_scope': 'ListGraphs excludes transient graphs. Full Blueprint export also preserves its exported author data.',
              'pin_scope': 'Structured ListAllPins contains visible K2 pins; graph/Blueprint T3D retains exported hidden pin/type/default data.',
              'runtime_verified': False, 'manual_gameplay_and_visual_verification': 'User responsibility; no automatic PIE.'}
    try:
        check_parent_baseline(baseline, changed=(mode == 'verify'))
        reference = None
        if mode != 'inspect':
            previous_run = argument(command, 'DCWeaponAmmoSetupReference', '[A-Za-z0-9_]+')
            assert previous_run != run
            reference_file = REPORTS / previous_run / 'report.json'
            reference = json.loads(reference_file.read_text(encoding='utf-8-sig'))
            assert reference['complete'] and reference['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert reference['baseline_sha256'] == report['baseline_sha256']
            report['reference'] = {'run': previous_run, 'report_sha256': hud.sha(reference_file)}
            if mode == 'verify':
                assert hashes(reference['asset_sha256']) == reference['asset_sha256'], 'Applied asset files changed before verify'
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings_class, 'Blueprint editor settings class unavailable'
        settings = hud.export(ue.get_default_object(settings_class), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never'], 'SaveOnCompile must already be Never'
        report['save_on_compile'] = 'SoC_Never (read only; not changed)'
        report['original_source_reference'] = reference_source()
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet')
        assert umg_class, 'Enable the existing UMGToolSet plugin for this process'
        umg = ue.get_default_object(umg_class)
        assets = {}
        for name, package in ASSETS.items():
            bp = ue.load_asset(package)
            assert bp and bp.get_path_name() == package + '.' + name, 'Unexpected target asset: ' + package
            assets[name] = bp
            report['assets'][name] = snapshot(name, bp, umg, directory, 'before_' if mode == 'apply' else '')
        hud.clean()
        if mode == 'apply':
            for name in ASSETS:
                assert signature(report['assets'][name]) == signature(reference['assets'][name]), 'Graph changed after inspected approval'
            row_bp = assets['W_QuickBarSlot']
            _, widgets = hud.tree(umg, row_bp)
            _, source_widgets = hud.tree(umg, assets['W_WeaponAmmoAndName'])
            event = ue.BlueprintGraphEditor.get_graph_editor_by_name(row_bp, 'EventGraph')
            assert event.add_member_variable('DCAllowUpdates', ue.BlueprintEditorLibrary.get_basic_type_by_name('bool'), 'true')
            edit_row_tree(row_bp, umg, widgets, source_widgets['WeaponNameWidget'])
            compile_widget(umg, row_bp, report)
            create_clear_row(row_bp)
            changes = edit_row_graphs(row_bp)
            compile_widget(umg, row_bp, report)
            fit_font_tree(row_bp, umg)
            compile_widget(umg, row_bp, report)
            font_changes = fit_font_graphs(row_bp)
            compile_widget(umg, row_bp, report)
            for graph_name, pins in font_changes.items():
                changes.setdefault(graph_name, []).extend(pins)
            report['font_profile'] = FONT_PROFILE
            _, panel_widgets = hud.tree(umg, assets['W_QuickBar'])
            overrides = edit_panel_tree(assets['W_QuickBar'], umg, panel_widgets)
            compile_widget(umg, assets['W_QuickBar'], report)
            edit_panel_graphs(assets['W_QuickBar'], row_bp.generated_class())
            compile_widget(umg, assets['W_QuickBar'], report)
            report['approved_pin_changes'] = changes
            report['layout_contract'] = check_layout(assets, umg, overrides, FONT_PROFILE)
            report['after'] = {name: snapshot(name, assets[name], umg, directory, 'compiled_') for name in CHANGED}
            report['preservation'] = {}
            for name in CHANGED:
                preserve_original_graphs(report['assets'][name], report['after'][name], changes if name == 'W_QuickBarSlot' else {})
                report['preservation'][name] = preserve_animation_and_defaults(report['assets'][name], report['after'][name], name, FONT_PROFILE)
            hud.clean(ASSETS[name] for name in CHANGED)
            check_parent_baseline(baseline)  # Disk still equals Before_A, before the first asset save.
            hud.log_check(log)
            assert hashes(before_hashes) == before_hashes
            for name in CHANGED:
                assert ue.EditorAssetLibrary.save_loaded_asset(assets[name], only_if_is_dirty=False), 'Explicit asset save failed: ' + name
                report['asset_saves'] += 1
            report['asset_sha256'] = hashes(sorted(WRITABLE))
            report['changed_assets'] = [ASSETS[name] for name in CHANGED]
        elif mode == 'verify':
            profile = reference.get('font_profile', 'legacy')
            report['font_profile'] = profile
            report['layout_contract'] = check_layout(assets, umg, reference['layout_contract']['original_slot_overrides'], profile)
            assert report['layout_contract'] == reference['layout_contract']
            for name in CHANGED:
                assert signature(report['assets'][name], profile) == signature(reference['after'][name], profile), 'Reloaded graph differs: ' + name
                report.setdefault('preservation', {})[name] = preserve_animation_and_defaults(reference['assets'][name], report['assets'][name], name, profile)
            report['asset_sha256'] = hashes(sorted(WRITABLE))
            assert report['asset_sha256'] == reference['asset_sha256']
        hud.clean()
        hud.log_check(log)
    except Exception:
        report['failures'].append(traceback.format_exc())
    finally:
        try:
            hud.clean(ASSETS[name] for name in CHANGED) if mode == 'apply' and report['failures'] else hud.clean()
            ignored = WRITABLE if mode == 'apply' else set()
            assert {key: value for key, value in inventory().items() if key not in ignored} == {
                key: value for key, value in before_inventory.items() if key not in ignored}, 'Project file inventory changed outside approved asset saves'
            expected_hashes = {key: value for key, value in before_hashes.items() if key not in ignored}
            assert hashes(expected_hashes) == expected_hashes, 'Protected file hash changed during setup'
            check_parent_baseline(baseline, changed=(mode == 'verify' or report['asset_saves'] > 0))
            assert hud.sha(BASELINE) == report['baseline_sha256'], 'Before_A diagnostic baseline changed'
            report['protection_unchanged'] = True
        except Exception:
            report['protection_unchanged'] = False
            report['failures'].append(traceback.format_exc())
        report['complete'] = not report['failures'] and len(report['assets']) == len(ASSETS)
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Weapon/ammo ' + mode + ' failed; see ' + str(directory / 'report.json')
    ue.log('Weapon/ammo ' + mode + ' complete; saves=' + str(report['asset_saves']) + ' compile=' + str(report['compile']) + '; ' + str(directory / 'report.json'))


if __name__ == '__main__':
    main()
