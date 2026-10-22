"""Approved OFF ammo style: one AmmoOff row, 5Medium size10, existing ON intact.

Native inspect/apply/fresh verify only; no C++ build, PIE or Git writes.
Use -DCAmmoOffMode=inspect|apply|verify -DCAmmoOffRun=<unique_run> and
-DCAmmoOffReference=<previous_run> after inspect. The absolute Editor.log must
be inside Saved/Diagnostics/WeaponAmmoOffStyle20261023/<run>.
Only W_DC_WeaponRow and DT_DC_AmmoTextStyle may be saved; panel is read-only.
"""
import csv
import importlib.util
import io
import json
from pathlib import Path
import re
import sys
import traceback

sys.dont_write_bytecode = True
P = Path('E:/Epic Games/UEProjects/DreamCatcher')
R = P / 'Saved/Diagnostics/WeaponAmmoOffStyle20261023'
SCRIPT = P / 'Scripts/Editor/weapon_ammo_off_style_setup.py'
TARGETS = {'row': '/Game/DreamCatcher/UI/HUD/W_DC_WeaponRow', 'style': '/Game/DreamCatcher/UI/HUD/DT_DC_AmmoTextStyle'}
PANEL = '/Game/DreamCatcher/UI/HUD/W_DC_WeaponPanel'
FONT = '/Game/DreamCatcher/UI/Font/Paperlogy-5Medium_Font'
BASE = {'row': '05dac7f379adee024f54279df2223babeb357d430a86c17f681d8605b94b808f',
        'style': 'd552f94af293ea585626ff17464f3360c3e30d8db4637de786e796550f8e21e6'}
PANEL_SHA = '53cccd7501117557f8be51c4b88e76f407081ae9c047233b29f8f2d5a3af1a35'


def initialize():
    global ue, direct, rich, screens, hud, UMG
    spec = importlib.util.spec_from_file_location('ammo_off_helpers', SCRIPT.with_name('weapon_panel_size_setup.py'))
    direct = importlib.util.module_from_spec(spec); spec.loader.exec_module(direct); direct.initialize()
    ue, rich, screens, hud, UMG = direct.ue, direct.rich, direct.screens, direct.hud, direct.UMG


def disk(package):
    return P / ('Content/' + package[6:] + '.uasset')


def hashes():
    return {key: rich.sha(disk(package)) for key, package in TARGETS.items()}


def protection():
    allowed = {SCRIPT, *(disk(package) for package in TARGETS.values())}
    state = {}
    for top in ('Content','Source','Config','Scripts','Plugins','docs','Saved/SaveGames'):
        for path in (P/top).rglob('*'):
            if path.is_file() and path not in allowed:
                stat=path.stat(); state[path.relative_to(P).as_posix()]=[stat.st_size,stat.st_mtime_ns]
    names = ['.git/HEAD','.git/index','DreamCatcher.uproject','Binaries/Win64/UnrealEditor-DreamCatcher.dll',
        'Content/DreamCatcher/UI/HUD/W_DC_WeaponPanel.uasset','Content/DreamCatcher/UI/Font/Paperlogy-5Medium_Font.uasset']
    return {'inventory':state,'sha256':{name:rich.sha(P/name) for name in names}}


def snapshot(assets, folder, label):
    return direct.snapshot(assets,folder,label)


def decode_text(value):
    match=re.fullmatch(r'NSLOCTEXT\("[^"]*", "[^"]*", ("(?:\\.|[^"])*")\)',value)
    if match: return json.loads(match[1])
    match=re.fullmatch(r'INVTEXT\(("(?:\\.|[^"])*")\)',value)
    return json.loads(match[1]) if match else value


def plain(text):
    return re.sub(r'<(?:Magazine|Slash|Spare)>|</>','',text)


def inspect(assets, report):
    assert 'AmmoOff' not in report['before']['styles']
    _, widgets=hud.tree(UMG,assets['row']); ammo=widgets['AmmoText']
    assert isinstance(ammo,ue.RichTextBlock) and ammo.get_text_style_set()==assets['style']
    graphs=report['before']['row']['graphs']
    formats=[(name,node) for name,node in graphs['UpdateData'].items() if node['class'].endswith('K2Node_FormatText')]
    assert len(formats)==1
    name,node=formats[0]; on=decode_text(node['pins']['Format']['value'])
    assert set(node['pins'])=={'Format','Magazine','Spare','Result'}
    assert plain(on).count('{Magazine}')==1 and plain(on).count('{Spare}')==1
    off='<AmmoOff>'+plain(on)+'</>'
    preview=str(ammo.get_text())
    report['contract']={'on_node':name,'on_format':on,'off_format':off,'on_preview':preview,
        'off_preview':'<AmmoOff>'+plain(preview)+'</>','styles':report['before']['styles']}
    report['style_summary']={}
    for name,raw in report['before']['styles'].items():
        style=ue.TextBlockStyle(); assert style.import_text(raw)
        font=style.get_editor_property('Font')
        report['style_summary'][name]={'font':font.export_text(),'size':font.get_editor_property('Size'),
            'color':style.get_editor_property('ColorAndOpacity').export_text(),'shadow':style.get_editor_property('ShadowColorAndOpacity').export_text()}
    for name in ('Magazine','Slash','Spare'):
        assert report['style_summary'][name]['color']==report['style_summary']['Spare']['color'], 'Different ON portion colors require review'
        assert report['style_summary'][name]['shadow']==report['style_summary']['Spare']['shadow'], 'Different ON portion shadows require review'
    font=ue.load_asset(FONT); assert font
    report['format_action']=[str(name) for name in ue.BlueprintGraphEditor.get_graph_editor_by_name(assets['row'],'UpdateData').list_available_nodes([])
        if 'formattext' in str(name).lower().replace(' ','')]
    assert len(report['format_action'])==1


def normalized(value):
    result=direct.normalized(value)
    # Native saving may assign localization keys to newly authored literals.
    # Keep the existing ON Format node raw; canonicalize only our new OFF
    # Format and the two new preview SelectText constants by exact text content.
    for graph in result['row']['graphs'].values():
        for node in graph.values():
            for name,pin in node.get('pins',{}).items():
                if pin['type']!='Text': continue
                decoded=decode_text(pin['value'])
                if (name=='Format' and decoded.startswith('<AmmoOff>')) or (
                    name in ('A','B') and decoded.startswith(('<Magazine>','<AmmoOff>'))):
                    pin['value']=decoded
    return result


def append_style(table, contract):
    value=ue.TextBlockStyle(); assert value.import_text(contract['styles']['Spare'])
    font=value.get_editor_property('Font')
    font.set_editor_property('FontObject',ue.load_asset(FONT))
    font.set_editor_property('TypefaceFontName','Default'); font.set_editor_property('Size',10)
    value.set_editor_property('Font',font)
    contract['off_style']=value.export_text()
    stream=io.StringIO(newline=''); writer=csv.writer(stream); writer.writerow(['Name','TextStyle'])
    for name,raw in contract['styles'].items(): writer.writerow([name,raw])
    writer.writerow(['AmmoOff',contract['off_style']])
    assert ue.DataTableFunctionLibrary.fill_data_table_from_csv_string(table,stream.getvalue())


def patch_graphs(bp, contract, action, report):
    pin,wire=screens.pin,screens.wire
    g,nodes=direct.graph_context(bp,'UpdateData')
    on=nodes[contract['on_node']]
    original_targets=list(pin(on,'Result').list_connected_pins()); assert len(original_targets)==1
    target=original_targets[0]; assert str(target.get_pin_name())=='InText'
    entry=[node for node in nodes.values() if node.get_class().get_name()=='K2Node_FunctionEntry']; assert len(entry)==1
    off=g.e.create_node_from_name(action,ue.Vector2D(1800,1000),[])
    assert off and off.get_class().get_name()=='K2Node_FormatText'
    screens.literal(off,'Format','INVTEXT('+json.dumps(contract['off_format'])+')')
    for name in ('Magazine','Spare'):
        sources=list(pin(on,name).list_connected_pins()); assert len(sources)==1
        source=sources[0]
        assert pin(source.get_owning_node(),'bUseGrouping').get_pin_value()=='false'
        wire(source,pin(off,name))
    assert target.break_pin_links()
    select=g.node('/Script/Engine.KismetMathLibrary.SelectText',A=pin(on,'Result'),B=pin(off,'Result'),bPickA=pin(entry[0],'InSelected'))
    wire(pin(select,'ReturnValue'),target)
    report['patch']={'UpdateData':{'old_nodes':list(nodes),'on_node':on.get_name(),'off_node':off.get_name(),
        'select_node':select.get_name(),'setter':target.get_owning_node().get_name(),
        'count_text_sources':[pin(on,name).list_connected_pins()[0].get_owning_node().get_name() for name in ('Magazine','Spare')]}}
    g,nodes=direct.graph_context(bp,'PreparePreview')
    returns=[node for node in nodes.values() if node.get_class().get_name()=='K2Node_FunctionResult']; assert len(returns)==1
    result=pin(returns[0],'execute'); predecessors=list(result.list_connected_pins()); assert len(predecessors)==1
    assert result.break_pin_links(); g.tail=predecessors[0]
    select=g.node('/Script/Engine.KismetMathLibrary.SelectText',
        A='INVTEXT('+json.dumps(contract['on_preview'])+')',B='INVTEXT('+json.dumps(contract['off_preview'])+')',bPickA=g.get('PreviewSelected'))
    setter=g.widget('AmmoText','RichTextBlock.SetText',InText=pin(select,'ReturnValue'))
    wire(g.tail,result)
    report['patch']['PreparePreview']={'old_nodes':list(nodes),'select_node':select.get_name(),'setter':setter.get_name(),
        'old_tail':predecessors[0].get_owning_node().get_name(),'result':returns[0].get_name()}


def preserve(before,after,patch):
    a,b=direct.normalized(before),direct.normalized(after)
    assert a['panel']==b['panel'],'Protected panel/template changed in memory'
    for name in ('parent','cdo','widgets'): assert a['row'][name]==b['row'][name],'Unapproved row change: '+name
    for graph_name,old_graph in a['row']['graphs'].items():
        new_graph=b['row']['graphs'][graph_name]
        if graph_name not in patch:
            assert old_graph==new_graph,'Unapproved graph change: '+graph_name; continue
        original=set(old_graph)
        assert original<=set(new_graph)
        for name,old in old_graph.items():
            old,new=json.loads(json.dumps(old)),json.loads(json.dumps(new_graph[name]))
            for pin_name in old['pins']:
                allowed=(graph_name=='UpdateData' and ((name==patch[graph_name]['on_node'] and pin_name=='Result') or
                    (name==patch[graph_name]['setter'] and pin_name=='InText') or
                    (name in patch[graph_name]['count_text_sources'] and pin_name=='ReturnValue') or
                    (old['class'].endswith('K2Node_FunctionEntry') and pin_name=='InSelected'))) or (
                    graph_name=='PreparePreview' and ((name==patch[graph_name]['old_tail'] and pin_name=='then') or
                    (name==patch[graph_name]['result'] and pin_name=='execute')))
                if allowed: old['pins'][pin_name]['links']=new['pins'][pin_name]['links']
            assert old==new,'Unexpected existing node change: '+graph_name+'/'+name
    assert {name:b['styles'][name] for name in a['styles']}==a['styles'],'Existing ON styles changed'


def validate(assets,contract,report):
    actual=direct.styles(assets['style'])
    assert set(actual)==set(contract['styles'])|{'AmmoOff'}
    for name,raw in contract['styles'].items(): assert actual[name]==raw,'Existing style changed: '+name
    assert actual['AmmoOff']==contract['off_style']
    style=ue.TextBlockStyle(); assert style.import_text(actual['AmmoOff'])
    font=style.get_editor_property('Font')
    assert font.get_editor_property('FontObject').get_path_name().split('.')[0]==FONT
    assert font.get_editor_property('Size')==10 and str(font.get_editor_property('TypefaceFontName'))=='Default'
    graphs=screens.old_art.graph_state(assets['row'])
    formats=[node for node in graphs['UpdateData'].values() if node['class'].endswith('K2Node_FormatText')]
    assert len(formats)==2
    assert {decode_text(node['pins']['Format']['value']) for node in formats}=={contract['on_format'],contract['off_format']}
    report['validation']={'existing_ON_rows_unchanged':True,'off_font':FONT,'off_size':10,
        'all_OFF_characters_in_single_style':True,'empty_path_and_layout_unchanged':True}


def object_tests(assets,contract,report):
    library=ue.get_default_object(ue.load_class(None,'/Script/UMG.WidgetBlueprintLibrary'))
    world=ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_editor_world()
    panel=library.call_method('Create',args=(world,assets['panel'].generated_class(),None)); assert panel
    rows=[panel.get_editor_property(name) for name in ('Row1','Row2')]
    item=ue.new_object(ue.load_class(None,'/Script/DreamCatcher.DCInventoryItemInstance'))
    tags=[]
    for suffix in ('MagazineAmmo','SpareAmmo'):
        tag=ue.GameplayTag(); assert tag.import_text('(TagName="Lyra.ShooterGame.Weapon.'+suffix+'")'); tags.append(tag)
    results={}
    for index,row in enumerate(rows,1):
        ammo=row.get_editor_property('AmmoText'); values=[]
        for magazine,spare in ((30,120),(10,120),(9,120),(0,120),(30,90),(9,1234)):
            for tag,desired in zip(tags,(magazine,spare)):
                existing=item.call_method('GetStatTagStackCount',args=(tag,))
                if existing: item.call_method('RemoveStatTagStack',args=(tag,existing))
                if desired: item.call_method('AddStatTagStack',args=(tag,desired))
            for selected in (True,False):
                row.call_method('UpdateData',args=(item,selected))
                actual=str(ammo.get_text()); fmt=contract['on_format'] if selected else contract['off_format']
                assert actual==fmt.replace('{Magazine}',str(magazine)).replace('{Spare}',str(spare))
                if not selected: assert actual.startswith('<AmmoOff>') and actual.endswith('</>') and actual.count('<')==2
                values.append({'selected':selected,'text':actual})
        row.call_method('UpdateData',args=(None,False)); row.call_method('ApplyVisuals')
        assert str(ammo.get_text())=='' and not row.get_editor_property('HasItem')
        row.call_method('UpdateData',args=(item,True)); assert str(ammo.get_text())==values[-2]['text']
        row.call_method('AdvanceTransition',args=(.25,)); row.call_method('ApplyVisuals')
        assert row.get_editor_property('CurrentAlpha')==1
        row.call_method('UpdateData',args=(item,False)); assert str(ammo.get_text())==values[-1]['text']
        assert row.get_editor_property('CurrentAlpha')==1,'Style must change before positional transition'
        row.call_method('AdvanceTransition',args=(.125,)); row.call_method('ApplyVisuals')
        assert row.get_editor_property('CurrentAlpha')==.5
        row.call_method('UpdateData',args=(item,True)); assert str(ammo.get_text())==values[-2]['text']
        row.call_method('AdvanceTransition',args=(.125,)); assert row.get_editor_property('CurrentAlpha')==.75
        row.call_method('AdvanceTransition',args=(.125,)); assert row.get_editor_property('CurrentAlpha')==1
        row.call_method('UpdateData',args=(item,False)); row.call_method('AdvanceTransition',args=(.25,)); row.call_method('ApplyVisuals')
        assert row.get_editor_property('CurrentAlpha')==0
        assert ammo.get_editor_property('slot').get_position().export_text()==row.get_editor_property('OffMagazinePosition').export_text()
        preview=[]
        for selected in (True,False,True):
            row.set_editor_property('PreviewSelected',selected); row.call_method('PreparePreview')
            actual=str(ammo.get_text()); assert actual==contract['on_preview' if selected else 'off_preview']
            assert row.get_editor_property('CurrentAlpha')==int(selected)
            preview.append({'selected':selected,'text':actual})
        results['Row'+str(index)]={'ammo_cases':values,'empty_rebind':True,'immediate_style':True,
            'transition_and_reversal_250ms':True,'preview':preview,
            'off_position':row.get_editor_property('OffMagazinePosition').export_text(),'off_height':row.get_editor_property('OffHeight')}
    report['object_tests']={'actual_protected_panel_children':results,'PIE':False,'visual_or_click_verified':False}


def apply(assets,previous,folder,report):
    contract=json.loads(json.dumps(previous['contract']))
    append_style(assets['style'],contract)
    patch_graphs(assets['row'],contract,previous['format_action'][0],report)
    screens.compile_widget(assets['row'],report)
    report['after']=snapshot(assets,folder,'after')
    preserve(report['before'],report['after'],report['patch']); validate(assets,contract,report); object_tests(assets,contract,report)
    assert normalized(snapshot(assets,folder,'tested'))==normalized(report['after']),'Transient tests changed saved templates'
    hud.clean(TARGETS.values()); hud.log_check(folder/'Editor.log')
    assert protection()==report['protected_before'] and hashes()==report['before_sha256']
    for key in ('style','row'):
        hud.assert_no_other_editor(); assert ue.EditorAssetLibrary.save_loaded_asset(assets[key],only_if_is_dirty=False)
        report['asset_saves'].append(TARGETS[key])
    report['contract']=contract


def verify(assets,previous,folder,report):
    report['after']=snapshot(assets,folder,'reloaded')
    assert normalized(report['after'])==normalized(previous['after']),'Fresh reload changed authored data'
    validate(assets,previous['contract'],report); screens.compile_widget(assets['row'],report)
    object_tests(assets,previous['contract'],report)
    assert normalized(snapshot(assets,folder,'tested'))==normalized(report['after'])


def main():
    initialize()
    command=ue.SystemLibrary.get_command_line()
    assert all(flag in command.lower() for flag in ('-unattended','-run=pythonscript','-sccprovider=none','-enableplugins=umgtoolset'))
    assert not ue.SourceControl.is_enabled()
    assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==P.resolve()
    hud.assert_no_other_editor(); hud.clean()
    mode=screens.arg(command,'DCAmmoOffMode','inspect|apply|verify')
    run=screens.arg(command,'DCAmmoOffRun','[A-Za-z0-9_]{1,64}')
    folder=R/run; folder.mkdir(parents=True,exist_ok=True)
    output=folder/'report.json'; assert not output.exists()
    logs=re.findall(r'"-abslog=([^"\r\n]+)"|-abslog="([^"\r\n]+)"|-abslog=([^\s"\r\n]+)',command,re.I)
    assert len(logs)==1
    supplied=Path(next(value for value in logs[0] if value).replace('\\','/'))
    assert supplied.is_absolute() and supplied.resolve()==(folder/'Editor.log').resolve()
    before_sha,protected=hashes(),protection()
    assert rich.sha(disk(PANEL))==PANEL_SHA
    report={'mode':mode,'run':run,'complete':False,'asset_saves':[],'compile_requested':[],'PIE':False,'cpp_build':False,
        'visual_verified':False,'script_sha256':rich.sha(SCRIPT),'before_sha256':before_sha,'protected_before':protected}
    try:
        settings=hud.export(ue.get_default_object(ue.load_class(None,'/Script/BlueprintGraph.BlueprintEditorSettings')),folder/'settings.t3d')
        assert re.findall(r'^\s*SaveOnCompile=(\S+)\s*$',settings,re.M)==['SoC_Never']
        assets={key:ue.load_asset(package) for key,package in {**TARGETS,'panel':PANEL}.items()}; assert all(assets.values())
        report['before']=snapshot(assets,folder,'before')
        if mode=='inspect':
            assert before_sha==BASE; inspect(assets,report)
        else:
            reference=screens.arg(command,'DCAmmoOffReference','[A-Za-z0-9_]{1,64}')
            previous=rich.read(R/reference/'report.json')
            assert previous['complete'] and previous['protection_passed']
            assert previous['mode']==('inspect' if mode=='apply' else 'apply')
            assert previous['script_sha256']==report['script_sha256']
            assert previous['protected_before']==protected and before_sha==previous['after_sha256']
            report['reference']=reference
            if mode=='apply':
                assert before_sha==BASE and direct.normalized(report['before'])==direct.normalized(previous['before'])
                apply(assets,previous,folder,report)
            else: verify(assets,previous,folder,report)
        hud.clean((TARGETS['row'],) if mode=='verify' else ()); hud.log_check(folder/'Editor.log'); report['complete']=True
    except Exception:
        report['error']=traceback.format_exc(); raise
    finally:
        report['after_sha256']=hashes(); report['protection_passed']=protection()==protected
        if not report['protection_passed'] or (mode!='apply' and report['after_sha256']!=before_sha): report['complete']=False
        output.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
        ue.log('DC_AMMO_OFF_STYLE '+json.dumps({'mode':mode,'complete':report['complete'],'saves':report['asset_saves']}))


if __name__=='__main__': main()
