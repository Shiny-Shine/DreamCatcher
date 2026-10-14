"""After USER build: inspect R13 native registrations/defaults without loading UI assets or saving settings.
-DCR13NativeRun=<fresh receipt>, separate unattended Python commandlet.
"""
import json,re,hashlib
from pathlib import Path
import unreal as ue

P=Path(__file__).resolve().parents[2]
cmd=ue.SystemLibrary.get_command_line()
run=re.search(r'-DCR13NativeRun=([A-Za-z0-9_]+)',cmd).group(1)
out=P/'Saved/Diagnostics/R13'/('native_verify_'+run+'.json');assert not out.exists()
assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
manifest=json.loads((P/'Saved/Diagnostics/R13/native_applied_A.json').read_text(encoding='utf-8'))
notification_scope='-DCR13NativeScope=notification' in cmd
rows=manifest['new'].values()
if notification_scope:
    rows=json.loads((P/'Saved/Diagnostics/R13/notification_native_A.json').read_text(encoding='utf-8'))['files']
classes=set()
structs=set()
for row in rows:
    if not row['target'].endswith('.h'):continue
    text=(P/row['target']).read_text(encoding='utf-8')
    for name in re.findall(r'UCLASS\([^\n]*\)\s*class\s+(?:(?:[A-Z_]+_API)\s+)?([UA]\w+)',text):
        classes.add('/Script/DreamCatcher.'+name[1:])
    if notification_scope:
        for name in re.findall(r'USTRUCT\([^\n]*\)\s*struct\s+(F\w+)',text):
            structs.add('/Script/DreamCatcher.'+name[1:])
if not notification_scope:
    classes.update('/Script/DreamCatcher.'+name for name in ['DCStageUIComponent','DCStageOverlayWidget','DCStageResultLayer','DCStageUIBlueprintLibrary'])
    classes.update('/Script/GameSettings.'+name for name in ['GameSettingRegistry','GameSettingScreen','GameSettingPanel','GameSettingValueScalarDynamic'])
files=[p for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI'] for p in (P/root).rglob('*') if p.is_file()]
before={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
loaded=[]
for name in sorted(classes):
    value=ue.load_class(None,name);assert value,'User build/native registration missing: '+name
    loaded.append(value.get_path_name())
for name in sorted(structs):
    assert ue.load_object(None,name),'Notification struct registration missing: '+name
mode=ue.get_default_object(ue.load_class(None,'/Script/DreamCatcher.DCStageRunGameMode'))
assert not mode.get_editor_property('bUseExperienceUI'),'Existing R11/R12 default changed'
assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()==0
changed=[p for p,h in before.items() if hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h];assert not changed
out.write_text(json.dumps(dict(complete=True,classes=loaded,structs=sorted(structs),notification_scope=notification_scope,protected_changed=changed,asset_saves=0,
    settings_saves=0,PIE=False,menus_or_feature_activation_verified=False),indent=2),encoding='utf-8')
print('R13_NATIVE_VERIFY '+str(dict(complete=True,classes=len(loaded),asset_saves=0,settings_saves=0)))
