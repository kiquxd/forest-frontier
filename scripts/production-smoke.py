#!/usr/bin/env python3
"""Short native production catalogue/search/requirements checks on new worlds."""
import importlib.util
import json
import os
import shutil
from pathlib import Path
import subprocess
import tempfile
import time

spec = importlib.util.spec_from_file_location('workflow', Path(__file__).with_name('menu-workflow-smoke.py'))
workflow = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow)
ui = workflow.ui

def launch(folder, save, output):
    process = subprocess.Popen([
        str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen',
        '--save', str(save), '--settings', str(folder / 'settings.json'),
        '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
    try:
        # Sway may keep a native Wayland app focused when an XWayland game opens.
        # Focus only the process owned by this test; every input still checks PID.
        if os.environ.get('SWAYSOCK') and shutil.which('swaymsg'):
            time.sleep(.8)
            subprocess.run(['swaymsg', f'[pid={process.pid}] focus'],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                           timeout=3, check=False)
        ui.ensure_focus(process)
        time.sleep(.5)
        return process
    except BaseException:
        workflow.stop(process)
        raise

def hover(process, x, y):
    ui.ensure_focus(process)
    ui.t.XTestFakeMotionEvent(ui.display, -1, int(x), int(y), 0)
    ui.x.XFlush(ui.display)
    time.sleep(.15)

def search(process, uppercase=False):
    ui.key(process, 'c')
    ui.click(process, 260, 265)
    # These letters include gameplay shortcuts P/I/C/K; typing must stay here.
    for letter in 'pickaxe':
        if uppercase:
            workflow.shortcut(process, 'Shift_L', letter)
        else:
            ui.key(process, letter)

with tempfile.TemporaryDirectory(prefix='frontier-production-smoke-') as temporary:
    folder = Path(temporary)
    with (ui.root / 'logs/production-smoke.log').open('w') as output:
        fresh = folder / 'fresh.json'
        process = launch(folder, fresh, output)
        try:
            width = ui.x.XDisplayWidth(ui.display, 0)
            ui.key(process, 'k')
            workflow.capture(process, 'production-skill-dependencies.png')
            ui.key(process, 'Escape')
            search(process)
            hover(process, 240, 386)
            workflow.capture(process, 'production-locked-tooltip.png')
            ui.click(process, width - 116, 404)  # Disabled Queue.
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            state = json.loads(fresh.read_text())
            assert not state['nation']['production'], 'Locked recipe was queued'
            assert state['skills'] == [], 'Search typed a gameplay action'
        finally:
            workflow.stop(process)
        ready = folder / 'ready.json'
        subprocess.run([str(ui.root / 'build/nation_tests'), '--fixture', str(ready)],
                       cwd=ui.root, stdout=output, stderr=output, check=True)
        before = json.loads(ready.read_text())
        process = launch(folder, ready, output)
        try:
            search(process, uppercase=True)
            hover(process, 900, 265)
            workflow.capture(process, 'production-search-ready.png')
            ui.click(process, width - 116, 404)  # Filtered Pickaxe Queue.
            ui.click(process, width - 100, 265)  # Clear.
            hover(process, 900, 265)
            workflow.capture(process, 'production-catalogue.png')
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            after = json.loads(ready.read_text())
            jobs = after['nation']['production']
            assert len(jobs) == 1 and jobs[0]['recipe'] == 'pickaxe', jobs
            home = after['home_city']
            assert after['stocks'][home][0] == before['stocks'][home][0] - 3
            assert after['stocks'][home][1] == before['stocks'][home][1] - 5
        finally:
            workflow.stop(process)
        poor = folder / 'low-resources.json'
        depleted = before
        home = depleted['home_city']
        depleted['stocks'][home][0] = 0
        poor.write_text(json.dumps(depleted))
        process = launch(folder, poor, output)
        try:
            search(process)
            hover(process, 240, 386)
            workflow.capture(process, 'production-low-resources.png')
            ui.click(process, width - 116, 404)
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            assert not json.loads(poor.read_text())['nation']['production'], 'Insufficient quantities allowed production'
        finally:
            workflow.stop(process)
    print('Production smoke passed: locked recipes, requirement tooltips, shortcut-safe search, case folding, queue and Clear.')
