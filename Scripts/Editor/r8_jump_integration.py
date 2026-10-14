"""Approved integration-only Jump binding; preserves death and all other inputs.

-DCR8JumpIntegration=apply|verify -DCR8JumpRun=<fresh>
No C++ build, shared asset writes, gameplay-rule replacement or final HUD changes.
"""
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode=True
import unreal as ue
sys.path.insert(0,str(Path(__file__).resolve().parent))
import r8_jump_assets as port
import r5_rifle_player_setup as player
P,R=port.P,port.R
ROOT='/Game/DreamCatcher/GAS/Test/R5/Integration/'
PAWN,DATA,INPUT=[ROOT+n for n in ('BP_DC_RiflePawn','DA_DC_RiflePawn','DA_DC_RifleInput')]
SET=ROOT+'AS_DC_RifleJumpAndDeath'
IMC=ROOT+'IMC_DC_RifleBase'
OLDSET='/Game/DreamCatcher/GAS/Test/AS_DC_FoundationTest'
OLDIMC='/Game/Input/IMC_Player'
OLDACTION='/Game/Input/Actions/IA_Jump'
ACTION='/Game/LyraMigration/BaseInput/Actions/IA_Jump'
GA=port.ROOT+'GA_Hero_Jump'
OLDGA='/Game/DreamCatcher/GAS/Test/GA_DC_FoundationTest'

def rows(obj,field):return [v.export_text() for v in obj.get_editor_property(field)]
def hero(bp):
    _,objects=player.components(bp,editable=True)
    found=[o for _,o in objects if o.get_class().get_name()=='DCHeroComponent']
    assert len(found)==1 and found[0].get_path_name().startswith(PAWN+'.')
    return found[0]
def key_rows(obj):return obj.get_editor_property('DefaultKeyMappings').get_editor_property('Mappings')
def replace_struct(entry,old,new):
    # EditDefaultsOnly struct members reject per-instance Python assignment.
    # Reconstruct the one value, then assign the containing asset array.
    text=entry.export_text().replace(old,new)
    value=type(entry)();assert value.import_text(text)
    assert value.export_text()==text
    return value
def signature(bp,data,inputs,context,aset):
    return dict(graphs=port.helper.snapshot(bp),hero=rows(hero(bp),'DefaultInputMappings'),
        sets=[x.get_path_name() for x in data.get_editor_property('AbilitySets')],
        pawn_data=port.scope['reflected_fields'](data)['values'],
        native=rows(inputs,'NativeInputActions'),ability=rows(inputs,'AbilityInputActions'),
        mappings=[x.export_text() for x in key_rows(context)],
        grants={n:rows(aset,n) for n in ('GrantedGameplayAbilities','GrantedGameplayEffects','GrantedAttributes')})

def validate(after,before):
    assert after['graphs']==before['graphs'],'Pawn graph changed'
    assert after['native']==before['native'],'Other native input changed'
    assert after['hero']==[s.replace(OLDIMC+'.IMC_Player',IMC+'.IMC_DC_RifleBase') for s in before['hero']]
    assert after['sets']==[s.replace(OLDSET+'.AS_DC_FoundationTest',SET+'.AS_DC_RifleJumpAndDeath') for s in before['sets']]
    assert after['pawn_data']==json.loads(json.dumps(before['pawn_data']).replace(OLDSET+'.AS_DC_FoundationTest',SET+'.AS_DC_RifleJumpAndDeath'))
    assert after['ability']==[s.replace(OLDACTION+'.IA_Jump',ACTION+'.IA_Jump') for s in before['ability']]
    assert after['mappings']==[s.replace(OLDIMC+'.IMC_Player',IMC+'.IMC_DC_RifleBase').replace(OLDACTION+'.IA_Jump',ACTION+'.IA_Jump') for s in before['mappings']]
    expected={n:[s.replace(OLDGA+'.GA_DC_FoundationTest_C',GA+'.GA_Hero_Jump_C') for s in a] for n,a in before['grants'].items()}
    assert after['grants']==expected,'AbilitySet changed beyond Jump'
    assert sum('GA_Hero_Death' in s for s in after['grants']['GrantedGameplayAbilities'])==1

def main():
    cmd=ue.SystemLibrary.get_command_line()
    mode=re.search(r'-DCR8JumpIntegration=(apply|verify)(?:\s|$)',cmd).group(1)
    run=re.search(r'-DCR8JumpRun=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    log_prefix='R8Jump-' if mode=='apply' else 'R8Jump-Integration'
    port.helper.LOG=P/'Saved/Logs'/(log_prefix+run+'.log')
    out=R/('jump_integration_'+run);assert not out.exists();out.mkdir()
    baseline=json.loads((R/'jump_assets_Verify_A/report.json').read_text());assert baseline['complete']
    mutable=set(str(p) for package in (PAWN,DATA,INPUT) for p in port.files_helper.files(P,package))
    protected={p:port.helper.digest(Path(p)) for p in baseline['protected'] if p not in mutable}
    for package in (OLDSET,OLDIMC,OLDACTION,ACTION):
        for p in port.files_helper.files(P,package):protected[str(p)]=port.helper.digest(p)
    if mode=='verify':
        for package in (PAWN,DATA,INPUT,SET,IMC):
            for p in port.files_helper.files(P,package):protected[str(p)]=port.helper.digest(p)
    result=dict(mode=mode,complete=False,asset_saves=0,protected=protected)
    editor=ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    try:
        bp,data,inputs=[port.load(p) for p in (PAWN,DATA,INPUT)]
        original_set,original_context=port.load(OLDSET),port.load(OLDIMC)
        unchanged_set={n:rows(original_set,n) for n in ('GrantedGameplayAbilities','GrantedGameplayEffects','GrantedAttributes')}
        if mode=='apply':
            assert all(not p.exists() for package in (SET,IMC) for p in port.files_helper.files(P,package))
            before=signature(bp,data,inputs,original_context,original_set);result['before']=before
            assert sum(OLDSET in p for p in before['sets'])==1 and len(before['sets'])==5
            assert sum(OLDACTION in p for p in before['ability'])==1
            assert sum(OLDIMC in p for p in before['hero'])==1
            assert sum(OLDACTION in p for p in before['mappings'])==1
            aset=editor.duplicate_loaded_asset(original_set,SET);context=editor.duplicate_loaded_asset(original_context,IMC)
            assert aset and context
            grants=list(aset.get_editor_property('GrantedGameplayAbilities'))
            matches=0
            for index,entry in enumerate(grants):
                if OLDGA in entry.export_text():
                    assert 'InputTag.Jump' in entry.export_text();matches+=1
                    assert ue.BlueprintEditorLibrary.generated_class(port.load(GA))
                    grants[index]=replace_struct(entry,OLDGA+'.GA_DC_FoundationTest_C',GA+'.GA_Hero_Jump_C')
            assert matches==1
            aset.set_editor_property('GrantedGameplayAbilities',grants)
            mappings=context.get_editor_property('DefaultKeyMappings');entries=list(mappings.get_editor_property('Mappings'))
            for index,entry in enumerate(entries):
                if entry.get_editor_property('Action').get_path_name()==OLDACTION+'.IA_Jump':
                    assert entry.get_editor_property('Key').export_text()=='SpaceBar'
                    assert port.load(ACTION)
                    entries[index]=replace_struct(entry,OLDACTION+'.IA_Jump',ACTION+'.IA_Jump')
            mappings.set_editor_property('Mappings',entries);context.set_editor_property('DefaultKeyMappings',mappings)
            entries=list(inputs.get_editor_property('AbilityInputActions'))
            for index,entry in enumerate(entries):
                if OLDACTION in entry.export_text():entries[index]=replace_struct(entry,OLDACTION+'.IA_Jump',ACTION+'.IA_Jump')
            inputs.set_editor_property('AbilityInputActions',entries)
            h=hero(bp);entries=list(h.get_editor_property('DefaultInputMappings'))
            for index,entry in enumerate(entries):
                if entry.get_editor_property('InputMapping')==original_context:entries[index]=replace_struct(entry,OLDIMC+'.IMC_Player',IMC+'.IMC_DC_RifleBase')
            h.set_editor_property('DefaultInputMappings',entries)
            data.set_editor_property('AbilitySets',[aset if x==original_set else x for x in data.get_editor_property('AbilitySets')])
            assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
            port.helper.clean()
            after=signature(bp,data,inputs,context,aset);validate(after,before)
            assert {n:rows(original_set,n) for n in unchanged_set}==unchanged_set
            assert [x.export_text() for x in key_rows(original_context)]==before['mappings']
            assert all(port.helper.digest(Path(p))==h for p,h in protected.items())
            result['after']=after
            for obj in (aset,context,inputs,bp,data):assert editor.save_loaded_asset(obj,False);result['asset_saves']+=1
        else:
            applied=json.loads((R/'jump_integration_Apply_B/report.json').read_text());assert applied['complete']
            after=signature(bp,data,inputs,port.load(IMC),port.load(SET));validate(after,applied['before'])
            assert after==applied['after'],'Reloaded configuration changed'
            result['after']=after
        port.helper.clean();result['complete']=True
    finally:
        result['protected_unchanged']=all(port.helper.digest(Path(p))==h for p,h in protected.items())
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert result['protected_unchanged']
        print('R8_JUMP_INTEGRATION '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_unchanged')}))

if __name__=='__main__':main()
