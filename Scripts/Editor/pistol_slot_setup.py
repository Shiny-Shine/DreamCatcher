"""Approved original Lyra pistol import and human starting slot 2.

Offline: python pistol_slot_setup.py --commands <fresh_prefix> prints argument
arrays and Windows command lines; it never launches them. Create each printed
log directory before launch. UE modes are migrate (original Lyra), apply
(DreamCatcher with the printed process-only package redirects), and verify
(fresh DreamCatcher process WITHOUT those redirects). No PIE or build mode.

Migrate uses Engine MigratePackages for exactly the approved 196 new packages.
Apply validates loaded intermediate copies before RenameAssets (which can save),
repairs only the known inherited skeletal component override through the engine
SubobjectData API, adds the two approved HUD guards, and sets StartingPistol on
the one production Controller. Existing shared packages are never saved.
Verify requests no compile/save/repair and compares the completed apply record.
The later approved extension adds one dedicated root AnimBP copy (197 new
packages total), restores only its authored hand_l vector/pin to original zero,
and reparents the two new pistol layers. Its engine duplication can leave that
one file saved before later validation; the report records this checkpoint.
Failures preserve reports and partial engine output; there is no silent resume,
overwrite, binary copy, redirector deletion, or automatic cleanup of assets.
"""
import difflib
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
SOURCE = PROJECT.parent / 'LyraStarterGame'
REPORTS = PROJECT / 'Saved/Diagnostics/Pistol20261020'
SCRIPT = PROJECT / 'Scripts/Editor/pistol_slot_setup.py'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
CONTROLLER = '/Game/DreamCatcher/Blueprints/Player/BP_DC_StageUIController'
RIFLE = '/Game/DreamCatcher/Lyra/Weapons/Rifle/ID_Rifle'
GE = '/Game/Weapons/Pistol/GE_Damage_Pistol'
CORE = '/ShooterCore/Weapons/Pistol/'
AMMO = CORE + 'W_AmmoCounter_Pistol'
RETICLE = CORE + 'W_Reticle_Pistol'
ACTOR = CORE + 'B_Pistol'
INSTANCE = CORE + 'B_WeaponInstance_Pistol'
FIRE = CORE + 'GA_Weapon_Fire_Pistol'
ITEM = CORE + 'ID_Pistol'
WID = CORE + 'WID_Pistol'
SHARED_ANIM_BASE = '/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase'
PISTOL_ANIM_BASE = '/Game/DreamCatcher/Lyra/Weapons/Pistol/Animations/Character/LinkedLayers/ABP_PistolItemAnimLayersBase'
PISTOL_LAYERS = ('/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/ABP_PistolAnimLayers',
                 '/Game/Characters/Heroes/Mannequin/Animations/Locomotion/Pistol/ABP_PistolAnimLayers_Feminine')
TAG = 'GameplayCue.Weapon.Pistol.Fire'
SIDECARS = ('.uasset', '.uexp', '.ubulk', '.uptnl')
KNOWN_LOAD_DIRTY = {'/Game/Effects/Particles/Weapons/NS_WeaponFire',
                    '/Game/Effects/Particles/Weapons/Emitters/NE_MuzzleFlashCards'}
REFERENCE_READ_SERIAL = 0


def read_json(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def persist(path, value):
    Path(path).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def manifest():
    scope = read_json(REPORTS / 'scope_candidates.json')
    baseline = read_json(REPORTS / 'approved_baseline.json')
    assert sha(REPORTS / 'scope_candidates.json') == baseline['manifest_sha256']
    mapping, reuse = scope['proposed_mapping'], scope['reuse_candidates']
    assert len(mapping) == 196 and len(reuse) == 60
    assert set(mapping) == set(scope['new_asset_candidates'])
    assert len(set(mapping.values())) == 196 and not (set(mapping) & set(reuse))
    assert all(new.startswith('/Game/DreamCatcher/Lyra/Weapons/Pistol/') for new in mapping.values())
    assert baseline['allowed_existing_content'] == [str(asset_file(PROJECT, CONTROLLER).relative_to(PROJECT)).replace('\\', '/')]
    return scope, baseline


def staged(package):
    # Engine PackageMigrationContext.cpp GetNewPackageNameForMigration maps the
    # ShooterCore mount into /Game while retaining Weapons/Pistol/<asset>.
    return '/Game/' + package.removeprefix('/ShooterCore/') if package.startswith(CORE) else package


def asset_file(root, package, suffix='.uasset'):
    assert package.startswith('/Game/')
    return Path(root) / 'Content' / (package[6:] + suffix)


def inventory(root):
    return {str(path.relative_to(root)).replace('\\', '/'): [path.stat().st_size, path.stat().st_mtime_ns]
            for path in (Path(root) / 'Content').rglob('*') if path.is_file()}


def source_inventory():
    result = {}
    for directory in (SOURCE / 'Content', SOURCE / 'Plugins/GameFeatures/ShooterCore/Content'):
        for path in directory.rglob('*'):
            if path.is_file():
                result[str(path)] = [path.stat().st_size, path.stat().st_mtime_ns]
    return result


def redirected_argument(scope):
    pairs = {old: new for old, new in scope['reuse_candidates'].items() if old != new}
    pairs.update({old: staged(old) for old in scope['source_core']})
    # ConfigCacheIni.cpp ArrayAdd: repeat [CoreRedirects]:+PackageRedirects keys
    # inside ONE -ini:Engine argument. Do not redirect the original source run.
    rows = ['[CoreRedirects]:+PackageRedirects=(OldName="' + old + '",NewName="' + new + '")'
            for old, new in sorted(pairs.items())]
    return '-ini:Engine:' + ','.join(rows)


def print_commands(prefix, recovery=None):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,48}', prefix)
    assert recovery in (None, 'Port_P_apply')
    scope, _ = manifest()
    commands = {}
    for mode in (('apply', 'verify') if recovery else ('migrate', 'apply', 'verify')):
        run = prefix + '_' + mode
        root = SOURCE if mode == 'migrate' else PROJECT
        args = [ENGINE.as_posix(), (root / (root.name + '.uproject')).as_posix(), '-run=pythonscript',
                '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
                '-nosound', '-nullrhi', '-EnablePlugins=UMGToolSet',
                '-DCPistolSetupMode=' + mode, '-DCPistolSetupRun=' + run,
                '-abslog=' + (REPORTS / run / 'Editor.log').as_posix()]
        if mode != 'migrate':
            reference = 'Port_C_migrate' if recovery and mode == 'apply' else prefix + ('_migrate' if mode == 'apply' else '_apply')
            args.append('-DCPistolSetupReference=' + reference)
        if mode == 'apply' and recovery:
            args.append('-DCPistolSetupRecovery=' + recovery)
        elif mode == 'apply':
            args.append(redirected_argument(scope))
        # Unreal reads the raw Windows command line. Keep the validated -ini
        # value's inner quotes literal, rather than CRT-backslash escaping them.
        assert not any('"' in arg and ' ' in arg for arg in args)
        commands[mode] = {'args': args, 'windows_command_line': ' '.join('"' + arg + '"' if ' ' in arg else arg for arg in args),
                          'launch_guidance': 'Use this raw command-line string with shell=False on Windows; do not CRT-escape the inner -ini quotes.',
                          'create_log_directory_first': (REPORTS / run).as_posix()}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False, 'commands': commands}, indent=2))


def import_existing(name):
    spec = importlib.util.spec_from_file_location('pistol_reuse_' + name, SCRIPT.parent / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def text_file(path):
    data = Path(path).read_bytes()
    return data.decode('utf-16' if data[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')


def export(obj, directory, label):
    path = directory / (label + '.t3d')
    assert not path.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter, task.filename = obj, ue.ObjectExporterT3D(), str(path)
    task.automated, task.prompt, task.replace_identical = True, False, False
    assert ue.Exporter.run_asset_export_task(task), obj.get_path_name()
    return text_file(path)


def normalizer(scope):
    pairs = dict(scope['proposed_mapping'])
    pairs.update(scope['reuse_candidates'])
    pairs.update({staged(old): new for old, new in scope['proposed_mapping'].items()})
    # Explicit approved extra asset, outside the historical 196-entry manifest.
    old_name, new_name = SHARED_ANIM_BASE.rsplit('/', 1)[1], PISTOL_ANIM_BASE.rsplit('/', 1)[1]
    pairs.update({SHARED_ANIM_BASE: PISTOL_ANIM_BASE,
                  SHARED_ANIM_BASE + '.' + old_name: PISTOL_ANIM_BASE + '.' + new_name,
                  SHARED_ANIM_BASE + '.' + old_name + '_C': PISTOL_ANIM_BASE + '.' + new_name + '_C',
                  old_name: new_name, old_name + '_C': new_name + '_C'})
    for old, new in re.findall(r'OldName="([^"]+)",NewName="([^"]+)"',
                               (PROJECT / 'Config/DefaultEngine.ini').read_text(encoding='utf-8-sig')):
        pairs[old] = new
    pairs = {old: new for old, new in pairs.items() if old != new}
    # Match complete absolute path tokens, never a /Game/... suffix embedded in
    # an already-mapped /Game/DreamCatcher/.../Game/... destination.
    pattern = re.compile(r'(?<![A-Za-z0-9_/])(?:' +
                         '|'.join(re.escape(old) for old in sorted(pairs, key=len, reverse=True)) +
                         r')(?![A-Za-z0-9_/])')

    def canonical(value):
        return pattern.sub(lambda match: pairs[match.group(0)], value)
    return canonical


def pin(node, name):
    rows = [value for value in ue.BlueprintEditorLibrary.list_all_pins(node) if str(value.get_pin_name()) == name]
    assert len(rows) == 1, (node.get_name(), name)
    return rows[0]


def nodes(bp):
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
    assert editor
    return editor, {node.get_name(): node for node in editor.list_all_nodes()}


def links(value):
    return sorted([[p.get_owning_node().get_name(), str(p.get_pin_name())] for p in value.list_connected_pins()])


def graphs(bp):
    result = []
    for graph in ue.BlueprintEditorLibrary.list_graphs(bp):
        rows = []
        for node in ue.ObjectIterator(ue.EdGraphNode):
            if node.get_outer() != graph:
                continue
            row = {'name': node.get_name(), 'class': node.get_class().get_path_name()}
            if 'K2Node' in node.get_class().get_name():
                row['pins'] = [{'name': str(p.get_pin_name()), 'value': p.get_pin_value(), 'links': links(p)}
                               for p in ue.BlueprintEditorLibrary.list_all_pins(node)]
            rows.append(row)
        result.append({'name': graph.get_name(), 'nodes': sorted(rows, key=lambda row: row['name'])})
    return sorted(result, key=lambda row: row['name'])


def source_graphs(rows):
    result = []
    for graph in rows:
        entries = []
        for node in graph['nodes']:
            row = {'name': node['name'], 'class': node['class']}
            if 'K2Node' in node['class']:
                row['pins'] = [{'name': p['name'], 'value': p['value'], 'links': sorted(p['links'])}
                               for p in node['pins']]
            entries.append(row)
        result.append({'name': graph['name'], 'nodes': sorted(entries, key=lambda row: row['name'])})
    return sorted(result, key=lambda row: row['name'])


def comparable_graphs(value, canonical, asset=None):
    rows = json.loads(json.dumps(value))
    if asset == RETICLE:
        # Only these two reconstructed native-type pin identifiers and links to
        # them differ. Values and every other node/pin name remain exact.
        names = {('K2Node_BreakStruct_1', 'LyraVerbMessage'): 'DCVerbMessage',
                 ('K2Node_DynamicCast_1', 'AsLyra Player State'): 'AsDCPlayer State'}
        for graph in rows:
            if graph['name'] != 'EventGraph':
                continue
            for node in graph['nodes']:
                for value in node.get('pins', []):
                    value['name'] = names.get((node['name'], value['name']), value['name'])
                    value['links'] = [[owner, names.get((owner, name), name)] for owner, name in value['links']]
    return canonical(json.dumps(rows, ensure_ascii=False, sort_keys=True))


def object_blocks(text, wanted):
    lines, result, stack = text.splitlines(), {}, []
    for index, line in enumerate(lines):
        if line.lstrip().startswith('Begin Object'):
            match = re.search(r'\bName="([^"]+)"', line)
            stack.append((index, match.group(1) if match else None))
        elif line.strip() == 'End Object':
            assert stack, 'Unbalanced exported object'
            start, name = stack.pop()
            if name in wanted:
                block = '\n'.join(lines[start:index + 1])
                if len(block) > len(result.get(name, '')):
                    result[name] = block
    assert not stack
    return result


def widget_visuals(text):
    names = {'WidgetTree'} | set(re.findall(r'Class=/Script/UMG.WidgetAnimation Name="([^"]+)"', text))
    result = object_blocks(text, names)
    assert 'WidgetTree' in result
    cached = re.findall(r'^      AllWidgets\((\d+)\)="([^\']+)\'([^\']+)\'"$', result['WidgetTree'], re.M)
    if cached:
        owner = re.search(r'^Begin Object[^\n]* Name="([^"]+)"', text)[1]
        assert owner in ('W_AmmoCounter_Pistol', 'W_Reticle_Pistol')
        count = 6 if owner == 'W_AmmoCounter_Pistol' else 9
        widgets = {}
        for cls, name, path in re.findall(r'Begin Object Class=(\S+) Name="([^"]+)"[^\n]* ExportPath="[^\']*\'([^\']+)\'"', text):
            if ':WidgetTree.' in path and '.' not in path.split(':WidgetTree.', 1)[1]:
                assert name not in widgets or widgets[name] == cls
                widgets[name] = cls
        assert len(widgets) == len(cached) == count
        assert [int(row[0]) for row in cached] == list(range(count))
        cache_widgets = {}
        for _, cls, reference in cached:
            name = reference.rsplit('.', 1)[-1]
            assert name not in cache_widgets and name in widgets and widgets[name] == cls
            assert reference == name or reference.endswith(':' + 'WidgetTree.' + name)
            cache_widgets[name] = cls
        assert cache_widgets == widgets
        # Only this editor-only root cache is removed. Every widget definition,
        # slot/parent relationship, material and animation remains compared.
        result['WidgetTree'] = re.sub(r'^      AllWidgets\(\d+\)="[^\n]+"\n?', '', result['WidgetTree'], flags=re.M)
    result['animation_bindings'] = '\n'.join(line for line in text.splitlines()
                                             if re.match(r'^   (?:Animations|Bindings)(?:\(|=)', line))
    return result


def item_subobject_order(text):
    """Compare ID fragment object blocks by name, preserving every block byte.

    UObject export enumeration is not Fragments array order. Root properties
    and all array text remain ordered; the array is also checked via public API.
    """
    lines = text.splitlines()
    assert lines[0].startswith('Begin Object') and lines[-1] == 'End Object'
    root, blocks, index = [lines[0]], {}, 1
    while index < len(lines):
        line = lines[index]
        if not line.startswith('   Begin Object'):
            root.append(line)
            index += 1
            continue
        start, depth = index, 0
        while index < len(lines):
            if lines[index].lstrip().startswith('Begin Object'):
                depth += 1
            elif lines[index].strip() == 'End Object':
                depth -= 1
            index += 1
            if depth == 0:
                break
        assert depth == 0, 'Unbalanced fragment object block'
        name = re.search(r'\bName="([^"]+)"', line).group(1)
        key = (name, 'declaration' if ' Class=' in line else 'definition')
        assert key not in blocks, 'Duplicate fragment object block: ' + str(key)
        blocks[key] = '\n'.join(lines[start:index])
    return '\n'.join([root[0]] + [blocks[key] for key in sorted(blocks)] + root[1:]) + '\n'


def equal_text(expected, actual, label, report, collect=False):
    if expected != actual:
        report.setdefault('comparison_failures', {})[label] = list(difflib.unified_diff(
            expected.splitlines(), actual.splitlines(), fromfile='original', tofile='target', n=2))[:100]
        if not collect:
            raise AssertionError('Original-data mismatch: ' + label)
        return False
    return True


def actor_component(bp, create=False):
    if create:
        lib = ue.SubobjectDataBlueprintFunctionLibrary
        subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
        matches = []
        for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
            data = lib.get_data(handle)
            if str(lib.get_variable_name(data)) == 'SkeletalMesh':
                matches.append(lib.get_object_for_blueprint(data, bp))
        assert len(matches) == 1 and isinstance(matches[0], ue.SkeletalMeshComponent)
        obj = matches[0]
    else:
        path = bp.generated_class().get_path_name() + ':SkeletalMesh_GEN_VARIABLE'
        matches = [obj for obj in ue.ObjectIterator(ue.SkeletalMeshComponent) if obj.get_path_name() == path]
        assert len(matches) == 1, 'Saved inherited skeletal override missing: ' + path
        obj = matches[0]
    assert obj.get_outer() == bp.generated_class(), 'Refuse to alter a shared parent component'
    return obj


def component_values(obj):
    def vector(value):
        return [float(value.x), float(value.y), float(value.z)]
    rotation = obj.get_editor_property('RelativeRotation')
    return {'mesh': obj.get_editor_property('SkeletalMeshAsset').get_path_name()
            if obj.get_editor_property('SkeletalMeshAsset') else None,
            'anim': obj.get_editor_property('AnimClass').get_path_name() if obj.get_editor_property('AnimClass') else None,
            'location': vector(obj.get_editor_property('RelativeLocation')),
            'rotation': [float(rotation.pitch), float(rotation.yaw), float(rotation.roll)],
            'scale': vector(obj.get_editor_property('RelativeScale3D'))}


def check_component(bp, canonical, report, assets, repair=False):
    source = read_json(REPORTS / 'SourceFields_C.json')['actor_components']
    assert len(source) == 1
    expected = {'mesh': canonical(source[0]['SkeletalMeshAsset']), 'anim': canonical(source[0]['AnimClass']),
                'location': [0., 0., 0.], 'rotation': [0., 0., 0.], 'scale': [1., 1., 1.]}
    for name, values in [('RelativeLocation', (0, 0, 0)), ('RelativeRotation', (0, 0, 0)), ('RelativeScale3D', (1, 1, 1))]:
        numbers = re.findall(r'(?:x|y|z|pitch|yaw|roll):\s*(-?[\d.]+)', source[0][name])
        assert tuple(map(float, numbers)) == values, 'Source transform changed: ' + name
    obj = actor_component(bp, create=repair)
    before = component_values(obj)
    normalize_paths = lambda values: {key: canonical(value) if isinstance(value, str) else value
                                     for key, value in values.items()}
    if normalize_paths(before) != expected and repair:
        # GetObjectForBlueprint creates/reuses the override with the CURRENT
        # parent's SCS key. Only this new child-owned template is modified.
        obj.set_editor_property('SkeletalMeshAsset', ue.load_asset(expected['mesh'].split('.')[0]))
        animation = assets[source[0]['AnimClass'].split('.')[0]]
        assert isinstance(animation, ue.Blueprint) and animation.generated_class()
        obj.set_editor_property('AnimClass', animation.generated_class())
        obj.set_editor_property('RelativeLocation', ue.Vector(0., 0., 0.))
        obj.set_editor_property('RelativeRotation', ue.Rotator(0., 0., 0.))
        obj.set_editor_property('RelativeScale3D', ue.Vector(1., 1., 1.))
        report.setdefault('inherited_override_repairs', []).append({'before': before, 'expected': expected})
    assert normalize_paths(component_values(obj)) == expected, 'Inherited skeletal override differs; save refused'
    report['inherited_component'] = expected


def ammo_guard(bp):
    editor, current = nodes(bp)
    names = {}
    for route, event, target, inventory_node in ammo.ROUTES:
        assert links(pin(current[event], 'then')) == [[target, 'execute']]
        valid = editor.add_call_function_node('/Script/Engine.KismetSystemLibrary.IsValid')
        branch = editor.add_branch_node()
        assert valid and branch
        assert pin(current[event], 'then').break_single_pin_link(pin(current[target], 'execute'))
        for start, end in [(pin(current[event], 'then'), pin(branch, 'execute')),
                           (pin(current[inventory_node], 'InventoryInstance'), pin(valid, 'Object')),
                           (pin(valid, 'ReturnValue'), pin(branch, 'Condition')),
                           (pin(branch, 'then'), pin(current[target], 'execute'))]:
            assert start.try_create_connection(end)
        names[route] = {'valid': valid.get_name(), 'branch': branch.get_name()}
    ammo.check_routes(bp, names)
    return names


def reticle_guard(bp):
    editor, current = nodes(bp)
    listener, target, cast = (current[name] for name in (
        'K2Node_AsyncAction_ListenForGameplayMessages_0', 'K2Node_IfThenElse_0', 'K2Node_DynamicCast_1'))
    assert links(pin(listener, 'OnMessageReceived')) == [[target.get_name(), 'execute']]
    assert not links(pin(cast, 'bSuccess'))
    branch = editor.add_branch_node()
    assert branch
    assert pin(listener, 'OnMessageReceived').break_single_pin_link(pin(target, 'execute'))
    for start, end in [(pin(listener, 'OnMessageReceived'), pin(branch, 'execute')),
                       (pin(cast, 'bSuccess'), pin(branch, 'Condition')),
                       (pin(branch, 'then'), pin(target, 'execute'))]:
        assert start.try_create_connection(end)
    check_reticle_guard(bp, branch.get_name())
    return branch.get_name()


def recovery_guards(assets, partial):
    known = partial['guard_names']
    _, current = nodes(assets[AMMO])
    names = {name for group in known['ammo'].values() for name in group.values()}
    present = names & current.keys()
    assert not present or present == names, 'Partially persisted ammo guard is not an approved recovery state'
    if present:
        ammo.check_routes(assets[AMMO], known['ammo'])
        ammo_names = known['ammo']
    else:
        ammo_names = ammo_guard(assets[AMMO])
    _, current = nodes(assets[RETICLE])
    if known['reticle'] in current:
        check_reticle_guard(assets[RETICLE], known['reticle'])
        reticle_name = known['reticle']
    else:
        reticle_name = reticle_guard(assets[RETICLE])
    return {'ammo': ammo_names, 'reticle': reticle_name}


def check_reticle_guard(bp, name):
    _, current = nodes(bp)
    assert links(pin(current['K2Node_AsyncAction_ListenForGameplayMessages_0'], 'OnMessageReceived')) == [[name, 'execute']]
    assert links(pin(current['K2Node_DynamicCast_1'], 'bSuccess')) == [[name, 'Condition']]
    assert links(pin(current[name], 'then')) == [['K2Node_IfThenElse_0', 'execute']]
    assert not links(pin(current[name], 'else')), 'Failed cast must not execute the GetController-dependent condition'


def strip_guards(value, names, kind):
    rows = json.loads(json.dumps(value))
    if kind == 'ammo':
        added = {name for group in names.values() for name in group.values()}
        mutable = {(event, 'then') for _, event, _, _ in ammo.ROUTES}
        mutable |= {(target, 'execute') for _, _, target, _ in ammo.ROUTES}
        mutable |= {(inventory_node, 'InventoryInstance') for _, _, _, inventory_node in ammo.ROUTES}
    else:
        added = {names} if names else set()
        mutable = {('K2Node_AsyncAction_ListenForGameplayMessages_0', 'OnMessageReceived'),
                   ('K2Node_IfThenElse_0', 'execute'), ('K2Node_DynamicCast_1', 'bSuccess')}
    for graph in rows:
        if graph['name'] == 'EventGraph':
            graph['nodes'] = [node for node in graph['nodes'] if node['name'] not in added]
            for node in graph['nodes']:
                for value in node.get('pins', []):
                    if (node['name'], value['name']) in mutable:
                        value['links'] = []
    return rows


def validate_original(assets, canonical, directory, label, report, guard_names=None):
    audit = read_json(REPORTS / 'SourceInspect_B/report.json')
    comparisons_ok = True
    for index, (old, source) in enumerate(audit['assets'].items()):
        if old not in assets:
            continue
        obj = assets[old]
        assert obj.get_class().get_path_name() == canonical(source['class']), old
        if not isinstance(obj, ue.Blueprint):
            actual = export(obj, directory, label + '_asset_%02d' % index)
            comparisons_ok = equal_text(canonical(text_file(source['export'])), canonical(actual), old, report,
                                        collect=True) and comparisons_ok
            continue
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(obj).get_path_name() == canonical(source['parent']), old
        actual = export(ue.get_default_object(obj.generated_class()), directory, label + '_cdo_%02d' % index)
        expected = canonical(text_file(source['cdo_export']))
        actual = canonical(actual)
        if old in PISTOL_LAYERS:
            caches = report['duplicate_metadata_correspondence']['generated_property_names']
            assert len(caches) == 13
            pairs = []
            for cache in caches:
                names = [re.fullmatch(r'"(K2Node_PropertyAccess(?:_\d+)?)"', cache[key])
                         for key in ('source', 'target')]
                assert all(names)
                before_name, after_name = [match[1] for match in names]
                before_pattern = r'^   ' + re.escape(before_name) + r'=(.*)$'
                after_pattern = r'^   ' + re.escape(after_name) + r'=(.*)$'
                before_values = re.findall(before_pattern, expected, re.M)
                after_values = re.findall(after_pattern, actual, re.M)
                assert len(before_values) == 1
                if 'root_reload_basis' in report:
                    assert before_values[0] in ('0.000000', 'False', 'None', '(X=0.000000,Y=0.000000,Z=0.000000)')
                    old_actual_values = re.findall(before_pattern, actual, re.M)
                    assert len(after_values) + len(old_actual_values) <= 1
                    assert not after_values or after_values == before_values
                    assert not old_actual_values or old_actual_values == before_values
                    # These exact 13 properties are compiler-created internal
                    # destinations, marked BlueprintCompilerGeneratedDefaults
                    # by K2Node_PropertyAccess.cpp:29-33. Fresh loads may retain
                    # their original names or omit the zero CDO default. The
                    # serialized root/bindings are hash-bound to Port_P.
                    expected = re.sub(before_pattern + r'\n?', '', expected, count=1, flags=re.M)
                    actual = re.sub(after_pattern + r'\n?', '', actual, count=1, flags=re.M)
                    actual = re.sub(before_pattern + r'\n?', '', actual, count=1, flags=re.M)
                    report['compiler_generated_cache_default_basis'] = {
                        'properties': 13, 'source_each_once_and_known_defaults_checked': True,
                        'present_target_values_equal': True,
                        'engine': 'K2Node_PropertyAccess.cpp:21-33; AnimBlueprintCompiler.cpp:1837-1842',
                        'metadata': 'BlueprintCompilerGeneratedDefaults=true; private compiler-created destination',
                        'CPF_Transient_claimed': False, 'root_reload_hash_basis': report['root_reload_basis']}
                else:
                    assert len(after_values) == 1 and before_values == after_values, (old, before_name, after_name, before_values, after_values)
                    actual = re.sub(after_pattern, lambda match: '   ' + before_name + '=' + match[1], actual, count=1, flags=re.M)
                pairs.append([before_name, after_name])
            assert len({pair[0] for pair in pairs}) == len({pair[1] for pair in pairs}) == 13
            report.setdefault('cdo_cache_property_name_correspondence', {})[label + ':' + old] = pairs
        if old == INSTANCE:
            # One already-approved native addition, absent from original Lyra.
            actual = re.sub(r'^   DamageToArmorRatio=0\.800000\n', '', actual, flags=re.M)
        if old == ITEM:
            expected, actual = item_subobject_order(expected), item_subobject_order(actual)
            source_fragments = read_json(REPORTS / 'SourceCompile_H.json')[ITEM]['Fragments']
            target_fragments = [{'name': obj.get_name(), 'class': obj.get_class().get_path_name()}
                                for obj in ue.get_default_object(assets[ITEM].generated_class()).get_editor_property('Fragments')]
            expected_fragments = [{'name': obj['name'], 'class': canonical(obj['class'])} for obj in source_fragments]
            report['item_fragment_order'] = {'source': expected_fragments, 'target': target_fragments}
            comparisons_ok = equal_text(json.dumps(expected_fragments, sort_keys=True), json.dumps(target_fragments, sort_keys=True),
                                        old + ' public Fragments order', report, collect=True) and comparisons_ok
        if old == FIRE:
            # Only this reproduced inherited FText resource-id correspondence:
            # the exact cost-failure tag and literal must match on both sides.
            no_ammo = re.compile(r'^   FailureTagToUserFacingMessages=\(\(\(TagName="Ability\.ActivateFail\.Cost"\), NSLOCTEXT\("([^"\n]+)", "([^"\n]+)", "No Ammo!"\)\)\)$', re.M)
            source_ids, target_ids = no_ammo.findall(expected), no_ammo.findall(actual)
            if len(source_ids) == len(target_ids) == 1 and source_ids != target_ids:
                report.setdefault('no_ammo_resource_id_correspondence', {})[label] = {
                    'asset': FIRE, 'property': 'FailureTagToUserFacingMessages',
                    'tag': 'Ability.ActivateFail.Cost', 'literal': 'No Ammo!',
                    'source_namespace_key': source_ids[0], 'target_namespace_key': target_ids[0],
                    'normalized_for_comparison_only': True}
                actual = no_ammo.sub(lambda match: no_ammo.search(expected).group(0), actual)
        comparisons_ok = equal_text(expected, actual, old + ' CDO', report, collect=True) and comparisons_ok
        expected_graph = source_graphs(source['graphs'])
        actual_graph = graphs(obj)
        if old in (AMMO, RETICLE) and guard_names:
            kind = 'ammo' if old == AMMO else 'reticle'
            expected_graph = strip_guards(expected_graph, {} if kind == 'ammo' else None, kind)
            actual_graph = strip_guards(actual_graph, guard_names[kind], kind)
        comparisons_ok = equal_text(comparable_graphs(expected_graph, canonical, old), comparable_graphs(actual_graph, canonical, old),
                                    old + ' graph', report, collect=True) and comparisons_ok
        if old in (AMMO, RETICLE):
            original_visuals = widget_visuals(text_file(source['export']))
            actual_visuals = widget_visuals(export(obj, directory, label + '_widget_%02d' % index))
            comparisons_ok = equal_text(canonical(json.dumps(original_visuals, sort_keys=True)),
                                        canonical(json.dumps(actual_visuals, sort_keys=True)), old + ' layout/animations',
                                        report, collect=True) and comparisons_ok
    assert comparisons_ok, 'Original-data mismatches collected; see comparison_failures. Rename/save refused.'
    # Public properties catch missing derived CDO overrides that a successful
    # Blueprint compile alone does not establish.
    source_fields = read_json(REPORTS / 'SourceFields_C.json')[WID]
    cdo = ue.get_default_object(assets[WID].generated_class())
    assert canonical(cdo.get_editor_property('InstanceType').get_path_name()) == canonical(source_fields['InstanceType'])
    assert [canonical(obj.get_path_name()) for obj in cdo.get_editor_property('AbilitySetsToGrant')] == [canonical(x) for x in source_fields['AbilitySetsToGrant']]
    assert [canonical(row.export_text()) for row in cdo.get_editor_property('ActorsToSpawn')] == [canonical(x) for x in source_fields['ActorsToSpawn']]
    fire = ue.get_default_object(assets[FIRE].generated_class())
    assert abs(float(fire.get_editor_property('FireDelayTimeSecs')) - .12) < 1.e-6
    assert fire.get_editor_property('GE_Damage').get_path_name() == GE + '.GE_Damage_Pistol_C'
    weapon = ue.get_default_object(assets[INSTANCE].generated_class())
    assert abs(float(weapon.get_editor_property('DamageToArmorRatio')) - .8) < 1.e-6
    ge = ue.get_default_object(ue.EditorAssetLibrary.load_blueprint_class(GE))
    expected_ge = read_json(REPORTS / 'TargetFields_D.json')[GE]
    assert [row.export_text() for row in ge.get_editor_property('Executions')] == expected_ge['Executions']
    assert not list(ge.get_editor_property('Modifiers'))
    report['preserved_contract'] = dict(magazine_size=12, magazine_ammo=12, spare_ammo=48,
                                       fire_delay=.12, damage_bonus=18, armor_ratio=.8,
                                       attachment='weapon_r, yaw -90', source_CDO_and_audited_graphs=True,
                                       source_widget_tree_and_animations=True, PIE=False)


def compile_new(assets, report):
    for old, obj in assets.items():
        if isinstance(obj, ue.Blueprint):
            start = log_offset()
            assert ue.BlueprintEditorLibrary.compile_blueprint(obj), old
            assert_blueprint_status(obj)
            scoped_log_check(report, start)
            report['compile_requested'].append(obj.get_path_name())


def log_offset():
    ue.log_flush()
    return ACTIVE_LOG.stat().st_size


def error_rows(start=0, end=None):
    ue.log_flush()
    raw = ACTIVE_LOG.read_bytes()
    end = len(raw) if end is None else end
    rows, offset = [], start
    for line in raw[start:end].splitlines(keepends=True):
        text = line.decode('utf-8-sig', errors='replace').rstrip('\r\n')
        if re.search(r'\bLog\w+:\s*(?:Error|Fatal):|Ensure condition failed:|=== Handled ensure:', text):
            rows.append({'offset': offset, 'line': text})
        offset += len(line)
    return rows


def scoped_log_check(report, start=0):
    allowed = report.get('auto_duplicate_transient_compiler_error_recovered', False)
    known = report.get('auto_duplicate_log', {})
    bad = [row for row in error_rows(start) if not (allowed and known.get('error') == row
           and known['start'] <= row['offset'] < known['end'])]
    assert not bad, 'Engine Error/ensure/Fatal outside recovered duplicate event: ' + str(bad[:8])


def assert_blueprint_status(bp):
    status = str(bp.get_editor_property('Status'))
    name = status.split(':', 1)[0].rsplit('.', 1)[-1]
    assert name in ('BS_UP_TO_DATE', 'BS_UP_TO_DATE_WITH_WARNINGS'), (bp.get_path_name(), status)
    return status


def authored_nodes(graph, raw):
    # UEdGraph.Nodes cannot be Pythonized through EditorProperty. Its root-level
    # serialized array is authoritative; nested graph arrays are never included.
    rows = re.findall(r'^   Nodes\((\d+)\)="([^"\n]+)"$', raw, re.M)
    assert [int(index) for index, _ in rows] == list(range(len(rows)))
    names, classes = [], []
    for _, value in rows:
        match = re.fullmatch(r"([^']+)'([^']+)'", value)
        assert match, value
        names.append(match[2].rsplit('.', 1)[-1])
        classes.append(match[1])
    assert len(names) == len(set(names))
    objects = {obj.get_name(): obj for obj in ue.ObjectIterator(ue.EdGraphNode)
               if obj.get_outer() == graph and obj.get_name() in names}
    assert set(objects) == set(names), ('Authored node membership/outer mismatch', graph.get_path_name())
    assert all(objects[name].get_class().get_path_name() == cls for name, cls in zip(names, classes))
    return [objects[name] for name in names]


def reference_graph_text(graph):
    global REFERENCE_READ_SERIAL
    REFERENCE_READ_SERIAL += 1
    return export(graph, ACTIVE_LOG.parent, 'reference_graph_%03d' % REFERENCE_READ_SERIAL)


def resolve_class_reference(value, owner):
    match = re.fullmatch(r'"/Script/(?:Engine.AnimBlueprintGeneratedClass|CoreUObject.Class)\'([^\']+)\'"', value)
    assert match, ('Unsupported class-reference text', value)
    path = match[1]
    if not path.startswith('/'):
        assert re.fullmatch(r'[A-Za-z0-9_]+', path), 'Relative class must belong to this package only'
        path = owner.get_path_name().split('.', 1)[0] + '.' + path
    cls = ue.load_class(None, path)
    assert cls and cls.get_path_name() == path, ('Unresolved actual class reference', path)
    return cls


def raw_cast_class(node, raw):
    block = object_blocks(raw, {node.get_name()})[node.get_name()]
    values = re.findall(r'^      TargetType=(.+)$', block, re.M)
    assert len(values) == 1, 'Missing explicit raw cast TargetType'
    return resolve_class_reference(values[0], node)


def actual_pin_class(value, owner):
    # GetPinType is public but opaque; its documented export_text avoids
    # CPF_NotEditorVisible members while retaining the real object reference.
    text = value.get_pin_type().export_text()
    refs = re.findall(r'PinSubCategoryObject=("[^"]+")', text)
    assert len(refs) == 1, ('Missing explicit pin class reference', text)
    return resolve_class_reference(refs[0], owner)


def base_graphs(bp, canonical, directory, label, restore_zero=False):
    result = {}
    for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp)):
        name = graph.get_path_name().split(':', 1)[1]
        assert name not in result
        raw = export(graph, directory, label + '_%03d' % index)
        text = canonical(raw)
        if restore_zero and name == 'FullBody_SkeletalControls':
            text = zero_effector_export(text)
        # UEdGraph.Nodes is the reflected authored array (EdGraph.h:77-79).
        # Exported orphan split-pin helpers are deliberately not in this array.
        members = authored_nodes(graph, raw)
        editor = ue.BlueprintGraphEditor.get_graph_editor(graph)
        message_nodes = {obj.get_name() for obj in list(editor.list_nodes_with_errors()) +
                         list(editor.list_nodes_with_warnings()) + list(editor.list_nodes_with_notes())}
        order = [node.get_name() for node in members]
        assert len(order) == len(set(order))
        blocks = object_blocks(text, set(order))
        records = {}
        for node in members:
            block = blocks[node.get_name()]
            if not re.search(r'^      NodeGuid=', block, re.M):
                # A few referenced state nodes have declaration-only graph
                # exports. Export that actual node directly rather than losing
                # its data or inventing a GUID from the declaration.
                direct = canonical(export(node, directory, label + '_%03d_node_%03d' % (index, len(records))))
                block = '\n'.join('   ' + line for line in direct.splitlines())
            lines = [line for line in re.findall(r'^      (\S.*)$', block, re.M)
                     if not line.startswith(('Begin Object', 'End Object'))]
            children = {}
            for cls, child in re.findall(r'^      Begin Object Class=(\S+) Name="([^"]+)"', block, re.M):
                if not cls.rsplit('.', 1)[-1].endswith('Graph'):
                    children[child] = object_blocks(block, {child})[child]
            record = {'class': node.get_class().get_path_name(), 'lines': lines, 'owned_objects': children,
                      'has_compiler_message': node.get_name() in message_nodes,
                      'error_message': next((line.removeprefix('ErrorMsg=') for line in lines if line.startswith('ErrorMsg=')), '')}
            if name == 'GetMainAnimBPThreadSafe' and node.get_name() == 'K2Node_DynamicCast_0':
                target_class = raw_cast_class(node, raw)
                assert target_class == bp.generated_class(), 'Self cast still references another root class'
                record['resolved_target_type'] = target_class.get_path_name()
            records[node.get_name()] = record
        props = [line for line in re.findall(r'^   (\S.*)$', text, re.M)
                 if not line.startswith(('Begin Object', 'End Object', 'Nodes('))]
        result[name] = {'order': order, 'properties': props, 'nodes': records}
    assert 'FullBody_SkeletalControls' in result
    return result


def compare_base_graphs(expected, actual, report):
    """Only measured Engine duplicate IDs/caches are corresponded, never erased."""
    assert expected.keys() == actual.keys()
    normalized = json.loads(json.dumps(actual))
    guid_pairs, variable_pairs, pin_pairs, caches, reverse = {}, {}, {}, [], {}
    node_count = 0
    def bind(old, new):
        # Original Lyra has reused NodeGuids in several different functions.
        # Node identity is (graph, name); each target GUID must still resolve to
        # exactly its corresponding source GUID, never an arbitrary placeholder.
        assert new not in reverse or reverse[new] == old, ('Conflicting duplicate GUID mapping', old, new)
        reverse[new] = old
    for graph, source in expected.items():
        target = normalized[graph]
        assert source['order'] == target['order'] and source['nodes'].keys() == target['nodes'].keys(), graph
        for name, left in source['nodes'].items():
            right = target['nodes'][name]
            assert left['class'] == right['class']
            if graph == 'GetMainAnimBPThreadSafe' and name == 'K2Node_DynamicCast_0':
                assert left['resolved_target_type'] == SHARED_ANIM_BASE + '.ABP_ItemAnimLayersBase_C'
                assert right['resolved_target_type'] == PISTOL_ANIM_BASE + '.ABP_PistolItemAnimLayersBase_C'
                # Only this self-cast output's friendly identifier changes when
                # the engine reconstructs it for the actual new root class.
                right['lines'] = [line.replace('ABP Pistol Item Anim Layers Base', 'ABP Item Anim Layers Base')
                    if line.startswith('CustomProperties Pin ') and 'PinName="AsABP Pistol Item Anim Layers Base"' in line
                    else line for line in right['lines']]
            node_count += 1
            value = lambda lines, key: [line.removeprefix(key + '=') for line in lines if line.startswith(key + '=')]
            a, b = value(left['lines'], 'NodeGuid'), value(right['lines'], 'NodeGuid')
            assert len(a) == len(b) and len(a) <= 1, (graph, name, 'NodeGuid', a, b)
            if a:
                guid_pairs[graph + ':' + name] = [a[0], b[0]]
                bind(a[0], b[0])
            av = [re.search(r'MemberName="([^"]+)".*MemberGuid=([A-F0-9]{32})', line)
                  for line in left['lines'] if line.startswith('VariableReference=')]
            bv = [re.search(r'MemberName="([^"]+)".*MemberGuid=([A-F0-9]{32})', line)
                  for line in right['lines'] if line.startswith('VariableReference=')]
            assert len(av) == len(bv)
            for aa, bb in zip(av, bv):
                assert bool(aa) == bool(bb)
                if aa:
                    assert aa[1] == bb[1]
                    bind(aa[2], bb[2])
                    if aa[2] != bb[2]:
                        assert aa[1] not in variable_pairs or variable_pairs[aa[1]] == [aa[2], bb[2]]
                        variable_pairs[aa[1]] = [aa[2], bb[2]]
            ap = [line for line in left['lines'] if line.startswith('CustomProperties Pin ')]
            bp = [line for line in right['lines'] if line.startswith('CustomProperties Pin ')]
            assert len(ap) == len(bp)
            for aa, bb in zip(ap, bp):
                an = re.search(r'PinName="([^"]+)"', aa)[1]
                bn = re.search(r'PinName="([^"]+)"', bb)[1]
                assert an == bn
                old, new = re.search(r'PinId=([A-F0-9]{32})', aa)[1], re.search(r'PinId=([A-F0-9]{32})', bb)[1]
                if old != new:
                    self_result = graph == 'GetMainAnimBPThreadSafe' and name == 'K2Node_DynamicCast_0' and an == 'AsABP Item Anim Layers Base'
                    assert (an == 'ErrorTolerance' and all('bHidden=True' in line for line in (aa, bb))) or self_result
                    assert all('LinkedTo=' not in line for line in (aa, bb))
                    assert aa.replace(old, new, 1) == bb
                    bind(old, new)
                    pin_pairs[graph + ':' + name + '.' + an] = [old, new]
            a, b = value(left['lines'], 'GeneratedPropertyName'), value(right['lines'], 'GeneratedPropertyName')
            if a != b:
                assert left['class'] == '/Script/PropertyAccessNode.K2Node_PropertyAccess' and len(a) == len(b) == 1
                caches.append({'node': graph + ':' + name, 'source': a[0], 'target': b[0]})
                right['lines'] = ['GeneratedPropertyName=' + a[0] if line == 'GeneratedPropertyName=' + b[0] else line for line in right['lines']]
            if graph == 'SetUpPivotAnim' and name == 'K2Node_PropertyAccess_0' and 'ErrorType=1' in right['lines'] and 'ErrorType=1' not in left['lines']:
                assert not right['has_compiler_message'] and right['error_message'] == ''
                right['lines'].remove('ErrorType=1')
                report['duplicate_diagnostic_error_type_only'] = graph + ':' + name
            if graph == 'GetMainAnimBPThreadSafe' and name == 'K2Node_DynamicCast_0':
                a, b = value(left['lines'], 'TargetType'), value(right['lines'], 'TargetType')
                assert len(a) == len(b) == 1
                permitted = {'"/Script/Engine.AnimBlueprintGeneratedClass\'ABP_PistolItemAnimLayersBase_C\'"',
                             '"/Script/Engine.AnimBlueprintGeneratedClass\'' + right['resolved_target_type'] + '\'"'}
                assert a[0] in permitted and b[0] in permitted
                right['lines'] = ['TargetType=' + a[0] if line == 'TargetType=' + b[0] else line for line in right['lines']]
                right['resolved_target_type'] = left['resolved_target_type']
    self_pin = 'GetMainAnimBPThreadSafe:K2Node_DynamicCast_0.AsABP Item Anim Layers Base'
    assert len(expected) == 83 and len(variable_pairs) == 54 and len(pin_pairs.keys() - {self_pin}) == 16 and len(caches) == 13, (len(expected), len(variable_pairs), len(pin_pairs), len(caches))
    assert len({values[1] for values in guid_pairs.values()}) == len(guid_pairs)
    actual_text = json.dumps(normalized, sort_keys=True)
    actual_text = re.sub(r'\b[A-F0-9]{32}\b', lambda match: reverse.get(match[0], match[0]), actual_text)
    report['duplicate_metadata_correspondence'] = {'node_count': node_count, 'guid_pairs': guid_pairs,
        'variable_guid_pairs': variable_pairs, 'unlinked_hidden_pin_id_pairs': pin_pairs, 'generated_property_names': caches,
        'basis': 'BlueprintEditorUtils PostDuplicateBlueprint 1701-1746; K2Node_PropertyAccess compilation cache; measured Port_J'}
    equal_text(json.dumps(expected, sort_keys=True), actual_text, 'Dedicated base actual authored nodes/data/pins/links', report)
    return json.loads(actual_text)


def zero_effector_export(text):
    name = 'AnimGraphNode_TwoBoneIK_2'
    block = object_blocks(text, {name})[name]
    old = 'EffectorLocation=(X=-2.000000,Y=-2.500000,Z=-2.500000)'
    assert block.count(old) == 1
    node_lines = [line for line in block.splitlines() if line.startswith('      Node=')]
    assert len(node_lines) == 1 and node_lines[0].count(old + ',') == 1
    # Object export omits this restored zero default. The actual struct's
    # original zero value is independently asserted before/after compilation.
    corrected = block.replace(node_lines[0], node_lines[0].replace(old + ',', '', 1), 1)
    pin_lines = [line for line in corrected.splitlines() if 'PinName="EffectorLocation"' in line]
    assert len(pin_lines) == 1 and pin_lines[0].count('DefaultValue="-2.000000,-2.500000,-2.500000"') == 1
    corrected = corrected.replace(pin_lines[0], pin_lines[0].replace(
        'DefaultValue="-2.000000,-2.500000,-2.500000"', 'DefaultValue="0.000000,0.000000,0.000000"'), 1)
    assert text.count(block) == 1
    return text.replace(block, corrected, 1)


def check_self_cast(bp, external_main=None, adapt_from=None):
    graph = ue.BlueprintEditorLibrary.find_graph(bp, 'GetMainAnimBPThreadSafe')
    raw = reference_graph_text(graph)
    members = {node.get_name(): node for node in authored_nodes(graph, raw)}
    self_cast, main_cast = members['K2Node_DynamicCast_0'], members['K2Node_DynamicCast_2']
    target_class = raw_cast_class(self_cast, raw)
    if adapt_from is not None and target_class != bp.generated_class():
        assert target_class == adapt_from
        editor = ue.BlueprintGraphEditor.get_graph_editor(graph)
        assert editor.retarget_node_class(self_cast, adapt_from, bp.generated_class())
        raw = reference_graph_text(graph)
    assert raw_cast_class(self_cast, raw) == bp.generated_class()
    results = [value for value in ue.BlueprintEditorLibrary.list_all_pins(self_cast)
               if str(value.get_pin_name()).startswith('AsABP ')]
    assert len(results) == 1 and actual_pin_class(results[0], self_cast) == bp.generated_class()
    assert not links(results[0]), 'The original self-cast result is unused'
    main = raw_cast_class(main_cast, raw)
    assert main.get_path_name() == '/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base.ABP_Mannequin_Base_C'
    assert external_main is None or main == external_main
    assert actual_pin_class(pin(main_cast, 'AsABP Mannequin Base'), main_cast) == main
    returns = [node for node in members.values() if node.get_class().get_name() == 'K2Node_FunctionResult']
    assert returns and all(actual_pin_class(pin(node, 'ReturnValue'), node) == main for node in returns)
    return main


def pistol_base(assets, canonical, directory, report, repair, previous=None):
    """One approved root copy; only the reproduced authored hand_l vector changes."""
    source_evidence = read_json(REPORTS / 'SourceCompile_H.json')
    target_evidence = read_json(REPORTS / 'TargetParent_I.json')
    assert source_evidence['source_assets_unchanged'] and target_evidence['asset'] == SHARED_ANIM_BASE
    suffix = ':FullBody_SkeletalControls.AnimGraphNode_TwoBoneIK_2'
    original = [row for row in source_evidence['authored_parent_nodes'] if row['path'].endswith(suffix)]
    current = [row for row in target_evidence['nodes'] if row['path'].endswith(suffix)]
    assert len(original) == len(current) == 1
    source_node, current_node = original[0]['Node'], current[0]['Node']
    old = 'EffectorLocation=(X=-2.000000,Y=-2.500000,Z=-2.500000)'
    new = 'EffectorLocation=(X=0.000000,Y=0.000000,Z=0.000000)'
    assert 'IKBone=(BoneName="hand_l")' in source_node and source_node.count(new) == 1
    assert current_node.count(old) == 1 and current_node.replace(old, new, 1) == source_node
    shared = ue.load_asset(SHARED_ANIM_BASE)
    assert isinstance(shared, ue.Blueprint)
    native_parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(shared)
    assert native_parent.get_path_name().startswith('/Script/'), 'Dedicated copy must remain an independent root AnimBP'
    external_main = check_self_cast(shared)
    graph = ue.BlueprintEditorLibrary.find_graph(shared, 'FullBody_SkeletalControls')
    matches = [obj for obj in ue.ObjectIterator(ue.EdGraphNode)
               if obj.get_outer() == graph and obj.get_name() == 'AnimGraphNode_TwoBoneIK_2']
    assert len(matches) == 1 and matches[0].get_editor_property('Node').export_text() == current_node
    assert not links(pin(matches[0], 'EffectorLocation'))
    assert pin(matches[0], 'EffectorLocation').get_pin_value() == '-2.000000,-2.500000,-2.500000'
    if not repair:
        # Fresh loads omit editor-only GeneratedPropertyName caches. Bind this
        # reload to the exact saved root whose 83 authored graphs were checked
        # in the explicitly reviewed Port_P run, and reuse only its 13 cache-key
        # correspondences for the still-strict runtime CDO comparisons.
        snapshot = read_json(REPORTS / 'Port_P_recovery_snapshot.json')
        proof_path = REPORTS / 'Port_P_apply/report.json'
        proof = read_json(proof_path)
        assert snapshot['reviewed_partial_run'] == 'Port_P_apply'
        assert snapshot['source_report_sha256'] == sha(proof_path)
        saved_sha = proof['dedicated_base_saved_sha256']
        assert snapshot['final_sha256'][PISTOL_ANIM_BASE] == saved_sha == sha(asset_file(PROJECT, PISTOL_ANIM_BASE))
        assert proof['protection_passed'] and len(proof['dedicated_base_authored_graphs']) == 83
        assert sha(asset_file(PROJECT, SHARED_ANIM_BASE)) == proof['dedicated_base_change']['shared_base_sha256']
        cache_names = proof['duplicate_metadata_correspondence']['generated_property_names']
        assert len(cache_names) == len({row['node'] for row in cache_names}) == 13
        base = ue.load_asset(PISTOL_ANIM_BASE)
        assert isinstance(base, ue.Blueprint) and ue.BlueprintEditorLibrary.get_blueprint_parent_class(base) == native_parent
        assert_blueprint_status(base)
        check_self_cast(base, external_main)
        for package in PISTOL_LAYERS:
            assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets[package]) == base.generated_class()
        private_graph = ue.BlueprintEditorLibrary.find_graph(base, 'FullBody_SkeletalControls')
        private_nodes = [obj for obj in ue.ObjectIterator(ue.EdGraphNode)
                         if obj.get_outer() == private_graph and obj.get_name() == 'AnimGraphNode_TwoBoneIK_2']
        assert len(private_nodes) == 1 and private_nodes[0].get_editor_property('Node').export_text() == source_node
        effector = pin(private_nodes[0], 'EffectorLocation')
        assert not links(effector) and tuple(map(float, effector.get_pin_value().split(','))) == (0., 0., 0.)
        report['duplicate_metadata_correspondence'] = proof['duplicate_metadata_correspondence']
        report['dedicated_base_authored_graphs'] = proof['dedicated_base_authored_graphs']
        report['dedicated_base_change'] = proof['dedicated_base_change']
        report['root_reload_basis'] = {'source_run': 'Port_P_apply', 'source_report_sha256': sha(proof_path),
            'unchanged_saved_root_sha256': saved_sha, 'full_authored_graphs_previously_verified': 83,
            'editor_transient_graph_cache_recheck': False, 'runtime_cdo_cache_name_pairs_reused': 13,
            'native_and_pistol_parents_self_and_main_casts_and_original_hand_l_zero_checked': True}
        assert sha(asset_file(PROJECT, PISTOL_ANIM_BASE)) == saved_sha
        return base
    expected = base_graphs(shared, canonical, directory, 'base_shared', restore_zero=True)
    shared_sha = sha(asset_file(PROJECT, SHARED_ANIM_BASE))
    if repair:
        assert all(not asset_file(PROJECT, PISTOL_ANIM_BASE, suffix).exists() for suffix in SIDECARS), 'Existing dedicated base requires an explicitly reviewed recovery; no automatic resume'
        report['checkpoint'] = 'before_dedicated_base_engine_duplicate_partial_save_possible'
        persist(directory / 'report.json', report)
        hud.assert_no_other_editor()
        scoped_log_check(report)
        evidence = read_json(REPORTS / 'DuplicateStatus_K.json')
        assert evidence['existing_base_unchanged'] and evidence['asset_saves'] == 0
        assert len(evidence['compile_status']) == 2 and all(row['compile_result'] and 'BS_UP_TO_DATE:' in row['status'] for row in evidence['compile_status'])
        duplicate_start = log_offset()
        base = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset(
            PISTOL_ANIM_BASE.rsplit('/', 1)[1], PISTOL_ANIM_BASE.rsplit('/', 1)[0], shared)
        duplicate_end = log_offset()
        assert isinstance(base, ue.Blueprint)
        duplicate_errors = error_rows(duplicate_start, duplicate_end)
        known = 'LogBlueprint: Error: [AssetLog] ' + PISTOL_ANIM_BASE + '.ABP_PistolItemAnimLayersBase: [Compiler] Property Access  Accessing an object reference is not thread-safe'
        assert not duplicate_errors or (len(duplicate_errors) == 1 and duplicate_errors[0]['line'].endswith(known)), duplicate_errors
        report['auto_duplicate_log'] = {'start': duplicate_start, 'end': duplicate_end,
            'error': duplicate_errors[0] if duplicate_errors else None,
            'status_after_duplicate': assert_blueprint_status(base), 'basis': 'DuplicateStatus_K.json; BlueprintEditorUtils.cpp:1780,1831,1836'}
        report['dedicated_base_duplicate'] = {
            'package': PISTOL_ANIM_BASE, 'engine_duplicate_can_save': True,
            'file_exists': asset_file(PROJECT, PISTOL_ANIM_BASE).exists(),
            'sha256': sha(asset_file(PROJECT, PISTOL_ANIM_BASE)) if asset_file(PROJECT, PISTOL_ANIM_BASE).exists() else None}
        report['checkpoint'] = 'dedicated_base_duplicated_before_authored_vector_restore'
        persist(directory / 'report.json', report)
        check_self_cast(base, external_main, adapt_from=shared.generated_class())
        report['self_cast_type_adaptation'] = {'node': 'GetMainAnimBPThreadSafe.K2Node_DynamicCast_0',
            'source_actual_class': shared.generated_class().get_path_name(), 'target_actual_class': base.generated_class().get_path_name(),
            'target_and_output_pin_pointer_checked': True, 'external_main_and_return_pointers_preserved': True}
        graph = ue.BlueprintEditorLibrary.find_graph(base, 'FullBody_SkeletalControls')
        matches = [obj for obj in ue.ObjectIterator(ue.EdGraphNode)
                   if obj.get_outer() == graph and obj.get_name() == 'AnimGraphNode_TwoBoneIK_2']
        assert len(matches) == 1
        node = matches[0]
        value = node.get_editor_property('Node')
        assert value.export_text() == current_node
        value.set_editor_property('EffectorLocation', ue.Vector(0., 0., 0.))
        node.set_editor_property('Node', value)
        effector = pin(node, 'EffectorLocation')
        assert not links(effector) and effector.set_pin_value('0.000000,0.000000,0.000000')
        assert node.get_editor_property('Node').export_text() == source_node
        compile_start = log_offset()
        assert ue.BlueprintEditorLibrary.compile_blueprint(base)
        report['auto_duplicate_log']['status_after_explicit_compile'] = assert_blueprint_status(base)
        check_self_cast(base, external_main)
        assert not error_rows(compile_start), 'Dedicated root explicit compile emitted a new Error/ensure/Fatal'
        report['auto_duplicate_transient_compiler_error_recovered'] = bool(duplicate_errors)
        report['process_exit_expectation'] = ('Known preserved automatic-duplicate error may make this commandlet exit 1; complete does not mean exit 0. Fresh verify requires all errors 0 and exit 0.'
            if duplicate_errors else 'No recovered error; normal exit 0 required.')
        scoped_log_check(report)
        report['compile_requested'].append(PISTOL_ANIM_BASE)
        for package in PISTOL_LAYERS:
            assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets[package]) == shared.generated_class()
            reparent_start = log_offset()
            ue.BlueprintEditorLibrary.reparent_blueprint(assets[package], base.generated_class())
            assert_blueprint_status(assets[package])
            scoped_log_check(report, reparent_start)
    else:
        base = ue.load_asset(PISTOL_ANIM_BASE)
        assert isinstance(base, ue.Blueprint)
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(base) == native_parent
    check_self_cast(base, external_main)
    for package in PISTOL_LAYERS:
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(assets[package]) == base.generated_class()
    assert_blueprint_status(base)
    actual = compare_base_graphs(expected, base_graphs(base, canonical, directory, 'base_dedicated'), report)
    if previous is not None:
        assert actual == previous['dedicated_base_authored_graphs']
    assert sha(asset_file(PROJECT, SHARED_ANIM_BASE)) == shared_sha
    report['dedicated_base_authored_graphs'] = actual
    report['dedicated_base_change'] = {'source': SHARED_ANIM_BASE, 'target': PISTOL_ANIM_BASE,
        'native_parent': native_parent.get_path_name(), 'node': suffix, 'bone': 'hand_l',
        'old_effector': [-2., -2.5, -2.5], 'original_effector': [0., 0., 0.],
        'node_and_unlinked_pin_only': True, 'shared_base_sha256': shared_sha}
    return base


def dirty_check(allowed):
    dirty = {package.get_name() for package in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert dirty <= set(allowed) | KNOWN_LOAD_DIRTY, 'Unexpected dirty packages: ' + str(sorted(dirty - set(allowed) - KNOWN_LOAD_DIRTY))
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    return sorted(dirty)


def check_hashes(values, root=None):
    changes = []
    for name, expected in values.items():
        path = (Path(root) / name) if root is not None else Path(name)
        if not path.is_file() or sha(path) != expected:
            changes.append(name)
    assert not changes, 'Protected hash changed: ' + str(changes[:12])


def target_protection(baseline, allowed, reuse):
    current = inventory(PROJECT)
    errors = [name for name, value in baseline['content_inventory'].items() if name not in allowed and current.get(name) != value]
    additions = set(current) - set(baseline['content_inventory'])
    errors.extend(sorted(additions - set(allowed)))
    assert not errors, 'Unapproved content change: ' + str(errors[:15])
    check_hashes(baseline['protected'], PROJECT)
    check_hashes({str(asset_file(PROJECT, reuse[source_package])): value
                  for source_package, value in baseline['reused_assets'].items()})


def dependency_check(packages, forbidden, report):
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = ue.AssetRegistryDependencyOptions(True, True, False, False, False)
    rows = {package: sorted(map(str, registry.get_dependencies(package, options))) for package in packages}
    unresolved = sorted({dependency for values in rows.values() for dependency in values
                         if dependency in forbidden or (dependency.startswith(('/Game/', '/ShooterCore/'))
                         and not ue.EditorAssetLibrary.does_asset_exist(dependency))})
    report['dependencies'] = rows
    assert not unresolved, 'Missing or old package references: ' + str(unresolved[:20])


def reviewed_recovery(mapping, stage_map, report):
    path = REPORTS / 'Port_P_recovery_snapshot.json'
    snapshot = read_json(path)
    source_path = REPORTS / 'Port_P_apply/report.json'
    partial = read_json(source_path)
    checkpoint = 'dedicated_base_original_vector_saved_before_196_rename'
    assert snapshot['reviewed_partial_run'] == 'Port_P_apply' and snapshot['checkpoint'] == checkpoint
    assert snapshot['mapping'] == partial['mapping'] == mapping
    assert snapshot['source_report_sha256'] == sha(source_path)
    assert snapshot['automatic_PIE'] is False and snapshot['new_migrate'] is False
    assert partial['mode'] == 'apply' and partial['complete'] is False and partial['protection_passed']
    assert partial['checkpoint'] == checkpoint and partial['engine_rename_can_save'] is True
    assert partial['asset_saves'] == [PISTOL_ANIM_BASE]
    assert set(partial['comparison_failures']) == {AMMO + ' layout/animations', RETICLE + ' layout/animations'}
    assert set(snapshot['final_sha256']) == set(mapping.values()) | {PISTOL_ANIM_BASE}
    check_hashes({str(asset_file(PROJECT, package)): value for package, value in snapshot['final_sha256'].items()})
    assert snapshot['final_sha256'][PISTOL_ANIM_BASE] == partial['dedicated_base_saved_sha256']
    assert sha(asset_file(PROJECT, CONTROLLER)) == snapshot['controller_before_sha256']
    assert all(not asset_file(PROJECT, package, suffix).exists() for package in stage_map.values() for suffix in SIDECARS)
    report['reviewed_recovery'] = {'source_run': 'Port_P_apply', 'snapshot_sha256': sha(path),
        'source_report_sha256': snapshot['source_report_sha256'], 'final_package_count': 197,
        'controller_before_sha256': snapshot['controller_before_sha256'],
        'historical_duplicate_error_recovered': partial.get('auto_duplicate_transient_compiler_error_recovered', False),
        'new_process_errors_must_be_zero': True}
    return partial


def run_editor():
    global ue, ammo, hud, ACTIVE_LOG
    import unreal as ue
    hud = import_existing('ingame_hud_layout_setup')
    ammo = import_existing('frontend_ammo_guard')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor()
    mode = re.search(r'-DCPistolSetupMode=(migrate|apply|verify)\b', command).group(1)
    recovery_match = re.search(r'-DCPistolSetupRecovery=([^\s"]+)', command)
    recovery = recovery_match is not None
    if recovery:
        assert mode == 'apply' and recovery_match[1] == 'Port_P_apply', 'Only the explicitly reviewed Port_P_apply recovery is permitted'
    run = re.search(r'-DCPistolSetupRun=([A-Za-z0-9_]{1,64})\b', command).group(1)
    root = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert root == (SOURCE if mode == 'migrate' else PROJECT).resolve()
    directory = REPORTS / run
    directory.mkdir(parents=True, exist_ok=True)
    out = directory / 'report.json'
    assert not out.exists(), 'Use a fresh report run id'
    scope, baseline = manifest()
    mapping, reuse = scope['proposed_mapping'], scope['reuse_candidates']
    stage_map = {old: staged(old) for old in mapping}
    assert len(set(stage_map.values())) == 196
    allowed_packages = set(stage_map.values()) | set(mapping.values())
    if mode in ('apply', 'verify'):
        allowed_packages.add(PISTOL_ANIM_BASE)
    allowed_files = {str(asset_file(PROJECT, package, suffix).relative_to(PROJECT)).replace('\\', '/')
                     for package in allowed_packages for suffix in SIDECARS}
    if mode in ('apply', 'verify'):
        allowed_files.update(str(asset_file(PROJECT, CONTROLLER, suffix).relative_to(PROJECT)).replace('\\', '/') for suffix in SIDECARS)
    report = dict(mode=mode, run=run, complete=False, PIE=False, automatic_PIE=False,
                  asset_saves=[], compile_requested=[], mapping=mapping, stage_map=stage_map,
                  approved_extra_packages=[PISTOL_ANIM_BASE], expanded_new_package_count=197,
                  limits='No runtime/visual/physical-input/network/packaging validation.')
    source_before = source_inventory()
    dll = PROJECT / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'
    dll_before = sha(dll)
    canonical = normalizer(scope)
    log_path = directory / 'Editor.log'
    ACTIVE_LOG = log_path
    log_arguments = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(log_arguments) == 1, 'Use one explicit printed log path'
    supplied_log = Path(next(value for value in log_arguments[0] if value).replace('\\', '/'))
    assert supplied_log.is_absolute() and supplied_log.resolve() == log_path.resolve(), 'Use the printed absolute log path'
    try:
        check_hashes(baseline['source_assets'])
        check_hashes(read_json(REPORTS / 'frozen_cpp_config.json')['files'], PROJECT)
        if mode == 'verify' and run == 'Port_U_verify':
            # One reviewed optional-lock stat-cache refresh, not a changed Git
            # entry. Preserve the historical baseline and every other check.
            evidence_path = REPORTS / 'ReviewedIndexCache_Port_U.json'
            assert sha(evidence_path) == '33df1208398e98f2c45e8acfa6166a3eb1431f8628839ea3ad16eb94e6d39a81'
            evidence = read_json(evidence_path)
            reference = re.search(r'-DCPistolSetupReference=([A-Za-z0-9_]{1,64})\b', command).group(1)
            assert reference == evidence['reference'] == 'Port_S_apply'
            assert sha(REPORTS / 'approved_baseline.json') == evidence['approved_baseline_sha256']
            assert baseline['protected']['.git/index'] == evidence['original_index_sha256']
            assert baseline['protected']['.git/HEAD'] == evidence['head_sha256'] == sha(PROJECT / '.git/HEAD')
            assert evidence['index_entries'] == evidence['head_tree_entries'] == 4190
            assert evidence['all_stages_zero'] is True and evidence['staged_changes'] == []
            assert evidence['index_semantic_digest'] == 'def7c6871346168f61a76c1fb56d4c271cbe839b6ce4ae4de3c51692657f2137'
            assert sha(PROJECT / '.git/index') == evidence['reviewed_index_sha256']
            baseline['protected']['.git/index'] = evidence['reviewed_index_sha256']
            report['reviewed_index_cache_change'] = evidence
            report['reviewed_index_cache_evidence_sha256'] = sha(evidence_path)
        target_protection(baseline, allowed_files, reuse)
        assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        settings_text = export(ue.get_default_object(settings), directory, 'settings')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        if mode == 'migrate':
            assert all(not asset_file(PROJECT, p, suffix).exists() for p in allowed_packages for suffix in SIDECARS)
            hud.assert_no_other_editor()
            options = ue.MigrationOptions()
            options.set_editor_property('bPrompt', False)
            options.set_editor_property('bIgnoreDependencies', True)
            options.set_editor_property('AssetConflict', ue.AssetMigrationConflict.SKIP)
            scoped_log_check(report)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping), str(PROJECT / 'Content'), options)
            assert all(asset_file(PROJECT, package).is_file() for package in stage_map.values())
            assert all(not asset_file(PROJECT, package).exists() for package in mapping.values())
            report['asset_saves'] = list(stage_map.values())
            report['staged_sha256'] = {package: sha(asset_file(PROJECT, package)) for package in stage_map.values()}
        else:
            reference = re.search(r'-DCPistolSetupReference=([A-Za-z0-9_]{1,64})\b', command).group(1)
            previous = read_json(REPORTS / reference / 'report.json')
            assert previous['complete'] and previous['mapping'] == mapping
            assert previous['mode'] == ('migrate' if mode == 'apply' else 'apply')
            partial = reviewed_recovery(mapping, stage_map, report) if recovery else None
            if not recovery:
                check_hashes({str(asset_file(PROJECT, package)): digest for package, digest in
                              previous['staged_sha256' if mode == 'apply' else 'final_sha256'].items()})
            assert ('-ini:engine:' in command.lower()) == (mode == 'apply' and not recovery), 'Final-path recovery/verify must have no temporary package redirects'
            if mode == 'apply' and not recovery:
                assert redirected_argument(scope) in command.replace('\\"', '"'), 'Use the exact printed process-only redirects'
                assert all(not asset_file(PROJECT, package).exists() for package in mapping.values())
                registry = ue.AssetRegistryHelpers.get_asset_registry()
                registry.search_all_assets(True)
                options = ue.AssetRegistryDependencyOptions(True, True, False, False, False)
                for package in stage_map.values():
                    outsiders = set(map(str, registry.get_referencers(package, options))) - set(stage_map.values())
                    assert not outsiders, ('Existing referencer prevents scoped rename', package, sorted(outsiders))
            locations = stage_map if mode == 'apply' and not recovery else mapping
            assets = {old: ue.load_asset(package) for old, package in locations.items()}
            assert all(assets.values()), 'New asset load failed'
            dedicated_base = pistol_base(assets, canonical, directory, report, repair=mode == 'apply' and not recovery,
                                         previous=partial if recovery else previous if mode == 'verify' else None)
            # Add the two original animation layers already covered by the source audit.
            check_component(assets[ACTOR], canonical, report, assets, repair=mode == 'apply' and not recovery)
            if mode == 'apply':
                if not recovery:
                    source_actor = read_json(REPORTS / 'SourceInspect_B/report.json')['assets'][ACTOR]
                    source_actor_cdo = text_file(source_actor['cdo_export'])
                    assert re.findall(r'^   bReplicateUsingRegisteredSubObjectList=(\S+)$', source_actor_cdo, re.M) == ['True']
                    actor_cdo = ue.get_default_object(assets[ACTOR].generated_class())
                    if not actor_cdo.get_editor_property('bReplicateUsingRegisteredSubObjectList'):
                        actor_cdo.set_editor_property('bReplicateUsingRegisteredSubObjectList', True)
                        report['actor_registered_subobject_list_restored'] = True
                    compile_new(assets, report)
                    check_component(assets[ACTOR], canonical, report, assets)
                    validate_original(assets, canonical, directory, 'stage', report)
                    dirty_check(allowed_packages)
                    check_hashes(baseline['source_assets'])
                    target_protection(baseline, allowed_files, reuse)
                    report['checkpoint'] = 'original_data_validated_before_engine_rename'
                    persist(out, report)
                    hud.assert_no_other_editor()
                    scoped_log_check(report)
                    assert ue.EditorAssetLibrary.save_loaded_asset(dedicated_base, only_if_is_dirty=False)
                    report['asset_saves'].append(PISTOL_ANIM_BASE)
                    report['dedicated_base_saved_sha256'] = sha(asset_file(PROJECT, PISTOL_ANIM_BASE))
                    report['checkpoint'] = 'dedicated_base_original_vector_saved_before_196_rename'
                    persist(out, report)
                    renames = [ue.AssetRenameData(assets[old], new.rsplit('/', 1)[0], new.rsplit('/', 1)[1])
                               for old, new in mapping.items()]
                    assert ue.AssetToolsHelpers.get_asset_tools().rename_assets(renames), 'Engine rename failed; inspect partial output'
                    report['engine_rename_can_save'] = True
                names = recovery_guards(assets, partial) if recovery else {
                    'ammo': ammo_guard(assets[AMMO]), 'reticle': reticle_guard(assets[RETICLE])}
                report['guard_names'] = names
                compile_new({AMMO: assets[AMMO], RETICLE: assets[RETICLE]}, report)
                validate_original(assets, canonical, directory, 'final', report, names)
                check_component(assets[ACTOR], canonical, report, assets)
                controller = ue.load_asset(CONTROLLER)
                assert isinstance(controller, ue.Blueprint)
                controller_cdo = ue.get_default_object(controller.generated_class())
                assert controller_cdo.get_editor_property('StartingPistol') is None
                assert controller_cdo.get_editor_property('StartingRifle').get_path_name() == RIFLE + '.ID_Rifle_C'
                before_controller = export(controller_cdo, directory, 'controller_before')
                before_graphs = graphs(controller)
                controller_cdo.set_editor_property('StartingPistol', assets[ITEM].generated_class())
                compile_start = log_offset()
                assert ue.BlueprintEditorLibrary.compile_blueprint(controller)
                assert_blueprint_status(controller)
                scoped_log_check(report, compile_start)
                report['compile_requested'].append(CONTROLLER)
                after_controller = export(ue.get_default_object(controller.generated_class()), directory, 'controller_after')
                mask = lambda text: re.sub(r'^   StartingPistol=.*\n', '', text, flags=re.M)
                equal_text(mask(before_controller), mask(after_controller), 'Controller defaults outside StartingPistol', report)
                assert before_graphs == graphs(controller), 'Existing Controller graphs changed'
                report['controller_graphs'] = before_graphs
                report['controller_cdo_after'] = after_controller
                dirty_check(allowed_packages | {CONTROLLER})
                target_protection(baseline, allowed_files, reuse)
                scoped_log_check(report)
                # The recovered root is already saved and hash-bound to Port_P;
                # keep that verified file unchanged rather than resaving it.
                saves = list(assets.items()) + [(CONTROLLER, controller)]
                for old, obj in saves:
                    hud.assert_no_other_editor()
                    assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), old
                    report['asset_saves'].append(obj.get_path_name())
                report['final_sha256'] = {package: sha(asset_file(PROJECT, package))
                                          for package in list(mapping.values()) + [PISTOL_ANIM_BASE]}
                report['controller_sha256'] = sha(asset_file(PROJECT, CONTROLLER))
            else:
                assert not report['compile_requested'] and not report['asset_saves']
                names = previous['guard_names']
                ammo.check_routes(assets[AMMO], names['ammo'])
                check_reticle_guard(assets[RETICLE], names['reticle'])
                validate_original(assets, canonical, directory, 'verify', report, names)
                controller = ue.load_asset(CONTROLLER)
                cdo = ue.get_default_object(controller.generated_class())
                assert cdo.get_editor_property('StartingPistol') == assets[ITEM].generated_class()
                assert cdo.get_editor_property('StartingRifle').get_path_name() == RIFLE + '.ID_Rifle_C'
                assert graphs(controller) == previous['controller_graphs']
                controller_text = export(cdo, directory, 'controller_verify')
                equal_text(item_subobject_order(previous['controller_cdo_after']), item_subobject_order(controller_text),
                           'Saved Controller CDO named subobjects and exact root values', report)
                report['controller_cdo_subobjects_compared_by_name'] = sorted(re.findall(
                    r'^   Begin Object Class=\S+ Name="([^"]+)"', controller_text, re.M))
                assert sha(asset_file(PROJECT, CONTROLLER)) == previous['controller_sha256']
                report['final_sha256'] = previous['final_sha256']
                check_hashes({str(asset_file(PROJECT, package)): value for package, value in previous['final_sha256'].items()})
            # Old staged redirectors may remain at the 196 approved temporary
            # paths. They are not deleted and must never remain final references.
            forbidden = set(stage_map.values()) | set(scope['source_core']) | {old for old, new in reuse.items() if old != new} | {SHARED_ANIM_BASE}
            dependency_check(list(mapping.values()) + [PISTOL_ANIM_BASE, CONTROLLER], forbidden, report)
            tag = ue.GameplayTag()
            assert tag.import_text('(TagName="' + TAG + '")') and ue.GameplayTagLibrary.is_gameplay_tag_valid(tag)
            cue = ue.get_default_object(assets[CORE + 'GCN_Weapon_Pistol_Fire'].generated_class())
            assert cue.get_editor_property('GameplayCueTag').export_text() == '(TagName="' + TAG + '")'
            assert '+GameplayCueNotifyPaths=/Game/DreamCatcher/Lyra/Weapons/Pistol' in (PROJECT / 'Config/DefaultGame.ini').read_text(encoding='utf-8-sig')
        scoped_log_check(report)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            check_hashes(baseline['source_assets'])
            assert source_inventory() == source_before, 'Original source content inventory changed'
            check_hashes(read_json(REPORTS / 'frozen_cpp_config.json')['files'], PROJECT)
            assert sha(dll) == dll_before, 'DLL changed during asset operation'
            target_protection(baseline, allowed_files, reuse)
            report['protection_passed'] = True
        except Exception:
            report['complete'] = False
            report['protection_error'] = traceback.format_exc()
        persist(out, report)
        print('DC_PISTOL_SETUP ' + json.dumps({key: report.get(key) for key in
              ('mode', 'run', 'complete', 'checkpoint', 'error', 'protection_error')}, ensure_ascii=False))
        if not report['complete'] and 'error' not in report:
            raise AssertionError(report.get('protection_error', 'Incomplete pistol setup'))


if __name__ == '__main__':
    if '--commands' in sys.argv:
        recovery = sys.argv[sys.argv.index('--recovery') + 1] if '--recovery' in sys.argv else None
        print_commands(sys.argv[sys.argv.index('--commands') + 1], recovery)
    else:
        run_editor()
