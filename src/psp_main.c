#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspge.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <stdint.h>
#include "game.h"
#include "render3d.h"
PSP_MODULE_INFO("Narcade",0,2,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);
static volatile int running=1;
static int exit_cb(int a,int b,void *p){(void)a;(void)b;(void)p;running=0;return 0;}
static int callbacks(SceSize n,void *p){(void)n;(void)p;int cb=sceKernelCreateCallback("Narcade exit",exit_cb,0);sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
static void audio_cb(void *buffer,unsigned int frames,void *p){(void)p;game_audio(buffer,frames);}
int main(void){
 int th=sceKernelCreateThread("Narcade callbacks",callbacks,0x11,0x1000,0,0);if(th>=0)sceKernelStartThread(th,0,0);
 scePowerSetClockFrequency(333,333,166);
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 sceDisplaySetMode(0,480,272);
 uint32_t *buffers[2];buffers[0]=(uint32_t*)((uintptr_t)sceGeEdramGetAddr()|0x40000000);buffers[1]=buffers[0]+512*272;
 r3_init();game_init();sceIoMkdir("ms0:/PSP/SAVEDATA/NARCADE3D",0777);game_set_save_path("ms0:/PSP/SAVEDATA/NARCADE3D/PROGRESS.BIN");
 pspAudioInit();pspAudioSetChannelCallback(0,audio_cb,0);
 uint64_t before=sceKernelGetSystemTimeWide();int index=0;
 while(running){SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);uint64_t now=sceKernelGetSystemTimeWide();float dt=(now-before)/1000000.0f;before=now;
  game_tick(pad.Buttons,((float)pad.Lx-128)/127,((float)pad.Ly-128)/127,dt);game_draw(buffers[index],512);
  /* v1.2: pedir el cambio de buffer ANTES de esperar el vblank: el cambio ocurre en ese vblank
     y el siguiente fotograma se dibuja en el buffer ya oculto. (v1.0 esperaba primero y pedia
     el cambio despues, asi que redibujaba el buffer aun visible: parpadeo en la parte superior.) */
  sceDisplaySetFrameBuf(buffers[index],512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);sceDisplayWaitVblankStart();index^=1;
 }
 game_save();pspAudioEnd();r3_shutdown();sceKernelExitGame();return 0;
}
