/* chat.c — inferencia de un GPT diminuto a nivel de byte (formato PLM1 de tools/train_lm.py) en la PSP.
 * Pesos lineales int8 con escala por fila; activaciones float. Cache K/V para generar token a token. */
#include "chat.h"
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

typedef struct {const signed char *q;const float *sc;int out,in;} Lin;
typedef struct {const float *ln1w,*ln1b,*ln2w,*ln2b;Lin qkv,proj,fc,fc2;} Blk;
static int L=0,D=0,HH=0,CTX=0;static unsigned char *blob=0;
static const float *tok,*pos,*lnfw,*lnfb;static Blk *blk;static Lin head;
static float *kc,*vc; /* cache [L][CTX][D] */
static float *x,*xb,*q,*k,*v,*att,*hbuf,*logits;

static const unsigned char *rd(const unsigned char **p,unsigned n){const unsigned char *r=*p;*p+=n;return r;}
static void lin_init(Lin *l,const unsigned char **p,int out,int in){l->q=(const signed char*)rd(p,(unsigned)out*in);l->sc=(const float*)rd(p,out*4);l->out=out;l->in=in;}
int lm_load(const char *path){
    SceUID f=sceIoOpen(path,PSP_O_RDONLY,0);if(f<0)return 0;
    SceOff size=sceIoLseek(f,0,PSP_SEEK_END);sceIoLseek(f,0,PSP_SEEK_SET);
    blob=(unsigned char*)malloc((size_t)size);if(!blob){sceIoClose(f);return 0;}
    unsigned got=0;while(got<(unsigned)size){int n=sceIoRead(f,blob+got,(unsigned)size-got>1<<20?1<<20:(unsigned)size-got);if(n<=0)break;got+=n;}
    sceIoClose(f);if(got!=(unsigned)size||memcmp(blob,"PLM1",4))return 0;
    const unsigned char *p=blob+4;unsigned hdr[4];memcpy(hdr,p,16);p+=16;L=hdr[0];D=hdr[1];HH=hdr[2];CTX=hdr[3];
    tok=(const float*)rd(&p,256*D*4);pos=(const float*)rd(&p,CTX*D*4);
    blk=(Blk*)calloc(L,sizeof(Blk));
    for(int i=0;i<L;i++){Blk *b=&blk[i];
        b->ln1w=(const float*)rd(&p,D*4);b->ln1b=(const float*)rd(&p,D*4);b->ln2w=(const float*)rd(&p,D*4);b->ln2b=(const float*)rd(&p,D*4);
        lin_init(&b->qkv,&p,3*D,D);lin_init(&b->proj,&p,D,D);lin_init(&b->fc,&p,4*D,D);lin_init(&b->fc2,&p,D,4*D);}
    lnfw=(const float*)rd(&p,D*4);lnfb=(const float*)rd(&p,D*4);lin_init(&head,&p,256,D);
    kc=(float*)malloc(sizeof(float)*L*CTX*D);vc=(float*)malloc(sizeof(float)*L*CTX*D);
    x=malloc(D*4);xb=malloc(D*4);q=malloc(3*D*4);k=q+D;v=q+2*D;att=malloc(CTX*4);hbuf=malloc(4*D*4);logits=malloc(256*4);
    return kc&&vc&&x&&xb&&q&&att&&hbuf&&logits;
}
int lm_ready(void){return blob!=0&&L>0;}
static void layernorm(float *out,const float *in,const float *w,const float *b){
    float mean=0,var=0;for(int i=0;i<D;i++)mean+=in[i];mean/=D;
    for(int i=0;i<D;i++){float d=in[i]-mean;var+=d*d;}var/=D;float inv=1.0f/sqrtf(var+1e-5f);
    for(int i=0;i<D;i++)out[i]=(in[i]-mean)*inv*w[i]+b[i];
}
static void matmul(float *out,const float *in,const Lin *l){
    for(int o=0;o<l->out;o++){const signed char *row=l->q+(size_t)o*l->in;float s0=0,s1=0,s2=0,s3=0;int i=0;
        for(;i+3<l->in;i+=4){s0+=row[i]*in[i];s1+=row[i+1]*in[i+1];s2+=row[i+2]*in[i+2];s3+=row[i+3]*in[i+3];}
        for(;i<l->in;i++)s0+=row[i]*in[i];
        out[o]=(s0+s1+s2+s3)*l->sc[o];}
}
/* procesa el token 'tk' en la posicion 'pos' (0..CTX-1) y deja logits */
static void forward(int tk,int position){
    for(int i=0;i<D;i++)x[i]=tok[tk*D+i]+pos[position*D+i];
    int hd=D/HH;
    for(int li=0;li<L;li++){Blk *b=&blk[li];
        layernorm(xb,x,b->ln1w,b->ln1b);matmul(q,xb,&b->qkv);
        float *kcl=kc+(size_t)li*CTX*D,*vcl=vc+(size_t)li*CTX*D;
        memcpy(kcl+position*D,k,D*4);memcpy(vcl+position*D,v,D*4);
        for(int h=0;h<HH;h++){const float *qh=q+h*hd;float mx=-1e30f;
            for(int t=0;t<=position;t++){const float *kh=kcl+t*D+h*hd;float s=0;for(int i=0;i<hd;i++)s+=qh[i]*kh[i];s/=sqrtf((float)hd);att[t]=s;if(s>mx)mx=s;}
            float sum=0;for(int t=0;t<=position;t++){att[t]=expf(att[t]-mx);sum+=att[t];}
            float *oh=hbuf+h*hd;for(int i=0;i<hd;i++)oh[i]=0;
            for(int t=0;t<=position;t++){float w=att[t]/sum;const float *vh=vcl+t*D+h*hd;for(int i=0;i<hd;i++)oh[i]+=w*vh[i];}
        }
        matmul(xb,hbuf,&b->proj);for(int i=0;i<D;i++)x[i]+=xb[i];
        layernorm(xb,x,b->ln2w,b->ln2b);matmul(hbuf,xb,&b->fc);
        for(int i=0;i<4*D;i++){float z=hbuf[i];hbuf[i]=0.5f*z*(1+tanhf(0.7978845608f*(z+0.044715f*z*z*z)));}
        matmul(xb,hbuf,&b->fc2);for(int i=0;i<D;i++)x[i]+=xb[i];
    }
    layernorm(xb,x,lnfw,lnfb);matmul(logits,xb,&head);
}
static unsigned rng=12345u;static float frand(void){rng=rng*1664525u+1013904223u;return (rng>>8)/16777216.0f;}
static int sample(float temp,float topp){
    float mx=-1e30f;for(int i=0;i<256;i++){logits[i]/=temp;if(logits[i]>mx)mx=logits[i];}
    float sum=0;for(int i=0;i<256;i++){logits[i]=expf(logits[i]-mx);sum+=logits[i];}
    for(int i=0;i<256;i++)logits[i]/=sum;
    /* top-p: ordenar indices por probabilidad (256 valores: seleccion simple) */
    int idx[256];for(int i=0;i<256;i++)idx[i]=i;
    for(int i=1;i<256;i++){int j=i;while(j>0&&logits[idx[j-1]]<logits[idx[j]]){int t=idx[j];idx[j]=idx[j-1];idx[j-1]=t;j--;}}
    float acc=0,cut=0;int n=256;for(int i=0;i<256;i++){acc+=logits[idx[i]];if(acc>=topp){n=i+1;cut=acc;break;}}
    float r=frand()*cut;acc=0;for(int i=0;i<n;i++){acc+=logits[idx[i]];if(r<=acc)return idx[i];}
    return idx[0];
}
/* Genera hasta 'maxBytes' de continuacion de 'prompt' (UTF-8). Llama a 'tick' cada token para refrescar pantalla. */
int lm_generate(const char *prompt,char *out,int cap,int maxBytes,float temp,void (*tick)(const char *partial)){
    if(!lm_ready())return 0;
    int n=(int)strlen(prompt);if(n>CTX-8){prompt+=n-(CTX-8);n=CTX-8;}
    int position=0;
    for(int i=0;i<n;i++){forward((unsigned char)prompt[i],position);position++;}
    int o=0;rng^=(unsigned)sceKernelGetSystemTimeLow();
    while(o<maxBytes&&o<cap-1&&position<CTX){
        int t=sample(temp,0.9f);
        if(t=='\n'){if(o>0)break;else continue;}  /* formato de dialogo: la respuesta termina en el salto de linea */
        out[o++]=(char)t;out[o]=0;
        if(tick)tick(out);
        if(o>=maxBytes-1)break;
        forward(t,position);position++;
    }
    /* cortar en byte UTF-8 completo y en fin de frase si es posible */
    if(o>0){int st=o-1;while(st>0&&((unsigned char)out[st]&0xc0)==0x80)st--;unsigned c=(unsigned char)out[st];int need=c<0x80?1:(c&0xe0)==0xc0?2:(c&0xf0)==0xe0?3:4;if(o-st<need)o=st;}
    out[o]=0;
    for(int i=o-1;i>o/2&&i>0;i--)if(out[i]=='.'){out[i+1]=0;break;}
    return o;
}
