/* v2.30 (Claude): tirones en pendientes. Sube y baja todas las calles norte-sur
   (las unicas con cuesta) a pie y en coche y mide lo que se ve: aceleracion
   vertical del ojo de la camara (como la calcula el renderizador) y cambios de
   velocidad en pantalla entre fotogramas. */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static float eye_y(void){
 /* misma formula que camera() en render3d.c */
 float px,pz;geo_project(g.x,g.y,&px,&pz);float d=g.cameraDistance>0?g.cameraDistance:cam_distance();
 float base=g.camBase!=0?g.camBase:geo_height(g.x,g.y)+g.lift;
 float ex=px-cosf(g.viewYaw)*d,ez=pz-sinf(g.viewYaw)*d,y=base+cam_eye();
 float lx,lz;geo_unproject(ex,ez,&lx,&lz);
 float floorY=geo_height(lx,lz)+12;
 return g.camClear!=0?fmaxf(y,g.camClear):fmaxf(y,floorY);
}
static void clear_traffic(void){for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}}
int main(void){
 double footEye=0,footSpd=0,carEye=0,carSpd=0;long fn=0,cn=0,footBig=0,carBig=0;
 for(int col=0;col<8;col++)for(int dir=0;dir<2;dir++)for(int mode=0;mode<2;mode++){
  game_init();fresh_game();g.screen=WORLD;clear_traffic();
  g.x=col*320+42;g.y=dir?2150:100;g.lift=0;
  if(mode){g.car=30;Car*c=&g.cars[30];c->x=g.x;c->y=g.y;c->a=dir?-PI*.5f:PI*.5f;c->parked=0;c->hp=100;c->speed=120;}
  g.viewYaw=proj_heading(g.x,g.y,dir?-PI*.5f:PI*.5f);
  float le=eye_y(),lde=0,lpx,lpz,lv=0;physics_project(g.x,g.y,&lpx,&lpz);
  for(int f=0;f<700;f++){
   if(mode){g.cars[30].hp=100;game_tick(B_CROSS,0,0,1.f/60);}else game_tick(f%2?B_CROSS:B_CROSS,0,-1,1.f/60);
   if(g.screen!=WORLD)g.screen=WORLD;
   float e=eye_y(),de=e-le,px,pz;physics_project(g.x,g.y,&px,&pz);float v=hypotf(px-lpx,pz-lpz);
   if(f>20&&v>.2f&&lv>.2f){float acc=fabsf(de-lde),sj=fabsf(v-lv)/v;
    if(mode){carEye+=acc;carSpd+=sj;cn++;if(acc>.15f)carBig++;}else{footEye+=acc;footSpd+=sj;fn++;if(acc>.15f)footBig++;}}
   le=e;lde=de;lv=v;lpx=px;lpz=pz;
  }
 }
 printf("A PIE : tiron vertical camara %.4f (grandes %ld) | tiron velocidad %.4f\n",footEye/fn,footBig,footSpd/fn);
 printf("COCHE : tiron vertical camara %.4f (grandes %ld) | tiron velocidad %.4f\n",carEye/cn,carBig,carSpd/cn);
 return 0;
}
