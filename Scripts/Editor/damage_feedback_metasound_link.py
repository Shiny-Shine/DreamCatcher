"""Match the five newly imported presets to the existing RandomStereo class ID.

The existing source and target RandomStereo documents differ only in this class ID.
Uses Unreal's reflected struct import and asset save; never edits binary packages.
"""
import hashlib,json,re
from pathlib import Path
import unreal as u
P=Path('E:/Epic Games/UEProjects/DreamCatcher');D=P/'Saved/Diagnostics/HitZones20261018'
OLD='C7725AA14BADE5EB7F9F54894DB8BE03';NEW='5A9AD0844AAAEB75E9111BAC4F8BFFC3'
ROOT='/Game/DreamCatcher/Lyra/Feedback/DamageTaken/Audio/MetaSounds/'
NAMES=['sfx_Character_DamageGivenKill_nl_meta','sfx_Character_DamageGivenWeakSpot_nl_meta','sfx_Character_DamageGiven_nl_meta','sfx_Character_DamageTakenWeakSpot_nl_meta','sfx_Character_DamageTaken_nl_meta']
mode=re.search(r'-DCFeedbackSound=(apply|verify)',u.SystemLibrary.get_command_line()).group(1)
out=D/('sound_link_'+mode+'_O.json');assert not out.exists()
assert not u.SourceControl.is_enabled() and '-unattended' in u.SystemLibrary.get_command_line().lower()
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def text(p):
 b=p.read_bytes();return b.decode('utf-16' if b[:2] in [b'\xff\xfe',b'\xfe\xff'] else 'utf-8-sig').replace('\r','')
assert text(D/'shared_source_E/013.t3d').replace('ClassName=(Name="'+OLD+'")','ClassName=(Name="'+NEW+'")')==text(D/'shared_target_E/013.t3d')
allowed={P/'Content'/((ROOT+n)[6:]+'.uasset') for n in NAMES}
protected={str(p):sha(p) for p in (P/'Content').rglob('*') if p.is_file() and p not in allowed}
r=dict(mode=mode,complete=False,assets=[],saved=[],protected_changes=[])
try:
 parent=u.load_asset('/Game/Audio/MetaSounds/sfx_RandomStereo_nl_meta');assert parent
 parentdoc=parent.get_editor_property('RootMetasoundDocument').export_text();assert NEW in parentdoc and OLD not in parentdoc
 assets=[]
 for name in NAMES:
  obj=u.load_asset(ROOT+name);assert obj
  doc=obj.get_editor_property('RootMetasoundDocument');before=doc.export_text()
  if mode=='apply':
   assert before.count(OLD)==1,(name,before.count(OLD))
   expected=before.replace(OLD,NEW);assert doc.import_text(expected)
   obj.set_editor_property('RootMetasoundDocument',doc)
   assert obj.get_editor_property('RootMetasoundDocument').export_text()==expected,name
   (D/(name+'_before_O.txt')).write_text(before,encoding='utf-8')
  else:assert OLD not in before and NEW in before,name
  assets.append(obj);r['assets'].append(obj.get_path_name())
 for obj in assets:
  if mode=='apply':assert u.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False);r['saved'].append(obj.get_path_name())
 r['complete']=True
finally:
 r['protected_changes']=[p for p,h in protected.items() if not Path(p).exists() or sha(Path(p))!=h]
 r['complete']=r['complete'] and not r['protected_changes'];out.write_text(json.dumps(r,ensure_ascii=False,indent=2),encoding='utf-8');print('FEEDBACK_SOUND_LINK '+json.dumps(r))
