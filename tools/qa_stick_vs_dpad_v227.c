/* v2.27 (Claude): la cruceta iba bien y el joystick no. Con la misma direccion
   mantenida, compara cuanto se tuerce el rumbo en pantalla con cruceta (sin ruido)
   y con analogico (ruido real de la palanca de la PSP, +-3 cuentas de 127, y una
   inclinacion que oscila un poco como hace el pulgar). */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static unsigned rs=777;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static float run(int analog,float angle,float *wobble){
 game_init();fresh_game();g.screen=WORLD;
 for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
 g.x=1002;g.y=1060;g.lift=0;g.viewYaw=proj_heading(g.x,g.y,PI*.5f);
 float first=0,last=0,sumTurn=0;float lx=0,lz=0,px,pz;
 for(int f=0;f<300;f++){
  float ax,ay;
  if(analog){float q=127.f,thumb=.9f+.08f*sinf(f*.13f);
   ax=roundf(sinf(angle)*thumb*q+(rnd()-.5f)*6)/q;ay=roundf(-cosf(angle)*thumb*q+(rnd()-.5f)*6)/q;
   game_tick(0,ax,ay,1.f/60);}
  else{unsigned b=0;/* cruceta: diagonal = dos botones */
   if(sinf(angle)>.38f)b|=B_RIGHT;if(sinf(angle)<-.38f)b|=B_LEFT;if(cosf(angle)>.38f)b|=B_UP;if(cosf(angle)<-.38f)b|=B_DOWN;
   game_tick(b,0,0,1.f/60);}
  physics_project(g.x,g.y,&px,&pz);
  if(f>=2){float h=atan2f(pz-lz,px-lx);if(f==30)first=h;if(f>30){sumTurn+=fabsf(angle_delta(h,last));}last=h;}
  lx=px;lz=pz;
 }
 *wobble=sumTurn;
 return angle_delta(last,first);
}
int main(void){
 float angles[]={0,.785f,-.785f,1.57f};
 for(int i=0;i<4;i++){
  float wd,wa;float d=run(0,angles[i],&wd),a=run(1,angles[i],&wa);
  printf("direccion %5.2f rad | cruceta: giro total %+.2f rad, zigzag %.2f | joystick: giro total %+.2f rad, zigzag %.2f\n",angles[i],d,wd,a,wa);
 }
 return 0;
}
