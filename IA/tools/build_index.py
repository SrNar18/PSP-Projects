"""PSP-IA — construye la base de conocimiento para la PSP a partir de la Wikipedia en espanol (parquet de
wikimedia/wikipedia 20231101.es).

Salida (carpeta out/IA, se copia a ms0:/IA/):
  ia.txt  articulos: por documento, zlib("titulo\\n" + resumen)            (~1,2 GB)
  ia.doc  tabla de documentos: por doc, offset u32 + longitud u16 + titlelen u8 (7 bytes)
  ia.idx  diccionario: terminos ordenados por hash u32: hash u32, offset u32 en ia.pst, df u32, npost u32
  ia.pst  postings: por termino, secuencia de (delta docid varint, peso u8)
  ia.ttl  titulos normalizados concatenados (para mostrar la lista de resultados sin descomprimir)  [opcional]

Normalizacion: minusculas, sin acentos (NFKD), solo [a-z0-9n]. Se indexan las palabras del titulo (peso 3),
las primeras 90 palabras del resumen (peso 1) y se eliminan stopwords.
Uso: venv/Scripts/python tools/build_index.py [--limit N] [--maxchars 1400]
"""
import argparse, glob, os, re, struct, sys, unicodedata, zlib, time
import numpy as np
import pyarrow.parquet as pq

ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STOP=set("""a al algo algunas algunos ante antes como con contra cual cuando de del desde donde durante e el ella ellas
ellos en entre era erais eran eras eres es esa esas ese eso esos esta estaba estaban estamos estan estar estas este esto
estos fue fueron fui fuimos ha habia habian han has hasta hay la las le les lo los mas me mi mis mucho muchos muy nada ni
no nos nosotros o os otra otras otro otros para pero poco por porque que quien quienes se sea sean segun ser si sido sin
sobre sois somos son soy su sus tambien tanto te tenia tiene tienen todo todos tu tus un una uno unos vosotros y ya yo
the of and in on at is was""".split())

def norm(s):
    s=s.lower().replace('ñ','')
    s=unicodedata.normalize('NFKD',s)
    s=''.join(c for c in s if not unicodedata.combining(c))
    s=s.replace('','ñ')
    return re.sub(r'[^a-z0-9ñ ]+',' ',s)

def words(s):
    return [w for w in norm(s).split() if len(w)>1 and w not in STOP]

def fnv(w):
    h=2166136261
    for b in w.encode('utf-8'):
        h=((h^b)*16777619)&0xffffffff
    return h

def summary(text,maxchars):
    text=text.strip()
    # quitar lineas de seccion y espacios raros
    text=re.sub(r'\n+',' ',text)
    text=re.sub(r'\s+',' ',text)
    if len(text)<=maxchars:return text
    cut=text[:maxchars]
    m=max(cut.rfind('. '),cut.rfind('.\n'))
    if m>maxchars*.5:cut=cut[:m+1]
    return cut

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--limit',type=int,default=0);ap.add_argument('--maxchars',type=int,default=1400)
    ap.add_argument('--out',default=os.path.join(ROOT,'out','IA'));a=ap.parse_args()
    os.makedirs(a.out,exist_ok=True)
    files=sorted(glob.glob(os.path.join(ROOT,'data','wiki','20231101.es','*.parquet')))
    assert files,'no hay parquet en data/wiki'
    ftxt=open(os.path.join(a.out,'ia.txt'),'wb');fdoc=open(os.path.join(a.out,'ia.doc'),'wb')
    post_chunks=[];cur_h=[];cur_d=[];cur_w=[]
    ndoc=0;offset=0;t0=time.time();skipped=0
    def flush():
        nonlocal cur_h,cur_d,cur_w
        if cur_h:
            post_chunks.append((np.array(cur_h,dtype=np.uint32),np.array(cur_d,dtype=np.uint32),np.array(cur_w,dtype=np.uint8)))
            cur_h=[];cur_d=[];cur_w=[]
    for f in files:
        pf=pq.ParquetFile(f)
        for batch in pf.iter_batches(batch_size=4096,columns=['title','text']):
            titles=batch.column('title').to_pylist();texts=batch.column('text').to_pylist()
            for title,text in zip(titles,texts):
                if a.limit and ndoc>=a.limit:break
                if not text or len(text)<200:skipped+=1;continue
                low=text[:300].lower()
                if 'puede referirse a' in low or 'puede hacer referencia a' in low or title.endswith('(desambiguación)'):skipped+=1;continue
                if title.startswith(('Anexo:','Categoría:','Plantilla:','Wikipedia:','Portal:')):skipped+=1;continue
                summ=summary(text,a.maxchars)
                blob=zlib.compress((title+'\n'+summ).encode('utf-8'),6)
                if offset+len(blob)>=0xffffffff:print('ia.txt supera 4 GB');break
                ftxt.write(blob);fdoc.write(struct.pack('<IHB',offset,len(blob),min(255,len(title.encode('utf-8')))))
                offset+=len(blob)
                seen={}
                for w in words(title):seen[w]=3
                for w in words(summ)[:160]:
                    if w not in seen:seen[w]=1
                for w,wt in seen.items():cur_h.append(fnv(w));cur_d.append(ndoc);cur_w.append(wt)
                ndoc+=1
                if len(cur_h)>=5_000_000:flush()
            if a.limit and ndoc>=a.limit:break
        print(f'{os.path.basename(f)}: {ndoc} docs, {offset/1e6:.0f} MB, {time.time()-t0:.0f}s',flush=True)
        if a.limit and ndoc>=a.limit:break
    flush();ftxt.close();fdoc.close()
    H=np.concatenate([c[0] for c in post_chunks]);D=np.concatenate([c[1] for c in post_chunks]);W=np.concatenate([c[2] for c in post_chunks])
    del post_chunks
    print('postings:',len(H),flush=True)
    order=np.lexsort((D,H));H=H[order];D=D[order];W=W[order];del order
    # diccionario + postings varint
    fidx=open(os.path.join(a.out,'ia.idx'),'wb');fpst=open(os.path.join(a.out,'ia.pst'),'wb')
    bounds=np.flatnonzero(np.diff(H))+1;starts=np.concatenate(([0],bounds));ends=np.concatenate((bounds,[len(H)]))
    poff=0;nterms=0
    out=bytearray()
    for s,e in zip(starts,ends):
        h=int(H[s]);df=e-s
        if df>600000:continue  # termino demasiado comun: no aporta
        docs=D[s:e];wts=W[s:e]
        deltas=np.empty(df,dtype=np.uint32);deltas[0]=docs[0];deltas[1:]=np.diff(docs)
        buf=bytearray()
        for dlt,wt in zip(deltas.tolist(),wts.tolist()):
            while dlt>=0x80:buf.append((dlt&0x7f)|0x80);dlt>>=7
            buf.append(dlt);buf.append(wt)
        fidx.write(struct.pack('<IIII',h,poff,df,len(buf)));fpst.write(buf);poff+=len(buf);nterms+=1
        if poff>=0xffffffff:print('ia.pst supera 4 GB');break
    fidx.close();fpst.close()
    with open(os.path.join(a.out,'ia.hdr'),'wb') as fh:fh.write(struct.pack('<4sIII',b'PIA1',ndoc,nterms,a.maxchars))
    print(f'docs {ndoc} (saltados {skipped}), terminos {nterms}, postings {poff/1e6:.0f} MB, texto {offset/1e6:.0f} MB, {time.time()-t0:.0f}s')

if __name__=='__main__':main()
