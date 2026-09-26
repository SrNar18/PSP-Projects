"""Genera out/IA/ia.ttl: por documento 18 bytes = nwords u8, bytelen u8, hash u32 x4 (primeras 4 palabras normalizadas
del titulo, 0 si no hay). Permite reordenar cientos de candidatos por titulo sin descomprimir articulos."""
import os, struct, zlib, sys, time
sys.path.insert(0,os.path.dirname(__file__))
from build_index import words, fnv
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
d=os.path.join(ROOT,'out','IA')
hdr=open(os.path.join(d,'ia.hdr'),'rb').read();magic,ndoc,nterms,nvol=struct.unpack('<4sIII',hdr)
fmt=2 if magic==b'PIA2' else 1
vols=[open(os.path.join(d,f'ia{i}.txt'),'rb') for i in range(nvol)] if fmt==2 else [open(os.path.join(d,'ia.txt'),'rb')]
doc=open(os.path.join(d,'ia.doc'),'rb');out=open(os.path.join(d,'ia.ttl'),'wb');tw=open(os.path.join(d,'ia.tw'),'wb');t0=time.time()
for i in range(ndoc):
    if fmt==2:v,off,ln=struct.unpack('<BII',doc.read(9))
    else:off,ln,_=struct.unpack('<IHB',doc.read(7));v=0
    vols[v].seek(off);blob=vols[v].read(min(ln,4096))
    dz=zlib.decompressobj();head=dz.decompress(blob,600)
    title=head.split(b'\n',1)[0].decode('utf-8','replace')
    ws=words(title)[:4];hs=[fnv(w) for w in ws]+[0]*(4-len(ws))
    nw=min(255,len(words(title)));out.write(struct.pack('<BBIIII',nw,min(255,len(title.encode('utf-8'))),*hs));tw.write(bytes([nw]))
    if i%200000==0:print(i,f'{time.time()-t0:.0f}s',flush=True)
out.close();tw.close();print('ia.ttl e ia.tw listos',ndoc)
