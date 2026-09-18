/* PSP-IA — asistente de conocimiento offline para PSP (Claude, 2026).
 * Pregunta con el teclado de la PSP y responde con la Wikipedia en espanol guardada en ms0:/IA (1,9 millones de
 * articulos). Dibujo por CPU en el framebuffer 8888 (reglas de la E-1000: SetFrameBuf NEXTFRAME + WaitVblank; los
 * dialogos del sistema dibujan en el buffer de dibujo del GE, que se apunta al visible mientras estan abiertos). */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspge.h>
#include <psputility.h>
#include <psppower.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "font.h"
#include "search.h"
#include "chat.h"
#include "testquery.h"

PSP_MODULE_INFO("PSP-IA",0,1,0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(15360); /* particion de usuario de 24 MB: mas de ~18 MB de heap falla en malloc */

#define W 480
#define H 272
#define STRIDE 512
#define RGB(r,g,b) (0xff000000u|((b)<<16)|((g)<<8)|(r))
static const unsigned BG=RGB(18,22,30),PANEL=RGB(30,36,48),INK=RGB(12,14,20),WHITE=RGB(236,238,242),MUTED=RGB(150,158,172),ACC=RGB(120,220,170),ACC2=RGB(255,200,90),USER=RGB(110,170,255),BAD=RGB(255,110,100);
static unsigned *fb;
static unsigned int __attribute__((aligned(16))) gulist[4096];
static int running=1;

static int exit_cb(int a1,int a2,void *c){(void)a1;(void)a2;(void)c;running=0;sceKernelExitGame();return 0;} /* salir SIEMPRE, aunque el bucle este en un dialogo */
static int cb_thread(SceSize a,void *p){(void)a;(void)p;int cb=sceKernelCreateCallback("exit",exit_cb,0);sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}

/* ---------- dibujo ---------- */
static void rect(int x,int y,int w,int h,unsigned c){
    if(x<0){w+=x;x=0;}if(y<0){h+=y;y=0;}if(x+w>W)w=W-x;if(y+h>H)h=H-y;if(w<=0||h<=0)return;
    for(int j=0;j<h;j++){unsigned *p=fb+(y+j)*STRIDE+x;for(int i=0;i<w;i++)p[i]=c;}
}
static unsigned utf8_next(const char **s){
    const unsigned char *p=(const unsigned char*)*s;unsigned c=*p;
    if(c<0x80){*s+=1;return c;}
    if((c&0xe0)==0xc0&&p[1]){*s+=2;return ((c&0x1f)<<6)|(p[1]&0x3f);}
    if((c&0xf0)==0xe0&&p[1]&&p[2]){*s+=3;return ((c&0x0f)<<12)|((p[1]&0x3f)<<6)|(p[2]&0x3f);}
    if((c&0xf8)==0xf0&&p[1]&&p[2]&&p[3]){*s+=4;return ((c&0x07)<<18)|((p[1]&0x3f)<<12)|((p[2]&0x3f)<<6)|(p[3]&0x3f);}
    *s+=1;return '?';
}
static void glyph(int x,int y,unsigned code,unsigned c){
    if(code<32||code>255){ if(code==0x2013||code==0x2014)code='-'; else if(code==0x2018||code==0x2019)code='\''; else if(code==0x201c||code==0x201d)code='"'; else code='?'; }
    const unsigned char *g=font_bits[code-32];
    for(int j=0;j<FONT_H;j++){int yy=y+j;if(yy<0||yy>=H)continue;unsigned b=g[j];
        for(int i=0;i<FONT_W;i++)if(b&(0x80>>i)){int xx=x+i;if(xx>=0&&xx<W)fb[yy*STRIDE+xx]=c;}}
}
static int text(int x,int y,const char *s,unsigned c){
    while(*s){unsigned code=utf8_next(&s);glyph(x,y,code,c);x+=FONT_W;}
    return x;
}
/* Ajuste de lineas por palabras: escribe desde la linea 'skip' hasta llenar 'maxLines'. Devuelve total de lineas. */
static int textwrap(int x,int y,int wpx,const char *s,unsigned c,int skip,int maxLines){
    int cols=wpx/FONT_W,line=0,drawn=0;const char *p=s;
    while(*p){
        /* medir cuantos caracteres caben hasta el ultimo espacio */
        const char *q=p;int n=0;const char *lastSpace=0;
        while(*q&&n<cols){const char *save=q;unsigned code=utf8_next(&q);if(code=='\n'){break;}if(code==' '){lastSpace=save;}n++;}
        const char *end=q;
        if(*q&&*q!='\n'&&lastSpace&&n>=cols){end=lastSpace;}
        if(line>=skip&&drawn<maxLines){int xx=x;const char *r=p;while(r<end){unsigned code=utf8_next(&r);glyph(xx,y+drawn*(FONT_H+2),code,c);xx+=FONT_W;}drawn++;}
        line++;p=end;while(*p==' ')p++;if(*p=='\n')p++;
        if(!*p)break;
    }
    return line;
}
static void present(int *index,unsigned **buffers){
    sceDisplaySetFrameBuf(buffers[*index],STRIDE,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);
    sceDisplayWaitVblankStart();*index^=1;fb=buffers[*index];
}
static void gu_draw_buffer(unsigned *b){sceGuStart(GU_DIRECT,gulist);sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)b&0x001fffff),STRIDE);sceGuFinish();sceGuSync(0,0);}

/* ---------- estado ---------- */
#define QMAX 160
#define TXT (200*1024)
static char question[QMAX]="";
static KbHit hits[8];static int nhits=0,sel=0,scroll=0;
static char qwords[12][24];static int nq=0;
static char title[200],answer[1200];static char *body; /* TXT bytes, malloc */
static int haveKB=0;static char status[120]="";
static int history=0;
static int reveal=0;static int showArticle=0; /* TRIANGULO: ver el articulo completo */
static char smallTalk[600]=""; /* respuesta conversacional (saludo, calculo, identidad) */ /* bytes de la respuesta ya 'escritos' (animacion letra a letra) */
static int mode=0; /* 0 buscar (Wikipedia), 1 charla (modelo diminuto) */
static char chatPrompt[QMAX]="",chatOut[900]="";static int haveLM=0,generating=0;
static unsigned **gBuffers;static int *gIndex;

static void draw_chat(void){
    rect(0,0,W,H,BG);rect(0,0,W,30,INK);text(12,8,"PSP-IA",ACC);text(80,8,"modo charla (experimental)",ACC2);
    text(W-12-15*FONT_W,8,"SELECT: buscar",MUTED);
    rect(10,38,W-20,22,PANEL);rect(10,38,3,22,USER);
    if(chatPrompt[0])text(20,42,chatPrompt,WHITE);else text(20,42,"Pulsa X y escribe algo para que la IA continue...",MUTED);
    rect(10,66,W-20,H-66-26,PANEL);rect(10,66,3,H-66-26,ACC2);
    if(status[0]&&strstr(status,"teclado"))text(20,H-44,status,BAD);
    if(!haveLM)text(20,76,"No encuentro ms0:/IA/lm.bin (modelo de lenguaje).",BAD);
    else if(chatOut[0]||generating){char shown[1000];snprintf(shown,sizeof(shown),"%s%s",chatOut,generating?"_":"");textwrap(20,74,W-40,shown,WHITE,0,10);if(generating)text(20,H-44,"escribiendo...",MUTED);}
    else{text(20,76,"Modelo de lenguaje de 5 millones de parametros entrenado",WHITE);text(20,92,"con la Wikipedia en espanol, corriendo en la propia PSP.",WHITE);
        text(20,118,"Inventa texto plausible pero NO fiable: es un juguete.",ACC2);text(20,134,"Para respuestas reales usa el modo buscar (SELECT).",MUTED);
        text(20,160,"Prueba: 'Medellin es' / 'El futbol' / 'Simon Bolivar fue'",MUTED);}
    rect(0,H-22,W,22,INK);text(12,H-17,"X escribir   O borrar   SELECT modo   START salir",MUTED);
}
static void draw_frame(void){
    if(mode==1){draw_chat();return;}
    rect(0,0,W,H,BG);
    rect(0,0,W,30,INK);text(12,8,"PSP-IA",ACC);text(80,8,"conocimiento offline",MUTED);
    char b[96];snprintf(b,sizeof(b),"%u articulos",kb_docs());text(W-12-strlen(b)*FONT_W,8,b,MUTED);
    /* pregunta */
    rect(10,38,W-20,22,PANEL);rect(10,38,3,22,USER);
    if(question[0])text(20,42,question,WHITE);else text(20,42,"Pulsa X y escribe tu pregunta...",MUTED);
    if(nhits>0){
        /* respuesta */
        rect(10,66,W-20,H-66-26,PANEL);rect(10,66,3,H-66-26,ACC);
        if(showArticle){ /* articulo completo, con scroll */
            snprintf(b,sizeof(b),"%.70s",title);text(20,71,b,ACC2);snprintf(b,sizeof(b),"%d/%d",sel+1,nhits);text(W-20-strlen(b)*FONT_W,71,b,MUTED);
            int y=89;int avail=(H-26-6-y)/(FONT_H+2);int total=textwrap(20,y,W-40,body,WHITE,scroll,avail);
            if(total>avail){snprintf(b,sizeof(b),"%d/%d",scroll+1,total-avail+1);text(W-20-strlen(b)*FONT_W,H-26-16,b,MUTED);}
        }else{ /* UNA respuesta, escrita letra a letra, y la fuente debajo */
            char shown[1200];int al=(int)strlen(answer);int r=reveal<al?reveal:al;
            while(r<al&&r>0&&((unsigned char)answer[r]&0xc0)==0x80)r++; /* no cortar un caracter UTF-8 */
            memcpy(shown,answer,r);shown[r]=0;if(r<al&&((history+reveal)/4)&1)strcat(shown,"_");
            int y=74;int avail=(H-26-30-y)/(FONT_H+2);int total=textwrap(20,y,W-40,shown,WHITE,scroll,avail);
            if(total>avail){snprintf(b,sizeof(b),"%d/%d",scroll+1,total-avail+1);text(W-20-strlen(b)*FONT_W,H-26-16,b,MUTED);}
            rect(20,H-26-30,W-40,1,RGB(60,70,90));
            snprintf(b,sizeof(b),"Fuente: %.26s%s (%d/%d)",title,strlen(title)>26?"...":"",sel+1,nhits);text(20,H-26-24,b,ACC2);
            text(W-20-22*FONT_W,H-26-24,"TRIANGULO: articulo",MUTED);
        }
    }else if(smallTalk[0]){
        rect(10,66,W-20,H-66-26,PANEL);rect(10,66,3,H-66-26,ACC);
        char shown[600];int al=(int)strlen(smallTalk);int r=reveal<al?reveal:al;while(r<al&&r>0&&((unsigned char)smallTalk[r]&0xc0)==0x80)r++;
        memcpy(shown,smallTalk,r);shown[r]=0;if(r<al)strcat(shown,"_");textwrap(20,74,W-40,shown,WHITE,0,9);
    }else if(question[0]){
        rect(10,66,W-20,40,PANEL);text(20,80,status[0]?status:"No encontre nada sobre eso. Prueba con otras palabras.",BAD);
    }else{
        rect(10,66,W-20,H-66-26,PANEL);
        text(20,76,"Hola. Soy la IA de tu PSP: respondo con la Wikipedia",WHITE);
        text(20,94,"en espanol guardada en la Memory Stick, sin internet.",WHITE);
        text(20,122,"Ejemplos:",ACC);
        text(20,140,"  quien fue Simon Bolivar",MUTED);text(20,156,"  que es la fotosintesis",MUTED);
        text(20,172,"  capital de Australia",MUTED);text(20,188,"  Medellin metro",MUTED);
        if(status[0])text(20,214,status,BAD);
    }
    rect(0,H-22,W,22,INK);text(12,H-17,"X preguntar  L/R otra fuente  /\\ articulo  O borrar  SELECT charla",MUTED);
}

/* ---------- teclado del sistema ---------- */
static void utf8_to_utf16(const char *s,unsigned short *out,int cap){int n=0;while(*s&&n<cap-1){unsigned c=utf8_next(&s);out[n++]=c>0xffff?'?':(unsigned short)c;}out[n]=0;}
static void utf16_to_utf8(const unsigned short *s,char *out,int cap){int n=0;for(;*s;s++){unsigned c=*s;
    if(c<0x80){if(n+1>=cap)break;out[n++]=c;}
    else if(c<0x800){if(n+2>=cap)break;out[n++]=0xc0|(c>>6);out[n++]=0x80|(c&0x3f);}
    else{if(n+3>=cap)break;out[n++]=0xe0|(c>>12);out[n++]=0x80|((c>>6)&0x3f);out[n++]=0x80|(c&0x3f);}}
    out[n]=0;}
/* Teclado propio de respaldo (si el del sistema no arranca): rejilla con cruceta, X letra, O borrar, START listo. */
static const char *KBD_ROWS[4]={"1234567890-","qwertyuiop","asdfghjklñ","zxcvbnm,.?"};
static int simple_kbd(char *result,int cap,unsigned **buffers,int *index){
    int r=1,c=0;unsigned prev=0xffffffff;char buf[QMAX];snprintf(buf,sizeof(buf),"%s",result);
    for(;;){
        SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);unsigned pr=pad.Buttons&~prev;prev=pad.Buttons;
        int rl=(int)strlen(KBD_ROWS[r]);
        if(pr&PSP_CTRL_UP)r=(r+3)%4;if(pr&PSP_CTRL_DOWN)r=(r+1)%4;if(pr&PSP_CTRL_LEFT)c=(c+rl-1)%rl;if(pr&PSP_CTRL_RIGHT)c=(c+1)%rl;
        rl=(int)strlen(KBD_ROWS[r]);if(c>=rl)c=rl-1;
        int len=(int)strlen(buf);
        if(pr&PSP_CTRL_CROSS){unsigned char ch=KBD_ROWS[r][c];if(ch==0xf1){if(len<cap-3){buf[len++]=(char)0xc3;buf[len++]=(char)0xb1;buf[len]=0;}}else if(len<cap-2){buf[len++]=ch;buf[len]=0;}}
        if(pr&PSP_CTRL_SQUARE){if(len<cap-2){buf[len++]=' ';buf[len]=0;}}
        if(pr&PSP_CTRL_CIRCLE){if(len>0){len--;while(len>0&&((unsigned char)buf[len]&0xc0)==0x80)len--;buf[len]=0;}else return 0;}
        if(pr&PSP_CTRL_START){snprintf(result,cap,"%s",buf);return buf[0]!=0;}
        if(pr&PSP_CTRL_SELECT)return 0;
        rect(0,0,W,H,BG);rect(0,0,W,30,INK);text(12,8,"Teclado",ACC);text(90,8,"cruceta mover  X letra  [] espacio  O borrar  START listo  SELECT cancelar",MUTED);
        rect(10,40,W-20,24,PANEL);text(20,45,buf,WHITE);rect(20+(int)strlen(buf)*FONT_W,45,FONT_W,FONT_H,USER);
        for(int i=0;i<4;i++){const char *row=KBD_ROWS[i];int n=(int)strlen(row);int x0=(W-n*34)/2;
            for(int j=0;j<n;j++){int sel=(i==r&&j==c);rect(x0+j*34,90+i*40,30,30,sel?ACC:PANEL);char ch[3]={row[j],0,0};if((unsigned char)row[j]==0xf1){ch[0]=(char)0xc3;ch[1]=(char)0xb1;}text(x0+j*34+11,90+i*40+8,ch,sel?INK:WHITE);}}
        present(index,buffers);
    }
}
static int osk(char *result,int cap,unsigned **buffers,int *index){
    static unsigned short intext[QMAX],outtext[QMAX],desc[64];
    utf8_to_utf16(result,intext,QMAX);utf8_to_utf16("Escribe tu pregunta",desc,64);memset(outtext,0,sizeof(outtext));
    SceUtilityOskData data;memset(&data,0,sizeof(data));
    data.language=PSP_UTILITY_OSK_LANGUAGE_DEFAULT; /* la del sistema; SPANISH explicito fallaba en consola */data.lines=1;data.unk_24=1;data.inputtype=PSP_UTILITY_OSK_INPUTTYPE_ALL;
    data.desc=desc;data.intext=intext;data.outtextlength=QMAX;data.outtextlimit=QMAX-1;data.outtext=outtext;
    SceUtilityOskParams p;memset(&p,0,sizeof(p));p.base.size=sizeof(p);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&p.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&p.base.buttonSwap);
    p.base.graphicsThread=17;p.base.accessThread=19;p.base.fontThread=18;p.base.soundThread=16;
    p.datacount=1;p.data=&data;
    int rc=sceUtilityOskInitStart(&p);if(rc<0){snprintf(status,sizeof(status),"Teclado del sistema no disponible (%08x): teclado propio.",(unsigned)rc);return simple_kbd(result,cap,buffers,index);}
    /* un solo buffer visible mientras dura el dialogo; el GE dibuja en el */
    /* Fondo estatico en RAM; cada fotograma: copiarlo al buffer OCULTO, apuntar el GE a el, dejar que el dialogo
       dibuje encima y mostrarlo en el siguiente vblank (doble buffer como en los ejemplos del SDK). Restaurar el
       buffer VISIBLE mientras el dialogo aun lo estaba pintando era lo que hacia parpadear el teclado. */
    static unsigned snap[STRIDE*H] __attribute__((aligned(64)));
    fb=snap;draw_frame();
    int back=*index;
    memcpy(buffers[back],snap,STRIDE*H*4);gu_draw_buffer(buffers[back]);
    int frames=0,seen=0;
    for(;;){
        if(!running)break;
        int st=sceUtilityOskGetStatus();
        if(st==PSP_UTILITY_DIALOG_VISIBLE)seen=1;
        if(st==PSP_UTILITY_DIALOG_INIT||st==PSP_UTILITY_DIALOG_VISIBLE)sceUtilityOskUpdate(1);
        else if(st==PSP_UTILITY_DIALOG_QUIT)sceUtilityOskShutdownStart();
        else if(st==PSP_UTILITY_DIALOG_FINISHED||st==PSP_UTILITY_DIALOG_NONE)break;
        sceGuSync(0,0);sceDisplayWaitVblankStart();
        sceDisplaySetFrameBuf(buffers[back],STRIDE,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);
        back^=1;memcpy(buffers[back],snap,STRIDE*H*4);gu_draw_buffer(buffers[back]);
        /* si en 4 s el teclado del sistema no se ha mostrado, se cierra y se usa el propio (nunca colgarse) */
        if(++frames>240&&!seen){sceUtilityOskShutdownStart();for(int k=0;k<60&&sceUtilityOskGetStatus()!=PSP_UTILITY_DIALOG_NONE;k++)sceDisplayWaitVblankStart();fb=buffers[*index];snprintf(status,sizeof(status),"Teclado del sistema no responde: teclado propio.");return simple_kbd(result,cap,buffers,index);}
    }
    *index=back;fb=buffers[*index];
    if(data.result==PSP_UTILITY_OSK_RESULT_CHANGED){utf16_to_utf8(outtext,result,cap);return 1;}
    return 0;
}

static void chat_tick(const char *partial){
    snprintf(chatOut,sizeof(chatOut),"%s%s",chatPrompt,partial);draw_frame();present(gIndex,gBuffers);
}
static void chat_run(void){
    generating=1;chatOut[0]=0;
    char prompt[QMAX+8];snprintf(prompt,sizeof(prompt),"%s",chatPrompt);
    static char out[900];int n=lm_generate(prompt,out,sizeof(out),420,0.85f,chat_tick);
    if(n>0)snprintf(chatOut,sizeof(chatOut),"%s%s",chatPrompt,out);else snprintf(chatOut,sizeof(chatOut),"(no pude generar texto)");
    generating=0;
}
static void load_hit(int i){
    scroll=0;
    if(!kb_doc(hits[i].doc,title,sizeof(title),body,TXT)){snprintf(title,sizeof(title),"(error de lectura)");body[0]=0;answer[0]=0;return;}
    kb_best_answer_t(title,body,qwords,nq,question,answer,sizeof(answer));reveal=0;showArticle=0;
}
/* Reordena los mejores candidatos leyendo sus titulos: premia que el titulo contenga las palabras de la pregunta
   y penaliza titulos largos ("Australia" antes que "Australia Occidental" para "capital de Australia"). */
static void rerank(KbHit *h,int n){
    unsigned qh[12];for(int a=0;a<nq;a++)qh[a]=kb_hash(qwords[a]);
    for(int i=0;i<n;i++){int ntw,blen;unsigned th[4];
        if(kb_title_info(h[i].doc,&ntw,&blen,th)){ /* rapido: tabla de titulos, sin descomprimir */
            int inTitle=0;for(int a=0;a<nq;a++)for(int b=0;b<4;b++)if(th[b]&&th[b]==qh[a]){inTitle++;break;}
            int extra=ntw-inTitle;if(extra<0)extra=0;
            h[i].score+=inTitle*5.0f-extra*1.8f-blen*0.03f+((inTitle==nq&&ntw==nq)?4.0f:0)+((inTitle>0&&ntw<=2)?1.5f:0);
            continue;
        }
        char t[200],dummy[8];
        if(!kb_doc(h[i].doc,t,sizeof(t),dummy,sizeof(dummy)))continue;
        char tw[16][24];int ntw2=kb_words(t,tw,16);int inTitle=0;
        for(int a=0;a<nq;a++)for(int b=0;b<ntw2;b++)if(!strcmp(qwords[a],tw[b])){inTitle++;break;}
        h[i].score+=inTitle*4.0f-(ntw2-inTitle)*1.5f-(strchr(t,'(')?2.0f:0)-strlen(t)*0.03f+((inTitle==nq&&ntw2==nq)?3.0f:0);
    }
    for(int i=1;i<n;i++){KbHit x=h[i];int j=i;while(j>0&&h[j-1].score<x.score){h[j]=h[j-1];j--;}h[j]=x;}
}
/* Comprension basica de la pregunta antes de buscar: saludos, identidad, calculo, hora. Devuelve 1 si ya respondio. */
static double parse_num(const char **s){while(**s==' ')(*s)++;double v=0,f=0,d=1;int any=0,neg=0;if(**s=='-'){neg=1;(*s)++;}
    while(**s>='0'&&**s<='9'){v=v*10+(**s-'0');(*s)++;any=1;}if(**s=='.'||**s==','){(*s)++;while(**s>='0'&&**s<='9'){d/=10;f+=(**s-'0')*d;(*s)++;any=1;}}
    if(!any)return -1e300;return neg?-(v+f):v+f;}
static int small_talk(const char *qraw){
    char ql[QMAX];int n=0;for(const char *c=qraw;*c&&n<QMAX-1;c++)ql[n++]=(*c>='A'&&*c<='Z')?*c+32:*c;ql[n]=0;
    char w[12][24];int nw=kb_words(qraw,w,12);
    if(strstr(ql,"cuanto es")||strstr(ql,"cuánto es")||strstr(ql,"calcula")||(nw<=3&&strpbrk(ql,"+*/x")&&strpbrk(ql,"0123456789"))){
        const char *s=ql;while(*s&&!(*s>='0'&&*s<='9')&&*s!='-')s++;
        double a=parse_num(&s);if(a>-1e299){while(*s==' ')s++;char op=*s;if(op=='x')op='*';if(op=='+'||op=='-'||op=='*'||op=='/'||op==':'){s++;double b=parse_num(&s);
            if(b>-1e299){double r=op=='+'?a+b:op=='-'?a-b:op=='*'?a*b:(b!=0?a/b:0);if(op=='/'&&b==0)snprintf(smallTalk,sizeof(smallTalk),"No se puede dividir entre cero.");
            else if(r==(long long)r)snprintf(smallTalk,sizeof(smallTalk),"%g %c %g = %lld",a,op,b,(long long)r);else snprintf(smallTalk,sizeof(smallTalk),"%g %c %g = %.4f",a,op,b,r);return 1;}}}
    }
    if(nw==0){
        if(strstr(ql,"hola")||strstr(ql,"buenas")||strstr(ql,"buenos dias")||strstr(ql,"hey")){snprintf(smallTalk,sizeof(smallTalk),"Hola. Soy la IA de tu PSP. Preguntame lo que quieras: personas, lugares, historia, ciencia, deportes... Respondo con la Wikipedia en espanol que llevo en la Memory Stick.");return 1;}
        if(strstr(ql,"gracias")){snprintf(smallTalk,sizeof(smallTalk),"De nada. Cuando quieras, otra pregunta.");return 1;}
        if(strstr(ql,"adios")||strstr(ql,"chao")||strstr(ql,"hasta luego")){snprintf(smallTalk,sizeof(smallTalk),"Hasta luego. Pulsa START para salir.");return 1;}
    }
    if((strstr(ql,"quien eres")||strstr(ql,"quién eres")||strstr(ql,"que eres")||strstr(ql,"qué eres")||strstr(ql,"como te llamas")||strstr(ql,"cómo te llamas"))&&nw<=4){
        snprintf(smallTalk,sizeof(smallTalk),"Soy PSP-IA, un asistente que corre entero dentro de esta PSP: busco entre %u articulos de la Wikipedia en espanol y tengo un pequeno modelo de lenguaje (SELECT) para charlar. No necesito internet.",kb_docs());return 1;}
    if((strstr(ql,"como estas")||strstr(ql,"cómo estás")||strstr(ql,"que tal")||strstr(ql,"qué tal"))&&nw<=3){snprintf(smallTalk,sizeof(smallTalk),"Muy bien, a 333 MHz y con la Memory Stick llena de conocimiento. Y tu, que quieres saber?");return 1;}
    return 0;
}
/* Reformula la pregunta para buscar mejor: quita "quien fue/es", "que es", "cuando", "donde", "cuantos", "capital de"... conserva el resto. */
static void ask(void){
    smallTalk[0]=0;reveal=0;showArticle=0;
    if(small_talk(question)){nhits=0;history++;return;}
    static KbHit cand[200];int nc=kb_search(question,cand,200,qwords,&nq);
    rerank(cand,nc);nhits=nc<8?nc:8;for(int i=0;i<nhits;i++)hits[i]=cand[i];sel=0;scroll=0;
    if(nhits>0)load_hit(0);
    else snprintf(status,sizeof(status),nq?"No encontre nada sobre eso. Prueba con otras palabras.":"Escribe alguna palabra clave (no solo 'que', 'de', 'la').");
    history++;
}

int main(void){
    int th=sceKernelCreateThread("cb",cb_thread,0x11,0x1000,0,0);if(th>=0)sceKernelStartThread(th,0,0);
    scePowerSetClockFrequency(333,333,166);
    sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    unsigned *buffers[2];buffers[0]=(unsigned*)((uintptr_t)sceGeEdramGetAddr()|0x40000000);buffers[1]=buffers[0]+STRIDE*H;
    /* GU completo aunque dibujemos por CPU: los dialogos del sistema (teclado) usan el contexto del GE */
    sceGuInit();sceGuStart(GU_DIRECT,gulist);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,STRIDE);sceGuDispBuffer(W,H,(void*)(STRIDE*H*4),STRIDE);sceGuDepthBuffer((void*)(STRIDE*H*8),STRIDE);
    sceGuOffset(2048-(W/2),2048-(H/2));sceGuViewport(2048,2048,W,H);sceGuScissor(0,0,W,H);sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();sceGuDisplay(GU_TRUE);
    int index=0;fb=buffers[index];
    gBuffers=buffers;gIndex=&index;body=(char*)malloc(TXT);body[0]=0;
    haveKB=kb_open("ms0:/IA");haveLM=lm_load("ms0:/IA/lm.bin");
    if(haveKB!=1){snprintf(status,sizeof(status),"No encuentro la base de datos en ms0:/IA (codigo %d).",haveKB);haveKB=0;}
#ifdef TEST_CHAT
    if(haveLM){mode=1;snprintf(chatPrompt,sizeof(chatPrompt),"%s",TEST_CHAT);chat_run();}
#endif
#ifdef TEST_QUERY
    if(haveKB){snprintf(question,sizeof(question),"%s",TEST_QUERY);ask();}
#endif
    unsigned prev=0;
    while(running){
        SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);unsigned pressed=pad.Buttons&~prev;prev=pad.Buttons;
        if(pressed&PSP_CTRL_START)break;
        if(pressed&PSP_CTRL_SELECT)mode^=1;
        if(mode==1){
            if(pressed&PSP_CTRL_CROSS){if(osk(chatPrompt,sizeof(chatPrompt),buffers,&index)&&haveLM&&chatPrompt[0])chat_run();}
            if(pressed&PSP_CTRL_CIRCLE){chatPrompt[0]=0;chatOut[0]=0;}
            draw_frame();present(&index,buffers);continue;
        }
        if(pressed&PSP_CTRL_CROSS){if(osk(question,sizeof(question),buffers,&index)&&haveKB&&question[0])ask();}
        if(pressed&PSP_CTRL_CIRCLE){question[0]=0;nhits=0;status[0]=0;smallTalk[0]=0;}
        if(nhits>0){
            if((pressed&PSP_CTRL_RTRIGGER)&&sel<nhits-1){sel++;load_hit(sel);}
            if((pressed&PSP_CTRL_LTRIGGER)&&sel>0){sel--;load_hit(sel);}
            if(pressed&PSP_CTRL_TRIANGLE){showArticle^=1;scroll=0;}
            if(pressed&PSP_CTRL_DOWN)scroll++;
            if((pressed&PSP_CTRL_UP)&&scroll>0)scroll--;
        }
        {int tl=(int)strlen(smallTalk[0]?smallTalk:answer);if(reveal<tl)reveal+=1;else reveal=tl;} /* ~60 letras/s */
        draw_frame();present(&index,buffers);
    }
    sceKernelExitGame();return 0;
}
