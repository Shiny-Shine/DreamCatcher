"""Original CommonUI config-selected assets omitted by ordinary package closure.
Exact six UI roots; stock Unreal migration and new-only target rename, no builds.
"""
import json,re,sys,hashlib,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];S=P.parent/'LyraStarterGame';R=P/'Saved/Diagnostics/R13'
sys.path.insert(0,str(P/'Scripts/Editor'))
import r13_ui_assets as recipe
import r9_dash_assets as reader
ROOT='/Game/LyraMigration/UI/R13/Config'
ROOTS=['/Game/UI/B_CommonInputData','/Game/UI/Foundation/Materials/M_UI_Throbber_Base',
 '/Game/UI/Foundation/RichTextData/CommonUIRichTextData',
 '/Game/UI/Foundation/Platform/Input/KeyboardMouse/CommonInput_KeyboardMouse',
 '/Game/UI/Foundation/Platform/Input/GamepadXboxSeriesX/CommonInput_Gamepad_XSX',
 '/Game/UI/Foundation/Platform/Input/GamepadPS5/CommonInput_Gamepad_PS5']
def digest(f):return hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
def asset_data(obj,out,name):
 row=dict(cls=obj.get_class().get_path_name(),fields=reader.scope['reflected_fields'](obj)['values'])
 if isinstance(obj,ue.Blueprint):
  row['snapshot']=reader.helper.snapshot(obj)
  default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(obj))
  row['defaults']=reader.scope['reflected_fields'](default)['values']
  info=reader.scope['export_report'](default,out/(name+'_cdo.t3d'),ue.ObjectExporterT3D());assert info['success']
  raw=Path(info['path']).read_bytes();row['cdo_text']=raw.decode('utf-16' if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else 'utf-8-sig')
 return row
def main():
 cmd=ue.SystemLibrary.get_command_line();mode=re.search(r'-DCR13UIConfig=(inspect|migrate|relocate|verify)',cmd).group(1)
 run=re.search(r'-DCR13UIConfigRun=([A-Za-z0-9_]+)',cmd).group(1)
 assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==(S if mode in ('inspect','migrate') else P).resolve()
 out=R/('ui_config_'+run);out.mkdir(exist_ok=False)
 reader.helper.LOG=P/'Saved/Logs'/('R13UIConfig-'+run+'.log')
 protected={str(f):digest(f) for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Plugins/GameFeatures/DCStageUI'] for f in (P/root).rglob('*') if f.is_file()}
 result=dict(complete=False,mode=mode,asset_saves=0,protected=protected)
 try:
  registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
  opts=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
  if mode=='inspect':
   main_plan=json.loads((R/'ui_assets_Prepare_D/report.json').read_text(encoding='utf-8'))
   known=dict(recipe.known_reuse(),**main_plan['mapping']);known.update(main_plan['reuse'])
   mapping={};reuse={};seen=set();pending=list(ROOTS);result.update(mapping=mapping,reuse=reuse,assets={},existing=[])
   while pending:
    pkg=pending.pop()
    if pkg in seen or pkg.startswith(('/Engine/','/Script/','/CommonUI/')):continue
    seen.add(pkg);assert pkg.startswith('/Game/'),pkg
    if pkg in known:reuse[pkg]=known[pkg];continue
    assert len(mapping)<350,'Unexpected config UI closure'
    if recipe.helpers.files(P,pkg)[0].is_file():result['existing'].append(pkg);continue
    mapping[pkg]=ROOT+pkg;pending.extend(str(x) for x in registry.get_dependencies(pkg,opts))
   result['protected'].update(recipe.states(S,mapping))
   assert not result['existing'],'Compare colliding config assets before importing'
   for i,pkg in enumerate(mapping):
    obj=ue.load_asset(pkg);assert obj
    result['assets'][pkg]=asset_data(obj,out,'A'+str(i))
   assert all(not f.exists() for d in mapping.values() for f in recipe.helpers.files(P,d))
  else:
   plan=json.loads((R/'ui_config_Source_B/report.json').read_text(encoding='utf-8'));assert plan['complete']
   mapping,reuse=plan['mapping'],plan['reuse'];result.update(mapping=mapping,reuse=reuse)
   if mode=='migrate':
    assert all(not f.exists() for d in list(mapping)+list(mapping.values()) for f in recipe.helpers.files(P,d))
    result['protected'].update({f:h for f,h in plan['protected'].items() if Path(f).is_relative_to(S)})
    options=ue.MigrationOptions();options.set_editor_property('prompt',False);options.set_editor_property('ignore_dependencies',True);options.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
    ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(mapping),str(P/'Content'),options)
    assert all(recipe.helpers.files(P,p)[0].is_file() for p in mapping)
    result['output_hashes']=recipe.states(P,mapping)
   else:
    objects={p:ue.load_asset(p if mode=='relocate' else d) for p,d in mapping.items()};assert all(objects.values())
    reader.helper.clean()
    if mode=='relocate':
     assert all(not f.exists() for d in mapping.values() for f in recipe.helpers.files(P,d))
     allowed=set(mapping)
     outside={p:[str(x) for x in registry.get_referencers(p,opts) if str(x) not in allowed] for p in mapping}
     assert not any(outside.values()),outside
     assert ue.AssetToolsHelpers.get_asset_tools().rename_assets([ue.AssetRenameData(o,mapping[p].rsplit('/',1)[0],mapping[p].rsplit('/',1)[1]) for p,o in objects.items()])
    result['assets']={}
    for i,(p,obj) in enumerate(objects.items()):
     if isinstance(obj,ue.Blueprint):assert ue.BlueprintEditorLibrary.compile_blueprint(obj)
     reader.helper.clean()
     if mode=='relocate':
      assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(obj,False);result['asset_saves']+=1
     if isinstance(obj,ue.Blueprint):result['assets'][p]=asset_data(obj,out,'B'+str(i))
    result['output_hashes']=recipe.states(P,mapping.values())
  reader.helper.clean();result['complete']=True
 except Exception:result['failure']=traceback.format_exc();raise
 finally:
  result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
  (out/'report.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');assert not result['protected_changed']
  print('R13_UI_CONFIG '+str({k:result[k] for k in ['mode','complete','asset_saves','protected_changed']}))
if __name__=='__main__':main()
