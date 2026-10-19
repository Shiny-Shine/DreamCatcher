"""TODO6 real PIE status/armor/Q probe; arm on Frontend before Project.Maps.PIE.

Use an isolated editor, recommended PIE duration 300 seconds:
-DCPlayerStatusProbe=<fresh> -DCPlayerStatusPaintDiagnostics
-DCBossHUDPaintDiagnostics
-RenderOffscreen -nosound -unattended -SCCProvider=None -forcelogflush
-abslog=<project>/Saved/Diagnostics/PlayerStatus20261020/<fresh>/Editor.log
-GameUserSettingsINI=<same folder>/GameUserSettings.ini
Optional -DCPlayerStatusRequestedResolution=1920x1080 records the request alongside
the actual embedded viewport and native logical paint size. It does not resize it.
The runner must set __file__ to this script's absolute path before exec().

Actual NPC equipped weapons use their unchanged default .8 ratio and original
GA/trace/damage GE. Expected D is
calculated from current NPC BaseDamage and the existing body/no-falloff contract;
it is not described as an observed raw damage-message payload. Only live PIE
transforms/AI/movement/input/immunity are temporarily changed.
No asset/CDO/AttributeData manipulation or replacement gameplay effect is used
for the armor tests. The existing diagnostic Damage GE only clears fights/death.
MaxHP mutation has no suitable reflected setter here and remains unverified.
Non-default .25/.75 weapon-source ratios belong to the separate native Armor
tests; EditDefaultsOnly prevents changing this property on live PIE instances.
Q absence/ambiguity is accepted only after inspecting actual spec input tags and
an existing instanced ability's actual ASC/Avatar ActorInfo. No skill is granted.

After the editor exits:
python player_status_hud_gameplay_probe.py --verify-report REPORT LOG INDEX EXIT_CODE
"""
import hashlib
import json
import math
from pathlib import Path
import re
import sys
import time
import traceback

sys.dont_write_bytecode = True
PROJECT = Path(__file__).resolve().parents[2]
REPORT_ROOT = PROJECT / 'Saved/Diagnostics/PlayerStatus20261020'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    return {str(p.relative_to(PROJECT)): [p.stat().st_size, p.stat().st_mtime_ns]
            for folder in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames')
            for p in (PROJECT / folder).rglob('*') if p.is_file()}


def protected_hashes():
    names = ['.git/HEAD', '.git/index', 'DreamCatcher.uproject', 'AGENTS.md',
             'Binaries/Win64/UnrealEditor-DreamCatcher.dll',
             'Content/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.umap',
             'Content/DreamCatcher/UI/HUD/W_DC_PlayerStatus.uasset',
             'Content/DreamCatcher/Lyra/UI/Stage/Game/UI/Hud/W_Healthbar.uasset',
             'Content/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout.uasset',
             'Content/DreamCatcher/Experiences/B_DC_StageUIExperience.uasset',
             'Saved/Config/WindowsEditor/GameUserSettings.ini', 'Saved/Config/Windows/GameUserSettings.ini']
    return {name: sha(PROJECT / name) if (PROJECT / name).is_file() else None for name in names}


def observations(log):
    errors = [line for line in log.splitlines() if re.search(
        r': Error:|: Fatal:|Assertion failed:|Ensure condition failed:|=== Critical error:', line)]
    warnings = {}
    for line in log.splitlines():
        if ': Warning:' not in line:
            continue
        category = ('audio' if 'TapDelay' in line or 'WeaponAudioFunctions' in line else
                    'pose_asset' if 'PoseAsset' in line else
                    'navigation_crowd' if 'Navigation' in line or 'Crowd' in line else
                    'other_requires_review')
        bucket = warnings.setdefault(category, dict(count=0, lines=[]))
        bucket['count'] += 1
        if line not in bucket['lines']:
            bucket['lines'].append(line)
    return errors, warnings


def native_rows(log, prefix, first_frame, last_frame):
    rows = {}
    for line in log.splitlines():
        if prefix + ' ' not in line:
            continue
        try:
            row = json.loads(line.split(prefix + ' ', 1)[1])
        except json.JSONDecodeError:
            continue  # A bounded tail may start inside a log line.
        frame = row.get('frame')
        if isinstance(frame, int) and first_frame < frame <= last_frame:
            rows[(row.get('widget'), frame)] = row
    return sorted(rows.values(), key=lambda row: row['frame'])


def paint_problems(row, expected, previous_health=None):
    """Check actual NativePaint text/MIDs/bounds, not Python slot geometry."""
    problems = []
    for key in ('health', 'max_health', 'armor', 'max_armor'):
        if not math.isfinite(row.get(key, math.nan)) or abs(row[key] - expected[key]) > .025:
            problems.append(key)
    root = row.get('root_size', [])
    if len(root) != 2 or not all(math.isfinite(v) and v > 0 for v in root):
        return problems + ['root_geometry']
    for key in ('bars', 'armor_bounds', 'health_bounds', 'star_bounds', 'ring_bounds'):
        bounds = row.get(key)
        if not bounds or not all(math.isfinite(v) for v in bounds['position'] + bounds['size']) or min(bounds['size']) <= 0:
            problems.append(key)
    if any(key in problems for key in ('bars', 'health_bounds', 'star_bounds')):
        return problems
    bars, hp, star = row['bars'], row['health_bounds'], row['ring_bounds']
    if max(abs(a - b) for a, b in zip(bars['size'], [440, 64])) > .5:
        problems.append('bar_group_size')
    if abs(bars['position'][0] + 220 - root[0] / 2) > .5 or abs(root[1] - sum([bars['position'][1], bars['size'][1]]) - 72) > .5:
        problems.append('bar_center_bottom')
    if max(abs(a - b) for a, b in zip(hp['size'], [440, 32])) > .5:
        problems.append('health_size')
    if abs(star['position'][0] - (bars['position'][0] + 440) - 40) > .5 or max(abs(v - 80) for v in star['size']) > .5:
        problems.append('signature_gap_size')
    cap = expected['max_armor'] / 3
    fills = [min(1., max(0., (expected['armor'] - cap * i) / cap)) if cap > 0 else 0. for i in range(3)]
    segments = row.get('segments', [])
    if len(segments) != 3:
        problems.append('segment_count')
    else:
        for i, segment in enumerate(segments):
            if segment.get('index') != i or any(abs(segment.get(key, -9) - fills[i]) > .002
                    for key in ('fraction', 'material_current', 'material_updated')):
                problems.append('segment_fill_' + str(i))
            b = segment.get('bounds')
            if not b or abs(b['size'][1] - 24) > .5 or abs(b['size'][0] - (440 - 16) / 3) > .5:
                problems.append('segment_bounds_' + str(i))
    for key, wanted in [('health_current_text', expected['health']), ('armor_text', expected['armor'])]:
        values = re.findall(r'-?\d+(?:\.\d+)?', row.get(key, '').replace(',', ''))
        if len(values) != 1 or abs(float(values[0]) - wanted) > .51:
            problems.append(key + '_not_settled')
    values = re.findall(r'/\s*(\d+(?:\.\d+)?)', row.get('health_max_text', '').replace(',', ''))
    if len(values) != 1 or abs(float(values[0]) - expected['max_health']) > .51:
        problems.append('health_max_text')
    # The original damage animation keeps Old/Max in Health_Current and New/Max
    # in Health_Updated. Do not falsely require both equal to the new HP value.
    materials = row.get('health_materials', [])
    wanted_current = (expected['health'] if previous_health is None else previous_health) / expected['max_health']
    wanted_updated = expected['health'] / expected['max_health']
    if len(materials) != 3 or {m['name'] for m in materials} != {'BarBorder', 'BarFill', 'BarGlow'}:
        problems.append('health_materials')
    elif any(abs(m.get('current', -9) - wanted_current) > .002 or abs(m.get('updated', -9) - wanted_updated) > .002 for m in materials):
        problems.append('health_material_values')
    active = row.get('active_range', [])
    if len(active) != 4 or any(not math.isfinite(v) for v in active) or any(
            abs(a - b) > .002 for a, b in zip(active, [0, row.get('progress', -9), 0, 0])):
        problems.append('signature_actual_active_range')
    return problems


def verify_report(report_path, log_path, index_path, exit_code):
    path = Path(report_path).resolve()
    assert path.name == 'pie.json' and path.is_relative_to(REPORT_ROOT.resolve())
    assert Path(log_path).resolve().is_relative_to(path.parent)
    report = json.loads(path.read_text(encoding='utf-8'))
    index = json.loads(Path(index_path).read_text(encoding='utf-8-sig'))
    log = Path(log_path).read_text(encoding='utf-8-sig', errors='replace')
    errors, warnings = observations(log)
    tests = [row for row in index.get('tests', []) if row.get('fullTestPath') == 'Project.Maps.PIE']
    automation = bool(tests) and all(row.get('state') == 'Success' for row in tests) and all(
        int(index.get(key, -1)) == 0 for key in ('failed', 'notRun', 'inProcess'))
    before = json.loads((path.parent / 'protection_before.json').read_text(encoding='utf-8'))
    protection = inventory() == before['inventory'] and protected_hashes() == before['hashes']
    matching = any('PLAYER_STATUS_PIE ' in line and '"event": "armed"' in line and
                   '"run": "' + report['run'] + '"' in line for line in log.splitlines())
    report['final_verification'] = dict(editor_exit_code=int(exit_code), automation_passed=automation,
        engine_errors=errors, warnings=warnings, matching_run_log=matching, protection_after_editor_exit=protection,
        log=str(Path(log_path).resolve()), automation_index=str(Path(index_path).resolve()),
        warning_context='-nosound audio, pose and navigation warnings are classified without asserting all warnings are harmless; other warnings require review')
    report['complete'] = bool(report.get('probe_passed') and int(exit_code) == 0 and automation and protection and matching and not errors)
    report['overall_user_acceptance_complete'] = False
    report['final_verification_pending'] = False
    path.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(dict(complete=report['complete'], engine_errors=len(errors), automation_passed=automation)))
    return 0 if report['complete'] else 1


if '--verify-report' in sys.argv:
    pos = sys.argv.index('--verify-report')
    assert len(sys.argv[pos + 1:]) == 4
    raise SystemExit(verify_report(*sys.argv[pos + 1:]))

import unreal as ue

COMMAND = ue.SystemLibrary.get_command_line()
match = re.search(r'-DCPlayerStatusProbe=([A-Za-z0-9_]+)(?:\s|$)', COMMAND)
assert match
requested_size = re.search(r'-DCPlayerStatusRequestedResolution=(1920x1080|1280x720)(?:\s|$)', COMMAND, re.I)
if '-dcplayerstatusrequestedresolution=' in COMMAND.lower():
    assert requested_size, 'Supported optional resolution requests: 1920x1080 or 1280x720'
assert all(flag in COMMAND.lower() for flag in ('-unattended', '-sccprovider=none', '-renderoffscreen', '-nosound',
    '-dcplayerstatuspaintdiagnostics', '-dcbosshudpaintdiagnostics', '-forcelogflush'))
assert '-nullrhi' not in COMMAND.lower() and not ue.SourceControl.is_enabled()
RUN = match.group(1)
DIRECTORY = REPORT_ROOT / RUN
DIRECTORY.mkdir(parents=True, exist_ok=True)
OUT = DIRECTORY / 'pie.json'
assert not OUT.exists() and not (DIRECTORY / 'protection_before.json').exists()
def argument(name):
    found = re.search(r'-' + name + r'=(?:"([^"]+)"|(\S+))', COMMAND, re.I)
    assert found, name
    return found.group(1) or found.group(2)
PAINT_LOG = Path(argument('abslog')).resolve()
assert PAINT_LOG.is_relative_to(DIRECTORY.resolve())
assert Path(argument('GameUserSettingsINI')).resolve().is_relative_to(DIRECTORY.resolve())
editor = ue.get_editor_subsystem(ue.UnrealEditorSubsystem)
assert not editor.get_game_world(), 'Pre-arm before Project.Maps.PIE'
before = dict(inventory=inventory(), hashes=protected_hashes())
(DIRECTORY / 'protection_before.json').write_text(json.dumps(before, ensure_ascii=False), encoding='utf-8')
state = dict(step='menu_initial', started=time.monotonic(), busy=False, done=False, mutations=[], immunity=None, inputs={},
             visited=set(), hit_index=0, shot_count=0, paint_floor=0)
report = dict(run=RUN, complete=False, probe_passed=False, events=[], failures=[], shots=[], actual_paints=[], asset_saves=0,
    cpp_build=False, visual_acceptance=False, physical_input_verified=False, two_resolution_user_confirmation_pending=True,
    max_health_change_verified=False, max_health_change_limit='No suitable reflected runtime MaxHealth setter; no CDO/AttributeData mutation or non-CDO MakeSpecHandle workaround',
    nondefault_weapon_ratios_PIE_verified=False,
    nondefault_weapon_ratios_limit='DamageToArmorRatio is EditDefaultsOnly; this PIE run reads unchanged defaults and performs no ratio mutation',
    nondefault_weapon_ratios_evidence_scope='Separate native Armor tests, including DreamCatcher.UI.PlayerStatus.Armor.AllocationAndWeaponSource; not this PIE run',
    screenshot_verified=False, screenshot_limit='Prior Shot SHOWUI captured editor grid; no new screenshot is treated as UI proof',
    raw_damage_message_observed=False, expected_damage_source='Current NPC CombatSet.BaseDamage, original body-hit/no-distance-falloff contract; actual shot trace and ammo consumption are checked')


def native(name):
    value = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert value, name
    return value


def bp(path):
    value = ue.load_class(None, path + '.' + path.rsplit('/', 1)[-1] + '_C')
    assert value, path
    return value


HUD = bp('/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout')
FRONTEND = bp('/Game/DreamCatcher/UI/Frontend/W_DC_Frontend')
DAMAGE = bp('/Game/GameplayEffects/Damage/GE_Damage_Basic_SetByCaller')
HEALTH = native('DCLyraHealthComponent')
BOT = native('DCLyraPlayerBotController')
WEAPON = native('DCLyraRangedWeaponInstance')
EQUIPMENT = native('DCLyraEquipmentManagerComponent')
OVERLAY = native('DCStageOverlayWidget')
RESULT = native('DCStageResultLayer')
gas = ue.get_default_object(ue.load_class(None, '/Script/GameplayAbilities.AbilitySystemBlueprintLibrary'))
subs = ue.get_default_object(ue.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
math_library = ue.get_default_object(ue.load_class(None, '/Script/Engine.KismetMathLibrary'))


def tag(name):
    value = ue.GameplayTag()
    assert value.import_text('(TagName="' + name + '")')
    return value


IMMUNITY = ue.GameplayTagContainer()
assert IMMUNITY.import_text('(GameplayTags=((TagName="Gameplay.DamageImmunity")))')
INPUT_DATA = ue.load_asset('/Game/DreamCatcher/Input/DA_DC_RifleInput')
FIRE = INPUT_DATA.find_ability_input_action_for_tag(tag('InputTag.Weapon.FireAuto'), True)
ULTIMATE = INPUT_DATA.find_ability_input_action_for_tag(tag('InputTag.Ultimate'), True)
assert FIRE and ULTIMATE


def valid(obj):
    if obj is None:
        return False
    if not isinstance(obj, ue.Object):
        raise TypeError('Expected UObject')
    try:
        obj.get_path_name()
    except Exception as exc:
        if re.fullmatch(r'(?:[^:\r\n]+: )?Internal Error - ObjectInstance is null!', str(exc)):
            return False
        raise
    return ue.SystemLibrary.is_valid(obj)


def event(name, **values):
    row = dict(event=name, frame=int(ue.SystemLibrary.get_frame_count()), **values)
    report['events'].append(row)
    print('PLAYER_STATUS_PIE ' + json.dumps(row, ensure_ascii=False))


def set_step(name, now, **values):
    state.update(step=name, at=now, paint_floor=int(ue.SystemLibrary.get_frame_count()), **values)


def owned_by(obj, owner):
    outer = obj.get_outer()
    while outer:
        if outer == owner:
            return True
        outer = outer.get_outer()
    return False


def child(owner, name):
    values = [w for w in ue.ObjectIterator(ue.Widget) if valid(w) and w.get_name() == name and owned_by(w, owner)]
    assert len(values) == 1, (name, len(values))
    return values[0]


def widgets(pc, world, cls):
    return [w for w in ue.ObjectIterator(ue.UserWidget) if valid(w) and w.get_class() == cls
            and w.get_owning_player() == pc and w.get_world() == world and not w.get_name().startswith('Default__') and w.is_activated()]


def click(widget):
    if isinstance(widget, ue.CommonButtonBase):
        widget.call_method('HandleButtonClicked')
    else:
        widget.get_editor_property('OnClicked').broadcast()


def context(pc, world):
    layouts = widgets(pc, world, HUD)
    assert len(layouts) <= 1
    if not layouts:
        return None
    layout = layouts[0]
    status = child(layout, 'BW_Healthbar')
    assert isinstance(status, ue.DCPlayerStatusWidget)
    assert status.get_owning_player() == pc and status.get_world() == world
    if not status.has_player_status():
        return None
    assert status.get_status_pawn() == pc.get_controlled_pawn()
    point = child(layout, 'ExtensionPoint_ModeStatus')
    entries = [e for e in point.get_all_entries() if valid(e) and e.get_class() == OVERLAY]
    assert len(entries) == 1
    return dict(layout=layout, status=status, overlay=entries[0], radar=child(layout, 'Radar'))


def stats(pawn):
    component = pawn.get_component_by_class(HEALTH)
    assert component
    return dict(health=component.get_health(), max_health=component.get_max_health(),
                armor=component.get_armor(), max_armor=component.get_max_armor())


def asc(pawn):
    return gas.call_method('GetAbilitySystemComponent', args=(pawn,))


def split_spec_export(value):
    """Bounded top-level split, using mission_hud_setup.split_fields' quote rules."""
    assert isinstance(value, str) and len(value) <= 524288, 'Ability export exceeds bounded schema size'
    value = value.strip()
    if value in ('', '()'):
        return []
    assert value.startswith('(') and value.endswith(')'), 'Expected parenthesized ability export'
    parts, start, depth, quoted, escaped = [], 1, 0, False, False
    for index, char in enumerate(value[1:-1], 1):
        if quoted:
            if escaped:
                escaped = False
            elif char == '\\':
                escaped = True
            elif char == '"':
                quoted = False
        elif char == '"':
            quoted = True
        elif char == '(':
            depth += 1
            assert depth <= 64, 'Ability export nesting exceeds schema bound'
        elif char == ')':
            depth -= 1
            assert depth >= 0, 'Unbalanced ability export'
        elif char == ',' and depth == 0:
            part = value[start:index].strip()
            assert part and len(parts) < 512, 'Empty or excessive ability export token'
            parts.append(part)
            start = index + 1
    assert not quoted and not escaped and depth == 0, 'Unterminated ability export quote/group'
    tail = value[start:-1].strip()
    assert tail and len(parts) < 512, 'Empty or excessive final ability export token'
    return parts + [tail]


def spec_export_fields(value):
    fields = {}
    for part in split_spec_export(value):
        name, separator, content = part.partition('=')
        name = name.strip()
        assert separator and re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name), 'Unsupported ability export field'
        assert name not in fields, 'Duplicate ability export field: ' + name
        fields[name] = content.strip()
    return fields


def parse_spec_handle(value):
    fields = spec_export_fields(value)
    assert set(fields) == {'Handle'} and re.fullmatch(r'[1-9][0-9]*', fields['Handle']), 'Missing/invalid exported ability handle'
    handle = int(fields['Handle'])
    assert handle <= 2147483647, 'Exported ability handle exceeds native int32 range'
    return handle


def parse_ability_container(value):
    fields = spec_export_fields(value)
    assert 'Items' in fields, 'Ability container export omitted Items; not proof of no Q'
    items = split_spec_export(fields['Items'])
    assert len(items) <= 256, 'Ability spec count exceeds bounded schema'
    parsed, handles = [], set()
    for item in items:
        spec = spec_export_fields(item)
        assert 'Handle' in spec, 'Required spec handle missing from public export'
        handle = parse_spec_handle(spec['Handle'])
        assert handle not in handles, 'Duplicate exported ability handle'
        handles.add(handle)
        # FArrayProperty::ExportTextInnerItem compares struct array entries to
        # initialized CPP defaults. FGameplayAbilitySpec defaults Ability=null,
        # PendingRemove=false, and DynamicAbilityTags=empty. Schema preflight
        # separately proves non-default references/true/nonempty tags survive.
        pending = spec.get('PendingRemove', 'False')
        assert pending in ('True', 'False'), 'Unsupported PendingRemove export'
        tags = spec_export_fields(spec['DynamicAbilityTags']) if 'DynamicAbilityTags' in spec else {'GameplayTags': ''}
        assert set(tags) == {'GameplayTags'}, 'Unsupported dynamic source tag container'
        names = []
        for tag_value in split_spec_export(tags['GameplayTags']):
            tag_fields = spec_export_fields(tag_value)
            assert set(tag_fields) == {'TagName'}, 'Unsupported dynamic input tag export'
            name = json.loads(tag_fields['TagName']) if tag_fields['TagName'].startswith('"') else tag_fields['TagName']
            assert isinstance(name, str) and re.fullmatch(r'[A-Za-z_0-9.]+', name), 'Invalid exported dynamic tag name'
            assert name not in names, 'Duplicate exported dynamic input tag'
            names.append(name)
        reference = spec.get('Ability', 'None')
        reference = json.loads(reference) if reference.startswith('"') else reference
        assert isinstance(reference, str) and re.fullmatch(
            r"(?:None|/[A-Za-z_0-9./:]+|/?[A-Za-z_][A-Za-z_0-9./:]*'/[A-Za-z_0-9./:]+')", reference), 'Unsupported ability reference export'
        omissions = [name for name in ('Ability', 'PendingRemove', 'DynamicAbilityTags') if name not in spec]
        parsed.append(dict(handle=handle, ability=None if reference == 'None' else reference, input_tags=names,
                           pending_remove=pending == 'True', cpp_default_omissions=omissions,
                           tag_evidence='CPP default empty omitted by array diff export' if 'DynamicAbilityTags' not in spec else 'Explicit dynamic source tags in export'))
    return parsed


def query_q_specs(ctx, pc, pawn):
    """Read actual granted input tags and ActorInfo; never infer absence from HUD 0."""
    source = asc(pawn)
    assert valid(source) and source == ctx['status'].get_status_ability_system()
    assert source.get_world() == pawn.get_world() == ctx['status'].get_world()
    handles = list(source.get_all_abilities())
    # ActivatableAbilities is BlueprintReadOnly and publicly readable. Its
    # StructBase.export_text is the engine's public UScriptStruct serializer;
    # no protected AvatarActor/spec field getter or property write is used.
    container_text = source.get_editor_property('ActivatableAbilities').export_text()
    values = parse_ability_container(container_text)
    handle_texts = [handle.export_text() for handle in handles]
    handle_ids = [parse_spec_handle(text) for text in handle_texts]
    assert len(handle_ids) == len(set(handle_ids))
    assert len(handle_ids) == len(values) and set(handle_ids) == {value['handle'] for value in values}, 'GetAllAbilities disagrees with actual exported spec container'
    actor_info = None
    for handle in handles:
        entry = next(value for value in values if value['handle'] == parse_spec_handle(handle.export_text()))
        assert entry['ability'] is not None, 'Live granted spec has null serialized Ability; not evidence of normal Q absence'
        assert not entry['pending_remove'], 'Pending removal is not a stable live Q inspection point'
        result = gas.call_method('GetGameplayAbilityFromSpecHandle', args=(source, handle))
        assert isinstance(result, tuple) and len(result) == 2, 'Unexpected reflected ability/instance result'
        ability, is_instance = result
        assert valid(ability) and isinstance(is_instance, bool)
        entry['public_ability'] = ability.get_path_name()
        entry['public_is_instance'] = is_instance
        if not is_instance or actor_info is not None:
            continue  # GetActorInfo on a CDO can ensure; do not call it.
        info = ability.get_actor_info()
        info_asc = info.get_editor_property('AbilitySystemComponent')
        info_avatar = info.get_editor_property('AvatarActor')
        info_pc = info.get_editor_property('PlayerController')
        assert info_asc == source and info_avatar == pawn and info_pc == pc, 'Actual instanced ability ActorInfo does not match current HUD source'
        actor_info = dict(instance=ability.get_path_name(), asc=info_asc.get_path_name(),
                          avatar=info_avatar.get_path_name(), controller=info_pc.get_path_name())
    assert actor_info, 'No valid instanced ability ActorInfo evidence; do not classify HUD 0 as normal absence'
    matches = [value for value in values if 'InputTag.Ultimate' in value['input_tags']]
    return dict(asc=source.get_path_name(), pawn=pawn.get_path_name(), handles=handle_texts,
                specs=values, ultimate_matches=matches, actor_info=actor_info, container_export=container_text)


def begin_human_fire(pc, pawn, now):
    player_weapon = weapon(pawn)
    assert player_weapon
    state['human_ammo'] = ammo(player_weapon)
    pc.set_control_rotation(ue.Rotator(pitch=60, yaw=90))
    inject(pc, FIRE, True)
    set_step('human_fire', now)


def immunity(pawn, enabled):
    if enabled:
        assert state.get('immunity') in (None, pawn)
        if state.get('immunity') == pawn:
            return
        owned = ue.GameplayTagLibrary.get_owned_gameplay_tags(asc(pawn)).export_text()
        assert 'Gameplay.DamageImmunity' not in owned, 'Do not borrow a preexisting immunity contribution'
        assert gas.call_method('AddLooseGameplayTags', args=(pawn, IMMUNITY, False))
        state['immunity'] = pawn
    else:
        previous = state.pop('immunity', None)
        if valid(previous) and asc(previous):
            assert gas.call_method('RemoveLooseGameplayTags', args=(previous, IMMUNITY, False))


def inject(pc, action, enabled):
    system = subs.call_method('GetLocalPlayerSubSystemFromPlayerController', args=(pc,
        ue.load_class(None, '/Script/EnhancedInput.EnhancedInputLocalPlayerSubsystem')))
    assert system
    system.inject_input_vector_for_action(action, ue.Vector(1 if enabled else 0, 0, 0), [], [])
    state['inputs'][action.get_path_name()] = (pc, action)


def paints(prefix, floor):
    ue.log_flush()
    if not PAINT_LOG.is_file():
        return []
    with PAINT_LOG.open('rb') as f:
        f.seek(0, 2)
        f.seek(max(0, f.tell() - 1048576))
        text = f.read().decode('utf-8-sig', errors='replace')
    return native_rows(text, prefix, floor, int(ue.SystemLibrary.get_frame_count()))


def status_paint(ctx, world, expected, label, previous_health=None):
    rows = [r for r in paints('DCPLAYER_STATUS_PAINT', state['paint_floor'])
            if r['world'] == world.get_path_name() and r['widget'] == ctx['status'].get_path_name()
            and r['pawn'] == ctx['status'].get_status_pawn().get_path_name()]
    passing = [r for r in rows if not paint_problems(r, expected, previous_health)]
    if not passing:
        state['last_paint_problems'] = paint_problems(rows[-1], expected, previous_health) if rows else ['no_fresh_matching_paint']
        assert ue.GameplayStatics.get_time_seconds(world) - state['at'] < 8, (label, state['last_paint_problems'])
        return None
    row = passing[-1]
    report['actual_paints'].append(dict(label=label, paint=row))
    event('actual_status_paint', label=label, paint_frame=row['frame'])
    return row


def weapon(pawn):
    manager = pawn.get_component_by_class(EQUIPMENT)
    return manager.get_first_instance_of_type(WEAPON) if manager else None


def ammo(instance):
    item = instance.get_instigator()
    assert item
    return item.get_stat_tag_stack_count(tag('Lyra.ShooterGame.Weapon.MagazineAmmo'))


def freeze_actor(actor):
    if any(obj == actor and kind == 'transform' for obj, kind, _, _ in state['mutations']):
        return
    state['mutations'].append((actor, 'transform', '', actor.get_actor_transform()))
    movement = actor.get_movement_component()
    state['mutations'].append((movement, 'movement', '', (movement.get_editor_property('MovementMode'), movement.get_editor_property('CustomMovementMode'))))
    movement.stop_movement_immediately()
    movement.disable_movement()


def stop_bots(world):
    bots = []
    for bot in ue.GameplayStatics.get_all_actors_of_class(world, BOT):
        pawn = bot.get_controlled_pawn()
        if not pawn or not pawn.get_component_by_class(HEALTH) or pawn.get_component_by_class(HEALTH).is_dead_or_dying():
            continue
        # Pawn inventory/QuickBar readiness is asynchronous. Leave its initial
        # behavior and movement alone until the real equipped weapon exists.
        if not weapon(pawn):
            continue
        brain = bot.get_editor_property('BrainComponent')
        if brain and brain.is_running():
            if not any(obj == brain and kind == 'brain' for obj, kind, _, _ in state['mutations']):
                state['mutations'].append((brain, 'brain', '', True))
            brain.stop_logic('bounded player status PIE probe')
        bot.stop_movement()
        bots.append(bot)
    return bots


def setup_shot(pc, pawn, bot):
    enemy = bot.get_controlled_pawn()
    instance = weapon(enemy)
    assert instance and valid(instance) and instance.get_world() == pawn.get_world()
    ratio = float(instance.get_editor_property('DamageToArmorRatio'))
    assert abs(ratio - .8) < .0001, 'The approved default ratio is read, never overwritten'
    player_state = bot.get_editor_property('PlayerState')
    sets = [s for s in ue.ObjectIterator(native('DCCombatSet')) if valid(s) and s.get_outer() == player_state]
    assert len(sets) == 1
    text = sets[0].get_editor_property('BaseDamage').export_text()
    damage = float(re.search(r'CurrentValue=([^,)]+)', text).group(1))
    assert damage > 0 and 'Gameplay.Damage.IgnoreWeaponBonus' in ue.GameplayTagLibrary.get_owned_gameplay_tags(asc(enemy)).export_text()
    freeze_actor(pawn)
    freeze_actor(enemy)
    for controller in (pc, bot):
        if not any(obj == controller and kind == 'control_rotation' for obj, kind, _, _ in state['mutations']):
            state['mutations'].append((controller, 'control_rotation', '', controller.get_control_rotation()))
    if not any(obj == bot and kind == 'focus' for obj, kind, _, _ in state['mutations']):
        state['mutations'].append((bot, 'focus', '', (bot.get_focus_actor(), bot.get_focal_point())))
    origin = pawn.get_actor_location()
    if 'fixture_origin' not in state:
        state['fixture_origin'] = origin + ue.Vector(0, 0, 6000)
    pawn.set_actor_location(state['fixture_origin'], False, True)
    enemy.set_actor_location(state['fixture_origin'] + ue.Vector(650, 0, 0), False, True)
    enemy.set_actor_rotation(ue.Rotator(yaw=180), True)
    mesh = pawn.get_component_by_class(ue.SkeletalMeshComponent)
    physics = mesh.get_editor_property('PhysicsAssetOverride') or mesh.get_skinned_asset().get_editor_property('PhysicsAsset')
    bodies = [b for b in ue.ObjectIterator(ue.load_class(None, '/Script/Engine.SkeletalBodySetup'))
              if valid(b) and b.get_outer() == physics and str(b.get_editor_property('BoneName')) == 'spine_02']
    assert len(bodies) == 1
    capsules = list(bodies[0].get_editor_property('AggGeom').get_editor_property('SphylElems'))
    assert capsules
    state.update(bot=bot, enemy=enemy, shot_weapon=instance, ratio=ratio, expected_damage=damage,
                 body_center=capsules[0].get_editor_property('Center'))


def aim_shot(world, pawn):
    mesh = pawn.get_component_by_class(ue.SkeletalMeshComponent)
    transform = mesh.get_socket_transform('spine_02', ue.RelativeTransformSpace.RTS_WORLD)
    point = math_library.call_method('TransformLocation', args=(transform, state['body_center']))
    enemy = state['enemy']
    start = enemy.get_actor_location() + ue.Vector(0, 0, enemy.get_editor_property('BaseEyeHeight'))
    state['bot'].set_focal_point(point)
    state['bot'].set_control_rotation(math_library.call_method('FindLookAtRotation', args=(start, point)))
    return start, point


def clear_enemy(source, target):
    source_asc, target_asc = asc(source), asc(target)
    assert source_asc and target_asc
    spec = source_asc.make_outgoing_spec(DAMAGE, 1, source_asc.make_effect_context())
    spec = gas.call_method('AssignTagSetByCallerMagnitude', args=(spec, tag('SetByCaller.Damage'), 100000.))
    source_asc.apply_gameplay_effect_spec_to_target(spec, target_asc)


def restore_mutations():
    for obj, kind, name, value in reversed(state.pop('mutations', [])):
        try:
            if not valid(obj):
                continue
            if kind == 'transform':
                obj.set_actor_transform(value, False, True)
            elif kind == 'movement':
                obj.set_movement_mode(value[0], value[1])
            elif kind == 'control_rotation':
                obj.set_control_rotation(value)
            elif kind == 'focus':
                if valid(value[0]):
                    obj.set_focus(value[0])
                elif all(math.isfinite(v) and abs(v) < 1.e20 for v in (value[1].x, value[1].y, value[1].z)):
                    obj.set_focal_point(value[1])
                else:
                    obj.clear_focus()
            elif kind == 'brain' and value:
                obj.restart_logic()
        except Exception:
            report['failures'].append(traceback.format_exc())
    state['mutations'] = []


def before_travel():
    for pc, action in list(state.get('inputs', {}).values()):
        try:
            if valid(pc):
                inject(pc, action, False)
        except Exception:
            report['failures'].append(traceback.format_exc())
    state['inputs'] = {}
    for cleanup in (lambda: immunity(None, False), restore_mutations):
        try:
            cleanup()
        except Exception:
            report['failures'].append(traceback.format_exc())
    for key in ('bot', 'enemy', 'shot_weapon', 'body_center', 'fixture_origin'):
        state.pop(key, None)


def finish(ok):
    if state['done']:
        return
    state['done'] = True
    for cleanup in (before_travel,):
        try:
            cleanup()
        except Exception:
            report['failures'].append(traceback.format_exc())
    report['protection_before_exit'] = inventory() == before['inventory'] and protected_hashes() == before['hashes']
    report['feature_flow_passed'] = bool(ok and not report['failures'])
    report['probe_passed'] = bool(report['feature_flow_passed'] and report['protection_before_exit'])
    report['final_verification_pending'] = True
    event('finished', probe_passed=report['probe_passed'], max_health_change_verified=False, visual_acceptance=False)
    OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.unregister_slate_post_tick_callback(state['callback'])
    if not report['probe_passed']:
        ue.log_error('Player status feature PIE probe failed: ' + str(OUT))


def tick(delta):
    if state['done'] or state['busy']:
        return
    state['busy'] = True
    try:
        assert time.monotonic() - state['started'] < 250, ('timeout', state['step'], state.get('last_paint_problems'))
        world = editor.get_game_world()
        if not world or 'UEDPIE_' not in world.get_path_name():
            return
        pc = ue.GameplayStatics.get_player_controller(world, 0)
        if not pc:
            return
        now = ue.GameplayStatics.get_time_seconds(world)
        level = ue.GameplayStatics.get_current_level_name(world, True)
        step = state['step']
        if step in ('menu_initial', 'menu_return'):
            menus = widgets(pc, world, FRONTEND)
            if level != 'L_DC_FrontEnd' or now < 2 or not menus:
                return
            assert len(menus) == 1 and not widgets(pc, world, HUD)
            click(child(menus[0], 'StoryButton'))
            set_step('initial' if step == 'menu_initial' else 'reentry', now)
            return
        if level != 'Level_1':
            return
        mode = ue.GameplayStatics.get_game_mode(world)
        if step in ('success', 'death_result'):
            results = widgets(pc, world, RESULT)
            if not results or now - state['at'] < .5:
                return
            assert len(results) == 1 and mode.has_run_ended()
            if step == 'death_result':
                current_pawn = pc.get_controlled_pawn()
                if current_pawn:
                    assert current_pawn.get_path_name() == state['death_pawn_path'], 'Death result acquired a different current Pawn'
                death_health = current_pawn.get_component_by_class(HEALTH) if current_pawn else None
                if death_health:
                    assert death_health.is_dead_or_dying() and abs(death_health.get_health()) < .01, 'Retained current Pawn is not actually dead'
                component_released = not death_health or death_health.get_max_health() <= 0
                layouts = widgets(pc, world, HUD)
                assert len(layouts) <= 1
                if current_pawn and not component_released:
                    assert layouts, 'Missing retained-death HUD evidence'
                cleanup = []
                for layout in layouts:
                    status = child(layout, 'BW_Healthbar')
                    assert status.get_world() == world and status.get_owning_player() == pc
                    assert status.get_signature_state() == ue.DCPlayerSignatureState.UNAVAILABLE
                    assert status.get_signature_progress() == 0
                    if current_pawn and not component_released:
                        assert status.has_player_status() and status.get_status_pawn() == current_pawn
                        assert abs(status.get_displayed_health()) < .01
                        rows = [r for r in paints('DCPLAYER_STATUS_PAINT', state['paint_floor'])
                                if r['world'] == world.get_path_name() and r['widget'] == status.get_path_name()
                                and r['pawn'] == current_pawn.get_path_name() and r['controller'] == pc.get_path_name()]
                        visible_dead = []
                        for row in rows:
                            numbers = re.findall(r'-?\d+(?:\.\d+)?', row.get('health_current_text', '').replace(',', ''))
                            if (abs(row['health']) < .01 and row['signature_state'] == 'Unavailable'
                                    and row['progress'] == 0 and row['active_range'] == [0., 0., 0., 0.]
                                    and len(numbers) == 1 and abs(float(numbers[0])) < .01):
                                visible_dead.append(row)
                        if not visible_dead:
                            assert now - state['at'] < 8, 'No fresh retained-death HP0/Unavailable paint after original numeric animation settled'
                            return
                        report['death_status_actual_paint'] = visible_dead[-1]
                        branch = 'retained_current_dead_pawn'
                    else:
                        assert not status.has_player_status()
                        if current_pawn:
                            assert status.get_status_pawn() in (None, current_pawn), 'Released component HUD points at another Pawn'
                            branch = 'retained_pawn_health_component_released'
                        else:
                            assert not status.get_status_pawn()
                            branch = 'unpossessed_source_cleared'
                        assert child(status, 'PlayerStatusBars').get_visibility() == ue.SlateVisibility.COLLAPSED
                        assert child(status, 'PlayerSignature').get_visibility() == ue.SlateVisibility.COLLAPSED
                    cleanup.append(dict(widget=status.get_path_name(), branch=branch,
                                        pawn=current_pawn.get_path_name() if current_pawn else None))
                event('death_status_current_source_or_cleanup', active_layout_statuses=cleanup,
                      retained_pawn=current_pawn.get_path_name() if current_pawn else None,
                      health_component_released=component_released,
                      original_death_material_timeline_preserved=True)
            before_travel()
            if step == 'success':
                assert mode.was_run_successful() and state['visited'] == {0, 1, 2}
                click(child(results[0], 'RetryButton'))
                state['travel_time'] = now
                set_step('restart', now)
            else:
                assert not mode.was_run_successful()
                click(child(results[0], 'ExitButton'))
                set_step('menu_return', now)
            assert mode.is_stage_travel_pending()
            return
        if step == 'restart' and now >= state['travel_time'] - .5:
            return
        pawn = pc.get_controlled_pawn()
        if not pawn or not pawn.get_component_by_class(HEALTH) or stats(pawn)['max_health'] <= 0:
            return
        if state['step'] != 'shot_fired':
            immunity(pawn, True)
        bots = stop_bots(world)
        ctx = context(pc, world)
        if not ctx or now < 2:
            return
        director = mode.get_stage_director()
        assert director and not mode.has_run_ended()
        phase = director.get_current_phase_index()
        state['visited'].add(phase)
        target = director.get_tracked_mission_target_component()
        assert ctx['radar'].is_radar_active() and ctx['radar'].get_objective_component() == target and valid(target)
        if step in ('initial', 'restart', 'reentry'):
            assert phase == 0
            expected = dict(health=100., max_health=100., armor=150., max_armor=150.)
            assert all(abs(stats(pawn)[k] - v) < .01 for k, v in expected.items())
            row = status_paint(ctx, world, expected, step)
            if not row:
                return
            actual_viewport = list(pc.get_viewport_size())
            requested = [int(v) for v in requested_size.group(1).split('x')] if requested_size else None
            report.setdefault('viewport_observations', []).append(dict(label=step, actual_viewport=actual_viewport,
                native_root=row['root_size'], requested=requested, viewport_matches_request=actual_viewport == requested if requested else None,
                two_resolution_visual_acceptance=False))
            if step == 'reentry':
                event('fresh_reentry_initialization_and_paint')
                before_travel()
                finish(True)
                return
            if step == 'restart':
                event('retry_fresh_stats_and_paint')
                state['death_pawn_path'] = pawn.get_path_name()
                immunity(pawn, False)
                clear_enemy(pawn, pawn)
                set_step('death_result', now)
                return
            if not bots or not weapon(bots[0].get_controlled_pawn()) or not weapon(pawn):
                return
            # Discover unsupported live metadata before the already-verified
            # seventeen-shot armor sequence, without treating unreadable data as 0.
            report['Q_initial_source_metadata'] = query_q_specs(ctx, pc, pawn)
            setup_shot(pc, pawn, bots[0])
            state['shot_label'] = 'default_npc_ratio'
            set_step('shot_settle', now)
        elif step == 'shot_settle':
            start, point = aim_shot(world, pawn)
            if now - state['at'] < 1:
                return
            direction = (point - start).normal()
            hit = ue.SystemLibrary.line_trace_single(world, start, point + direction * 100, ue.TraceTypeQuery.cast(2), True,
                [state['enemy']], ue.DrawDebugTrace.NONE, True, ue.LinearColor(), ue.LinearColor(), 0)
            trace = hit.export_text() if isinstance(hit, ue.HitResult) else str(hit)
            assert pawn.get_name() in trace and 'WeakSpot' not in trace, trace
            old = stats(pawn)
            assert old['health'] > state['expected_damage']
            state.update(before_stats=old, before_ammo=ammo(state['shot_weapon']), trace=trace)
            immunity(pawn, False)
            gas.call_method('SendGameplayEventToActor', args=(state['enemy'], tag('InputTag.Weapon.Fire'), ue.GameplayEventData()))
            set_step('shot_fired', now)
        elif step == 'shot_fired':
            aim_shot(world, pawn)
            current = stats(pawn)
            used = state['before_ammo'] - ammo(state['shot_weapon'])
            assert used <= 1, ('Expected one actual GA shot', used)
            if used != 1 or current == state['before_stats']:
                assert now - state['at'] < 5, 'Actual equipped NPC shot produced no observed damage'
                return
            immunity(pawn, True)
            old, damage, ratio = state['before_stats'], state['expected_damage'], state['ratio']
            expected_armor = max(0., old['armor'] - damage * ratio)
            expected_hp = old['health'] - damage * (1 - ratio) if old['armor'] > 0 else old['health'] - damage
            assert abs(current['armor'] - expected_armor) < .025 and abs(current['health'] - expected_hp) < .025, (old, current, damage, ratio)
            row = dict(label=state['shot_label'], weapon=state['shot_weapon'].get_path_name(), ratio=ratio,
                calculated_expected_D=damage, before=old, after=current, ammo_used=used, trace=state['trace'],
                shortage=0 < old['armor'] < damage * ratio, unarmored=old['armor'] == 0)
            report['shots'].append(row)
            event('actual_npc_weapon_GE_damage', **row)
            set_step('shot_paint', now, expected=current)
        elif step == 'shot_paint':
            row = status_paint(ctx, world, state['expected'], state['shot_label'], state['before_stats']['health'])
            if not row:
                return
            label = state['shot_label']
            if label == 'default_npc_ratio':
                state['first_weapon_path'] = state['shot_weapon'].get_path_name()
                restore_mutations()
                for bot in bots:
                    clear_enemy(pawn, bot.get_controlled_pawn())
                set_step('second_encounter', now)
            elif state['before_stats']['armor'] == 0:
                assert any(r['shortage'] for r in report['shots'])
                state['q_before_health'] = state['expected']['health']
                set_step('q_ready', now)
            else:
                state['shot_label'] = 'second_weapon_until_shortage'
                set_step('shot_settle', now)
        elif step == 'second_encounter':
            assert now - state['at'] < 15, ('Second encounter phase/equipment did not become ready within 15 world seconds',
                phase, len(bots))
            if phase != 1 or not bots:
                return
            setup_shot(pc, pawn, bots[0])
            assert state['shot_weapon'].get_path_name() != state['first_weapon_path']
            state['shot_label'] = 'second_weapon_default_ratio'
            set_step('shot_settle', now)
        elif step == 'q_ready':
            row = status_paint(ctx, world, stats(pawn), 'before_Q', state['before_stats']['health'])
            if not row:
                return
            evidence = query_q_specs(ctx, pc, pawn)
            count = len(evidence['ultimate_matches'])
            assert row['matching_specs'] == count, 'Native HUD count disagrees with actual spec input tags'
            if count == 1:
                assert not evidence['ultimate_matches'][0]['pending_remove']
                if row['signature_state'] != 'Ready':
                    assert now - state['at'] < 8, 'A real single Q spec did not reach Ready'
                    return
                assert row['cooldown_remaining'] == 0
                branch = 'single_ability'
            else:
                branch = 'no_ability' if count == 0 else 'ambiguous_multiple_abilities'
                assert row['signature_state'] == 'Unavailable' and row['progress'] == 0
                assert row['active_range'] == [0., 0., 0., 0.]
            report['Q_actual_spec_inspection_before'] = evidence
            report['Q_branch'] = branch
            inject(pc, ULTIMATE, True)
            set_step('q_release', now)
        elif step == 'q_release':
            inject(pc, ULTIMATE, False)
            # Keep the frame floor from before the press; do not discard the
            # first actual Active paint when releasing on the following tick.
            state.update(step='q_observe', at=now, q_release_frame=int(ue.SystemLibrary.get_frame_count()))
        elif step == 'q_observe':
            rows = [r for r in paints('DCPLAYER_STATUS_PAINT', state['paint_floor'])
                    if r['world'] == world.get_path_name() and r['widget'] == ctx['status'].get_path_name()]
            report.setdefault('q_paints', []).extend(r for r in rows if r['frame'] not in {v['frame'] for v in report.get('q_paints', [])})
            all_rows = report.get('q_paints', [])
            if report['Q_branch'] != 'single_ability':
                after_release = [r for r in all_rows if r['frame'] > state['q_release_frame']]
                if not after_release:
                    assert now - state['at'] < 8, 'No fresh disabled Q paint after actual action release'
                    return
                evidence = query_q_specs(ctx, pc, pawn)
                count = len(evidence['ultimate_matches'])
                assert count == len(report['Q_actual_spec_inspection_before']['ultimate_matches'])
                for r in all_rows:
                    assert r['signature_state'] == 'Unavailable' and r['matching_specs'] == count
                    assert r['progress'] == 0 and r['active_range'] == [0., 0., 0., 0.]
                    assert r['cooldown_remaining'] == 0 and r['cooldown_duration'] == 0
                report['Q_actual_spec_inspection_after'] = evidence
                report['Q_disabled_actual_input_paint_verified'] = True
                report['Q_input_and_actual_paint_verified'] = False
                report['Q_ready_active_cooldown_verified'] = False
                report['Q_partial_radial_progress_verified'] = False
                report['Q_ready_active_cooldown_limit'] = ('No InputTag.Ultimate spec is actually granted in this stage' if count == 0 else
                    'Multiple actual InputTag.Ultimate specs make presentation ambiguous; this is distinct from absent ability or invalid ActorInfo')
                event('actual_Q_disabled_after_action_press_release', branch=report['Q_branch'], matching_specs=count,
                      after_release_paint=after_release[-1]['frame'])
                begin_human_fire(pc, pawn, now)
                return
            active = [r for r in all_rows if r['signature_state'] == 'Active']
            cooldown = [r for r in all_rows if r['cooldown_remaining'] > 0 and r['cooldown_duration'] > 0]
            ready = [r for r in all_rows if r['signature_state'] == 'Ready']
            for r in all_rows:
                assert abs(r['active_range'][1] - r['progress']) < .002
                if r['signature_state'] == 'Cooldown':
                    assert abs(r['progress'] - max(0., min(1., 1 - r['cooldown_remaining'] / r['cooldown_duration']))) < .002
            if not active or not cooldown or not ready or ready[-1]['frame'] < cooldown[0]['frame']:
                assert now - state['at'] < 30, 'Missing actual Q active/cooldown/ready paint'
                return
            report['Q_input_and_actual_paint_verified'] = True
            report['Q_ready_active_cooldown_verified'] = True
            report['Q_cooldown_only_state_sampled'] = any(r['signature_state'] == 'Cooldown' for r in all_rows)
            report['Q_active_with_remaining_accepted'] = any(r['signature_state'] == 'Active' and r['cooldown_remaining'] > 0 for r in all_rows)
            report['Q_partial_radial_progress_verified'] = any(r['signature_state'] == 'Cooldown' and 0 < r['progress'] < 1 for r in all_rows)
            report['Q_partial_radial_progress_limit'] = None if report['Q_partial_radial_progress_verified'] else (
                'No cooldown-only partial arc sampled; an ability remaining Active through its cooldown displays the Active presentation. No ability duration was changed.')
            event('actual_EnhancedInput_Q_state_cycle', duration_samples=sorted({r['cooldown_duration'] for r in cooldown}))
            begin_human_fire(pc, pawn, now)
        elif step == 'human_fire':
            if now - state['at'] < .2:
                inject(pc, FIRE, True)
                return
            inject(pc, FIRE, False)
            assert ammo(weapon(pawn)) < state['human_ammo']
            event('human_weapon_EnhancedInput_ammo_regression')
            restore_mutations()
            for enemy in [b.get_controlled_pawn() for b in bots]:
                clear_enemy(pawn, enemy)
            set_step('boss_wait', now)
        elif step == 'boss_wait':
            if phase != 2:
                return
            bosses = [b for b in ue.GameplayStatics.get_all_actors_of_class(world, native('DCBossCharacter'))
                      if b.get_component_by_class(HEALTH) and not b.get_component_by_class(HEALTH).is_dead_or_dying()]
            if not bosses:
                return
            rows = [r for r in paints('DCBOSS_HUD_PAINT', state['paint_floor']) if r['world'] == world.get_path_name()]
            if not rows:
                return
            assert rows[-1]['name'] and rows[-1]['subtitle'] and rows[-1]['bar_size'] == [640., 18.]
            report['boss_actual_paint'] = rows[-1]
            event('mission_radar_boss_regression', phase=phase, radar_target=target.get_path_name())
            for boss in bosses:
                clear_enemy(pawn, boss)
            set_step('success', now)
    except Exception:
        report['failures'].append(traceback.format_exc())
        finish(False)
    finally:
        state['busy'] = False


state['callback'] = ue.register_slate_post_tick_callback(tick)
event('armed', run=RUN, initial_world=False, renderer='offscreen', sound=False)
