"""One-time original Hero inventory graph extraction into a new isolated Pawn.

Uses Editor duplication/removal/reparent APIs. The original Hero is never saved.
Only AddInitialInventory, ClearInventory, WaitForInventoryReadiness and their
original possession/death/reset callers are retained in this new test asset.
"""
import hashlib,json,re,sys,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];S=P.parent/'LyraStarterGame'
sys.path.insert(0,str(P/'Scripts/Editor'))
import r7_root_ui_setup as reader
from r7_quickslot_transfer_probe import live_snapshot
SOURCE='/ShooterCore/Game/B_Hero_ShooterMannequin'
DEST='/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraInventoryPawn'
KEEP_GRAPHS={'EventGraph','AddInitialInventory','ClearInventory','WaitForInventoryReadiness'}
KEEP_NODES={'K2Node_Event_2','K2Node_MacroInstance_0','K2Node_AsyncAction_2',
            'K2Node_MacroInstance_4','K2Node_CallFunction_37',
            'K2Node_Event_3','K2Node_Event_4','K2Node_CallFunction_47'}
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
def canonical(snapshot):
    graphs=[]
    for graph in snapshot['graphs']:
        if graph['name'] not in KEEP_GRAPHS:continue
        row=dict(graph)
        if graph['name']=='EventGraph':row['nodes']=[n for n in graph['nodes'] if n['name'] in KEEP_NODES]
        graphs.append(row)
    return sorted(graphs,key=lambda x:x['name'])
def main():
    cmd=ue.SystemLibrary.get_command_line()
    run=re.search(r'-DCR10InventoryRun=([A-Za-z0-9_]+)',cmd).group(1)
    mode=re.search(r'-DCR10Inventory=(memory|prepare)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==S.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    out=P/'Saved/Diagnostics/R10'/('inventory_source_'+run+'.json');assert not out.exists()
    reader.LOG=P/'Saved/Logs'/('R10InventorySource-'+run+'.log')
    protected={str(p):digest(p) for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test') for p in root.rglob('*') if p.is_file()}
    source_file=S/'Plugins/GameFeatures/ShooterCore/Content/Game/B_Hero_ShooterMannequin.uasset'
    protected[str(source_file)]=digest(source_file)
    result=dict(complete=False,saved=[],protected=protected)
    try:
        assets=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        ui_stage=json.loads((P/'Saved/Diagnostics/R10/inventory_ui_stage.json').read_text())
        assert ui_stage['path']==DEST and ui_stage['original_hash']==digest(source_file)
        assert assets.does_asset_exist(DEST),'Create the bounded UI working copy first'
        original=ue.load_asset(SOURCE);assert original
        before,_=live_snapshot(original,reader.snapshot(original))
        original_default=ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(original))
        result['original_initial_items']=[v.get_path_name() for v in original_default.get_editor_property('InitialInventoryItems')]
        copy=ue.load_asset(DEST);assert copy
        emote_interface=ue.load_class(None,'/Game/Audio/Sounds/Emotes/BI_EmoteSoundInterface.BI_EmoteSoundInterface_C')
        assert not ue.SystemLibrary.does_implement_interface(ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(copy)),emote_interface), 'Remove Emote interface in the new copy using Editor Class Settings, then Compile/save/close that Editor before continuing'
        result['removed_interfaces']=['BI_EmoteSoundInterface via Editor Class Settings UI']
        for graph in list(ue.BlueprintEditorLibrary.list_graphs(copy)):
            if graph.get_name() not in KEEP_GRAPHS:ue.BlueprintEditorLibrary.remove_graph(copy,graph)
            elif graph.get_name()=='EventGraph':
                editor=ue.BlueprintGraphEditor.get_graph_editor(graph)
                editor.remove_nodes([n for n in editor.list_all_nodes() if n.get_name() not in KEEP_NODES])
                for comment in editor.list_comment_nodes():editor.remove_comment_node(comment)
        subsystem=ue.get_engine_subsystem(ue.SubobjectDataSubsystem)
        handles=subsystem.k2_gather_subobject_data_for_blueprint(copy)
        lib=ue.SubobjectDataBlueprintFunctionLibrary
        remove=[]
        result['removed_local_components']=[]
        for handle in handles:
            data=lib.get_data(handle)
            obj=lib.get_object_for_blueprint(data,copy)
            if isinstance(obj,ue.ActorComponent) and not lib.is_native_component(data) and not lib.is_inherited_component(data):
                remove.append(handle);result['removed_local_components'].append(obj.get_name())
        if remove:result['deleted_component_count']=subsystem.delete_subobjects(handles[0],remove,copy)
        result['removed_unused_variables']=ue.BlueprintEditorLibrary.remove_unused_variables(copy)
        ue.BlueprintEditorLibrary.reparent_blueprint(copy,ue.load_class(None,'/Script/LyraGame.LyraCharacter'))
        assert ue.BlueprintEditorLibrary.compile_blueprint(copy)
        actual,_=live_snapshot(copy,reader.snapshot(copy))
        result['expected']=canonical(before);result['actual']=canonical(actual)
        assert result['expected']==result['actual'],'Selected original graph changed'
        result['all_graphs']=actual['graphs']
        extras=[g for g in actual['graphs'] if g['name'] not in KEEP_GRAPHS]
        # Unreal may retain/regenerate the construction entry graph on reparent.
        assert all(g['name']=='UserConstructionScript' and all(n['class'] in
            ('/Script/BlueprintGraph.K2Node_FunctionEntry','/Script/BlueprintGraph.K2Node_CallParentFunction')
            for n in g['nodes']) for g in extras), 'Unexpected retained gameplay graph'
        result['initial_items']=[v.get_path_name() for v in ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(copy)).get_editor_property('InitialInventoryItems')]
        assert result['initial_items']==result['original_initial_items']
        # This staging asset carries no foreign Pistol package into DreamCatcher.
        # The new AI-only target child will select the already verified rifle.
        ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(copy)).set_editor_property('InitialInventoryItems',[])
        result['staging_initial_items']=[]
        after,_=live_snapshot(original,reader.snapshot(original));assert after==before
        registry=ue.AssetRegistryHelpers.get_asset_registry()
        result['source_graph_unchanged']=True
        reader.clean()
        if mode=='prepare':
            assert assets.save_loaded_asset(copy,False)
            result['saved'].append(DEST)
            saved_file=S/'Content'/Path(DEST[6:]+'.uasset')
            registry.scan_files_synchronous([str(saved_file)],True)
            options=ue.AssetRegistryDependencyOptions(True,True,False,False,False)
            result['dependencies']=[str(p) for p in registry.get_dependencies(DEST,options)]
            result['saved_hash']=digest(saved_file)
            assert not any(p.startswith('/ShooterCore/') or 'BI_EmoteSoundInterface' in p for p in result['dependencies']), 'Unprepared original Hero dependency remains'
        result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        out.write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_INVENTORY_SOURCE '+json.dumps({k:result[k] for k in ('complete','saved','protected_changed')}))
if __name__=='__main__':main()
