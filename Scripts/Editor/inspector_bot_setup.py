"""Approved Inspector bot preflight and fixed 307-package integration.

Outside Unreal, --commands PREFIX prints commands without launching processes.
Inside a separate unattended Python commandlet, inspect exports current object
fields and follows animation/Inventory/Equipment/Ability references. It does not
compile Blueprints, duplicate assets, change source data, save packages or run PIE.
The original memory phase creates only two transient rigs and a transient retargeter. It
checks definitions/mappings, not evaluated poses, and requires a successful
inspect report from the same frozen source. Integration requires a separately
reviewed successful user C++ build. Batch retarget may internally save only its
explicit new destinations. No PIE, packaging, source consolidation or broad save.
"""
import csv
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import sys
import subprocess
import traceback
from collections import Counter, deque
from functools import lru_cache

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/InspectorBot20261023/Preflight'
SCRIPT = P / 'Scripts/Editor/inspector_bot_setup.py'
BASELINE = R / 'baseline.json'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
TOPS = ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
DESTINATION = '/Game/DreamCatcher/AI/Inspector'
INTEGRATION = P / 'Saved/Diagnostics/InspectorBot20261023/Integration'
AUTHOR_BASELINE = INTEGRATION / 'authoring-baseline.json'
CPP_FROZEN = INTEGRATION / 'cpp-freeze-review.json'
USER_BUILD = INTEGRATION / 'user-build-success.json'
MANIFEST = R / 'Validated_D/existing-asset-copy-plan.csv'
MANIFEST_SHA = '9ac4dbdb12c5ae21d3f595849c0dd1747c7ce6af6642a96036f51d1fc11332cf'
RIGS = {'source': DESTINATION + '/Rig/IK_DC_Inspector_Source',
        'target': DESTINATION + '/Rig/IK_DC_Inspector_Target',
        'retargeter': DESTINATION + '/Rig/RTG_DC_Manny_Inspector'}
EXPERIENCE_FILE = 'Content/DreamCatcher/Experiences/B_DC_StageUIExperience.uasset'
DLL = 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'
ITEM_LAYER_COPY_SOURCE = '/Game/Characters/Heroes/Mannequin/Animations/LinkedLayers/ABP_ItemAnimLayersBase'
ITEM_LAYER_COPY_TARGET = DESTINATION + '/Animation/ABP_ItemAnimLayersBase'
ITEM_LAYER_COPY_ERROR = ('LogBlueprint: Error: [AssetLog] ' + ITEM_LAYER_COPY_TARGET
    + '.ABP_ItemAnimLayersBase: [Compiler] Property Access  Accessing an object reference is not thread-safe')
ITEM_LAYER_LOG_PREFIX = r'\[\d{4}\.\d{2}\.\d{2}-\d{2}\.\d{2}\.\d{2}:\d{3}\]\[\s*\d+\]'
INSPECTOR_ANIM_BLUEPRINT_ORDER = ('ABP_Mannequin_Base', 'ABP_ItemAnimLayersBase', 'ABP_RifleAnimLayers',
    'ABP_RifleAnimLayers_Feminine', 'ABP_UnarmedAnimLayers', 'ABP_UnarmedAnimLayers_Feminine')
ROOTS = {
    'experience': '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience',
    'pawn_data': '/Game/DreamCatcher/Data/Pawns/DA_DC_LyraAIPawn',
    'pawn': '/Game/DreamCatcher/Blueprints/Player/BP_DC_LyraInventoryPawn',
    'source_anim': '/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base',
    'source_mesh': '/Game/Characters/Heroes/Mannequin/Meshes/SKM_Manny',
    'target_mesh': '/Game/DreamCatcher/Main_Charcher/Inspector/Inspector/Inspector',
}
ANIMATION_CLASSES = {'AnimBlueprint', 'AnimSequence', 'AnimMontage', 'AnimComposite',
    'BlendSpace', 'BlendSpace1D', 'AimOffsetBlendSpace', 'AimOffsetBlendSpace1D', 'PoseAsset'}
TRAVERSE_NATIVE = {'DCInventoryItemDefinition': 'inventory_item',
    'DCLyraEquipmentDefinition': 'equipment_definition',
    'DCLyraWeaponInstance': 'weapon_instance', 'DCGameplayAbility': 'gameplay_ability',
    'AnimNotify': 'animation_notify', 'AnimNotifyState': 'animation_notify'}
REF = re.compile(r'/(?:(?:Game|Engine|Script)|[A-Za-z][A-Za-z0-9_]*)/[^\s\x00\"\'(),;<>]+')
RUNTIME_FIELDS = re.compile(r'InitialInventoryItems|EquipmentDefinition|InstanceType|'
    r'EquippedAnimSet|UneuippedAnimSet|AbilitySets|GrantedGameplayAbilities|'
    r'Actors?ToSpawn|AnimClass|ParentClass|Montage|AnimBlueprint|AnimationAsset|'
    r'ControlRig|BlendProfile|BlendMask|FullBody|ItemAnimLayers', re.I)


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def persist(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


@lru_cache(maxsize=None)
def module(name):
    spec = importlib.util.spec_from_file_location('inspector_preflight_' + name,
                                                SCRIPT.parent / (name + '.py'))
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def inventory():
    values = {}
    for top in TOPS:
        for path in (P / top).rglob('*'):
            if path.is_file() and path.resolve() != SCRIPT.resolve():
                stat = path.stat()
                values[path.relative_to(P).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    return values


def protection(baseline):
    if CPP_FROZEN.exists():
        return integration_protection(False)
    expected = {k.replace('\\', '/'): v for k, v in baseline['inventory'].items()
                if k.replace('\\', '/') != SCRIPT.relative_to(P).as_posix()}
    current = inventory()
    inventory_changed = sorted(k for k in set(expected) | set(current)
                               if expected.get(k) != current.get(k))
    changed = []
    for name, value in baseline['files'].items():
        if Path(name.replace('\\', '/')).as_posix() == SCRIPT.relative_to(P).as_posix():
            continue
        path = P / name
        if not path.is_file() or sha(path) != value:
            changed.append(name)
    result = {'hash_count': len(baseline['files']), 'hash_changes': changed,
              'inventory_count': len(expected), 'inventory_changes': inventory_changed}
    assert not changed and not inventory_changed, 'Protected files changed: ' + str(result)
    return result


def commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,20}', prefix)
    result = {}
    for mode in ('inspect', 'memory'):
        run = prefix + '_' + mode
        folder = R / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-run=pythonscript',
            '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4', '-SCCProvider=None',
            '-nullrhi', '-nosound', '-DCInspectorMode=' + mode, '-DCInspectorRun=' + run,
            '-DCInspectorSHA=' + sha(SCRIPT), '-abslog=' + (folder / 'Editor.log').as_posix()]
        if mode == 'memory':
            args.append('-DCInspectorReference=' + prefix + '_inspect')
        result[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': subprocess.list2cmdline(args)}
    for mode in ('prepare', 'verify', 'activate', 'verify_activation'):
        run = prefix + '_' + mode
        folder = INTEGRATION / run
        args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(),
            "-ExecCmds=py __import__('runpy').run_path('" + SCRIPT.as_posix() + "')['main']()", '-unattended', '-nop4',
            '-SCCProvider=None', '-nullrhi', '-nosound', '-NoSplash', '-NoAutoSave',
            '-DCInspectorBotAssets', '-DCInspectorBotRun=' + run,
            '-DCInspectorMode=' + mode, '-DCInspectorRun=' + run,
            '-DCInspectorSHA=' + sha(SCRIPT), '-DCInspectorPrefix=' + prefix,
            '-abslog=' + (folder / 'Editor.log').as_posix()]
        result[mode] = {'args': args, 'create_log_directory_first': folder.as_posix(),
            'windows_command_line': subprocess.list2cmdline(args),
            'requires_user_build_receipt': USER_BUILD.as_posix(),
            'regular_editor_world_required': True}
    pose_run = prefix + '_saved_pose'
    pose_dir = P / 'Saved/Diagnostics/InspectorBot20261023/Pose' / pose_run
    args = [ENGINE.as_posix(), (P / 'DreamCatcher.uproject').as_posix(), '-unattended',
        '-nop4', '-SCCProvider=None', '-nullrhi', '-nosound', '-NoSplash',
        '-DCInspectorRetargetPose', '-DCInspectorRetargetPrivate', '-DCInspectorPrivateSavedAssets',
        '-DCInspectorPoseRun=' + pose_run,
        '-ExecCmds=Automation RunTests DreamCatcher.Editor.InspectorRetargetPose',
        '-TestExit=Automation Test Queue Empty',
        '-ReportExportPath=' + (pose_dir / 'Automation').as_posix(),
        '-abslog=' + (pose_dir / 'Editor.log').as_posix()]
    result['saved_pose'] = {'args': args, 'create_log_directory_first': pose_dir.as_posix(),
        'windows_command_line': subprocess.list2cmdline(args),
        'run_only_after_successful_fresh_verify': prefix + '_verify'}
    print(json.dumps({'launches_processes': False, 'asset_saves': 0,
        'blueprint_compiles_requested': 0, 'PIE': False, 'cpp_build': False,
        'retargeted_pose_verified': False,
        'integration_execution_order': ['prepare', 'verify', 'saved_pose', 'activate', 'verify_activation'],
        'commands': result},
        ensure_ascii=False, indent=2))


def approved_rows():
    assert sha(MANIFEST) == MANIFEST_SHA, 'Approved 304-row manifest changed'
    with MANIFEST.open(encoding='utf-8-sig', newline='') as stream:
        rows = list(csv.DictReader(stream))
    assert len(rows) == 304
    assert len({r['source'] for r in rows}) == 304
    assert len({r['target'].casefold() for r in rows}) == 304
    assert all(r['source'].startswith('/Game/') and
               r['target'].startswith(DESTINATION + '/') and '..' not in r['target'] for r in rows)
    return rows


def package_disk(package_name):
    assert package_name.startswith('/Game/')
    return P / ('Content/' + package_name[6:] + '.uasset')


@lru_cache(maxsize=1)
def new_packages():
    return frozenset({row['target'] for row in approved_rows()} | set(RIGS.values()))


def new_file(name):
    return name in new_file_names()


@lru_cache(maxsize=1)
def new_file_names():
    return frozenset('Content/' + package_name[6:] + suffix for package_name in new_packages()
                     for suffix in ('.uasset', '.uexp', '.ubulk', '.m.ubulk'))


def valid_build_file(name):
    assert '\\' not in name and not Path(name).is_absolute() and '..' not in Path(name).parts
    assert Path(name).suffix.lower() not in ('.uasset', '.umap', '.uexp', '.ubulk', '.uptnl')
    return bool(re.fullmatch(r'Binaries/[^:]+|Plugins/(?:[^/]+/)+(?:Binaries|Intermediate/Build)/[^:]+', name))


def integration_protection(allow_experience):
    assert sha(MANIFEST) == MANIFEST_SHA
    baseline, frozen, build = read(AUTHOR_BASELINE), read(CPP_FROZEN), read(USER_BUILD)
    scope = set(baseline['approved_source_scope'])
    script_name = SCRIPT.relative_to(P).as_posix()
    assert scope == set(frozen['files']) | {script_name} and len(frozen['files']) == 8
    assert baseline['new_asset_candidate_count'] == 307 and baseline['automatic_PIE'] is False
    assert baseline['existing_asset_write_scope'] == {ROOTS['experience']: ['BotPawnData']}
    assert build['user_reported_success'] is True and build['cpp_freeze_sha256'] == sha(CPP_FROZEN)
    files, stats = build['build_files'], build['build_inventory']
    assert DLL in files and files[DLL] is not None
    for name in set(files) | set(stats):
        assert valid_build_file(name), 'Invalid build receipt exception: ' + name
        path = P / name
        if name in files:
            assert not path.exists() if files[name] is None else path.is_file() and sha(path) == files[name], name
        if name in stats:
            assert not path.exists() if stats[name] is None else path.is_file() and [path.stat().st_size, path.stat().st_mtime_ns] == stats[name], name
    assert all(sha(P / name) == value for name, value in frozen['files'].items()), 'Frozen C++ changed'
    hashes = {}
    for name, value in baseline['original_protected_files'].items():
        if name in scope or name in files or allow_experience and name == EXPERIENCE_FILE:
            continue
        assert (P / name).is_file() and sha(P / name) == value, 'Original file changed: ' + name
        hashes[name] = value
    expected = {k.replace('\\', '/'): v for k, v in baseline['original_inventory'].items()}
    current = inventory()
    excluded = scope | set(stats) | ({EXPERIENCE_FILE} if allow_experience else set())
    before = {k: v for k, v in expected.items() if k not in excluded}
    after = {k: v for k, v in current.items() if k not in excluded and not new_file(k)}
    assert before == after, 'Original inventory changed outside frozen sources and exact build receipt'
    hashes.update(frozen['files'])
    hashes[script_name] = sha(SCRIPT)
    hashes.update({k: v for k, v in files.items() if v is not None})
    return {'author_baseline_sha256': sha(AUTHOR_BASELINE), 'cpp_freeze_sha256': sha(CPP_FROZEN),
        'user_build_receipt_sha256': sha(USER_BUILD), 'hashes': hashes,
        'original_inventory_unchanged': True, 'protected_hash_count': len(hashes)}


def scoped_dirty(allow_experience=False):
    allowed = new_packages() | ({ROOTS['experience']} if allow_experience else set())
    dirty = {p.get_name() for p in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert dirty <= allowed, 'Unexpected original dirty packages; do not save: ' + str(sorted(dirty - allowed))
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'World saving is forbidden'
    return sorted(dirty)


def private_asset_catalog(require_all=False):
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    assets = list(registry.get_assets_by_path(DESTINATION, recursive=True, include_only_on_disk_assets=False))
    names = {str(row.package_name) for row in assets}
    assert names <= new_packages(), 'Unexpected Inspector asset packages: ' + str(sorted(names - new_packages()))
    if require_all:
        assert names == new_packages(), 'Missing approved Inspector packages: ' + str(sorted(new_packages() - names))
    return sorted(names)


def disk_hashes():
    return {name: sha(P / name) for name in new_file_names() if (P / name).is_file()}


def load_exact(package_name, expected_class=None):
    obj = ue.load_asset(package_name)
    assert obj and obj.get_path_name() == package_name + '.' + package_name.rsplit('/', 1)[-1], package_name
    if expected_class:
        assert obj.get_class().get_path_name() == expected_class, package_name
    return obj


def integration_log_events(raw):
    events, offset = [], 0
    for line in raw.splitlines(keepends=True):
        text = line.decode('utf-8-sig', errors='replace').rstrip('\r\n')
        if re.search(r'\bLog\w+:\s*(?:Error|Fatal):|Ensure condition failed:|=== Handled ensure:|Fatal error:|Assertion failed:', text, re.I):
            events.append({'start': offset, 'end': offset + len(line), 'text': text,
                'sha256': hashlib.sha256(line).hexdigest()})
        offset += len(line)
    return events


def integration_log_check(path, report):
    ue.log_flush()
    raw = path.read_bytes()
    events = integration_log_events(raw)
    recovery = report.get('item_layer_copy_recovery')
    if recovery is not None:
        assert report['mode'] == 'prepare' and recovery['validated'] is True
        assert recovery['target'] == ITEM_LAYER_COPY_TARGET and recovery['log_path'] == path.resolve().as_posix()
        assert recovery['compile_return'] is True and recovery['compile_return_python_type'] == 'bool'
        assert recovery['source_state_unchanged'] is True and recovery['source_file_protection_passed'] is True
        assert not recovery['new_compile_events']
        length = recovery['validated_log_prefix_length']
        assert len(raw) >= length and hashlib.sha256(raw[:length]).hexdigest() == recovery['validated_log_prefix_sha256'], 'Verified recovery log prefix changed or shrank'
        event = recovery['event']
        assert re.fullmatch(ITEM_LAYER_LOG_PREFIX + re.escape(ITEM_LAYER_COPY_ERROR), event['text'])
        assert hashlib.sha256(raw[event['start']:event['end']]).hexdigest() == event['sha256']
        assert events.count(event) == 1, 'Verified physical copy-error line changed or disappeared'
        events = [entry for entry in events if entry != event]
    assert not events, 'Engine Error/ensure/Fatal: ' + '\n'.join(entry['text'] for entry in events[:12])
    return {'byte_length': len(raw), 'sha256': hashlib.sha256(raw).hexdigest()}


def recover_item_layer_copy(source, copied, row, directory, report, item, before_log):
    attempt = item['item_layer_recovery_attempt'] = {'validated': False, 'compile_calls_requested': 0,
        'source': row['source'], 'target': row['target'], 'asset_saves': 0}
    assert report['mode'] == 'prepare' and 'item_layer_copy_recovery' not in report, 'Only one prepare copy recovery is allowed'
    assert row['source'] == ITEM_LAYER_COPY_SOURCE and row['target'] == ITEM_LAYER_COPY_TARGET
    assert row['class'] == '/Script/Engine.AnimBlueprint' and copied != source
    assert source.get_path_name() == ITEM_LAYER_COPY_SOURCE + '.ABP_ItemAnimLayersBase'
    assert copied.get_path_name() == ITEM_LAYER_COPY_TARGET + '.ABP_ItemAnimLayersBase'
    log_path = directory / 'Editor.log'
    ue.log_flush()
    raw = log_path.read_bytes()
    offset = before_log['byte_length']
    assert len(raw) >= offset and hashlib.sha256(raw[:offset]).hexdigest() == before_log['sha256'], 'Copy log prefix changed'
    events = integration_log_events(raw)
    assert len(events) == 1 and events[0]['start'] >= offset, 'Copy recovery requires exactly one new error line'
    assert re.fullmatch(ITEM_LAYER_LOG_PREFIX + re.escape(ITEM_LAYER_COPY_ERROR), events[0]['text']), 'Copy error differs from the verified ItemBase case'
    attempt.update({'log_path': log_path.resolve().as_posix(), 'before_duplicate_log': before_log,
        'event': events[0], 'duplicate_log_end': len(raw)})
    def state(obj):
        return {'class': obj.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(obj)),
            'skeleton': value_summary(obj.get_editor_property('TargetSkeleton')),
            'status': value_summary(obj.get_editor_property('Status'))}
    attempt['before'] = {'source': state(source), 'target': state(copied)}
    assert attempt['before']['source'] == attempt['before']['target']
    assert {key: attempt['before']['source'][key] for key in ('class', 'parent', 'skeleton')} == item['source_state']
    assert attempt['before']['source']['skeleton'] == '/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin'
    up_to_date = ue.BlueprintStatus.BS_UP_TO_DATE
    assert source.get_editor_property('Status') == up_to_date and copied.get_editor_property('Status') == up_to_date
    attempt['dirty_packages_before'] = scoped_dirty()
    ue.log_flush()
    compile_prefix = log_path.read_bytes()
    attempt['compile_log_offset'] = len(compile_prefix)
    attempt['compile_calls_requested'] = 1
    compiled = ue.BlueprintEditorLibrary.compile_blueprint(copied)
    attempt['compile_return'] = value_summary(compiled)
    attempt['compile_return_python_type'] = type(compiled).__name__
    ue.log_flush()
    after_log = log_path.read_bytes()
    assert after_log.startswith(compile_prefix), 'Recompile log prefix changed'
    interval = after_log[len(compile_prefix):]
    attempt['compile_log_end'] = len(after_log)
    attempt['new_compile_interval_sha256'] = hashlib.sha256(interval).hexdigest()
    attempt['new_compile_events'] = integration_log_events(interval)
    attempt['after'] = {'source': state(source), 'target': state(copied)}
    attempt['source_state_unchanged'] = attempt['after']['source'] == attempt['before']['source']
    assert compiled is True and type(compiled) is bool and not attempt['new_compile_events']
    assert attempt['source_state_unchanged'] and attempt['after']['target'] == attempt['before']['target']
    assert source.get_editor_property('Status') == up_to_date and copied.get_editor_property('Status') == up_to_date
    attempt['dirty_packages_after'] = scoped_dirty()
    proof = integration_protection(False)
    attempt['source_file_protection_passed'] = True
    attempt['source_file_protection'] = {key: proof[key] for key in (
        'author_baseline_sha256', 'cpp_freeze_sha256', 'user_build_receipt_sha256')}
    # Validate the entire unchanged log, excluding only this proved byte range.
    ue.log_flush()
    validated_log = log_path.read_bytes()
    assert validated_log.startswith(after_log), 'Verified recovery log is not append-only'
    attempt['validated_log_prefix_length'] = len(validated_log)
    attempt['validated_log_prefix_sha256'] = hashlib.sha256(validated_log).hexdigest()
    validated = dict(attempt, validated=True)
    integration_log_check(log_path, dict(report, item_layer_copy_recovery=validated))
    attempt['validated'] = True
    report['item_layer_copy_recovery'] = attempt


def native_phase(world, directory, phase, report):
    assert phase in ('Prepare', 'RepairFootPlantCopy', 'Adapt', 'Verify')
    receipt = directory / ('native_' + phase + '.csv')
    assert not receipt.exists(), 'Preserve previous native phase receipts'
    ue.SystemLibrary.execute_console_command(world, 'DC.InspectorBot.' + phase)
    ue.log_flush()
    assert receipt.is_file(), 'Native command did not finish synchronously: ' + phase
    with receipt.open(encoding='utf-8-sig', newline='') as stream:
        rows = list(csv.reader(stream))
    assert rows[0] == ['key', 'value']
    values = dict(rows[1:])
    report.setdefault('native', {})[phase] = {'receipt': receipt.name, 'sha256': sha(receipt), 'values': values}
    if phase == 'Prepare' and values.get('success') != 'true':
        try:
            path = directory / 'failed_prepare_retargeter.t3d'
            export(load_exact(RIGS['retargeter'], '/Script/IKRig.IKRetargeter'), path)
            report['native'][phase]['failed_retargeter_export'] = {'path': path.as_posix(), 'sha256': sha(path)}
        except Exception:
            # Keep the native failure as the primary error even if evidence
            # export fails; protection and Editor shutdown still run in finally.
            report['native'][phase]['failed_retargeter_export_error'] = traceback.format_exc()
    assert values.get('phase') == phase and values.get('success') == 'true', values.get('error') or phase
    assert values.get('asset_saves') == '0' and values.get('PIE') == 'false'
    scoped_dirty()
    integration_log_check(directory / 'Editor.log', report)


def data_snapshot(obj, fields):
    return {field: value_summary(obj.get_editor_property(field)) for field in fields}


def montage_segment_metadata(obj, slot_tracks):
    """Separate only FAnimSegment's editor length cache in the known slot path."""
    tolerance = 0.00000051  # ObjectExporterT3D writes this float with six decimals.
    private = obj.get_path_name().startswith(DESTINATION + '/')
    evidence = {'montage': obj.get_path_name(), 'private_target': private, 'export_roundoff_seconds': tolerance,
        'policy': 'FAnimSegment.CachedPlayLength is editor-only/default0; source stale cache is evidence, not authoring. '
            'Private nonzero cache must match referenced AnimationAsset.get_play_length. '
            'IsPlayLengthOutOfDate is editor C++ only and was not executed; no source setters.',
        'segments': [], 'validation_errors': []}

    def parts(value):
        assert value.startswith('(') and value.endswith(')'), 'Expected montage slot/track/segment tuple'
        body = value[1:-1]
        if not body:
            return []
        result, start, depth, quoted, escaped = [], 0, 0, False, False
        for index, char in enumerate(body):
            if escaped:
                escaped = False
            elif quoted and char == '\\':
                escaped = True
            elif char == '"':
                quoted = not quoted
            elif not quoted:
                if char == '(':
                    depth += 1
                elif char == ')':
                    depth -= 1
                    assert depth >= 0, 'Unbalanced montage segment tuple'
                elif char == ',' and depth == 0:
                    result.append(body[start:index])
                    start = index + 1
        assert depth == 0 and not quoted and not escaped
        result.append(body[start:])
        assert all(result), 'Empty montage tuple token'
        return result

    normalized = []
    for slot_index, slot in enumerate(slot_tracks):
        slot_parts = parts(slot)
        tracks = [index for index, value in enumerate(slot_parts) if value.startswith('AnimTrack=')]
        assert len(tracks) <= 1
        for track_index in tracks:
            track_parts = parts(slot_parts[track_index][len('AnimTrack='):])
            arrays = [index for index, value in enumerate(track_parts) if value.startswith('AnimSegments=')]
            assert len(arrays) <= 1
            for array_index in arrays:
                segments = parts(track_parts[array_index][len('AnimSegments='):])
                semantic_segments = []
                for segment_index, segment in enumerate(segments):
                    segment_parts = parts(segment)
                    fields = [re.fullmatch(r'([A-Za-z0-9_]+)=(.*)', value, re.S) for value in segment_parts]
                    assert all(fields) and len({field[1] for field in fields}) == len(fields), 'Ambiguous FAnimSegment fields'
                    values = {field[1]: field[2] for field in fields}
                    path = 'SlotAnimTracks[' + str(slot_index) + '].AnimTrack.AnimSegments[' + str(segment_index) + ']'
                    entry = {'field_path': path, 'anim_reference_raw': values.get('AnimReference'), 'cache_raw': values.get('CachedPlayLength'),
                        'cache_exported': 'CachedPlayLength' in values}
                    evidence['segments'].append(entry)
                    try:
                        reference = re.fullmatch(r'"?/Script/Engine\.(AnimSequence|AnimComposite)\'(/Game/[^\'\"]+)\'"?', values['AnimReference'])
                        assert reference, 'Expected exact typed FAnimSegment.AnimReference'
                        asset = load_exact(package(reference[2]), '/Script/Engine.' + reference[1])
                        assert asset.get_path_name() == reference[2]
                        length = float(asset.get_play_length())
                        cached = float(values.get('CachedPlayLength', '0'))
                        entry.update({'anim_reference': asset.get_path_name(), 'referenced_class': asset.get_class().get_path_name(),
                            'referenced_play_length': length if math.isfinite(length) else None,
                            'cached_play_length': cached if math.isfinite(cached) else None,
                            'referenced_play_length_repr': repr(length), 'cached_play_length_repr': repr(cached)})
                        assert math.isfinite(length) and length >= 0 and math.isfinite(cached), 'Nonfinite/invalid segment length evidence'
                        assert not private or cached >= 0, 'Private segment cache cannot be negative'
                        if cached == 0:
                            entry['cache_state'] = 'explicit_default_zero' if entry['cache_exported'] else 'omitted_default_zero'
                        elif abs(cached - length) <= tolerance:
                            entry['cache_state'] = 'matches_referenced_length'
                        elif not private:
                            entry['cache_state'] = 'stale_source_editor_cache'
                        else:
                            raise AssertionError('Private FAnimSegment cache differs from actual referenced clip length')
                    except Exception:
                        entry['validation_error'] = traceback.format_exc()
                        evidence['validation_errors'].append({'field_path': path, 'error': entry['validation_error'],
                            'expected': 'finite actual clip/cache length; private cache0 or actual clip length within export roundoff',
                            'after': {key: entry.get(key) for key in ('cache_raw', 'cached_play_length', 'anim_reference', 'referenced_play_length')}})
                    # Keep every authoring token, including nested fields named
                    # CachedPlayLength, verbatim. Remove this segment field only.
                    semantic_segments.append('(' + ','.join(value for value, field in zip(segment_parts, fields)
                        if field[1] != 'CachedPlayLength') + ')')
                track_parts[array_index] = 'AnimSegments=(' + ','.join(semantic_segments) + ')'
            slot_parts[track_index] = 'AnimTrack=(' + ','.join(track_parts) + ')'
        normalized.append('(' + ','.join(slot_parts) + ')')
    return normalized, evidence


def same_time_sync_marker_metadata(markers):
    """Canonicalize full records only inside consecutive exact-time ties."""
    raw, canonical, groups = [], [], []
    for index, marker in enumerate(markers):
        time = float(marker.time)  # BlueprintReadOnly FAnimSyncMarker::Time, no export rounding.
        record = value_summary(marker)
        assert math.isfinite(time) and isinstance(record, str), 'Expected finite typed sync-marker time and full export_text'
        raw.append({'original_index': index, 'typed_time': time, 'typed_time_repr': repr(time), 'record': record})
    start = 0
    while start < len(raw):
        end = start + 1
        while end < len(raw) and raw[end]['typed_time'] == raw[start]['typed_time']:
            end += 1
        groups.append({'start': start, 'count': end - start, 'typed_time': raw[start]['typed_time']})
        # Sorting this slice neither merges nonconsecutive ties nor changes the
        # order of different-time groups. Full payloads and duplicates survive.
        canonical.extend({'typed_time': item['typed_time'], 'record': item['record']}
            for item in sorted(raw[start:end], key=lambda item: item['record']))
        start = end
    return canonical, {'raw_order': raw, 'consecutive_exact_time_groups': groups,
        'policy': 'Exact typed Time equality only; full export_text sorted inside each consecutive tie; '
            'different-time order, duplicate count, Name/Track/Guid and all payloads retained'}


def animation_metadata(obj, directory, segment_cache_evidence=None, sync_marker_evidence=None):
    kind = obj.get_class().get_name()
    assert kind in ('AnimSequence', 'AnimMontage')
    # These EditAnywhere fields remain typed reads. The arrays below only have
    # UPROPERTY(), so Python get_editor_property is not allowed to read them.
    names = ()
    if kind == 'AnimSequence':
        names = ('AdditiveAnimType', 'RefPoseType', 'RefFrameIndex', 'RefPoseSeq',
            'bEnableRootMotion', 'bForceRootLock', 'RootMotionRootLock',
            'bUseNormalizedRootMotionScale', 'Interpolation')
    else:
        names = ('BlendIn', 'BlendOut',
                  'BlendModeIn', 'BlendModeOut', 'BlendProfileIn', 'BlendProfileOut', 'bEnableAutoBlendOut')
    result = data_snapshot(obj, names)
    library = ue.AnimationLibrary
    result['Notifies'] = value_summary(library.get_animation_notify_events(obj))
    result['RateScale'] = library.get_rate_scale(obj)
    # Keep the current full export as evidence. Only depth-one array properties
    # belong to this animation; notify/data-model subobjects are not conflated.
    token = hashlib.sha256(obj.get_path_name().encode('utf-8')).hexdigest()[:16]
    raw = export(obj, directory / ('metadata_' + token + '.t3d'))
    fields = ('AnimNotifyTracks',) if kind == 'AnimSequence' else (
        'AnimNotifyTracks', 'CompositeSections', 'SlotAnimTracks')
    arrays = {field: [] for field in fields}
    explicit_empty = set()
    depth = roots = 0
    for line in raw.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object'):
            if depth == 0:
                roots += 1
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
            assert depth >= 0, 'Unbalanced animation export'
        elif depth == 1:
            match = re.fullmatch(r'([A-Za-z0-9_]+)(?:\((\d+)\))?=(.*)', stripped)
            if match and match[1] in arrays:
                field, index, value = match.groups()
                if index is None:
                    assert value == '()' and not arrays[field] and field not in explicit_empty
                    explicit_empty.add(field)
                else:
                    assert field not in explicit_empty and int(index) == len(arrays[field]), 'Incomplete animation array export: ' + field
                    arrays[field].append(value)
    assert roots == 1 and depth == 0, 'Expected one complete animation root export'
    tracks = [str(name) for name in library.get_animation_notify_track_names(obj)]
    assert len(arrays['AnimNotifyTracks']) == len(tracks), 'Notify track export/count mismatch'
    result.update(arrays)
    result['notify_track_names'] = tracks
    result['metadata_read_sources'] = {'Notifies': 'AnimationLibrary.get_animation_notify_events/full struct export_text',
        'RateScale': 'AnimationLibrary.get_rate_scale',
        'AnimNotifyTracks': 'ObjectExporterT3D depth-one indexed array; public track-name count checked',
        'empty_array_policy': 'Omitted/default or explicit () means empty; public array counts must agree'}
    if kind == 'AnimSequence':
        markers = list(library.get_animation_sync_markers(obj))
        result['AuthoredSyncMarkers'], marker_evidence = same_time_sync_marker_metadata(markers)
        if sync_marker_evidence is not None:
            assert obj.get_path_name() not in sync_marker_evidence
            sync_marker_evidence[obj.get_path_name()] = marker_evidence
        result['metadata_read_sources']['AuthoredSyncMarkers'] = ('AnimationLibrary.get_animation_sync_markers read once; exact '
            'typed marker.time plus full export_text; canonical full-record order only inside consecutive equal-time groups; '
            'original order and typed values in separate Saved evidence')
        curves = {}
        for name in ue.AnimationLibrary.get_animation_curve_names(obj, ue.RawCurveTrackTypes.RCT_FLOAT):
            times, values = ue.AnimationLibrary.get_float_keys(obj, name)
            curves[str(name)] = {'times': list(times), 'values': list(values)}
        result['float_curve_keys'] = curves
    else:
        assert len(arrays['CompositeSections']) == obj.get_num_sections(), 'Montage section export/count mismatch'
        slots = [str(name) for name in library.get_montage_slot_names(obj)]
        assert len(arrays['SlotAnimTracks']) == len(slots), 'Montage slot export/count mismatch'
        result['montage_slot_names'] = slots
        result['metadata_read_sources'].update({
            'CompositeSections': 'ObjectExporterT3D depth-one indexed array; AnimMontage.get_num_sections checked',
            'SlotAnimTracks': 'ObjectExporterT3D exact SlotAnimTracks/AnimTrack/AnimSegments; only FAnimSegment.CachedPlayLength '
                'separated as editor cache; every authoring token retained; public slot count checked',
            'AnimSegmentCachedPlayLength': 'Separate Saved evidence; source default/stale cache recorded; private cache0 or '
                'referenced get_play_length within 0.00000051 seconds; referenced source/target actual lengths compared separately'})
        result['SlotAnimTracks'], cache = montage_segment_metadata(obj, arrays['SlotAnimTracks'])
        cache.update({'raw_export': (directory / ('metadata_' + token + '.t3d')).as_posix(),
            'raw_export_sha256': sha(directory / ('metadata_' + token + '.t3d'))})
        cache_path = directory / ('metadata_' + token + '_segment_cache.json')
        assert not cache_path.exists(), 'Preserve existing montage segment cache evidence'
        cache['cache_evidence_path'] = cache_path.as_posix()
        persist(cache_path, cache)
        if segment_cache_evidence is not None:
            assert obj.get_path_name() not in segment_cache_evidence
            segment_cache_evidence[obj.get_path_name()] = cache
    return result


def remap_metadata(value, mapping):
    if isinstance(value, dict):
        return {k: remap_metadata(v, mapping) for k, v in value.items()}
    if isinstance(value, list):
        return [remap_metadata(v, mapping) for v in value]
    if isinstance(value, str):
        for source, target in sorted(mapping.items(), key=lambda item: -len(item[0])):
            value = value.replace(source, target)
        return value
    return value


def animation_metadata_differences(expected, actual, path='$'):
    """Retain exact expected/actual values at mismatching metadata field paths."""
    if expected == actual:
        return []
    differences = []
    if isinstance(expected, dict) and isinstance(actual, dict):
        for key in sorted(set(expected) | set(actual)):
            if key not in expected or key not in actual:
                differences.append({'field_path': path + '.' + key, 'expected': expected.get(key, '<missing>'),
                    'after': actual.get(key, '<missing>'), 'kind': 'missing_field'})
            else:
                differences.extend(animation_metadata_differences(expected[key], actual[key], path + '.' + key))
    elif isinstance(expected, list) and isinstance(actual, list):
        for index in range(max(len(expected), len(actual))):
            if index >= len(expected) or index >= len(actual):
                differences.append({'field_path': path + '[' + str(index) + ']',
                    'expected': expected[index] if index < len(expected) else '<missing>',
                    'after': actual[index] if index < len(actual) else '<missing>', 'kind': 'missing_item'})
            else:
                differences.extend(animation_metadata_differences(expected[index], actual[index], path + '[' + str(index) + ']'))
    else:
        differences.append({'field_path': path, 'expected': expected, 'after': actual, 'kind': 'value'})
    return differences


def duplicate(row):
    target = row['target']
    assert target in new_packages() and not ue.EditorAssetLibrary.does_asset_exist(target)
    assert not package_disk(target).exists()
    source = load_exact(row['source'], row['class'])
    folder, name = target.rsplit('/', 1)
    result = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, folder, source)
    assert result and result.get_path_name() == target + '.' + name
    assert result.get_class() == source.get_class()
    scoped_dirty()
    return result


def pawn_mesh_transform_diagnostics(directory, report, stage):
    """Observe loaded pawn templates without setters, reconstruction or compile."""
    assert report['mode'] == 'prepare' and stage in ('before_native_adapt', 'after_native_adapt')
    assert directory.resolve() == (INTEGRATION / report['run']).resolve()
    stages = report.setdefault('pawn_mesh_transform_diagnostics', {})
    assert stage not in stages
    result = stages[stage] = {'stage': stage, 'complete': False, 'asset_saves': 0,
        'raw_transform_source': 'get_editor_property; no quaternion conversion', 'packages': {}, 'errors': []}
    pairs = {'/Game/DreamCatcher/Blueprints/Player/BP_DC_RiflePawn': DESTINATION + '/Assets/BP_DC_RiflePawn',
        ROOTS['pawn']: DESTINATION + '/Assets/BP_DC_LyraInventoryPawn'}
    scopes = {package_name: {'role': role, 'source': source, 'private': target}
        for source, target in pairs.items() for role, package_name in (('source', source), ('private', target))}
    result['dirty_before'] = scoped_dirty()
    integration_protection(False)

    def outer_chain(obj):
        chain, seen = [], set()
        while obj is not None:
            path = obj.get_path_name()
            assert path not in seen and len(chain) < 32, 'Unexpected pawn component outer chain'
            seen.add(path)
            chain.append({'path': path, 'class': obj.get_class().get_path_name()})
            obj = obj.get_outer()
        return chain

    def raw_transform(obj):
        values = {}
        for field, native, coordinates in (('relative_location', 'RelativeLocation', ('x', 'y', 'z')),
                ('relative_rotation', 'RelativeRotation', ('pitch', 'yaw', 'roll')),
                ('relative_scale3d', 'RelativeScale3D', ('x', 'y', 'z'))):
            value = obj.get_editor_property(native)
            values[field] = {axis: float(getattr(value, axis)) for axis in coordinates}
        return values

    def evidence(obj, stem):
        path = directory / (stem + '.t3d')
        text = export(obj, path)
        header = text.splitlines()[0]
        archetype = re.search(r'\bArchetype="([^"]+)"', header)
        return {'path': path.as_posix(), 'sha256': sha(path), 'header': header,
            'archetype_from_export_header': archetype[1] if archetype else None}

    source_objects, source_transforms, source_meshes = {}, {}, {}
    for package_name, scope in sorted(scopes.items()):
        entry = result['packages'][package_name] = dict(scope, components=[])
        try:
            name = package_name.rsplit('/', 1)[-1]
            bp = ue.find_object(None, package_name + '.' + name)
            assert bp and bp.get_class().get_path_name() == '/Script/Engine.Blueprint'
            generated = bp.generated_class()
            assert generated
            cdo = ue.get_default_object(generated)
            entry.update({'blueprint': bp.get_path_name(), 'generated_class': generated.get_path_name(),
                'generated_cdo': cdo.get_path_name(), 'generated_cdo_outer_chain': outer_chain(cdo)})
            entry['generated_cdo_export'] = evidence(cdo, 'pawn_mesh_' + stage + '_' + scope['role'] + '_' + name + '_cdo')
            mesh = cdo.get_editor_property('mesh')  # ACharacter::GetMesh's reflected property.
            assert mesh and isinstance(mesh, ue.SkeletalMeshComponent)
            entry['generated_cdo_mesh'] = mesh.get_path_name()
            entry['generated_cdo_mesh_raw_transform'] = raw_transform(mesh)
            if scope['role'] == 'source':
                source_meshes[package_name] = {'cdo': cdo.get_path_name(), 'mesh': mesh.get_path_name(),
                    'raw_transform': entry['generated_cdo_mesh_raw_transform']}
        except Exception:
            entry['generated_cdo_error'] = traceback.format_exc()
            result['errors'].append({'package': package_name, 'operation': 'generated CDO/mesh', 'error': entry['generated_cdo_error']})
    for component in ue.ObjectIterator(ue.SkeletalMeshComponent):
        if component.get_name() != 'CharacterMesh0':
            continue
        chain = outer_chain(component)
        package_name = chain[-1]['path']
        if package_name not in scopes:
            continue
        entry = result['packages'][package_name]
        item = {'object': component.get_path_name(), 'class': component.get_class().get_path_name(),
            'outer_chain': chain, 'is_generated_cdo_get_mesh': component.get_path_name() == entry.get('generated_cdo_mesh')}
        entry['components'].append(item)
        try:
            item['raw_transform'] = raw_transform(component)
            item['double_roundtrip_repr'] = {field: {axis: repr(value) for axis, value in coordinates.items()}
                for field, coordinates in item['raw_transform'].items()}
            if entry['role'] == 'source':
                source_objects[item['object']] = component
                source_transforms[item['object']] = item['raw_transform']
        except Exception:
            item['raw_transform_error'] = traceback.format_exc()
            result['errors'].append({'object': item['object'], 'operation': 'component raw fields', 'error': item['raw_transform_error']})
        try:
            item['export'] = evidence(component, 'pawn_mesh_' + stage + '_' + entry['role'] + '_'
                + package_name.rsplit('/', 1)[-1] + '_' + str(len(entry['components'])))
        except Exception:
            item['export_error'] = traceback.format_exc()
            result['errors'].append({'object': item['object'], 'operation': 'component export', 'error': item['export_error']})
    for package_name, entry in result['packages'].items():
        entry['components'].sort(key=lambda item: item['object'])
        reference = source_meshes.get(entry['source'])
        if not entry['components'] or reference is None:
            result['errors'].append({'package': package_name, 'operation': 'required loaded component/source reference missing'})
        for item in entry['components']:
            if reference is not None and 'raw_transform' in item:
                item['source_generated_cdo_mesh'] = reference['mesh']
                item['raw_delta_to_source_generated_cdo_mesh'] = {field: {axis: value - reference['raw_transform'][field][axis]
                    for axis, value in coordinates.items()} for field, coordinates in item['raw_transform'].items()}
    result['source_raw_transforms'] = source_transforms
    result['source_generated_cdo_meshes'] = source_meshes
    result['source_unchanged_by_diagnostic'] = {path: raw_transform(obj) for path, obj in source_objects.items()} == source_transforms
    if stage == 'after_native_adapt':
        before = stages['before_native_adapt']
        result['source_matches_pre_adapt'] = (source_transforms == before['source_raw_transforms']
            and source_meshes == before['source_generated_cdo_meshes'])
    result['dirty_after'] = scoped_dirty()
    integration_protection(False)
    result['source_file_protection_passed'] = True
    result['complete'] = (not result['errors'] and len(source_meshes) == 2 and result['source_unchanged_by_diagnostic']
        and result.get('source_matches_pre_adapt', True))
    persist(directory / ('pawn_mesh_transforms_' + stage + '.json'), result)
    persist(directory / 'checkpoint.json', report)
    assert result['complete'], 'Pawn mesh transform observation incomplete or source state changed; inspect Saved evidence'


def integration_prepare(world, directory, report):
    rows = approved_rows()
    assert not private_asset_catalog() and not disk_hashes(), 'New Inspector root must be empty; partial outputs are never overwritten'
    report['batch_may_save_explicit_new_assets_internally'] = True
    report['source_animation_metadata'] = {}
    report['source_montage_segment_cache'] = {}
    report['source_sync_marker_evidence'] = {}
    anim = [row for row in rows if row['class'].rsplit('.', 1)[-1] in ANIMATION_CLASSES]
    assert len(anim) == 287 and all(row['target'].rsplit('/', 1)[0] == DESTINATION + '/Animation' for row in anim)
    assert all(row['source'].rsplit('/', 1)[-1] == row['target'].rsplit('/', 1)[-1] for row in anim), 'Batch requires exact unchanged names from the approved manifest'
    batch_anim = [row for row in anim if row['class'] != '/Script/Engine.AnimBlueprint']
    blueprint_order = INSPECTOR_ANIM_BLUEPRINT_ORDER
    blueprint_rows = {row['source'].rsplit('/', 1)[-1]: row for row in anim if row['class'] == '/Script/Engine.AnimBlueprint'}
    assert len(batch_anim) == 281 and len(blueprint_rows) == 6 and set(blueprint_rows) == set(blueprint_order)
    blueprints = [blueprint_rows[name] for name in blueprint_order]
    report['batch_input_packages'] = [row['source'] for row in batch_anim]
    report['deferred_anim_blueprint_packages'] = [row['target'] for row in blueprints]
    for row in anim:
        if row['class'] in ('/Script/Engine.AnimSequence', '/Script/Engine.AnimMontage'):
            report['source_animation_metadata'][row['source']] = animation_metadata(load_exact(row['source'], row['class']), directory,
                report['source_montage_segment_cache'], report['source_sync_marker_evidence'])
    report['source_segment_cache_errors'] = [dict(error, montage=path)
        for path, cache in report['source_montage_segment_cache'].items() for error in cache['validation_errors']]
    source_marker_path = directory / 'source_sync_marker_evidence.json'
    assert not source_marker_path.exists(), 'Preserve original sync-marker evidence'
    persist(source_marker_path, report['source_sync_marker_evidence'])
    persist(directory / 'checkpoint.json', report)
    assert not report['source_segment_cache_errors'], 'Invalid source segment length evidence; no source cache was modified'
    scoped_dirty()
    initial = [row for row in rows if row['source'].startswith(ROOTS['target_mesh'].rsplit('/', 1)[0] + '/')
               and row['class'] in ('/Script/Engine.SkeletalMesh', '/Script/Engine.Skeleton', '/Script/Engine.PhysicsAsset')]
    assert len(initial) == 3
    created = {row['target']: duplicate(row) for row in initial}
    for key, package_name in RIGS.items():
        folder, name = package_name.rsplit('/', 1)
        if key == 'retargeter':
            cls, factory = ue.IKRetargeter, ue.IKRetargetFactory()
        else:
            cls, factory = ue.IKRigDefinition, ue.IKRigDefinitionFactory()
        obj = ue.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, cls, factory)
        assert obj and obj.get_path_name() == package_name + '.' + name
        if key == 'retargeter':
            control = ue.IKRetargeterController.get_controller(obj)
            count = control.get_num_retarget_ops()
            report['factory_initial_retarget_ops'] = count
            if count:
                assert count == 5 and control.get_ik_rig(ue.RetargetSourceOrTarget.SOURCE) is None and control.get_ik_rig(ue.RetargetSourceOrTarget.TARGET) is None
                assert control.remove_all_ops(), 'Only fresh factory defaults may be reset before native configuration'
                assert control.get_num_retarget_ops() == 0
        created[package_name] = obj
    native_phase(world, directory, 'Prepare', report)
    report['prepared_packages'] = private_asset_catalog()
    persist(directory / 'checkpoint.json', report)
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    selection = []
    for row in batch_anim:
        source = load_exact(row['source'], row['class'])
        found = list(registry.get_assets_by_package_name(row['source'], True))
        assert len(found) == 1
        selection.append(found[0])
    inputs = ue.IKRetargetBatchOperationInputs()
    settings = {'AssetsToRetarget': selection, 'SourceMesh': load_exact(ROOTS['source_mesh']),
        'TargetMesh': created[DESTINATION + '/Assets/Inspector'],
        'IKRetargetAsset': created[RIGS['retargeter']], 'TargetPath': DESTINATION + '/Animation',
        'bUseSourcePath': False, 'bIncludeReferencedAssets': False,
        'bOverwriteExistingFiles': False, 'bRetainAdditiveFlags': True,
        'Search': '', 'Replace': '', 'Prefix': '', 'Suffix': ''}
    for name, value in settings.items():
        inputs.set_editor_property(name, value)
    scoped_dirty()
    integration_log_check(directory / 'Editor.log', report)
    result = list(ue.IKRetargetBatchOperation.run_batch_retarget(inputs))
    actual = {str(row.package_name) for row in result}
    expected = {row['target'] for row in batch_anim}
    report['batch_returned_packages'] = sorted(actual)
    evidence = report['batch_return_evidence'] = {'actual': sorted(actual),
        'missing': sorted(expected - actual), 'unexpected': sorted(actual - expected),
        'raw_return': [], 'expected_assets': []}
    try:
        evidence['dirty_content_packages'] = sorted(obj.get_path_name() for obj in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        evidence['dirty_map_packages'] = sorted(obj.get_path_name() for obj in ue.EditorLoadingAndSavingUtils.get_dirty_map_packages())
        for data in result:
            class_data = data.asset_class_path
            evidence['raw_return'].append({'package': str(data.package_name), 'asset_name': str(data.asset_name),
                'class': str(class_data.get_editor_property('package_name')) + '.' + str(class_data.get_editor_property('asset_name'))})
        evidence['private_asset_catalog'] = private_asset_catalog()
        for row in batch_anim:
            object_path = row['target'] + '.' + row['target'].rsplit('/', 1)[-1]
            loaded = ue.find_object(None, object_path)
            found = list(registry.get_assets_by_package_name(row['target'], False))
            evidence['expected_assets'].append({'package': row['target'], 'expected_object_path': object_path,
                'expected_class': row['class'], 'registry_asset_count': len(found), 'loaded': loaded is not None,
                'registry_classes': [str(data.asset_class_path.get_editor_property('package_name')) + '.'
                    + str(data.asset_class_path.get_editor_property('asset_name')) for data in found],
                'loaded_object_path': loaded.get_path_name() if loaded else None,
                'loaded_class': loaded.get_class().get_path_name() if loaded else None})
    except Exception:
        evidence['query_error'] = traceback.format_exc()
    if actual != expected:
        exports = evidence['anim_blueprint_exports'] = []
        evidence['anim_blueprint_count'] = len(blueprints)
        for index, row in enumerate(blueprints):
            for label in ('source', 'target'):
                item = {'kind': label, 'package': row[label], 'expected_class': row['class'],
                    'target_creation_stage': 'after successful 281-animation batch'}
                exports.append(item)
                try:
                    obj = load_exact(row[label], row['class'])
                    path = directory / ('batch_mismatch_' + label + '_' + str(index) + '_' + row[label].rsplit('/', 1)[-1] + '.t3d')
                    export(obj, path)
                    item.update({'object_path': obj.get_path_name(), 'class': obj.get_class().get_path_name(),
                        'path': path.as_posix(), 'sha256': sha(path)})
                except Exception:
                    # Evidence failure must not replace the original batch assertion.
                    item['export_error'] = traceback.format_exc()
    assert actual == expected, 'Batch returned an unexpected or incomplete package set'
    assert 'query_error' not in evidence, 'Batch result inspection failed: ' + evidence.get('query_error', '')
    assert len(result) == len(evidence['raw_return']) == len(evidence['expected_assets']) == 281
    expected_classes = {row['target']: row['class'] for row in batch_anim}
    assert all(item['class'] == expected_classes[item['package']] for item in evidence['raw_return']), 'Batch returned an unexpected animation class'
    assert all(item['loaded'] and item['loaded_object_path'] == item['expected_object_path']
        and item['loaded_class'] == item['expected_class'] and item['registry_asset_count'] == 1
        and item['registry_classes'] == [item['expected_class']] for item in evidence['expected_assets']), 'Batch animation object/class/registry mismatch'
    assert set(evidence['private_asset_catalog']) == set(report['prepared_packages']) | expected, 'Unexpected private catalog after 281-animation batch'
    report['batch_disk_files'] = disk_hashes()
    private_asset_catalog()
    scoped_dirty()
    integration_log_check(directory / 'Editor.log', report)
    integration_protection(False)
    # Keep these six copies source-compatible until native Adapt rewrites the
    # whole reference graph. Duplication may compile; every error still stops.
    report['anim_blueprint_copy_evidence'] = []
    for row in blueprints:
        source = load_exact(row['source'], row['class'])
        source_parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(source)
        source_skeleton = source.get_editor_property('TargetSkeleton')
        source_state = {'class': source.get_class().get_path_name(), 'parent': value_summary(source_parent),
            'skeleton': value_summary(source_skeleton)}
        assert source_skeleton and source_state['skeleton'] == '/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin'
        scoped_dirty()
        before_duplicate_log = integration_log_check(directory / 'Editor.log', report)
        copied = duplicate(row)
        private_state = {'class': copied.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(copied)),
            'skeleton': value_summary(copied.get_editor_property('TargetSkeleton'))}
        report['anim_blueprint_copy_evidence'].append({'source': row['source'], 'target': row['target'],
            'source_state': source_state, 'private_state_before_native_adapt': private_state,
            'before_duplicate_log': before_duplicate_log})
        assert private_state == source_state, 'AnimBP copy must preserve source type, parent and skeleton before native Adapt'
        assert source.get_editor_property('TargetSkeleton') == source_skeleton
        assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(source) == source_parent
        scoped_dirty()
        try:
            integration_log_check(directory / 'Editor.log', report)
        except Exception:
            item = report['anim_blueprint_copy_evidence'][-1]
            item['copy_log_error'] = traceback.format_exc()
            item['failure_exports'] = []
            for label, obj in (('source', source), ('target', copied)):
                evidence_item = {'kind': label, 'object_path': obj.get_path_name()}
                item['failure_exports'].append(evidence_item)
                try:
                    path = directory / ('anim_blueprint_copy_failure_' + label + '_' + row['target'].rsplit('/', 1)[-1] + '.t3d')
                    export(obj, path)
                    evidence_item.update({'path': path.as_posix(), 'sha256': sha(path)})
                except Exception:
                    evidence_item['export_error'] = traceback.format_exc()
            recovered = False
            try:
                recover_item_layer_copy(source, copied, row, directory, report, item, before_duplicate_log)
                recovered = True
            except Exception:
                item['recovery_rejected'] = traceback.format_exc()
            if not recovered:
                raise  # Preserve the original strict log error; never continue.

    assert set(private_asset_catalog()) == set(report['prepared_packages']) | {row['target'] for row in anim}, 'Expected all 287 animation/AnimBP copies after deferred blueprint duplication'
    integration_protection(False)
    initial_targets = {row['target'] for row in initial}
    remaining = [row for row in rows if row not in anim and row['target'] not in initial_targets]
    assert len(remaining) == 14
    for row in remaining:
        try:
            duplicate(row)
        except Exception as duplicate_error:
            if (row['source'] == '/Game/Characters/Heroes/Mannequin/Rig/CR_Mannequin_FootPlant'
                    and row['target'] == DESTINATION + '/Assets/CR_Mannequin_FootPlant'):
                failure = report['control_rig_duplicate_failure'] = {'source': row['source'], 'target': row['target'],
                    'expected_class': row['class'], 'error': traceback.format_exc(), 'error_text': str(duplicate_error),
                    'error_repr': repr(duplicate_error), 'error_args': value_summary(duplicate_error.args), 'exports': []}
                for label in ('source', 'target'):
                    entry = {'kind': label, 'object_path': row[label] + '.CR_Mannequin_FootPlant'}
                    failure['exports'].append(entry)
                    try:
                        obj = ue.find_object(None, entry['object_path'])
                        entry['found'] = obj is not None
                        if obj:
                            path = directory / ('control_rig_copy_failure_' + label + '_CR_Mannequin_FootPlant.t3d')
                            export(obj, path)
                            entry.update({'class': obj.get_class().get_path_name(), 'path': path.as_posix(), 'sha256': sha(path)})
                    except Exception:
                        entry['export_error'] = traceback.format_exc()
                repaired = False
                try:
                    assert report['mode'] == 'prepare' and type(duplicate_error) is RuntimeError
                    assert row['class'] == '/Script/ControlRigDeveloper.ControlRigBlueprint'
                    expected_errors = {('RerouteNode_11.Value', 'MathFloatAdd.B'), ('AlphaInterp.Result', 'RerouteNode_9.Value'),
                        ('RerouteNode_7.Value', 'MathTransformRotateVector_1.Vector.X'), ('AlphaInterp_1.Result', 'RerouteNode_2.Value'),
                        ('RerouteNode_8.Value', 'TwoBoneIKSimplePerItem.Effector'), ('GetTransform_8_3.Transform', 'RerouteNode_8.Value')}
                    pattern = (r'(?:RuntimeError: )?AssetTools: ' + re.escape(row['target'])
                        + r' : Cannot break link (.+?) -> (.+?) in package ' + re.escape(row['target']))
                    matches = [re.fullmatch(pattern, line.strip()) for line in str(duplicate_error).splitlines()]
                    assert len(matches) == 6 and all(matches), 'FootPlant duplication error differs from the verified six link failures'
                    pairs = {(match[1], match[2]) for match in matches}
                    assert pairs == expected_errors, 'Unexpected FootPlant link failure'
                    source_rig = ue.find_object(None, row['source'] + '.CR_Mannequin_FootPlant')
                    private_rig = ue.find_object(None, row['target'] + '.CR_Mannequin_FootPlant')
                    assert source_rig and private_rig and source_rig != private_rig
                    assert source_rig.get_class().get_path_name() == private_rig.get_class().get_path_name() == row['class']
                    assert not package_disk(row['target']).exists(), 'Only an unsaved new FootPlant copy may be repaired'
                    failure['repair'] = {'error_pairs': sorted([list(pair) for pair in pairs]), 'adopted': False,
                        'target_object_path': private_rig.get_path_name(), 'asset_saves': 0}
                    scoped_dirty()
                    integration_protection(False)
                    integration_log_check(directory / 'Editor.log', report)
                    native_phase(world, directory, 'RepairFootPlantCopy', report)
                    assert load_exact(row['target'], row['class']) == private_rig
                    scoped_dirty()
                    integration_protection(False)
                    integration_log_check(directory / 'Editor.log', report)
                    failure['repair']['adopted'] = True
                    repaired = True
                except Exception:
                    failure['repair_rejected'] = traceback.format_exc()
                if repaired:
                    continue  # Adopt only the natively verified, exactly repaired copy.
            raise  # Preserve any unverified AssetTools duplication failure.
    assert len(private_asset_catalog(True)) == 307
    # Restore the input that the original 287-asset batch supplied to native
    # Adapt. Only these six private skeleton pointers change. Suppress editor
    # property notifications here; native Adapt owns marking/compile/reparent.
    private_skeleton = load_exact(DESTINATION + '/Assets/Inspector_Skeleton', '/Script/Engine.Skeleton')
    report['anim_blueprint_skeleton_preconditions'] = []
    for row in blueprints:
        source = load_exact(row['source'], row['class'])
        copied = load_exact(row['target'], row['class'])
        source_before = {'class': source.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(source)),
            'skeleton': value_summary(source.get_editor_property('TargetSkeleton'))}
        original = next(item for item in report['anim_blueprint_copy_evidence'] if item['source'] == row['source'])
        assert source_before == original['source_state']
        private_before = {'class': copied.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(copied)),
            'skeleton': value_summary(copied.get_editor_property('TargetSkeleton'))}
        assert private_before == source_before
        copied.set_editor_property('TargetSkeleton', private_skeleton,
            notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
        private_after = {'class': copied.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(copied)),
            'skeleton': value_summary(copied.get_editor_property('TargetSkeleton'))}
        source_after = {'class': source.get_class().get_path_name(),
            'parent': value_summary(ue.BlueprintEditorLibrary.get_blueprint_parent_class(source)),
            'skeleton': value_summary(source.get_editor_property('TargetSkeleton'))}
        report['anim_blueprint_skeleton_preconditions'].append({'source': row['source'], 'target': row['target'],
            'source_before': source_before, 'source_after': source_after,
            'private_before': private_before, 'private_after': private_after})
        assert source_after == source_before and copied.get_editor_property('TargetSkeleton') == private_skeleton
        assert private_after['class'] == private_before['class'] and private_after['parent'] == private_before['parent']
        scoped_dirty()
        integration_log_check(directory / 'Editor.log', report)
    integration_protection(False)
    pawn_mesh_transform_diagnostics(directory, report, 'before_native_adapt')
    native_phase(world, directory, 'Adapt', report)
    pawn_mesh_transform_diagnostics(directory, report, 'after_native_adapt')
    native_phase(world, directory, 'Verify', report)
    report['target_animation_metadata'] = {}
    report['target_montage_segment_cache'] = {}
    report['target_sync_marker_evidence'] = {}
    comparison = report['animation_metadata_comparison'] = {'expected_count': 278, 'compared_count': 0,
        'read_errors': [], 'mismatches': [], 'reference_length_comparisons': [], 'complete': False}
    plan_by_source = {row['source']: row['target'] for row in rows}
    plan_by_source['/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin.SK_Mannequin'] = (
        DESTINATION + '/Assets/Inspector_Skeleton.Inspector_Skeleton')
    for row in anim:
        if row['class'] not in ('/Script/Engine.AnimSequence', '/Script/Engine.AnimMontage'):
            continue
        try:
            assert row['target'] not in report['target_animation_metadata'], 'Duplicate target metadata request'
            before = report['source_animation_metadata'][row['source']]
            target = load_exact(row['target'], row['class'])
            after = animation_metadata(target, directory, report['target_montage_segment_cache'], report['target_sync_marker_evidence'])
            report['target_animation_metadata'][row['target']] = after
            expected = remap_metadata(before, plan_by_source)
            differences = animation_metadata_differences(expected, after)
            comparison['mismatches'].extend(dict(delta, source=row['source'], target=row['target'], category='authoring_metadata')
                for delta in differences)
            if row['class'] == '/Script/Engine.AnimMontage':
                source_path = row['source'] + '.' + row['source'].rsplit('/', 1)[-1]
                source_cache = report['source_montage_segment_cache'][source_path]
                target_cache = report['target_montage_segment_cache'][target.get_path_name()]
                comparison['mismatches'].extend(dict(error, source=row['source'], target=row['target'], category='invalid_target_segment_cache')
                    for error in target_cache['validation_errors'])
                # Cache values are deliberately outside semantic metadata. These
                # explicit records instead prove the remapped clip and its actual
                # public play length are preserved, without remapping dict keys.
                keys = ('field_path', 'anim_reference', 'referenced_class', 'referenced_play_length')
                expected_lengths = remap_metadata([{key: item.get(key) for key in keys} for item in source_cache['segments']], plan_by_source)
                actual_lengths = [{key: item.get(key) for key in keys} for item in target_cache['segments']]
                length_differences = animation_metadata_differences(expected_lengths, actual_lengths, '$.ReferencedSegmentLengths')
                comparison['reference_length_comparisons'].append({'source': row['source'], 'target': row['target'],
                    'expected': expected_lengths, 'after': actual_lengths, 'equal': not length_differences})
                comparison['mismatches'].extend(dict(delta, source=row['source'], target=row['target'], category='referenced_clip_length_or_identity')
                    for delta in length_differences)
            comparison['compared_count'] += 1
        except Exception:
            comparison['read_errors'].append({'source': row['source'], 'target': row['target'], 'error': traceback.format_exc()})
    comparison['source_snapshot_count'] = len(report['source_animation_metadata'])
    comparison['target_snapshot_count'] = len(report['target_animation_metadata'])
    comparison['complete'] = (comparison['compared_count'] == comparison['source_snapshot_count'] == comparison['target_snapshot_count'] == 278
        and not comparison['read_errors'] and not comparison['mismatches'])
    target_marker_path = directory / 'target_sync_marker_evidence.json'
    assert not target_marker_path.exists(), 'Preserve retargeted sync-marker evidence'
    persist(target_marker_path, report['target_sync_marker_evidence'])
    comparison_path = directory / 'animation_metadata_comparison.json'
    assert not comparison_path.exists(), 'Preserve previous metadata comparison evidence'
    persist(comparison_path, comparison)
    persist(directory / 'checkpoint.json', report)
    scoped_dirty()
    integration_protection(False)
    assert comparison['complete'], 'Animation metadata/segment length validation failed; all collected differences are in Saved evidence'
    report['saves'] = []
    integration_log_check(directory / 'Editor.log', report)
    integration_protection(False)
    for package_name in sorted(new_packages()):
        obj = load_exact(package_name)
        assert not isinstance(obj, ue.World)
        assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), package_name
        report['saves'].append(package_name)
    scoped_dirty()
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    assert len(report['saves']) == 307
    report['saved_files'] = disk_hashes()
    assert sum(name.endswith('.uasset') for name in report['saved_files']) == 307
    report['asset_saves'] = len(report['saves'])
    report['activation_performed'] = False


def previous_report(prefix, mode):
    path = INTEGRATION / (prefix + '_' + mode) / 'report.json'
    result = read(path)
    assert result['mode'] == mode and result['complete'] and result['protection_passed']
    assert result['script_sha256'] == sha(SCRIPT) and result['cpp_freeze_sha256'] == sha(CPP_FROZEN)
    assert result['build_receipt_sha256'] == sha(USER_BUILD)
    assert result['before_protection']['author_baseline_sha256'] == sha(AUTHOR_BASELINE)
    return result


def saved_pose_evidence(prefix):
    directory = P / 'Saved/Diagnostics/InspectorBot20261023/Pose' / (prefix + '_saved_pose')
    automation, status = directory / 'Automation/index.json', directory / 'Probe/status.csv'
    verified_at = (INTEGRATION / (prefix + '_verify') / 'report.json').stat().st_mtime_ns
    assert automation.stat().st_mtime_ns >= verified_at and status.stat().st_mtime_ns >= verified_at, 'Saved-pose evidence must be newer than fresh verification'
    index = read(automation)
    tests = index['tests']
    assert len(tests) == 1 and tests[0]['fullTestPath'] == 'DreamCatcher.Editor.InspectorRetargetPose'
    assert tests[0]['state'] == 'Success' and index['failed'] == 0 and index['notRun'] == 0 and index['inProcess'] == 0
    assert not [entry for entry in tests[0]['entries'] if entry.get('event', {}).get('type') in ('Error', 'Fatal')]
    with status.open(encoding='utf-8-sig', newline='') as stream:
        values = dict(list(csv.reader(stream))[1:])
    assert values['complete'] == 'true' and values['evaluated_animation_poses'] == '50'
    assert values['private_target_bones'] == '69 raw + 3 virtual'
    assert int(values['saved_joint_comparisons']) == 3100
    assert float(values['saved_max_joint_position_error_cm']) <= 5 and float(values['saved_max_joint_orientation_error_deg']) <= 5
    assert values['private_saved_animation_check'].startswith('10 saved clips x 5 poses')
    return {'automation_sha256': sha(automation), 'status_sha256': sha(status), 'status': values}


def experience_snapshot(directory, label):
    bp = load_exact(ROOTS['experience'], '/Script/Engine.Blueprint')
    cdo = ue.get_default_object(bp.generated_class())
    assert cdo
    return bp, cdo, {'cdo': export(cdo, directory / (label + '_cdo.t3d')),
        'blueprint': export(bp, directory / (label + '_blueprint.t3d')),
        'bot_pawn_data': value_summary(cdo.get_editor_property('BotPawnData')),
        'default_pawn_data': value_summary(cdo.get_editor_property('DefaultPawnData'))}


def without_bot_field(raw):
    return re.sub(r'^   BotPawnData=[^\n]*\n?', '', raw, flags=re.M)


def activate(directory, prefix, report):
    prepared = previous_report(prefix, 'prepare')
    verified = previous_report(prefix, 'verify')
    assert disk_hashes() == prepared['saved_files'] == verified['saved_files'], 'Verified private assets changed'
    report['pose_evidence'] = saved_pose_evidence(prefix)
    assert sha(P / EXPERIENCE_FILE) == prepared['experience_before_sha256'], 'Original experience changed before activation'
    bp, cdo, before = experience_snapshot(directory, 'before')
    assert before['bot_pawn_data'] is None, 'Existing bot override must not be overwritten'
    assert before['default_pawn_data'] == ROOTS['pawn_data'] + '.DA_DC_LyraAIPawn'
    target = load_exact(DESTINATION + '/Assets/DA_DC_LyraAIPawn', '/Script/DreamCatcher.DCPawnData')
    cdo.set_editor_property('BotPawnData', target)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    _, _, after = experience_snapshot(directory, 'after')
    assert after['bot_pawn_data'] == target.get_path_name()
    assert before['default_pawn_data'] == after['default_pawn_data']
    assert without_bot_field(before['cdo']) == without_bot_field(after['cdo']), 'Non-BotPawnData experience defaults changed'
    # Class-only Experience: authored graphs/variables and parent are preserved.
    for field in ('ParentClass', 'UbergraphPages', 'FunctionGraphs', 'NewVariables', 'ActionSets'):
        pattern = r'^   ' + field + r'(?:\([^\n]*|=[^\n]*)$'
        assert re.findall(pattern, before['blueprint'], re.M) == re.findall(pattern, after['blueprint'], re.M), field
    scoped_dirty(True)
    report['experience_before'] = before
    report['experience_after'] = after
    assert sha(P / EXPERIENCE_FILE) == prepared['experience_before_sha256'], 'Concurrent experience file change'
    integration_log_check(directory / 'Editor.log', report)
    assert ue.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    report['asset_saves'] = 1
    report['experience_after_sha256'] = sha(P / EXPERIENCE_FILE)
    report['saved_files'] = disk_hashes()
    assert report['saved_files'] == prepared['saved_files']
    report['activation_performed'] = True


def integration_main(command, mode):
    run = argument(command, 'DCInspectorRun', '[A-Za-z0-9_]{1,40}')
    prefix = argument(command, 'DCInspectorPrefix', '[A-Za-z0-9_]{1,20}')
    frozen_sha = argument(command, 'DCInspectorSHA', '[a-f0-9]{64}')
    assert run == prefix + '_' + mode and sha(SCRIPT) == frozen_sha
    assert argument(command, 'DCInspectorBotRun', '[A-Za-z0-9_]{1,40}') == run
    assert '-run=pythonscript' not in command.lower(), 'Integration needs a regular Editor world'
    assert all(flag in command.lower() for flag in ('-unattended', '-sccprovider=none', '-dcinspectorbotassets'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    directory = INTEGRATION / run
    directory.mkdir(parents=True, exist_ok=True)
    assert all(p.name == 'Editor.log' for p in directory.iterdir()), 'Preserve existing run receipts'
    log_path = directory / 'Editor.log'
    log_arg = re.findall(r'(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(log_arg) == 1 and Path(log_arg[0][0] or log_arg[0][1]).resolve() == log_path.resolve()
    report = {'schema': 2, 'mode': mode, 'run': run, 'prefix': prefix,
        'script_sha256': frozen_sha, 'cpp_freeze_sha256': sha(CPP_FROZEN),
        'build_receipt_sha256': sha(USER_BUILD), 'complete': False,
        'protection_passed': False, 'asset_saves': 0, 'PIE': False, 'cpp_build': False,
        'physical_play_verified': False, 'new_asset_candidate_count': 307}
    helpers = None
    try:
        report['before_protection'] = integration_protection(mode in ('activate', 'verify_activation'))
        helpers = module('ingame_hud_layout_setup')
        helpers.assert_no_other_editor()
        scoped_dirty()
        settings_cls = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings_cls
        text = export(ue.get_default_object(settings_cls), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', text, re.M) == ['SoC_Never']
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
        assert world and world.get_class().get_name() == 'World', 'Actual Editor world is required'
        report['world_path'] = world.get_path_name()
        report['experience_before_sha256'] = sha(P / EXPERIENCE_FILE)
        if mode == 'prepare':
            integration_prepare(world, directory, report)
        elif mode == 'verify':
            prepared = previous_report(prefix, 'prepare')
            assert disk_hashes() == prepared['saved_files']
            assert sha(P / EXPERIENCE_FILE) == prepared['experience_before_sha256']
            private_asset_catalog(True)
            assert 'item_layer_copy_recovery' not in report, 'Cold verification allows no copy-error exception'
            report['cold_compile_file_stats_before'] = {name: [(P / name).stat().st_size, (P / name).stat().st_mtime_ns]
                for name in prepared['saved_files']}
            blueprint_rows = {row['target'].rsplit('/', 1)[-1]: row for row in approved_rows()
                if row['class'] == '/Script/Engine.AnimBlueprint'}
            assert set(blueprint_rows) == set(INSPECTOR_ANIM_BLUEPRINT_ORDER)
            report['cold_anim_blueprint_compiles'] = []
            for name in INSPECTOR_ANIM_BLUEPRINT_ORDER:
                row = blueprint_rows[name]
                assert row['target'].startswith(DESTINATION + '/Animation/')
                bp = load_exact(row['target'], row['class'])
                entry = {'path': bp.get_path_name(), 'status_before': value_summary(bp.get_editor_property('Status'))}
                report['cold_anim_blueprint_compiles'].append(entry)
                before_compile_log = integration_log_check(log_path, report)
                compiled = ue.BlueprintEditorLibrary.compile_blueprint(bp)
                entry.update({'compile_return': value_summary(compiled), 'compile_return_python_type': type(compiled).__name__,
                    'status_after': value_summary(bp.get_editor_property('Status'))})
                entry['dirty_packages_after'] = scoped_dirty()
                after_compile_log = integration_log_check(log_path, report)
                raw_log = log_path.read_bytes()
                assert hashlib.sha256(raw_log[:before_compile_log['byte_length']]).hexdigest() == before_compile_log['sha256'], 'Cold compiler log prefix changed'
                interval = raw_log[before_compile_log['byte_length']:after_compile_log['byte_length']].decode('utf-8-sig', errors='replace')
                entry['log_offset_before'] = before_compile_log['byte_length']
                entry['log_offset_after'] = after_compile_log['byte_length']
                entry['new_warning_lines'] = [line for line in interval.splitlines() if re.search(r'\bLog\w+:\s*Warning:', line, re.I)]
                entry['warning_review_required'] = bool(entry['new_warning_lines'])
                assert compiled is True and type(compiled) is bool and bp.get_editor_property('Status') in (
                    ue.BlueprintStatus.BS_UP_TO_DATE, ue.BlueprintStatus.BS_UP_TO_DATE_WITH_WARNINGS)
            assert len(report['cold_anim_blueprint_compiles']) == 6
            assert disk_hashes() == prepared['saved_files'], 'Cold compilation must not save any asset'
            integration_protection(False)
            native_phase(world, directory, 'Verify', report)
            report['saved_files'] = disk_hashes()
            assert report['saved_files'] == prepared['saved_files']
            report['fresh_reload_verified'] = True
        elif mode == 'activate':
            activate(directory, prefix, report)
        else:
            previous = previous_report(prefix, 'activate')
            assert sha(P / EXPERIENCE_FILE) == previous['experience_after_sha256']
            _, _, current = experience_snapshot(directory, 'reloaded')
            assert current['cdo'] == previous['experience_after']['cdo']
            assert current['bot_pawn_data'] == DESTINATION + '/Assets/DA_DC_LyraAIPawn.DA_DC_LyraAIPawn'
            report['experience_reloaded'] = current
            report['saved_files'] = disk_hashes()
            assert report['saved_files'] == previous['saved_files']
            report['fresh_activation_reload_verified'] = True
        dirty_at_exit = scoped_dirty()
        if mode == 'verify':
            report['cold_verify_dirty_packages_at_exit'] = dirty_at_exit
            report['verification_memory_activity'] = 'Six private AnimBPs compiled in memory; permitted private dirty packages are recorded, never cleared or saved'
            assert report['asset_saves'] == 0 and disk_hashes() == prepared['saved_files'], 'Cold verification must not write asset files'
            report['cold_compile_file_stats_after'] = {name: [(P / name).stat().st_size, (P / name).stat().st_mtime_ns]
                for name in prepared['saved_files']}
            assert report['cold_compile_file_stats_after'] == report['cold_compile_file_stats_before'], 'Cold verification changed asset file size/mtime'
        else:
            assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages(), 'Fresh saved state must not need another repair/save'
        helpers.assert_no_other_editor()
        integration_log_check(log_path, report)
        assert sha(SCRIPT) == frozen_sha
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            report['after_protection'] = integration_protection(mode in ('activate', 'verify_activation'))
            report['private_disk_files_at_exit'] = disk_hashes()
            report['protection_passed'] = True
        except Exception:
            report['complete'] = False
            report['protection_error'] = traceback.format_exc()
            raise
        finally:
            persist(directory / 'report.json', report)
            ue.SystemLibrary.quit_editor()


def argument(command, name, pattern):
    found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
    assert len(found) == 1, 'Exactly one -' + name + ' is required'
    return found[0]


def package(path):
    return path.split('.', 1)[0].split(':', 1)[0]


def export(obj, path):
    assert obj and not path.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter = obj, ue.ObjectExporterT3D()
    task.filename, task.automated, task.prompt = path.as_posix(), True, False
    assert ue.Exporter.run_asset_export_task(task), 'Cannot export ' + obj.get_path_name()
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n')


def references(raw, source, export_file, origin):
    """Record exact exported property/line evidence, not arbitrary folder membership."""
    found = []
    context = []
    for index, line in enumerate(raw.splitlines(), 1):
        stripped = line.strip()
        if stripped.startswith('Begin Object'):
            names = re.findall(r'Name="([^"]+)"', stripped)
            context.append(names[0] if names else '?')
        elif stripped == 'End Object':
            if context:
                context.pop()
            continue
        # Declarations and ExportPaths describe the exporting object itself.
        if not stripped or stripped.startswith(('Begin Object', 'End Object')):
            continue
        field = stripped.split('=', 1)[0] if '=' in stripped else '<text>'
        for match in REF.finditer(line):
            path = match.group(0).rstrip('\\')
            target = package(path)
            if target == source:
                # Subobjects such as Skeleton:FastFeet still need explicit evidence.
                if ':' not in path or ':FastFeet' not in path and ':UpperBody' not in path:
                    continue
            found.append({'source': source, 'origin': origin, 'object': '/'.join(context),
                'field': field, 'target': target, 'object_path': path,
                'export': export_file, 'line': index})
    return found


def native_kind(cdo):
    for name, role in TRAVERSE_NATIVE.items():
        cls = getattr(ue, name, None)
        if cls is not None and isinstance(cdo, cls):
            return role
    return None


def property_value(obj, name, unresolved, required=False):
    try:
        return obj.get_editor_property(name)
    except Exception as error:
        unresolved.append({'type': 'property_unavailable', 'object': obj.get_path_name()
                           if hasattr(obj, 'get_path_name') else str(type(obj)),
                           'field': name, 'required': required, 'detail': str(error)})
        return None


def value_summary(value):
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if hasattr(value, 'get_path_name'):
        return value.get_path_name()
    if hasattr(value, 'export_text'):
        return value.export_text()
    try:
        return [value_summary(v) for v in value]
    except TypeError:
        return str(value)


def field_facts(asset, cdo, kind, unresolved):
    result = {}
    fields = {
        'pawn': ('InitialInventoryItems',), 'pawn_data': ('PawnClass', 'AbilitySets'),
        'inventory_item': ('Fragments',),
        'equipment_definition': ('InstanceType', 'AbilitySetsToGrant', 'ActorsToSpawn'),
        'weapon_instance': ('EquippedAnimSet', 'UneuippedAnimSet'),
        'ability_set': ('GrantedGameplayAbilities',),
        'experience': ('DefaultPawnData',),
    }.get(kind, ())
    for field in fields:
        result[field] = value_summary(property_value(cdo or asset, field, unresolved, True))
    # Fragment objects are instanced; their class and direct EquipmentDefinition
    # property establish the dynamic Inventory -> Equipment edge explicitly.
    if kind == 'inventory_item':
        fragments = property_value(cdo, 'Fragments', unresolved, True) or []
        result['fragment_fields'] = []
        for fragment in fragments:
            item = {'object': fragment.get_path_name(), 'class': fragment.get_class().get_path_name()}
            if 'Equippable' in fragment.get_class().get_name():
                item['EquipmentDefinition'] = value_summary(property_value(
                    fragment, 'EquipmentDefinition', unresolved, True))
            result['fragment_fields'].append(item)
    if kind == 'weapon_instance':
        for field in ('EquippedAnimSet', 'UneuippedAnimSet'):
            data = property_value(cdo, field, unresolved, True)
            if data is not None:
                result[field] = {name: value_summary(property_value(data, name, unresolved, True))
                                 for name in ('LayerRules', 'DefaultLayer')}
    return result


def inspect(directory):
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    pending = deque((path, role, None) for role, path in ROOTS.items())
    loaded, metadata, edges, unresolved, objects = {}, {}, [], [], {}
    package_evidence, field_gaps, auxiliary_dependencies = [], [], []
    registry_options = {
        kind: ue.AssetRegistryDependencyOptions(include_soft_package_references=kind == 'soft',
            include_hard_package_references=kind == 'hard', include_searchable_names=False,
            include_soft_management_references=False, include_hard_management_references=False)
        for kind in ('hard', 'soft')}

    def meta(path):
        if path not in metadata:
            rows = list(registry.get_assets_by_package_name(path, True))
            if len(rows) == 1:
                row = rows[0]
                # FTopLevelAssetPath.__str__ is a debug Struct repr in this
                # engine. Its two reflected FName values are the native path.
                class_data = row.asset_class_path
                class_package = str(class_data.get_editor_property('package_name'))
                class_name = str(class_data.get_editor_property('asset_name'))
                canonical = class_package + '.' + class_name
                assert re.fullmatch(r'/[A-Za-z0-9_/]+\.[A-Za-z0-9_]+', canonical), canonical
                metadata[path] = {'class': canonical, 'name': str(row.asset_name)}
            elif not rows:
                metadata[path] = {'class': None, 'name': path.rsplit('/', 1)[-1]}
            else:
                unresolved.append({'type': 'multiple_assets_in_package', 'package': path,
                                   'count': len(rows)})
                metadata[path] = {'class': None, 'name': path.rsplit('/', 1)[-1]}
        return metadata[path]

    while pending:
        path, role, trigger = pending.popleft()
        if path in loaded or path.startswith('/Script/'):
            continue
        assert len(loaded) < 600, 'Unexpected reference expansion; stop before more asset loads'
        data = meta(path)
        if data['class'] is None:
            unresolved.append({'type': 'missing_root_or_traversed_asset', 'package': path,
                               'role': role, 'trigger': trigger})
            continue
        asset = ue.load_asset(path)
        if not asset:
            unresolved.append({'type': 'load_failed', 'package': path, 'role': role})
            continue
        objects[path] = asset
        cls_name = asset.get_class().get_name()
        assert asset.get_class().get_path_name() == data['class'], 'Registry class differs from loaded class: ' + path
        cdo = None
        if isinstance(asset, ue.Blueprint):
            generated = asset.generated_class()
            if generated:
                cdo = ue.get_default_object(generated)
        determined = native_kind(cdo) if cdo else None
        if role not in ROOTS:
            role = determined or role
        if cls_name == 'DCAbilitySet':
            role = 'ability_set'
        entry = {'class': asset.get_class().get_path_name(), 'role': role,
                 'trigger': trigger, 'cdo_class': cdo.get_class().get_path_name() if cdo else None,
                 'facts': field_facts(asset, cdo, role, unresolved), 'exports': []}
        loaded[path] = entry
        number = len(loaded)
        raw_parts = []
        for origin, obj in [('asset', asset)] + ([('cdo', cdo)] if cdo else []):
            name = f'{number:03d}_{data["name"]}_{origin}.t3d'
            raw = export(obj, directory / name)
            entry['exports'].append(name)
            raw_parts.append(raw)
            local_edges = references(raw, path, name, origin)
            edges.extend(local_edges)
        for field, value in entry['facts'].items():
            for match in REF.finditer(json.dumps(value, ensure_ascii=False)):
                object_path = match.group(0).rstrip('\\')
                if package(object_path) != path:
                    edges.append({'source': path, 'origin': 'reflected_property',
                        'object': cdo.get_path_name() if cdo else asset.get_path_name(),
                        'field': field, 'target': package(object_path),
                        'object_path': object_path, 'export': None, 'line': None})
        raw = '\n'.join(raw_parts)
        entry['interface'] = bool(re.search(r'BlueprintType=BPTYPE_Interface', raw))
        entry['skeleton_references'] = sorted({e['target'] for e in edges
            if e['source'] == path and e['field'] in ('Skeleton', 'TargetSkeleton')
            and e['target'].startswith('/Game/')})
        # Package references are real persistent dependencies even when the
        # T3D exporter does not expose their owning field. Keep the evidence
        # kinds separate rather than inventing a property for them.
        persistent = {}
        for kind, options in registry_options.items():
            for value in registry.get_dependencies(path, options):
                target = str(value)
                if target.startswith('/Game/') and target != path:
                    persistent.setdefault(target, []).append(kind)
        field_targets = {e['target'] for e in edges if e['source'] == path}
        for target, kinds in sorted(persistent.items()):
            target_meta = meta(target)
            name = (target_meta['class'] or '').rsplit('.', 1)[-1]
            row = {'source': path, 'target': target, 'class': target_meta['class'],
                   'dependency_kinds': kinds, 'field_evidence': target in field_targets}
            package_evidence.append(row)
            if target not in field_targets:
                edge = {'source': path, 'origin': 'asset_registry_package_reference',
                    'object': None, 'field': None, 'target': target, 'object_path': None,
                    'export': None, 'line': None, 'field_evidence': False,
                    'dependency_kinds': kinds}
                edges.append(edge)
                if name in ANIMATION_CLASSES | {'ControlRigBlueprint'} and role != 'source_mesh':
                    field_gaps.append(dict(row, detail='Persistent package reference is proved; owning property is not exposed by T3D'))
        targets = {edge['target'] for edge in edges if edge['source'] == path and edge['target'] != path}
        for target in sorted(targets):
            if not target.startswith('/Game/'):
                continue
            target_meta = meta(target)
            name = (target_meta['class'] or '').rsplit('.', 1)[-1]
            reasons = [edge for edge in edges if edge['source'] == path and edge['target'] == target]
            # Manny is a proportions/reference mesh, not the current bot's
            # body. Its preview/post-process references are not automatically
            # active bot animation roots.
            auxiliary = role == 'source_mesh'
            if auxiliary:
                if name in ANIMATION_CLASSES | {'ControlRigBlueprint', 'Blueprint'}:
                    auxiliary_dependencies.append({'source': path, 'target': target,
                        'class': target_meta['class'], 'reason': 'Reference/proportions Manny mesh only, not an active bot animation root'})
                continue
            if name in ANIMATION_CLASSES or name == 'ControlRigBlueprint':
                pending.append((target, 'animation' if name in ANIMATION_CLASSES else 'control_rig', reasons[0]))
            elif name in {'DCAbilitySet', 'DCPawnData'}:
                pending.append((target, 'ability_set' if name == 'DCAbilitySet' else 'pawn_data', reasons[0]))
            elif name == 'Blueprint':
                # Includes actual Blueprint helpers/parents and ActorsToSpawn,
                # so a selector cannot hide another body montage behind a BP.
                pending.append((target, 'dynamic_blueprint', reasons[0]))
            elif target_meta['class'] is None:
                unresolved.append({'type': 'unresolved_game_reference', 'source': path,
                                   'target': target, 'evidence': reasons[0]})

    relevant_classes = ANIMATION_CLASSES | {'ControlRigBlueprint', 'Blueprint', 'DCAbilitySet', 'DCPawnData'}
    missing_closure = [row for row in package_evidence
        if row['source'] != ROOTS['source_mesh'] and
        (row['class'] or '').rsplit('.', 1)[-1] in relevant_classes and row['target'] not in loaded]
    for row in missing_closure:
        unresolved.append(dict(row, type='persistent_dependency_not_inspected'))

    initial_items = loaded[ROOTS['pawn']]['facts']['InitialInventoryItems']
    assert isinstance(initial_items, list) and initial_items, 'Current pawn has no initial inventory evidence'
    dynamic_chains = []
    for item_path in initial_items:
        item_package = package(item_path)
        item = loaded.get(item_package)
        assert item and item['role'] == 'inventory_item', 'Initial item was not inspected: ' + item_package
        equippable = [fragment for fragment in item['facts']['fragment_fields']
                      if fragment.get('EquipmentDefinition')]
        assert equippable, 'Initial weapon has no EquipmentDefinition: ' + item_package
        for fragment in equippable:
            equipment_package = package(fragment['EquipmentDefinition'])
            equipment = loaded.get(equipment_package)
            assert equipment and equipment['role'] == 'equipment_definition', 'Equipment closure is missing: ' + equipment_package
            instance_package = package(equipment['facts']['InstanceType'])
            instance = loaded.get(instance_package)
            assert instance and instance['role'] == 'weapon_instance', 'Weapon instance closure is missing: ' + instance_package
            assert instance['facts'].get('EquippedAnimSet') is not None
            assert instance['facts'].get('UneuippedAnimSet') is not None
            dynamic_chains.append({'initial_item': item_package, 'equipment': equipment_package,
                                   'weapon_instance': instance_package})
    loaded_classes = Counter(e['class'].rsplit('.', 1)[-1] for e in loaded.values())
    assert loaded_classes['AnimBlueprint'] >= 2 and loaded_classes['AnimSequence'] > 0, 'Animation reference traversal did not reach layers/sequences'

    plans, reuse, subobjects = [], [], []
    references_by_target = {}
    for edge in edges:
        references_by_target.setdefault(edge['target'], []).append(edge)
        if edge['object_path'] and (':FastFeet' in edge['object_path'] or ':UpperBody' in edge['object_path']):
            subobjects.append(edge)
    body_skeletons = {
        '/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin': 'Lyra Manny/Quinn body',
        '/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_Skeleton': 'Current HARU body',
        ROOTS['target_mesh'] + '_Skeleton': 'Inspector body'}
    weapon_skeletons = {
        '/Game/Weapons/Rifle/Mesh/SK_Rifle_Skeleton': 'Lyra rifle weapon',
        '/Game/Weapons/Pistol/Mesh/SK_Pistol_Skeleton': 'Lyra pistol weapon'}
    selected, classifications = {}, {}
    for path in sorted(set(metadata) | set(loaded)):
        data = meta(path)
        name = (data['class'] or '').rsplit('.', 1)[-1]
        entry = loaded.get(path, {})
        role = entry.get('role')
        reason, action = None, None
        skeletons = entry.get('skeleton_references', [])
        auxiliary_only = bool(references_by_target.get(path)) and all(
            edge['source'] == ROOTS['source_mesh'] for edge in references_by_target[path]) and path not in loaded
        skeleton_kind = 'not_animation'
        if name in ANIMATION_CLASSES:
            if entry.get('interface'):
                skeleton_kind = 'animation_interface'
            elif auxiliary_only:
                skeleton_kind = 'auxiliary_reference_only'
            elif skeletons and all(s in body_skeletons for s in skeletons):
                skeleton_kind = 'character_body'
            elif skeletons and all(s in weapon_skeletons for s in skeletons):
                skeleton_kind = 'weapon'
            else:
                skeleton_kind = 'unknown'
                unresolved.append({'type': 'animation_skeleton_not_classified',
                    'package': path, 'class': data['class'], 'skeletons': skeletons})
            classifications[path] = {'kind': skeleton_kind, 'skeletons': skeletons,
                'labels': {s: body_skeletons.get(s) or weapon_skeletons.get(s) or 'unknown' for s in skeletons}}
        if name in ANIMATION_CLASSES and skeleton_kind == 'character_body':
            action, reason = 'copy_retarget', 'Character-body animation/AnimBP requires Inspector skeleton; original preserved'
        elif name == 'ControlRigBlueprint' and not auxiliary_only:
            action, reason = 'copy_adapt', 'Bone hierarchy and proportions need Inspector-specific verification/mapping'
        elif role in {'pawn', 'pawn_data', 'inventory_item', 'equipment_definition', 'weapon_instance'}:
            action, reason = 'copy_adapt', 'Separate bot-only references without changing the player loadout or shared weapon'
        elif name in {'Skeleton', 'PhysicsAsset'} and path.startswith(ROOTS['target_mesh'].rsplit('/', 1)[0] + '/'):
            action, reason = 'copy_adapt', 'Private Inspector rig/hit zones may need additions; original remains protected'
        elif path == ROOTS['target_mesh']:
            action, reason = 'copy_adapt', 'Private bot mesh binds private rig/physics; original geometry/weights/material preserved'
        if action:
            selected[path] = {'action': action, 'reason': reason, 'selection': 'seed'}

    # Retargeted montages are not enough: their GA/notify/helper selectors and
    # AbilitySets must also redirect privately. Propagate backwards through
    # actual loaded references, without mutating original shared selectors.
    redirectable = {'Blueprint', 'DCAbilitySet', 'DCPawnData', 'AnimBlueprint', 'ControlRigBlueprint'}
    changed = True
    while changed:
        changed = False
        for edge in edges:
            source, target = edge['source'], edge['target']
            if source in selected or target not in selected or source not in loaded:
                continue
            if source in (ROOTS['experience'], ROOTS['source_mesh']):
                continue
            entry = loaded[source]
            if entry['interface'] or entry['class'].rsplit('.', 1)[-1] not in redirectable:
                continue
            selected[source] = {'action': 'copy_adapt', 'selection': 'reverse_reference_closure',
                'reason': 'Private selector/parent/helper must redirect a copied dependency while preserving gameplay behavior',
                'closure_evidence': edge}
            changed = True

    for path in sorted(set(metadata) | set(loaded)):
        data = meta(path)
        name = (data['class'] or '').rsplit('.', 1)[-1]
        entry = loaded.get(path, {})
        role = entry.get('role')
        classification = classifications.get(path, {})
        skeleton_kind = classification.get('kind')
        if path in selected:
            short = path.rsplit('/', 1)[-1]
            plans.append({'source': path,
                'target': DESTINATION + '/' + ('Animation' if name in ANIMATION_CLASSES else 'Assets') + '/' + short,
                'class': data['class'], 'role': role, **selected[path],
                'status': 'proposed_not_created', 'skeleton_classification': classification,
                'evidence_count': len(references_by_target.get(path, []))})
        else:
            reason = 'No Inspector-specific source change proposed'
            if entry.get('interface'):
                reason = 'Interface has no pose motion; later Inspector linked-layer compile compatibility remains unverified'
            elif skeleton_kind == 'weapon':
                reason = 'Animation drives a weapon skeleton, not the character body; retain original weapon montage/sequence'
            elif skeleton_kind == 'auxiliary_reference_only':
                reason = 'Reference Manny preview/post-process only, not an active bot animation root'
            elif skeleton_kind == 'unknown':
                reason = 'Skeleton classification unresolved; neither reuse nor copy is yet finalized'
            reuse.append({'source': path, 'class': data['class'], 'role': role,
                'action': 'unresolved' if skeleton_kind == 'unknown' else
                          'reuse_pending_compatibility' if entry.get('interface') else 'reuse',
                'reason': reason, 'skeleton_classification': classification,
                'evidence_count': len(references_by_target.get(path, []))})
    groups = {}
    for plan in plans:
        groups.setdefault(plan['target'].casefold(), []).append(plan)
    collisions = []
    for values in groups.values():
        if len(values) > 1:
            for plan in values:
                before = plan['target']
                plan['target'] += '_' + hashlib.sha256(plan['source'].encode('utf-8')).hexdigest()[:8]
                collisions.append({'source': plan['source'], 'original_collision': before,
                                   'disambiguated_target': plan['target']})
    assert len({p['target'].casefold() for p in plans}) == len(plans)
    assert all(p['target'].startswith(DESTINATION + '/') for p in plans)
    plan_by_source = {p['source']: p for p in plans}
    inspector_skeleton = ROOTS['target_mesh'] + '_Skeleton'
    inspector_physics = ROOTS['target_mesh'] + '_PhysicsAsset'
    assert all(p in plan_by_source for p in (ROOTS['target_mesh'], inspector_skeleton, inspector_physics)), 'Private Inspector mesh/skeleton/physics plan is incomplete'

    def future_object(old, target_package):
        if not old:
            return None
        suffix = old.split(':', 1)[1] if ':' in old else None
        object_name = old.split(':', 1)[0].split('.', 1)[1] if '.' in old.split(':', 1)[0] else ''
        name = target_package.rsplit('/', 1)[-1]
        if object_name.startswith('Default__'):
            name = 'Default__' + name
        if object_name.endswith('_C'):
            name += '_C'
        return target_package + '.' + name + (':' + suffix if suffix else '')

    redirects = []
    for edge in edges:
        source, target = edge['source'], edge['target']
        if source not in plan_by_source:
            continue
        if target in plan_by_source:
            target_package = plan_by_source[target]['target']
            reason = 'Redirect only within the new private asset; original reference remains unchanged'
            kind = 'copied_dependency'
        elif target in body_skeletons:
            target_package = plan_by_source[inspector_skeleton]['target']
            reason = 'Retarget copied character animation/profile reference to the private Inspector skeleton; bone/profile correspondence requires validation'
            kind = 'character_skeleton_or_subobject'
        else:
            continue
        redirects.append({'source_asset': source, 'new_asset': plan_by_source[source]['target'],
            'source_reference': target, 'new_reference': target_package,
            'source_object_path': edge['object_path'],
            'new_object_path': future_object(edge['object_path'], target_package),
            'field': edge['field'], 'origin': edge['origin'], 'object': edge['object'],
            'export': edge['export'], 'line': edge['line'],
            'dependency_kinds': edge.get('dependency_kinds'),
            'field_evidence': edge['origin'] != 'asset_registry_package_reference',
            'kind': kind, 'reason': reason, 'status': 'planned_only_not_applied'})
    for plan in plans:
        own = [row for row in redirects if row['source_asset'] == plan['source']]
        plan['redirect_count'] = len(own)
        if plan['role'] in ('gameplay_ability', 'ability_set'):
            assert own, 'Copied GA/AbilitySet has no concrete redirect evidence: ' + plan['source']
    activation = {'experience': ROOTS['experience'], 'existing_field': 'DefaultPawnData',
        'preserve_existing_reference': ROOTS['pawn_data'],
        'future_bot_override_field': 'BotPawnData',
        'future_reference': plan_by_source[ROOTS['pawn_data']]['target'],
        'status': 'requires_separately_approved_runtime_field_and_asset_connection',
        'reason': 'Only ordinary bots receive the Inspector PawnData; player and boss keep existing selection'}
    existing = [p['target'] for p in plans if ue.EditorAssetLibrary.does_asset_exist(p['target'])]
    assert not existing, 'Proposed destination already exists; no overwrite allowed: ' + str(existing)
    unique_edges = {(e['source'], e['origin'], e['object'], e['field'], e['object_path'], e['line']): e for e in edges}
    return {'roots': ROOTS, 'loaded_assets': loaded, 'metadata': metadata,
        'dynamic_inventory_chains': dynamic_chains,
        'edges': list(unique_edges.values()),
        'registry_package_dependencies': package_evidence,
        'registry_only_edges': [row for row in package_evidence if not row['field_evidence']],
        'unverified_reference_fields': field_gaps,
        'excluded_reference_mesh_dependencies': auxiliary_dependencies,
        'skeleton_classifications': classifications,
        'reference_redirect_plan': redirects, 'bot_activation_plan': activation,
        'skeleton_subobject_edges': subobjects, 'copy_adapt_plan': plans, 'reuse': reuse,
        'resolved_name_collisions': collisions, 'destination_exists': existing,
        'counts': {'loaded': len(loaded),
            'field_edges': sum(e['origin'] != 'asset_registry_package_reference' for e in unique_edges.values()),
            'registry_package_edges': len(package_evidence),
            'unverified_animation_reference_fields': len(field_gaps),
            'reverse_reference_copy_assets': sum(p['selection'] == 'reverse_reference_closure' for p in plans),
            'reference_redirects': len(redirects),
            'proposed_copy_adapt': len(plans), 'reuse': len(reuse),
            'copy_by_class': dict(Counter(p['class'] for p in plans)),
            'loaded_by_role': dict(Counter(e['role'] for e in loaded.values()))},
        'unresolved': unresolved,
        'persistent_package_closure_complete': not missing_closure and not any(
            row.get('type') in ('missing_root_or_traversed_asset', 'load_failed', 'unresolved_game_reference') for row in unresolved),
        'direct_field_proof_complete': not field_gaps,
        'exact_list_complete': not unresolved and not missing_closure,
        'limits': ['No retargeted pose was evaluated', 'No game asset was created or saved',
            'Persistent AssetRegistry hard/soft package closure and owning-field proof are distinct; field gaps stay explicitly reported',
            'Runtime graph execution and soft path construction are not proved by export/package closure',
            'Plans are candidates for a later explicit approval, not implementation authorization']}


def memory(directory, reference):
    """Check only public IK controller definitions in the transient package."""
    assert reference['complete'] and reference['protection_passed']
    assert reference['mode'] == 'inspect' and reference['script_sha256'] == sha(SCRIPT)
    assert reference['manifest']['roots'] == ROOTS
    assert reference['manifest']['loaded_assets'], 'Inspect did not produce asset evidence'
    source = ue.load_asset(ROOTS['source_mesh'])
    target = ue.load_asset(ROOTS['target_mesh'])
    assert source and target and source != target
    originals = {'source_mesh': source, 'target_mesh': target,
        'source_skeleton': ue.load_asset('/Game/Characters/Heroes/Mannequin/Meshes/SK_Mannequin'),
        'target_skeleton': ue.load_asset(ROOTS['target_mesh'] + '_Skeleton')}
    assert all(originals.values())
    before = {name: export(obj, directory / (name + '_before.t3d'))
              for name, obj in originals.items()}

    # Explicitly map common humanoid chains. 61-bone Inspector has three spine
    # bones/one neck bone; no fake names, added bones or changed parents are used.
    chains = [('Root', 'root', 'root', 'root', 'root'),
        ('Spine', 'spine_01', 'spine_05', 'spine_01', 'spine_03'),
        ('Neck', 'neck_01', 'neck_02', 'neck_01', 'neck_01'),
        ('Head', 'head', 'head', 'head', 'head')]
    for side in ('l', 'r'):
        chains.extend([
            ('Clavicle_' + side, 'clavicle_' + side, 'clavicle_' + side,
                'clavicle_' + side, 'clavicle_' + side),
            ('Arm_' + side, 'upperarm_' + side, 'hand_' + side,
                'upperarm_' + side, 'hand_' + side),
            ('Leg_' + side, 'thigh_' + side, 'foot_' + side,
                'thigh_' + side, 'foot_' + side),
            ('Foot_' + side, 'foot_' + side, 'ball_' + side,
                'foot_' + side, 'ball_' + side)])
        for digit in ('thumb', 'index', 'middle', 'ring', 'pinky'):
            chains.append((digit.title() + '_' + side,
                digit + '_01_' + side, digit + '_03_' + side,
                digit + '_01_' + side, digit + '_03_' + side))
    assert len(chains) == 22 and len({c[0] for c in chains}) == 22

    def hierarchy(mesh):
        todo, parents = deque(['root']), {'root': 'None'}
        while todo:
            bone = todo.popleft()
            for child_name in ue.SkeletalMeshEditorSubsystem.get_bone_children(mesh, bone):
                child = str(child_name)
                assert child not in parents, 'Cycle/repeated bone: ' + child
                parents[child] = bone
                todo.append(child)
        assert 'pelvis' in parents and 'hand_l' in parents and 'foot_r' in parents
        return parents

    hierarchies = [hierarchy(source), hierarchy(target)]
    assert len(hierarchies[1]) == 61, 'Inspector bone structure changed since review'
    for chain in chains:
        for hierarchy_map, start, end in ((hierarchies[0], chain[1], chain[2]),
                                          (hierarchies[1], chain[3], chain[4])):
            assert start in hierarchy_map and end in hierarchy_map, 'Missing endpoint: ' + str(chain)
            ancestor = end
            while ancestor != start and ancestor != 'None':
                ancestor = hierarchy_map[ancestor]
            assert ancestor == start, 'Chain endpoint is not a descendant: ' + str(chain)

    created = [ue.new_object(ue.IKRigDefinition, name='InspectorPreflightSource'),
               ue.new_object(ue.IKRigDefinition, name='InspectorPreflightTarget'),
               ue.new_object(ue.IKRetargeter, name='InspectorPreflightRetargeter')]
    assert all(o.get_outer().get_path_name() == '/Engine/Transient' for o in created)
    source_rig, target_rig, retargeter = created
    controllers = [ue.IKRigController.get_controller(source_rig),
                   ue.IKRigController.get_controller(target_rig)]
    assert all(controllers)
    for index, (controller, mesh) in enumerate(zip(controllers, (source, target))):
        assert controller.set_skeletal_mesh(mesh)
        assert controller.get_skeletal_mesh() == mesh
        assert controller.is_skeletal_mesh_compatible(mesh)
        assert controller.set_retarget_root('pelvis')
        assert str(controller.get_retarget_root()) == 'pelvis'
        assert controller.set_root_motion_bone('root')
        assert str(controller.get_root_motion_bone()) == 'root'
        for name, s_start, s_end, t_start, t_end in chains:
            start, end = (s_start, s_end) if index == 0 else (t_start, t_end)
            assert str(controller.add_retarget_chain(name, start, end, 'None')) == name
            assert str(controller.get_retarget_chain_start_bone(name)) == start
            assert str(controller.get_retarget_chain_end_bone(name)) == end
            assert str(controller.get_retarget_chain_goal(name)) == 'None'
        assert len(controller.get_retarget_chains()) == len(chains)

    controller = ue.IKRetargeterController.get_controller(retargeter)
    assert controller
    source_side, target_side = ue.RetargetSourceOrTarget.SOURCE, ue.RetargetSourceOrTarget.TARGET
    controller.set_ik_rig(source_side, source_rig)
    controller.set_ik_rig(target_side, target_rig)
    controller.set_preview_mesh(source_side, source)
    controller.set_preview_mesh(target_side, target)
    assert controller.get_ik_rig(source_side) == source_rig
    assert controller.get_ik_rig(target_side) == target_rig
    assert controller.get_preview_mesh(source_side) == source
    assert controller.get_preview_mesh(target_side) == target
    controller.add_default_ops()
    assert controller.get_num_retarget_ops() == 5, 'Expected 5 documented default ops'
    controller.assign_ik_rig_to_all_ops(source_side, source_rig)
    controller.assign_ik_rig_to_all_ops(target_side, target_rig)
    mapping = []
    for name, s_start, s_end, t_start, t_end in chains:
        assert controller.set_source_chain(name, name)
        actual = str(controller.get_source_chain(name))
        assert actual == name, 'Retarget mapping mismatch: ' + name
        mapping.append({'name': name, 'source_bones': [s_start, s_end],
                        'target_bones': [t_start, t_end], 'read_source_chain': actual})
    ops = [{'index': i, 'name': str(controller.get_op_name(i)),
            'enabled': controller.get_retarget_op_enabled(i)} for i in range(5)]
    assert all(o['enabled'] for o in ops)
    transient_exports = []
    for name, obj in zip(('source_rig', 'target_rig', 'retargeter'), created):
        filename = name + '_transient.t3d'
        export(obj, directory / filename)
        transient_exports.append(filename)
    for name, obj in originals.items():
        assert export(obj, directory / (name + '_after.t3d')) == before[name], 'Original memory data changed: ' + name
    return {'transient_objects': [o.get_path_name() for o in created],
        'transient_object_count': 3, 'source_bone_count': len(hierarchies[0]),
        'target_bone_count': len(hierarchies[1]), 'chain_count_per_rig': len(chains),
        'mapping': mapping, 'ops': ops, 'exports': transient_exports,
        'original_mesh_and_skeleton_exports_equal': True,
        'configuration_verified': True, 'retargeted_pose_verified': False,
        'batch_retarget_executed': False, 'animation_duplicates': 0,
        'limits': ['No IK goals/solver was authored; only explicit retarget definitions and mappings were checked',
            'No pose was evaluated and no motion/hand contact/FootPlant quality was verified',
            'FIKRetargetProcessor.Initialize/RunRetargeter are C++ only; Python configuration does not prove pose conversion',
            'AnimationBlueprintLibrary input-pose evaluation has a same-skeleton contract and is not a cross-skeleton substitute',
            'A scoped Editor C++ helper and separate approval would be needed for an automatic no-save evaluated-pose probe',
            'RunBatchRetarget is not used because no-save behavior has not been established']}


def main():
    global ue
    import unreal as ue
    command = ue.SystemLibrary.get_command_line()
    mode = argument(command, 'DCInspectorMode', 'inspect|memory|prepare|verify|activate|verify_activation')
    if mode not in ('inspect', 'memory'):
        integration_main(command, mode)
        return
    run = argument(command, 'DCInspectorRun', '[A-Za-z0-9_]{1,64}')
    frozen_sha = argument(command, 'DCInspectorSHA', '[a-f0-9]{64}')
    assert sha(SCRIPT) == frozen_sha, 'Script changed after command preparation'
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled(), 'Source Control must already be disabled'
    baseline = read(BASELINE)
    assert baseline['script_scope'] == [SCRIPT.relative_to(P).as_posix()]
    assert baseline['asset_saves_allowed'] == 0 and baseline['PIE'] is False and baseline['cpp_build'] is False
    initial_protection = protection(baseline)
    helpers = module('ingame_hud_layout_setup')
    directory = R / run
    directory.mkdir(parents=True, exist_ok=True)
    assert not (directory / 'report.json').exists() and all(p.name == 'Editor.log' for p in directory.iterdir()), 'Use a fresh run'
    expected_log = directory / 'Editor.log'
    log_arg = re.findall(r'(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(log_arg) == 1 and Path(log_arg[0][0] or log_arg[0][1]).resolve() == expected_log.resolve()
    report = {'schema': 1, 'mode': mode, 'run': run, 'script_sha256': frozen_sha,
        'complete': False, 'protection_passed': False, 'asset_saves': 0,
        'blueprint_compiles_requested': 0, 'PIE': False, 'cpp_build': False,
        'retargeted_pose_verified': False, 'source_mutations_requested': 0}
    try:
        report['before_protection'] = initial_protection
        helpers.assert_no_other_editor()
        helpers.clean()
        settings_cls = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings_cls
        settings = export(ue.get_default_object(settings_cls), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings, re.M) == ['SoC_Never'], 'SaveOnCompile must already be Never'
        if mode == 'inspect':
            assert '-DCInspectorReference=' not in command
            report['manifest'] = inspect(directory)
        else:
            previous = argument(command, 'DCInspectorReference', '[A-Za-z0-9_]{1,64}')
            assert previous != run
            report['reference'] = previous
            report['memory'] = memory(directory, read(R / previous / 'report.json'))
        helpers.clean()
        helpers.assert_no_other_editor()
        helpers.log_check(expected_log)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            assert sha(SCRIPT) == frozen_sha, 'Frozen script changed during execution'
            report['after_protection'] = protection(baseline)
            report['protection_passed'] = True
        except Exception:
            report['protection_error'] = traceback.format_exc()
            report['complete'] = False
            raise
        finally:
            persist(directory / 'report.json', report)


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        commands(sys.argv[2])
    else:
        main()
