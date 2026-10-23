"""Final-approved Level_1/2 registration and 75m setup, through Unreal only.

Modes inspect/apply/verify; fresh -DCSpawnerRegistrationRun and, for apply/verify,
-DCSpawnerRegistrationReference. Separate unattended Python commandlet, SCC None.
Only two maps, the existing spawner BP and ONE new mission list may be saved.
No C++ build, Config/Git edits, binary asset IO, environment/spawner movement,
GameMode/movie changes or Level_3 edit. Partial saves are journaled, never retried
blindly. Uses the existing map/component/brush preservation implementation.
"""
import gzip
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / 'Saved/Diagnostics/SpawnerRegistration20261024'
MAPS = ['/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_' + str(i) for i in (1, 2)]
SPAWNER = '/Game/DreamCatcher/Blueprints/AI/BP_DC_LyraFiniteSpawner'
MISSION = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Level2'
MISSION_SOURCE = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_LevelExample'
MISSION_ONE = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example'
MODES = ['/Game/DreamCatcher/Blueprints/Core/BP_DC_Level' + str(i) + 'GameMode' for i in (1, 2, 3)]
WRITES = MAPS + [SPAWNER, MISSION]
GROUPS = [[1, 2, 7, 8, 9, 10, 13, 14, 18, 19, 20, 21, 22, 23, 24], [3, 4, 5, 6, 11, 12, 15, 16, 17]]
REMOVED = ['BP_DC_LyraFiniteSpawner_C_0', 'BP_DC_LyraFiniteSpawner_C_1', 'BP_DC_BossSpawner_C_0']
NEW_NAMES = ['DC_Level2_PlayerStart', 'DC_Level2_Encounter', 'DC_Level2_StageDirector', 'DC_Level2_NavBounds', 'DC_Level2_Recast']


def module(name):
    spec = importlib.util.spec_from_file_location('dc_registration_' + name, Path(__file__).with_name(name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def disk(package):
    return PROJECT / ('Content/' + package[6:] + ('.umap' if package in MAPS else '.uasset'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    allowed = {disk(p) for p in WRITES}
    return {p.relative_to(PROJECT).as_posix(): [p.stat().st_size, p.stat().st_mtime_ns]
            for top in ('Content', 'Source', 'Config', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames')
            for p in (PROJECT / top).rglob('*') if p.is_file() and p not in allowed}


def protected_hashes():
    names = ['.git/HEAD', '.git/index', 'AGENTS.md', 'DreamCatcher.uproject',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    files = [PROJECT / name for name in names] + [disk(p) for p in MODES + [MISSION_ONE, MISSION_SOURCE,
        '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience']]
    files += [p for top in ('Source', 'Config') for p in (PROJECT / top).rglob('*') if p.is_file()]
    return {p.relative_to(PROJECT).as_posix(): sha(p) for p in files}


def inputs():
    return {p: sha(disk(p)) if disk(p).is_file() else None for p in WRITES}


def label(number):
    return 'BP_DC_LyraFiniteSpawner' + (str(number) if number != 1 else '')


def coords(value):
    return [value.x, value.y, value.z]


def save_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


def save_environment(path, value):
    assert not path.exists()
    with gzip.open(path, 'wt', encoding='utf-8') as stream:
        json.dump(value, stream, ensure_ascii=False)


def read_environment(path):
    with gzip.open(path, 'rt', encoding='utf-8') as stream:
        return json.load(stream)


def player_start_export(text, row=None):
    # Apply_D versus Preflight_A: native direct-component enumeration changed.
    # Validate against the independent component inventory, preserving every
    # complete declaration/definition block and every actor property verbatim.
    names = {'CollisionCapsule', 'Arrow', 'Sprite', 'Sprite2'} if row is None else {
        pair[1] for pair in row['inline_declarations']}
    if row is not None:
        assert names <= {component['path'].rsplit('.', 1)[-1] for component in row['components']}
    lines = text.splitlines(keepends=True)
    blocks, index = {}, 1
    while index < len(lines) and lines[index].startswith('   Begin Object '):
        header = lines[index]
        name = re.search(r'\bName="([^"]+)"', header)
        assert name and name.group(1) in names, header
        key = (header.startswith('   Begin Object Class='), name.group(1))
        assert key not in blocks
        end = index + 1
        while end < len(lines) and lines[end] != '   End Object\n':
            assert not lines[end].lstrip().startswith(('Begin Object', 'End Object'))
            end += 1
        assert end < len(lines)
        blocks[key] = ''.join(lines[index:end + 1])
        index = end + 1
    assert set(blocks) == {(kind, name) for kind in (True, False) for name in names}
    return lines[0] + ''.join(blocks[key] for key in sorted(blocks)) + ''.join(lines[index:])


def compare_environment(before, after, cross_process=False, allow_factory_child_labels=False):
    # The volume factory calls GetActorLabel on construction-generated children.
    # label_diagnostic_B records the exact empty -> generated label per child.
    # Keep raw exports intact. Fresh reload verification does NOT allow this
    # difference: it must reproduce the original empty child labels on disk.
    changed = []
    original, normalized = copy.deepcopy(before), copy.deepcopy(after)
    native_classes = {'/Script/Engine.' + name for name in ('DecalActor', 'LocalFogVolume',
        'TargetPoint', 'DirectionalLight', 'ExponentialHeightFog', 'SkyAtmosphere', 'SkyLight', 'VolumetricCloud')}
    native_classes.add('/Script/DreamCatcher.DCLyraPlayerStart')
    for view in (original, normalized):
        for path, row in view['actors'].items():
            if row['cls'] in native_classes:
                row['text'] = player_start_export(row['text'], row)
    if allow_factory_child_labels:
        known = json.loads((ROOT / 'label_diagnostic_B.json').read_text(encoding='utf-8'))['changed']
        known = {row['name']: row for row in known}
        for path, old in before['actors'].items():
            new = normalized['actors'][path]
            if old['label'] == new['label']:
                continue
            rule = known.get(old['name'])
            assert rule and old['generated_child'] and old['cls'] == '/Script/Engine.StaticMeshActor'
            assert old['label'] == rule['before'] == '' and new['label'] == rule['materialized']
            new['label'] = ''
            new['text'] = strip_properties(new['text'], ('ActorLabel',))
            if old['text'].endswith('\n'):
                new['text'] += '\n'
            changed.append(path)
    result = evidence.compare_maps(original, normalized, cross_process=cross_process)
    result['generated_child_label_cache_materialized'] = changed
    return result


def native(name):
    value = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert value, name
    return value


def load(package):
    value = ue.load_asset(package)
    assert value, package
    return value


def cdo(package):
    return ue.get_default_object(load(package).generated_class())


def actors(world):
    return list(ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors())


def of_type(world, name):
    return list(ue.GameplayStatics.get_all_actors_of_class(world, native(name)))


def strip_properties(text, names):
    return '\n'.join(line for line in text.splitlines()
                     if not any(line.startswith('   ' + n + '=') or line.startswith('   ' + n + '(') for n in names))


def record(actor, directory, name):
    return dict(path=actor.get_path_name(), cls=actor.get_class().get_path_name(),
                transform=actor.get_actor_transform().export_text(), label=actor.get_actor_label(False),
                text=evidence.hud.export(actor, directory / (name + '.t3d')))


def spawner_table(world, index, before=False):
    result = {a.get_actor_label(False): a for a in of_type(world, 'DCEnemySpawner')}
    expected = {label(i) for i in range(1, 25 if index == 0 else 18)}
    if before and index == 0:
        expected |= {'Level 1 Lyra Spawner 1', 'Level 1 Lyra Spawner 2', 'Level 1 Boss Spawner'}
    assert set(result) == expected, 'Spawner placement/labels changed since approval: ' + str(sorted(result))
    return result


def environment(world, index, directory, name, expected_paths=None):
    mutable = {a.get_path_name() for kind in ('DCEnemySpawner', 'DCEncounterController', 'DCStageDirector')
               for a in of_type(world, kind)}
    mutable |= {a.get_path_name() for a in ue.GameplayStatics.get_all_actors_of_class(world, ue.PlayerStart)} if index == 1 else set()
    selected = [a for a in actors(world) if a.get_path_name() not in mutable]
    if expected_paths is not None:
        selected = [a for a in selected if a.get_path_name() in expected_paths]
    state = evidence.map_snapshot(world, selected, world.get_world_settings(), directory, name, {})
    settings = state['actors'].pop(state['settings'])
    if index == 1:
        settings['text'] = strip_properties(settings['text'], ('DefaultGameMode',))
    settings['components'] = sorted(settings['components'], key=lambda row: row['path'])
    settings['components_not_inline'] = sorted(settings['components_not_inline'])
    state['world_settings'] = settings
    return state


def set_property(obj, name, value):
    obj.set_editor_property(name, value, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)


def configure_spawner(actor):
    for name, value in dict(PlayerTriggerRadius=7500.0, bRequirePlayerProximity=True, SpawnCount=1,
                            MaxAliveAtOnce=1, bActivateOnBeginPlay=False).items():
        set_property(actor, name, value)


def binding(actor):
    value = ue.DCMissionTargetBinding()
    value.set_editor_property('TargetKey', ue.Name('EncounterGoal'))
    value.set_editor_property('TargetActor', actor)
    return value


def clone_phase(value):
    result = ue.DCStagePhaseDefinition()
    assert result.import_text(value.export_text())
    return result


def add_level2_navigation(world, low, high):
    # The existing factory helper uses SetActorLabel, which materializes a
    # default label when asked to restore an empty label (Preflight_A).
    # Restore the reflected ActorLabel value instead, then compare the full map.
    original_labels = {a.get_name(): a.get_actor_label(False) for a in actors(world)}
    center = [(a + b) / 2 for a, b in zip(low, high)]
    half = [(b - a) / 2 for a, b in zip(low, high)]
    volume = ue.get_editor_subsystem(ue.EditorActorSubsystem).spawn_actor_from_class(ue.NavMeshBoundsVolume, ue.Vector(*center))
    assert volume and volume.rename(NEW_NAMES[3]); volume.set_actor_label(NEW_NAMES[3])
    _, original_extent = volume.get_actor_bounds(False)
    assert min(coords(original_extent)) > 0
    volume.set_actor_scale3d(ue.Vector(*[a / b for a, b in zip(half, coords(original_extent))]))
    navs = list(ue.GameplayStatics.get_all_actors_of_class(world, ue.RecastNavMesh))
    assert len(navs) <= 1
    nav = navs[0] if navs else progression.spawn(world, ue.RecastNavMesh, NEW_NAMES[4], center)
    if nav.get_name() != NEW_NAMES[4]:
        assert nav.rename(NEW_NAMES[4]); nav.set_actor_label(NEW_NAMES[4])
    set_property(nav, 'RuntimeGeneration', ue.RuntimeGenerationType.DYNAMIC)
    set_property(nav, 'bCanBeMainNavData', True)
    restored = []
    known = {row['name']: row for row in json.loads((ROOT / 'label_diagnostic_B.json').read_text(encoding='utf-8'))['changed']}
    for actor in actors(world):
        name = actor.get_name()
        if name in original_labels and actor.get_actor_label(False) != original_labels[name]:
            if original_labels[name]:
                actor.set_actor_label(original_labels[name], False)
                assert actor.get_actor_label(False) == original_labels[name], name
            else:
                assert actor.is_child_actor() and name in known and actor.get_actor_label(False) == known[name]['materialized']
            restored.append(name)
    system = ue.NavigationSystemV1.get_navigation_system(world)
    assert system
    system.on_navigation_bounds_updated(volume)
    return restored


def create_mission():
    result = ue.AssetToolsHelpers.get_asset_tools().duplicate_asset(MISSION.rsplit('/', 1)[1], MISSION.rsplit('/', 1)[0], load(MISSION_SOURCE))
    assert result
    expected_style = load(MISSION_SOURCE).get_editor_property('HUDStyle').export_text()
    definition = evidence.make_definition(('DefeatNightmares', '악몽 처치하기', 'CurrentEncounterDefeats', 'EncounterGoal'))
    set_property(result, 'Missions', [definition])
    set_property(result, 'DefaultTrackedMissionId', ue.Name('DefeatNightmares'))
    assert result.get_editor_property('HUDStyle').export_text() == expected_style
    return result


def map_facts(world, index):
    table = spawner_table(world, index)
    for actor in table.values():
        assert actor.get_editor_property('SpawnCount') == actor.get_editor_property('MaxAliveAtOnce') == 1
        assert actor.get_editor_property('bRequirePlayerProximity') is True
        assert actor.get_editor_property('bActivateOnBeginPlay') is False
        assert abs(actor.get_editor_property('PlayerTriggerRadius') - 7500) < .001
    directors = of_type(world, 'DCStageDirector')
    assert len(directors) == 1 and not directors[0].get_editor_property('bAutoStartStage')
    phases = list(directors[0].get_editor_property('Phases'))
    groups = GROUPS if index == 0 else [list(range(1, 18))]
    assert len(phases) == len(groups) + 1
    expected_anchors = [20, 16] if index == 0 else [17]
    counts = []
    for n, group in enumerate(groups):
        phase = phases[n]
        assert phase.get_editor_property('PhaseType') == ue.DCStagePhaseType.ENCOUNTER
        encounter = phase.get_editor_property('Encounter')
        assert encounter and not encounter.get_editor_property('bAutoStartOnBeginPlay')
        registered = list(encounter.get_editor_property('Spawners'))
        assert registered == [table[label(i)] for i in group]
        assert phase.get_editor_property('MissionList') == load(MISSION_ONE if index == 0 else MISSION)
        targets = list(phase.get_editor_property('MissionTargets'))
        assert len(targets) == 1 and str(targets[0].get_editor_property('TargetKey')) == 'EncounterGoal'
        assert targets[0].get_editor_property('TargetActor') == table[label(expected_anchors[n])]
        counts.append(sum(a.get_planned_enemy_count() for a in registered))
    assert phases[-1].get_editor_property('PhaseType') == ue.DCStagePhaseType.COMPLETE
    assert not phases[-1].get_editor_property('Encounter')
    assert world.get_world_settings().get_editor_property('DefaultGameMode') == load(MODES[index]).generated_class()
    if index == 0:
        assert not any(a.get_name() in REMOVED for a in actors(world))
        idle = next(a for a in of_type(world, 'DCEncounterController') if a.get_name() == 'BP_EncounterController_C_2')
        assert not list(idle.get_editor_property('Spawners')) and not idle.get_editor_property('bAutoStartOnBeginPlay')
    else:
        assert {a.get_name() for a in actors(world)} >= set(NEW_NAMES)
        starts = list(ue.GameplayStatics.get_all_actors_of_class(world, ue.PlayerStart))
        assert len(starts) == 1 and starts[0].get_class() == native('DCLyraPlayerStart')
        assert len(list(ue.GameplayStatics.get_all_actors_of_class(world, ue.NavMeshBoundsVolume))) == 1
        navs = list(ue.GameplayStatics.get_all_actors_of_class(world, ue.RecastNavMesh))
        assert len(navs) == 1 and navs[0].get_editor_property('RuntimeGeneration') == ue.RuntimeGenerationType.DYNAMIC
    return dict(counts=counts, spawners={name: dict(name=a.get_name(), transform=a.get_actor_transform().export_text(),
                position=coords(a.get_editor_property('SpawnPoint').get_world_location())) for name, a in table.items()},
                phases=[p.export_text() for p in phases])


def compare_facts(before, after, index):
    original, current = copy.deepcopy(before), copy.deepcopy(after)
    if index == 1:
        # Saving new FText assigns localization identities. Keep raw exports in
        # reports; compare only this engine-generated wrapper by source text.
        pattern = r'PhaseTitle=NSLOCTEXT\("", "[A-F0-9]{32}", ("(?:[^"\\]|\\.)*")\)'
        for view in (original, current):
            view['phases'] = [re.sub(pattern, r'PhaseTitle=\1', phase) for phase in view['phases']]
    assert original == current, 'Saved registration values changed'


def configure_map(world, index, mission):
    table = spawner_table(world, index, before=True)
    original = {a.get_name(): a for a in actors(world)}
    old_controls = {}
    for name, actor in table.items():
        if name.startswith('BP_DC_LyraFiniteSpawner'):
            configure_spawner(actor)
    if index == 0:
        director = of_type(world, 'DCStageDirector')[0]
        phases = list(director.get_editor_property('Phases'))
        assert len(phases) == 4
        kept = [clone_phase(phases[i]) for i in (0, 1, 3)]
        for n, group in enumerate(GROUPS):
            encounter = phases[n].get_editor_property('Encounter')
            set_property(encounter, 'Spawners', [table[label(i)] for i in group])
            kept[n].set_editor_property('MissionTargets', [binding(table[label([20, 16][n])])])
            assert evidence.without_fields(phases[n].export_text(), {'MissionTargets'}) == evidence.without_fields(kept[n].export_text(), {'MissionTargets'})
        set_property(phases[2].get_editor_property('Encounter'), 'Spawners', [])
        set_property(director, 'Phases', kept)
        for name in REMOVED:
            assert name in original and ue.get_editor_subsystem(ue.EditorActorSubsystem).destroy_actor(original[name])
        assert {a.get_name() for a in actors(world)} == set(original) - set(REMOVED)
    else:
        assert not of_type(world, 'DCStageDirector') and not of_type(world, 'DCEncounterController')
        boxes = [a.get_actor_bounds(True) for name, a in original.items() if name != 'PlayerStart_1']
        boxes = [(c, e) for c, e in boxes if max(coords(e)) > 1]
        assert boxes
        low = [min(coords(c)[i] - coords(e)[i] for c, e in boxes) - 500 for i in range(3)]
        high = [max(coords(c)[i] + coords(e)[i] for c, e in boxes) + 500 for i in range(3)]
        starts = list(ue.GameplayStatics.get_all_actors_of_class(world, ue.PlayerStart))
        assert len(starts) == 1 and starts[0].get_name() == 'PlayerStart_1' and starts[0].get_class() == ue.PlayerStart.static_class()
        transform = starts[0].get_actor_transform()
        old_controls['start_transform'] = transform.export_text()
        tag = starts[0].get_editor_property('PlayerStartTag')
        assert ue.get_editor_subsystem(ue.EditorActorSubsystem).destroy_actor(starts[0])
        start = progression.spawn(world, native('DCLyraPlayerStart'), NEW_NAMES[0], coords(transform.translation), [0, 0, 0])
        start.set_actor_transform(transform, False, True)
        set_property(start, 'PlayerStartTag', tag)
        assert start.get_actor_transform().export_text() == old_controls['start_transform']
        encounter = progression.spawn(world, ue.load_class(None, '/Game/Blueprint/Stage/BP_EncounterController.BP_EncounterController_C'), NEW_NAMES[1], [0, 0, 0])
        set_property(encounter, 'Spawners', [table[label(i)] for i in range(1, 18)])
        set_property(encounter, 'bAutoStartOnBeginPlay', False)
        director = progression.spawn(world, ue.load_class(None, '/Game/Blueprint/Stage/BP_StageDirector.BP_StageDirector_C'), NEW_NAMES[2], [0, 0, 0])
        set_property(director, 'bAutoStartStage', False)
        phase = ue.DCStagePhaseDefinition(); assert phase.import_text('(PhaseType=Encounter,PhaseTitle="전투")')
        phase.set_editor_property('Encounter', encounter); phase.set_editor_property('MissionList', mission)
        phase.set_editor_property('MissionTargets', [binding(table[label(17)])])
        complete = ue.DCStagePhaseDefinition(); assert complete.import_text('(PhaseType=Complete,PhaseTitle="완료")')
        set_property(director, 'Phases', [phase, complete])
        set_property(world.get_world_settings(), 'DefaultGameMode', load(MODES[1]).generated_class())
        old_controls['factory_labels_restored'] = add_level2_navigation(world, low, high)
        assert {a.get_name() for a in actors(world)} == (set(original) - {'PlayerStart_1'}) | set(NEW_NAMES)
        old_controls['navigation_bounds'] = [low, high]
    return old_controls


def main():
    global ue, evidence, progression
    import unreal as ue
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCSpawnerRegistrationMode=(inspect|apply|verify)\b', command).group(1)
    run = re.search(r'-DCSpawnerRegistrationRun=([A-Za-z0-9_]+)\b', command).group(1)
    reference_match = re.search(r'-DCSpawnerRegistrationReference=([A-Za-z0-9_]+)\b', command)
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower() and not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.project_dir()).resolve() == PROJECT.resolve()
    evidence = module('mission_hud_setup')
    progression = module('level_progression_setup'); progression.ue = ue; progression.evidence = evidence
    proximity = module('spawner_proximity_setup')
    evidence.hud.assert_no_other_editor()
    directory = ROOT / run; assert not directory.exists(), 'Use a fresh run; inspect partial save receipts first'
    directory.mkdir(parents=True)
    output = directory / 'report.json'
    report = dict(mode=mode, run=run, complete=False, saved=[], saved_hashes={}, map_checks={}, environments={}, cpp_build=False, PIE=False)
    before_inventory, hashes, initial_inputs = inventory(), protected_hashes(), inputs()
    report.update(input_hashes=initial_inputs, protected_hashes=hashes)
    reference = json.loads((ROOT / reference_match.group(1) / 'report.json').read_text(encoding='utf-8')) if reference_match else None
    recovery = None
    if '-DCSpawnerRegistrationReuseVerifiedEnvironment' in command:
        assert mode == 'verify' and reference['run'] == 'Apply_G'
        recovery = json.loads((ROOT / 'Verify_A/report.json').read_text(encoding='utf-8'))
        assert recovery['input_hashes'] == initial_inputs and recovery['saved'] == []
        assert recovery['protected_hashes'] == hashes and recovery['protected_hashes_unchanged']
        assert not recovery['protected_inventory_changes'] and not recovery['new_protected_files']
        assert set(recovery['environment_checks']) == set(MAPS)
        for index, package in enumerate(MAPS):
            compare_facts(reference['map_checks'][package], recovery['map_checks'][package], index)
        report['reused_saved_environment_verification'] = 'Verify_A; unchanged inputs/runtime; only new FText localization IDs differ'
    resume = '-DCSpawnerRegistrationResumePreflight' in command
    reuse_match = re.search(r'-DCSpawnerRegistrationReuseLevel1=([A-Za-z0-9_]+)\b', command)
    reused = None
    if reuse_match:
        assert mode == 'apply' and reuse_match.group(1) == 'Apply_E'
        reused = json.loads((ROOT / 'Apply_E/report.json').read_text(encoding='utf-8'))
        assert reused['saved'] == [] and reused['input_hashes'] == initial_inputs
        assert reused['protected_hashes_unchanged'] and reused['protected_inventory_changes'] == ['Scripts/Editor/bigbrother_boss_setup.py']
        assert 'Protected files changed before save' in reused['failure']
        assert all(hashes.get(name) == value for name, value in reused['protected_hashes'].items())
        assert MAPS[0] in reused['environment_checks'] and (ROOT / 'Apply_E/comparison_0.json.gz').is_file()
        report['reused_level1_preservation_check'] = 'Apply_E; exact map/asset/runtime input hashes; fresh saved-map reload still required'
    if resume:
        assert mode == 'inspect' and reference and reference['mode'] == 'inspect' and reference['saved'] == []
        assert reference['input_hashes'] == initial_inputs
        external = [name for name, value in reference['protected_hashes'].items() if hashes.get(name) != value]
        allowed_external = {'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
            'Source/DreamCatcher/AbilitySystem/Abilities/DCGameplayAbility_EnemyRangedAttack.cpp',
            'Source/DreamCatcher/AI/DCGASEnemyCharacter.cpp', 'Source/DreamCatcher/AI/DCGASEnemyCharacter.h',
            'Source/DreamCatcher/AI/Boss/DCBossCharacter.cpp', 'Source/DreamCatcher/AI/Boss/DCBossCharacter.h',
            'Source/DreamCatcher/Tests/DCBossAutomationTests.cpp'}
        assert set(external) <= allowed_external, 'Relevant gameplay/config dependencies changed: ' + str(external)
        report['external_combat_changes_preserved'] = external
        assert reference['preflight_checks'][MAPS[0]]['counts'] == [15, 9]
        assert 'Editor factory changed an unrestorable label' in reference['failure']
        report['reused_level1_preflight'] = reference['run']
        for key in ('placements', 'environments', 'memory_preflight', 'preflight_checks'):
            report.setdefault(key, {})[MAPS[0]] = reference[key][MAPS[0]]

    def persist():
        save_json(output, report)

    def save(asset, package):
        assert package in WRITES and package not in report['saved']
        evidence.hud.assert_no_other_editor()
        assert protected_hashes() == hashes and inventory() == before_inventory, 'Protected files changed before save'
        report['save_intent'] = package; persist()
        if package in MAPS:
            assert asset.get_path_name() == package + '.' + package.rsplit('/', 1)[-1]
            assert ue.EditorLoadingAndSavingUtils.save_map(asset, package)
        else:
            assert asset.get_path_name().split('.')[0] == package
            assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        report['saved'].append(package); report['saved_hashes'][package] = sha(disk(package))
        report.pop('save_intent'); persist()

    try:
        assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages() and not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        settings = ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
        text = evidence.hud.export(settings, directory / 'compile_settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', text, re.M) == ['SoC_Never']
        if mode in ('inspect', 'apply'):
            assert not disk(MISSION).exists() and not ue.EditorAssetLibrary.does_asset_exist(MISSION)
        if mode != 'inspect':
            assert reference and reference['complete']
            expected = reference['input_hashes'] if mode == 'apply' else reference['saved_hashes']
            assert expected == initial_inputs, 'Inputs differ from the completed reference'
        if mode == 'apply':
            assert reference['mode'] == 'inspect'
            changed_dependencies = [name for name, value in reference['protected_hashes'].items() if hashes.get(name) != value]
            # Preserve the combat task's latest damage values. Freeze the current
            # files for this run and prove that its Experience change leaves the
            # pawn/AI/component configuration identical to the inspected export.
            experience_path = 'Content/DreamCatcher/Experiences/B_DC_StageUIExperience.uasset'
            assert set(changed_dependencies) <= {'Source/DreamCatcher/Tests/DCBaseDamageAutomationTests.cpp', experience_path}, changed_dependencies
            if experience_path in changed_dependencies:
                reviewed = ROOT.parent / 'CombatValues20261024'
                old = (reviewed / '0_B_DC_StageUIExperience.t3d').read_text(encoding='utf-8')
                prior = json.loads((reviewed / 'prechange_review.json').read_text(encoding='utf-8'))
                experience_package = '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience'
                assert reference['protected_hashes'][experience_path] == prior['sha256'][experience_package]
                current = evidence.hud.export(cdo(experience_package), directory / 'experience_current.t3d')
                damage_fields = ('bUseRoleBasedBaseDamage', 'PlayerBaseDamage', 'EnemyBaseDamage')
                assert strip_properties(old, damage_fields) == strip_properties(current, damage_fields)
                report['external_damage_values_preserved'] = {name: cdo(experience_package).get_editor_property(name) for name in damage_fields}
            report['external_changes_preserved'] = changed_dependencies
            bp = load(SPAWNER)
            before = evidence.hud.export(cdo(SPAWNER), directory / 'spawner_before.t3d')
            assert cdo(SPAWNER).get_editor_property('PlayerTriggerRadius') == 2000
            cdo(SPAWNER).set_editor_property('PlayerTriggerRadius', 7500.0)
            after = evidence.hud.export(cdo(SPAWNER), directory / 'spawner_after.t3d')
            assert proximity.cdo_signature(before, ('PlayerTriggerRadius',)) == proximity.cdo_signature(after, ('PlayerTriggerRadius',))
            mission = create_mission()
        elif mode == 'inspect':
            mission = create_mission()  # In-memory dry run only; the new package is never saved.
        elif mode == 'verify':
            assert cdo(SPAWNER).get_editor_property('PlayerTriggerRadius') == 7500
            assert load(MISSION).get_editor_property('HUDStyle').export_text() == load(MISSION_SOURCE).get_editor_property('HUDStyle').export_text()
        for index, package in enumerate(MAPS):
            if resume and index == 0:
                if report['external_combat_changes_preserved']:
                    world = ue.EditorLoadingAndSavingUtils.load_map(package)
                    configure_map(world, 0, mission)
                    assert map_facts(world, 0) == reference['preflight_checks'][package]
                    report['level1_latest_dll_layout_rechecked'] = True
                continue
            world = ue.EditorLoadingAndSavingUtils.load_map(package); assert world
            if mode == 'inspect':
                table = spawner_table(world, index, before=True)
                report.setdefault('placements', {})[package] = {name: record(a, directory, 'inspect_' + str(index) + '_' + a.get_name()) for name, a in table.items()}
                if index == 1:
                    starts = list(ue.GameplayStatics.get_all_actors_of_class(world, ue.PlayerStart))
                    assert len(starts) == 1 and starts[0].get_name() == 'PlayerStart_1'
                env = environment(world, index, directory, 'environment_' + str(index))
                path = directory / ('environment_' + str(index) + '.json.gz'); save_environment(path, env)
                report['environments'][package] = str(path)
                report.setdefault('memory_preflight', {})[package] = configure_map(world, index, mission)
                report.setdefault('preflight_checks', {})[package] = map_facts(world, index)
                after = environment(world, index, directory, 'preflight_after_' + str(index), set(env['actors']))
                report.setdefault('environment_checks', {})[package] = compare_environment(env, after, allow_factory_child_labels=index == 1)
            else:
                baseline = read_environment(Path(reference['environments'][package]))
                if mode == 'verify':
                    if recovery:
                        report.setdefault('environment_checks', {})[package] = recovery['environment_checks'][package]
                    else:
                        before = environment(world, index, directory, 'environment_reload_' + str(index), set(baseline['actors']))
                        report.setdefault('environment_checks', {})[package] = compare_environment(baseline, before, cross_process=True)
                if mode == 'apply':
                    table = spawner_table(world, index, before=True)
                    originals = {name: record(actor, directory, 'before_' + str(index) + '_' + actor.get_name()) for name, actor in table.items() if name.startswith('BP_DC_LyraFiniteSpawner')}
                    control_actors = of_type(world, 'DCEncounterController') + of_type(world, 'DCStageDirector')
                    control_text = {a.get_name(): record(a, directory, 'control_before_' + a.get_name())['text'] for a in control_actors}
                    report.setdefault('configuration', {})[package] = configure_map(world, index, load(MISSION))
                    after_table = spawner_table(world, index)
                    for name, actor in after_table.items():
                        after_actor = record(actor, directory, 'after_' + str(index) + '_' + actor.get_name())
                        assert originals[name]['transform'] == after_actor['transform'] and originals[name]['label'] == after_actor['label']
                        assert proximity.cdo_signature(originals[name]['text'], ('PlayerTriggerRadius',)) == proximity.cdo_signature(after_actor['text'], ('PlayerTriggerRadius',))
                    if index == 0:
                        for actor in of_type(world, 'DCEncounterController') + of_type(world, 'DCStageDirector'):
                            before_text = control_text[actor.get_name()]
                            after_text = record(actor, directory, 'control_after_' + actor.get_name())['text']
                            fields = ('Phases',) if actor in of_type(world, 'DCStageDirector') else ('Spawners',)
                            assert strip_properties(before_text, fields) == strip_properties(after_text, fields), actor.get_name()
                    report['map_checks'][package] = map_facts(world, index)
                    if reused and index == 0:
                        assert report['map_checks'][package] == reused['map_checks'][package]
                        after = read_environment(ROOT / 'Apply_E/comparison_0.json.gz')
                    else:
                        after = environment(world, index, directory, 'environment_after_' + str(index), set(baseline['actors']))
                        save_environment(directory / ('comparison_' + str(index) + '.json.gz'), after)
                    report.setdefault('environment_checks', {})[package] = compare_environment(baseline, after, cross_process=True, allow_factory_child_labels=index == 1)
                    if index == 0:
                        # Complete the Level_1 loaded-data preservation check
                        # before the first persistent write of this run.
                        save(bp, SPAWNER)
                        save(mission, MISSION)
                    save(world, package)
                    report['environments'][package] = reference['environments'][package]
                else:
                    report['map_checks'][package] = map_facts(world, index)
                    expected_map = reference['map_checks'][package]
                    compare_facts(expected_map, report['map_checks'][package], index)
                    report['environments'][package] = reference['environments'][package]
        if mode == 'apply':
            assert set(report['saved']) == set(WRITES)
        if mode == 'inspect':
            assert inputs() == initial_inputs, 'Dry run must leave every input and new destination unchanged on disk'
        report['complete'] = True
    except Exception:
        report['failure'] = traceback.format_exc()
        raise
    finally:
        after_inventory = inventory()
        report['protected_inventory_changes'] = sorted(k for k, v in before_inventory.items() if after_inventory.get(k) != v)
        report['new_protected_files'] = sorted(set(after_inventory) - set(before_inventory))
        report['protected_hashes_unchanged'] = protected_hashes() == hashes
        report['complete'] &= not report['protected_inventory_changes'] and not report['new_protected_files'] and report['protected_hashes_unchanged']
        persist()
        print('SPAWNER_REGISTRATION_RESULT ' + json.dumps({k:v for k,v in report.items() if k in ('mode','run','complete','saved','failure','protected_inventory_changes','new_protected_files','protected_hashes_unchanged')}, ensure_ascii=False))
    assert report['complete'], 'Registration setup/verification incomplete'


if __name__ == '__main__':
    main()
