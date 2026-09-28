from pathlib import Path
from PIL import Image,ImageDraw
root=Path('.')
sheet=Image.new('RGB',(960,600),(12,23,31));draw=ImageDraw.Draw(sheet)
for i,name in enumerate(['menu-es','menu-en','settings-es','settings-en']):
 im=Image.open(root/'build'/f'{name}.ppm');sheet.paste(im,((i%2)*480,24+(i//2)*298));draw.text(((i%2)*480+12,(i//2)*298+7),name,fill='white');im.save(root/'build'/f'{name}.png')
sheet.save(root/'build/settings-preview.png')
