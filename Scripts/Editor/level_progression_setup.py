"""Approved Level_2/3 sample encounters and three-map mission-clear setup.

Separate unattended Unreal Editor; no C++ build or PIE. Modes:
inspect (read-only placement/protection baseline), apply (after user build),
verify (fresh-process editor reload, no saves). Each run is new; partial writes are
journaled and never blindly retried. --commands RUN [REFERENCE] prints commands.
Only the explicit packages, DefaultGame.ini and the raw MP4 may be written.
UE packages are created/edited/saved exclusively through Unreal APIs.
"""
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
ROOT = PROJECT / 'Saved/Diagnostics/LevelProgression20261023'
MAP_ROOT = '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/'
MAPS = [MAP_ROOT + 'Level_' + str(i) for i in (1, 2, 3)]
FRONTEND = '/Game/DreamCatcher/Maps/L_DC_FrontEnd'
MODE_ROOT = '/Game/DreamCatcher/Blueprints/Core/'
MODES = [MODE_ROOT + 'BP_DC_Level' + str(i) + 'GameMode' for i in (1, 2, 3)]
VIDEO = '/Game/DreamCatcher/Screens/Dreamcatcher_Mission_Clear'
MISSION = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_LevelExample'
MISSION_SOURCE = '/Game/DreamCatcher/Data/Stage/DA_DC_MissionList_Example'
EXPERIENCE = '/Game/DreamCatcher/Experiences/B_DC_StageUIExperience'
SPAWNER = '/Game/DreamCatcher/Blueprints/AI/BP_DC_LyraFiniteSpawner'
MOVIE = PROJECT / 'Content/Movies/Dreamcatcher_Mission_Clear.mp4'
CONFIG = PROJECT / 'Config/DefaultGame.ini'
ASSET_WRITES = MODES + [VIDEO, MISSION] + MAPS[1:]
NEW_ASSETS = MODES[1:] + [MISSION]
FIELDS = ('bPlayMissionClearOnSuccess', 'MissionClearSource', 'NextLevel', 'MissionClearFadeDuration')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def disk(package):
    return PROJECT / ('Content/' + package[6:] + ('.umap' if package in MAPS or package == FRONTEND else '.uasset'))


def helpers():
    # Existing strict actor/component/brush exports, not LevelExporterT3D.
    path = PROJECT / 'Scripts/Editor/mission_hud_setup.py'
    spec = importlib.util.spec_from_file_location('dc_progression_map_evidence', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def inventory():
    allowed = {disk(p).resolve() for p in ASSET_WRITES} | {CONFIG.resolve(), MOVIE.resolve()}
    result = {}
    for top in ('Content', 'Config', 'Source', 'Scripts', 'Plugins', 'docs', 'Saved/SaveGames'):
        for path in (PROJECT / top).rglob('*'):
            if path.is_file() and path.resolve() not in allowed:
                stat = path.stat()
                result[str(path.relative_to(PROJECT))] = [stat.st_size, stat.st_mtime_ns]
    return result


def protected_hashes():
    names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'Config/DefaultEngine.ini',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    return {name: sha(PROJECT / name) for name in names}


def inputs():
    packages = MAPS + [MODES[0], VIDEO, MISSION_SOURCE, EXPERIENCE, SPAWNER, FRONTEND]
    return {str(disk(p).relative_to(PROJECT)): sha(disk(p)) for p in packages} | {
        'Config/DefaultGame.ini': sha(CONFIG)}


def load(package):
    value = ue.load_asset(package)
    assert value, package
    return value


def native(name):
    value = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert value, 'User Development Editor build required: ' + name
    return value


def cdo(bp):
    return ue.get_default_object(bp.generated_class())


def unchanged_mode_defaults(before, after):
    # Four simple top-level fields are the entire approved existing-mode edit.
    def strip(text):
        return '\n'.join(line for line in text.splitlines()
                         if not any(line.lstrip().startswith(field + '=') for field in FIELDS))
    assert strip(before) == strip(after), 'Unexpected GameMode default change'


def vec(values):
    return ue.Vector(*values)


def coords(value):
    return [value.x, value.y, value.z]


def actors(world):
    return list(ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors())


def by_type(world, cls):
    return list(ue.GameplayStatics.get_all_actors_of_class(world, cls))


def hit_parts(hit):
    # Python omits the out-HitResult when the native trace returns false.
    if hit is None:
        return (False,)
    assert isinstance(hit, ue.HitResult), type(hit)
    # NativeBreakFunc is intentionally omitted from Python's generated class
    # methods. Invoke the existing reflected engine UFunction, as other probes do.
    library = ue.get_default_object(ue.load_class(None, '/Script/Engine.GameplayStatics'))
    return library.call_method('BreakHitResult', args=(hit,))


def trace(world, start, end):
    return hit_parts(ue.SystemLibrary.line_trace_single(world, start, end,
        ue.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], ue.DrawDebugTrace.NONE, True))


def capsule_clear(world, point, radius, height):
    hit = ue.SystemLibrary.capsule_trace_single(world, point, point + ue.Vector(0, 0, .1),
        radius, height, ue.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], ue.DrawDebugTrace.NONE, True)
    return not hit_parts(hit)[0]


def capsule(data):
    pawn = ue.get_default_object(data.get_editor_property('PawnClass'))
    cap = pawn.get_component_by_class(ue.CapsuleComponent)
    assert cap
    return cap.get_scaled_capsule_radius(), cap.get_scaled_capsule_half_height()


def placement(world):
    starts = by_type(world, ue.PlayerStart)
    assert len(starts) <= 1, 'Review multiple existing starts before changing this map'
    assert not by_type(world, native('DCStageDirector'))
    assert not by_type(world, native('DCEnemySpawner'))
    assert not by_type(world, native('DCEncounterController'))
    assert not by_type(world, ue.NavMeshBoundsVolume) and not by_type(world, ue.RecastNavMesh)
    assert not by_type(world, ue.Character), 'Review existing characters before adding a sample enemy'
    boxes = []
    for actor in actors(world):
        center, extent = actor.get_actor_bounds(True)
        if max(extent.x, extent.y, extent.z) > 1:
            boxes.append((center, extent))
    assert boxes, 'No collision geometry found'
    minimum = [min(coords(c)[i] - coords(e)[i] for c, e in boxes) for i in range(3)]
    maximum = [max(coords(c)[i] + coords(e)[i] for c, e in boxes) for i in range(3)]
    experience = cdo(load(EXPERIENCE))
    player_data = experience.get_editor_property('DefaultPawnData')
    bot_data = experience.get_editor_property('BotPawnData') or player_data
    pr, ph = capsule(player_data)
    br, bh = capsule(bot_data)
    seeds = [starts[0].get_actor_location()] if starts else []
    flat = [(c, e) for c, e in boxes if max(e.x, e.y) > 150 and e.z < max(e.x, e.y) / 3]
    if starts:
        origin = starts[0].get_actor_location()
        flat.sort(key=lambda pair: (pair[0].x - origin.x) ** 2 + (pair[0].y - origin.y) ** 2)
    else:
        flat.sort(key=lambda pair: pair[1].x * pair[1].y, reverse=True)
    for center, extent in flat[:80]:
        for dx, dy in ((0, 0), (.5, .5), (.5, -.5), (-.5, .5), (-.5, -.5)):
            seeds.append(ue.Vector(center.x + extent.x * dx, center.y + extent.y * dy, center.z + extent.z + 50))
    for seed in seeds:
        ground = trace(world, seed + ue.Vector(0, 0, 10), seed - ue.Vector(0, 0, 3000))
        if not ground[0] or ground[7].z < .7:
            continue
        player = ground[5] + ue.Vector(0, 0, ph + 4)
        if not capsule_clear(world, player, pr, ph):
            continue
        for distance in (700, 1000, 500):
            for angle in (0, 90, 180, 270, 45, 135, 225, 315):
                radians = math.radians(angle)
                xy = ground[5] + ue.Vector(math.cos(radians) * distance, math.sin(radians) * distance, 0)
                target = trace(world, xy + ue.Vector(0, 0, 400), xy - ue.Vector(0, 0, 500))
                if not target[0] or target[7].z < .7 or abs(target[5].z - ground[5].z) > 150:
                    continue
                enemy = target[5] + ue.Vector(0, 0, bh + 4)
                if not capsule_clear(world, enemy, br, bh):
                    continue
                # A short, visible example encounter; no final combat/level design claim.
                if trace(world, player, enemy)[0]:
                    continue
                rotation = ue.MathLibrary.find_look_at_rotation(player, enemy)
                return dict(player=coords(player), player_rotation=[0, rotation.yaw, 0],
                            enemy=coords(enemy), old_start=starts[0].get_name() if starts else None,
                            player_capsule=[pr, ph], enemy_capsule=[br, bh],
                            nav_min=[v - 500 for v in minimum], nav_max=[v + 500 for v in maximum])
    raise AssertionError('No verified clear start/enemy pair; review placement rather than force-spawn inside geometry')


def snapshot(world, directory, label, old_start, expected_paths=None):
    # Reuse the prior complete native-property, component and brush evidence.
    selected = actors(world)
    if expected_paths is not None:
        selected = [actor for actor in selected if actor.get_path_name() in expected_paths]
    result = evidence.map_snapshot(world, selected, world.get_world_settings(), directory, label, {})
    settings = result['actors'].pop(world.get_world_settings().get_path_name())
    # WorldSettings is also preserved in full except the one approved mode link.
    settings['text'] = re.sub(r'^   DefaultGameMode=.*\n?', '', settings['text'], flags=re.M)
    settings['components'] = sorted(settings['components'], key=lambda row: row['path'])
    settings['components_not_inline'] = sorted(settings['components_not_inline'])
    result['world_settings'] = settings
    if old_start:
        path = world.get_path_name() + ':PersistentLevel.' + old_start
        result['actors'].pop(path, None)
    return result


def existing_environment(before, world, directory, label, old_start, cross_process=False):
    # Snapshot only the original environment: the existing strict brush exporter
    # has no reason to normalize the newly generated volume's transient model.
    after = snapshot(world, directory, label, old_start, expected_paths=set(before['actors']))
    return evidence.compare_maps(before, after, cross_process=cross_process)


def spawn(world, cls, name, position, rotation=None):
    pitch, yaw, roll = rotation or [0, 0, 0]
    assert pitch == roll == 0, 'This bounded placement only authors horizontal facing'
    half_yaw = math.radians(yaw) / 2
    transform = ue.Transform()
    assert transform.import_text('(Rotation=(X=0,Y=0,Z=' + str(math.sin(half_yaw)) + ',W=' + str(math.cos(half_yaw))
                                 + '),Translation=(X=' + str(position[0]) + ',Y=' + str(position[1]) + ',Z=' + str(position[2])
                                 + '),Scale3D=(X=1,Y=1,Z=1))')
    game = ue.get_default_object(ue.load_class(None, '/Script/Engine.GameplayStatics'))
    actor = game.call_method('BeginDeferredActorSpawnFromClass', args=(world, cls, transform,
        ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN, None, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor
    actor = game.call_method('FinishSpawningActor', args=(actor, transform, ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
    assert actor and actor.rename(name)
    actor.set_actor_label(name)
    assert actor.get_world() == world and actor.get_package() == world.get_package()
    return actor


def add_navigation(world, plan):
    lo, hi = plan['nav_min'], plan['nav_max']
    center = [(a + b) / 2 for a, b in zip(lo, hi)]
    half = [(b - a) / 2 for a, b in zip(lo, hi)]
    subsystem = ue.get_editor_subsystem(ue.EditorActorSubsystem)
    # The editor's volume factory supplies the brush model. No external package IO.
    labels = {a.get_name(): a.get_actor_label(False) for a in actors(world)}
    volume = subsystem.spawn_actor_from_class(ue.NavMeshBoundsVolume, vec(center))
    assert volume
    volume.rename('DC_ExampleNavBounds'); volume.set_actor_label('DC_ExampleNavBounds')
    _, original = volume.get_actor_bounds(False)
    assert min(coords(original)) > 0, 'Editor volume factory did not build a brush'
    volume.set_actor_scale3d(vec([a / b for a, b in zip(half, coords(original))]))
    # Factory name caching may materialize default labels on unrelated actors.
    # Restore those exact original labels, then require full environment equality.
    for actor in actors(world):
        if actor.get_name() in labels and actor.get_actor_label(False) != labels[actor.get_name()]:
            actor.set_actor_label(labels[actor.get_name()], False)
            assert actor.get_actor_label(False) == labels[actor.get_name()], 'Editor factory changed an unrestorable label'
    navs = by_type(world, ue.RecastNavMesh)
    assert len(navs) <= 1
    nav = navs[0] if navs else spawn(world, ue.RecastNavMesh, 'DC_ExampleRecast', center)
    nav.set_editor_property('RuntimeGeneration', ue.RuntimeGenerationType.DYNAMIC)
    nav.set_editor_property('bCanBeMainNavData', True)
    system = ue.NavigationSystemV1.get_navigation_system(world)
    assert system
    system.on_navigation_bounds_updated(volume)
    return [volume.get_name(), nav.get_name()]


def configure_map(world, plan, mode, mission):
    old = next((a for a in actors(world) if a.get_name() == plan['old_start']), None)
    if old:
        assert old.get_class() == ue.PlayerStart.static_class(), 'Only the inspected plain engine start may be replaced'
        assert ue.get_editor_subsystem(ue.EditorActorSubsystem).destroy_actor(old)
    start = spawn(world, native('DCLyraPlayerStart'), 'DC_ExamplePlayerStart', plan['player'], plan['player_rotation'])
    spawner = spawn(world, load(SPAWNER).generated_class(), 'DC_ExampleSpawner', plan['enemy'])
    spawn_point = spawner.get_editor_property('SpawnPoint')
    offset = vec(plan['enemy']) - spawn_point.get_world_location()
    if offset.length() > .01:
        spawner.set_actor_location(spawner.get_actor_location() + offset, False, True)
    for name, value in dict(SpawnCount=1, MaxAliveAtOnce=1, bActivateOnBeginPlay=False).items():
        spawner.set_editor_property(name, value, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    encounter = spawn(world, ue.load_class(None, '/Game/Blueprint/Stage/BP_EncounterController.BP_EncounterController_C'),
                      'DC_ExampleEncounter', plan['enemy'])
    encounter.set_editor_property('Spawners', [spawner], notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    encounter.set_editor_property('bAutoStartOnBeginPlay', False, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    encounter.set_editor_property('StartDelay', 0.0, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    director = spawn(world, ue.load_class(None, '/Game/Blueprint/Stage/BP_StageDirector.BP_StageDirector_C'),
                     'DC_ExampleStageDirector', plan['player'])
    director.set_editor_property('bAutoStartStage', False, notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    phase = ue.DCStagePhaseDefinition()
    assert phase.import_text('(PhaseType=Encounter,PhaseTitle="예시 적 처치")')
    phase.set_editor_property('Encounter', encounter); phase.set_editor_property('MissionList', mission)
    complete = ue.DCStagePhaseDefinition(); assert complete.import_text('(PhaseType=Complete,PhaseTitle="완료")')
    director.set_editor_property('Phases', [phase, complete], notify_mode=ue.PropertyAccessChangeNotifyMode.NEVER)
    world.get_world_settings().set_editor_property('DefaultGameMode', mode.generated_class())
    return [start.get_name(), spawner.get_name(), encounter.get_name(), director.get_name()] + add_navigation(world, plan)


def check_map(world, index, expected_names, plan):
    assert world.get_world_settings().get_editor_property('DefaultGameMode') == load(MODES[index]).generated_class()
    directors = by_type(world, native('DCStageDirector'))
    spawners = by_type(world, native('DCEnemySpawner'))
    encounters = by_type(world, native('DCEncounterController'))
    assert len(directors) == len(spawners) == len(encounters) == 1
    phases = list(directors[0].get_editor_property('Phases'))
    assert len(phases) == 2 and phases[0].get_editor_property('Encounter') == encounters[0]
    assert phases[0].get_editor_property('PhaseType') == ue.DCStagePhaseType.ENCOUNTER
    assert phases[0].get_editor_property('MissionList') == load(MISSION)
    assert phases[1].get_editor_property('PhaseType') == ue.DCStagePhaseType.COMPLETE
    assert not directors[0].get_editor_property('bAutoStartStage')
    assert list(encounters[0].get_editor_property('Spawners')) == spawners
    assert not encounters[0].get_editor_property('bAutoStartOnBeginPlay') and encounters[0].get_editor_property('StartDelay') == 0
    assert spawners[0].get_editor_property('SpawnCount') == spawners[0].get_editor_property('MaxAliveAtOnce') == 1
    assert not spawners[0].get_editor_property('GASEnemyClass') and spawners[0].get_editor_property('LyraBotControllerClass')
    assert len(by_type(world, ue.PlayerStart)) == len(by_type(world, ue.NavMeshBoundsVolume)) == len(by_type(world, ue.RecastNavMesh)) == 1
    assert by_type(world, ue.RecastNavMesh)[0].get_editor_property('RuntimeGeneration') == ue.RuntimeGenerationType.DYNAMIC
    assert all(name in {a.get_name() for a in actors(world)} for name in expected_names)
    assert by_type(world, ue.PlayerStart)[0].get_actor_location().distance(vec(plan['player'])) < .1
    assert spawners[0].get_editor_property('SpawnPoint').get_world_location().distance(vec(plan['enemy'])) < .1
    center, extent = by_type(world, ue.NavMeshBoundsVolume)[0].get_actor_bounds(False)
    assert all(abs(value - (lo + hi) / 2) < 1 for value, lo, hi in zip(coords(center), plan['nav_min'], plan['nav_max']))
    assert all(abs(value - (hi - lo) / 2) < 1 for value, lo, hi in zip(coords(extent), plan['nav_min'], plan['nav_max']))
    return dict(actor_count=len(actors(world)), gameplay_actors=expected_names)


def check_modes():
    for index, package in enumerate(MODES):
        value = cdo(load(package))
        assert value.get_editor_property('bUseExperienceUI')
        assert value.get_editor_property('bPlayMissionClearOnSuccess')
        assert value.get_editor_property('MissionClearSource') == load(VIDEO)
        assert value.get_editor_property('NextLevel').get_path_name() == (MAPS[index + 1] if index < 2 else FRONTEND) + '.' + (MAPS[index + 1] if index < 2 else FRONTEND).rsplit('/', 1)[-1]
        assert value.get_editor_property('ReturnMap').get_path_name() == FRONTEND + '.L_DC_FrontEnd'
        assert abs(value.get_editor_property('MissionClearFadeDuration') - .5) < .0001
    assert load(VIDEO).validate() and Path(load(VIDEO).get_url().removeprefix('file://')).resolve() == MOVIE.resolve()
    config = CONFIG.read_text(encoding='utf-8-sig')
    assert all(config.count('+MapsToCook=(FilePath="' + package + '")') == 1 for package in MAPS + [FRONTEND])
    assert config.count('+NonUFSMovies=Dreamcatcher_Mission_Clear') == 1
    definitions = list(load(MISSION).get_editor_property('Missions'))
    assert len(definitions) == 1 and str(definitions[0].get_editor_property('MissionId')) == 'DefeatExample'
    assert definitions[0].get_editor_property('ProgressSource') == ue.DCMissionProgressSource.CURRENT_ENCOUNTER_DEFEATS
    assert load(MISSION).get_editor_property('HUDStyle').export_text() == load(MISSION_SOURCE).get_editor_property('HUDStyle').export_text()


def inspect_ticking(report, directory, original_inventory, original_hashes, all_input_hashes):
    """Collision queries wait for editor physics registration after each map load."""
    assert '-run=pythonscript' not in ue.SystemLibrary.get_command_line().lower(), 'Use --commands: inspect needs editor ticks'
    state = dict(index=0, loaded=False, at=0.0, busy=False, done=False, wall=time.monotonic())
    source = load(VIDEO)
    assert isinstance(source, ue.FileMediaSource)
    source_path = Path(source.get_url().removeprefix('file://'))
    assert source_path.is_file() and source_path.suffix.lower() == '.mp4'
    report['movie'] = dict(source=str(source_path), size=source_path.stat().st_size, sha256=sha(source_path))

    def finish():
        state['done'] = True
        final_inventory = inventory()
        report['inventory_changes'] = [name for name in sorted(set(original_inventory) | set(final_inventory))
                                       if original_inventory.get(name) != final_inventory.get(name)]
        report['protected_inventory_unchanged'] = not report['inventory_changes']
        report['protected_hashes_unchanged'] = protected_hashes() == original_hashes
        report['read_only_inputs_unchanged'] = inputs() == all_input_hashes
        report['complete'] = bool(not report.get('failure') and state['index'] == 2 and report['protected_inventory_unchanged']
                                  and report['protected_hashes_unchanged'] and report['read_only_inputs_unchanged'])
        (directory / 'report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        ue.unregister_slate_post_tick_callback(state['callback'])
        if not report['complete']:
            ue.log_error('Read-only progression inspection failed: ' + str(report.get('failure', report['inventory_changes'])))
        print('DC_PROGRESSION_INSPECT ' + json.dumps(dict(complete=report['complete'], map_plans=report['map_plans'])))
        ue.SystemLibrary.quit_editor()

    def tick(delta):
        if state['done'] or state['busy']:
            return
        state['busy'] = True
        try:
            assert time.monotonic() - state['wall'] < 180, 'Inspection timeout'
            package = MAPS[state['index'] + 1]
            if not state['loaded']:
                assert ue.EditorLoadingAndSavingUtils.load_map(package)
                state.update(loaded=True, at=time.monotonic())
                return
            if time.monotonic() - state['at'] < 2:
                return
            world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
            assert world.get_path_name() == package + '.' + package.rsplit('/', 1)[-1]
            plan = placement(world)
            report['map_plans'][package] = plan
            report['environments'][package] = snapshot(world, directory, package.rsplit('/', 1)[-1] + '_before', plan['old_start'])
            report.setdefault('navigation_preflight', {})[package] = add_navigation(world, plan)
            existing_environment(report['environments'][package], world, directory,
                                 package.rsplit('/', 1)[-1] + '_nav_trial', plan['old_start'])
            state.update(index=state['index'] + 1, loaded=False)
            if state['index'] == 2:
                finish()
        except Exception:
            report['failure'] = traceback.format_exc(); finish()
        finally:
            state['busy'] = False

    state['callback'] = ue.register_slate_post_tick_callback(tick)
    ue.EditorPythonScripting.set_keep_python_script_alive(True)


def main():
    global ue, evidence
    import unreal as ue
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCProgressionMode=(inspect|apply|verify)\b', command).group(1)
    run = re.search(r'-DCProgressionRun=([A-Za-z0-9_]+)\b', command).group(1)
    reference_match = re.search(r'-DCProgressionReference=([A-Za-z0-9_]+)\b', command)
    compatibility_match = re.search(r'-DCProgressionMovieCompatibility=([A-Za-z0-9_]+)\b', command)
    assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
    assert Path(ue.Paths.project_dir()).resolve() == PROJECT.resolve() and not ue.SourceControl.is_enabled()
    evidence = helpers(); evidence.hud.assert_no_other_editor()
    directory = ROOT / run; directory.mkdir(parents=True, exist_ok=True)
    report_file = directory / 'report.json'; assert not report_file.exists(), 'Preserve existing evidence'
    report = dict(mode=mode, complete=False, saved=[], map_plans={}, environments={}, map_checks={}, PIE=False, cpp_build=False)
    original_inventory, original_hashes = inventory(), protected_hashes()
    all_input_hashes = inputs()
    report['input_hashes'] = all_input_hashes
    reference = json.loads((ROOT / reference_match.group(1) / 'report.json').read_text(encoding='utf-8')) if reference_match else None
    if mode == 'inspect':
        assert all(not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package) for package in NEW_ASSETS)
        return inspect_ticking(report, directory, original_inventory, original_hashes, all_input_hashes)

    def persist():
        report_file.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')

    def save(asset, package):
        assert package in ASSET_WRITES and package not in report['saved']
        if package in MAPS:
            assert ue.EditorLoadingAndSavingUtils.save_map(asset, package)
        else:
            assert ue.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
        report['saved'].append(package); persist()

    try:
        assert not ue.EditorLoadingAndSavingUtils.get_dirty_map_packages()
        assert not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages()
        settings = ue.get_default_object(ue.load_class(None, '/Script/BlueprintGraph.BlueprintEditorSettings'))
        text = evidence.hud.export(settings, directory / 'compile_settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$', text, re.M) == ['SoC_Never']
        if mode != 'verify':
            assert all(not disk(package).exists() and not ue.EditorAssetLibrary.does_asset_exist(package) for package in NEW_ASSETS)
        if mode == 'apply':
            assert reference and reference['mode'] == 'inspect' and reference['complete']
            assert reference['input_hashes'] == all_input_hashes, 'Inputs changed since inspection; inspect again'
            native('DCMissionClearWidget')
            assert cdo(load(MODES[0])).get_editor_property('bPlayMissionClearOnSuccess') is False
            source_path = Path(reference['movie']['source'])
            assert sha(source_path) == reference['movie']['sha256']
            assert not MOVIE.exists(), 'Preserve an existing movie destination'
            assert not (PROJECT / 'Content/Movies').exists() or not list((PROJECT / 'Content/Movies').iterdir()), 'Review other movie staging before setting the one-movie packaging policy'
            MOVIE.parent.mkdir(exist_ok=True); shutil.copyfile(source_path, MOVIE)
            assert sha(MOVIE) == sha(source_path)
            report['movie'] = reference['movie']; report['movie']['destination_sha256'] = sha(MOVIE); persist()
            movie = load(VIDEO); movie.set_file_path(str(MOVIE)); save(movie, VIDEO)
            tools = ue.AssetToolsHelpers.get_asset_tools()
            modes = [load(MODES[0])]
            source_defaults = evidence.hud.export(cdo(modes[0]), directory / 'mode_1_before.t3d')
            original_graphs = {graph.get_name(): evidence.hud.export(graph, directory / ('mode_1_graph_before_' + str(i) + '.t3d'))
                               for i, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(modes[0]))}
            for package in MODES[1:]:
                duplicate = tools.duplicate_asset(package.rsplit('/', 1)[-1], package.rsplit('/', 1)[0], modes[0])
                assert duplicate; modes.append(duplicate)
                clone_defaults = evidence.hud.export(cdo(duplicate), directory / (package.rsplit('/', 1)[-1] + '_clone.t3d'))
                clone_defaults = clone_defaults.replace(package, MODES[0]).replace(
                    'Default__' + package.rsplit('/', 1)[-1] + '_C', 'Default__BP_DC_Level1GameMode_C')
                # Generated class/object short names accompany the qualified paths.
                clone_defaults = clone_defaults.replace(package.rsplit('/', 1)[-1] + '_C', 'BP_DC_Level1GameMode_C')
                unchanged_mode_defaults(source_defaults, clone_defaults)
            for index, value in enumerate(modes):
                defaults = cdo(value)
                defaults.set_editor_property('bPlayMissionClearOnSuccess', True)
                defaults.set_editor_property('MissionClearSource', movie)
                defaults.set_editor_property('MissionClearFadeDuration', .5)
                defaults.set_editor_property('NextLevel', load(MAPS[index + 1] if index < 2 else FRONTEND))
                assert ue.BlueprintEditorLibrary.compile_blueprint(value)
                after_defaults = evidence.hud.export(cdo(value), directory / ('mode_' + str(index + 1) + '_after.t3d'))
                if index == 0:
                    unchanged_mode_defaults(source_defaults, after_defaults)
                    after_graphs = {graph.get_name(): evidence.hud.export(graph, directory / ('mode_1_graph_after_' + str(i) + '.t3d'))
                                    for i, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(value))}
                    assert original_graphs == after_graphs, 'Existing GameMode authored graphs changed'
                save(value, MODES[index])
            mission = tools.duplicate_asset(MISSION.rsplit('/', 1)[-1], MISSION.rsplit('/', 1)[0], load(MISSION_SOURCE))
            assert mission
            definition = ue.DCMissionDefinition()
            assert definition.import_text('(MissionId=DefeatExample,DisplayText="예시 적 처치하기",ProgressSource=CurrentEncounterDefeats,TargetCount=1,bEnabled=True)')
            mission.set_editor_property('Missions', [definition]); mission.set_editor_property('DefaultTrackedMissionId', ue.Name('DefeatExample'))
            save(mission, MISSION)
            for index, package in enumerate(MAPS[1:], 1):
                world = ue.EditorLoadingAndSavingUtils.load_map(package)
                # The input file hashes prove this is the inspected map. Physics
                # registration/clearance was checked in the ticking editor.
                plan = reference['map_plans'][package]
                original_actor_names = {a.get_name() for a in actors(world)}
                before = snapshot(world, directory, 'Level_' + str(index + 1) + '_before', plan['old_start'])
                report['environments'][package] = before; report['map_plans'][package] = plan
                expected = configure_map(world, plan, modes[index], mission)
                existing_environment(before, world, directory, 'Level_' + str(index + 1) + '_after', plan['old_start'])
                assert {a.get_name() for a in actors(world)} == (original_actor_names - {plan['old_start']}) | set(expected)
                report['map_checks'][package] = check_map(world, index, expected, plan)
                save(world, package)
            config = CONFIG.read_text(encoding='utf-8-sig')
            marker = '+MapsToCook=(FilePath="' + MAPS[0] + '")'
            assert config.count(marker) == 1 and all(package not in config for package in MAPS[1:])
            assert not re.search(r'^\s*(?:bSkipMovies|[+!\-]?NonUFSMovies|[+!\-]?UFSMovies)\s*=', config, re.M)
            movie_lines = '\nbSkipMovies=True\n+NonUFSMovies=Dreamcatcher_Mission_Clear'
            config = config.replace(marker, marker + ''.join('\n+MapsToCook=(FilePath="' + package + '")' for package in MAPS[1:]) + movie_lines)
            CONFIG.write_text(config, encoding='utf-8', newline='\r\n')
            report['config_sha256'] = sha(CONFIG)
            check_modes()
            report['saved_hashes'] = {package: sha(disk(package)) for package in ASSET_WRITES}
        else:
            assert reference and reference['mode'] == 'apply' and reference['complete']
            assert {package: sha(disk(package)) for package in ASSET_WRITES} == reference['saved_hashes']
            expected_movie = reference['movie']['destination_sha256']
            if compatibility_match:
                compatibility_path = ROOT / compatibility_match.group(1) / 'report.json'
                compatibility = json.loads(compatibility_path.read_text(encoding='utf-8'))
                assert compatibility['complete'] and compatibility['operation'] == 'mission_clear_movie_compatibility'
                assert Path(compatibility['target']).resolve() == MOVIE.resolve()
                assert compatibility['before_sha256'] == expected_movie
                assert compatibility['source_sha256'] == reference['movie']['sha256']
                assert sha(Path(reference['movie']['source'])) == compatibility['source_sha256']
                expected_movie = compatibility['after_sha256']
                report['movie_compatibility_journal'] = str(compatibility_path)
            assert sha(CONFIG) == reference['config_sha256'] and sha(MOVIE) == expected_movie
            check_modes()
            for index, package in enumerate(MAPS[1:], 1):
                world = ue.EditorLoadingAndSavingUtils.load_map(package)
                report['map_checks'][package] = check_map(world, index, reference['map_checks'][package]['gameplay_actors'], reference['map_plans'][package])
                report.setdefault('environment_comparisons', {})[package] = existing_environment(
                    reference['environments'][package], world, directory, 'Level_' + str(index + 1) + '_reload',
                    reference['map_plans'][package]['old_start'], cross_process=True)
            assert {package: sha(disk(package)) for package in ASSET_WRITES} == reference['saved_hashes']
        report['complete'] = True
    except Exception:
        report['failure'] = traceback.format_exc(); ue.log_error(report['failure'])
    finally:
        final_inventory = inventory()
        report['inventory_changes'] = [name for name in sorted(set(original_inventory) | set(final_inventory))
                                       if original_inventory.get(name) != final_inventory.get(name)]
        report['protected_inventory_unchanged'] = not report['inventory_changes']
        report['protected_hashes_unchanged'] = protected_hashes() == original_hashes
        if mode != 'apply':
            report['read_only_inputs_unchanged'] = inputs() == all_input_hashes
        report['complete'] &= report['protected_inventory_unchanged'] and report['protected_hashes_unchanged'] and report.get('read_only_inputs_unchanged', True)
        persist()
    assert report['complete'], 'Setup failed; inspect this run journal before any retry'
    print('DC_PROGRESSION_SETUP ' + json.dumps({key: report[key] for key in ('mode', 'complete', 'saved', 'map_plans')}))


if __name__ == '__main__':
    if len(sys.argv) == 3 and sys.argv[1] == '--commands':
        run = sys.argv[2]
        assert re.fullmatch(r'[A-Za-z0-9_]+', run)
        for mode in ('inspect', 'apply', 'verify'):
            args = ['E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe', str(PROJECT / 'DreamCatcher.uproject')]
            # Use the same editor context for all modes: commandlets omit the
            # editor-only light billboard sprite/scale used in the full snapshots.
            args += [MAPS[1], '-ExecutePythonScript=' + str(Path(__file__).resolve())]
            args += ['-unattended', '-NullRHI', '-NoSound', '-NoAutoSave',
                    '-NoSaveConfig', '-SCCProvider=None', '-DCProgressionMode=' + mode, '-DCProgressionRun=' + run + '_' + mode,
                    '-abslog=' + str(ROOT / (run + '_' + mode) / 'Editor.log')]
            if mode != 'inspect':
                args.append('-DCProgressionReference=' + run + ('_inspect' if mode == 'apply' else '_apply'))
            print(subprocess.list2cmdline(args))
    else:
        main()
