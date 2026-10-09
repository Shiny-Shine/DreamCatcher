"""Pure, non-Unreal helpers for the approved core staging workflow."""
import re

ROOT = "/Game/LyraMigration/Rifle/Diagnostics/Explicit"
BASE = "/ShooterCore/Weapons/B_WeaponInstance_Base"
HERO_PACKAGE = "/ShooterCore/Game/B_Hero_ShooterMannequin"
HERO_OLD = HERO_PACKAGE + ".B_Hero_ShooterMannequin_C"
HERO_NEW = "/Script/LyraGame.LyraCharacter"
EXTENSIONS = (".uasset", ".uexp", ".ubulk", ".uptnl")
LIBRARIES = ("/Game/Audio/Blueprints/WeaponAudioMacros", "/ShooterCore/System/Audio/WeaponAudioFunctions")
HERO_RETURN_KEY = "EventGraph.K2Node_CallFunction_1|ReturnValue1|Type="


def require(value, message):
    if not value:
        raise RuntimeError(message)


def build_plan(sources, run):
    require(re.fullmatch(r"RifleCore_[A-Za-z0-9_]{1,38}", run), "Use a fresh RifleCore_<name> (maximum 48 characters)")
    require(len(sources) == 18 and len(set(sources)) == 18, "Exactly 18 distinct approved sources required")
    require(set(LIBRARIES).issubset(sources), "The two original audio libraries must be in the bundle")
    names = [source.rsplit("/", 1)[1] for source in sources]
    require(len({name.lower() for name in names}) == 18, "Destination leaf-name collision")
    return {source: ROOT + "/" + run + "/" + name for source, name in zip(sources, names)}


def translate(value, plan):
    if isinstance(value, str):
        for old in sorted(plan, key=len, reverse=True):
            value = value.replace(old + ".", plan[old] + ".")
        return value
    if isinstance(value, list):
        return [translate(item, plan) for item in value]
    if isinstance(value, dict):
        return {translate(key, plan): translate(item, plan) for key, item in value.items()}
    return value


def expected_dependencies(source, dependencies, plan, patched):
    result = {plan.get(str(package), str(package)) for package in dependencies}
    if patched and source == BASE:
        result.discard(HERO_PACKAGE)
        result.add("/Script/LyraGame")
    return result


def check_dirty_names(dirty, sources, destinations, allowed_dependencies):
    dirty = set(dirty)
    require(not dirty.intersection(sources), "An original core package is dirty")
    unexpected = dirty - set(destinations) - set(allowed_dependencies)
    require(not unexpected, "Unexpected dirty packages: " + repr(sorted(unexpected)))


def expected_graph(source, signature, plan, patched):
    result = translate(signature, plan)
    if source == BASE and patched:
        changed = 0
        for nodes in result.values():
            for node in nodes.values():
                if node.get("title") != "GetTypedPawn":
                    continue
                for pin in node.get("pins", {}).values():
                    if pin["name"] == "PawnType" and pin["value"] == HERO_OLD:
                        pin["value"] = HERO_NEW
                        changed += 1
        require(changed == 1, "Expected exactly one original Hero PawnType pin")
    return result


def expected_type_references(source, references, plan, patched, original_graph):
    result = translate(references, plan)
    if source == BASE and patched:
        # Verified in R5Closure18-memory.log and original GetTypedPawn metadata:
        # DeterminesOutputType=PawnType. This is NOT a global Hero type rewrite.
        node = original_graph.get("EventGraph", {}).get("K2Node_CallFunction_1", {})
        require(node.get("title") == "GetTypedPawn"
                and node.get("class") == "/Script/BlueprintGraph.K2Node_CallFunction",
                "The verified Hero GetTypedPawn node changed; review the source")
        inputs = [pin for pin in node.get("pins", {}).values() if pin.get("name") == "PawnType"]
        outputs = [pin for pin in node.get("pins", {}).values() if pin.get("name") == "ReturnValue"]
        require(len(inputs) == 1 and inputs[0].get("value") == HERO_OLD and len(outputs) == 1,
                "Expected the original Hero PawnType input and ReturnValue output")
        old = HERO_RETURN_KEY + HERO_OLD
        require(result.count(old) == 1, "Expected exactly one original Hero return subtype")
        result[result.index(old)] = HERO_RETURN_KEY + HERO_NEW
    return sorted(result)
