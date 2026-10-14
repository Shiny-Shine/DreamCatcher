"""Offline report comparison only; does not import Unreal or modify any files."""
import hashlib
import json
from pathlib import Path
import re
import sys
import zipfile
from contextlib import contextmanager


def records(path):
    result = []
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if "DC_RIFLE_SHARED " in line:
            result.append(json.loads(line.split("DC_RIFLE_SHARED ", 1)[1]))
    return result


@contextmanager
def report_stream(path):
    path = Path(path).resolve()
    report_root = (Path(__file__).resolve().parents[2] / "Saved/Diagnostics/R5SharedAudit").resolve()
    if not path.is_relative_to(report_root):
        raise RuntimeError("Report path outside generated audit scope")
    if path.is_file():
        with path.open("rb") as stream:
            yield stream
    else:
        # Large generated reports may have been archived after byte-for-byte verification.
        archive_path = report_root.parent / "R5SharedAudit_20261006.zip"
        with zipfile.ZipFile(archive_path) as archive:
            with archive.open("R5SharedAudit/" + path.relative_to(report_root).as_posix()) as stream:
                yield stream


def read_export(path):
    with report_stream(path) as stream:
        data = stream.read()
    return data.decode("utf-16" if data.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").replace("\r\n", "\n")


def main():
    target = Path(__file__).resolve().parents[2]
    original = target.parent / "LyraStarterGame"
    log = sys.argv[1] if len(sys.argv) > 1 else "R5SharedAudit-full2.log"
    before = records(original / "Saved/Logs" / log)
    after = records(target / "Saved/Logs" / log)
    if not any(r["event"] == "complete" for r in before) or not any(r["event"] == "complete" for r in after):
        raise RuntimeError("Both audit processes must complete before comparison")
    old = {r["key"]: r for r in before if r["event"] == "asset"}
    new = {r["key"]: r for r in after if r["event"] == "asset"}
    redirects = {}
    for line in (target / "Config/DefaultEngine.ini").read_text(encoding="utf-8-sig").splitlines():
        if line.lstrip().startswith("+ClassRedirects="):
            match = re.search(r'OldName="([^"]+)",NewName="([^"]+)"', line)
            if match:
                redirects[match.group(1)] = match.group(2)

    def canonical(value, class_redirects=True):
        if isinstance(value, str):
            # Only the observed unbound delegate representation contains a process address.
            # Do not normalize bound delegates or arbitrary hex/GUID asset data.
            if re.fullmatch(r"<Multicast delegate '[^']+' \(0x[0-9A-Fa-f]+\) <Unbound>>", value):
                return re.sub(r"\(0x[0-9A-Fa-f]+\)", "(process-local)", value)
            for old_path, new_path in sorted(redirects.items(), key=lambda pair: -len(pair[0])) if class_redirects else []:
                if old_path not in value:
                    continue
                value = re.sub(re.escape(old_path) + r"(?=$|[\s'\":().,=])", lambda _: new_path, value)
            return value
        if isinstance(value, list):
            return [canonical(item, class_redirects) for item in value]
        if isinstance(value, dict):
            return {key: canonical(item, class_redirects) for key, item in value.items()}
        return value

    output = []
    for key in sorted(set(old) & set(new)):
        source, dest = old[key], new[key]
        lhs, rhs = canonical(source["fields"]["values"]), canonical(dest["fields"]["values"], False)
        field_diffs = [name for name in sorted(set(lhs) | set(rhs)) if lhs.get(name) != rhs.get(name)]
        expected_deps = set(source["dependencies"])
        # Only known same-asset identity for this explicitly paired UDS export.
        if key == "StructUI":
            expected_deps.discard(source["package"])
            expected_deps.add(dest["package"]) if source["package"] in source["dependencies"] else None
        text_equal = None
        if source["text"]["success"] and dest["text"]["success"]:
            source_text = canonical(read_export(source["text"]["path"]))
            if key == "StructUI":
                source_text = source_text.replace(source["package"] + ".", dest["package"] + ".")
            text_equal = source_text == read_export(dest["text"]["path"])
        pixels_equal = None
        a, b = source["texture_pixels"], dest["texture_pixels"]
        if a and b and a["success"] and b["success"] and Path(a["path"]).suffix == Path(b["path"]).suffix:
            with report_stream(a["path"]) as f:
                a_hash = hashlib.file_digest(f, "sha256").hexdigest()
            with report_stream(b["path"]) as f:
                b_hash = hashlib.file_digest(f, "sha256").hexdigest()
            pixels_equal = a_hash == b_hash
        item = {"key": key, "package": source["package"], "class": source["asset_class"],
                "class_equal": canonical(source["asset_class"]) == dest["asset_class"],
                "field_differences": field_diffs, "readable_fields": len(lhs),
                "unavailable_fields": sorted(set(source["fields"]["unavailable"]) | set(dest["fields"]["unavailable"])),
                "dependencies_missing": sorted(expected_deps - set(dest["dependencies"])),
                "dependencies_extra": sorted(set(dest["dependencies"]) - expected_deps),
                "text_export_equal": text_equal, "texture_export_equal": pixels_equal}
        if key == "StructUI":
            a_text, b_text = read_export(source["text"]["path"]), read_export(dest["text"]["path"])
            member_lines = lambda text: [line.strip() for line in text.splitlines() if line.strip().startswith("VariablesDescriptions(")]
            item["struct_member_descriptions_equal"] = member_lines(a_text) == member_lines(b_text)
            item["source_struct_guid"] = re.findall(r"^   Guid=(\w+)$", a_text, re.MULTILINE)
            item["target_struct_guid"] = re.findall(r"^   Guid=(\w+)$", b_text, re.MULTILINE)
        output.append(item)
        print("DC_RIFLE_COMPARISON " + json.dumps(item, ensure_ascii=False))
    common = [item for item in output if item["key"].startswith("C")]
    print("DC_RIFLE_COMPARISON_SUMMARY " + json.dumps({
        "compared_common": len(common), "unavailable_source": [r for r in before if r["event"] == "asset_unverified"],
        "unavailable_target": [r for r in after if r["event"] == "asset_unverified"],
        "same_readable_fields": sum(not item["field_differences"] for item in common),
        "same_text_export": sum(item["text_export_equal"] is True for item in common),
        "same_texture_export": sum(item["texture_export_equal"] is True for item in common),
        "different_texture_export": sum(item["texture_export_equal"] is False for item in common),
        "same_dependencies": sum(not item["dependencies_missing"] and not item["dependencies_extra"] for item in common),
        "coverage": "Reported/reflected values, registry dependencies, text and supported texture exports only; not all native/bulk/runtime data."
    }, ensure_ascii=False))


if __name__ == "__main__":
    main()
