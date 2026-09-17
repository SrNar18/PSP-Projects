"""Compile the original 4x4 material atlas into PSP-swizzled RGB565 tiles.

This is asset compilation, not runtime image loading. Pillow is only needed
when rebuilding materials; checked-in textures3d.bin builds without Python.
"""
from pathlib import Path
import struct
from PIL import Image

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
    (ROOT/'assets/textures3d').mkdir(exist_ok=True)
    result=bytearray()
    for i,name in enumerate(NAMES):
        atlas=atlases[i>=16];grid=2 if i>=16 else 4;idx=i-16 if i>=16 else i
        if name in ('jacket','jacket-back','jeans','sleeve'):
            atlas=streetwear;grid=2;idx={'jacket':0,'jacket-back':1,'jeans':2,'sleeve':3}[name]
        w,h=atlas.size;x,y=idx%grid,idx//grid
        # Exact integer grid boundaries, with a tiny inset to exclude grid seams.
        tile=atlas.crop((round(x*w/grid)+2,round(y*h/grid)+2,round((x+1)*w/grid)-2,round((y+1)*h/grid)-2))
        tile=tile.resize((128,128),Image.Resampling.LANCZOS)
        tile.save(ROOT/'assets/textures3d'/f'{name}.png')
        raw=b''.join(struct.pack('<H',(r>>3)|((g>>2)<<5)|((b>>3)<<11)) for r,g,b in tile.getdata())
        result.extend(swizzle(raw,256,128))
    white=Image.new('RGB',(128,128),'white');white.save(ROOT/'assets/textures3d/flat.png')
    result.extend(b'\xff\xff'*(128*128))
    assert len(result)==688128
    (ROOT/'assets/textures3d.bin').write_bytes(result)
    print('21 materials, RGB565, swizzled, 128x128: 688128 bytes')

if __name__=='__main__':main()
