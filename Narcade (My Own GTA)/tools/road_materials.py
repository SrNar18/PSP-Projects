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
    # No large cracks/spots: recognizable landmarks betray a repeating tile.
    rng = random.Random(23601)
    coarse = periodic_noise(41, 4, 2.0)
    medium = periodic_noise(42, 16, 2.8)
    fine = periodic_noise(43, 64, 2.2)
    img = Image.new('RGB', (128, 128)); pix = img.load()
    for y in range(128):
        for x in range(128):
            v = coarse[y][x] + medium[y][x] + fine[y][x] + rng.gauss(0, 2.3)
            if rng.random() < .09: v += rng.choice((-7, 8))
            pix[x,y] = tuple(max(0,min(255,round(base+v))) for base in (98,100,98))
    return img.filter(ImageFilter.GaussianBlur(.25))

def pavement():
    rng = random.Random(23602)
    broad = periodic_noise(45, 4, 3)
    fine = periodic_noise(46, 64, 1.7)
    img = Image.new('RGB', (128,128)); px = img.load()
    # Four different concrete slabs with hairline joints and bevel highlights.
    tones=((0,2),(-2,1))
    for y in range(128):
        for x in range(128):
            v=broad[y][x]+fine[y][x]+rng.gauss(0,1.5)+tones[y//64][x//64]
            edge=min(x%64,y%64,63-x%64,63-y%64)
            if edge==0: v-=13
            elif edge==1: v+=4
            px[x,y]=tuple(max(0,min(255,round(base+v))) for base in (183,177,163))
    return img

def create(name):
    return asphalt() if name == 'asphalt' else pavement()

if __name__ == '__main__':
    out = ROOT / 'assets'
    for name in ('asphalt', 'sidewalk'):
        create(name).save(out / f'{name}-v215.png')
