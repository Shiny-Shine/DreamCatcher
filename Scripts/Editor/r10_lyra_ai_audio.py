"""Finalize only the three newly migrated pickup MetaSound reference identities.

Uses UObject editor properties and the stock MetaSound editor registration path.
No binary file I/O, native changes, shared audio saves, or graph rewiring.
"""
import json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r10_lyra_ai_assets as port
P,R=port.P,port.R
NAMES=('sfx_BaseLayer_Interactable_Pad_nl_meta','sfx_Interactable_WeaponPad_Replenish_nl_metaPreset','sfx_Interactable_WeaponPad_Collect_nl_metaPreset')
ROOT=port.ROOT+'/Game/Audio/MetaSounds/'
OLD_RANDOM='765086AE405E982E1EC878BE7C5CD0C6'
NEW_RANDOM='F17B99BE4A92B851C06214BABB1984BE'
OLD_BASE='87E63ED54A1E59C8836FC6AD23EB9C26'


def document(obj):
    prop='RootMetaSoundDocument' if isinstance(obj,ue.MetaSoundPatch) else 'RootMetasoundDocument'
    return prop,obj.get_editor_property(prop)


def export(obj,path):
    task=ue.AssetExportTask()
    task.object=obj;task.exporter=ue.ObjectExporterT3D();task.filename=str(path)
    task.automated=True;task.prompt=False
    assert ue.Exporter.run_asset_export_task(task)
    return path.read_text(encoding='utf-8-sig')


def main():
    cmd=ue.SystemLibrary.get_command_line()
    assert '-DCR10AIAudioFinalize' in cmd and '-unattended' in cmd and '-RenderOffscreen' in cmd
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    run=re.search(r'-DCR10AIAudioRun=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    out=R/('ai_audio_'+run);out.mkdir(exist_ok=False)
    port.reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-audio-'+run+'.log')
    previous=json.loads((R/'ai_assets_Verify_B/report.json').read_text())
    assert previous['complete']
    mutable={str(f) for name in NAMES for f in port.helpers.files(P,ROOT+name)}
    protected={p:h for p,h in previous['protected'].items() if p not in mutable}
    assert all(port.digest(Path(p))==h for p,h in previous['protected'].items())
    result=dict(complete=False,asset_saves=0,protected=protected,before={},after={},reference_keys={})
    try:
        parent=ue.load_asset('/Game/Audio/MetaSounds/sfx_Random_nl_meta')
        assert parent and NEW_RANDOM in document(parent)[1].export_text()
        objects={n:ue.load_asset(ROOT+n) for n in NAMES}
        assert all(objects.values())
        base_text=document(objects[NAMES[0]])[1].export_text()
        base_id=re.search(r'Metadata=\(ClassName=\(Namespace="",Name="([^"]+)"',base_text).group(1)
        expected={}
        for name,obj in objects.items():
            prop,doc=document(obj)
            before=doc.export_text()
            result['before'][name]=before
            assert before.count(OLD_RANDOM)==1 and NEW_RANDOM not in before
            expected[name]=before.replace(OLD_RANDOM,NEW_RANDOM)
            assert doc.import_text(expected[name])
            obj.set_editor_property(prop,doc)
            assert document(obj)[1].export_text()==expected[name], 'Unexpected document change'
        # Like the approved R9 preset fix, stock editor registration refreshes the
        # serialized referenced-class cache; commandlet PreSave alone does not.
        editor=ue.get_editor_subsystem(ue.AssetEditorSubsystem)
        for name,obj in objects.items():
            assert editor.open_editor_for_assets([obj])
            after=document(obj)[1].export_text()
            result.setdefault('editor_open_documents',{})[name]=after
            if after!=expected[name]:
                # The preset editor adds trigger-member editor metadata. Restore
                # the exact reviewed document; retain only its refreshed class cache.
                prop,doc=document(obj)
                assert doc.import_text(expected[name])
                obj.set_editor_property(prop,doc)
            result['after'][name]=document(obj)[1].export_text()
            assert result['after'][name]==expected[name]
        port.reader.helper.clean()
        assets=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        for name,obj in objects.items():
            assert assets.save_loaded_asset(obj,False)
            result['asset_saves']+=1
            assert document(obj)[1].export_text()==expected[name], 'Save changed document'
            text=export(obj,out/(name+'.t3d'))
            keys=re.search(r'ReferencedAssetClassKeys=.*',text).group(0)
            result['reference_keys'][name]=keys
            assert NEW_RANDOM in keys and OLD_RANDOM not in keys and OLD_BASE not in keys
            if name==NAMES[2]:assert base_id in keys
            port.reader.helper.clean()
        result['updated_hashes']={str(f):port.digest(f) for name in NAMES for f in port.helpers.files(P,ROOT+name)}
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if port.digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_AUDIO '+json.dumps({k:result[k] for k in ('complete','asset_saves','protected_changed')}))


if __name__=='__main__':
    try:main()
    finally:ue.SystemLibrary.quit_editor()
