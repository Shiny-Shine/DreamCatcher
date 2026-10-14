"""Approved R9-2 GE + two Dash task connections. UE-only, bounded writes; no builds."""
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
import unreal as ue
sys.path.insert(0, str(Path(__file__).resolve().parent))
import r9_dash_assets as port
from r9_dash_noinput_fix import pin, function_results

P, R = port.P, port.R
GA = '/Game/LyraMigration/Dash/GA_Hero_Dash'
GE = '/Game/DreamCatcher/GAS/Effects/GE_DC_DashDefense'
ACTION = 'DreamCatcher|Ability|Tasks|DashDefenseWindow'
ROUTES = (('local', 'K2Node_IfThenElse_4', 'K2Node_IfThenElse_3', 'K2Node_CallFunction_2'),
          ('server', 'K2Node_CallFunction_17', 'K2Node_LatentAbilityCall_1', 'K2Node_CallFunction_17'))


def normalized_graph(snapshot, new_names, changed):
    value = json.loads(json.dumps(snapshot))
    for graph in value['graphs']:
        if graph['name'] != 'EventGraph':
            continue
        graph['nodes'] = [n for n in graph['nodes'] if n['name'] not in new_names]
        for node in graph['nodes']:
            for p in node.get('pins', []):
                if (node['name'], p['name']) in changed:
                    p['links'] = []
                p['links'].sort(key=lambda x: (x['node'], x['pin']))
    return value


def verify_graph(before, after, names):
    changed = {(node, p) for _, start, end, commit in ROUTES
               for node, p in ((start, 'then'), (end, 'execute'), (commit, 'ReturnValue'))}
    new_names = [n for pair in names.values() for n in pair.values()]
    assert normalized_graph(before, [], changed) == normalized_graph(after, new_names, changed), 'Unapproved original graph change'
    old = {n['name']: n for g in before['graphs'] if g['name'] == 'EventGraph' for n in g['nodes']}
    new = {n['name']: n for g in after['graphs'] if g['name'] == 'EventGraph' for n in g['nodes']}
    def links(nodes, name, field):
        return sorted((l['node'], l['pin']) for p in nodes[name]['pins'] if p['name'] == field for l in p['links'])
    for route, start, end, commit in ROUTES:
        task, duration = names[route]['task'], names[route]['duration']
        assert new[task]['class'] == '/Script/GameplayAbilitiesEditor.K2Node_LatentAbilityCall'
        assert links(new, start, 'then') == [(task, 'execute')]
        expected = set(links(old, end, 'execute')) - {(start, 'then')} | {(task, 'then')}
        assert links(new, end, 'execute') == sorted(expected)
        assert links(new, commit, 'ReturnValue') == sorted(links(old, commit, 'ReturnValue') + [(task, 'bCommitSucceeded')])
        for field, expected in {'execute': [(start, 'then')], 'then': [(end, 'execute')],
                                'bCommitSucceeded': [(commit, 'ReturnValue')],
                                'Duration': [(duration, 'RootMotionDuration')]}.items():
            assert links(new, task, field) == expected
        for field in ('OnOpened', 'OnClosed', 'OnDamageAvoided', 'EventData', 'AsyncTaskProxy'):
            assert not links(new, task, field), 'Unapproved feedback/reward connection'
        effect = next(p for p in new[task]['pins'] if p['name'] == 'DefenseEffect')
        assert GE + '.GE_DC_DashDefense_C' in effect['value'] and not effect['links']
        assert links(new, duration, 'RootMotionDuration') == [(task, 'Duration')]


def ge_data(bp):
    cdo = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    fields = port.scope['reflected_fields'](cdo)['values']
    components = list(cdo.get_editor_property('GEComponents'))
    parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name()
    assert parent == '/Script/GameplayAbilities.GameplayEffect'
    assert cdo.get_editor_property('DurationPolicy') == ue.GameplayEffectDurationType.HAS_DURATION
    assert not cdo.get_editor_property('Modifiers') and not cdo.get_editor_property('Executions')
    assert cdo.get_editor_property('StackingType') == ue.GameplayEffectStackingType.NONE
    assert cdo.get_editor_property('Period').get_editor_property('Value') == 0
    assert len(components) == 1
    assert components[0].get_class().get_path_name() == '/Script/GameplayAbilities.TargetTagsGameplayEffectComponent'
    tags = components[0].get_editor_property('InheritableGrantedTagsContainer').export_text()
    assert tags == '(CombinedTags=(GameplayTags=((TagName="Gameplay.DamageImmunity"))),Added=(GameplayTags=((TagName="Gameplay.DamageImmunity"))),Removed=(GameplayTags=))', tags
    assert 'ScalableFloatMagnitude=(Value=0.250000,' in fields['duration_magnitude']
    return dict(fields=fields, tags=tags, parent=parent)


def create_ge():
    assert not ue.EditorAssetLibrary.does_asset_exist(GE)
    assert not any(p.exists() for p in port.files_helper.files(P, GE))
    factory = ue.BlueprintFactory()
    factory.set_editor_property('ParentClass', ue.load_class(None, '/Script/GameplayAbilities.GameplayEffect'))
    bp = ue.AssetToolsHelpers.get_asset_tools().create_asset(GE.rsplit('/', 1)[1], GE.rsplit('/', 1)[0], ue.Blueprint, factory)
    assert bp
    cdo = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    component = ue.new_object(ue.load_class(None, '/Script/GameplayAbilities.TargetTagsGameplayEffectComponent'), cdo, 'DashDefenseTags')
    tags = component.get_editor_property('InheritableGrantedTagsContainer')
    assert tags.import_text('(Added=(GameplayTags=((TagName="Gameplay.DamageImmunity"))))')
    component.set_editor_property('InheritableGrantedTagsContainer', tags)
    cdo.set_editor_property('GEComponents', [component])
    cdo.set_editor_property('DurationPolicy', ue.GameplayEffectDurationType.HAS_DURATION)
    magnitude = cdo.get_editor_property('DurationMagnitude')
    assert magnitude.import_text('(MagnitudeCalculationType=ScalableFloat,ScalableFloatMagnitude=(Value=0.250000))')
    cdo.set_editor_property('DurationMagnitude', magnitude)
    assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
    return bp


def main():
    cmd = ue.SystemLibrary.get_command_line()
    mode = re.search(r'-DCR9Defense=(apply|verify)(?:\s|$)', cmd).group(1)
    run = re.search(r'-DCR9Run=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve() == P.resolve()
    assert ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    tests = json.loads((P / 'Saved/Automation/R9DefenseCode_A/index.json').read_text(encoding='utf-8-sig'))
    assert tests['succeeded'] == 4 and tests['failed'] == tests['succeededWithWarnings'] == 0
    port.helper.LOG = P / 'Saved/Logs' / ('R9Defense-' + run + '.log')
    out = R / ('defense_' + run)
    assert not out.exists()
    out.mkdir()
    baseline = json.loads((R / 'idle_fix_IdleVerify_A/report.json').read_text())
    assert baseline['complete']
    all_paths = set(baseline['protected'])
    for root in (P/'Config', P/'Source', P/'Content/LyraMigration/Dash', P/'Content/DreamCatcher/GAS/Test/R5/Integration'):
        all_paths.update(str(p) for p in root.rglob('*') if p.is_file())
    all_paths.update(str(p) for asset in (GA, GE) for p in port.files_helper.files(P, asset))
    mutable = {str(p) for asset in (GA, GE) for p in port.files_helper.files(P, asset)} if mode == 'apply' else set()
    protected = {p: port.helper.digest(Path(p)) for p in all_paths if p not in mutable}
    result = dict(mode=mode, complete=False, asset_saves=[], protected=protected)
    try:
        bp = ue.load_asset(GA)
        assert bp
        ue.collect_garbage()
        before = port.data(bp, out, 'before')
        result['before'] = before
        if mode == 'apply':
            assert port.comparable(before) == port.comparable(baseline['after']), 'Current Dash differs from accepted baseline'
            result['direction_before'] = function_results(bp)
            ge = create_ge()
            result['ge'] = ge_data(ge)
            editor = ue.BlueprintGraphEditor.get_graph_editor_by_name(bp, 'EventGraph')
            assert ACTION in editor.list_available_nodes([])
            nodes = {n.get_name(): n for n in editor.list_all_nodes()}
            names = {}
            for route, start, end, commit in ROUTES:
                position = nodes[start].get_node_pos()
                task = editor.create_node_from_name(ACTION, ue.Vector2D(position.x + 300, position.y - 250), [])
                duration = editor.add_get_member_variable_node('RootMotionDuration')
                assert task and duration
                duration.set_node_pos(ue.IntPoint(position.x + 80, position.y - 140))
                assert task.get_class().get_path_name() == '/Script/GameplayAbilitiesEditor.K2Node_LatentAbilityCall'
                assert pin(task, 'DefenseEffect').set_pin_value(ue.BlueprintEditorLibrary.generated_class(ge).get_path_name())
                assert pin(nodes[start], 'then').break_single_pin_link(pin(nodes[end], 'execute'))
                for a, b in ((pin(nodes[start], 'then'), pin(task, 'execute')),
                             (pin(task, 'then'), pin(nodes[end], 'execute')),
                             (pin(nodes[commit], 'ReturnValue'), pin(task, 'bCommitSucceeded')),
                             (pin(duration, 'RootMotionDuration'), pin(task, 'Duration'))):
                    assert a.try_create_connection(b)
                names[route] = dict(task=task.get_name(), duration=duration.get_name())
            result['names'] = names
        else:
            applied_run = re.search(r'-DCR9Applied=([A-Za-z0-9_]{1,32})(?:\s|$)', cmd).group(1)
            applied = json.loads((R / ('defense_' + applied_run) / 'report.json').read_text())
            assert applied['complete']
            ge = ue.load_asset(GE)
            assert ge and ue.BlueprintEditorLibrary.compile_blueprint(ge)
            result['ge'] = ge_data(ge)
            assert result['ge'] == applied['ge']
            names = applied['names']
            before = applied['before']
            result['direction_before'] = applied['direction_before']
        assert ue.BlueprintEditorLibrary.compile_blueprint(bp)
        ue.collect_garbage()
        after = port.data(bp, out, 'after')
        result['after'] = after
        verify_graph(before['snapshot'], after['snapshot'], names)
        # Runtime defaults, parent, and non-event graphs are not part of this change.
        assert port.comparable(before['defaults']) == port.comparable(after['defaults'])
        assert before['dash'] == after['dash']
        result['direction_after'] = function_results(bp)
        assert result['direction_after'] == result['direction_before']
        if mode == 'verify':
            assert port.comparable(after) == port.comparable(applied['after']), 'Cold reload differs'
        port.helper.clean()
        if mode == 'apply':
            assets = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
            for obj in (ge, bp):
                assert assets.save_loaded_asset(obj, False)
                result['asset_saves'].append(obj.get_path_name())
        result['complete'] = True
    finally:
        result['protected_changed'] = [p for p, h in protected.items() if port.helper.digest(Path(p)) != h]
        (out / 'report.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
        assert not result['protected_changed']
        print('R9_DEFENSE_SETUP ' + json.dumps({k: result[k] for k in ('mode', 'complete', 'asset_saves', 'protected_changed')}))


if __name__ == '__main__':
    main()
