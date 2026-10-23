"""Approved Inspector-only presentation edit; no PIE, C++ build or broad save.

Outside Unreal: --commands PREFIX prints fixed apply / fresh verify commands.
Inside a separate unattended Editor: main() edits exactly three existing private
assets plus one new weapon copy. All memory checks precede the three saves.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import traceback
from collections import Counter

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
ROOT = P / 'Saved/Diagnostics/InspectorBotPresentation20261023'
BASELINE = ROOT / 'baseline.json'
BASELINE_SHA = '109c30a277219add37406c1de627a60b5fac2fb6004356074be405db4dd21a06'
SCRIPT = P / 'Scripts/Editor/inspector_bot_presentation_setup.py'
PRIVATE = '/Game/DreamCatcher/AI/Inspector'
WEAPON = '/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle'
HIDDEN = PRIVATE + '/Assets/B_Rifle_NoVisual'
WID = PRIVATE + '/Assets/WID_Rifle'
PAWN = PRIVATE + '/Assets/DA_DC_LyraAIPawn'
MAIN = PRIVATE + '/Animation/ABP_Mannequin_Base'
DASH = PRIVATE + '/Assets/AS_DC_RifleDash'
VISUALS = {'SMGVisual', 'SMGMagazineVisual'}
E_APPLY = 'Presentation_E_apply'
E_REPORT_SHA = '4f02ca558efad00e0f506d0d2e4a1ee01ffd4217c4c3e1417eb0f5dcb1b8ff64'
E_SCRIPT_SHA = '03ea08b9a6c9920f1c21f6f25a2f8931770f01b2e16b186f92d05128a62c67a4'
E_SAVED_FILES = {
    'Content/DreamCatcher/AI/Inspector/Assets/B_Rifle_NoVisual.uasset': 'ffeb1f734bbf803dd37bea8753f5238796b554e6d495f0724570091fe64a6c49',
    'Content/DreamCatcher/AI/Inspector/Assets/WID_Rifle.uasset': '5675d0beb3ee98450c3eb3bd5f3641422d8fef2b345073254501017234254aa2',
    'Content/DreamCatcher/AI/Inspector/Assets/DA_DC_LyraAIPawn.uasset': '7c8483aabddc759c156c17e9f6fc99994e5bfdf7a23a497f7174ae6d616fa884',
    'Content/DreamCatcher/AI/Inspector/Animation/ABP_Mannequin_Base.uasset': '7fa596c263df1eacf8b2493f0385601567db1ee6e87fdaa2066d2c6861714e2e',
}


def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1048576), b''):
            value.update(block)
    return value.hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def persist(path, value):
    assert not path.exists(), 'Preserve existing evidence: ' + str(path)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def disk(package):
    assert package in (HIDDEN, WID, PAWN, MAIN)
    return P / ('Content/' + package[6:] + '.uasset')


def inventory():
    result = {}
    for top in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for path in (P / top).rglob('*'):
            if path.is_file() and path.resolve() != SCRIPT.resolve():
                stat = path.stat()
                result[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    return result


def protect(base):
    assert sha(BASELINE) == BASELINE_SHA
    assert base['project'] == P.as_posix() and base['approved_script'] == SCRIPT.relative_to(P).as_posix()
    assert base['asset_write_scope'] == {HIDDEN: 'new', WID: 'modify', PAWN: 'modify', MAIN: 'modify'}
    assert base['automatic_PIE'] is False and base['cpp_build'] is False
    assert len(base['protected_files']) == 1507 and len(base['inventory']) == 6365
    for name, expected in base['protected_files'].items():
        assert (P / name).is_file() and sha(P / name) == expected, 'Protected file changed: ' + name
    assert all(sha(P / name) == expected for name, expected in base['cpp8'].items())
    assert sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll') == base['DLL_sha256']
    allowed = set(base['asset_files'])
    current = inventory()
    assert {k: v for k, v in current.items() if k not in allowed} == {
        k: v for k, v in base['inventory'].items() if k not in allowed}, 'Out-of-scope inventory change'
    return {'protected_count': len(base['protected_files']), 'inventory_count': len(base['inventory']),
        'baseline_sha256': BASELINE_SHA, 'originals_unchanged': True}


def dirty():
    names = {obj.get_name() for obj in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert names <= {HIDDEN, WID, PAWN, MAIN}, 'Unexpected dirty original: ' + str(sorted(names))
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Map saving forbidden'
    return sorted(names)


def log_check(path):
    ue.log_flush()
    raw = path.read_bytes()
    errors = [line for line in raw.decode('utf-8-sig', errors='replace').splitlines()
        if re.search(r'\bLog\w+:\s*(?:Error|Fatal):|Ensure condition failed:|=== Handled ensure:|Fatal error:|Assertion failed:', line, re.I)]
    assert not errors, 'Engine Error/ensure/Fatal: ' + '\n'.join(errors[:10])
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}


def load(package):
    obj = ue.load_asset(package)
    assert obj and obj.get_path_name() == package + '.' + package.rsplit('/', 1)[-1], package
    return obj


def export(obj, path):
    assert not path.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter, task.filename = obj, ue.ObjectExporterT3D(), path.as_posix()
    task.automated, task.prompt = True, False
    assert ue.Exporter.run_asset_export_task(task)
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n')


def root_lines(text):
    depth, result = 0, []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object'):
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
        elif depth == 1:
            result.append(stripped)
        assert depth >= 0
    assert depth == 0
    return result


def without_fields(text, fields):
    return '\n'.join(line for line in text.splitlines()
        if not any(re.match(r'^\s*' + re.escape(field) + r'(?:\(\d+\))?=', line) for field in fields))


def copy_names(text):
    return text.replace(WEAPON + '.B_Rifle', HIDDEN + '.B_Rifle_NoVisual').replace(
        'Default__B_Rifle_C', 'Default__B_Rifle_NoVisual_C').replace('Name="B_Rifle"', 'Name="B_Rifle_NoVisual"')


def component_rows(bp, private_copy):
    # Use the same canonical SubobjectData query for source and copy. Source
    # objects are read only; any outside-scope dirty package still aborts.
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    lib = ue.SubobjectDataBlueprintFunctionLibrary
    owned_before = {obj.get_path_name() for obj in ue.ObjectIterator(ue.SceneComponent) if obj.get_outer() == bp.generated_class()}
    rows = []
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = lib.get_data(handle)
        obj = lib.get_object_for_blueprint(data, bp)
        if isinstance(obj, ue.SceneComponent) and obj.get_outer() == bp.generated_class() and obj.get_name() != 'DefaultSceneRoot_GEN_VARIABLE':
            rows.append((str(lib.get_variable_name(data)), obj))
    if not private_copy:
        assert owned_before == {obj.get_path_name() for obj in ue.ObjectIterator(ue.SceneComponent) if obj.get_outer() == bp.generated_class()}, 'Read-only source query created an override'
    dirty()
    assert len({name for name, obj in rows}) == len(rows)
    result = dict(rows)
    assert VISUALS <= set(result)
    for name in VISUALS:
        obj = result[name]
        assert isinstance(obj, ue.StaticMeshComponent) and obj.get_outer() == bp.generated_class(), 'Shared visual override refused'
    drivers = [obj for obj in result.values() if isinstance(obj, ue.SkeletalMeshComponent)]
    assert len(drivers) == 1
    assert drivers[0].get_editor_property('visibility_based_anim_tick_option') == ue.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES
    return result


def weapon_state(bp, directory, label, private_copy):
    rows = component_rows(bp, private_copy)
    components, unlisted = {}, []
    listed = {obj.get_path_name() for obj in rows.values()}
    extras = [obj for obj in ue.ObjectIterator(ue.SceneComponent) if obj.get_outer() == bp.generated_class()
        and obj.get_name().endswith('_GEN_VARIABLE') and obj.get_path_name() not in listed]
    assert len(extras) <= 1, 'Unexpected unlisted own component templates'
    for obj in extras:
        text = export(obj, directory / (label + '_unlisted_root.t3d'))
        archetype = 'Archetype="/Script/Engine.SceneComponent\'/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Weapon.B_Weapon_C:DefaultSceneRoot_GEN_VARIABLE\'"'
        assert obj.get_name() == 'DefaultSceneRoot_GEN_VARIABLE' and obj.get_class().get_path_name() == '/Script/Engine.SceneComponent'
        assert archetype in text.splitlines()[0] and not root_lines(text), 'Unlisted root has an override; refuse omission'
        unlisted.append({'name': obj.get_name(), 'class': obj.get_class().get_path_name(), 'raw': text,
            'serialized_override_fields': [], 'reason': 'Empty inherited parent DefaultSceneRoot placeholder, absent from canonical SubobjectData'})
    for index, (name, obj) in enumerate(sorted(rows.items())):
        text = export(obj, directory / (label + '_component_' + str(index) + '.t3d'))
        components[name] = {'class': obj.get_class().get_path_name(), 'raw': text,
            'visible': bool(obj.get_editor_property('visible')), 'hidden': bool(obj.get_editor_property('hidden_in_game'))}
    graphs = {graph.get_name(): export(graph, directory / (label + '_graph_' + str(index) + '.t3d'))
        for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp))}
    parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name()
    assert parent == '/Game/DreamCatcher/Lyra/Weapons/Rifle/B_Weapon.B_Weapon_C'
    parent_root_path = parent + ':DefaultSceneRoot_GEN_VARIABLE'
    parent_root = ue.find_object(None, parent_root_path)
    assert parent_root and parent_root.get_class().get_path_name() == '/Script/Engine.SceneComponent'
    parent_root_raw = export(parent_root, directory / (label + '_inherited_parent_root.t3d'))
    return {'parent': parent, 'components': components, 'graphs': graphs, 'unlisted_template_observations': unlisted,
        'effective_default_root': {'parent_template': parent_root_path, 'parent_raw': parent_root_raw,
            'own_empty_placeholder': bool(unlisted)}}


def check_weapon(source, target):
    assert source['parent'] == target['parent'] and set(source['components']) == set(target['components'])
    assert source['effective_default_root']['parent_template'] == target['effective_default_root']['parent_template']
    assert source['effective_default_root']['parent_raw'] == target['effective_default_root']['parent_raw']
    assert set(source['graphs']) == set(target['graphs'])
    identities = {}
    for name, text in source['graphs'].items():
        before, after = copy_names(text), target['graphs'][name]
        if name == 'UserConstructionScript':
            normalized, guids = [], []
            for value in (before, after):
                nodes = [line for line in root_lines(value) if line.startswith('Nodes(')]
                assert len(nodes) == 1 and re.fullmatch(r"Nodes\(0\)=\"/Script/BlueprintGraph.K2Node_FunctionEntry'K2Node_FunctionEntry_0'\"", nodes[0])
                found = re.findall(r'^[ \t]*NodeGuid=([A-Fa-f0-9]{32})$', value, re.M)
                assert len(found) == 1 and int(found[0], 16) != 0, 'Expected exactly one nonzero copied constructor entry identity'
                guids.append(found[0])
                normalized.append(re.sub(r'^([ \t]*NodeGuid=)' + re.escape(found[0]) + r'$',
                    r'\1<new-copy-constructor-entry-identity>', value, count=1, flags=re.M))
            assert normalized[0] == normalized[1], 'Copied constructor changed beyond its single entry NodeGuid'
            identities['UserConstructionScript.K2Node_FunctionEntry_0.NodeGuid'] = {'source': guids[0], 'copy': guids[1]}
        else:
            assert before == after, name
    for name, before in source['components'].items():
        after = target['components'][name]
        assert before['class'] == after['class']
        fields = {'bVisible', 'bHiddenInGame'} if name in VISUALS else set()
        assert copy_names(without_fields(before['raw'], fields)) == without_fields(after['raw'], fields), name
        if name in VISUALS:
            assert after['visible'] is False and after['hidden'] is True
        else:
            assert before['visible'] == after['visible'] and before['hidden'] == after['hidden']
    assert len(identities) == 1
    return identities


def pin_lines(text):
    return [line for line in root_lines(text) if line.startswith('CustomProperties Pin ')]


def sparse_tolerance_pin_identity(text):
    """Compare only the generated ID of one inert promotable-operator pin."""
    stack, output, identities = [], [], {}
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            found = re.search(r'ExportPath="(/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+)\'([^\']+)\'"', stripped)
            assert found, 'Expected typed object context in node export'
            stack.append((found[1], found[2]))
        elif stripped == 'End Object':
            assert stack
            stack.pop()
        elif stripped.startswith('CustomProperties Pin ') and stack and stack[-1][0] == '/Script/BlueprintGraph.K2Node_PromotableOperator':
            def field(name, value):
                return re.search(r'(?<=[(,])' + re.escape(name) + '=' + re.escape(value) + r'(?=[,)])', stripped) is not None
            sparse = all(field(name, value) for name, value in {
                'PinName': '"ErrorTolerance"', 'PinType.PinCategory': '""',
                'PinType.PinSubCategory': '""', 'PinType.PinSubCategoryObject': 'None',
                'bHidden': 'True', 'PersistentGuid': '0' * 32}.items())
            sparse = sparse and not re.search(r'(?<=[(,])(?:LinkedTo|DefaultValue|AutogeneratedDefaultValue|ParentPin|SubPins)=', stripped)
            if sparse:
                owner = stack[-1][1]
                assert owner.startswith(MAIN + '.')
                found = re.findall(r'(?<=[(,])PinId=([A-Fa-f0-9]{32})(?=[,)])', stripped)
                assert len(found) == 1 and int(found[0], 16) != 0 and owner not in identities
                identities[owner] = {'pin': 'ErrorTolerance', 'PinId': found[0],
                    'class': stack[-1][0], 'raw': line}
                line = re.sub(r'(?<=[(,])PinId=' + re.escape(found[0]) + r'(?=[,)])',
                    'PinId=<inert-tolerance-editor-identity>', line, count=1)
        output.append(line)
    assert not stack
    return '\n'.join(output), identities


def hit_node(bp, directory, label):
    candidates, records = [], {}
    registered, discarded, graph_nodes = {}, {}, {}
    pinlib = ue.BlueprintGraphPinLibrary
    for graph_index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp)):
        assert graph.get_path_name().startswith(MAIN + '.')
        graph_text = export(graph, directory / (label + '_registered_graph_' + str(graph_index) + '.t3d'))
        entries, explicit_empty = [], False
        for line in root_lines(graph_text):
            if not line.startswith('Nodes'):
                continue
            if line == 'Nodes=()':
                assert not entries and not explicit_empty
                explicit_empty = True
                continue
            match = re.fullmatch(r'Nodes\((\d+)\)="(/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+)\'([A-Za-z0-9_]+)\'"', line)
            assert not explicit_empty and match and int(match[1]) == len(entries), 'Canonical graph Nodes export is incomplete or unsupported'
            entries.append((match[3], match[2]))
        assert len({name for name, cls in entries}) == len(entries), 'Duplicate canonical node name'
        outer_objects = [node for node in ue.ObjectIterator(ue.EdGraphNode) if node.get_outer() == graph]
        by_name = {node.get_name(): node for node in outer_objects}
        assert len(by_name) == len(outer_objects)
        nodes = []
        for name, cls in entries:
            assert name in by_name and by_name[name].get_outer() == graph and by_name[name].get_class().get_path_name() == cls
            nodes.append(by_name[name])
        registered[graph.get_path_name()] = [[name, cls] for name, cls in entries]
        graph_nodes[graph.get_path_name()] = nodes
        discarded[graph.get_path_name()] = []
        for extra_index, extra in enumerate(sorted((obj for obj in outer_objects if obj.get_name() not in {name for name, cls in entries}), key=lambda obj: obj.get_name())):
            extra_text = export(extra, directory / (label + '_unregistered_' + str(graph_index) + '_' + str(extra_index) + '.t3d'))
            extra_pins = {}
            if isinstance(extra, ue.K2Node):
                for pin in ue.BlueprintEditorLibrary.list_all_pins(extra):
                    extra_pins[str(pinlib.get_pin_name(pin))] = sorted([pinlib.get_owning_node(other).get_path_name(), str(pinlib.get_pin_name(other))]
                        for other in pinlib.list_connected_pins(pin))
            discarded[graph.get_path_name()].append({'name': extra.get_name(), 'class': extra.get_class().get_path_name(),
                'serialized_node_guid': next((line.split('=', 1)[1] for line in root_lines(extra_text) if line.startswith('NodeGuid=')), None),
                'pins': extra_pins, 'reason': 'Absent from this graph canonical serialized Nodes array'})
        for node_index, node in enumerate(sorted(nodes, key=lambda obj: obj.get_name())):
            text = export(node, directory / (label + '_node_' + str(graph_index) + '_' + str(node_index) + '.t3d'))
            pins = {}
            if isinstance(node, ue.K2Node):
                for pin in ue.BlueprintEditorLibrary.list_all_pins(node):
                    name = str(pinlib.get_pin_name(pin))
                    assert name not in pins
                    pins[name] = sorted([pinlib.get_owning_node(other).get_path_name(), str(pinlib.get_pin_name(other))]
                        for other in pinlib.list_connected_pins(pin))
            records[node.get_path_name()] = {'class': node.get_class().get_path_name(), 'text': text, 'pins': pins}
            if node.get_class().get_path_name() == '/Script/AnimGraph.AnimGraphNode_Slot':
                runtime = node.get_editor_property('node')
                if str(runtime.get_editor_property('slot_name')) == 'AdditiveHitReact':
                    candidates.append((graph, node, text))
    assert len(candidates) == 1, 'Expected one authored AdditiveHitReact slot'
    graph, slot, slot_text = candidates[0]
    assert slot.get_name() == 'AnimGraphNode_Slot_1'
    source_pin = ue.BlueprintEditorLibrary.find_input_pin(slot, 'Source')
    slot_pose = ue.BlueprintEditorLibrary.find_output_pin(slot, 'Pose')
    assert pinlib.is_valid(source_pin) and pinlib.is_valid(slot_pose)
    upstream = list(pinlib.list_connected_pins(source_pin))
    assert len(upstream) == 1
    upstream_pin = upstream[0]
    upstream_node = pinlib.get_owning_node(upstream_pin)
    assert upstream_node.get_outer() == graph and upstream_node.get_name() == 'AnimGraphNode_LinkedAnimLayer_0'
    assert str(pinlib.get_pin_name(upstream_pin)) == 'Pose'
    matches = [node for node in graph_nodes[graph.get_path_name()] if node.get_outer() == graph
        and node.get_name() == 'AnimGraphNode_ApplyAdditive_0'
        and node.get_class().get_path_name() == '/Script/AnimGraph.AnimGraphNode_ApplyAdditive']
    assert len(matches) == 1
    consumer = matches[0]
    base_pin = ue.BlueprintEditorLibrary.find_input_pin(consumer, 'Base')
    assert pinlib.is_valid(base_pin)
    runtime = consumer.get_editor_property('node')
    alpha_input = ue.BlueprintEditorLibrary.find_input_pin(consumer, 'Alpha')
    assert pinlib.is_valid(alpha_input) and not pinlib.list_connected_pins(alpha_input)
    assert abs(float(pinlib.get_pin_value(alpha_input)) - 0.65) < 1.e-6
    alpha_pin = [line for line in pin_lines(records[consumer.get_path_name()]['text']) if 'PinName="Alpha"' in line]
    assert len(alpha_pin) == 1 and 'LinkedTo=' not in alpha_pin[0]
    return (slot_pose, base_pin, upstream_pin), {'target': consumer.get_path_name(), 'slot': slot.get_path_name(),
        'upstream': upstream_node.get_path_name(), 'records': records, 'registered_nodes': registered,
        'discarded_outer_objects': discarded,
        'parent': ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
        'target_skeleton': bp.get_editor_property('TargetSkeleton').get_path_name(),
        'runtime': runtime.export_text()}


def check_hit_graph(before, after):
    assert before['target'] == after['target'] and before['slot'] == after['slot'] and before['upstream'] == after['upstream']
    assert before['parent'] == after['parent'] and before['target_skeleton'] == after['target_skeleton']
    assert before['registered_nodes'] == after['registered_nodes'], 'Canonical graph node lists or order changed'
    assert set(before['records']) == set(after['records'])
    assert before['runtime'] == after['runtime'], 'Consumer tuning including Alpha and ScaleBias must stay unchanged'
    slot, target, upstream = before['slot'], before['target'], before['upstream']
    allowed = {(slot, 'Pose'), (target, 'Base'), (upstream, 'Pose')}
    assert before['records'][slot]['pins']['Pose'] == [[target, 'Base']]
    assert before['records'][target]['pins']['Base'] == [[slot, 'Pose']]
    assert after['records'][slot]['pins']['Pose'] == []
    assert after['records'][target]['pins']['Base'] == [[upstream, 'Pose']]
    assert [target, 'Base'] not in before['records'][upstream]['pins']['Pose']
    assert after['records'][upstream]['pins']['Pose'] == sorted(before['records'][upstream]['pins']['Pose'] + [[target, 'Base']])
    changed, identity_evidence = set(), {}
    for path, row in before['records'].items():
        other = after['records'][path]
        assert row['class'] == other['class'] and set(row['pins']) == set(other['pins']), path
        for name in row['pins']:
            if row['pins'][name] != other['pins'][name]:
                changed.add((path, name))
                assert (path, name) in allowed
        old_text, old_ids = sparse_tolerance_pin_identity(row['text'])
        new_text, new_ids = sparse_tolerance_pin_identity(other['text'])
        assert set(old_ids) == set(new_ids), 'Sparse pin count or owner locations changed'
        if old_ids:
            identity_evidence[path] = {'count': len(old_ids), 'before': old_ids, 'after': new_ids}
        def tuning(text):
            output = []
            for line in text.splitlines():
                if line.strip().startswith(('ErrorType=', 'ErrorMsg=', 'bHasCompilerMessage=')):
                    continue  # Compiler diagnostics retained in the unmodified raw evidence.
                match = re.search(r'CustomProperties Pin .*?PinName="([^"]+)"', line)
                if match and (path, match[1]) in allowed:
                    line = re.sub(r'LinkedTo=\([^)]*\),?', '', line)
                output.append(line)
            return '\n'.join(output)
        assert tuning(old_text) == tuning(new_text), path
    assert changed == allowed, 'Exactly three reciprocal pin lists must change'
    return identity_evidence


def runtime_hit(bp, directory, label):
    cdo = ue.get_default_object(bp.generated_class())
    text = export(cdo, directory / (label + '_generated_cdo.t3d'))
    values = dict(line.split('=', 1) for line in root_lines(text) if '=' in line)
    assert any(name.startswith('AnimGraphNode_ApplyAdditive') and 'Alpha=0.650000' in value for name, value in values.items())
    return {'properties': values}


def check_runtime_hit(before, after):
    def values(snapshot):
        nodes, ordinary, hit_slots = [], {}, []
        for name, value in snapshot['properties'].items():
            if name.startswith('AnimGraphNode_'):
                normalized = re.sub(r'(?<![A-Za-z0-9_])(SourceLinkID|LinkID)=-?\d+', r'\1=<compiler-index>', value)
                kind = re.sub(r'_\d+$', '', name)
                if kind == 'AnimGraphNode_Slot' and 'SlotName="AdditiveHitReact"' in value:
                    hit_slots.append(normalized)
                else:
                    nodes.append((kind, normalized))
            else:
                ordinary[name] = value
        return Counter(nodes), ordinary, hit_slots
    old_nodes, old_ordinary, old_slots = values(before)
    new_nodes, new_ordinary, new_slots = values(after)
    assert len(old_slots) == 1 and len(new_slots) <= 1
    assert not new_slots or new_slots == old_slots
    assert old_nodes == new_nodes and old_ordinary == new_ordinary, 'Runtime tuning changed beyond dead hit slot / compiler pose indices'


def wid_state(bp, directory, label):
    cdo = ue.get_default_object(bp.generated_class())
    return cdo, {'raw': export(cdo, directory / (label + '_wid_cdo.t3d')),
        'spawn': [item.export_text() for item in cdo.get_editor_property('ActorsToSpawn')],
        'abilities': [obj.get_path_name() for obj in cdo.get_editor_property('AbilitySetsToGrant')],
        'instance': cdo.get_editor_property('InstanceType').get_path_name()}


def pawn_state(obj, directory, label):
    return {'raw': export(obj, directory / (label + '_pawn_data.t3d')),
        'sets': [value.get_path_name() for value in obj.get_editor_property('AbilitySets')],
        'pawn_class': obj.get_editor_property('PawnClass').get_path_name()}


def compile_bp(bp, log_path=None, allow_unused_hit=False):
    ue.log_flush()
    offset = log_path.stat().st_size if log_path else 0
    result = ue.BlueprintEditorLibrary.compile_blueprint(bp)
    status = bp.get_editor_property('Status')
    accepted = {ue.BlueprintStatus.BS_UP_TO_DATE}
    if allow_unused_hit:
        assert bp.get_path_name() == MAIN + '.ABP_Mannequin_Base' and log_path is not None
        accepted.add(ue.BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS)
    assert result is True and type(result) is bool and status in accepted, bp.get_path_name()
    ue.log_flush()
    warnings = [] if log_path is None else [line for line in log_path.read_bytes()[offset:].decode('utf-8-sig', errors='replace').splitlines() if ': Warning:' in line]
    if warnings:
        assert allow_unused_hit
        assert all(('additivehitreact' in line.lower() or 'animgraphnode_slot_1' in line.lower())
            and any(word in line.lower() for word in ('unused', 'not used', 'not connected', 'ignored', 'not linked')) for line in warnings), 'Unexpected compile warning'
    dirty()
    return {'asset': bp.get_path_name(), 'status': str(status), 'new_warning_lines': warnings}


def check_changes(before, after):
    actor_old = WEAPON + '.B_Rifle_C'
    actor_new = HIDDEN + '.B_Rifle_NoVisual_C'
    assert len(before['wid']['spawn']) == len(after['wid']['spawn']) == 1
    assert before['wid']['spawn'][0].count(actor_old) == 1
    assert before['wid']['spawn'][0].replace(actor_old, actor_new) == after['wid']['spawn'][0]
    assert before['wid']['abilities'] == after['wid']['abilities'] and before['wid']['instance'] == after['wid']['instance']
    assert without_fields(before['wid']['raw'], {'ActorsToSpawn'}) == without_fields(after['wid']['raw'], {'ActorsToSpawn'})
    dash = DASH + '.AS_DC_RifleDash'
    assert before['pawn']['sets'].count(dash) == 1
    assert [name for name in before['pawn']['sets'] if name != dash] == after['pawn']['sets'] and after['pawn']['sets']
    assert before['pawn']['pawn_class'] == after['pawn']['pawn_class']
    assert without_fields(before['pawn']['raw'], {'AbilitySets'}) == without_fields(after['pawn']['raw'], {'AbilitySets'})
    pin_identity = check_hit_graph(before['hit'], after['hit'])
    check_runtime_hit(before['compiled_hit'], after['compiled_hit'])
    return check_weapon(before['weapon'], after['weapon']), pin_identity


def catalog(base, new_exists):
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    names = {str(row.package_name) for row in registry.get_assets_by_path(PRIVATE, recursive=True, include_only_on_disk_assets=False)}
    assert names == set(base['original_private_packages']) | ({HIDDEN} if new_exists else set())


def main():
    global ue
    import unreal as ue
    report = {'complete': False, 'protection_passed': False, 'asset_saves': 0, 'saved_packages': [],
        'PIE': False, 'cpp_build': False, 'physical_play_verified': False}
    directory, base = None, None
    try:
        command = ue.SystemLibrary.get_command_line()
        def argument(name, pattern):
            found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
            assert len(found) == 1, name
            return found[0]
        mode = argument('DCInspectorPresentationMode', 'apply|verify')
        assert mode == 'verify', 'E assets are already saved; this revision permits read-only cold verification only'
        run = argument('DCInspectorPresentationRun', '[A-Za-z0-9_]{1,32}')
        frozen = argument('DCInspectorPresentationSHA', '[a-f0-9]{64}')
        assert sha(SCRIPT) == frozen and sha(BASELINE) == BASELINE_SHA
        directory = ROOT / run
        directory.mkdir(parents=True, exist_ok=True)
        assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Fresh run required'
        log_path = directory / 'Editor.log'
        logs = re.findall(r'(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
        assert len(logs) == 1 and Path(logs[0][0] or logs[0][1]).resolve() == log_path.resolve()
        assert all(flag in command.lower() for flag in ('-unattended', '-sccprovider=none', '-noautosave'))
        assert '-run=pythonscript' not in command.lower() and not ue.SourceControl.is_enabled()
        assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
        spec = importlib.util.spec_from_file_location('inspector_presentation_process_guard', P / 'Scripts/Editor/ingame_hud_layout_setup.py')
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        helper.assert_no_other_editor()
        base = read(BASELINE)
        report.update({'mode': mode, 'run': run, 'script_sha256': frozen, 'baseline_sha256': BASELINE_SHA})
        report['before_protection'] = protect(base)
        assert not dirty(), 'Clean Editor required'
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings_text = export(ue.get_default_object(settings), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        if mode == 'apply':
            assert not ue.EditorAssetLibrary.does_asset_exist(HIDDEN) and not disk(HIDDEN).exists()
            assert all(sha(P / name) == value for name, value in base['existing_asset_sha256'].items())
            catalog(base, False)
        else:
            reference = argument('DCInspectorPresentationReference', '[A-Za-z0-9_]{1,32}')
            previous_path = ROOT / reference / 'report.json'
            previous = read(previous_path)
            assert previous['mode'] == 'apply' and previous['complete'] and previous['protection_passed']
            assert previous['baseline_sha256'] == BASELINE_SHA
            assert reference == E_APPLY and sha(previous_path) == E_REPORT_SHA
            assert previous['script_sha256'] == E_SCRIPT_SHA and previous['asset_saves'] == 4
            assert previous['saved_files'] == E_SAVED_FILES
            assert set(previous['saved_packages']) == {HIDDEN, MAIN, WID, PAWN} and len(previous['saved_packages']) == 4
            assert sha(ROOT / 'BeforeSparseTolerancePinIdentity/inspector_bot_presentation_setup.py.txt') == E_SCRIPT_SHA
            assert previous['before_protection']['originals_unchanged'] and previous['after_protection']['originals_unchanged']
            log_check(ROOT / reference / 'Editor.log')
            report['read_only_predecessor'] = {'run': E_APPLY, 'report_sha256': E_REPORT_SHA,
                'old_script_sha256': E_SCRIPT_SHA, 'current_script_sha256': frozen, 'saved_files': E_SAVED_FILES}
            assert all(sha(P / name) == value for name, value in previous['saved_files'].items())
            catalog(base, True)
        source, wid, pawn, main = [load(package) for package in (WEAPON, WID, PAWN, MAIN)]
        source_before = weapon_state(source, directory, 'source_before', False)
        if mode == 'apply':
            _, wid_before = wid_state(wid, directory, 'before')
            hit_pins, hit_before = hit_node(main, directory, 'before')
            before = {'weapon': source_before, 'wid': wid_before, 'pawn': pawn_state(pawn, directory, 'before'),
                'hit': hit_before, 'compiled_hit': runtime_hit(main, directory, 'before')}
            dash = load(DASH)
            grants = list(dash.get_editor_property('GrantedGameplayAbilities'))
            assert len(grants) == 1 and grants[0].get_editor_property('Ability').get_path_name() == PRIVATE + '/Assets/GA_Hero_Dash.GA_Hero_Dash_C'
            assert not dash.get_editor_property('GrantedGameplayEffects') and not dash.get_editor_property('GrantedAttributes')
            report['dash_grants'] = [item.export_text() for item in grants]
            hidden = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset('B_Rifle_NoVisual', PRIVATE + '/Assets', source)
            assert hidden and hidden.get_path_name() == HIDDEN + '.B_Rifle_NoVisual'
            report['duplicate_may_save_new_package_only'] = True
            report['new_package_written_by_duplicate'] = disk(HIDDEN).exists()
            for name, obj in component_rows(hidden, True).items():
                if name in VISUALS:
                    obj.set_visibility(False, False)
                    obj.set_hidden_in_game(True, False)
            report.setdefault('compiles', []).append(compile_bp(hidden, log_path))
            cdo = ue.get_default_object(wid.generated_class())
            spawn = list(cdo.get_editor_property('ActorsToSpawn'))
            assert len(spawn) == 1
            original = spawn[0]
            changed = type(original)()
            assert changed.import_text(original.export_text())
            assert original.get_editor_property('ActorToSpawn').get_path_name() == WEAPON + '.B_Rifle_C'
            changed.set_editor_property('ActorToSpawn', hidden.generated_class())
            cdo.set_editor_property('ActorsToSpawn', [changed])
            report['compiles'].append(compile_bp(wid, log_path))
            sets = list(pawn.get_editor_property('AbilitySets'))
            assert [obj.get_path_name() for obj in sets].count(DASH + '.AS_DC_RifleDash') == 1
            pawn.set_editor_property('AbilitySets', [obj for obj in sets if obj.get_path_name() != DASH + '.AS_DC_RifleDash'])
            slot_pose, consumer_base, upstream_pose = hit_pins
            pinlib = ue.BlueprintGraphPinLibrary
            assert pinlib.break_single_pin_link(slot_pose, consumer_base), 'Cannot detach the exact hit-slot output'
            assert pinlib.try_create_connection(upstream_pose, consumer_base), 'Cannot connect the original source pose directly'
            _, rewired = hit_node(main, directory, 'rewired')
            check_hit_graph(before['hit'], rewired)
            report['compiles'].append(compile_bp(main, log_path, allow_unused_hit=True))
        else:
            hidden = load(HIDDEN)
            before = previous['before']
        _, wid_after = wid_state(wid, directory, 'after')
        after = {'weapon': weapon_state(hidden, directory, 'hidden_after', True), 'wid': wid_after,
            'pawn': pawn_state(pawn, directory, 'after')}
        _, hit_after = hit_node(main, directory, 'after')
        after.update({'hit': hit_after, 'compiled_hit': runtime_hit(main, directory, 'after')})
        report['weapon_copy_entry_identity'], report['sparse_tolerance_pin_identity'] = check_changes(before, after)
        assert weapon_state(source, directory, 'source_after', False) == source_before, 'Shared source weapon changed in memory'
        report.update({'before': before, 'after': after, 'memory_checks_passed': True})
        catalog(base, True)
        dirty()
        protect(base)
        log_check(log_path)
        helper.assert_no_other_editor()
        if mode == 'apply':
            assert all(sha(P / name) == value for name, value in base['existing_asset_sha256'].items()), 'Existing files changed before authorized save'
            for obj in (hidden, main, wid, pawn):
                assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False)
                report['saved_packages'].append(obj.get_path_name().split('.', 1)[0])
            report['asset_saves'] = len(report['saved_packages'])
        else:
            # Hit graph/tuning has already been rechecked against the original
            # before state above. Compiler diagnostics and pose-link indices
            # remain raw evidence, not an extra cross-process equality contract.
            current_stored = {k: v for k, v in after.items() if k not in ('hit', 'compiled_hit')}
            previous_stored = {k: v for k, v in previous['after'].items() if k not in ('hit', 'compiled_hit')}
            for state in (current_stored, previous_stored):
                weapon = dict(state['weapon'])
                weapon.pop('unlisted_template_observations')
                root = dict(weapon['effective_default_root'])
                root.pop('own_empty_placeholder')
                weapon['effective_default_root'] = root
                state['weapon'] = weapon
            assert current_stored == previous_stored
            report['fresh_reload_verified'] = True
        assert not dirty(), 'Clean saved/reloaded state required'
        report['saved_files'] = {name: sha(P / name) for name in base['asset_files']}
        if mode == 'verify':
            assert report['saved_files'] == previous['saved_files']
        report['log_check'] = log_check(log_path)
        assert sha(SCRIPT) == frozen
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            if base is not None:
                report['after_protection'] = protect(base)
                report['protection_passed'] = True
        except Exception:
            report['complete'] = False
            report['protection_error'] = traceback.format_exc()
            raise
        finally:
            try:
                if directory is not None:
                    persist(directory / 'report.json', report)
            finally:
                ue.SystemLibrary.quit_editor()


def commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,20}', prefix)
    result = {}
    for mode in ('verify',):
        run = prefix + '_' + mode
        directory = ROOT / run
        args = ['E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe', (P / 'DreamCatcher.uproject').as_posix(),
            "-ExecCmds=py __import__('runpy').run_path('" + SCRIPT.as_posix() + "')['main']()", '-unattended', '-nop4',
            '-SCCProvider=None', '-nullrhi', '-nosound', '-NoSplash', '-NoAutoSave',
            '-DCInspectorPresentationMode=' + mode, '-DCInspectorPresentationRun=' + run,
            '-DCInspectorPresentationSHA=' + sha(SCRIPT), '-abslog=' + (directory / 'Editor.log').as_posix()]
        if mode == 'verify':
            args.append('-DCInspectorPresentationReference=' + E_APPLY)
        result[mode] = {'args': args, 'windows_command_line': subprocess.list2cmdline(args),
            'create_log_directory_first': directory.as_posix()}
    print(json.dumps({'launches_processes': False, 'PIE': False, 'cpp_build': False, 'commands': result}, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        commands(sys.argv[2])
    else:
        main()
