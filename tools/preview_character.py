"""Orthographic textured preview of the actual exported runtime OBJ (not concept art).
No Blender dependency. The PSP still renders with its own GE backend.
"""
from pathlib import Path
import math,sys,struct
import numpy as np
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
verts=[];uv=[];faces=[];material=None
model=Path(sys.argv[sys.argv.index('--model')+1]) if '--model' in sys.argv else ROOT/'assets/nico.obj'
for line in model.read_text().splitlines():
    fields=line.split()
    if not fields:continue
    if fields[0]=='v':verts.append(list(map(float,fields[1:])))
    if fields[0]=='vt':uv.append(list(map(float,fields[1:])))
    if fields[0]=='usemtl':material=fields[1]
    if fields[0]=='f':faces.append((material,[[int(n)-1 for n in f.split('/')] for f in fields[1:]]))
verts=np.array(verts);uv=np.array(uv)
textures={m:np.array(Image.open(ROOT/f'assets/textures3d/{m}.png').convert('RGB')) for m,_ in faces}
W,H=400,500
def render(angle):
    look=np.array([math.cos(angle),.12,math.sin(angle)]);look/=np.linalg.norm(look)
    right=np.array([-math.sin(angle),0,math.cos(angle)])
    up=np.cross(right,look)
    v=verts[:,:3]-[0,14.5,0]
    scale=min(W/28,H/35)
    projected=np.column_stack((W/2+v@right*scale,H/2-v@up*scale,v@look))
    output=np.zeros((H,W,3),dtype=np.uint8);output[:]=[30,36,44]
    depth=np.full((H,W),-np.inf)
    for mat,inds in faces:
        ids=[i[0] for i in inds];p=projected[ids];t=uv[[i[1] for i in inds]]
        xmin=max(0,int(np.floor(p[:,0].min())));xmax=min(W-1,int(np.ceil(p[:,0].max())))
        ymin=max(0,int(np.floor(p[:,1].min())));ymax=min(H-1,int(np.ceil(p[:,1].max())))
        if xmin>xmax or ymin>ymax:continue
        yy,xx=np.mgrid[ymin:ymax+1,xmin:xmax+1];xx=xx+.5;yy=yy+.5
        den=(p[1,1]-p[2,1])*(p[0,0]-p[2,0])+(p[2,0]-p[1,0])*(p[0,1]-p[2,1])
        if abs(den)<1e-8:continue
        a=((p[1,1]-p[2,1])*(xx-p[2,0])+(p[2,0]-p[1,0])*(yy-p[2,1]))/den
        b=((p[2,1]-p[0,1])*(xx-p[2,0])+(p[0,0]-p[2,0])*(yy-p[2,1]))/den;c=1-a-b
        z=a*p[0,2]+b*p[1,2]+c*p[2,2]
        old=depth[ymin:ymax+1,xmin:xmax+1];mask=(a>=-1e-5)&(b>=-1e-5)&(c>=-1e-5)&(z>old)
        u=a*t[0,0]+b*t[1,0]+c*t[2,0];vv=1-(a*t[0,1]+b*t[1,1]+c*t[2,1])
        tex=textures[mat];size=tex.shape[0];tx=np.clip((u*(size-1)).astype(int),0,size-1);ty=np.clip((vv*(size-1)).astype(int),0,size-1)
        colors=verts[ids,3:6] if verts.shape[1]>=6 else np.ones((3,3))
        shade=a[...,None]*colors[0]+b[...,None]*colors[1]+c[...,None]*colors[2]
        rgb=np.clip(tex[ty,tx]*shade,0,255).astype(np.uint8)
        output[ymin:ymax+1,xmin:xmax+1][mask]=rgb[mask];old[mask]=z[mask]
    return Image.fromarray(output)
sheet=Image.new('RGB',(W*3,H+60),(30,36,44));draw=ImageDraw.Draw(sheet)
for i,(angle,label) in enumerate([(0,'FRENTE'),(.8,'TRES CUARTOS'),(math.pi,'ESPALDA')]):
    sheet.paste(render(angle),(i*W,40));draw.text((i*W+155,20),label,fill=(235,240,248))
draw.text((30,H+43),'NARCADE - Malla y texturas del juego. Vista de estudio; no es una captura de PSP.',fill=(185,199,212))
out=Path(sys.argv[sys.argv.index('--output')+1]) if '--output' in sys.argv else ROOT/'assets/nico-streetwear-preview.png';sheet.save(out);print(out)
if '--animate' in sys.argv:
    raw=(ROOT/'build/player-animation.bin').read_bytes();nf,nv=struct.unpack_from('<II',raw)
    poses=np.frombuffer(raw[8:],dtype='<f4').reshape(nf,nv,3);assert nv==len(verts)
    W,H=240,360
    for name,start,duration in [('walk',0,36),('jog',16,27),('run',32,23)]:
        frames=[]
        for pose in poses[start:start+16]:
            verts[:,:3]=pose
            # Same rasterizer and actual runtime deformations; no artistic interpolation.
            pic=render(.8);ImageDraw.Draw(pic).text((12,12),'NARCADE / '+name.upper(),fill=(240,245,250));frames.append(pic)
        path=ROOT/f'assets/nico-{name}.gif';frames[0].save(path,save_all=True,append_images=frames[1:],duration=duration,loop=0);print(path)
