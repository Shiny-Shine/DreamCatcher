"""New R13 killfeed widget only: keep team0/1/2 visuals, handle NoTeam safely."""
import json,re,hashlib
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
PATH='/Game/LyraMigration/UI/R13/ShooterCore/UserInterface/Notifications/EliminationFeed/W_EliminationFeedEntryWidget'
cmd=ue.SystemLibrary.get_command_line();run=re.search(r'-DCR13ColorRun=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('killfeed_color_'+run+'.json');assert not out.exists()
target=P/'Content'/(PATH[6:]+'.uasset')
baseline=json.loads((R/'relocation_saved_A.json').read_text(encoding='utf-8'))['output_hashes']
digest=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
assert digest(target)==baseline[str(target)]
protected={str(f):digest(f) for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI'] for f in (P/root).rglob('*') if f.is_file() and f!=target}
bp=ue.load_asset(PATH);assert bp
graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'SetTeamColors')
nodes={n.get_name():n for n in graph.list_all_nodes()};changes=[]
for group in [('K2Node_Select_0','K2Node_Select_1'),('K2Node_Select_2',)]:
 clamp=graph.add_call_function_node('/Script/Engine.KismetMathLibrary.Clamp');assert clamp
 assert clamp.find_input_pin('Min').set_pin_value('0') and clamp.find_input_pin('Max').set_pin_value('2')
 for index,name in enumerate(group):
  node=nodes[name];assert [float(node.find_input_pin('Option '+str(i)).get_pin_value()) for i in range(3)]==[0,1,0]
  pin=node.find_input_pin('Index');links=list(pin.list_connected_pins());assert len(links)==1
  assert pin.break_single_pin_link(links[0])
  if index==0:assert links[0].try_create_connection(clamp.find_input_pin('Value'))
  assert clamp.find_result_pin().try_create_connection(pin);changes.append(name)
assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()==0
assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
changed=[f for f,h in protected.items() if digest(Path(f))!=h];assert not changed
out.write_text(json.dumps({'complete':True,'asset_saves':1,'selectors':changes,'known_team_values_unchanged':True,'gameplay_team_or_message_changed':False,'protected_changed':changed,'after_sha256':digest(target)},indent=2),encoding='utf-8')
print('R13_KILLFEED_COLOR_COMPLETE')
