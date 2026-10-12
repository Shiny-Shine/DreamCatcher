"""Approved single-preset repair through UE's MetaSound Builder; no shared saves."""
import json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r9_dash_assets as port
P,R=port.P,port.R
PRESET='/Game/LyraMigration/Dash/sfx_DashStereo_nl_meta_Preset'
PARENT='/Game/Audio/MetaSounds/sfx_RandomStereo_nl_meta'
OLD_ID='C7725AA14BADE5EB7F9F54894DB8BE03'
NEW_ID='5A9AD0844AAAEB75E9111BAC4F8BFFC3'

def ok(value):return str(value)=='<MetaSoundBuilderResult.SUCCEEDED: 0>'
def builder_for(obj):
    editor=ue.get_editor_subsystem(ue.load_class(None,'/Script/MetasoundEditor.MetaSoundEditorSubsystem'))
    builder,status=editor.find_or_begin_building(obj)
    assert builder and ok(status)
    return builder
def inputs(builder):
    names,status=builder.get_graph_input_names();assert ok(status)
    result={}
    for name in names:
        literal,status=builder.get_graph_input_default(name);assert ok(status)
        result[str(name)]=literal.export_text()
    return result
def snapshot(obj,builder):
    parent=builder.get_referenced_preset_asset()
    doc=obj.get_editor_property('RootMetasoundDocument').export_text()
    return dict(fields=port.scope['reflected_fields'](obj)['values'],inputs=inputs(builder),
        parent=parent.get_path_name() if parent else None,is_preset=builder.is_preset(),
        class_id=re.search(r'Metadata=\(ClassName=\(Namespace="",Name="([^"]+)"',doc).group(1))
def exported(obj,path):
    info=port.scope['export_report'](obj,path,ue.ObjectExporterT3D());assert info['success']
    return path.read_text(encoding='utf-8-sig')
def compare(before,after):
    assert before['inputs']==after['inputs'],'Preset input values changed'
    assert before['parent']==after['parent']==PARENT+'.sfx_RandomStereo_nl_meta'
    assert before['class_id']==after['class_id'] and after['is_preset']
    assert {k:v for k,v in before['fields'].items() if k!='root_metasound_document'}=={k:v for k,v in after['fields'].items() if k!='root_metasound_document'},'Sound settings changed'

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR9AudioFix=(apply|finish|editor_finish|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    if mode=='editor_finish':
        assert '-unattended' in cmd and '-RenderOffscreen' in cmd and '-DCR9AudioEditorFinalize' in cmd
        assert '-run=pythonscript' not in cmd.lower()
    else:assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    port.helper.LOG=P/'Saved/Logs'/('R9Dash-'+run+'.log')
    out=R/('audio_fix_'+run);assert not out.exists();out.mkdir()
    prior=json.loads((R/'idle_fix_IdleVerify_A/report.json').read_text());assert prior['complete']
    mutable={str(p) for p in port.files_helper.files(P,PRESET)} if mode in ('apply','finish','editor_finish') else set()
    protected={p:port.helper.digest(Path(p)) for p in prior['protected'] if p not in mutable}
    result=dict(complete=False,mode=mode,asset_saves=0,protected=protected)
    try:
        parent=ue.load_asset(PARENT);preset=ue.load_asset(PRESET);assert parent and preset
        parent_before=port.scope['reflected_fields'](parent)['values']
        builder=builder_for(preset)
        before=snapshot(preset,builder);result['before']=before
        result['before_export']=str(out/'before.t3d');text=exported(preset,out/'before.t3d')
        if mode in ('apply','finish','editor_finish'):
            if mode=='apply':
                inspected=json.loads((R/'AudioTarget_B/report.json').read_text());assert inspected['complete']
                assert all(port.helper.digest(Path(p))==h for p,h in inspected['protected'].items()),'Audio changed since review'
                assert OLD_ID in text and NEW_ID not in re.search(r'ReferencedAssetClassKeys=.*',text).group(0)
            else:
                partial_name='audio_fix_AudioFinish_A/report.json' if mode=='editor_finish' else 'audio_fix_AudioApply_A/report.json'
                partial=json.loads((R/partial_name).read_text())
                assert partial['asset_saves']==1 and partial['protected_unchanged']
                assert before==partial['after_save'],'Unexpected post-repair edit'
            # Preserve exactly the four original overridden inputs. OnPlay stays inherited.
            if mode=='editor_finish':
                # Stock MetaSound editor opening invokes its registration path,
                # whose default options rebuild the referenced asset class set.
                assets=ue.get_editor_subsystem(ue.AssetEditorSubsystem)
                assert assets.open_editor_for_assets([preset])
            else:
                assert ok(builder.convert_to_preset(parent))
                for name in ('Sounds','Min','Max','Gain'):
                    literal,status=builder.get_graph_input_default(name);assert ok(status)
                    assert literal.import_text(before['inputs'][name])
                    assert ok(builder.set_graph_input_default(name,literal))
            after=snapshot(preset,builder);result['after']=after
            compare(before,after)
            assert OLD_ID not in after['fields']['root_metasound_document']
            assert NEW_ID in after['fields']['root_metasound_document']
            assert port.scope['reflected_fields'](parent)['values']==parent_before,'Shared parent document changed'
            port.helper.clean()
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(preset,False)
            result['asset_saves']=1
            result['after_save']=snapshot(preset,builder)
            compare(before,result['after_save'])
            final_text=exported(preset,out/'after_save.t3d')
        else:
            applied=json.loads((R/'audio_fix_AudioEditorFinish_B/report.json').read_text());assert applied['complete']
            compare(applied['before'],before)
            assert before==applied['after_save'],'Reloaded preset document differs'
            final_text=text
        result['reference_keys']=re.search(r'ReferencedAssetClassKeys=.*',final_text).group(0).strip()
        assert NEW_ID in result['reference_keys'] and OLD_ID not in result['reference_keys'],'Serialized parent class cache not repaired'
        port.helper.clean();result['complete']=True
    finally:
        result['protected_unchanged']=all(port.helper.digest(Path(p))==h for p,h in protected.items())
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert result['protected_unchanged']
        print('R9_DASH_AUDIO_FIX '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))

if __name__=='__main__':
    try:main()
    finally:
        if '-DCR9AudioEditorFinalize' in ue.SystemLibrary.get_command_line():
            ue.SystemLibrary.quit_editor()
