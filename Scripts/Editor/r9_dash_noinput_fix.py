"""Approved idle-only forward bias in the imported Dash BP; all other assets protected."""
import json,math,re,sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r9_dash_assets as port
P,R=port.P,port.R
GA='/Game/LyraMigration/Dash/GA_Hero_Dash'
KEY='/ShooterCore/Game/Dash/GA_Hero_Dash'
FUNCTION='SelectDirectionalMontage'
ANGLES=(0,30,44,46,90,135,180,-90)

def function_results(bp):
    obj=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    result=[]
    for yaw in ANGLES:
        facing=ue.Vector(math.cos(math.radians(yaw)),math.sin(math.radians(yaw)),0)
        for input_yaw in (None,0,45,90,135,180,225,270,315):
            movement=ue.Vector() if input_yaw is None else ue.Vector(math.cos(math.radians(input_yaw)),math.sin(math.radians(input_yaw)),0)
            montage,bias=obj.call_method(FUNCTION,args=(facing,movement))
            result.append(dict(yaw=yaw,input_yaw=input_yaw,montage=montage.get_path_name(),bias=bias))
    return result

def pin(node,name):
    found=[p for p in ue.BlueprintEditorLibrary.list_all_pins(node) if str(p.get_pin_name())==name]
    assert len(found)==1,(node.get_name(),name)
    return found[0]

def original_graphs_without_links(snapshot,new_names):
    # Preserve every old node/default/type; strip only the four known old pins'
    # links for separate, exact connection validation below.
    result=json.loads(json.dumps(snapshot))
    for graph in result['graphs']:
        if graph['name']!=FUNCTION:continue
        graph['nodes']=[n for n in graph['nodes'] if n['name'] not in new_names]
        for node in graph['nodes']:
            for p in node['pins']:
                if (node['name'],p['name']) in {
                    ('K2Node_FunctionEntry_0','then'),('K2Node_FunctionEntry_0','MovementDirection'),
                    ('K2Node_IfThenElse_1','execute'),('K2Node_VariableSet_1','execute')}:
                    p['links']=[]
    return result

def verify_graph(before,after,names):
    assert original_graphs_without_links(before,[])==original_graphs_without_links(after,names.values()),'Original graph/default/type changed'
    g=next(g for g in after['graphs'] if g['name']==FUNCTION)
    nodes={n['name']:n for n in g['nodes']}
    zero,branch=names['zero'],names['branch']
    expected={
        ('K2Node_FunctionEntry_0','then'):[(branch,'execute')],
        ('K2Node_FunctionEntry_0','MovementDirection'):[('K2Node_CallFunction_12','InVec'),(zero,'A')],
        ('K2Node_IfThenElse_1','execute'):[(branch,'else')],
        ('K2Node_VariableSet_1','execute'):[('K2Node_IfThenElse_1','then'),(branch,'then')],
        (zero,'A'):[('K2Node_FunctionEntry_0','MovementDirection')],
        (zero,'ReturnValue'):[(branch,'Condition')],
        (branch,'execute'):[('K2Node_FunctionEntry_0','then')],
        (branch,'Condition'):[(zero,'ReturnValue')],
        (branch,'then'):[('K2Node_VariableSet_1','execute')],
        (branch,'else'):[('K2Node_IfThenElse_1','execute')]}
    for (node,name),pairs in expected.items():
        p=next(p for p in nodes[node]['pins'] if p['name']==name)
        assert sorted((l['node'],l['pin']) for l in p['links'])==sorted(pairs),(node,name)
    tolerance=next(p for p in nodes[zero]['pins'] if p['name']=='Tolerance')
    assert float(tolerance['value'])==0.0001 and not tolerance['links']

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR9IdleFix=(apply|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    port.helper.LOG=P/'Saved/Logs'/('R9Dash-'+run+'.log')
    out=R/('idle_fix_'+run);assert not out.exists();out.mkdir()
    baseline=json.loads((R/'assets_Verify_B/report.json').read_text());assert baseline['complete']
    all_paths=set(baseline['protected'])|set(json.loads((R/'integration_IntegrationVerify_A/report.json').read_text())['protected'])
    mutable={str(p) for p in port.files_helper.files(P,GA)} if mode=='apply' else set()
    protected={p:port.helper.digest(Path(p)) for p in all_paths if p not in mutable}
    result=dict(mode=mode,complete=False,asset_saves=0,protected=protected)
    try:
        bp=ue.load_asset(GA);assert bp
        ue.collect_garbage()
        before=port.data(bp,out,'before');result['before']=before
        if mode=='apply':
            assert port.comparable(before)==port.comparable(baseline['actual_data'][KEY]),'User/source BP differs; review before patching'
            result['function_before']=function_results(bp)
            editor=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,FUNCTION)
            assert editor
            nodes={n.get_name():n for n in editor.list_all_nodes()}
            entry,old_branch,forward=[nodes[n] for n in ('K2Node_FunctionEntry_0','K2Node_IfThenElse_1','K2Node_VariableSet_1')]
            assert [p.get_owning_node() for p in pin(entry,'then').list_connected_pins()]==[old_branch]
            zero=editor.add_call_function_node('/Script/Engine.KismetMathLibrary.Vector_IsNearlyZero')
            branch=editor.add_branch_node();assert zero and branch
            position=entry.get_node_pos()
            zero.set_node_pos(ue.IntPoint(position.x+40,position.y-340))
            branch.set_node_pos(ue.IntPoint(position.x+330,position.y-150))
            assert pin(zero,'Tolerance').set_pin_value('0.000100')
            assert pin(entry,'then').break_single_pin_link(pin(old_branch,'execute'))
            for a,b in ((pin(entry,'then'),pin(branch,'execute')),
                        (pin(entry,'MovementDirection'),pin(zero,'A')),
                        (pin(zero,'ReturnValue'),pin(branch,'Condition')),
                        (pin(branch,'then'),pin(forward,'execute')),
                        (pin(branch,'else'),pin(old_branch,'execute'))):
                assert a.try_create_connection(b)
            result['names']=dict(zero=zero.get_name(),branch=branch.get_name())
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            ue.collect_garbage()
            after=port.data(bp,out,'after');result['after']=after
            assert {k:port.comparable(v) for k,v in before.items() if k!='snapshot'}=={k:port.comparable(v) for k,v in after.items() if k!='snapshot'},'Unapproved ability settings changed'
            verify_graph(before['snapshot'],after['snapshot'],result['names'])
        else:
            applied=json.loads((R/'idle_fix_IdleApply_A/report.json').read_text());assert applied['complete']
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            ue.collect_garbage()
            after=port.data(bp,out,'after');result['after']=after
            assert port.comparable(after)==port.comparable(applied['after']), 'Fresh reload differs'
            verify_graph(applied['before']['snapshot'],after['snapshot'],applied['names'])
            result['function_before']=applied['function_before']
        result['function_after']=function_results(bp)
        for old,new in zip(result['function_before'],result['function_after']):
            if new['input_yaw'] is None:
                assert new['bias'] and new['montage']==GA.rsplit('/',1)[0]+'/AM_MM_Dash_Forward.AM_MM_Dash_Forward'
            else:assert old==new,'Nonzero input behavior changed'
        port.helper.clean()
        if mode=='apply':
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
            result['asset_saves']=1
        result['complete']=True
    finally:
        result['protected_unchanged']=all(port.helper.digest(Path(p))==h for p,h in protected.items())
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert result['protected_unchanged']
        print('R9_IDLE_FIX '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))

if __name__=='__main__':main()
