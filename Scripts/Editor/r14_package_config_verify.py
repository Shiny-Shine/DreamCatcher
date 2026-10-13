"""Read effective package entry/cook defaults without cooking or saving assets."""
import hashlib,json,re,traceback
from pathlib import Path
import unreal as ue
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R14'
cmd=ue.SystemLibrary.get_command_line();assert '-run=pythonscript' in cmd.lower()
run=re.search(r'-DCR14PackageConfig=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('package_config_'+run+'.json');assert not out.exists()
log=P/'Saved/Logs'/('R14PackageConfig-'+run+'.log')
baseline=json.loads((R/'package_prepare_before_A.json').read_text(encoding='utf-8'))
START='/Game/DreamCatcher/GAS/Test/R13/L_DC_Level1UI'
RETURN='/Game/DreamCatcher/GAS/Test/R10/LyraEncounter/L_DC_LyraFiniteEncounter'
MODE='/Game/DreamCatcher/GAS/Test/R13/BP_DC_StageUIGameMode'
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
result=dict(complete=False,cook_executed=False,package_executed=False,asset_saves=0,PIE=False)
def fields(obj,names):
 def encode(v):
  if isinstance(v,(str,bool,int,float)) or v is None:return v
  if isinstance(v,ue.Object):return v.get_path_name()
  if isinstance(v,ue.Array):return [encode(x) for x in v]
  return v.export_text() if hasattr(v,'export_text') else str(v)
 return {n:encode(obj.get_editor_property(n)) for n in names}
try:
 maps=ue.get_default_object(ue.load_class(None,'/Script/EngineSettings.GameMapsSettings'))
 result['maps_settings']=fields(maps,['GameDefaultMap','EditorStartupMap','GlobalDefaultGameMode','GameInstanceClass'])
 assert START+'.L_DC_Level1UI' in result['maps_settings']['GameDefaultMap']
 assert '/Game/DreamCatcher/DreamCatcher_ENV/Map/Main/Level_1.Level_1' in result['maps_settings']['EditorStartupMap']
 assert '/Game/Blueprint/BP_DreamCatcherGameMode.BP_DreamCatcherGameMode_C' in result['maps_settings']['GlobalDefaultGameMode']
 packaging=ue.get_default_object(ue.load_class(None,'/Script/DeveloperToolSettings.ProjectPackagingSettings'))
 result['packaging_settings']=fields(packaging,['MapsToCook','bCookAll','bCookMapsOnly','bSkipEditorContent','DirectoriesToNeverCook','DirectoriesToAlwaysCook'])
 cooked=[x.get_editor_property('FilePath') for x in packaging.get_editor_property('MapsToCook')]
 assert START in cooked and RETURN in cooked
 assert not packaging.get_editor_property('bCookAll'),'Unexpected CookAll override; inspect before packaging'
 registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
 result['map_assets']={p:[str(a.asset_name) for a in registry.get_assets_by_package_name(p)] for p in [START,RETURN]}
 assert all(result['map_assets'].values())
 world=ue.load_asset(START);assert world
 result['world_game_mode']=world.get_world_settings().get_editor_property('DefaultGameMode').get_path_name()
 assert result['world_game_mode']==MODE+'.BP_DC_StageUIGameMode_C'
 bp=ue.load_asset(MODE);assert bp
 default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
 result['game_mode']=fields(default,['DefaultExperience','ReturnMap','bUseExperienceUI'])
 assert result['game_mode']['DefaultExperience']=='DCLyraExperienceDefinition:B_DC_StageUIExperience'
 assert RETURN+'.L_DC_LyraFiniteEncounter' in result['game_mode']['ReturnMap'] and result['game_mode']['bUseExperienceUI']
 asset_settings=ue.get_default_object(ue.load_class(None,'/Script/Engine.AssetManagerSettings'))
 result['experience_scan_rules']=[x.export_text() for x in asset_settings.get_editor_property('PrimaryAssetTypesToScan') if 'DCLyraExperience' in x.export_text()]
 assert len(result['experience_scan_rules'])==2 and all('/Game/DreamCatcher/GAS/Test/R13' in x and 'AlwaysCook' in x for x in result['experience_scan_rules'])
 project=json.loads((P/'DreamCatcher.uproject').read_text(encoding='utf-8-sig'))
 assert any(x['Name']=='DCStageUI' and x['Enabled'] for x in project['Plugins'])
 result['DCStageUI_enabled']=True
 assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()==0
 errors=[x for x in log.read_text(encoding='utf-8-sig',errors='replace').splitlines() if re.search(r': Error:|Ensure condition failed|Fatal:',x)]
 assert not errors,errors[:3]
 result['complete']=True
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 stats={f.relative_to(P).as_posix():[f.stat().st_size,f.stat().st_mtime_ns] for root in ['Content','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
 result['asset_stats_unchanged']=stats==baseline['assets']
 result['changed_files']=[p for p,h in baseline['files'].items() if digest(P/p)!=h]
 result['unexpected_changes']=sorted(set(result['changed_files'])-set(baseline['targets']))
 result['complete']=result['complete'] and result['asset_stats_unchanged'] and not result['unexpected_changes']
 out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 print('R14_PACKAGE_CONFIG '+str(result))
