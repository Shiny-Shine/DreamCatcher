"""Create the approved isolated Lyra AI pickup test assembly via Unreal only.

This first map exercises empty-inventory pickup, not original Hero initial loadout.
No existing asset, C++ or Config is saved. Builds and packaging are user-owned.
"""
import hashlib
import json
import re
import sys
import traceback
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
P = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(P/'Scripts/Editor'))
from r9_signature_foundation import blueprint
from r10_enemy_setup import cdo, generated, load

ROOT = '/Game/DreamCatcher/GAS/Test/R10/LyraAI'
OLD = '/Game/DreamCatcher/GAS/Test/R5/Integration'
AI = '/Game/LyraMigration/AI/ShooterCore'
PATHS = dict(controller=ROOT+'/BP_DC_LyraAIPlayerController', data=ROOT+'/DA_DC_LyraAIPawn',
             experience=ROOT+'/B_DC_LyraAIExperience', mode=ROOT+'/BP_DC_LyraAIGameMode',
             map=ROOT+'/L_DC_LyraAIIntegration')

def cls(name):
    value=ue.load_class(None, '/Script/DreamCatcher.'+name)
    assert value,name
    return value

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None

def component_entry(entry, actor, component):
    assert entry.import_text('(ActorClass="'+actor+'",ComponentClass="'+component+'",bClientComponent=True,bServerComponent=True,AdditionFlags=0)')
    return entry

def assemble():
    assets=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    assert all(not assets.does_asset_exist(p) for p in PATHS.values()), 'Fresh paths required'
    controller=assets.duplicate_loaded_asset(load(OLD+'/BP_DC_RifleController'),PATHS['controller'])
    ue.BlueprintEditorLibrary.reparent_blueprint(controller,cls('DCLyraAIIntegrationController'))
    assert ue.BlueprintEditorLibrary.compile_blueprint(controller)
    for field in ('StartingRifle','WeaponReticleHostClass','DefaultMappingContexts'):
        assert cdo(controller).get_editor_property(field)==cdo(load(OLD+'/BP_DC_RifleController')).get_editor_property(field),field
    data=assets.duplicate_loaded_asset(load(OLD+'/DA_DC_RiflePawn'),PATHS['data'])
    experience=blueprint(PATHS['experience'],cls('DCLyraExperienceDefinition'))
    default=cdo(experience)
    default.set_editor_property('DefaultPawnData',data)
    action=ue.new_object(ue.load_class(None,'/Script/GameFeatures.GameFeatureAction_AddComponents'),default,'LyraAITestComponents')
    entries=[
        ('/Script/DreamCatcher.DCLyraPlayerBotController','/Script/DreamCatcher.DCInventoryManagerComponent'),
        ('/Script/DreamCatcher.DCLyraPlayerBotController','/Script/DreamCatcher.DCLyraWeaponStateComponent'),
        ('/Script/DreamCatcher.DCLyraPlayerBotController',AI+'/Game/B_QuickBarComponent.B_QuickBarComponent_C'),
        ('/Script/DreamCatcher.DCLyraGameState',AI+'/Game/B_TeamSetup_TwoTeams.B_TeamSetup_TwoTeams_C'),
        ('/Script/DreamCatcher.DCLyraGameState',AI+'/Game/B_TeamSpawningRules.B_TeamSpawningRules_C'),
        ('/Script/DreamCatcher.DCLyraGameState',AI+'/Bot/B_ShooterBotSpawner.B_ShooterBotSpawner_C')]
    component_list=action.get_editor_property('ComponentList')
    component_list.resize(len(entries))
    for index,row in enumerate(entries):
        component_list[index]=component_entry(component_list[index],*row)
    action.set_editor_property('ComponentList',component_list)
    default.set_editor_property('Actions',[action])
    assert ue.BlueprintEditorLibrary.compile_blueprint(experience)
    mode=blueprint(PATHS['mode'],cls('DCLyraGameMode'))
    cdo(mode).set_editor_property('PlayerControllerClass',generated(controller))
    asset_id=cdo(mode).get_editor_property('DefaultExperience')
    assert asset_id.import_text('DCLyraExperienceDefinition:B_DC_LyraAIExperience')
    assert asset_id.export_text()=='DCLyraExperienceDefinition:B_DC_LyraAIExperience'
    cdo(mode).set_editor_property('DefaultExperience',asset_id)
    assert ue.BlueprintEditorLibrary.compile_blueprint(mode)
    return dict(controller=controller,data=data,experience=experience,mode=mode)

def make_map(objects,result,log,resume=False):
    levels=ue.get_editor_subsystem(ue.LevelEditorSubsystem)
    assert levels.load_level(PATHS['map']) if resume else levels.new_level(PATHS['map'])
    result['created'].append(PATHS['map'])
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('DefaultGameMode',generated(objects['mode']))
    actors=ue.get_editor_subsystem(ue.EditorActorSubsystem)
    def spawn(class_,pos,rotation=ue.Rotator(),label=None):
        actor=actors.spawn_actor_from_class(class_,ue.Vector(*pos),rotation)
        assert actor
        if label: actor.set_actor_label(label)
        return actor
    cube=load('/Engine/BasicShapes/Cube')
    floor=spawn(ue.StaticMeshActor,(0,0,-25),label='R10 AI Navigation Floor')
    floor.static_mesh_component.set_static_mesh(cube)
    floor.set_actor_scale3d(ue.Vector(40,30,.5))
    cover=spawn(ue.StaticMeshActor,(0,0,150),label='R10 AI Central Cover')
    cover.static_mesh_component.set_static_mesh(cube)
    cover.set_actor_scale3d(ue.Vector(3,8,3))
    spawn(ue.DirectionalLight,(0,0,600),ue.Rotator(-55,35,0),'R10 AI Sun')
    spawn(ue.SkyLight,(0,0,600),label='R10 AI Skylight')
    for i,(x,y) in enumerate(((-1300,-600),(1300,600),(-1300,600),(1300,-600))):
        spawn(cls('DCLyraPlayerStart'),(x,y,100),ue.Rotator(0,0 if x<0 else 180,0),'R10 AI Start '+str(i+1))
    pickup_class=generated(load(AI+'/Blueprint/B_WeaponSpawner'))
    pickup_data=load(AI+'/Weapons/Rifle/WeaponPickupData_Rifle')
    for i,(x,y) in enumerate(((-900,-600),(900,600),(-900,600),(900,-600))):
        actor=spawn(pickup_class,(x,y,0),label='R10 Original Rifle Pickup '+str(i+1))
        actor.set_editor_property('WeaponDefinition',pickup_data)
    volume=spawn(ue.NavMeshBoundsVolume,(0,0,200),label='R10 AI Navigation Bounds')
    volume.set_actor_scale3d(ue.Vector(20,15,5))
    origin,extent=volume.get_actor_bounds(False)
    assert extent.x>=1900 and extent.y>=1400 and extent.z>=400, str((origin,extent))
    nav=ue.NavigationSystemV1.get_navigation_system(world)
    assert nav,'Navigation system absent'
    nav.on_navigation_bounds_updated(volume)
    nav_data=list(ue.GameplayStatics.get_all_actors_of_class(world,ue.RecastNavMesh))
    if not nav_data:
        # Recast is NotPlaceable: the Editor placement factory rejects it.
        # Use Unreal's normal World spawn API, which is also used by NavigationSystem.
        statics=ue.get_default_object(ue.GameplayStatics.static_class())
        nav_actor=statics.call_method('BeginDeferredActorSpawnFromClass',args=(world,ue.RecastNavMesh.static_class(),ue.Transform(),ue.SpawnActorCollisionHandlingMethod.ALWAYS_SPAWN,None,ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))
        assert nav_actor,'Recast world spawn failed'
        nav_data=[statics.call_method('FinishSpawningActor',args=(nav_actor,ue.Transform(),ue.SpawnActorScaleMethod.MULTIPLY_WITH_ROOT))]
    for actor in nav_data:
        actor.set_editor_property('RuntimeGeneration',ue.RuntimeGenerationType.DYNAMIC)
    result['navigation']=dict(bounds=str(extent),data=[a.get_path_name() for a in nav_data],runtime_generation='Dynamic',pathfinding_verified=False)
    clean(log)
    assert levels.save_current_level()
    result['saved'].append(PATHS['map'])

def clean(log):
    assert ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count()==0
    errors=[line for line in log.read_text(encoding='utf-8-sig',errors='replace').splitlines()
            if re.search(r': Error:|Fatal:|Ensure condition failed',line)]
    assert not errors,errors[:3]

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR10AIMap=(apply|resume|bind|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR10MapRun=([A-Za-z0-9_]+)(?:\s|$)',cmd).group(1)
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    out=P/'Saved/Diagnostics/R10'/('map_'+run+'.json');assert not out.exists()
    log=P/'Saved/Logs'/('R10LyraAIMap-'+run+'.log')
    files=[p for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS/Test',P/'Content/LyraMigration/AI') for p in root.rglob('*') if p.is_file()]
    files += [P/'.git/index',P/'DreamCatcher.uproject',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    protected={str(p):digest(p) for p in files}
    if mode=='bind':
        prior=json.loads((P/'Saved/Diagnostics/R10/map_Resume_E.json').read_text())
        assert prior['complete'] and all(digest(Path(p))==h for p,h in prior['output_hashes'].items())
        editable=str(P/'Content/DreamCatcher/GAS/Test/R10/LyraAI/BP_DC_LyraAIGameMode.uasset')
        protected.pop(editable)
    if mode=='resume':
        previous=re.search(r'-DCR10Resume=([A-Za-z0-9_]+)(?:\s|$)',cmd).group(1)
        prior=json.loads((P/'Saved/Diagnostics/R10'/('map_'+previous+'.json')).read_text())
        assert not prior['complete'] and prior['saved']==[PATHS[k] for k in ('controller','data','experience','mode')]
        assert all(digest(Path(p))==h for p,h in prior['output_hashes'].items()),'Partial output changed externally'
        assert all(digest(Path(p))==h for p,h in prior['protected'].items()),'Protected file changed'
        protected=prior['protected']
    result=dict(complete=False,mode=mode,created=[],saved=[],protected=protected,initial_inventory='empty-inventory pickup test only; original Hero graph pending',PIE=False)
    try:
        if mode=='apply':
            assert not (P/'Content/DreamCatcher/GAS/Test/R10/LyraAI').exists()
            objects=assemble();clean(log)
            for key,obj in objects.items():
                assert obj.get_path_name().split('.')[0]==PATHS[key]
                assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(obj,False)
                result['created'].append(PATHS[key]);result['saved'].append(PATHS[key])
            out.write_text(json.dumps(result,indent=2),encoding='utf-8')
            make_map(objects,result,log)
        elif mode=='resume':
            objects={k:load(p) for k,p in PATHS.items() if k!='map'}
            make_map(objects,result,log,resume=True)
        else:
            objects={k:load(p) for k,p in PATHS.items() if k!='map'}
            assert ue.get_editor_subsystem(ue.LevelEditorSubsystem).load_level(PATHS['map'])
            if mode=='bind':
                asset_id=cdo(objects['mode']).get_editor_property('DefaultExperience')
                assert not asset_id.export_text()
                assert asset_id.import_text('DCLyraExperienceDefinition:B_DC_LyraAIExperience')
                assert asset_id.export_text()=='DCLyraExperienceDefinition:B_DC_LyraAIExperience'
                cdo(objects['mode']).set_editor_property('DefaultExperience',asset_id)
                assert ue.BlueprintEditorLibrary.compile_blueprint(objects['mode'])
                clean(log)
                assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(objects['mode'],False)
                result['saved'].append(PATHS['mode'])
        result['experience_actions']=[e.export_text() for e in cdo(objects['experience']).get_editor_property('Actions')[0].get_editor_property('ComponentList')]
        result['default_experience']=cdo(objects['mode']).get_editor_property('DefaultExperience').export_text()
        assert result['default_experience']=='DCLyraExperienceDefinition:B_DC_LyraAIExperience'
        result['player_pawn']=objects['data'].get_editor_property('PawnClass').get_path_name()
        world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
        result['actors']=[dict(name=a.get_actor_label(),cls=a.get_class().get_path_name()) for a in ue.get_editor_subsystem(ue.EditorActorSubsystem).get_all_level_actors()]
        assert world.get_world_settings().get_editor_property('DefaultGameMode')==generated(objects['mode'])
        clean(log)
        result['complete']=True
    except Exception:
        result['failure']=traceback.format_exc()
        raise
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        result['output_hashes']={str(p):digest(p) for p in (P/'Content/DreamCatcher/GAS/Test/R10/LyraAI').rglob('*') if p.is_file()}
        out.write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_AI_MAP '+json.dumps({k:result[k] for k in ('complete','mode','saved','protected_changed')}))

if __name__=='__main__':main()
