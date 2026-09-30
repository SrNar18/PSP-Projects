"""Original streetwear finishes. Keep existing UVs, sizes and material IDs."""
import math
import random
from PIL import Image, ImageDraw, ImageFilter
from road_materials import periodic_noise

NAMES = {'jacket', 'jacket-back', 'sleeve', 'jeans', 'skin', 'hair'}

def clamp(v):
    return max(0, min(255, round(v)))

def create(name):
    rng = random.Random(3900 + sum(map(ord, name)))
    broad = periodic_noise(27 + len(name), 4, 1)
    im = Image.new('RGB', (128, 128)); p = im.load()
    for y in range(128):
        for x in range(128):
            n = broad[y][x]; grain = rng.gauss(0, 1.5)
            if name in ('jacket', 'jacket-back', 'sleeve'):
                # Fine cotton knit; broad seam shading follows each named piece.
                edge = math.exp(-min(x,127-x)/6)
                folds = math.cos(x*.098)*math.cos(y*.049)*2.5
                base = 58 - 9*edge + folds + grain
                c = (base, base+2, base+3)
            elif name == 'jeans':
                # Washed indigo twill, pocket/stitch details on the upper thigh.
                weave = (3 if (x+y)%4==0 else -1) + grain
                worn = 9*math.exp(-((x-64)/30)**2) + n*7
                fold = 8*math.cos(y*.147+math.sin(x*.049)*1.5)*math.exp(-((y-84)/28)**2)
                c = (45+weave+worn+fold,64+weave+worn+fold,85+weave+worn+fold)
            elif name == 'skin':
                # Match the warm terracotta midtones of Nico's painted face.
                # The old beige forearms/neck looked like a second complexion.
                c = (191+n*3+grain*.4,112+n*2+grain*.3,72+n*2+grain*.3)
            elif name == 'hair':
                curls = math.sin(x*.49+math.sin(y*.39))*math.cos(y*.59+x*.21)*6
                c = (38+curls+grain,30+curls+grain,26+curls+grain)
            p[x,y]=tuple(clamp(v) for v in c)
    d=ImageDraw.Draw(im)
    if name=='jeans':
        # Side seams and double stitching follow each named trouser leg.
        for x in (5,8,119,122):
            for y in range(0,128,4):d.line((x,y,x,y+1),fill=(137,133,110))
        for x in (16,110):
            d.line((x,6,x,24),fill=(108,111,104))
            d.arc((x-14,7,x+14,42),10,80,fill=(115,116,105))
        d.line((28,10,100,10),fill=(92,102,109))
    if name in ('jacket','jacket-back'):
        d.arc((40,-13,88,17),0,180,fill=(88,92,93),width=2)
        d.line((3,118,124,118),fill=(82,87,89))
        if name=='jacket-back':
            d.text((44,28),'NARCADE',fill=(179,201,194))
        else:
            d.rounded_rectangle((72,37,100,63),radius=2,outline=(80,84,86))
            d.text((75,42),'NR',fill=(141,193,183))
    if name=='sleeve':
        d.line((0,118,127,118),fill=(84,88,90),width=2)
    return im.filter(ImageFilter.GaussianBlur(.18))
