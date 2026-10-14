"""Approved R7 Host-only port and integration Controller assignment; no C++ build.

Run in separate guarded Python commandlets with -DCR7HostMode=prepare|source-reload|assign|verify
and -DCR7HostRun=<fresh_name>. Prepare/source-reload run in the sibling Lyra project.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import sys
sys.dont_write_bytecode = True
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / "LyraStarterGame"
ORIGINAL = "/ShooterCore/UserInterface/HUD/W_WeaponReticleHost"
HOST = "/Game/LyraMigration/UI/R7/W_WeaponReticleHost"
CONTROLLER = "/Game/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleController"
CORE = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/"
REPORT = PROJECT / "Saved/Diagnostics/R7ReticleHost"
EXTENSIONS = (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl")
LOG = None


def require(value, message):
    if not value:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_R7_HOST " + json.dumps(dict(event=event, **values), ensure_ascii=False))


def digest(path):
    if not path.is_file():
        return None
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def files(root, package):
    mount, relative = package[1:].split("/", 1)
    require(mount in ("Game", "ShooterCore") and ".." not in relative, "Unapproved package")
    content = root / ("Content" if mount == "Game" else "Plugins/GameFeatures/ShooterCore/Content")
    return [(content / relative).with_suffix(ext) for ext in EXTENSIONS]


def states(paths):
    return {str(path): digest(path) for path in paths}


def unchanged(before):
    require(all(digest(Path(p)) == h for p, h in before.items()), "Protected files changed")


def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; save refused")
    errors = [line[:1200] for line in LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
              if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine diagnostics; save refused: " + repr(errors[:3]))


def cdo(bp):
    return ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))


def snapshot(bp):
    # Reuse the existing read-only graph collector, excluding its top-level main.
    tree = ast.parse((PROJECT / "Scripts/Editor/r5_rifle_source_audit.py").read_text(encoding="utf-8-sig"))
    functions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name in ("graphs",)]
    records = []
    scope = dict(unreal=ue, emit=lambda event, **data: records.append(dict(event=event, **data)))
    exec(compile(ast.Module(body=functions, type_ignores=[]), "existing_graph_reader", "exec"), scope)
    scope["graphs"](bp, bp.get_path_name().split(".")[0])
    require(all(r["event"] == "graph" for r in records), "Graph inspection incomplete")
    structural = []
    for row in records:
        nodes = []
        for node in row["nodes"]:
            require("unverified" not in node, "Node inspection incomplete")
            # Display titles/types change with native class names. Preserve raw reports,
            # compare node identity, pin identity/value and every connection separately.
            nodes.append(dict(name=node["name"], cls=node["class"], pins=[
                dict(name=p["name"], direction=p["direction"], value=p["value"],
                     links=sorted(p["links"], key=lambda link: (link["node"], link["pin"])))
                for p in node.get("pins", [])]))
        structural.append(dict(name=row["name"], nodes=sorted(nodes, key=lambda n: n["name"])))
    widgets = []
    prefix = bp.get_path_name() + ":WidgetTree."
    for widget in ue.ObjectIterator(ue.Widget):
        if widget.get_path_name().startswith(prefix):
            parent = widget.get_parent()
            slot = widget.get_editor_property("Slot")
            widgets.append(dict(name=widget.get_name(), cls=widget.get_class().get_path_name(),
                                parent=parent.get_name() if parent else None,
                                visibility=str(widget.get_visibility()),
                                slot=slot.get_class().get_path_name() if slot else None))
    require(sorted(w["name"] for w in widgets) == ["VisWrapper", "WidgetStack"], "Unexpected Host tree")
    values = {name: scope_value(cdo(bp).get_editor_property(name)) for name in
              ("CurrentHealthComponent", "CurrentPawn", "NewHealthComponent", "SpawnedWidgets")}
    return dict(graphs=sorted(structural, key=lambda r: r["name"]),
                widgets=sorted(widgets, key=lambda r: r["name"]), defaults=values, raw_graphs=records)


def scope_value(value):
    if value is None:
        return None
    if isinstance(value, ue.Object):
        return value.get_path_name()
    if isinstance(value, (list, ue.Array)):
        return [scope_value(v) for v in value]
    return str(value)


def comparable(snapshot_value):
    result = {key: value for key, value in snapshot_value.items() if key != "raw_graphs"}
    text = json.dumps(result, sort_keys=True).replace(ORIGINAL, HOST)
    # Existing project redirects are the only permitted native type substitutions.
    ini = (PROJECT / "Config/DefaultEngine.ini").read_text(encoding="utf-8-sig")
    for old, new in re.findall(r'ClassRedirects=\(OldName="([^"]+)",NewName="([^"]+)"\)', ini):
        text = text.replace(old, new)
    # K2 reconstructs this cast output pin from the redirected native class name.
    # Both the declaration and its linked input changed together; no wire changed.
    text = text.replace("AsLyra Inventory Item Instance", "AsDCInventory Item Instance")
    return json.loads(text)


def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR7HostMode=(prepare|source-reload|assign|verify)(?:\s|$)", command)
    run = re.search(r"-DCR7HostRun=([A-Za-z0-9_]{1,32})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit commandlet mode/run required")
    mode, run = mode.group(1), run.group(1)
    LOG = PROJECT / "Saved/Logs" / ("R7Host-" + run + ".log")
    require(LOG.is_file(), "Expected fresh per-run log")
    actual = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    require(actual == (SOURCE if mode in ("prepare", "source-reload") else PROJECT).resolve(), "Wrong project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context rejected")
    protected_paths = files(SOURCE, ORIGINAL)
    for package in (CORE + "ID_Rifle", CORE + "W_Reticle_Rifle", CORE + "W_AmmoCounter_Rifle"):
        protected_paths += files(PROJECT, package)
    protected_paths += [PROJECT / p for p in ("Config/DefaultEngine.ini", "Config/DefaultGame.ini", "Config/DefaultInput.ini",
        "DreamCatcher.uproject", ".git/index", "Source/DreamCatcher/Development/DCRifleIntegrationActors.h",
        "Source/DreamCatcher/Development/DCRifleIntegrationActors.cpp")]
    if mode != "assign":
        protected_paths += files(PROJECT, CONTROLLER)
    if mode == "verify":
        protected_paths += files(PROJECT, HOST)
    if mode == "source-reload":
        protected_paths += files(SOURCE, HOST) + files(PROJECT, HOST)
    before = states(protected_paths)
    require(digest(files(SOURCE, ORIGINAL)[0]) == "fbab64d38d1ee305921176263df4fbea5021440a4cf6948d5bc79ca5ee138430", "Original source changed")
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    REPORT.mkdir(parents=True, exist_ok=True)
    receipt = REPORT / (run + ".json")
    require(not receipt.exists(), "Use a fresh run name")
    clean()
    try:
        if mode == "source-reload":
            # A migration-time search-index ensure is NOT a successful migration verdict.
            # Revalidate its saved output in a clean process without overwriting anything.
            original = ue.load_asset(ORIGINAL)
            duplicate = ue.load_asset(HOST)
            require(original and duplicate, "Saved source assets missing")
            original_snapshot = snapshot(original)
            ue.BlueprintEditorLibrary.compile_blueprint(duplicate)
            cloned_snapshot = snapshot(duplicate)
            require(comparable(original_snapshot) == comparable(cloned_snapshot), "Reloaded source copy differs")
            require(files(PROJECT, HOST)[0].is_file(), "Migrated target missing")
            bytes_equal = all(digest(a) == digest(b) for a, b in zip(files(SOURCE, HOST), files(PROJECT, HOST)))
            clean()
            result = dict(mode=mode, source_snapshot=original_snapshot, prepared_snapshot=cloned_snapshot,
                          source_target_bytes_equal=bytes_equal, target_semantics_verified=False,
                          protected=before, asset_saves=0,
                          prior_migration_ensure_retained=True)
        elif mode == "prepare":
            require(all(not p.exists() for p in files(SOURCE, HOST) + files(PROJECT, HOST)), "New-only source and target destination required")
            original = ue.load_asset(ORIGINAL)
            require(original, "Original Host missing")
            dependencies = sorted(str(d) for d in registry.get_dependencies(ORIGINAL, ue.AssetRegistryDependencyOptions()))
            require(all(d.startswith(("/Engine/", "/Script/")) for d in dependencies), "Unexpected content dependency")
            original_snapshot = snapshot(original)
            duplicate = editor.duplicate_asset(ORIGINAL, HOST)
            require(duplicate, "Host duplication failed")
            ue.BlueprintEditorLibrary.compile_blueprint(duplicate)
            cloned_snapshot = snapshot(duplicate)
            require(comparable(original_snapshot) == comparable(cloned_snapshot), "Duplicate graph/tree/default mismatch")
            clean()
            require(editor.save_loaded_asset(duplicate, False), "Staging Host save failed")
            opts = ue.MigrationOptions()
            opts.set_editor_property("prompt", False)
            opts.set_editor_property("ignore_dependencies", True)
            opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
            ue.AssetToolsHelpers.get_asset_tools().migrate_packages([HOST], str(PROJECT / "Content"), opts)
            require(files(PROJECT, HOST)[0].is_file(), "Migrated Host missing")
            result = dict(mode=mode, source_snapshot=original_snapshot, prepared_snapshot=cloned_snapshot,
                          dependencies=dependencies, target_new_packages=[HOST], protected=before)
        else:
            source_receipts = list(REPORT.glob("prepare_*.json"))
            require(len(source_receipts) == 1, "One successful source prepare receipt required")
            source = json.loads(source_receipts[0].read_text(encoding="utf-8"))["source_snapshot"]
            host = ue.load_asset(HOST)
            require(host, "Host load failed")
            ue.BlueprintEditorLibrary.compile_blueprint(host)
            parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(host).get_path_name()
            require(parent == "/Script/DreamCatcher.DCLyraWeaponUserInterface", "Wrong native Host parent")
            current = snapshot(host)
            if comparable(source) != comparable(current):
                (REPORT / (run + "_comparison.json")).write_text(json.dumps(
                    dict(source=comparable(source), target=comparable(current)), ensure_ascii=False, indent=2), encoding="utf-8")
            require(comparable(source) == comparable(current), "Port graph/tree/default mismatch")
            pc = ue.load_asset(CONTROLLER)
            require(pc, "Controller load failed")
            host_class = ue.BlueprintEditorLibrary.generated_class(host)
            previous = cdo(pc).get_editor_property("WeaponReticleHostClass")
            if mode == "assign":
                require(previous is None, "Refuse to overwrite an existing Host class")
                cdo(pc).set_editor_property("WeaponReticleHostClass", host_class)
                ue.BlueprintEditorLibrary.compile_blueprint(pc)
                clean()
                # Resave only the new Host to persist native redirects, then the approved PC field.
                require(editor.save_loaded_asset(host, False), "Port Host save failed")
                require(editor.save_loaded_asset(pc, False), "Controller save failed")
            else:
                ue.BlueprintEditorLibrary.compile_blueprint(pc)
            require(cdo(pc).get_editor_property("WeaponReticleHostClass") == host_class, "Controller class not retained")
            clean()
            dependencies = sorted(str(d) for d in registry.get_dependencies(HOST, ue.AssetRegistryDependencyOptions()))
            require(not any(d.startswith("/ShooterCore/") for d in dependencies), "Residual ShooterCore dependency")
            result = dict(mode=mode, parent=parent, host_snapshot=current, dependencies=dependencies,
                          controller_class=host_class.get_path_name(), protected=before, asset_saves=2 if mode == "assign" else 0)
        clean()
        unchanged(before)
        receipt.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        emit("complete", mode=mode, report=str(receipt), protected_unchanged=True, cpp_build=False, packaging=False)
    finally:
        unchanged(before)


if __name__ == "__main__":
    main()
