#!/usr/bin/env python3
"""Keyboard-only UI flow with a fresh starter fixture and isolated settings."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

spec = importlib.util.spec_from_file_location('workflow', Path(__file__).with_name('menu-workflow-smoke.py'))
workflow = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow)
ui = workflow.ui

with tempfile.TemporaryDirectory(prefix='frontier-keyboard-') as temporary:
    folder = Path(temporary)
    save = folder / 'fresh.json'
    settings = folder / 'settings.json'
    settings.write_text(json.dumps({'width': 900, 'height': 650}))
    with (ui.root / 'logs/keyboard-smoke.log').open('w') as output:
        subprocess.run([str(ui.root / 'build/nation_tests'), '--starter-fixture', str(save)],
                       stdout=output, stderr=output, check=True)
        process = subprocess.Popen([
            str(ui.root / 'build/forest_frontier'), '--play', '--save', str(save),
            '--width', '900', '--height', '650', '--settings', str(settings),
            '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
        try:
            if os.environ.get('SWAYSOCK') and shutil.which('swaymsg'):
                time.sleep(.8)
                for command in ['floating enable', 'resize set 900 650', 'focus']:
                    subprocess.run(['swaymsg', f'[pid={process.pid}] {command}'], check=True,
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=3)
            ui.ensure_focus(process)
            ui.key(process, 'k')
            for key in ['Down', 'Right', 'Right', 'Return']:
                ui.key(process, key)
            workflow.capture(process, 'keyboard-tools.png')
            ui.key(process, 'Escape')
            ui.key(process, 'y')
            workflow.capture(process, 'keyboard-resources.png')
            ui.key(process, 'Right')
            workflow.capture(process, 'keyboard-production.png')
            # Common controls: Close, two city arrows, transfer, seven tabs, search.
            for _ in range(12):
                ui.key(process, 'Tab')
            ui.key(process, 'Return')
            for key in ['a', 'x', 'e', 'Return']:
                ui.key(process, key)
            workflow.capture(process, 'keyboard-axe.png')
            for _ in range(3):
                workflow.shortcut(process, 'Shift_L', 'Tab')
            ui.key(process, 'Return')  # Queue the first axe.
            ui.key(process, 'Escape')
            ui.key(process, 'Escape')
            for key in ['Down', 'Down', 'Down', 'Return']:
                ui.key(process, key)
            ui.key(process, 'Tab')  # Visible Music toggle.
            ui.key(process, 'Return')
            ui.key(process, 'Right')  # Apply.
            ui.key(process, 'Return')
            assert json.loads(settings.read_text())['music_mode'] == 4, 'Music Off was not applied from the world'
            workflow.capture(process, 'keyboard-music-off.png')
            ui.key(process, 'Escape')
            ui.key(process, 'Escape')
            ui.key(process, 'm')
            ui.key(process, 'Tab')
            ui.key(process, 'Tab')
            ui.key(process, 'Return')  # Road planner, without a mouse.
            ui.key(process, 'Tab')
            ui.key(process, 'Return')  # First waypoint at our town.
            workflow.capture(process, 'keyboard-road-plan.png')
            ui.key(process, 'BackSpace')
            ui.key(process, 'Escape')  # Cancel the plan; keep the map open.
            ui.key(process, 'Tab')
            ui.key(process, 'Return')  # Note at the explored map centre.
            workflow.text(process, 'Keyboard note')
            workflow.shortcut(process, 'Control_L', 'Return')
            workflow.capture(process, 'keyboard-journal.png')
            for key in ['Escape', 'Escape', 'Escape', 'Down', 'Return']:
                ui.key(process, key)
            data = json.loads(save.read_text())
            assert 'tools' in data['skills'], 'Arrow tree navigation did not learn Tools'
            assert any(j['recipe'] == 'axe' for j in data['nation']['production']), 'Keyboard Queue failed'
            assert any(n['title'] == 'Keyboard note' for n in data['map_notes']), 'Keyboard map note failed'
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            ui.key(process, 'Tab')
            ui.key(process, 'Down')
            ui.key(process, 'Return')
            workflow.capture(process, 'keyboard-saved-worlds.png')
            print('Keyboard flow passed: arrows/Tools, Resources/Production, search/Queue, music Off/Apply, map note, save and lobby focus.')
        finally:
            workflow.stop(process)
