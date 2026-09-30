"""v2.50 (Claude): ilustraciones de la portada animada -> assets/title-slides/slideN.rgb565 (PSP 5650, 512x288).
Fuentes, en orden: assets/title-cover-v246.png (portada actual) y, si existen, assets/title-slide-1.png y
assets/title-slide-2.png (las dos nuevas de Codex). Recorte centrado a 16:9 y reescalado con LANCZOS."""
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'assets/title-slides';out.mkdir(exist_ok=True)
sources=[ROOT/'assets/title-cover-v246.png',ROOT/'assets/title-slide-1.png',ROOT/'assets/title-slide-2.png']
n=0
for src in sources:
    if not src.exists():continue
    im=Image.open(src).convert('RGB');w,h=im.size;tw=h*16/9
    if w>tw:im=im.crop((int((w-tw)/2),0,int((w+tw)/2),h))
    else:th=w*9/16;im=im.crop((0,int((h-th)/2),w,int((h+th)/2)))
    a=np.asarray(im.resize((512,288),Image.LANCZOS)).astype(np.uint16)
    v=(a[...,0]>>3)|((a[...,1]>>2)<<5)|((a[...,2]>>3)<<11)   # PSP 5650: rojo en los bits bajos
    (out/f'slide{n}.rgb565').write_bytes(v.astype('<u2').tobytes());n+=1
for k in range(n,3):
    p=out/f'slide{k}.rgb565'
    if p.exists():p.unlink()
print(f'{n} ilustraciones de portada')
