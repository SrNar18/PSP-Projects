"""Neutral metallic automotive paint mapped in shared car-local coordinates.

Doors, windows, wheel arches and handles are modelled in 3D. A photograph of
one car would put those features in the wrong places on the other six bodies.
"""
import math
import random
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter


def clamp(value):
    return max(0, min(255, round(value)))


def create(name):
    side = name == 'car-side'
    rng = random.Random(731 if side else 732)
    # Reference generated for this project: neutral clearcoat and city reflection.
    # It carries no baked doors, logos, wheels or windshield outlines. A soft
    # wrap at each edge prevents a visible seam on the PSP's repeating UVs.
    reference = Image.open(Path(__file__).resolve().parents[1] /
                           'assets/reference/automotive-clearcoat-reference-v237.png')
    reference = reference.convert('RGB').resize((128,128),Image.Resampling.LANCZOS)
    refpx = reference.load()
    image = Image.new('RGB', (128, 128))
    pixels = image.load()
    for y in range(128):
        v = y / 127
        for x in range(128):
            u = x / 127
            # Reflections are broad enough to survive the PSP's RGB565 atlas.
            skyline = 0.25 + 0.035 * math.sin(u * 17) + 0.018 * math.sin(u * 39)
            sky = 27 * math.exp(-((v - skyline) / 0.095) ** 2)
            long_glint = 38 * math.exp(-((v - (0.46 + 0.025 * math.sin(u * 8))) / 0.035) ** 2)
            soft_reflection = 13 * math.exp(-((u - 0.68 - v * 0.12) / 0.18) ** 2)
            metal = rng.gauss(0, 2.1)
            if side:
                base = 192 - 39 * v + sky + long_glint + soft_reflection
                base += 7 * math.sin(u * 12 + v * 3) * (0.3 + v)
                base -= 12 * math.exp(-((v - 0.68) / 0.08) ** 2)
                base += 11 * math.exp(-((v - 0.56) / 0.025) ** 2)
                base -= 19 * max(0, (v - 0.76) / 0.24)
            else:
                base = 188 + 31 * math.exp(-((v - 0.20 - 0.05 * math.sin(u * 7)) / 0.12) ** 2)
                base += 18 * math.exp(-((u - 0.30) / 0.15) ** 2)
                base -= 22 * math.exp(-((v - 0.75) / 0.17) ** 2)
                base += 5 * math.sin(u * 22 + v * 7)
            photo = refpx[x,y]
            luminance = .2126*photo[0]+.7152*photo[1]+.0722*photo[2]
            # Preserve the old neutral tint range so car paint colors stay intact.
            # Stronger reference reflection at the shoulder; softer on the roof.
            reflection = (luminance-145) * (0.46 if side else 0.30)
            q = clamp(base + reflection + metal)
            pixels[x, y] = (q, clamp(q + 2), clamp(q + 5))

    # Blend the raster reference at the texture edges. The body still has its
    # geometry-driven panel and glass boundaries in src/shapes.inc.
    p=image.load()
    for y in range(128):
        for k in range(5):
            a=p[k,y];b=p[127-k,y];mean=tuple((a[c]+b[c])//2 for c in range(3))
            t=(5-k)/6
            p[k,y]=tuple(clamp(a[c]*(1-t)+mean[c]*t) for c in range(3))
            p[127-k,y]=tuple(clamp(b[c]*(1-t)+mean[c]*t) for c in range(3))
    draw = ImageDraw.Draw(image)
    if side:
        # Fixed-height shoulder and sill lines, continuous over all sections.
        draw.line((0, 63, 127, 63), fill=(110, 117, 123), width=1)
        draw.line((0, 65, 127, 65), fill=(230, 234, 235), width=1)
        draw.line((0, 102, 127, 102), fill=(92, 97, 100), width=2)
        draw.line((0, 105, 127, 105), fill=(178, 185, 188), width=1)
    else:
        draw.line((0, 122, 127, 122), fill=(133, 140, 146), width=1)
    return image.filter(ImageFilter.GaussianBlur(0.22))
