"""Shared read-only Unreal Editor helpers used by the spawner tools.

The one-time role BaseDamage apply/verify commandlet has been retired.
Keep this import path for spawner_proximity_setup.py and
spawner_registration_probe.py. Story damage is authored in the Difficulty
Presets; this module does not load, compile or save game assets.
"""
import ctypes
from ctypes import wintypes
import os

import unreal as ue


def assert_no_other_editor():
    """Read process names only; refuse concurrent editors rather than stopping any."""
    class ProcessEntry(ctypes.Structure):
        _fields_ = [('dwSize', wintypes.DWORD), ('cntUsage', wintypes.DWORD),
                    ('th32ProcessID', wintypes.DWORD), ('th32DefaultHeapID', ctypes.c_size_t),
                    ('th32ModuleID', wintypes.DWORD), ('cntThreads', wintypes.DWORD),
                    ('th32ParentProcessID', wintypes.DWORD), ('pcPriClassBase', wintypes.LONG),
                    ('dwFlags', wintypes.DWORD), ('szExeFile', wintypes.WCHAR * 260)]
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    api.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    api.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessEntry)]
    api.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = api.CreateToolhelp32Snapshot(2, 0)
    assert handle not in (None, ctypes.c_void_p(-1).value), 'Cannot inspect running editors'
    others = []
    try:
        entry = ProcessEntry()
        entry.dwSize = ctypes.sizeof(entry)
        more = api.Process32FirstW(handle, ctypes.byref(entry))
        assert more, 'Cannot enumerate processes'
        while more:
            if entry.szExeFile.lower().startswith('unrealeditor') and entry.th32ProcessID != os.getpid():
                others.append([entry.th32ProcessID, entry.szExeFile])
            more = api.Process32NextW(handle, ctypes.byref(entry))
    finally:
        api.CloseHandle(handle)
    assert not others, 'Close other Unreal Editors before applying/verifying: ' + str(others)


def normalize(text):
    # Do not discard GUIDs, references, arbitrary tags or numeric properties.
    return '\n'.join(line.rstrip() for line in text.splitlines()).strip()


def export(obj, path):
    assert not path.exists(), str(path)
    task = ue.AssetExportTask()
    task.object = obj
    task.exporter = ue.ObjectExporterT3D()
    task.filename = str(path)
    task.automated = True
    task.prompt = False
    assert ue.Exporter.run_asset_export_task(task), obj.get_path_name()
    raw = path.read_bytes()
    return normalize(raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig'))


def graph_snapshot(bp, directory, label):
    return {graph.get_name(): export(graph, directory / (label + '_' + str(index) + '.t3d'))
            for index, graph in enumerate(ue.BlueprintEditorLibrary.list_graphs(bp))}


if __name__ == '__main__':
    raise RuntimeError(
        'The legacy BaseDamage apply/verify commandlet has been retired. '
        'Edit story damage in /Game/DreamCatcher/Data/Difficulty/. '
        'This module only provides shared read-only editor helpers.'
    )
