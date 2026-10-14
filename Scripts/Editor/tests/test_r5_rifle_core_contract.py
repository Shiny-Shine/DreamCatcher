"""Run with Python -B. No Unreal import or asset/file mutation."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from r5_rifle_core_contract import (
    BASE, HERO_NEW, HERO_OLD, HERO_PACKAGE, ROOT, LIBRARIES, HERO_RETURN_KEY,
    build_plan, check_dirty_names, expected_dependencies, expected_graph, expected_type_references, translate,
)


class CoreContractTests(unittest.TestCase):
    def hero_graph(self):
        return {"EventGraph": {"K2Node_CallFunction_1": {
            "class": "/Script/BlueprintGraph.K2Node_CallFunction", "title": "GetTypedPawn",
            "pins": {"in": {"name": "PawnType", "value": HERO_OLD}, "out": {"name": "ReturnValue"}}
        }}}

    def test_verified_hero_return_subtype_changes_once(self):
        unrelated = "OtherNode|ReturnValue1|Type=" + HERO_OLD
        refs = [HERO_RETURN_KEY + HERO_OLD, unrelated]
        expected = expected_type_references(BASE, refs, {}, True, self.hero_graph())
        self.assertEqual(expected, sorted([HERO_RETURN_KEY + HERO_NEW, unrelated]))
        self.assertEqual(refs, [HERO_RETURN_KEY + HERO_OLD, unrelated])
        self.assertNotEqual(expected, sorted([HERO_RETURN_KEY + HERO_NEW, unrelated.replace(HERO_OLD, HERO_NEW)]))

    def test_hero_return_exception_only_applies_after_base_patch(self):
        refs = [HERO_RETURN_KEY + HERO_OLD]
        self.assertEqual(expected_type_references(BASE, refs, {}, False, {}), refs)
        self.assertEqual(expected_type_references("/Game/Other", refs, {}, True, {}), refs)

    def test_missing_or_duplicate_hero_subtype_rejected(self):
        for refs in ([], [HERO_RETURN_KEY + HERO_OLD] * 2, [HERO_RETURN_KEY + HERO_NEW]):
            with self.assertRaises(RuntimeError):
                expected_type_references(BASE, refs, {}, True, self.hero_graph())

    def test_hero_node_identity_must_match_evidence(self):
        refs = [HERO_RETURN_KEY + HERO_OLD]
        for field in ("title", "class"):
            graph = self.hero_graph()
            graph["EventGraph"]["K2Node_CallFunction_1"][field] = "Unknown"
            with self.assertRaises(RuntimeError):
                expected_type_references(BASE, refs, {}, True, graph)
        graph = self.hero_graph()
        graph["EventGraph"]["K2Node_CallFunction_1"]["pins"]["in"]["value"] = HERO_NEW
        with self.assertRaises(RuntimeError):
            expected_type_references(BASE, refs, {}, True, graph)

    def test_plan_bounds(self):
        sources = ["/Game/Test/Asset" + str(index) for index in range(16)] + list(LIBRARIES)
        plan = build_plan(sources, "RifleCore_Test")
        self.assertEqual(len(plan), 18)
        self.assertTrue(all(path.startswith(ROOT + "/RifleCore_Test/") for path in plan.values()))
        for run in ("", "../Escape", "RifleCore_../Escape", "WeaponActor_Old", "RifleCore_" + "A" * 39):
            with self.assertRaises(RuntimeError):
                build_plan(sources, run)
        with self.assertRaises(RuntimeError):
            build_plan(sources[:-1], "RifleCore_Test")
        with self.assertRaises(RuntimeError):
            build_plan(["/Game/Other/asset1"] + sources[1:], "RifleCore_Test")
        with self.assertRaises(RuntimeError):
            build_plan(sources[:-1] + ["/Game/Unapproved"], "RifleCore_Test")

    def test_only_exact_package_references_translate(self):
        plan = {"/Game/A": "/Game/New/A"}
        original = {"values": ["/Game/A.A_C:Child", "/Game/AB.AB", None, 7]}
        self.assertEqual(translate(original, plan), {"values": ["/Game/New/A.A_C:Child", "/Game/AB.AB", None, 7]})
        self.assertEqual(original["values"][0], "/Game/A.A_C:Child")

    def test_macro_and_function_references_translate_together(self):
        plan = {source: "/Game/New/" + source.rsplit("/", 1)[1] for source in LIBRARIES}
        macro, function = LIBRARIES
        refs = ["Node|MacroGraph=" + macro + ".WeaponAudioMacros:LyraGetWeapon",
                "Call|FunctionOwner=" + function + ".WeaponAudioFunctions_C"]
        mapped = translate(refs, plan)
        self.assertIn("/Game/New/WeaponAudioMacros.WeaponAudioMacros:LyraGetWeapon", mapped[0])
        self.assertIn("/Game/New/WeaponAudioFunctions.WeaponAudioFunctions_C", mapped[1])
        self.assertEqual(translate([macro + "Extra.Asset"], plan), [macro + "Extra.Asset"])

    def test_hero_dependency_exception_is_narrow(self):
        plan = {BASE: "/Game/New/B_WeaponInstance_Base"}
        deps = {BASE, HERO_PACKAGE, "/Game/Keep"}
        expected = expected_dependencies(BASE, deps, plan, True)
        self.assertEqual(expected, {plan[BASE], "/Script/LyraGame", "/Game/Keep"})
        self.assertIn(HERO_PACKAGE, expected_dependencies(BASE, deps, plan, False))
        self.assertIn(HERO_PACKAGE, expected_dependencies("/Game/Other", deps, plan, True))

    def test_graph_change_is_one_pin_and_nonmutating(self):
        graph = {"EventGraph": {"Call": {"title": "GetTypedPawn", "pins": {
            "PawnType|Input": {"name": "PawnType", "value": HERO_OLD, "links": []},
            "Then|Output": {"name": "Then", "value": "", "links": ["Other|Exec|Input"]},
        }}}}
        expected = expected_graph(BASE, graph, {}, True)
        self.assertEqual(expected["EventGraph"]["Call"]["pins"]["PawnType|Input"]["value"], HERO_NEW)
        self.assertEqual(graph["EventGraph"]["Call"]["pins"]["PawnType|Input"]["value"], HERO_OLD)
        self.assertEqual(expected["EventGraph"]["Call"]["pins"]["Then|Output"], graph["EventGraph"]["Call"]["pins"]["Then|Output"])
        with self.assertRaises(RuntimeError):
            expected_graph(BASE, {}, {}, True)
        graph["OtherGraph"] = graph["EventGraph"]
        with self.assertRaises(RuntimeError):
            expected_graph(BASE, graph, {}, True)

    def test_dirty_guard(self):
        check_dirty_names({"/Game/Known", "/Game/New"}, {"/Game/Original"}, {"/Game/New"}, {"/Game/Known"})
        for dirty in ({"/Game/Original"}, {"/Game/Unexpected"}):
            with self.assertRaises(RuntimeError):
                check_dirty_names(dirty, {"/Game/Original"}, {"/Game/New"}, {"/Game/Known"})


if __name__ == "__main__":
    unittest.main()
