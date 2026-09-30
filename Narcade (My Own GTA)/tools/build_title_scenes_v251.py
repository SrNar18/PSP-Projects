"""Prepare the two extra, text-free title scenes for Claude's carousel.

The original 1800px PNGs remain editable sources. The 480x272 PNGs are PSP
previews and the .565 files are little-endian RGB565 frames for assets.S.
"""
from pathlib import Path
import struct
from PIL import Image, ImageOps

assets = Path(__file__).resolve().parents[1] / "assets"
for scene in ("car", "parques"):
    stem = f"title-{scene}-barrio-v251"
    image = ImageOps.fit(Image.open(assets / f"{stem}-source.png").convert("RGB"),
                         (480, 272), method=Image.Resampling.LANCZOS)
    # A soft native-resolution shade keeps the shared white Start prompt
    # readable across the table and sidewalk without a generic opaque panel.
    pixels = image.load()
    for y in range(177, 248):
        for x in range(0, 231):
            strength = .36 * (1 - x / 231) * min((y - 177) / 28, 1)
            r, g, b = pixels[x, y]
            pixels[x, y] = (round(r * (1 - strength) + 5 * strength),
                            round(g * (1 - strength) + 12 * strength),
                            round(b * (1 - strength) + 23 * strength))
    image.save(assets / f"{stem}.png")
    raw = bytearray()
    for r, g, b in image.getdata():
        raw += struct.pack("<H", (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11))
    assert len(raw) == 480 * 272 * 2
    (assets / f"{stem}.565").write_bytes(raw)
    print(stem, len(raw))
