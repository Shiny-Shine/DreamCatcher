"""Approved R14-2A: two disconnected nodes and exactly three unused assets.

Only Unreal graph/save/delete APIs mutate assets. Committed local LFS payloads
are verified recovery data. Separate apply and verify processes are required.
"""
import hashlib,json,re,sys,traceback
from pathlib import Path
import unreal as ue
sys.dont_write_bytecode=True
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R14'
sys.path.insert(0,str(P/'Scripts/Editor'))
from r7_root_ui_setup import snapshot
from r10_enemy_setup import cdo,components,fields
cmd=ue.SystemLibrary.get_command_line()
mode=re.search(r'-DCR14Cleanup=(apply|delete|verify)',cmd).group(1)
run=re.search(r'-DCR14CleanupRun=([A-Za-z0-9_]+)',cmd).group(1)
assert mode=='delete' or '-run=pythonscript' in cmd.lower()
assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P
out=R/('cleanup_'+run+'.json');assert not out.exists()
log=P/'Saved/Logs'/('R14Cleanup-'+run+'.log')
PAWN='/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn'
REMOVED=['/Game/DreamCatcher/GAS/Abilities/GA_DC_Death',
 '/Game/DreamCatcher/Camera/Modes/CM_DC_Hip','/Game/LyraMigration/Death/GCNL_Death']
NODES={'K2Node_CallFunction_8','K2Node_VariableGet_0'}
recovery=json.loads((R/'cleanup_recovery_A.json').read_text(encoding='utf-8'));assert recovery['complete']
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
file_for=lambda p:P/'Content'/(p[6:]+'.uasset')
def asset_stats():
 return {f.relative_to(P).as_posix():[f.stat().st_size,f.stat().st_mtime_ns] for root in ['Content','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
before_stats=asset_stats()
protected=recovery['protected'];assert all(digest(Path(f))==h for f,h in protected.items())
result=dict(complete=False,mode=mode,asset_saves=0,deleted=[],recovery_commit=recovery['commit'],cpp_build=False,PIE=False)
def log_errors(allow_old_cue=False):
 errors=[x for x in log.read_text(encoding='utf-8-sig',errors='replace').splitlines() if re.search(r': Error:|Ensure condition failed|Fatal:',x)]
 allowed=[];unexpected=[]
 for line in errors:
  known=allow_old_cue and ('Content\\LyraMigration\\Death\\GCNL_Death.uasset' in line or 'Content/LyraMigration/Death/GCNL_Death.uasset' in line) and any(t in line for t in [
   'In use pin  Skeletal Mesh Component  no longer exists on node  Macro instance',
   'Macro node  Macro instance  is pointing at an invalid macro graph.'])
  (allowed if known else unexpected).append(line)
 assert not unexpected,unexpected[:3]
 assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()==0
 return allowed
def package_is_dirty(pkg):
 return any(x.get_path_name()==pkg for x in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages())
def component_state(bp):
 rows=[]
 for obj in components(bp):
  row=dict(name=obj.get_name(),cls=obj.get_class().get_path_name())
  if row['cls']=='/Script/DreamCatcher.DCHeroComponent':row['values']=fields(obj,['bUseLegacyPlayerInput','bUseLegacyCameraMode','bUseOriginalADSInputRouting','DefaultInputMappings'])
  if isinstance(obj,ue.SkeletalMeshComponent):row['values']=fields(obj,['SkeletalMeshAsset','AnimClass','RelativeLocation','RelativeRotation','RelativeScale3D'])
  rows.append(row)
 return sorted(rows,key=lambda x:x['name'])
try:
 registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
 assets=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
 options=ue.AssetRegistryDependencyOptions(True,True,True,True,True)
 if mode=='apply':
  for p in [PAWN]+REMOVED:
   rec=recovery['assets']['Content/'+p[6:]+'.uasset']
   assert digest(file_for(p))==rec['sha256'],p
   if rec['lfs_oid']:assert digest(Path(rec['lfs_payload']))==rec['lfs_oid']
   assert not package_is_dirty(p),'Dirty target before modification: '+p
  result['pre_delete_referencers']={p:[str(x) for x in registry.get_referencers(p,options)] for p in REMOVED}
  assert all(v==[] for v in result['pre_delete_referencers'].values())
  bp=ue.load_asset(PAWN);assert bp and not package_is_dirty(PAWN)
  before=snapshot(bp);component_before=component_state(bp)
  audited=json.loads((R/'references_D/selected_blueprints.json').read_text(encoding='utf-8'))[PAWN]['snapshot']
  assert before==audited,'Blueprint changed since approved graph review'
  graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'EventGraph')
  nodes={n.get_name():n for n in graph.list_all_nodes()}
  equip=nodes['K2Node_CallFunction_8'];variable=nodes['K2Node_VariableGet_0']
  assert ue.BlueprintEditorLibrary.get_node_title(equip)=='EquipItem'
  assert equip.find_input_pin('EquipmentDefinition').get_pin_value()=='/Game/DreamCatcher/Equipment/Definitions/WID_DC_Rifle.WID_DC_Rifle_C'
  for pin in [equip.find_execute_pin(),equip.find_output_pin('then'),equip.find_result_pin()]:assert not pin.list_connected_pins()
  links=variable.find_output_pin('EquipmentManager').list_connected_pins();assert len(links)==1 and links[0].get_owning_node()==equip
  graph.remove_nodes([equip,variable])
  expected=json.loads(json.dumps(before))
  for g in expected['graphs']:
   if g['name']=='EventGraph':g['nodes']=[n for n in g['nodes'] if n['name'] not in NODES]
  assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
  # The shared reader enumerates UObjects by graph outer. Removed node UObjects
  # can survive until GC; use the graph's actual membership to exclude only the
  # two approved removals, while still comparing every remaining node and pin.
  assert not NODES.intersection(n.get_name() for n in graph.list_all_nodes())
  actual=snapshot(bp)
  result['removed_nodes_still_in_object_iterator']=[]
  for g in actual['graphs']:
   if g['name']=='EventGraph':
    result['removed_nodes_still_in_object_iterator']=[n['name'] for n in g['nodes'] if n['name'] in NODES]
    g['nodes']=[n for n in g['nodes'] if n['name'] not in NODES]
  result['graph_comparison']=dict(expected=expected,actual=actual)
  assert actual==expected,'Unexpected remaining graph change'
  assert component_state(bp)==component_before,'Component or mesh configuration changed'
  log_errors()
  assert assets.save_loaded_asset(bp,False);result['asset_saves']=1
  result.update(expected_snapshot=expected,expected_components=component_before,pawn_sha256=digest(file_for(PAWN)))
  # Flush a recovery-aware receipt before each independent, approved deletion.
  out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
  for p in REMOVED:
   assert [str(x) for x in registry.get_referencers(p,options)]==[],p
   assert not package_is_dirty(p),'Unexpected dirty target before deletion: '+p
   assert digest(file_for(p))==recovery['assets']['Content/'+p[6:]+'.uasset']['sha256']
   assert assets.delete_asset(p),'Unreal could not delete '+p
   assert not file_for(p).exists() and not registry.get_assets_by_package_name(p)
   result['deleted'].append(p)
   out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
  result['expected_old_cue_load_errors']=log_errors(True)
 elif mode=='delete':
  applied=json.loads((R/'cleanup_Apply_B.json').read_text(encoding='utf-8'))
  assert applied['asset_saves']==1 and applied['deleted']==[]
  assert digest(file_for(PAWN))==applied['pawn_sha256']
  result['pawn_sha256']=applied['pawn_sha256']
  for p in REMOVED:
   assert digest(file_for(p))==recovery['assets']['Content/'+p[6:]+'.uasset']['sha256']
   assert [str(x) for x in registry.get_referencers(p,options)]==[],p
   assert not package_is_dirty(p),'Dirty target before deletion: '+p
  log_errors()
  # UE5.8's default reference gathering can retain the emptied package even
  # after ForceDeleteObjects reports success. Use its built-in comparison
  # algorithm, still with the same external reference and file hash guards.
  world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
  result['reference_algorithm_before']=ue.SystemLibrary.get_console_variable_int_value('Editor.UseLegacyGetReferencersForDeletion')
  ue.SystemLibrary.execute_console_command(world,'Editor.UseLegacyGetReferencersForDeletion 1')
  assert ue.SystemLibrary.get_console_variable_int_value('Editor.UseLegacyGetReferencersForDeletion')==1
  for p in REMOVED:
   assert assets.delete_asset(p),'Unreal could not delete '+p
   result.setdefault('delete_api_results',{})[p]=dict(file_absent=not file_for(p).exists(),registry_empty=not registry.get_assets_by_package_name(p))
   assert not file_for(p).exists() and not registry.get_assets_by_package_name(p),result['delete_api_results'][p]
   result['deleted'].append(p)
   out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
  result['expected_old_cue_load_errors']=log_errors(True)
 else:
  applied=json.loads((R/'cleanup_Apply_B.json').read_text(encoding='utf-8'))
  deleted=json.loads((R/'cleanup_Delete_B.json').read_text(encoding='utf-8'))
  assert applied['asset_saves']==1 and deleted['complete'] and deleted['deleted']==REMOVED
  assert digest(file_for(PAWN))==applied['pawn_sha256']
  for p in REMOVED:assert not file_for(p).exists() and not registry.get_assets_by_package_name(p),p
  bp=ue.load_asset(PAWN);assert bp
  assert snapshot(bp)==applied['expected_snapshot']
  assert component_state(bp)==applied['expected_components']
  assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
  assert snapshot(bp)==applied['expected_snapshot']
  # Directly affected and preserved active parents; no entire-project compile.
  checks=[
   '/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn',
   '/Game/DreamCatcher/Blueprints/Player/Test/BP_DC_Player_GAS_Test',
   '/Game/Characters/Heroes/Abilities/GA_Hero_Death',
   '/Game/DreamCatcher/GAS/Test/R10/GA_DC_EnemyDeath',
   '/Game/GameplayCueNotifies/DCMigration/Death/GCNL_Death',
   '/Game/Characters/Cameras/CM_ThirdPerson']
  result['compiled']=[PAWN]
  for p in checks:
   obj=ue.load_asset(p);assert obj and ue.BlueprintEditorLibrary.compile_blueprint(obj),p
   result['compiled'].append(p)
  deps=[str(x) for x in registry.get_dependencies(PAWN,ue.AssetRegistryDependencyOptions(True,True,False,False,False))]
  result['pawn_dependencies']=deps
  assert '/Game/DreamCatcher/Equipment/Definitions/WID_DC_Rifle' not in deps,'Old definition still referenced by edited Pawn'
  data=ue.load_asset('/Game/DreamCatcher/GAS/Test/R10/LyraAI/DA_DC_LyraAIPawn');assert data
  result['active_pawn_data']=fields(data,['PawnClass','DefaultWeaponDefinition','DefaultCameraMode','AbilitySets'])
  assert data.get_editor_property('DefaultWeaponDefinition') is None
  assert len([x for x in component_state(ue.load_asset(checks[0])) if x['cls']=='/Script/DreamCatcher.DCLyraEquipmentManagerComponent'])==1
  result['log_errors']=log_errors()
 result['complete']=True
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 if 'reference_algorithm_before' in result:
  ue.SystemLibrary.execute_console_command(ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world(),'Editor.UseLegacyGetReferencersForDeletion '+str(result['reference_algorithm_before']))
 current=asset_stats()
 result['asset_file_changes']={p:dict(before=before_stats.get(p),after=current.get(p)) for p in set(before_stats)|set(current) if before_stats.get(p)!=current.get(p)}
 allowed={'Content/'+p[6:]+'.uasset' for p in ([PAWN]+REMOVED if mode=='apply' else REMOVED)} if mode in ['apply','delete'] else set()
 result['unexpected_asset_changes']=sorted(set(result['asset_file_changes'])-allowed)
 result['protected_changes']=[p for p,h in protected.items() if digest(Path(p))!=h]
 result['complete']=result['complete'] and not result['unexpected_asset_changes'] and not result['protected_changes']
 out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
 print('R14_CLEANUP '+str({k:v for k,v in result.items() if k not in ['expected_snapshot','expected_components','pawn_dependencies','active_pawn_data','asset_file_changes','graph_comparison']}))
