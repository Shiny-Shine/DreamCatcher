"""Bounded read-only source/target audit; no asset save, Compile, copy or Config edits.

Source and target: -run=pythonscript -DCRifleCopyDiagnostics -DCRifleNativeContractAudit
                  -DCRifleNativeAuditRun=<fresh_name>
Run source first, then target with the SAME run name. Reports go only under
DreamCatcher/Saved/Diagnostics/R5NativeContracts/<run>. No structural match proves
non-reflected C++ behavior or runtime equivalence. `--self-test` needs no Unreal.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

PREPARED = "/Game/LyraMigration/Rifle/Diagnostics/Explicit/RifleCore_Prepared_0247/"
EXTRAS = (
    "/ShooterCore/Input/Abilities/Struct_UIMessaging",
    "/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto",
    "/Game/B_LyraGameInstance",
    "/Game/Characters/Heroes/Mannequin/Animations/ABP_Mannequin_Base",
)
ROW_FIELDS = ("kind", "context", "owner", "name", "type", "flags", "index")
NATIVE_PATH = re.compile(r"/Script/[A-Za-z0-9_]+\.[A-Za-z0-9_]+")
MAX_TYPES = 128

# Review candidates, NOT Redirects or claims of full feature equivalence.
CANDIDATE_NAMES = {
    "AsyncAction_ObserveTeamColors": "DCLyraAsyncAction_ObserveTeamColors",
    "CircumferenceMarkerWidget": "DCLyraCircumferenceMarkerWidget",
    "CircumferenceMarkerEntry": "DCLyraCircumferenceMarkerEntry",
    "HitMarkerConfirmationWidget": "DCLyraHitMarkerConfirmationWidget",
    "LyraAbilityMontageFailureMessage": "DCAbilityMontageFailureMessage",
    "LyraAbilitySet": "DCAbilitySet", "LyraAbilityCost": "DCAbilityCost",
    "LyraAbilityCost_ItemTagStack": "DCLyraAbilityCost_ItemTagStack",
    "LyraAnimInstance": "DCAnimInstance", "LyraCharacter": "DreamCatcherCharacter",
    "LyraEquipmentDefinition": "DCLyraEquipmentDefinition", "LyraEquipmentInstance": "DCLyraEquipmentInstance",
    "LyraEquipmentActorToSpawn": "DCLyraEquipmentActorToSpawn",
    "LyraEquipmentManagerComponent": "DCLyraEquipmentManagerComponent",
    "LyraGameInstance": "DCGameInstance", "LyraGameplayAbility": "DCGameplayAbility",
    "LyraGameplayAbility_FromEquipment": "DCLyraGameplayAbility_FromEquipment",
    "LyraGameplayAbility_RangedWeapon": "DCLyraGameplayAbility_RangedWeapon", "LyraGameState": "DCLyraGameState",
    "LyraHealthComponent": "DCLyraHealthComponent", "LyraInventoryItemDefinition": "DCInventoryItemDefinition",
    "LyraInventoryItemInstance": "DCInventoryItemInstance", "LyraInventoryItemFragment": "DCInventoryItemFragment",
    "LyraInventoryFunctionLibrary": "DCInventoryFunctionLibrary",
    "LyraPawnComponent_CharacterParts": "DCLyraPawnComponent_CharacterParts",
    "LyraPlayerController": "DreamCatcherPlayerController", "LyraPlayerState": "DCPlayerState",
    "LyraQuickBarComponent": "DCLyraQuickBarComponent", "LyraRangedWeaponInstance": "DCLyraRangedWeaponInstance",
    "LyraReticleWidgetBase": "DCLyraReticleWidgetBase", "LyraSystemStatics": "DCSystemStatics",
    "LyraTeamDisplayAsset": "DCTeamDisplayAsset", "LyraTeamAgentInterface": "DCTeamAgentInterface",
    "LyraVerbMessage": "DCVerbMessage", "LyraWeaponInstance": "DCLyraWeaponInstance",
    "LyraWeaponStateComponent": "DCLyraWeaponStateComponent", "PhysicalMaterialWithTags": "DCPhysicalMaterialWithTags",
    "GameplayTagStack": "GameplayTagStack", "GameplayTagStackContainer": "GameplayTagStackContainer",
}
for _name in ("EquippableItem", "SetStats", "ReticleConfig", "QuickBarIcon", "PickupIcon"):
    CANDIDATE_NAMES["InventoryFragment_" + _name] = "DCInventoryFragment_" + _name
for _name in ("GameplayAbility", "GameplayEffect", "AttributeSet", "GrantedHandles"):
    CANDIDATE_NAMES["LyraAbilitySet_" + _name] = "DCAbilitySet_" + _name
for _name in ("AnimLayerSelectionEntry", "AnimLayerSelectionSet", "AnimBodyStyleSelectionEntry", "AnimBodyStyleSelectionSet",
              "CharacterPart", "CharacterPartHandle", "CharacterPartList"):
    CANDIDATE_NAMES["Lyra" + _name] = "DCLyra" + _name
# Exact source/target declarations reviewed after NativeAudit_20261007_0150.
# This list is for read-only comparison only; it neither installs Redirects nor proves equivalence.
REVIEWED_20261007_CANDIDATES = {
    "LyraAbilitySystemComponent": "DCAbilitySystemComponent",
    "LyraCameraComponent": "DCCameraComponent",
    "LyraCameraModeStack": "DCCameraModeStack",
    "LyraEquipmentList": "DCLyraEquipmentList",
    "LyraAppliedEquipmentEntry": "DCLyraAppliedEquipmentEntry",
    "LyraExperienceManagerComponent": "DCLyraExperienceManagerComponent",
    "LyraExperienceDefinition": "DCLyraExperienceDefinition",
    "LyraExperienceActionSet": "DCLyraExperienceActionSet",
    "ELyraDeathState": "EDCLyraDeathState",
    "LyraAttributeSet": "DCAttributeSet",
    "LyraHeroComponent": "DCHeroComponent",
    "InputMappingContextAndPriority": "InputMappingContextAndPriority",
    "ECharacterCustomizationCollisionMode": "EDCLyraCharacterCustomizationCollisionMode",
    "LyraAppliedCharacterPartEntry": "DCLyraAppliedCharacterPartEntry",
    "LyraPawnData": "DCPawnData",
    "LyraInputConfig": "DCInputConfig",
    "LyraInputAction": "DCInputAction",
    "LyraPawnExtensionComponent": "DCPawnExtensionComponent",
    "LyraCameraAssistInterface": "DCCameraAssistInterface",
    "LyraAbilitySourceInterface": "DCAbilitySourceInterface",
}
CANDIDATE_NAMES.update(REVIEWED_20261007_CANDIDATES)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def emit(event, **values):
    print("DC_RIFLE_NATIVE_CONTRACT " + json.dumps({"event": event, **values}, ensure_ascii=False))


def option(command, name):
    match = re.search(r"(?:^|\s)-" + re.escape(name) + r"=(\S+)", command)
    return match.group(1) if match else ""


def selected_packages(originals):
    require(len(originals) == len(set(originals)) == 18, "Expected exact approved closure18")
    leaves = [item.rsplit("/", 1)[-1] for item in originals]
    require(len({leaf.lower() for leaf in leaves}) == 18, "Duplicate prepared asset leaf")
    result = [PREPARED + leaf for leaf in leaves] + list(EXTRAS)
    require(len(set(result)) == 22, "Expected 22 source assets")
    return result


def snapshots(project, packages):
    result = {}
    for package in packages:
        if package.startswith("/Game/"):
            root, relative = project / "Content", package[6:]
        elif package.startswith("/ShooterCore/"):
            root, relative = project / "Plugins/GameFeatures/ShooterCore/Content", package[13:]
        else:
            raise RuntimeError("Unapproved asset mount: " + package)
        for extension in (".uasset", ".uexp", ".ubulk", ".uptnl"):
            path = (root / (relative + extension)).resolve()
            require(path.is_relative_to(root.resolve()), "Asset path escaped mount")
            require(extension != ".uasset" or path.is_file(), "Missing asset: " + str(path))
            result[str(path)] = None
            if path.is_file():
                with path.open("rb") as stream:
                    result[str(path)] = hashlib.file_digest(stream, "sha256").hexdigest()
    return result


def pack(report):
    return {"succeeded": report.get_editor_property("succeeded"),
            "subject": report.get_editor_property("subject_path"),
            "rows": [{key: row.get_editor_property(key) for key in ROW_FIELDS}
                     for row in report.get_editor_property("rows")],
            "references": list(report.get_editor_property("referenced_types")),
            "messages": list(report.get_editor_property("messages"))}


def mappings(config):
    candidates = {"/Script/LyraGame." + old: "/Script/DreamCatcher." + new for old, new in CANDIDATE_NAMES.items()}
    registered = {}
    functions = {}
    # Read only matching Redirect records; never print other configuration (which can contain credentials).
    for line in config.read_text(encoding="utf-8-sig").splitlines():
        match = re.match(r'^\+(Class|Struct|Enum|Function)Redirects=\(OldName="([^"]+)",NewName="([^"]+)"\)', line)
        if not match:
            continue
        kind, old, new = match.groups()
        if kind == "Function":
            functions[old] = new
        else:
            registered[old] = new
            candidates[old] = new
    return candidates, registered, functions


def normalize_row(row, candidates, function_redirects):
    result = dict(row)
    original_owner = result["owner"]
    context = result["context"]
    if result["kind"] in ("function", "parameter"):
        redirected = function_redirects.get(original_owner + "." + context)
        if redirected:
            function_name = redirected.rsplit(".", 1)[-1]
            result["context"] = function_name
            if result["kind"] == "function":
                result["name"] = function_name
    for key in ("owner", "type"):
        result[key] = NATIVE_PATH.sub(lambda m: candidates.get(m.group(), m.group()), result[key])
    if result["kind"] == "class" and original_owner in candidates:
        result["name"] = candidates[original_owner].rsplit(".", 1)[-1]
    if result["kind"] == "enum_value" and original_owner in candidates:
        old_name = original_owner.rsplit(".", 1)[-1]
        if result["name"] == old_name + "_MAX":
            result["name"] = candidates[original_owner].rsplit(".", 1)[-1] + "_MAX"
    return result


def compare(source, target, candidates, function_redirects):
    comparisons = []
    for original, report in sorted(source["types"].items()):
        mapped = candidates.get(original)
        destination = target["types"].get(mapped)
        item = {"source": original, "candidate": mapped, "status": "unverified"}
        if not mapped:
            item["reason"] = "No reviewed mapping candidate; do not guess a class name"
        elif not destination:
            item["reason"] = "Candidate not found/inspected in target"
        elif not report["succeeded"] or not destination["succeeded"]:
            item["reason"] = "At least one bounded report is incomplete"
        else:
            left = {json.dumps(normalize_row(row, candidates, function_redirects), sort_keys=True) for row in report["rows"]}
            right = {json.dumps(row, sort_keys=True) for row in destination["rows"]}
            item.update(status="reflected_shape_match" if left == right else "reflected_shape_difference",
                        source_only=[json.loads(row) for row in sorted(left - right)],
                        target_only=[json.loads(row) for row in sorted(right - left)])
        comparisons.append(item)
    return {"comparisons": comparisons, "behavior_equivalence_verified": False,
            "note": "Type candidate renaming and EXISTING function redirects only. Differences include metadata/API flags; they are not automatically runtime incompatibility. No Config was changed.",
            "function_rename_candidates_not_applied": {
                "/Script/LyraGame.LyraPlayerState.GetLyraPlayerController": "/Script/DreamCatcher.DCPlayerState.GetDCPlayerController",
                "/Script/LyraGame.LyraPlayerState.GetLyraAbilitySystemComponent": "/Script/DreamCatcher.DCPlayerState.GetDCAbilitySystemComponent"}}


def main():
    import unreal
    command = unreal.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower(), "Separate Python commandlet required")
    require("-dcriflenativecontractaudit" in command.lower(), "Explicit native audit opt-in required")
    require("-dclyraruntimediagnostics" not in command.lower(), "No runtime-subsystem activation in this audit")
    run = option(command, "DCRifleNativeAuditRun")
    require(re.fullmatch(r"[A-Za-z0-9_]{1,48}", run), "Explicit safe fresh report run required")
    target = Path(__file__).resolve().parents[2]
    original = target.parent / "LyraStarterGame"
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    require(project in (target.resolve(), original.resolve()), "Unexpected project")
    side = "source" if project == original.resolve() else "target"
    bridge = unreal.DCRifleMigrationLibrary
    require(bridge.validate_diagnostic_context() is not None, "Diagnostic context refused")
    require(bridge.get_engine_ensure_failure_count() == 0, "Existing ensure; use a fresh process")
    require(not unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages(), "Initial dirty content refused")
    require(hasattr(bridge, "inspect_asset_native_contract") and hasattr(bridge, "inspect_native_type_contract"), "User must rebuild the native contract Editor API first")
    folder = (target / "Saved/Diagnostics/R5NativeContracts" / run).resolve()
    require(folder.is_relative_to((target / "Saved/Diagnostics/R5NativeContracts").resolve()), "Report path escaped root")
    output = folder / (side + ".json")
    require(not output.exists(), "Report overwrite refused")
    if side == "target":
        require((folder / "source.json").is_file(), "Run the original project audit first")
        require(not (folder / "comparison.json").exists(), "Comparison overwrite refused")
    candidates, registered, function_redirects = mappings(target / "Config/DefaultEngine.ini")
    selected = selected_packages(list(bridge.get_approved_rifle_closure_packages())) if side == "source" else []
    # Protect selected packages, original core/siblings and known load-dirty files; loading can follow refs.
    watch = (selected + list(bridge.get_approved_rifle_closure_packages())
             + list(bridge.get_protected_rifle_sibling_packages()) + list(bridge.get_known_rifle_load_dirty_packages())) if side == "source" else []
    before = snapshots(project, sorted(set(watch)))
    report = {"version": 1, "side": side, "assets": {}, "types": {}, "unverified": [],
              "mapping_candidates": candidates, "registered_redirects": registered,
              "cpp_build": False, "asset_writes": 0, "compile_requested": False}
    queue = set()
    emit("begin", side=side, selected_assets=len(selected), run=run)
    try:
        if side == "source":
            for package in selected:
                asset = unreal.load_asset(package)
                require(asset is not None, "Source asset load failed: " + package)
                entry = pack(bridge.inspect_asset_native_contract(asset))
                report["assets"][package] = entry
                queue.update(path for path in entry["references"] if path.startswith(("/Script/LyraGame.", "/Script/ShooterCoreRuntime.")))
                if not entry["succeeded"]:
                    report["unverified"].append({"path": package, "messages": entry["messages"]})
                require(bridge.get_engine_ensure_failure_count() == 0, "Ensure while inspecting " + package)
        else:
            source = json.loads((folder / "source.json").read_text(encoding="utf-8"))
            require(source["version"] == 1 and source["side"] == "source", "Unexpected source report")
            queue.update(candidates[path] for path in source["types"] if path in candidates)
            report["unverified"].extend({"path": path, "reason": "No candidate mapping"} for path in source["types"] if path not in candidates)
        seen = set()
        while queue:
            path = sorted(queue)[0]
            queue.remove(path)
            if path in seen:
                continue
            require(len(seen) < MAX_TYPES, "Native type bound reached; review scope before expanding")
            seen.add(path)
            # find_object is deliberately non-loading. Startup registers native classes/structs/enums.
            obj = unreal.find_object(None, path)
            if obj is None:
                report["unverified"].append({"path": path, "reason": "Not registered; no implicit module load or guessed replacement"})
                continue
            entry = pack(bridge.inspect_native_type_contract(obj))
            report["types"][path] = entry
            if not entry["succeeded"]:
                report["unverified"].append({"path": path, "messages": entry["messages"]})
            if side == "source":
                queue.update(ref for ref in entry["references"] if ref.startswith(("/Script/LyraGame.", "/Script/ShooterCoreRuntime.")) and ref not in seen)
            require(bridge.get_engine_ensure_failure_count() == 0, "Ensure while inspecting native type")
    finally:
        require(snapshots(project, sorted(set(watch))) == before, "Protected asset file/companion state changed")
        report["protected_file_states"] = before
        emit("file_states_unchanged", count=len(before))
    dirty = sorted(package.get_path_name() for package in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages())
    allowed = set(bridge.get_known_rifle_load_dirty_packages()) if side == "source" else set()
    require(set(dirty).issubset(allowed), "Unexpected dirty content; nothing is saved or marked clean: " + repr(dirty))
    report["dirty_packages"] = dirty
    report["external_type_references_not_expanded"] = sorted({ref for entry in list(report["assets"].values()) + list(report["types"].values())
                                                            for ref in entry["references"] if ref not in report["types"]})
    report["behavior_equivalence_verified"] = False
    require(bridge.get_engine_ensure_failure_count() == 0, "Ensure encountered")
    folder.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
    if side == "target":
        comparison = compare(source, report, candidates, function_redirects)
        with (folder / "comparison.json").open("x", encoding="utf-8") as stream:
            json.dump(comparison, stream, ensure_ascii=False, indent=2)
    emit("complete", side=side, reports_only=True, types=len(report["types"]), unverified=len(report["unverified"]), asset_writes=0, ensure_failures=0)


def self_test():
    # Pure contract checks. They do not validate the C++ implementation or any actual asset.
    import unittest
    class Contracts(unittest.TestCase):
        def test_selection_bound(self):
            self.assertEqual(len(selected_packages(["/Game/Fixture/A" + str(n) for n in range(18)])), 22)
            with self.assertRaises(RuntimeError):
                selected_packages(["/Game/Fixture/A"] * 18)

        def test_exact_type_renaming(self):
            row = dict(zip(ROW_FIELDS, ("property", "", "/Script/LyraGame.Foo", "Value", "object(/Script/LyraGame.FooBar)[1]", "0x0", -1)))
            changed = normalize_row(row, {"/Script/LyraGame.Foo": "/Script/DreamCatcher.Bar"}, {})
            self.assertEqual(changed["owner"], "/Script/DreamCatcher.Bar")
            self.assertEqual(changed["type"], row["type"])

        def test_incomplete_is_not_match(self):
            source = {"types": {"old": {"succeeded": False, "rows": []}}}
            target = {"types": {"new": {"succeeded": True, "rows": []}}}
            self.assertEqual(compare(source, target, {"old": "new"}, {})["comparisons"][0]["status"], "unverified")

        def test_existing_function_redirect(self):
            row = dict(zip(ROW_FIELDS, ("parameter", "OldFn", "/Script/LyraGame.Foo", "Value", "IntProperty[1]", "0x0", 0)))
            fixed = normalize_row(row, {}, {"/Script/LyraGame.Foo.OldFn": "/Script/DreamCatcher.Foo.NewFn"})
            self.assertEqual(fixed["context"], "NewFn")
            self.assertEqual(fixed["name"], "Value")

        def test_reviewed_candidate_boundary(self):
            expected = {
                "LyraAbilitySystemComponent", "LyraCameraComponent", "LyraCameraModeStack",
                "LyraEquipmentList", "LyraAppliedEquipmentEntry", "LyraExperienceManagerComponent",
                "LyraExperienceDefinition", "LyraExperienceActionSet", "ELyraDeathState", "LyraAttributeSet",
                "LyraHeroComponent", "InputMappingContextAndPriority", "ECharacterCustomizationCollisionMode",
                "LyraAppliedCharacterPartEntry", "LyraPawnData", "LyraInputConfig", "LyraInputAction",
                "LyraPawnExtensionComponent", "LyraCameraAssistInterface", "LyraAbilitySourceInterface",
            }
            self.assertEqual(set(REVIEWED_20261007_CANDIDATES), expected)
            self.assertEqual(len(expected), 20)
            for source, candidate in REVIEWED_20261007_CANDIDATES.items():
                self.assertEqual(CANDIDATE_NAMES[source], candidate)
            self.assertNotIn("ELyraCharacterCustomizationCollisionMode", CANDIDATE_NAMES)
            self.assertEqual(CANDIDATE_NAMES["ECharacterCustomizationCollisionMode"], "EDCLyraCharacterCustomizationCollisionMode")
            self.assertEqual(CANDIDATE_NAMES["ELyraDeathState"], "EDCLyraDeathState")

        def test_unreviewed_four_are_not_guessed(self):
            names = ("LyraHUD", "ELyraPlayerConnectionType",
                     "LyraReplicatedAcceleration", "SharedRepMovement")
            for name in names:
                self.assertNotIn(name, CANDIDATE_NAMES)
            candidates = {"/Script/LyraGame." + old: "/Script/DreamCatcher." + new for old, new in CANDIDATE_NAMES.items()}
            source = {"types": {"/Script/LyraGame." + name: {"succeeded": True, "rows": []} for name in names}}
            result = compare(source, {"types": {}}, candidates, {})
            self.assertEqual(len(result["comparisons"]), 4)
            self.assertTrue(all(item["status"] == "unverified" for item in result["comparisons"]))

        def test_hit_marker_candidate_requires_runtime_registration(self):
            original = "/Script/LyraGame.HitMarkerConfirmationWidget"
            candidate = "/Script/DreamCatcher.DCLyraHitMarkerConfirmationWidget"
            self.assertEqual(CANDIDATE_NAMES["HitMarkerConfirmationWidget"], "DCLyraHitMarkerConfirmationWidget")
            source = {"types": {original: {"succeeded": True, "rows": []}}}
            result = compare(source, {"types": {}}, {original: candidate}, {})
            self.assertEqual(result["comparisons"][0]["status"], "unverified")
            self.assertFalse(result["behavior_equivalence_verified"])

        def test_raw_flags_and_delegate_names_are_retained(self):
            source_path, target_path = "/Script/LyraGame.Foo", "/Script/DreamCatcher.Bar"
            row = dict(zip(ROW_FIELDS, ("property", "", source_path, "Event", "multicast(/Script/LyraGame.OldDelegate__DelegateSignature)", "0x800", -1)))
            candidates = {source_path: target_path}
            normalized = normalize_row(row, candidates, {})
            self.assertEqual(normalized["flags"], "0x800")
            self.assertEqual(normalized["type"], row["type"])
            for field, value in (("flags", "0x0"), ("type", "multicast(/Script/DreamCatcher.NewDelegate__DelegateSignature)")):
                changed = {**normalized, field: value}
                source = {"types": {source_path: {"succeeded": True, "rows": [row]}}}
                target = {"types": {target_path: {"succeeded": True, "rows": [changed]}}}
                self.assertEqual(compare(source, target, candidates, {})["comparisons"][0]["status"], "reflected_shape_difference")

        def test_shape_match_is_not_behavior_equivalence(self):
            original, candidate = "/Script/LyraGame.LyraEquipmentList", "/Script/DreamCatcher.DCLyraEquipmentList"
            row = dict(zip(ROW_FIELDS, ("property", "", original, "Value", "IntProperty[1]", "0x0", -1)))
            target_row = {**row, "owner": candidate}
            source = {"types": {original: {"succeeded": True, "rows": [row]}}}
            target = {"types": {candidate: {"succeeded": True, "rows": [target_row]}}}
            result = compare(source, target, {original: candidate}, {})
            self.assertEqual(result["comparisons"][0]["status"], "reflected_shape_match")
            self.assertFalse(result["behavior_equivalence_verified"])
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Contracts))
    require(result.wasSuccessful(), "Pure contract tests failed")


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        self_test()
    else:
        main()
