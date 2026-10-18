"""Approved HUD TODO1: move only VerticalBox_0 from (-32,-32) to (-64,-48).

Separate UnrealEditor-Cmd Python commandlet only. Required flags:
-unattended -run=pythonscript -EnablePlugins=UMGToolSet -SCCProvider=None
-DCInGameHUDMode=inspect|apply|verify -DCInGameHUDRun=<unique_id>
-abslog=<project>/Saved/Diagnostics/InGameHUD20261019/<unique_id>/Editor.log
apply references an inspect run; verify references a completed apply run with
-DCInGameHUDReference=<previous_id>. Close other Unreal Editors first.
No C++ build, PIE, Config change, binary copy, bulk save or new game asset.
"""
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import traceback

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/InGameHUD20261019'
ASSET = '/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout'
OBJECT = ASSET + '.W_ShooterHUDLayout'
SLOT = OBJECT + ':WidgetTree.CanvasPanel_0.CanvasPanelSlot_4'
DISK = PROJECT / ('Content/' + ASSET.removeprefix('/Game/') + '.uasset')
POSITIONS = {'before': (-32.0, -32.0), 'after': (-64.0, -48.0)}
GENERATED_GRAPH = OBJECT + ':ExecuteUbergraph_W_ShooterHUDLayout'
GENERATED_ENTRY = GENERATED_GRAPH + '.K2Node_FunctionEntry_0'
COMPILER_GUID_BASIS = {
    'Engine/Source/Runtime/Engine/Classes/Engine/Blueprint.h:559':
        'IntermediateGeneratedGraphs is UPROPERTY(transient, duplicatetransient).',
    'Engine/Source/Editor/KismetCompiler/Private/KismetCompiler.cpp:4760':
        'A full compile clears IntermediateGeneratedGraphs.',
    'Engine/Source/Editor/KismetCompiler/Private/KismetCompiler.cpp:5042':
        'Compiler SourceGraph is added to IntermediateGeneratedGraphs and RF_Transient is set.',
    'reproduced_difference': 'Apply_A: three generated GUIDs differ. Apply_B compile: this exact transient graph declaration/body are absent.',
    'strictly_preserved': 'Authored graph GUIDs/pins, entry NodeGuid, all other pin data and all links.'}
SCHEMA = 1


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def persist(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def assert_no_other_editor():
    # Same read-only Toolhelp process check used by base_damage_setup.py.
    class Entry(ctypes.Structure):
        _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD),
                    ('th32ProcessID', wintypes.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                    ('th32ModuleID', wintypes.DWORD), ('cntThreads', wintypes.DWORD),
                    ('th32ParentProcessID', wintypes.DWORD), ('pcPriClassBase', wintypes.LONG),
                    ('dwFlags', wintypes.DWORD), ('szExeFile', wintypes.WCHAR * 260)]
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    api.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
    api.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = api.CreateToolhelp32Snapshot(2, 0)
    assert handle not in (None, ctypes.c_void_p(-1).value), 'Cannot inspect editor processes'
    others = []
    try:
        entry = Entry()
        entry.dwSize = ctypes.sizeof(entry)
        more = api.Process32FirstW(handle, ctypes.byref(entry))
        assert more, 'Cannot enumerate processes'
        while more:
            if entry.szExeFile.lower().startswith('unrealeditor') and entry.th32ProcessID != os.getpid():
                others.append([entry.th32ProcessID, entry.szExeFile])
            more = api.Process32NextW(handle, ctypes.byref(entry))
    finally:
        api.CloseHandle(handle)
    assert not others, 'Close other Unreal Editors: ' + str(others)


def inventory():
    result = {}
    for name in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for path in (PROJECT / name).rglob('*'):
            if path.is_file() and path != DISK:
                info = path.stat()
                result[str(path.relative_to(PROJECT))] = [info.st_size, info.st_mtime_ns]
    return result


def protected_hashes():
    # Registration assets, Reticle and its ancestors, damage direction and input
    # settings stay untouched; inventory also guards every other existing file.
    names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
             'Content/DreamCatcher/Experiences/B_DC_StageUIExperience.uasset',
             'Content/DreamCatcher/Experiences/ActionSets/LAS_DC_StageHUD.uasset',
             'Content/DreamCatcher/Experiences/ActionSets/LAS_DC_StageNotifications.uasset',
             'Plugins/GameFeatures/DCStageUI/Content/DCStageUI.uasset',
             'Content/DreamCatcher/Lyra/UI/Shared/Foundation/W_OverallUILayout.uasset',
             'Content/DreamCatcher/Lyra/UI/Shared/Foundation/B_LyraUIPolicy.uasset',
             'Content/DreamCatcher/Lyra/UI/Shared/W_WeaponReticleHost.uasset',
             'Content/DreamCatcher/Lyra/Weapons/Rifle/W_Reticle_Rifle.uasset',
             'Content/DreamCatcher/Lyra/Feedback/DamageTaken/Effects/Camera/Damage/NS_Screen_DamageDirection.uasset',
             'Content/DreamCatcher/Lyra/Feedback/DamageTaken/Effects/Camera/Damage/Materials/M_DamageDirection.uasset']
    return {name: sha(PROJECT / name) for name in names}


def clean(allowed=()):
    dirty = {p.get_name() for p in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert dirty <= set(allowed), 'Unexpected dirty content: ' + str(sorted(dirty))
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Unexpected dirty map'


def log_check(path):
    ue.log_flush()
    text = path.read_text(encoding='utf-8-sig', errors='replace')
    errors = [line for line in text.splitlines() if re.search(
        r'\bLog\w+:\s*(?:Error|Fatal):|Ensure condition failed:|=== Handled ensure:', line)]
    assert not errors, 'Engine Error/ensure/Fatal: ' + '\n'.join(errors[:12])


def export(obj, path):
    assert not path.exists()
    task = ue.AssetExportTask()
    task.object = obj
    task.exporter = ue.ObjectExporterT3D()
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    assert ue.Exporter.run_asset_export_task(task), obj.get_path_name()
    raw = path.read_bytes()
    text = raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')
    # Only platform newline differences are normalized; GUIDs, paths, delegates,
    # graph nodes, flags and all numeric data remain part of the comparison.
    return text.replace('\r\n', '\n').replace('\r', '\n')


def path_of(obj):
    return obj.get_path_name() if obj else None


def tree(umg, bp):
    rows = list(umg.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets'))
    assert 1 <= len(rows) <= 96, 'Unexpected widget tree size'
    names = [str(row.get_editor_property('widget_name')) for row in rows]
    assert len(names) == len(set(names))
    return rows, {name: row.get_editor_property('widget') for name, row in zip(names, rows)}


def check_layout(umg, bp, state):
    _, widgets = tree(umg, bp)
    required = {'SafeZone_0', 'Overlay_0', 'CanvasPanel_0', 'VerticalBox_0',
                'ExtensionPoint_Equipment', 'ExtensionPoint_ExtraEquipment',
                'ExtensionPoint_Reticle', 'ExtensionPoint_ModeStatus', 'BW_Healthbar'}
    assert required <= widgets.keys(), 'Expected Lyra layout is missing'
    assert widgets['SafeZone_0'].get_parent() is None
    for name, parent in [('Overlay_0', 'SafeZone_0'), ('CanvasPanel_0', 'Overlay_0'),
                         ('VerticalBox_0', 'CanvasPanel_0'), ('BW_Healthbar', 'CanvasPanel_0'),
                         ('ExtensionPoint_Reticle', 'CanvasPanel_0'), ('ExtensionPoint_ModeStatus', 'CanvasPanel_0'),
                         ('ExtensionPoint_Equipment', 'VerticalBox_0'), ('ExtensionPoint_ExtraEquipment', 'VerticalBox_0')]:
        assert widgets[name].get_parent() == widgets[parent], 'Unexpected parent: ' + name
    slot = widgets['VerticalBox_0'].get_editor_property('slot')
    assert isinstance(slot, ue.CanvasPanelSlot) and slot.get_path_name() == SLOT
    assert (slot.get_position().x, slot.get_position().y) == POSITIONS[state]
    anchors, alignment = slot.get_anchors(), slot.get_alignment()
    assert (anchors.minimum.x, anchors.minimum.y, anchors.maximum.x, anchors.maximum.y) == (1, 1, 1, 1)
    assert (alignment.x, alignment.y) == (1, 1) and slot.get_auto_size()
    health = widgets['BW_Healthbar'].get_editor_property('slot')
    assert health.get_position().y == -72.0, 'Preserve current health position'
    reticle = widgets['ExtensionPoint_Reticle']
    rslot = reticle.get_editor_property('slot')
    ra, rl = rslot.get_anchors(), rslot.get_alignment()
    assert (ra.minimum.x, ra.minimum.y, ra.maximum.x, ra.maximum.y, rl.x, rl.y) == (.5, .5, .5, .5, .5, .5)
    assert rslot.get_auto_size()
    mode = widgets['ExtensionPoint_ModeStatus']
    mslot = mode.get_editor_property('slot')
    ma, ml, mo = mslot.get_anchors(), mslot.get_alignment(), mslot.get_offsets()
    assert (ma.minimum.x, ma.minimum.y, ma.maximum.x, ma.maximum.y, ml.x, ml.y) == (0, 0, 1, 1, 0, 0)
    assert (mo.left, mo.top, mo.right, mo.bottom) == (0, 0, 0, 0)
    assert not mslot.get_auto_size() and mslot.get_z_order() == 20
    assert mode.get_editor_property('EntryBoxType') == ue.DynamicBoxType.OVERLAY
    assert mode.get_editor_property('EntryHorizontalAlignment') == ue.HorizontalAlignment.H_ALIGN_FILL
    assert mode.get_editor_property('EntryVerticalAlignment') == ue.VerticalAlignment.V_ALIGN_FILL
    pattern = list(mode.get_editor_property('SpacingPattern'))
    assert len(pattern) == 1 and (pattern[0].x, pattern[0].y) == (0, 0)
    for name, tag in [('ExtensionPoint_Equipment', 'HUD.Slot.Equipment'),
                      ('ExtensionPoint_ExtraEquipment', 'HUD.Slot.ExtraEquipment'),
                      ('ExtensionPoint_Reticle', 'HUD.Slot.Reticle'), ('ExtensionPoint_ModeStatus', 'HUD.Slot.ModeStatus')]:
        assert widgets[name].get_editor_property('ExtensionPointTag').export_text() == '(TagName="' + tag + '")'
    return slot


def snapshot(umg, bp, directory, label):
    rows, _ = tree(umg, bp)
    result = {'parent_class': path_of(ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp)),
              'hierarchy': [], 'texts': {}}
    result['texts']['blueprint'] = export(bp, directory / (label + '_blueprint.t3d'))
    result['texts']['cdo'] = export(ue.get_default_object(bp.generated_class()), directory / (label + '_cdo.t3d'))
    for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp)):
        result['texts']['graph:' + graph.get_path_name()] = export(graph, directory / (label + '_graph_%02d.t3d' % index))
    for index, row in enumerate(rows):
        widget, parent, slot, host = [row.get_editor_property(name) for name in ('widget', 'parent', 'slot', 'named_slot_host')]
        result['hierarchy'].append(row.export_text())
        assert widget and widget.get_path_name().startswith(OBJECT + ':WidgetTree.')
        result['texts']['widget:' + widget.get_path_name()] = export(widget, directory / (label + '_widget_%02d.t3d' % index))
        if slot:
            result['texts']['slot:' + slot.get_path_name()] = export(slot, directory / (label + '_slot_%02d.t3d' % index))
    result['export_sha256'] = {p.name: sha(p) for p in directory.glob(label + '_*.t3d')}
    return result


def approved_text(text, state):
    """Mask only Left/Top inside the one approved slot's LayoutData export.

    Object stack checks prevent identical values in another slot/property being
    accepted. All other text, including compile-generated data, stays exact.
    """
    stack, output = [], []
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            stack.append('ExportPath=' in stripped and SLOT + "'\"" in stripped)
        elif stripped == 'End Object':
            assert stack, 'Invalid export object nesting'
            stack.pop()
        elif stack and stack[-1] and stripped.startswith('LayoutData='):
            x, y = POSITIONS[state]
            line, nx = re.subn(r'(?<=Offsets=\()Left=' + re.escape('%.6f' % x) + r'(?=,)', 'Left=<approved-x>', line)
            line, ny = re.subn(r'(?<=,)Top=' + re.escape('%.6f' % y) + r'(?=[,)])', 'Top=<approved-y>', line)
            assert nx == ny == 1, 'Unexpected approved slot LayoutData: ' + stripped
        output.append(line)
    assert not stack
    return ''.join(output)


def separate_generated_guids(text):
    """Separate the three observed compiler GUIDs in the full BP export only.

    This exact graph is a compiler intermediate, not the authored EventGraph.
    No object, node, pin property or link is removed. Entry NodeGuid stays exact.
    """
    stack, output, evidence, entry_fields = [], [], [], set()
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            match = re.search(r'ExportPath="[^\']*\'([^\']+)\'"', stripped)
            assert match, 'Missing full export object path'
            stack.append(match.group(1))
        elif stripped == 'End Object':
            assert stack
            stack.pop()
        elif stack and stack[-1] == GENERATED_ENTRY:
            if stripped in ('bIsIntermediateNode=True',
                            'CustomGeneratedFunctionName="ExecuteUbergraph_W_ShooterHUDLayout"',
                            'FunctionReference=(MemberParent="/Script/CoreUObject.Class\'/Script/CoreUObject.Object\'",MemberName="ExecuteUbergraph")'):
                entry_fields.add(stripped)
            if stripped.startswith('CustomProperties Pin '):
                match = re.match(r'CustomProperties Pin \(PinId=([0-9A-F]{32}),PinName="(then|EntryPoint)"(?=,)', stripped)
                assert match and len(entry_fields) == 3, 'Unexpected generated function entry/pin'
                guid, name = match.groups()
                evidence.append({'object': GENERATED_ENTRY, 'field': 'PinId', 'pin': name, 'guid': guid})
                line = line.replace('PinId=' + guid + ',', 'PinId=<compiler-' + name + '>,', 1)
        elif stack and stack[-1] == GENERATED_GRAPH and stripped.startswith('GraphGuid='):
            match = re.fullmatch(r'GraphGuid=([0-9A-F]{32})', stripped)
            assert match, 'Unexpected generated graph GUID'
            evidence.append({'object': GENERATED_GRAPH, 'field': 'GraphGuid', 'guid': match.group(1)})
            line = line.replace(match.group(1), '<compiler-graph>', 1)
        output.append(line)
    assert not stack
    assert not evidence or (len(evidence) == 3 and {(row['field'], row.get('pin')) for row in evidence} == {
        ('PinId', 'then'), ('PinId', 'EntryPoint'), ('GraphGuid', None)}), 'Unexpected compiler intermediate shape'
    return ''.join(output), evidence


def object_blocks(text, wanted):
    lines, stack, result = text.splitlines(keepends=True), [], []
    for index, line in enumerate(lines):
        if line.strip().startswith('Begin Object '):
            match = re.search(r'ExportPath="[^\']*\'([^\']+)\'"', line)
            assert match
            stack.append((match.group(1), index))
        elif line.strip() == 'End Object':
            path, start = stack.pop()
            if path == wanted:
                result.append((start, index + 1, ''.join(lines[start:index + 1])))
    assert not stack
    return result


def separate_generated_graph(text):
    """Exclude only the validated, optional compiler graph; keep raw evidence."""
    canonical, guids = separate_generated_guids(text)
    raw_blocks = object_blocks(text, GENERATED_GRAPH)
    blocks = object_blocks(canonical, GENERATED_GRAPH)
    if not blocks:
        assert not guids
        return text, {'present': False, 'guids': [], 'raw_blocks': [], 'canonical_blocks': []}
    assert len(blocks) == len(raw_blocks) == 2 and len(guids) == 3, 'Expected declaration and body only'
    allowed_paths = {GENERATED_GRAPH, GENERATED_ENTRY, GENERATED_GRAPH + '.EdGraphNode_Comment_0'}
    for _, _, block in blocks:
        paths = re.findall(r'ExportPath="[^\']*\'([^\']+)\'"', block)
        assert len(paths) == 3 and set(paths) == allowed_paths, 'Unexpected intermediate graph children'
    entry = object_blocks(canonical, GENERATED_ENTRY)
    assert len(entry) == 2
    pins = [line.strip() for line in entry[1][2].splitlines() if line.strip().startswith('CustomProperties Pin ')]
    assert len(pins) == 2
    for line, name, category in zip(pins, ('then', 'EntryPoint'), ('exec', 'int')):
        assert 'PinName="' + name + '"' in line and 'Direction="EGPD_Output"' in line
        assert 'PinType.PinCategory="' + category + '"' in line
        assert not any(value in line for value in ('LinkedTo=', 'SubPins=', 'ParentPin=')), 'Generated entry must be unlinked'
    # The compiler copies this layout-only comment. Its complete declaration and
    # body must equal the authored EventGraph comment; authored data stays in output.
    generated_comment = object_blocks(canonical, GENERATED_GRAPH + '.EdGraphNode_Comment_0')
    authored_comment = object_blocks(canonical, OBJECT + ':EventGraph.EdGraphNode_Comment_0')
    assert len(generated_comment) == len(authored_comment) == 2
    assert [b[2].replace(GENERATED_GRAPH, OBJECT + ':EventGraph') for b in generated_comment] == [b[2] for b in authored_comment]
    output, previous = [], 0
    lines = canonical.splitlines(keepends=True)
    for start, end, _ in blocks:
        output.extend(lines[previous:start])
        previous = end
    output.extend(lines[previous:])
    return ''.join(output), {'present': True, 'guids': guids,
                            'raw_blocks': [b[2] for b in raw_blocks],
                            'canonical_blocks': [b[2] for b in blocks]}


def same(before, after, before_state, after_state):
    assert before['parent_class'] == after['parent_class'] and before['hierarchy'] == after['hierarchy'], 'Widget hierarchy changed'
    assert before['texts'].keys() == after['texts'].keys(), 'Export object set changed'
    differences, compiler_evidence = [], {}
    for key in before['texts']:
        old = approved_text(before['texts'][key], before_state)
        new = approved_text(after['texts'][key], after_state)
        if key == 'blueprint':
            old, compiler_evidence['before'] = separate_generated_graph(old)
            new, compiler_evidence['after'] = separate_generated_graph(new)
            if compiler_evidence['before']['present'] and compiler_evidence['after']['present']:
                assert compiler_evidence['before']['canonical_blocks'] == compiler_evidence['after']['canonical_blocks'], 'Generated graph data changed beyond its three GUIDs'
        if old != new:
            differences.append(key)
    assert not differences, 'Unexpected authored or compile output difference: ' + str(differences)
    if before_state != after_state:
        changed = {key for key in before['texts'] if before['texts'][key] != after['texts'][key]}
        assert 'blueprint' in changed and 'slot:' + SLOT in changed, 'Approved edit missing from full exports'
    return {'compiler_guid_basis': COMPILER_GUID_BASIS, 'compiler_generated_guids': compiler_evidence,
            'all_remaining_data_equal': True}


def main():
    command = ue.SystemLibrary.get_command_line()
    def argument(name, choices):
        found = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
        assert len(found) == 1, 'Exactly one -' + name + ' is required'
        return found[0]
    mode = argument('DCInGameHUDMode', 'inspect|apply|verify').lower()
    run = argument('DCInGameHUDRun', '[A-Za-z0-9_]{1,64}')
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    lower = command.lower()
    assert all(flag in lower for flag in ('-unattended', '-run=pythonscript', '-enableplugins=umgtoolset', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    assert_no_other_editor()
    clean()
    directory = REPORTS / run
    log_path = directory / 'Editor.log'
    log_match = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(log_match) == 1 and Path(log_match[0][0] or log_match[0][1]).resolve() == log_path.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert not (directory / 'report.json').exists() and all(p.name == 'Editor.log' for p in directory.iterdir()), 'Use a fresh run directory'
    before_inventory, hashes, before_sha = inventory(), protected_hashes(), sha(DISK)
    report = dict(schema=SCHEMA, asset=ASSET, mode=mode, run=run, pid=os.getpid(), complete=False,
                  saved=[], before_sha256=before_sha, protected_hashes=hashes,
                  cpp_build=False, PIE=False, physical_input_or_visual_verified=False)
    try:
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings
        settings_text = export(ue.get_default_object(settings), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never'], 'SaveOnCompile must already be Never'
        cls = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet')
        assert cls, 'UMGToolSet is required'
        umg = ue.get_default_object(cls)
        bp = ue.load_asset(ASSET)
        assert bp and bp.get_path_name() == OBJECT
        state = 'after' if mode == 'verify' else 'before'
        check_layout(umg, bp, state)
        clean()
        current = snapshot(umg, bp, directory, 'loaded')
        report['loaded'] = current
        if mode != 'inspect':
            reference = argument('DCInGameHUDReference', '[A-Za-z0-9_]{1,64}')
            assert reference != run
            previous = json.loads((REPORTS / reference / 'report.json').read_text(encoding='utf-8'))
            assert previous['schema'] == SCHEMA and previous['asset'] == ASSET and previous['complete']
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['pid'] != os.getpid(), 'A separate process is required'
            assert hashes == previous['protected_hashes'], 'Protected inputs changed since reference'
            assert before_sha == previous['after_sha256'], 'Asset changed since reference'
            report['reference'] = reference
            baseline = previous['loaded'] if mode == 'apply' else previous['baseline']
            report['baseline'] = baseline
            report['loaded_comparison'] = same(baseline, current, 'before', state)
            if mode == 'apply':
                slot = check_layout(umg, bp, 'before')
                original_layout = slot.get_editor_property('LayoutData').export_text()
                slot.set_position(ue.Vector2D(*POSITIONS['after']))
                restored = ue.AnchorData()
                assert restored.import_text(slot.get_editor_property('LayoutData').export_text())
                offsets = restored.get_editor_property('Offsets')
                offsets.set_editor_property('Left', -32.0)
                offsets.set_editor_property('Top', -32.0)
                restored.set_editor_property('Offsets', offsets)
                assert restored.export_text() == original_layout, 'Position setter changed another AnchorData field'
            # Separate only the three reproduced, engine-backed intermediate
            # GUIDs; authored graphs and all other compiler output stay exact.
            assert umg.call_method('CompileWidgetBlueprint', args=(bp,)), 'Blueprint compile failed'
            check_layout(umg, bp, 'after')
            compiled = snapshot(umg, bp, directory, 'compiled')
            report['compiled'] = compiled
            report['compiled_comparison'] = same(baseline, compiled, 'before', 'after')
            clean((ASSET,))
            log_check(log_path)
            assert inventory() == before_inventory and protected_hashes() == hashes
            assert sha(DISK) == before_sha, 'Unexpected implicit asset save'
            if mode == 'apply':
                assert_no_other_editor()
                report['save_intent'] = True
                persist(directory / 'report.json', report)
                assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
                report['saved'].append(ASSET)
                report['save_intent'] = False
                clean()
        else:
            assert not re.search(r'(?i)(?:^|\s)-DCInGameHUDReference=', command), 'Inspect has no reference'
            clean()
        report['after_sha256'] = sha(DISK)
        if mode != 'apply':
            assert report['after_sha256'] == before_sha, 'Read-only mode changed the asset'
        log_check(log_path)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_inventory
        report['protected_hashes_unchanged'] = protected_hashes() == hashes
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        persist(directory / 'report.json', report)
    assert report['complete'], 'Protected inputs changed; inspect the report'
    print('INGAME_HUD_LAYOUT ' + json.dumps({key: report[key] for key in ('mode', 'run', 'complete', 'saved', 'before_sha256', 'after_sha256')}))


if __name__ == '__main__':
    main()
