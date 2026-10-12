"""Finish the approved new AI inventory Pawn after source UI preparation.

Only the new Pawn and this task's new PawnData may be saved. The existing rifle
player Pawn, Controller and PawnData stay untouched. Source/target assets are
transferred with Unreal Migrate, never ordinary binary file operations.
"""
import hashlib,json,re,sys,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];S=P.parent/'LyraStarterGame'
sys.path.insert(0,str(P/'Scripts/Editor'))
import r7_root_ui_setup as reader
from r7_quickslot_transfer_probe import live_snapshot
from r7_quickbar_target import normalize
from r10_lyra_inventory_prepare import DEST,SOURCE,canonical,digest,KEEP_GRAPHS,KEEP_NODES
from r10_enemy_setup import cdo,generated,load,components
ROOT='/Game/DreamCatcher/GAS/Test/R10/LyraAI'
DATA=ROOT+'/DA_DC_LyraAIPawn'
PARENT='/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn'
RIFLE='/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/ID_Rifle'
EVENTS={'K2Node_Event_2':('K2Node_MacroInstance_0','Execute'),
        'K2Node_Event_3':('K2Node_CallFunction_47','execute'),
        'K2Node_Event_4':('K2Node_CallFunction_47','execute')}

def mapped(value):
    value=normalize(value,{SOURCE:DEST,
        '/Script/LyraGame.LyraInventoryManagerComponent':'/Script/DreamCatcher.DCInventoryManagerComponent'})
    text=json.dumps(value,sort_keys=True)
    for old,new in (
        ('Lyra Inventory Manager Component','DCInventory Manager Component'),
        ('Lyra Inventory Item Definition','DCInventory Item Definition'),
        ('Async Action Experience Ready','DCLyra Async Action Experience Ready')):
        text=text.replace(old,new)
    return json.loads(text)

def restore_parent_templates(bp,parent):
    """Set only new child native templates to the already verified player's values."""
    fields={
        '/Script/Engine.SkeletalMeshComponent':('SkeletalMeshAsset','AnimClass','PhysicsAssetOverride','RelativeLocation','RelativeRotation','RelativeScale3D'),
        '/Script/Engine.CapsuleComponent':('CapsuleRadius','CapsuleHalfHeight'),
        '/Script/DreamCatcher.DCCharacterMovementComponent':('MaxWalkSpeed','MaxWalkSpeedCrouched','MaxAcceleration','BrakingDecelerationWalking','GroundFriction','BrakingFriction','BrakingFrictionFactor','bUseSeparateBrakingFriction','JumpZVelocity','AirControl','AirControlBoostMultiplier','AirControlBoostVelocityThreshold','GravityScale','RotationRate','bOrientRotationToMovement','bUseControllerDesiredRotation')}
    parent_components=components(parent)
    restored=[]
    for obj in components(bp):
        class_path=obj.get_class().get_path_name()
        if class_path not in fields:continue
        assert obj.get_path_name().startswith(DEST+'.'), 'Refuse inherited/shared template write: '+obj.get_path_name()
        source=next(c for c in parent_components if c.get_name()==obj.get_name() and c.get_class()==obj.get_class())
        for field in fields[class_path]:
            obj.set_editor_property(field,source.get_editor_property(field))
            assert obj.get_editor_property(field)==source.get_editor_property(field),field
        if isinstance(obj,ue.PrimitiveComponent):
            obj.set_collision_profile_name(source.get_collision_profile_name(),False)
            assert obj.get_collision_profile_name()==source.get_collision_profile_name()
        restored.append(dict(name=obj.get_name(),fields=list(fields[class_path])))
    for field in ('bUseControllerRotationYaw','bUseControllerRotationPitch','bUseControllerRotationRoll'):
        cdo(bp).set_editor_property(field,cdo(parent).get_editor_property(field))
    assert len(restored)==3,restored
    return restored

def add_bot_guards(bp):
    graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'EventGraph')
    nodes={n.get_name():n for n in graph.list_all_nodes()}
    assert set(nodes)==KEEP_NODES,'Expected untouched original lifecycle graph'
    bot=graph.add_call_function_node('/Script/Engine.Pawn.IsBotControlled')
    assert bot
    result=[]
    for event,(destination,pin_name) in EVENTS.items():
        source_pin=nodes[event].find_then_pin()
        dest_pin=nodes[destination].find_input_pin(pin_name)
        assert len(source_pin.list_connected_pins())==1
        assert source_pin.break_single_pin_link(dest_pin)
        branch=graph.add_branch_node();assert branch
        assert source_pin.try_create_connection(branch.find_execute_pin())
        assert bot.find_result_pin().try_create_connection(branch.find_condition_pin())
        assert branch.find_then_pin().try_create_connection(dest_pin)
        pos=nodes[event].get_node_pos();branch.set_node_pos(ue.IntPoint(pos.x+220,pos.y))
        result.append(dict(event=event,destination=destination,pin=pin_name,branch=branch.get_name()))
    # Keep this explanation in the recipe: AddCommentToNodes leaves an unset
    # protected NodeGuid in this engine version, so do not create editor comments.
    return result

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR10InventoryTarget=(migrate|apply|fix-comment|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR10InventoryRun=([A-Za-z0-9_]+)',cmd).group(1)
    source_run=re.search(r'-DCR10InventorySource=([A-Za-z0-9_]+)',cmd).group(1)
    project=Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    assert project==(S if mode=='migrate' else P).resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    prior=json.loads((P/'Saved/Diagnostics/R10'/('inventory_source_'+source_run+'.json')).read_text())
    assert prior['complete'] and prior['saved']==[DEST]
    out=P/'Saved/Diagnostics/R10'/('inventory_target_'+run+'.json');assert not out.exists()
    reader.LOG=P/'Saved/Logs'/('R10InventoryTarget-'+run+'.log')
    paths=[p for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI') for p in root.rglob('*') if p.is_file()]
    protected={str(p):digest(p) for p in paths}
    source_file=S/'Content'/Path(DEST[6:]+'.uasset')
    assert digest(source_file)==prior['saved_hash'],'Prepared source changed'
    protected[str(source_file)]=prior['saved_hash']
    protected.update({p:h for p,h in prior['protected'].items() if Path(p).is_relative_to(S)})
    pawn_file=P/'Content'/Path(DEST[6:]+'.uasset')
    data_file=P/'Content'/Path(DATA[6:]+'.uasset')
    if mode=='apply':
        migration_run=re.search(r'-DCR10InventoryMigration=([A-Za-z0-9_]+)',cmd).group(1)
        migration=json.loads((P/'Saved/Diagnostics/R10'/('inventory_target_'+migration_run+'.json')).read_text())
        assert migration['complete'] and migration['mode']=='migrate'
        assert digest(pawn_file)==migration['migrated_hash'],'New incoming Pawn changed after migration'
        assert digest(data_file)==prior['protected'][str(data_file)],'New map PawnData changed since source preparation'
        protected.pop(str(pawn_file));protected.pop(str(data_file))
    if mode=='fix-comment':
        applied=json.loads((P/'Saved/Diagnostics/R10/inventory_target_Apply_B.json').read_text())
        assert applied['complete'] and digest(pawn_file)==applied['pawn_hash'] and digest(data_file)==applied['data_hash']
        protected.pop(str(pawn_file))
    result=dict(complete=False,mode=mode,saved=[],protected=protected,PIE=False)
    try:
        editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
        if mode=='migrate':
            assert not pawn_file.exists()
            opts=ue.MigrationOptions();opts.set_editor_property('prompt',False)
            opts.set_editor_property('ignore_dependencies',True);opts.set_editor_property('asset_conflict',ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages([DEST],str(P/'Content'),opts)
            assert pawn_file.is_file()
            result['migrated_hash']=digest(pawn_file)
        else:
            bp=load(DEST);parent=load(PARENT);data=load(DATA)
            if mode=='apply':
                assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name()=='/Script/DreamCatcher.DreamCatcherCharacter'
                assert data.get_editor_property('PawnClass')==generated(parent)
                ue.BlueprintEditorLibrary.reparent_blueprint(bp,generated(parent))
                result['restored_parent_templates']=restore_parent_templates(bp,parent)
                assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
                before,_=live_snapshot(bp,reader.snapshot(bp))
                result['before_adapter']=canonical(before)
                assert mapped(prior['expected'])==result['before_adapter'],'Original selected graph mismatch after type mapping'
                result['bot_guards']=add_bot_guards(bp)
                cdo(bp).set_editor_property('InitialInventoryItems',[generated(load(RIFLE))])
                assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
                after,_=live_snapshot(bp,reader.snapshot(bp))
                result['after_adapter']=canonical(after)
                expected=[g for g in mapped(prior['expected']) if g['name']!='EventGraph']
                actual=[g for g in result['after_adapter'] if g['name']!='EventGraph']
                assert expected==actual,'Inventory functions or readiness macro changed'
                reader.clean()
                assert editor.save_loaded_asset(bp,False);result['saved'].append(DEST)
                data.set_editor_property('PawnClass',generated(bp))
                assert editor.save_loaded_asset(data,False);result['saved'].append(DATA)
            else:
                assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp)==generated(parent)
                assert data.get_editor_property('PawnClass')==generated(bp)
                assert list(cdo(bp).get_editor_property('InitialInventoryItems'))==[generated(load(RIFLE))]
                if mode=='fix-comment':
                    graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(bp,'EventGraph')
                    comments=list(graph.list_comment_nodes())
                    assert len(comments)==1
                    assert comments[0].get_name()=='EdGraphNode_Comment_0'
                    before_fix,_=live_snapshot(bp,reader.snapshot(bp))
                    graph.remove_comment_node(comments[0])
                    after_fix,_=live_snapshot(bp,reader.snapshot(bp))
                    for g in before_fix['graphs']:
                        if g['name']=='EventGraph':g['nodes']=[n for n in g['nodes'] if n['class']!='/Script/UnrealEd.EdGraphNode_Comment']
                    assert before_fix==after_fix,'Runtime graph changed during comment removal'
                    result['comment_repair']='Removed the one optional generated editor comment; runtime nodes unchanged'
                assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
                actual,_=live_snapshot(bp,reader.snapshot(bp))
                assert [g for g in mapped(prior['expected']) if g['name']!='EventGraph']==[g for g in canonical(actual) if g['name']!='EventGraph']
                if mode=='fix-comment':
                    reader.clean()
                    assert editor.save_loaded_asset(bp,False);result['saved'].append(DEST)
            result['initial_items']=[v.get_path_name() for v in cdo(bp).get_editor_property('InitialInventoryItems')]
            result['parent']=ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name()
        reader.clean();result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        result['pawn_hash']=digest(pawn_file);result['data_hash']=digest(data_file)
        out.write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_INVENTORY_TARGET '+json.dumps({k:result[k] for k in ('complete','mode','saved','protected_changed')}))
if __name__=='__main__':main()
