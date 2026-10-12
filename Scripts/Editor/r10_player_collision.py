"""Approved original collision profiles on exactly BP_DC_RiflePawn's two templates."""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r10_enemy_setup as helper
import r9_dash_assets as audit

P,R=helper.P,helper.R
PAWN='/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RiflePawn'
PROFILES={'CollisionCylinder':'LyraPawnCapsule','CharacterMesh0':'LyraPawnMesh'}
OLD={'CollisionCylinder':'Pawn','CharacterMesh0':'CharacterMesh'}
CHANNELS={'WorldStatic':0,'WorldDynamic':1,'Pawn':2,'Visibility':3,'Camera':4,'PhysicsBody':5,
          'Vehicle':6,'Destructible':7,'Weapon':15,'WeaponCapsule':16,'WeaponMulti':17}


def snapshot(bp):
    result=dict(defaults=audit.scope['reflected_fields'](helper.cdo(bp))['values'],graphs=helper.reader.snapshot(bp),components={})
    for obj in helper.components(bp):
        assert obj.get_path_name().startswith(PAWN+'.'), obj.get_path_name()
        result['components'][obj.get_name()]=dict(cls=obj.get_class().get_path_name(),values=audit.scope['reflected_fields'](obj)['values'])
    # Compile reallocates delegate storage. Preserve/compare the actual bound object/function names.
    health=result['components']['HealthComponent']['values']
    for name in ('on_death_started','on_death_finished'):
        health[name]=re.sub(r"(Multicast delegate 'DCLyraHealth_DeathEvent' \()0x[0-9A-Fa-f]+(\))",r'\1ADDRESS\2',health[name])
    return audit.comparable(result)


def without_collision(snapshot):
    value=json.loads(json.dumps(snapshot))
    for name in PROFILES:
        assert 'body_instance' in value['components'][name]['values']
        value['components'][name]['values'].pop('body_instance')
    return value


def responses(bp):
    result={}
    for obj in helper.components(bp):
        if obj.get_name() in PROFILES:
            result[obj.get_name()]=dict(profile=str(obj.get_collision_profile_name()),enabled=str(obj.get_collision_enabled()),
                channels={n:str(obj.get_collision_response_to_channel(ue.CollisionChannel.cast(index))) for n,index in CHANNELS.items()},
                body=obj.get_editor_property('BodyInstance').export_text())
    return result


def validate(bp):
    actual=responses(bp)
    expected={
        'CollisionCylinder':{'WorldStatic':2,'WorldDynamic':2,'Pawn':2,'Visibility':2,'Camera':0,'PhysicsBody':2,'Vehicle':2,'Destructible':0,'Weapon':0,'WeaponCapsule':2,'WeaponMulti':1},
        'CharacterMesh0':{'WorldStatic':0,'WorldDynamic':2,'Pawn':0,'Visibility':2,'Camera':0,'PhysicsBody':0,'Vehicle':0,'Destructible':0,'Weapon':2,'WeaponCapsule':0,'WeaponMulti':1}}
    for obj in helper.components(bp):
        name=obj.get_name()
        if name not in PROFILES: continue
        assert str(obj.get_collision_profile_name())==PROFILES[name]
        assert obj.get_collision_enabled()==ue.CollisionEnabled.QUERY_ONLY
        for channel,number in expected[name].items():
            assert obj.get_collision_response_to_channel(ue.CollisionChannel.cast(CHANNELS[channel]))==ue.CollisionResponseType.cast(number), (name,channel,actual[name])
    assert set(actual)==set(PROFILES)
    return actual


def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR10Collision=(apply|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR10Run=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    helper.LOG=P/'Saved/Logs'/('R10PlayerCollision-'+run+'.log')
    out=R/('collision_'+run)
    out.mkdir(parents=True,exist_ok=False)
    pattern=re.compile(r'^\+Profiles=\(Name="LyraPawn(?:Mesh|Capsule)"')
    source_lines=[line for line in (P.parent/'LyraStarterGame/Config/DefaultEngine.ini').read_text(encoding='utf-8-sig').splitlines() if pattern.match(line)]
    target_lines=[line for line in (P/'Config/DefaultEngine.ini').read_text(encoding='utf-8-sig').splitlines() if pattern.match(line)]
    assert len(source_lines)==len(target_lines)==2 and sorted(source_lines)==sorted(target_lines)
    mutable={(P/'Content'/PAWN[6:]).with_suffix(ext) for ext in ('.uasset','.uexp','.ubulk','.uptnl')}
    files=[p for root in (P/'Source',P/'Config',P/'Content/DreamCatcher/GAS',P/'Content/Blueprint/AI',P/'Content/Blueprint/Stage')
           for p in root.rglob('*') if p.is_file() and (mode=='verify' or p not in mutable)]
    files += [P/'DreamCatcher.uproject',P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll']
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    protected={str(p):digest(p) for p in files}
    result=dict(complete=False,mode=mode,asset_saves=[],profiles=target_lines,protected=protected)
    try:
        bp=helper.load(PAWN)
        before=snapshot(bp)
        result['before']=before
        result['before_responses']=responses(bp)
        if mode=='apply':
            for obj in helper.components(bp):
                name=obj.get_name()
                if name in PROFILES:
                    assert str(obj.get_collision_profile_name())==OLD[name], 'Refuse unexpected prior/user state'
                    obj.set_collision_profile_name(PROFILES[name])
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            result['after']=snapshot(bp)
            assert without_collision(before)==without_collision(result['after']), 'Non-collision template/default/graph changed'
        else:
            applied=re.search(r'-DCR10Applied=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
            baseline=json.loads((R/('collision_'+applied)/'report.json').read_text())
            assert baseline['complete'] and baseline['asset_saves']==[PAWN]
            assert before==baseline['after'], 'Stored Blueprint differs from approved save'
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            result['after']=snapshot(bp)
            assert before==result['after'], 'Reload/Compile changed defaults or graph'
        result['after_responses']=validate(bp)
        helper.clean()
        if mode=='apply':
            assert ue.get_editor_subsystem(ue.EditorAssetSubsystem).save_loaded_asset(bp,False)
            result['asset_saves'].append(PAWN)
        helper.clean()
        result['complete']=True
    finally:
        result['protected_changed']=[p for p,h in protected.items() if digest(Path(p))!=h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R10_PLAYER_COLLISION '+json.dumps({k:result[k] for k in ('complete','mode','asset_saves','protected_changed')}))


if __name__=='__main__': main()
