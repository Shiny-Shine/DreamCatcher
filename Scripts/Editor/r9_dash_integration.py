"""Integration-only original Dash grant/binding. No builds, shared edits or HUD shortcuts."""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r9_dash_assets as port
P, R = port.P, port.R
ROOT = '/Game/DreamCatcher/GAS/Test/R5/Integration/'
DATA, INPUT, IMC, SET = [ROOT+n for n in ('DA_DC_RiflePawn', 'DA_DC_RifleInput', 'IMC_DC_RifleBase', 'AS_DC_RifleDash')]
OLD = '/Game/Input/Actions/IA_Dodge.IA_Dodge'
ACTION = '/Game/LyraMigration/BaseInput/Actions/IA_Ability_Dash.IA_Ability_Dash'
GA = '/Game/LyraMigration/Dash/GA_Hero_Dash.GA_Hero_Dash_C'


def load(p):
    obj = ue.load_asset(p)
    assert obj, p
    return obj


def rows(obj, key):
    return [v.export_text() for v in obj.get_editor_property(key)]


def signature(data, inputs, context):
    return dict(pawn_data=port.scope['reflected_fields'](data)['values'],
        sets=[v.get_path_name() for v in data.get_editor_property('AbilitySets')],
        native=rows(inputs, 'NativeInputActions'), ability=rows(inputs, 'AbilityInputActions'),
        mappings=rows(context.get_editor_property('DefaultKeyMappings'), 'Mappings'))


def imported(sample, text):
    result = type(sample)()
    assert result.import_text(text)
    assert result.export_text() == text, 'Struct import changed values'
    return result


def validate(before, after, gamepad):
    assert after['native'] == before['native']
    assert after['ability'] == [v.replace(OLD, ACTION).replace('InputTag.Dodge', 'InputTag.Ability.Dash') for v in before['ability']]
    assert after['mappings'] == [v.replace(OLD, ACTION) for v in before['mappings']] + [gamepad]
    assert after['sets'] == before['sets'] + [SET + '.AS_DC_RifleDash']
    expected = dict(before['pawn_data'])
    expected['ability_sets'] = after['sets']
    assert after['pawn_data'] == expected, 'Unrelated PawnData changed'


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR9Integration=(apply|verify)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    port.helper.LOG = P / 'Saved/Logs' / ('R9Dash-' + run + '.log')
    out = R / ('integration_' + run)
    assert not out.exists()
    out.mkdir()
    baseline = json.loads((R / 'assets_Verify_A/report.json').read_text())
    assert baseline['complete'] and baseline['protected_unchanged']
    mutable = {str(p) for package in (DATA, INPUT, IMC) for p in port.files_helper.files(P, package)}
    protected = {p: port.helper.digest(Path(p)) for p in baseline['protected'] if p not in mutable}
    if mode == 'verify':
        for package in (DATA, INPUT, IMC, SET):
            for p in port.files_helper.files(P, package):
                protected[str(p)] = port.helper.digest(p)
    result = dict(mode=mode, complete=False, asset_saves=0, protected=protected)
    editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    try:
        port.helper.clean()
        data, inputs, context = [load(p) for p in (DATA, INPUT, IMC)]
        if mode == 'apply':
            assert all(not p.exists() for p in port.files_helper.files(P, SET))
            before = signature(data, inputs, context)
            result['before'] = before
            assert len(before['sets']) == 5 and any('AS_DC_RifleJumpAndDeath' in v for v in before['sets'])
            assert sum(OLD in v for v in before['ability']) == 1
            assert sum(OLD in v for v in before['mappings']) == 1
            assert any(OLD in v and 'Key=LeftShift' in v for v in before['mappings'])
            assert not any('Gamepad_LeftThumbstick' in v for v in before['mappings'])
            load(ACTION)
            assert ue.load_class(None, GA)
            cls = ue.load_class(None, '/Script/DreamCatcher.DCAbilitySet')
            factory = ue.DataAssetFactory()
            factory.set_editor_property('DataAssetClass', cls)
            aset = ue.AssetToolsHelpers.get_asset_tools().create_asset('AS_DC_RifleDash', ROOT.rstrip('/'), cls, factory)
            assert aset
            source = json.loads((R / 'source_B/report.json').read_text())
            grants = source['assets']['/ShooterCore/Game/AbilitySet_ShooterHero']['fields']['values']['granted_gameplay_abilities']
            original = [v for v in grants if '/ShooterCore/Game/Dash/GA_Hero_Dash.GA_Hero_Dash_C' in v]
            assert len(original) == 1
            entry_text = original[0].replace('/ShooterCore/Game/Dash/GA_Hero_Dash.GA_Hero_Dash_C', GA)
            sample = load(ROOT+'AS_DC_RifleJumpAndDeath').get_editor_property('GrantedGameplayAbilities')[0]
            aset.set_editor_property('GrantedGameplayAbilities', [imported(sample, entry_text)])
            assert not aset.get_editor_property('GrantedGameplayEffects') and not aset.get_editor_property('GrantedAttributes')
            actions = [imported(v, v.export_text().replace(OLD, ACTION).replace('InputTag.Dodge', 'InputTag.Ability.Dash'))
                       for v in inputs.get_editor_property('AbilityInputActions')]
            inputs.set_editor_property('AbilityInputActions', actions)
            mapping = context.get_editor_property('DefaultKeyMappings')
            entries = [imported(v, v.export_text().replace(OLD, ACTION)) for v in mapping.get_editor_property('Mappings')]
            trigger = ue.new_object(ue.InputTriggerPressed, outer=context, name='DashPressed')
            # Source IMC_Default InputTriggerPressed_0 has no serialized overrides;
            # engine CDO defaults are threshold 0.5 / no always-tick.
            assert trigger.get_editor_property('ActuationThreshold') == 0.5
            assert not trigger.get_editor_property('bShouldAlwaysTick')
            gamepad = '(Triggers=("/Script/EnhancedInput.InputTriggerPressed\'' + trigger.get_path_name() + '\'"),Action="/Script/EnhancedInput.InputAction\'' + ACTION + '\'",Key=Gamepad_LeftThumbstick)'
            gamepad_row = type(entries[0])()
            assert gamepad_row.import_text(gamepad)
            assert gamepad_row.get_editor_property('Action').get_path_name() == ACTION
            assert gamepad_row.get_editor_property('Key').export_text() == 'Gamepad_LeftThumbstick'
            assert list(gamepad_row.get_editor_property('Triggers')) == [trigger]
            assert not gamepad_row.get_editor_property('Modifiers')
            assert gamepad_row.get_editor_property('PlayerMappableKeySettings') is None
            # Unlike export_text(), the source asset's T3D omits default fields.
            gamepad = gamepad_row.export_text()
            entries.append(gamepad_row)
            mapping.set_editor_property('Mappings', entries)
            context.set_editor_property('DefaultKeyMappings', mapping)
            data.set_editor_property('AbilitySets', list(data.get_editor_property('AbilitySets')) + [aset])
            result['gamepad'] = gamepad
            result['grant'] = entry_text
            after = signature(data, inputs, context)
            validate(before, after, gamepad)
            assert rows(aset, 'GrantedGameplayAbilities') == [entry_text]
            result['after'] = after
            port.helper.clean()
            for obj in (aset, inputs, context, data):
                assert editor.save_loaded_asset(obj, False)
                result['asset_saves'] += 1
        else:
            applied = json.loads((R / 'integration_Apply_B/report.json').read_text())
            assert applied['complete']
            after = signature(data, inputs, context)
            validate(applied['before'], after, applied['gamepad'])
            assert after == applied['after']
            assert rows(load(SET), 'GrantedGameplayAbilities') == [applied['grant']]
            result['after'] = after
        port.helper.clean()
        result['complete'] = True
    finally:
        result['protected_unchanged'] = all(port.helper.digest(Path(p)) == h for p,h in protected.items())
        (out / 'report.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        assert result['protected_unchanged']
        print('R9_DASH_INTEGRATION ' + json.dumps({k: result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))


if __name__ == '__main__':
    main()
