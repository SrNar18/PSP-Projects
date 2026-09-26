"""Read-only state inspection plus controller input in an isolated PPSSPP run.
Never use a user's active emulator or savedata for this test.
"""
import argparse,base64,json,struct,subprocess,time
from pathlib import Path
from ppsspp_check import PSP
p=argparse.ArgumentParser();p.add_argument('--nm',required=True);p.add_argument('--port',type=int,default=19322);args=p.parse_args()
root=Path(__file__).resolve().parents[1]
layout=json.loads(subprocess.check_output([str(root/'build/character_layout.exe')],cwd=root))
symbols={}
for line in subprocess.check_output([args.nm,'-S','narcade_static.elf'],cwd=root,text=True).splitlines():
 fields=line.split()
 if len(fields)==4 and fields[-1] in layout:symbols[fields[-1]]=int(fields[0],16)
client=PSP(args.port)
def read(group,field,fmt='i'):
 r=client.call('memory.read',address=symbols[group]+layout[group][field],size=4)
 return struct.unpack('<'+fmt,base64.b64decode(r['base64']))[0]
def buttons(**keys):client.call('input.buttons.send',buttons=keys)
def until(fn,seconds=6):
 start=time.monotonic()
 while time.monotonic()-start<seconds:
  if fn():return
  time.sleep(.06)
 raise AssertionError('Timed out waiting for PSP state')
print('Initial PSP screen:',read('g','screen'))
if read('g','screen')==0:
 client.tap('cross',6);time.sleep(.35);client.tap('down',6);client.tap('cross',6);time.sleep(.35)
 for i in range(3):
  if read('g','screen')==1:break
  client.tap('cross',6);time.sleep(.35)
until(lambda:read('g','screen')==1)
buttons(rtrigger=True);until(lambda:read('combat','aiming')==1)
buttons(circle=True);until(lambda:read('combat','punch','f')>0)
buttons(circle=False,rtrigger=False);until(lambda:read('combat','aiming')==0)
buttons(square=True);until(lambda:read('combat','jump','f')>.1)
buttons(square=False);until(lambda:read('combat','jump','f')==0)
buttons(ltrigger=True);until(lambda:read('g','weaponWheel')==1)
client.call('input.analog.send',x=1,y=0);time.sleep(.25)
buttons(ltrigger=False);client.call('input.analog.send',x=0,y=0)
until(lambda:read('g','weapon')==2)
buttons(rtrigger=True);until(lambda:read('combat','aiming')==1)
buttons(circle=True);until(lambda:read('combat','shotTime','f')>0)
buttons(circle=False,rtrigger=False)
time.sleep(.4)
assert read('g','screen')==1
status=client.call('cpu.status');assert not status['paused']
print('PASS actual PSP ISO: aim, held punch, jump/landing, weapon wheel/equip, gunshot, no unexpected journal or pause.')
