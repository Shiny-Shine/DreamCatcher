"""Bounded TODO4 mission assets/map setup, after the user's native build.

Separate UnrealEditor-Cmd -run=pythonscript -unattended -EnablePlugins=UMGToolSet
-SCCProvider=None -DCMissionHUDSetupMode=inspect|apply|verify
-DCMissionHUDSetupRun=<fresh> -DCMissionHUDSetupReference=<inspect/apply run>
-abslog=<project>/Saved/Diagnostics/MissionHUD20261019/<fresh>/Editor.log
Read-only recovery of the known four saved Apply_D packages additionally needs
-DCMissionHUDRecoverSavedApply with mode=verify and reference=Apply_D.

Only the four packages in PATHS may be saved. New destinations must be absent.
No C++ build, PIE, source/config change, binary file I/O, material save or bulk
save. Unknown/partial apply outcomes require review; they are never overwritten.
LevelExporterT3D is deliberately excluded: prior Level_1 evidence reproduced
detach/reattach side effects. Actor ObjectExporterT3D plus component/level
identity inventories preserve the loaded map without that exporter behavior.
"""
import importlib.util
import json
import math
import os
from pathlib import Path
import re
import sys
import traceback

import unreal as ue

sys.dont_write_bytecode = True
PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/MissionHUD20261019'
SCHEMA = 2  # Every brush now has independent full-model geometry coverage.
PATHS = {
    'list': '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example',
    'entry': '/Game/DreamCatcher/UI/HUD/W_DC_MissionEntry',
    'map': '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1',
    'boss': '/Game/DreamCatcher/Data/Boss/DA_DC_Boss',
}
NEW = ('list', 'entry')
MARKERS = ('DC_MissionTarget_Combat1', 'DC_MissionTarget_Combat2', 'DC_MissionTarget_Boss')
ART = {
    'panel': '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/Art/MI_UI_AccoladeBorder',
    'text_panel': '/Game/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/Art/MI_UI_AccoladeBorder_Text',
    'circle': '/Game/DreamCatcher/UI/Textures/Crosshair/T_Circle_Filled',
}
STYLE = dict(RightMargin=64, TopOffset=288, PanelWidth=360, MinHeight=132,
             MaxHeight=392, Padding=16, HeaderHeight=40, RowMinHeight=48,
             FontSize=18, HeaderFontSize=22, StageFontSize=14)
EXAMPLES = (
    ('DefeatNightmares', '악몽 처치하기', 'CurrentEncounterDefeats', ''),
    ('ReachPlaza', '광장구역으로 이동하기', 'Guidance', 'EncounterGoal'),
)
GUID_EVIDENCE = PROJECT / 'Saved/Diagnostics/Level1Merge20261016/apply_merge.py'
EXPORTER_EVIDENCE = Path('E:/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Private/UnrealExporter.cpp')
SPRITE_EVIDENCE = Path('E:/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Private/Components/SceneComponent.cpp')
MODEL_EXPORT_EVIDENCE = Path('E:/Epic Games/UE_5.8/Engine/Source/Editor/UnrealEd/Private/EditorExporters.cpp')
ISM_HEADER = Path('E:/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/Components/InstancedStaticMeshComponent.h')
ISM_SOURCE = Path('E:/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Private/InstancedStaticMesh.cpp')
CACHE_OWNER = PATHS['map'] + '.Level_1:PersistentLevel.BPP_NewMap_C_1'
CACHE_CLASS = '/Script/Engine.InstancedStaticMeshComponent'
CACHE_PATHS = tuple(CACHE_OWNER + '.InstancedStaticMesh' + (str(index) if index else '') for index in range(9))
spec = importlib.util.spec_from_file_location(
    'dc_mission_hud_evidence', PROJECT / 'Scripts/Editor/ingame_hud_layout_setup.py')
hud = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hud)


def disk(key):
    return PROJECT / ('Content/' + PATHS[key][6:] + ('.umap' if key == 'map' else '.uasset'))


def inventory():
    result = hud.inventory()
    # Shared helper excludes the HUD it was written to edit; TODO4 protects it.
    value = hud.DISK.stat()
    result[str(hud.DISK.relative_to(PROJECT))] = [value.st_size, value.st_mtime_ns]
    for key in PATHS:
        result.pop(str(disk(key).relative_to(PROJECT)), None)
    return result


def hashes():
    result = hud.protected_hashes()
    for path in [hud.DISK, GUID_EVIDENCE] + [PROJECT / ('Content/' + value[6:] + '.uasset') for value in ART.values()]:
        result[str(path.relative_to(PROJECT))] = hud.sha(path)
    return result


def target_hashes():
    return {key: hud.sha(disk(key)) if disk(key).is_file() else None for key in PATHS}


def clean(content=(), maps=()):
    dirty = {package.get_name() for package in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()}
    dirty_maps = {package.get_name() for package in ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()}
    assert dirty <= set(content), 'Unexpected dirty content: ' + str(sorted(dirty))
    assert dirty_maps <= set(maps), 'Unexpected dirty maps: ' + str(sorted(dirty_maps))


def native(name):
    cls = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert cls, 'BUILD_REQUIRED: missing approved native class ' + name
    return cls


def struct(name):
    cls = getattr(ue, name, None)
    assert cls, 'BUILD_REQUIRED: missing reflected struct ' + name
    return cls()


def split_fields(value):
    """Split one Unreal struct export without changing quoted/nested values."""
    assert value.startswith('(') and value.endswith(')'), value
    result, start, depth, quote, escaped = [], 1, 0, False, False
    for index, char in enumerate(value[1:-1], 1):
        if quote:
            if escaped:
                escaped = False
            elif char == '\\':
                escaped = True
            elif char == '"':
                quote = False
        elif char == '"':
            quote = True
        elif char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
            assert depth >= 0, value
        elif char == ',' and depth == 0:
            result.append(value[start:index])
            start = index + 1
    assert not quote and depth == 0, value
    if value[start:-1]:
        result.append(value[start:-1])
    keys = [field.split('=', 1)[0] for field in result]
    assert all('=' in field for field in result) and len(keys) == len(set(keys)), value
    return result


def without_fields(value, names):
    return '(' + ','.join(field for field in split_fields(value) if field.split('=', 1)[0] not in names) + ')'


def strip_director_missions(text):
    # Only the top-level Phases property of this one actor may lose the two
    # approved fields. All phase titles/types/delays/encounters remain exact.
    output, depth, found = [], 0, set()
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
        elif depth == 1:
            match = re.fullmatch(r'(\s*Phases\((\d+)\)=)([^\r\n]+)(\n?)', line)
            if match:
                index = int(match[2])
                assert index not in found and 0 <= index <= 3
                found.add(index)
                line = match[1] + without_fields(match[3], {'MissionList', 'MissionTargets'}) + match[4]
        output.append(line)
    assert depth == 0 and found == {0, 1, 2, 3}, 'Unexpected phase export schema'
    return ''.join(output)


def strip_boss_offset(text):
    # Native defaults omit PanelOffset=(0,88) before the edit. Remove just that
    # field on both sides; the reflected check separately enforces 0,88 -> 0,48.
    output, found = [], 0
    for line in text.splitlines(keepends=True):
        match = re.fullmatch(r'(\s*HUDStyle=)([^\r\n]+)(\n?)', line)
        if match:
            found += 1
            line = match[1] + without_fields(match[2], {'PanelOffset'}) + match[3]
        output.append(line)
    assert found == 1, 'Expected one boss HUDStyle export'
    return ''.join(output)


def strip_generated_child_guid(text, guid):
    """Only a confirmed construction child actor's top-level ActorGuid."""
    output, depth, found = [], 0, 0
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
        elif depth == 1:
            match = re.fullmatch(r'(\s*ActorGuid=)([^\r\n]+)(\n?)', line)
            if match:
                found += 1
                assert match[2] == guid, 'Exported ActorGuid differs from the recorded original GUID'
                line = match[1] + '<cross-process-construction-child-guid>' + match[3]
        output.append(line)
    assert depth == 0 and found <= 1
    return ''.join(output)


def direct_child_declarations(text):
    """Native exporter declares direct inners by Class and quoted Name.

    UnrealExporter.cpp:453 EmitBeginObject always writes Name and writes Class
    unless PPF_SeparateDefine. Lines 571+ enumerate direct inners and emit both
    separate declarations and definitions. Full ExportPath is not the identity
    contract. The parser records declarations that are actually present; it
    does not imply every component is inline. ExportObjectInner:606,618 omits
    RF_TextExportTransient inners, and SceneComponent.cpp:902 creates editor
    sprites with that flag. Every component is exported separately regardless.
    """
    depth, result = 0, set()
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            if depth == 1:
                cls = re.search(r'\bClass=([^\s]+)', stripped)
                name = re.search(r'\bName="([^"]+)"', stripped)
                assert name, 'Object declaration has no quoted Name'
                if cls:
                    pair = (cls[1], name[1])
                    assert pair not in result, 'Duplicate direct child declaration'
                    result.add(pair)
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
            assert depth >= 0
    assert depth == 0
    return result


def brush_reference(text):
    """Read only the exact top-level exported instanced Brush reference."""
    depth, found = 0, []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
        elif depth == 1 and stripped.startswith('Brush='):
            match = re.fullmatch(r'Brush="/Script/Engine\.Model\'([^\'"\r\n]+)\'"', stripped)
            assert match, 'Unsupported brush model reference: ' + stripped
            found.append((stripped[6:], match[1]))
    assert depth == 0 and len(found) == 1, 'Expected one explicit native Brush model reference'
    return found[0]


def validate_model_geometry(text, name):
    """Accept the full native Model/Polys exporter structure, never a declaration."""
    lines = text.splitlines()
    assert len(lines) >= 4 and text.endswith('\n')
    assert lines[0] == 'Begin Brush Name=' + name and lines[-1] == 'End Brush'
    assert lines[1] == '   Begin PolyList' and lines[-2] == '   End PolyList'
    active, roles, vertices, polygons = False, [], 0, 0
    for line in lines[2:-2]:
        if re.fullmatch(r'      Begin Polygon(?: .*)?', line):
            assert not active, 'Nested polygon export'
            active, roles, vertices = True, [], 0
        elif line == '      End Polygon':
            assert active and roles == ['Origin', 'Normal', 'TextureU', 'TextureV'] and vertices >= 3
            active = False
            polygons += 1
        else:
            assert active, 'Unknown model geometry outside a polygon'
            match = re.fullmatch(r'         (Origin|Normal|TextureU|TextureV|Vertex)\s+([^\r\n]+)', line)
            assert match, 'Unknown model geometry line: ' + line
            if match[1] == 'Vertex':
                assert roles == ['Origin', 'Normal', 'TextureU', 'TextureV']
                vertices += 1
            else:
                assert vertices == 0
                roles.append(match[1])
    assert not active, 'Unclosed model polygon'
    return polygons


def normalize_inline_model(text, model):
    """Only remove a complete duplicate verified against separate full geometry.

    Generic instanced-property export uses persistent OBJECTMARK_TagImp
    (UnrealExporter.cpp:1259-1282); ModelExporterT3D:423-433 directly emits Polys
    without that mark guard. Empty inline declarations and Brush refs stay exact.
    """
    if model is not None:
        assert brush_reference(text) == (model['reference'], model['reference_path'])
        polygons = validate_model_geometry(model['text'], model['name'])
        assert polygons == model['polygon_count']
    lines, output, depth, index = text.splitlines(keepends=True), [], 0, 0
    complete, declarations = 0, 0
    while index < len(lines):
        line = lines[index]
        stripped = line.strip()
        if stripped.startswith('Begin Brush'):
            assert model is not None and depth == 1, 'Unknown or nested inline brush model'
            assert stripped == 'Begin Brush Name=' + model['name'], 'Unknown inline model identity'
            end = index + 1
            while end < len(lines) and lines[end].strip() != 'End Brush':
                assert not lines[end].strip().startswith(('Begin Brush', 'Begin Object', 'End Object'))
                end += 1
            assert end < len(lines), 'Unclosed inline brush'
            prefix = line[:len(line) - len(line.lstrip(' '))]
            block = lines[index:end + 1]
            assert all(value.startswith(prefix) for value in block)
            unindented = ''.join(value[len(prefix):] for value in block)
            if unindented == 'Begin Brush Name=' + model['name'] + '\nEnd Brush\n':
                declarations += 1
                output.extend(block)
            else:
                assert unindented == model['text'], 'Inline geometry differs from separate full model export'
                complete += 1
            assert complete <= 1 and declarations <= 1, 'Duplicate/unknown inline model block'
            index = end + 1
            continue
        if stripped.startswith('Begin Object '):
            depth += 1
        elif stripped == 'End Object':
            depth -= 1
            assert depth >= 0
        output.append(line)
        index += 1
    assert depth == 0
    return ''.join(output), dict(complete_duplicates_matched=complete,
        inline_declarations_preserved=declarations, separate_full_geometry=model is not None)


def export_model(model, path):
    assert not path.exists()
    task = ue.AssetExportTask()
    task.object = model
    task.exporter = ue.ModelExporterT3D()
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    assert ue.Exporter.run_asset_export_task(task), model.get_path_name()
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')


def preflight_brush_models(actors, folder, proof):
    # Fail on the small brush set before exporting thousands of map actors.
    brush_class = ue.load_class(None, '/Script/Engine.Brush')
    model_class = ue.load_class(None, '/Script/Engine.Model')
    assert brush_class and model_class and getattr(ue, 'ModelExporterT3D', None)
    models, paths = {}, set()
    brushes = [actor for actor in actors if ue.MathLibrary.class_is_child_of(actor.get_class(), brush_class)]
    for index, actor in enumerate(brushes):
        source_file = folder / ('brush_source_%03d.t3d' % index)
        source = hud.export(actor, source_file)
        reference, reference_path = brush_reference(source)
        prefix = actor.get_path_name() + '.'
        if reference_path.startswith(prefix):
            name = reference_path[len(prefix):]
        else:
            assert not any(char in reference_path for char in '/:.'), 'Model reference is not actor-owned'
            name = reference_path
        assert name and not any(char in name for char in '/:.'), 'Unsupported nested model path'
        # PyCore.find_object supports exact outer/name/type lookup and disabling
        # redirectors. No protected ABrush.Brush property is accessed.
        model = ue.find_object(actor, name, type=model_class, follow_redirectors=False)
        assert model and model.get_class() == model_class and model.get_outer() == actor
        assert model.get_name() == name and model.get_path_name() == prefix + name
        assert model.get_package() == actor.get_package()
        assert model.get_path_name() not in paths, 'Repeated model reference has ambiguous ownership'
        paths.add(model.get_path_name())
        model_file = folder / ('model_%03d.t3d' % index)
        repeat_file = folder / ('model_%03d_repeat.t3d' % index)
        geometry, repeated = export_model(model, model_file), export_model(model, repeat_file)
        assert geometry == repeated, 'Direct ModelExporter is not repeatable'
        state = dict(path=model.get_path_name(), parent=actor.get_path_name(),
                     cls=model.get_class().get_path_name(), name=name, reference=reference,
                     reference_path=reference_path, text=geometry,
                     polygon_count=validate_model_geometry(geometry, name), repeated_export_equal=True)
        _, matching = normalize_inline_model(source, state)
        models[actor.get_path_name()] = state
        proof[actor.get_path_name()] = dict(model_path=state['path'], raw_reference=reference,
            source_actor_file=str(source_file), model_file=str(model_file), repeat_file=str(repeat_file),
            model_sha256=hud.sha(model_file), repeat_sha256=hud.sha(repeat_file),
            polygons=state['polygon_count'], source_inline_matching=matching)
    return models


def check_boss(data, y):
    assert data.get_class() == native('DCBossData')
    assert str(data.get_editor_property('DisplayName')) == '춘식이'
    assert str(data.get_editor_property('Subtitle')) == '나스닥의 수호자'
    offset = data.get_editor_property('HUDStyle').get_editor_property('PanelOffset')
    assert (offset.x, offset.y) == (0, y), (offset.x, offset.y)


def make_definition(values):
    mission_id, text, source, target = values
    definition = struct('DCMissionDefinition')
    # Native enum spelling is checked by import_text instead of guessing Python
    # enum type/uppercase conversion. Invalid or stale schemas fail closed.
    assert definition.import_text('(ProgressSource=' + source + ')')
    definition.set_editor_property('MissionId', ue.Name(mission_id))
    definition.set_editor_property('DisplayText', ue.Text(text))
    definition.set_editor_property('TargetCount', 1)
    definition.set_editor_property('TargetKey', ue.Name(target))
    definition.set_editor_property('bEnabled', True)
    return definition


def expected_brushes():
    panel = ue.load_asset(ART['panel'])
    circle = ue.load_asset(ART['circle'])
    assert panel and circle and ue.load_asset(ART['text_panel'])
    brush = ue.WidgetLibrary.make_brush_from_material(panel, 16, 16)
    brush.set_editor_property('DrawAs', ue.SlateBrushDrawType.BOX)
    brush.set_editor_property('Margin', ue.Margin(.5, .5, .5, .5))
    icon = ue.WidgetLibrary.make_brush_from_texture(circle, 40, 40)
    # Exact original Border brush geometry is recorded at source A184:270.
    # UE5.8 exposes ImageSize as DeprecateSlateVector2D, without x/y members.
    image_sizes = [field for field in split_fields(brush.export_text()) if field.split('=', 1)[0] == 'ImageSize']
    assert image_sizes == ['ImageSize=(X=16.000000,Y=16.000000)'], image_sizes
    result = {'PanelBrush': brush, 'HeaderIconBrush': icon}
    # A default FSlateBrush draws Image; configure the approved outline/fill
    # explicitly instead of relying on the native NoDrawType fallback branch.
    for name, draw_as in (('CheckboxBrush', 'Border'), ('CompletedBrush', 'Box')):
        checkbox = ue.SlateBrush()
        assert checkbox.import_text('(DrawAs=' + draw_as + ',ImageSize=(X=8.000000,Y=8.000000),'
            'Margin=(Left=0.125000,Top=0.125000,Right=0.125000,Bottom=0.125000),'
            'ResourceObject=None,TintColor=(SpecifiedColor=(R=1.000000,G=1.000000,B=1.000000,A=1.000000),ColorUseRule=UseColor_Specified))')
        assert ue.WidgetLibrary.get_brush_resource(checkbox) is None
        result[name] = checkbox
    return result


def native_contract():
    list_class, entry_class = native('DCMissionListData'), native('DCMissionEntryWidget')
    cdo = ue.get_default_object(list_class)
    cdo.get_editor_property('Missions')
    cdo.get_editor_property('DefaultTrackedMissionId')
    style = cdo.get_editor_property('HUDStyle')
    for name, expected in STYLE.items():
        assert abs(float(style.get_editor_property(name)) - expected) < .000001, name
    for name in ('TextColor', 'PanelBrush', 'HeaderIconBrush', 'CheckboxBrush', 'CompletedBrush', 'EntryWidgetClass'):
        style.get_editor_property(name)
    assert not ue.get_default_object(entry_class).get_editor_property('bIsFocusable')
    for values in EXAMPLES:
        make_definition(values)
    binding = struct('DCMissionTargetBinding')
    binding.get_editor_property('TargetKey')
    binding.get_editor_property('TargetActor')
    return list_class, entry_class


def check_entry(bp, cls, umg, directory, label):
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp) == cls
    rows = list(umg.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets'))
    assert not rows, 'Thin entry Blueprint must use the native RebuildWidget tree'
    cdo = ue.get_default_object(bp.generated_class())
    assert not cdo.get_editor_property('bIsFocusable')
    assert cdo.get_editor_property('Visibility') == ue.SlateVisibility.HIT_TEST_INVISIBLE
    return hud.export(cdo, directory / (label + '_entry_cdo.t3d'))


def create_entry(cls, umg):
    path = PATHS['entry']
    bp = umg.call_method('CreateWidgetBlueprint', args=(path.rsplit('/', 1)[0], path.rsplit('/', 1)[1], cls))
    assert bp and bp.get_path_name().split('.')[0] == path
    rows = list(umg.call_method('GetWidgets', args=(bp,)).get_editor_property('widgets'))
    roots = [row for row in rows if not row.get_editor_property('parent')]
    assert len(roots) <= 1
    if roots:
        assert umg.call_method('RemoveWidget', args=(bp, roots[0].get_editor_property('widget')))
    cdo = ue.get_default_object(bp.generated_class())
    cdo.set_editor_property('bIsFocusable', False)
    cdo.set_editor_property('Visibility', ue.SlateVisibility.HIT_TEST_INVISIBLE)
    assert umg.call_method('CompileWidgetBlueprint', args=(bp,))
    return bp


def create_list(cls, entry, brushes):
    factory = ue.DataAssetFactory()
    factory.set_editor_property('DataAssetClass', cls)
    path = PATHS['list']
    data = ue.AssetToolsHelpers.get_asset_tools().create_asset(
        path.rsplit('/', 1)[1], path.rsplit('/', 1)[0], cls, factory)
    assert data and data.get_class() == cls
    data.set_editor_property('Missions', [make_definition(values) for values in EXAMPLES])
    data.set_editor_property('DefaultTrackedMissionId', ue.Name('ReachPlaza'))
    style = data.get_editor_property('HUDStyle')
    for name, value in STYLE.items():
        style.set_editor_property(name, value)
    for name, value in brushes.items():
        style.set_editor_property(name, value)
    style.set_editor_property('EntryWidgetClass', entry.generated_class())
    data.set_editor_property('HUDStyle', style)
    return data


def check_list(data, cls, entry, brushes):
    assert data.get_class() == cls
    actual = list(data.get_editor_property('Missions'))
    assert len(actual) == len(EXAMPLES)
    for definition, values in zip(actual, EXAMPLES):
        expected = make_definition(values)
        for name in ('MissionId', 'DisplayText', 'ProgressSource', 'TargetCount', 'TargetKey', 'bEnabled'):
            value, wanted = definition.get_editor_property(name), expected.get_editor_property(name)
            assert (str(value) == str(wanted) if name in ('MissionId', 'DisplayText', 'TargetKey') else value == wanted), name
    assert str(data.get_editor_property('DefaultTrackedMissionId')) == 'ReachPlaza'
    style = data.get_editor_property('HUDStyle')
    for name, expected in STYLE.items():
        assert abs(float(style.get_editor_property(name)) - expected) < .000001, name
    for name, expected in brushes.items():
        assert style.get_editor_property(name).export_text() == expected.export_text(), name
    assert style.get_editor_property('EntryWidgetClass') == entry.generated_class()
    default_style = ue.get_default_object(cls).get_editor_property('HUDStyle')
    # Existing boss_hud_setup.py check() uses the four reflected LinearColor
    # scalar members. Do not depend on a particular color wrapper export API.
    tint, expected_tint = style.get_editor_property('TextColor'), default_style.get_editor_property('TextColor')
    assert (tint.r, tint.g, tint.b, tint.a) == (
        expected_tint.r, expected_tint.g, expected_tint.b, expected_tint.a)


def get_map():
    world = ue.EditorLoadingAndSavingUtils.load_map(PATHS['map'])
    assert world and world.get_path_name() == PATHS['map'] + '.Level_1'
    subsystem = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    actors = list(subsystem.get_all_level_actors())
    assert actors and len({actor.get_path_name() for actor in actors}) == len(actors)
    # load_class returns a UClass object, not a Python type. Use the engine's
    # reflected inheritance predicate (KismetMathLibrary.h ClassIsChildOf).
    directors = [actor for actor in actors if ue.MathLibrary.class_is_child_of(
        actor.get_class(), native('DCStageDirector'))]
    assert len(directors) == 1, 'Expected one current Level_1 StageDirector'
    director = directors[0]
    # World.cpp:2694 constructs NewObject<ULevel>(this, TEXT("PersistentLevel")).
    # Confirm that public outer/world relationship instead of reading the
    # protected World.PersistentLevel property through Python reflection.
    level = director.get_outer()
    assert isinstance(level, ue.Level) and level.get_name() == 'PersistentLevel'
    assert level.get_outer() == world and director.get_world() == world, 'Director is not in saved persistent level'
    phases = list(director.get_editor_property('Phases'))
    assert len(phases) == 4
    expected = ('Encounter', 'Encounter', 'BossEncounter', 'Complete')
    for index, (phase, kind) in enumerate(zip(phases, expected)):
        # import_text provides an independent expected enum without relying on
        # generated Python enum spelling. No live phase property is modified.
        sample = struct('DCStagePhaseDefinition')
        assert sample.import_text('(PhaseType=' + kind + ')')
        assert phase.get_editor_property('PhaseType') == sample.get_editor_property('PhaseType'), index
        phase.get_editor_property('MissionList')
        phase.get_editor_property('MissionTargets')
        if index < 3:
            encounter = phase.get_editor_property('Encounter')
            assert encounter and ue.MathLibrary.class_is_child_of(
                encounter.get_class(), native('DCEncounterController'))
    return world, subsystem, actors, director, phases


def component_state(component, text):
    expected_header = 'Begin Object Class=' + component.get_class().get_path_name() + ' Name="' + component.get_name() + '"'
    assert text.splitlines()[0].startswith(expected_header), 'Incorrect separate component export: ' + component.get_path_name()
    result = dict(path=component.get_path_name(), cls=component.get_class().get_path_name(), text=text)
    if isinstance(component, ue.SceneComponent):
        result['transform'] = component.get_world_transform().export_text()
        parent = component.get_attach_parent()
        result['parent'] = parent.get_path_name() if parent else None
    return result


def map_snapshot(world, actors, director, directory, label, model_evidence):
    """Full actor/component raw, independent brush geometry and identity inventory."""
    folder = directory / label
    folder.mkdir()
    rows, levels = {}, {}
    all_actors = {actor.get_path_name(): actor for actor in actors}
    settings = world.get_world_settings()
    all_actors.setdefault(settings.get_path_name(), settings)
    model_evidence[label] = {}
    models = preflight_brush_models([actor for _, actor in sorted(all_actors.items())],
                                    folder, model_evidence[label])
    for index, (path, actor) in enumerate(sorted(all_actors.items())):
        text = hud.export(actor, folder / ('actor_%04d.t3d' % index))
        model = models.get(path)
        _, inline_matching = normalize_inline_model(text, model)
        if model is not None:
            model_evidence[label][path]['actor_inline_matching'] = inline_matching
        components = list(actor.get_components_by_class(ue.ActorComponent))
        owned = [component for component in components if component.get_outer() == actor]
        declarations = direct_child_declarations(text)
        not_inline = [component.get_path_name() for component in owned
                      if (component.get_class().get_path_name(), component.get_name()) not in declarations]
        component_rows = [component_state(component, hud.export(
            component, folder / ('component_%04d_%03d.t3d' % (index, number))))
            for number, component in enumerate(components)]
        parent = actor.get_attach_parent_actor()
        level = actor.get_outer()
        generated_child = '_GEN_VARIABLE_' in actor.get_name()
        if generated_child:
            assert actor.is_child_actor(), 'Construction child name is not an actual child actor: ' + path
        levels[level.get_path_name()] = dict(cls=level.get_class().get_path_name(), outer=level.get_outer().get_path_name())
        rows[path] = dict(name=actor.get_name(), cls=actor.get_class().get_path_name(),
                          label=actor.get_actor_label(False), outer=level.get_path_name(),
                          guid=actor.get_editor_property('ActorGuid').export_text(),
                          generated_child=generated_child,
                          transform=actor.get_actor_transform().export_text(),
                          parent=parent.get_path_name() if parent else None,
                          folder=str(actor.get_folder_path()),
                          inline_declarations=[list(pair) for pair in sorted(declarations)],
                          components_not_inline=not_inline,
                          components=component_rows,
                          brush_model=model,
                          text=text)
    assert director.get_path_name() in rows
    return dict(world=world.get_path_name(), world_class=world.get_class().get_path_name(),
                settings=settings.get_path_name(), director=director.get_path_name(),
                levels=levels, actors=rows)


def component_export_coverage(snapshot):
    # This is evidence only, never a component whitelist or comparison filter.
    missing = [path for row in snapshot['actors'].values() for path in row['components_not_inline']]
    return dict(actor_count=len(snapshot['actors']),
                separate_component_exports=sum(len(row['components']) for row in snapshot['actors'].values()),
                components_not_actor_inline=missing, not_inline_count=len(missing),
                comparison='All actor/component raw text and metadata remain strict')


def canonical_new_target_components(text, path, row):
    """Only the three new native TargetPoints' direct inner enumeration order.

    UnrealExporter.cpp:571+ uses unsorted object inners except PPF_DebugDump.
    Preserve each declaration/definition block byte-for-byte and the actor body.
    """
    allowed = {PATHS['map'] + '.Level_1:PersistentLevel.' + name for name in MARKERS}
    if path not in allowed:
        return text
    assert row['cls'] == '/Script/Engine.TargetPoint'
    components = {component['path']: component for component in row['components']}
    assert len(components) == len(row['components']) == 3
    assert {component['cls'] for component in components.values()} == {
        '/Script/Engine.SceneComponent', '/Script/Engine.ArrowComponent', '/Script/Engine.BillboardComponent'}
    lines = text.splitlines(keepends=True)
    root = export_identity(lines[0].strip())
    assert root['path'] == path and root['cls'] == row['cls'] and root['name'] == row['name']
    index, blocks = 1, {}
    while index < len(lines) and lines[index].strip().startswith('Begin Object '):
        header = lines[index].strip()
        identity = export_identity(header)
        component = components.get(identity['path'])
        assert component and identity['cls'] == component['cls']
        assert identity['name'] == identity['path'].rsplit('.', 1)[1]
        kind = 0 if header.startswith('Begin Object Class=') else 1
        key = (kind, identity['path'])
        assert key not in blocks, 'Duplicate native target component block'
        end = index + 1
        while end < len(lines) and lines[end].strip() != 'End Object':
            assert not lines[end].strip().startswith(('Begin Object', 'End Object')), 'Unknown nested target component'
            end += 1
        assert end < len(lines)
        blocks[key] = ''.join(lines[index:end + 1])
        index = end + 1
    assert set(blocks) == {(kind, component) for kind in (0, 1) for component in components}
    assert lines[-1] == 'End Object\n'
    assert not any(line.strip().startswith(('Begin Object', 'End Object')) for line in lines[index:-1])
    return lines[0] + ''.join(blocks[key] for key in sorted(blocks)) + ''.join(lines[index:])


def compare_maps(before, after, added=(), cross_process=False, cache_policy=None):
    assert {key: before[key] for key in before if key != 'actors'} == {
        key: after[key] for key in after if key != 'actors'}, 'Map/level/settings identity changed'
    added = set(added)
    assert set(after['actors']) == set(before['actors']) | added
    assert not (set(before['actors']) & added)
    excluded, changed_guids = [], []
    for path, row in before['actors'].items():
        old, new = dict(row), dict(after['actors'][path])
        # Full model identity/reference/polygon text stays strict. Only its
        # verified duplicate inside the actor export can vary with TagImp.
        assert old['brush_model'] == new['brush_model'], 'Brush model identity/geometry changed: ' + path
        for view in (old, new):
            view['text'], _ = normalize_inline_model(view['text'], view['brush_model'])
            view['text'] = canonical_new_target_components(view['text'], path, view)
            if cache_policy is not None:
                view['text'], _ = known_cache_fields(view['text'], cache_policy, remove=True)
        # Actor.cpp K2_GetComponentsByClass/GetComponents enumerates the
        # OwnedComponents TSet (Actor.h:3901,4331). Compare that API view by
        # unique component path; keep original report ordering and every raw
        # component property/actor body unchanged. No authored array is sorted.
        for view in (old, new):
            component_paths = [component['path'] for component in view['components']]
            assert len(component_paths) == len(set(component_paths)), 'Duplicate component path: ' + path
            components = [dict(component) for component in view['components']]
            if cache_policy is not None:
                for component in components:
                    component['text'], _ = known_cache_fields(component['text'], cache_policy, remove=True)
            view['components'] = sorted(components, key=lambda component: component['path'])
            missing = view['components_not_inline']
            assert len(missing) == len(set(missing)), 'Duplicate non-inline component path: ' + path
            assert set(missing) <= set(component_paths), 'Non-inline component has no separate export: ' + path
            view['components_not_inline'] = sorted(missing)
        if path == before['director']:
            old['text'], new['text'] = strip_director_missions(old['text']), strip_director_missions(new['text'])
        assert old['generated_child'] == new['generated_child']
        if cross_process and old['generated_child']:
            # Actual base/local unchanged loads already showed 463 generated
            # child GUID changes (GUID_EVIDENCE:57-70). The snapshot independently
            # confirms is_child_actor(). No general actor/component GUID is
            # excluded, and every within-process comparison remains strict.
            assert '_GEN_VARIABLE_' in old['name'] and '_GEN_VARIABLE_' in new['name']
            excluded.append(path)
            if old['guid'] != new['guid']:
                changed_guids.append(path)
            old['text'] = strip_generated_child_guid(old['text'], old['guid'])
            new['text'] = strip_generated_child_guid(new['text'], new['guid'])
            old.pop('guid')
            new.pop('guid')
        assert old == new, 'Unapproved existing actor/component change: ' + path
    return dict(cross_process=bool(cross_process), excluded_guid_paths=excluded,
                excluded_guid_count=len(excluded), changed_guid_paths=changed_guids)


def phase_targets(phases):
    result = []
    for phase in phases[:3]:
        encounter = phase.get_editor_property('Encounter')
        configured = list(encounter.get_editor_property('Spawners'))
        unique = {spawner.get_path_name(): spawner for spawner in configured if spawner}
        assert len(unique) == 1 and all(configured), 'Example target requires one valid unique configured spawner per encounter'
        spawner = next(iter(unique.values()))
        assert ue.MathLibrary.class_is_child_of(spawner.get_class(), native('DCEnemySpawner'))
        point = spawner.get_editor_property('SpawnPoint')
        assert point and isinstance(point, ue.SceneComponent) and point.get_owner() == spawner
        result.append(dict(encounter=encounter.get_path_name(), spawner=spawner.get_path_name(),
                           component=point.get_path_name(), transform=point.get_world_transform().export_text()))
    return result


def ensure_unconfigured(phases, actors):
    assert not {actor.get_name() for actor in actors}.intersection(MARKERS)
    assert not {actor.get_actor_label(False) for actor in actors}.intersection(MARKERS)
    for phase in phases:
        assert phase.get_editor_property('MissionList') is None
        assert not phase.get_editor_property('MissionTargets')


def configure_map(world, subsystem, actors, director, phases, data, sources):
    ensure_unconfigured(phases, actors)
    marker_class = ue.load_class(None, '/Script/Engine.TargetPoint')
    assert marker_class
    game_class = ue.load_class(None, '/Script/Engine.GameplayStatics')
    assert game_class
    game = ue.get_default_object(game_class)
    # Editor factory placement calls SetActorLabelUnique/FCachedActorLabels,
    # which creates default labels on unrelated actors. Native deferred spawn
    # avoids that factory path. World.cpp:585-608 defaults ObjectFlags to
    # RF_Transactional; SpawnActorDeferred preserves those non-transient flags.
    world.modify()
    markers = []
    for index, (name, source) in enumerate(zip(MARKERS, sources)):
        transform = ue.Transform()
        assert transform.import_text(source['transform'])
        marker = game.call_method('BeginDeferredActorSpawnFromClass', args=(world, marker_class, transform,
            ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        assert marker and marker.get_world() == world
        marker = game.call_method('FinishSpawningActor', args=(marker, transform,
            ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        assert marker and marker.get_class() == marker_class
        marker.modify()
        assert marker.rename(name), 'Could not give new marker its unique object name'
        marker.set_actor_label(name)
        marker.set_actor_transform(transform, False, True)
        assert marker.get_name() == name and marker.get_actor_label(False) == name
        assert marker.get_outer() == director.get_outer()
        assert marker.get_package() == world.get_package()
        root = marker.call_method('K2_GetRootComponent')
        assert root and root.get_outer() == marker and root.get_owner() == marker and root.get_world() == world
        assert marker.get_actor_transform().export_text() == source['transform']
        binding = struct('DCMissionTargetBinding')
        binding.set_editor_property('TargetKey', ue.Name('EncounterGoal'))
        binding.set_editor_property('TargetActor', marker)
        phases[index].set_editor_property('MissionList', data)
        phases[index].set_editor_property('MissionTargets', [binding])
        markers.append(marker)
    director.modify()
    # Phases is plain native configuration. The supported NEVER notify option
    # copies its data without PostEditChange/reconstructing the placed BP's
    # visualization components (PyWrapperOwnerContext.cpp:55, PropertyAccessUtil.cpp:669+).
    # Explicit Modify/dirty/map save and all strict component comparisons remain.
    director.set_editor_property('Phases', phases, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    return markers


def check_map(world, actors, phases, data, sources):
    marker_class = ue.load_class(None, '/Script/Engine.TargetPoint')
    result = []
    for index, name in enumerate(MARKERS):
        matches = [actor for actor in actors if actor.get_name() == name]
        assert len(matches) == 1
        marker = matches[0]
        assert marker.get_class() == marker_class and marker.get_actor_label(False) == name
        assert marker.get_actor_transform().export_text() == sources[index]['transform']
        assert phases[index].get_editor_property('MissionList') == data
        bindings = list(phases[index].get_editor_property('MissionTargets'))
        assert len(bindings) == 1 and str(bindings[0].get_editor_property('TargetKey')) == 'EncounterGoal'
        assert bindings[0].get_editor_property('TargetActor') == marker
        root = marker.call_method('K2_GetRootComponent')
        assert root and root.get_outer() == marker and root.get_owner() == marker and root.get_world() == world
        assert marker.get_world() == world and marker.get_package() == world.get_package()
        result.append(marker.get_path_name())
    assert phases[3].get_editor_property('MissionList') is None and not phases[3].get_editor_property('MissionTargets')
    return result


def read_export(path):
    raw = path.read_bytes()
    return raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n').replace('\r', '\n')


def export_identity(line):
    cls = re.findall(r'(?:^Begin Object| )Class=([^\s]+)', line)
    names = re.findall(r'\bName="([^\"]+)"', line)
    paths = re.findall(r'\bExportPath="([^\'\"]+)\'([^\'\"]+)\'"', line)
    assert len(names) == len(paths) == 1 and len(cls) <= 1, 'Unsupported export object identity'
    if cls:
        assert cls[0] == paths[0][0]
    return dict(cls=paths[0][0], name=names[0], path=paths[0][1])


def validate_cache_payload(payload):
    fields = dict(field.split('=', 1) for field in split_fields(payload))
    assert set(fields) <= {'bIsValid', 'Hash', 'Value'} and fields.get('bIsValid') == 'True'
    if 'Hash' in fields:
        assert re.fullmatch(r'[0-9]+', fields['Hash']) and 0 <= int(fields['Hash']) <= 0xffffffff
    if 'Value' in fields:
        bounds = dict(field.split('=', 1) for field in split_fields(fields['Value']))
        assert set(bounds) <= {'Origin', 'BoxExtent', 'SphereRadius'}
        for name, value in bounds.items():
            if name == 'SphereRadius':
                assert math.isfinite(float(value)) and float(value) >= 0
            else:
                vector = dict(field.split('=', 1) for field in split_fields(value))
                assert set(vector) <= {'X', 'Y', 'Z'}
                assert all(math.isfinite(float(number)) for number in vector.values())
                if name == 'BoxExtent':
                    assert all(float(number) >= 0 for number in vector.values())
    return fields


def known_cache_fields(text, policy=None, remove=False):
    """Only the nine independently audited Apply_D ISM paths/three indices.

    ISM PreSave fills these saved derived caches; this is never enabled for a
    normal compare/apply. Unknown paths remain in raw text and compare strictly.
    """
    if 'CachedBounds' not in text:
        return text, {}
    output, stack, found = [], [], {}
    for line in text.splitlines(keepends=True):
        stripped = line.strip()
        if stripped.startswith('Begin Object '):
            stack.append(export_identity(stripped))
        elif stripped == 'End Object':
            assert stack
            stack.pop()
        elif stack and stack[-1]['path'] in CACHE_PATHS and stripped.startswith('CachedBounds'):
            identity = stack[-1]
            assert identity['cls'] == CACHE_CLASS, 'Known cache path has a different native class'
            match = re.fullmatch(r'CachedBounds\(([0-9]+)\)=(.+)', stripped)
            assert match and int(match[1]) in (0, 1, 2), 'Unexpected cache index/field'
            index, payload = int(match[1]), match[2]
            validate_cache_payload(payload)
            values = found.setdefault(identity['path'], {})
            assert index not in values, 'Duplicate known cache index'
            values[index] = payload
            if policy is not None:
                assert identity['path'] in policy and policy[identity['path']].get(index) == payload, 'Known saved cache payload changed'
            if remove:
                assert policy is not None, 'Cache normalization requires audited saved payloads'
                continue
        output.append(line)
    assert not stack
    assert all(set(values) == {0, 1, 2} for values in found.values()), 'Partial known cache coverage'
    return ''.join(output), found


def check_known_save_receipt(previous, current_hashes, protected):
    assert previous['run'] == 'Apply_D' and previous['mode'] == 'apply' and previous['schema'] == SCHEMA
    assert previous['complete'] is False and previous['asset_saves'] == 4
    assert previous['saved'] == [PATHS[key] for key in ('entry', 'list', 'map', 'boss')]
    assert 'save_intent' not in previous and set(previous['after_hashes']) == set(PATHS)
    assert all(re.fullmatch(r'[0-9a-f]{64}', value or '') for value in previous['after_hashes'].values())
    assert previous['after_hashes'] == current_hashes, 'Known saved packages changed'
    assert previous['protected_hashes'] == protected
    for name in ('protected_inventory_unchanged', 'protected_hashes_unchanged', 'preserved_existing_map_and_boss_fields'):
        assert previous[name] is True, name
    comparisons = previous['map_comparisons']
    assert len(comparisons) == 2
    assert comparisons[0]['label'] == 'inspect_to_apply_reload' and comparisons[0]['cross_process'] is True
    assert comparisons[1]['label'] == 'apply_memory_edit' and comparisons[1]['cross_process'] is False
    assert comparisons[1]['excluded_guid_count'] == 0 and not comparisons[1]['excluded_guid_paths']
    assert not comparisons[1]['changed_guid_paths']
    error = previous['error']
    assert error.startswith('Traceback (most recent call last):\n')
    assert error.count('Traceback (most recent call last):') == 1 and error.count('AssertionError:') == 1
    assert "compare('apply_memory_save', report['configured_map'], saved_map)" in error
    assert error.splitlines()[-1] == 'AssertionError: Unapproved existing actor/component change: ' + CACHE_OWNER
    assert previous['reference'] == 'Inspect_G' and 'saved_map' not in previous
    assert previous['before_hashes']['list'] is None and previous['before_hashes']['entry'] is None
    return dict(original_apply_complete_false=True, original_apply='Apply_D',
                saved=list(previous['saved']), asset_saves=4, hashes=dict(current_hashes),
                original_failure=error, recovery_asset_saves=0)


def audit_known_saved_cache(previous, directory):
    """Read-only audit of the completed raw export after exactly four saves."""
    assert directory.resolve() == (REPORTS / 'Apply_D').resolve()
    baseline = previous['configured_map']
    assert len(baseline['actors']) == 5633
    components = {component['path']: component for row in baseline['actors'].values() for component in row['components']}
    assert len(components) == 6797 and all(path in components for path in CACHE_PATHS)
    assert len(components) == sum(len(row['components']) for row in baseline['actors'].values()), 'Duplicate configured component identity'
    after_folder = directory / 'after_save_map'
    actor_files, component_files = {}, {}
    for pattern, expected, target in (('actor_*.t3d', baseline['actors'], actor_files),
                                      ('component_*.t3d', components, component_files)):
        for path in sorted(after_folder.glob(pattern)):
            text = read_export(path)
            identity = export_identity(text.splitlines()[0])
            key = identity['path']
            assert key in expected and key not in target, 'Missing/extra/duplicate after-save export identity'
            assert identity['cls'] == expected[key]['cls']
            target[key] = (text, path)
        assert set(target) == set(expected), 'Incomplete after-save raw export set'
    policy, proof = {}, {}
    for path in CACHE_PATHS:
        before = components[path]
        assert before['cls'] == CACHE_CLASS
        assert known_cache_fields(before['text'])[1] == {}, 'Before-save cache must be absent'
        after_text, source_file = component_files[path]
        _, actual = known_cache_fields(after_text)
        assert set(actual) == {path} and set(actual[path]) == {0, 1, 2}
        policy[path] = actual[path]
        proof[path] = dict(cls=CACHE_CLASS, before_raw=before['text'], after_raw=after_text,
                          source_file=str(source_file), source_sha256=hud.sha(source_file),
                          payloads=actual[path])
    before_actor = baseline['actors'][CACHE_OWNER]['text']
    after_actor, actor_file = actor_files[CACHE_OWNER]
    assert known_cache_fields(before_actor)[1] == {}
    _, inline = known_cache_fields(after_actor, policy)
    assert inline == policy, 'Actor-inline cache payloads differ from separate component exports'
    for path, row in baseline['actors'].items():
        before, _ = normalize_inline_model(row['text'], row['brush_model'])
        after, _ = normalize_inline_model(actor_files[path][0], row['brush_model'])
        assert known_cache_fields(before, policy, remove=True)[0] == known_cache_fields(after, policy, remove=True)[0], 'Unexpected after-save actor raw change: ' + path
    for path, row in components.items():
        assert known_cache_fields(row['text'], policy, remove=True)[0] == known_cache_fields(component_files[path][0], policy, remove=True)[0], 'Unexpected after-save component raw change: ' + path
    models = {path: row['brush_model'] for path, row in baseline['actors'].items() if row['brush_model'] is not None}
    model_proof = previous['brush_model_export_coverage']['after_save_map']
    assert set(model_proof) == set(models)
    for path, model in models.items():
        item = model_proof[path]
        assert item['model_path'] == model['path'] and item['raw_reference'] == model['reference']
        for file_key, hash_key in (('model_file', 'model_sha256'), ('repeat_file', 'repeat_sha256')):
            source = Path(item[file_key])
            assert source.resolve().parent == after_folder.resolve()
            assert hud.sha(source) == item[hash_key] and read_export(source) == model['text']
        assert validate_model_geometry(model['text'], model['name']) == item['polygons']
    return policy, dict(component_count=9, entries_added=27, actor_inline_entries_added=27,
        component_evidence=proof, actor_before_raw=before_actor, actor_after_raw=after_actor,
        actor_source_file=str(actor_file), actor_source_sha256=hud.sha(actor_file),
        after_save_actor_raw_count=len(actor_files), after_save_component_raw_count=len(component_files),
        after_save_metadata_available=False, independent_model_geometry_equal=True,
        scope='Only exact Apply_D nine ISM paths, indices 0/1/2 and recorded payloads; every other raw field strict')


def require_fresh_saved_cache(snapshot, policy):
    # Verify_A observed all nine SCS components cold after reload, exactly like
    # configured_map before save. Non-edit CachedBounds need not survive SCS
    # reconstruction (ComponentInstanceDataCache.cpp:54-65); allow only this
    # observed complete absence, never different/partially retained payloads.
    components = {component['path']: component for row in snapshot['actors'].values() for component in row['components']}
    assert len(components) == sum(len(row['components']) for row in snapshot['actors'].values())
    for path in CACHE_PATHS:
        assert path in components and components[path]['cls'] == CACHE_CLASS
        assert known_cache_fields(components[path]['text'], policy)[1] == {}, 'Fresh cache must match the observed cold state: ' + path
    assert known_cache_fields(snapshot['actors'][CACHE_OWNER]['text'], policy)[1] == {}
    return dict(state='observed_cold', component_paths=list(CACHE_PATHS), entries_present=0,
                basis='Verify_A actual fresh raw and Apply_D configured_map before-save raw')


def main():
    command = ue.SystemLibrary.get_command_line()

    def arg(name, choices):
        values = re.findall(r'(?i)(?:^|\s)-' + name + '=(' + choices + r')(?=\s|$)', command)
        assert len(values) == 1, 'Explicit unique -' + name + ' required'
        return values[0]

    mode = arg('DCMissionHUDSetupMode', 'inspect|apply|verify').lower()
    run = arg('DCMissionHUDSetupRun', '[A-Za-z0-9_]{1,64}')
    recover_flags = re.findall(r'(?i)(?:^|\s)-DCMissionHUDRecoverSavedApply(?=\s|$)', command)
    recovery = bool(recover_flags)
    if '-dcmissionhudrecoversavedapply' in command.lower():
        assert len(recover_flags) == 1 and mode == 'verify', 'Known-saved recovery is verify-only'
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    assert all(flag in command.lower() for flag in (
        '-run=pythonscript', '-unattended', '-enableplugins=umgtoolset', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    hud.assert_no_other_editor()
    clean()
    directory = REPORTS / run
    log = directory / 'Editor.log'
    matches = re.findall(r'(?i)(?:^|\s)-abslog=(?:"([^"]+)"|(\S+))', command)
    assert len(matches) == 1 and Path(matches[0][0] or matches[0][1]).resolve() == log.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    assert all(path.name == 'Editor.log' for path in directory.iterdir()), 'Use a fresh evidence directory'
    before_files, protected, before_hashes = inventory(), hashes(), target_hashes()
    report = dict(schema=SCHEMA, mode=mode, run=run, pid=os.getpid(), complete=False,
                  saved=[], asset_saves=0, explicit_compile=0, cpp_build=False, PIE=False,
                  protected_hashes=protected, before_hashes=before_hashes,
                  map_comparisons=[],
                  source_evidence={
                      'cross_process_child_guids': dict(path=str(GUID_EVIDENCE),
                          sha256=hud.sha(GUID_EVIDENCE), lines='57-70',
                          observed_unchanged_load_child_guid_differences=463),
                      'object_declaration_format': dict(path=str(EXPORTER_EVIDENCE),
                          sha256=hud.sha(EXPORTER_EVIDENCE), lines='453-485,571-650'),
                      'editor_sprite_inline_omission': dict(path=str(SPRITE_EVIDENCE),
                          sha256=hud.sha(SPRITE_EVIDENCE), lines='902',
                          reason='Editor sprite uses RF_TextExportTransient; actor inline exporter omits flagged inners'),
                      'direct_model_geometry': dict(path=str(MODEL_EXPORT_EVIDENCE),
                          sha256=hud.sha(MODEL_EXPORT_EVIDENCE), lines='355-433',
                          reason='Direct ModelExporterT3D emits full Polys without instanced-property TagImp guard'),
                      'ism_presave_cache': dict(header=str(ISM_HEADER), header_sha256=hud.sha(ISM_HEADER),
                          header_lines='99-153,729-735,824-827', source=str(ISM_SOURCE),
                          source_sha256=hud.sha(ISM_SOURCE), source_lines='3064-3137,3495-3507',
                          meaning='Serialized derived Local/Component/Navigation bounds populated by registered ISM PreSave')},
                  component_export_coverage={},
                  brush_model_export_coverage={},
                  map_exporter='ObjectExporterT3D actor/component raw + independently repeated full ModelExporterT3D geometry + identity/transform/parent inventory; no LevelExporterT3D')
    cache_policy, previous = None, None
    report['fresh_reload_checked'] = False

    def compare(label, before, after, added=(), cross_process=False):
        # Fresh snapshots must be cold and compare without cache normalization.
        # The warm 27-payload save delta is audited separately from saved raw.
        evidence = compare_maps(before, after, added, cross_process=cross_process)
        evidence['label'] = label
        report['map_comparisons'].append(evidence)

    try:
        if recovery:
            reference = arg('DCMissionHUDSetupReference', '[A-Za-z0-9_]{1,64}')
            assert reference == 'Apply_D' and run != reference
            previous_file = REPORTS / reference / 'report.json'
            previous = json.loads(previous_file.read_text(encoding='utf-8'))
            report['known_save_receipt'] = check_known_save_receipt(previous, before_hashes, protected)
            report['original_apply_complete_false'] = True
            report['original_apply_report_sha256'] = hud.sha(previous_file)
            original_file = REPORTS / 'Inspect_G/report.json'
            original = json.loads(original_file.read_text(encoding='utf-8'))
            assert original['schema'] == SCHEMA and original['complete'] is True and original['mode'] == 'inspect'
            assert original['saved'] == [] and original['asset_saves'] == 0
            assert previous['baseline_map'] == original['baseline_map']
            assert previous['baseline_boss'] == original['baseline_boss']
            assert previous['before_hashes'] == original['after_hashes']
            assert previous['protected_hashes'] == original['protected_hashes']
            report['original_inspect_report_sha256'] = hud.sha(original_file)
            cache_policy, report['derived_cache_change_evidence'] = audit_known_saved_cache(previous, REPORTS / reference)
        settings = ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings')
        assert settings
        settings_text = hud.export(ue.get_default_object(settings), directory / 'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', settings_text, re.M) == ['SoC_Never']
        list_class, entry_class = native_contract()
        umg_class = ue.load_class(None, '/Script/UMGToolSet.UMGToolSet')
        assert umg_class
        umg = ue.get_default_object(umg_class)
        brushes = expected_brushes()
        report['brushes'] = {key: value.export_text() for key, value in brushes.items()}
        world, subsystem, actors, director, phases = get_map()
        clean()
        boss = ue.load_asset(PATHS['boss'])
        assert boss
        check_boss(boss, 48 if mode == 'verify' else 88)
        loaded_boss = hud.export(boss, directory / 'loaded_boss.t3d')
        loaded_map = map_snapshot(world, actors, director, directory, 'loaded_map', report['brush_model_export_coverage'])
        report['component_export_coverage']['loaded_map'] = component_export_coverage(loaded_map)
        sources = phase_targets(phases)
        report.update(loaded_boss=loaded_boss, loaded_map=loaded_map, sources=sources)
        if recovery:
            report['fresh_cache_state'] = require_fresh_saved_cache(loaded_map, cache_policy)
        if mode == 'inspect':
            for key in NEW:
                assert not disk(key).exists() and not ue.EditorAssetLibrary.does_asset_exist(PATHS[key]), 'New destination already exists: ' + key
            ensure_unconfigured(phases, actors)
            report.update(baseline_boss=loaded_boss, baseline_map=loaded_map)
        else:
            reference = arg('DCMissionHUDSetupReference', '[A-Za-z0-9_]{1,64}')
            assert reference != run
            if not recovery:
                previous = json.loads((REPORTS / reference / 'report.json').read_text(encoding='utf-8'))
                assert previous['schema'] == SCHEMA and previous['complete'], 'Use a fresh schema-2 reference with full model geometry'
            assert previous['mode'] == ('inspect' if mode == 'apply' else 'apply')
            assert previous['pid'] != report['pid']
            assert previous['protected_hashes'] == protected and previous['after_hashes'] == before_hashes
            expected_source = dict(report['source_evidence'])
            if recovery:
                expected_source.pop('ism_presave_cache')
            assert previous['source_evidence'] == expected_source
            assert previous['brushes'] == report['brushes'] and previous['sources'] == sources
            baseline_boss, baseline_map = previous['baseline_boss'], previous['baseline_map']
            report.update(reference=reference, baseline_boss=baseline_boss, baseline_map=baseline_map)
            if mode == 'apply':
                for key in NEW:
                    assert not disk(key).exists() and not ue.EditorAssetLibrary.does_asset_exist(PATHS[key]), 'Existing/partial destination requires review: ' + key
                assert loaded_boss == baseline_boss
                compare('inspect_to_apply_reload', baseline_map, loaded_map, cross_process=True)
                ensure_unconfigured(phases, actors)
                entry = create_entry(entry_class, umg)
                report['explicit_compile'] += 1
                data = create_list(list_class, entry, brushes)
                markers = configure_map(world, subsystem, actors, director, phases, data, sources)
                style = boss.get_editor_property('HUDStyle')
                style.set_editor_property('PanelOffset', ue.Vector2D(0, 48))
                boss.set_editor_property('HUDStyle', style)
                actors = list(subsystem.get_all_level_actors())
                phases = list(director.get_editor_property('Phases'))
                assert len(actors) == len(loaded_map['actors']) + 3 or (
                    world.get_world_settings().get_path_name() not in {actor.get_path_name() for actor in actors}
                    and len(actors) + 1 == len(loaded_map['actors']) + 3)
                assert set(marker.get_name() for marker in markers) == set(MARKERS)
            else:
                assert set(previous['saved']) == set(PATHS.values()) and previous['asset_saves'] == 4
                saved_basis = previous['configured_map'] if recovery else previous['saved_map']
                compare('known_saved_apply_to_verify_reload' if recovery else 'apply_to_verify_reload',
                        saved_basis, loaded_map, cross_process=True)
                data, entry = ue.load_asset(PATHS['list']), ue.load_asset(PATHS['entry'])
                assert data and entry
                # Verify is a separate reload, with no explicit compile/save.
            check_list(data, list_class, entry, brushes)
            report['entry_cdo'] = check_entry(entry, entry_class, umg, directory, 'configured')
            report['configured_list'] = hud.export(data, directory / 'configured_list.t3d')
            if mode == 'verify':
                assert report['entry_cdo'] == previous['entry_cdo']
                assert report['configured_list'] == previous['configured_list']
            check_boss(boss, 48)
            report['configured_boss'] = hud.export(boss, directory / 'configured_boss.t3d')
            assert strip_boss_offset(baseline_boss) == strip_boss_offset(report['configured_boss']), 'Unapproved boss data/style edit'
            added = check_map(world, actors, phases, data, sources)
            report['configured_map'] = map_snapshot(world, actors, director, directory, 'configured_map', report['brush_model_export_coverage'])
            report['component_export_coverage']['configured_map'] = component_export_coverage(report['configured_map'])
            if mode == 'apply':
                compare('apply_memory_edit', loaded_map, report['configured_map'], added)
            else:
                compare('verify_memory_unchanged', loaded_map, report['configured_map'])
            report['preserved_existing_map_and_boss_fields'] = True
            if recovery:
                require_fresh_saved_cache(report['configured_map'], cache_policy)
                assert report['asset_saves'] == 0 and report['saved'] == [] and report['explicit_compile'] == 0
                report['fresh_reload_checked'] = True
            clean((PATHS['list'], PATHS['entry'], PATHS['boss']), (PATHS['map'],))
            hud.log_check(log)
            assert inventory() == before_files and hashes() == protected
            assert target_hashes() == before_hashes, 'Implicit package save detected'
            if mode == 'apply':
                hud.assert_no_other_editor()
                objects = {'entry': entry, 'list': data, 'boss': boss}
                # Save dependencies first. Persist each exact save intent/result
                # so an interrupted attempt cannot be mistaken for completion.
                for key in ('entry', 'list', 'map', 'boss'):
                    report['save_intent'] = PATHS[key]
                    hud.persist(directory / 'report.json', report)
                    if key == 'map':
                        assert ue.EditorLoadingAndSavingUtils.save_map(world, PATHS[key])
                    else:
                        assert ue.EditorAssetLibrary.save_loaded_asset(objects[key], only_if_is_dirty=False)
                    report['saved'].append(PATHS[key])
                    report['asset_saves'] += 1
                    report['after_hashes'] = target_hashes()
                    hud.persist(directory / 'report.json', report)
                report.pop('save_intent', None)
                # Saving must not mutate any existing actor/component either.
                saved_map = map_snapshot(world, list(subsystem.get_all_level_actors()), director, directory, 'after_save_map', report['brush_model_export_coverage'])
                report['component_export_coverage']['after_save_map'] = component_export_coverage(saved_map)
                compare('apply_memory_save', report['configured_map'], saved_map)
                report['saved_map'] = saved_map
            clean()
        report['after_hashes'] = target_hashes()
        if mode != 'apply':
            assert report['after_hashes'] == before_hashes
        hud.log_check(log)
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['protected_inventory_unchanged'] = inventory() == before_files
        report['protected_hashes_unchanged'] = hashes() == protected
        report['after_hashes'] = target_hashes()
        report['complete'] = report['complete'] and report['protected_inventory_unchanged'] and report['protected_hashes_unchanged']
        hud.persist(directory / 'report.json', report)
    assert report['complete'], 'Mission setup protection/verification failed'
    print('MISSION_HUD_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'run', 'complete', 'saved', 'asset_saves')}))


if __name__ == '__main__':
    main()
