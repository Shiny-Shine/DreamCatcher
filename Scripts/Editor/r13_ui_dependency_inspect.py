"""R13 bounded read-only dependency inventory. No asset migration/save/Compile.
Run in original Lyra: -DCR13UIDependencyRun=<fresh receipt>.
Existing source UI audit is reused as seeds; this does not repeat completed gameplay tests.
"""
import hashlib,json,re
from pathlib import Path
import unreal as ue

P=Path('E:/Epic Games/UEProjects/DreamCatcher')
S=P.parent/'LyraStarterGame'
cmd=ue.SystemLibrary.get_command_line()
run=re.search(r'-DCR13UIDependencyRun=([A-Za-z0-9_]+)',cmd).group(1)
out=P/'Saved/Diagnostics/R13'/('ui_dependencies_'+run+'.json');assert not out.exists()
assert Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()==S.resolve()
ROOTS=['/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD','/ShooterCore/UserInterface/W_ShooterHUDLayout',
       '/Game/UI/Hud/W_LyraGameMenu','/Game/UI/Foundation/Dialogs/W_ControllerDisconnected']
registry=ue.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
options=ue.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,
    include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
pending=list(ROOTS);packages={};external=set();hashes={}
while pending:
    package=pending.pop()
    if package in packages:continue
    if not package.startswith(('/Game/','/ShooterCore/')):external.add(package);continue
    assert len(packages)<2000,'Dependency bound exceeded; inspect before expanding the asset scope'
    records=registry.get_assets_by_package_name(package)
    rows=[]
    for record in records:
        rows.append(dict(name=str(record.asset_name),class_path=str(record.asset_class_path),
            native_parent=str(record.get_tag_value('NativeParentClass')),parent=str(record.get_tag_value('ParentClass'))))
    dependencies=[str(x) for x in registry.get_dependencies(package,options)]
    packages[package]=dict(assets=rows,dependencies=dependencies)
    pending.extend(dependencies)
    mount,relative=package[1:].split('/',1)
    base=S/('Content' if mount=='Game' else 'Plugins/GameFeatures/ShooterCore/Content')/relative
    for ext in ['.uasset','.umap','.uexp','.ubulk']:
        f=base.with_suffix(ext)
        if f.exists():hashes[str(f)]=hashlib.sha256(f.read_bytes()).hexdigest()
result=dict(complete=True,roots=ROOTS,packages=packages,external=sorted(external),source_hashes=hashes,
            source_asset_loads=0,asset_saves=0,blueprint_compiles=0,PIE=False)
out.write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print('R13_UI_DEPENDENCIES '+str(dict(packages=len(packages),files=len(hashes),asset_saves=0)))
