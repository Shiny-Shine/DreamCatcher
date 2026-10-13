"""Only the new R13 killfeed relay: preserve player names; tolerate no PlayerState.
No original/shared assets, C++ builds, gameplay rules or elimination messages changed.
"""
import json,re,sys,hashlib,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R13'
sys.path.insert(0,str(P/'Scripts/Editor'))
from r10_lyra_ai_map import clean
import r7_root_ui_setup as reader
ASSET='/Game/LyraMigration/UI/R13/ShooterCore/Accolades/B_EliminationFeedRelay'
def digest(f):return hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
def connect(a,b):assert a.try_create_connection(b)
def main():
 cmd=ue.SystemLibrary.get_command_line();mode=re.search(r'-DCR13Killfeed=(apply|verify)',cmd).group(1)
 run=re.search(r'-DCR13KillfeedRun=([A-Za-z0-9_]+)',cmd).group(1)
 out=R/('killfeed_'+run+'.json');assert not out.exists()
 log=P/'Saved/Logs'/('R13Killfeed-'+run+'.log')
 target=P/'Content'/(ASSET[6:]+'.uasset')
 protected={str(f):digest(f) for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Plugins/GameFeatures/DCStageUI'] for f in (P/root).rglob('*') if f.is_file() and f!=target}
 result=dict(complete=False,mode=mode,asset_saves=0,before_sha256=digest(target))
 try:
  bp=ue.load_asset(ASSET);assert bp
  if mode=='apply':
   baseline=json.loads((R/'relocation_saved_A.json').read_text(encoding='utf-8'))['output_hashes'];assert digest(target)==baseline[str(target)]
   result['before_graphs']=reader.snapshot(bp)
   graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'EventGraph');nodes={n.get_name():n for n in graph.list_all_nodes()}
   getters=[nodes[x] for x in ['K2Node_CallFunction_3','K2Node_CallFunction_5']]
   assert all(ue.BlueprintEditorLibrary.get_node_title(n)=='GetPlayerName' for n in getters)
   object_type=getters[0].find_input_pin('self').get_pin_type()
   string_type=getters[0].find_result_pin().get_pin_type()
   fn=ue.BlueprintGraphEditor.create_and_edit_function_graph(bp,'GetSafeFeedName')
   subject=fn.add_graph_input_parameter('Subject',object_type)
   ret=fn.add_graph_output_parameter('ReturnValue',string_type)
   assert fn.add_local_variable('SafeName',string_type,'Enemy')
   valid=fn.add_call_function_node('/Script/Engine.KismetSystemLibrary.IsValid')
   branch=fn.add_branch_node()
   name=fn.add_call_function_node('/Script/Engine.PlayerState.GetPlayerName')
   setter=fn.add_set_local_variable_node('SafeName');getter=fn.add_get_local_variable_node('SafeName')
   connect(subject,valid.find_input_pin('Object'));connect(subject,name.find_input_pin('self'))
   connect(fn.find_graph_entry_pin(),branch.find_execute_pin());connect(valid.find_result_pin(),branch.find_condition_pin())
   connect(branch.find_then_pin(),setter.find_execute_pin());connect(name.find_result_pin(),setter.find_input_pin('SafeName'))
   connect(setter.find_then_pin(),ret.find_execute_pin());connect(branch.find_else_pin(),ret.find_execute_pin())
   connect(getter.find_output_pin('SafeName'),ret.find_input_pin('ReturnValue'))
   assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
   fn.set_is_pure_function(True);assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
   function_path=ue.BlueprintEditorLibrary.generated_class(bp).get_path_name()+'.GetSafeFeedName'
   for old in getters:
    call=graph.add_call_function_node(function_path);assert call
    call.set_node_pos(old.get_node_pos())
    links=old.find_input_pin('self').list_connected_pins();assert len(links)==1
    connect(links[0],call.find_input_pin('Subject'))
    for dest in list(old.find_result_pin().list_connected_pins()):
     assert old.find_result_pin().break_single_pin_link(dest);connect(call.find_result_pin(),dest)
   graph.remove_nodes(getters)
   payload=nodes['K2Node_BreakStruct_0']
   for name,field in [('K2Node_CallFunction_10','Instigator'),('K2Node_CallFunction_9','Target')]:
    pin=nodes[name].find_input_pin('TestActor')
    for old in list(pin.list_connected_pins()):assert pin.break_single_pin_link(old)
    connect(payload.find_output_pin(field),pin)
   assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
   default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
   assert default.call_method('GetSafeFeedName',args=(None,))=='Enemy'
   clean(log);assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False);result['asset_saves']=1
  else:
   prior=json.loads((R/'killfeed_Apply_A.json').read_text(encoding='utf-8'));assert prior['complete'] and digest(target)==prior['after_sha256']
   assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
   default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
   assert default.call_method('GetSafeFeedName',args=(None,))=='Enemy'
  result['after_graphs']=reader.snapshot(bp)
  clean(log);result['complete']=True
 except Exception:result['failure']=traceback.format_exc();raise
 finally:
  result['after_sha256']=digest(target);result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
  out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8');assert not result['protected_changed']
  print('R13_KILLFEED '+str({k:result[k] for k in ['complete','mode','asset_saves','protected_changed']}))
if __name__=='__main__':main()
