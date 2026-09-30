"""Compile eight original painted inventory icons into PSP RGB565 masks."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]
sheet = Image.open(ROOT / 'assets/weapon-icon-sheet-v247.png').convert('RGBA')
cell_w, cell_h = sheet.width // 4, sheet.height // 2
icons = []
for i in range(8):
    col, row = i % 4, i // 4
    cell = sheet.crop((col*cell_w, row*cell_h, (col+1)*cell_w, (row+1)*cell_h))
    alpha = cell.getchannel('A')
    bounds = alpha.getbbox()
    if not bounds:
        raise ValueError(f'empty weapon icon {i}')
    artwork = cell.crop(bounds)
    fitted = ImageOps.contain(artwork, (60, 27), method=Image.Resampling.LANCZOS)
    icon = Image.new('RGBA', (64, 32))
    icon.alpha_composite(fitted, ((64-fitted.width)//2, (32-fitted.height)//2))
    icons.append(icon)

out = ['/* Original painted icon sheet, compiled by tools/weapon_icons.py. */',
       'static const unsigned short weapon_icons[8][2048]={']
for icon in icons:
    values = []
    for r, g, b, a in icon.get_flattened_data():
        values.append(0 if a < 72 else (((r>>3)|((g>>2)<<5)|((b>>3)<<11)) or 1))
    out.append('{' + ','.join(map(str, values)) + '},')
out.append('};')
(ROOT/'src/weapon_icons.h').write_text('\n'.join(out)+'\n', encoding='ascii')
preview = Image.new('RGB', (512, 64), (22, 20, 26))
draw = ImageDraw.Draw(preview)
labels = ['FISTS','PISTOL','REVOLVER','SMG','AK','SHOTGUN','RIFLE','BAT']
for i, icon in enumerate(icons):
    preview.paste(icon, (i*64, 0), icon)
    draw.text((i*64+2, 42), labels[i], fill=(231, 211, 181))
(ROOT/'build').mkdir(exist_ok=True)
preview.save(ROOT/'build/weapon-icons-v247-preview.png')
print('weapon icons:', len(icons), 'x 64x32')
