"""Approved single-asset guard for the menu -> story re-entry null InventoryInstance.

Use -DCFrontendAmmoGuard=apply|verify -SCCProvider=None in a separate unattended commandlet.
Only W_AmmoCounter_Rifle may be saved. Keep all original nodes/data/other links.
"""
import hashlib
import json
import re
from pathlib import Path

import unreal as ue

P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/Frontend'
ASSET = '/Game/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle'
FILE = P / 'Content/DreamCatcher/Lyra/Weapons/Rifle/W_AmmoCounter_Rifle.uasset'
ROUTES = [('tick', 'K2Node_Event_2', 'K2Node_CallFunction_1', 'K2Node_VariableGet_8'),
          ('initialize', 'K2Node_Event_0', 'K2Node_CallFunction_7', 'K2Node_VariableGet_5')]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pin(node, name):
    values = [p for p in ue.BlueprintEditorLibrary.list_all_pins(node) if str(p.get_pin_name()) == name]
    assert len(values) == 1, (node.get_name(), name)
    return values[0]


def snapshot(bp):
    rows = []
    for graph in ue.BlueprintEditorLibrary.list_graphs(bp):
        nodes = []
        for node in ue.ObjectIterator(ue.EdGraphNode):
            if node.get_outer() != graph:
                continue
            data = dict(name=node.get_name(), cls=node.get_class().get_path_name())
            if 'K2Node' in node.get_class().get_name():
                data['title'] = ue.BlueprintEditorLibrary.get_node_title(node)
                data['pins'] = [dict(name=str(p.get_pin_name()), direction=str(p.get_pin_direction()),
                                     value=p.get_pin_value(), type=str(p.get_pin_type_display_string()),
                                     links=sorted((other.get_owning_node().get_name(), str(other.get_pin_name())) for other in p.list_connected_pins()))
                                for p in ue.BlueprintEditorLibrary.list_all_pins(node)]
            nodes.append(data)
        rows.append(dict(name=graph.get_name(), nodes=sorted(nodes, key=lambda n: n['name'])))
    return sorted(rows, key=lambda g: g['name'])


def cdo_export(bp, path):
    task = ue.AssetExportTask()
    task.object = ue.get_default_object(bp.generated_class())
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    task.replace_identical = False
    assert ue.Exporter.run_asset_export_task(task)
    return path.read_text(encoding='utf-8-sig')


def normalized(rows, added):
    value = json.loads(json.dumps(rows))
    mutable = {(event, 'then') for _, event, _, _ in ROUTES}
    mutable |= {(target, 'execute') for _, _, target, _ in ROUTES}
    mutable |= {(inventory, 'InventoryInstance') for _, _, _, inventory in ROUTES}
    for graph in value:
        if graph['name'] != 'EventGraph':
            continue
        graph['nodes'] = [n for n in graph['nodes'] if n['name'] not in added]
        for node in graph['nodes']:
            for p in node.get('pins', []):
                if (node['name'], p['name']) in mutable:
                    p['links'] = []
    return value


def check_routes(bp, names):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
    nodes = {n.get_name(): n for n in editor.list_all_nodes()}
    for route, event, target, inventory in ROUTES:
        valid, branch = names[route]['valid'], names[route]['branch']
        for start, start_pin, end, end_pin in [(event, 'then', branch, 'execute'),
                                             (branch, 'then', target, 'execute'),
                                             (valid, 'ReturnValue', branch, 'Condition'),
                                             (inventory, 'InventoryInstance', valid, 'Object')]:
            links = [(p.get_owning_node().get_name(), str(p.get_pin_name())) for p in pin(nodes[start], start_pin).list_connected_pins()]
            assert (end, end_pin) in links, (start, start_pin, end, end_pin)
            if start != inventory:
                assert len(links) == 1
        assert not pin(nodes[branch], 'else').list_connected_pins(), 'Invalid data must skip only this update'


def main():
    command = ue.SystemLibrary.get_command_line()
    assert '-unattended' in command.lower() and '-run=pythonscript' in command.lower()
    assert '-sccprovider=none' in command.lower(), 'Explicitly disable Editor source control'
    source_control = ue.get_default_object(ue.load_class(None, '/Script/SourceControl.SourceControlHelpers'))
    assert not source_control.call_method('IsEnabled'), 'Editor source control must be disabled'
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    mode = re.search(r'-DCFrontendAmmoGuard=(apply|verify)\b', command).group(1)
    out = R / ('ammo_guard_' + mode + '.json')
    assert not out.exists()
    protected_paths = [P / 'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
                       P / 'Content/DreamCatcher/Blueprints/Core/BP_DC_Level1GameMode.uasset',
                       P / 'Config/DefaultEngine.ini', P / 'Config/DefaultGame.ini',
                       P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    protected = {str(path): digest(path) for path in protected_paths}
    result = dict(mode=mode, complete=False, asset_saves=0, protected_sha256=protected)
    try:
        before_file = digest(FILE)
        if mode == 'apply':
            inspection = json.loads((R / 'AmmoTarget_A/report.json').read_text(encoding='utf-8'))
            assert before_file == inspection['sha256'][str(FILE)], 'Asset changed since the approved inspection'
        else:
            applied = json.loads((R / 'ammo_guard_apply.json').read_text(encoding='utf-8'))
            assert applied['complete'] and before_file == applied['asset_sha256']
        bp = ue.load_asset(ASSET)
        assert bp
        before = snapshot(bp)
        before_cdo = cdo_export(bp, R / ('ammo_guard_' + mode + '_before_cdo.t3d'))
        if mode == 'apply':
            editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
            assert editor
            nodes = {n.get_name(): n for n in editor.list_all_nodes()}
            names = {}
            for route, event_name, target_name, inventory_name in ROUTES:
                event, target, inventory = [nodes[n] for n in (event_name, target_name, inventory_name)]
                assert [(p.get_owning_node().get_name(), str(p.get_pin_name())) for p in pin(event, 'then').list_connected_pins()] == [(target_name, 'execute')]
                valid = editor.add_call_function_node('/Script/Engine.KismetSystemLibrary.IsValid')
                branch = editor.add_branch_node()
                assert valid and branch
                position = event.get_node_pos()
                valid.set_node_pos(ue.IntPoint(position.x + 30, position.y - 320))
                branch.set_node_pos(ue.IntPoint(position.x + 320, position.y - 130))
                assert pin(event, 'then').break_single_pin_link(pin(target, 'execute'))
                for a, b in [(pin(event, 'then'), pin(branch, 'execute')),
                             (pin(inventory, 'InventoryInstance'), pin(valid, 'Object')),
                             (pin(valid, 'ReturnValue'), pin(branch, 'Condition')),
                             (pin(branch, 'then'), pin(target, 'execute'))]:
                    assert a.try_create_connection(b)
                names[route] = dict(valid=valid.get_name(), branch=branch.get_name())
        else:
            names = applied['names']
        assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
        after = snapshot(bp)
        after_cdo = cdo_export(bp, R / ('ammo_guard_' + mode + '_after_cdo.t3d'))
        assert before_cdo == after_cdo, 'Widget defaults changed'
        if mode == 'apply':
            added = {name for group in names.values() for name in group.values()}
            assert normalized(before, set()) == normalized(after, added), 'Original graph/data changed outside six approved pins'
            result['before'] = before
        else:
            assert json.loads(json.dumps(after)) == applied['after'], 'Fresh reload graph differs'
        check_routes(bp, names)
        result.update(names=names, after=after, widget_defaults_unchanged=True)
        if mode == 'apply':
            assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
            result['asset_saves'] = 1
        else:
            assert digest(FILE) == before_file
        result['asset_sha256'] = digest(FILE)
        result['complete'] = True
    finally:
        result['protected_changes'] = [path for path, expected in protected.items() if digest(Path(path)) != expected]
        assert not result['protected_changes']
        out.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print('DC_FRONTEND_AMMO_GUARD_' + mode.upper() + '_COMPLETE')


if __name__ == '__main__':
    main()
