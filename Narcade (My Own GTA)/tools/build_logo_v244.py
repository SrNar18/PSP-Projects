"""Extract the original PSP cover mark as one reusable, transparent logo.

The source is our own icon artwork. Keeping the source here lets the cover,
menu and future screens share precisely the same identity.
"""
from pathlib import Path
import struct
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
source = Image.open(ASSETS / "icon-source.png").convert("RGB")
# The original mark occupies this rectangle; the surroundings are dark sky.
crop = source.crop((265, 190, 1410, 580))
alpha = Image.new("L", crop.size)
ap = alpha.load()
for y in range(crop.height):
    for x in range(crop.width):
        r, g, b = crop.getpixel((x, y))
        hi, lo = max(r, g, b), min(r, g, b)
        # Saturated, bright neon only. Fade the faint glow at the edge.
        strength = max(0, min(255, (hi - 44) * 3))
        saturation = max(0, min(255, (hi - lo - 19) * 5))
        ap[x, y] = min(strength, saturation)
alpha = alpha.filter(ImageFilter.GaussianBlur(.65))
mark = crop.convert("RGBA")
mark.putalpha(alpha)

for name, size in (("cover", (420, 143)), ("small", (115, 39))):
    image = mark.resize(size, Image.Resampling.LANCZOS)
    image.save(ASSETS / f"narcade-logo-{name}-v244.png")
    # PSP-friendly RGBA4444, little endian (GU_PSM_4444 layout).
    raw = bytearray()
    for r, g, b, a in image.get_flattened_data():
        raw += struct.pack("<H", (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12))
    (ASSETS / f"narcade-logo-{name}-v244.4444").write_bytes(raw)
