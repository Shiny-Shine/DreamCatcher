"""Read one rendered layout from an already user-started PIE session.

Run in Unreal Editor's Output Log (Cmd mode):
py "E:/Epic Games/UEProjects/DreamCatcher/Scripts/Editor/reticle_layout_capture.py"

No PIE startup, layout refresh, asset loading/saving, or gameplay changes.
Only a new Saved/Diagnostics JSON report is written. Geometry is a snapshot
from the preceding rendered frame, not proof of the visible marker's paint.
"""

from datetime import datetime, timezone
import json
import math
from pathlib import Path
import traceback


PROJECT = Path(__file__).resolve().parents[2]
REPORT_ROOT = PROJECT / "Saved/Diagnostics/ReticlePlacement20261024"
CLASSES = {
    "W_OverallUILayout_C": "overall_layout",
    "W_ShooterHUDLayout_C": "hud",
    "W_WeaponReticleHost_C": "host",
    "W_Reticle_Rifle_C": "rifle_reticle",
    "W_Reticle_Pistol_C": "pistol_reticle",
}
REQUIRED_CHILDREN = {
    "hud": ("SafeZone_0", "Overlay_0", "CanvasPanel_0", "ExtensionPoint_Reticle"),
    "host": ("VisWrapper", "WidgetStack"),
    "rifle_reticle": ("CrossHairs", "HitMarkerConfirmations"),
    "pistol_reticle": ("CrossHairs", "HitMarkerConfirmations"),
}


def _vector(value):
    result = [float(value.x), float(value.y)]
    if not all(math.isfinite(number) for number in result):
        raise ValueError("Non-finite geometry coordinate")
    return result


def _path(obj):
    return obj.get_path_name() if obj is not None else None


def _read(reader):
    try:
        return reader()
    except Exception as exc:
        return {"read_error": str(exc)}


def _valid(ue, obj):
    try:
        return obj is not None and ue.SystemLibrary.is_valid(obj)
    except Exception as exc:
        if "ObjectInstance is null" in str(exc):
            return False
        raise


def _slot_record(ue, slot):
    if slot is None:
        return None
    row = {"path": _path(slot), "class": _path(slot.get_class())}
    if isinstance(slot, ue.CanvasPanelSlot):
        row["layout"] = _read(lambda: slot.get_layout().export_text())
        row["auto_size"] = _read(lambda: bool(slot.get_auto_size()))
        row["z_order"] = _read(lambda: int(slot.get_z_order()))
    else:
        for name in ("horizontal_alignment", "vertical_alignment", "padding"):
            value = _read(lambda name=name: slot.get_editor_property(name))
            row[name] = value if isinstance(value, dict) else str(value)
    return row


def _measure_geometry(ue, world, geometry, viewport_size):
    """Never replace invalid rendered geometry with desired or slot sizes."""
    size = _vector(ue.SlateLibrary.get_local_size(geometry))
    result = {"local_size": size, "valid": False}
    if min(size) <= 0:
        result["invalid_reason"] = "Zero or negative rendered size"
        return result
    origin = ue.Vector2D(0.0, 0.0)
    center = ue.Vector2D(size[0] * 0.5, size[1] * 0.5)
    result["absolute_origin"] = _vector(ue.SlateLibrary.local_to_absolute(geometry, origin))
    result["absolute_center"] = _vector(ue.SlateLibrary.local_to_absolute(geometry, center))
    for label, point in (("origin", origin), ("center", center)):
        pixel, viewport = ue.SlateLibrary.local_to_viewport(world, geometry, point)
        result["pixel_" + label] = _vector(pixel)
        result["viewport_" + label] = _vector(viewport)
    if len(viewport_size) != 2 or min(viewport_size) <= 0:
        result["invalid_reason"] = "Viewport size unavailable or zero"
        return result
    result["pixel_center_delta_from_viewport_center"] = [
        result["pixel_center"][axis] - viewport_size[axis] * 0.5 for axis in (0, 1)
    ]
    result["valid"] = True
    return result


def _widget_record(ue, world, widget, viewport_size):
    slot = _read(lambda: widget.get_editor_property("slot"))
    row = {
        "path": _path(widget),
        "name": widget.get_name(),
        "class": _path(widget.get_class()),
        "outer": _path(widget.get_outer()),
        "parent": _read(lambda: _path(widget.get_parent())),
        "visibility": _read(lambda: str(widget.get_visibility())),
        "visible": _read(lambda: bool(widget.is_visible())),
        "in_viewport": _read(lambda: bool(widget.is_in_viewport())),
        "render_opacity": _read(lambda: float(widget.get_render_opacity())),
        "desired_size_reference_only": _read(lambda: _vector(widget.get_desired_size())),
        "render_transform": _read(lambda: widget.get_editor_property("render_transform").export_text()),
        "render_transform_pivot": _read(lambda: _vector(widget.get_editor_property("render_transform_pivot"))),
        "slot": _read(lambda: _slot_record(ue, slot)),
    }
    if isinstance(widget, ue.UserWidget):
        row["owning_player"] = _read(lambda: _path(widget.get_owning_player()))
    if isinstance(widget, ue.CommonActivatableWidget):
        row["activated"] = _read(lambda: bool(widget.is_activated()))
    for label, method in (("cached", "get_cached_geometry"),
                          ("tick", "get_tick_space_geometry"),
                          ("paint", "get_paint_space_geometry")):
        row[label] = _read(lambda method=method: _measure_geometry(
            ue, world, getattr(widget, method)(), viewport_size))
    return row


def _rendered(row):
    return (row.get("visible") is True
            and isinstance(row.get("render_opacity"), (int, float))
            and row["render_opacity"] > 0
            and row.get("paint", {}).get("valid") is True)


def _owning_selection(widget, selected):
    current = widget
    for _ in range(64):
        if current is None:
            return None
        path = _path(current)
        if path in selected:
            return path
        current = current.get_outer()
    raise RuntimeError("Unexpectedly deep UObject ownership chain")


def capture(ue):
    """Queries only; the caller separately writes the report as JSON."""
    if Path(ue.Paths.project_dir()).resolve() != PROJECT:
        raise RuntimeError("Run this script in the DreamCatcher project")
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    if world is None:
        raise RuntimeError("Level_1 PIE must already be running. This script never starts PIE.")
    pc = ue.GameplayStatics.get_player_controller(world, 0)
    if pc is None:
        raise RuntimeError("The existing game world has no local player controller yet")
    viewport_size = _vector(ue.WidgetLayoutLibrary.get_viewport_size(world))
    report = {
        "status": "captured",
        "utc": datetime.now(timezone.utc).isoformat(),
        "frame": int(ue.SystemLibrary.get_frame_count()),
        "world": _path(world),
        "controller": _path(pc),
        "controller_class": _path(pc.get_class()),
        "game_mode": _read(lambda: _path(ue.GameplayStatics.get_game_mode(world))),
        "viewport_size_pixels": viewport_size,
        "viewport_dpi_scale": _read(lambda: float(ue.WidgetLayoutLibrary.get_viewport_scale(world))),
        "controller_viewport_size_pixels": _read(lambda: list(pc.get_viewport_size())),
        "viewport_geometry": _read(lambda: _measure_geometry(
            ue, world, ue.WidgetLayoutLibrary.get_viewport_widget_geometry(world), viewport_size)),
        "player_screen_geometry": _read(lambda: _measure_geometry(
            ue, world, ue.WidgetLayoutLibrary.get_player_screen_widget_geometry(pc), viewport_size)),
        "automatic_PIE": False,
        "asset_saves": 0,
        "cpp_or_config_changes": False,
        "limitations": [
            "Single snapshot of prior-frame geometry; zero/read errors are invalid.",
            "Positive geometry does not prove that the marker's custom paint is centered.",
            "Hidden/inactive instances are retained in the report, not treated as active.",
        ],
    }
    selected = {}
    for widget in ue.ObjectIterator(ue.UserWidget):
        if not _valid(ue, widget) or widget.get_name().startswith("Default__"):
            continue
        role = CLASSES.get(widget.get_class().get_name())
        if role and widget.get_world() == world:
            selected[_path(widget)] = (widget, role)
    records = {}
    for path, (widget, role) in sorted(selected.items()):
        row = _widget_record(ue, world, widget, viewport_size)
        row.update(role=role, children=[], parent_chain=[])
        parent = widget.get_parent()
        for _ in range(12):
            if parent is None:
                break
            row["parent_chain"].append(_widget_record(ue, world, parent, viewport_size))
            parent = parent.get_parent()
        records[path] = row
    # GetRootWidget/GetWidgetFromName are native-only in this engine. Read the
    # actual widget objects by UObject ownership instead of loading asset trees.
    for widget in ue.ObjectIterator(ue.Widget):
        if not _valid(ue, widget) or _path(widget) in selected:
            continue
        owner = _owning_selection(widget, selected)
        if owner is not None:
            records[owner]["children"].append(_widget_record(ue, world, widget, viewport_size))
    counts = {}
    ready_roles = set()
    for row in records.values():
        row["children"].sort(key=lambda item: item["path"])
        role = row["role"]
        count = counts.setdefault(role, {"instances": 0, "rendered_candidates": 0})
        count["instances"] += 1
        candidates = {item["name"]: item for item in row["children"]}
        required = REQUIRED_CHILDREN.get(role, ())
        row["missing_required_children"] = [name for name in required if name not in candidates]
        row["invalid_required_child_geometry"] = [
            name for name in required if name in candidates and
            candidates[name].get("paint", {}).get("valid") is not True
        ]
        if _rendered(row) and row.get("activated", True) is not False:
            count["rendered_candidates"] += 1
            if not row["missing_required_children"] and not row["invalid_required_child_geometry"]:
                ready_roles.add(role)
    report["instance_counts"] = counts
    report["widgets"] = list(records.values())
    required_roles = ("hud", "host")
    report["missing_or_invalid_roles"] = [role for role in required_roles if role not in ready_roles]
    if not ready_roles.intersection(("rifle_reticle", "pistol_reticle")):
        report["missing_or_invalid_roles"].append("active_reticle")
    report["duplicate_rendered_roles"] = [
        role for role, count in counts.items() if count["rendered_candidates"] > 1
    ]
    viewport_valid = report["viewport_geometry"].get("valid") is True
    player_valid = report["player_screen_geometry"].get("valid") is True
    report["geometry_ready_for_diagnosis"] = (
        not report["missing_or_invalid_roles"] and viewport_valid and player_valid)
    if not report["geometry_ready_for_diagnosis"]:
        report["status"] = "captured_with_missing_or_invalid_geometry"
    return report


def main():
    import unreal as ue

    try:
        report = capture(ue)
    except Exception:
        report = {"status": "capture_failed", "error": traceback.format_exc(),
                  "automatic_PIE": False, "asset_saves": 0, "cpp_or_config_changes": False}
    directory = REPORT_ROOT / ("Live_" + datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S_%fZ"))
    directory.mkdir(parents=True, exist_ok=False)
    output = directory / "report.json"
    with output.open("x", encoding="utf-8") as handle:
        json.dump(report, handle, ensure_ascii=False, indent=2, allow_nan=False)
        handle.write("\n")
    logger = ue.log_warning if report["status"] != "captured" else ue.log
    logger("RETICLE_LAYOUT_CAPTURE " + report["status"] + " -> " + str(output))
    if report.get("error"):
        ue.log_warning(report["error"])


if __name__ == "__main__":
    main()
