"""R14 pre-merge Blueprint asset compile pass, explicitly without save/cook/build.

Uses UE BlueprintEditorLibrary's SkipSave compile operation. Existing maps and
legacy compatibility assets are retained. This is not packaged-build validation.
"""
import hashlib,json,re,traceback
from pathlib import Path
import unreal as ue
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R14'
cmd=ue.SystemLibrary.get_command_line();assert '-run=pythonscript' in cmd.lower()
run=re.search(r'-DCR14FinalBlueprints=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('final_blueprints_'+run+'.json');assert not out.exists()
log=P/'Saved/Logs'/('R14FinalBlueprints-'+run+'.log')
digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None
def assets_state():
 return {f.relative_to(P).as_posix():[f.stat().st_size,f.stat().st_mtime_ns] for root in ['Content','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
before=assets_state()
protected={str(f):digest(f) for root in ['Source','Config'] for f in (P/root).rglob('*') if f.is_file()}
for f in [P/'DreamCatcher.uproject',P/'.git/index',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']:protected[str(f)]=digest(f)
result=dict(complete=False,asset_saves=0,cpp_build=False,cook=False,package=False,PIE=False,
 explicit_scope='On-disk Blueprint/WidgetBlueprint/AnimBlueprint/ControlRigBlueprint assets under /Game; excludes map level-script and external plugin assets',
 compiled=[],failed=[],candidates=[])
try:
 assert ue.find_object(None,'/Script/DreamCatcher.DCGameplayAbility_Death') is None
 registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
 types={'/Script/Engine.Blueprint','/Script/UMGEditor.WidgetBlueprint','/Script/Engine.AnimBlueprint','/Script/ControlRigDeveloper.ControlRigBlueprint'}
 for a in registry.get_assets_by_path('/Game',True,True):
  path=str(a.asset_class_path.package_name)+'.'+str(a.asset_class_path.asset_name)
  if path in types and not a.is_redirector():result['candidates'].append(str(a.package_name))
 result['candidates']=sorted(set(result['candidates']));assert 0<len(result['candidates'])<=350
 for i,p in enumerate(result['candidates']):
  print('R14_FINAL_BP_BEGIN '+p)
  obj=ue.load_asset(p)
  if not isinstance(obj,ue.Blueprint):result['failed'].append(dict(asset=p,reason='Load did not return Blueprint'));continue
  if ue.BlueprintEditorLibrary.compile_blueprint(obj):result['compiled'].append(p)
  else:result['failed'].append(dict(asset=p,reason='CompileBlueprint returned false'))
  if i%25==0:
   out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
   print('R14_FINAL_BP_PROGRESS '+str(i+1)+'/'+str(len(result['candidates'])))
  if ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count():raise RuntimeError('Engine ensure; stop compile pass')
 lines=log.read_text(encoding='utf-8-sig',errors='replace').splitlines()
 result['error_lines']=[x for x in lines if re.search(r': Error:|Ensure condition failed|Fatal:',x)]
 result['warning_lines']=[x for x in lines if ': Warning:' in x]
 result['complete']=not result['failed'] and not result['error_lines'] and len(result['compiled'])==len(result['candidates'])
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 result['asset_stats_unchanged']=before==assets_state()
 result['protected_changes']=[p for p,h in protected.items() if digest(Path(p))!=h]
 result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
 result['complete']=result['complete'] and result['asset_stats_unchanged'] and not result['protected_changes'] and result['ensure_count']==0
 out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 print('R14_FINAL_BP_RESULT '+str({k:v for k,v in result.items() if k not in ['compiled','candidates','warning_lines','error_lines']}))
