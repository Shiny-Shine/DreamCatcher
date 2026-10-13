"""R14-1 read-only inventory for the accepted R13 path and preserved legacy assets.

Reuses the existing registry/graph readers. No compile, save, rename, deletion,
config write, build, package, or migration API is called. Disk references are
evidence of dependency, not proof of execution or absence of dynamic lookup.
"""
import hashlib,json,re,sys,traceback
from collections import deque
from pathlib import Path
import unreal as ue
sys.dont_write_bytecode=True
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R14'
sys.path.insert(0,str(P/'Scripts/Editor'))
from r7_root_ui_setup import snapshot
from r10_enemy_setup import cdo,components,encode
run=re.search(r'-DCR14Audit=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line()).group(1)
reuse=re.search(r'-DCR14ReuseBlueprints=([A-Za-z0-9_]+)',ue.SystemLibrary.get_command_line())
out=R/('references_'+run);out.mkdir(parents=True,exist_ok=False)
assert '-run=pythonscript' in ue.SystemLibrary.get_command_line().lower()
assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P
LEGACY=['DCCombatComponent','DCHealthComponent','DCEquipmentDefinition','DCEquipmentInstance',
 'DCEquipmentManagerComponent','DCGameplayAbility_FromEquipment','DCWeaponActor','DCWeaponInstance',
 'DCRangedWeaponInstance','DCGameplayAbility_RangedWeapon','DCReticleWidgetBase','DCPlayerHUDWidget',
 'DCCameraMode_ThirdPerson','DCGameplayAbility_Death']
CORE=['DreamCatcherCharacter','DreamCatcherPlayerController','DCPawnData','DCHeroComponent',
 'DCAbilitySystemComponent','DCAbilitySet','DCCameraMode','DCCameraComponent','DCGASEnemyCharacter',
 'DCBossCharacter','DCStageRunGameMode','DCEnemySpawner']
R13='/Game/DreamCatcher/GAS/Test/R13'
ROOTS=[R13+'/L_DC_Level1UI',R13+'/B_DC_StageUIExperience',R13+'/LAS_DC_StageHUD',
 R13+'/LAS_DC_StageNotifications','/DCStageUI/DCStageUI']
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
def file_for(pkg):
 if pkg.startswith('/Game/'):return P/'Content'/(pkg[6:]+'.uasset')
 if pkg.startswith('/DCStageUI/'):return P/'Plugins/GameFeatures/DCStageUI/Content'/(pkg[len('/DCStageUI/'):]+'.uasset')
 return None
def stats():
 return {str(f.relative_to(P)):[f.stat().st_size,f.stat().st_mtime_ns] for root in ['Content','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
before_stats=stats()
protected={str(f):digest(f) for root in ['Source','Config','Plugins/DCRifleMigrationTools/Source','Plugins/GameFeatures/DCStageUI','Saved/SaveGames'] for f in (P/root).rglob('*') if f.is_file()}
for f in [P/'DreamCatcher.uproject',P/'.git/index',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']:
 protected[str(f)]=digest(f)
result=dict(complete=False,asset_saves=0,blueprint_compiles=0,PIE=False,classes=LEGACY,roots=ROOTS,limitations=[
 'Saved AssetRegistry dependencies and selected Blueprint graphs; unsaved editor state is not inspected.',
 'Native script-package edges do not identify individual classes; metadata parents, selected graphs and C++ consumers are recorded separately.',
 'Runtime string/tag lookups, unmounted external content and designer uncommitted changes are not proven absent.'])
def write(name,value):(out/name).write_text(json.dumps(value,ensure_ascii=False,indent=2),encoding='utf-8')
def read_fields(obj,names):
 values={};unavailable={}
 for n in names:
  try:values[n]=encode(obj.get_editor_property(n))
  except Exception as e:unavailable[n]=str(e)
 return dict(values=values,unavailable=unavailable)
try:
 registry=ue.AssetRegistryHelpers.get_asset_registry()
 if not ue.PluginBlueprintLibrary.is_plugin_mounted('DCStageUI'):
  world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
  ue.SystemLibrary.execute_console_command(world,'PackageName.RegisterMountPoint /DCStageUI/ "'+str(P/'Plugins/GameFeatures/DCStageUI/Content').replace('\\','/')+'/"')
 registry.search_all_assets(True)
 registry.scan_paths_synchronous(['/DCStageUI/'],True)
 options=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
 all_options=ue.AssetRegistryDependencyOptions(True,True,True,True,True)
 assets={}
 for a in registry.get_all_assets(True):
  pkg=str(a.package_name)
  if not pkg.startswith(('/Game/','/DCStageUI/')):continue
  assets[pkg]=dict(name=str(a.asset_name),cls=str(a.asset_class_path.package_name)+'.'+str(a.asset_class_path.asset_name),parent=a.get_tag_value('ParentClass'),
   native_parent=a.get_tag_value('NativeParentClass'),generated=a.get_tag_value('GeneratedClass'),
   redirector=a.is_redirector(),destination=a.get_tag_value('DestinationObject'))
 write('asset_metadata.json',assets)
 def deps(p):return sorted(str(x) for x in registry.get_dependencies(p,options))
 def refs(p):
  values=registry.get_referencers(p,all_options)
  return None if values is None else sorted(str(x) for x in values)
 def closure(roots):
  queue=deque(roots);seen={};external=set()
  while queue:
   p=queue.popleft()
   if p in seen or p in external:continue
   if not p.startswith(('/Game/','/DCStageUI/')):external.add(p);continue
   assert len(seen)<20000,'Incomplete closure: bounded limit reached'
   seen[p]=deps(p);queue.extend(seen[p])
  return dict(packages=seen,external=sorted(external),missing_metadata=sorted(set(seen)-assets.keys()))
 active=closure(ROOTS);write('r13_dependencies.json',active)
 preserved_maps=sorted(p for p,a in assets.items() if a['cls']=='/Script/Engine.World' and (p.startswith('/Game/DreamCatcher/GAS/Test/') or p.startswith('/Game/DreamCatcher/DreamCatcher_ENV/Map/')))
 write('preserved_map_dependencies.json',{p:deps(p) for p in preserved_maps})
 legacy_bps={p:a for p,a in assets.items() if any('/Script/DreamCatcher.'+n+"'" in str(a[k]) or '/Script/DreamCatcher.'+n+'"' in str(a[k]) or str(a[k])=='/Script/DreamCatcher.'+n for n in LEGACY for k in ['parent','native_parent'])}
 # Native parent metadata is the important classifier; suffix matching above
 # avoids confusing the old classes with the separate DCLyra counterparts.
 selected=set(legacy_bps)
 selected.update(p for p,a in assets.items() if p.startswith((R13+'/', '/Game/Blueprint/')) and a['generated'])
 selected.update(p for p,a in assets.items() if p in active['packages'] and a['generated'] and any(n in str(a['native_parent']) for n in ['DreamCatcherCharacter','DCLyraAIIntegrationController','DCStageRunGameMode','DCBossCharacter','DCLyraExperienceDefinition']))
 assert len(selected)<=200,len(selected)
 details={}
 if reuse:
  prior=R/('references_'+reuse.group(1))
  old_hashes=json.loads((prior/'protected_hashes.json').read_text(encoding='utf-8'))
  assert all(digest(Path(f))==h for f,h in old_hashes.items()),'Previously inspected files changed'
  details=json.loads((prior/'selected_blueprints.json').read_text(encoding='utf-8'))
  assert set(details)==selected,'Selected Blueprint set changed'
  protected.update(old_hashes);result['blueprint_evidence_reused_from']=reuse.group(1)
 for index,p in enumerate([] if reuse else sorted(selected)):
  f=file_for(p)
  if f:protected[str(f)]=digest(f)
  obj=ue.load_asset(p);assert isinstance(obj,ue.Blueprint),p
  row=dict(metadata=assets[p],referencers=refs(p),r13_dependency=p in active['packages'],snapshot=snapshot(obj))
  default=cdo(obj)
  names=[]
  for cname,fields in {
   'DreamCatcherPlayerController':['HUDWidgetClass'],
   'DCLyraAIIntegrationController':['StartingRifle','WeaponReticleHostClass','DefaultMappingContexts'],
   'DCStageRunGameMode':['bUseExperienceUI','DefaultExperience','PlayerControllerClass','DefaultPawnClass','ReturnMap'],
   'DCLyraExperienceDefinition':['DefaultPawnData','GameFeaturesToEnable','ActionSets','Actions'],
   'DCEnemySpawner':['EnemyClass','GASEnemyClass','LyraBotControllerClass','bCleanupOwnedGASNPCs'],
  }.items():
   cl=ue.load_class(None,'/Script/DreamCatcher.'+cname)
   assert cl,cname
   if isinstance(default,type(ue.get_default_object(cl))):names.extend(fields)
  row['defaults']=read_fields(default,names)
  if isinstance(default,ue.Actor):
   row['components']=[]
   for comp in components(obj):
    record=dict(name=comp.get_name(),cls=comp.get_class().get_path_name())
    if record['cls']=='/Script/DreamCatcher.DCHeroComponent':record.update(read_fields(comp,['bUseLegacyPlayerInput','bUseLegacyCameraMode','bUseOriginalADSInputRouting','DefaultInputMappings']))
    row['components'].append(record)
  details[p]=row
  if index%10==0:print('R14_READ_BP '+str(index)+'/'+str(len(selected)))
 write('selected_blueprints.json',details)
 data={}
 pawn_cls=ue.load_class(None,'/Script/DreamCatcher.DCPawnData')
 for p,a in assets.items():
  if a['cls']!='/Script/DreamCatcher.DCPawnData':continue
  f=file_for(p);protected[str(f)]=digest(f)
  obj=ue.load_asset(p);assert isinstance(obj,type(ue.get_default_object(pawn_cls)))
  data[p]=dict(fields=read_fields(obj,['PawnClass','AbilitySets','InputConfig','DefaultCameraMode','ShoulderCameraMode','ScopeCameraMode','DefaultWeaponDefinition','TagRelationshipMapping']),referencers=refs(p),r13_dependency=p in active['packages'])
 write('pawn_data.json',data)
 assert '/Game/DreamCatcher/GAS/Test/R10/LyraAI/DA_DC_LyraAIPawn' in data,'Active PawnData class classification incomplete'
 assert ROOTS[0] in preserved_maps,'World class classification incomplete'
 redirects={}
 main=json.loads((P/'Saved/Diagnostics/R13/ui_assets_Prepare_D/report.json').read_text(encoding='utf-8'))
 planned={('/Game/'+p.split('/',2)[2]):dest for p,dest in main['mapping'].items() if p!='/Game/UI/Hud/W_LyraGameMenu' and '/Game/'+p.split('/',2)[2]!=dest}
 planned.update(json.loads((P/'Saved/Diagnostics/R13/ui_config_Source_B/report.json').read_text(encoding='utf-8'))['mapping'])
 for p,dest in planned.items():
  a=assets.get(p)
  redirects[p]=dict(expected_destination=dest,metadata=a,referencers=refs(p),r13_dependency=p in active['packages'])
 write('r13_intermediate_redirectors.json',redirects)
 consumers={}
 for name in LEGACY+CORE:
  rows=[];pat=re.compile(r'\b[AUF]?'+re.escape(name)+r'\b')
  for root in ['Source','Config']:
   for f in (P/root).rglob('*'):
    if not f.is_file() or f.suffix not in ['.h','.cpp','.ini','.cs']:continue
    for line,text in enumerate(f.read_text(encoding='utf-8-sig',errors='replace').splitlines(),1):
     if pat.search(text):rows.append(dict(file=str(f.relative_to(P)),line=line,text=text.strip()))
  consumers[name]=rows
 write('native_text_consumers.json',consumers)
 result.update(asset_metadata_count=len(assets),r13_dependency_count=len(active['packages']),legacy_parent_blueprints=sorted(legacy_bps),blueprint_read_count=len(details),pawn_data_count=len(data),preserved_maps=preserved_maps,
  r13_redirector_count=sum(bool(a['metadata'] and a['metadata']['redirector']) for a in redirects.values()),
  r13_redirectors_without_disk_referencers=[p for p,a in redirects.items() if a['metadata'] and a['metadata']['redirector'] and a['referencers']==[]],ensure_count=ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count())
 result['complete']=result['ensure_count']==0
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 after_stats=stats()
 result['asset_file_stat_changes']=[p for p in set(before_stats)|set(after_stats) if before_stats.get(p)!=after_stats.get(p)]
 result['protected_hash_changes']=[p for p,h in protected.items() if digest(Path(p))!=h]
 result['protected_hash_count']=len(protected)
 result['complete']=result['complete'] and not result['asset_file_stat_changes'] and not result['protected_hash_changes']
 write('protected_hashes.json',protected);write('report.json',result)
 print('R14_REFERENCE_AUDIT '+str({k:v for k,v in result.items() if k not in ['legacy_parent_blueprints','preserved_maps','r13_redirectors_without_disk_referencers']}))
