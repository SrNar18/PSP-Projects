"""Build the compact translucent HUD frame used by the two live meters."""
from pathlib import Path
from PIL import Image, ImageDraw
import struct

ROOT = Path(__file__).resolve().parents[1] / 'assets'
W, H = 114, 36
im = Image.new('RGBA', (W, H), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
d.polygon([(6, 0), (W-7, 0), (W-1, 6), (W-1, H-7), (W-7, H-1),
           (6, H-1), (0, H-7), (0, 6)], fill=(7, 14, 27, 223),
          outline=(58, 113, 133, 210))
d.line([(8, 2), (W-9, 2)], fill=(113, 214, 225, 190), width=1)
d.line([(8, H-3), (W-9, H-3)], fill=(43, 82, 100, 195), width=1)
d.line([(4, 7), (4, H-8)], fill=(46, 171, 192, 180), width=1)
d.line([(W-5, 7), (W-5, H-8)], fill=(177, 95, 139, 175), width=1)

# Two empty recesses; game.c adds only the illuminated fill segments.
for top in (7, 21):
    d.rounded_rectangle((27, top, 106, top+8), radius=2,
                        fill=(13, 29, 39, 245), outline=(58, 81, 91, 225))
    for segment in range(10):
        x = 29 + segment * 8
        d.rectangle((x, top+2, x+5, top+6), fill=(35, 51, 59, 210))
    d.line((27, top+1, 106, top+1), fill=(121, 173, 178, 80))

im.save(ROOT / 'hud-meter-frame-v253.png')
raw = bytearray()
for r, g, b, a in im.getdata():
    raw += struct.pack('<H', (r >> 4) | ((g >> 4) << 4) |
                       ((b >> 4) << 8) | ((a >> 4) << 12))
(ROOT / 'hud-meter-frame-v253.4444').write_bytes(raw)
