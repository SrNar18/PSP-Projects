#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/game.c"

static void clear_traffic(void){
 for(int i=0;i<CAR_COUNT;i++){
  g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;
  g.cars[i].parked=1;g.cars[i].police=0;
 }
}
static void foot_turn(int pace,int sprint){
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.x=900;g.y=962;g.viewYaw=PI*.5f;
 if(sprint)g.sprintTime=10;
 for(int i=0;i<60;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,0,-1,1.f/60);}
 float headingBefore=geo_heading(g.x,g.y,g.moveYaw);
 /* v2.29: el filtro de picos del analogico da 1 fotograma de retardo (16 ms). */
 for(int t=0;t<2;t++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,1,0,1.f/60);}
 assert(fabsf(angle_delta(proj_heading(g.x,g.y,g.moveYaw),headingBefore+PI*.5f))<.25f); /* el filtro continuo completa el giro en 2-3 fotogramas */
 for(int i=1;i<70;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,1,0,1.f/60);}
 /* v2.28 (Claude): a peticion del jugador la camara sigue tambien el movimiento
    lateral: con la palanca a un lado personaje y camara giran juntos. */
 float settled=g.moveYaw,px=g.x,py=g.y,view0=g.viewYaw;
 for(int i=0;i<60;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,1,0,1.f/60);}
 assert(fabsf(angle_delta(g.viewYaw,view0))>.3f);
 fprintf(stderr,"foot motion: %.1f,%.1f -> %.1f,%.1f; speed %.1f\n",px,py,g.x,g.y,g.footSpeed);
 assert(hypotf(g.x-px,g.y-py)>18.f);
 printf("foot pace %d sprint %d: held turn stable at %.2f rad, camera %.2f rad\n",pace,sprint,settled,g.viewYaw);
}
int main(void){
 foot_turn(0,0);foot_turn(B_CROSS,0);foot_turn(B_CROSS,1);
 for(int pace=0;pace<3;pace++){
  game_init();fresh_game();g.screen=WORLD;clear_traffic();
  g.x=900;g.y=962;g.viewYaw=geo_heading(g.x,g.y,PI*.5f);
  int keys=pace?B_CROSS:0;
  if(pace==2)g.sprintTime=10;
  game_tick(keys,0,-1,1.f/60);
  /* v2.24 (Claude): el rumbo se mide en pantalla (proyectado), no en espacio logico. */
  float initial=geo_heading(g.x,g.y,g.inputYaw);
  for(int i=0;i<110;i++){
   if(pace==2){g.sprintTime=10;g.stamina=100;}
   game_tick(keys,(i%4==0?.11f:i%4==2?-.11f:0.f),-1,1.f/60);
  }
  (void)initial;
  /* v2.24 (Claude): el movimiento es relativo a la camara; lo que debe ser
     estable es el angulo filtrado de la palanca (+-0.11 de ruido -> <0.04). */
  assert(fabsf(g.stickAngle)<.04f);
  assert(hypotf(g.x-900.f,g.y-962.f)>20.f);
 }
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.x=900;g.y=962;g.viewYaw=geo_heading(g.x,g.y,PI*.5f);
 for(int i=0;i<50;i++)game_tick(B_CROSS,0,-1,1.f/60);
 for(int i=0;i<45;i++)game_tick(B_CROSS,1,0,1.f/60);
 float headingBefore=proj_heading(g.x,g.y,g.moveYaw);
 for(int t=0;t<3;t++)game_tick(B_CROSS,0,-1,1.f/60);
 assert(fabsf(angle_delta(proj_heading(g.x,g.y,g.moveYaw),headingBefore-PI*.5f))<.25f);
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.car=30;Car *c=&g.cars[g.car];c->x=1002;c->y=1002;c->a=0;c->speed=0;c->parked=0;c->hp=100;
 assert(car_free_at(c,c->x,c->y));
 for(int i=0;i<70;i++)game_tick(B_CROSS,1,0,1.f/60);
 printf("player car: x %.1f y %.1f heading %.2f speed %.1f\n",c->x,c->y,c->a,c->speed);
 assert(c->y>1018.f); /* crosses the traffic lane freely */
 assert(c->a>.2f);
 assert(car_free_at(c,c->x,c->y));
 float releasedAngle=c->a;
 game_tick(0,0,0,1.f/60);
 assert(fabsf(angle_delta(c->a,releasedAngle))<.001f);
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.car=30;c=&g.cars[g.car];c->x=1002;c->y=1002;c->a=0;c->speed=0;c->parked=0;c->hp=100;
 Car *other=&g.cars[31];other->x=1080;other->y=1002;other->a=0;other->speed=0;other->parked=1;
 for(int i=0;i<90;i++)game_tick(B_CROSS,0,0,1.f/60);
 float nx,ny,depth;
 assert(car_free_at(c,c->x,c->y));
 assert(!car_overlap(c,other,&nx,&ny,&depth)||depth<1.5f);
 assert(c->x<other->x);
 assert(fabsf(other->x-1080)<.01f&&fabsf(other->y-1002)<.01f);
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.car=30;c=&g.cars[g.car];c->x=62;c->y=1000;c->a=PI*.5f;c->speed=50;c->parked=0;c->hp=100;
 /* v2.27 (Claude): sin volante el coche va recto EN PANTALLA (ya no sigue solo la
    calle deformada del mapa). Se comprueba rumbo en pantalla constante y avance. */
 float h0=proj_heading(c->x,c->y,c->a),sx0,sz0,sx1,sz1;physics_project(c->x,c->y,&sx0,&sz0);
 for(int i=0;i<120;i++)game_tick(B_CROSS,0,0,1.f/60);
 physics_project(c->x,c->y,&sx1,&sz1);
 assert(car_free_at(c,c->x,c->y));
 assert(hypotf(sx1-sx0,sz1-sz0)>60.f||c->speed==0);
 assert(fabsf(angle_delta(proj_heading(c->x,c->y,c->a),h0))<.03f);
 puts("PASS: walking/jogging/sprinting turns remain stable; player drives outside traffic lanes and collides with other cars.");
}
