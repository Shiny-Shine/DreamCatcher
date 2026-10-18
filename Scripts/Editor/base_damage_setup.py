"""Apply/verify the approved role BaseDamage settings after the user's C++ build.

Separate unattended UnrealEditor-Cmd only, with -SCCProvider=None,
-DCBaseDamageMode=apply|verify and -DCBaseDamageRun=<stable journal id>.
Close other Unreal Editors first. This script never builds, runs PIE, alters
Config, creates game assets, or saves anything outside the two approved assets.
Reusing a run id checks its journal; an unexpected disk edit stops the operation.
"""
import ctypes
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import re
import time

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/BaseDamage20261018'
EXPERIENCE = '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience'
RIFLE_GE = '/Game/Weapons/Rifle/GE_Damage_RifleAuto'
TARGETS = (EXPERIENCE, RIFLE_GE)
TAG = 'Gameplay.Damage.IgnoreWeaponBonus'
FIELDS = {'bUseRoleBasedBaseDamage': True, 'PlayerBaseDamage': 0.0, 'EnemyBaseDamage': 12.0}
SCHEMA = 1


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def disk(package):
    assert package in TARGETS
    return PROJECT / 'Content' / (package.removeprefix('/Game/') + '.uasset')


def assert_no_other_editor():
    """Read process names only; refuse concurrent editors rather than stopping any."""
    class ProcessEntry(ctypes.Structure):
        _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD),
                    ('th32ProcessID', wintypes.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                    ('th32ModuleID', wintypes.DWORD), ('cntThreads', wintypes.DWORD),
                    ('th32ParentProcessID', wintypes.DWORD), ('pcPriClassBase', wintypes.LONG),
                    ('dwFlags', wintypes.DWORD), ('szExeFile', wintypes.WCHAR * 260)]
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    api.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = api.CreateToolhelp32Snapshot(2, 0)
    assert handle not in (None, ctypes.c_void_p(-1).value), 'Cannot inspect running editors'
    others = []
    try:
        entry = ProcessEntry()
        entry.dwSize = ctypes.sizeof(entry)
        more = api.Process32FirstW(handle, ctypes.byref(entry))
        assert more, 'Cannot enumerate processes'
        while more:
            if entry.szExeFile.lower().startswith('unrealeditor') and entry.th32ProcessID != os.getpid():
                others.append([entry.th32ProcessID, entry.szExeFile])
            more = api.Process32NextW(handle, ctypes.byref(entry))
    finally:
        api.CloseHandle(handle)
    assert not others, 'Close other Unreal Editors before applying/verifying: ' + str(others)


def inventory():
    """Preserve all existing assets/sidecars, source, config, plugin and save data."""
    result = {}
    allowed = {str(disk(package)) for package in TARGETS}
    for name in ('Content', 'Plugins', 'Source', 'Config', 'Saved/SaveGames'):
        for path in (PROJECT / name).rglob('*'):
            if path.is_file() and str(path) not in allowed:
                info = path.stat()
                result[str(path)] = [info.st_size, info.st_mtime_ns]
    return result


def protected_hashes():
    paths = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
             'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
             'Content/DreamCatcher/Lyra/Weapons/Rifle/B_Rifle.uasset',
             'Content/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss.uasset']
    return {str(PROJECT / name): sha(PROJECT / name) for name in paths}


def normalize(text):
    # Do not discard GUIDs, references, arbitrary tags or numeric properties.
    return '\n'.join(line.rstrip() for line in text.splitlines()).strip()


def export(obj, path):
    assert not path.exists(), str(path)
    task = ue.AssetExportTask()
    task.object = obj
    task.exporter = ue.ObjectExporterT3D()
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    assert ue.Exporter.run_asset_export_task(task), obj.get_path_name()
    raw = path.read_bytes()
    return normalize(raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig'))


def defaults(bp):
    return ue.get_default_object(bp.generated_class())


def graph_snapshot(bp, directory, label):
    return {graph.get_name(): export(graph, directory / (label + '_' + str(index) + '.t3d'))
            for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp))}


def without_properties(text, properties):
    return '\n'.join(line for line in text.splitlines()
                     if not any(line.strip().startswith(name + '=') for name in properties))


def rifle_modifier(cdo):
    assert len(cdo.get_editor_property('Modifiers')) == 0, 'Unexpected direct GE modifiers'
    executions = list(cdo.get_editor_property('Executions'))
    assert len(executions) == 1
    execution = executions[0]
    assert execution.get_editor_property('CalculationClass').get_path_name() == '/Script/DreamCatcher.DCLyraDamageExecution'
    modifiers = list(execution.get_editor_property('CalculationModifiers'))
    assert len(modifiers) == 1
    modifier = modifiers[0]
    captured = modifier.get_editor_property('CapturedAttribute').export_text()
    assert 'AttributeName="BaseDamage"' in captured and '/Script/DreamCatcher.DCCombatSet:BaseDamage' in captured
    assert 'bSnapshot=True' in captured and 'AttributeSource=Target' not in captured
    # UE 5.8 exposes AddBase; Additive is its hidden backwards-compatible alias (both 0).
    assert modifier.get_editor_property('ModifierOp') == ue.GameplayModOp.ADD_BASE
    assert modifier.get_editor_property('AggregatorType') == ue.GameplayEffectScopedModifierAggregatorType.CAPTURED_ATTRIBUTE_BACKED
    expected = ue.GameplayEffectModifierMagnitude()
    assert expected.import_text('(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value=12.000000))')
    assert modifier.get_editor_property('ModifierMagnitude').export_text() == expected.export_text(), 'Expected fixed Rifle bonus 12'
    return executions, execution, modifiers, modifier


def ignore_container(modifier):
    return modifier.get_editor_property('SourceTags').get_editor_property('IgnoreTags')


def has_ignore_tag(modifier):
    text = ignore_container(modifier).export_text()
    names = re.findall(r'TagName="([^"]+)"', text)
    # The approved prior state has no ignore tags. Do not replace other policy.
    assert names in ([], [TAG]), 'Unexpected SourceTags.IgnoreTags: ' + text
    return names == [TAG]


def state_of(bp, package):
    cdo = defaults(bp)
    if package == EXPERIENCE:
        values = {name: cdo.get_editor_property(name) for name in FIELDS}
        assert values['PlayerBaseDamage'] == 0.0 and values['EnemyBaseDamage'] == 12.0, values
        return 'after' if values['bUseRoleBasedBaseDamage'] else 'before'
    return 'after' if has_ignore_tag(rifle_modifier(cdo)[3]) else 'before'


def import_struct_field(value, name, old_text, new_text):
    """Import one field into an independent clone and prove the others survive.

    Nested exports omit defaults differently from standalone struct exports.
    Use Unreal's field parser, then reverse just this field in a fresh copy and
    require the complete original export. Never edit serialized text by regex.
    """
    before = value.export_text()
    assert value.import_text('(' + name + '=' + new_text + ')')
    restored = copy_struct(value, type(value))
    assert restored.import_text('(' + name + '=' + old_text + ')')
    assert restored.export_text() == before, 'Field import changed other data: ' + name


def copy_struct(value, struct_type):
    # get_editor_property can return a view into its owner. Never mutate that view.
    original = value.export_text()
    independent = struct_type()
    assert independent.import_text(original)
    assert independent.export_text() == original
    return independent


def set_values(bp, package):
    cdo = defaults(bp)
    if package == EXPERIENCE:
        for name, value in FIELDS.items():
            cdo.set_editor_property(name, value)
        return
    executions, original_execution, _, original_modifier = rifle_modifier(cdo)
    execution = copy_struct(original_execution, ue.GameplayEffectExecutionDefinition)
    modifier = copy_struct(original_modifier, ue.GameplayEffectExecutionScopedModifierInfo)
    source = copy_struct(modifier.get_editor_property('SourceTags'), ue.GameplayTagRequirements)
    source_before = source.export_text()
    modifier_before = modifier.export_text()
    ignore = copy_struct(ignore_container(modifier), ue.GameplayTagContainer)
    assert not has_ignore_tag(modifier)
    ignore_before = ignore.export_text()
    assert ignore.import_text('(GameplayTags=((TagName="' + TAG + '")))')
    import_struct_field(source, 'IgnoreTags', ignore_before, ignore.export_text())
    import_struct_field(modifier, 'SourceTags', source_before, source.export_text())
    import_struct_field(execution, 'CalculationModifiers', '(' + modifier_before + ')', '(' + modifier.export_text() + ')')
    executions[0] = execution
    cdo.set_editor_property('Executions', executions)


def dirty_names():
    return {package.get_name() for package in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}


def persist(path, journal):
    # JSON diagnostics only. Binary assets are always saved through Unreal.
    path.write_text(json.dumps(journal, ensure_ascii=False, indent=2), encoding='utf-8')


def main():
    command = ue.SystemLibrary.get_command_line()
    mode_match = re.search(r'-DCBaseDamageMode=(apply|verify)\b', command)
    run_match = re.search(r'-DCBaseDamageRun=([A-Za-z0-9_]+)\b', command)
    assert mode_match and run_match, 'Explicit mode and stable run id required'
    mode = mode_match.group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert not ue.SourceControl.is_enabled()
    assert_no_other_editor()
    assert not dirty_names() and not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
    settings_class = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
    assert settings_class
    # UE 5.8 cannot Pythonize this ByteProperty's ESaveOnCompile wrapper. Export
    # the effective CDO instead: ExportObjectInner compares CDO data to itself,
    # and FProperty::ExportText_Direct explicitly emits scalar Data == Delta.
    # Thus a missing field is an error, never an assumption of its default.
    REPORTS.mkdir(parents=True, exist_ok=True)
    settings_text = export(ue.get_default_object(settings_class), REPORTS / (
        'settings_' + mode + '_' + run_match.group(1) + '_' + str(time.time_ns()) + '.t3d'))
    save_on_compile = re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M)
    assert save_on_compile == ['SoC_Never'], 'Effective Save on Compile must be Never: ' + str(save_on_compile)
    native = ue.load_class(None, '/Script/DreamCatcher.DCLyraExperienceDefinition')
    assert native, 'User Development Editor/Win64 build required'
    native_values = {name: ue.get_default_object(native).get_editor_property(name) for name in FIELDS}
    assert native_values == dict(FIELDS, bUseRoleBasedBaseDamage=False), native_values
    tag = ue.GameplayTag()
    assert tag.import_text('(TagName="' + TAG + '")') and ue.GameplayTagLibrary.is_gameplay_tag_valid(tag)
    assert tag.export_text() == '(TagName="' + TAG + '")', 'New native tag is missing'
    directory = REPORTS / ('assets_' + run_match.group(1))
    directory.mkdir(parents=True, exist_ok=True)
    journal_path = directory / 'state.json'
    if journal_path.exists():
        journal = json.loads(journal_path.read_text(encoding='utf-8'))
        assert journal['schema'] == SCHEMA and journal['targets'] == list(TARGETS)
        for package in TARGETS:
            record = journal['assets'][package]
            assert sha(disk(package)) == record['disk_sha256'], 'Disk state differs from journal; inspect before resuming: ' + package
    else:
        assert mode == 'apply', 'Verify requires a completed apply journal'
        journal = {'schema': SCHEMA, 'targets': list(TARGETS), 'assets': {}, 'complete': False}
    attempt = len(list(directory.glob('attempt_*'))) + 1
    attempt_dir = directory / ('attempt_%03d_%s' % (attempt, mode))
    attempt_dir.mkdir(exist_ok=False)
    before_inventory = inventory()
    hashes = protected_hashes()
    report = {'mode': mode, 'complete': False, 'saved': [], 'PIE': False, 'cpp_build': False}
    try:
        blueprints = {package: ue.load_asset(package) for package in TARGETS}
        assert all(blueprints.values())
        # Validate both assets before changing either; loading is not permission to save dependencies.
        for package, bp in blueprints.items():
            state = state_of(bp, package)
            if package not in journal['assets']:
                assert state == 'before', 'Already configured without this run journal: ' + package
                journal['assets'][package] = {'state': state, 'disk_sha256': sha(disk(package))}
            assert state == journal['assets'][package]['state'], package
        persist(journal_path, journal)
        assert not dirty_names(), 'Loading dirtied content; investigate before saving'
        for index, (package, bp) in enumerate(blueprints.items()):
            record = journal['assets'][package]
            prefix = str(index)
            before = export(defaults(bp), attempt_dir / (prefix + '_before.t3d'))
            graphs = graph_snapshot(bp, attempt_dir, prefix + '_graph_before')
            if record['state'] == 'after':
                assert before == record['after_cdo'], 'Saved CDO differs from the apply evidence: ' + package
                continue
            assert mode == 'apply', 'Apply is incomplete: ' + package
            if 'before_cdo' in record:
                assert before == record['before_cdo'], 'Pending asset defaults changed: ' + package
            record['before_cdo'] = before
            original_execution = rifle_modifier(defaults(bp))[0][0].export_text() if package == RIFLE_GE else None
            set_values(bp, package)
            expected_execution = rifle_modifier(defaults(bp))[0][0].export_text() if package == RIFLE_GE else None
            if original_execution is not None:
                old_mod = ue.GameplayEffectExecutionDefinition()
                assert old_mod.import_text(expected_execution)
                modifier = copy_struct(list(old_mod.get_editor_property('CalculationModifiers'))[0], ue.GameplayEffectExecutionScopedModifierInfo)
                source = copy_struct(modifier.get_editor_property('SourceTags'), ue.GameplayTagRequirements)
                source_before = source.export_text()
                modifier_before = modifier.export_text()
                import_struct_field(source, 'IgnoreTags', source.get_editor_property('IgnoreTags').export_text(),
                                    ue.GameplayTagContainer().export_text())
                import_struct_field(modifier, 'SourceTags', source_before, source.export_text())
                import_struct_field(old_mod, 'CalculationModifiers', '(' + modifier_before + ')', '(' + modifier.export_text() + ')')
                assert old_mod.export_text() == original_execution, 'GE changed beyond the approved ignore tag'
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp), package
            after = export(defaults(bp), attempt_dir / (prefix + '_after.t3d'))
            allowed = FIELDS.keys() if package == EXPERIENCE else ('Executions',)
            assert without_properties(before, allowed) == without_properties(after, allowed), 'Other CDO data changed: ' + package
            assert graph_snapshot(bp, attempt_dir, prefix + '_graph_after') == graphs, 'Graph changed: ' + package
            assert state_of(bp, package) == 'after'
            if expected_execution is not None:
                assert rifle_modifier(defaults(bp))[0][0].export_text() == expected_execution
            assert dirty_names() <= set(TARGETS) and not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
            assert_no_other_editor()
            assert inventory() == before_inventory, 'Protected disk files changed before save'
            assert protected_hashes() == hashes
            record['after_cdo'] = after
            record['save_intent'] = True
            persist(journal_path, journal)
            assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False), package
            record.update(state='after', disk_sha256=sha(disk(package)), save_intent=False)
            report['saved'].append(package)
            persist(journal_path, journal)
        journal['complete'] = all(record['state'] == 'after' for record in journal['assets'].values())
        assert journal['complete']
        persist(journal_path, journal)
        report['complete'] = True
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_inventory
        report['protected_hashes_unchanged'] = protected_hashes() == hashes
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        persist(attempt_dir / 'report.json', report)
        print('BASE_DAMAGE_SETUP ' + json.dumps(report, ensure_ascii=False))
    assert report['complete'], 'Protection/verification failed; inspect this run before continuing'


if __name__ == '__main__':
    main()
