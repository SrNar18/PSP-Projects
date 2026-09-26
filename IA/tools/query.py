"""Prototipo en Python del buscador que correra en la PSP (misma logica que src/search.c):
busqueda binaria de terminos en ia.idx, lectura de postings, acumulacion de puntuaciones, top-N, mejor frase."""
import os, re, struct, sys, zlib, math, unicodedata
sys.path.insert(0,os.path.dirname(__file__))
from build_index import words, fnv, norm, STOP

class KB:
    def __init__(self,d):
        self.hdr=struct.unpack('<4sIII',open(os.path.join(d,'ia.hdr'),'rb').read(16));self.ndoc=self.hdr[1];self.nterms=self.hdr[2];self.fmt=2 if self.hdr[0]==b'PIA2' else 1
        self.fidx=open(os.path.join(d,'ia.idx'),'rb');self.fpst=open(os.path.join(d,'ia.pst'),'rb');self.fdoc=open(os.path.join(d,'ia.doc'),'rb')
        self.vols=[open(os.path.join(d,f'ia{i}.txt'),'rb') for i in range(self.hdr[3])] if self.fmt==2 else [open(os.path.join(d,'ia.txt'),'rb')]
    def term(self,h):
        lo,hi=0,self.nterms-1
        while lo<=hi:
            mid=(lo+hi)//2;self.fidx.seek(mid*16);th,off,df,n=struct.unpack('<IIII',self.fidx.read(16))
            if th==h:return off,df,n
            if th<h:lo=mid+1
            else:hi=mid-1
        return None
    def postings(self,off,n,cap=600000):
        self.fpst.seek(off);buf=self.fpst.read(n);i=0;doc=0;out=[]
        while i<len(buf) and len(out)<cap:
            d=0;sh=0
            while True:
                b=buf[i];i+=1;d|=(b&0x7f)<<sh;sh+=7
                if b<0x80:break
            doc+=d;out.append((doc,buf[i]));i+=1
        return out
    def doc(self,i):
        if self.fmt==2:self.fdoc.seek(i*9);v,off,ln=struct.unpack('<BII',self.fdoc.read(9))
        else:self.fdoc.seek(i*7);off,ln,tl=struct.unpack('<IHB',self.fdoc.read(7));v=0
        self.vols[v].seek(off);title,_,text=zlib.decompress(self.vols[v].read(ln)).decode('utf-8').partition('\n');return title,text
    def search(self,q,k=5):
        qs=words(q);scores={}
        terms=[]
        for w in dict.fromkeys(qs):
            t=self.term(fnv(w))
            if t:terms.append((w,)+t)
        if not terms:return [],qs
        for w,off,df,n in terms:
            idf=math.log(1+self.ndoc/(df+1))
            for d,wt in self.postings(off,n):
                scores[d]=scores.get(d,0)+idf*(1.0 if wt<3 else 2.2)
        top=sorted(scores.items(),key=lambda x:-x[1])[:k]
        return top,qs

def best_sentence(text,qs):
    sents=re.split(r'(?<=[.!?])\s+',text);best=None;bs=-1
    for i,s in enumerate(sents):
        sw=set(words(s));ov=len(sw&set(qs))
        sc=ov*10-i*.3+min(len(s),160)/80
        if sc>bs:bs=sc;best=s
    return best or sents[0]

if __name__=='__main__':
    kb=KB(sys.argv[1]);
    for q in sys.argv[2:]:
        top,qs=kb.search(q)
        print('\n>>',q,qs)
        for d,sc in top:
            t,txt=kb.doc(d);print(f'  [{sc:.1f}] {t}: {best_sentence(txt,qs)[:160]}')
