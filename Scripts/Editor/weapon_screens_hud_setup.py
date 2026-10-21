"""Approved Screens/Paperlogy weapon HUD replacement; no PIE or C++ build.

Only new W_DC_WeaponRow/W_DC_WeaponPanel and one LAS Equipment WidgetClass
may be saved. Existing user-edited Lyra widgets are protected, never copied.
--commands prints separate native command arrays without launching them.
Inspect, apply and fresh verify are separate native processes. A failed or
partial apply is reported for review and is never resumed or overwritten.
"""
import hashlib
import importlib.util
import json
from decimal import Decimal
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/WeaponScreensHUD20261022'
SCRIPT = P / 'Scripts/Editor/weapon_screens_hud_setup.py'
BASELINE = R / 'author_baseline.json'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
ASSETS = {'row': '/Game/DreamCatcher/UI/HUD/W_DC_WeaponRow',
          'panel': '/Game/DreamCatcher/UI/HUD/W_DC_WeaponPanel'}
LAS = '/Game/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD'
SLOT_TAG = '(TagName="HUD.Slot.Equipment")'
ART_ROOT = '/Game/DreamCatcher/Screens/InGame_UI/'
ART = {key: ART_ROOT + name for key, name in {'main_on': 'Main_GUN_ON', 'main_off': 'Main_GUN_OFF',
       'sub_on': 'Sub_GUN_ON', 'sub_off': 'Sub_GUN_OFF', 'button1': 'Button_1', 'button2': 'Button_2'}.items()}
FONTS = {'magazine': '/Game/DreamCatcher/UI/Font/Paperlogy-7Bold_Font',
         'slash': '/Game/DreamCatcher/UI/Font/Paperlogy-7Bold_Font',
         'spare': '/Game/DreamCatcher/UI/Font/Paperlogy-5Medium_Font',
         'name': '/Game/DreamCatcher/UI/Font/Paperlogy-6SemiBold_Font'}
FONT_SIZES = {'magazine': 32, 'slash': 16, 'spare': 14, 'name': 14}
INPUTS = ['/Game/DreamCatcher/Lyra/UI/Shared/Equipment/Input/Actions/IA_QuickSlot' + str(i) for i in (1, 2)]
REPAIR_RUN = 'Screens_E_apply'
REPAIR_SCRIPT_SHA = 'b1f20d764f90eda7111573d09b869f329ef6e476ee52bed4411b8ef8f4fda19c'
REPAIR_REPORT_SHA = '936b0552dc19a56515a2f692051db820d48dfe97fff4aa04687d2add65f4c207'


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
    spec = importlib.util.spec_from_file_location('weapon_screens_' + name, SCRIPT.parent / (name + '.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def protection(allow_registration=False):
    baseline = read(BASELINE)
    assert baseline['user_editor_closed'] and not baseline['PIE'] and not baseline['cpp_build']
    allowed = {disk(value) for value in ASSETS.values()}
    if allow_registration: allowed.add(disk(LAS))
    protected = {}
    for name, expected in baseline['files'].items():
        item = P / name.replace('\\', '/')
        if item in allowed: continue
        actual = sha(item)
        assert actual == expected, 'Approved current user file changed: ' + name
        protected[name.replace('\\', '/')] = actual
    inventory = {}
    for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for item in (P / top).rglob('*'):
            if item.is_file() and item not in allowed:
                stat = item.stat()
                inventory[item.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    return {'sha256': protected, 'inventory': inventory, 'baseline_sha256': sha(BASELINE)}


def commands(prefix, repair=False):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    values = {}
    modes = ('repair', 'verify') if repair else ('inspect', 'apply', 'verify')
    for mode in modes:
        run = prefix + '_' + mode
        folder = R / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-run=pythonscript',
            '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
            '-EnablePlugins=UMGToolSet', '-nullrhi', '-nosound', '-DCWeaponScreensMode=' + mode,
            '-DCWeaponScreensRun=' + run, '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode != 'inspect':
            reference = REPAIR_RUN if mode == 'repair' else prefix + ('_repair' if repair else '_inspect' if mode == 'apply' else '_apply')
            args.append('-DCWeaponScreensReference=' + reference)
        values[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': ' '.join('"' + item + '"' if ' ' in item else item for item in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False,
                      'available_modes': list(modes), 'commands': values}, indent=2))


def arg(command, name, pattern):
    found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
    assert len(found) == 1, 'Use one explicit ' + name
    return found[0]


def inspect_assets(folder, report):
    row_parent = ue.load_class(None, '/Script/UMG.UserWidget')
    panel_parent = ue.load_class(None, '/Script/DreamCatcher.DCLyraTaggedWidget')
    assert row_parent and panel_parent
    report['native_parents'] = {'row': row_parent.get_path_name(), 'panel': panel_parent.get_path_name()}
    for package in ASSETS.values():
        assert not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package), 'New HUD asset already exists'
    report['resources'] = {}
    for index, (role, package) in enumerate(ART.items()):
        obj = ue.load_asset(package); assert obj and obj.get_class().get_name() == 'Texture2D'
        size = [obj.blueprint_get_size_x(), obj.blueprint_get_size_y()]
        expected = [24, 24] if role.startswith('button') else [316, 76 if role.endswith('on') else 36]
        assert size == expected, (package, size)
        report['resources'][role] = {'path': obj.get_path_name(), 'size': size, 'sha256': sha(disk(package)),
            'raw': hud.export(obj, folder / ('texture_' + str(index).zfill(2) + '.t3d'))}
    report['fonts'] = {}
    for role, package in FONTS.items():
        obj = ue.load_asset(package); assert obj and obj.get_class().get_name() == 'Font'
        report['fonts'][role] = {'path': obj.get_path_name(), 'display_size': FONT_SIZES[role],
            'sha256': sha(disk(package)), 'raw': hud.export(obj, folder / ('font_' + role + '.t3d'))}
    for index, package in enumerate(INPUTS):
        obj = ue.load_asset(package); assert obj and obj.get_class().get_name() == 'InputAction'
        report.setdefault('functional_inputs', []).append({'slot_index': index, 'path': obj.get_path_name(),
            'raw': hud.export(obj, folder / ('input_' + str(index) + '.t3d'))})
    registration = ue.load_asset(LAS); assert registration
    actions = list(registration.get_editor_property('Actions'))
    report['registration_raw'] = hud.export(registration, folder / 'registration.t3d')
    matches = []
    report['registration_actions'] = []
    for action_index, action in enumerate(actions):
        item = {'path': action.get_path_name(), 'class': action.get_class().get_path_name()}
        if 'AddWidgets' in action.get_class().get_name():
            widgets = list(action.get_editor_property('Widgets'))
            item['widgets'] = [value.export_text() for value in widgets]
            for index, value in enumerate(widgets):
                if value.get_editor_property('SlotID').export_text() == SLOT_TAG:
                    matches.append([action_index, index])
        report['registration_actions'].append(item)
    assert len(matches) == 1, 'Expected one Equipment registration'
    report['equipment_entry'] = matches[0]
    umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_class
    report['editor_api'] = {'UMGToolSet': umg_class.get_path_name()}
    report['layout_contract'] = {'width': 347, 'on_card': [31, 0, 316, 76], 'off_card': [30, 0, 316, 36],
        'on_button': [0, 5, 24, 24], 'off_button': [0, 6, 24, 24], 'row2_y_main': 71, 'row2_y_sub': 33,
        'main_size': [347, 107], 'sub_size': [347, 109], 'transition_seconds': .25,
        'transition': 'EaseInOut; allocated row height and opacity/positions; fixed-size card images clipped, never rescaled.',
        'empty': 'External panel stays ticking; only empty rows collapse. No writes to gameplay slot3.'}


# The following helpers author only the two explicitly approved widget BPs.
def pin(node, name):
    return helpers.pin(node, name)


def wire(start, end):
    helpers.wire(start, end)


def literal(node, name, value):
    helpers.literal(node, name, str(value).lower() if isinstance(value, bool) else value)


def basic(name):
    return ue.BlueprintEditorLibrary.get_basic_type_by_name(name)


def object_type(name):
    cls = ue.load_class(None, name); assert cls
    return ue.BlueprintEditorLibrary.get_object_reference_type(cls)


def vector_type():
    value = ue.load_object(None, '/Script/CoreUObject.Vector2D'); assert value
    return ue.BlueprintEditorLibrary.get_struct_type(value)


class Graph:
    def __init__(ctx, bp, name, inputs=()):
        ctx.e = ue.BlueprintGraphEditor.create_and_edit_function_graph(bp, name)
        assert ctx.e
        ctx.inputs = {key: ctx.e.add_graph_input_parameter(key, typ, default) for key, typ, default in inputs}
        ctx.tail = ctx.e.find_graph_entry_pin()

    def node(ctx, function, **values):
        result = ctx.e.add_call_function_node(function); assert result, function
        for key, value in values.items():
            if isinstance(value, ue.BlueprintGraphPin): wire(value, pin(result, key))
        for key, value in values.items():
            if not isinstance(value, ue.BlueprintGraphPin): literal(result, key, value)
        return result

    def call(ctx, function, **values):
        result = ctx.node(function, **values)
        names = {str(value.get_pin_name()) for value in ue.BlueprintEditorLibrary.list_all_pins(result)}
        if 'execute' in names:
            wire(ctx.tail, pin(result, 'execute'))
            ctx.tail = pin(result, 'then')
        return result

    def pure(ctx, name, **values):
        return pin(ctx.node('/Script/Engine.KismetMathLibrary.' + name, **values), 'ReturnValue')

    def get(ctx, name, target=None, cls=''):
        node = ctx.e.add_get_member_variable_node(name, cls); assert node, name
        if target is not None: wire(target, pin(node, 'self'))
        return pin(node, name)

    def set(ctx, name, value):
        node = ctx.e.add_set_member_variable_node(name); assert node, name
        wire(ctx.tail, pin(node, 'execute'))
        if isinstance(value, ue.BlueprintGraphPin): wire(value, pin(node, name))
        else: literal(node, name, value)
        ctx.tail = pin(node, 'then')

    def branch(ctx, condition):
        node = ctx.e.add_branch_node(); assert node
        wire(ctx.tail, pin(node, 'execute')); wire(condition, pin(node, 'Condition'))
        return pin(node, 'then'), pin(node, 'else')

    def widget(ctx, name, function, **values):
        return ctx.call('/Script/UMG.' + function, self=ctx.get(name), **values)

    def finish(ctx, **outputs):
        result = None
        for name, (typ, value) in outputs.items():
            result = ctx.e.add_graph_output_parameter(name, typ)
            if isinstance(value, ue.BlueprintGraphPin): wire(value, pin(result, name))
            else: literal(result, name, value)
        if result is None: result = ctx.e.add_return_node()
        assert result
        wire(ctx.tail, pin(result, 'execute'))


def add_member(bp, editor, name, typ, default, editable=False):
    assert editor.add_member_variable(name, typ, default)
    ue.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(bp, name, editable)


def create_widget(key, parent):
    package = ASSETS[key]
    assert not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package)
    bp = UMG.call_method('CreateWidgetBlueprint', args=(package.rsplit('/', 1)[0], package.rsplit('/', 1)[1], parent))
    assert bp
    for value in UMG.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets'):
        if not value.get_editor_property('parent'):
            assert UMG.call_method('RemoveWidget', args=(bp, value.get_editor_property('widget')))
            break
    return bp


def widget(bp, cls, name, parent=None):
    value = UMG.call_method('AddWidget', args=(bp, cls, name, parent, -1)).get_editor_property('widget')
    assert value
    UMG.call_method('ToggleWidgetAsVariable', args=(bp, value, True))
    return value


def place(value, x, y, width, height, auto=False):
    slot = value.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot)
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(0, 0)))
    slot.set_alignment(ue.Vector2D(0, 0)); slot.set_position(ue.Vector2D(x, y))
    slot.set_size(ue.Vector2D(width, height)); slot.set_auto_size(auto)


def font_text(bp, parent, name, role, text, bounds, fonts):
    value = widget(bp, ue.TextBlock, name, parent)
    value.set_text(text)
    font = value.get_editor_property('font')
    font.set_editor_property('FontObject', fonts[role]); font.set_editor_property('TypefaceFontName', 'Default')
    font.set_editor_property('FontMaterial', None)
    value.set_font(font)
    value.set_font_size(FONT_SIZES[role])  # Native TextBlock API; never changed by runtime graphs.
    value.set_color_and_opacity(ue.SlateColor(specified_color=ue.LinearColor(0, 0, 0, 1)))
    value.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    if role == 'name':
        value.set_text_overflow_policy(ue.TextOverflowPolicy.ELLIPSIS)
        value.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS)
    place(value, *bounds)
    return value


def row_tree(bp, resources, fonts):
    size = widget(bp, ue.SizeBox, 'RowSize')
    size.set_width_override(347); size.set_height_override(76)
    size.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS)
    canvas = widget(bp, ue.CanvasPanel, 'RowCanvas', size)
    for name, role, bounds in (('OnCard', 'main_on', (31, 0, 316, 76)),
                              ('OffCard', 'main_off', (31, 0, 316, 36)),
                              ('NumberButton', 'button1', (0, 5, 24, 24))):
        image = widget(bp, ue.Image, name, canvas)
        image.set_brush_from_texture(resources[role], True)
        image.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
        if name == 'OffCard': image.set_render_opacity(0)
        place(image, *bounds)
    font_text(bp, canvas, 'WeaponName', 'name', 'MPX', (43, 7, 135, 21), fonts)
    font_text(bp, canvas, 'Magazine', 'magazine', '30', (43, 27, 70, 48), fonts)
    font_text(bp, canvas, 'Slash', 'slash', '/', (113, 44, 14, 24), fonts)
    font_text(bp, canvas, 'Spare', 'spare', '999', (130, 46, 46, 22), fonts)


ROW_VECTORS = {'OnNamePosition': (43, 7), 'OffNamePosition': (40, 6),
    'OnMagazinePosition': (43, 27), 'OffMagazinePosition': (185, -8),
    'OnSlashPosition': (113, 44), 'OffSlashPosition': (256, 9),
    'OnSparePosition': (130, 46), 'OffSparePosition': (273, 11)}


def row_members(bp):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph'); assert editor
    for name, value in {'TransitionDuration': .25, 'CurrentAlpha': 1, 'StartAlpha': 1,
                        'TargetAlpha': 1, 'TransitionElapsed': .25, 'AllocatedHeight': 76,
                        'OnHeight': 76, 'OffHeight': 36, 'OnCardX': 31, 'OffCardX': 30,
                        'OnButtonY': 5, 'OffButtonY': 6}.items():
        add_member(bp, editor, name, basic('real'), str(value), name in {
            'TransitionDuration', 'OnHeight', 'OffHeight', 'OnCardX', 'OffCardX', 'OnButtonY', 'OffButtonY'})
    for name in ('HasItem', 'HasState', 'SelectedState', 'DesignPreview'):
        add_member(bp, editor, name, basic('bool'), 'false')
    add_member(bp, editor, 'PreviewSelected', basic('bool'), 'true', True)
    add_member(bp, editor, 'SlotIndex', basic('int'), '0', True)
    for name, package in (('TextureOn', ART['main_on']), ('TextureOff', ART['main_off']), ('ButtonTexture', ART['button1'])):
        add_member(bp, editor, name, object_type('/Script/Engine.Texture2D'), package + '.' + package.rsplit('/', 1)[1], True)
    add_member(bp, editor, 'SlotInputAction', object_type('/Script/EnhancedInput.InputAction'), INPUTS[0] + '.IA_QuickSlot1', True)
    for name, (x, y) in ROW_VECTORS.items():
        add_member(bp, editor, name, vector_type(), '(X=%s,Y=%s)' % (x, y), True)


def lerp_vector(g, off, on, alpha):
    left = g.node('/Script/Engine.KismetMathLibrary.BreakVector2D', InVec=off)
    right = g.node('/Script/Engine.KismetMathLibrary.BreakVector2D', InVec=on)
    x = g.pure('Lerp', A=pin(left, 'X'), B=pin(right, 'X'), Alpha=alpha)
    y = g.pure('Lerp', A=pin(left, 'Y'), B=pin(right, 'Y'), Alpha=alpha)
    return g.pure('MakeVector2D', X=x, Y=y)


def row_transition_graphs(bp):
    g = Graph(bp, 'ComputeLayout', [('InAlpha', basic('real'), '1')])
    alpha = g.inputs['InAlpha']
    outputs = {name: (basic('real'), g.pure('Lerp', A=g.get(off), B=g.get(on), Alpha=alpha))
        for name, off, on in [('Height', 'OffHeight', 'OnHeight'), ('CardX', 'OffCardX', 'OnCardX'),
                             ('ButtonY', 'OffButtonY', 'OnButtonY')]}
    outputs['OnOpacity'] = (basic('real'), alpha)
    outputs['OffOpacity'] = (basic('real'), g.pure('Subtract_DoubleDouble', A=1, B=alpha))
    for role in ('Name', 'Magazine', 'Slash', 'Spare'):
        outputs[role + 'Position'] = (vector_type(), lerp_vector(g, g.get('Off' + role + 'Position'), g.get('On' + role + 'Position'), alpha))
    g.finish(**outputs)
    g = Graph(bp, 'RequestSelection', [('InSelected', basic('bool'), 'false')])
    changed = g.pure('NotEqual_BoolBool', A=g.get('SelectedState'), B=g.inputs['InSelected'])
    yes, no = g.branch(changed)
    g.tail = yes
    g.set('StartAlpha', g.get('CurrentAlpha'))
    g.set('TargetAlpha', g.pure('Conv_BoolToDouble', InBool=g.inputs['InSelected']))
    g.set('TransitionElapsed', 0); g.set('SelectedState', g.inputs['InSelected'])
    g.tail = [g.tail, no]; g.finish()
    g = Graph(bp, 'AdvanceTransition', [('DeltaSeconds', basic('real'), '0')])
    elapsed = g.pure('Add_DoubleDouble', A=g.get('TransitionElapsed'), B=g.inputs['DeltaSeconds'])
    g.set('TransitionElapsed', elapsed)
    duration = g.pure('FMax', A=g.get('TransitionDuration'), B=.001)
    ratio = g.pure('FClamp', Value=g.pure('Divide_DoubleDouble', A=g.get('TransitionElapsed'), B=duration), Min=0, Max=1)
    eased = g.pure('Ease', A=0, B=1, Alpha=ratio, EasingFunc='EaseInOut', BlendExp=2, Steps=2)
    g.set('CurrentAlpha', g.pure('Lerp', A=g.get('StartAlpha'), B=g.get('TargetAlpha'), Alpha=eased))
    g.finish(Alpha=(basic('real'), g.get('CurrentAlpha')))


def canvas_position(g, target, vector):
    slot = g.call('/Script/UMG.WidgetLayoutLibrary.SlotAsCanvasSlot', Widget=target)
    g.call('/Script/UMG.CanvasPanelSlot.SetPosition', self=pin(slot, 'ReturnValue'), InPosition=vector)


def row_visual_graph(bp):
    g = Graph(bp, 'ApplyVisuals')
    layout = g.call('ComputeLayout', InAlpha=g.get('CurrentAlpha'))
    g.set('AllocatedHeight', pin(layout, 'Height'))
    g.widget('RowSize', 'SizeBox.SetHeightOverride', InHeightOverride=pin(layout, 'Height'))
    for name, output in (('OnCard', 'OnOpacity'), ('OffCard', 'OffOpacity')):
        g.widget(name, 'Widget.SetRenderOpacity', InOpacity=pin(layout, output))
        canvas_position(g, g.get(name), g.pure('MakeVector2D', X=pin(layout, 'CardX'), Y=0))
    canvas_position(g, g.get('NumberButton'), g.pure('MakeVector2D', X=0, Y=pin(layout, 'ButtonY')))
    for name, output in (('WeaponName', 'NamePosition'), ('Magazine', 'MagazinePosition'),
                         ('Slash', 'SlashPosition'), ('Spare', 'SparePosition')):
        canvas_position(g, g.get(name), pin(layout, output))
    yes, no = g.branch(g.get('HasItem'))
    g.tail = yes; g.widget('RowSize', 'Widget.SetVisibility', InVisibility='Visible'); a = g.tail
    g.tail = no; g.widget('RowSize', 'Widget.SetVisibility', InVisibility='Collapsed')
    g.tail = [a, g.tail]; g.finish()


def set_text(g, widget_name, value):
    g.widget(widget_name, 'TextBlock.SetText', InText=value)


def valid(g, value):
    return pin(g.node('/Script/Engine.KismetSystemLibrary.IsValid', Object=value), 'ReturnValue')


def row_data_graph(bp):
    g = Graph(bp, 'UpdateData', [('InItem', object_type('/Script/DreamCatcher.DCInventoryItemInstance'), 'None'),
                               ('InSelected', basic('bool'), 'false')])
    yes, no = g.branch(valid(g, g.inputs['InItem']))
    g.tail = no
    for name in ('HasItem', 'HasState', 'SelectedState'): g.set(name, False)
    for name in ('CurrentAlpha', 'StartAlpha', 'TargetAlpha'): g.set(name, 0)
    g.set('TransitionElapsed', g.get('TransitionDuration'))
    set_text(g, 'WeaponName', ''); set_text(g, 'Magazine', '0'); set_text(g, 'Spare', '0')
    empty_end = g.tail
    g.tail = yes; g.set('HasItem', True)
    existing, first = g.branch(g.get('HasState'))
    g.tail = existing; g.call('RequestSelection', InSelected=g.inputs['InSelected']); old_end = g.tail
    g.tail = first
    alpha = g.pure('Conv_BoolToDouble', InBool=g.inputs['InSelected'])
    for name in ('CurrentAlpha', 'StartAlpha', 'TargetAlpha'): g.set(name, alpha)
    g.set('SelectedState', g.inputs['InSelected']); g.set('TransitionElapsed', g.get('TransitionDuration')); g.set('HasState', True)
    g.tail = [old_end, g.tail]
    fragment = g.call('/Script/DreamCatcher.DCInventoryItemInstance.FindFragmentByClass', self=g.inputs['InItem'],
                      FragmentClass='/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon')
    has_name, no_name = g.branch(valid(g, pin(fragment, 'ReturnValue')))
    g.tail = has_name
    name = g.get('DisplayNameWhenEquipped', pin(fragment, 'ReturnValue'), '/Script/DreamCatcher.DCInventoryFragment_QuickBarIcon')
    set_text(g, 'WeaponName', name); named_end = g.tail
    g.tail = no_name; set_text(g, 'WeaponName', ''); g.tail = [named_end, g.tail]
    for widget_name, tag in (('Magazine', 'Lyra.ShooterGame.Weapon.MagazineAmmo'),
                             ('Spare', 'Lyra.ShooterGame.Weapon.SpareAmmo')):
        amount = g.call('/Script/DreamCatcher.DCInventoryItemInstance.GetStatTagStackCount', self=g.inputs['InItem'],
                        Tag='(TagName="' + tag + '")')
        text = g.call('/Script/Engine.KismetTextLibrary.Conv_IntToText', Value=pin(amount, 'ReturnValue'), bUseGrouping=False)
        set_text(g, widget_name, pin(text, 'ReturnValue'))
    g.tail = [empty_end, g.tail]; g.finish()


def row_preview(bp):
    g = Graph(bp, 'PreparePreview')
    g.set('HasItem', True)
    g.set('CurrentAlpha', g.pure('Conv_BoolToDouble', InBool=g.get('PreviewSelected')))
    g.call('ApplyVisuals'); g.finish()


def row_construct(bp):
    event = ue.BlueprintEditorLibrary.add_event_override(bp, 'PreConstruct', ue.IntPoint(0, 0)); assert event
    g = Graph.__new__(Graph); g.e = ue.BlueprintGraphEditor.get_graph_editor(event.get_outer()); g.tail = pin(event, 'then')
    for widget_name, variable in (('OnCard', 'TextureOn'), ('OffCard', 'TextureOff'), ('NumberButton', 'ButtonTexture')):
        g.widget(widget_name, 'Image.SetBrushFromTexture', Texture=g.get(variable), bMatchSize=False)
    design, runtime = g.branch(pin(event, 'IsDesignTime'))
    g.tail = design
    g.call('PreparePreview')
    g.tail = [g.tail, runtime]
    g.call('ApplyVisuals')


def row_click(bp):
    graph = ue.BlueprintEditorLibrary.add_function_override(bp, 'OnMouseButtonDown'); assert graph
    g = Graph.__new__(Graph); g.e = ue.BlueprintGraphEditor.get_graph_editor(graph)
    nodes = list(g.e.list_all_nodes())
    entry = [node for node in nodes if node.get_class().get_name() == 'K2Node_FunctionEntry']; assert len(entry) == 1
    parents = [node for node in nodes if node.get_class().get_name() == 'K2Node_CallParentFunction']
    results = [node for node in nodes if node.get_class().get_name() == 'K2Node_FunctionResult']; assert len(results) == 1
    assert len(parents) <= 1
    g.tail = g.e.find_graph_entry_pin(); assert g.tail.break_pin_links()
    # Native UserWidget declares this as BlueprintImplementableEvent, with no
    # callable parent implementation. Retain the ordinary unhandled response.
    if parents: g.e.remove_nodes(parents)
    reply = g.node('/Script/UMG.WidgetBlueprintLibrary.Unhandled')
    wire(pin(reply, 'ReturnValue'), pin(results[0], 'ReturnValue'))
    unhandled = pin(results[0], 'execute')
    pc = pin(g.node('/Script/UMG.Widget.GetOwningPlayer'), 'ReturnValue')
    mouse = g.node('/Script/Engine.KismetInputLibrary.PointerEvent_GetEffectingButton', Input=pin(entry[0], 'MouseEvent'))
    left = pin(g.node('/Script/Engine.KismetInputLibrary.EqualEqual_KeyKey', A=pin(mouse, 'ReturnValue'), B='LeftMouseButton'), 'ReturnValue')
    allowed = g.pure('BooleanAND', A=g.get('HasItem'), B=valid(g, pc))
    allowed = g.pure('BooleanAND', A=allowed, B=valid(g, g.get('SlotInputAction')))
    allowed = g.pure('BooleanAND', A=allowed, B=left)
    yes, no = g.branch(allowed); wire(no, unhandled); g.tail = yes
    subsystem = g.e.create_node_from_name('LocalPlayerSubsystems|GetEnhancedInputLocalPlayerSubsystem', ue.Vector2D(0, 0), [])
    assert subsystem
    yes, no = g.branch(valid(g, pin(subsystem, 'ReturnValue'))); wire(no, unhandled); g.tail = yes
    value = g.node('/Script/EnhancedInput.EnhancedInputLibrary.MakeInputActionValueOfType', X=1, Y=0, Z=0, ValueType='Boolean')
    g.call('/Script/EnhancedInput.EnhancedInputSubsystemInterface.InjectInputForAction', self=pin(subsystem, 'ReturnValue'),
           Action=g.get('SlotInputAction'), RawValue=pin(value, 'ReturnValue'))
    handled = g.node('/Script/UMG.WidgetBlueprintLibrary.Handled')
    result = g.e.add_return_node(); assert result
    wire(g.tail, pin(result, 'execute')); wire(pin(handled, 'ReturnValue'), pin(result, 'ReturnValue'))


def compile_widget(bp, report):
    report['compile_requested'].append(bp.get_path_name())
    assert UMG.call_method('CompileWidgetBlueprint', args=(bp,)), bp.get_path_name()
    assert 'BS_UP_TO_DATE' in str(bp.get_editor_property('status')), str(bp.get_editor_property('status'))


def create_row(resources, fonts, report):
    bp = create_widget('row', ue.load_class(None, '/Script/UMG.UserWidget'))
    row_tree(bp, resources, fonts); row_members(bp)
    row_transition_graphs(bp); row_visual_graph(bp); row_data_graph(bp); row_preview(bp); row_construct(bp); row_click(bp)
    ue.get_default_object(bp.generated_class()).set_editor_property('bIsFocusable', False)
    compile_widget(bp, report)
    return bp


def panel_tree(bp, row):
    size = widget(bp, ue.SizeBox, 'PanelSize')
    size.set_width_override(347); size.set_height_override(107)
    size.set_visibility(ue.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    canvas = widget(bp, ue.CanvasPanel, 'PanelCanvas', size)
    for index in range(2):
        child = widget(bp, row.generated_class(), 'Row' + str(index + 1), canvas)
        place(child, 0, 0 if index == 0 else 71, 347, 76 if index == 0 else 36, True)
        child.set_editor_property('SlotIndex', index)
        child.set_editor_property('SlotInputAction', ue.load_asset(INPUTS[index]))
        child.set_editor_property('PreviewSelected', index == 0)
        child.set_editor_property('CurrentAlpha', 1 if index == 0 else 0)
        for field, role in (('TextureOn', 'main_on' if index == 0 else 'sub_on'),
                             ('TextureOff', 'main_off' if index == 0 else 'sub_off'),
                             ('ButtonTexture', 'button1' if index == 0 else 'button2')):
            child.set_editor_property(field, ue.load_asset(ART[role]))


def panel_math(bp):
    inputs = [('FirstHeight', basic('real'), '76'), ('SecondHeight', basic('real'), '36'),
              ('FirstHasItem', basic('bool'), 'true'), ('SecondHasItem', basic('bool'), 'true'),
              ('FirstAlpha', basic('real'), '1')]
    g = Graph(bp, 'ComputePanelLayout', inputs)
    y = g.pure('Lerp', A=33, B=71, Alpha=g.inputs['FirstAlpha'])
    y = g.pure('SelectFloat', A=y, B=0, bPickA=g.inputs['FirstHasItem'])
    first = g.pure('SelectFloat', A=g.inputs['FirstHeight'], B=0, bPickA=g.inputs['FirstHasItem'])
    second = g.pure('Add_DoubleDouble', A=y, B=g.inputs['SecondHeight'])
    second = g.pure('SelectFloat', A=second, B=0, bPickA=g.inputs['SecondHasItem'])
    height = g.pure('FMax', A=1, B=g.pure('FMax', A=first, B=second))
    any_item = g.pure('BooleanOR', A=g.inputs['FirstHasItem'], B=g.inputs['SecondHasItem'])
    g.finish(Row2Y=(basic('real'), y), Height=(basic('real'), height),
             Opacity=(basic('real'), g.pure('Conv_BoolToDouble', InBool=any_item)))


def panel_layout(bp, row):
    g = Graph(bp, 'ApplyPanelLayout')
    cls = row.generated_class().get_path_name()
    layout = g.call('ComputePanelLayout', FirstHeight=g.get('AllocatedHeight', g.get('Row1'), cls),
        SecondHeight=g.get('AllocatedHeight', g.get('Row2'), cls), FirstHasItem=g.get('HasItem', g.get('Row1'), cls),
        SecondHasItem=g.get('HasItem', g.get('Row2'), cls), FirstAlpha=g.get('CurrentAlpha', g.get('Row1'), cls))
    g.widget('PanelSize', 'SizeBox.SetHeightOverride', InHeightOverride=pin(layout, 'Height'))
    canvas_position(g, g.get('Row2'), g.pure('MakeVector2D', X=0, Y=pin(layout, 'Row2Y')))
    # Opacity does not suspend Tick. Never hide/collapse the external panel.
    g.call('/Script/UMG.Widget.SetRenderOpacity', InOpacity=pin(layout, 'Opacity'))
    g.finish()


def panel_refresh(bp, row):
    g = Graph(bp, 'Refresh', [('DeltaSeconds', basic('real'), '0')])
    clear = []
    pc = pin(g.node('/Script/UMG.Widget.GetOwningPlayer'), 'ReturnValue')
    pawn = pin(g.node('/Script/UMG.UserWidget.GetOwningPlayerPawn'), 'ReturnValue')
    good, bad = g.branch(g.pure('BooleanAND', A=valid(g, pc), B=valid(g, pawn)))
    clear.append(bad); g.tail = good
    component = g.call('/Script/Engine.Actor.GetComponentByClass', self=pc, ComponentClass='/Script/DreamCatcher.DCLyraQuickBarComponent')
    good, bad = g.branch(valid(g, pin(component, 'ReturnValue'))); clear.append(bad); g.tail = good
    health = g.call('/Script/DreamCatcher.DCLyraHealthComponent.FindHealthComponent', Actor=pawn)
    good, bad = g.branch(valid(g, pin(health, 'ReturnValue'))); clear.append(bad); g.tail = good
    maximum = g.call('/Script/DreamCatcher.DCLyraHealthComponent.GetMaxHealth', self=pin(health, 'ReturnValue'))
    good, bad = g.branch(g.pure('Greater_DoubleDouble', A=pin(maximum, 'ReturnValue'), B=0)); clear.append(bad); g.tail = good
    death = g.node('/Script/DreamCatcher.DCLyraHealthComponent.IsDeadOrDying', self=pin(health, 'ReturnValue'))
    wire(g.tail, pin(death, 'execute')); clear.append(pin(death, 'True')); g.tail = pin(death, 'False')
    slots = g.call('/Script/DreamCatcher.DCLyraQuickBarComponent.GetSlots', self=pin(component, 'ReturnValue'))
    active = g.call('/Script/DreamCatcher.DCLyraQuickBarComponent.GetActiveSlotIndex', self=pin(component, 'ReturnValue'))
    cls = row.generated_class().get_path_name()
    for index in range(2):
        valid_index = g.call('/Script/Engine.KismetArrayLibrary.Array_IsValidIndex', TargetArray=pin(slots, 'ReturnValue'), IndexToTest=index)
        yes, no = g.branch(pin(valid_index, 'ReturnValue'))
        g.tail = yes
        item = g.call('/Script/Engine.KismetArrayLibrary.Array_Get', TargetArray=pin(slots, 'ReturnValue'), Index=index)
        selected = g.pure('EqualEqual_IntInt', A=pin(active, 'ReturnValue'), B=index)
        g.call(cls + ':UpdateData', self=g.get('Row' + str(index + 1)), InItem=pin(item, 'Item'), InSelected=selected)
        item_end = g.tail
        g.tail = no; g.call(cls + ':UpdateData', self=g.get('Row' + str(index + 1)), InSelected=False)
        g.tail = [item_end, g.tail]
    ready = g.tail
    g.tail = clear
    for name in ('Row1', 'Row2'): g.call(cls + ':UpdateData', self=g.get(name), InSelected=False)
    g.tail = [ready, g.tail]
    for name in ('Row1', 'Row2'):
        g.call(cls + ':AdvanceTransition', self=g.get(name), DeltaSeconds=g.inputs['DeltaSeconds'])
        g.call(cls + ':ApplyVisuals', self=g.get(name))
    g.call('ApplyPanelLayout'); g.finish()


def panel_events(bp, row):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph'); assert editor
    add_member(bp, editor, 'DesignPreview', basic('bool'), 'false')
    event = ue.BlueprintEditorLibrary.add_event_override(bp, 'PreConstruct', ue.IntPoint(0, 0)); assert event
    g = Graph.__new__(Graph); g.e = ue.BlueprintGraphEditor.get_graph_editor(event.get_outer()); g.tail = pin(event, 'then')
    g.set('DesignPreview', pin(event, 'IsDesignTime'))
    yes, no = g.branch(g.get('DesignPreview'))
    g.tail = yes
    for name in ('Row1', 'Row2'):
        g.call(row.generated_class().get_path_name() + ':PreparePreview', self=g.get(name))
    g.call('ApplyPanelLayout')
    g.tail = no; g.call('/Script/UMG.Widget.SetRenderOpacity', InOpacity=0)
    event = ue.BlueprintEditorLibrary.add_event_override(bp, 'Tick', ue.IntPoint(0, 600)); assert event
    g = Graph.__new__(Graph); g.e = ue.BlueprintGraphEditor.get_graph_editor(event.get_outer()); g.tail = pin(event, 'then')
    yes, no = g.branch(g.get('DesignPreview'))
    g.tail = no; g.call('Refresh', DeltaSeconds=pin(event, 'InDeltaTime'))


def create_panel(row, report):
    bp = create_widget('panel', ue.load_class(None, '/Script/DreamCatcher.DCLyraTaggedWidget'))
    panel_tree(bp, row); panel_math(bp); panel_layout(bp, row); panel_refresh(bp, row); panel_events(bp, row)
    cdo = ue.get_default_object(bp.generated_class())
    cdo.set_editor_property('bIsFocusable', False)
    cdo.set_editor_property('Visibility', ue.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
    compile_widget(bp, report)
    return bp


def registration_state(folder, label):
    obj = ue.load_asset(LAS); assert obj
    actions = list(obj.get_editor_property('Actions'))
    result, matches = [], []
    for action_index, action in enumerate(actions):
        entry = {'path': action.get_path_name(), 'class': action.get_class().get_path_name(),
                 'raw': hud.export(action, folder / (label + '_action_' + str(action_index) + '.t3d'))}
        if 'AddWidgets' in action.get_class().get_name():
            values = list(action.get_editor_property('Widgets'))
            entry['widgets'] = [value.export_text() for value in values]
            for index, value in enumerate(values):
                if value.get_editor_property('SlotID').export_text() == SLOT_TAG: matches.append([action_index, index])
        result.append(entry)
    assert len(matches) == 1
    return {'raw': hud.export(obj, folder / (label + '_registration.t3d')), 'actions': result, 'equipment': matches[0]}


def replace_registration(panel):
    obj = ue.load_asset(LAS)
    matches = 0
    for action in obj.get_editor_property('Actions'):
        if 'AddWidgets' not in action.get_class().get_name(): continue
        values = list(action.get_editor_property('Widgets'))
        changed = False
        for index, value in enumerate(values):
            if value.get_editor_property('SlotID').export_text() != SLOT_TAG: continue
            value.set_editor_property('WidgetClass', panel.generated_class())
            values[index] = value; matches += 1; changed = True
        if changed: action.set_editor_property('Widgets', values)
    assert matches == 1
    return obj


def check_registration(before, after):
    assert before['equipment'] == after['equipment']
    action_index, index = before['equipment']
    assert len(before['actions']) == len(after['actions'])
    for number, (old, new) in enumerate(zip(before['actions'], after['actions'])):
        assert old['path'] == new['path'] and old['class'] == new['class']
        if number != action_index:
            assert old == new
            continue
        assert len(old['widgets']) == len(new['widgets'])
        for position, (a, b) in enumerate(zip(old['widgets'], new['widgets'])):
            if position != index: assert a == b
            else:
                assert fields.without_fields(a, {'WidgetClass'}) == fields.without_fields(b, {'WidgetClass'})
                assert ASSETS['panel'] + '.W_DC_WeaponPanel_C' in b
        old_class = re.search(r'/Game/[^\s\"\x27,()]+_C', old['widgets'][index]).group(0)
        new_class = ASSETS['panel'] + '.W_DC_WeaponPanel_C'
        assert new['raw'].replace(new_class, old_class) == old['raw'], 'Other AddWidgets action properties changed'
    # Bundle metadata can reflect the same exact WidgetClass replacement.
    old_class = re.search(r'/Game/[^\s\"\x27,()]+_C', before['actions'][action_index]['widgets'][index]).group(0)
    assert after['raw'].replace(ASSETS['panel'] + '.W_DC_WeaponPanel_C', old_class) == before['raw'], 'Other ActionSet properties changed'


def comparable_registration_after(before, value):
    result = json.loads(json.dumps(value))
    action_index, index = before['equipment']
    old_class = re.search(r'/Game/[^\s\"\x27,()]+_C', before['actions'][action_index]['widgets'][index]).group(0)
    new_class = ASSETS['panel'] + '.W_DC_WeaponPanel_C'
    rows = re.findall(r'^   AssetBundleData=[^\n]*$', result['raw'], re.M)
    assert len(rows) == 1
    row = rows[0]
    assert (row.count(old_class), row.count(new_class)) in ((1, 0), (0, 1))
    # E's pre-save snapshot retained the old Equipment bundle cache. Native
    # save rebuilt precisely this path; every other field stays byte-compared.
    result['raw'] = result['raw'].replace(row, row.replace(old_class, new_class), 1)
    return result


def snapshot_widgets(assets, folder, label):
    result = {}
    for key, bp in assets.items():
        sub = folder / (label + '_' + key); sub.mkdir()
        _, widgets = hud.tree(UMG, bp)
        result[key] = {'parent': ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
            'cdo': hud.export(ue.get_default_object(bp.generated_class()), sub / 'cdo.t3d'),
            'blueprint': hud.export(bp, sub / 'blueprint.t3d'), 'graphs': old_art.graph_state(bp),
            'widgets': {name: {'class': value.get_class().get_path_name(), 'parent': value.get_parent().get_name() if value.get_parent() else None,
                'raw': hud.export(value, sub / ('widget_' + str(index).zfill(2) + '.t3d')),
                'slot': hud.export(value.get_editor_property('slot'), sub / ('slot_' + str(index).zfill(2) + '.t3d'))
                    if value.get_editor_property('slot') else None}
                for index, (name, value) in enumerate(sorted(widgets.items()))}}
    return result


def comparable(value):
    result = json.loads(json.dumps(value))
    for item in result.values():
        item.pop('blueprint', None)  # Authored graph state and widget/CDO exports are compared independently.
        item['cdo'] = ordered.item_subobject_order(item['cdo'])
        for widget_value in item['widgets'].values():
            widget_value['raw'] = ordered.item_subobject_order(widget_value['raw'])
            if widget_value['slot']: widget_value['slot'] = ordered.item_subobject_order(widget_value['slot'])
    # Native save canonicalizes numeric pin defaults. Keep pin types and links
    # intact, including connected pins whose unused default text is serialized.
    for item in result.values():
        for graph_name, graph in item['graphs'].items():
            for node_name, node in graph.items():
                for name, value in node.get('pins', {}).items():
                    text = value['value']
                    if value['type'] in ('Float (double-precision)', 'Float (single-precision)', 'Integer'):
                        assert text == '' or re.fullmatch(r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?', text), text
                        value['value'] = str(Decimal(text or '0').normalize())
                    elif (item is result['row'] and graph_name == 'UpdateData'
                          and node_name in ('K2Node_CallFunction_2', 'K2Node_CallFunction_3') and name == 'InText'):
                        assert node['title'] == 'SetText (Text)' and value['type'] == 'Text'
                        assert text == '0' or re.fullmatch(r'NSLOCTEXT\("", "[A-Fa-f0-9]{32}", "0"\)', text), text
                        value['value'] = '0'
    return result


def validate_ui(assets, report):
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets['row']).get_path_name() == '/Script/UMG.UserWidget'
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets['panel']).get_path_name() == '/Script/DreamCatcher.DCLyraTaggedWidget'
    _, row = hud.tree(UMG, assets['row']); _, panel = hud.tree(UMG, assets['panel'])
    assert set(row) == {'RowSize', 'RowCanvas', 'OnCard', 'OffCard', 'NumberButton', 'WeaponName', 'Magazine', 'Slash', 'Spare'}
    assert set(panel) == {'PanelSize', 'PanelCanvas', 'Row1', 'Row2'}
    assert row['RowSize'].get_editor_property('WidthOverride') == 347
    assert row['RowSize'].get_clipping() == ue.WidgetClipping.CLIP_TO_BOUNDS
    for name, expected in (('OnCard', (316, 76)), ('OffCard', (316, 36)), ('NumberButton', (24, 24))):
        size = row[name].get_editor_property('slot').get_size()
        assert (size.x, size.y) == expected
        brush = row[name].get_editor_property('Brush')
        assert brush.get_editor_property('DrawAs') == ue.SlateBrushDrawType.IMAGE
        assert brush.get_editor_property('Margin').export_text() == ue.Margin(0).export_text()
        assert brush.get_editor_property('Tiling') == ue.SlateBrushTileType.NO_TILE
    for name, role in (('WeaponName', 'name'), ('Magazine', 'magazine'), ('Slash', 'slash'), ('Spare', 'spare')):
        assert row[name].get_class().get_name() == 'TextBlock'
        assert abs(row[name].get_font_size() - FONT_SIZES[role]) < 1e-5
        font = row[name].get_editor_property('Font')
        assert font.get_editor_property('FontObject').get_path_name().split('.')[0] == FONTS[role]
        assert font.get_editor_property('FontMaterial') is None
        assert row[name].get_editor_property('ColorAndOpacity').export_text() == ue.SlateColor(specified_color=ue.LinearColor(0, 0, 0, 1)).export_text()
    cdo = ue.get_default_object(assets['row'].generated_class())
    assert abs(cdo.get_editor_property('TransitionDuration') - .25) < 1e-6
    for index in range(2):
        value = panel['Row' + str(index + 1)]
        assert value.get_editor_property('SlotIndex') == index
        assert value.get_editor_property('SlotInputAction').get_path_name().split('.')[0] == INPUTS[index]
        assert value.get_editor_property('TextureOn').get_path_name().split('.')[0] == ART['main_on' if index == 0 else 'sub_on']
        assert value.get_editor_property('TextureOff').get_path_name().split('.')[0] == ART['main_off' if index == 0 else 'sub_off']
        assert value.get_editor_property('ButtonTexture').get_path_name().split('.')[0] == ART['button1' if index == 0 else 'button2']
        assert value.get_editor_property('slot').get_auto_size()
    panel_cdo = ue.get_default_object(assets['panel'].generated_class())
    assert panel_cdo.get_editor_property('Visibility') == ue.SlateVisibility.SELF_HIT_TEST_INVISIBLE
    assert panel_cdo.get_editor_property('HiddenByTags').export_text() == '(GameplayTags=)'
    for bp in assets.values():
        graph_data = old_art.graph_state(bp)
        for graph in graph_data.values():
            for node in graph.values():
                assert node.get('title') not in ('SetFontSize', 'SetFont', 'SetActiveSlotIndex', 'AddItemToSlot', 'RemoveItemFromSlot')
        raw = report['after'][next(key for key, obj in assets.items() if obj == bp)]['blueprint']
        assert '/Script/UMG.ScaleBox' not in raw and '/Script/CommonUI.CommonNumericTextBlock' not in raw
        assert 'Class=/Script/UMG.WidgetAnimation ' not in raw
        allowed = set(ASSETS.values()) | set(ART.values()) | set(FONTS.values()) | set(INPUTS)
        referenced = set(re.findall(r'/Game/[A-Za-z0-9_/.-]+', raw))
        for ref in referenced:
            package = ref.split('.', 1)[0]
            assert package in allowed, 'Unexpected visual/asset dependency: ' + ref


def object_transition_tests(assets, report):
    defaults_before = transition_defaults(assets)
    row = ue.new_object(assets['row'].generated_class())
    panel = ue.new_object(assets['panel'].generated_class())
    assert row != ue.get_default_object(assets['row'].generated_class())
    assert panel != ue.get_default_object(assets['panel'].generated_class())
    values = {}
    for alpha in (0, .5, 1):
        actual = row.call_method('ComputeLayout', args=(alpha,))
        assert len(actual) == 9
        expected = (36 + 40 * alpha, 30 + alpha, 6 - alpha, alpha, 1 - alpha)
        assert max(abs(float(a) - b) for a, b in zip(actual[:5], expected)) < 1e-5
        for position, role in zip(actual[5:], ('Name', 'Magazine', 'Slash', 'Spare')):
            start, end = ROW_VECTORS['Off' + role + 'Position'], ROW_VECTORS['On' + role + 'Position']
            assert abs(position.x - (start[0] + (end[0] - start[0]) * alpha)) < 1e-5
            assert abs(position.y - (start[1] + (end[1] - start[1]) * alpha)) < 1e-5
        values[str(alpha)] = list(map(float, actual[:5]))
    # Private state is default-only for Editor reflection. Drive the transient
    # instance through its public compiled BP functions, as runtime does.
    row.call_method('RequestSelection', args=(True,))
    row.call_method('RequestSelection', args=(False,))
    assert abs(float(row.call_method('AdvanceTransition', args=(.25,)))) < 1e-5
    row.call_method('RequestSelection', args=(True,))
    result = []
    for dt, expected in ((0, 0), (.125, .5), (.125, 1)):
        actual = row.call_method('AdvanceTransition', args=(dt,))
        assert abs(float(actual) - expected) < 1e-5; result.append(float(actual))
    row.call_method('RequestSelection', args=(False,))
    assert abs(float(row.call_method('AdvanceTransition', args=(.125,))) - .5) < 1e-5
    assert abs(float(row.call_method('AdvanceTransition', args=(.125,)))) < 1e-5
    row.call_method('RequestSelection', args=(True,))
    assert abs(float(row.call_method('AdvanceTransition', args=(.125,))) - .5) < 1e-5
    row.call_method('RequestSelection', args=(False,))
    reverse = [float(row.call_method('AdvanceTransition', args=(dt,))) for dt in (0, .125, .125)]
    assert max(abs(a - b) for a, b in zip(reverse, (.5, .25, 0))) < 1e-5
    for arguments, expected in (((76, 36, True, True, 1), (71, 107, 1)),
                                 ((36, 76, True, True, 0), (33, 109, 1)),
                                 ((56, 56, True, True, .5), (52, 108, 1)),
                                 ((0, 0, False, False, 0), (0, 1, 0)),
                                 ((0, 76, False, True, 0), (0, 76, 1))):
        actual = panel.call_method('ComputePanelLayout', args=arguments)
        assert max(abs(float(a) - b) for a, b in zip(actual, expected)) < 1e-5
    report['editor_object_tests'] = {'layout_outputs': values, 'times_0_125_250': result,
        'mid_transition_reverse': reverse, 'empty_panel_minimum_height': 1, 'runtime_or_Slate_geometry': False,
        'method': 'Actual compiled Blueprint functions called on new transient Editor objects; CDOs never modified.'}
    assert transition_defaults(assets) == defaults_before, 'Transient tests changed a CDO or panel row template'
    report['editor_object_tests']['CDO_and_templates_unchanged'] = True


def transition_defaults(assets):
    names = ('CurrentAlpha', 'StartAlpha', 'TargetAlpha', 'TransitionElapsed', 'SelectedState')
    _, widgets = hud.tree(UMG, assets['panel'])
    objects = {'row_CDO': ue.get_default_object(assets['row'].generated_class()),
               'Row1': widgets['Row1'], 'Row2': widgets['Row2']}
    return {key: {name: obj.get_editor_property(name) for name in names} for key, obj in objects.items()}


def same_protection_except_this_script(old, new):
    before, after = json.loads(json.dumps(old)), json.loads(json.dumps(new))
    name = SCRIPT.relative_to(P).as_posix()
    assert name not in before['sha256'] and name not in after['sha256']
    before['inventory'].pop(name); after['inventory'].pop(name)
    assert before == after, 'Protected files changed beyond this approved script correction'


def root_properties(text):
    result, depth = {}, 0
    for line in text.splitlines():
        if line.lstrip().startswith('Begin Object'): depth += 1
        elif line.strip() == 'End Object': depth -= 1
        elif depth == 1:
            match = re.fullmatch(r'   ([A-Za-z0-9_]+)=(.*)', line)
            if match:
                assert match[1] not in result
                result[match[1]] = match[2]
    assert depth == 0
    return result


def repair_panel(assets, previous, folder, report):
    expected = previous['after']
    report['repair_before'] = snapshot_widgets(assets, folder, 'repair_before')
    # Only the observed four private initialization properties may differ in
    # the reviewed, hash-bound E save. No graph, layout or other data changes.
    stripped = json.loads(json.dumps(report['repair_before']))
    target = json.loads(json.dumps(expected))
    keys = ('CurrentAlpha', 'StartAlpha', 'TransitionElapsed', 'SelectedState')
    for snapshot in (stripped, target):
        for name in ('Row1', 'Row2'):
            value = snapshot['panel']['widgets'][name]
            value['raw'] = re.sub(r'^   (?:' + '|'.join(keys) + r')=[^\n]*\n', '', value['raw'], flags=re.M)
    assert comparable(stripped) == comparable(target), 'The E save differs beyond the four reviewed initial fields'
    defaults = root_properties(expected['row']['cdo'])
    _, widgets = hud.tree(UMG, assets['panel'])
    report['repaired_initial_properties'] = {}
    for name in ('Row1', 'Row2'):
        overrides = root_properties(expected['panel']['widgets'][name]['raw'])
        changes = {}
        for key in keys:
            raw = overrides.get(key, defaults[key])
            desired = raw == 'True' if key == 'SelectedState' else float(raw)
            actual = widgets[name].get_editor_property(key)
            if actual != desired:
                widgets[name].set_editor_property(key, desired)
                changes[key] = {'before': actual, 'after': desired}
        report['repaired_initial_properties'][name] = changes
    assert set(report['repaired_initial_properties']['Row1']) == {'StartAlpha', 'TransitionElapsed', 'SelectedState'}
    assert set(report['repaired_initial_properties']['Row2']) == set(keys)
    compile_widget(assets['panel'], report)
    report['after'] = snapshot_widgets(assets, folder, 'after')
    assert comparable(report['after']) == comparable(expected), 'Repaired panel does not match the E pre-test snapshot'
    validate_ui(assets, report)
    object_transition_tests(assets, report)


def main():
    global ue, hud, helpers, old_art, ordered, fields, UMG
    import unreal as ue
    hud = module('ingame_hud_layout_setup')
    helpers = module('weapon_ammo_hud_setup')
    old_art = module('ingame_hud_art_setup'); old_art.ue = ue
    ordered = module('pistol_slot_setup')
    fields = module('mission_hud_setup')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = arg(command, 'DCWeaponScreensMode', 'inspect|apply|repair|verify')
    run = arg(command, 'DCWeaponScreensRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'; assert not output.exists(), 'Use a fresh report run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1
    supplied = Path(next(value for value in logs[0] if value).replace('\\', '/'))
    assert supplied.is_absolute() and supplied.resolve() == (folder / 'Editor.log').resolve()
    protected = protection(True)
    targets = {**ASSETS, 'registration': LAS}
    before_sha = {key: sha(disk(package)) if disk(package).exists() else None for key, package in targets.items()}
    report = {'mode': mode, 'run': run, 'complete': False,
        'asset_saves': [], 'compile_requested': [], 'PIE': False, 'automatic_PIE': False, 'cpp_build': False,
        'script_sha256': sha(SCRIPT), 'protected_before': protected, 'before_sha256': before_sha,
        'scope': targets, 'runtime_and_visual_verified': False,
        'limits': 'No PIE/render/play/input test. Editor object math tests do not prove Slate glyph clipping or gameplay.'}
    try:
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'); assert settings_class
        settings = hud.export(ue.get_default_object(settings_class), folder / 'blueprint_editor_settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'); assert umg_class
        UMG = ue.get_default_object(umg_class)
        previous = None
        if mode != 'inspect':
            ref = arg(command, 'DCWeaponScreensReference', '[A-Za-z0-9_]{1,64}')
            path = R / ref / 'report.json'; previous = read(path)
            assert previous['complete'] and previous['protection_passed']
            if mode == 'repair':
                assert ref == REPAIR_RUN and sha(path) == REPAIR_REPORT_SHA
                assert previous['mode'] == 'apply' and previous['script_sha256'] == REPAIR_SCRIPT_SHA
                same_protection_except_this_script(previous['protected_before'], protected)
            else:
                assert previous['mode'] in (('inspect',) if mode == 'apply' else ('apply', 'repair'))
                assert previous['script_sha256'] == report['script_sha256']
                assert previous['protected_before'] == protected
            expected_sha = previous['before_sha256'] if mode == 'apply' else previous['after_sha256']
            assert before_sha == expected_sha, 'Assets changed since the reviewed preceding run'
            report['reference'], report['reference_sha256'] = ref, sha(path)
        if mode in ('inspect', 'apply'):
            assert before_sha['row'] is None and before_sha['panel'] is None, 'No overwrite or partial-apply recovery'
            assert before_sha['registration'] == read(BASELINE)['files'][disk(LAS).relative_to(P).as_posix()]
            inspect_assets(folder, report)
            report['registration_before'] = registration_state(folder, 'before')
            if mode == 'inspect':
                report['complete'] = True
            else:
                assert report['registration_before'] == previous['registration_before']
                assert report['resources'] == previous['resources'] and report['fonts'] == previous['fonts']
                assert report['functional_inputs'] == previous['functional_inputs']
                resources = {key: ue.load_asset(package) for key, package in ART.items()}
                fonts = {key: ue.load_asset(package) for key, package in FONTS.items()}
                assets = {'row': create_row(resources, fonts, report)}
                assets['panel'] = create_panel(assets['row'], report)
                report['after'] = snapshot_widgets(assets, folder, 'after')
                validate_ui(assets, report)
                object_transition_tests(assets, report)
                registration = replace_registration(assets['panel'])
                report['registration_after'] = registration_state(folder, 'after')
                check_registration(report['registration_before'], report['registration_after'])
                assert len(report['compile_requested']) == 2
                # All three packages pass data, compile, log and protection gates before any save.
                hud.clean(targets.values()); hud.log_check(folder / 'Editor.log')
                assert protection(True) == protected
                assert {key: sha(disk(package)) if disk(package).exists() else None
                        for key, package in targets.items()} == before_sha
                for key, obj in (('row', assets['row']), ('panel', assets['panel']), ('registration', registration)):
                    hud.assert_no_other_editor()
                    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), key
                    report['asset_saves'].append(targets[key])
                report['complete'] = True
        elif mode == 'repair':
            assert all(before_sha.values())
            assets = {key: ue.load_asset(package) for key, package in ASSETS.items()}
            assert all(assets.values())
            report['registration_before'] = previous['registration_before']
            report['registration_after'] = registration_state(folder, 'repair')
            assert comparable_registration_after(report['registration_before'], report['registration_after']) == comparable_registration_after(
                report['registration_before'], previous['registration_after'])
            check_registration(report['registration_before'], report['registration_after'])
            repair_panel(assets, previous, folder, report)
            assert report['compile_requested'] == [assets['panel'].get_path_name()]
            hud.clean([ASSETS['panel']]); hud.log_check(folder / 'Editor.log')
            assert protection(True) == protected
            assert {key: sha(disk(package)) for key, package in targets.items()} == before_sha
            hud.assert_no_other_editor()
            assert ue.EditorAssetLibrary.save_loaded_asset(assets['panel'], only_if_is_dirty=False)
            report['asset_saves'].append(ASSETS['panel'])
            report['complete'] = True
        else:
            assert all(before_sha.values()), 'Fresh verify requires all three saved packages'
            assets = {key: ue.load_asset(package) for key, package in ASSETS.items()}
            assert all(assets.values())
            report['after'] = snapshot_widgets(assets, folder, 'verify')
            assert comparable(report['after']) == comparable(previous['after']), 'Fresh saved CDO/widget/graph data differs'
            validate_ui(assets, report)
            object_transition_tests(assets, report)
            report['registration_before'] = previous['registration_before']
            report['registration_after'] = registration_state(folder, 'verify')
            check_registration(report['registration_before'], report['registration_after'])
            assert comparable_registration_after(report['registration_before'], report['registration_after']) == comparable_registration_after(
                report['registration_before'], previous['registration_after']), 'Fresh saved registration differs'
            assert not report['asset_saves'] and not report['compile_requested']
            report['complete'] = True
        hud.clean(); hud.log_check(folder / 'Editor.log')
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            report['after_sha256'] = {key: sha(disk(package)) if disk(package).exists() else None
                                      for key, package in targets.items()}
            report['protection_passed'] = protection(True) == protected
            assert report['protection_passed']
            if mode == 'verify': assert report['after_sha256'] == before_sha
            if mode == 'repair':
                assert all(report['after_sha256'][key] == before_sha[key] for key in ('row', 'registration'))
        except Exception:
            report['complete'], report['protection_passed'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_WEAPON_SCREENS ' + json.dumps({key: report.get(key) for key in ('mode', 'run', 'complete', 'error', 'protection_error')}))
        assert report['complete'], 'Weapon Screens HUD operation incomplete'


if __name__ == '__main__':
    if '--repair-commands' in sys.argv:
        commands(sys.argv[sys.argv.index('--repair-commands') + 1], repair=True)
    elif '--commands' in sys.argv:
        commands(sys.argv[sys.argv.index('--commands') + 1])
    else:
        main()
