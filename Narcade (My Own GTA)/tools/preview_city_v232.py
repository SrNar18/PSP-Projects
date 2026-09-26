"""Software inspection of the exact PSP mesh; this is not a PSP screenshot."""
from pathlib import Path
import struct
import numpy as np
from PIL import Image, ImageDraw
from textures3d import NAMES
from npc_textures import NAMES as NPC
from extra_textures import NAMES as EXTRA
root=Path(__file__).resolve().parents[1]
names=NAMES+['flat']+NPC+EXTRA
textures=[np.array(Image.open(root/'assets/textures3d'/f'{n}.png').convert('RGB')) for n in names]
W,H=800,450
sheet=Image.new('RGB',(W,H*3+100),(24,30,37))
for scene in range(3):
 raw=(root/f'build/city-v232-{scene}.bin').read_bytes();offset=24
 eye=np.array(struct.unpack_from('<3f',raw,0));target=np.array(struct.unpack_from('<3f',raw,12))
 forward=target-eye;forward/=np.linalg.norm(forward)
 right=np.cross(forward,[0,1,0]);right/=np.linalg.norm(right);up=np.cross(right,forward)
 output=np.full((H,W,3),[163,194,207],dtype=np.uint8);depth=np.full((H,W),np.inf)
 for mat in range(41):
  count=struct.unpack_from('<I',raw,offset)[0];offset+=4
  vertices=np.frombuffer(raw,dtype=[('uv','<f4',2),('color','<u4'),('xyz','<f4',3)],count=count,offset=offset);offset+=count*24
  rel=vertices['xyz']-eye;z=rel@forward
  p=np.column_stack((W/2+(rel@right)*520/np.maximum(z,.01),H/2-(rel@up)*520/np.maximum(z,.01),z))
  for i in range(0,count,3):
   tri=p[i:i+3]
   if len(tri)!=3 or tri[:,2].min()<1:continue
   xmin=max(0,int(np.floor(tri[:,0].min())));xmax=min(W-1,int(np.ceil(tri[:,0].max())))
   ymin=max(0,int(np.floor(tri[:,1].min())));ymax=min(H-1,int(np.ceil(tri[:,1].max())))
   if xmin>xmax or ymin>ymax:continue
   yy,xx=np.mgrid[ymin:ymax+1,xmin:xmax+1];xx=xx+.5;yy=yy+.5
   den=(tri[1,1]-tri[2,1])*(tri[0,0]-tri[2,0])+(tri[2,0]-tri[1,0])*(tri[0,1]-tri[2,1])
   if abs(den)<1e-8:continue
   a=((tri[1,1]-tri[2,1])*(xx-tri[2,0])+(tri[2,0]-tri[1,0])*(yy-tri[2,1]))/den
   b=((tri[2,1]-tri[0,1])*(xx-tri[2,0])+(tri[0,0]-tri[2,0])*(yy-tri[2,1]))/den;c=1-a-b
   inverse=a/tri[0,2]+b/tri[1,2]+c/tri[2,2];zz=1/np.maximum(inverse,1e-8)
   old=depth[ymin:ymax+1,xmin:xmax+1];mask=(a>=0)&(b>=0)&(c>=0)&(zz<old)
   if not mask.any():continue
   weights=np.stack((a/tri[0,2],b/tri[1,2],c/tri[2,2]),axis=-1)*zz[...,None]
   uv=weights@vertices['uv'][i:i+3];tex=textures[mat];th,tw=tex.shape[:2]
   tx=(uv[...,0]*tw).astype(int)%tw;ty=(uv[...,1]*th).astype(int)%th
   colors=vertices['color'][i:i+3];rgb=np.stack((colors&255,(colors>>8)&255,(colors>>16)&255),axis=-1)/255
   tint=weights@rgb;pixels=np.clip(tex[ty,tx]*tint,0,255).astype(np.uint8)
   output[ymin:ymax+1,xmin:xmax+1][mask]=pixels[mask];old[mask]=zz[mask]
 sheet.paste(Image.fromarray(output),(0,30+scene*(H+20)))
 ImageDraw.Draw(sheet).text((12,12+scene*(H+20)),['FACHADAS Y BALCONES','CASAS ADOSADAS','CAUCE CONTINUO'][scene],fill='white')
ImageDraw.Draw(sheet).text((12,H*3+86),'Narcade 2.32 / malla real, render de inspeccion en PC; no captura PSP.',fill='white')
out=root/'assets/city-v232-preview.png';sheet.save(out);print(out)
