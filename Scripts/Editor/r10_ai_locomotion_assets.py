"""Bounded R10 AI locomotion settings inspection/repair; Unreal asset APIs only."""
import hashlib,json,re,sys,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];S=P.parent/'LyraStarterGame'
sys.path.insert(0,str(P/'Scripts/Editor'))
from r10_enemy_setup import components
import r7_root_ui_setup as reader
ROOT='/Game/DreamCatcher/GAS/Test/R10/LyraAI'
PAWN=ROOT+'/BP_DC_LyraInventoryPawn'
PARENT='/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
def movement(bp):
    found=[c for c in components(bp) if isinstance(c,ue.CharacterMovementComponent)]
    assert len(found)==1 and found[0].get_path_name().startswith(PAWN+'.')
    return found[0]
def export_cdo(bp,path):
    task=ue.AssetExportTask();task.object=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    task.exporter=ue.ObjectExporterT3D();task.filename=str(path);task.automated=True;task.prompt=False
    assert ue.Exporter.run_asset_export_task(task)
    return path.read_text(encoding='utf-8-sig')
def without_nav_line(text):
    result=[];inside=False
    for line in text.splitlines():
        if re.match(r'\s*Begin Object Name="CharMoveComp" ',line):inside=True
        if not (inside and line.strip().startswith('NavMovementProperties=')):result.append(line)
        if re.match(r'\s*End Object$',line):inside=False
    return '\n'.join(result)
def inspect(bp):
    result=[]
    for obj in components(bp):
        names=()
        if isinstance(obj,ue.CharacterMovementComponent):names=('NavMovementProperties','bRequestedMoveUseAcceleration','MaxAcceleration','MaxWalkSpeed')
        elif isinstance(obj,ue.SkeletalMeshComponent):names=('AnimClass','AnimationMode','SkeletalMeshAsset','VisibilityBasedAnimTickOption','bEnableUpdateRateOptimizations')
        if not names:continue
        row=dict(path=obj.get_path_name(),cls=obj.get_class().get_path_name(),fields={})
        for n in names:
            try:
                v=obj.get_editor_property(n)
                row['fields'][n]=v.get_path_name() if isinstance(v,ue.Object) else v.export_text() if hasattr(v,'export_text') else v if isinstance(v,(bool,float,int,str)) or v is None else str(v)
            except Exception as e:row['fields'][n]={'unavailable':str(e)}
        result.append(row)
    return result
def main():
    cmd=ue.SystemLibrary.get_command_line()
    run=re.search(r'-DCR10LocoRun=([A-Za-z0-9_]+)',cmd).group(1)
    mode_match=re.search(r'-DCR10LocoAssets=(inspect|apply|verify)(?:\s|$)',cmd)
    mode=mode_match.group(1) if mode_match else 'inspect'
    project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert project in (P.resolve(),S.resolve())
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    source=project==S.resolve()
    assert mode=='inspect' or not source
    paths=['/Game/Characters/Heroes/B_Hero_Default','/ShooterCore/Game/B_Hero_ShooterMannequin'] if source else [PARENT,PAWN]
    out=P/'Saved/Diagnostics/R10'/('locomotion_'+run+'.json');assert not out.exists()
    files=[p for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI') for p in root.rglob('*') if p.is_file()]
    files+=[P/'.git/index',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    if source:
        files+=[S/'Content/Characters/Heroes/B_Hero_Default.uasset',S/'Plugins/GameFeatures/ShooterCore/Content/Game/B_Hero_ShooterMannequin.uasset']
    protected={str(p):digest(p) for p in files}
    pawn_file=P/'Content'/Path(PAWN[6:]+'.uasset')
    if mode=='apply':
        original=json.loads((P/'Saved/Diagnostics/R10/locomotion_Source_A.json').read_text())
        assert original['complete'] and not original['protected_changed']
        source_move=next(row for row in original['assets']['/ShooterCore/Game/B_Hero_ShooterMannequin'] if row['cls'].endswith('LyraCharacterMovementComponent'))
        assert 'bUseAccelerationForPaths=True' in source_move['fields']['NavMovementProperties']
        assert digest(pawn_file)==original['protected'][str(pawn_file)],'AI Pawn changed after diagnosis'
        protected.pop(str(pawn_file))
    reader.LOG=P/'Saved/Logs'/('R10Locomotion-'+run+'.log')
    result=dict(complete=False,mode=mode,asset_saves=0,source=source,assets={},protected=protected)
    try:
        for path in paths:
            bp=ue.load_asset(path);assert bp,path
            result['assets'][path]=inspect(bp)
        if mode in ('apply','verify'):
            bp=ue.load_asset(PAWN)
            component=movement(bp)
            nav=component.get_editor_property('NavMovementProperties').copy()
            result['before_nav']=nav.export_text()
            before_graph=reader.snapshot(bp)
            before_text=export_cdo(bp,out.with_name(out.stem+'_before_cdo.t3d'))
            if mode=='apply':
                assert not nav.get_editor_property('bUseAccelerationForPaths')
                nav.set_editor_property('bUseAccelerationForPaths',True)
                component.set_editor_property('NavMovementProperties',nav)
                assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            result['after_nav']=movement(bp).get_editor_property('NavMovementProperties').export_text()
            expected=result['before_nav'].replace('bUseAccelerationForPaths=False','bUseAccelerationForPaths=True')
            assert result['after_nav']==expected and 'bUseAccelerationForPaths=True' in result['after_nav']
            after_text=export_cdo(bp,out.with_name(out.stem+'_after_cdo.t3d'))
            assert without_nav_line(before_text)==without_nav_line(after_text),'Other CDO data changed'
            assert reader.snapshot(bp)==before_graph,'Blueprint graph changed'
            reader.clean()
            if mode=='apply':
                assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
                result['asset_saves']=1
            else:
                applied=json.loads((P/'Saved/Diagnostics/R10/locomotion_Apply_C.json').read_text())
                assert applied['complete'] and digest(pawn_file)==applied['pawn_after_hash']
                assert before_graph==applied['snapshot'] and result['after_nav']==applied['after_nav']
            result.update(snapshot=before_graph,other_cdo_fields_unchanged=True,pawn_after_hash=digest(pawn_file))
        reader.clean()
        result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        out.write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_LOCOMOTION_ASSETS '+json.dumps({k:result[k] for k in ('complete','asset_saves','source','protected_changed')}))
if __name__=='__main__':main()
