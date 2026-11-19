"""Locate the Win64 Unreal Editor for DreamCatcher's standalone command printers."""
import json
import os
from pathlib import Path


_EDITOR = Path('Engine/Binaries/Win64/UnrealEditor-Cmd.exe')


def _installed_roots(association):
    program_data = os.environ.get('ProgramData')
    if program_data:
        manifest = Path(program_data) / 'Epic/UnrealEngineLauncher/LauncherInstalled.dat'
        try:
            installs = json.loads(manifest.read_text(encoding='utf-8-sig'))['InstallationList']
        except (OSError, ValueError, KeyError, TypeError):
            installs = []
        if isinstance(installs, list):
            for entry in installs:
                if isinstance(entry, dict) and entry.get('AppName') == 'UE_' + association:
                    location = entry.get('InstallLocation')
                    if isinstance(location, str) and Path(location).is_absolute():
                        yield Path(location)

    # Source builds registered by UnrealVersionSelector use the project association as the value name.
    try:
        import winreg
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, r'SOFTWARE\Epic Games\Unreal Engine\Builds') as key:
            location, kind = winreg.QueryValueEx(key, association)
    except (ImportError, OSError):
        return
    if kind in (winreg.REG_SZ, winreg.REG_EXPAND_SZ):
        root = Path(os.path.expandvars(location))
        if root.is_absolute():
            yield root


def editor_command(project_root: Path) -> str:
    override = os.environ.get('DC_UNREAL_EDITOR_CMD')
    if override:
        editor = Path(override)
        if editor.is_absolute() and editor.name.casefold() == _EDITOR.name.casefold() and editor.is_file():
            return str(editor)
        raise ValueError('DC_UNREAL_EDITOR_CMD must be the full path to an existing UnrealEditor-Cmd.exe')

    project = project_root / 'DreamCatcher.uproject'
    association = json.loads(project.read_text(encoding='utf-8-sig')).get('EngineAssociation', '')
    if not association:
        roots = (project_root, *project_root.parents)
    elif '/' in association or '\\' in association:
        roots = [(project_root / association).resolve()]
    else:
        roots = _installed_roots(association)
    for root in roots:
        editor = root / _EDITOR
        if editor.is_file():
            return str(editor)
    raise FileNotFoundError(
        f'Cannot find UnrealEditor-Cmd.exe for {project} (EngineAssociation={association!r}). '
        'Set DC_UNREAL_EDITOR_CMD to the full path of the matching editor executable.'
    )
