/* v2.41/v2.44 (Claude): vista previa de la pantalla de carga (build/loading-NN.ppm) simulando 13 s de carga. */
#include <stdio.h>
#include "../src/game.c"
static uint32_t shot[512*272];
static void save(int n){char p[64];sprintf(p,"build/loading-%02d.ppm",n);FILE *f=fopen(p,"wb");fprintf(f,"P6 480 272 255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=shot[y*512+x];unsigned char px3[3]={c&255,(c>>8)&255,(c>>16)&255};fwrite(px3,1,3,f);}fclose(f);}
int main(void){game_init();fresh_game();game_load_begin();int n=0;
 for(float t=0;t<13.01f;t+=.25f){game_load_clock(t);int stage=(int)(t/.55f);if(stage>19)stage=19;game_load_present(shot,512,stage,t>12.9f);
  if(((int)(t*4))%4==0||(t>4.9f&&t<5.8f))save(n++);}
 printf("%d cuadros",n);putchar(10);return 0;}
