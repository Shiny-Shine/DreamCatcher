"""Real integration PIE: Action -> copied Pawn graph -> original GA -> QuickBar.

Adds/removes a second real rifle item only in PIE. Click checks invoke the
original widget handler, not OS mouse focus. No asset/config saves.
"""
import json
from pathlib import Path
import re
import traceback
import unreal as ue

PROJECT = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
run = re.search(r"-DCR7SlotPieRun=([A-Za-z0-9_]{1,36})(?:\s|$)", ue.SystemLibrary.get_command_line())
assert run, "Explicit QuickSlot PIE run name required"
output = PROJECT / "Saved/Diagnostics/R7QuickBar" / ("slot_pie_" + run.group(1) + ".json")
assert not output.exists()
root_scope = {"__name__": "r7_slot_existing_root_probe"}
exec(compile((PROJECT / "Scripts/Editor/r7_root_ui_probe.py").read_text(encoding="utf-8-sig"), "r7_root_ui_probe.py", "exec"), root_scope)
scope = root_scope["scope"]
state, base_tick, base_finish = scope["state"], scope["tick"], scope["finish"]
ue.unregister_slate_post_tick_callback(state["callback"])
ROOT = "/Game/LyraMigration/UI/R7/Equipment"
actions = {suffix: ue.load_asset(ROOT + "/Input/Actions/IA_QuickSlot" + suffix)
           for suffix in ("1", "2", "3", "_CycleForward", "_CycleBackward")}
assert all(actions.values())
widget_class = ue.load_class(None, ROOT + "/W_QuickBar.W_QuickBar_C")
slot_class = ue.load_class(None, ROOT + "/W_QuickBarSlot.W_QuickBarSlot_C")
widget_library = ue.get_default_object(ue.load_class(None, "/Script/UMG.WidgetBlueprintLibrary"))
quick = dict(last_pawn=None, running=False, widget=None, extra=None, checks=[], rounds=0, inputs={})
steps = [("select_second", "2", 1), ("reject_empty", "3", 1),
         ("cycle_forward", "_CycleForward", 0), ("cycle_backward", "_CycleBackward", 1),
         ("original_widget_click_first", None, 0), ("select_second_again", "2", 1),
         ("select_first", "1", 0)]


def inject(action, pressed):
    quick["inputs"][action] = ue.Vector(1 if pressed else 0, 0, 0)
    state["subsystem"].inject_input_vector_for_action(action, quick["inputs"][action], [], [])


def begin(now):
    pc, pawn = state["pc"], state["pawn"]
    inventory = pc.get_component_by_class(scope["cls"]("DCInventoryManagerComponent"))
    item = inventory.add_item_definition(pc.get_editor_property("StartingRifle"), 1)
    assert item and item != state["item"], "Second rifle was not a distinct real item"
    assert state["qb"].get_slots()[1] is None
    state["qb"].add_item_to_slot(1, item)
    widget = widget_library.call_method("Create", args=(state["world"], widget_class, pc))
    assert widget and widget.add_to_player_screen(20)
    widget.set_desired_size_in_viewport(ue.Vector2D(620, 170))
    widget.set_position_in_viewport(ue.Vector2D(40, 40), False)
    quick.update(last_pawn=pawn, running=True, extra=item, inventory=inventory, widget=widget,
                 step=0, pressed=False, started=now, since=now, rounds=quick["rounds"] + 1)


def slot_tick(now):
    assert now - quick["started"] < 8, "QuickSlot round timeout"
    name, suffix, expected = steps[quick["step"]]
    elapsed = now - quick["since"]
    if not quick["pressed"] and elapsed > 0.15:
        if suffix is None:
            # Do not overwrite the widget's one-frame injected action with a
            # retained zero from a previous round.
            quick["inputs"].pop(actions["1"], None)
            widgets = [w for w in ue.ObjectIterator(ue.UserWidget)
                       if w.get_class() == slot_class and w.get_owning_player() == state["pc"]
                       and w.get_editor_property("SlotIndex") == 0 and w.get_typed_outer(ue.UserWidget) == quick["widget"]]
            assert len(widgets) == 1, "Original slot widget missing/ambiguous"
            widgets[0].call_method("On_SelectionBorder_MouseButtonDown_0", args=(widgets[0].get_cached_geometry(), ue.PointerEvent()))
        else:
            if suffix.startswith("_Cycle"):
                # A wheel detent is one frame, not a held key. The original
                # cycle Actions have no Pressed trigger on their wheel rows.
                quick["inputs"].pop(actions[suffix], None)
                state["subsystem"].inject_input_vector_for_action(actions[suffix], ue.Vector(1, 0, 0), [], [])
            else:
                inject(actions[suffix], True)
        quick["pressed"] = True
    if quick["pressed"] and elapsed > 0.28 and suffix:
        inject(actions[suffix], False)
    if quick["pressed"] and elapsed > 0.5:
        index = state["qb"].get_active_slot_index()
        assert index == expected, name + ": expected " + str(expected) + " got " + str(index)
        scope["bind_pawn"](state["pawn"])
        scope["inspect_ui"]("quickslot_" + name)
        record = dict(round=quick["rounds"], pawn=state["pawn"].get_path_name(), step=name,
                      active_slot=index, real_item=state["item"].get_path_name(), via_action_event_ga=True)
        quick["checks"].append(record)
        print("DC_R7_SLOT_PIE " + json.dumps(record))
        quick.update(step=quick["step"] + 1, since=now, pressed=False)
        if quick["step"] == len(steps):
            assert state["qb"].get_active_slot_index() == 0
            state["qb"].remove_item_from_slot(1)
            quick["inventory"].remove_item_instance(quick["extra"])
            quick["widget"].remove_from_parent()
            quick.update(running=False, widget=None, extra=None)
            state["since"] = now
            if state["phase"] == "settle": state["started"] = now


def finish(reason):
    if "subsystem" in state:
        for action in actions.values(): inject(action, False)
    if quick["widget"]: quick["widget"].remove_from_parent()
    passed = reason == "host_data_and_lifecycle_pass" and quick["rounds"] == 2 and len(quick["checks"]) == 14
    result = dict(complete=passed, reason=reason, rounds=quick["rounds"], checks=quick["checks"],
                  asset_saves=0, physical_keys_mouse_focus=False, gamepad_device_tested=False, replication_tested=False)
    output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    if not passed: ue.log_error("QuickSlot PIE incomplete: " + reason)
    print("DC_R7_SLOT_PIE_FINISH " + json.dumps(dict(complete=passed, checks=len(quick["checks"]), asset_saves=0)))
    base_finish(reason)


def tick(delta):
    try:
        if state["phase"] == "waiting":
            base_tick(delta)
            return
        world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
        if not world or "UEDPIE_" not in world.get_path_name(): return
        now = ue.GameplayStatics.get_time_seconds(world)
        if state["phase"] in ("settle", "new_pawn") and quick["last_pawn"] != state["pawn"] and now - state["since"] > 0.12:
            begin(now)
        if quick["running"]:
            slot_tick(now)
            for action, value in quick["inputs"].items(): state["subsystem"].inject_input_vector_for_action(action, value, [], [])
            return
        base_tick(delta)
    except Exception:
        details = traceback.format_exc()
        ue.log_error("QuickSlot PIE failed: " + details)
        finish("failed")


scope["finish"] = finish
state["callback"] = ue.register_slate_post_tick_callback(tick)
print("DC_R7_SLOT_PIE armed=true asset_saves=0")
