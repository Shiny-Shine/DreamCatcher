"""Finish the final-approved six-ID Pistol MetaSound repair's cache checkpoint.

Apply_B already saved exactly six document IDs. This continuation changes only
the six corresponding cache keys, guarded by Apply_B hashes and document text.
Run through the full Editor with -ExecutePythonScript and
-DCPistolAudioRepair=finishcache. Opening the asset and saving invokes the
standard MetaSound editor registration path; it never auditions the sound.
This saves one asset through EditorAssetLibrary. It does not reload, audition,
cook, package, build, or run tests; those checks belong to the user.
"""
import csv
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import traceback

import unreal as u


PROJECT = Path("E:/Epic Games/UEProjects/DreamCatcher")
DIAGNOSTICS = PROJECT / "Saved/Diagnostics/PistolCookRepair20261023"
OUTPUT = DIAGNOSTICS / "Apply_D"
TARGET = "/Game/DreamCatcher/Lyra/Weapons/Pistol/Audio/MSS_Weapons_Pistol_Fire"
SHARED = "/Game/Audio/Sounds/Weapons/"
REPAIRS = (
    ("MS_GatedWavePlayer", "728E71154FBB3AE99A828492471ED856", "A17B01044114C219F2F2BBAC942340B1"),
    ("MS_LowAmmoTone", "7E5EA8D34156D229936AA7B3393C6DAC", "9436B8F3432FB3F96017E0BA10300790"),
    ("MS_RandomEQ", "C0904C9840D545D0B423BAA1FD2720A2", "178B6F30413C9EBE96186C9690D2159B"),
    ("MS_StereoGain", "555DB64343C323DACA13F2A3570F78A2", "3AD1E4824591E0CB2332598D3AD1F3CD"),
    ("MS_StereoHighShelf", "B4AC825943D61E7B6375C1B8FD433061", "521DC16148BA0E9160C043A287BACA66"),
    ("MS_WavePlayerCrossfader", "014521A74B80A4F9A1EBA2B41083D2A2", "BC307B3A43B601280D0D0F925D908473"),
)


def asset_file(package):
    assert package.startswith("/Game/")
    return PROJECT / "Content" / (package[6:] + ".uasset")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def document(obj):
    return obj.get_editor_property("RootMetasoundDocument").export_text()


def reference_cache(obj, stage):
    # The cache UPROPERTY fields are protected. Export through Unreal's object
    # exporter rather than relying on Python access to noneditable properties.
    path = OUTPUT / (stage + ".t3d")
    task = u.AssetExportTask()
    task.object, task.exporter, task.filename = obj, u.ObjectExporterT3D(), str(path)
    task.automated, task.prompt, task.replace_identical = True, False, False
    assert u.Exporter.run_asset_export_task(task), "Object export failed"
    data = path.read_bytes()
    text = data.decode("utf-16" if data[:2] in (b"\xff\xfe", b"\xfe\xff") else "utf-8-sig")
    return {
        "keys": [line.strip() for line in text.splitlines() if line.strip().startswith("ReferencedAssetClassKeys=")],
        "objects": [line.strip() for line in text.splitlines() if line.strip().startswith("ReferencedAssetClassObjects=")],
    }


def ensure_only_this_editor():
    # Read-only process inventory at the save boundary prevents concurrent saves.
    for image_name in ("UnrealEditor.exe", "UnrealEditor-Cmd.exe"):
        result = subprocess.run(
            ["tasklist.exe", "/FI", "IMAGENAME eq " + image_name, "/FO", "CSV", "/NH"],
            capture_output=True, check=True, text=True, encoding="mbcs", errors="replace",
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        for row in csv.reader(io.StringIO(result.stdout)):
            if len(row) >= 2 and row[0].lower() == image_name.lower():
                assert int(row[1]) == os.getpid(), ("Another Unreal Editor is running", row[:2])


def content_inventory():
    # File metadata catches any unexpected content save; the six shared assets
    # are additionally protected by SHA256 before and after the operation.
    roots = [PROJECT / "Content"]
    roots.extend(p for p in (PROJECT / "Plugins").rglob("Content") if p.is_dir())
    result = {}
    for root in roots:
        for path in root.rglob("*"):
            if path.is_file():
                stat = path.stat()
                result[str(path.relative_to(PROJECT))] = [stat.st_size, stat.st_mtime_ns]
    return result


command = u.SystemLibrary.get_command_line().lower()
assert all(flag in command for flag in (
    "-executepythonscript=", "-unattended", "-sccprovider=none", "-dcpistolaudiorepair=finishcache",
))
assert not u.SourceControl.is_enabled()
assert not OUTPUT.exists(), "Refusing to overwrite an earlier attempt"
ensure_only_this_editor()
OUTPUT.mkdir(parents=True)

baseline = json.loads((DIAGNOSTICS / "Inspect_A/report.json").read_text(encoding="utf-8-sig"))
checkpoint = json.loads((DIAGNOSTICS / "Apply_B/report.json").read_text(encoding="utf-8"))
assert checkpoint["document_exact_after_save"] and checkpoint["saved"] == [TARGET + "." + TARGET.rsplit("/", 1)[1]]
assert checkpoint["shared_assets_unchanged"] and not checkpoint["unexpected_content_changes"]
before_inventory = content_inventory()
before_hashes = {path: sha(asset_file(path)) for path in baseline["before_sha256"]}
assert before_hashes == checkpoint["after_sha256"], "Assets changed since the document repair checkpoint"
report = {
    "complete": False, "saved": [], "before_sha256": before_hashes,
    "verification_not_run": ["fresh_reload_verification", "audio_playback", "PIE", "cook", "package", "C++_build", "tests"],
}

try:
    # Load the actual six dependencies before the repaired target is serialized.
    references = []
    for name, old, new in REPAIRS:
        obj = u.load_asset(SHARED + name)
        assert obj, name
        current_document = document(obj)
        assert 'ClassName=(Namespace="",Name="' + new + '",Variant="")' in current_document, name
        assert current_document == baseline["assets"][SHARED + name]["document"], name
        references.append(obj)

    target = u.load_asset(TARGET)
    assert target
    doc = target.get_editor_property("RootMetasoundDocument")
    before = doc.export_text()
    expected = (DIAGNOSTICS / "Apply_B/document_expected.txt").read_text(encoding="utf-8")
    assert before == expected, "Loaded target differs from the document repair checkpoint"
    before_cache = reference_cache(target, "before")
    report["cache_before"] = before_cache
    assert before_cache == checkpoint["cache_after"], "Reference cache changed since checkpoint"
    assert len(before_cache["keys"]) == 1
    old_keys = re.findall(r'"([^"]+)"', before_cache["keys"][0])
    assert len(old_keys) == 6
    new_keys = list(old_keys)
    for name, old, new in REPAIRS:
        assert old not in expected and expected.count(new) == 1, name
        assert sum(key.count(old) for key in new_keys) == 1, name
        new_keys = [key.replace(old, new) for key in new_keys]
    assert 'ClassName=(Namespace="",Name="9FF6B5724AEF8E4580F83F93E255A528",Variant="")' in expected
    (OUTPUT / "document_before.txt").write_text(before, encoding="utf-8")
    (OUTPUT / "document_expected.txt").write_text(expected, encoding="utf-8")

    # No-audio commandlet PreSave did not rebuild this protected cache, and
    # direct reflection is prohibited. Use the standard asset editor's graph
    # registration path, without activating its playback controls.
    editor = u.get_editor_subsystem(u.AssetEditorSubsystem)
    assert editor.open_editor_for_assets([target]), "Could not open MetaSound asset editor"
    assert document(target) == expected, "Unexpected in-memory document change"
    during_cache = reference_cache(target, "before_save")
    assert during_cache["objects"] == before_cache["objects"]
    assert set(re.findall(r'"([^"]+)"', " ".join(during_cache["keys"]))) == set(new_keys)
    report["document_from_checkpoint_preserved"] = True
    report["exact_six_cache_key_changes_before_save"] = True
    ensure_only_this_editor()
    assert {path: sha(asset_file(path)) for path in before_hashes} == before_hashes
    assert u.EditorAssetLibrary.save_loaded_asset(target, only_if_is_dirty=False), "Asset save failed"
    report["saved"].append(target.get_path_name())
    report["cache_after"] = reference_cache(target, "after_save")
    after = document(target)
    (OUTPUT / "document_after_save.txt").write_text(after, encoding="utf-8")
    report["document_exact_after_save"] = after == expected
    assert after == expected, "UE serialization changed more than the approved dependency IDs"
    assert not any(old in " ".join(report["cache_after"]["keys"]) for _, old, _ in REPAIRS)
    assert all(new in " ".join(report["cache_after"]["keys"]) for _, _, new in REPAIRS)
    assert all(obj.get_path_name() in " ".join(report["cache_after"]["objects"]) for obj in references)
    report["complete"] = True
except Exception:
    report["error"] = traceback.format_exc()
    raise
finally:
    report["after_sha256"] = {path: sha(asset_file(path)) for path in before_hashes}
    target_relative = str(asset_file(TARGET).relative_to(PROJECT))
    after_inventory = content_inventory()
    changed = [path for path in set(before_inventory) | set(after_inventory)
               if before_inventory.get(path) != after_inventory.get(path)]
    report["changed_content_files"] = sorted(changed)
    report["shared_assets_unchanged"] = all(
        report["after_sha256"][path] == value for path, value in before_hashes.items() if path != TARGET
    )
    report["unexpected_content_changes"] = sorted(set(changed) - {target_relative})
    report["complete"] = report["complete"] and report["shared_assets_unchanged"] and not report["unexpected_content_changes"]
    (OUTPUT / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    u.log("PISTOL_AUDIO_REFERENCE_REPAIR " + json.dumps({
        "complete": report["complete"], "saved": report["saved"],
        "report": str(OUTPUT / "report.json"),
    }))
    u.SystemLibrary.quit_editor()
