"""Entrena un GPT diminuto a nivel de byte (UTF-8) en espanol con los resumenes de la Wikipedia, para correr en la PSP.
Arquitectura (por defecto): 6 capas, d=256, 8 cabezas, contexto 192, vocab 256 -> ~4,9 M parametros.
Uso: venv312/Scripts/python tools/train_lm.py --minutes 45
Salida: out/lm.pt (checkpoint) y out/IA/lm.bin (pesos int8 + escalas float, formato de src/chat.c)."""
import argparse, glob, os, struct, time, math, random
import numpy as np, torch, torch.nn as nn, torch.nn.functional as F
import pyarrow.parquet as pq
ROOT=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

class Block(nn.Module):
    def __init__(s,d,h,ctx):
        super().__init__();s.ln1=nn.LayerNorm(d);s.ln2=nn.LayerNorm(d);s.h=h
        s.qkv=nn.Linear(d,3*d,bias=False);s.proj=nn.Linear(d,d,bias=False)
        s.fc=nn.Linear(d,4*d,bias=False);s.fc2=nn.Linear(4*d,d,bias=False)
        s.register_buffer('mask',torch.tril(torch.ones(ctx,ctx)).view(1,1,ctx,ctx))
    def forward(s,x):
        B,T,C=x.shape;q,k,v=s.qkv(s.ln1(x)).split(C,dim=2)
        q=q.view(B,T,s.h,C//s.h).transpose(1,2);k=k.view(B,T,s.h,C//s.h).transpose(1,2);v=v.view(B,T,s.h,C//s.h).transpose(1,2)
        att=(q@k.transpose(-2,-1))/math.sqrt(C//s.h);att=att.masked_fill(s.mask[:,:,:T,:T]==0,float('-inf'));att=F.softmax(att,dim=-1)
        y=(att@v).transpose(1,2).contiguous().view(B,T,C);x=x+s.proj(y)
        return x+s.fc2(F.gelu(s.fc(s.ln2(x))))
class GPT(nn.Module):
    def __init__(s,L,d,h,ctx):
        super().__init__();s.tok=nn.Embedding(256,d);s.pos=nn.Embedding(ctx,d);s.blocks=nn.ModuleList([Block(d,h,ctx) for _ in range(L)]);s.lnf=nn.LayerNorm(d);s.head=nn.Linear(d,256,bias=False);s.ctx=ctx
    def forward(s,idx):
        B,T=idx.shape;x=s.tok(idx)+s.pos(torch.arange(T,device=idx.device))
        for b in s.blocks:x=b(x)
        return s.head(s.lnf(x))

def load_text(mb):
    files=sorted(glob.glob(os.path.join(ROOT,'data','wiki','20231101.es','*.parquet')));out=[];size=0
    for f in files:
        pf=pq.ParquetFile(f)
        for batch in pf.iter_batches(batch_size=2048,columns=['title','text']):
            for t,x in zip(batch.column('title').to_pylist(),batch.column('text').to_pylist()):
                if not x or len(x)<300 or 'puede referirse a' in x[:200].lower():continue
                s=x.replace('\n',' ')[:900];cut=s.rfind('. ');s=s[:cut+1] if cut>300 else s
                out.append(t+': '+s+'\n');size+=len(out[-1])
                if size>mb*1e6:return ''.join(out)
    return ''.join(out)

def export(model,path):
    """int8 por fila con escala float32; LayerNorm y embeddings en float32."""
    def q8(w):
        w=w.detach().float().cpu().numpy();sc=np.abs(w).max(axis=1)/127.0+1e-8;q=np.round(w/sc[:,None]).astype(np.int8);return q,sc.astype(np.float32)
    with open(path,'wb') as f:
        L=len(model.blocks);d=model.tok.weight.shape[1];h=model.blocks[0].h
        f.write(struct.pack('<4sIIII',b'PLM1',L,d,h,model.ctx))
        f.write(model.tok.weight.detach().float().cpu().numpy().astype(np.float32).tobytes())
        f.write(model.pos.weight.detach().float().cpu().numpy().astype(np.float32).tobytes())
        for b in model.blocks:
            for ln in (b.ln1,b.ln2):f.write(ln.weight.detach().float().cpu().numpy().tobytes());f.write(ln.bias.detach().float().cpu().numpy().tobytes())
            for lin in (b.qkv,b.proj,b.fc,b.fc2):q,sc=q8(lin.weight);f.write(q.tobytes());f.write(sc.tobytes())
        f.write(model.lnf.weight.detach().float().cpu().numpy().tobytes());f.write(model.lnf.bias.detach().float().cpu().numpy().tobytes())
        q,sc=q8(model.head.weight);f.write(q.tobytes());f.write(sc.tobytes())

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--minutes',type=float,default=45);ap.add_argument('--mb',type=float,default=300)
    ap.add_argument('--layers',type=int,default=6);ap.add_argument('--dim',type=int,default=256);ap.add_argument('--heads',type=int,default=8);ap.add_argument('--ctx',type=int,default=192)
    ap.add_argument('--batch',type=int,default=96);ap.add_argument('--resume',action='store_true');ap.add_argument('--data',default='');ap.add_argument('--sample',default='Medellín es ');a=ap.parse_args()
    dev='cuda' if torch.cuda.is_available() else 'cpu';print('device',dev,flush=True)
    cache=a.data if a.data else os.path.join(ROOT,'data','lm_text.bin')
    if not os.path.exists(cache):
        txt=load_text(a.mb);open(cache,'wb').write(txt.encode('utf-8'));del txt
    data=torch.frombuffer(bytearray(open(cache,'rb').read()),dtype=torch.uint8);print('bytes',len(data),flush=True)
    n=int(len(data)*.98);train,val=data[:n],data[n:]
    model=GPT(a.layers,a.dim,a.heads,a.ctx).to(dev);print('params',sum(p.numel() for p in model.parameters())/1e6,'M',flush=True)
    ck=os.path.join(ROOT,'out','lm.pt')
    if a.resume and os.path.exists(ck):model.load_state_dict(torch.load(ck))
    opt=torch.optim.AdamW(model.parameters(),lr=6e-4,betas=(0.9,0.95),weight_decay=0.1)
    def batch(src):
        ix=torch.randint(len(src)-a.ctx-1,(a.batch,));x=torch.stack([src[i:i+a.ctx] for i in ix]).long();y=torch.stack([src[i+1:i+a.ctx+1] for i in ix]).long();return x.to(dev),y.to(dev)
    t0=time.time();step=0;scaler=torch.amp.GradScaler('cuda',enabled=dev=='cuda')
    while time.time()-t0<a.minutes*60:
        lr=6e-4*min(1,step/300)*(0.15+0.85*max(0,1-(time.time()-t0)/(a.minutes*60)))
        for g in opt.param_groups:g['lr']=lr
        x,y=batch(train)
        with torch.autocast('cuda',dtype=torch.bfloat16,enabled=dev=='cuda'):loss=F.cross_entropy(model(x).view(-1,256),y.view(-1))
        opt.zero_grad(set_to_none=True);loss.backward();torch.nn.utils.clip_grad_norm_(model.parameters(),1.0);opt.step();step+=1
        if step%200==0:
            model.eval()
            with torch.no_grad():
                vx,vy=batch(val);vl=F.cross_entropy(model(vx).view(-1,256),vy.view(-1)).item()
                idx=torch.tensor([list(a.sample.encode('utf-8'))],device=dev)
                for _ in range(120):
                    logits=model(idx[:,-a.ctx:])[:,-1,:]/0.8;probs=F.softmax(logits,-1);nxt=torch.multinomial(probs,1);idx=torch.cat([idx,nxt],1)
                sample=bytes(idx[0].tolist()).decode('utf-8','replace')
            model.train()
            print(f'step {step} loss {loss.item():.3f} val {vl:.3f} lr {lr:.2e} {(time.time()-t0)/60:.1f} min | {sample[:110].encode("ascii","replace").decode()}',flush=True)
            torch.save(model.state_dict(),ck)
    torch.save(model.state_dict(),ck)
    os.makedirs(os.path.join(ROOT,'out','IA'),exist_ok=True);export(model,os.path.join(ROOT,'out','IA','lm.bin'))
    print('exportado out/IA/lm.bin',os.path.getsize(os.path.join(ROOT,'out','IA','lm.bin')),'bytes')

if __name__=='__main__':main()
