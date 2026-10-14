"""Bounded original TeamSetup/Spawning/QuickBar asset bundle for the AI test map."""
import json,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r10_lyra_ai_assets as port
P,S,R=port.P,port.S,port.R
ROOTS=['/ShooterCore/Game/B_TeamSetup_TwoTeams','/ShooterCore/Game/B_TeamSpawningRules','/ShooterCore/Game/B_QuickBarComponent']


def main():
    cmd=ue.SystemLibrary.get_command_line()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    mode=re.search(r'-DCR10AITeams=(inspect|migrate|relocate|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR10AITeamsRun=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    if mode=='inspect':
        port.ROOTS=ROOTS
        port.inspect('Teams_'+run)
        return
    manifest=json.loads((R/'ai_manifest_Teams_A/report.json').read_text())
    mapping=manifest['mapping']
    assert manifest['complete'] and len(mapping)==5 and not manifest['different_existing']
    middle={p:port.migrated_path(p) for p in mapping}
    out=R/('ai_teams_'+run);out.mkdir(exist_ok=False)
    port.reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-Teams-'+run+'.log')
    protected=dict(manifest['protected'])
    protected.update(json.loads((R/'lyra_ai_assembly_postbuild_A_before.json').read_text()))
    result=dict(complete=False,mode=mode,mapping=mapping,intermediate=middle,asset_saves=0,protected=protected)
    try:
        project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
        assert project==(S if mode=='migrate' else P).resolve()
        registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
        assert all(port.digest(Path(p))==h for p,h in protected.items())
        editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        if mode=='migrate':
            assert all(not f.exists() for d in list(middle.values())+list(mapping.values()) for f in port.helpers.files(P,d))
            opts=ue.MigrationOptions();opts.set_editor_property('prompt',False)
            opts.set_editor_property('ignore_dependencies',True);opts.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping),str(P/'Content'),opts)
            assert all(port.helpers.files(P,d)[0].is_file() for d in middle.values())
            result['migrated_hashes']={str(f):port.digest(f) for d in middle.values() for f in port.helpers.files(P,d)}
        else:
            migrated=json.loads((R/'ai_teams_Migrate_A/report.json').read_text());assert migrated['complete']
            if mode=='relocate':
                assert all(port.digest(Path(p))==h for p,h in migrated['migrated_hashes'].items())
                assert all(not f.exists() for d in mapping.values() for f in port.helpers.files(P,d))
                paths=middle
            else:
                assert 'Redirects=' not in cmd
                previous=json.loads((R/'ai_teams_Relocate_A/report.json').read_text());assert previous['complete']
                protected.update(previous['saved_hashes']);paths=mapping
            objects={p:ue.load_asset(d) for p,d in paths.items()};assert all(objects.values())
            port.reader.helper.clean()
            if mode=='relocate':
                opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
                assert not any(str(r) not in set(middle.values()) for d in middle.values() for r in registry.get_referencers(d,opts)), 'External referencer'
                for p,obj in objects.items():
                    if isinstance(obj,ue.Blueprint):
                        default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(obj))
                        expected=manifest['assets'][p]['defaults'].get('replicate_using_registered_sub_object_list')
                        if expected is not None:default.set_editor_property('replicate_using_registered_sub_object_list',expected)
                assert ue.AssetToolsHelpers.get_asset_tools().rename_assets([ue.AssetRenameData(o,mapping[p].rsplit('/',1)[0],mapping[p].rsplit('/',1)[1]) for p,o in objects.items()])
                for obj in objects.values():
                    if isinstance(obj,ue.Blueprint):ue.BlueprintEditorLibrary.compile_blueprint(obj)
                    port.reader.helper.clean()
                    assert editor.save_loaded_asset(obj,False);result['asset_saves']+=1
                result['saved_hashes']={str(f):port.digest(f) for d in mapping.values() for f in port.helpers.files(P,d)}
            result['assets']={p:port.reader.data(o,out,str(i)) for i,(p,o) in enumerate(objects.items())}
            opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
            result['dependencies']={d:[str(r) for r in registry.get_dependencies(d,opts)] for d in mapping.values()}
            assert all(not r.startswith('/ShooterCore/') and r not in middle.values() for deps in result['dependencies'].values() for r in deps)
        port.reader.helper.clean();result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if port.digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_TEAMS '+json.dumps({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))


if __name__=='__main__':main()
