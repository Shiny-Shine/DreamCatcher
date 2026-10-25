"""After the user's build, move only the existing frontend BGM start path.

Separate unattended commandlet, -SCCProvider=None, -DCFrontendMusicMode=apply|verify
-DCFrontendMusicRun=<id>. Saves only L_DC_FrontEnd and BP_DC_FrontendStateComponent.
Keeps the original music node and cue/wave data; breaks its one BeginPlay link.
No build, PIE, config write, asset file copying, or Git mutation.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time

import unreal as ue

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
from base_damage_setup import assert_no_other_editor, export, without_properties
from frontend_ammo_guard import snapshot, pin

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/FrontendMusic20261025'
MAP = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'
COMPONENT = '/Game/DreamCatcher/Blueprints/UI/BP_DC_FrontendStateComponent'
CUE = '/Game/DreamCatcher/Audio/선택화면_Cue'
TARGETS = (MAP, COMPONENT)
EVENT = 'K2Node_Event_0'
PLAY = 'K2Node_CallFunction_0'


def disk(package):
    assert package in TARGETS
    return PROJECT / ('Content/' + package.removeprefix('/Game/') + ('.umap' if package == MAP else '.uasset'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def staged_entries():
    env = dict(os.environ, GIT_OPTIONAL_LOCKS='0')
    value = subprocess.run(['git', 'ls-files', '--stage', '-z'], cwd=PROJECT, env=env,
                           capture_output=True, check=True).stdout
    return hashlib.sha256(value).hexdigest()


def inventory():
    allowed = {disk(p) for p in TARGETS}
    result = {}
    for folder in ('Content', 'Source', 'Config', 'Plugins', 'Saved/SaveGames'):
        for path in (PROJECT / folder).rglob('*'):
            if path.is_file() and path not in allowed:
                s = path.stat()
                result[str(path)] = [s.st_size, s.st_mtime_ns]
    return result


def scene_snapshot(world, directory, label):
    prefix = MAP + '.L_DC_FrontEnd:PersistentLevel'
    actors = {}
    for actor in ue.ObjectIterator(ue.Actor):
        if actor.get_outer().get_path_name() != prefix:
            continue
        if actor.get_class().get_name().startswith(('REINST_', 'SKEL_')):
            continue
        cls = actor.get_class().get_path_name()
        # Compile replaces this one engine-generated actor (_C_0 -> _C_1).
        # Preserve its class/transform and every other actor's exact identity.
        key = '__LevelScriptActor__' if cls == MAP + '.L_DC_FrontEnd_C' else actor.get_name()
        assert key not in actors, 'Duplicate level-script actor or actor identity'
        actors[key] = dict(cls=cls, transform=actor.get_actor_transform().export_text())
    settings_actors = [a for a in ue.ObjectIterator(ue.WorldSettings) if a.get_outer().get_path_name() == prefix]
    assert len(settings_actors) == 1
    settings = export(settings_actors[0], directory / (label + '_settings.t3d'))
    return dict(actors=actors, settings=settings)


def normalize_link(rows):
    rows = json.loads(json.dumps(rows))
    for graph in rows:
        for node in graph['nodes']:
            for item in node.get('pins', []):
                if (node['name'], item['name']) in {(EVENT, 'then'), (PLAY, 'execute')}:
                    item['links'] = []
    return rows


def graph_nodes(bp):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
    assert editor
    return {n.get_name(): n for n in editor.list_all_nodes()}


def validate_music_node(nodes, directory, label):
    assert EVENT in nodes and PLAY in nodes
    # Serialized member names are independent of the user's Editor language.
    text = export(nodes[PLAY], directory / (label + '_play.t3d'))
    event = export(nodes[EVENT], directory / (label + '_event.t3d'))
    assert 'MemberName="PlaySound2D"' in text and 'MemberName="ReceiveBeginPlay"' in event
    assert any('PinName="Sound"' in line and CUE in line for line in text.splitlines())
    for name, expected in [('VolumeMultiplier', 1.0), ('PitchMultiplier', 1.0), ('StartTime', 0.0)]:
        assert float(pin(nodes[PLAY], name).get_pin_value()) == expected, name


def main():
    command = ue.SystemLibrary.get_command_line()
    mode_match = re.search(r'-DCFrontendMusicMode=(apply|verify)\b', command)
    run_match = re.search(r'-DCFrontendMusicRun=([A-Za-z0-9_]+)\b', command)
    assert mode_match and run_match
    mode = mode_match.group(1)
    assert '-unattended' in command.lower() and '-run=pythonscript' in command.lower()
    assert '-sccprovider=none' in command.lower() and not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert_no_other_editor()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    native = ue.load_class(None, '/Script/DreamCatcher.DCLyraFrontendStateComponent')
    assert native
    ue.get_default_object(native).get_editor_property('MenuMusic')  # requires the user's new build
    directory = REPORTS / ('assets_' + run_match.group(1))
    directory.mkdir(parents=True, exist_ok=True)
    journal = directory / 'apply.json'
    stamp = str(time.time_ns())
    out = directory / (mode + '_' + stamp + '.json')
    result = dict(mode=mode, complete=False, asset_saves=[], cpp_build=False, pie=False)
    protected = inventory()
    staged_before = staged_entries()
    protected_hashes = {str(PROJECT / p): sha(PROJECT / p) for p in (
        '.git/HEAD', 'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Config/DefaultEngine.ini', 'Config/DefaultGame.ini',
        'Source/DreamCatcher/UI/Frontend/DCFrontendIntroScreen.cpp',
        'Content/DreamCatcher/Audio/선택화면_Cue.uasset',
        'Content/DreamCatcher/Audio/선택화면.uasset')}
    try:
        settings_cls = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings = export(ue.get_default_object(settings_cls), directory / ('settings_' + stamp + '.t3d'))
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never']
        # Initialize the editor world before the snapshot. Merely loading its
        # package leaves the LevelScriptActor uninstantiated until BP compilation.
        world = ue.EditorLoadingAndSavingUtils.load_map(MAP)
        component, cue = [ue.load_asset(p) for p in (COMPONENT, CUE)]
        assert world and component and cue
        blueprints = [bp for bp in ue.ObjectIterator(ue.LevelScriptBlueprint) if bp.get_path_name().startswith(MAP + '.')]
        assert len(blueprints) == 1
        bp = blueprints[0]
        cdo = ue.get_default_object(component.generated_class())
        before_graph = snapshot(bp)
        before_scene = scene_snapshot(world, directory, 'before_' + stamp)
        nodes = graph_nodes(bp)
        validate_music_node(nodes, directory, 'before_' + stamp)

        def save(asset, package):
            assert package in TARGETS
            assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
            result['asset_saves'].append(package)
            result.setdefault('saved_sha256', {})[package] = sha(disk(package))
            out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')

        if mode == 'apply':
            assert not journal.exists(), 'Inspect any earlier apply before retrying'
            assert not cdo.get_editor_property('MenuMusic'), 'Existing music binding requires review'
            expected = [(PLAY, 'execute')]
            actual = [(p.get_owning_node().get_name(), str(p.get_pin_name())) for p in pin(nodes[EVENT], 'then').list_connected_pins()]
            assert actual == expected, 'BeginPlay wiring changed; preserve current graph and inspect'
            assert pin(nodes[EVENT], 'then').break_single_pin_link(pin(nodes[PLAY], 'execute'))
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            after_graph = snapshot(bp)
            assert normalize_link(before_graph) == normalize_link(after_graph), 'Unexpected graph/data change'
            after_scene = scene_snapshot(world, directory, 'after_' + stamp)
            result.update(map_scene_before=before_scene, map_scene_after=after_scene)
            out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
            assert before_scene == after_scene, 'Unexpected map actor/settings change'
            save(world, MAP)

            component_before = export(cdo, directory / ('component_before_' + stamp + '.t3d'))
            component_graph_before = snapshot(component)
            cdo.set_editor_property('MenuMusic', cue)
            assert ue.BlueprintEditorLibrary.compile_blueprint(component)
            component_after = export(ue.get_default_object(component.generated_class()), directory / ('component_after_' + stamp + '.t3d'))
            assert without_properties(component_before, ['MenuMusic']) == without_properties(component_after, ['MenuMusic'])
            assert component_graph_before == snapshot(component)
            save(component, COMPONENT)
            result.update(map_graph=after_graph, map_scene=before_scene, component_cdo=component_after)
        else:
            previous = json.loads(journal.read_text(encoding='utf-8'))
            assert previous['complete']
            for package, expected in previous['saved_sha256'].items():
                assert sha(disk(package)) == expected, 'Changed after apply: ' + package
            assert before_graph == previous['map_graph'] and before_scene == previous['map_scene']
            assert export(cdo, directory / ('component_reload_' + stamp + '.t3d')) == previous['component_cdo']

        nodes = graph_nodes(bp)
        assert not pin(nodes[EVENT], 'then').list_connected_pins()
        assert not pin(nodes[PLAY], 'execute').list_connected_pins()
        validate_music_node(nodes, directory, 'checked_' + stamp)
        assert ue.get_default_object(component.generated_class()).get_editor_property('MenuMusic') == cue
        assert protected == inventory() and staged_before == staged_entries()
        assert all(sha(Path(p)) == h for p, h in protected_hashes.items())
        result.update(complete=True, protected_files_unchanged=True, staged_entries_unchanged=True,
                      cue_gain=cue.get_editor_property('VolumeMultiplier'), runtime_verified=False)
    finally:
        out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
        if mode == 'apply':
            journal.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print('DC_FRONTEND_MUSIC_SETUP_COMPLETE ' + str(out))


if __name__ == '__main__':
    main()
