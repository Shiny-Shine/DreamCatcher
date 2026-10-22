"""Approved Designer-owned health/shield authoring, after the USER C++ build.

Separate UnrealEditor-Cmd processes: -unattended -run=pythonscript
-SCCProvider=None -EnablePlugins=UMGToolSet -DCStatusDesignerMode=inspect|apply|verify
-DCStatusDesignerRun=<fresh_run> -DCStatusDesignerReference=<preceding_run>
-abslog=<project>/Saved/Diagnostics/PlayerStatusDesigner20261023/<fresh_run>/Editor.log
Only the existing status/health WBP packages may be saved. Existing six material
instances are inspected, not modified speculatively. No build, PIE or Git actions.
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
R = P / 'Saved/Diagnostics/PlayerStatusDesigner20261023'
SCRIPT = P / 'Scripts/Editor/player_status_designer_setup.py'
ASSETS = {'status': '/Game/DreamCatcher/UI/HUD/W_DC_PlayerStatus',
          'health': '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_Healthbar'}
BASE = {'status': '85af6b1bcd76321826013cf20d6172d67b1abd27c470bc6cfb06d381347f2cac',
        'health': '1ad77d3d2623a8a74a3166aa8bf8e0ac0a31076043cca61514fdb83c22f6e213'}
ART = '/Game/DreamCatcher/UI/HUD/Art/'
MATERIALS = {key: ART + name for key, name in {
    'fill': 'MI_DC_HUD_HealthArt_Fill', 'border': 'MI_DC_HUD_HealthArt_Border',
    'glow': 'MI_DC_HUD_HealthArt_Glow', 'left': 'MI_DC_HUD_ArmorArt_Left',
    'middle': 'MI_DC_HUD_ArmorArt_Middle', 'right': 'MI_DC_HUD_ArmorArt_Right'}.items()}
FONT = '/Game/DreamCatcher/UI/Font/Paperlogy-7Bold_Font'
TEXTURES = {name: '/Game/DreamCatcher/Screens/InGame_UI/' + name for name in ('HP_Bar', 'Shield_Bar')}
NEW_HEALTH = {'HealthArtwork', 'HealthLabelCanvas'}


def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''): value.update(block)
    return value.hexdigest()


def disk(package):
    assert package.startswith('/Game/DreamCatcher/')
    return P / ('Content/' + package[6:] + '.uasset')


def module(name):
    spec = importlib.util.spec_from_file_location('status_designer_' + name, SCRIPT.with_name(name + '.py'))
    value = importlib.util.module_from_spec(spec); spec.loader.exec_module(value)
    return value


def initialize():
    global ue, rich, screens, hud, UMG, old_status, art, MEL
    rich = module('weapon_ammo_richtext_setup'); rich.initialize()
    ue, screens, hud, UMG = rich.ue, rich.screens, rich.hud, rich.UMG
    old_status = module('player_status_hud_setup')
    art = screens.old_art
    MEL = ue.MaterialEditingLibrary; art.MEL = MEL


def protection():
    allowed = {SCRIPT, *(disk(package) for package in ASSETS.values())}
    state = {}
    for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (P / top).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat(); state[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    protected = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
                 'Content/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD.uasset',
                 'Content/DreamCatcher/UI/HUD/W_DC_WeaponPanel.uasset', 'Content/DreamCatcher/UI/HUD/W_DC_WeaponRow.uasset']
    protected += [disk(package).relative_to(P).as_posix() for package in (*MATERIALS.values(), *TEXTURES.values(), FONT, ART + 'M_DC_HUD_HealthArt')]
    return {'inventory': state, 'sha256': {name: sha(P / name) for name in protected}}


def hashes():
    return {name: sha(disk(package)) for name, package in ASSETS.items()}


def tree(bp):
    # The native fallback WBP deliberately has no saved widgets before migration.
    rows = UMG.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets')
    widgets = {str(row.get_editor_property('widget_name')): row.get_editor_property('widget') for row in rows}
    assert len(rows) == len(widgets), 'Duplicate widget names'
    return widgets


def snapshot(assets, folder, label):
    result = {}
    for key, bp in assets.items():
        sub = folder / (label + '_' + key); sub.mkdir()
        result[key] = {'parent': ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
            'cdo': hud.export(ue.get_default_object(bp.generated_class()), sub / 'cdo.t3d'),
            'blueprint': hud.export(bp, sub / 'blueprint.t3d'), 'graphs': art.graph_state(bp),
            'widgets': {name: {'class': widget.get_class().get_path_name(),
                'parent': widget.get_parent().get_name() if widget.get_parent() else None,
                'raw': hud.export(widget, sub / ('widget_' + str(index).zfill(2) + '.t3d')),
                'slot': hud.export(widget.get_editor_property('slot'), sub / ('slot_' + str(index).zfill(2) + '.t3d'))
                    if widget.get_editor_property('slot') else None}
                for index, (name, widget) in enumerate(sorted(tree(bp).items()))}}
    return result


def normalized(value):
    value = json.loads(json.dumps(value))
    for item in value.values():
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
    return value


def animations(raw):
    records = old_status.flat_objects(raw)
    roots = {name for name, rows in records.items() if any(
        re.search(r'\bClass=/Script/UMG\.WidgetAnimation(?:\s|$)', row.splitlines()[0]) for row in rows)}
    return {name: rows for name, rows in records.items() if any(name == root or name.startswith(root + '.') for root in roots)}


def strip_root_fields(raw, fields):
    if not fields: return raw
    return re.sub(r'^   (?:' + '|'.join(re.escape(name) for name in fields) + r')(?:\(\d+\))?=[^\n]*\n?', '', raw, flags=re.M)


def inspect_resources(assets, folder, report):
    font = ue.load_asset(FONT); assert font and isinstance(font, ue.Font)
    textures = {key: ue.load_asset(value) for key, value in TEXTURES.items()}
    assert [textures['HP_Bar'].blueprint_get_size_x(), textures['HP_Bar'].blueprint_get_size_y()] == [446, 38]
    assert [textures['Shield_Bar'].blueprint_get_size_x(), textures['Shield_Bar'].blueprint_get_size_y()] == [440, 24]
    materials = {name: ue.load_asset(path) for name, path in MATERIALS.items()}; assert all(materials.values())
    report['materials'] = {name: {'path': value.get_path_name(), 'parameters': art.material_parameters(value)} for name, value in materials.items()}
    for name in ('fill', 'border', 'left', 'middle', 'right'):
        params = report['materials'][name]['parameters']
        assert params['static_switch']['ArtEnabled']
        expected = TEXTURES['Shield_Bar' if name in ('left', 'middle', 'right') else 'HP_Bar']
        assert params['texture']['ArtworkTexture'].split('.')[0] == expected
    assert not report['materials']['glow']['parameters']['static_switch']['ArtEnabled']
    report['font'] = font.get_path_name()
    report['original_animations'] = sorted(animations(report['before']['health']['blueprint']))
    graph = report['before']['health']['graphs']['EventGraph']
    events = [(name, node) for name, node in graph.items() if node['class'].endswith('K2Node_CustomEvent')
              and node.get('title') == 'OnPosessedPawnChanged']
    assert len(events) == 1
    name, callback = events[0]
    assert set(callback['pins']) == {'OutputDelegate', 'then', 'OldPawn', 'NewPawn'}
    assert all(callback['pins'][pin]['type'] == 'Pawn Object Reference' for pin in ('OldPawn', 'NewPawn'))
    binds = callback['pins']['OutputDelegate']['links']; assert len(binds) == 1
    assert graph[binds[0][0]]['title'] == 'Bind Event to On Possessed Pawn Changed'
    assert any(node.get('title') == 'Unbind Event from On Health Changed' for node in graph.values())
    assert any(node.get('title') == 'Bind Event to On Health Changed' for node in graph.values())
    report['original_callback_contract'] = {'name': name, 'event': 'OnPosessedPawnChanged',
        'parameters': ['OldPawn: APawn*', 'NewPawn: APawn*'], 'original_controller_delegate_link': binds,
        'old_health_unbind_and_new_health_bind_nodes_present': True, 'runtime_possession_verified': False}
    return font, materials


def add(bp, cls, name, parent=None, index=-1):
    value = UMG.call_method('AddWidget', args=(bp, cls, name, parent, index)).get_editor_property('widget')
    assert value
    # Native typed name lookup needs no new generated Blueprint member properties.
    UMG.call_method('ToggleWidgetAsVariable', args=(bp, value, False))
    value.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    return value


def place(widget, x, y, width, height):
    screens.place(widget, x, y, width, height)
    return widget.get_editor_property('slot')


def fill(widget):
    slot = widget.get_editor_property('slot')
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(1, 1)))
    slot.set_offsets(ue.Margin(0, 0, 0, 0))


def centered_label(widget, font, size, color, position):
    value = widget.get_editor_property('Font')
    value.set_editor_property('FontObject', font); value.set_editor_property('TypefaceFontName', 'Default')
    value.set_editor_property('Size', size); value.set_editor_property('LetterSpacing', 0)
    value.set_editor_property('FontMaterial', None)
    widget.set_font(value); widget.set_color_and_opacity(ue.SlateColor(specified_color=ue.LinearColor(*color)))
    widget.set_editor_property('Justification', ue.TextJustify.CENTER)
    slot = place(widget, *position, 0, 0)
    slot.set_auto_size(True); slot.set_alignment(ue.Vector2D(.5, .5))


def move(bp, widget, parent, index=-1):
    moved = UMG.call_method('MoveWidget', args=(bp, widget, parent, index))
    assert moved.get_editor_property('widget') == widget


def edit_health(bp, font):
    widgets = tree(bp)
    assert not (NEW_HEALTH & set(widgets)), 'Do not reapply to an already authored health layout'
    assert widgets['HealthNumber'].get_parent() == widgets['DCHealthTextRow']
    widgets['AnimBoundSB'].set_width_override(446); widgets['AnimBoundSB'].set_height_override(38)
    artwork = add(bp, ue.CanvasPanel, 'HealthArtwork', widgets['Overlay_4'], 0)
    slot = artwork.get_editor_property('slot')
    slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    for name, bounds in (('BarBorder', (0, 0, 446, 38)), ('BarFill', (0, 0, 446, 38)), ('BarGlow', (0, -40, 446, 118))):
        move(bp, widgets[name], artwork); place(widgets[name], *bounds)
    label_canvas = add(bp, ue.CanvasPanel, 'HealthLabelCanvas', widgets['Overlay_0'])
    slot = label_canvas.get_editor_property('slot')
    slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    number = widgets['HealthNumber']; move(bp, number, label_canvas)
    centered_label(number, font, 18, (0, 0, 0, 1), (223, 19))
    number.set_current_value(100)
    widgets['DCMaxHealthText'].set_visibility(ue.SlateVisibility.COLLAPSED)
    widgets['DCHealthTextRow'].set_visibility(ue.SlateVisibility.COLLAPSED)


def edit_status(bp, health, font, materials):
    assert not tree(bp), 'Native fallback should be the current untouched baseline'
    cdo = ue.get_default_object(bp.generated_class())
    style = cdo.get_editor_property('StatusStyle')
    root = add(bp, ue.CanvasPanel, 'PlayerStatusRoot')
    bars = add(bp, ue.CanvasPanel, 'PlayerStatusBars', root)
    slot = place(bars, style.get_editor_property('HorizontalOffset'), -style.get_editor_property('BottomMargin'), 446, 72)
    slot.set_anchors(ue.Anchors(ue.Vector2D(.5, 1), ue.Vector2D(.5, 1)))
    slot.set_alignment(ue.Vector2D(.5, 1))
    armor = add(bp, ue.CanvasPanel, 'PlayerArmorBar', bars); place(armor, 3, 0, 440, 24)
    for index, (name, x, width) in enumerate((('left', 0, 141), ('middle', 149, 142), ('right', 299, 141))):
        segment = add(bp, ue.CanvasPanel, 'PlayerArmorSegment' + str(index), armor); place(segment, x, 0, width, 24)
        border = add(bp, ue.Image, 'PlayerArmorBorder' + str(index), segment); fill(border)
        border.set_brush(style.get_editor_property('ArmorBorderBrush'))
        image = add(bp, ue.Image, 'PlayerArmorFill' + str(index), segment); fill(image)
        brush = style.get_editor_property('ArmorSegmentFillBrushes')[index]
        assert brush.get_editor_property('ResourceObject') == materials[name]
        image.set_brush(brush); image.set_render_transform_pivot(ue.Vector2D(0, .5))
    label = add(bp, ue.TextBlock, 'ShieldNumber', armor)
    label.set_text('150'); centered_label(label, font, 14, (1, 1, 1, 1), (220, 12))
    host = add(bp, ue.SizeBox, 'PlayerHealthHost', bars); place(host, 0, 34, 446, 38)
    host.set_width_override(446); host.set_height_override(38)
    child = add(bp, health.generated_class(), 'OriginalHealthbar', host)
    child.get_editor_property('slot').set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    child.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    signature = add(bp, ue.CanvasPanel, 'PlayerSignature', root)
    # Preserve the old Q group's screen position and style independently of the new bar bounds.
    height = sum(style.get_editor_property(name) for name in ('ArmorHeight', 'ArmorHealthGap', 'HealthHeight'))
    size = style.get_editor_property('StarSize')
    slot = place(signature, style.get_editor_property('HorizontalOffset') + style.get_editor_property('BarWidth') * .5 + style.get_editor_property('StarGap'),
                 -style.get_editor_property('BottomMargin') - height * .5, size, size)
    slot.set_anchors(ue.Anchors(ue.Vector2D(.5, 1), ue.Vector2D(.5, 1))); slot.set_alignment(ue.Vector2D(0, .5))
    ring = add(bp, ue.Image, 'PlayerSignatureRing', signature); fill(ring); ring.set_brush(style.get_editor_property('SignatureRingBrush'))
    star = add(bp, ue.Image, 'PlayerSignatureStar', signature)
    slot = place(star, 0, 0, size * style.get_editor_property('StarIconScale'), size * style.get_editor_property('StarIconScale'))
    slot.set_anchors(ue.Anchors(ue.Vector2D(.5, .5), ue.Vector2D(.5, .5))); slot.set_alignment(ue.Vector2D(.5, .5))
    star.set_brush(style.get_editor_property('StarBrush'))
    cdo.set_editor_property('bUseDesignerLayout', True)
    cdo.set_editor_property('PreviewHealth', 100); cdo.set_editor_property('PreviewMaxHealth', 100)
    cdo.set_editor_property('PreviewArmor', 150); cdo.set_editor_property('PreviewMaxArmor', 150)


def preserve(before, after):
    old, new = normalized(before), normalized(after)
    for key in ASSETS:
        assert old[key]['graphs'] == new[key]['graphs'], 'Original graph changed: ' + key
        assert animations(before[key]['blueprint']) == animations(after[key]['blueprint']), 'Original animations changed: ' + key
    assert old['status']['parent'] == new['status']['parent']
    assert strip_root_fields(old['status']['cdo'], {'bUseDesignerLayout'}) == strip_root_fields(new['status']['cdo'], {'bUseDesignerLayout'}), 'Other status CDO defaults changed'
    assert set(new['health']['widgets']) == set(old['health']['widgets']) | NEW_HEALTH
    changed_properties = {'AnimBoundSB': {'WidthOverride', 'HeightOverride'},
        'HealthNumber': {'Slot', 'Font', 'ColorAndOpacity', 'Justification', 'Text', 'CurrentNumericValue', 'TargetNumericValue'},
        'DCHealthTextRow': {'Slots', 'Visibility'}, 'DCMaxHealthText': {'Visibility'},
        'BarBorder': {'Slot'}, 'BarFill': {'Slot'}, 'BarGlow': {'Slot'}, 'Overlay_0': {'Slots'}, 'Overlay_4': {'Slots'}}
    moved = {'HealthNumber', 'BarBorder', 'BarFill', 'BarGlow'}
    for name, original in old['health']['widgets'].items():
        current = new['health']['widgets'][name]
        assert current['class'] == original['class']
        if name not in moved:
            assert original['parent'] == current['parent'] and original['slot'] == current['slot'], 'Unrelated slot changed: ' + name
        fields = changed_properties.get(name, set())
        # Parent slot subobject records are legitimately changed by reparenting. Own widget properties
        # remain exact apart from the explicit allowlist; the whole originals are kept in the journal.
        old_records = old_status.flat_objects(original['raw']); new_records = old_status.flat_objects(current['raw'])
        own_path = next(key for key in old_records if key.endswith('.' + name))
        old_own, new_own = old_records[own_path], new_records[own_path]
        assert len(old_own) == len(new_own)
        for left, right in zip(old_own, new_own):
            assert strip_root_fields(left, fields) == strip_root_fields(right, fields), 'Unapproved widget property changed: ' + name


def validate(assets, report):
    status, health = tree(assets['status']), tree(assets['health'])
    cdo = ue.get_default_object(assets['status'].generated_class())
    assert cdo.get_editor_property('bUseDesignerLayout')
    assert cdo.get_editor_property('HealthWidgetClass') == assets['health'].generated_class()
    assert status['OriginalHealthbar'].get_parent() == status['PlayerHealthHost']
    assert status['OriginalHealthbar'].get_class() == assets['health'].generated_class()
    expected = {'PlayerStatusBars': (None, None, 446, 72), 'PlayerArmorBar': (3, 0, 440, 24), 'PlayerHealthHost': (0, 34, 446, 38)}
    for name, values in expected.items():
        slot = status[name].get_editor_property('slot'); pos, size = slot.get_position(), slot.get_size()
        assert [size.x, size.y] == list(values[2:])
        if values[0] is not None: assert [pos.x, pos.y] == list(values[:2])
    for widget, size in ((status['ShieldNumber'], 14), (health['HealthNumber'], 18)):
        font = widget.get_editor_property('Font')
        assert font.get_editor_property('FontObject').get_path_name().split('.')[0] == FONT
        assert font.get_editor_property('Size') == size and font.get_editor_property('LetterSpacing') == 0
        assert isinstance(widget.get_editor_property('slot'), ue.CanvasPanelSlot)
    for widget, expected in ((status['ShieldNumber'], (1, 1, 1, 1)), (health['HealthNumber'], (0, 0, 0, 1))):
        color = widget.get_editor_property('ColorAndOpacity').get_editor_property('SpecifiedColor')
        assert [color.r, color.g, color.b, color.a] == list(expected)
    assert health['DCMaxHealthText'].get_visibility() == ue.SlateVisibility.COLLAPSED
    assert all(health[name].get_parent() == health['HealthArtwork'] for name in ('BarFill', 'BarBorder', 'BarGlow'))
    report['validated'] = {'saved_designer_tree': True, 'shield_440x24_at_3_0': True, 'health_446x38_at_0_34': True,
        'shield_font_14_white_health_font_18_black': True, 'current_only_authored': True,
        'direct_canvas_editable': True, 'original_health_graphs_and_animations_preserved': True,
        'native_designer_tests_required': 'DreamCatcher.UI.PlayerStatus.Designer', 'PIE_verified': False, 'visual_verified': False}


def original_health_nopawn_probe(assets, report):
    # Apply_A established that the original callback resets animations and requires
    # initialized Slate. Keep the real graph signature audit above, but do not
    # invoke gameplay callbacks without their real Pawn/controller context.
    report['original_health_callback_probe'] = {'executed': False,
        'actual_class': assets['health'].generated_class().get_path_name(),
        'reason': 'Actual Pawn/controller rebinding remains user PIE validation; commandlet has no Slate',
        'PIE': False, 'runtime_controller_rebinding_verified': False}


def main():
    initialize()
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode = screens.arg(command, 'DCStatusDesignerMode', 'inspect|apply|verify')
    run = screens.arg(command, 'DCStatusDesignerRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in folder.iterdir()), 'Fresh diagnostic folder required'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1 and Path(next(value for value in logs[0] if value)).resolve() == (folder / 'Editor.log').resolve()
    protected, before_hashes = protection(), hashes()
    report = {'mode': mode, 'run': run, 'complete': False, 'asset_saves': [], 'compile_requested': [],
        'cpp_build': False, 'PIE': False, 'visual_verified': False, 'script_sha256': sha(SCRIPT),
        'before_sha256': before_hashes, 'protected_before': protected}
    try:
        native = ue.load_class(None, '/Script/DreamCatcher.DCPlayerStatusWidget'); assert native
        try:
            native_cdo = ue.get_default_object(native)
            assert native_cdo.get_editor_property('bUseDesignerLayout') is False
            assert native_cdo.get_editor_property('PreviewArmor') == 150
        except Exception as exc: raise AssertionError('BUILD_REQUIRED: approved Designer C++ schema missing; user must build first') from exc
        settings = hud.export(ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')), folder / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        assets = {name: ue.load_asset(path) for name, path in ASSETS.items()}; assert all(assets.values())
        report['before'] = snapshot(assets, folder, 'before')
        font, materials = inspect_resources(assets, folder, report)
        if mode == 'inspect':
            assert before_hashes == BASE, 'Baseline changed; inspect user edits before authoring'
            assert not report['before']['status']['widgets']
        else:
            reference = screens.arg(command, 'DCStatusDesignerReference', '[A-Za-z0-9_]{1,64}')
            previous = json.loads((R / reference / 'report.json').read_text(encoding='utf-8'))
            assert previous['complete'] and previous['protection_passed'] and previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == report['script_sha256'] and previous['protected_before'] == protected
            assert previous['after_sha256'] == before_hashes and previous['materials'] == report['materials']
            report['reference'] = reference
            if mode == 'apply':
                assert before_hashes == BASE and normalized(previous['before']) == normalized(report['before'])
                edit_health(assets['health'], font); screens.compile_widget(assets['health'], report)
                edit_status(assets['status'], assets['health'], font, materials); screens.compile_widget(assets['status'], report)
                report['after'] = snapshot(assets, folder, 'after')
                preserve(report['before'], report['after']); validate(assets, report)
                original_health_nopawn_probe(assets, report)
                hud.clean(ASSETS.values()); hud.log_check(folder / 'Editor.log')
                assert protection() == protected and hashes() == before_hashes
                for key in ('health', 'status'):
                    hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(assets[key], only_if_is_dirty=False)
                    report['asset_saves'].append(ASSETS[key])
            else:
                assert normalized(report['before']) == normalized(previous['after']), 'Fresh reload changed authored state'
                validate(assets, report)
                for bp in assets.values(): screens.compile_widget(bp, report)
                original_health_nopawn_probe(assets, report)
                report['after'] = snapshot(assets, folder, 'recompiled')
                assert normalized(report['after']) == normalized(report['before']), 'Recompile changed saved layout'
        hud.clean(ASSETS.values() if mode == 'verify' else ()); hud.log_check(folder / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc(); raise
    finally:
        report['after_sha256'] = hashes(); report['protection_passed'] = protection() == protected
        if not report['protection_passed'] or (mode != 'apply' and report['after_sha256'] != before_hashes): report['complete'] = False
        (folder / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.log('DC_STATUS_DESIGNER ' + json.dumps({'complete': report['complete'], 'mode': mode, 'saves': report['asset_saves']}))


VISUAL_MATERIAL_KEYS = ('fill', 'left', 'middle', 'right')
VISUAL_SAVE_MATERIAL_KEYS = ('fill', 'left', 'middle', 'right', 'border', 'glow')
VISUAL_PARAMETERS = {'NotchBrightness': 0., 'Notch_NoHealthOpacity': 0., 'Radius_InnerBar': 1.}


def visual_hashes():
    return {**hashes(), **{key: sha(disk(MATERIALS[key])) for key in VISUAL_SAVE_MATERIAL_KEYS}}


def visual_protection():
    state = protection()
    for key in VISUAL_SAVE_MATERIAL_KEYS:
        name = disk(MATERIALS[key]).relative_to(P).as_posix()
        state['inventory'].pop(name, None); state['sha256'].pop(name, None)
    return state


def visual_material_state(materials):
    return {key: {'parent': obj.get_editor_property('Parent').get_path_name(), 'parameters': art.material_parameters(obj)}
            for key, obj in materials.items()}


def image_brush(widget, width, height, no_draw=False):
    brush = widget.get_editor_property('Brush')
    brush.set_editor_property('DrawAs', ue.SlateBrushDrawType.NO_DRAW_TYPE if no_draw else ue.SlateBrushDrawType.IMAGE)
    brush.set_editor_property('Margin', ue.Margin(0, 0, 0, 0))
    image_size = brush.get_editor_property('ImageSize')
    assert image_size.import_text('(X=%s,Y=%s)' % (width, height))
    brush.set_editor_property('ImageSize', image_size)
    widget.set_brush(brush)


def apply_visual_correction(assets, materials):
    health, status = tree(assets['health']), tree(assets['status'])
    assert health['Overlay_4'].get_parent() == health['HorizontalBox_1']
    # Retain the same animated descendants and identity; the legacy 56px number
    # holder must not affect artwork geometry even when Designer shows hidden widgets.
    move(assets['health'], health['Overlay_4'], health['Overlay_0'], 3)
    slot = health['Overlay_4'].get_editor_property('slot')
    slot.set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_FILL)
    # bHiddenInDesigner is protected from Python. The obsolete number-box image
    # is not an animation target; a no-draw brush removes its Designer underlay
    # while preserving the existing collapsed holder and every original object.
    image_brush(health['NumberBorder'], 56, 38, True)
    for name in ('BarFill', 'BarBorder'): image_brush(health[name], 446, 38)
    for index, width in enumerate((141, 142, 141)):
        image_brush(status['PlayerArmorFill' + str(index)], width, 24)
        # The supplied shield art already owns its silhouette. Keep each legacy
        # border widget and native binding, but do not draw a rounded procedural underlay.
        image_brush(status['PlayerArmorBorder' + str(index)], width, 24, True)
    # Border/glow inherit from Fill. Pin their existing effective values first so
    # changing the artwork mask does not silently alter the original effect branches.
    for key in ('border', 'glow'):
        for name in VISUAL_PARAMETERS:
            old = MEL.get_material_instance_scalar_parameter_value(materials[key], name)
            MEL.set_material_instance_scalar_parameter_value(materials[key], name, old)
    for key in VISUAL_MATERIAL_KEYS:
        for name, value in VISUAL_PARAMETERS.items():
            MEL.set_material_instance_scalar_parameter_value(materials[key], name, value)
        MEL.update_material_instance(materials[key])
    for key in ('border', 'glow'): MEL.update_material_instance(materials[key])


def preserve_visual_correction(before, after, before_materials, after_materials):
    old, new = normalized(before), normalized(after)
    for key in ASSETS:
        assert old[key]['cdo'] == new[key]['cdo'] and old[key]['graphs'] == new[key]['graphs'], 'CDO or graph changed'
        assert animations(before[key]['blueprint']) == animations(after[key]['blueprint']), 'Animation changed'
        assert set(old[key]['widgets']) == set(new[key]['widgets'])
        for name, previous in old[key]['widgets'].items():
            current = new[key]['widgets'][name]
            assert previous['class'] == current['class']
            if not (key == 'health' and name == 'Overlay_4'):
                assert previous['slot'] == current['slot'] and previous['parent'] == current['parent'], 'Unapproved geometry changed: ' + name
            fields = set()
            if key == 'health':
                if name in ('Overlay_0', 'HorizontalBox_1'): fields = {'Slots'}
                elif name == 'Overlay_4': fields = {'Slot'}
                elif name in ('BarFill', 'BarBorder'): fields = {'Brush'}
                elif name == 'NumberBorder': fields = {'Brush'}
            elif name.startswith(('PlayerArmorFill', 'PlayerArmorBorder')): fields = {'Brush'}
            previous_records, current_records = old_status.flat_objects(previous['raw']), old_status.flat_objects(current['raw'])
            own_path = next(path for path in previous_records if path.endswith('.' + name))
            for left, right in zip(previous_records[own_path], current_records[own_path]):
                assert strip_root_fields(left, fields) == strip_root_fields(right, fields), 'Unapproved widget property: ' + name
    for key, previous in before_materials.items():
        current = json.loads(json.dumps(after_materials[key]))
        if key in VISUAL_MATERIAL_KEYS:
            for name, expected in VISUAL_PARAMETERS.items():
                assert current['parameters']['scalar'][name] == expected
                current['parameters']['scalar'][name] = previous['parameters']['scalar'][name]
        assert current == previous, 'Unapproved material parameter or parent changed: ' + key


def validate_visual_correction(assets, materials, report):
    validate(assets, report)
    health, status = tree(assets['health']), tree(assets['status'])
    assert health['Overlay_4'].get_parent() == health['Overlay_0']
    assert health['NumberBorder'].get_editor_property('Brush').get_editor_property('DrawAs') == ue.SlateBrushDrawType.NO_DRAW_TYPE
    for widget in [health['BarFill'], health['BarBorder'], *(status['PlayerArmorFill' + str(i)] for i in range(3))]:
        brush = widget.get_editor_property('Brush')
        assert brush.get_editor_property('DrawAs') == ue.SlateBrushDrawType.IMAGE
        assert brush.get_editor_property('Margin').export_text() == ue.Margin(0, 0, 0, 0).export_text()
    for i in range(3):
        assert status['PlayerArmorBorder' + str(i)].get_editor_property('Brush').get_editor_property('DrawAs') == ue.SlateBrushDrawType.NO_DRAW_TYPE
    for key in VISUAL_MATERIAL_KEYS:
        for name, value in VISUAL_PARAMETERS.items():
            assert MEL.get_material_instance_scalar_parameter_value(materials[key], name) == value
    report['visual_correction'] = {'legacy_56px_layout_dependency_removed': True, 'source_art_uses_image_uvs': True,
        'legacy_notch_overlay_removed': True, 'radial_center_mask_neutralized': True,
        'health_border_and_glow_preserved': True, 'Q_and_text_settings_preserved': True, 'visual_verified': False}


def visual_main():
    initialize()
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor(); hud.clean()
    mode = screens.arg(command, 'DCStatusVisualMode', 'inspect|apply|verify')
    run = screens.arg(command, 'DCStatusVisualRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in folder.iterdir()), 'Fresh folder required'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1 and Path(next(value for value in logs[0] if value)).resolve() == (folder / 'Editor.log').resolve()
    protected, before_hashes = visual_protection(), visual_hashes()
    report = {'mode': mode, 'run': run, 'complete': False, 'asset_saves': [], 'compile_requested': [],
        'cpp_build': False, 'PIE': False, 'visual_verified': False, 'script_sha256': sha(SCRIPT),
        'before_sha256': before_hashes, 'protected_before': protected}
    try:
        settings = hud.export(ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')), folder / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        assets = {name: ue.load_asset(path) for name, path in ASSETS.items()}; assert all(assets.values())
        materials = {name: ue.load_asset(path) for name, path in MATERIALS.items()}; assert all(materials.values())
        report['before'] = snapshot(assets, folder, 'before')
        report['materials_before'] = visual_material_state(materials)
        if mode == 'inspect':
            baseline = json.loads((R / 'Verify_A/report.json').read_text(encoding='utf-8-sig'))
            assert all(before_hashes[key] == baseline['after_sha256'][key] for key in ASSETS)
            assert normalized(report['before']) == normalized(baseline['after'])
            for key in VISUAL_SAVE_MATERIAL_KEYS:
                assert before_hashes[key] == baseline['protected_before']['sha256'][disk(MATERIALS[key]).relative_to(P).as_posix()]
        else:
            reference = screens.arg(command, 'DCStatusVisualReference', '[A-Za-z0-9_]{1,64}')
            previous = json.loads((R / reference / 'report.json').read_text(encoding='utf-8-sig'))
            assert previous['complete'] and previous['protection_passed'] and previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == report['script_sha256'] and previous['protected_before'] == protected
            assert previous['after_sha256'] == before_hashes
            report['reference'] = reference
            if mode == 'apply':
                assert normalized(previous['before']) == normalized(report['before']) and previous['materials_before'] == report['materials_before']
                apply_visual_correction(assets, materials)
                for bp in assets.values(): screens.compile_widget(bp, report)
                report['after'] = snapshot(assets, folder, 'after'); report['materials_after'] = visual_material_state(materials)
                preserve_visual_correction(report['before'], report['after'], report['materials_before'], report['materials_after'])
                validate_visual_correction(assets, materials, report)
                allowed = [*ASSETS.values(), *(MATERIALS[key] for key in VISUAL_SAVE_MATERIAL_KEYS)]
                hud.clean(allowed); hud.log_check(folder / 'Editor.log')
                assert visual_protection() == protected and visual_hashes() == before_hashes
                for key in VISUAL_SAVE_MATERIAL_KEYS:
                    hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(materials[key], only_if_is_dirty=False)
                    report['asset_saves'].append(MATERIALS[key])
                for key, bp in assets.items():
                    hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
                    report['asset_saves'].append(ASSETS[key])
            else:
                assert normalized(report['before']) == normalized(previous['after']) and report['materials_before'] == previous['materials_after']
                validate_visual_correction(assets, materials, report)
                for bp in assets.values(): screens.compile_widget(bp, report)
                report['after'] = snapshot(assets, folder, 'recompiled'); report['materials_after'] = visual_material_state(materials)
                assert normalized(report['before']) == normalized(report['after']) and report['materials_before'] == report['materials_after']
        hud.clean(ASSETS.values() if mode == 'verify' else ()); hud.log_check(folder / 'Editor.log'); report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc(); raise
    finally:
        report['after_sha256'] = visual_hashes(); report['protection_passed'] = visual_protection() == protected
        if not report['protection_passed'] or (mode != 'apply' and before_hashes != report['after_sha256']): report['complete'] = False
        (folder / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.log('DC_STATUS_VISUAL ' + json.dumps({'complete': report['complete'], 'mode': mode, 'saves': report['asset_saves']}))


MASTER = ART + 'M_DC_HUD_HealthArt'


def flat_hashes():
    return {**visual_hashes(), 'master': sha(disk(MASTER))}


def flat_protection():
    state = visual_protection()
    name = disk(MASTER).relative_to(P).as_posix()
    state['inventory'].pop(name, None); state['sha256'].pop(name, None)
    return state


def master_snapshot(master, folder, label):
    directory = folder / label; directory.mkdir()
    expressions = {obj.get_name(): obj for obj in MEL.get_material_expressions(master)}
    return {'expressions': {name: {'class': obj.get_class().get_name(),
        'raw': hud.export(obj, directory / ('expression_%03d.t3d' % index))}
        for index, (name, obj) in enumerate(sorted(expressions.items()))},
        'outputs': {str(prop): [MEL.get_material_property_input_node(master, prop).get_name(),
            MEL.get_material_property_input_node_output_name(master, prop)]
            for prop in (ue.MaterialProperty.MP_EMISSIVE_COLOR, ue.MaterialProperty.MP_OPACITY)}}


def flat_master_contract(master):
    nodes = {obj.get_name(): obj for obj in MEL.get_material_expressions(master)}
    required = ('MaterialExpressionTextureSampleParameter2D_0', 'MaterialExpressionStaticSwitchParameter_19',
        'MaterialExpressionStaticSwitchParameter_20', 'MaterialExpressionMultiply_9',
        'MaterialExpressionLinearInterpolate_41', 'MaterialExpressionScalarParameter_36', 'MaterialExpressionScalarParameter_5')
    assert set(required) <= nodes.keys()
    assert str(nodes[required[0]].get_editor_property('ParameterName')) == 'ArtworkTexture'
    assert all(str(nodes[name].get_editor_property('ParameterName')) == 'ArtEnabled' for name in required[1:3])
    assert str(nodes[required[-2]].get_editor_property('ParameterName')) == 'Health_Current'
    assert str(nodes[required[-1]].get_editor_property('ParameterName')) == 'Health_Updated'
    return nodes


def apply_flat_master(master, report):
    nodes = flat_master_contract(master)
    minimum = MEL.create_material_expression(master, ue.MaterialExpressionMin, 2100, 700); assert minimum
    one = MEL.create_material_expression(master, ue.MaterialExpressionConstant, 2100, 900); assert one
    one.set_editor_property('R', 1.)
    endpoint = MEL.create_material_expression(master, ue.MaterialExpressionIf, 2350, 700); assert endpoint
    endpoint.set_editor_property('ConstB', 1.)
    endpoint.set_editor_property('EqualsThreshold', 0.)
    inputs = list(MEL.get_material_expression_input_names(endpoint))
    def if_pin(symbol, property_name):
        matches = [name for name in inputs if name.replace(' ', '').lower() in (symbol.lower(), property_name.lower())]
        assert len(matches) == 1, inputs
        return matches[0]
    def connect(start, output, end, input_name):
        assert MEL.connect_material_expressions(start, output, end, input_name)
    connect(nodes['MaterialExpressionScalarParameter_36'], '', minimum, 'A')
    connect(nodes['MaterialExpressionScalarParameter_5'], '', minimum, 'B')
    connect(minimum, '', endpoint, 'A')
    connect(one, '', endpoint, if_pin('A>B', 'AGreaterThanB'))
    connect(one, '', endpoint, if_pin('A==B', 'AEqualsB'))
    connect(nodes['MaterialExpressionLinearInterpolate_41'], '', endpoint, if_pin('A<B', 'ALessThanB'))
    # Only the existing ArtEnabled=true branch is changed. Legacy false branch
    # and all health-current/updated interpolation nodes remain byte-preserved.
    connect(nodes['MaterialExpressionTextureSampleParameter2D_0'], 'RGB', nodes['MaterialExpressionStaticSwitchParameter_19'], 'True')
    connect(endpoint, '', nodes['MaterialExpressionMultiply_9'], 'A')
    report['new_master_nodes'] = [minimum.get_name(), one.get_name(), endpoint.get_name()]
    report['endpoint_inputs'] = inputs
    errors = list(MEL.recompile_material(master)); assert not errors, errors
    report['compile_requested'].append(MASTER)


def preserve_flat(before, after, old_master, new_master, old_materials, new_materials, added):
    previous, current = normalized(before), normalized(after)
    for key in ASSETS:
        assert previous[key]['cdo'] == current[key]['cdo'] and previous[key]['graphs'] == current[key]['graphs']
        assert animations(before[key]['blueprint']) == animations(after[key]['blueprint'])
        assert set(previous[key]['widgets']) == set(current[key]['widgets'])
        for name, old in previous[key]['widgets'].items():
            new = current[key]['widgets'][name]
            assert old['class'] == new['class'] and old['slot'] == new['slot'] and old['parent'] == new['parent'], name
            fields = {'Brush'} if key == 'health' and name in ('BarBorder', 'BarGlow', 'Shadow') else (
                {'BlurStrength', 'BlurRadius', 'bOverrideAutoRadiusCalculation'} if key == 'health' and name == 'BackgroundBlur_145' else set())
            assert strip_root_fields(old['raw'], fields) == strip_root_fields(new['raw'], fields), name
    assert old_master['outputs'] == new_master['outputs']
    assert set(new_master['expressions']) == set(old_master['expressions']) | set(added)
    for name, old in old_master['expressions'].items():
        new = new_master['expressions'][name]
        assert old['class'] == new['class']
        fields = {'A'} if name in ('MaterialExpressionStaticSwitchParameter_19', 'MaterialExpressionMultiply_9') else set()
        assert strip_root_fields(old['raw'], fields) == strip_root_fields(new['raw'], fields), 'Unapproved master graph change: ' + name
    for key, old in old_materials.items():
        new = json.loads(json.dumps(new_materials[key]))
        # Default UI preview values only. Border/glow inherit these values from Fill.
        for name in ('Health_Current', 'Health_Updated'):
            assert new['parameters']['scalar'][name] == 1.
            new['parameters']['scalar'][name] = old['parameters']['scalar'][name]
        assert new == old, 'Unexpected material setting: ' + key


def validate_flat(assets, materials, master, report):
    validate(assets, report)
    widgets = tree(assets['health'])
    for name in ('BarBorder', 'BarGlow', 'Shadow'):
        assert widgets[name].get_editor_property('Brush').get_editor_property('DrawAs') == ue.SlateBrushDrawType.NO_DRAW_TYPE
    assert widgets['BackgroundBlur_145'].get_editor_property('BlurStrength') == 0.
    assert widgets['BackgroundBlur_145'].get_editor_property('BlurRadius') == 0
    for key in VISUAL_MATERIAL_KEYS:
        values = art.material_parameters(materials[key])
        assert values['static_switch']['ArtEnabled'] and not values['static_switch']['IsGlowFill']
        assert values['scalar']['Health_Current'] == 1. and values['scalar']['Health_Updated'] == 1.
        assert values['scalar']['OffsetTransitions'] == 0.
    nodes = flat_master_contract(master)
    # Native expression inputs are protected Python properties. Inspect the
    # already-exported engine records rather than requesting private fields.
    records = report.get('master_after', report['master_before'])['expressions']
    def field(node_name, property_name):
        values = re.findall(r'^   ' + re.escape(property_name) + r'=(.*)$', records[node_name]['raw'], re.M)
        assert len(values) == 1, (node_name, property_name)
        return values[0]
    color = field('MaterialExpressionStaticSwitchParameter_19', 'A')
    assert 'MaterialExpressionTextureSampleParameter2D_0' in color
    alpha = field('MaterialExpressionMultiply_9', 'A')
    minimum, one, endpoint = report['new_master_nodes']
    assert endpoint in alpha
    assert minimum in field(endpoint, 'A')
    assert float(field(endpoint, 'ConstB')) == 1.
    assert float(field(endpoint, 'EqualsThreshold')) == 0.
    assert all(one in field(endpoint, name) for name in ('AGreaterThanB', 'AEqualsB'))
    assert 'MaterialExpressionLinearInterpolate_41' in field(endpoint, 'ALessThanB')
    assert float(field(one, 'R')) == 1.
    assert 'MaterialExpressionScalarParameter_36' in field(minimum, 'A')
    assert 'MaterialExpressionScalarParameter_5' in field(minimum, 'B')
    # Bounded numeric evidence for the traced UV4.x mask; this is not GPU/PIE proof.
    sharpness = MEL.get_material_instance_scalar_parameter_value(materials['fill'], 'TransitionSharpness')
    def mask(h, u): return min(1., max(0., (h - u) * sharpness))
    samples = (.001, .25, .499, .501, .75, .999)
    steady = {str(h): [1. if h >= 1. else mask(h, u) for u in samples] for h in (0., .25, .5, .75, 1.)}
    assert all(value == 0. for value in steady['0.0']) and all(value == 1. for value in steady['1.0'])
    assert mask(.5, .25) == 1. and mask(.5, .75) == 0.
    damage = [(1. - fade) * mask(.25, .5) + fade * mask(.75, .5) for fade in (1., .5, 0.)]
    assert damage == [1., .5, 0.]
    heal_front = [(1. - t) * .75 + t * .25 for t in (1., .5, 0.)]
    assert heal_front == [.25, .5, .75]
    report['horizontal_numeric_cases'] = {'u': samples, 'steady_fraction_alpha': steady,
        'damage_fade_at_u_half': damage, 'healing_front_u': heal_front,
        'uses_y': False, 'shader_render_verified': False}
    report['flat_validation'] = {'direct_source_RGB': True, 'source_alpha_times_horizontal_transition': True,
        'full_endpoint_mask_1': True, 'empty_mask_0': True, 'procedural_glow_shadow_border_not_drawn': True,
        'geometry_fonts_Q_and_original_graphs_animations_preserved': True,
        'default_full_values_are_UI_preview_only': True, 'PIE': False, 'GPU_pixels_verified': False}


def flat_main():
    initialize()
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none', '-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor(); hud.clean()
    mode = screens.arg(command, 'DCStatusFlatMode', 'inspect|apply|verify')
    run = screens.arg(command, 'DCStatusFlatRun', '[A-Za-z0-9_]{1,64}')
    folder = R / run; folder.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in folder.iterdir())
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1 and Path(next(value for value in logs[0] if value)).resolve() == (folder / 'Editor.log').resolve()
    protected, before_hashes = flat_protection(), flat_hashes()
    report = {'mode': mode, 'run': run, 'complete': False, 'asset_saves': [], 'compile_requested': [],
        'cpp_build': False, 'PIE': False, 'GPU_pixels_verified': False, 'script_sha256': sha(SCRIPT),
        'before_sha256': before_hashes, 'protected_before': protected}
    try:
        settings = hud.export(ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')), folder / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        assets = {name: ue.load_asset(path) for name, path in ASSETS.items()}; assert all(assets.values())
        materials = {name: ue.load_asset(path) for name, path in MATERIALS.items()}; assert all(materials.values())
        master = ue.load_asset(MASTER); assert master
        report['before'] = snapshot(assets, folder, 'before')
        report['materials_before'] = visual_material_state(materials)
        report['master_before'] = master_snapshot(master, folder, 'master_before')
        flat_master_contract(master)
        if mode == 'inspect':
            # Snapshot the user's current layout as the baseline; no forced reset
            # to earlier fonts/geometry. These known preview defaults need updating.
            for key in VISUAL_MATERIAL_KEYS:
                scalars = report['materials_before'][key]['parameters']['scalar']
                assert scalars['Health_Current'] == .875 and scalars['Health_Updated'] == .625
            probe = ue.new_object(ue.MaterialExpressionIf)
            probe.set_editor_property('ConstB', 1.); probe.set_editor_property('EqualsThreshold', 0.)
            report['endpoint_input_contract'] = list(MEL.get_material_expression_input_names(probe))
        else:
            reference = screens.arg(command, 'DCStatusFlatReference', '[A-Za-z0-9_]{1,64}')
            previous = json.loads((R / reference / 'report.json').read_text(encoding='utf-8-sig'))
            assert previous['complete'] and previous['protection_passed'] and previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['script_sha256'] == report['script_sha256'] and previous['protected_before'] == protected
            assert previous['after_sha256'] == before_hashes
            report['reference'] = reference
            if mode == 'apply':
                assert normalized(previous['before']) == normalized(report['before'])
                assert previous['master_before'] == report['master_before'] and previous['materials_before'] == report['materials_before']
                apply_flat_master(master, report)
                health = tree(assets['health'])
                for name in ('BarBorder', 'BarGlow', 'Shadow'):
                    brush = health[name].get_editor_property('Brush')
                    brush.set_editor_property('DrawAs', ue.SlateBrushDrawType.NO_DRAW_TYPE); health[name].set_brush(brush)
                health['BackgroundBlur_145'].call_method('SetBlurStrength', args=(0.,))
                health['BackgroundBlur_145'].call_method('SetBlurRadius', args=(0,))
                for key in VISUAL_MATERIAL_KEYS:
                    for name in ('Health_Current', 'Health_Updated'):
                        MEL.set_material_instance_scalar_parameter_value(materials[key], name, 1.)
                    MEL.update_material_instance(materials[key])
                for bp in assets.values(): screens.compile_widget(bp, report)
                report['after'] = snapshot(assets, folder, 'after'); report['materials_after'] = visual_material_state(materials)
                report['master_after'] = master_snapshot(master, folder, 'master_after')
                preserve_flat(report['before'], report['after'], report['master_before'], report['master_after'],
                    report['materials_before'], report['materials_after'], report['new_master_nodes'])
                validate_flat(assets, materials, master, report)
                allowed = [*ASSETS.values(), *MATERIALS.values(), MASTER]
                hud.clean(allowed); hud.log_check(folder / 'Editor.log')
                assert flat_protection() == protected and flat_hashes() == before_hashes
                # Compile may mark the status BP or dependent MIs dirty as well.
                # All preservation gates above run before any save; save the exact
                # approved package set so no compile-only dirty package is left over.
                for obj, package in [(master, MASTER), *((materials[key], MATERIALS[key]) for key in MATERIALS),
                                     *((assets[key], ASSETS[key]) for key in ASSETS)]:
                    hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
                    report['asset_saves'].append(package)
            else:
                report['new_master_nodes'] = previous['new_master_nodes']
                assert normalized(report['before']) == normalized(previous['after'])
                assert report['master_before'] == previous['master_after'] and report['materials_before'] == previous['materials_after']
                validate_flat(assets, materials, master, report)
                errors = list(MEL.recompile_material(master)); assert not errors, errors
                for bp in assets.values(): screens.compile_widget(bp, report)
                report['after'] = snapshot(assets, folder, 'recompiled'); report['materials_after'] = visual_material_state(materials)
                report['master_after'] = master_snapshot(master, folder, 'master_recompiled')
                assert normalized(report['before']) == normalized(report['after'])
                assert report['master_before'] == report['master_after'] and report['materials_before'] == report['materials_after']
        hud.clean([*ASSETS.values(), MASTER] if mode == 'verify' else ()); hud.log_check(folder / 'Editor.log'); report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc(); raise
    finally:
        report['after_sha256'] = flat_hashes(); report['protection_passed'] = flat_protection() == protected
        if not report['protection_passed'] or (mode != 'apply' and before_hashes != report['after_sha256']): report['complete'] = False
        (folder / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.log('DC_STATUS_FLAT ' + json.dumps({'complete': report['complete'], 'mode': mode, 'saves': report['asset_saves']}))


if __name__ == '__main__':
    import unreal as ue
    if '-dcstatusflatmode=' in ue.SystemLibrary.get_command_line().lower(): flat_main()
    elif '-dcstatusvisualmode=' in ue.SystemLibrary.get_command_line().lower(): visual_main()
    else: main()
