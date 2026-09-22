"""Compile the original 4x4 material atlas into PSP-swizzled RGB565 tiles.

This is asset compilation, not runtime image loading. Pillow is only needed
when rebuilding materials; checked-in textures3d.bin builds without Python.
"""
from pathlib import Path
import struct
from PIL import Image
from npc_textures import create, NAMES as NPC_NAMES
from extra_textures import create as create_extra, NAMES as EXTRA_NAMES
from road_materials import create as create_road
from car_materials import create as create_car

ROOT = Path(__file__).resolve().parents[1]
NAMES = ['asphalt','sidewalk','brick','stucco','shop','roof','grass','water',
         'jacket','jeans','face','wheel','car-side','car-paint','glass','mural',
         'jacket-back','sleeve','skin','hair']

def swizzle(raw, width_bytes, height):
    return b''.join(raw[y*width_bytes+x:y*width_bytes+x+16]
                    for block_y in range(0,height,8)
                    for x in range(0,width_bytes,16)
                    for y in range(block_y,block_y+8))

def main():
    atlases=[Image.open(ROOT/'assets/textures3d-atlas.png').convert('RGB'),
             Image.open(ROOT/'assets/character-atlas.png').convert('RGB')]
    streetwear=Image.open(ROOT/'assets/streetwear-atlas.png').convert('RGB')
    urban=Image.open(ROOT/'assets/urban-atlas-v28.png').convert('RGB')
    detail=Image.open(ROOT/'assets/urban-detail-atlas-v28.png').convert('RGB')
    refreshed=Image.open(ROOT/'assets/materials-v210.png').convert('RGB')
    (ROOT/'assets/textures3d').mkdir(exist_ok=True)
    result=bytearray()
    for i,name in enumerate(NAMES):
        atlas=atlases[i>=16];grid=2 if i>=16 else 4;idx=i-16 if i>=16 else i
        if name in ('jacket','jacket-back','jeans','sleeve'):
            atlas=streetwear;grid=2;idx={'jacket':0,'jacket-back':1,'jeans':2,'sleeve':3}[name]
        w,h=atlas.size;x,y=idx%grid,idx//grid
        # Exact integer grid boundaries, with a tiny inset to exclude grid seams.
        tile=atlas.crop((round(x*w/grid)+2,round(y*h/grid)+2,round((x+1)*w/grid)-2,round((y+1)*h/grid)-2))
        if i<8:
            w,h=urban.size;x,y=i%4,i//4
            tile=urban.crop((round(x*w/4)+2,round(y*h/2)+2,round((x+1)*w/4)-2,round((y+1)*h/2)-2))
        replacements={'glass':1,'brick':2,'stucco':3,'jeans':4,'sleeve':5,'roof':6,'sidewalk':7}
        if name in replacements:
            j=replacements[name];w,h=refreshed.size;x,y=j%4,j//4
            tile=refreshed.crop((round(x*w/4)+2,round(y*h/2)+2,round((x+1)*w/4)-2,round((y+1)*h/2)-2))
        if name in ('asphalt','sidewalk'):
            tile=create_road(name)
        if name in ('car-side','car-paint'):
            tile=create_car(name)
        tile=tile.resize((128,128),Image.Resampling.LANCZOS)
        tile.save(ROOT/'assets/textures3d'/f'{name}.png')
        raw=b''.join(struct.pack('<H',(r>>3)|((g>>2)<<5)|((b>>3)<<11)) for r,g,b in tile.getdata())
        result.extend(swizzle(raw,256,128))
    white=Image.new('RGB',(128,128),'white');white.save(ROOT/'assets/textures3d/flat.png')
    result.extend(b'\xff\xff'*(128*128))
    assert len(result)==688128
    # Additional pedestrian materials stay in main RAM, preserving the PSP's
    # existing 2MB VRAM layout. Each swizzled 64px tile is only 8KB.
    for i,name in enumerate(NPC_NAMES):
        tile=create(i);tile.save(ROOT/'assets/textures3d'/f'{name}.png')
        raw=b''.join(struct.pack('<H',(r>>3)|((g>>2)<<5)|((b>>3)<<11)) for r,g,b in tile.getdata())
        result.extend(swizzle(raw,128,64))
    assert len(result)==753664
    # Eight established detail tiles plus four distinct street-facing facades.
    for i,name in enumerate(EXTRA_NAMES):
        if i<8:
            w,h=detail.size;x,y=i%4,i//4
            tile=detail.crop((round(x*w/4)+2,round(y*h/2)+2,round((x+1)*w/4)-2,round((y+1)*h/2)-2)).resize((64,64),Image.Resampling.LANCZOS)
        else:tile=create_extra(i)
        tile.save(ROOT/'assets/textures3d'/f'{name}.png')
        raw=b''.join(struct.pack('<H',(r>>3)|((g>>2)<<5)|((b>>3)<<11)) for r,g,b in tile.getdata())
        result.extend(swizzle(raw,128,64))
    assert len(result)==753664+12*8192
    (ROOT/'assets/textures3d.bin').write_bytes(result)
    print('21 VRAM materials + 8 pedestrian + 12 extra RAM materials:',len(result),'bytes')

if __name__=='__main__':main()
