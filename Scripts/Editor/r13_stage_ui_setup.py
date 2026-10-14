"""R13-only HUD/menu editing and separate stage assembly. Unreal APIs only.

Run edit-ui, create, verify in fresh processes. Existing R11/R12 assets stay read-only.
Use a full unattended editor for edit-ui/create (DataRegistry engine subsystem).
"""
import json,re,sys,hashlib,traceback
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
P=Path(__file__).resolve().parents[2];R=P/'Saved/Diagnostics/R13'
sys.path.insert(0,str(P/'Scripts/Editor'))
from r10_enemy_setup import load,cdo,generated
from r9_signature_foundation import blueprint
from r11_stage_setup import environment
from r10_lyra_ai_map import clean,component_entry
import r13_ui_transfer as transfer

ROOT='/Game/DreamCatcher/GAS/Test/R13'
UI='/Game/LyraMigration/UI/R13'
PATHS={k:ROOT+'/'+v for k,v in dict(controller='BP_DC_StageUIController',mode='BP_DC_StageUIGameMode',
    experience='B_DC_StageUIExperience',hud='LAS_DC_StageHUD',notifications='LAS_DC_StageNotifications',map='L_DC_Level1UI').items()}
PATHS['feature']='/DCStageUI/DCStageUI'
MENU=UI+'/Game/UI/Hud/W_LyraGameMenu'
LAYOUT=UI+'/ShooterCore/UserInterface/W_ShooterHUDLayout'
FINITE='/Game/DreamCatcher/GAS/Test/R10/LyraAI/FiniteEncounter/B_DC_LyraFiniteExperience'
SOURCE_MAP='/Game/DreamCatcher/GAS/Test/R12/L_DC_Level1Boss'
SOURCE_CONTROLLER='/Game/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraAIPlayerController'
SOURCE_MODE='/Game/DreamCatcher/GAS/Test/R11/BP_DC_StageRunGameMode'
OLD_RANDOM='C7725AA14BADE5EB7F9F54894DB8BE03'
NEW_RANDOM='5A9AD0844AAAEB75E9111BAC4F8BFFC3'

def cls(name):
    value=ue.load_class(None,'/Script/DreamCatcher.'+name);assert value,name
    return value
def world():return ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
def digest(f):return hashlib.sha256(f.read_bytes()).hexdigest() if f.is_file() else None
def object_path(asset):return asset.get_path_name().split('.')[0]
def tag(name):
    value=ue.GameplayTag();assert value.import_text('(TagName="'+name+'")');return value
def data(path,class_):
    factory=ue.DataAssetFactory();factory.set_editor_property('DataAssetClass',class_)
    value=ue.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/',1)[1],path.rsplit('/',1)[0],class_,factory)
    assert value,path;return value

def edit_ui(result,log):
    editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    menu=load(MENU);layout=load(LAYOUT)
    graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(menu,'EventGraph')
    nodes={n.get_name():n for n in graph.list_all_nodes()}
    old=nodes['K2Node_CallFunction_7'];assert ue.BlueprintEditorLibrary.get_node_title(old)=='Open Level (by Object Reference)'
    confirmed=nodes['K2Node_SwitchEnum_0'].find_output_pin('Confirmed')
    assert len(confirmed.list_connected_pins())==1
    call=graph.add_call_function_node('/Script/DreamCatcher.DCStageUIBlueprintLibrary.ReturnFromStageMenu');assert call
    call.set_node_pos(old.get_node_pos())
    assert confirmed.break_single_pin_link(old.find_execute_pin())
    assert confirmed.try_create_connection(call.find_execute_pin())
    graph.remove_nodes([old])
    assert ue.BlueprintEditorLibrary.compile_blueprint(menu)
    result['return_node']=call.get_name()
    assert cdo(menu).get_editor_property('FrontEndMapReference') is None
    # The original empty mode-status extension is intended for experience-specific
    # status. Reuse it instead of accessing UMG's protected widget GUID map.
    extension=ue.find_object(None,layout.get_path_name()+':WidgetTree.ExtensionPoint_ModeStatus');assert extension
    reticle=ue.find_object(None,layout.get_path_name()+':WidgetTree.ExtensionPoint_Reticle');assert reticle
    original_tag=extension.get_editor_property('ExtensionPointTag').export_text()
    result['mode_slot_tag_before']=original_tag
    extension.set_editor_property('ExtensionPointTag',tag('HUD.Slot.ModeStatus'))
    assert extension.get_editor_property('ExtensionPointTag').export_text()==tag('HUD.Slot.ModeStatus').export_text()
    extension.set_editor_property('EntryBoxType',reticle.get_editor_property('EntryBoxType'))
    extension.set_editor_property('EntryHorizontalAlignment',ue.HorizontalAlignment.H_ALIGN_FILL)
    extension.set_editor_property('EntryVerticalAlignment',ue.VerticalAlignment.V_ALIGN_FILL)
    # Without a pattern, UE's Overlay overrides entry alignment to Left/Top.
    extension.set_editor_property('SpacingPattern',[ue.Vector2D(0,0)])
    extension.set_visibility(ue.SlateVisibility.HIT_TEST_INVISIBLE)
    slot=extension.get_editor_property('Slot');assert isinstance(slot,ue.CanvasPanelSlot)
    slot.set_anchors(ue.Anchors(ue.Vector2D(0,0),ue.Vector2D(1,1)))
    slot.set_offsets(ue.Margin(0,0,0,0));slot.set_alignment(ue.Vector2D(0,0));slot.set_auto_size(False);slot.set_z_order(20)
    assert ue.BlueprintEditorLibrary.compile_blueprint(layout)
    clean(log)
    for obj in [menu,layout]:
        assert editor.save_loaded_asset(obj,False);result['saved'].append(object_path(obj))

    # Retain the previously integrated shared sound. Adjust only new preset
    # references to its observed root graph identity, then refresh engine caches.
    changed=[];documents={}
    for original,dest in transfer.MAPPING.items():
        if '/Audio/' not in original:continue
        obj=load(dest)
        if not isinstance(obj,(ue.MetaSoundSource,ue.MetaSoundPatch)):continue
        prop='RootMetaSoundDocument' if isinstance(obj,ue.MetaSoundPatch) else 'RootMetasoundDocument'
        document=obj.get_editor_property(prop);text=document.export_text()
        if OLD_RANDOM not in text:continue
        updated=text.replace(OLD_RANDOM,NEW_RANDOM)
        assert document.import_text(updated);obj.set_editor_property(prop,document)
        changed.append((obj,prop,updated))
    for obj,prop,expected in changed:
        ue.get_editor_subsystem(ue.AssetEditorSubsystem).open_editor_for_assets([obj])
        document=obj.get_editor_property(prop)
        if document.export_text()!=expected:
            assert document.import_text(expected);obj.set_editor_property(prop,document)
        assert obj.get_editor_property(prop).export_text()==expected
        clean(log);assert editor.save_loaded_asset(obj,False)
        result['saved'].append(object_path(obj));documents[object_path(obj)]=expected
    result['audio_reference_documents']=documents

def create_stage(result,log):
    assets=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    # Public engine mount command, scoped to this approved content-only plugin.
    # It is process-local; the next process uses the normal GameFeature lifecycle.
    if not ue.PluginBlueprintLibrary.is_plugin_mounted('DCStageUI'):
        ue.SystemLibrary.execute_console_command(world(),'PackageName.RegisterMountPoint /DCStageUI/ "'+str(P/'Plugins/GameFeatures/DCStageUI/Content').replace('\\','/')+'/"')
    assert all(not assets.does_asset_exist(p) for p in PATHS.values()),'Fresh R13 assembly required'
    controller=blueprint(PATHS['controller'],generated(load(SOURCE_CONTROLLER)))
    cdo(controller).set_editor_property('WeaponReticleHostClass',None)
    assert ue.BlueprintEditorLibrary.compile_blueprint(controller)
    mode=blueprint(PATHS['mode'],generated(load(SOURCE_MODE)))
    cdo(mode).set_editor_property('PlayerControllerClass',generated(controller))
    cdo(mode).set_editor_property('bUseExperienceUI',True)
    experience=assets.duplicate_loaded_asset(load(FINITE),PATHS['experience']);assert experience
    hud=assets.duplicate_loaded_asset(load(UI+'/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD'),PATHS['hud']);assert hud
    notifications=assets.duplicate_loaded_asset(load(UI+'/ShooterCore/Accolades/EAS_BasicShooterAcolades'),PATHS['notifications']);assert notifications
    hud.set_editor_property('GameFeaturesToEnable',['DCStageUI'])
    actions=hud.get_editor_property('Actions');assert len(actions)==1
    widgets=actions[0].get_editor_property('Widgets');assert len(widgets)==11
    layouts=actions[0].get_editor_property('Layout');assert len(layouts)==1
    widgets.resize(12)
    extra=widgets[11];assert extra.import_text('(WidgetClass="/Script/DreamCatcher.DCStageOverlayWidget",SlotID=(TagName="HUD.Slot.ModeStatus"))')
    widgets[11]=extra;actions[0].set_editor_property('Widgets',widgets)
    cdo(experience).set_editor_property('ActionSets',[hud,notifications])
    cdo(experience).set_editor_property('GameFeaturesToEnable',['DCStageUI'])
    assert ue.BlueprintEditorLibrary.compile_blueprint(experience)
    expid=cdo(mode).get_editor_property('DefaultExperience')
    assert expid.import_text('DCLyraExperienceDefinition:B_DC_StageUIExperience')
    cdo(mode).set_editor_property('DefaultExperience',expid)
    assert ue.BlueprintEditorLibrary.compile_blueprint(mode)
    feature=data(PATHS['feature'],ue.load_class(None,'/Script/GameFeatures.GameFeatureData'))
    add=ue.new_object(ue.load_class(None,'/Script/GameFeatures.GameFeatureAction_AddComponents'),feature,'StageUIComponents')
    entries=[('/Script/DreamCatcher.DCLyraGameState',UI+'/ShooterCore/Accolades/B_EliminationFeedRelay.B_EliminationFeedRelay_C'),
        (generated(controller).get_path_name(),'/Script/DreamCatcher.DCStageUIComponent')]
    if cdo(controller).get_component_by_class(cls('DCLyraIndicatorManagerComponent')) is None:
        entries.append((generated(controller).get_path_name(),'/Script/DreamCatcher.DCLyraIndicatorManagerComponent'))
    rows=add.get_editor_property('ComponentList');rows.resize(len(entries))
    for i,row in enumerate(entries):rows[i]=component_entry(rows[i],*row)
    add.set_editor_property('ComponentList',rows)
    registry=ue.new_object(ue.load_class(None,'/Script/GameFeatures.GameFeatureAction_DataRegistry'),feature,'StageUIAccoladeRegistry')
    registry.set_editor_property('RegistriesToAdd',[load(UI+'/ShooterCore/Accolades/AccoladeDataRegistry')])
    registry.set_editor_property('bPreloadInEditor',True)
    feature.set_editor_property('Actions',[add,registry])
    values=dict(controller=controller,mode=mode,experience=experience,hud=hud,notifications=notifications,feature=feature)
    clean(log)
    for k,obj in values.items():
        assert assets.save_loaded_asset(obj,False);result['saved'].append(PATHS[k])
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.load_level(SOURCE_MAP);before=environment()
    assert levels.new_level_from_template(PATHS['map'],SOURCE_MAP);result['saved'].append(PATHS['map'])
    world().get_world_settings().set_editor_property('DefaultGameMode',generated(mode))
    assert environment()==before
    clean(log);assert levels.save_current_level()
    result['environment']=before
    result['feature_components']=[r.export_text() for r in rows]
    result['hud_widget_count']=len(widgets)

def main():
    cmd=ue.SystemLibrary.get_command_line();operation=re.search(r'-DCR13StageUI=(edit-ui|create)',cmd).group(1)
    run=re.search(r'-DCR13StageUIRun=([A-Za-z0-9_]+)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    out=R/('stage_ui_'+run+'.json');assert not out.exists()
    log=P/'Saved/Logs'/('R13StageUI-'+run+'.log')
    protected=json.loads((R/'postbuild_before_C.json').read_text(encoding='utf-8'))['files']
    result=dict(complete=False,operation=operation,saved=[],protected=protected)
    try:
        assert all(digest(Path(f))==h for f,h in protected.items())
        if operation=='edit-ui':edit_ui(result,log)
        else:create_stage(result,log)
        clean(log);result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc();raise
    finally:
        result['protected_changed']=[f for f,h in protected.items() if digest(Path(f))!=h]
        result['output_hashes']={str(f):digest(f) for root in ['Content/DreamCatcher/GAS/Test/R13','Content/LyraMigration/UI/R13','Plugins/GameFeatures/DCStageUI/Content'] for f in (P/root).rglob('*') if f.is_file()}
        out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R13_STAGE_UI '+str({k:result[k] for k in ('complete','operation','saved','protected_changed')}))

if __name__=='__main__':
    try:main()
    finally:ue.SystemLibrary.quit_editor()
