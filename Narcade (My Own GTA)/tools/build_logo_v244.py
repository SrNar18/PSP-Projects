"""Build a clean original Narcade wordmark at PSP native resolution.

The former logo keyed neon pixels out of a painted image. That process left
dark holes and isolated square artifacts. This version draws the glyph masks
from the project's OFL-licensed Oxanium face and colors every covered pixel.
"""
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
SCALE = 4
SIZE = (115, 39)
hi = (SIZE[0] * SCALE, SIZE[1] * SCALE)
face = ROOT / "tools/fonts/Oxanium.ttf"
font_size = 34 * SCALE
while True:
    font = ImageFont.truetype(str(face), font_size)
    font.set_variation_by_axes([760])
    bounds = font.getbbox("NARCADE", stroke_width=0)
    if bounds[2] - bounds[0] <= 109 * SCALE and bounds[3] - bounds[1] <= 29 * SCALE:
        break
    font_size -= 2
mask = Image.new("L", hi)
draw = ImageDraw.Draw(mask)
word = "NARCADE"
width = draw.textlength(word, font=font)
left = int((hi[0] - width) / 2)
top = 2 * SCALE - bounds[1]
draw.text((left, top), word, font=font, fill=255)
n_width = int(draw.textlength("N", font=font))
n_mask = Image.new("L", hi)
ImageDraw.Draw(n_mask).text((left, top), "N", font=font, fill=255)

image = Image.new("RGBA", hi)
src, nsrc, out = mask.load(), n_mask.load(), image.load()
for y in range(hi[1]):
    for x in range(hi[0]):
        alpha = src[x, y]
        if not alpha:
            continue
        if nsrc[x, y] and x < left + n_width + SCALE:
            color = (25, 193 + min(43, y // 8), 255)
        else:
            t = max(0, min(1, (x - left - n_width) / max(1, width - n_width)))
            color = (255, int(199 - 111 * t), int(93 + 117 * t))
        out[x, y] = (*color, alpha)

# One tapered baseline ties the colored letters together without a background.
draw = ImageDraw.Draw(image)
draw.line((left, 34*SCALE, hi[0] - 5*SCALE, 34*SCALE), fill=(35, 207, 244, 210), width=SCALE)
draw.line((hi[0] - 28*SCALE, 34*SCALE, hi[0] - 5*SCALE, 34*SCALE), fill=(243, 87, 192, 210), width=SCALE)
image = image.resize(SIZE, Image.Resampling.LANCZOS)
image.save(ASSETS / "narcade-logo-small-v244.png")
raw = bytearray()
for r, g, b, a in image.get_flattened_data():
    raw += struct.pack("<H", (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12))
(ASSETS / "narcade-logo-small-v244.4444").write_bytes(raw)
print("clean logo:", SIZE, "pixels")
