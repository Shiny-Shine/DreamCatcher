"""One R13 HUD asset: give the stage/boss extension its full canvas allocation."""
import hashlib,json,sys,traceback
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
sys.dont_write_bytecode=True
sys.path.insert(0,str(P/'Scripts/Editor'))
from r7_root_ui_setup import snapshot
from r10_lyra_ai_map import clean
PATH='/Game/LyraMigration/UI/R13/ShooterCore/UserInterface/W_ShooterHUDLayout'
verify='-DCR13StatusVerify' in ue.SystemLibrary.get_command_line()
target=P/'Content'/(PATH[6:]+'.uasset');out=R/('status_layout_verify_A.json' if verify else 'status_layout_fix_B.json')
assert not out.exists()
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
baseline=json.loads((R/'integration_summary_B.json').read_text(encoding='utf-8'))['asset_hashes']
expected=json.loads((R/'status_layout_fix_B.json').read_text(encoding='utf-8'))['after_sha256'] if verify else baseline[str(target)]
assert digest(target)==expected
paths={Path(f) for f in json.loads((R/'postbuild_before_C.json').read_text(encoding='utf-8'))['files']}
paths.update(Path(f) for f in baseline)
protected={str(f):digest(f) for f in paths if f!=target}
result=dict(complete=False,asset=PATH,asset_saves=0,before_sha256=digest(target),protected=protected)
def properties(widget):
 return {n:str(widget.get_editor_property(n)) for n in ['EntryBoxType','EntryHorizontalAlignment','EntryVerticalAlignment','EntrySpacing','SpacingPattern']}
try:
 bp=ue.load_asset(PATH);assert bp
 before=snapshot(bp)
 extension=ue.find_object(None,bp.get_path_name()+':WidgetTree.ExtensionPoint_ModeStatus');assert extension
 result['before']=properties(extension)
 assert extension.get_editor_property('EntryBoxType')==ue.DynamicBoxType.OVERLAY
 assert len(extension.get_editor_property('SpacingPattern'))==(1 if verify else 0)
 slot=extension.get_editor_property('Slot')
 result['canvas_before']={n:slot.get_editor_property(n).export_text() if hasattr(slot.get_editor_property(n),'export_text') else str(slot.get_editor_property(n)) for n in ['LayoutData','bAutoSize','ZOrder']}
 if not verify:
  extension.set_editor_property('EntryHorizontalAlignment',ue.HorizontalAlignment.H_ALIGN_FILL)
  extension.set_editor_property('EntryVerticalAlignment',ue.VerticalAlignment.V_ALIGN_FILL)
  # UE Overlay otherwise overrides both alignments to Left/Top from EntrySpacing.
  # A zero spacing pattern keeps Fill and adds no padding, including future entries.
  extension.set_editor_property('SpacingPattern',[ue.Vector2D(0,0)])
 assert extension.get_editor_property('EntryHorizontalAlignment')==ue.HorizontalAlignment.H_ALIGN_FILL
 assert extension.get_editor_property('EntryVerticalAlignment')==ue.VerticalAlignment.V_ALIGN_FILL
 pattern=extension.get_editor_property('SpacingPattern');assert len(pattern)==1 and pattern[0].x==0 and pattern[0].y==0
 assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
 assert snapshot(bp)==before,'Unexpected graph/tree/default changes'
 result['after']=properties(extension)
 assert result['canvas_before']=={n:slot.get_editor_property(n).export_text() if hasattr(slot.get_editor_property(n),'export_text') else str(slot.get_editor_property(n)) for n in ['LayoutData','bAutoSize','ZOrder']}
 clean(P/'Saved/Logs'/('R13StatusLayout-Verify_A.log' if verify else 'R13StatusLayout-Fix_B.log'))
 if not verify:
  assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
  result['asset_saves']=1
 result['after_sha256']=digest(target);result['complete']=True
except Exception:
 result['failure']=traceback.format_exc();raise
finally:
 result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
 out.write_text(json.dumps(result,indent=2),encoding='utf-8')
 assert not result['protected_changed']
 print('R13_STATUS_LAYOUT_FIX '+str({k:v for k,v in result.items() if k!='protected'}))
