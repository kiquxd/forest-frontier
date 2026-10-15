#!/usr/bin/env python3
"""Live production progress in menus and while the owned window is hidden."""
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

def sway(process, command):
    subprocess.run(['swaymsg', f'[pid={process.pid}] {command}'],
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                   timeout=3, check=True)

with tempfile.TemporaryDirectory(prefix='frontier-production-clock-') as temporary:
    folder = Path(temporary)
    save = folder / 'fresh.json'
    with (ui.root / 'logs/production-clock-smoke.log').open('w') as output:
        subprocess.run([str(ui.root / 'build/nation_tests'), '--production-fixture', str(save)],
                       stdout=output, stderr=output, check=True)
        initial = json.loads(save.read_text())
        process = subprocess.Popen([
            str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen',
            '--save', str(save), '--settings', str(folder / 'settings.json'),
            '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
        hidden = False
        try:
            can_hide = bool(os.environ.get('SWAYSOCK') and shutil.which('swaymsg'))
            if can_hide:
                time.sleep(.8)
                sway(process, 'focus')
            ui.ensure_focus(process)
            ui.key(process, 'c')
            workflow.capture(process, 'production-clock-live.png')
            ui.key(process, 'Escape')  # Close production.
            ui.key(process, 'Escape')  # Session pause.
            ui.key(process, 'Down')
            ui.key(process, 'Return')  # Save while still paused.
            before = json.loads(save.read_text())
            if can_hide:
                sway(process, 'move scratchpad')
                hidden = True
            # The fixture has only five seconds of real work left, not a minute-long idle test.
            time.sleep(4)
            if hidden:
                sway(process, 'scratchpad show')
                hidden = False
                sway(process, 'focus')
            ui.key(process, 'Return')  # Save again; selected action stays Save.
            after = json.loads(save.read_text())
            assert after['nation']['production_time'] >= before['nation']['production_time'] + 3.5
            assert after['game_time'] == before['game_time'], 'Paused world advanced with production'
            assert len(after['nation']['production']) == 1, 'Background runtime did not finish the queue head exactly once'
            home = after['home_city']
            assert after['nation']['city_products'][home].get('campfire', 0) == initial['nation']['city_products'][home].get('campfire', 0) + 1
            ui.key(process, 'Escape')
            ui.key(process, 'c')
            workflow.capture(process, 'production-clock-completed.png')
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            print('Production clock passed: live progress, paused simulation, background completion, FIFO and save.')
        finally:
            if hidden:
                sway(process, 'scratchpad show')
            workflow.stop(process)
