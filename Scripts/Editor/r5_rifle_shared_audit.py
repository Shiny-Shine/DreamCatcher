"""Read-only asset audit in each project; reports only, never asset save/Compile/copy.

-DCRifleAuditRun=<fresh ASCII name> [-DCRifleAuditLimit=<smoke count>]
Uses the verified dependency-plan log's fixed 114 collisions and two named auxiliaries.
Exporter reports go ONLY to DreamCatcher/Saved/Diagnostics/R5SharedAudit/<run>/<project>.
T3D/reflection comparison is not proof of bulk-data or runtime equivalence.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import sys

import unreal

sys.dont_write_bytecode = True


def require(value, message):
    if not value:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_RIFLE_SHARED " + json.dumps({"event": event, **values}, ensure_ascii=False))


def option(command, key, fallback=""):
    match = re.search(r"(?:^|\s)-" + re.escape(key) + r"=(\S+)", command)
    return match.group(1) if match else fallback


def hashes(project, packages):
    result = {}
    for package in packages:
        if package.startswith("/Game/"):
            root, relative = project / "Content", package[6:]
        elif package.startswith("/ShooterCore/"):
            root, relative = project / "Plugins/GameFeatures/ShooterCore/Content", package[13:]
        else:
            raise RuntimeError("Unapproved file root: " + package)
        for extension in (".uasset", ".uexp", ".ubulk", ".uptnl"):
            path = (root / (relative + extension)).resolve()
            require(path.is_relative_to(root.resolve()), "File path escaped asset mount")
            require(extension != ".uasset" or path.is_file(), "Asset file missing: " + str(path))
            digest = None
            if path.is_file():
                with path.open("rb") as stream:
                    digest = hashlib.file_digest(stream, "sha256").hexdigest()
            result[str(path)] = digest
    return result


def encode(value):
    if value is None or isinstance(value, (str, bool, float, int)):
        return value
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, (tuple, list, unreal.Array)):
        return [encode(item) for item in value]
    if isinstance(value, (dict, unreal.Map)):
        return {str(encode(k)): encode(v) for k, v in value.items()}
    if callable(getattr(value, "export_text", None)):
        return value.export_text()
    return str(value)


def reflected_fields(obj):
    # Public Python documentation names only; no flag changes or protected-property bypass.
    names = set()
    for cls in type(obj).__mro__:
        names.update(re.findall(r"-\s+``([a-zA-Z_][a-zA-Z_0-9]*)``", cls.__doc__ or ""))
    values, unavailable = {}, {}
    for name in sorted(names):
        try:
            values[name] = encode(obj.get_editor_property(name))
        except Exception as error:
            unavailable[name] = str(error)
    return {"values": values, "unavailable": unavailable}


def export_report(obj, output, exporter):
    require(not output.exists(), "Report overwrite refused: " + str(output))
    task = unreal.AssetExportTask()
    task.object = obj
    if exporter is not None:
        task.exporter = exporter
    task.filename = str(output)
    task.automated = True
    task.prompt = False
    task.replace_identical = False
    success = unreal.Exporter.run_asset_export_task(task)
    return {"success": bool(success and output.is_file()), "path": str(output), "errors": list(task.errors)}


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    target = Path(__file__).resolve().parents[2]
    original = target.parent / "LyraStarterGame"
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    is_original = os.path.normcase(str(project)) == os.path.normcase(str(original.resolve()))
    require(is_original or os.path.normcase(str(project)) == os.path.normcase(str(target)), "Unexpected project")
    bridge = unreal.DCRifleMigrationLibrary
    require(bridge.validate_diagnostic_context() is not None, "Context rejected")
    run = option(command, "DCRifleAuditRun")
    require(re.fullmatch(r"[A-Za-z0-9_]{1,48}", run), "Explicit fresh run name required")
    reports_root = (target / "Saved/Diagnostics/R5SharedAudit").resolve()
    output = reports_root / run / project.name
    require(output.resolve().is_relative_to(reports_root) and not output.exists(), "Report folder must be new")
    records = []
    for line in (original / "Saved/Logs/R5Closure18-dependency-plan.log").read_text(encoding="utf-8-sig").splitlines():
        if "DC_RIFLE_DEPENDENCIES " in line:
            records.append(json.loads(line.split("DC_RIFLE_DEPENDENCIES ", 1)[1]))
    plans = [record for record in records if record["event"] == "plan"]
    require(len(plans) == 1 and plans[0]["saved_run"] == "RifleCore_Prepared_0247", "Expected verified plan")
    collisions = sorted(item["package"] for item in plans[0]["collisions"])
    require(len(collisions) == len(set(collisions)) == 114, "Collision set changed; review first")
    limit = int(option(command, "DCRifleAuditLimit", "114"))
    require(1 <= limit <= 114, "Invalid smoke limit")
    selected = [(f"C{index:03d}", package) for index, package in enumerate(collisions[:limit])]
    focus = option(command, "DCRifleAuditFocus")
    require(focus in ("", "mesh", "reticle"), "Unsupported audit focus")
    if focus == "mesh":
        selected = [(key, package) for key, package in selected if package == "/Game/Weapons/Rifle/Mesh/SK_Rifle"]
        require(len(selected) == 1, "Mesh focus requires full collision list")
    if focus == "reticle":
        require(is_original, "Original reticle inspection only")
        selected = [("Reticle", "/ShooterCore/Weapons/Rifle/W_Reticle_Rifle")]
    selected.append(("StructUI", "/ShooterCore/Input/Abilities/Struct_UIMessaging" if is_original
                     else "/Game/LyraMigration/ADS/Struct_UIMessaging"))
    if is_original:
        selected.append(("RifleDamage", "/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto"))
    packages = [package for _, package in selected]
    before = hashes(project, packages)
    output.mkdir(parents=True, exist_ok=False)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
    emit("begin", project=project.name, reports=str(output), assets=len(selected), cpp_build=False, packaging=False)
    unavailable_assets = []
    try:
        for key, package in selected:
            asset = unreal.load_asset(package)
            if asset is None:
                unavailable_assets.append(package)
                emit("asset_unverified", key=key, package=package, reason="Load failed; no repair, save, or assumed equivalence")
                continue
            parent = None
            subject = asset
            if isinstance(asset, unreal.Blueprint):
                cls = unreal.BlueprintEditorLibrary.generated_class(asset)
                require(cls is not None, "Missing generated class: " + package)
                subject = unreal.get_default_object(cls)
                parent = unreal.BlueprintEditorLibrary.get_blueprint_parent_class(asset).get_path_name()
            if key == "Reticle":
                widgets = [{"path": widget.get_path_name(), "outside_radius": widget.get_editor_property("bReticleCornerOutsideSpreadRadius"),
                            "radius": widget.get_editor_property("Radius")}
                           for widget in unreal.ObjectIterator(unreal.CircumferenceMarkerWidget)
                           if widget.get_path_name().startswith(package + ".")]
                emit("reticle_widgets", widgets=widgets)
            text = export_report(asset, output / (key + ".t3d"), unreal.ObjectExporterT3D())
            cdo = export_report(subject, output / (key + "_cdo.t3d"), unreal.ObjectExporterT3D()) if subject != asset else None
            pixels = None
            if asset.get_class().get_name() == "Texture2D":
                # Let FindExporter check SupportsObject/SupportsTexture. Supplying a
                # TGA exporter explicitly skips that check and asserts on unsupported formats.
                pixels = export_report(asset, output / (key + ".tga"), None)
                if not pixels["success"]:
                    pixels = export_report(asset, output / (key + ".dds"), None)
                if not pixels["success"]:
                    pixels = export_report(asset, output / (key + ".hdr"), None)
            mesh_metrics = None
            if asset.get_class().get_name() == "SkeletalMesh":
                mesh_editor = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
                mesh_metrics = [{"lod": lod, "vertices": mesh_editor.get_num_verts(asset, lod),
                                 "sections": mesh_editor.get_num_sections(asset, lod)}
                                for lod in range(mesh_editor.get_lod_count(asset))]
            emit("asset", key=key, package=package, asset_class=asset.get_class().get_path_name(), parent=parent,
                 dependencies=sorted(str(dep) for dep in registry.get_dependencies(package, options)),
                 fields=reflected_fields(subject), text=text, cdo_text=cdo, texture_pixels=pixels, mesh_metrics=mesh_metrics)
            require(bridge.get_engine_ensure_failure_count() == 0, "Engine ensure; stop read-only audit")
    finally:
        after = hashes(project, packages)
        require(before == after, "Inspected asset file state changed")
        emit("file_states_unchanged", count=len(before))
    dirty = unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()
    emit("complete", project=project.name, asset_writes=0, reports_only=True, explicit_compile=False,
         unavailable_assets=unavailable_assets, dirty_packages=sorted(package.get_path_name() for package in dirty),
         ensure_failures=bridge.get_engine_ensure_failure_count())


main()
