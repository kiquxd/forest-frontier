#!/usr/bin/env python3
"""Real-window regressions for file workflows, remapping and resident management."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time

spec = importlib.util.spec_from_file_location('menu_smoke', Path(__file__).with_name('menu-smoke.py'))
ui = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ui)


def shortcut(process, modifier, key):
    ui.ensure_focus(process)
    code = ui.x.XKeysymToKeycode(ui.display, ui.x.XStringToKeysym(modifier.encode()))
    ui.t.XTestFakeKeyEvent(ui.display, code, 1, 0)
    ui.x.XFlush(ui.display)
    ui.key(process, key)
    ui.t.XTestFakeKeyEvent(ui.display, code, 0, 0)
    ui.x.XFlush(ui.display)
    time.sleep(.1)


def text(process, value):
    shortcut(process, 'Control_L', 'a')
    names = {' ': 'space', '/': 'slash', '.': 'period', '-': 'minus', '_': 'underscore'}
    for char in value:
        if char.isupper() or char == '_':
            shortcut(process, 'Shift_L', names.get(char, char.lower()))
        else:
            ui.key(process, names.get(char, char))


def capture(process, name):
    ui.ensure_focus(process)
    window = ui.c.c_ulong()
    revert = ui.c.c_int()
    ui.x.XGetInputFocus(ui.display, ui.c.byref(window), ui.c.byref(revert))
    ui.x.XGetGeometry.argtypes = [ui.c.c_void_p, ui.c.c_ulong, ui.c.POINTER(ui.c.c_ulong), ui.c.POINTER(ui.c.c_int), ui.c.POINTER(ui.c.c_int), ui.c.POINTER(ui.c.c_uint), ui.c.POINTER(ui.c.c_uint), ui.c.POINTER(ui.c.c_uint), ui.c.POINTER(ui.c.c_uint)]
    parent = ui.c.c_ulong()
    left, top = ui.c.c_int(), ui.c.c_int()
    width, height, border, depth = (ui.c.c_uint() for _ in range(4))
    ui.x.XGetGeometry(ui.display, window.value, ui.c.byref(parent), ui.c.byref(left), ui.c.byref(top), ui.c.byref(width), ui.c.byref(height), ui.c.byref(border), ui.c.byref(depth))
    ui.x.XTranslateCoordinates.argtypes = [ui.c.c_void_p, ui.c.c_ulong, ui.c.c_ulong, ui.c.c_int, ui.c.c_int, ui.c.POINTER(ui.c.c_int), ui.c.POINTER(ui.c.c_int), ui.c.POINTER(ui.c.c_ulong)]
    child = ui.c.c_ulong()
    ui.x.XTranslateCoordinates(ui.display, window.value, parent.value, 0, 0, ui.c.byref(left), ui.c.byref(top), ui.c.byref(child))
    # Capture the compositor's current pixels, restricted to the verified game window.
    subprocess.run(['grim', '-g', f'{left.value},{top.value} {width.value}x{height.value}', str(ui.root / 'logs' / name)], check=True, timeout=10)


def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='frontier-workflow-') as folder:
        folder = Path(folder)
        settings = folder / 'settings.json'
        saves = folder / 'saves'
        log = ui.root / 'logs' / 'menu-workflow-smoke.log'
        with log.open('w') as output:
            process = subprocess.Popen([str(ui.root / 'build/forest_frontier'), '--fullscreen', '--save', str(saves / 'world.json'), '--settings', str(settings), '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
            try:
                ui.ensure_focus(process)
                time.sleep(.5)
                width = ui.x.XDisplayWidth(ui.display, 0)
                height = ui.x.XDisplayHeight(ui.display, 0)
                # Controls panel supports conflict detection, explicit swap, persistence and reset.
                for _ in range(3):
                    ui.key(process, 'Down')
                ui.key(process, 'Return')
                for _ in range(7):
                    ui.key(process, 'Down')
                ui.key(process, 'Return')
                for _ in range(5):
                    ui.key(process, 'Down')  # Talk primary.
                ui.key(process, 'Return')
                ui.key(process, 'p')  # Conflict with hall.
                capture(process, 'menu-binding-conflict.png')
                ui.key(process, 'Return')
                ui.key(process, 'F5')
                ui.wait_for(settings.exists, 'Rebinding settings were not written')
                bindings = json.loads(settings.read_text())['bindings']
                print('Control conflict/swap persisted', flush=True)
                assert bindings['talk'][0] == 80 and bindings['hall'][0] == 69, 'Conflict swap did not update both actions'
                capture(process, 'menu-controls.png')
                ui.click(process, width - 330, height - 43)  # Reset controls.
                ui.key(process, 'F5')
                assert json.loads(settings.read_text())['bindings']['talk'][0] == 69
                ui.key(process, 'Escape')
                ui.key(process, 'Escape')
                ui.key(process, 'Up')
                ui.key(process, 'Return')
                ui.key(process, 'Return')  # New world.
                world = saves / 'New-world.json'
                ui.wait_for(world.exists, 'World not created')
                time.sleep(1)
                capture(process, 'menu-context-footer.png')
                ui.key(process, 'k')
                ui.key(process, 'Return')
                capture(process, 'menu-live-tree.png')
                ui.key(process, 'Escape')
                ui.quit_to_menu(process)
                assert json.loads(world.read_text())['skills'] == ['gathering']
                ui.key(process, 'Down')
                ui.key(process, 'Return')  # Saved worlds.
                original = world.read_bytes()
                ui.key(process, 'F2')
                for _ in range(4):
                    ui.key(process, 'Right')
                ui.key(process, 'Return')  # Read-only tree must neither learn nor load.
                assert world.read_bytes() == original
                capture(process, 'menu-saved-tree.png')
                ui.key(process, 'Escape')
                ui.key(process, 'F7')
                text(process, 'copy')
                ui.key(process, 'Return')
                print('Read-only preview verified', flush=True)
                copied = saves / 'copy.json'
                ui.wait_for(copied.exists, 'UI copy did not create a slot')
                ui.key(process, 'F6')
                text(process, 'renamed')
                ui.key(process, 'Return')
                renamed = saves / 'renamed.json'
                ui.wait_for(renamed.exists, 'UI rename failed')
                assert not copied.exists()
                ui.key(process, 'F8')
                exported = folder / 'export.json'
                text(process, str(exported))
                ui.key(process, 'Return')
                ui.wait_for(exported.exists, 'UI export failed')
                ui.key(process, 'F9')
                text(process, str(exported))
                ui.key(process, 'Tab')
                text(process, 'imported')
                ui.key(process, 'Return')
                imported = saves / 'imported.json'
                ui.wait_for(imported.exists, 'UI import failed')
                print('Copy, rename, export and import verified', flush=True)
                assert json.loads(imported.read_text())['skills'] == ['gathering']
                ui.key(process, 'Delete')
                ui.key(process, 'Return')
                ui.wait_for(lambda: not imported.exists(), 'UI delete failed')
                ui.key(process, 'F10')
                capture(process, 'menu-recovery.png')
                ui.key(process, 'Return')
                text(process, 'restored')
                ui.key(process, 'Return')
                restored = saves / 'restored.json'
                ui.wait_for(restored.exists, 'UI restore failed')
                assert json.loads(restored.read_text())['skills'] == ['gathering']
                ui.key(process, 'Escape')
                ui.click(process, 180, 120 + 4 * 62 + 25)
                assert process.wait(timeout=10) == 0

                # A ready owned-town fixture verifies remote role/gear allocation through the dashboard.
                fixture = json.loads(restored.read_text())
                fixture['technologies'] = list(set(fixture['technologies']) | {'apprenticeship'})
                fixture['outfit_copies']['forager'] = 1
                resident = min(fixture['citizens'], key=lambda item: item['npc'])['npc']
                dashboard = folder / 'dashboard.json'
                dashboard.write_text(json.dumps(fixture))
                process = subprocess.Popen([str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen', '--save', str(dashboard), '--settings', str(settings), '--log-dir', str(folder / 'dashboard-logs')], cwd=ui.root, stdout=output, stderr=output)
                ui.ensure_focus(process)
                time.sleep(.8)
                ui.key(process, 'k')
                ui.key(process, 'Left')  # Settlements.
                capture(process, 'menu-settlements.png')
                # Live Summary bounds: x=44, body y=152; town list is 220 px wide.
                ui.click(process, 450, 474)
                ui.key(process, '3')  # Idle -> gatherer.
                ui.key(process, '5')  # Empty -> forager (content order).
                ui.key(process, 'Return')
                capture(process, 'menu-resident-assignment.png')
                ui.key(process, 'Escape')
                ui.key(process, 'Escape')
                for _ in range(3):
                    ui.key(process, 'Down')
                ui.key(process, 'Return')  # The same Enter must not toggle fullscreen.
                ui.key(process, 'F5')
                assert json.loads(settings.read_text())['fullscreen'], 'Opening pause settings changed fullscreen'
                capture(process, 'menu-pause-settings.png')
                ui.key(process, 'Escape')
                ui.key(process, 'Up')
                ui.key(process, 'Up')
                ui.key(process, 'Return')
                ui.wait_for(dashboard.exists, 'Dashboard Save missing')
                assigned = next(item for item in json.loads(dashboard.read_text())['citizens'] if item['npc'] == resident)
                assert assigned['job'] == 'gatherer' and assigned['outfit'] == 'forager', assigned
                ui.key(process, 'Down')
                ui.key(process, 'Return')  # Return to main menu.
                ui.click(process, 180, 120 + 4 * 62 + 25)
                assert process.wait(timeout=10) == 0
                runtime_settings = json.loads(settings.read_text())
                runtime_settings['bindings']['up'][0] = 90  # Z; Up alternate remains.
                settings.write_text(json.dumps(runtime_settings))
                before = json.loads(dashboard.read_text())['player']
                process = subprocess.Popen([str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen', '--save', str(dashboard), '--settings', str(settings), '--log-dir', str(folder / 'remapped-logs')], cwd=ui.root, stdout=output, stderr=output)
                ui.ensure_focus(process)
                time.sleep(.8)
                ui.hold(process, 'z', .4)
                ui.quit_to_menu(process)
                after = json.loads(dashboard.read_text())['player']
                assert before[0] - after[0] > 10 and before[1] - after[1] > 10, 'Remapped movement did not reach runtime'
                ui.click(process, 180, 120 + 4 * 62 + 25)
                assert process.wait(timeout=10) == 0
                print('Workflow smoke passed: controls swap/reset, read-only tree, copy/rename/export/import/recovery and owned resident assignment')
            finally:
                stop(process)
