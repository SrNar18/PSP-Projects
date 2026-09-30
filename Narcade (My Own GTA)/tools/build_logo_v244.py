"""Extract the actual PSP XMB cover mark as one reusable, transparent logo.

The source is our own icon artwork. The compact mark is shared by the cover,
menu and future screens without embedding an unused 120 KB large duplicate.
"""
from pathlib import Path
import struct
from PIL import Image, ImageFilter, ImageChops

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
source = Image.open(ASSETS / "icon-source.png").convert("RGB")
# Include the full tapered N and a margin so its outline is never cut by the crop.
crop = source.crop((245, 190, 1420, 580))
alpha = Image.new("L", crop.size)
ap = alpha.load()
for y in range(crop.height):
    for x in range(crop.width):
        r, g, b = crop.getpixel((x, y))
        hi, lo = max(r, g, b), min(r, g, b)
        # Keep the painted neon; reject the dark blue sky and hillside.
        # The old low threshold turned the cropped sky into a cyan rectangle.
        strength = max(0, min(255, (hi - 92) * 5))
        saturation = max(0, min(255, (hi - lo - 35) * 5))
        ap[x, y] = min(strength, saturation)
# Only close subpixel cracks at source resolution; never dilate the 115px logo.
closed = alpha.filter(ImageFilter.MaxFilter(3)).filter(ImageFilter.MinFilter(3))
alpha = ImageChops.lighter(alpha, closed).filter(ImageFilter.GaussianBlur(.45))
mark = crop.convert("RGBA")
mark.putalpha(alpha)

for name, size in (("small", (115, 39)),):
    image = mark.resize(size, Image.Resampling.LANCZOS)
    image.save(ASSETS / f"narcade-logo-{name}-v244.png")
    # PSP-friendly RGBA4444, little endian (GU_PSM_4444 layout).
    raw = bytearray()
    for r, g, b, a in image.get_flattened_data():
        raw += struct.pack("<H", (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12))
    (ASSETS / f"narcade-logo-{name}-v244.4444").write_bytes(raw)
