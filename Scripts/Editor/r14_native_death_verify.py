"""After the user's R14-2B build: native registration and affected Blueprint load.
No game build, migration, asset save or PIE. Requires a fresh commandlet process.
"""
import hashlib,json,re,sys,traceback
from pathlib import Path
import unreal as ue
sys.dont_write_bytecode=True
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R14'
cmd=ue.SystemLibrary.get_command_line()
assert '-run=pythonscript' in cmd.lower()
assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P
run=re.search(r'-DCR14NativeDeath=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('native_death_verify_'+run+'.json');assert not out.exists()
log=P/'Saved/Logs'/('R14NativeDeath-'+run+'.log')
prior=json.loads((R/'native_death_removed_A.json').read_text(encoding='utf-8'))
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
dll=P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll'
assert digest(dll)!=prior['old_dll_sha256'],'User rebuild has not updated the DLL'
assert all(not (P/f).exists() for f in prior['deleted'])
assert all(digest(P/f)==h for f,h in prior['active_lyra_death_sha256'].items())
def asset_stats():
 return {f.relative_to(P).as_posix():[f.stat().st_size,f.stat().st_mtime_ns] for root in ['Content','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
before=asset_stats()
protected={str(f):digest(f) for root in ['Source','Config'] for f in (P/root).rglob('*') if f.is_file()}
result=dict(complete=False,asset_saves=0,PIE=False,dll_sha256=digest(dll),compiled=[])
try:
 assert ue.find_object(None,'/Script/DreamCatcher.DCGameplayAbility_Death') is None,'Removed native class still registered'
 active=ue.load_class(None,'/Script/DreamCatcher.DCLyraGameplayAbility_Death');assert active
 result['original_death_class']=active.get_path_name()
 paths=['/Game/Characters/Heroes/Abilities/GA_Hero_Death',
  '/Game/DreamCatcher/GAS/Test/R10/GA_DC_EnemyDeath',
  '/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn',
  '/Game/DreamCatcher/GAS/Test/R12/BP_DC_Boss']
 for p in paths:
  bp=ue.load_asset(p);assert bp and ue.BlueprintEditorLibrary.compile_blueprint(bp),p
  if p==paths[0]:assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp)==active
  result['compiled'].append(p)
 result['ensure_count']=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()
 assert result['ensure_count']==0
 errors=[x for x in log.read_text(encoding='utf-8-sig',errors='replace').splitlines() if re.search(r': Error:|Ensure condition failed|Fatal:',x)]
 assert not errors,errors[:3]
 result['complete']=True
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 result['asset_stats_unchanged']=before==asset_stats()
 result['protected_changes']=[f for f,h in protected.items() if digest(Path(f))!=h]
 result['complete']=result['complete'] and result['asset_stats_unchanged'] and not result['protected_changes']
 out.write_text(json.dumps(result,indent=2),encoding='utf-8')
 print('R14_NATIVE_DEATH '+str(result))
