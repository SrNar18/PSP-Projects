"""Original brushed steel and finished wood tiles for the handheld models.
Append after the established city textures AND their street mip levels.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import random,math,struct
NAMES=['weapon-steel','weapon-wood']
BASE_BYTES=753664+14*8192
def tiles():
 rng=random.Random(3301);result=[]
 for index in range(2):
  im=Image.new('RGB',(64,64));pixels=im.load()
  for y in range(64):
   for x in range(64):
    if index==0:
     grain=rng.randrange(-10,11)+6*math.sin(y*3.1);glint=26*math.exp(-((y-18)/9)**2)
     pixels[x,y]=tuple(int(max(0,min(255,b+grain+glint))) for b in (159,169,178))
    else:
     grain=13*math.sin(y*.55+math.sin(x*.06)*1.8)+7*math.sin(y*1.4+x*.03)+rng.randrange(-4,5)
     pixels[x,y]=tuple(int(max(0,min(255,b+grain))) for b in (189,135,77))
  d=ImageDraw.Draw(im)
  if index==0:
   for k in range(5):
    x,y=rng.randrange(4,54),rng.randrange(6,58);d.line((x,y,x+8,y),fill=(191,198,202))
  result.append(im)
 return result
def append_weapon_tiles(data,root):
 assert len(data)==BASE_BYTES
 for name,im in zip(NAMES,tiles()):
  im.save(root/'assets/textures3d'/f'{name}.png')
  raw=b''.join(struct.pack('<H',(r>>3)|((g>>2)<<5)|((b>>3)<<11)) for r,g,b in im.getdata())
  # PSP swizzle: 16-byte wide by 8-row blocks, RGB565.
  for y in range(0,64,8):
   for x in range(0,128,16):
    for row in range(8):data.extend(raw[(y+row)*128+x:(y+row)*128+x+16])
 return data
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1];p=root/'assets/textures3d.bin'
 data=bytearray(p.read_bytes()[:BASE_BYTES]);p.write_bytes(append_weapon_tiles(data,root))
