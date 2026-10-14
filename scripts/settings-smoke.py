#!/usr/bin/env python3
"""Short real-window check for applying settings with the visible button."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time

spec = importlib.util.spec_from_file_location('workflow', Path(__file__).with_name('menu-workflow-smoke.py'))
workflow = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow)
ui = workflow.ui

with tempfile.TemporaryDirectory(prefix='frontier-settings-smoke-') as folder:
    folder = Path(folder)
    settings = folder / 'settings.json'
    with (ui.root / 'logs/settings-smoke.log').open('w') as output:
        process = subprocess.Popen([
            str(ui.root / 'build/forest_frontier'), '--preview', 'settings',
            '--fullscreen', '--settings', str(settings), '--save', str(folder / 'world.json'),
            '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
        try:
            ui.ensure_focus(process)
            time.sleep(.3)
            ui.key(process, 'Down')
            ui.key(process, 'Down')
            ui.key(process, 'Right')
            ui.apply_settings(process)
            ui.wait_for(settings.exists, 'Apply button did not save settings')
            assert json.loads(settings.read_text())['fps'] == 120
            ui.key(process, 'Right')
            ui.key(process, 'F5')
            assert json.loads(settings.read_text())['fps'] == 120, 'F5 still applies settings'
            ui.apply_settings(process)
            assert json.loads(settings.read_text())['fps'] == 144
            workflow.capture(process, 'settings-apply.png')
            print('Settings smoke passed: Apply button saves drafts; F5 does not apply.')
        finally:
            workflow.stop(process)
