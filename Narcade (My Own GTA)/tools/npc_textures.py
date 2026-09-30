"""Original, deterministic 64px fabric and face materials for PSP pedestrians."""
from PIL import Image, ImageDraw
import math
from pathlib import Path

NAMES = ['npc-tee', 'npc-knit', 'npc-denim', 'npc-floral',
         'npc-sport', 'npc-plaid', 'npc-face-woman', 'npc-face-man']

def create(index):
    if index >= 6:
        source = 'npc-face-woman-painted-v247.png' if index == 6 else 'npc-face-man-painted-v247.png'
        art = Image.open(Path(__file__).resolve().parents[1] / 'assets' / source).convert('RGB').resize((64,64),Image.Resampling.LANCZOS)
        # The renderer tints one face material across several complexions.
        # Preserve painted features while normalizing each source's albedo.
        pixels = list(art.get_flattened_data())
        mean = [sum(p[c] for p in pixels)/len(pixels) for c in range(3)]
        art.putdata([tuple(max(0,min(255,round(175+(p[c]-mean[c])*.70))) for c in range(3)) for p in pixels])
        return art
    bases = [(192,102,62),(197,175,134),(78,116,144),(119,65,110),
             (70,134,129),(160,164,174),(239,225,211),(239,225,211)]
    im=Image.new('RGB',(64,64));pixels=im.load()
    for y in range(64):
        for x in range(64):
            weave=((x+2*y)%3-1)*3 if index<6 else 0
            folds=math.cos(x*.29)*5 if index<6 else -abs(x-32)*.18
            pixels[x,y]=tuple(max(0,min(255,int(c+weave+folds))) for c in bases[index])
    d=ImageDraw.Draw(im)
    if index==0:
        d.line((4,4,60,4),fill=(220,148,104),width=2)
        d.rectangle((24,18,40,30),outline=(232,207,149),width=2)
        d.line((26,28,32,21,38,28),fill=(234,218,180),width=2)
    elif index==1:
        for x in range(3,64,6):d.line((x,0,x,63),fill=(161,140,104))
        for y in range(8,64,16):d.line((0,y,63,y),fill=(220,207,174),width=3)
    elif index==2:
        d.line((31,0,31,63),fill=(198,171,110),width=2)
        for x in (9,40):d.rectangle((x,18,x+15,31),outline=(179,164,125))
        for y in range(8,64,10):d.ellipse((30,y,33,y+3),fill=(191,191,180))
    elif index==3:
        for y in range(8,64,16):
            for x in range(8,64,16):
                d.ellipse((x-4,y-2,x+4,y+2),fill=(230,190,152))
                d.ellipse((x-2,y-4,x+2,y+4),fill=(230,190,152))
                d.ellipse((x-1,y-1,x+1,y+1),fill=(250,216,87))
    elif index==4:
        d.polygon([(0,10),(63,28),(63,34),(0,16)],fill=(216,219,205))
        d.line((31,0,31,63),fill=(34,75,78),width=2)
    elif index==5:
        for a in range(5,64,16):
            d.rectangle((a,0,a+4,63),fill=(68,77,96))
            d.rectangle((0,a,63,a+4),fill=(88,91,106))
            d.line((a+7,0,a+7,63),fill=(198,137,121))
    else:
        # Neutral albedo; renderer supplies a consistent face/neck skin tint.
        for x in (20,44):
            d.line((x-6,23,x+5,22),fill=(73,62,54),width=2 if index==6 else 3)
            d.ellipse((x-5,27,x+5,31),fill=(224,222,210))
            d.ellipse((x-2,27,x+2,31),fill=(57,49,42))
        d.line((32,30,29,41,34,41),fill=(191,170,151),width=2)
        d.line((25,48,32,49,39,48),fill=(153,108,99),width=2)
        if index==7:
            for x in range(20,46,3):d.point((x,54+(x%4)),fill=(155,143,129))
    return im
