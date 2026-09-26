"""Genera src/font.h: fuente de mapa de bits 8x14 (Latin-1, codigos 32..255) a partir de DejaVu Sans Mono."""
import os
from PIL import Image, ImageDraw, ImageFont
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ttf=r'C:\Users\carmo\Downloads\Narcade_v1.1_fuente\tools\DejaVuSansMono.ttf'
if not os.path.exists(ttf):
    for c in [r'C:\Windows\Fonts\consola.ttf',r'C:\Windows\Fonts\lucon.ttf',r'C:\Windows\Fonts\cour.ttf']:
        if os.path.exists(c):ttf=c;break
font=ImageFont.truetype(ttf,13)
W,H=8,14
rows=[]
for code in range(32,256):
    im=Image.new('L',(W,H),0);d=ImageDraw.Draw(im)
    ch=chr(code) if code!=0xAD else '-'
    d.text((0,-1),ch,font=font,fill=255)
    px=im.load();bits=[]
    for y in range(H):
        b=0
        for x in range(W):
            if px[x,y]>110:b|=1<<(7-x)
        bits.append(b)
    rows.append(bits)
with open(os.path.join(ROOT,'src','font.h'),'w') as f:
    f.write('/* generado por tools/make_font.py: 224 glifos Latin-1 (32..255), 8x14, 1 bit por pixel, fila por byte */\n')
    f.write('#define FONT_W 8\n#define FONT_H 14\nstatic const unsigned char font_bits[224][14]={\n')
    for bits in rows:f.write('{'+','.join(str(b) for b in bits)+'},\n')
    f.write('};\n')
print('font.h ok, fuente:',ttf)
