#!/usr/bin/env python3
"""Quick real-window journal/save check; --extended exercises full workflows."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import time
spec=importlib.util.spec_from_file_location('workflow',Path(__file__).with_name('menu-workflow-smoke.py'))
workflow=importlib.util.module_from_spec(spec);spec.loader.exec_module(workflow)
ui=workflow.ui
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--extended', action='store_true', help='Also exercise note reload/deletion, overlays and audio settings')
args = parser.parse_args()
if not args.extended:
    with tempfile.TemporaryDirectory(prefix='frontier-world-quick-') as folder:
        folder = Path(folder)
        save = folder / 'world.json'
        with (ui.root / 'logs/world-smoke.log').open('w') as output:
            process = subprocess.Popen([
                str(ui.root / 'build/forest_frontier'), '--play', '--fullscreen',
                '--settings', str(folder / 'settings.json'), '--save', str(save),
                '--log-dir', str(folder / 'logs')], cwd=ui.root, stdout=output, stderr=output)
            try:
                ui.ensure_focus(process)
                time.sleep(.8)
                width = ui.x.XDisplayWidth(ui.display, 0)
                height = ui.x.XDisplayHeight(ui.display, 0)
                ui.key(process, 'm')
                ui.click(process, width * .5, height * .5)
                workflow.capture(process, 'world-journal-quick.png')
                workflow.shortcut(process, 'Control_L', 'Return')
                ui.key(process, 'Escape')
                ui.key(process, 'Escape')
                ui.quit_to_menu(process)
                ui.wait_for(save.exists, 'World was not saved')
                assert len(json.loads(save.read_text())['map_notes']) == 1, 'Note was not saved'
                for _ in range(4):
                    ui.key(process, 'Down')
                ui.key(process, 'Return')
                assert process.wait(timeout=5) == 0
                print('Quick world smoke passed: map click, journal save and return to main menu.')
            finally:
                workflow.stop(process)
    raise SystemExit(0)
with tempfile.TemporaryDirectory(prefix='frontier-world-smoke-') as folder:
    folder=Path(folder);settings=folder/'settings.json';save=folder/'saves/world.json'
    with (ui.root/'logs/world-smoke.log').open('w') as output:
        process=subprocess.Popen([str(ui.root/'build/forest_frontier'),'--fullscreen','--settings',str(settings),'--save',str(save),'--log-dir',str(folder/'logs')],cwd=ui.root,stdout=output,stderr=output)
        try:
            ui.ensure_focus(process);time.sleep(.5)
            width=ui.x.XDisplayWidth(ui.display,0);height=ui.x.XDisplayHeight(ui.display,0)
            ui.key(process,'Down');ui.key(process,'Down');ui.key(process,'Return');ui.key(process,'Return')
            world=save.parent/'New-world.json';ui.wait_for(world.exists,'New world missing');time.sleep(1)
            ui.key(process,'m');ui.click(process,width*.5,height*.5)
            workflow.text(process,'Home entry');ui.key(process,'Tab');workflow.text(process,'The nearby town has a forge. Come back with materials.')
            workflow.capture(process,'world-journal-editor.png')
            workflow.shortcut(process,'Control_L','Return');ui.key(process,'Escape')
            workflow.capture(process,'world-map-note.png')
            ui.key(process,'2');workflow.capture(process,'world-map-politics.png')
            ui.key(process,'3');workflow.capture(process,'world-map-economy.png')
            ui.key(process,'Escape');ui.quit_to_menu(process)
            saved=json.loads(world.read_text());assert len(saved['map_notes'])==1,saved['map_notes']
            assert saved['map_notes'][0]['title']=='Home entry'
            assert saved['map_notes'][0]['text'].startswith('The nearby town'),saved['map_notes']
            ui.key(process,'Return');time.sleep(.8);ui.key(process,'m');ui.click(process,width*.5,height*.5)
            workflow.capture(process,'world-journal-reloaded.png')
            ui.click(process,370,height-120)  # Delete note.
            ui.key(process,'Escape');ui.key(process,'Escape');ui.quit_to_menu(process)
            assert json.loads(world.read_text())['map_notes']==[],'Deleted note remained in save'
            for _ in range(3):ui.key(process,'Down')
            ui.key(process,'Return')
            for _ in range(8):ui.key(process,'Down')
            for _ in range(4):ui.key(process,'Return')  # Automatic -> Off.
            ui.key(process,'Down');ui.key(process,'Down')
            for _ in range(7):ui.key(process,'Left')  # Disable effects separately.
            ui.key(process,'F5')
            config=json.loads(settings.read_text());assert config['music_mode']==4 and config['effects_volume']==0,config
            workflow.capture(process,'world-audio-settings.png')
            ui.key(process,'Escape');ui.key(process,'Down');ui.key(process,'Return')
            assert process.wait(timeout=10)==0
            print('World smoke passed: map clicks, note editing/reload/deletion, overlays and independent audio disable.')
        finally:workflow.stop(process)
