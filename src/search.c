/* search.c — motor de busqueda de PSP-IA sobre la base ms0:/IA (ver tools/build_index.py para el formato).
 * Todo desde disco con seeks: la PSP tiene 64 MB y el diccionario ocupa decenas de MB. */
#include "search.h"
#include <pspkernel.h>
#include <pspiofilemgr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <zlib.h>

static SceUID fidx=-1,fpst=-1,fdoc=-1,ftxt=-1;
static unsigned ndoc=0,nterms=0;
static char base[64];
#define HASH_SIZE 262144
static unsigned hdoc[HASH_SIZE];static float hsc[HASH_SIZE];static unsigned char hused[HASH_SIZE/8];
static unsigned char *pbuf=0;static unsigned pcap=0;

int kb_open(const char *dir){
    char p[128];snprintf(base,sizeof(base),"%s",dir);
    snprintf(p,sizeof(p),"%s/ia.hdr",dir);SceUID f=sceIoOpen(p,PSP_O_RDONLY,0);if(f<0)return 0;
    unsigned char h[16];int n=sceIoRead(f,h,16);sceIoClose(f);if(n!=16||memcmp(h,"PIA1",4))return 0;
    memcpy(&ndoc,h+4,4);memcpy(&nterms,h+8,4);
    snprintf(p,sizeof(p),"%s/ia.idx",dir);fidx=sceIoOpen(p,PSP_O_RDONLY,0);
    snprintf(p,sizeof(p),"%s/ia.pst",dir);fpst=sceIoOpen(p,PSP_O_RDONLY,0);
    snprintf(p,sizeof(p),"%s/ia.doc",dir);fdoc=sceIoOpen(p,PSP_O_RDONLY,0);
    snprintf(p,sizeof(p),"%s/ia.txt",dir);ftxt=sceIoOpen(p,PSP_O_RDONLY,0);
    if(fidx<0||fpst<0||fdoc<0||ftxt<0)return 0;
    pcap=4<<20;pbuf=(unsigned char*)malloc(pcap);
    return pbuf!=0;
}
unsigned kb_docs(void){return ndoc;}
unsigned kb_terms(void){return nterms;}

/* --- normalizacion identica a tools/build_index.py: minusculas, sin acentos, [a-z0-9ñ], 'ñ' se codifica como '~' --- */
static const char *STOP[]={"a","al","algo","algunas","algunos","ante","antes","como","con","contra","cual","cuando","de","del","desde","donde","durante","e","el","ella","ellas","ellos","en","entre","era","erais","eran","eras","eres","es","esa","esas","ese","eso","esos","esta","estaba","estaban","estamos","estan","estar","estas","este","esto","estos","fue","fueron","fui","fuimos","ha","habia","habian","han","has","hasta","hay","la","las","le","les","lo","los","mas","me","mi","mis","mucho","muchos","muy","nada","ni","no","nos","nosotros","o","os","otra","otras","otro","otros","para","pero","poco","por","porque","que","quien","quienes","se","sea","sean","segun","ser","si","sido","sin","sobre","sois","somos","son","soy","su","sus","tambien","tanto","te","tenia","tiene","tienen","todo","todos","tu","tus","un","una","uno","unos","vosotros","y","ya","yo","the","of","and","in","on","at","is","was",0};
static int is_stop(const char *w){for(int i=0;STOP[i];i++)if(!strcmp(w,STOP[i]))return 1;return 0;}
/* decodifica un punto UTF-8 */
static unsigned utf8_next(const char **s){
    const unsigned char *p=(const unsigned char*)*s;unsigned c=*p;
    if(c<0x80){*s+=1;return c;}
    if((c&0xe0)==0xc0&&p[1]){*s+=2;return ((c&0x1f)<<6)|(p[1]&0x3f);}
    if((c&0xf0)==0xe0&&p[1]&&p[2]){*s+=3;return ((c&0x0f)<<12)|((p[1]&0x3f)<<6)|(p[2]&0x3f);}
    if((c&0xf8)==0xf0&&p[1]&&p[2]&&p[3]){*s+=4;return ((c&0x07)<<18)|((p[1]&0x3f)<<12)|((p[2]&0x3f)<<6)|(p[3]&0x3f);}
    *s+=1;return '?';
}
/* letra normalizada: 'a'..'z','0'..'9','~'(ñ) o ' ' */
static char norm_char(unsigned c){
    if(c>='A'&&c<='Z')return c+32;
    if((c>='a'&&c<='z')||(c>='0'&&c<='9'))return c;
    switch(c){
        case 0xe1:case 0xe0:case 0xe4:case 0xe2:case 0xc1:case 0xc0:case 0xc4:case 0xc2:return 'a';
        case 0xe9:case 0xe8:case 0xeb:case 0xea:case 0xc9:case 0xc8:case 0xcb:case 0xca:return 'e';
        case 0xed:case 0xec:case 0xef:case 0xee:case 0xcd:case 0xcc:case 0xcf:case 0xce:return 'i';
        case 0xf3:case 0xf2:case 0xf6:case 0xf4:case 0xd3:case 0xd2:case 0xd6:case 0xd4:return 'o';
        case 0xfa:case 0xf9:case 0xfc:case 0xfb:case 0xda:case 0xd9:case 0xdc:case 0xdb:return 'u';
        case 0xf1:case 0xd1:return '~';
        case 0xe7:case 0xc7:return 'c';
        default:return ' ';
    }
}
/* FNV-1a de la palabra normalizada codificada como UTF-8 (la ñ vuelve a ser 0xC3 0xB1) */
static unsigned fnv_word(const char *w){
    unsigned h=2166136261u;
    for(;*w;w++){
        if(*w=='~'){h=(h^0xc3)*16777619u;h=(h^0xb1)*16777619u;}
        else h=(h^(unsigned char)*w)*16777619u;
    }
    return h;
}
int kb_words(const char *text,char out[][24],int max){
    int n=0;char cur[24];int cl=0;const char *s=text;
    for(;;){
        unsigned c=*s?utf8_next(&s):0;char nc=c?norm_char(c):' ';
        if(nc==' '){
            if(cl>0){cur[cl]=0;if(cl>1&&!is_stop(cur)&&n<max){int dup=0;for(int i=0;i<n;i++)if(!strcmp(out[i],cur))dup=1;if(!dup){strcpy(out[n],cur);n++;}}cl=0;}
            if(!c)break;
        }else if(cl<23)cur[cl++]=nc;
    }
    return n;
}
static int find_term(unsigned h,unsigned *off,unsigned *df,unsigned *len){
    int lo=0,hi=(int)nterms-1;unsigned rec[4];
    while(lo<=hi){
        int mid=(lo+hi)/2;
        sceIoLseek(fidx,(SceOff)mid*16,PSP_SEEK_SET);if(sceIoRead(fidx,rec,16)!=16)return 0;
        if(rec[0]==h){*off=rec[1];*df=rec[2];*len=rec[3];return 1;}
        if(rec[0]<h)lo=mid+1;else hi=mid-1;
    }
    return 0;
}
static void acc(unsigned doc,float s){
    unsigned i=(doc*2654435761u)>>14;
    for(int k=0;k<64;k++){unsigned j=(i+k)&(HASH_SIZE-1);
        if(!(hused[j>>3]&(1<<(j&7)))){hused[j>>3]|=1<<(j&7);hdoc[j]=doc;hsc[j]=s;return;}
        if(hdoc[j]==doc){hsc[j]+=s;return;}
    }
}
int kb_search(const char *query,KbHit *hits,int max,char qwords[][24],int *nq){
    char w[12][24];int n=kb_words(query,w,12);*nq=n;for(int i=0;i<n;i++)strcpy(qwords[i],w[i]);
    memset(hused,0,sizeof(hused));
    int found=0;
    for(int i=0;i<n;i++){
        unsigned off,df,len;if(!find_term(fnv_word(w[i]),&off,&df,&len))continue;found++;
        float idf=logf(1+(float)ndoc/(df+1));
        unsigned take=len<pcap?len:pcap;
        sceIoLseek(fpst,(SceOff)off,PSP_SEEK_SET);int got=sceIoRead(fpst,pbuf,take);if(got<=0)continue;
        unsigned p=0,doc=0,cnt=0;
        while(p<(unsigned)got&&cnt<250000){
            unsigned d=0,sh=0;unsigned char b;
            do{b=pbuf[p++];d|=(b&0x7f)<<sh;sh+=7;}while((b&0x80)&&p<(unsigned)got);
            if(p>=(unsigned)got)break;
            unsigned wt=pbuf[p++];doc+=d;cnt++;
            acc(doc,idf*(wt>=3?2.2f:1.0f));
        }
    }
    if(!found)return 0;
    int nh=0;
    for(unsigned j=0;j<HASH_SIZE;j++){
        if(!(hused[j>>3]&(1<<(j&7))))continue;
        float s=hsc[j];
        if(nh<max||s>hits[nh-1].score){
            int k=nh<max?nh:max-1;
            while(k>0&&hits[k-1].score<s){hits[k]=hits[k-1];k--;}
            hits[k].doc=hdoc[j];hits[k].score=s;if(nh<max)nh++;
        }
    }
    return nh;
}
int kb_doc(unsigned doc,char *title,int tcap,char *text,int cap){
    unsigned char rec[7];
    sceIoLseek(fdoc,(SceOff)doc*7,PSP_SEEK_SET);if(sceIoRead(fdoc,rec,7)!=7)return 0;
    unsigned off;unsigned short len;memcpy(&off,rec,4);memcpy(&len,rec+4,2);
    static unsigned char zbuf[65536];
    sceIoLseek(ftxt,(SceOff)off,PSP_SEEK_SET);int got=sceIoRead(ftxt,zbuf,len);if(got!=len)return 0;
    static char plain[8192];uLongf outLen=sizeof(plain)-1;
    if(uncompress((Bytef*)plain,&outLen,zbuf,len)!=Z_OK)return 0;
    plain[outLen]=0;
    char *nl=strchr(plain,'\n');if(!nl)return 0;*nl=0;
    snprintf(title,tcap,"%s",plain);snprintf(text,cap,"%s",nl+1);
    return 1;
}
/* Mejor frase: la que comparte mas palabras con la pregunta; a igualdad, la mas temprana. */
int kb_best_sentence(const char *text,char qwords[][24],int nq,char *out,int cap){
    const char *s=text;int idx=0;float best=-1e9f;const char *bs=text;int bl=0;
    while(*s){
        const char *e=s;
        while(*e&&!((*e=='.'||*e=='!'||*e=='?')&&(e[1]==' '||e[1]==0)))e++;
        if(*e)e++;
        int len=(int)(e-s);if(len>3){
            char tmp[1024];int l=len<1023?len:1023;memcpy(tmp,s,l);tmp[l]=0;
            char sw[24][24];int nsw=kb_words(tmp,sw,24);int ov=0;
            for(int i=0;i<nq;i++)for(int j=0;j<nsw;j++)if(!strcmp(qwords[i],sw[j])){ov++;break;}
            float sc=ov*10.f-idx*.3f+(len<160?len:160)/80.f;
            if(sc>best){best=sc;bs=s;bl=len;}
        }
        idx++;while(*e==' ')e++;s=e;
        if(idx>40)break;
    }
    if(bl<=0){snprintf(out,cap,"%s",text);return 0;}
    int l=bl<cap-1?bl:cap-1;memcpy(out,bs,l);out[l]=0;return 1;
}
