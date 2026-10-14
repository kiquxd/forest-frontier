#!/usr/bin/env python3
"""Exercise the raylib menu with keyboard input and isolated saves/settings."""
import ctypes as c
import json
import subprocess
import tempfile
import time
from pathlib import Path

root = Path(__file__).resolve().parent.parent
x = c.CDLL("libX11.so.6")
t = c.CDLL("libXtst.so.6")
x.XOpenDisplay.argtypes = [c.c_char_p]; x.XOpenDisplay.restype = c.c_void_p
x.XGetInputFocus.argtypes = [c.c_void_p, c.POINTER(c.c_ulong), c.POINTER(c.c_int)]
x.XInternAtom.argtypes = [c.c_void_p, c.c_char_p, c.c_int]; x.XInternAtom.restype = c.c_ulong
x.XGetWindowProperty.argtypes = [c.c_void_p,c.c_ulong,c.c_ulong,c.c_long,c.c_long,c.c_int,c.c_ulong,c.POINTER(c.c_ulong),c.POINTER(c.c_int),c.POINTER(c.c_ulong),c.POINTER(c.c_ulong),c.POINTER(c.POINTER(c.c_ubyte))]
x.XFree.argtypes = [c.c_void_p]
x.XStringToKeysym.argtypes = [c.c_char_p]; x.XStringToKeysym.restype = c.c_ulong
x.XKeysymToKeycode.argtypes = [c.c_void_p,c.c_ulong]; x.XKeysymToKeycode.restype = c.c_uint
x.XFlush.argtypes = [c.c_void_p]
x.XDisplayWidth.argtypes = [c.c_void_p,c.c_int];x.XDisplayWidth.restype=c.c_int
x.XDisplayHeight.argtypes = [c.c_void_p,c.c_int];x.XDisplayHeight.restype=c.c_int
t.XTestFakeKeyEvent.argtypes = [c.c_void_p,c.c_uint,c.c_int,c.c_ulong]
t.XTestFakeMotionEvent.argtypes = [c.c_void_p,c.c_int,c.c_int,c.c_int,c.c_ulong]
t.XTestFakeButtonEvent.argtypes = [c.c_void_p,c.c_uint,c.c_int,c.c_ulong]
display=x.XOpenDisplay(None)
if not display:
    raise SystemExit("X11/XWayland display required")

def wait_for(predicate, reason, timeout=10):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        if predicate(): return
        time.sleep(.1)
    raise RuntimeError(reason)

def focused_pid():
    window=c.c_ulong(); revert=c.c_int(); x.XGetInputFocus(display,c.byref(window),c.byref(revert))
    if window.value<=1: return None
    atom=x.XInternAtom(display,b"_NET_WM_PID",0)
    kind=c.c_ulong(); bits=c.c_int(); count=c.c_ulong(); remaining=c.c_ulong(); data=c.POINTER(c.c_ubyte)()
    status=x.XGetWindowProperty(display,window,atom,0,1,0,0,c.byref(kind),c.byref(bits),c.byref(count),c.byref(remaining),c.byref(data))
    if status or not count.value: return None
    value=c.cast(data,c.POINTER(c.c_ulong))[0]; x.XFree(data); return value

def ensure_focus(process):
    if process.poll() is not None: raise RuntimeError("Game exited unexpectedly")
    wait_for(lambda: focused_pid()==process.pid,"Test window did not receive focus")

def key(process, name):
    ensure_focus(process)
    code=x.XKeysymToKeycode(display,x.XStringToKeysym(name.encode()))
    t.XTestFakeKeyEvent(display,code,1,0);x.XFlush(display);time.sleep(.06)
    t.XTestFakeKeyEvent(display,code,0,0);x.XFlush(display);time.sleep(.15)

def quit_to_menu(process):
    key(process,"Escape") # Open pause menu.
    assert process.poll() is None,"Escape closed the game"
    key(process,"Down");key(process,"Return") # Save and quit to main menu.

def click(process,px,py):
    ensure_focus(process)
    t.XTestFakeMotionEvent(display,-1,int(px),int(py),0);x.XFlush(display);time.sleep(.1)
    t.XTestFakeButtonEvent(display,1,1,0);x.XFlush(display);time.sleep(.06)
    t.XTestFakeButtonEvent(display,1,0,0);x.XFlush(display);time.sleep(.2)

def hold(process,name,seconds=.3):
    ensure_focus(process)
    code=x.XKeysymToKeycode(display,x.XStringToKeysym(name.encode()))
    t.XTestFakeKeyEvent(display,code,1,0);x.XFlush(display);time.sleep(seconds)
    t.XTestFakeKeyEvent(display,code,0,0);x.XFlush(display);time.sleep(.15)

with tempfile.TemporaryDirectory(prefix="frontier-menu-") as folder:
    folder=Path(folder); save=folder/"saves"/"world.json"; settings=folder/"settings.json"
    log=root/"logs"/"v5-menu-smoke.log"
    with log.open("w") as output:
        process=subprocess.Popen([str(root/"build/forest_frontier"),"--fullscreen","--save",str(save),"--settings",str(settings),"--log-dir",str(folder/"logs")],cwd=root,stdout=output,stderr=output)
        try:
            ensure_focus(process);time.sleep(.5)
            key(process,"Escape")
            assert process.poll() is None,"Escape closed the main menu"
            for _ in range(3): key(process,"Down")
            key(process,"Return") # Settings.
            for _ in range(2): key(process,"Down")
            key(process,"Right") # Change the frame limit.
            key(process,"F5") # Apply.
            wait_for(settings.exists,"Settings not saved")
            assert json.loads(settings.read_text())["fps"] == 120
            key(process,"Escape")
            key(process,"Up");key(process,"Return") # New game.
            key(process,"Return") # Create and play.
            created=save.parent/"New-world.json"
            wait_for(created.exists,"New game did not create a separate slot")
            time.sleep(1.5)
            key(process,"e") # E must converse even beside the hall, not learn Gathering.
            events=[json.loads(line) for path in (folder/'logs').glob('perf-*.jsonl') for line in path.read_text().splitlines() if line.strip()]
            assert any(row.get('event')=='conversation_started' for row in events),"E beside the hall did not start NPC conversation"
            key(process,"Return");key(process,"Escape")
            quit_to_menu(process)
            assert json.loads(created.read_text())['skills']==[],"E beside the hall opened lessons instead of NPC interaction"
            key(process,"Return");time.sleep(.7)
            key(process,"p") # Fresh campaign's hall selects Gathering.
            # Use the real action button to cover the original mouse-driven report.
            width=x.XDisplayWidth(display,0);height=x.XDisplayHeight(display,0)
            rows=min(8,max(1,int((height-40-102-155)/50)))
            box_width=min(960,width-40);box_height=rows*50+102+155
            button_x=(width-box_width)/2+box_width-274+125
            button_y=(height-box_height)/2+102+rows*50+128
            ensure_focus(process)
            t.XTestFakeMotionEvent(display,-1,int(button_x),int(button_y),0);x.XFlush(display);time.sleep(.1)
            t.XTestFakeButtonEvent(display,1,1,0);x.XFlush(display);time.sleep(.06)
            t.XTestFakeButtonEvent(display,1,0,0);x.XFlush(display);time.sleep(.2)
            key(process,"Escape") # Close the hall.
            key(process,"Escape");key(process,"Down") # Pause → Quit.
            # Force a save failure in the isolated fixture; session must stay open.
            original=created.read_bytes();held=created.with_suffix('.held')
            created.rename(held);created.mkdir()
            try:
                key(process,"Return")
                assert process.poll() is None,"Failed save closed the game"
                assert held.read_bytes()==original,"Failed save damaged the previous world"
                assert created.is_dir(),"Failed save unexpectedly replaced its blocked destination"
            finally:
                created.rmdir();held.rename(created)
            key(process,"Return") # Retry saves successfully and returns to main menu.
            wait_for(lambda: json.loads(created.read_text())['skills']==['gathering'],"Clicking the Gathering action did not update progression")
            saved_position=json.loads(created.read_text())['player']
            key(process,"Return") # Continue the same world.
            time.sleep(1)
            key(process,"Escape");key(process,"Right") # Paused world map.
            click(process,(width-min(1240,width-48))*.5+174,160) # Preview the seafloor without diving.
            key(process,"d");key(process,"Escape") # Movement must do nothing while paused; ESC resumes.
            key(process,"Escape")
            for _ in range(3): key(process,"Right") # World → inventory → skills → tree.
            key(process,"Next");key(process,"Prior") # Tree scrolls without moving the player.
            key(process,"Left");key(process,"Next");key(process,"Prior") # Vertical skills list.
            key(process,"Escape")
            key(process,"Escape");key(process,"Return") # Resume button also works.
            key(process,"p");key(process,"Next");key(process,"Down");key(process,"Down") # Town actions → learned Gathering.
            key(process,"Return") # Learned lesson must stay complete, without duplicate progress.
            key(process,"Escape") # Close the hall.
            quit_to_menu(process)
            wait_for(lambda: bool(json.loads(created.read_text())["explored"]),"Gameplay did not save explored map")
            progress=json.loads(created.read_text())
            assert progress['skills']==['gathering'],"Town-hall lesson did not update/persist exactly once"
            assert progress['player']==saved_position,"Movement continued while paused"
            assert not progress['submerged'],"Previewing the seafloor changed the actual player's world layer"
            time.sleep(.4);key(process,"Down");key(process,"Return") # Saved worlds.
            for _ in range(5): key(process,"Right") # Visit every summary tab.
            key(process,"Delete");key(process,"Escape") # Cancel deletion.
            assert created.exists(),"Cancel deleted the slot"
            key(process,"Delete");key(process,"Return") # Confirm deletion.
            wait_for(lambda:not created.exists(),"Confirmed deletion did not remove the slot")
            assert len(list((save.parent/".trash").glob("*.json")))==1
            key(process,"Escape");key(process,"Escape")
            assert process.poll() is None,"Escape closed the app instead of remaining in the main menu"
            for _ in range(3): key(process,"Down") # Saved worlds selection → Quit.
            key(process,"Return")
            assert process.wait(timeout=10)==0
            # Direct --play must also return to the lobby; both Quit buttons work by mouse.
            direct=folder/'direct'/'world.json'
            process=subprocess.Popen([str(root/'build/forest_frontier'),'--play','--fullscreen','--save',str(direct),'--settings',str(settings),'--log-dir',str(folder/'direct-logs'),'--screenshot',str(root/'logs/summary-tree.png')],cwd=root,stdout=output,stderr=output)
            ensure_focus(process);time.sleep(1.5)
            key(process,'Escape')
            for _ in range(4): key(process,'Right') # Inspect the dependency graph on pause.
            panel_width=min(1240,width-48);button_width=min(330,(panel_width-60)*.5)
            click(process,(width-panel_width)*.5+665,543) # Militia cell: actual requirements shown below.
            click(process,(width-panel_width)*.5+panel_width-button_width*.5-20,height-73)
            wait_for(direct.exists,'Direct session Quit did not save its world')
            assert process.poll() is None,'Direct session Quit closed the app instead of returning to menu'
            key(process,'Escape')
            assert process.poll() is None,'Escape closed the main menu after direct play'
            for movement,sx,sy in [('d',1,-1),('a',-1,1),('w',-1,-1),('s',1,1),('Right',1,-1),('Left',-1,1)]:
                before=json.loads(direct.read_text())['player']
                key(process,'Return');time.sleep(.7) # Continue saved session.
                if movement=='d':
                    key(process,'p');key(process,'Escape') # New town-hall binding opens/closes its panel.
                hold(process,movement)
                quit_to_menu(process)
                after=json.loads(direct.read_text())['player']
                assert sx*(after[0]-before[0])>10 and sy*(after[1]-before[1])>10,f'{movement} did not move in the expected direction (or opened a conflicting action)'
            click(process,180,120+4*62+25) # Explicit main-menu Quit.
            assert process.wait(timeout=10)==0
            print("Menu smoke passed: lessons, pause/resume, save failure/retry, quit to menu, main Quit, settings and save management")
        finally:
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired: process.kill();process.wait()
