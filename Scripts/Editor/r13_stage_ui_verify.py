"""Reload the R13-only assembly and edited widgets without saving assets/settings."""
import json,re,hashlib,sys
from pathlib import Path
import unreal as ue
P=Path('E:/Epic Games/UEProjects/DreamCatcher');R=P/'Saved/Diagnostics/R13'
sys.dont_write_bytecode=True;sys.path.insert(0,str(P/'Scripts/Editor'))
import r13_stage_ui_setup as setup
from r10_enemy_setup import load,cdo,generated
from r11_stage_setup import environment
from r10_lyra_ai_map import clean
cmd=ue.SystemLibrary.get_command_line();run=re.search(r'-DCR13AssemblyVerify=([A-Za-z0-9_]+)',cmd).group(1)
out=R/('assembly_verify_'+run+'.json');assert not out.exists()
log=P/'Saved/Logs'/('R13AssemblyVerify-'+run+'.log')
protected={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for root in ['Source','Config','Content/DreamCatcher/GAS/Test','Content/LyraMigration/UI','Plugins/GameFeatures/DCStageUI'] for f in (P/root).rglob('*') if f.is_file()}
result=dict(complete=False,asset_saves=0,settings_saves=0)
try:
    assert ue.get_editor_subsystem(ue.LevelEditorSubsystem).load_level(setup.SOURCE_MAP)
    if not ue.PluginBlueprintLibrary.is_plugin_mounted('DCStageUI'):
        ue.SystemLibrary.execute_console_command(setup.world(),'PackageName.RegisterMountPoint /DCStageUI/ "'+str(P/'Plugins/GameFeatures/DCStageUI/Content').replace('\\','/')+'/"')
    values={k:load(v) for k,v in setup.PATHS.items() if k!='map'}
    for obj in values.values():
        if isinstance(obj,ue.Blueprint):assert ue.BlueprintEditorLibrary.compile_blueprint(obj)
    assert cdo(values['controller']).get_editor_property('WeaponReticleHostClass') is None
    assert cdo(values['mode']).get_editor_property('bUseExperienceUI')
    assert not cdo(load(setup.SOURCE_MODE)).get_editor_property('bUseExperienceUI')
    assert cdo(values['mode']).get_editor_property('PlayerControllerClass')==generated(values['controller'])
    assert cdo(values['mode']).get_editor_property('DefaultExperience').export_text()=='DCLyraExperienceDefinition:B_DC_StageUIExperience'
    assert cdo(values['mode']).get_editor_property('ReturnMap')==cdo(load(setup.SOURCE_MODE)).get_editor_property('ReturnMap')
    exp=cdo(values['experience']);source=cdo(load(setup.FINITE))
    assert exp.get_editor_property('DefaultPawnData')==source.get_editor_property('DefaultPawnData')
    assert [x.export_text() for x in exp.get_editor_property('Actions')[0].get_editor_property('ComponentList')]==[x.export_text() for x in source.get_editor_property('Actions')[0].get_editor_property('ComponentList')]
    assert list(exp.get_editor_property('ActionSets'))==[values['hud'],values['notifications']]
    action=values['hud'].get_editor_property('Actions')[0]
    assert len(action.get_editor_property('Layout'))==1 and len(action.get_editor_property('Widgets'))==12
    result['hud_widgets']=[x.export_text() for x in action.get_editor_property('Widgets')]
    assert len(values['notifications'].get_editor_property('Actions')[0].get_editor_property('ComponentList'))==4
    assert len(values['feature'].get_editor_property('Actions'))==2
    menu=load(setup.MENU);assert ue.BlueprintEditorLibrary.compile_blueprint(menu)
    graph=ue.BlueprintGraphEditor.get_graph_editor_by_name(menu,'EventGraph')
    nodes={n.get_name():n for n in graph.list_all_nodes()}
    assert 'K2Node_CallFunction_7' not in nodes
    calls=[n for n in nodes.values() if ue.BlueprintEditorLibrary.get_node_title(n).lower().replace(' ','')=='returnfromstagemenu']
    assert len(calls)==1,[ue.BlueprintEditorLibrary.get_node_title(n) for n in nodes.values()]
    assert len(nodes['K2Node_SwitchEnum_0'].find_output_pin('Confirmed').list_connected_pins())==1
    layout=load(setup.LAYOUT);assert ue.BlueprintEditorLibrary.compile_blueprint(layout)
    extension=ue.find_object(None,layout.get_path_name()+':WidgetTree.ExtensionPoint_ModeStatus');assert extension
    assert 'HUD.Slot.ModeStatus' in extension.get_editor_property('ExtensionPointTag').export_text()
    slot=extension.get_editor_property('Slot');assert not slot.get_auto_size()
    result['mode_slot_layout']=slot.get_editor_property('LayoutData').export_text()
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.load_level(setup.SOURCE_MAP);env=environment()
    assert levels.load_level(setup.PATHS['map']);assert environment()==env
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    assert world.get_world_settings().get_editor_property('DefaultGameMode')==generated(values['mode'])
    result['environment_actor_count']=len(env)
    assert len(ue.GameplayStatics.get_all_actors_of_class(world,setup.cls('DCEnemySpawner')))==3
    clean(log);result['complete']=True
finally:
    result['protected_changed']=[f for f,h in protected.items() if hashlib.sha256(Path(f).read_bytes()).hexdigest()!=h]
    out.write_text(json.dumps(result,indent=2),encoding='utf-8');assert not result['protected_changed']
    print('R13_ASSEMBLY_VERIFY '+str({k:result[k] for k in ['complete','protected_changed','asset_saves']}))
