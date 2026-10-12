"""Explicitly approved Q press-once change, restricted to the private test IMC."""
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r9_signature_foundation as foundation
port = foundation.port
P,R,IMC = foundation.P,foundation.R,foundation.IMC
ACTION = '/Game/Input/Actions/IA_Ultimate.IA_Ultimate'


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR9SignatureInput=(apply|verify)(?:\s|$)',cmd).group(1)
    run = re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    port.helper.LOG = P/'Saved/Logs'/('R9Signature-'+run+'.log')
    out = R/('signature_'+run)
    assert not out.exists()
    out.mkdir()
    before_report = json.loads((R/'signature_Verify_A/report.json').read_text())
    assert before_report['complete']
    paths = set(before_report['protected'])
    paths.update(str(p) for p in port.files_helper.files(P,'/Game/Input/Actions/IA_Ultimate'))
    mutable = {str(p) for p in port.files_helper.files(P,IMC)} if mode == 'apply' else set()
    protected = {p:port.helper.digest(Path(p)) for p in paths if p not in mutable}
    result = dict(mode=mode,complete=False,asset_saves=[],protected=protected)
    try:
        context = ue.load_asset(IMC)
        assert context
        before = foundation.fields(context)
        result['before'] = before
        if mode == 'apply':
            assert before == before_report['after']['mappings']
            mappings = context.get_editor_property('DefaultKeyMappings')
            entries = list(mappings.get_editor_property('Mappings'))
            indices = [i for i,e in enumerate(entries) if e.get_editor_property('Action').get_path_name() == ACTION]
            assert len(indices) == 1
            index = indices[0]
            old = entries[index]
            assert old.get_editor_property('Key').export_text() == 'Q'
            assert not old.get_editor_property('Triggers')
            old_text = old.export_text()
            trigger = ue.new_object(ue.InputTriggerPressed,context,'SignaturePressed')
            assert trigger.get_editor_property('ActuationThreshold') == .5
            assert not trigger.get_editor_property('bShouldAlwaysTick')
            replacement = type(old)()
            assert replacement.import_text(old_text)
            replacement.set_editor_property('Triggers',[trigger])
            expected_old = type(old)()
            assert expected_old.import_text(replacement.export_text())
            expected_old.set_editor_property('Triggers',[])
            assert expected_old.export_text() == old_text
            entries[index] = replacement
            mappings.set_editor_property('Mappings',entries)
            context.set_editor_property('DefaultKeyMappings',mappings)
            result['index'] = index
            result['old_entry'] = old_text
            result['new_entry'] = replacement.export_text()
        else:
            applied = json.loads((R/'signature_InputApply_A/report.json').read_text())
            assert applied['complete']
            result.update({k:applied[k] for k in ('index','old_entry','new_entry')})
            before = applied['before']
        after = foundation.fields(context)
        result['after'] = after
        assert {k:v for k,v in after.items() if k != 'default_key_mappings'} == {k:v for k,v in before.items() if k != 'default_key_mappings'}
        mappings = context.get_editor_property('DefaultKeyMappings')
        comparison = type(mappings)()
        assert comparison.import_text(mappings.export_text())
        compared_entries = list(comparison.get_editor_property('Mappings'))
        restored = type(compared_entries[result['index']])()
        assert restored.import_text(result['old_entry'])
        compared_entries[result['index']] = restored
        comparison.set_editor_property('Mappings',compared_entries)
        assert comparison.export_text() == before['default_key_mappings'], 'Only the exact Q mapping may change'
        entries = list(context.get_editor_property('DefaultKeyMappings').get_editor_property('Mappings'))
        triggers = list(entries[result['index']].get_editor_property('Triggers'))
        assert len(triggers) == 1 and triggers[0].get_class() == ue.InputTriggerPressed.static_class()
        assert triggers[0].get_editor_property('ActuationThreshold') == .5
        assert not ue.load_asset('/Game/Input/Actions/IA_Ultimate').get_editor_property('Triggers'), 'Shared action changed'
        # Recheck new ability/effect policies after cold load without saving them.
        base,ge,ga,aset = [ue.load_asset(p) for p in foundation.NEW]
        assert all((base,ge,ga,aset))
        foundation.verify_policy(base,ga,ge,aset)
        assert foundation.fields(ue.load_asset(foundation.DATA)) == before_report['after']['data']
        port.helper.clean()
        if mode == 'apply':
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(context,False)
            result['asset_saves'] = [context.get_path_name()]
        result['complete'] = True
    finally:
        result['protected_changed'] = [p for p,h in protected.items() if port.helper.digest(Path(p)) != h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R9_SIGNATURE_INPUT '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_changed')}))


if __name__ == '__main__':
    main()
