"""Exact approved R13 UI transfer, using stock Unreal Migrate/Rename APIs only.

No builds, original saves, broad dependency expansion, or binary file manipulation.
Run source migrate, target inspect/relocate/verify separately with fresh receipts.
"""
import json,re,sys,hashlib,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];S=P.parent/'LyraStarterGame';R=P/'Saved/Diagnostics/R13'
sys.path.insert(0,str(P/'Scripts/Editor'))
import r13_ui_assets as recipe
import r9_dash_assets as reader
helpers=recipe.helpers
digest=recipe.digest
PLAN=json.loads((R/'ui_assets_Prepare_D/report.json').read_text())
MAPPING,REUSE=PLAN['mapping'],PLAN['reuse']
assert PLAN['complete'] and len(MAPPING)==214 and len(REUSE)==49
INPUT={p:(PLAN['prepared_menu'] if p==recipe.MENU else p) for p in MAPPING}
MIDDLE={p:('/Game/'+q.split('/',2)[2]) for p,q in INPUT.items()}

def hashes(project,paths):return recipe.states(project,paths)
def unchanged(rows):return [f for f,h in rows.items() if digest(Path(f))!=h]

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR13Transfer=(migrate|inspect|relocate|verify)',cmd).group(1)
    run=re.search(r'-DCR13TransferRun=([A-Za-z0-9_]+)',cmd).group(1)
    assert '-run=pythonscript' in cmd.lower()
    project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert project==(S if mode=='migrate' else P).resolve()
    out=R/('transfer_'+run);out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R13Transfer-'+run+'.log')
    protected={f:h for f,h in PLAN['protected'].items() if Path(f).is_relative_to(S)}
    protected.update(PLAN['prepared_menu_hashes'])
    protected.update(json.loads((R/'postbuild_before_C.json').read_text())['files'])
    result=dict(complete=False,mode=mode,mapping=MAPPING,reuse=REUSE,intermediate=MIDDLE,
        protected=protected,asset_saves=0)
    try:
        assert not unchanged(protected),'Protected files changed'
        registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
        editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        if mode=='migrate':
            assert len(set(MIDDLE.values()))==214
            assert all(not f.exists() for d in set(MIDDLE.values())|set(MAPPING.values()) for f in helpers.files(P,d))
            assert ue.SystemLibrary.get_console_variable_int_value('AssetTools.UseNewPackageMigration')==1
            reader.helper.clean()
            options=ue.MigrationOptions();options.set_editor_property('prompt',False)
            options.set_editor_property('ignore_dependencies',True);options.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(INPUT.values()),str(P/'Content'),options)
            result['missing']=[d for d in MIDDLE.values() if not helpers.files(P,d)[0].is_file()]
            result['imported_hashes']=hashes(P,MIDDLE.values())
            assert not result['missing'],result['missing']
        else:
            imported=json.loads((R/'transfer_Migrate_A/report.json').read_text())
            # Migrate_A wrote the complete set but hit an in-memory Blueprint
            # search-index ensure in the source process. Preserve its failed
            # result and inspect the recorded files in a fresh target process.
            if not imported['complete']:
                assert imported.get('missing')==[] and not imported['protected_changed']
                assert 'Engine ensure; stop' in imported.get('failure','')
                if mode!='inspect':
                    checked=json.loads((R/'transfer_Inspect_C/report.json').read_text());assert checked['complete']
                    assert checked['deferred']==['/ShooterCore/Accolades/B_AccoladeRelay']
                    assert callable(getattr(ue.get_default_object(ue.load_class(None,'/Script/DreamCatcher.DCPlayerState')),'client_broadcast_message',None)), 'User RPC build required'
                result['incoming_source_migration_had_ensure']=True
            if mode in ('inspect','relocate'):
                assert not unchanged(imported['imported_hashes']),'Imported files changed'
                paths=MIDDLE
            else:
                assert 'PackageRedirects=' not in cmd,'Final verify cannot depend on temporary aliases'
                previous=json.loads((R/'relocation_saved_A.json').read_text());assert previous['source_run_failed']
                assert not unchanged(previous['output_hashes'])
                paths=MAPPING
            # Load the source menu from its prepared path, and every other object
            # using explicit command-line aliases for source mounts/reused assets.
            result['deferred']=['/ShooterCore/Accolades/B_AccoladeRelay'] if '-DCR13DeferAccoladeRelay' in cmd else []
            assert mode=='inspect' or not result['deferred']
            reused_objects={p:ue.load_asset(d) for p,d in REUSE.items()}
            assert all(reused_objects.values())
            objects={}
            for p,d in paths.items():
                if p in result['deferred']:continue
                objects[p]=ue.load_asset(d);assert objects[p],d
            result['blueprint_count']=sum(isinstance(o,ue.Blueprint) for o in objects.values())
            reader.helper.clean()
            if mode=='relocate':
                assert all(not f.exists() for p,d in MAPPING.items() if d!=MIDDLE[p] for f in helpers.files(P,d))
                opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
                permitted=set(MIDDLE.values())
                outside={d:[str(x) for x in registry.get_referencers(d,opts) if str(x) not in permitted] for d in permitted}
                result['external_referencers']={d:r for d,r in outside.items() if r}
                assert not result['external_referencers'],'Existing referencers would be modified'
                request=[ue.AssetRenameData(o,MAPPING[p].rsplit('/',1)[0],MAPPING[p].rsplit('/',1)[1]) for p,o in objects.items() if MAPPING[p]!=MIDDLE[p]]
                assert ue.AssetToolsHelpers.get_asset_tools().rename_assets(request)
                reader.helper.clean()
            result['assets']={}
            for i,(p,obj) in enumerate(objects.items()):
                if isinstance(obj,ue.Blueprint):
                    assert ue.BlueprintEditorLibrary.compile_blueprint(obj),p
                reader.helper.clean()
                if mode=='relocate':
                    assert obj.get_path_name().split('.')[0]==MAPPING[p]
                    assert editor.save_loaded_asset(obj,False);result['asset_saves']+=1
                if isinstance(obj,ue.Blueprint):result['assets'][p]=reader.data(obj,out,'BP'+str(i))
            if mode=='relocate':result['output_hashes']=hashes(P,MAPPING.values())
            result['dependencies']={d:[str(x) for x in registry.get_dependencies(d,ue.AssetRegistryDependencyOptions(True,True,False,False,False))] for d in MAPPING.values() if mode!='inspect'}
        reader.helper.clean();result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=unchanged(protected)
        (out/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R13_TRANSFER '+str({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))

if __name__=='__main__':main()
