"""Approved Inspector death assets only. Requires the user's frozen C++ build.

Run in an isolated unattended Editor, never PIE. New material copies are made by
AssetTools; only the private InventoryPawn and the two new copies may be saved.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import re
import subprocess
import sys
import traceback

P = Path(__file__).resolve().parents[2]
SCRIPT = Path(__file__).resolve()
ROOT = P / 'Saved/Diagnostics/InspectorDeath20261023'
FREEZE = ROOT / 'cpp-freeze-review.json'
BUILD = ROOT / 'user-build-success.json'
TESTS = ROOT / 'isolated-tests-reviewed.json'
PAWN = '/Game/DreamCatcher/AI/Inspector/Assets/BP_DC_LyraInventoryPawn'
MASTER = '/Game/DreamCatcher/AI/Inspector/Materials/M_Inspector_Death'
INSTANCE = '/Game/DreamCatcher/AI/Inspector/Materials/MI_Inspector_Death'
SOURCE_MASTER = '/Game/DreamCatcher/DreamCatcher_MM/Basic_MM/MM_Basic'
SOURCE_INSTANCE = '/Game/DreamCatcher/Main_Charcher/Inspector/Inspector/Fbx_Default_Material_0'
EDGE = '/Game/DreamCatcher/Lyra/GameplayCues/Death/LyraStarterGame/Effects/MaterialFunctions/MF_MannequinEdge'
WRITES = {PAWN, MASTER, INSTANCE}
ue = None


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def disk(package):
    assert package.startswith('/Game/')
    return P / 'Content' / (package[6:] + '.uasset')


def load(package):
    obj = ue.load_asset(package)
    assert obj and obj.get_path_name() == package + '.' + package.rsplit('/', 1)[-1], package
    return obj


def export(obj, path):
    assert not path.exists()
    task = ue.AssetExportTask()
    task.object, task.exporter, task.filename = obj, ue.ObjectExporterT3D(), str(path)
    task.automated, task.prompt = True, False
    assert ue.Exporter.run_asset_export_task(task)
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')


def clean_log(path):
    ue.log_flush()
    raw = path.read_bytes()
    errors = [line for line in raw.decode('utf-8-sig', errors='replace').splitlines()
              if re.search(r'\bLog\w+:\s*(?:Error|Fatal):|Ensure condition failed:|=== Handled ensure:|Fatal error:|Assertion failed:', line, re.I)]
    assert not errors, '\n'.join(errors[:12])
    return {'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw)}


def scoped_dirty():
    names = {obj.get_name() for obj in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    assert names <= WRITES, 'Dirty package outside approved scope: ' + str(sorted(names - WRITES))
    assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages(), 'Map writes forbidden'
    return sorted(names)


def protection():
    frozen = read(FREEZE)
    assert all(sha(P / name) == value for name, value in frozen['approved_cpp'].items()), 'Frozen C++ changed'
    built = read(BUILD)
    assert built['user_reported_success'] is True and built['approved_cpp'] == frozen['approved_cpp']
    assert built['dll_sha256'] == sha(P / 'Binaries/Win64/UnrealEditor-DreamCatcher.dll'), 'Build DLL changed'
    result = read(TESTS)
    assert result['success'] is True and result['passed'] == 2 and result['failed'] == 0
    assert result['dll_sha256'] == built['dll_sha256']
    # The asset phase starts after the independent level-progression task finished.
    # Keep its current Config too; never restore the earlier C++-phase snapshot.
    baseline = read(ROOT / 'asset-protection-baseline.json')
    assert baseline['cpp_freeze_sha256'] == sha(FREEZE)
    assert baseline['cpp_authoring_baseline_sha256'] == sha(ROOT / 'cpp-authoring-baseline.json')
    protected = baseline['protected_files']
    assert all(name.startswith(('Content/', 'Config/')) for name in protected)
    assert disk(PAWN).relative_to(P).as_posix() not in protected
    for name, value in protected.items():
        assert sha(P / name) == value, 'Original/config changed: ' + name
    return {'protected_asset_config_files': len(protected), 'originals_unchanged': True,
            'cpp_freeze_sha256': sha(FREEZE), 'dll_sha256': built['dll_sha256'],
            'asset_protection_baseline_sha256': sha(ROOT / 'asset-protection-baseline.json')}


def component_targets(bp):
    generated = bp.generated_class()
    cdo = ue.get_default_object(generated)
    body, health = cdo.get_editor_property('Mesh'), cdo.get_editor_property('HealthComponent')
    assert isinstance(body, ue.SkeletalMeshComponent) and isinstance(health, ue.DCLyraHealthComponent)
    assert body.get_outer() == cdo and health.get_outer() == cdo
    result = {body.get_path_name(): body, health.get_path_name(): health}
    subsystem = ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
    lib = ue.SubobjectDataBlueprintFunctionLibrary
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = lib.get_data(handle)
        obj = lib.get_object_for_blueprint(data, bp)
        if obj and obj.get_outer() in (generated, cdo) and obj.get_name() in (body.get_name(), health.get_name()):
            result[obj.get_path_name()] = obj
    for obj in result.values():
        assert obj.get_path_name().startswith(PAWN + '.'), 'Inherited/shared component edit refused'
        assert isinstance(obj, (ue.SkeletalMeshComponent, ue.DCLyraHealthComponent))
    scoped_dirty()
    return cdo, list(result.values())


def pawn_snapshot(bp, directory, label):
    cdo, components = component_targets(bp)
    body, health = cdo.get_editor_property('Mesh'), cdo.get_editor_property('HealthComponent')
    # An unregistered Blueprint CDO can return None from GetMaterial even while
    # its skeletal asset has a valid default slot. Resolve the authored override
    # and asset slot explicitly instead of treating that as missing game data.
    overrides = list(body.get_editor_property('OverrideMaterials'))
    skeletal = body.get_editor_property('SkeletalMesh')
    slots = list(skeletal.get_editor_property('Materials'))
    assert len(slots) == 1 and len(overrides) <= 1
    material = overrides[0] if overrides and overrides[0] else slots[0].material_interface
    assert material
    return {'mesh_tags': [str(t) for t in body.get_editor_property('ComponentTags')],
            'material': material.get_path_name(),
            'reject_after_death': bool(health.get_editor_property('bPreventDamageAfterDeath')),
            'mesh': body.get_editor_property('SkeletalMesh').get_path_name(),
            'anim_class': body.get_editor_property('AnimClass').get_path_name(),
            'transform': body.get_relative_transform().export_text(),
            'components': {obj.get_path_name(): export(obj, directory / (label + '_component_%d.t3d' % i))
                           for i, obj in enumerate(components)}}


def parameters(material):
    values = {}
    mel = ue.MaterialEditingLibrary
    for kind in ('scalar', 'vector', 'texture', 'static_switch'):
        current = {}
        for name in sorted(set(str(v) for v in getattr(mel, 'get_' + kind + '_parameter_names')(material))):
            val = getattr(mel, 'get_material_instance_' + kind + '_parameter_value')(material, name)
            current[name] = val.export_text() if hasattr(val, 'export_text') else (
                val.get_path_name() if hasattr(val, 'get_path_name') else val)
        values[kind] = current
    return values


def original_parameters_preserved(original, copied):
    # The source instance's inherited defaults and explicit overrides remain exact;
    # the new parent's death-function parameters are the only added names.
    for kind, values in original.items():
        for name, value in values.items():
            assert copied[kind][name] == value, (kind, name)


def material_outputs(master):
    mel = ue.MaterialEditingLibrary
    result = {}
    for prop in (ue.MaterialProperty.MP_BASE_COLOR, ue.MaterialProperty.MP_METALLIC,
                 ue.MaterialProperty.MP_SPECULAR, ue.MaterialProperty.MP_ROUGHNESS,
                 ue.MaterialProperty.MP_NORMAL, ue.MaterialProperty.MP_AMBIENT_OCCLUSION,
                 ue.MaterialProperty.MP_WORLD_POSITION_OFFSET):
        node = mel.get_material_property_input_node(master, prop)
        result[str(prop)] = [node.get_name() if node else None, mel.get_material_property_input_node_output_name(master, prop)]
    return result


def verify_custom_uv(master, function, directory):
    # Read the serialized Editor data through the native exporter. The input is
    # deliberately protected/Hidden from Python, but its actual connection can
    # still be verified without changing it or the binary package outside Unreal.
    text = export(master, directory / 'custom_uv_connection.t3d')
    lines = re.findall(r'^\s*CustomizedUVs\(1\)=(.+)$', text, re.M)
    assert len(lines) == 1, 'Connect MF_MannequinEdge.customUVData to Customized UV1 in the Material Editor first'
    assert ":" + function.get_name() + "'" in lines[0]
    assert 'OutputIndex=1' in lines[0], 'UV1 must use the original customUVData output index 1'


def duplicate(source, destination):
    assert destination in {MASTER, INSTANCE}
    assert not disk(destination).exists() and not ue.EditorAssetLibrary.does_asset_exist(destination)
    folder, name = destination.rsplit('/', 1)
    obj = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, folder, source)
    assert obj and obj.get_path_name() == destination + '.' + name
    scoped_dirty()
    return obj


def make_materials(directory, report):
    mel = ue.MaterialEditingLibrary
    source, original_instance, edge = load(SOURCE_MASTER), load(SOURCE_INSTANCE), load(EDGE)
    assert original_instance.get_editor_property('Parent') == source
    original = parameters(original_instance)
    original_outputs = material_outputs(source)
    master = duplicate(source, MASTER)
    assert material_outputs(master) == original_outputs
    source_names = {obj.get_name() for obj in mel.get_material_expressions(master)}
    emissive = mel.get_material_property_input_node(master, ue.MaterialProperty.MP_EMISSIVE_COLOR)
    output = mel.get_material_property_input_node_output_name(master, ue.MaterialProperty.MP_EMISSIVE_COLOR)
    assert emissive and emissive.get_name() == 'MaterialExpressionStaticSwitchParameter_0'
    function = mel.create_material_expression(master, ue.MaterialExpressionMaterialFunctionCall, 1600, 450)
    color = mel.create_material_expression(master, ue.MaterialExpressionVectorParameter, 1300, 650)
    add = mel.create_material_expression(master, ue.MaterialExpressionAdd, 1900, 250)
    assert function.set_material_function(edge)
    color.set_editor_property('ParameterName', 'KillEdgeColor')
    color.set_editor_property('DefaultValue', ue.LinearColor(0, 1, 1, 1))
    assert 'KillEdgeColor' in mel.get_material_expression_input_names(function)
    assert {'KillEdge', 'MaskedOpacity', 'customUVData'} <= set(mel.get_material_expression_output_names(function))
    assert mel.connect_material_expressions(color, '', function, 'KillEdgeColor')
    assert mel.connect_material_expressions(emissive, output, add, 'A')
    assert mel.connect_material_expressions(function, 'KillEdge', add, 'B')
    assert mel.connect_material_property(add, '', ue.MaterialProperty.MP_EMISSIVE_COLOR)
    assert mel.connect_material_property(function, 'MaskedOpacity', ue.MaterialProperty.MP_OPACITY_MASK)
    # Match the original M_Mannequin's MF_MannequinEdge custom-UV output slot.
    # The user connects customUVData to Customized UV1 in the Material Editor.
    # Python cannot marshal the Hidden enum or access this protected input.
    master.set_editor_property('NumCustomizedUVs', 2)
    master.set_editor_property('BlendMode', ue.BlendMode.BLEND_MASKED)
    assert material_outputs(master) == original_outputs
    assert {obj.get_name() for obj in mel.get_material_expressions(master)} == source_names | {
        function.get_name(), color.get_name(), add.get_name()}
    errors = list(mel.recompile_material(master))
    assert not errors, errors
    instance = duplicate(original_instance, INSTANCE)
    mel.set_material_instance_parent(instance, master)
    mel.update_material_instance(instance)
    original_parameters_preserved(original, parameters(instance))
    report['material_original_outputs'] = original_outputs
    report['material_original_parameters'] = original
    report['new_expression_names'] = [function.get_name(), color.get_name(), add.get_name()]
    export(master, directory / 'new_master.t3d')
    export(instance, directory / 'new_instance.t3d')
    return master, instance


def configure_pawn(bp, instance):
    bp.modify()
    _, targets = component_targets(bp)
    for obj in targets:
        obj.modify()
        if isinstance(obj, ue.SkeletalMeshComponent):
            obj.set_editor_property('ComponentTags', [tag for tag in obj.get_editor_property('ComponentTags')
                                                     if str(tag).lower() != 'ignore'])
            obj.set_material(0, instance)
        else:
            obj.set_editor_property('bPreventDamageAfterDeath', True)
    ue.BlueprintEditorLibrary.compile_blueprint(bp)
    assert bp.get_editor_property('Status') == ue.BlueprintStatus.BS_UP_TO_DATE
    scoped_dirty()


def verify_assets(bp, master, instance, before, directory, report):
    mel = ue.MaterialEditingLibrary
    after = pawn_snapshot(bp, directory, 'after')
    assert after['mesh_tags'] == [tag for tag in before['mesh_tags'] if tag.lower() != 'ignore']
    assert after['material'] == INSTANCE + '.MI_Inspector_Death' and after['reject_after_death']
    for field in ('mesh', 'anim_class', 'transform'):
        assert after[field] == before[field], field
    assert instance.get_editor_property('Parent') == master
    assert master.get_editor_property('BlendMode') == ue.BlendMode.BLEND_MASKED
    assert master.get_editor_property('NumCustomizedUVs') == 2
    opacity = mel.get_material_property_input_node(master, ue.MaterialProperty.MP_OPACITY_MASK)
    assert isinstance(opacity, ue.MaterialExpressionMaterialFunctionCall)
    assert opacity.get_editor_property('MaterialFunction').get_path_name() == EDGE + '.MF_MannequinEdge'
    assert mel.get_material_property_input_node_output_name(master, ue.MaterialProperty.MP_OPACITY_MASK) == 'MaskedOpacity'
    verify_custom_uv(master, opacity, directory)
    assert material_outputs(master) == report['material_original_outputs']
    original_parameters_preserved(report['material_original_parameters'], parameters(instance))
    report['after'] = after
    return after


def main():
    global ue
    import unreal as ue
    sys.dont_write_bytecode = True
    directory = None
    report = {'complete': False, 'asset_saves': 0, 'saved_packages': [], 'PIE': False, 'cpp_build': False,
              'asset_save_count_definition': 'Explicit save_loaded_asset calls; AssetTools duplication may also write the two new package files.'}
    try:
        command = ue.SystemLibrary.get_command_line()
        def arg(name, pattern):
            found = re.findall(r'(?:^|\s)-' + name + '=(' + pattern + r')(?=\s|$)', command)
            assert len(found) == 1, name
            return found[0]
        mode = arg('DCInspectorDeathMode', 'prepare|apply|verify_prepared|verify')
        run = arg('DCInspectorDeathRun', '[A-Za-z0-9_]{1,40}')
        frozen = arg('DCInspectorDeathSHA', '[a-f0-9]{64}')
        assert sha(SCRIPT) == frozen
        assert all(flag in command.lower() for flag in ('-unattended', '-noautosave', '-sccprovider=none'))
        assert not ue.SourceControl.is_enabled()
        assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P
        spec = importlib.util.spec_from_file_location('inspector_death_process_guard', P / 'Scripts/Editor/ingame_hud_layout_setup.py')
        helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(helper)
        helper.assert_no_other_editor()
        directory = ROOT / run
        directory.mkdir(parents=True, exist_ok=True)
        assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Fresh diagnostic run required'
        logs = re.findall(r'(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
        assert len(logs) == 1 and Path(logs[0][0] or logs[0][1]).resolve() == directory / 'Editor.log'
        settings = ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
        assert 'SaveOnCompile=SoC_Never' in export(settings, directory / 'settings.t3d')
        report.update({'mode': mode, 'run': run, 'script_sha256': frozen, 'before_protection': protection()})
        assert not scoped_dirty()
        bp = load(PAWN)
        if mode in ('prepare', 'apply'):
            initial = read(ROOT / 'Readonly_B/report.json')['assets'][PAWN]['disk_sha']
            assert sha(disk(PAWN)) == initial, 'Private Pawn changed after the approved inspection'
            before = pawn_snapshot(bp, directory, 'before')
            assert sum(tag.lower() == 'ignore' for tag in before['mesh_tags']) == 1
            assert not before['reject_after_death'] and before['material'] == SOURCE_INSTANCE + '.Fbx_Default_Material_0'
            report['before'] = before
            if mode == 'prepare':
                assert all(not disk(q).exists() and not ue.EditorAssetLibrary.does_asset_exist(q) for q in (MASTER, INSTANCE))
                master, instance = make_materials(directory, report)
                save_packages = (MASTER, INSTANCE)
                report['manual_uv_pending'] = True
                report['pawn_modified'] = False
            else:
                reference = arg('DCInspectorDeathReference', '[A-Za-z0-9_]{1,40}')
                prepared = read(ROOT / reference / 'report.json')
                assert prepared['mode'] == 'prepare' and prepared['complete'] and prepared['manual_uv_pending']
                assert prepared['script_sha256'] == frozen and prepared['asset_saves'] == 2
                for field in ('material_original_outputs', 'material_original_parameters'):
                    report[field] = prepared[field]
                master, instance = load(MASTER), load(INSTANCE)
                functions = [obj for obj in ue.MaterialEditingLibrary.get_material_expressions(master)
                             if isinstance(obj, ue.MaterialExpressionMaterialFunctionCall)
                             and obj.get_editor_property('MaterialFunction') == load(EDGE)]
                assert len(functions) == 1
                verify_custom_uv(master, functions[0], directory)
                configure_pawn(bp, instance)
                # Already exported the UV proof above; the other verification is identical.
                (directory / 'custom_uv_connection.t3d').rename(directory / 'manual_uv_connection.t3d')
                verify_assets(bp, master, instance, before, directory, report)
                save_packages = (MASTER, INSTANCE, PAWN)
                report['manual_uv_pending'] = False
                report['pawn_modified'] = True
            report['pre_save_log'] = clean_log(directory / 'Editor.log')
            protection()
            for package in save_packages:
                assert package in WRITES and ue.EditorAssetLibrary.save_loaded_asset(load(package), only_if_is_dirty=False)
                report['asset_saves'] += 1
                report['saved_packages'].append(package)
            report['saved_files'] = {disk(q).relative_to(P).as_posix(): sha(disk(q)) for q in save_packages}
        elif mode == 'verify_prepared':
            reference = arg('DCInspectorDeathReference', '[A-Za-z0-9_]{1,40}')
            prior = read(ROOT / reference / 'report.json')
            assert prior['complete'] and prior['mode'] == 'prepare' and prior['script_sha256'] == frozen
            assert prior['asset_saves'] == 2 and prior['manual_uv_pending']
            assert all(sha(P / name) == value for name, value in prior['saved_files'].items())
            master, instance = load(MASTER), load(INSTANCE)
            assert instance.get_editor_property('Parent') == master
            assert material_outputs(master) == prior['material_original_outputs']
            original_parameters_preserved(prior['material_original_parameters'], parameters(instance))
            assert sha(disk(PAWN)) == read(ROOT / 'Readonly_B/report.json')['assets'][PAWN]['disk_sha']
            report.update({'prepared_reload_verified': True, 'manual_uv_pending': True, 'pawn_modified': False})
        else:
            reference = arg('DCInspectorDeathReference', '[A-Za-z0-9_]{1,40}')
            prior = read(ROOT / reference / 'report.json')
            assert prior['complete'] and prior['mode'] == 'apply' and prior['script_sha256'] == frozen
            assert prior['asset_saves'] == 3 and set(prior['saved_packages']) == WRITES
            assert all(sha(P / name) == value for name, value in prior['saved_files'].items())
            for field in ('material_original_outputs', 'material_original_parameters'):
                report[field] = prior[field]
            master, instance = load(MASTER), load(INSTANCE)
            verify_assets(bp, master, instance, prior['before'], directory, report)
            report['fresh_reload_verified'] = True
        scoped_dirty()
        report['after_protection'] = protection()
        report['log'] = clean_log(directory / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        ue.log_error(report['error'])
    finally:
        if directory is not None:
            report['new_asset_files_after_run'] = {q: sha(disk(q)) for q in (MASTER, INSTANCE) if disk(q).exists()}
            if 'before_protection' in report:
                try:
                    report['final_protection'] = protection()
                except Exception:
                    report['complete'] = False
                    report['protection_error'] = traceback.format_exc()
            (directory / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.SystemLibrary.quit_editor()


def commands(prefix):
    assert re.fullmatch('[A-Za-z0-9_]{1,24}', prefix)
    result = {}
    for mode in ('prepare', 'verify_prepared', 'apply', 'verify'):
        run = prefix + '_' + mode
        directory = ROOT / run
        args = ['E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe', str(P / 'DreamCatcher.uproject'),
                "-ExecCmds=py __import__('runpy').run_path('" + SCRIPT.as_posix() + "')['main']()",
                '-unattended', '-nop4', '-SCCProvider=None', '-NoAutoSave', '-NoSplash', '-nosound',
                '-DCInspectorDeathMode=' + mode, '-DCInspectorDeathRun=' + run,
                '-DCInspectorDeathSHA=' + sha(SCRIPT), '-abslog=' + directory.as_posix() + '/Editor.log']
        if mode in ('verify_prepared', 'apply', 'verify'):
            args += ['-DCInspectorDeathReference=' + prefix + ('_apply' if mode == 'verify' else '_prepare')]
        result[mode] = {'args': args, 'directory': directory.as_posix(), 'windows_command_line': subprocess.list2cmdline(args)}
    print(json.dumps({'launches_processes': False, 'PIE': False, 'cpp_build': False, 'commands': result}, indent=2))


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        commands(sys.argv[2])
    else:
        main()
