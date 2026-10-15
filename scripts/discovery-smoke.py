#!/usr/bin/env python3
"""Verify town discovery, modal input priority and no repeat after reload."""
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


def launch(save, folder, output):
    process = subprocess.Popen([
        str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen',
        '--save', str(save), '--settings', str(folder / 'settings.json'),
        '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
    try:
        if os.environ.get('SWAYSOCK') and shutil.which('swaymsg'):
            time.sleep(.8)
            subprocess.run(['swaymsg', f'[pid={process.pid}] focus'], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=3)
        ui.ensure_focus(process)
        time.sleep(.5)
        return process
    except BaseException:
        workflow.stop(process)
        raise



with tempfile.TemporaryDirectory(prefix='frontier-discovery-') as temporary:
    folder = Path(temporary)
    save = folder / 'fresh.json'
    with (ui.root / 'logs/discovery-smoke.log').open('w') as output:
        subprocess.run([str(ui.root / 'build/nation_tests'), '--discovery-fixture', str(save)],
                       stdout=output, stderr=output, check=True)
        initial = json.loads(save.read_text())
        target = next(i for i, r in enumerate(initial['relations']) if not r['discovered'])
        process = launch(save, folder, output)
        try:
            # These shortcuts must not replace or dismiss the discovery modal.
            for key in ['n', 'c', 'k']:
                ui.key(process, key)
            workflow.capture(process, 'town-discovery.png')
            ui.key(process, 'Escape')  # Dismiss only discovery, without opening pause.
            ui.key(process, 'Escape')  # Now open pause.
            ui.key(process, 'Down')
            ui.key(process, 'Return')
            saved = json.loads(save.read_text())
            assert saved['relations'][target]['discovered'], 'Discovery or Escape priority failed'
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
        finally:
            workflow.stop(process)
        process = launch(save, folder, output)
        try:
            # A repeated card would consume this Escape, preventing save below.
            ui.key(process, 'Escape')
            ui.key(process, 'Down')
            ui.key(process, 'Return')
            reloaded = json.loads(save.read_text())
            assert reloaded['nation']['production_time'] > saved['nation']['production_time']
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
        finally:
            workflow.stop(process)
        events = [json.loads(line) for path in (folder / 'logs').glob('perf-*.jsonl')
                  for line in path.read_text().splitlines()]
        assert sum(e.get('event') == 'town_discovered' and e.get('city') == target for e in events) == 1
        assert sum(e.get('event') == 'discovery_dismissed' and e.get('city') == target for e in events) == 1
        print('Discovery passed: summary, blocked shortcuts, Escape priority, save and no repeat after reload.')
