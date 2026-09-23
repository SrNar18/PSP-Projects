/* v2.30 (Claude): reparto de CPU por fotograma en el PC. Dos mediciones separadas
   (logica y dibujo no pueden compilarse en la misma unidad):
   -DPART_LOGIC: game_tick con trafico;  sin el: r3_draw (geometria en CPU).
   La PSP es mucho mas lenta, pero el reparto relativo indica que optimizar. */
#include <stdio.h>
#include <time.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
#ifdef PART_LOGIC
#include "../src/game.c"
int main(void){
 game_init();fresh_game();g.screen=WORLD;g.x=1002;g.y=1180;g.viewYaw=proj_heading(g.x,g.y,PI*.5f);
 double t=0;int n=0;
 for(int f=0;f<2000;f++){double a=now();game_tick(B_CROSS,sinf(f*.01f),-cosf(f*.01f),1.f/60);if(g.screen!=WORLD)g.screen=WORLD;if(f>30){t+=now()-a;n++;}}
 printf("logica: %.4f ms/fotograma (PC)\n",t/n);return 0;
}
#else
#define R3_HOST
#include "../src/render3d.c"
int main(void){
 R3Scene s={0};s.cameraDistance=50;s.eyeHeight=34;s.target=-1;s.carCount=40;s.personCount=20;
 double tc=0,tr=0;int n=0;
 for(int f=0;f<600;f++){s.x=1002+(f%30)*20;s.z=1180;s.yaw=geo_heading(s.x,s.z,PI*.5f+f*.01f);
  for(int i=0;i<40;i++){s.cars[i].x=s.x+(i%8)*40-160;s.cars[i].z=s.z+(i/8)*50-100;}
  double a=now();r3_draw(NULL,&s);double b=now();tc+=b-a;n++;}
 printf("dibujo (toda la geometria en CPU): %.4f ms/fotograma (PC)\n",tc/n);return 0;
}
#endif
