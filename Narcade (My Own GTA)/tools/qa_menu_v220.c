#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/game.c"

int main(void){
 game_init();g.screen=TITLE;g.titleStage=1;g.menu=0;
 unsigned before=menuEvent;
 game_tick(B_DOWN,0,0,1.f/60);
 assert(g.menu==3&&menuEvent==before+1);
 short samples[4410*2];game_audio(samples,4410);
 int peak=0,tail=0;
 for(int i=0;i<4410*2;i++){
  int v=abs(samples[i]);if(v>peak)peak=v;
  if(i>4000*2&&v>tail)tail=v;
 }
 assert(peak>500&&peak<6000&&tail<60);
 FILE *out=fopen("assets/menu-click-v220.wav","wb");assert(out);
 uint32_t size=36+sizeof(samples),fmt=16,rate=44100,bytes=rate*4,data=sizeof(samples);
 uint16_t pcm=1,channels=2,align=4,bits=16;
 fwrite("RIFF",1,4,out);fwrite(&size,4,1,out);fwrite("WAVEfmt ",1,8,out);
 fwrite(&fmt,4,1,out);fwrite(&pcm,2,1,out);fwrite(&channels,2,1,out);
 fwrite(&rate,4,1,out);fwrite(&bytes,4,1,out);fwrite(&align,2,1,out);
 fwrite(&bits,2,1,out);fwrite("data",1,4,out);fwrite(&data,4,1,out);
 fwrite(samples,1,sizeof(samples),out);fclose(out);
 printf("PASS: menu selection emits clean 90 ms cue (peak %d, tail %d).\n",peak,tail);
}
