"""Approved, exact 62-package AI/pickup port via Unreal Migrate then RenameAssets.

Reuses existing graph/export helpers. No C++ build or global consolidation.
"""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
P = Path(__file__).resolve().parents[2]
S = P.parent / 'LyraStarterGame'
R = P / 'Saved/Diagnostics/R10'
sys.path.insert(0, str(P/'Scripts/Editor'))
import r9_dash_assets as reader
import r7_quickbar_setup as helpers

ROOT = '/Game/LyraMigration/AI'
CORE = '/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247'
ORIGINAL_CORE = {
    '/ShooterCore/Weapons/B_WeaponInstance_Base':'B_WeaponInstance_Base',
    '/Game/Weapons/B_Weapon':'B_Weapon',
    '/Game/Weapons/GA_Weapon_Fire':'GA_Weapon_Fire',
    '/Game/Weapons/GA_Weapon_ReloadMagazine':'GA_Weapon_ReloadMagazine',
    '/Game/Weapons/GA_Weapon_AutoReload':'GA_Weapon_AutoReload',
    '/Game/GameplayCueNotifies/GCN_Weapon_Impact':'GCN_Weapon_Impact',
    '/Game/Audio/Blueprints/WeaponAudioMacros':'WeaponAudioMacros',
    '/ShooterCore/System/Audio/WeaponAudioFunctions':'WeaponAudioFunctions',
}
ORIGINAL_CORE.update({'/ShooterCore/Weapons/Rifle/'+n:n for n in (
    'B_WeaponInstance_Rifle','B_Rifle','GA_Weapon_Fire_Rifle_Auto','GA_Weapon_Reload_Rifle',
    'W_Reticle_Rifle','W_AmmoCounter_Rifle','AbilitySet_ShooterRifle','WID_Rifle','ID_Rifle','GCN_Weapon_Rifle_Fire')})
REUSED_PORTS = {p:CORE+'/'+n for p,n in ORIGINAL_CORE.items()}
ROOTS = ['/ShooterCore/Bot/B_AI_Controller_LyraShooter', '/ShooterCore/Bot/B_ShooterBotSpawner',
         '/ShooterCore/Blueprint/B_WeaponSpawner','/ShooterCore/Blueprint/B_AbilitySpawner',
         '/ShooterCore/Weapons/Rifle/WeaponPickupData_Rifle','/ShooterCore/GameplayCues/GCN_InteractPickUp']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None


def file_states(project, package):
    return {str(p):digest(p) for p in helpers.files(project, package)}


def destination(package):
    assert package.startswith(('/Game/','/ShooterCore/')) and '..' not in package
    return ROOT+'/'+package[1:]


def inspect(run):
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==S.resolve()
    out=R/('ai_manifest_'+run)
    out.mkdir(exist_ok=False)
    registry=ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
    result=dict(complete=False,asset_saves=0,roots=ROOTS,dependencies={},mapping={},reuse={},
                different_existing={},external_mounts=[],protected={},assets={})
    pending=list(ROOTS)
    try:
        while pending:
            path=pending.pop()
            if path in result['dependencies'] or path in result['reuse'] or path in result['different_existing']:
                continue
            if path.startswith(('/Engine/','/Script/')):
                continue
            if not path.startswith(('/Game/','/ShooterCore/')):
                result['external_mounts'].append(path)
                continue
            if path in REUSED_PORTS:
                dest=REUSED_PORTS[path]
                assert helpers.files(P,dest)[0].is_file(), dest
                result['reuse'][path]=dict(target=dest,reason='Previously approved R5/R6 port; retain approved fixes')
                result['protected'].update(file_states(P,dest))
                continue
            source_files=file_states(S,path)
            assert source_files[str(helpers.files(S,path)[0])], path
            result['protected'].update(source_files)
            if path.startswith('/Game/') and helpers.files(P,path)[0].is_file():
                target_files=file_states(P,path)
                result['protected'].update(target_files)
                same=all(source_files[str(a)]==target_files[str(b)] for a,b in zip(helpers.files(S,path),helpers.files(P,path)))
                if same:
                    result['reuse'][path]=dict(target=path,reason='All package/sidecar bytes identical')
                    continue
                result['different_existing'][path]=path
                continue
            assert len(result['mapping'])<160, 'Unexpected closure expansion: review before proceeding'
            assert ue.EditorAssetLibrary.does_asset_exist(path), path
            deps=sorted(str(d) for d in registry.get_dependencies(path,opts))
            result['dependencies'][path]=deps
            result['mapping'][path]=destination(path)
            pending.extend(deps)
        assert len(set(result['mapping'].values()))==len(result['mapping'])
        for path in sorted(result['mapping']):
            obj=ue.load_asset(path)
            assert obj,path
            row=reader.data(obj,out,obj.get_name())
            task=ue.AssetExportTask()
            task.object=obj
            task.exporter=ue.ObjectExporterT3D()
            task.filename=str(out/(obj.get_name()+'.t3d'))
            task.automated=True
            task.prompt=False
            row['export_ok']=bool(ue.Exporter.run_asset_export_task(task))
            result['assets'][path]=row
        result['external_mounts']=sorted(set(result['external_mounts']))
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in result['protected'].items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_MANIFEST '+json.dumps(dict(complete=result['complete'],new=len(result['mapping']),
            reuse=len(result['reuse']),different_existing=list(result['different_existing']),
            external_mounts=result['external_mounts'],asset_saves=0,protected_changed=result['protected_changed'])))


def inspect_reuse(run):
    project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert project in (S.resolve(),P.resolve())
    side='source' if project==S.resolve() else 'target'
    out=R/('ai_reuse_'+side+'_'+run)
    out.mkdir(exist_ok=False)
    manifest=json.loads((R/'ai_manifest_A/report.json').read_text())
    paths=sorted(manifest['different_existing'])
    assert len(paths)==5, 'Review changed common assets before expanding reuse'
    result=dict(complete=False,assets={},protected={},asset_saves=0)
    try:
        for path in paths:
            result['protected'].update(file_states(project,path))
            obj=ue.load_asset(path)
            assert obj,path
            row=reader.data(obj,out,obj.get_name())
            if isinstance(obj,ue.Blueprint):
                default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(obj))
                row['effect']={n:reader.scope['encode'](default.get_editor_property(n)) for n in
                    ('DurationPolicy','Modifiers','Executions','GameplayCues','GEComponents')}
            if isinstance(obj,ue.Texture2D):
                task=ue.AssetExportTask()
                task.object=obj
                # This fixed candidate is HDR; let FindExporter validate source format.
                # Explicit PNG bypasses SupportsTexture and asserts in UE5.8.
                task.filename=str(out/(obj.get_name()+'.exr'))
                task.automated=True
                task.prompt=False
                assert ue.Exporter.run_asset_export_task(task)
                row['texture_export_sha256']=digest(Path(task.filename))
            result['assets'][path]=row
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in result['protected'].items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_REUSE '+json.dumps(dict(side=side,complete=result['complete'],assets=len(result['assets']),asset_saves=0,protected_changed=result['protected_changed'])))


def compare_copy(before,after,old,new):
    expected=reader.mapped(before,{old:new})
    def normalize(value):
        if isinstance(value,dict): return {k:normalize(v) for k,v in value.items()}
        if isinstance(value,list): return [normalize(v) for v in value]
        if isinstance(value,str):
            return re.sub(r'(ClassName=\(Namespace="",Name=")[0-9A-F]{32}("(?:,Variant="")?\),Version=\(Major=1,Minor=0\),Type=Graph)',r'\1ROOT_ID\2',value)
        return value
    return normalize(reader.comparable(expected))==normalize(reader.comparable(after))


def source_assets(mode,run):
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==S.resolve()
    manifest=json.loads((R/'ai_manifest_A/report.json').read_text())
    reuse=json.loads((R/'ai_reuse_comparison_B.json').read_text())
    assert manifest['complete'] and len(manifest['mapping'])==62
    assert all(r['fieldsSame'] and r['defaultsMatchMapped'] and r['effectMatchesMapped'] for r in reuse['rows'])
    mapping=manifest['mapping']
    assert all(d.startswith(ROOT+'/') for d in mapping.values())
    out=R/('ai_assets_'+run)
    out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-assets-'+run+'.log')
    protected=dict(manifest['protected'])
    # No existing target sources/config/gameplay assets may be changed by this source-side step.
    protected.update(json.loads((R/'lyra_ai_postbuild_C_before.json').read_text()))
    if mode!='prepare':
        prior=json.loads((R/'ai_assets_Prepare_A/report.json').read_text())
        assert prior['complete'] and prior['asset_saves']==62
        protected.update(prior['prepared_hashes'])
    assert all(digest(Path(p))==h for p,h in protected.items()), 'Reviewed files changed'
    editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    result=dict(mode=mode,complete=False,asset_saves=0,mapping=mapping,protected=protected)
    try:
        reader.helper.clean()
        if mode=='prepare':
            assert all(not f.exists() for d in mapping.values() for project in (S,P) for f in helpers.files(project,d)), 'New destinations only'
            originals={p:ue.load_asset(p) for p in mapping}
            assert all(originals.values())
            result['source_data']={p:reader.data(o,out,'source_'+str(i)) for i,(p,o) in enumerate(originals.items())}
            copies={p:editor.duplicate_loaded_asset(o,mapping[p]) for p,o in originals.items()}
            assert all(copies.values())
            result['copy_data']={p:reader.data(o,out,'copy_'+str(i)) for i,(p,o) in enumerate(copies.items())}
            result['differences']=[p for p in copies if not compare_copy(result['source_data'][p],result['copy_data'][p],p,mapping[p])]
            assert not result['differences'], 'Copy contract changed: '+repr(result['differences'])
            reader.helper.clean()
            # Record comparisons before the first save so a process interruption is inspectable.
            (out/'before_save.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
            for obj in copies.values():
                assert obj.get_path_name().split('.')[0] in mapping.values()
                assert editor.save_loaded_asset(obj,False)
                result['asset_saves']+=1
                reader.helper.clean()
            result['prepared_hashes']={str(f):digest(f) for d in mapping.values() for f in helpers.files(S,d)}
        else:
            objects={p:ue.load_asset(d) for p,d in mapping.items()}
            assert all(objects.values())
            result['actual_data']={p:reader.data(o,out,'reload_'+str(i)) for i,(p,o) in enumerate(objects.items())}
            result['differences']=[p for p in objects if not compare_copy(prior['copy_data'][p],result['actual_data'][p],mapping[p],mapping[p])]
            assert not result['differences'], 'Saved copy differs: '+repr(result['differences'])
            reader.helper.clean()
            if mode=='migrate':
                assert all(not f.exists() for d in mapping.values() for f in helpers.files(P,d))
                assert ue.SystemLibrary.get_console_variable_int_value('AssetTools.UseNewPackageMigration')==0
                opts=ue.MigrationOptions()
                opts.set_editor_property('prompt',False)
                opts.set_editor_property('ignore_dependencies',True)
                opts.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
                ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping.values()),str(P/'Content'),opts)
                assert all(helpers.files(P,d)[0].is_file() for d in mapping.values())
                result['migrated']=62
        reader.helper.clean()
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_ASSETS '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_changed')}))


def migrated_path(package):
    return '/Game/'+package.split('/',2)[2]


def migrate_originals(run):
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==S.resolve()
    manifest=json.loads((R/'ai_manifest_A/report.json').read_text())
    mapping=manifest['mapping']
    assert manifest['complete'] and len(mapping)==62
    middle={p:migrated_path(p) for p in mapping}
    assert len(set(middle.values()))==62
    assert all(not f.exists() for d in list(middle.values())+list(mapping.values()) for f in helpers.files(P,d))
    out=R/('ai_assets_'+run)
    out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-assets-'+run+'.log')
    protected=dict(manifest['protected'])
    protected.update(json.loads((R/'lyra_ai_postbuild_C_before.json').read_text()))
    assert all(digest(Path(p))==h for p,h in protected.items())
    result=dict(complete=False,mode='migrate_originals',mapping=mapping,intermediate=middle,protected=protected)
    try:
        registry=ue.AssetRegistryHelpers.get_asset_registry()
        registry.search_all_assets(True)
        # New migration reloads serialized packages in an isolated mount instead of
        # DuplicateObject. Legacy migration cannot map an absent ShooterCore mount.
        assert ue.SystemLibrary.get_console_variable_int_value('AssetTools.UseNewPackageMigration')==1
        reader.helper.clean()
        opts=ue.MigrationOptions()
        opts.set_editor_property('prompt',False)
        opts.set_editor_property('ignore_dependencies',True)
        opts.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
        ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping),str(P/'Content'),opts)
        result['missing']=[d for d in middle.values() if not helpers.files(P,d)[0].is_file()]
        result['migrated_hashes']={str(f):digest(f) for d in middle.values() for f in helpers.files(P,d)}
        assert not result['missing'], repr(result['missing'])
        reader.helper.clean()
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_MIGRATE '+json.dumps({k:result[k] for k in ('complete','protected_changed')}))


def target_assets(mode,run):
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    manifest=json.loads((R/'ai_manifest_A/report.json').read_text())
    prior=json.loads((R/'ai_assets_MigrateOriginal_A/report.json').read_text())
    assert prior['complete'] and len(prior['mapping'])==62
    mapping,middle=prior['mapping'],prior['intermediate']
    if mode=='target_verify':
        assert 'PackageRedirects=' not in ue.SystemLibrary.get_command_line()
        assert 'ClassRedirects=' not in ue.SystemLibrary.get_command_line()
        relocated=json.loads((R/'ai_assets_Relocate_A/report.json').read_text())
        assert relocated['complete']
        middle=mapping
    out=R/('ai_assets_'+run)
    out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-assets-'+run+'.log')
    protected=dict(prior['protected'])
    if mode=='target_inspect': protected.update(prior['migrated_hashes'])
    if mode=='target_verify':
        protected.update(relocated['relocated_hashes'])
        for name in ('ai_audio_Finalize_B','ai_metadata_Metadata_A'):
            receipt=R/name/'report.json'
            if receipt.is_file():
                updated=json.loads(receipt.read_text())
                if updated['complete']:
                    protected.update(updated['updated_hashes'])
                else:
                    assert updated['asset_saves']==0, 'Review partially saved follow-up'
    if mode=='target_relocate':
        assert all(digest(Path(p))==h for p,h in prior['migrated_hashes'].items()), 'Migrated files changed'
        assert all(not f.exists() for d in mapping.values() for f in helpers.files(P,d)), 'Final destinations must be unused'
    assert all(digest(Path(p))==h for p,h in protected.items())
    result=dict(complete=False,mode=mode,asset_saves=0,assets={},mapping=mapping,protected=protected)
    try:
        registry=ue.AssetRegistryHelpers.get_asset_registry()
        registry.search_all_assets(True)
        objects={p:ue.load_asset(d) for p,d in middle.items()}
        assert all(objects.values())
        result['blueprints_compiled']=0
        for i,(p,obj) in enumerate(objects.items()):
            if mode=='target_verify' and isinstance(obj,ue.Blueprint):
                ue.BlueprintEditorLibrary.compile_blueprint(obj)
                result['blueprints_compiled']+=1
            result['assets'][p]=reader.data(obj,out,str(i))
            task=ue.AssetExportTask()
            task.object=obj
            task.exporter=ue.ObjectExporterT3D()
            task.filename=str(out/(obj.get_name()+'.t3d'))
            task.automated=True
            task.prompt=False
            result['assets'][p]['export_ok']=bool(ue.Exporter.run_asset_export_task(task))
        reader.helper.clean()
        if mode=='target_relocate':
            opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
            permitted=set(middle.values())
            result['external_referencers']={d:[str(r) for r in registry.get_referencers(d,opts) if str(r) not in permitted]
                for d in middle.values()}
            assert not any(result['external_referencers'].values()), 'Would affect pre-existing assets'
            # Engine actor/component defaults depend on each project's replication
            # settings. Preserve the four original BP CDO values locally, not globally.
            result['restored_defaults']=[]
            for p,obj in objects.items():
                if isinstance(obj,ue.Blueprint):
                    default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(obj))
                    name='replicate_using_registered_sub_object_list'
                    expected=manifest['assets'][p].get('defaults',{}).get(name)
                    if expected is not None and default.get_editor_property(name)!=expected:
                        default.set_editor_property(name,expected)
                        result['restored_defaults'].append(p)
            request=[ue.AssetRenameData(obj,mapping[p].rsplit('/',1)[0],mapping[p].rsplit('/',1)[1]) for p,obj in objects.items()]
            result['rename_success']=bool(ue.AssetToolsHelpers.get_asset_tools().rename_assets(request))
            assert result['rename_success'], 'Unreal RenameAssets did not complete'
            reader.helper.clean()
            editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
            for p,obj in objects.items():
                assert obj.get_path_name().split('.')[0]==mapping[p]
                if isinstance(obj,ue.Blueprint): ue.BlueprintEditorLibrary.compile_blueprint(obj)
                reader.helper.clean()
                assert editor.save_loaded_asset(obj,False)
                result['asset_saves']+=1
            result['relocated_hashes']={str(f):digest(f) for d in mapping.values() for f in helpers.files(P,d)}
            result['after_assets']={p:reader.data(o,out,'after_'+str(i)) for i,(p,o) in enumerate(objects.items())}
            reader.helper.clean()
        if mode=='target_verify':
            opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
            old_paths=set(prior['intermediate'].values())|set(manifest['mapping'])
            result['stale_dependencies']={d:[str(r) for r in registry.get_dependencies(d,opts) if str(r) in old_paths or str(r).startswith('/ShooterCore/')]
                for d in mapping.values()}
            result['redirectors']=[]
            for d in prior['intermediate'].values():
                for ad in registry.get_assets_by_package_name(d):
                    assert ad.is_redirector(), 'Unexpected asset at intermediate path'
                    result['redirectors'].append(d)
            bt_text=(out/'BT_Lyra_Shooter_Bot.t3d').read_text(encoding='utf-8-sig')
            result['editor_stale_class_paths']=re.findall(r'GeneratedClassPackage="(/ShooterCore/[^"]+)"',bt_text)
            if (R/'ai_audio_Finalize_B/report.json').is_file():
                result['audio_reference_keys']={}
                for name in ('sfx_BaseLayer_Interactable_Pad_nl_meta','sfx_Interactable_WeaponPad_Replenish_nl_metaPreset','sfx_Interactable_WeaponPad_Collect_nl_metaPreset'):
                    text=(out/(name+'.t3d')).read_text(encoding='utf-8-sig')
                    keys=re.search(r'ReferencedAssetClassKeys=.*',text).group(0)
                    assert 'F17B99BE4A92B851C06214BABB1984BE' in keys
                    assert '765086AE405E982E1EC878BE7C5CD0C6' not in keys and '87E63ED54A1E59C8836FC6AD23EB9C26' not in keys
                    result['audio_reference_keys'][name]=keys
            assert not any(result['stale_dependencies'].values()), 'Stale package references'
            reader.helper.clean()
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_TARGET '+json.dumps({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))


def finalize_tree_metadata(run):
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    previous=json.loads((R/'ai_assets_Verify_B/report.json').read_text())
    audio=json.loads((R/'ai_audio_Finalize_B/report.json').read_text())
    assert previous['complete'] and audio['complete']
    package=ROOT+'/ShooterCore/Bot/BT/BT_Lyra_Shooter_Bot'
    state=dict(previous['protected'],**audio['updated_hashes'])
    assert all(digest(Path(p))==h for p,h in state.items())
    mutable={str(f) for f in helpers.files(P,package)}
    protected={p:h for p,h in state.items() if p not in mutable}
    out=R/('ai_metadata_'+run);out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R10LyraAI-assets-'+run+'.log')
    result=dict(complete=False,asset_saves=0,changes=[],protected=protected)
    try:
        obj=ue.load_asset(package);assert obj
        def export(suffix):
            task=ue.AssetExportTask();task.object=obj;task.exporter=ue.ObjectExporterT3D()
            task.filename=str(out/(suffix+'.t3d'));task.automated=True;task.prompt=False
            assert ue.Exporter.run_asset_export_task(task)
            return Path(task.filename).read_text(encoding='utf-8-sig')
        before=export('before')
        paths=re.findall(r'Begin Object Name="[^"]+" ExportPath="/Script/BehaviorTreeEditor\.BehaviorTreeGraphNode_(?:Decorator|Service)\'([^\']+)\'"',before)
        expected=before
        for path in paths:
            node=ue.find_object(None,path);assert node
            data=node.get_editor_property('ClassData')
            old=data.export_text()
            if 'GeneratedClassPackage="/ShooterCore/' not in old:continue
            instance=node.get_editor_property('NodeInstance');assert instance
            cls=instance.get_class().get_path_name()
            new_package,new_class=cls.split('.')
            assert new_package.startswith(ROOT+'/ShooterCore/Bot/')
            new=re.sub(r'GeneratedClassPackage="[^"]+"','GeneratedClassPackage="'+new_package+'"',old)
            new=re.sub(r'AssetName="[^"]+"','AssetName="'+new_package.rsplit('/',1)[1]+'"',new)
            new=re.sub(r'ClassName="[^"]+"','ClassName="'+new_class+'"',new)
            assert data.import_text(new)
            node.set_editor_property('ClassData',data)
            assert data.export_text()==new
            expected=expected.replace('ClassData='+old,'ClassData='+new)
            result['changes'].append(dict(node=path,before=old,after=new))
        assert len(result['changes'])==7
        after=export('after')
        assert after==expected, 'Changed more than editor class metadata'
        reader.helper.clean()
        assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(obj,False)
        result['asset_saves']=1
        reader.helper.clean()
        result['updated_hashes']={str(f):digest(f) for f in helpers.files(P,package)}
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_METADATA '+json.dumps({k:result[k] for k in ('complete','asset_saves','protected_changed')}))


if __name__=='__main__':
    cmd=ue.SystemLibrary.get_command_line()
    assert '-run=pythonscript' in cmd.lower() and ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    run=re.search(r'-DCR10AIAssetsRun=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    mode=re.search(r'-DCR10AIAssetsMode=(prepare|source_verify|migrate|migrate_originals|target_inspect|target_relocate|target_verify|tree_metadata)(?:\s|$)',cmd)
    if mode and mode.group(1)=='migrate_originals': migrate_originals(run)
    elif mode and mode.group(1)=='tree_metadata': finalize_tree_metadata(run)
    elif mode and mode.group(1).startswith('target_'): target_assets(mode.group(1),run)
    elif mode: source_assets(mode.group(1),run)
    elif '-DCR10AIReuse' in cmd: inspect_reuse(run)
    else: inspect(run)
