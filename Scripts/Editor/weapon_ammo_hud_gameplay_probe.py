"""TODO7 manual helper for an ALREADY running user PIE. No automatic PIE or input.

Import this module in the Editor Python console, then explicitly call:
    module.snapshot('User_1080_A')
Optional, only when the user asks to prepare duplicate instances of their current
weapon for 2/3-row manual inspection:
    module.prepare_test_slots(3)
    module.snapshot('User_ThreeRows_A')
    module.cleanup_test_slots()  # Select an original weapon before cleanup.
Running this file directly performs only one read-only snapshot.
No shooting, reload, travel, asset/CDO edits or saves. No tick callback or timer.
Stored helper state contains only strings/numbers, never PIE UObject references.
"""
import sys

sys.dont_write_bytecode = True

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import traceback

import unreal as ue

PROJECT = Path('E:/Epic Games/UEProjects/DreamCatcher')
REPORTS = PROJECT / 'Saved/Diagnostics/WeaponAmmo20261020'
BASELINE = REPORTS / 'Before_A/baseline.json'
ROOT_CLASS = '/Game/DreamCatcher/Lyra/UI/Shared/Equipment/W_QuickBar.W_QuickBar_C'
HUD_CLASS = '/Game/DreamCatcher/Lyra/UI/Stage/ShooterCore/UserInterface/W_ShooterHUDLayout.W_ShooterHUDLayout_C'
_prepared = None


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def protection():
    baseline = json.loads(BASELINE.read_text(encoding='utf-8-sig'))
    names = list(baseline['protected']) + list(baseline['approved_assets'])
    names += [file.relative_to(PROJECT).as_posix() for file in (PROJECT / 'Saved/SaveGames').rglob('*') if file.is_file()]
    files = {}
    for root in ('Content', 'Plugins', 'Source', 'Config', 'Scripts', 'docs', 'Saved/SaveGames'):
        for file in (PROJECT / root).rglob('*'):
            if file.is_file():
                info = file.stat()
                files[file.relative_to(PROJECT).as_posix()] = [info.st_size, info.st_mtime_ns]
    return {'inventory': files, 'hashes': {name: sha(PROJECT / name) for name in names}}


def valid(obj):
    if obj is None:
        return False
    if not isinstance(obj, ue.Object):
        raise TypeError('Expected UObject or None')
    try:
        obj.get_path_name()
    except Exception as exc:
        if re.fullmatch(r'(?:[^:\r\n]+: )?Internal Error - ObjectInstance is null!', str(exc)):
            return False
        raise
    return ue.SystemLibrary.is_valid(obj)


def identity(obj):
    # UObject wrapper hash derives from the native object; path alone can recur after PIE/travel.
    return {'path': obj.get_path_name(), 'native_hash': hash(obj)} if valid(obj) else None


def native(name):
    cls = ue.load_class(None, '/Script/DreamCatcher.' + name)
    assert cls, name
    return cls


def tag(name):
    value = ue.GameplayTag()
    assert value.import_text('(TagName="Lyra.ShooterGame.Weapon.' + name + '")')
    return value


def current_context():
    world = ue.get_editor_subsystem(ue.UnrealEditorSubsystem).get_game_world()
    assert valid(world), 'Start your PIE session first; this helper never starts PIE'
    pc = ue.GameplayStatics.get_player_controller(world, 0)
    assert valid(pc) and pc.get_world() == world and pc.is_local_controller(), 'Local player controller is not ready'
    pawn = pc.get_pawn()
    assert valid(pawn) and pawn.get_world() == world, 'Possessed pawn is not ready'
    quickbar = pc.get_component_by_class(native('DCLyraQuickBarComponent'))
    assert valid(quickbar) and quickbar.get_owner() == pc and quickbar.get_world() == world, 'QuickBar is not ready'
    return world, pc, pawn, quickbar


def owned_by(obj, owner):
    outer = obj.get_outer()
    while outer:
        if outer == owner:
            return True
        outer = outer.get_outer()
    return False


def child(owner, name):
    values = [widget for widget in ue.ObjectIterator(ue.Widget)
              if valid(widget) and widget.get_name() == name and owned_by(widget, owner)]
    assert len(values) == 1, (owner.get_path_name(), name, len(values))
    return values[0]


def active_root(world, pc):
    layouts = [widget for widget in ue.ObjectIterator(ue.UserWidget)
               if valid(widget) and widget.get_class().get_path_name() == HUD_CLASS
               and not widget.get_name().startswith('Default__') and widget.get_world() == world
               and widget.get_owning_player() == pc and widget.is_activated()]
    assert len(layouts) == 1, 'Expected exactly one active shooter HUD'
    point = child(layouts[0], 'ExtensionPoint_Equipment')
    assert isinstance(point, ue.DynamicEntryBoxBase)
    roots = [entry for entry in point.get_all_entries() if valid(entry)
             and entry.get_class().get_path_name() == ROOT_CLASS and entry.get_owning_player() == pc]
    assert len(roots) == 1, 'Expected one current Equipment entry; dormant objects are excluded'
    return roots[0]


def item_data(item):
    if not valid(item):
        return None
    fragment = item.find_fragment_by_class(native('DCInventoryFragment_QuickBarIcon'))
    return {'identity': identity(item), 'magazine': item.get_stat_tag_stack_count(tag('MagazineAmmo')),
            'spare': item.get_stat_tag_stack_count(tag('SpareAmmo')),
            'display_name': str(fragment.get_editor_property('DisplayNameWhenEquipped')) if valid(fragment) else None,
            'brush': fragment.get_editor_property('Brush').export_text() if valid(fragment) else None,
            'ammo_brush': fragment.get_editor_property('AmmoBrush').export_text() if valid(fragment) else None}


def snapshot(run=None):
    """One read-only UI/data observation; it does not assert pixel paint/gameplay success."""
    run = run or ('User_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f'))
    assert re.fullmatch(r'[A-Za-z0-9_]+', run)
    directory = REPORTS / run
    assert not (directory / 'snapshot.json').exists(), 'Choose a fresh snapshot name'
    before = protection()
    report = {'schema': 1, 'run': run, 'mode': 'manual_read_only_snapshot', 'complete': False,
              'frame': int(ue.SystemLibrary.get_frame_count()), 'asset_saves': 0,
              'automatic_PIE': False, 'input_or_travel': False, 'rows': [], 'errors': [],
              'pixel_paint_verified': False, 'gameplay_verified': False,
              'geometry_note': 'Template/desired size is not actual pixel paint. User verifies both resolutions.'}
    try:
        world, pc, pawn, quickbar = current_context()
        root = active_root(world, pc)
        slots = list(quickbar.get_slots())
        report.update(world=identity(world), pc=identity(pc), pawn=identity(pawn), root=identity(root),
                      active_index=int(quickbar.get_active_slot_index()), slot_count=len(slots),
                      panel_visibility=str(child(root, 'DCWeaponPanel').get_visibility()))
        for index in range(3):
            row = child(root, 'W_QuickBarSlot_' + str(index + 1))
            item = item_data(slots[index] if index < len(slots) else None)
            magazine = child(row, 'WeaponAmmoCount')
            spare = child(row, 'DCWeaponSpareAmmo')
            name = str(child(row, 'DCWeaponDisplayName').get_text())
            number = str(child(row, 'DCWeaponSlotNumber').get_text())
            visible = child(row, 'SelectionBorder').get_visibility() != ue.SlateVisibility.COLLAPSED
            observation = {'slot_index': int(row.get_editor_property('SlotIndex')), 'row': identity(row),
                           'source_item': item, 'content_visible': visible, 'name_text': name, 'slot_text': number,
                           'magazine_text': str(magazine.get_text()), 'spare_text': str(spare.get_text()),
                           'magazine_numeric': float(magazine.get_editor_property('CurrentNumericValue')),
                           'spare_numeric': float(spare.get_editor_property('CurrentNumericValue')),
                           'template_height_override': child(row, 'SizeBox_274').get_editor_property('HeightOverride')}
            observation['data_matches_at_snapshot'] = (not visible if item is None else
                visible and observation['slot_index'] == index and number == str(index + 1)
                and name == item['display_name'] and abs(observation['magazine_numeric'] - item['magazine']) < .01
                and abs(observation['spare_numeric'] - item['spare']) < .01)
            report['rows'].append(observation)
        report['complete'] = True
    except Exception:
        report['errors'].append(traceback.format_exc())
    finally:
        report['protection_unchanged'] = protection() == before
        report['complete'] = report['complete'] and report['protection_unchanged']
        directory.mkdir(parents=True, exist_ok=True)
        (directory / 'snapshot.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    ue.log('Weapon/ammo manual snapshot: ' + str(directory / 'snapshot.json'))
    return report


def original_state(quickbar, inventory):
    return {'slots': [identity(item) for item in quickbar.get_slots()],
            'active_index': int(quickbar.get_active_slot_index()),
            'items': {item.get_path_name(): {'identity': identity(item),
                      'magazine': item.get_stat_tag_stack_count(tag('MagazineAmmo')),
                      'spare': item.get_stat_tag_stack_count(tag('SpareAmmo'))}
                      for item in inventory.get_all_items() if valid(item)}}


def check_originals(state, quickbar, inventory):
    current = original_state(quickbar, inventory)
    assert current['active_index'] == state['active_index'], 'Original active index changed'
    assert len(current['slots']) == len(state['slots'])
    for index, previous in enumerate(state['slots']):
        if previous is not None:
            assert current['slots'][index] == previous, 'Existing slot changed'
    for path, previous in state['items'].items():
        assert current['items'].get(path) == previous, 'Existing item/stat changed'


def item_definition_path(text):
    # Exactly one exported root ItemDef field; no guessed ID or general T3D parser.
    fields = re.findall(r'^   ItemDef="[^\'"\r\n]+\'([^\'\r\n]+)\'"\s*$', text, re.M)
    assert len(fields) == 1 and fields[0].startswith(('/Game/', '/Script/')), 'Expected one exact exported ItemDef class reference'
    assert len(re.findall(r'^\s*ItemDef=', text, re.M)) == 1, 'Ambiguous ItemDef fields'
    return fields[0]


def read_active_definition(item):
    directory = REPORTS / ('Manual_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f'))
    directory.mkdir(parents=True, exist_ok=False)
    filename = directory / 'active_item.t3d'
    task = ue.AssetExportTask()
    task.object = item
    task.exporter = ue.ObjectExporterT3D()
    task.filename = str(filename)
    task.automated = True
    task.prompt = False
    assert ue.Exporter.run_asset_export_task(task), 'Read-only active ItemInstance export failed'
    raw = filename.read_bytes()
    text = raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig')
    path = item_definition_path(text)
    definition = ue.load_class(None, path)
    math_library = ue.get_default_object(ue.load_class(None, '/Script/Engine.KismetMathLibrary'))
    assert definition and definition.get_path_name() == path
    assert math_library.class_is_child_of(definition, native('DCInventoryItemDefinition'))
    record = {'item': identity(item), 'definition_class': path, 'export': str(filename),
              'sha256': sha(filename), 'asset_saves': 0, 'read_only_export': True}
    (directory / 'definition.json').write_text(json.dumps(record, ensure_ascii=False, indent=2), encoding='utf-8')
    return definition, record


def cleanup_test_slots():
    """Remove only this helper's exact instances, still in their original test slots."""
    global _prepared
    if not _prepared:
        return {'removed': [], 'skipped': [], 'reason': 'No prepared test instances'}
    before_files = protection()
    world, pc, _, quickbar = current_context()
    assert identity(world) == _prepared['world'] and identity(pc) == _prepared['pc']
    assert identity(quickbar) == _prepared['quickbar'], 'World/controller changed; no mutation performed'
    assert pc.has_authority(), 'Authority PIE is required'
    inventory = pc.get_component_by_class(native('DCInventoryManagerComponent'))
    assert valid(inventory)
    result = {'removed': [], 'skipped': []}
    pending = []
    for record in reversed(_prepared['added']):
        slots = list(quickbar.get_slots())
        items = {item.get_path_name(): item for item in inventory.get_all_items() if valid(item)}
        item = items.get(record['item']['path'])
        index = record['index']
        if not valid(item) or identity(item) != record['item']:
            result['skipped'].append({'index': index, 'reason': 'Instance no longer exists; not recreated'})
            continue
        if index >= len(slots) or identity(slots[index]) != record['item']:
            if record['slotted']:
                result['skipped'].append({'index': index, 'reason': 'Slot changed; instance left untouched'})
                pending.append(record)
                continue
        elif quickbar.get_active_slot_index() == index:
            result['skipped'].append({'index': index, 'reason': 'Select an original weapon before cleanup'})
            pending.append(record)
            continue
        else:
            removed = quickbar.remove_item_from_slot(index)
            assert identity(removed) == record['item']
        inventory.remove_item_instance(item)
        result['removed'].append(record['item'])
    _prepared['added'] = list(reversed(pending))
    if not _prepared['added']:
        _prepared = None
    assert protection() == before_files, 'File/SaveGame protection changed during cleanup'
    return result


def prepare_test_slots(occupied_count):
    """Explicit user-only PIE fixture; duplicate current ItemDefinition into empty slots."""
    global _prepared
    assert occupied_count in (2, 3) and not isinstance(occupied_count, bool)
    assert _prepared is None, 'Clean up the previous test instances first'
    before_files = protection()
    world, pc, pawn, quickbar = current_context()
    assert pc.has_authority(), 'Authority PIE is required'
    health = ue.get_default_object(native('DCLyraHealthComponent')).find_health_component(pawn)
    assert valid(health) and not health.is_dead_or_dying(), 'Living possessed pawn is required'
    inventory = pc.get_component_by_class(native('DCInventoryManagerComponent'))
    assert valid(inventory) and inventory.get_owner() == pc and inventory.get_world() == world
    slots = list(quickbar.get_slots())
    assert len(slots) == 3, 'This fixture preserves the existing three-slot QuickBar'
    active = int(quickbar.get_active_slot_index())
    assert 0 <= active < len(slots) and valid(slots[active]), 'Equip an existing weapon first'
    existing_count = sum(valid(item) for item in slots)
    assert existing_count < occupied_count, 'Requested occupied count is already reached; nothing changed'
    definition, definition_record = read_active_definition(slots[active])
    assert definition and inventory.can_add_item_definition(definition, 1), 'Current ItemDefinition is unavailable'
    previous = original_state(quickbar, inventory)
    _prepared = {'world': identity(world), 'pc': identity(pc), 'quickbar': identity(quickbar),
                 'frame': int(ue.SystemLibrary.get_frame_count()), 'definition_read': definition_record, 'added': []}
    used_counts = {item.get_stat_tag_stack_count(tag('MagazineAmmo')) for item in slots if valid(item)}
    try:
        for index in [i for i, item in enumerate(slots) if not valid(item)][:occupied_count - existing_count]:
            assert not valid(list(quickbar.get_slots())[index]), 'Empty slot changed before insertion'
            item = inventory.add_item_definition(definition, 1)
            assert valid(item)
            assert item.get_path_name() not in previous['items'], 'Expected a distinct new ItemInstance'
            record = {'index': index, 'item': identity(item), 'slotted': False}
            _prepared['added'].append(record)  # Registered immediately, before any subsequent mutator.
            assert item.get_world() == world
            quickbar.add_item_to_slot(index, item)
            assert identity(list(quickbar.get_slots())[index]) == record['item']
            record['slotted'] = True
            magazine = item.get_stat_tag_stack_count(tag('MagazineAmmo'))
            reduction = 1
            while magazine - reduction in used_counts and reduction < magazine:
                reduction += 1
            if magazine > 0:
                item.remove_stat_tag_stack(tag('MagazineAmmo'), min(magazine, reduction))
            record['magazine'] = item.get_stat_tag_stack_count(tag('MagazineAmmo'))
            record['spare'] = item.get_stat_tag_stack_count(tag('SpareAmmo'))
            used_counts.add(record['magazine'])
            check_originals(previous, quickbar, inventory)
        assert protection() == before_files, 'File/SaveGame protection changed'
        return json.loads(json.dumps(_prepared))
    except Exception:
        try:
            cleanup_test_slots()
        except Exception:
            ue.log_error('Weapon test cleanup incomplete; retained primitive identifiers for retry: ' + traceback.format_exc())
        raise


if __name__ == '__main__':
    snapshot()
