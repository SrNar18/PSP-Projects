"""Neutral metallic paint; doors and windows are modelled in geometry."""
import random
from PIL import Image

def create(name):
    rng=random.Random(315 if name=='car-side' else 316)
    image=Image.new('RGB',(128,128));pix=image.load()
    for y in range(128):
        for x in range(128):
            grain=rng.gauss(0,2.5)
            sheen=3.5*(1-abs((y%128)-64)/64)
            shade=int(205+grain+sheen)
            pix[x,y]=(shade,shade,shade)
    return image
