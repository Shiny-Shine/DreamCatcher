"""Approved two-asset Root UI port; run prepare, migrate, verify in separate commandlets.

No C++ build, global asset replacement, or existing HUD/Reticle changes.
Requires -DCR7RootMode=prepare|migrate|verify -DCR7RootRun=<fresh_name>.
"""
import ast
import hashlib
import json
from pathlib import Path
import re
import unreal as ue

PROJECT = Path(__file__).resolve().parents[2]
SOURCE = PROJECT.parent / "LyraStarterGame"
REPORT = PROJECT / "Saved/Diagnostics/R7RootUI"
NAMES = ("W_OverallUILayout", "B_LyraUIPolicy")
MAPPING = {"/Game/UI/" + n: "/Game/LyraMigration/UI/R7/Foundation/" + n for n in NAMES}
EXTENSIONS = (".uasset", ".uexp", ".ubulk", ".uptnl")
LOG = None


def require(value, message):
    if not value:
        raise RuntimeError(message)


def digest(path):
    if not path.is_file():
        return None
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def asset_files(root, package):
    require(package.startswith("/Game/") and ".." not in package, "Unexpected package")
    return [(root / "Content" / package[6:]).with_suffix(e) for e in EXTENSIONS]


def clean():
    require(ue.DCRifleMigrationLibrary.get_engine_ensure_failure_count() == 0, "Engine ensure; stop")
    lines = LOG.read_text(encoding="utf-8-sig", errors="replace").splitlines()
    errors = [line[:1000] for line in lines if re.search(r": Error:|Fatal:|Ensure condition failed|Invalid GameplayTag", line)]
    require(not errors, "Engine errors: " + repr(errors[:3]))


def normalize(value):
    text = json.dumps(value, sort_keys=True)
    for old, new in MAPPING.items():
        text = text.replace(old, new)
    return json.loads(text)


def snapshot(bp):
    # Reuse the existing reader without running its top-level rifle audit.
    tree = ast.parse((PROJECT / "Scripts/Editor/r5_rifle_source_audit.py").read_text(encoding="utf-8-sig"))
    functions = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "graphs"]
    records = []
    scope = dict(unreal=ue, emit=lambda event, **data: records.append(dict(event=event, **data)))
    exec(compile(ast.Module(body=functions, type_ignores=[]), "existing_graph_reader", "exec"), scope)
    scope["graphs"](bp, bp.get_path_name().split(".")[0])
    require(all(row["event"] == "graph" for row in records), "Incomplete graph read")
    graphs = []
    for row in records:
        require(all("unverified" not in node for node in row["nodes"]), "Incomplete node read")
        graphs.append(dict(name=row["name"], nodes=sorted(row["nodes"], key=lambda n: n["name"])))
    widgets = []
    for w in ue.ObjectIterator(ue.Widget):
        if not w.get_path_name().startswith(bp.get_path_name() + ":WidgetTree."):
            continue
        parent, slot = w.get_parent(), w.get_editor_property("Slot")
        values = {}
        # Publicly reflected designer values; fail instead of silently skipping.
        for name in ("RenderTransform", "RenderTransformPivot", "Clipping", "bIsEnabled"):
            value = w.get_editor_property(name)
            values[name] = value.export_text() if callable(getattr(value, "export_text", None)) else str(value)
        widgets.append(dict(name=w.get_name(), cls=w.get_class().get_path_name(),
                            parent=parent.get_name() if parent else None,
                            slot=slot.get_class().get_path_name() if slot else None,
                            visibility=str(w.get_visibility()), values=values))
    cdo = ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(bp))
    defaults = {}
    if bp.get_name() == "B_LyraUIPolicy":
        value = cdo.get_editor_property("LayoutClass")
        defaults["LayoutClass"] = value.get_path_name() if isinstance(value, ue.Object) else str(value)
    return dict(parent=ue.BlueprintEditorLibrary.get_blueprint_parent_class(bp).get_path_name(),
                graphs=sorted(graphs, key=lambda r: r["name"]),
                widgets=sorted(widgets, key=lambda w: w["name"]), defaults=defaults)


def main():
    global LOG
    command = ue.SystemLibrary.get_command_line()
    mode = re.search(r"-DCR7RootMode=(prepare|migrate|verify)(?:\s|$)", command)
    run = re.search(r"-DCR7RootRun=([A-Za-z0-9_]{1,40})(?:\s|$)", command)
    require(mode and run and "-run=pythonscript" in command.lower(), "Explicit commandlet mode/run required")
    mode, run = mode.group(1), run.group(1)
    actual = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    require(actual == (PROJECT if mode == "verify" else SOURCE).resolve(), "Wrong project")
    guard = ue.DCRifleMigrationLibrary.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Context rejected")
    LOG = PROJECT / "Saved/Logs" / ("R7Root-" + run + ".log")
    require(LOG.is_file(), "Fresh named log required")
    REPORT.mkdir(parents=True, exist_ok=True)
    receipt = REPORT / (run + ".json")
    require(not receipt.exists(), "Fresh run name required")
    originals = [p for package in MAPPING for p in asset_files(SOURCE, package)]
    protected = originals + [PROJECT / p for p in (
        "Config/DefaultGame.ini", "Config/DefaultGameplayTags.ini", "Config/DefaultEngine.ini",
        "DreamCatcher.uproject", ".git/index", "Binaries/Win64/UnrealEditor-DreamCatcher.dll",
        "Source/DreamCatcher/Development/DCRifleIntegrationActors.cpp",
        "Source/DreamCatcher/Development/DCRifleIntegrationActors.h",
        "Content/DreamCatcher/GAS/Test/R5/Integration/BP_DC_RifleController.uasset",
        "Content/DreamCatcher/GAS/Test/R5/Integration/L_DC_RifleIntegration.umap",
        "Content/LyraMigration/UI/R7/W_WeaponReticleHost.uasset")]
    if mode != "prepare":
        protected += [p for package in MAPPING.values() for p in asset_files(SOURCE, package)]
    if mode == "verify":
        protected += [p for package in MAPPING.values() for p in asset_files(PROJECT, package)]
    before = {str(p): digest(p) for p in protected}
    audit = json.loads((PROJECT / "Saved/Diagnostics/R5UIFoundation/UIAudit_20261008_A3/source.json").read_text(encoding="utf-8-sig"))
    for original in originals:
        if original.suffix == ".uasset":
            expected = {str(Path(p)): h for p, h in audit["source_file_states"].items()}
            require(digest(original) == expected[str(original)], "Original changed since approved audit")
    registry = ue.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    editor = ue.get_editor_subsystem(ue.EditorAssetSubsystem)
    result = dict(mode=mode, protected=before, snapshots={}, dependencies={})
    clean()
    try:
        if mode == "prepare":
            for dest in MAPPING.values():
                require(all(not p.exists() for root in (SOURCE, PROJECT) for p in asset_files(root, dest)), "New-only destination required")
            copies = {}
            for original, dest in MAPPING.items():
                bp = ue.load_asset(original)
                require(bp, "Original missing")
                result["snapshots"][original] = snapshot(bp)
                deps = [str(p) for p in registry.get_dependencies(original, ue.AssetRegistryDependencyOptions())]
                require(all(p in MAPPING or p.startswith("/Script/") for p in deps), "Unexpected content dependency")
                result["dependencies"][original] = deps
                copies[original] = editor.duplicate_asset(original, dest)
                require(copies[original], "Duplication failed")
            policy = copies["/Game/UI/B_LyraUIPolicy"]
            layout_class = ue.BlueprintEditorLibrary.generated_class(copies["/Game/UI/W_OverallUILayout"])
            ue.get_default_object(ue.BlueprintEditorLibrary.generated_class(policy)).set_editor_property("LayoutClass", layout_class)
            for original, bp in copies.items():
                ue.BlueprintEditorLibrary.compile_blueprint(bp)
                require(normalize(result["snapshots"][original]) == normalize(snapshot(bp)), "Copy graph/tree/default mismatch")
            clean()
            for bp in copies.values():
                require(editor.save_loaded_asset(bp, False), "Scoped save failed")
            result["asset_saves"] = 2
        else:
            receipts = [json.loads(p.read_text(encoding="utf-8")) for p in REPORT.glob("prepare_*.json")]
            require(len(receipts) == 1, "One successful prepare receipt required")
            if mode == "migrate":
                require(ue.SystemLibrary.get_console_variable_int_value("AssetTools.UseNewPackageMigration") == 0,
                        "Use existing engine legacy migration in this isolated process")
                require(all(not p.exists() for dest in MAPPING.values() for p in asset_files(PROJECT, dest)), "Target must be new")
            for original, dest in MAPPING.items():
                bp = ue.load_asset(dest)
                require(bp, "Prepared/migrated Blueprint missing")
                ue.BlueprintEditorLibrary.compile_blueprint(bp)
                result["snapshots"][dest] = snapshot(bp)
                require(normalize(receipts[0]["snapshots"][original]) == normalize(result["snapshots"][dest]), "Reload graph/tree/default mismatch")
                deps = [str(p) for p in registry.get_dependencies(dest, ue.AssetRegistryDependencyOptions())]
                require(all(p in MAPPING.values() or p.startswith("/Script/") for p in deps), "Residual original/extra content dependency")
                result["dependencies"][dest] = deps
            clean()
            if mode == "migrate":
                opts = ue.MigrationOptions()
                opts.set_editor_property("prompt", False)
                opts.set_editor_property("ignore_dependencies", True)
                opts.set_editor_property("asset_conflict", ue.AssetMigrationConflict.SKIP)
                ue.AssetToolsHelpers.get_asset_tools().migrate_packages(list(MAPPING.values()), str(PROJECT / "Content"), opts)
                require(all(asset_files(PROJECT, p)[0].is_file() for p in MAPPING.values()), "Migration output missing")
            result["asset_saves"] = 0
        clean()
        require(all(digest(Path(p)) == h for p, h in before.items()), "Protected file changed")
        receipt.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
        print("DC_R7_ROOT " + json.dumps(dict(event="complete", mode=mode, report=str(receipt), protected_unchanged=True)))
    finally:
        require(all(digest(Path(p)) == h for p, h in before.items()), "Protected file changed")


if __name__ == "__main__":
    main()
