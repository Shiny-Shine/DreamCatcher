"""Bounded 2D Frontend setup after the user's Development Editor build.

Run in a separate unattended UnrealEditor-Cmd process with -EnablePlugins=UMGToolSet -SCCProvider=None
and -DCFrontendMode=create|verify|connect. No C++ build, source-project migration,
PIE, deletion, bulk save, or Level_1 map write. New paths must not already exist.
create: seven new assets; verify: fresh-process reload, no saves;
connect: one existing GameMode.ReturnMap plus two narrowly edited config files.
"""
import hashlib
import json
import re
from pathlib import Path

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/Frontend'
PATHS = {
    'card': '/Game/DreamCatcher/UI/Frontend/W_DC_StoryModeCard',
    'screen': '/Game/DreamCatcher/UI/Frontend/W_DC_Frontend',
    'loading': '/Game/DreamCatcher/UI/Frontend/W_DC_LoadingScreen',
    'component': '/Game/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent',
    'experience': '/Game/DreamCatcher/Experiences/B_DC_FrontEndExperience',
    'mode': '/Game/DreamCatcher/Blueprints/Core/BP_DC_FrontEndGameMode',
    'map': '/Game/DreamCatcher/Maps/L_DC_FrontEnd',
}
STORY = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1'
STAGE_MODE = '/Game/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode'
SHARED = '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI'
# The activatable screen owns the inner W_SettingsPanel and Apply/Cancel actions.
SETTINGS = SHARED + '/Settings/W_LyraSettingScreen'
MENU_BUTTON = SHARED + '/Menu/W_LyraMenuButton'
CLEAR_STYLE = SHARED + '/Foundation/Buttons/ButtonStyle-Clear'


def disk(package, world=False):
    return PROJECT / ('Content/' + package.removeprefix('/Game/') + ('.umap' if world else '.uasset'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native(name):
    value = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert value, 'User C++ build required: ' + name
    return value


def load(path):
    obj = ue.load_asset(path)
    assert obj, path
    return obj


def defaults(bp):
    return ue.get_default_object(bp.generated_class())


def tool(name, *args):
    return UMG.call_method(name, args=args)


def color(r, g, b, a=1):
    return ue.LinearColor(r, g, b, a)


def widget(bp, cls, name, parent=None):
    row = tool('AddWidget', bp, cls, name, parent, -1)
    obj = row.get_editor_property('widget')
    assert obj, name
    return obj


def bind(bp, obj):
    tool('ToggleWidgetAsVariable', bp, obj, True)
    return obj


def place(obj, x, y, width, height, ax=0, ay=0, alignx=0, aligny=0):
    slot = obj.get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot), obj.get_name()
    slot.set_anchors(ue.Anchors(ue.Vector2D(ax, ay), ue.Vector2D(ax, ay)))
    slot.set_alignment(ue.Vector2D(alignx, aligny))
    slot.set_position(ue.Vector2D(x, y))
    slot.set_size(ue.Vector2D(width, height))


def fill(obj):
    slot = obj.get_editor_property('slot')
    slot.set_anchors(ue.Anchors(ue.Vector2D(0, 0), ue.Vector2D(1, 1)))
    slot.set_offsets(ue.Margin(0, 0, 0, 0))


def label(bp, parent, name, text, size, tint=None):
    obj = widget(bp, ue.TextBlock, name, parent)
    obj.set_text(text)
    font = obj.get_editor_property('font')
    font.set_editor_property('size', size)
    obj.set_font(font)
    obj.set_color_and_opacity(ue.SlateColor(specified_color=tint or color(1, 1, 1)))
    obj.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    return obj


def new_widget(key, parent):
    path = PATHS[key]
    bp = tool('CreateWidgetBlueprint', path.rsplit('/', 1)[0], path.rsplit('/', 1)[1], parent)
    assert bp
    # Remove only the new Blueprint's engine-selected default root.
    for row in tool('GetWidgets', bp).get_editor_property('widgets'):
        if not row.get_editor_property('parent'):
            assert tool('RemoveWidget', bp, row.get_editor_property('widget'))
            break
    return bp


def new_blueprint(key, parent):
    bp = ue.BlueprintEditorLibrary.create_blueprint_asset_with_parent(PATHS[key], parent)
    assert bp
    return bp


def compile_bp(bp, is_widget=False):
    ok = tool('CompileWidgetBlueprint', bp) if is_widget else ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert ok, bp.get_path_name()


def create_assets(result):
    for key, path in PATHS.items():
        assert not disk(path, key == 'map').exists() and not ue.EditorAssetLibrary.does_asset_exist(path), path
    # Use a parent with no required BindWidget fields until the tree is complete.
    card = new_widget('card', native('DCButtonBase'))
    frame = bind(card, widget(card, ue.Border, 'CardOutline'))
    frame.set_padding(ue.Margin(4, 4, 4, 4))
    frame.set_brush_color(color(.75, .75, .8))
    inner = widget(card, ue.Border, 'CardBackground', frame)
    inner.set_padding(ue.Margin(0, 0, 0, 0))
    inner.set_brush_color(color(.065, .025, .16))
    canvas = widget(card, ue.CanvasPanel, 'CardContent', inner)
    artwork = bind(card, widget(card, ue.Image, 'ArtworkImage', canvas))
    fill(artwork)
    artwork.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    bottom = widget(card, ue.Border, 'DescriptionBackground', canvas)
    bottom.set_brush_color(color(.025, .015, .06, .94))
    place(bottom, 0, 0, 412, 250, 0, 1, 0, 1)
    number = bind(card, label(card, canvas, 'NumberText', '01', 36))
    place(number, 28, 25, 70, 50)
    eyebrow = bind(card, label(card, canvas, 'LabelText', 'STORY MODE', 19))
    place(eyebrow, 100, 36, 270, 40)
    title = bind(card, label(card, canvas, 'TitleText', '스토리 모드', 43))
    place(title, 28, -205, 360, 65, 0, 1)
    description = bind(card, label(card, canvas, 'DescriptionText', '스테이지에 진입합니다.', 20))
    description.set_auto_wrap_text(True)
    place(description, 28, -125, 340, 80, 0, 1)
    arrow = label(card, canvas, 'Arrow', '›', 48)
    place(arrow, -55, -62, 45, 60, 1, 1)
    ue.BlueprintEditorLibrary.reparent_blueprint(card, native('DCModeCardButton'))
    cdo = defaults(card)
    cdo.set_editor_property('ModeNumber', '01')
    cdo.set_editor_property('ModeLabel', 'STORY MODE')
    cdo.set_editor_property('ModeTitle', '스토리 모드')
    cdo.set_editor_property('ModeDescription', '스테이지에 진입합니다.')
    cdo.set_editor_property('Style', load(CLEAR_STYLE).generated_class())
    compile_bp(card, True)

    screen = new_widget('screen', native('DCLyraActivatableWidget'))
    backdrop = widget(screen, ue.Border, 'Backdrop')
    backdrop.set_padding(ue.Margin(0, 0, 0, 0))
    backdrop.set_brush_color(color(.035, .04, .06))
    safe = widget(screen, ue.SafeZone, 'SafeZone', backdrop)
    layout = widget(screen, ue.CanvasPanel, 'MenuLayout', safe)
    logo = label(screen, layout, 'GameTitle', 'DREAMCATCHER', 42)
    place(logo, 96, 100, 600, 70)
    heading = label(screen, layout, 'ScreenTitle', '모드 선택', 56)
    place(heading, 96, 250, 500, 95)
    summary = label(screen, layout, 'ScreenDescription', '스토리 모드를 선택해 시작하세요.', 22, color(.7, .72, .8))
    place(summary, 96, 365, 560, 65)
    story_button = bind(screen, widget(screen, card.generated_class(), 'StoryButton', layout))
    place(story_button, 210, 0, 420, 800, .5, .5, .5, .5)
    story_button.set_render_shear(ue.Vector2D(-11.5, 0))
    button_class = load(MENU_BUTTON).generated_class()
    settings = bind(screen, widget(screen, button_class, 'SettingsButton', layout))
    # Set template data without calling Blueprint UI events on an uninitialized archetype.
    settings.set_editor_property('ButtonText', '설정')
    settings.set_editor_property('bOverride_ButtonText', False)
    place(settings, 96, -230, 330, 70, 0, 1)
    quit_button = bind(screen, widget(screen, button_class, 'QuitButton', layout))
    quit_button.set_editor_property('ButtonText', '종료')
    quit_button.set_editor_property('bOverride_ButtonText', False)
    place(quit_button, 96, -140, 330, 70, 0, 1)
    status = bind(screen, label(screen, layout, 'StatusText', '', 20, color(.9, .75, .95)))
    place(status, 96, 470, 560, 130)
    status.set_auto_wrap_text(True)
    hints = label(screen, layout, 'KeyboardHints', '방향키  이동     Enter  확인     ESC  종료', 21)
    place(hints, -96, -58, 650, 40, 1, 1, 1, 0)
    hints.set_editor_property('Justification', ue.TextJustify.RIGHT)
    ue.BlueprintEditorLibrary.reparent_blueprint(screen, native('DCFrontendScreen'))
    defaults(screen).set_editor_property('SettingsScreenClass', load(SETTINGS).generated_class())
    compile_bp(screen, True)

    loading = new_widget('loading', ue.UserWidget)
    background = widget(loading, ue.Border, 'LoadingBackground')
    background.set_brush_color(color(.035, .04, .06))
    loading_text = label(loading, background, 'LoadingText', '불러오는 중…', 28)
    loading_text.set_editor_property('Justification', ue.TextJustify.CENTER)
    loading_text.get_editor_property('slot').set_vertical_alignment(ue.VerticalAlignment.V_ALIGN_CENTER)
    compile_bp(loading, True)

    component = new_blueprint('component', native('DCLyraFrontendStateComponent'))
    defaults(component).set_editor_property('MainScreenClass', screen.generated_class())
    # PC local-user initialization uses the original flow without a Press Start page.
    defaults(component).set_editor_property('PressStartScreenClass', None)
    compile_bp(component)
    experience = new_blueprint('experience', native('DCLyraExperienceDefinition'))
    exp = defaults(experience)
    add = ue.new_object(ue.load_class(None, '/Script/GameFeatures.GameFeatureAction_AddComponents'), exp, 'FrontendUI')
    entries = add.get_editor_property('ComponentList')
    entries.resize(1)
    entry = entries[0]
    component_class_path = component.generated_class().get_path_name()
    assert entry.import_text('(ActorClass="/Script/DreamCatcher.DCLyraGameState",ComponentClass="' + component_class_path + '",bClientComponent=True,bServerComponent=False)')
    entries[0] = entry
    add.set_editor_property('ComponentList', entries)
    perf = ue.new_object(native('DCApplyFrontendPerfSettingsAction'), exp, 'FrontendPerformance')
    exp.set_editor_property('Actions', [add, perf])
    exp.set_editor_property('GameFeaturesToEnable', [])
    exp.set_editor_property('ActionSets', [])
    exp.set_editor_property('DefaultPawnData', None)
    compile_bp(experience)
    mode = new_blueprint('mode', native('DCFrontendGameMode'))
    expid = defaults(mode).get_editor_property('DefaultExperience')
    assert expid.import_text('DCLyraExperienceDefinition:B_DC_FrontEndExperience')
    defaults(mode).set_editor_property('DefaultExperience', expid)
    defaults(mode).set_editor_property('StoryMap', load(STORY))
    compile_bp(mode)

    for key, obj in [('card', card), ('screen', screen), ('loading', loading), ('component', component), ('experience', experience), ('mode', mode)]:
        assert obj.get_path_name().split('.')[0] == PATHS[key]
        assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
        result['saved'].append(PATHS[key])
    world = ue.EditorLoadingAndSavingUtils.new_blank_map(False)
    assert world
    world.get_world_settings().set_editor_property('DefaultGameMode', mode.generated_class())
    assert ue.EditorLoadingAndSavingUtils.save_map(world, PATHS['map'])
    result['saved'].append(PATHS['map'])
    result['artwork'] = 'Replaceable Artwork property is unset; no draft-image crop/import.'


def verify_assets(result):
    assets = {key: load(path) for key, path in PATHS.items()}
    for key, parent in [('card', 'DCModeCardButton'), ('screen', 'DCFrontendScreen'), ('component', 'DCLyraFrontendStateComponent'), ('experience', 'DCLyraExperienceDefinition'), ('mode', 'DCFrontendGameMode')]:
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets[key]) == native(parent), key
    for key, required in [('card', {'CardOutline', 'ArtworkImage', 'NumberText', 'LabelText', 'TitleText', 'DescriptionText'}), ('screen', {'StoryButton', 'SettingsButton', 'QuitButton', 'StatusText'})]:
        rows = tool('GetWidgets', assets[key]).get_editor_property('widgets')
        names = {str(row.get_editor_property('widget_name')) for row in rows}
        assert required <= names, (key, required - names)
        assert not any('Free' in name for name in names)
    exp = defaults(assets['experience'])
    assert exp.get_editor_property('DefaultPawnData') is None
    assert len(exp.get_editor_property('Actions')) == 2
    assert not exp.get_editor_property('GameFeaturesToEnable') and not exp.get_editor_property('ActionSets')
    mode = defaults(assets['mode'])
    assert mode.get_editor_property('DefaultPawnClass') is None
    assert mode.get_editor_property('PlayerControllerClass').get_path_name() == '/Script/CommonGame.CommonPlayerController'
    assert mode.get_editor_property('PlayerStateClass').get_path_name() == '/Script/Engine.PlayerState'
    assert 'B_DC_FrontEndExperience' in mode.get_editor_property('DefaultExperience').export_text()
    assert STORY in str(mode.get_editor_property('StoryMap').get_path_name())
    assert assets['map'].get_world_settings().get_editor_property('DefaultGameMode') == assets['mode'].generated_class()
    assert defaults(assets['screen']).get_editor_property('SettingsScreenClass') == load(SETTINGS).generated_class()
    assert defaults(assets['component']).get_editor_property('MainScreenClass') == assets['screen'].generated_class()
    assert defaults(assets['component']).get_editor_property('PressStartScreenClass') is None
    result['asset_sha256'] = {key: sha(disk(path, key == 'map')) for key, path in PATHS.items()}
    result['dll_sha256'] = sha(PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll')
    result['runtime_and_visual_verified'] = False


def connect_assets(result):
    previous = json.loads((REPORTS / 'verify.json').read_text(encoding='utf-8'))
    assert previous['complete']
    verify_assets(result)
    assert previous['asset_sha256'] == result['asset_sha256'], 'Assets changed after reload verification'
    assert previous['dll_sha256'] == result['dll_sha256'], 'DLL changed after reload verification'
    engine_path = PROJECT / 'Config/DefaultEngine.ini'
    game_path = PROJECT / 'Config/DefaultGame.ini'
    engine_text = engine_path.read_text(encoding='utf-8-sig')
    game_text = game_path.read_text(encoding='utf-8-sig')
    old_start = 'GameDefaultMap=' + STORY + '.Level_1'
    assert engine_text.count(old_start) == 1
    assert 'LoadingScreenWidget=' not in game_text
    cook_line = '+MapsToCook=(FilePath="' + STORY + '")'
    assert game_text.count(cook_line) == 1 and PATHS['map'] not in game_text
    stage_bp = load(STAGE_MODE)
    old_return = defaults(stage_bp).get_editor_property('ReturnMap')
    assert old_return and old_return.get_path_name() == STORY + '.Level_1'
    result['previous_return_map'] = old_return.get_path_name()
    defaults(stage_bp).set_editor_property('ReturnMap', load(PATHS['map']))
    compile_bp(stage_bp)
    assert ue.EditorAssetLibrary.save_loaded_asset(stage_bp, only_if_is_dirty=False)
    result['saved'].append(STAGE_MODE)
    engine_text = engine_text.replace(old_start, 'GameDefaultMap=' + PATHS['map'] + '.L_DC_FrontEnd')
    game_text = game_text.replace(cook_line, cook_line + '\n+MapsToCook=(FilePath="' + PATHS['map'] + '")')
    game_text = game_text.replace('; Level_1 now hosts the accepted R13 gameplay flow; ReturnMap temporarily restarts it.', '; Production story and 2D menu maps; retry reloads the story, return opens the menu.')
    game_text += '\n; Existing CommonLoadingScreen lifecycle, with a 2D presentation widget.\n[/Script/CommonLoadingScreen.CommonLoadingScreenSettings]\nLoadingScreenWidget=' + PATHS['loading'] + '.W_DC_LoadingScreen_C\n'
    engine_path.write_text(engine_text, encoding='utf-8', newline='\r\n')
    game_path.write_text(game_text, encoding='utf-8', newline='\r\n')
    result['config_edits'] = ['GameDefaultMap', 'MapsToCook (add menu)', 'CommonLoadingScreen.LoadingScreenWidget']
    result['editor_startup_map_unchanged'] = 'EditorStartupMap=' + STORY + '.Level_1' in engine_text


def main():
    global UMG
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    command = ue.SystemLibrary.get_command_line()
    assert '-unattended' in command.lower() and '-run=pythonscript' in command.lower()
    assert '-sccprovider=none' in command.lower(), 'Explicitly disable Editor source control; do not auto-stage new assets'
    source_control = ue.get_default_object(ue.load_class(None, '/Script/SourceControl.SourceControlHelpers'))
    assert not source_control.call_method('IsEnabled'), 'Editor source control must be disabled'
    match = re.search(r'-DCFrontendMode=(create|verify|connect)\b', command)
    assert match, 'Explicit -DCFrontendMode is required'
    mode = match.group(1)
    out = REPORTS / (mode + '.json')
    assert not out.exists(), 'Preserve the earlier report and inspect before retrying: ' + str(out)
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    UMG = ue.get_default_object(ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
    assert UMG, 'Use the bundled engine plugin via -EnablePlugins=UMGToolSet'
    for name in ['DCFrontendGameMode', 'DCFrontendScreen', 'DCModeCardButton', 'DCLyraFrontendStateComponent', 'DCApplyFrontendPerfSettingsAction']:
        native(name)
    protected = [disk(STORY, True), PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    if mode != 'connect':
        protected += [disk(STAGE_MODE), PROJECT / 'Config/DefaultEngine.ini', PROJECT / 'Config/DefaultGame.ini']
    before = {str(path): sha(path) for path in protected}
    result = dict(mode=mode, complete=False, saved=[], protected_sha256=before, cpp_build=False, pie=False)
    try:
        {'create': create_assets, 'verify': verify_assets, 'connect': connect_assets}[mode](result)
        result['protected_changes'] = [path for path, digest in before.items() if sha(Path(path)) != digest]
        assert not result['protected_changes']
        result['complete'] = True
    finally:
        REPORTS.mkdir(parents=True, exist_ok=True)
        out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print('DC_FRONTEND_' + mode.upper() + '_COMPLETE')


if __name__ == '__main__':
    main()
