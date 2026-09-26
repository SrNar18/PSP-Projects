"""PSP-IA v2 — base de conocimiento con ARTICULOS COMPLETOS (formato PIA2) para Memory Sticks grandes.

Salida (out/IA):
  ia0.txt, ia1.txt, ...  volumenes (< 3,9 GB cada uno, FAT32): zlib("titulo\\n" + texto completo recortado a 120k chars)
  ia.doc   por documento: vol u8, offset u32, longitud u32 (9 bytes)
  ia.idx   diccionario ordenado por hash: hash u32, offset u32, df u32, bytes u32
  ia.pst   postings: (delta docid varint, peso u8); peso 3 titulo, 2 introduccion (primeras 60 palabras), 1 resto
  ia.hdr   'PIA2', ndoc, nterms, nvol
Indexa titulo + hasta 320 palabras distintas por articulo. Ordena por cubos de hash (16) para no agotar la RAM.
Uso: venv/Scripts/python tools/build_index_full.py [--limit N]
"""
import argparse, glob, os, re, struct, time, zlib
import numpy as np
import pyarrow.parquet as pq
from build_index import words, fnv

ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VOL_MAX=3_900_000_000
MAXCHARS=120_000
BUCKETS=16

def clean(text):
    text=re.sub(r'\n{2,}','\n',text.strip());text=re.sub(r'[ \t]+',' ',text)
    if len(text)>MAXCHARS:
        cut=text[:MAXCHARS];m=cut.rfind('. ');text=cut[:m+1] if m>MAXCHARS*.6 else cut
    return text

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--limit',type=int,default=0);ap.add_argument('--out',default=os.path.join(ROOT,'out','IA'));a=ap.parse_args()
    out=a.out;os.makedirs(out,exist_ok=True);tmp=os.path.join(out,'tmp');os.makedirs(tmp,exist_ok=True)
    files=sorted(glob.glob(os.path.join(ROOT,'data','wiki','20231101.es','*.parquet')));assert files
    vol=0;fvol=open(os.path.join(out,f'ia{vol}.txt'),'wb');voff=0
    fdoc=open(os.path.join(out,'ia.doc'),'wb')
    bfiles=[open(os.path.join(tmp,f'b{i}.bin'),'wb') for i in range(BUCKETS)]
    bufH=[[] for _ in range(BUCKETS)];bufD=[[] for _ in range(BUCKETS)];bufW=[[] for _ in range(BUCKETS)]
    def flush(force=False):
        for i in range(BUCKETS):
            if len(bufH[i])>=400_000 or (force and bufH[i]):
                arr=np.empty(len(bufH[i]),dtype=[('h','<u4'),('d','<u4'),('w','u1')]);arr['h']=bufH[i];arr['d']=bufD[i];arr['w']=bufW[i]
                bfiles[i].write(arr.tobytes());bufH[i].clear();bufD[i].clear();bufW[i].clear()
    ndoc=0;skipped=0;t0=time.time();npost=0
    for f in files:
        pf=pq.ParquetFile(f)
        for batch in pf.iter_batches(batch_size=2048,columns=['title','text']):
            for title,text in zip(batch.column('title').to_pylist(),batch.column('text').to_pylist()):
                if a.limit and ndoc>=a.limit:break
                if not text or len(text)<200:skipped+=1;continue
                low=text[:300].lower()
                if 'puede referirse a' in low or 'puede hacer referencia a' in low or title.endswith('(desambiguación)'):skipped+=1;continue
                if title.startswith(('Anexo:','Categoría:','Plantilla:','Wikipedia:','Portal:')):skipped+=1;continue
                body=clean(text);blob=zlib.compress((title+'\n'+body).encode('utf-8'),6)
                if voff+len(blob)>VOL_MAX:
                    fvol.close();vol+=1;fvol=open(os.path.join(out,f'ia{vol}.txt'),'wb');voff=0
                fvol.write(blob);fdoc.write(struct.pack('<BII',vol,voff,len(blob)));voff+=len(blob)
                seen={}
                for w in words(title):seen[w]=3
                ws=words(body)
                for w in ws[:60]:
                    if w not in seen:seen[w]=2
                for w in ws[60:]:
                    if len(seen)>=320:break
                    if w not in seen:seen[w]=1
                for w,wt in seen.items():
                    h=fnv(w);b=h>>28;bufH[b].append(h);bufD[b].append(ndoc);bufW[b].append(wt)
                npost+=len(seen);ndoc+=1
                if ndoc%2048==0:flush()
            if a.limit and ndoc>=a.limit:break
        print(f'{os.path.basename(f)}: {ndoc} docs, vol {vol} ({voff/1e9:.2f} GB), {npost/1e6:.0f}M postings, {time.time()-t0:.0f}s',flush=True)
        if a.limit and ndoc>=a.limit:break
    flush(True);fvol.close();fdoc.close();[bf.close() for bf in bfiles]
    # diccionario + postings, cubo a cubo (los cubos van en orden de hash, asi el idx queda ordenado)
    fidx=open(os.path.join(out,'ia.idx'),'wb');fpst=open(os.path.join(out,'ia.pst'),'wb');poff=0;nterms=0
    for i in range(BUCKETS):
        path=os.path.join(tmp,f'b{i}.bin');arr=np.fromfile(path,dtype=[('h','<u4'),('d','<u4'),('w','u1')])
        if len(arr)==0:continue
        arr.sort(order=['h','d'])
        H=arr['h'];D=arr['d'];W=arr['w']
        bounds=np.flatnonzero(np.diff(H))+1;starts=np.concatenate(([0],bounds));ends=np.concatenate((bounds,[len(H)]))
        for s,e in zip(starts,ends):
            df=int(e-s)
            if df>700000:continue
            docs=D[s:e].astype(np.int64);deltas=np.diff(docs,prepend=0);wts=W[s:e]
            buf=bytearray()
            for dlt,wt in zip(deltas.tolist(),wts.tolist()):
                while dlt>=0x80:buf.append((dlt&0x7f)|0x80);dlt>>=7
                buf.append(dlt);buf.append(wt)
            fidx.write(struct.pack('<IIII',int(H[s]),poff,df,len(buf)));fpst.write(buf);poff+=len(buf);nterms+=1
            if poff>=0xffffffff:raise SystemExit('ia.pst supera 4 GB')
        del arr;os.remove(path);print(f'cubo {i}: terminos {nterms}, postings {poff/1e6:.0f} MB, {time.time()-t0:.0f}s',flush=True)
    fidx.close();fpst.close();os.rmdir(tmp)
    with open(os.path.join(out,'ia.hdr'),'wb') as fh:fh.write(struct.pack('<4sIII',b'PIA2',ndoc,nterms,vol+1))
    print(f'LISTO docs {ndoc} (saltados {skipped}), terminos {nterms}, volumenes {vol+1}, {time.time()-t0:.0f}s')

if __name__=='__main__':main()
