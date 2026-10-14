"""Read-only registry plan for the verified 18-asset Rifle staging bundle.

No asset loads/Compile/copy/save/Migrate. Only registry queries and file reads.
Run in original Lyra with -DCRiflePlanRun=RifleCore_Prepared_<name>.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import sys
from collections import deque

import unreal

sys.path.insert(0, str(Path(__file__).resolve().parent))
from r5_rifle_core_contract import HERO_PACKAGE, EXTENSIONS, build_plan, require


def emit(event, **data):
    print("DC_RIFLE_DEPENDENCIES " + json.dumps({"event": event, **data}, ensure_ascii=False))


def main():
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    target = Path(__file__).resolve().parents[2]
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(os.path.normcase(str(project)) == os.path.normcase(str((target.parent / "LyraStarterGame").resolve())),
            "Original Lyra only")
    bridge = unreal.DCRifleMigrationLibrary
    require(bridge.validate_diagnostic_context() is not None, "Diagnostic context rejected")
    match = re.search(r"(?:^|\s)-DCRiflePlanRun=(\S+)", command)
    require(match, "Explicit saved run name required")
    plan = build_plan(list(bridge.get_approved_rifle_closure_packages()), match.group(1))
    for package in plan.values():
        require((project / "Content" / (package[len("/Game/"):] + ".uasset")).is_file(), "Saved root missing: " + package)
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.search_all_assets(True)
    options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
        include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
    pending = deque(plan.values())
    parents = {package: None for package in plan.values()}
    visited, external, edges = set(), set(), {}
    while pending:
        package = pending.popleft()
        if package.startswith(("/Engine/", "/Script/")):
            external.add(package)
            continue
        visited.add(package)
        deps = registry.get_dependencies(package, options)
        require(deps is not None, "Unreadable registry package: " + package)
        edges[package] = sorted(str(dep) for dep in deps)
        for dep in edges[package]:
            if dep not in parents:
                parents[dep] = package
                pending.append(dep)

    def chain(package):
        result = []
        while package is not None:
            result.append(package)
            package = parents[package]
        return result[::-1]

    collisions = []
    for package in sorted(visited):
        if not package.startswith("/Game/") or package in plan.values():
            continue
        relative = package[len("/Game/"):]
        if not any((target / "Content" / (relative + ext)).exists() for ext in (".uasset", ".umap")):
            continue
        diffs = []
        for ext in EXTENSIONS + (".umap",):
            hashes = []
            for root in (project, target):
                path = root / "Content" / (relative + ext)
                digest = None
                if path.is_file():
                    with path.open("rb") as stream:
                        digest = hashlib.file_digest(stream, "sha256").hexdigest()
                hashes.append(digest)
            if hashes[0] != hashes[1]:
                diffs.append(ext)
        collisions.append({"package": package, "byte_identical": not diffs, "different_files": diffs, "chain": chain(package)})
    shooter = sorted(package for package in visited if package.startswith("/ShooterCore/"))
    native_assets = []
    for package in sorted(visited):
        for data in registry.get_assets_by_package_name(package, True):
            # Registry metadata only: do not call GetAsset/GetClass or load a package.
            native_assets.append({"package": package, "asset": data.get_export_text_name(),
                                  "parent": data.get_tag_value("ParentClass"),
                                  "native_parent": data.get_tag_value("NativeParentClass")})
    emit("plan", saved_run=match.group(1), package_count=len(visited), engine_script_count=len(external),
         remaining_shooter_core=[{"package": package, "chain": chain(package)} for package in shooter],
         original_core_references=sorted(set(plan) & visited), original_hero_reachable=HERO_PACKAGE in visited,
         collisions=collisions, package_edges=edges, native_assets=native_assets,
         note="Hash differences are not proof of gameplay/property differences. No overwrite/reuse decision made.")
    require(bridge.get_engine_ensure_failure_count() == 0, "Engine ensure during registry plan")
    emit("complete", asset_writes=0, explicit_asset_loads=0, cpp_build=False, packaging=False)


main()
