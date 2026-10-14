"""Bounded, read-only source UI audit. No asset copy/save/Compile or runtime UI.

Original Lyra commandlet only: -run=pythonscript -DCRifleCopyDiagnostics
  -DCR5UISourceAudit -DCR5UIAuditRun=<fresh_name>
Use -abslog=<DreamCatcher>/Saved/Logs/R5UIAudit_<fresh_name>.log.
The Editor bridge supplies guards and a fixed-eight WidgetBlueprint read adapter.
--self-test runs without Unreal. Never import r5_rifle_source_audit (it runs main).
"""
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True

SEEDS = (
    "/Game/UI/B_LyraUIPolicy",
    "/Game/UI/W_OverallUILayout",
    "/Game/UI/Hud/W_DefaultHUDLayout",
    "/Game/UI/Foundation/Dialogs/W_ControllerDisconnected",
    "/ShooterCore/Experiences/LAS_ShooterGame_StandardHUD",
    "/ShooterCore/UserInterface/W_ShooterHUDLayout",
    "/ShooterCore/UserInterface/HUD/W_WeaponReticleHost",
    "/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName",
    "/ShooterCore/UserInterface/HUD/W_QuickBar",
    "/ShooterCore/UserInterface/HUD/W_QuickBarSlot",
)
WIDGET_SEEDS = tuple(path for path in SEEDS if path.rsplit("/", 1)[-1].startswith("W_"))
DISCONNECT_PACKAGE = "/Game/UI/Foundation/Dialogs/W_ControllerDisconnected"
NATIVE_CDO_FIELDS = ("HBox_SwitchUser", "Button_ChangeUser")
MAX_PACKAGES = 4096
MAX_BYTES = 64 * 1024 * 1024
MAX_ITEMS = 512
MAX_STRING = 32768
LIMITS = {"graphs": 128, "nodes": 4096, "pins": 32768, "links": 50000,
          "widgets": 4096, "iterator_objects": 250000}
FIELDS = {
    "B_LyraUIPolicy": ("LayoutClass",),
    "W_DefaultHUDLayout": ("InputConfig", "GameMouseCaptureMode", "EscapeMenuClass",
                           "ControllerDisconnectedScreen", "PlatformRequiresControllerDisconnectScreen"),
    "W_ShooterHUDLayout": ("InputConfig", "GameMouseCaptureMode", "EscapeMenuClass",
                           "ControllerDisconnectedScreen", "PlatformRequiresControllerDisconnectScreen"),
    "W_ControllerDisconnected": ("PlatformSupportsUserChangeTags", "HBox_SwitchUser", "Button_ChangeUser"),
    "LAS_ShooterGame_StandardHUD": ("Actions", "GameFeaturesToEnable"),
    "W_WeaponAmmoAndName": ("HiddenByTags", "ShownVisibility", "HiddenVisibility"),
    "W_QuickBar": ("HiddenByTags", "ShownVisibility", "HiddenVisibility"),
    "W_QuickBarSlot": ("HiddenByTags", "ShownVisibility", "HiddenVisibility"),
}


class AuditStop(RuntimeError):
    pass


def require(condition, reason):
    if not condition:
        raise AuditStop(reason)


def emit(event, **values):
    print("DC_UI_SOURCE_AUDIT " + json.dumps({"event": event, **values}, ensure_ascii=False))


def option(command, name):
    found = re.search(r"(?:^|\s)-" + re.escape(name) + r"=(\S+)", command)
    return found.group(1) if found else ""


def has_flag(command, name):
    return re.search(r"(?:^|\s)-" + re.escape(name) + r"(?:\s|$)", command, re.I) is not None


def fresh_folder(target, run):
    require(re.fullmatch(r"[A-Za-z0-9_]{1,48}", run) is not None, "A fresh ASCII run name is required")
    root = (target / "Saved/Diagnostics/R5UIFoundation").resolve()
    folder = (root / run).resolve()
    require(folder.is_relative_to(root) and folder != root, "Report path escaped its root")
    require(not folder.exists(), "Report directory reuse/overwrite refused")
    return folder


def package_base(project, package):
    require(re.fullmatch(r"/(Game|ShooterCore)/[A-Za-z0-9_+/-]+", package) is not None,
            "Unsupported package name for file protection: " + package)
    require(all(part for part in package[1:].split("/")), "Empty package path component")
    if package.startswith("/Game/"):
        root, relative = project / "Content", package[6:]
    else:
        root, relative = project / "Plugins/GameFeatures/ShooterCore/Content", package[13:]
    result = (root / relative).resolve()
    require(result.is_relative_to(root.resolve()), "Package escaped its mount")
    return result


def digest(path):
    if not path.is_file():
        return None
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def file_states(project, packages):
    states, missing = {}, []
    for package in sorted(packages):
        base = package_base(project, package)
        if not base.with_suffix(".uasset").is_file() and not base.with_suffix(".umap").is_file():
            missing.append(package)
        for suffix in (".uasset", ".umap", ".uexp", ".ubulk", ".uptnl"):
            path = base.with_suffix(suffix)
            states[str(path)] = digest(path)
    return states, missing


def check_states(before):
    return [path for path, value in before.items() if digest(Path(path)) != value]


def dependency_closure(seeds, query, maximum=MAX_PACKAGES):
    pending, visited, external = deque(seeds), {}, set()
    while pending:
        package = pending.popleft()
        if package in visited or package in external:
            continue
        if package.startswith(("/Engine/", "/Script/")):
            external.add(package)
            continue
        require(len(visited) < maximum, "Dependency metadata limit exceeded; no asset load allowed")
        references = sorted(set(query(package)))
        require(len(references) <= maximum, "Single-package dependency bound exceeded")
        visited[package] = references
        pending.extend(references)
    return visited, sorted(external)


class Budget:
    def __init__(self):
        self.counts = {key: 0 for key in LIMITS}

    def take(self, key, count=1):
        self.counts[key] += count
        require(self.counts[key] <= LIMITS[key], "Inspection bound exceeded: " + key)


def bounded_text(value):
    text = str(value)
    require(len(text) <= MAX_STRING, "String/export text bound exceeded")
    return text


def encode(value, ue=None, depth=0):
    require(depth <= 8, "Value nesting bound exceeded")
    if value is None or isinstance(value, (bool, int, float)):
        return value
    if isinstance(value, str):
        return bounded_text(value)
    if ue is not None and isinstance(value, ue.Object):
        return bounded_text(value.get_path_name())
    arrays = (list, tuple) if ue is None else (list, tuple, ue.Array)
    maps = (dict,) if ue is None else (dict, ue.Map)
    if isinstance(value, arrays):
        require(len(value) <= MAX_ITEMS, "Array bound exceeded")
        return [encode(item, ue, depth + 1) for item in value]
    if isinstance(value, maps):
        require(len(value) <= MAX_ITEMS, "Map bound exceeded")
        return {bounded_text(key): encode(item, ue, depth + 1) for key, item in value.items()}
    export = getattr(value, "export_text", None)
    return bounded_text(export() if callable(export) else value)


def problem(report, subject, part, error):
    report["unverified"].append({"subject": subject, "part": part, "reason": bounded_text(error),
                                  "next_check": "Read this field/graph in the source Editor or review an explicitly scoped API addition; do not infer equivalence."})


def fields(obj, names, report, ue):
    require(len(names) <= MAX_ITEMS, "Field enumeration bound exceeded")
    result = {}
    for name in sorted(set(names)):
        try:
            result[name] = encode(obj.get_editor_property(name), ue)
        except AuditStop:
            raise
        except Exception as error:
            problem(report, obj.get_path_name(), "property:" + name, error)
    return result


def selected_settings(path):
    # Never dump whole INIs; Android/online configuration can contain credentials.
    result, section = [], ""
    for raw in path.read_text(encoding="utf-8-sig").splitlines():
        line = raw.strip()
        if line.startswith("[") and line.endswith("]"):
            section = line
        if (section in ("[/Script/LyraGame.LyraUIManagerSubsystem]", "[/Script/DreamCatcher.DCUIManagerSubsystem]")
                and line.startswith("DefaultUIPolicyClass=")):
            result.append({"section": section, "line": bounded_text(line)})
        if section == "[/Script/CommonUI.CommonUIInputSettings]" and line.startswith("+InputActions=") and "ActionTag=UI.Action.Escape" in line:
            result.append({"section": section, "line": bounded_text(line)})
    return result


def log_errors(path):
    require(path.is_file(), "Use the expected per-run -abslog path")
    require(path.stat().st_size <= 64 * 1024 * 1024, "Log size guard exceeded")
    lines = path.read_text(encoding="utf-8-sig", errors="replace").splitlines()
    return [line[:2048] for line in lines if re.search(r": Error:|Fatal:|Ensure condition failed", line)][:64]


def asset_record(ue, asset, package, report):
    result = {"path": asset.get_path_name(), "class": asset.get_class().get_path_name(),
              "dependencies": report["dependencies"][package], "values": {}, "graphs": [], "widget_trees": []}
    owner = asset
    names = list(FIELDS.get(asset.get_name(), ()))
    if isinstance(asset, ue.Blueprint):
        parent = ue.BlueprintEditorLibrary.get_blueprint_parent_class(asset)
        result["parent"] = encode(parent, ue)
        generated = ue.BlueprintEditorLibrary.generated_class(asset)
        require(generated is not None, "Missing generated class for " + package)
        result["generated_class"] = generated.get_path_name()
        owner = ue.get_default_object(generated)
        require(owner is not None, "Missing source CDO for " + package)
        try:
            members = ue.BlueprintEditorLibrary.list_member_variable_names(asset, True)
            require(len(members) <= MAX_ITEMS, "Blueprint variable count bound exceeded")
            names.extend(re.split(r"[:.]", str(name))[-1] for name in members if not str(name).startswith("/Script/"))
        except AuditStop:
            raise
        except Exception as error:
            problem(report, package, "member_variables", error)
    result["owner"] = owner.get_path_name()
    # These two protected fields are read only by the dedicated, type-limited C++
    # adapter. Never change property flags, and never interpret null as failed binding.
    if package == DISCONNECT_PACKAGE:
        names = [name for name in names if name not in NATIVE_CDO_FIELDS]
    result["values"] = fields(owner, names, report, ue)
    if package.endswith("LAS_ShooterGame_StandardHUD"):
        actions = owner.get_editor_property("Actions")
        require(len(actions) <= 32, "Action list bound exceeded")
        result["actions"] = []
        for action in actions:
            if action is None:
                result["actions"].append(None)
                continue
            require(action.get_path_name().startswith(package + "."), "Action outside selected package refused")
            item = {"path": action.get_path_name(), "class": action.get_class().get_path_name()}
            if action.get_class().get_name() == "GameFeatureAction_AddWidgets":
                item["values"] = fields(action, ("Layout", "Widgets"), report, ue)
                for array_name, members in (("Layout", ("LayoutClass", "LayerID")), ("Widgets", ("WidgetClass", "SlotID"))):
                    try:
                        entries = action.get_editor_property(array_name)
                        require(len(entries) <= MAX_ITEMS, "Action entry count bound exceeded")
                        item[array_name.lower()] = [{name: encode(entry.get_editor_property(name), ue) for name in members} for entry in entries]
                    except AuditStop:
                        raise
                    except Exception as error:
                        problem(report, item["path"], array_name, error)
            else:
                problem(report, item["path"], "action_body", "Action type not in this UI-only field schema")
            result["actions"].append(item)
    return result


def decode_ui_read(raw):
    """Decode only the bounded value-bearing UI schema, not the native type report."""
    def value(obj, name):
        return obj.get_editor_property(name)

    result = {}
    for name in ("succeeded", "inspection_started", "read_only_state_preserved"):
        item = value(raw, name)
        require(isinstance(item, bool), "Invalid UI report boolean: " + name)
        result[name] = item
    require(not result["succeeded"] or (result["inspection_started"] and result["read_only_state_preserved"]),
            "Inconsistent successful UI report state")
    for name in ("subject_path", "source_tree_path", "source_root_path", "generated_class_path",
                 "existing_cdo_path", "tree_owner_class_path"):
        result[name] = bounded_text(value(raw, name))
    schemas = {
        "generated_trees": (64, ("class_path", "tree_path", "root_path")),
        "widgets": (LIMITS["widgets"], ("path", "name", "class_path", "parent_path", "slot_path", "slot_class_path")),
        "fields": (2, ("name", "owner_path", "declared_class_path", "value_path", "value_class_path", "status")),
        "comments": (LIMITS["nodes"], ("path",)),
    }
    for key, (limit, columns) in schemas.items():
        entries = value(raw, key)
        require(len(entries) <= limit, "Native UI row bound exceeded: " + key)
        result[key] = []
        for entry in entries:
            row = {name: bounded_text(value(entry, name)) for name in columns}
            if key == "comments":
                count = value(entry, "pin_count")
                require(isinstance(count, int) and not isinstance(count, bool) and 0 <= count <= LIMITS["pins"],
                        "Invalid native comment pin count")
                row["pin_count"] = count
            result[key].append(row)
    messages = value(raw, "messages")
    require(len(messages) <= 64, "Native UI message bound exceeded")
    result["messages"] = [bounded_text(message) for message in messages]
    require(len(json.dumps(result, ensure_ascii=False).encode("utf-8")) <= MAX_BYTES,
            "Single native UI report size bound exceeded")
    return result


def observed_cdo_fields(native):
    """Absent/unreadable fields stay absent. An observed null is explicitly None."""
    result, seen = {}, set()
    for row in native["fields"]:
        require(row["name"] in NATIVE_CDO_FIELDS and row["name"] not in seen, "Unexpected/duplicate named CDO field")
        seen.add(row["name"])
        if row["status"] == "null":
            require(not row["value_path"] and not row["value_class_path"], "Inconsistent native null field")
            result[row["name"]] = None
        elif row["status"] == "object":
            require(bool(row["value_path"]) and bool(row["value_class_path"]), "Incomplete native object field")
            result[row["name"]] = row["value_path"]
    return result


def inspect_native_ui(ue, loaded, report):
    adapter = getattr(ue.DCRifleMigrationLibrary, "inspect_ui_blueprint_read_only", None)
    for package in WIDGET_SEEDS:
        asset = loaded.get(package)
        if asset is None or not callable(adapter):
            problem(report, package, "native_ui_read", "Approved UI reader/loaded asset unavailable; build the Editor tool and rerun in a fresh process")
            continue
        try:
            native = decode_ui_read(adapter(asset))
            report["assets"][package]["native_ui_read"] = native
            if native["inspection_started"] and not native["read_only_state_preserved"]:
                report["failures"].append("Native UI read-only/ensure guard failed: " + package)
            if not native["succeeded"]:
                problem(report, package, "native_ui_read", "; ".join(native["messages"]) or "Bounded query incomplete")
            if not native["inspection_started"]:
                continue
            require(native["subject_path"] == asset.get_path_name(), "Native UI subject identity mismatch")
            if package == DISCONNECT_PACKAGE and native["read_only_state_preserved"]:
                values = observed_cdo_fields(native)
                report["assets"][package]["values"].update(values)
                for name in NATIVE_CDO_FIELDS:
                    if name not in values:
                        problem(report, package, "property:" + name, "Named CDO field unavailable; see native_ui_read.fields; no runtime binding inferred")
        except AuditStop:
            raise
        except Exception as error:
            problem(report, package, "native_ui_read", error)


def inspect_graphs(ue, loaded, report, budget):
    graph_index = {}
    comments = {}
    for asset_report in report["assets"].values():
        native = asset_report.get("native_ui_read", {})
        if native.get("succeeded"):
            for row in native["comments"]:
                require(row["path"] not in comments, "Duplicate native comment row")
                comments[row["path"]] = row["pin_count"]
    for package, asset in loaded.items():
        if not isinstance(asset, ue.Blueprint):
            continue
        try:
            graphs = ue.BlueprintEditorLibrary.list_graphs(asset)
            for graph in graphs:
                budget.take("graphs")
                record = {"path": graph.get_path_name(), "name": graph.get_name(), "nodes": []}
                report["assets"][package]["graphs"].append(record)
                graph_index[graph.get_path_name()] = record
        except AuditStop:
            raise
        except Exception as error:
            problem(report, package, "graphs", error)
    # Enumerate existing loaded nodes, without accessing/editing protected Graph.Nodes.
    for node in ue.ObjectIterator(ue.EdGraphNode):
        budget.take("iterator_objects")
        outer = node.get_outer()
        graph = graph_index.get(outer.get_path_name()) if outer else None
        if graph is None:
            continue
        budget.take("nodes")
        entry = {"path": node.get_path_name(), "name": node.get_name(), "class": node.get_class().get_path_name(), "pins": []}
        graph["nodes"].append(entry)
        if "K2Node" not in node.get_class().get_name():
            if entry["path"] in comments:
                count = comments[entry["path"]]
                entry["native_comment_pin_count"] = count
                entry["pin_count_source"] = "bounded_cpp_comment_pins"
                if count:
                    # The adapter counts comment pins, not their topology/defaults.
                    problem(report, entry["path"], "pins", "Nonempty comment pins counted; topology/default values remain unverified")
            else:
                problem(report, entry["path"], "pins", "Non-K2 node pin API not inspected")
            continue
        try:
            entry["title"] = bounded_text(ue.BlueprintEditorLibrary.get_node_title(node))
            for pin in ue.BlueprintEditorLibrary.list_all_pins(node):
                budget.take("pins")
                links = []
                for other in pin.list_connected_pins():
                    budget.take("links")
                    links.append({"node": other.get_owning_node().get_path_name(), "pin": str(other.get_pin_name())})
                entry["pins"].append({"name": str(pin.get_pin_name()), "direction": str(pin.get_pin_direction()),
                                      "type": bounded_text(pin.get_pin_type_display_string()), "value": encode(pin.get_pin_value(), ue), "links": links})
        except AuditStop:
            raise
        except Exception as error:
            problem(report, entry["path"], "pins/title", error)


def inspect_widget_trees(ue, loaded, report, budget):
    trees = {}
    widget_blueprint_type = getattr(ue, "WidgetBlueprint", None)
    if widget_blueprint_type is None:
        problem(report, "selected_widget_blueprints", "WidgetTree", "WidgetBlueprint type is not exposed to Python")
        return
    blueprint_owners = {}
    for package, asset in loaded.items():
        if not isinstance(asset, widget_blueprint_type):
            continue
        blueprint_owners[asset.get_path_name()] = package
        native = report["assets"][package].get("native_ui_read", {})
        if native.get("succeeded") and native.get("source_tree_path"):
            record = {"path": native["source_tree_path"], "selection": "bounded_cpp_source_tree_pointer",
                      "direct_widget_tree_property_verified": True,
                      "values": {"RootWidget": native["source_root_path"] or None},
                      "widgets": []}
            trees[record["path"]] = record
            report["assets"][package]["widget_trees"].append(record)
            continue
        try:
            tree = asset.get_editor_property("WidgetTree")
            if tree is None:
                problem(report, package, "WidgetTree", "Source WidgetTree unavailable; inherited/generated tree not inferred")
                continue
            record = {"path": tree.get_path_name(), "selection": "direct_property", "values": fields(tree, ("RootWidget", "NamedSlotBindings"), report, ue), "widgets": []}
            trees[tree.get_path_name()] = record
            report["assets"][package]["widget_trees"].append(record)
        except AuditStop:
            raise
        except Exception as error:
            problem(report, package, "WidgetTree", error)
    # Public object iteration + exact outer identity, not flag changes or a protected
    # property bypass. Keep the inaccessible WidgetTree pointer marked unverified.
    # This separately records already-loaded Blueprint-owned template objects.
    tree_type = getattr(ue, "WidgetTree", None)
    if tree_type is not None:
        for tree in ue.ObjectIterator(tree_type):
            budget.take("iterator_objects")
            outer = tree.get_outer()
            package = blueprint_owners.get(outer.get_path_name()) if outer else None
            if package is None or tree.get_path_name() in trees:
                continue
            record = {"path": tree.get_path_name(), "selection": "public_object_iterator_exact_blueprint_outer",
                      "direct_widget_tree_property_verified": False, "widgets": []}
            trees[tree.get_path_name()] = record
            report["assets"][package]["widget_trees"].append(record)
    else:
        problem(report, "selected_widget_blueprints", "owned_tree_inventory", "WidgetTree class is not exposed for public iteration")
    for widget in ue.ObjectIterator(ue.Widget):
        budget.take("iterator_objects")
        outer = widget.get_outer()
        tree = trees.get(outer.get_path_name()) if outer else None
        if tree is None:
            continue
        budget.take("widgets")
        record = {"path": widget.get_path_name(), "name": widget.get_name(), "class": widget.get_class().get_path_name(),
                  "values": fields(widget, ("Slot", "Visibility"), report, ue)}
        tree["widgets"].append(record)
        try:
            # UWidget::GetParent is a public, non-virtual const getter of Slot->Parent.
            record["parent"] = encode(widget.get_parent(), ue)
        except AuditStop:
            raise
        except Exception as error:
            problem(report, widget.get_path_name(), "public_parent_getter", error)
        if widget.get_class().get_name() == "UIExtensionPointWidget":
            record["extension"] = fields(widget, ("ExtensionPointTag", "ExtensionPointTagMatch", "DataClasses"), report, ue)
    # Public iteration retains extension-point/visibility evidence. Cross-check its
    # inventory against the exact source-tree C++ query; never silently use two lists.
    for package in WIDGET_SEEDS:
        native = report["assets"].get(package, {}).get("native_ui_read", {})
        if not native.get("succeeded"):
            continue
        tree = trees.get(native["source_tree_path"])
        require(tree is not None, "Native source tree missing from Python inventory")
        expected = {row["path"]: row for row in native["widgets"]}
        observed = {row["path"]: row for row in tree["widgets"]}
        require(len(expected) == len(native["widgets"]), "Duplicate native template path")
        require(set(expected) == set(observed), "Native/public source-template inventory mismatch: " + package)
        for path, row in observed.items():
            other = expected[path]
            require(row["name"] == other["name"] and row["class"] == other["class_path"], "Template identity mismatch: " + path)
            if "parent" in row:
                require((row["parent"] or "") == other["parent_path"], "Template parent mismatch: " + path)
            if "Slot" in row["values"]:
                require((row["values"]["Slot"] or "") == other["slot_path"], "Template slot mismatch: " + path)
        tree["native_inventory_match"] = True
    report["widget_tree_scope"] = "Source tree/root pointers and template hierarchy are read through the bounded C++ adapter, cross-checked with public iteration. Generated/inherited class/tree/root paths are reported without PostLoad; their full templates are not inspected. CDO named values (including null) do not prove runtime BindWidget assignment. Named-slot contents, animations and visual/runtime equivalence are outside this bounded check."


def status_for(report):
    if report["failures"]:
        return "failed"
    if report["unverified"]:
        return "partial"
    return "bounded_read_complete"


def write_reports(folder, report):
    report["status"] = status_for(report)
    summary = {key: report[key] for key in ("status", "run", "failures", "counts", "asset_writes", "runtime_verified")}
    summary.update(inspected=len(report["assets"]), unverified_count=len(report["unverified"]),
                   protected_file_states=len(report.get("source_file_states", {})),
                   note="Read-only bounded evidence, not migration/visual/runtime equivalence. See source.json for omissions.")
    full = json.dumps(report, ensure_ascii=False, indent=2, allow_nan=False)
    compact = json.dumps(summary, ensure_ascii=False, indent=2, allow_nan=False)
    require(len(full.encode("utf-8")) + len(compact.encode("utf-8")) <= MAX_BYTES, "Report size bound exceeded")
    folder.mkdir(parents=True, exist_ok=False)
    # These are the only non-engine writes: new diagnostic text reports, never binary assets.
    for name, content in (("source.json", full), ("summary.json", compact)):
        with (folder / name).open("x", encoding="utf-8") as stream:
            stream.write(content)
    return summary


def main():
    import unreal as ue
    command = ue.SystemLibrary.get_command_line()
    require("-run=pythonscript" in command.lower() and has_flag(command, "DCR5UISourceAudit"), "Explicit source Python commandlet required")
    require(not has_flag(command, "DCLyraRuntimeDiagnostics"), "Runtime-subsystem diagnostic mode is outside this audit")
    target = Path(__file__).resolve().parents[2]
    original = target.parent / "LyraStarterGame"
    project = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
    require(project == original.resolve(), "Only the sibling original Lyra project is allowed")
    run = option(command, "DCR5UIAuditRun")
    folder = fresh_folder(target, run)
    logfile = target / "Saved/Logs" / ("R5UIAudit_" + run + ".log")
    bridge = ue.DCRifleMigrationLibrary
    guard = bridge.validate_diagnostic_context()
    require(bool(guard) and (not isinstance(guard, tuple) or guard[0] is True), "Native context/SCC/SaveOnCompile/ensure guard refused")
    require(not log_errors(logfile), "Existing engine log errors; use a fresh process after diagnosis")
    require(not ue.EditorLoadingAndSavingUtils.get_dirty_content_packages(), "Initial dirty content refused")
    budget = Budget()
    report = {"run": run, "version": 2, "selected": list(SEEDS), "native_ui_selected": list(WIDGET_SEEDS), "assets": {}, "unverified": [], "failures": [],
              "dependencies": {}, "counts": budget.counts, "asset_writes": 0, "compile_requested": False,
              "cpp_build": False, "packaging": False, "runtime_verified": False, "source_log": str(logfile)}
    before = {}
    protected_paths = [target / name for name in ("Config/DefaultEngine.ini", "Config/DefaultInput.ini", "Config/DefaultGame.ini",
                       "DreamCatcher.uproject", ".git/index", "Source/DreamCatcher/DreamCatcher.Build.cs")]
    protected_paths += [original / "Config" / name for name in ("DefaultEngine.ini", "DefaultInput.ini", "DefaultGame.ini")]
    protected_paths += [original / "LyraStarterGame.uproject"]
    protected = {str(path): digest(path) for path in protected_paths}
    try:
        require(len(SEEDS) == len(set(SEEDS)) == 10, "Exact ten-asset boundary required")
        require(len(WIDGET_SEEDS) == len(set(WIDGET_SEEDS)) == 8, "Exact eight-WidgetBlueprint native boundary required")
        registry = ue.AssetRegistryHelpers.get_asset_registry()
        registry.search_all_assets(True)
        options = ue.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                    include_searchable_names=False, include_soft_management_references=False, include_hard_management_references=False)
        emit("metadata_begin", selected=len(SEEDS), run=run)
        dependencies, external = dependency_closure(SEEDS, lambda path: [str(item) for item in registry.get_dependencies(path, options)])
        report["dependencies"], report["engine_script_external"] = dependencies, external
        watched = [path for path in dependencies if path.startswith(("/Game/", "/ShooterCore/"))]
        report["external_file_protection_excluded"] = sorted(set(dependencies) - set(watched)) + external
        emit("hash_begin", packages=len(watched))
        before, missing = file_states(original, watched)
        report["source_file_states"] = before
        for path in missing:
            problem(report, path, "dependency_file", "Registry reference has no source .uasset/.umap file")
        require(all(package_base(original, seed).with_suffix(".uasset").is_file() for seed in SEEDS), "A selected asset file is missing")
        report["same_relative_target_files"] = [{"source_package": path, "file_exists": package_base(target, path).with_suffix(".uasset").is_file(),
                         "not_a_compatibility_result": True} for path in watched]
        report["settings"] = {side: {name: selected_settings(root / "Config" / name) for name in ("DefaultGame.ini", "DefaultInput.ini")}
                              for side, root in (("source", original), ("target", target))}
        emit("metadata_complete", packages=len(dependencies), protected_files=len(before), missing_files=len(missing))
        loaded = {}
        for package in SEEDS:
            require(bridge.get_engine_ensure_failure_count() == 0 and not log_errors(logfile), "Engine diagnostic failure before loading " + package)
            asset = ue.load_asset(package)  # The only explicit asset load; SEEDS is fixed.
            require(asset is not None, "Selected asset did not load: " + package)
            require(bridge.get_engine_ensure_failure_count() == 0 and not log_errors(logfile), "Engine diagnostic failure loading " + package)
            try:
                report["assets"][package] = asset_record(ue, asset, package, report)
            except AuditStop:
                raise
            except Exception as error:
                problem(report, package, "asset_record", error)
                report["assets"][package] = {"path": asset.get_path_name(), "class": asset.get_class().get_path_name(), "graphs": [], "widget_trees": []}
            loaded[package] = asset
            emit("asset_read", package=package, unverified=len(report["unverified"]))
        inspect_native_ui(ue, loaded, report)
        inspect_graphs(ue, loaded, report, budget)
        inspect_widget_trees(ue, loaded, report, budget)
    except Exception as error:
        report["failures"].append(str(error))
    finally:
        changed = check_states(before)
        protected_changed = check_states(protected)
        report["protected_config_and_target_states"] = protected
        report["changed_source_files"] = changed
        report["changed_protected_files"] = protected_changed
        if changed or protected_changed:
            report["failures"].append("Protected source/config/target file state changed")
        dirty = sorted(package.get_path_name() for package in ue.EditorLoadingAndSavingUtils.get_dirty_content_packages())
        report["dirty_packages"] = dirty
        if dirty:
            report["failures"].append("Unexpected load-dirty packages; not saved or marked clean")
        report["ensure_failures"] = bridge.get_engine_ensure_failure_count()
        report["engine_errors"] = log_errors(logfile)
        if report["ensure_failures"] or report["engine_errors"]:
            report["failures"].append("Engine ensure/Error detected")
        emit("protection_checked", states=len(before), changed=len(changed), protected_changed=len(protected_changed), dirty=len(dirty))
    summary = write_reports(folder, report)
    emit("finish", **summary)
    require(report["status"] != "failed", "UI audit failed; inspect the new report without expanding scope or saving assets")


def self_test():
    import tempfile
    import unittest

    class Contracts(unittest.TestCase):
        def test_ten_seeds(self):
            self.assertEqual(len(set(SEEDS)), 10)
            self.assertTrue(all(path.startswith(("/Game/", "/ShooterCore/")) for path in SEEDS))
            self.assertEqual(len(set(WIDGET_SEEDS)), 8)
            self.assertTrue(set(WIDGET_SEEDS).issubset(SEEDS))
            self.assertIn(DISCONNECT_PACKAGE, WIDGET_SEEDS)

        def test_native_cdo_null_is_not_missing_or_binding_proof(self):
            rows = [{"name": NATIVE_CDO_FIELDS[0], "status": "null", "value_path": "", "value_class_path": ""},
                    {"name": NATIVE_CDO_FIELDS[1], "status": "missing_property", "value_path": "", "value_class_path": ""}]
            self.assertEqual(observed_cdo_fields({"fields": rows}), {NATIVE_CDO_FIELDS[0]: None})
            rows[0]["value_path"] = "/Unexpected"
            with self.assertRaises(AuditStop): observed_cdo_fields({"fields": rows})

        def test_native_ui_schema_and_bounds(self):
            class Reflected:
                def __init__(self, **values): self.values = values
                def get_editor_property(self, name): return self.values[name]
            raw = Reflected(succeeded=True, inspection_started=True, read_only_state_preserved=True,
                            subject_path="/Fixture", source_tree_path="/Tree", source_root_path="/Root",
                            generated_class_path="/Class", existing_cdo_path="/CDO", tree_owner_class_path="/Class",
                            generated_trees=[], widgets=[], fields=[], comments=[Reflected(path="/Comment", pin_count=0)], messages=[])
            self.assertEqual(decode_ui_read(raw)["comments"][0]["pin_count"], 0)
            raw.values["comments"][0].values["pin_count"] = -1
            with self.assertRaises(AuditStop): decode_ui_read(raw)
            raw.values["comments"] = []
            raw.values["fields"] = [Reflected()] * 3
            with self.assertRaises(AuditStop): decode_ui_read(raw)

        def test_flags_and_option(self):
            self.assertTrue(has_flag("-run=pythonscript -DCR5UISourceAudit", "DCR5UISourceAudit"))
            self.assertFalse(has_flag("-DCR5UISourceAuditFake", "DCR5UISourceAudit"))
            self.assertEqual(option("-DCR5UIAuditRun=Safe_1", "DCR5UIAuditRun"), "Safe_1")

        def test_report_path_guards(self):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                for name in ("", "../escape", "/absolute", "x" * 49, "a b"):
                    with self.assertRaises(AuditStop): fresh_folder(root, name)
                folder = fresh_folder(root, "Fresh")
                folder.mkdir(parents=True)
                with self.assertRaises(AuditStop): fresh_folder(root, "Fresh")

        def test_package_path_guards(self):
            root = Path.cwd()
            self.assertEqual(package_base(root, "/Game/UI/Test"), (root / "Content/UI/Test").resolve())
            self.assertEqual(package_base(root, "/Game/UI/TextStyle-Small+"), (root / "Content/UI/TextStyle-Small+").resolve())
            for path in ("/Game/../Other", "/Game//Bad", "/Other/UI/Test", "/Game/Test.Asset", "/Game/"):
                with self.assertRaises(AuditStop): package_base(root, path)

        def test_dependency_cycles_and_limit(self):
            graph = {"/Game/A": ["/Game/B", "/Script/Engine"], "/Game/B": ["/Game/A"]}
            found, external = dependency_closure(["/Game/A"], lambda p: graph[p], maximum=2)
            self.assertEqual(len(found), 2)
            self.assertEqual(external, ["/Script/Engine"])
            with self.assertRaises(AuditStop): dependency_closure(["/Game/A"], lambda p: graph[p], maximum=1)

        def test_inspection_budgets(self):
            for key, limit in LIMITS.items():
                budget = Budget()
                budget.take(key, limit)
                with self.assertRaises(AuditStop): budget.take(key)

        def test_serialization_bounds(self):
            self.assertEqual(encode({"value": [1, None, True]}), {"value": [1, None, True]})
            with self.assertRaises(AuditStop): encode("x" * (MAX_STRING + 1))
            with self.assertRaises(AuditStop): encode(list(range(MAX_ITEMS + 1)))
            value = 1
            for _ in range(10): value = [value]
            with self.assertRaises(AuditStop): encode(value)

        def test_partial_is_not_complete(self):
            report = {"failures": [], "unverified": []}
            self.assertEqual(status_for(report), "bounded_read_complete")
            problem(report, "test", "field", "not exposed")
            self.assertEqual(status_for(report), "partial")
            report["failures"].append("ensure")
            self.assertEqual(status_for(report), "failed")

        def test_config_selection_does_not_dump_other_settings(self):
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / "test.ini"
                path.write_text('[Unrelated]\nPrivateKey=do-not-report\n[/Script/LyraGame.LyraUIManagerSubsystem]\nDefaultUIPolicyClass=/Game/UI/Test.Test_C\n[/Script/CommonUI.CommonUIInputSettings]\n+InputActions=(ActionTag=UI.Action.Escape)\n+InputActions=(ActionTag=UI.Action.Other)\n', encoding="utf-8")
                result = selected_settings(path)
                self.assertEqual(len(result), 2)
                self.assertNotIn("do-not-report", json.dumps(result))

        def test_file_state_changes_and_report_no_overwrite(self):
            with tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                path = root / "owned_fixture.txt"
                before = {str(path): None}
                self.assertEqual(check_states(before), [])
                path.write_text("fixture", encoding="utf-8")
                self.assertEqual(check_states(before), [str(path)])
                report = {"run": "Fresh", "failures": [], "unverified": [], "assets": {}, "counts": {}, "asset_writes": 0, "runtime_verified": False}
                folder = fresh_folder(root, "Fresh")
                write_reports(folder, report)
                with self.assertRaises(FileExistsError): write_reports(folder, report)

    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Contracts))
    require(result.wasSuccessful(), "Pure guard tests failed")


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        self_test()
    else:
        main()
