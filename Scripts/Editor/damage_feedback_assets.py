"""Scoped original Lyra DamageTaken/NumberPop port (48 approved new assets).

Run with -SCCProvider=None -unattended -DCDamageFeedback=migrate|integrate|verify.
Migrate runs in original Lyra; remaining modes run in DreamCatcher. No C++ build.
"""
import hashlib,json,re,traceback
from pathlib import Path
import unreal as u

P=Path('E:/Epic Games/UEProjects/DreamCatcher')
S=P.parent/'LyraStarterGame';D=P/'Saved/Diagnostics/HitZones20261018'
MANIFEST=json.loads((P/'docs/handoffs/hit-zones-and-damage-feedback-2026-10-18.json').read_text(encoding='utf-8'))
PACKAGES=MANIFEST['missing_package_candidates'];assert len(PACKAGES)==48
CUE='/Game/GameplayCueNotifies/GCNL_Character_DamageTaken'
NUMBER='/Game/Effects/Blueprints/B_NiagaraNumberPopComponent'
STYLE='/Game/Effects/Blueprints/Damage_BasicNiagaraStyle'
SYSTEM='/Game/Effects/Particles/Impacts/NS_DamageNumbers'
MAPPING={p:('/Game/DreamCatcher/Lyra/GameplayCues/DamageTaken/'+p.rsplit('/',1)[1] if p==CUE else '/Game/DreamCatcher/Lyra/Feedback/DamageTaken/'+p[6:]) for p in PACKAGES}
STAGE='/DCStageUI/DCStageUI'
CONTROLLER='/Game/DreamCatcher/Blueprints/Player/BP_DC_StageUIController.BP_DC_StageUIController_C'

def file(root,p):return root/'Content'/(p[6:]+'.uasset')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def snapshot(root):return {str(p.relative_to(root)):[p.stat().st_size,p.stat().st_mtime_ns] for p in (root/'Content').rglob('*') if p.is_file()}
def export(obj,label):
 path=D/(label+'.t3d');assert not path.exists()
 task=u.AssetExportTask();task.object=obj;task.exporter=u.ObjectExporterT3D();task.filename=str(path);task.automated=True;task.prompt=False
 assert u.Exporter.run_asset_export_task(task);return str(path)
def validate_assets(report):
 assets={}
 for old,new in MAPPING.items():
  obj=u.load_asset(new);assert obj,new
  if isinstance(obj,u.Blueprint):assert u.BlueprintEditorLibrary.compile_blueprint(obj),new
  assets[old]=obj
 number=assets[NUMBER];style=assets[STYLE]
 assert u.BlueprintEditorLibrary.get_blueprint_parent_class(number).get_path_name()=='/Script/DreamCatcher.DCLyraNumberPopComponent_NiagaraText'
 assert u.get_default_object(number.generated_class()).get_editor_property('Style')==style
 assert style.get_editor_property('TextNiagara')==assets[SYSTEM]
 assert str(style.get_editor_property('NiagaraArrayName'))=='DamageInfo'
 cue=u.get_default_object(assets[CUE].generated_class())
 assert str(cue.get_editor_property('GameplayCueTag').export_text())=='(TagName="GameplayCue.Character.DamageTaken")'
 report['assets']={k:dict(path=v.get_path_name(),cls=v.get_class().get_path_name()) for k,v in assets.items()}
 report['number_style']=style.get_path_name();report['number_system']=assets[SYSTEM].get_path_name()
 registry=u.AssetRegistryHelpers.get_asset_registry()
 report['dependencies']={new:[str(x) for x in registry.get_dependencies(new,u.AssetRegistryDependencyOptions(True,True,False,False,False))] for new in MAPPING.values()}
 unresolved=sorted({p for deps in report['dependencies'].values() for p in deps if p.startswith('/Game/') and (p in PACKAGES or not u.EditorAssetLibrary.does_asset_exist(p))})
 assert not unresolved,unresolved
 return assets

def main():
 cmd=u.SystemLibrary.get_command_line();assert '-unattended' in cmd.lower() and '-sccprovider=none' in cmd.lower() and not u.SourceControl.is_enabled()
 mode=re.search(r'-DCDamageFeedback=(migrate|integrate|verify)\b',cmd).group(1)
 current=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())).resolve()
 assert current==(S if mode=='migrate' else P).resolve()
 assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages() and not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
 run_match=re.search(r'-DCFeedbackRun=([A-Za-z0-9_]+)',cmd);run=run_match.group(1) if run_match else 'J'
 out=D/('feedback_'+mode+'_'+run+'.json');assert not out.exists()
 before=snapshot(P);source_before=snapshot(S)
 protected={str(file(P,p)):sha(file(P,p)) for p in MANIFEST['existing_package_candidates']}
 for p in [P/'.git/HEAD',P/'.git/index',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']:protected[str(p)]=sha(p)
 report=dict(mode=mode,complete=False,mapping=MAPPING,saved=[],PIE=False)
 try:
  if mode=='migrate':
   assert all(not file(P,p).exists() and not file(P,MAPPING[p]).exists() for p in PACKAGES)
   # Do not load assets before this call: MigratePackages checks/saves dirty packages.
   opts=u.MigrationOptions();opts.set_editor_property('bPrompt',False);opts.set_editor_property('bIgnoreDependencies',True);opts.set_editor_property('AssetConflict',u.AssetMigrationConflict.SKIP)
   u.AssetToolsHelpers.get_asset_tools().migrate_packages(PACKAGES,str(P/'Content'),opts)
   assert all(file(P,p).exists() for p in PACKAGES)
   report['saved']=PACKAGES
  else:
   if mode=='integrate' and not all(file(P,p).exists() for p in MAPPING.values()):
    registry=u.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
    for old,new in MAPPING.items():
     assert file(P,old).exists() and not file(P,new).exists()
     refs=[str(p) for p in registry.get_referencers(old,u.AssetRegistryDependencyOptions(True,True,False,False,False))]
     assert not [p for p in refs if p not in PACKAGES],(old,refs)
    rename=[]
    for old,new in MAPPING.items():
     obj=u.load_asset(old);assert obj,old
     rename.append(u.AssetRenameData(obj,new.rsplit('/',1)[0],new.rsplit('/',1)[1]))
    assert u.AssetToolsHelpers.get_asset_tools().rename_assets(rename)
   elif mode=='integrate':
    # Resume only the exact engine-renamed set after H's protected getter failure.
    previous=json.loads((D/'feedback_integrate_H.json').read_text(encoding='utf-8'))
    assert previous['mapping']==MAPPING and not previous['protected_changes'] and not previous['existing_content_changes']
    assert all(not file(P,p).exists() for p in PACKAGES)
   assets=validate_assets(report)
   number=assets[NUMBER]
   if not u.PluginBlueprintLibrary.is_plugin_mounted('DCStageUI'):
    world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    u.SystemLibrary.execute_console_command(world,'PackageName.RegisterMountPoint /DCStageUI/ "'+(P/'Plugins/GameFeatures/DCStageUI/Content').as_posix()+'/"')
   stage=u.load_asset(STAGE);assert stage
   export(stage,mode+'_'+run+'_stage_before')
   adds=[a for a in stage.get_editor_property('Actions') if a.get_class().get_name()=='GameFeatureAction_AddComponents'];assert len(adds)==1
   action=adds[0];entries=action.get_editor_property('ComponentList')
   entry_text=[e.export_text() for e in entries];report['component_entries_before']=entry_text
   number_path=number.generated_class().get_path_name()
   if mode=='integrate':
    assert not any(number_path in t for t in entry_text)
    entries.resize(len(entries)+1);entry=entries[-1]
    assert entry.import_text('(ActorClass="'+CONTROLLER+'",ComponentClass="'+number_path+'",bClientComponent=True,bServerComponent=False)')
    entries[-1]=entry;action.set_editor_property('ComponentList',entries)
    assert [e.export_text() for e in action.get_editor_property('ComponentList')][:-1]==entry_text
    for obj in list(assets.values())+[stage]:
     assert u.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False);report['saved'].append(obj.get_path_name())
   report['component_entries_after']=[e.export_text() for e in action.get_editor_property('ComponentList')]
   assert sum(number_path in t and CONTROLLER in t for t in report['component_entries_after'])==1
   for old,label in [(CUE,'cue'),(NUMBER,'number'),(STYLE,'style')]:
    export(assets[old],mode+'_'+run+'_'+label)
    if isinstance(assets[old],u.Blueprint):export(u.get_default_object(assets[old].generated_class()),mode+'_'+run+'_'+label+'_cdo')
   export(stage,mode+'_'+run+'_stage_after')
   report['new_asset_sha256']={p:sha(file(P,p)) for p in MAPPING.values()}
  report['complete']=True
 except Exception:
  report['error']=traceback.format_exc();raise
 finally:
  after=snapshot(P);source_after=snapshot(S)
  report['source_changes']=[p for p in source_before.keys()|source_after.keys() if source_before.get(p)!=source_after.get(p)]
  report['existing_content_changes']=[p for p in before if before[p]!=after.get(p) and p not in {str(file(P,x).relative_to(P)) for x in PACKAGES+list(MAPPING.values())}]
  report['protected_changes']=[p for p,h in protected.items() if not Path(p).exists() or sha(Path(p))!=h]
  report['complete']=report['complete'] and not any(report[k] for k in ['source_changes','existing_content_changes','protected_changes'])
  out.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
  print('DAMAGE_FEEDBACK '+json.dumps({k:report.get(k) for k in ['mode','complete','error','source_changes','existing_content_changes','protected_changes']},ensure_ascii=False))
if __name__=='__main__':main()
