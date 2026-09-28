"""Compile portable OFL fonts to antialiased fixed-cell PSP glyphs.
Preserve the existing 7x12 layout; large headings rasterize at their true size.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
F=ROOT/'tools/fonts'
records=[]; data=bytearray()
for face,scale in [('Rajdhani-Medium.ttf',1),('Oxanium.ttf',1),('Oxanium.ttf',2),('Oxanium.ttf',3)]:
    w,h=7*scale,12*scale; off=len(data)
    font=ImageFont.truetype(str(F/face),14*scale*4 if scale==1 and face.startswith('Raj') else 11*scale*4)
    if face.startswith('Ox'): font.set_variation_by_axes([650])
    for code in range(32,127):
        ch=chr(code);box=font.getbbox(ch);inkw=max(1,box[2]-box[0])
        im=Image.new('L',(max(w*4,inkw+4),h*4),0)
        ImageDraw.Draw(im).text((-box[0],(h-2*scale)*4),ch,font=font,fill=255,anchor='ls')
        # Keep a one-pixel advance gap for legibility at 480x272.
        crop=im.crop((0,0,inkw,h*4)).resize((min(w-1,max(1,round(inkw/4))),h),Image.Resampling.LANCZOS)
        cell=Image.new('L',(w,h),0);cell.paste(crop,(0,0));data.extend(cell.tobytes())
    records.append((off,w,h))
header=['/* Generated from Oxanium / Rajdhani; SIL OFL, see tools/fonts. */','#ifndef NARCADE_UI_FONT_H','#define NARCADE_UI_FONT_H',
        'typedef struct {unsigned offset; unsigned char w,h;} UiFont;',
        'static const UiFont uiFonts[] = {'+','.join('{%du,%d,%d}'%r for r in records)+'};',
        'static const unsigned char uiGlyphs[] = {']
for i in range(0,len(data),40):header.append(','.join(map(str,data[i:i+40]))+',')
header+=['};','#endif'];(ROOT/'src/ui_font.h').write_text('\n'.join(header)+'\n',encoding='ascii')
print('UI font bytes:',len(data))
