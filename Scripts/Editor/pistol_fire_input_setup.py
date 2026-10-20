"""Approved add-only pistol input repair; two existing data assets, no PIE/build.

Offline --commands <prefix> prints apply and fresh verify command arrays only.
Run each in a separate unattended commandlet with other Unreal Editors closed.
Only the original pressed Fire action/tag and its LeftMouseButton mapping are
appended. Existing IA_Fire/FireAuto and every other field remain unchanged.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/Pistol20261020'
SCRIPT = PROJECT / 'Scripts/Editor/pistol_fire_input_setup.py'
ENGINE = Path('E:/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe')
CONFIG = '/Game/DreamCatcher/Input/DA_DC_RifleInput'
IMC = '/Game/DreamCatcher/Input/Mappings/IMC_DC_RifleBase'
ACTION = '/Game/DreamCatcher/Lyra/BaseInput/Actions/IA_Weapon_Fire'
TAG = 'InputTag.Weapon.Fire'
TARGETS = (CONFIG, IMC)


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def asset_file(package):
    assert package.startswith('/Game/')
    return PROJECT / ('Content/' + package[6:] + '.uasset')


def local_module(name):
    spec = importlib.util.spec_from_file_location('pistol_fire_' + name, SCRIPT.parent / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def protection():
    allowed = {asset_file(package) for package in TARGETS}
    inventory = {}
    for directory in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for path in (PROJECT / directory).rglob('*'):
            if path.is_file() and path not in allowed:
                stat = path.stat()
                inventory[path.relative_to(PROJECT).as_posix()] = [stat.st_size, stat.st_mtime_ns]
    final = read(REPORTS / 'Port_U_verify/report.json')
    assert final['complete'] and len(final['final_sha256']) == 197
    frozen = read(REPORTS / 'frozen_cpp_config.json')['files']
    assert all(sha(PROJECT / name) == value for name, value in frozen.items())
    paths = [asset_file(package) for package in final['final_sha256']]
    paths += [PROJECT / name for name in frozen]
    paths += [PROJECT / name for name in ('.git/HEAD', '.git/index', 'DreamCatcher.uproject',
              'Binaries/Win64/UnrealEditor-DreamCatcher.dll')]
    paths += [PROJECT.parent / ('LyraStarterGame/Content/Input/Actions/' + name + '.uasset')
              for name in ('IA_Weapon_Fire', 'IA_Weapon_Fire_Auto')]
    paths += [REPORTS / name for name in ('InputFireRead_Source_A.log', 'InputFireRead_Target_A.log',
              'frozen_cpp_config.json', 'Port_U_verify/report.json')]
    return {'inventory': inventory, 'sha256': {path.as_posix(): sha(path) for path in paths}}


def print_commands(prefix):
    assert re.fullmatch(r'[A-Za-z0-9_]{1,40}', prefix)
    commands = {}
    for mode in ('apply', 'verify'):
        run = prefix + '_' + mode
        directory = REPORTS / ('FireInput_' + run)
        args = [ENGINE.as_posix(), (PROJECT / 'DreamCatcher.uproject').as_posix(),
                '-run=pythonscript', '-script=' + SCRIPT.as_posix(), '-unattended', '-nop4',
                '-SCCProvider=None', '-nullrhi', '-nosound', '-DCPistolFireMode=' + mode,
                '-DCPistolFireRun=' + run, '-abslog=' + (directory / 'Editor.log').as_posix()]
        if mode == 'verify':
            args.append('-DCPistolFireReference=' + prefix + '_apply')
        commands[mode] = {'args': args, 'create_log_directory_first': directory.as_posix(),
                         'windows_command_line': ' '.join('"' + arg + '"' if ' ' in arg else arg for arg in args)}
    print(json.dumps({'launches_processes': False, 'automatic_PIE': False, 'commands': commands}, indent=2))


def action_signature(action):
    return {'triggers': [{'class_name': trigger.get_class().get_name(),
                         'threshold': trigger.get_editor_property('actuation_threshold')}
                        for trigger in action.get_editor_property('triggers')],
            'value_type': str(action.get_editor_property('value_type')),
            'consume_input': action.get_editor_property('consume_input')}


def input_rows(config):
    return list(config.get_editor_property('ability_input_actions'))


def mapping_rows(context):
    return list(context.get_editor_property('default_key_mappings').get_editor_property('mappings'))


def compliant(config, context, action, expected_input, expected_mapping):
    entries, mappings = input_rows(config), mapping_rows(context)
    relevant = [row for row in entries if row.get_editor_property('input_action') == action
                or row.get_editor_property('input_tag').export_text() == '(TagName="' + TAG + '")']
    assert len(relevant) <= 1, 'Duplicate Fire action/tag input rows'
    assert not relevant or relevant[0].export_text() == expected_input, 'Conflicting Fire input row'
    relevant_mapping = [row for row in mappings if row.get_editor_property('action') == action]
    assert len(relevant_mapping) <= 1, 'Duplicate or unapproved Fire action mappings'
    assert not relevant_mapping or relevant_mapping[0].export_text() == expected_mapping, 'Conflicting Fire mapping'
    return bool(relevant), bool(relevant_mapping)


def capture(config, context, directory, label):
    # Only root array fields are separated; named subobjects retain every byte.
    result = {'input': [row.export_text() for row in input_rows(config)],
              'mappings': [row.export_text() for row in mapping_rows(context)]}
    for key, obj, field in (('config_other', config, 'AbilityInputActions'),
                            ('context_other', context, 'DefaultKeyMappings')):
        text = hud.export(obj, directory / (label + '_' + key + '.t3d'))
        text = re.sub(r'^   ' + field + r'(?:\(\d+\))?=[^\n]*\n?', '', text, flags=re.M)
        result[key] = pistol.item_subobject_order(text)
    return result


def run_editor():
    global ue, hud, pistol
    import unreal as ue
    hud, pistol = local_module('ingame_hud_layout_setup'), local_module('pistol_slot_setup')
    command = ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended', '-run=pythonscript', '-sccprovider=none'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == PROJECT.resolve()
    hud.assert_no_other_editor()
    mode = re.search(r'-DCPistolFireMode=(apply|verify)\b', command).group(1)
    run = re.search(r'-DCPistolFireRun=([A-Za-z0-9_]{1,64})\b', command).group(1)
    directory = REPORTS / ('FireInput_' + run)
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / 'report.json'
    assert not output.exists(), 'Use a fresh run ID'
    log_args = re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)', command, re.I)
    assert len(log_args) == 1
    supplied_log = Path(next(value for value in log_args[0] if value).replace('\\', '/'))
    assert supplied_log.is_absolute() and supplied_log.resolve() == (directory / 'Editor.log').resolve()
    before = protection()
    report = {'mode': mode, 'run': run, 'complete': False, 'PIE': False, 'automatic_PIE': False,
              'compile_requested': [], 'asset_saves': [], 'allowed_assets': list(TARGETS),
              'protection_before': before, 'before_sha256': {p: sha(asset_file(p)) for p in TARGETS},
              'limits': 'Physical input, firing, visual/audio and feel require user manual play.'}
    try:
        hud.clean()
        config, context, action = [ue.load_asset(p) for p in (CONFIG, IMC, ACTION)]
        assert config and context and action
        source_lines = re.findall(r'^.*LogPython: DC_PISTOL_INPUT_READ (\{.*\})$',
                                  (REPORTS / 'InputFireRead_Source_A.log').read_text(encoding='utf-8-sig'), re.M)
        assert len(source_lines) == 1
        signature = action_signature(action)
        assert signature == json.loads(source_lines[0])['actions']['/Game/Input/Actions/IA_Weapon_Fire']
        assert signature['triggers'] == [{'class_name': 'InputTriggerPressed', 'threshold': 0.5}]
        assert signature['consume_input'] and signature['value_type'] == '<InputActionValueType.BOOLEAN: 0>'
        report['original_fire_action_signature'] = signature
        new_input = ue.DCInputAction()
        assert new_input.import_text('(InputAction="/Script/EnhancedInput.InputAction\'' + ACTION +
                                     '.IA_Weapon_Fire\'",InputTag=(TagName="' + TAG + '"))')
        assert new_input.get_editor_property('input_action') == action
        new_mapping = ue.EnhancedActionKeyMapping()
        assert new_mapping.import_text('(Action="/Script/EnhancedInput.InputAction\'' + ACTION +
                                       '.IA_Weapon_Fire\'",Key=LeftMouseButton)')
        expected_input, expected_mapping = new_input.export_text(), new_mapping.export_text()
        assert not new_mapping.get_editor_property('triggers') and not new_mapping.get_editor_property('modifiers')
        assert 'SettingBehavior=InheritSettingsFromAction' in expected_mapping
        state = compliant(config, context, action, expected_input, expected_mapping)
        initial = capture(config, context, directory, 'before')
        report['before_data'], report['rows_already_present'] = initial, list(state)
        if mode == 'apply':
            changed = []
            if not state[0]:
                config.set_editor_property('ability_input_actions', input_rows(config) + [new_input])
                changed.append((CONFIG, config))
            if not state[1]:
                context.map_key(action, new_mapping.get_editor_property('key'))
                changed.append((IMC, context))
            expected = dict(initial)
            expected['input'] = initial['input'] + ([] if state[0] else [expected_input])
            expected['mappings'] = initial['mappings'] + ([] if state[1] else [expected_mapping])
            assert capture(config, context, directory, 'after') == expected, 'Unexpected data/subobject/array change'
            assert compliant(config, context, action, expected_input, expected_mapping) == (True, True)
            report['after_data'] = expected
            hud.clean([package for package, _ in changed])
            hud.log_check(directory / 'Editor.log')
            assert protection() == before, 'Protected inputs changed before save'
            for package, obj in changed:
                assert package in TARGETS
                hud.assert_no_other_editor()
                assert ue.EditorAssetLibrary.save_loaded_asset(obj, only_if_is_dirty=False), package
                report['asset_saves'].append(package)
            report['skipped_compliant_assets'] = [p for p in TARGETS if p not in report['asset_saves']]
        else:
            reference = re.search(r'-DCPistolFireReference=([A-Za-z0-9_]{1,64})\b', command).group(1)
            path = REPORTS / ('FireInput_' + reference) / 'report.json'
            previous = read(path)
            assert previous['mode'] == 'apply' and previous['complete'] and previous['protection_passed']
            assert before == previous['protection_before'], 'Protected state differs from completed apply'
            assert report['before_sha256'] == previous['after_sha256'], 'Saved input hashes differ'
            assert initial == previous['after_data'], 'Fresh saved data differs from checked apply'
            assert state == (True, True), 'Missing approved Fire connection'
            report['reference'], report['reference_sha256'] = reference, sha(path)
            report['after_data'] = initial
        hud.clean()
        hud.log_check(directory / 'Editor.log')
        report['complete'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        report['after_sha256'] = {p: sha(asset_file(p)) for p in TARGETS}
        try:
            report['protection_passed'] = protection() == before
            assert report['protection_passed'], 'Protected inventory/hash changed'
            for package in TARGETS:
                if package not in report['asset_saves']:
                    assert report['after_sha256'][package] == report['before_sha256'][package], 'Unsaved input bytes changed'
        except Exception:
            report['protection_passed'], report['complete'] = False, False
            report['protection_error'] = traceback.format_exc()
        report['complete'] = report['complete'] and report['protection_passed']
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        print('DC_PISTOL_FIRE_INPUT ' + json.dumps({key: report.get(key) for key in
              ('mode', 'run', 'complete', 'protection_passed', 'asset_saves', 'error', 'protection_error')}))
        assert report['complete'], 'Pistol input setup incomplete; inspect report'


if __name__ == '__main__':
    if '--commands' in sys.argv:
        print_commands(sys.argv[sys.argv.index('--commands') + 1])
    else:
        run_editor()
