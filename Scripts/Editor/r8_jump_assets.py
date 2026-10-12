"""Exact four-asset Jump port. No C++ builds, global consolidation or shared asset saves.

Source: -DCR8JumpAssets=prepare|migrate -DCR8JumpRun=<fresh>
Target: -DCR8JumpAssets=target_prepare|verify -DCR8JumpRun=<fresh>
Target prepare uses process-only package aliases supplied by its launcher.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r7_root_ui_setup as helper
import r7_quickbar_setup as files_helper
import r7_quickbar_target as normalizer
P=Path(__file__).resolve().parents[2]
S=P.parent/'LyraStarterGame'
R=P/'Saved/Diagnostics/R8'
ROOT='/Game/LyraMigration/Jump/'
MAPPING={
 '/Game/Characters/Heroes/Abilities/GA_Hero_Jump':ROOT+'GA_Hero_Jump',
 '/Game/Characters/Heroes/Abilities/W_JumpTouchButton':ROOT+'W_JumpTouchButton',
 '/Game/UI/Hud/Art/MI_UI_TouchCard_Jump':ROOT+'MI_UI_TouchCard_Jump',
 '/Game/UI/Hud/Art/T_UI_Icon_MobileOnly_Jump':ROOT+'T_UI_Icon_MobileOnly_Jump'}
GA,WIDGET,MATERIAL,TEXTURE=MAPPING
reuse=json.loads((R/'jump_reuse_target_A/report.json').read_text())
REUSE=dict(reuse['mapping'])
REUSE['/Game/Input/Actions/IA_Jump']='/Game/LyraMigration/BaseInput/Actions/IA_Jump'
tree=ast.parse((P/'Scripts/Editor/r5_rifle_shared_audit.py').read_text(encoding='utf-8-sig'))
scope=dict(unreal=ue,Path=Path,hashlib=hashlib,re=re)
exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name in ('require','encode','reflected_fields','export_report')],type_ignores=[]),'existing_export','exec'),scope)

def load(path):
    obj=ue.load_asset(path);assert obj,path
    return obj

def cdo(bp):return ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))

def button(bp):
    rows=[obj for obj in ue.ObjectIterator(ue.Widget) if obj.get_path_name().startswith(bp.get_path_name()+':WidgetTree.') and obj.get_name()=='ActionTouchButton']
    assert len(rows)==1,'Expected exactly one designer ActionTouchButton'
    return rows[0]

def detail(objects):
    ga,widget,mat,tex=[objects[p] for p in MAPPING]
    b=button(widget)
    return dict(ga_policy={n:scope['encode'](cdo(ga).get_editor_property(n)) for n in (
        'ActivationPolicy','ActivationGroup','InstancingPolicy','NetExecutionPolicy','AbilityTags','WidgetExtensionPointTag','WidgetClass')},
        associated_action=b.get_editor_property('AssociatedAction').get_path_name(),brush=b.get_editor_property('ButtonBrush').export_text(),
        parent=mat.get_editor_property('Parent').get_path_name(),
        parameters={n:[v.export_text() for v in mat.get_editor_property(n)] for n in ('TextureParameterValues','ScalarParameterValues','VectorParameterValues')},
        texture=scope['reflected_fields'](tex)['values'])

def mapped(value,mapping):
    text=json.dumps(value,sort_keys=True)
    for a,b in sorted(mapping.items(),key=lambda x:-len(x[0])):text=text.replace(a,b)
    return json.loads(text)

def target_normalize(value):
    value=normalizer.normalize(value,dict(REUSE,**MAPPING))
    text=json.dumps(value,sort_keys=True)
    text=text.replace('Lyra Gameplay Ability Jump Object Reference','DCLyra Gameplay Ability Jump Object Reference')
    text=text.replace('Lyra Gameplay Ability Object Reference','DCGameplay Ability Object Reference')
    text=text.replace('<LyraAbilityActivationPolicy.','<DCAbilityActivationPolicy.').replace('<LyraAbilityActivationGroup.','<DCAbilityActivationGroup.')
    return json.loads(text)

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR8JumpAssets=(prepare|migrate|target_prepare|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR8JumpRun=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    source=mode in ('prepare','migrate')
    actual=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert actual==(S if source else P).resolve() and ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    helper.LOG=P/'Saved/Logs'/('R8Jump-'+run+'.log')
    out=R/('jump_assets_'+run);assert not out.exists();out.mkdir()
    prior=json.loads((R/'jump_build_source/report.json').read_text())
    protected={p:helper.digest(Path(p)) for p in prior['protected']}
    for p,h in prior['protected'].items():
        if Path(p).is_relative_to(S):assert protected[p]==h,'Original changed since review'
    for p in (P/'Content/DreamCatcher/GAS/Test/R5/Integration').rglob('*'):
        if p.is_file():protected[str(p)]=helper.digest(p)
    for dest in REUSE.values():
        for p in files_helper.files(P,dest):protected[str(p)]=helper.digest(p)
    if mode=='verify':
        for dest in MAPPING.values():
            for p in files_helper.files(P,dest):protected[str(p)]=helper.digest(p)
    result=dict(mode=mode,complete=False,asset_saves=0,protected=protected,mapping=MAPPING,reuse=REUSE)
    editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    try:
        helper.clean()
        if mode=='prepare':
            assert all(not p.exists() for dest in MAPPING.values() for root in (S,P) for p in files_helper.files(root,dest))
            original={p:load(p) for p in MAPPING}
            result['source_detail']=detail(original)
            before={p:helper.snapshot(original[p]) for p in (GA,WIDGET)}
            result['source_snapshots']=before
            copies={p:editor.duplicate_loaded_asset(obj,MAPPING[p]) for p,obj in original.items()}
            assert all(copies.values())
            mat=copies[MATERIAL]
            params=list(mat.get_editor_property('TextureParameterValues'))
            for v in params:
                texture=v.get_editor_property('ParameterValue')
                if texture==original[TEXTURE]:v.set_editor_property('ParameterValue',copies[TEXTURE])
            mat.set_editor_property('TextureParameterValues',params)
            b=button(copies[WIDGET]);brush=b.get_editor_property('ButtonBrush');brush.set_editor_property('ResourceObject',mat);b.set_editor_property('ButtonBrush',brush)
            assert ue.BlueprintEditorLibrary.compile_blueprint(copies[WIDGET])
            cdo(copies[GA]).set_editor_property('WidgetClass',ue.BlueprintEditorLibrary.generated_class(copies[WIDGET]))
            assert ue.BlueprintEditorLibrary.compile_blueprint(copies[GA])
            assert detail(copies)==mapped(result['source_detail'],MAPPING),'Copy property differences'
            for p in (GA,WIDGET):
                assert helper.snapshot(original[p])==before[p],'Original graph changed'
                assert helper.snapshot(copies[p])==mapped(before[p],MAPPING),'Copy graph/tree changed'
            helper.clean()
            for obj in copies.values():assert editor.save_loaded_asset(obj,False);result['asset_saves']+=1
            result['prepared_hashes']={str(p):helper.digest(p) for dest in MAPPING.values() for p in files_helper.files(S,dest)}
        elif mode=='migrate':
            baseline=json.loads((R/'jump_assets_Prepare_A/report.json').read_text());assert baseline['complete'] and baseline['asset_saves']==4
            assert all(helper.digest(Path(p))==h for p,h in baseline['prepared_hashes'].items())
            assert all(not p.exists() for dest in MAPPING.values() for p in files_helper.files(P,dest))
            objects={p:load(dest) for p,dest in MAPPING.items()}
            for p in (WIDGET,GA):assert ue.BlueprintEditorLibrary.compile_blueprint(objects[p])
            assert detail(objects)==mapped(baseline['source_detail'],MAPPING)
            helper.clean()
            assert ue.SystemLibrary.get_console_variable_int_value('AssetTools.UseNewPackageMigration')==0
            options=ue.MigrationOptions();options.set_editor_property('prompt',False);options.set_editor_property('ignore_dependencies',True);options.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(MAPPING.values()),str(P/'Content'),options)
            assert all(files_helper.files(P,d)[0].is_file() for d in MAPPING.values())
            result['migrated']=4
        else:
            baseline=json.loads((R/'jump_assets_Prepare_A/report.json').read_text());assert baseline['complete']
            if mode=='verify':assert 'PackageRedirects=' not in cmd
            objects={p:load(dest) for p,dest in MAPPING.items()}
            for p in (WIDGET,GA):assert ue.BlueprintEditorLibrary.compile_blueprint(objects[p])
            helper.clean()
            result['actual_detail']=detail(objects)
            result['expected_detail']=target_normalize(baseline['source_detail'])
            assert result['actual_detail']==result['expected_detail'],'Target property mismatch'
            result['snapshots']={p:helper.snapshot(objects[p]) for p in (GA,WIDGET)}
            for p in (GA,WIDGET):assert result['snapshots'][p]==target_normalize(baseline['source_snapshots'][p]),'Target graph/tree mismatch'
            if mode=='target_prepare':
                for obj in objects.values():assert editor.save_loaded_asset(obj,False);result['asset_saves']+=1
            result['parent']=ue.BlueprintEditorLibrary.get_blueprint_parent_class(objects[GA]).get_path_name()
            assert result['parent']=='/Script/DreamCatcher.DCLyraGameplayAbility_Jump'
        helper.clean()
        result['complete']=True
    finally:
        result['protected_unchanged']=all(helper.digest(Path(p))==h for p,h in protected.items())
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert result['protected_unchanged']
        print('R8_JUMP_ASSETS '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))

if __name__=='__main__':main()
