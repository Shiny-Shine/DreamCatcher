"""Approved six-property override on the integration Pawn only; never a C++ build.

Separate Python commandlet: -DCRifleCopyDiagnostics -DCR8Movement=apply|verify
-DCR8MovementRun=<fresh name>. Uses the reviewed R8 source/target read receipts.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r7_root_ui_setup as helper
import r5_rifle_player_setup as player

PROJECT = Path(__file__).resolve().parents[2]
REPORT = PROJECT / 'Saved/Diagnostics/R8'
PAWN = '/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn'
TARGETS = dict(MaxAcceleration=1200.0, BrakingDecelerationWalking=1400.0,
               BrakingFriction=3.0, BrakingFrictionFactor=1.0, AirControl=0.4, JumpZVelocity=500.0)
OLD = dict(MaxAcceleration=2048.0, BrakingDecelerationWalking=1800.0,
           BrakingFriction=0.0, BrakingFrictionFactor=2.0, AirControl=0.2, JumpZVelocity=420.0)

tree = ast.parse((PROJECT/'Scripts/Editor/r5_rifle_shared_audit.py').read_text(encoding='utf-8-sig'))
scope = dict(unreal=ue, Path=Path, hashlib=hashlib, re=re)
exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ('require','export_report')], type_ignores=[]), 'existing_export_helper', 'exec'), scope)

def values(bp):
    _, objects = player.components(bp, editable=True)
    matches = [obj for _,obj in objects if isinstance(obj, ue.CharacterMovementComponent)]
    assert len(matches)==1 and matches[0].get_path_name().startswith(PAWN+'.')
    obj=matches[0]
    return obj, {n:obj.get_editor_property(n) for n in TARGETS}

def expect(actual, expected):
    assert all(abs(actual[n]-v)<0.00001 for n,v in expected.items()), repr(actual)

def cdo_text(bp, path):
    cdo = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    assert scope['export_report'](cdo,path,ue.ObjectExporterT3D())['success']
    return path.read_text(encoding='utf-8-sig')

def without_six_values(text):
    # Only the reviewed CharMoveComp property lines may vary; all other CDO text stays exact.
    result = []
    inside = False
    for line in text.splitlines():
        if re.match(r'\s*Begin Object Name="CharMoveComp" ', line):inside=True
        if inside:
            match = re.match(r'\s*([A-Za-z0-9_]+)=([-0-9.]+)$',line)
            if match and match[1] in TARGETS:
                # Before editing these equal the native parent's defaults and
                # T3D omits them; after editing they are explicit overrides.
                # Values are checked independently by expect().
                continue
        if re.match(r'\s*End Object$', line):inside=False
        result.append(line)
    return '\n'.join(result)

def main():
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR8Movement=(apply|verify)(?:\s|$)',command).group(1)
    run = re.search(r'-DCR8MovementRun=([A-Za-z0-9_]{1,32})(?:\s|$)',command).group(1)
    assert '-run=pythonscript' in command.lower()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==PROJECT.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    helper.LOG = PROJECT/'Saved/Logs'/('R8Movement-'+run+'.log')
    out = REPORT/('movement_'+run)
    assert not out.exists();out.mkdir()
    source=json.loads((REPORT/'source_C/report.json').read_text())
    target=json.loads((REPORT/'target_C/report.json').read_text())
    original=next(c for c in source['assets']['B_Hero_ShooterMannequin']['components'] if c['name']=='CharMoveComp')
    expect(original['values'],TARGETS)
    pawn_file=helper.asset_files(PROJECT,PAWN)[0]
    paths=set(source['protected'])|set(target['protected'])
    paths.update(str(p) for p in (PROJECT/'Content/DreamCatcher/GAS/Test/R5/Integration').rglob('*') if p.is_file())
    paths.update(str(p) for p in (PROJECT/'Plugins/DCRifleMigrationTools/Source').rglob('*') if p.is_file())
    protected={p:helper.digest(Path(p)) for p in paths if Path(p)!=pawn_file}
    result=dict(mode=mode,asset_saves=0,protected=protected,pawn_before_hash=helper.digest(pawn_file),complete=False)
    try:
        if mode=='apply':
            assert helper.digest(pawn_file)==target['protected'][str(pawn_file)], 'Pawn changed since reviewed R8 read'
        for p,h in source['protected'].items():
            if 'LyraStarterGame' in p:assert helper.digest(Path(p))==h,'Original source asset changed'
        helper.clean()
        bp=ue.load_asset(PAWN);assert bp
        movement,before=values(bp)
        result['before_values']=before
        graphs=helper.snapshot(bp)
        before_text=cdo_text(bp,out/'before_cdo.t3d')
        if mode=='apply':
            expect(before,OLD)
            for name,value in TARGETS.items():movement.set_editor_property(name,value)
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            helper.clean()
            _,after=values(bp);expect(after,TARGETS)
            after_text=cdo_text(bp,out/'after_cdo.t3d')
            assert without_six_values(before_text)==without_six_values(after_text),'Unexpected CDO difference; not saved'
            assert helper.snapshot(bp)==graphs,'Unexpected graph change; not saved'
            assert all(helper.digest(Path(p))==h for p,h in protected.items())
            result.update(after_values=after,graphs_unchanged=True,other_cdo_fields_unchanged=True)
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
            result['asset_saves']=1
        else:
            baseline=json.loads((REPORT/'movement_Apply_B/report.json').read_text())
            assert baseline['complete'] and baseline['asset_saves']==1
            assert helper.digest(pawn_file)==baseline['pawn_after_hash'],'Saved Pawn changed'
            expect(before,TARGETS)
            expected=(REPORT/'movement_Apply_B/after_cdo.t3d').read_text(encoding='utf-8-sig')
            assert before_text==expected,'Reload CDO mismatch'
            assert graphs==baseline['snapshot'],'Reload graph mismatch'
            result.update(after_values=before,graphs_unchanged=True,other_cdo_fields_unchanged=True)
        helper.clean()
        result.update(complete=True,snapshot=helper.snapshot(bp),pawn_after_hash=helper.digest(pawn_file))
    finally:
        result['protected_unchanged']=all(helper.digest(Path(p))==h for p,h in protected.items())
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert result['protected_unchanged']
        print('DC_R8_MOVEMENT '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))

if __name__=='__main__':main()
