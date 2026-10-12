"""R9-3 approved cooldown-only foundation. Four new assets + test PawnData only.

The abstract base has no effect or production cooldown value. The smoke child is
test-only: Commit -> count/log -> 0.35s task -> End; its cooldown is 2 seconds.
"""
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r9_dash_assets as port
from r9_dash_noinput_fix import pin
from r9_dash_integration import imported, rows

P, R = port.P, port.R
BASE = '/Game/DreamCatcher/GAS/Abilities/Signature/GA_DC_SignatureBase'
ROOT = '/Game/DreamCatcher/GAS/Test/R9/Signature/'
GA, GE, ASET = [ROOT+n for n in ('GA_DC_SignatureSmoke', 'GE_DC_SignatureCooldownSmoke', 'AS_DC_SignatureSmoke')]
DATA = '/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RiflePawn'
INPUT = '/Game/DreamCatcher/GAS/Test/R5/Integration/DA_DC_RifleInput'
IMC = '/Game/DreamCatcher/GAS/Test/R5/Integration/IMC_DC_RifleBase'
NEW = (BASE, GE, GA, ASET)


def bp_class(bp):
    return ue.BlueprintEditorLibrary.generated_class(bp)


def cdo(bp):
    return ue.get_default_object(bp_class(bp))


def tag_container(*names):
    tags = ue.GameplayTagContainer()
    assert tags.import_text('(GameplayTags=(' + ','.join('(TagName="'+n+'")' for n in names) + '))')
    return tags


def blueprint(path, parent):
    assert not ue.EditorAssetLibrary.does_asset_exist(path)
    factory = ue.BlueprintFactory()
    factory.set_editor_property('ParentClass', parent)
    asset = ue.AssetToolsHelpers.get_asset_tools().create_asset(path.rsplit('/',1)[1], path.rsplit('/',1)[0], ue.Blueprint, factory)
    assert asset
    return asset


def effect():
    bp = blueprint(GE, ue.load_class(None, '/Script/GameplayAbilities.GameplayEffect'))
    default = cdo(bp)
    default.set_editor_property('DurationPolicy', ue.GameplayEffectDurationType.HAS_DURATION)
    magnitude = default.get_editor_property('DurationMagnitude')
    assert magnitude.import_text('(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value=2.000000))')
    default.set_editor_property('DurationMagnitude', magnitude)
    component = ue.new_object(ue.load_class(None, '/Script/GameplayAbilities.TargetTagsGameplayEffectComponent'), default, 'SignatureCooldownTags')
    tags = component.get_editor_property('InheritableGrantedTagsContainer')
    assert tags.import_text('(Added=(GameplayTags=((TagName="Cooldown.Ultimate"))))')
    component.set_editor_property('InheritableGrantedTagsContainer', tags)
    default.set_editor_property('GEComponents', [component])
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    return bp


def base_ability():
    bp = blueprint(BASE, ue.load_class(None, '/Script/DreamCatcher.DCGameplayAbility'))
    bp.set_editor_property('bGenerateAbstractClass', True)
    default = cdo(bp)
    default.set_editor_property('AbilityTags', tag_container('Ability.Type.Action', 'Ability.Action.Ultimate'))
    default.set_editor_property('ActivationBlockedTags', tag_container('Status.Death', 'Gameplay.AbilityInputBlocked'))
    # Keep Lyra InstancedPerActor/LocalPredicted/OnInputTriggered/Independent defaults.
    default.set_editor_property('CostGameplayEffectClass', None)
    default.set_editor_property('AdditionalCosts', [])
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    return bp


def test_ability(base, cooldown):
    bp = blueprint(GA, bp_class(base))
    bp.set_editor_property('bGenerateAbstractClass', False)
    cdo(bp).set_editor_property('CooldownGameplayEffectClass', bp_class(cooldown))
    assert ue.BlueprintEditorLibrary.add_member_variable(bp, 'TestCommitCount', ue.BlueprintEditorLibrary.get_basic_type_by_name('int'))
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
    event = ue.BlueprintEditorLibrary.add_event_override(bp, 'K2_ActivateAbility', ue.IntPoint(0, 0))
    commit = editor.add_call_function_node('/Script/GameplayAbilities.GameplayAbility.K2_CommitAbility')
    branch = editor.add_branch_node()
    count = editor.add_get_member_variable_node('TestCommitCount')
    add = editor.add_call_function_node('/Script/Engine.KismetMathLibrary.Add_IntInt')
    setter = editor.add_set_member_variable_node('TestCommitCount')
    message = editor.add_call_function_node('/Script/Engine.KismetSystemLibrary.PrintString')
    assert 'Ability|Tasks|WaitDelay' in editor.list_available_nodes([])
    delay = editor.create_node_from_name('Ability|Tasks|WaitDelay', ue.Vector2D(1400, 0), [])
    end = editor.add_call_function_node('/Script/GameplayAbilities.GameplayAbility.K2_EndAbility')
    failed = editor.add_call_function_node('/Script/GameplayAbilities.GameplayAbility.K2_EndAbility')
    assert all((event, commit, branch, count, add, setter, message, delay, end, failed))
    assert delay.get_class().get_path_name() == '/Script/GameplayAbilitiesEditor.K2Node_LatentAbilityCall'
    for node, x, y in ((commit,240,0),(branch,500,0),(count,500,-220),(add,700,-220),
                       (setter,800,0),(message,1060,0),(end,1650,0),(failed,800,230)):
        node.set_node_pos(ue.IntPoint(x,y))
    assert pin(message, 'InString').set_pin_value('[R9-3 TEST ONLY] Signature committed; cooldown=2s; no gameplay effect')
    assert pin(message, 'Duration').set_pin_value('1.0')
    assert pin(delay, 'Time').set_pin_value('0.35')
    for a, ap, b, bp_name in ((event,'then',commit,'execute'),(commit,'then',branch,'execute'),
            (commit,'ReturnValue',branch,'Condition'),(branch,'then',setter,'execute'),
            (count,'TestCommitCount',add,'A'),(add,'ReturnValue',setter,'TestCommitCount'),
            (setter,'then',message,'execute'),(message,'then',delay,'execute'),
            (delay,'OnFinish',end,'execute'),(branch,'else',failed,'execute')):
        assert pin(a, ap).try_create_connection(pin(b, bp_name))
    # The engine's promotable math node resolves its numeric pins after wiring A.
    assert pin(add, 'B').set_pin_value('1')
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    return bp


def fields(obj):
    return port.scope['reflected_fields'](obj)['values']


def signature(data):
    return dict(data=fields(data), sets={s.get_path_name():fields(s) for s in data.get_editor_property('AbilitySets')},
                inputs=fields(ue.load_asset(INPUT)), mappings=fields(ue.load_asset(IMC)))


def verify_policy(base, ga, ge, aset):
    assert base.get_editor_property('bGenerateAbstractClass') and not ga.get_editor_property('bGenerateAbstractClass')
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(base).get_path_name() == '/Script/DreamCatcher.DCGameplayAbility'
    assert ue.BlueprintEditorLibrary.get_blueprint_parent_class(ga) == bp_class(base)
    assert cdo(base).get_editor_property('CooldownGameplayEffectClass') is None
    assert cdo(ga).get_editor_property('CooldownGameplayEffectClass') == bp_class(ge)
    for bp in (base, ga):
        default = cdo(bp)
        assert default.get_editor_property('CostGameplayEffectClass') is None and not default.get_editor_property('AdditionalCosts')
        assert default.get_editor_property('ActivationPolicy') == ue.DCAbilityActivationPolicy.ON_INPUT_TRIGGERED
        assert default.get_editor_property('ActivationGroup') == ue.DCAbilityActivationGroup.INDEPENDENT
        actual_tags = default.get_editor_property('AbilityTags').export_text()
        assert actual_tags == tag_container('Ability.Type.Action', 'Ability.Action.Ultimate').export_text(), (bp.get_name(), actual_tags)
        actual_blocked = default.get_editor_property('ActivationBlockedTags').export_text()
        assert actual_blocked == tag_container('Status.Death', 'Gameplay.AbilityInputBlocked').export_text(), (bp.get_name(), actual_blocked)
        assert default.get_editor_property('BlockAbilitiesWithTag').export_text() == '(GameplayTags=)'
        assert default.get_editor_property('CancelAbilitiesWithTag').export_text() == '(GameplayTags=)'
    assert cdo(ga).get_editor_property('TestCommitCount') == 0
    default = cdo(ge)
    assert default.get_editor_property('DurationPolicy') == ue.GameplayEffectDurationType.HAS_DURATION
    assert default.get_editor_property('StackingType') == ue.GameplayEffectStackingType.NONE
    assert default.get_editor_property('Period').get_editor_property('Value') == 0
    for name in ('Modifiers', 'Executions', 'GameplayCues'):
        assert not default.get_editor_property(name)
    components = list(default.get_editor_property('GEComponents'))
    assert len(components) == 1
    assert components[0].get_class().get_path_name() == '/Script/GameplayAbilities.TargetTagsGameplayEffectComponent'
    assert components[0].get_editor_property('InheritableGrantedTagsContainer').export_text() == '(CombinedTags=(GameplayTags=((TagName="Cooldown.Ultimate"))),Added=(GameplayTags=((TagName="Cooldown.Ultimate"))),Removed=(GameplayTags=))'
    assert 'ScalableFloatMagnitude=(Value=2.000000,' in fields(default)['duration_magnitude']
    grants = rows(aset, 'GrantedGameplayAbilities')
    assert len(grants) == 1 and GA+'.GA_DC_SignatureSmoke_C' in grants[0] and 'InputTag.Ultimate' in grants[0]
    assert not aset.get_editor_property('GrantedGameplayEffects') and not aset.get_editor_property('GrantedAttributes')


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR9Signature=(apply|verify)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    port.helper.LOG = P/'Saved/Logs'/('R9Signature-'+run+'.log')
    out = R/('signature_'+run)
    assert not out.exists()
    out.mkdir()
    protected_paths = {str(p) for root in (P/'Source', P/'Config', P/'Content/DreamCatcher/GAS', P/'Content/LyraMigration/Dash') for p in root.rglob('*') if p.is_file()}
    protected_paths.add(str(P/'Binaries/Win64/UnrealEditor-DreamCatcher.dll'))
    for package in NEW+(DATA,):
        protected_paths.update(str(p) for p in port.files_helper.files(P,package))
    mutable = {str(p) for package in NEW+(DATA,) for p in port.files_helper.files(P,package)} if mode == 'apply' else set()
    protected = {p:port.helper.digest(Path(p)) for p in protected_paths if p not in mutable}
    result = dict(complete=False,mode=mode,asset_saves=[],protected=protected,test_cooldown_seconds=2.0,test_lifetime_seconds=.35)
    assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    try:
        data = ue.load_asset(DATA)
        before = signature(data)
        result['before'] = before
        result['ultimate_input_action'] = fields(ue.load_asset('/Game/Input/Actions/IA_Ultimate'))
        if mode == 'apply':
            assert len(before['sets']) == 6
            inspected = json.loads((R/'signature_inspect_A.json').read_text())
            assert before['data'] == inspected['data'] and before['sets'] == inspected['sets']
            assert not any('InputTag.Ultimate' in g for s in before['sets'].values() for g in s['granted_gameplay_abilities'])
            ps = ue.get_default_object(ue.load_class(None, '/Script/DreamCatcher.DCPlayerState'))
            assert not ps.get_editor_property('PlayerStateAbilitySets'), 'Review additional PlayerState grants first'
            assert all(not p.exists() for package in NEW for p in port.files_helper.files(P,package))
            base = base_ability()
            ge = effect()
            ga = test_ability(base,ge)
            cls = ue.load_class(None, '/Script/DreamCatcher.DCAbilitySet')
            factory = ue.DataAssetFactory()
            factory.set_editor_property('DataAssetClass', cls)
            aset = ue.AssetToolsHelpers.get_asset_tools().create_asset(ASET.rsplit('/',1)[1],ROOT.rstrip('/'),cls,factory)
            assert aset
            sample = ue.load_asset('/Game/DreamCatcher/GAS/Test/R5/Integration/AS_DC_RifleDash').get_editor_property('GrantedGameplayAbilities')[0]
            grant = '(Ability="/Script/Engine.BlueprintGeneratedClass\''+bp_class(ga).get_path_name()+'\'",AbilityLevel=1,InputTag=(TagName="InputTag.Ultimate"))'
            aset.set_editor_property('GrantedGameplayAbilities',[imported(sample,grant)])
            data.set_editor_property('AbilitySets',list(data.get_editor_property('AbilitySets'))+[aset])
        else:
            applied_run = re.search(r'-DCR9Applied=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd).group(1)
            applied = json.loads((R/('signature_'+applied_run)/'report.json').read_text())
            assert applied['complete']
            before = dict(applied['before'])
            expected_after = dict(applied['after'])
            # A later explicit approval added Pressed only to the private Q row.
            input_run = re.search(r'-DCR9InputApplied=([A-Za-z0-9_]{1,32})(?:\s|$)',cmd)
            if input_run:
                input_receipt = json.loads((R/('signature_'+input_run.group(1))/'report.json').read_text())
                assert input_receipt['complete'] and input_receipt['before'] == expected_after['mappings']
                before['mappings'] = input_receipt['after']
                expected_after['mappings'] = input_receipt['after']
            base,ge,ga,aset = [ue.load_asset(p) for p in NEW]
            assert all((base,ge,ga,aset))
            for bp in (base,ge,ga):
                assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
        verify_policy(base,ga,ge,aset)
        ue.collect_garbage()
        result['assets'] = {obj.get_path_name():port.data(obj,out,obj.get_name()) for obj in (base,ge,ga,aset)}
        after = signature(data)
        result['after'] = after
        expected_data = dict(before['data'])
        expected_data['ability_sets'] = before['data']['ability_sets'] + [ASET+'.AS_DC_SignatureSmoke']
        assert after['data'] == expected_data
        assert {k:v for k,v in after['sets'].items() if k != ASET+'.AS_DC_SignatureSmoke'} == before['sets']
        assert after['inputs'] == before['inputs'] and after['mappings'] == before['mappings']
        assert sum('InputTag.Ultimate' in r for r in rows(ue.load_asset(INPUT),'AbilityInputActions')) == 1
        if mode == 'verify':
            assert port.comparable(result['assets']) == port.comparable(applied['assets'])
            assert after == expected_after
        port.helper.clean()
        if mode == 'apply':
            for obj in (base,ge,ga,aset,data):
                assert assets.save_loaded_asset(obj,False)
                result['asset_saves'].append(obj.get_path_name())
        result['complete'] = True
    finally:
        result['protected_changed'] = [p for p,h in protected.items() if port.helper.digest(Path(p)) != h]
        (out/'report.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
        assert not result['protected_changed']
        print('R9_SIGNATURE_SETUP '+json.dumps({k:result[k] for k in ('mode','complete','asset_saves','protected_changed')}))


if __name__ == '__main__':
    main()
