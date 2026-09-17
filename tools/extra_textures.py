"""v2.7 (Claude): materiales 64x64 adicionales en RAM (procedurales, deterministas) para formas nuevas:
corteza, follaje, metal, hormigon con juntas, muro cortina, toldo a rayas, adoquin, fachada moderna."""
from PIL import Image, ImageDraw
import math, random

NAMES = ['bark','leaves','metal','concrete','curtain','awning','cobble','modern']

def noise(seed):
    r=random.Random(seed);return lambda: r.random()

def create(index):
    rnd=noise(1000+index);im=Image.new('RGB',(64,64));px=im.load();d=ImageDraw.Draw(im)
    if index==0:  # corteza: vetas verticales marrones
        for y in range(64):
            for x in range(64):
                v=math.sin(x*.9+math.sin(y*.15)*2)*18+math.sin(x*2.3+y*.05)*8+(rnd()-.5)*14
                px[x,y]=(max(0,min(255,int(96+v))),max(0,min(255,int(72+v*.8))),max(0,min(255,int(50+v*.6))))
    elif index==1:  # follaje: manchas de hojas claras y oscuras
        for y in range(64):
            for x in range(64):
                v=(rnd()-.5)*30
                px[x,y]=(int(70+v*.6),int(130+v),int(55+v*.5))
        for k in range(140):
            x,y=int(rnd()*64),int(rnd()*64);c=int(150+rnd()*80)
            d.ellipse((x-3,y-2,x+3,y+2),fill=(int(c*.55),c,int(c*.4)))
        for k in range(60):
            x,y=int(rnd()*64),int(rnd()*64)
            d.ellipse((x-2,y-2,x+2,y+2),fill=(40,80,35))
    elif index==2:  # metal cepillado gris
        for y in range(64):
            base=175+math.sin(y*.5)*6
            for x in range(64):
                v=base+(rnd()-.5)*22+math.sin(x*.3)*4
                px[x,y]=(int(v),int(v+2),int(v+6))
        d.rectangle((0,0,63,63),outline=(120,124,130))
    elif index==3:  # hormigon con juntas (2x2 paneles)
        for y in range(64):
            for x in range(64):
                v=185+(rnd()-.5)*20+math.sin(x*.7+y*.3)*3
                px[x,y]=(int(v),int(v),int(v-4))
        for k in (0,31,63):
            d.line((k,0,k,63),fill=(120,120,118));d.line((0,k,63,k),fill=(120,120,118))
        for k in range(40):
            x,y=int(rnd()*64),int(rnd()*64);px[x,y]=(140,140,138)
    elif index==4:  # muro cortina: cristal azul con montantes
        for y in range(64):
            for x in range(64):
                v=math.sin((x+y)*.12)*18
                px[x,y]=(int(120+v),int(160+v),int(185+v))
        for k in range(0,64,16):
            d.line((k,0,k,63),fill=(60,70,80),width=2);d.line((0,k,63,k),fill=(60,70,80),width=2)
        for k in range(0,64,16):
            d.line((k+3,3,k+13,3),fill=(220,235,245))
    elif index==5:  # toldo a rayas (rojo/blanco); el color de vertice lo tine
        for x in range(64):
            c=(235,235,230) if (x//8)%2==0 else (200,70,60)
            d.line((x,0,x,63),fill=c)
        for y in range(56,64):
            for x in range(64):
                if (x+y)%6<3:px[x,y]=(230,230,225)
    elif index==6:  # adoquin
        for y in range(64):
            for x in range(64):
                v=150+(rnd()-.5)*16
                px[x,y]=(int(v),int(v-2),int(v-8))
        for row in range(0,64,16):
            off=8 if (row//16)%2 else 0
            for col in range(-8,64,16):
                d.rounded_rectangle((col+off+1,row+1,col+off+14,row+14),radius=3,outline=(105,102,96),width=2)
    elif index==7:  # fachada moderna: paneles claros con ventanas oscuras horizontales
        for y in range(64):
            for x in range(64):
                v=205+(rnd()-.5)*10
                px[x,y]=(int(v),int(v),int(v-3))
        for row in (10,42):
            d.rectangle((4,row,59,row+14),fill=(40,52,66));d.rectangle((4,row,59,row+14),outline=(90,96,104))
            for k in range(4,60,14):d.line((k,row,k,row+14),fill=(90,96,104))
            d.line((6,row+2,57,row+2),fill=(120,150,175))
    return im
