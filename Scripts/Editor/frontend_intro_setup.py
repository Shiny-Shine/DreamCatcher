"""After the user's build: apply/verify exactly three approved frontend PV assets.

Separate unattended UnrealEditor-Cmd, -EnablePlugins=UMGToolSet -SCCProvider=None
-DCFrontendIntroMode=apply|verify -DCFrontendIntroRun=<id>. No build/PIE/config
write, binary file copying, bulk save, source-control operation or map save.
"""
import hashlib
import json
from pathlib import Path
import re
import sys
import time

import unreal as ue

sys.path.insert(0, str(Path(__file__).resolve().parent))
from base_damage_setup import assert_no_other_editor, export, without_properties, graph_snapshot

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/FrontendPV20261024'
SOURCE = '/Game/DreamCatcher/Screens/DREAMCATCHER_PV'
WIDGET = '/Game/DreamCatcher/UI/Frontend/W_DC_FrontendIntro'
COMPONENT = '/Game/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent'
MOVIE = PROJECT / 'Content/Movies/DREAMCATCHER_PV.mp4'
TARGETS = (SOURCE, WIDGET, COMPONENT)


def disk(package):
    assert package in TARGETS
    return PROJECT / ('Content/' + package.removeprefix('/Game/') + '.uasset')


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def inventory():
    allowed = {disk(p) for p in TARGETS}
    result = {}
    for folder in ('Content', 'Source', 'Config', 'Plugins', 'Saved/SaveGames'):
        for path in (PROJECT / folder).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat()
                result[str(path)] = [stat.st_size, stat.st_mtime_ns]
    return result


def defaults(bp):
    return ue.get_default_object(bp.generated_class())


def snapshot(asset, directory, label):
    obj = defaults(asset) if isinstance(asset, ue.Blueprint) else asset
    return export(obj, directory / (label + '.t3d'))


def main():
    command = ue.SystemLibrary.get_command_line()
    mode_match = re.search(r'-DCFrontendIntroMode=(apply|verify)\b', command)
    run_match = re.search(r'-DCFrontendIntroRun=([A-Za-z0-9_]+)\b', command)
    assert mode_match and run_match, 'Explicit mode/run are required'
    mode = mode_match.group(1)
    assert '-unattended' in command.lower() and '-run=pythonscript' in command.lower()
    assert '-sccprovider=none' in command.lower() and not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert_no_other_editor()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    directory = REPORTS / ('assets_' + run_match.group(1))
    directory.mkdir(parents=True, exist_ok=True)
    journal_path = directory / 'apply.json'
    out = directory / (mode + '_' + str(time.time_ns()) + '.json')
    result = dict(complete=False, mode=mode, asset_saves=[], cpp_build=False, pie=False)
    protected = inventory()
    protected_hashes = {str(PROJECT / p): sha(PROJECT / p) for p in (
        '.git/HEAD', '.git/index', 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Config/DefaultEngine.ini', 'Config/DefaultGame.ini',
        'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
        'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_2.umap')}
    try:
        settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings = export(ue.get_default_object(settings_class), directory / ('settings_' + str(time.time_ns()) + '.t3d'))
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        native = ue.load_class(None, '/Script/DreamCatcher.DCFrontendIntroScreen')
        assert native, 'User Development Editor/Win64 build is required'
        preparation = json.loads((REPORTS / 'movie_prepare_A.json').read_text(encoding='utf-8'))
        assert MOVIE.is_file() and sha(MOVIE) == preparation['sha256']
        assert '+NonUFSMovies=DREAMCATCHER_PV' in (PROJECT / 'Config/DefaultGame.ini').read_text(encoding='utf-8-sig')
        media = ue.load_asset(SOURCE)
        component = ue.load_asset(COMPONENT)
        assert isinstance(media, ue.FileMediaSource) and isinstance(component, ue.Blueprint)
        assert defaults(component).get_editor_property('MainScreenClass').get_path_name() == '/Game/DreamCatcher/UI/Frontend/W_DC_Frontend.W_DC_Frontend_C'
        assert not defaults(component).get_editor_property('PressStartScreenClass')

        def save(asset):
            package = asset.get_path_name().split('.')[0]
            assert package in TARGETS
            assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
            result['asset_saves'].append(package)
            result.setdefault('saved_sha256', {})[package] = sha(disk(package))
            out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')

        if mode == 'apply':
            assert not journal_path.exists(), 'Inspect the earlier apply before retrying'
            assert not disk(WIDGET).exists() and not ue.EditorAssetLibrary.does_asset_exist(WIDGET)
            assert not defaults(component).get_editor_property('IntroScreenClass'), 'Existing intro configuration is outside this first-apply scope'
            source_before = snapshot(media, directory, 'source_before')
            component_before = snapshot(component, directory, 'component_before')
            graphs_before = graph_snapshot(component, directory, 'component_graph_before')
            media.set_file_path(MOVIE.as_posix())
            assert Path(media.get_url().removeprefix('file://')).resolve() == MOVIE.resolve()
            assert not Path(media.get_editor_property('file_path')).is_absolute(), 'Source must use a project-relative movie path'
            assert media.validate()
            source_after = snapshot(media, directory, 'source_after')
            assert without_properties(source_before, ['FilePath']) == without_properties(source_after, ['FilePath'])
            save(media)

            umg = ue.get_default_object(ue.load_class(None, '/Script/UMGToolSet.UMGToolSet'))
            assert umg, 'Use -EnablePlugins=UMGToolSet'
            widget = umg.call_method('CreateWidgetBlueprint', args=(WIDGET.rsplit('/', 1)[0], WIDGET.rsplit('/', 1)[1], native))
            assert widget
            # Native RebuildWidget owns the full-screen tree; remove only this new BP's default root.
            for row in umg.call_method('GetWidgets', args=(widget,)).get_editor_property('widgets'):
                if not row.get_editor_property('parent'):
                    assert umg.call_method('RemoveWidget', args=(widget, row.get_editor_property('widget')))
                    break
            defaults(widget).set_editor_property('IntroMediaSource', media)
            assert umg.call_method('CompileWidgetBlueprint', args=(widget,))
            save(widget)
            defaults(component).set_editor_property('IntroScreenClass', widget.generated_class())
            assert ue.BlueprintEditorLibrary.compile_blueprint(component)
            component_after = snapshot(component, directory, 'component_after')
            assert without_properties(component_before, ['IntroScreenClass']) == without_properties(component_after, ['IntroScreenClass'])
            assert graphs_before == graph_snapshot(component, directory, 'component_graph_after')
            save(component)
            result['expected_properties'] = {
                SOURCE: source_after,
                WIDGET: snapshot(widget, directory, 'widget_after'),
                COMPONENT: component_after,
            }
        else:
            previous = json.loads(journal_path.read_text(encoding='utf-8'))
            assert previous['complete'], 'A completed apply is required'
            for package, expected in previous['saved_sha256'].items():
                assert sha(disk(package)) == expected, 'Asset changed after apply: ' + package
            widget = ue.load_asset(WIDGET)
            assert widget
            for package, asset in ((SOURCE, media), (WIDGET, widget), (COMPONENT, component)):
                assert snapshot(asset, directory, 'reload_' + package.rsplit('/', 1)[1] + '_' + str(time.time_ns())) == previous['expected_properties'][package]

        assert media.validate() and Path(media.get_url().removeprefix('file://')).resolve() == MOVIE.resolve()
        assert defaults(widget).get_editor_property('IntroMediaSource') == media
        assert defaults(component).get_editor_property('IntroScreenClass') == widget.generated_class()
        assert protected == inventory(), 'Unexpected asset/source/config/save-file mutation'
        assert all(sha(Path(p)) == digest for p, digest in protected_hashes.items())
        result.update(complete=True, protected_files_unchanged=True, movie_sha256=sha(MOVIE), runtime_verified=False)
    finally:
        out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        if mode == 'apply':
            journal_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print('DC_FRONTEND_INTRO_SETUP ' + json.dumps(result, ensure_ascii=False))


if __name__ == '__main__':
    main()
