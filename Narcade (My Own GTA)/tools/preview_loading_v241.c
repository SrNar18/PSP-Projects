/* v2.41 (Claude): vista previa de la pantalla de carga ilustrada (build/loading-NN.ppm). */
#include <stdio.h>
#include "../src/game.c"
static uint32_t shot[512*272];
static void save(int n){char p[64];sprintf(p,"build/loading-%02d.ppm",n);FILE *f=fopen(p,"wb");fprintf(f,"P6 480 272 255\n");
 for(int y=0;y<272;y++)for(int x=0;x<480;x++){uint32_t c=shot[y*512+x];unsigned char px[3]={c&255,(c>>8)&255,(c>>16)&255};fwrite(px,1,3,f);}fclose(f);}
int main(void){game_init();fresh_game();int n=0;
 for(int st=0;st<24;st++){if(game_load_stage(shot,512,st))break;save(n++);while(game_load_anim(shot,512,st)){}save(n++);}
 printf("%d cuadros",n);putchar(10);return 0;}
