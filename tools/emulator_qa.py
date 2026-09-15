"""Local PPSSPP launch and capture. Requires Xvfb, python-xlib and Pillow."""
import os, pathlib, subprocess, time, json
from Xlib import display, X, XK
from Xlib.ext import xtest
from PIL import Image
root=pathlib.Path(__file__).resolve().parents[1];tools=root.parent/'tooling';out=root/'build/emulator';out.mkdir(parents=True,exist_ok=True)
env=dict(os.environ);env.update(DISPLAY='127.0.0.1:32',SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CONFIG_HOME=str(out/'config'),LD_LIBRARY_PATH=str(tools/'x11/usr/lib/x86_64-linux-gnu'))
(out/'config/ppsspp/PSP/SYSTEM').mkdir(parents=True,exist_ok=True)
serverlog=open(out/'xvfb.log','w');gameout=open(out/'stdout.log','w')
server=subprocess.Popen([str(tools/'x11/usr/bin/Xvfb'),':32','-screen','0','960x544x24','-ac','-nolisten','local','-nolisten','unix','-listen','tcp'],env=env,stdout=serverlog,stderr=subprocess.STDOUT)
game=None
try:
 for i in range(50):
  try:d=display.Display(env['DISPLAY']);break
  except Exception:time.sleep(.1)
 else:raise RuntimeError('X display unavailable')
 config=out/'qa.ini';config.write_text('[General]\nFirstRun = False\nCheckForNewVersion = False\nEnableStateUndo = False\n\n[Graphics]\nShowFPSCounter = 1\nRenderingMode = 1\n\n[Sound]\nEnable = True\n')
 cmd=[str(tools/'squashfs-root/bin/PPSSPPSDL'),'--windowed','--xres','960','--yres','544','--graphics=software','--appendconfig='+str(config),'--log='+str(out/'ppsspp.log'),str(root/'Narcade.iso')]
 game=subprocess.Popen(cmd,env=env,cwd=str(tools/'squashfs-root/bin'),stdout=gameout,stderr=subprocess.STDOUT)
 def shot(name):
  win=d.screen().root;raw=win.get_image(0,0,960,544,X.ZPixmap,0xffffffff);im=Image.frombytes('RGB',(960,544),raw.data,'raw','BGRX');im.save(out/(name+'.png'));print('captured',name,flush=True)
 def key(name,seconds=.65):
  code=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,code);d.sync();time.sleep(seconds);xtest.fake_input(d,X.KeyRelease,code);d.sync();time.sleep(.75)
 time.sleep(7)
 for child in d.screen().root.query_tree().children:
  if child.get_wm_name() and 'PPSSPP' in child.get_wm_name():
   print('Focusing',child.get_wm_name(),flush=True);child.set_input_focus(X.RevertToParent,X.CurrentTime);d.sync()
 shot('01-boot')
 key('z');time.sleep(1);shot('02-story')
 key('z');time.sleep(1);shot('03-city')
 key('z');time.sleep(1);shot('04-world')
 key('Right',2);shot('05-movement')
 key('space');shot('06-pause')
 key('Down');key('z');shot('07-saved')
 key('Return');shot('08-map')
 print('PPSSPP process alive:',game.poll() is None,flush=True)
finally:
 if game is not None:game.terminate();game.wait(timeout=10)
 server.terminate();server.wait(timeout=10)
 serverlog.close();gameout.close()
