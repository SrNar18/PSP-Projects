#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspge.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <pspiofilemgr.h>
#include <stdint.h>
#include <string.h>
#include "game.h"
PSP_MODULE_INFO("Narcade",0,1,1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER|THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(4096);
#define FB_STRIDE 512
#define FB_H 272
#define FB_BYTES (FB_STRIDE*FB_H*4)
/* v1.1: se dibuja en RAM (con cache, rapido) y se vuelca a VRAM con memcpy (escritura secuencial).
   Escribir pixel a pixel en VRAM sin cache era muy lento en hardware real. */
static uint32_t backbuf[FB_STRIDE*FB_H] __attribute__((aligned(64)));
static volatile int running=1;
static int exit_cb(int a,int b,void *p){(void)a;(void)b;(void)p;running=0;return 0;}
static int callbacks(SceSize n,void *p){(void)n;(void)p;int cb=sceKernelCreateCallback("Narcade exit",exit_cb,0);sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
static void audio_cb(void *buffer,unsigned int frames,void *p){(void)p;game_audio(buffer,frames);}
int main(void){
 int th=sceKernelCreateThread("Narcade callbacks",callbacks,0x11,0x1000,0,0);if(th>=0)sceKernelStartThread(th,0,0);
 scePowerSetClockFrequency(333,333,166);
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 sceDisplaySetMode(0,480,272);
 /* Dos buffers en VRAM (direccion sin cache: el DMA escribe directo). */
 uint32_t *vram[2];vram[0]=(uint32_t*)((uintptr_t)sceGeEdramGetAddr()|0x40000000);vram[1]=vram[0]+FB_STRIDE*FB_H;
 memset(vram[0],0,FB_BYTES*2);
 sceDisplaySetFrameBuf(vram[0],FB_STRIDE,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_IMMEDIATE);
 game_init();sceIoMkdir("ms0:/PSP/SAVEDATA/NARCADE",0777);game_set_save_path("ms0:/PSP/SAVEDATA/NARCADE/PROGRESS.BIN");
 pspAudioInit();pspAudioSetChannelCallback(0,audio_cb,0);
 uint64_t before=sceKernelGetSystemTimeWide();int index=1;
 while(running){SceCtrlData pad;sceCtrlPeekBufferPositive(&pad,1);uint64_t now=sceKernelGetSystemTimeWide();float dt=(now-before)/1000000.0f;before=now;
  game_tick(pad.Buttons,((float)pad.Lx-128)/127,((float)pad.Ly-128)/127,dt);
  game_draw(backbuf,FB_STRIDE);
  /* Volcar el backbuffer al buffer de VRAM que NO se esta mostrando. */
  memcpy(vram[index],backbuf,FB_BYTES);
  /* v1.1: esperar el vblank y cambiar de buffer de forma INMEDIATA. Antes se usaba
     NEXTFRAME y se empezaba a redibujar el buffer aun visible: parpadeo en PSP real. */
  sceDisplayWaitVblankStart();
  sceDisplaySetFrameBuf(vram[index],FB_STRIDE,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_IMMEDIATE);
  index^=1;
 }
 game_save();pspAudioEnd();sceKernelExitGame();return 0;
}
