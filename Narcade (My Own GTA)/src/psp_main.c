#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <psputility.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "render3d.h"
PSP_MODULE_INFO("Narcade",0,2,11);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
/* v2.13.1 (Claude): el juego NO usa malloc (0 llamadas en game.c/render3d.c/psp_main.c), pero reservaba 4 MB de heap.
   Con 17,4 MB de datos estaticos (mesh 8 MB, overlay 1 MB, arte del titulo...) quedaban ~2,6 MB libres de los 24 MB
   de la particion de usuario, y el dialogo de guardado de Sony necesita varios MB al CARGAR una partida: se quedaba
   sin memoria y la PSP se reiniciaba (en PPSSPP no pasa porque no aplica ese limite). 1 MB basta para stdio. */
PSP_HEAP_SIZE_KB(1024);
static volatile int running=1;
static int exit_cb(int a,int b,void *p){(void)a;(void)b;(void)p;running=0;sceKernelExitGame();return 0;} /* v2.9.1: salir siempre, aunque un dialogo este abierto */
static int callbacks(SceSize n,void *p){(void)n;(void)p;int cb=sceKernelCreateCallback("Narcade exit",exit_cb,0);sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
static void audio_cb(void *buffer,unsigned int frames,void *p){(void)p;game_audio(buffer,frames);}

/* ---- Guardado nativo: dialogo de la Memory Stick (sceUtilitySavedata), 4 ranuras ---- */
#define SAVE_GAME "NARC00001"
static char saveNames[][20]={"0000","0001","0002","0003",""};
static unsigned char saveBuf[4096] __attribute__((aligned(64)));
static SceUtilitySavedataParam sd;
extern const unsigned char icon0_png[],icon0_png_end[];
/* mode 0 = LISTSAVE, 1 = LISTLOAD. Devuelve 1 si termino bien, 0 si cancelado/error.
   Mientras el dialogo esta abierto seguimos dibujando el juego debajo con el mismo doble buffer. */
/* v2.13.2 (Claude): registro de pasos en ms0:/NARCADE_DEBUG.TXT. La PSP se reinicia al cargar partida y en PPSSPP
   no ocurre, asi que hay que saber en que paso exacto muere: el fichero conserva la ultima linea escrita. */
/* v2.13.5 (Claude): fase actual (sin escribir a disco) + hilo vigilante que la vuelca cada 200 ms. Permite saber si
   la consola se CUELGA (la fase se repite) o se ESTRELLA (el registro para), y en que celda de la ciudad. */
static int dbgOn=-1;
static volatile const char *gPhase="arranque";static volatile unsigned gPhaseSeq=0;
static void phase(const char *p){gPhase=p;gPhaseSeq++;}
static int watchdog(SceSize a,void *v){(void)a;(void)v;
    unsigned last=0;int same=0;
    for(;;){
        sceKernelDelayThread(200000);
        unsigned seq=gPhaseSeq;const char *ph=(const char*)gPhase;
        if(seq==last){
            if(++same>2){
                if(!dbgOn)continue;
                char m[128];int n=snprintf(m,sizeof(m),"VIGILANTE: atascado en '%s' (%d)%c",ph,same,10);
                SceUID f=sceIoOpen("ms0:/NARCADE_DEBUG.TXT",PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND,0777);
                if(f>=0){sceIoWrite(f,m,n);sceIoClose(f);}
            }
        }
        else{last=seq;same=0;}
    }
    return 0;
}
/* v2.29 (Claude): grabadora de joystick para diagnostico. Solo si existe
   ms0:/NARCADE_INPUT.ON: guarda por fotograma el tiempo (dt), Lx, Ly y botones
   (hasta 2 minutos, en RAM) y lo escribe en ms0:/NARCADE_INPUT.BIN al pulsar START
   o al llenarse. Permite reproducir en el PC la entrada real de la consola. */
typedef struct{uint16_t dt10us;uint8_t lx,ly;uint32_t buttons;
    uint16_t tick,city,geom,ge,hud,wait;}InputSample; /* v2.30: tiempos por etapa en unidades de 10 us */
extern unsigned r3ProfT[4];
static unsigned profTick,profDraw0,profDraw1,profWait0;
#define INPUT_MAX 7200
static InputSample inputLog[INPUT_MAX];static int inputOn=-1,inputCount=0,inputPrevStart=0,inputSaved=0;
static void input_flush(void){
    SceUID f=sceIoOpen("ms0:/NARCADE_INPUT.BIN",PSP_O_WRONLY|PSP_O_CREAT|PSP_O_TRUNC,0777);
    if(f>=0){sceIoWrite(f,inputLog,inputCount*sizeof(InputSample));sceIoClose(f);}
}
static void input_record(const SceCtrlData *pad,float dt){
    if(inputOn<0){SceUID t=sceIoOpen("ms0:/NARCADE_INPUT.ON",PSP_O_RDONLY,0);inputOn=t>=0;if(t>=0)sceIoClose(t);}
    if(!inputOn||inputSaved)return;
    if(inputCount<INPUT_MAX){InputSample *s=&inputLog[inputCount++];float v=dt*100000.f;s->dt10us=(uint16_t)(v>65535?65535:v);s->lx=pad->Lx;s->ly=pad->Ly;s->buttons=pad->Buttons;}
    if(inputCount>1){InputSample *p=&inputLog[inputCount-2]; /* tiempos del fotograma anterior (ya completo) */
#define T10(x) (uint16_t)((x)/10>65535?65535:(x)/10)
        p->tick=T10(profTick);p->city=T10(r3ProfT[1]-r3ProfT[0]);p->geom=T10(r3ProfT[2]-r3ProfT[1]);p->ge=T10(r3ProfT[3]-r3ProfT[2]);
        unsigned drawAll=profDraw1-profDraw0,r3=r3ProfT[3]-r3ProfT[0];p->hud=T10(drawAll>r3?drawAll-r3:0);p->wait=T10(sceKernelGetSystemTimeLow()-profWait0);
#undef T10
    }
    int start=(pad->Buttons&PSP_CTRL_START)!=0;
    if((start&&!inputPrevStart&&inputCount>60)||inputCount>=INPUT_MAX){input_flush();inputSaved=inputCount>=INPUT_MAX;}
    inputPrevStart=start;
}
static void dbg(const char *msg){
    if(dbgOn<0){SceUID t=sceIoOpen("ms0:/NARCADE_DEBUG.ON",PSP_O_RDONLY,0);dbgOn=t>=0;if(t>=0)sceIoClose(t);}
    if(!dbgOn)return; /* v2.13.6: diagnostico solo si existe ms0:/NARCADE_DEBUG.ON */
    SceUID f=sceIoOpen("ms0:/NARCADE_DEBUG.TXT",PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND,0777);
    if(f<0)return;
    char line[160];int n=snprintf(line,sizeof(line),"%u %s free=%dKB max=%dKB\n",(unsigned)(sceKernelGetSystemTimeLow()/1000),msg,
        (int)(sceKernelTotalFreeMemSize()>>10),(int)(sceKernelMaxFreeMemSize()>>10));
    sceIoWrite(f,line,n);sceIoClose(f);
}
/* v2.44 (Claude): hilo de carga por etapas (ver savedata_dialog). */
static volatile int loadStageNow,loadFinished;
static int load_worker(SceSize args,void *argp){(void)args;(void)argp;
 for(int stage=0;stage<24;stage++){char m[40];snprintf(m,sizeof(m),"etapa-%d-inicio",stage);dbg(m);
  loadStageNow=stage;int done=game_load_stage(0,512,stage);snprintf(m,sizeof(m),"etapa-%d-ok",stage);dbg(m);if(done)break;}
 loadFinished=1;return 0;}
static int savedata_dialog(int mode,uint32_t **buffers,int *index){
 memset(&sd,0,sizeof(sd));sd.base.size=sizeof(sd);
 sd.base.language=game_language()?PSP_SYSTEMPARAM_LANGUAGE_ENGLISH:PSP_SYSTEMPARAM_LANGUAGE_SPANISH;
 sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,&sd.base.buttonSwap);
 sd.base.graphicsThread=0x11;sd.base.accessThread=0x13;sd.base.fontThread=0x12;sd.base.soundThread=0x10;
 /* LISTLOAD/LISTSAVE invokes Sony's official Memory Stick slot browser. */
 sd.mode=mode?PSP_UTILITY_SAVEDATA_LISTLOAD:PSP_UTILITY_SAVEDATA_LISTSAVE;sd.overwrite=1;
 sd.focus=mode?PSP_UTILITY_SAVEDATA_FOCUS_LATEST:PSP_UTILITY_SAVEDATA_FOCUS_FIRSTEMPTY;
 strcpy(sd.key,"NARCADEKEY2026");strcpy(sd.gameName,SAVE_GAME);strcpy(sd.saveName,"<");sd.saveNameList=saveNames;strcpy(sd.fileName,"DATA.BIN");
 sd.dataBuf=saveBuf;sd.dataBufSize=sizeof(saveBuf);sd.dataSize=0;
 if(!mode){int n=game_export_save(saveBuf,sizeof(saveBuf));if(n<=0)return 0;sd.dataSize=n;
  strcpy(sd.sfoParam.title,"Narcade");game_save_summary(sd.sfoParam.savedataTitle,sizeof(sd.sfoParam.savedataTitle),sd.sfoParam.detail,sizeof(sd.sfoParam.detail));sd.sfoParam.parentalLevel=1;
  sd.icon0FileData.buf=(void*)icon0_png;sd.icon0FileData.bufSize=sd.icon0FileData.size=(unsigned)(icon0_png_end-icon0_png);}
 /* v2.9.1 (Claude): patron de los ejemplos del SDK (utility/savedata). El dialogo de Sony pinta con transparencia
    sobre el buffer de DIBUJO que figura en el estado interno de sceGu y solo repinta lo que cambia: hay que
    (1) fijar draw/disp con sceGuDrawBuffer/sceGuDispBuffer (no las variantes ...List), (2) restaurar el fondo en el
    buffer de dibujo en cada fotograma, (3) dejar que el dialogo pinte y (4) intercambiar con sceGuSwapBuffers().
    Restaurar el buffer visible o alternar buffers a mano dejaba el resaltado del slot anterior "pegado". */
 static uint32_t background[512*272] __attribute__((aligned(64)));
 game_draw(buffers[*index],512);
 memcpy(background,buffers[*index],sizeof(background));
 /* v2.13.1: si no hay memoria suficiente para el dialogo, avisar en vez de dejar que el sistema se caiga */
 if(sceKernelMaxFreeMemSize()<1400*1024){game_set_lowmem(sceKernelMaxFreeMemSize()>>10);return 0;}
 /* v2.13.4: el dialogo de Sony arranca sus propios hilos (grafico, acceso, fuente y SONIDO). Dejar nuestro callback
    de audio vivo mientras el utility toma y suelta el canal es una fuente conocida de cuelgues en consola. */
 pspAudioSetChannelCallback(0,0,0);
 dbg(mode?"dialogo-cargar-init":"dialogo-guardar-init");
 if(sceUtilitySavedataInitStart(&sd)<0)return 0;
 int cur=*index,finishing=0,idle=0;
 r3_gu_buffers(buffers[cur],buffers[cur^1]);
 while(running){
  phase("dialogo");memcpy(buffers[cur],background,sizeof(background));sceKernelDcacheWritebackAll();
  r3_gu_idle();
  int st=sceUtilitySavedataGetStatus();
  if(st==PSP_UTILITY_DIALOG_INIT||st==PSP_UTILITY_DIALOG_VISIBLE)sceUtilitySavedataUpdate(1);
  else if(st==PSP_UTILITY_DIALOG_QUIT)sceUtilitySavedataShutdownStart();
  else if(st==PSP_UTILITY_DIALOG_FINISHED){sceUtilitySavedataShutdownStart();finishing=1;}
  else if(st==PSP_UTILITY_DIALOG_NONE){if(finishing||++idle>120)break;}
  sceDisplayWaitVblankStart();
  r3_gu_swap();cur^=1;
 }
 /* v2.13.4: esperar a que el modulo quede descargado (estado NONE) antes de volver a dibujar: salir en FINISHED
    dejaba los hilos del utility vivos mientras el juego ya usaba el GE y la VRAM. */
 for(int k=0;k<180&&sceUtilitySavedataGetStatus()!=PSP_UTILITY_DIALOG_NONE;k++)sceDisplayWaitVblankStart();
 dbg("dialogo-cerrado");
 /* devolver el control de la pantalla al juego (dibuja por CPU y presenta con sceDisplaySetFrameBuf) */
 r3_gu_display(0);
 sceDisplaySetFrameBuf(buffers[cur],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 *index=cur;
 pspAudioSetChannelCallback(0,audio_cb,0); /* restaurar el audio del juego */
 if(sd.base.result!=0){dbg("dialogo-result-no-cero");return 0;}
 if(mode){
  dbg("import-inicio");
  int ok=game_import_save(saveBuf,(int)sd.dataSize);
  if(ok){char m[64];int px,py;game_player_pos(&px,&py);snprintf(m,sizeof(m),"import-ok pos=%d,%d celda=%d,%d",px,py,px/320,py/320);dbg(m);}else dbg("import-fallo");
  if(ok){
   /* v2.13.3: carga por etapas. Cada etapa hace un trozo acotado, se dibuja el progreso y se registra: si la consola
      se reinicia, la ultima linea del registro dice en que etapa exacta ocurrio. */
   /* v2.41 (Claude): doble bufer (se dibuja en el que no se ve) y un fotograma de animacion entre etapas
      (zoom y fundido de la ilustracion). La ultima imagen se queda hasta que el primer fotograma de juego
      la sustituye: la pantalla de carga dura exactamente hasta que ya te puedes mover. */
   /* v2.44 (Claude): la carga pesada va en un hilo de menor prioridad y este hilo presenta la pantalla a 30 fps
      (ilustracion por el GE con zoom suave y fundidos). Antes solo se redibujaba entre etapas: el zoom iba a
      saltos. La ultima imagen se queda hasta el primer fotograma de juego. */
   int cur=*index^1;uint64_t loadStart=sceKernelGetSystemTimeWide();
#define LOAD_CLOCK() game_load_clock((float)(sceKernelGetSystemTimeWide()-loadStart)*1e-6f)
   game_load_begin();loadStageNow=0;loadFinished=0;
   SceUID worker=sceKernelCreateThread("Narcade carga",load_worker,0x30,0x40000,THREAD_ATTR_USER|THREAD_ATTR_VFPU,0);
   if(worker>=0&&sceKernelStartThread(worker,0,0)>=0){
    while(!loadFinished){
     LOAD_CLOCK();game_load_present(buffers[cur],512,loadStageNow,0);sceKernelDcacheWritebackAll();
     sceDisplaySetFrameBuf(buffers[cur],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);
     sceDisplayWaitVblankStart();sceDisplayWaitVblankStart();cur^=1; /* 30 fps; en la espera trabaja el hilo de carga */
    }
    sceKernelWaitThreadEnd(worker,0);sceKernelDeleteThread(worker);
   }else{dbg("hilo-de-carga-no-disponible");if(worker>=0)sceKernelDeleteThread(worker);load_worker(0,0);} /* sin hilo: carga directa */
   LOAD_CLOCK();game_load_present(buffers[cur],512,loadStageNow,1);sceKernelDcacheWritebackAll();
   sceDisplaySetFrameBuf(buffers[cur],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);
   sceDisplayWaitVblankStart();cur^=1;
   *index=cur;game_load_finish();
   dbg("carga-por-etapas-ok");
   r3_trace(dbg,3);
  }
  return ok;
 }
 return 1;
}
static void handle_save_request(int req,uint32_t **buffers,int *index){
 if(req==2){
  game_request_result(2,savedata_dialog(1,buffers,index));
 }else game_request_result(req,savedata_dialog(0,buffers,index));
}
int main(void){
 int th=sceKernelCreateThread("Narcade callbacks",callbacks,0x11,0x1000,0,0);if(th>=0)sceKernelStartThread(th,0,0);
 int wd=sceKernelCreateThread("Narcade watchdog",watchdog,0x08,0x2000,0,0);if(wd>=0)sceKernelStartThread(wd,0,0);
 r3_phase_hook(phase);
 scePowerSetClockFrequency(333,333,166);
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 sceDisplaySetMode(0,480,272);
 uint32_t *buffers[2];buffers[0]=(uint32_t*)((uintptr_t)sceGeEdramGetAddr()|0x40000000);buffers[1]=buffers[0]+512*272;
 r3_init();game_init();game_set_native_savedata(1);sceIoMkdir("ms0:/PSP/SAVEDATA/NARCADE3D",0777);game_set_save_path("ms0:/PSP/SAVEDATA/NARCADE3D/PROGRESS.BIN");
 pspAudioInit();pspAudioSetChannelCallback(0,audio_cb,0);
 uint64_t before=sceKernelGetSystemTimeWide();int index=0;int postLoad=0;
 /* v2.30 (Claude): el tiempo de cada paso se mide en refrescos de pantalla (vcount), no
    con el reloj: asi cada imagen mostrada avanza exactamente lo que le toca (antes el
    tiempo de calculo variaba y el movimiento por imagen salia irregular). Ademas, si
    la zona es pesada, se fija un ritmo constante de 30 fps en vez de alternar 60/30. */
 unsigned lastV=sceDisplayGetVcount();float workAvg=0;int interval=1;
 while(running){SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);uint64_t now=sceKernelGetSystemTimeWide();before=now;
  unsigned vnow=sceDisplayGetVcount();int vf=(int)(vnow-lastV);if(vf<1)vf=1;if(vf>6)vf=6;lastV=vnow;float dt=vf/59.94f;
  SceCtrlLatch latch;sceCtrlReadLatch(&latch);game_latch_cross(latch.uiMake);
  input_record(&pad,dt);
  {unsigned t0=sceKernelGetSystemTimeLow();
  phase("tick");game_tick(pad.Buttons,((float)pad.Lx-128)/127,((float)pad.Ly-128)/127,dt);
  profTick=sceKernelGetSystemTimeLow()-t0;}
  phase("draw");
  int req=game_take_request();if(req){handle_save_request(req,buffers,&index);sceCtrlReadLatch(&latch);before=sceKernelGetSystemTimeWide();lastV=sceDisplayGetVcount();postLoad=req==2?3:0;continue;}
  if(postLoad>0){char m[32];snprintf(m,sizeof(m),"fotograma-%d-tick",4-postLoad);dbg(m);}
#ifdef NARCADE_PROFILE
  uint64_t p0=sceKernelGetSystemTimeWide();
#endif
  if(postLoad>0){char m[32];snprintf(m,sizeof(m),"fotograma-%d-dibujado",4-postLoad);dbg(m);postLoad--;if(!postLoad)dbg("mundo-estable");}
  profDraw0=sceKernelGetSystemTimeLow();game_draw(buffers[index],512);profDraw1=sceKernelGetSystemTimeLow();
#ifdef NARCADE_PROFILE
  game_set_profile((sceKernelGetSystemTimeWide()-p0)/1000.0f);
#endif
  /* v1.2: pedir el cambio de buffer ANTES de esperar el vblank: el cambio ocurre en ese vblank
     y el siguiente fotograma se dibuja en el buffer ya oculto. (v1.0 esperaba primero y pedia
     el cambio despues, asi que redibujaba el buffer aun visible: parpadeo en la parte superior.) */
  {float work=(sceKernelGetSystemTimeWide()-before)/1000000.0f;workAvg+=(work-workAvg)*.08f;
   if(interval==1&&workAvg>.0150f)interval=2;else if(interval==2&&workAvg<.0115f)interval=1;}
  profWait0=sceKernelGetSystemTimeLow();
  sceDisplaySetFrameBuf(buffers[index],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
  while((int)(sceDisplayGetVcount()-lastV)<interval)sceDisplayWaitVblankStart(); /* ritmo constante */
  index^=1;
 }
 game_save();pspAudioEnd();r3_shutdown();sceKernelExitGame();return 0;
}
