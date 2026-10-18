"""Approved HARU/body material setup and isolated boss Physics Asset copy.

Unattended Editor only, -SCCProvider=None -DCHitZonePhysics=apply|verify.
No changes to shapes, constraints, meshes, skeletons, weapon, map or native code.
"""
import hashlib,json,re
from pathlib import Path
import unreal as u

P=Path('E:/Epic Games/UEProjects/DreamCatcher');D=P/'Saved/Diagnostics/HitZones20261018'
run_match=re.search(r'-DCPhysicsRun=([A-Za-z0-9_]+)',u.SystemLibrary.get_command_line());RUN=run_match.group(1) if run_match else 'G'
HARU='/Game/DreamCatcher/Main_Charcher/Haru/하루리깅_PhysicsAsset'
BOSS_SOURCE='/Game/Characters/Mannequins/Rigs/PA_Mannequin'
BOSS_PA='/Game/DreamCatcher/Physics/PA_DC_BossHitZones'
BOSS_BP='/Game/DreamCatcher/Blueprints/AI/Boss/BP_DC_Boss'
NORMAL='/Game/Characters/Heroes/PhysMat_Player'
WEAK='/Game/Characters/Heroes/PhysMat_Player_WeakSpot'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def file(p):return P/'Content'/(p[6:]+'.uasset')
def text(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2] in [b'\xff\xfe',b'\xfe\xff'] else 'utf-8-sig')
def export(obj,label):
 p=D/(label+'_'+RUN+'.t3d');assert not p.exists()
 task=u.AssetExportTask();task.object=obj;task.exporter=u.ObjectExporterT3D();task.filename=str(p);task.automated=True;task.prompt=False
 assert u.Exporter.run_asset_export_task(task);return text(p)
def bodies(pa):return [b for b in u.ObjectIterator(u.load_class(None,'/Script/Engine.SkeletalBodySetup')) if b.get_outer()==pa]
def normalize_pa(t):
 t=t.replace(BOSS_PA,BOSS_SOURCE).replace('PA_DC_BossHitZones','PA_Mannequin')
 t=re.sub(r'^\s*PhysMaterial=.*(?:\r?\n)', '',t,flags=re.M)
 t=re.sub(r',?PhysMaterialOverride=(?:"[^"]*"|None),?',',',t)
 t=t.replace('(,','(').replace(',)',')')
 # Export order of sibling objects can change during an engine duplicate.
 # Compare properties within each exact exported object, keeping array indices.
 blocks={};stack=[]
 for raw in t.splitlines():
  line=raw.strip()
  if not line or line=='DefaultInstance=()':continue
  if line.startswith('Begin Object'):
   key=re.search(r'ExportPath="([^"]+)"',line).group(1)
   stack.append(key);blocks.setdefault(key,[]).append(line)
  elif line=='End Object':stack.pop()
  else:blocks[stack[-1]].append(line)
 return {key:sorted(values) for key,values in blocks.items()}
def assign(pa,weak_bones,count,normal,weak):
 bs=bodies(pa);assert len(bs)==count,(pa.get_path_name(),len(bs))
 names={str(b.get_editor_property('BoneName')) for b in bs};assert weak_bones<=names
 for b in bs:
  bi=b.get_editor_property('DefaultInstance')
  assert b.get_editor_property('PhysMaterial') is None and bi.get_editor_property('PhysMaterialOverride') is None
  material=weak if str(b.get_editor_property('BoneName')) in weak_bones else normal
  b.set_editor_property('PhysMaterial',material);bi.set_editor_property('PhysMaterialOverride',material);b.set_editor_property('DefaultInstance',bi)
def verify(pa,weak_bones,count):
 bs=bodies(pa);assert len(bs)==count
 result={}
 for b in bs:
  bone=str(b.get_editor_property('BoneName'));expected=WEAK if bone in weak_bones else NORMAL
  material=b.get_editor_property('PhysMaterial');override=b.get_editor_property('DefaultInstance').get_editor_property('PhysMaterialOverride')
  assert material and material.get_path_name().split('.')[0]==expected and override==material
  result[bone]=expected
 return result
def main():
 assert Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir())).resolve()==P.resolve()
 command=u.SystemLibrary.get_command_line();assert '-unattended' in command.lower() and '-sccprovider=none' in command.lower()
 assert not u.SourceControl.is_enabled()
 mode=re.search(r'-DCHitZonePhysics=(apply|verify)\b',command).group(1)
 out=D/('physics_'+mode+'_'+RUN+'.json');assert not out.exists()
 assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages() and not u.EditorLoadingAndSavingUtils.get_dirty_map_packages()
 protected={str(p):sha(p) for root in ['Content','Plugins/GameFeatures/DCStageUI/Content','Source','Config'] for p in (P/root).rglob('*') if p.is_file() and p not in [file(HARU),file(BOSS_BP)]}
 for path in ['Binaries/Win64/UnrealEditor-DreamCatcher.dll','.git/HEAD','.git/index']:protected[str(P/path)]=sha(P/path)
 result=dict(mode=mode,complete=False,saved=[],protected_sha256=protected,PIE=False)
 try:
  haru=u.load_asset(HARU);bp=u.load_asset(BOSS_BP);normal=u.load_asset(NORMAL);weak=u.load_asset(WEAK)
  assert haru and bp and normal and weak
  before_haru=export(haru,'physics_'+mode+'_haru_before')
  mesh=u.get_default_object(bp.generated_class()).get_component_by_class(u.SkeletalMeshComponent);assert mesh
  before_bp=export(u.get_default_object(bp.generated_class()),'physics_'+mode+'_boss_before_cdo')
  if mode=='apply':
   assert not file(BOSS_PA).exists()
   assert mesh.get_editor_property('PhysicsAssetOverride') is None
   source=u.load_asset(BOSS_SOURCE);before_boss=export(source,'physics_apply_boss_source')
   boss=u.AssetToolsHelpers.get_asset_tools().duplicate_asset('PA_DC_BossHitZones',BOSS_PA.rsplit('/',1)[0],source);assert boss
   assign(haru,{'neck_01','head'},18,normal,weak);assign(boss,{'neck_01','neck_02','head'},22,normal,weak)
   mesh.set_editor_property('PhysicsAssetOverride',boss)
   assert u.BlueprintEditorLibrary.compile_blueprint(bp)
  else:
   boss=u.load_asset(BOSS_PA)
   before_boss=export(boss,'physics_verify_boss_before')
  after_haru=export(haru,'physics_'+mode+'_haru_after');after_boss=export(boss,'physics_'+mode+'_boss_after')
  assert normalize_pa(before_haru)==normalize_pa(after_haru),'HARU changed beyond physical materials'
  assert normalize_pa(before_boss)==normalize_pa(after_boss),'Boss shapes/constraints changed'
  if mode=='verify':
   assert normalize_pa(after_haru)==normalize_pa(text(D/'physics_apply_haru_before_G.t3d'))
   assert normalize_pa(after_boss)==normalize_pa(text(D/'physics_apply_boss_source_G.t3d'))
  cdo=u.get_default_object(bp.generated_class());mesh=cdo.get_component_by_class(u.SkeletalMeshComponent)
  assert mesh.get_editor_property('PhysicsAssetOverride')==boss
  after_bp=export(cdo,'physics_'+mode+'_boss_after_cdo')
  strip=lambda t: '\n'.join(x.strip() for x in t.splitlines() if x.strip() and not x.strip().startswith('PhysicsAssetOverride='))
  assert strip(before_bp)==strip(after_bp),'Boss defaults changed beyond PhysicsAssetOverride'
  result['haru']=verify(haru,{'neck_01','head'},18);result['boss']=verify(boss,{'neck_01','neck_02','head'},22)
  if mode=='apply':
   for obj in [boss,haru,bp]:
    assert u.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False);result['saved'].append(obj.get_path_name())
  result['hashes']={p:sha(file(p)) for p in [HARU,BOSS_PA,BOSS_BP]}
  result['complete']=True
 finally:
  result['protected_changes']=[p for p,h in protected.items() if not Path(p).exists() or sha(Path(p))!=h]
  result['complete']=result['complete'] and not result['protected_changes'];out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
  print('HIT_ZONE_PHYSICS '+json.dumps({k:v for k,v in result.items() if k not in ['protected_sha256','haru','boss']},ensure_ascii=False))
if __name__=='__main__':main()
