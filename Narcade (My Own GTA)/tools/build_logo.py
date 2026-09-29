"""v2.44 (Claude): logo NARCADE extraido de la portada (assets/icon-source.png) con transparencia.
Genera src/logo_narcade.h: dos tamanos en ARGB4444 (unos 17 KB en total) y assets/logo-narcade.png."""
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
src=Image.open(ROOT/'assets/icon-source.png').convert('RGB').crop((325,195,1405,548))
a=np.asarray(src).astype(np.float32)
bg=np.array([22,16,44],np.float32)                        # cielo nocturno de la portada
dist=np.sqrt(((a-bg)**2).sum(axis=2))
alpha=np.clip((dist-18)/120,0,1)                           # el halo neon queda semitransparente
lum=a.max(axis=2);alpha=np.maximum(alpha,np.clip((lum-70)/90,0,1))
# ilumina el color: sin el fondo mezclado, el neon se ve limpio sobre cualquier superficie
rgb=np.clip((a-bg*(1-alpha[...,None]))/np.maximum(alpha[...,None],.05),0,255)
rgba=np.dstack([rgb,alpha*255]).astype(np.uint8)
full=Image.fromarray(rgba,'RGBA');full.save(ROOT/'assets/logo-narcade.png')
out=['/* Generado por tools/build_logo.py: logo de la portada, ARGB4444. No editar a mano. */','#ifndef NARCADE_LOGO_H','#define NARCADE_LOGO_H']
for name,w in (('logoLarge',200),('logoSmall',104)):
    h=round(w*full.height/full.width);im=full.resize((w,h),Image.LANCZOS);p=np.asarray(im).astype(np.uint16)
    v=((p[...,3]>>4)<<12)|((p[...,2]>>4)<<8)|((p[...,1]>>4)<<4)|(p[...,0]>>4)
    out.append(f'#define {name.upper()}_W {w}\n#define {name.upper()}_H {h}')
    out.append(f'static const unsigned short {name}[{w*h}]={{')
    flat=v.flatten().tolist()
    for i in range(0,len(flat),24):out.append(','.join(str(x) for x in flat[i:i+24])+',')
    out.append('};')
out.append('#endif')
(ROOT/'src/logo_narcade.h').write_text('\n'.join(out)+'\n',encoding='utf-8')
print('logo',full.size)
