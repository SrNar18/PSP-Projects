"""Reduce the user's GLB, retain named pieces for procedural animation, map cloth UVs.
Requires numpy and fast-simplification only when regenerating player_mesh.h.
"""
import sys,json,struct,math,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'build/python-deps'))
import numpy as np
import fast_simplification as fs
b=(ROOT/'assets/reference/personaje-v3.glb').read_bytes()
length=struct.unpack_from('<I',b,12)[0];doc=json.loads(b[20:20+length]);binary=b[28+length:]
def acc(index):
    a=doc['accessors'][index];v=doc['bufferViews'][a['bufferView']]
    dtype={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']]
    channels={'VEC3':3,'VEC2':2,'SCALAR':1}[a['type']]
    return np.ndarray((a['count'],channels),dtype=dtype,buffer=binary,offset=v.get('byteOffset',0)+a.get('byteOffset',0),strides=(v.get('byteStride',np.dtype(dtype).itemsize*channels),np.dtype(dtype).itemsize)).copy()
pieces=[];original=0
for node in doc['nodes']:
    if 'mesh' not in node:continue
    assert not any(k in node for k in ('translation','rotation','scale')),'Unexpected TRS transform'
    name=node.get('name','part')
    if name=='hair-texture':continue # dense individual curls replaced by textured cap
    if name.startswith(('sole-','outsole-','upper-','ankle-','collar-pad','heel-tab','stripe-')):continue # stable low-poly rounded shoes generated in renderer
    if name.startswith(('eyeball','iris','pupil','eyelid','lashline','brow-','lid-','canthus','lip-','mouth-line','nostril')):continue # baked face replaces subpixel facial parts
    for prim in doc['meshes'][node['mesh']]['primitives']:
        p=acc(prim['attributes']['POSITION']).astype(np.float64)
        if 'matrix' in node:
            matrix=np.array(node['matrix']).reshape(4,4).T
            p=(np.column_stack((p,np.ones(len(p))))@matrix.T)[:,:3]
        f=acc(prim['indices']).reshape(-1,3).astype(np.int32) if 'indices'in prim else np.arange(len(p),dtype=np.int32).reshape(-1,3)
        original+=len(f)
        # Weld duplicate seam vertices before QEM.
        p,inverse=np.unique(np.round(p,6),axis=0,return_inverse=True);f=inverse[f].astype(np.int32)
        target=24
        if name.startswith('pants-leg'):target=260
        elif name=='tee-body':target=400
        elif name=='head':target=700
        elif name.startswith(('arm-','sleeve-')):target=120
        elif name=='hair':target=450
        elif name in ('pants-seat','neck'):target=100
        elif name.startswith(('sole-','upper-','outsole-')):target=200
        elif name.startswith(('finger-','thumb-','palm-')):target=32
        p,f=fs.simplify(p,f,target_count=min(max(8,int(target*.65)),len(f)),agg=9)
        material=doc['materials'][prim['material']]
        pieces.append((name,p,np.asarray(f),material))
allp=np.concatenate([p for _,p,_,_ in pieces]);lo=allp.min(0);hi=allp.max(0);lo[1]=0
print('source bounds',lo,hi,'faces excluding hair strands',original)
# glTF Y-up, +Z facing. PSP Y-up, +X facing.
scale=29.2/(hi[1]-lo[1]);center=(lo+hi)/2
out=['/* Generated from user-provided personaje-v3.glb by tools/import_character.py. */',
     'typedef struct {float x,y,z,u,v;unsigned int color;unsigned char mat,bone;} PlayerVertex;',
     'static const PlayerVertex player_mesh[]={']
count=0
for name,positions,faces,material in pieces:
    p=np.column_stack((positions[:,2]-center[2],positions[:,1]-lo[1],positions[:,0]-center[0]))*scale
    normal=np.zeros_like(p)
    for f in faces:
        n=np.cross(p[f[1]]-p[f[0]],p[f[2]]-p[f[0]])
        for i in f:normal[i]+=n
    normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-8)
    matname=material['name'];mat=20;bone=0
    # Match the side token, never the '-l' inside 'pants-leg-r'.
    # The old substring test assigned BOTH trouser legs to the left limb.
    side_match=re.search(r'-(l|r)(?:-|$)',name)
    side={'l':1,'r':2}[side_match[1]] if side_match else 0
    if name.startswith(('pants-leg','hem-back','sole-','outsole-','upper-','ankle-','collar-pad','heel-tab','stripe-')):bone=side
    if name.startswith(('sleeve-','arm-','palm-','finger-','nail-','thumb-')):bone=side+2
    color=np.array(material['pbrMetallicRoughness']['baseColorFactor'][:3]);color=np.where(color<=.0031308,color*12.92,1.055*color**(1/2.4)-.055)
    if matname=='denim':mat=9;color=np.ones(3)
    if matname=='tee':mat=17 if name.startswith('sleeve') else 8;color=np.ones(3)
    if matname=='hair':mat=19;color=np.ones(3)*.65
    if matname=='skin':mat=18;color=np.ones(3)
    mn=p.min(0);mx=p.max(0);extent=np.maximum(mx-mn,.01)
    for f in faces:
        face_mat=16 if mat==8 and p[f,0].mean()<0 else mat
        if name=='head' and p[f,0].mean()>(mn[0]+extent[0]*.55):face_mat=10
        for i in f:
            q=p[i];u=(q[2]-mn[2])/extent[2];v=1-(q[1]-mn[1])/extent[1]
            light=np.clip(.82+.14*normal[i,0]+.10*normal[i,1]-.04*normal[i,2],.56,1)
            rgb=np.clip(color*light*255,0,255).astype(int);c=0xff000000|int(rgb[0])|(int(rgb[1])<<8)|(int(rgb[2])<<16)
            out.append('{'+','.join(f'{float(x):.5f}f' for x in (*q,u,v))+f',0x{c:08x}u,{face_mat},{bone}'+'},');count+=1
out+=['};',f'#define PLAYER_VERTEX_COUNT {count}']
(ROOT/'src/player_mesh.h').write_text('\n'.join(out)+'\n')
(ROOT/'assets/reference/character-import.json').write_text(json.dumps({'source':'personaje-v3.glb','source_triangles_without_hair_strands':original,'triangles':count//3,'pieces':len(pieces),'height':29.2,'textures':'streetwear-atlas.png','animation':'procedural per named limb, no source rig'},indent=2))
print('PSP player:',count//3,'triangles,',len(pieces),'pieces')
