"""Generate code-native graphics, bitmap type and four original hip-hop loops."""
from PIL import Image, ImageDraw, ImageFont
import numpy as np
import pathlib, json
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'assets';out.mkdir(exist_ok=True)
fontpath='/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf'
font=ImageFont.truetype(fontpath,11)
glyphs=[]
for i in range(32,127):
 im=Image.new('1',(7,12));ImageDraw.Draw(im).text((0,-2),chr(i),font=font,fill=1)
 glyphs.extend(sum((1<<x) for x in range(7) if im.getpixel((x,y))) for y in range(12))
allura=root.parent/'tooling/Allura-Regular.ttf'
if not allura.exists(): allura=root/'tools/Allura-Regular.ttf'
cf=ImageFont.truetype(str(allura),65)
logo=Image.new('L',(180,74));d=ImageDraw.Draw(logo);d.text((8,-9),'Naresz',font=cf,fill=255)
# Genuine cursive signature, loaded as a small alpha bitmap.
(out/'signature.bin').write_bytes(logo.tobytes())
names=['Ladera FM / Concreto y cielo','Rio 90.7 / Puente de noche','Sur Beats / Ventanas abiertas','Horizonte / Vuelta a casa']
SR=22050
audio=[];offs=[];lengths=[]
for track,bpm in enumerate([88,94,82,98]):
 n=int(SR*60/bpm*32); t=np.arange(n)/SR; a=np.zeros(n); beat=60/bpm
 rng=np.random.default_rng(3100+track)
 def put(start,v):
  k=int(start*SR);end=min(n,k+len(v))
  if end>k:a[k:end]+=v[:end-k]
 def sine(freq,dur,decay,amp):
  q=np.arange(int(dur*SR))/SR
  return np.sin(2*np.pi*freq*q)*np.exp(-q*decay)*amp
 roots=[[55,65.406,49,58.27],[65.406,58.27,51.913,49],[49,58.27,65.406,55],[61.735,55,49,46.25]][track]
 for b in range(32):
  bar=b//4; r=roots[(bar//2)%4]
  # Kick + snare with quiet ghost notes and swung closed hats.
  if b%4 in (0,2) or (bar%2 and b%4==3):
   q=np.arange(int(.28*SR))/SR
   put(b*beat,np.sin(2*np.pi*(46*q+8*(1-np.exp(-q*25))))*np.exp(-q*19)*.55)
  if b%2==1:
   q=np.arange(int(.19*SR))/SR
   put(b*beat,(rng.normal(0,1,len(q))*.17+np.sin(q*2*np.pi*180)*.13)*np.exp(-q*23))
  for h in range(2):
   q=np.arange(int(.065*SR))/SR;noise=rng.normal(0,1,len(q));noise=np.r_[0,np.diff(noise)]
   put((b+h*.56)*beat,noise*np.exp(-q*90)*(.045 if h else .065))
  if b%4 in (0,2,3): put(b*beat,sine(r if b%4!=3 else r*1.5,.6,5,.24))
  if b%4==0:
   for mul in [2,2*2**(3/12),3,2*2**(10/12)]:put(b*beat,sine(r*mul,beat*3.6,1.6,.058))
  if b%2==0:
   melody=[12,15,19,22,19,15,10,7][(b//2+track*2)%8]
   put((b+.5)*beat,sine(r*2**(melody/12)*2,.45,8,.045))
 a=np.tanh(a*1.1)*.73; pcm=(a*32767).astype('<i2')
 offs.append(sum(lengths));lengths.append(n);audio.append(pcm.tobytes())
(out/'radio.bin').write_bytes(b''.join(audio))
header='/* Generated graphics and original music metadata. */\n'
header+='static const unsigned char font_bits[] = {'+','.join(map(str,glyphs))+'};\n'
header+='static const int track_start[] = {'+','.join(map(str,offs))+'};\n'
header+='static const int track_length[] = {'+','.join(map(str,lengths))+'};\n'
header+='static const char *track_names[] = {'+','.join(json.dumps(x) for x in names)+'};\n'
header+='extern const unsigned char signature_data[];\nextern const short radio_data[];\n'
(root/'src/assets.h').write_text(header)
(root/'src/assets.S').write_text('.section .rodata\n.balign 16\n.global signature_data\nsignature_data:\n.incbin "assets/signature.bin"\n.balign 16\n.global radio_data\nradio_data:\n.incbin "assets/radio.bin"\n.section .note.GNU-stack,"",@progbits\n')
im=Image.new('RGB',(144,80),(13,23,32));d=ImageDraw.Draw(im)
d.rectangle((0,65,144,80),fill=(213,242,97))
big=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',22)
d.text((9,17),'NARCADE',font=big,fill=(234,240,221));d.text((11,52),'MEDELLIN',font=font,fill=(66,208,198))
im.save(out/'ICON0.png')
print('assets:',sum(lengths)*2,'bytes of original music')
