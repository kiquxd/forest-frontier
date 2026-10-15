#!/usr/bin/env python3
"""Focused native smoke: fresh resident control, warehouse and production UI."""
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

def manual_save(process):
    ui.key(process, 'Escape')
    ui.key(process, 'Down')
    ui.key(process, 'Return')

def launch(folder, save, output):
    return subprocess.Popen([
        str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen',
        '--save', str(save), '--settings', str(folder / 'settings.json'),
        '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)

with tempfile.TemporaryDirectory(prefix='frontier-nation-smoke-') as temporary:
    folder = Path(temporary)
    save = folder / 'fresh.json'
    with (ui.root / 'logs/nation-smoke.log').open('w') as output:
        process = launch(folder, save, output)
        try:
            ui.ensure_focus(process)
            time.sleep(.8)
            width = ui.x.XDisplayWidth(ui.display, 0)
            ui.key(process, 'k')
            ui.key(process, 'Return')  # Free state-wide Gathering lesson.
            workflow.capture(process, 'nation-knowledge.png')
            ui.key(process, 'Escape')
            ui.key(process, 'u')
            workflow.capture(process, 'nation-residents.png')
            ui.key(process, 'Escape')
            manual_save(process)
            initial = json.loads(save.read_text())
            assert initial['skills'] == ['gathering'], 'State tree lesson did not persist'
            first = initial['nation']['controlled']
            ui.key(process, 'Escape')
            ui.key(process, 'u')
            ui.click(process, width - 250, 348)  # Control the second resident.
            ui.hold(process, 'd', .15)
            manual_save(process)
            switched = json.loads(save.read_text())
            assert switched['nation']['controlled'] != first, 'Resident selection did not switch control'
            before = {c['npc']: c['position'] for c in initial['nation']['residents']}
            after = {c['npc']: c['position'] for c in switched['nation']['residents']}
            assert after[first] == before[first], 'Switching teleported the former resident'
            ui.key(process, 'Escape')
            ui.key(process, 'y')
            ui.click(process, width - 116, 348)  # Withdraw one wood.
            workflow.capture(process, 'nation-warehouse.png')
            ui.key(process, 'Escape')
            manual_save(process)
            stored = json.loads(save.read_text())
            assert stored['wood'] == 1, 'Warehouse withdrawal did not reach selected resident cargo'
            ui.key(process, 'Escape')
            ui.key(process, 'n')
            workflow.capture(process, 'nation-state.png')
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
        finally:
            workflow.stop(process)
        fixture = folder / 'fresh-production.json'
        subprocess.run([str(ui.root / 'build/nation_tests'), '--fixture', str(fixture)], cwd=ui.root, stdout=output, stderr=output, check=True)
        process = launch(folder, fixture, output)
        try:
            ui.ensure_focus(process)
            time.sleep(.5)
            ui.key(process, 'c')
            workflow.capture(process, 'nation-production.png')
            ui.key(process, 'Escape')
            ui.key(process, 'l')
            ui.click(process, width - 116, 276)  # Accept the home survey.
            workflow.capture(process, 'nation-projects.png')
            ui.key(process, 'Escape')
            ui.key(process, 'n')
            tab_width = (width - 100) / 7
            ui.click(process, 50 + 6.5 * tab_width, 213)
            workflow.capture(process, 'nation-growth.png')
            ui.key(process, 'Escape')
            ui.quit_to_menu(process)
            assert json.loads(fixture.read_text())['nation']['enabled']
            print('Nation smoke passed: fresh resident switching, cargo transfer, production, town boards and state management.')
        finally:
            workflow.stop(process)
