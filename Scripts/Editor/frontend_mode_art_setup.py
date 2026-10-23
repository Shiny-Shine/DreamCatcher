"""Approved three-card Frontend artwork; separate inspect/apply/fresh verify.

Only W_DC_Frontend, W_DC_StoryModeCard, two new mode-card BPs and the seven
named Screens textures may be saved. No map, Config, build, packaging or PIE.
--commands PREFIX prints launches without executing them. Close Editors first.
Apply requires a completed inspection of the user's rebuilt Editor DLL.
"""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import subprocess
import sys
import traceback

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/FrontendArt20261023'
SCRIPT = P / 'Scripts/Editor/frontend_mode_art_setup.py'
ENGINE = P.parent.parent / 'UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
REFERENCE = Path('C:/Users/Min/Downloads/Frame 4487909.png')
SCREEN = '/Game/DreamCatcher/UI/Frontend/W_DC_Frontend'
CARDS = {key: '/Game/DreamCatcher/UI/Frontend/W_DC_' + name + 'ModeCard'
         for key, name in [('story', 'Story'), ('free', 'Free'), ('challenge', 'Challenge')]}
ART_ROOT = '/Game/DreamCatcher/Screens/InGame_UI/'
ART = {key: ART_ROOT + name for key, name in [
    ('background', 'ModeSelect_BG'), ('heading', 'GamemodeSelection'),
    ('info', 'GameInfo'), ('hints', 'Button_Info'), ('story', 'Storymode'),
    ('free', 'Freemode'), ('challenge', 'Challengemode')]}
# Native image bounds, including the transparent glow/shadow margins.
LAYOUT = {'background': [0, 0, 1920, 1080], 'heading': [76, 116, 555, 222],
          'info': [74, 356, 496, 598], 'hints': [1076, 999, 850, 91],
          'story': [432, 108, 624, 862], 'free': [842, 109, 612, 854],
          'challenge': [1253, 110, 611, 850]}
# Input rectangles follow the white card frames. The inverse transform on each
# artwork cancels the button shear, so the already composed PNG stays unchanged.
HIT = {'story': [214, 8, 380, 824, -202],
       'free': [212, 8, 380, 822, -202],
       'challenge': [210, 7, 380, 822, -202]}
BUTTONS = {'story': 'StoryButton', 'free': 'FreeButton', 'challenge': 'ChallengeButton'}
STYLE = '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Foundation/Buttons/ButtonStyle-Clear'
SOURCES = ['Source/DreamCatcher/UI/Frontend/' + name for name in
           ('DCFrontendScreen.h', 'DCFrontendScreen.cpp', 'DCModeCardButton.h')]
EXISTING = {SCREEN, CARDS['story'], *ART.values()}
NEW = {CARDS['free'], CARDS['challenge']}
ALLOWED = EXISTING | NEW


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def disk(package):
    assert package in ALLOWED
    return P / ('Content/' + package[6:] + '.uasset')


def target_hashes():
    return {package: sha(disk(package)) if disk(package).is_file() else None
            for package in sorted(ALLOWED)}


def source_hashes():
    return {name: sha(P / name) for name in SOURCES}


def protection():
    allowed = {disk(package) for package in ALLOWED}
    inventory = {}
    for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (P / top).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat()
                inventory[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    fixed = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll', *SOURCES]
    return {'inventory': inventory, 'sha256': {name: sha(P / name) for name in fixed}}


def commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    result = {}
    for mode in ('inspect', 'apply', 'verify'):
        run = prefix + '_' + mode
        folder = R / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(),
                '-run=pythonscript', '-script=' + SCRIPT.as_posix(), '-unattended',
                '-SCCProvider=None', '-nop4', '-EnablePlugins=UMGToolSet', '-nullrhi', '-nosound',
                '-DCFrontendArtMode=' + mode, '-DCFrontendArtRun=' + run,
                '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode != 'inspect':
            args.append('-DCFrontendArtReference=' + prefix + ('_inspect' if mode == 'apply' else '_apply'))
        result[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
                        'windows_command_line': subprocess.list2cmdline(args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False,
                      'commands': result}, ensure_ascii=False, indent=2))


def tool(name, *args):
    return UMG.call_method(name, args=args)


def load(package):
    obj = ue.load_asset(package)
    assert obj, package
    return obj


def native(name):
    obj = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert obj, name
    return obj


def defaults(bp):
    return ue.get_default_object(bp.generated_class())


def widgets(bp):
    rows = tool('GetWidgets', bp).get_editor_property('widgets')
    result = {}
    for row in rows:
        obj = row.get_editor_property('widget')
        # Native BindWidget declarations may appear as extra entries without
        # a local template. Only this Blueprint's actual WidgetTree is authored.
        if not obj or not obj.get_outer() or obj.get_outer().get_outer() != bp:
            continue
        name = obj.get_name()
        assert name not in result
        result[name] = obj
    return result


def add(bp, cls, name, parent=None, bind=False):
    obj = tool('AddWidget', bp, cls, name, parent, -1).get_editor_property('widget')
    assert obj and obj.get_name() == name, 'Unexpected widget name: ' + name
    if bind:
        tool('ToggleWidgetAsVariable', bp, obj, True)
    return obj


def clear_tree(bp):
    roots = [obj for obj in widgets(bp).values() if obj.get_editor_property('slot') is None]
    for root in roots:
        assert tool('RemoveWidget', bp, root)
    assert not widgets(bp), 'Tree removal left unexpected widgets'


def place(obj, bounds):
    x, y, width, height = bounds
    slot = obj.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot)
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(0, 0)))
    slot.set_alignment(ue.Vector2D(0, 0))
    slot.set_position(ue.Vector2D(x, y))
    slot.set_size(ue.Vector2D(width, height))
    slot.set_auto_size(False)


def image(bp, parent, name, texture, bounds, bind=False):
    obj = add(bp, ue.Image, name, parent, bind)
    obj.set_brush_from_texture(texture, False)
    obj.set_color_and_opacity(ue.LinearColor(1, 1, 1, 1))
    obj.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    place(obj, bounds)
    return obj


def texture_capture(key, folder, label):
    obj = load(ART[key])
    size = [obj.blueprint_get_size_x(), obj.blueprint_get_size_y()]
    assert size == LAYOUT[key][2:], (key, size)
    png = folder / (label + '_' + key + '.png')
    assert not png.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter, task.filename = obj, ue.TextureExporterPNG(), png.as_posix()
    task.automated, task.prompt, task.replace_identical = True, False, True
    assert ue.Exporter.run_asset_export_task(task) and png.is_file()
    return {'path': ART[key], 'size': size, 'source_png': png.as_posix(), 'png_sha256': sha(png),
            'group': str(obj.get_editor_property('LODGroup')),
            'compression': str(obj.get_editor_property('CompressionSettings')),
            'srgb': obj.get_editor_property('SRGB')}


def native_ready(details=None):
    cdo = ue.get_default_object(native('DCFrontendScreen'))
    ready = True
    for name in ('FreeButton', 'ChallengeButton'):
        try:
            assert cdo.get_editor_property(name) is None
            status = 'registered_null_default'
        except Exception as error:
            status = str(error)
            # BindWidget fields are protected. A named protected-property error
            # proves registration; do not read or change the protected value.
            registered = name in status and 'protected' in status.lower()
            ready = ready and registered
        if details is not None:
            details[name] = status
    return ready


def require_manual_save():
    cls = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
    assert cls
    try:
        policy = ue.get_default_object(cls).get_editor_property('SaveOnCompile')
    except TypeError as error:
        # ESaveOnCompile is a non-Blueprint enum in this engine build. Its
        # Python conversion error reports the native value. The engine header
        # BlueprintEditorSettings.h declares SoC_Never as 0; fail closed for
        # every other property, error or value. Do not change the setting.
        message = str(error)
        assert "property 'SaveOnCompile' (ByteProperty)" in message
        assert "Cannot pythonize '0' (int64)" in message, 'SaveOnCompile must already be Never'
        return
    assert int(policy) == 0, 'SaveOnCompile must already be Never; settings are not changed'


def inspect(folder, report):
    assert all(disk(package).is_file() for package in EXISTING)
    assert all(not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package) for package in NEW)
    for key, package in [('screen', SCREEN), ('story', CARDS['story'])]:
        bp = load(package)
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == native(
            'DCFrontendScreen' if key == 'screen' else 'DCModeCardButton')
        report.setdefault('original_widgets', {})[key] = {
            'description': str(tool('GetWidgetDescription', bp).get_editor_property('description')),
            'export': hud.export(bp, folder / (key + '_before.t3d'))}
    report['textures'] = {key: texture_capture(key, folder, 'source') for key in ART}
    report['native_bindings'] = {}
    report['native_ready'] = native_ready(report['native_bindings'])
    report['user_build_required'] = not report['native_ready']
    report['reference_png_sha256'] = sha(REFERENCE)
    report['layout'] = LAYOUT
    report['hit_regions'] = HIT
    style = defaults(load(STYLE))
    report['button_padding'] = style.get_editor_property('ButtonPadding').export_text()
    report['button_style_sha256'] = sha(P / ('Content/' + STYLE[6:] + '.uasset'))


def compile_widget(bp, report):
    assert tool('CompileWidgetBlueprint', bp), bp.get_path_name()
    report['compiled'].append(bp.get_path_name().split('.')[0])


def author_card(key, report):
    package = CARDS[key]
    if key == 'story':
        bp = load(package)
        # Toolset tree mutations trigger Compile. Use the existing common
        # parent until the replacement's required ArtworkImage is present.
        ue.BlueprintEditorLibrary.reparent_blueprint(bp, native('DCButtonBase'))
    else:
        assert not ue.EditorAssetLibrary.does_asset_exist(package)
        bp = tool('CreateWidgetBlueprint', package.rsplit('/', 1)[0], package.rsplit('/', 1)[1], native('DCButtonBase'))
        assert bp
    clear_tree(bp)
    canvas = add(bp, ue.CanvasPanel, 'CardContent')
    left, top, width, height, delta = HIT[key]
    slope = delta / height
    angle = math.degrees(math.atan(slope))
    pad = defaults(load(STYLE)).get_editor_property('ButtonPadding')
    artwork = image(bp, canvas, 'ArtworkImage', load(ART[key]),
                    [-left + slope * top - pad.left, -top - pad.top, *LAYOUT[key][2:]], bind=True)
    artwork.set_render_transform_pivot(ue.Vector2D(0, 0))
    artwork.set_render_shear(ue.Vector2D(-angle, 0))
    ue.BlueprintEditorLibrary.reparent_blueprint(bp, native('DCModeCardButton'))
    cdo = defaults(bp)
    cdo.set_editor_property('Artwork', load(ART[key]))
    cdo.set_editor_property('Style', load(STYLE).generated_class())
    cdo.set_is_focusable(True)
    compile_widget(bp, report)
    return bp


def author_screen(cards, report, bp, settings_class):
    clear_tree(bp)
    backdrop = add(bp, ue.Border, 'Backdrop')
    backdrop.set_padding(ue.Margin(0, 0, 0, 0))
    backdrop.set_brush_color(ue.LinearColor(0, 0, 0, 1))
    scale = add(bp, ue.ScaleBox, 'ReferenceScale', backdrop)
    scale.set_stretch(ue.Stretch.SCALE_TO_FIT)
    scale.set_stretch_direction(ue.StretchDirection.BOTH)
    frame = add(bp, ue.SizeBox, 'DesignFrame', scale)
    frame.set_width_override(1920)
    frame.set_height_override(1080)
    frame.get_editor_property('slot').set_horizontal_alignment(ue.HorizontalAlignment.H_ALIGN_CENTER)
    frame.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_CENTER)
    canvas = add(bp, ue.CanvasPanel, 'MenuLayout', frame)
    canvas.set_clipping(ue.WidgetClipping.CLIP_TO_BOUNDS_ALWAYS)
    for key, name in [('background', 'BackgroundImage'), ('heading', 'HeadingImage'), ('info', 'GameInfoImage')]:
        image(bp, canvas, name, load(ART[key]), LAYOUT[key])
    for key in CARDS:
        button = add(bp, cards[key].generated_class(), BUTTONS[key], canvas, bind=True)
        x, y = LAYOUT[key][:2]
        left, top, width, height, delta = HIT[key]
        place(button, [x + left, y + top, width, height])
        button.set_render_transform_pivot(ue.Vector2D(0, 0))
        button.set_render_shear(ue.Vector2D(math.degrees(math.atan(delta / height)), 0))
    image(bp, canvas, 'ButtonInfoImage', load(ART['hints']), LAYOUT['hints'])
    status = add(bp, ue.TextBlock, 'StatusText', canvas, bind=True)
    status.set_text('')
    status.set_color_and_opacity(ue.SlateColor(specified_color=ue.LinearColor(.04, .04, .2, 1)))
    font = status.get_editor_property('font')
    font.set_editor_property('size', 20)
    status.set_font(font)
    status.set_auto_wrap_text(True)
    status.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    place(status, [76, 965, 900, 65])
    ue.BlueprintEditorLibrary.reparent_blueprint(bp, native('DCFrontendScreen'))
    defaults(bp).set_editor_property('SettingsScreenClass', settings_class)
    compile_widget(bp, report)
    return bp


def verify_authored(folder, report):
    screen = load(SCREEN)
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(screen) == native('DCFrontendScreen')
    sw = widgets(screen)
    assert set(sw) == {'Backdrop', 'ReferenceScale', 'DesignFrame', 'MenuLayout', 'BackgroundImage',
                       'HeadingImage', 'GameInfoImage', 'StoryButton', 'FreeButton', 'ChallengeButton',
                       'ButtonInfoImage', 'StatusText'}, set(sw)
    assert isinstance(sw['ReferenceScale'], ue.ScaleBox)
    assert sw['ReferenceScale'].get_editor_property('Stretch') == ue.Stretch.SCALE_TO_FIT
    assert sw['DesignFrame'].get_editor_property('WidthOverride') == 1920
    assert sw['DesignFrame'].get_editor_property('HeightOverride') == 1080
    assert str(sw['StatusText'].get_editor_property('Text')) == ''
    for key, name in [('background', 'BackgroundImage'), ('heading', 'HeadingImage'),
                      ('info', 'GameInfoImage'), ('hints', 'ButtonInfoImage')]:
        obj = sw[name]
        assert obj.get_editor_property('Brush').get_editor_property('ResourceObject') == load(ART[key])
        slot = obj.get_editor_property('slot')
        pos, size = slot.get_position(), slot.get_size()
        assert [pos.x, pos.y, size.x, size.y] == LAYOUT[key]
    report['geometry'] = {}
    for key, package in CARDS.items():
        bp = load(package)
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == native('DCModeCardButton')
        cw = widgets(bp)
        assert set(cw) == {'CardContent', 'ArtworkImage'}
        assert defaults(bp).get_editor_property('Artwork') == load(ART[key])
        assert sw[BUTTONS[key]].get_class() == bp.generated_class()
        button = sw[BUTTONS[key]]
        slot = button.get_editor_property('slot')
        pos, size = slot.get_position(), slot.get_size()
        left, top, width, height, delta = HIT[key]
        assert [pos.x, pos.y, size.x, size.y] == [LAYOUT[key][0] + left, LAYOUT[key][1] + top, width, height]
        outer = button.get_editor_property('RenderTransform')
        inner = cw['ArtworkImage'].get_editor_property('RenderTransform')
        assert abs(outer.shear.x + inner.shear.x) < .0001 and outer.shear.y == inner.shear.y == 0
        # Check the resulting image transform at every corner, including the
        # transparent margins. No additional visual skew or scaling is allowed.
        pad = defaults(load(STYLE)).get_editor_property('ButtonPadding')
        ipos = cw['ArtworkImage'].get_editor_property('slot').get_position()
        isize = cw['ArtworkImage'].get_editor_property('slot').get_size()
        assert [isize.x, isize.y] == LAYOUT[key][2:]
        slope = math.tan(math.radians(outer.shear.x))
        inverse = math.tan(math.radians(inner.shear.x))
        corners = []
        for ix, iy in [(0, 0), (isize.x, 0), (0, isize.y), (isize.x, isize.y)]:
            py = top + pad.top + ipos.y + iy
            px = left + pad.left + ipos.x + ix + inverse * iy + slope * (pad.top + ipos.y + iy)
            assert abs(px - ix) < .002 and abs(py - iy) < .002, (key, px, py)
            corners.append([px, py])
        report['geometry'][key] = {'input_bounds': [pos.x, pos.y, size.x, size.y],
                                   'artwork_corners_relative_to_reference': corners,
                                   'outer_shear_degrees': outer.shear.x, 'inner_shear_degrees': inner.shear.x}
    report['widget_exports'] = {key: hud.export(load(package), folder / (key + '_authored.t3d'))
                               for key, package in {'screen': SCREEN, **CARDS}.items()}
    report['textures'] = {key: texture_capture(key, folder, 'final') for key in ART}
    for row in report['textures'].values():
        assert row['group'] == str(ue.TextureGroup.TEXTUREGROUP_UI)
        assert row['compression'] == str(ue.TextureCompressionSettings.TC_EDITOR_ICON)
        assert row['srgb']


def apply(folder, report, previous):
    require_manual_save()
    assert previous['mode'] == 'inspect' and previous['native_ready'] and native_ready(), 'User Editor build required'
    assert previous['after_sha256'] == target_hashes(), 'Target assets changed since inspection'
    assert previous['native_sources'] == source_hashes()
    assert previous['dll_sha256'] == sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll')
    assert previous['reference_png_sha256'] == sha(REFERENCE)
    assert previous['button_style_sha256'] == sha(P / ('Content/' + STYLE[6:] + '.uasset'))
    assert all(not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package) for package in NEW)
    textures = [load(package) for package in ART.values()]
    for obj in textures:
        obj.set_editor_property('LODGroup', ue.TextureGroup.TEXTUREGROUP_UI)
        obj.set_editor_property('CompressionSettings', ue.TextureCompressionSettings.TC_EDITOR_ICON)
        obj.set_editor_property('SRGB', True)
    # Remove the screen's required StoryButton contract before changing the
    # referenced card's parent, then restore both final native contracts before
    # validation or any save. Preserve the pre-existing settings API default.
    screen = load(SCREEN)
    settings_class = defaults(screen).get_editor_property('SettingsScreenClass')
    ue.BlueprintEditorLibrary.reparent_blueprint(screen, native('DCLyraActivatableWidget'))
    cards = {key: author_card(key, report) for key in CARDS}
    screen = author_screen(cards, report, screen, settings_class)
    verify_authored(folder, report)
    assert all(report['textures'][key]['png_sha256'] == previous['textures'][key]['png_sha256'] for key in ART), 'Source PNG bytes changed'
    assert protection() == report['protection_before']
    hud.clean(ALLOWED)
    hud.log_check(folder / 'Editor.log')
    for obj in [*textures, *cards.values(), screen]:
        package = obj.get_path_name().split('.')[0]
        assert package in ALLOWED
        hud.assert_no_other_editor()
        assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), package
        report['saved'].append(package)
    assert set(report['saved']) == ALLOWED and len(report['saved']) == 11
    hud.clean()


def run_editor():
    global ue, hud, UMG
    import unreal as ue
    spec = importlib.util.spec_from_file_location('frontend_art_guards', SCRIPT.parent / 'ingame_hud_layout_setup.py')
    hud = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(hud)
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    hud.assert_no_other_editor()
    mode = re.search(r'-DCFrontendArtMode=(inspect|apply|verify)\b', command).group(1)
    run = re.search(r'-DCFrontendArtRun=([A-Za-z0-9_]{1,64})\b', command).group(1)
    folder = R / run
    folder.mkdir(parents=True, exist_ok=True)
    output = folder / 'report.json'
    assert not output.exists(), 'Preserve existing results; use a fresh run'
    logs = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(logs) == 1 and Path(next(v for v in logs[0] if v)).resolve() == (folder / 'Editor.log').resolve()
    UMG = ue.get_default_object(ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
    assert UMG
    before = protection()
    report = {'mode': mode, 'run': run, 'complete': False, 'saved': [], 'compiled': [],
              'PIE': False, 'cpp_build': False, 'physical_input_verified': False,
              'runtime_visual_acceptance': False, 'layout': LAYOUT, 'hit_regions': HIT,
              'script_sha256': sha(SCRIPT), 'native_sources': source_hashes(),
              'dll_sha256': sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'),
              'before_sha256': target_hashes(), 'protection_before': before}
    try:
        hud.clean()
        previous = None
        if mode != 'inspect':
            reference = re.search(r'-DCFrontendArtReference=([A-Za-z0-9_]{1,64})\b', command).group(1)
            previous = read(R / reference / 'report.json')
            assert previous['complete'] and previous['protection_passed']
            # Bind the phase to the approved destinations, geometry, target
            # hashes, sources and DLL. Record script hashes for provenance;
            # read-only API corrections do not invalidate an unchanged plan.
            assert previous['layout'] == LAYOUT and previous['hit_regions'] == HIT
            assert set(previous['after_sha256']) == ALLOWED
            report['reference_script_sha256'] = previous['script_sha256']
            report['reference'] = reference
        if mode == 'inspect':
            inspect(folder, report)
        elif mode == 'apply':
            apply(folder, report, previous)
        else:
            require_manual_save()
            assert previous['mode'] == 'apply' and previous['after_sha256'] == target_hashes()
            assert previous['native_sources'] == source_hashes() and native_ready()
            assert previous['dll_sha256'] == report['dll_sha256']
            # Explicit fresh-process Compile is in memory only; no auto-save.
            for package in [*CARDS.values(), SCREEN]:
                compile_widget(load(package), report)
            verify_authored(folder, report)
            assert all(report['textures'][key]['png_sha256'] == previous['textures'][key]['png_sha256'] for key in ART)
            assert report['before_sha256'] == target_hashes(), 'Verification saved an asset unexpectedly'
            hud.clean(ALLOWED)
        hud.log_check(folder / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = target_hashes()
        report['protection_passed'] = protection() == before
        report['complete'] = report['complete'] and report['protection_passed']
        if mode == 'inspect':
            report['complete'] = report['complete'] and report['before_sha256'] == report['after_sha256']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_FRONTEND_ART ' + json.dumps({key: report.get(key) for key in
              ('mode', 'run', 'complete', 'protection_passed', 'native_ready', 'user_build_required', 'saved', 'error')}))
    assert report['complete'], 'Inspect report before proceeding'


if __name__ == '__main__':
    if '--commands' in sys.argv:
        commands(sys.argv[sys.argv.index('--commands') + 1])
    else:
        run_editor()
