"""One-map PIE Root UI regression; reuses the existing real weapon/Host probe.

Only creates temporary empty layer widgets in PIE, then pops them immediately.
No game assets, Config, Blueprint graphs, or C++ are edited by this probe.
"""
from pathlib import Path
import json
import unreal as ue

PROJECT = Path(ue.Paths.convert_relative_path_to_full(ue.Paths.project_dir())).resolve()
scope = {"__name__": "r7_existing_host_probe"}
exec(compile((PROJECT / "Scripts/Editor/r7_reticle_host_probe.py").read_text(encoding="utf-8-sig"),
             "r7_reticle_host_probe.py", "exec"), scope)
original_inspect = scope["inspect_ui"]
original_finish = scope["finish"]
root_class = ue.load_class(None, "/Game/LyraMigration/UI/R7/Foundation/W_OverallUILayout.W_OverallUILayout_C")
extensions = ue.get_default_object(ue.load_class(None, "/Script/CommonGame.CommonUIExtensions"))
empty_widget_class = ue.load_class(None, "/Script/CommonUI.CommonActivatableWidget")
root_state = dict(root=None, layer_tested=False, observations=[])
LAYERS = {"UI.Layer.Game": "GameLayer_Stack", "UI.Layer.GameMenu": "GameMenu_Stack",
          "UI.Layer.Menu": "Menu_Stack", "UI.Layer.Modal": "Modal_Stack"}


def inspect_root(event):
    pc = scope["state"]["pc"]
    roots = [w for w in ue.ObjectIterator(ue.UserWidget)
             if w.get_class() == root_class and w.get_owning_player() == pc and w.is_in_viewport()]
    assert len(roots) == 1, "Expected exactly one policy-created viewport Root"
    root = roots[0]
    if root_state["root"] is not None:
        assert root_state["root"] == root, "Root was unnecessarily recreated during Pawn/weapon lifecycle"
    root_state["root"] = root
    # These four Blueprint variables are used by the original RegisterLayer graph.
    # Runtime widget subobject paths need not use the editor template ':' prefix.
    stacks = {name: root.get_editor_property(name) for name in LAYERS.values()}
    assert all(w and w.get_class().get_path_name() == "/Script/CommonUI.CommonActivatableWidgetStack"
               for w in stacks.values()), "Missing/unexpected Root layer containers"
    if not root_state["layer_tested"]:
        local_player = extensions.call_method("GetLocalPlayerFromController", args=(pc,))
        for layer, name in LAYERS.items():
            assert stacks[name].get_active_widget() is None, "Expected empty foundation layer"
            widget = extensions.call_method("PushContentToLayer_ForPlayer", args=(local_player, scope["tag"](layer), empty_widget_class))
            assert widget and stacks[name].get_active_widget() == widget, "Layer tag did not resolve to its original stack"
            extensions.call_method("PopContentFromLayer", args=(widget,))
            assert stacks[name].get_active_widget() is None, "Layer pop did not clean up"
        root_state["layer_tested"] = True
    record = dict(event=event, root=root.get_path_name(), viewport_roots=len(roots),
                  four_layer_push_pop=root_state["layer_tested"], stacks=sorted(stacks))
    root_state["observations"].append(record)
    print("DC_R7_ROOT_PIE " + json.dumps(record))


def inspect(event):
    inspect_root(event)
    return original_inspect(event)


def finish(reason):
    if reason == "host_data_and_lifecycle_pass":
        inspect_root("death_root_retained")
    root_state["observations"].append(dict(finish=reason, asset_saves=0))
    out = PROJECT / "Saved/Diagnostics/R7RootUI/PIE-observations.json"
    out.write_text(json.dumps(root_state["observations"], indent=2), encoding="utf-8")
    original_finish(reason)


scope["inspect_ui"] = inspect
scope["finish"] = finish
