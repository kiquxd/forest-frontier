#!/usr/bin/env python3
"""Inspect state-trait selection at minimum window size and enlarged text."""
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

with tempfile.TemporaryDirectory(prefix='frontier-state-trait-') as temporary:
    folder = Path(temporary)
    with (ui.root / 'logs/state-trait-smoke.log').open('w') as output:
        for scale in [1.0, 1.4]:
            case = folder / str(scale)
            case.mkdir()
            settings = case / 'settings.json'
            settings.write_text(json.dumps({'width': 900, 'height': 650, 'text_scale': scale}))
            process = subprocess.Popen([
                str(ui.root / 'build/forest_frontier'), '--preview', 'newgame',
                '--width', '900', '--height', '650', '--settings', str(settings),
                '--save', str(case / 'world.json'), '--log-dir', str(case / 'logs')],
                cwd=ui.root, stdout=output, stderr=output)
            try:
                if os.environ.get('SWAYSOCK') and shutil.which('swaymsg'):
                    time.sleep(.8)
                    for command in ['floating enable', 'resize set 900 650', 'focus']:
                        subprocess.run(['swaymsg', f'[pid={process.pid}] {command}'], check=True,
                                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=3)
                ui.ensure_focus(process)
                time.sleep(.3)
                for index in range(5):
                    workflow.capture(process, f'state-trait-{scale}-{index}.png')
                    ui.key(process, 'Right')
                if scale == 1.4:
                    ui.key(process, 'Next')
                    workflow.capture(process, 'state-trait-large-scrolled.png')
                else:
                    ui.key(process, 'Right')  # Balance -> Defense.
                    ui.key(process, 'Return')
                    save = case / 'New-world.json'
                    ui.wait_for(save.exists, 'New world was not created')
                    data = json.loads(save.read_text())
                    assert data['nation']['state_trait'] == 'warden'
                    assert data['version'] == 14
            finally:
                workflow.stop(process)
        print('State traits passed: all five choices, 900x650, enlarged text/scroll and saved state-wide choice.')
