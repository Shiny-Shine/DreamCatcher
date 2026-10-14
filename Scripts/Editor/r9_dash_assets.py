"""Approved, fixed 24-asset Dash port; UE-only writes, no shared saves/builds.

Run source prepare/migrate and target prepare/verify separately. Target preparation
uses process-only exact package aliases; final verification must not use aliases.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r7_root_ui_setup as helper
import r7_quickbar_setup as files_helper
import r7_quickbar_target as normalizer

P = Path(__file__).resolve().parents[2]
S = P.parent / 'LyraStarterGame'
R = P / 'Saved/Diagnostics/R9'
manifest = json.loads((R / 'reuse_target_A/report.json').read_text())
MAPPING, REUSE = manifest['new'], manifest['reuse']
assert len(MAPPING) == 24 and len(REUSE) == 200
assert all(d == '/Game/LyraMigration/Dash/' + p.rsplit('/', 1)[-1] for p, d in MAPPING.items())
scope = dict(unreal=ue, Path=Path, hashlib=hashlib, re=re)
tree = ast.parse((P / 'Scripts/Editor/r5_rifle_shared_audit.py').read_text(encoding='utf-8-sig'))
exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in
    ('require', 'encode', 'reflected_fields', 'export_report')], type_ignores=[]), 'existing_exports', 'exec'), scope)


def mapped(value, mapping):
    text = json.dumps(value, sort_keys=True)
    for a, b in sorted(mapping.items(), key=lambda x: -len(x[0])):
        text = text.replace(a, b)
    return json.loads(text)


def data(obj, out, name):
    row = dict(cls=obj.get_class().get_path_name(), fields=scope['reflected_fields'](obj)['values'])
    if isinstance(obj, ue.Blueprint):
        row['snapshot'] = helper.snapshot(obj)
        cdo = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(obj))
        row['defaults'] = scope['reflected_fields'](cdo)['values']
        if obj.get_name() == 'GA_Hero_Dash':
            row['dash'] = {n: scope['encode'](cdo.get_editor_property(n)) for n in (
                'Strength', 'RootMotionDuration', 'AbilityDuration', 'WidgetClasses', 'WidgetExtensionPointTags')}
        info = scope['export_report'](cdo, out / (name + '_cdo.t3d'), ue.ObjectExporterT3D())
        assert info['success']
        row['cdo_text'] = Path(info['path']).read_text(encoding='utf-8-sig')
    return row


def target_normalize(value):
    value = normalizer.normalize(value, dict(REUSE, **MAPPING))
    text = json.dumps(value, sort_keys=True)
    for old, new in (
        ('Lyra Health Component', 'DCLyra Health Component'),
        ('Lyra Team Display Asset', 'DCTeam Display Asset'),
        ('Lyra Interaction Duration Message', 'DCInteraction Duration Message'),
        ('LyraInteractionDurationMessage', 'DCInteractionDurationMessage'),
        ('Lyra Gameplay Ability', 'DCGameplay Ability'),
        ('Lyra Character', 'Dream Catcher Character'),
        ('GetLyraCharacterFromActorInfo', 'GetDCCharacterFromActorInfo'),
        ('Lyra Camera Component', 'DCCamera Component'),
        ('<LyraAbilityActivationPolicy.', '<DCAbilityActivationPolicy.'),
        ('<LyraAbilityActivationGroup.', '<DCAbilityActivationGroup.'),
        ('"CameraComponent"', '"ModeCamera"'),
        ('"Get CameraComponent"', '"Get ModeCamera"'),
    ):
        text = text.replace(old, new)
    return json.loads(text)


def comparable(value):
    # Only observed editor-generated identity differs; never omit gameplay values.
    if isinstance(value, dict):
        return {k: comparable(v) for k, v in value.items()}
    if isinstance(value, list):
        return [comparable(v) for v in value]
    if not isinstance(value, str):
        return value
    value = re.sub(r"(Multicast delegate '[^']+' \()0x[0-9A-Fa-f]+(\) <Unbound>)", r'\1ADDRESS\2', value)
    value = re.sub(r'/Engine/Transient\.(AnimDataController|AnimSequencerController)_\d+', r'/Engine/Transient.\1_ID', value)
    value = re.sub(r'(DashDeformTimeline_(?:Deform|RibbonTrail|linear|_Direction)_)[0-9A-F]{32}', r'\1GUID', value)
    value = re.sub(r'(ClassName=\(Namespace="",Name=")[0-9A-F]{32}(",Variant=""\),Version=\(Major=1,Minor=0\),Type=Graph)', r'\1GUID\2', value)
    value = re.sub(r'NSLOCTEXT\("\[[0-9A-F]{32}\]", "[0-9A-F]{32}", ("(?:Audio Wave Assets|Output Gain Modifier)")\)', r'NSLOCTEXT("NAMESPACE", "KEY", \1)', value)
    return value


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR9DashAssets=(prepare|source_verify|migrate|target_inspect|target_prepare|verify)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
    source = mode in ('prepare', 'source_verify', 'migrate')
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == (S if source else P).resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    helper.LOG = P / 'Saved/Logs' / ('R9Dash-' + run + '.log')
    out = R / ('assets_' + run)
    assert not out.exists()
    out.mkdir()
    original = json.loads((R / 'source_B/report.json').read_text())
    protected = dict(original['protected'])
    for p, h in protected.items():
        assert helper.digest(Path(p)) == h, 'Original changed since review: ' + p
    for dest in REUSE.values():
        for p in files_helper.files(P, dest):
            protected[str(p)] = helper.digest(p)
    for root in (P / 'Config', P / 'Content/DreamCatcher/GAS/Test/R5/Integration'):
        for p in root.rglob('*'):
            if p.is_file():
                protected[str(p)] = helper.digest(p)
    for p in (P / '.git/index', P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'):
        protected[str(p)] = helper.digest(p)
    if mode in ('target_inspect', 'verify'):
        if mode == 'verify':
            assert 'PackageRedirects=' not in cmd
        for dest in MAPPING.values():
            for p in files_helper.files(P, dest):
                protected[str(p)] = helper.digest(p)
    result = dict(mode=mode, complete=False, asset_saves=0, mapping=MAPPING, reuse=REUSE, protected=protected)
    editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    try:
        helper.clean()
        if mode == 'prepare':
            assert all(not p.exists() for d in MAPPING.values() for root in (S, P) for p in files_helper.files(root, d))
            originals = {p: ue.load_asset(p) for p in MAPPING}
            assert all(originals.values())
            ue.collect_garbage()
            result['source_data'] = {p: data(o, out, 'source_' + o.get_name()) for p, o in originals.items()}
            copies = {p: editor.duplicate_loaded_asset(o, MAPPING[p]) for p, o in originals.items()}
            assert all(copies.values())
            ue.collect_garbage()
            result['copy_data'] = {p: data(o, out, 'copy_' + o.get_name()) for p, o in copies.items()}
            result['differences'] = {}
            for p, row in result['copy_data'].items():
                expected = mapped(result['source_data'][p], {p: MAPPING[p]})
                if comparable(row) != comparable(expected):
                    result['differences'][p] = [k for k in row if comparable(row[k]) != comparable(expected[k])]
            assert not result['differences'], 'Copy differences require review: ' + repr(result['differences'])
            helper.clean()
            for obj in copies.values():
                assert editor.save_loaded_asset(obj, False)
                result['asset_saves'] += 1
            result['prepared_hashes'] = {str(p): helper.digest(p) for d in MAPPING.values() for p in files_helper.files(S, d)}
        elif mode == 'source_verify':
            baseline = json.loads((R / 'assets_Prepare_B/report.json').read_text())
            assert baseline['complete'] and baseline['asset_saves'] == 24
            assert all(helper.digest(Path(p)) == h for p, h in baseline['prepared_hashes'].items())
            protected.update(baseline['prepared_hashes'])
            objects = {p: ue.load_asset(d) for p, d in MAPPING.items()}
            assert all(objects.values())
            ue.collect_garbage()
            result['actual_data'] = {p: data(o, out, 'reload_' + o.get_name()) for p, o in objects.items()}
            result['differences'] = {}
            for p, row in result['actual_data'].items():
                expected = baseline['copy_data'][p]
                if comparable(row) != comparable(expected):
                    result['differences'][p] = [k for k in row if comparable(row[k]) != comparable(expected[k])]
            assert not result['differences'], repr(result['differences'])
        elif mode == 'migrate':
            baseline = json.loads((R / 'assets_Prepare_B/report.json').read_text())
            assert baseline['complete'] and baseline['asset_saves'] == 24
            assert all(helper.digest(Path(p)) == h for p, h in baseline['prepared_hashes'].items())
            assert all(not p.exists() for d in MAPPING.values() for p in files_helper.files(P, d))
            assert ue.SystemLibrary.get_console_variable_int_value('AssetTools.UseNewPackageMigration') == 0
            opts = ue.MigrationOptions()
            opts.set_editor_property('prompt', False)
            opts.set_editor_property('ignore_dependencies', True)
            opts.set_editor_property('asset_conflict', ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(MAPPING.values()), str(P / 'Content'), opts)
            assert all(files_helper.files(P, d)[0].is_file() for d in MAPPING.values())
            result['migrated'] = 24
        else:
            baseline = json.loads((R / 'assets_Prepare_B/report.json').read_text())
            assert baseline['complete']
            objects = {p: ue.load_asset(d) for p, d in MAPPING.items()}
            assert all(objects.values())
            for obj in objects.values():
                if isinstance(obj, ue.Blueprint):
                    assert ue.BlueprintEditorLibrary.compile_blueprint(obj)
            if mode == 'target_prepare':
                # Original CDO true; target engine/parent defaults false. Preserve
                # the original value explicitly on this new Cue only.
                cue = objects['/ShooterCore/GameplayCues/GCNL_Dash']
                ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(cue)).set_editor_property(
                    'replicate_using_registered_sub_object_list', True)
            ue.collect_garbage()
            helper.clean()
            result['actual_data'] = {p: data(o, out, 'target_' + o.get_name()) for p, o in objects.items()}
            result['differences'] = {}
            for p, row in result['actual_data'].items():
                expected = target_normalize(baseline['source_data'][p])
                if comparable(row) != comparable(expected):
                    result['differences'][p] = [k for k in row if comparable(row[k]) != comparable(expected[k])]
            if mode != 'target_inspect':
                assert not result['differences'], 'Target differences require review: ' + repr(result['differences'])
            if mode == 'target_prepare':
                for obj in objects.values():
                    assert editor.save_loaded_asset(obj, False)
                    result['asset_saves'] += 1
            if mode == 'verify':
                registry = ue.AssetRegistryHelpers.get_asset_registry()
                registry.search_all_assets(True)
                opts = ue.AssetRegistryDependencyOptions(include_hard_package_references=True,
                    include_soft_package_references=True, include_searchable_names=False,
                    include_soft_management_references=False, include_hard_management_references=False)
                result['package_references'] = {}
                aliases = dict(REUSE, **MAPPING)
                for old, dest in MAPPING.items():
                    expected = sorted(aliases.get(p, p) for p in original['dependencies'][old] if p.startswith(('/Game/', '/ShooterCore/')))
                    actual = sorted(str(p) for p in registry.get_dependencies(dest, opts) if str(p).startswith(('/Game/', '/ShooterCore/')))
                    result['package_references'][old] = dict(expected=expected, actual=actual)
                assert all(r['expected'] == r['actual'] for r in result['package_references'].values()), 'Project package references differ'
        helper.clean()
        result['complete'] = True
    finally:
        result['protected_unchanged'] = all(helper.digest(Path(p)) == h for p, h in protected.items())
        (out / 'report.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        assert result['protected_unchanged']
        print('R9_DASH_ASSETS ' + json.dumps({k: result[k] for k in ('mode', 'complete', 'asset_saves', 'protected_unchanged')}))


if __name__ == '__main__':
    main()
