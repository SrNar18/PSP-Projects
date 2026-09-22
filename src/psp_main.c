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
PSP_HEAP_SIZE_KB(4096);
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
static int savedata_dialog(int mode,uint32_t **buffers,int *index){
 memset(&sd,0,sizeof(sd));sd.base.size=sizeof(sd);
 sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&sd.base.language);
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
 if(sceUtilitySavedataInitStart(&sd)<0)return 0;
 int cur=*index;
 r3_gu_buffers(buffers[cur],buffers[cur^1]);
 while(running){
  memcpy(buffers[cur],background,sizeof(background));sceKernelDcacheWritebackAll();
  r3_gu_idle();
  int st=sceUtilitySavedataGetStatus();
  if(st==PSP_UTILITY_DIALOG_INIT||st==PSP_UTILITY_DIALOG_VISIBLE)sceUtilitySavedataUpdate(1);
  else if(st==PSP_UTILITY_DIALOG_QUIT)sceUtilitySavedataShutdownStart();
  else if(st==PSP_UTILITY_DIALOG_FINISHED||st==PSP_UTILITY_DIALOG_NONE)break;
  sceDisplayWaitVblankStart();
  r3_gu_swap();cur^=1;
 }
 /* devolver el control de la pantalla al juego (dibuja por CPU y presenta con sceDisplaySetFrameBuf) */
 r3_gu_display(0);
 sceDisplaySetFrameBuf(buffers[cur],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();
 *index=cur;
 if(sd.base.result!=0)return 0;
 if(mode)return game_import_save(saveBuf,(int)sd.dataSize);
 return 1;
}
static void handle_save_request(int req,uint32_t **buffers,int *index){
 if(req==2){
  game_request_result(2,savedata_dialog(1,buffers,index));
 }else game_request_result(req,savedata_dialog(0,buffers,index));
}
int main(void){
 int th=sceKernelCreateThread("Narcade callbacks",callbacks,0x11,0x1000,0,0);if(th>=0)sceKernelStartThread(th,0,0);
 scePowerSetClockFrequency(333,333,166);
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 sceDisplaySetMode(0,480,272);
 uint32_t *buffers[2];buffers[0]=(uint32_t*)((uintptr_t)sceGeEdramGetAddr()|0x40000000);buffers[1]=buffers[0]+512*272;
 r3_init();game_init();game_set_native_savedata(1);sceIoMkdir("ms0:/PSP/SAVEDATA/NARCADE3D",0777);game_set_save_path("ms0:/PSP/SAVEDATA/NARCADE3D/PROGRESS.BIN");
 pspAudioInit();pspAudioSetChannelCallback(0,audio_cb,0);
 uint64_t before=sceKernelGetSystemTimeWide();int index=0;
 while(running){SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);uint64_t now=sceKernelGetSystemTimeWide();float dt=(now-before)/1000000.0f;before=now;
  SceCtrlLatch latch;sceCtrlReadLatch(&latch);game_latch_cross(latch.uiMake);
  game_tick(pad.Buttons,((float)pad.Lx-128)/127,((float)pad.Ly-128)/127,dt);
  int req=game_take_request();if(req){handle_save_request(req,buffers,&index);sceCtrlReadLatch(&latch);before=sceKernelGetSystemTimeWide();continue;}
#ifdef NARCADE_PROFILE
  uint64_t p0=sceKernelGetSystemTimeWide();
#endif
  game_draw(buffers[index],512);
#ifdef NARCADE_PROFILE
  game_set_profile((sceKernelGetSystemTimeWide()-p0)/1000.0f);
#endif
  /* v1.2: pedir el cambio de buffer ANTES de esperar el vblank: el cambio ocurre en ese vblank
     y el siguiente fotograma se dibuja en el buffer ya oculto. (v1.0 esperaba primero y pedia
     el cambio despues, asi que redibujaba el buffer aun visible: parpadeo en la parte superior.) */
  sceDisplaySetFrameBuf(buffers[index],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();index^=1;
 }
 game_save();pspAudioEnd();r3_shutdown();sceKernelExitGame();return 0;
}
