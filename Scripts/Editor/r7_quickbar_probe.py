"""Temporary QuickBar UI in the existing rifle PIE; no active HUD/asset edits.

Reuses the Root/Host lifecycle test and observes the original widgets' own Tick
and message bindings. Slot changes use the real component API, not fake messages;
this is NOT a physical QuickSlot key/click or final HUD-slot integration test.
"""
import json
from pathlib import Path
import unreal as ue

PROJECT = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
root_scope = {"__name__": "r7_existing_root_probe"}
exec(compile((PROJECT / "Scripts/Editor/r7_root_ui_probe.py").read_text(encoding="utf-8-sig"),
             "r7_root_ui_probe.py", "exec"), root_scope)
scope = root_scope["scope"]
old_stage, old_inspect, old_emit, old_finish = (scope[k] for k in ("stage", "inspect_ui", "emit", "finish"))
QUICK = "/Game/LyraMigration/UI/R7/Equipment/"
quick_class = ue.load_class(None, QUICK + "W_QuickBar.W_QuickBar_C")
slot_class = ue.load_class(None, QUICK + "W_QuickBarSlot.W_QuickBarSlot_C")
ammo_class = ue.load_class(None, QUICK + "W_WeaponAmmoAndName.W_WeaponAmmoAndName_C")
widgets = ue.get_default_object(ue.load_class(None, "/Script/UMG.WidgetBlueprintLibrary"))
quick_state = dict(widget=None, checks=[], captured=False)
spare_tag = scope["tag"]("Lyra.ShooterGame.Weapon.SpareAmmo")
source_receipt = json.loads((PROJECT / "Saved/Diagnostics/R7QuickBar/prepare_A.json").read_text(encoding="utf-8"))
source_ammo = source_receipt["source_snapshots"]["/ShooterCore/UserInterface/HUD/W_WeaponAmmoAndName"]
assert next(w for w in source_ammo["widgets"] if w["name"] == "WeaponNameWidget")["visibility"] == "<SlateVisibility.COLLAPSED: 1>"
name_setter = next(n for g in source_ammo["graphs"] if g["name"] == "EventGraph" for n in g["nodes"] if n["name"] == "K2Node_CallFunction_6")
assert next(p for p in name_setter["pins"] if p["name"] == "execute")["links"] == [], "Original name behavior changed; review expectation"


def stage(name, now):
    old_stage(name, now)
    if name == "settle":
        state = scope["state"]
        quick = widgets.call_method("Create", args=(state["world"], quick_class, state["pc"]))
        assert quick and quick.add_to_player_screen(20), "Temporary QuickBar creation failed"
        # Test-only placement to inspect the original panel without replacing the HUD.
        quick.set_desired_size_in_viewport(ue.Vector2D(620, 170))
        quick.set_position_in_viewport(ue.Vector2D(40, 850), False)
        quick_state["widget"] = quick


def inspect_quick(event, check_display=True):
    state = scope["state"]
    quick = quick_state["widget"]
    assert quick and quick.is_in_viewport()
    slots = sorted([w for w in ue.ObjectIterator(ue.UserWidget)
                    if w.get_class() == slot_class and w.get_owning_player() == state["pc"]],
                   key=lambda w: w.get_editor_property("SlotIndex"))
    ammo_widgets = [w for w in ue.ObjectIterator(ue.UserWidget)
                    if w.get_class() == ammo_class and w.get_owning_player() == state["pc"]]
    assert len(slots) == 3 and len(ammo_widgets) == 1, "Unexpected original QuickBar children"
    component_slots, active_index = list(state["qb"].get_slots()), state["qb"].get_active_slot_index()
    record = dict(event=event, active_index=active_index, slots=[], final_hud_slot=False)
    for index, widget in enumerate(slots):
        assert widget.get_editor_property("SlotIndex") == index
        assert widget.get_editor_property("QuickBarComponent") == state["qb"]
        empty = component_slots[index] is None
        selected = active_index == index
        assert widget.get_editor_property("IsEmpty") == empty, "Slot item did not update through original graph"
        assert widget.call_method("IsThisTheActiveQuickbarSlot") == selected, "Widget active-slot query disagrees with its component"
        # The original UpdateIsSelected returns early for empty slots; its cache
        # is animation state, not an authoritative selected-and-nonempty flag.
        cached_selected = widget.get_editor_property("IsSelected")
        if event == "reequipped_ui" and not empty:
            assert cached_selected == selected, "Active-slot message did not reach original graph"
        row = dict(index=index, empty=empty, selected=selected, animation_selected=cached_selected)
        if not empty and check_display:
            item = component_slots[index]
            total = item.get_stat_tag_stack_count(scope["ammo_tag"]) + item.get_stat_tag_stack_count(spare_tag)
            shown = widget.get_editor_property("WeaponAmmoCount").get_target_value()
            assert abs(shown - total) < 0.01, "Slot total ammo disagrees with inventory"
            row.update(total=total, displayed=shown)
        record["slots"].append(row)
    ammo_widget = ammo_widgets[0]
    item = state["qb"].get_active_slot_item()
    assert ammo_widget.get_editor_property("QuickBar") == state["qb"]
    assert ammo_widget.get_editor_property("InventoryItem") == item, "Active item binding is stale"
    if item and check_display:
        magazine, spare = item.get_stat_tag_stack_count(scope["ammo_tag"]), item.get_stat_tag_stack_count(spare_tag)
        magazine_text = str(ammo_widget.get_editor_property("AmmoLeftInMagazineWidget").get_text())
        spare_text = str(ammo_widget.get_editor_property("TotalCountWidget").get_text())
        assert magazine_text == str(magazine) and spare_text == str(spare), "Ammo text disagrees with real item"
        fragment = item.find_fragment_by_class(scope["cls"]("DCInventoryFragment_QuickBarIcon"))
        assert fragment, "Original rifle icon fragment missing"
        name_widget = ammo_widget.get_editor_property("WeaponNameWidget")
        assert name_widget.get_visibility() == ue.SlateVisibility.COLLAPSED, "Preserve original hidden name text"
        record.update(magazine=magazine, spare=spare, weapon_name_visible=False,
                      item_name=str(fragment.get_editor_property("DisplayNameWhenEquipped")))
    quick_state["checks"].append(record)
    print("DC_R7_QUICK_PIE " + json.dumps(record))


def inspect(event):
    result = old_inspect(event)
    inspect_quick(event)
    if event == "initial_ui" and not quick_state["captured"]:
        automation = ue.get_default_object(ue.load_class(None, "/Script/FunctionalTesting.AutomationBlueprintFunctionLibrary"))
        options = automation.call_method("GetDefaultScreenshotOptionsForGameplay", args=(ue.ComparisonTolerance.LOW, 0.0))
        automation.call_method("TakeAutomationScreenshotOfUI", args=(scope["state"]["world"], ue.LatentActionInfo(), "R7QuickBar_20261010_initial", options))
        quick_state["captured"] = True
    return result


def emit(event, **values):
    old_emit(event, **values)
    if event in ("empty_slot_clean", "unpossess_clean", "death_clean"):
        inspect_quick(event, False)


def finish(reason):
    if quick_state["widget"]:
        quick_state["widget"].remove_from_parent()
    quick_state["checks"].append(dict(finish=reason, asset_saves=0, quickslot_input_verified=False))
    (PROJECT / "Saved/Diagnostics/R7QuickBar/PIE-observations.json").write_text(json.dumps(quick_state["checks"], indent=2), encoding="utf-8")
    old_finish(reason)


scope.update(stage=stage, inspect_ui=inspect, emit=emit, finish=finish)
