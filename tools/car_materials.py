"""Neutral metallic automotive paint mapped in shared car-local coordinates.

Doors, windows, wheel arches and handles are modelled in 3D. A photograph of
one car would put those features in the wrong places on the other six bodies.
"""
import math
import random
from PIL import Image, ImageDraw, ImageFilter


def clamp(value):
    return max(0, min(255, round(value)))


def create(name):
    side = name == 'car-side'
    rng = random.Random(731 if side else 732)
    image = Image.new('RGB', (128, 128))
    pixels = image.load()
    for y in range(128):
        v = y / 127
        for x in range(128):
            u = x / 127
            # Reflections are broad enough to survive the PSP's RGB565 atlas.
            skyline = 0.25 + 0.035 * math.sin(u * 17) + 0.018 * math.sin(u * 39)
            sky = 22 * math.exp(-((v - skyline) / 0.095) ** 2)
            long_glint = 27 * math.exp(-((v - (0.46 + 0.025 * math.sin(u * 8))) / 0.035) ** 2)
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
            q = clamp(base + metal)
            pixels[x, y] = (q, clamp(q + 2), clamp(q + 5))

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
