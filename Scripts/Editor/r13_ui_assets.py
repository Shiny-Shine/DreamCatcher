"""R13 HUD/settings asset recipe. Unreal APIs only; existing UI stays protected.

inspect-source and inspect-reuse are read-only. No C++ build or user settings save.
"""
import json, re, sys, hashlib
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
P = Path(__file__).resolve().parents[2]
S = P.parent / 'LyraStarterGame'
R = P / 'Saved/Diagnostics/R13'
sys.path.insert(0, str(P/'Scripts/Editor'))
import r9_dash_assets as reader
import r7_quickbar_setup as helpers

ROOT = '/Game/LyraMigration/UI/R13'
MENU = '/Game/UI/Hud/W_LyraGameMenu'
FRONTEND = '/Game/System/FrontEnd/Maps/L_LyraFrontEnd'
ROOTS = [
    '/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD',
    '/ShooterCore/UserInterface/W_ShooterHUDLayout', MENU,
    '/Game/UI/Foundation/Dialogs/W_ControllerDisconnected',
    '/Game/UI/Foundation/Dialogs/W_ConfirmationDefault',
    '/Game/UI/Foundation/Dialogs/W_ConfirmationError',
]

def digest(f):
    return hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None

def states(project, paths):
    return {str(f):digest(f) for pkg in paths for f in helpers.files(project,pkg)}

def known_reuse():
    reuse = dict(json.loads((P/'Saved/Diagnostics/R7QuickBar/prepare_A.json').read_text())['mapping'])
    reuse.update(json.loads((R/'asset_plan_A.json').read_text())['existing_assets_to_reuse'])
    for path in (P/'Content/LyraMigration/BaseInput/Actions').glob('*.uasset'):
        reuse['/Game/Input/Actions/'+path.stem] = '/Game/LyraMigration/BaseInput/Actions/'+path.stem
    reuse['/ShooterCore/Input/Actions/IA_ADS'] = '/Game/LyraMigration/ADS/Actions/IA_ADS'
    return reuse

def inspect_source(out, result):
    registry=ue.AssetRegistryHelpers.get_asset_registry(); registry.search_all_assets(True)
    opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
    known=known_reuse(); pending=list(ROOTS); seen=set()
    result.update(mapping={},reuse={},compare_existing=[],dependencies={},assets={})
    while pending:
        pkg=pending.pop()
        if pkg in seen or pkg.startswith(('/Script/','/Engine/')): continue
        if pkg=='/CommonUI/GenericInputActionDataTable':
            assert ue.EditorAssetLibrary.does_asset_exist(pkg)
            result.setdefault('engine_assets',[]).append(pkg); continue
        assert pkg.startswith(('/Game/','/ShooterCore/')),pkg
        seen.add(pkg)
        if pkg==FRONTEND: continue # Approved copied menu return adapter, not original lobby.
        assert len(seen)<400,'Unexpected R13 dependency expansion'
        if pkg in known:
            result['reuse'][pkg]=known[pkg]; continue
        if pkg.startswith('/Game/') and helpers.files(P,pkg)[0].is_file():
            result['reuse'][pkg]=pkg; result['compare_existing'].append(pkg); continue
        result['mapping'][pkg]=ROOT+pkg
        deps=sorted(str(x) for x in registry.get_dependencies(pkg,opts))
        result['dependencies'][pkg]=deps; pending.extend(deps)
    result['protected'].update(states(S,list(result['mapping'])+result['compare_existing']))
    result['protected'].update(states(P,result['reuse'].values()))
    for i,pkg in enumerate(sorted(result['mapping'])+sorted(result['compare_existing'])):
        obj=ue.load_asset(pkg); assert obj,pkg
        row=reader.data(obj,out,'A'+str(i))
        task=ue.AssetExportTask();task.object=obj;task.exporter=ue.ObjectExporterT3D()
        task.filename=str(out/('A'+str(i)+'.t3d'));task.automated=True;task.prompt=False
        row['export_ok']=bool(ue.Exporter.run_asset_export_task(task))
        row['export_file']=task.filename
        result['assets'][pkg]=row
    assert len(set(result['mapping'].values()))==len(result['mapping'])
    assert all(not f.exists() for pkg in result['mapping'].values() for f in helpers.files(P,pkg))
    reader.helper.clean()

def inspect_reuse(out,result,extra=False):
    prior=json.loads((R/'ui_assets_Source_B/report.json').read_text());assert prior['complete']
    result.update(assets={},reuse=prior['reuse'])
    paths=['/Game/Audio/Classes/SFX'] if extra else prior['compare_existing']
    result['protected'].update(states(P,paths))
    for i,pkg in enumerate(sorted(paths)):
        obj=ue.load_asset(pkg);assert obj,pkg
        row=reader.data(obj,out,'R'+str(i))
        task=ue.AssetExportTask();task.object=obj;task.exporter=ue.ObjectExporterT3D()
        task.filename=str(out/('R'+str(i)+'.t3d'));task.automated=True;task.prompt=False
        row['export_ok']=bool(ue.Exporter.run_asset_export_task(task));row['export_file']=task.filename
        result['assets'][pkg]=row
    reader.helper.clean()

def inspect_extras(out,result):
    paths=['/ShooterCore/Accolades/'+x for x in ('B_EliminationFeedRelay','B_ElimChainProcessor',
        'B_ElimStreakProcessor','EAS_BasicShooterAcolades')]
    result['protected'].update(states(S,paths)); result['assets']={}
    registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
    opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
    result['dependencies']={p:[str(x) for x in registry.get_dependencies(p,opts)] for p in paths}
    for i,pkg in enumerate(paths):
        obj=ue.load_asset(pkg);assert obj,pkg
        row=reader.data(obj,out,'Extra'+str(i))
        task=ue.AssetExportTask();task.object=obj;task.exporter=ue.ObjectExporterT3D()
        task.filename=str(out/('Extra'+str(i)+'.t3d'));task.automated=True;task.prompt=False
        row['export_ok']=bool(ue.Exporter.run_asset_export_task(task));row['export_file']=task.filename
        result['assets'][pkg]=row
    reader.helper.clean()

def prepare_source(out,result):
    """Finish the exact R13 import plan and prepare only the new menu copy."""
    prior=json.loads((R/'ui_assets_Source_B/report.json').read_text());assert prior['complete']
    mapping=dict(prior['mapping']);reuse=dict(prior['reuse']);deps=dict(prior['dependencies'])
    result['protected'].update(prior['protected'])
    # Replace prior target source/config hashes only with the checked current receipt.
    current=json.loads((R/'notification_before_assets_B.json').read_text())['files']
    result['protected'].update({str(P/f):h for f,h in current.items()})
    registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
    opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
    known=known_reuse();pending=['/ShooterCore/Accolades/'+n for n in (
        'AccoladeDataRegistry','DT_BasicShooterAccolades','B_AccoladeRelay','B_EliminationFeedRelay',
        'B_ElimChainProcessor','B_ElimStreakProcessor','EAS_BasicShooterAcolades')]
    added=[];new_shared=[]
    while pending:
        pkg=pending.pop()
        if pkg in mapping or pkg in reuse or pkg.startswith(('/Engine/','/Script/')):continue
        assert pkg.startswith(('/Game/','/ShooterCore/')),pkg
        if pkg in known:reuse[pkg]=known[pkg];continue
        if pkg.startswith('/Game/') and helpers.files(P,pkg)[0].is_file():
            reuse[pkg]=pkg;new_shared.append(pkg);continue
        assert len(mapping)<260,'R13 dependency bound exceeded'
        mapping[pkg]=ROOT+pkg;added.append(pkg)
        deps[pkg]=sorted(str(x) for x in registry.get_dependencies(pkg,opts));pending.extend(deps[pkg])
    result.update(mapping=mapping,reuse=reuse,dependencies=deps,added=added,new_shared=new_shared)
    result['protected'].update(states(S,added+new_shared))
    result['protected'].update(states(P,reuse.values()))
    assert new_shared==['/Game/Audio/Classes/SFX'],'Review supplemental shared dependencies'
    shared_report=json.loads((R/'ui_assets_ExtraReuse_A/report.json').read_text());assert shared_report['complete']
    for pkg in new_shared:
        obj=ue.load_asset(pkg);assert obj
        shared=reader.data(obj,out,'SharedSFX')
        other=shared_report['assets'][pkg]
        assert shared['fields']==other['fields'] and shared['cls']==other['cls'],'Shared SFX properties differ'
        task=ue.AssetExportTask();task.object=obj;task.exporter=ue.ObjectExporterT3D()
        task.filename=str(out/'SharedSFX.t3d');task.automated=True;task.prompt=False
        assert ue.Exporter.run_asset_export_task(task)
        assert Path(task.filename).read_text(encoding='utf-8-sig')==Path(other['export_file']).read_text(encoding='utf-8-sig')
    result['supplemental_shared_match']=new_shared
    assert all(not f.exists() for dest in mapping.values() for f in helpers.files(P,dest))
    supplemental=json.loads((R/'ui_assets_Prepare_B/report.json').read_text())
    assert supplemental['asset_saves']==0 and not supplemental['protected_changed']
    assert supplemental['added']==added and set(supplemental['supplemental_assets'])==set(added)
    assert all(digest(Path(f))==h for f,h in supplemental['protected'].items())
    result['supplemental_assets']=supplemental['supplemental_assets']
    result['supplemental_read_reused_from']='ui_assets_Prepare_B/report.json'
    reader.helper.clean()
    assert all(digest(Path(f))==h for f,h in result['protected'].items())
    editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    dest=mapping[MENU]
    assert all(not f.exists() for f in helpers.files(S,dest))
    source=ue.load_asset(MENU);ue.collect_garbage();before=reader.helper.snapshot(source)
    copy=editor.duplicate_loaded_asset(source,dest);assert copy
    cdo=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(copy))
    reference=cdo.get_editor_property('FrontEndMapReference')
    observed=reference.get_path_name() if isinstance(reference,ue.Object) else str(reference)
    assert FRONTEND+'.L_LyraFrontEnd' in observed,observed
    cdo.set_editor_property('FrontEndMapReference',None)
    cdo.set_editor_property('FrontEndExperienceReference',ue.PrimaryAssetId())
    assert ue.BlueprintEditorLibrary.compile_blueprint(copy)
    ue.collect_garbage()
    assert reader.helper.snapshot(source)==before
    expected=reader.mapped(before,{MENU:dest});actual=reader.helper.snapshot(copy)
    # UE assigns package-local text identities while duplicating. Only these two
    # observed confirmation literals may differ, and their source text must match.
    result['copied_text_identities']=[]
    expected_node=next(n for g in expected['graphs'] if g['name']=='EventGraph' for n in g['nodes'] if n['name']=='K2Node_AsyncAction_3')
    actual_node=next(n for g in actual['graphs'] if g['name']=='EventGraph' for n in g['nodes'] if n['name']=='K2Node_AsyncAction_3')
    for name,label in [('Title','Exit Game'),('Message','Are you sure?')]:
        left=next(p for p in expected_node['pins'] if p['name']==name)
        right=next(p for p in actual_node['pins'] if p['name']==name)
        pattern=r'NSLOCTEXT\("\[[A-F0-9]{32}\]", "[A-F0-9]{32}", "'+re.escape(label)+r'"\)'
        assert re.fullmatch(pattern,left['value']) and re.fullmatch(pattern,right['value'])
        result['copied_text_identities'].append(dict(pin=name,original=left['value'],copy=right['value']))
        left['value']=right['value']
    if actual!=expected:
        (out/'menu_difference.json').write_text(json.dumps(dict(expected=expected,actual=actual),ensure_ascii=False,indent=2),encoding='utf-8')
    assert actual==expected,'Menu graph/tree copy differs; inspect menu_difference.json before saving'
    reader.helper.clean()
    assert editor.save_loaded_asset(copy,False);result['asset_saves']=1
    result['prepared_menu']=dest
    result['prepared_menu_hashes']=states(S,[dest])
    result['menu_remaining_work']='Target DLL: replace confirmed OpenLevel execution with ReturnFromStageMenu; original source graph kept'

def verify_menu(out,result):
    prepared=json.loads((R/'ui_assets_Prepare_D/report.json').read_text());assert prepared['complete']
    result['protected'].update(prepared['protected'])
    result['protected'].update(prepared['prepared_menu_hashes'])
    assert all(digest(Path(f))==h for f,h in result['protected'].items())
    dest=prepared['prepared_menu'];bp=ue.load_asset(dest);assert bp
    cdo=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    assert cdo.get_editor_property('FrontEndMapReference') is None
    assert cdo.get_editor_property('FrontEndExperienceReference')==ue.PrimaryAssetId()
    registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
    deps=[str(x) for x in registry.get_dependencies(dest,ue.AssetRegistryDependencyOptions(True,True,False,False,False))]
    assert not any('/System/FrontEnd/' in x for x in deps),deps
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    reader.helper.clean()
    result.update(prepared_menu=dest,dependencies=deps,blueprint_compile=True,target_menu_connection_complete=False)

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR13UIAssets=([a-z-]+)',cmd).group(1)
    run=re.search(r'-DCR13UIRun=([A-Za-z0-9_]+)',cmd).group(1)
    assert '-run=pythonscript' in cmd.lower()
    project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert project==(S if mode in ('inspect-source','inspect-extras','prepare-source','verify-menu') else P).resolve()
    out=R/('ui_assets_'+run);out.mkdir(exist_ok=False)
    reader.helper.LOG=P/'Saved/Logs'/('R13UIAssets-'+run+'.log')
    baseline_name='notification_before_assets_B.json' if mode in ('prepare-source','inspect-extra-reuse','verify-menu') else 'postbuild_before_A.json'
    baseline=json.loads((R/baseline_name).read_text())['files']
    result=dict(complete=False,mode=mode,asset_saves=0,protected={str(P/f):h for f,h in baseline.items()})
    try:
        assert all(digest(Path(f))==h for f,h in result['protected'].items()),'Protected project changed before run'
        if mode=='inspect-source': inspect_source(out,result)
        elif mode=='inspect-reuse': inspect_reuse(out,result)
        elif mode=='inspect-extra-reuse': inspect_reuse(out,result,extra=True)
        elif mode=='inspect-extras': inspect_extras(out,result)
        elif mode=='prepare-source': prepare_source(out,result)
        elif mode=='verify-menu': verify_menu(out,result)
        else: raise RuntimeError('Unsupported R13 mode')
        result['complete']=True
    finally:
        result['protected_changed']=[f for f,h in result['protected'].items() if digest(Path(f))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
        assert not result['protected_changed']
        print('R13_UI_ASSETS '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_changed')}))

if __name__=='__main__': main()
