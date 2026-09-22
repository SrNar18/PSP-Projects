"""Metallic body and pressed side panel; windows remain real geometry."""
import random
import math
from PIL import Image, ImageDraw

def create(name):
    side=name=='car-side'
    rng=random.Random(315 if side else 316)
    image=Image.new('RGB',(128,128));pix=image.load()
    for y in range(128):
        for x in range(128):
            fleck=rng.gauss(0,3.8)
            broad=5*math.sin(x*.045)+3*math.sin((x+y)*.10)
            reflection=11*math.exp(-((y-28)/17)**2)+5*math.exp(-((y-54)/9)**2)
            shade=202+broad+reflection+fleck
            if side:
                shade+=4*math.sin(x*.085)
                shade-=14*max(0,(y-88)/40)
                if 71<=y<=72:shade-=24
                if 73<=y<=75:shade+=11
                if abs(x-63)<=1 and y>38:shade-=10
            else:
                if 100<y<103:shade-=6
            q=max(0,min(255,int(shade)))
            pix[x,y]=(q,q,min(255,q+2))
    if side:
        d=ImageDraw.Draw(image)
        for cx in (28,99):
            d.arc((cx-22,72,cx+22,116),180,360,fill=(139,143,146),width=3)
            d.arc((cx-20,74,cx+20,114),180,360,fill=(229,232,233),width=1)
        d.line((0,109,127,109),fill=(159,163,164),width=2)
        d.line((0,112,127,112),fill=(222,224,223),width=1)
    return image
