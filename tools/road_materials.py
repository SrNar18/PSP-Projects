"""Deterministic, tileable street materials sized for the PSP's 128px atlas."""
from pathlib import Path
import math
import random
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1]

def periodic_noise(seed, cells, amplitude):
    rng = random.Random(seed)
    grid = [[rng.random() * 2 - 1 for _ in range(cells)] for _ in range(cells)]
    out = []
    for y in range(128):
        fy = y * cells / 128
        iy = int(fy); ty = (fy - iy) * (fy - iy) * (3 - 2 * (fy - iy))
        row = []
        for x in range(128):
            fx = x * cells / 128
            ix = int(fx); tx = (fx - ix) * (fx - ix) * (3 - 2 * (fx - ix))
            a = grid[iy % cells][ix % cells] * (1 - tx) + grid[iy % cells][(ix + 1) % cells] * tx
            b = grid[(iy + 1) % cells][ix % cells] * (1 - tx) + grid[(iy + 1) % cells][(ix + 1) % cells] * tx
            row.append((a * (1 - ty) + b * ty) * amplitude)
        out.append(row)
    return out

def asphalt():
    rng = random.Random(21501)
    coarse = periodic_noise(1, 4, 13)
    medium = periodic_noise(2, 16, 8)
    fine = periodic_noise(3, 64, 4)
    img = Image.new('RGB', (128, 128))
    pix = img.load()
    for y in range(128):
        for x in range(128):
            v = coarse[y][x] + medium[y][x] + fine[y][x] + rng.gauss(0, 4)
            pix[x, y] = tuple(max(0, min(255, int(base + v))) for base in (86, 88, 87))
    draw = ImageDraw.Draw(img, 'RGBA')
    for _ in range(2300):
        x, y = rng.randrange(128), rng.randrange(128)
        q = rng.randrange(18, 46)
        draw.point((x, y), fill=(q, q + 2, q + 1, rng.randrange(25, 85)))
    for _ in range(850):
        x, y = rng.randrange(128), rng.randrange(128)
        q = rng.randrange(147, 203)
        draw.point((x, y), fill=(q, q - 1, q - 4, rng.randrange(45, 105)))
    # Fine tar repairs are subtle and stop short of becoming fake lane markings.
    for j in range(3):
        y = (j * 47 + 13) % 128
        points = [((x + j * 29) % 128, y + int(2 * math.sin(x * .11 + j))) for x in range(0, 27, 3)]
        draw.line(points, fill=(27, 28, 28, 55), width=1)
    return img

def pavement():
    rng = random.Random(21502)
    broad = periodic_noise(5, 8, 8)
    fine = periodic_noise(6, 64, 3)
    img = Image.new('RGB', (128, 128))
    px = img.load()
    for y in range(128):
        for x in range(128):
            slab = ((x // 32) * 17 + (y // 32) * 11) % 7 - 3
            v = broad[y][x] + fine[y][x] + rng.gauss(0, 2.8) + slab
            edge = min(x % 32, y % 32, 31 - x % 32, 31 - y % 32)
            if edge == 0: v -= 18
            elif edge == 1: v -= 7
            px[x, y] = tuple(max(0, min(255, int(base + v))) for base in (177, 169, 154))
    return img

def create(name):
    return asphalt() if name == 'asphalt' else pavement()

if __name__ == '__main__':
    out = ROOT / 'assets'
    for name in ('asphalt', 'sidewalk'):
        create(name).save(out / f'{name}-v215.png')
