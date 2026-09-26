"""v2.7 (Claude): materiales 64x64 adicionales en RAM (procedurales, deterministas) para formas nuevas:
corteza, follaje, metal, hormigon con juntas, muro cortina, toldo a rayas, adoquin, fachada moderna."""
from PIL import Image, ImageDraw
import math, random

NAMES = ['bark','leaves','metal','concrete','curtain','awning','cobble','modern',
         'retail','eatery','office-front','workshop-front']

def shop_sheet(index):
    names={8:('MUSA','NARES','SOL','PATIO'),9:('MIGA','SOMA','BARRA','CAFE'),11:('RUTA','MOTOR','AZUL','NODO')}[index]
    backgrounds={8:(38,126,125),9:(162,77,56),11:(55,106,155)}
    im=Image.new('RGB',(64,64));d=ImageDraw.Draw(im)
    for slot,name in enumerate(names):
        x=(slot%2)*32;y=(slot//2)*32;accent=backgrounds[index]
        d.rectangle((x,y,x+31,y+31),fill=(207,201,177))
        d.rectangle((x,y,x+31,y+10),fill=accent)
        d.text((x+max(1,(32-len(name)*6)//2),y+1),name,fill=(252,243,204))
        if index==11:
            d.rectangle((x+3,y+13,x+28,y+30),fill=(76,91,106),outline=(235,217,174))
            for line in range(y+16,y+29,4):d.line((x+5,line,x+26,line),fill=(133,152,161))
        else:
            d.rectangle((x+2,y+13,x+21,y+29),fill=(41,78,88),outline=(237,218,169))
            d.polygon(((x+4,y+16),(x+18,y+14),(x+8,y+25)),fill=(112,169,183))
            d.rectangle((x+23,y+13,x+30,y+30),fill=(57,72,75),outline=(236,211,166))
    return im

def noise(seed):
    r=random.Random(seed);return lambda: r.random()

def create(index):
    if index in (8,9,11):return shop_sheet(index)
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
    elif index==8:  # local de barrio: banda de color, escaparate y puerta
        d.rectangle((0,0,63,63),fill=(200,224,214))
        d.rectangle((0,0,63,15),fill=(22,111,119));d.text((8,2),'TIENDA',fill=(244,247,223))
        d.rectangle((3,19,44,55),fill=(34,61,72),outline=(235,218,171),width=2)
        d.polygon([(6,25),(30,20),(39,21),(13,37)],fill=(100,166,185))
        d.rectangle((47,18,61,62),fill=(58,75,76),outline=(239,209,150),width=2)
        d.ellipse((56,39,58,41),fill=(248,203,98))
        d.rectangle((0,56,63,63),fill=(39,119,127))
    elif index==9:  # restaurante: azulejo terracota y ventanas iluminadas
        d.rectangle((0,0,63,63),fill=(227,181,133))
        d.rectangle((0,0,63,16),fill=(153,49,43));d.text((13,2),'SABOR',fill=(255,233,173))
        d.rectangle((4,20,59,55),fill=(92,50,44),outline=(245,208,142),width=2)
        for x in (13,31,49):
            d.rectangle((x-7,25,x+5,49),fill=(220,137,70));d.line((x-7,25,x+5,49),fill=(250,190,103))
        d.rectangle((0,56,63,63),fill=(136,66,51))
    elif index==10:  # oficina: montantes, reflejos y plantas acristaladas
        d.rectangle((0,0,63,63),fill=(44,78,108))
        for y in (1,32):
            for x in (1,32):
                d.rectangle((x,y,x+29,y+29),fill=(56,109,142),outline=(184,207,211),width=2)
                d.polygon([(x+3,y+4),(x+18,y+3),(x+7,y+22)],fill=(102,158,183))
                d.rectangle((x+21,y+7,x+25,y+16),fill=(236,192,111))
        d.rectangle((0,29,63,33),fill=(32,53,72))
    elif index==11:  # taller: porton metalico y pintura viva
        d.rectangle((0,0,63,63),fill=(226,194,128))
        d.rectangle((0,0,63,14),fill=(43,91,145));d.text((9,2),'TALLER',fill=(253,232,154))
        d.rectangle((5,18,58,61),fill=(94,115,130),outline=(40,57,70),width=3)
        for y in range(22,59,6):d.line((8,y,55,y),fill=(156,177,184),width=2)
        d.rectangle((0,60,63,63),fill=(38,78,125))
    return im
