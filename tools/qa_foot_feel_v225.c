/* v2.25 (Claude): mide la "sensacion" del control a pie con una palanca analogica
   simulada como la mueve una persona (giros suaves, ruido, pasos por el centro).
   Compilar en el PC: gcc -O1 -DNARCADE_3D -Isrc tools/qa_foot_feel_v225.c <stubs.c> -lm */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static unsigned rs=12345;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static void clear_traffic(void){for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}}
int main(void){
 long pushed=0,openStall=0,wrongWay=0,bodyFlick=0,afterChange=0,speedDips=0;double chErr=0;
 for(int run=0;run<400;run++){
  game_init();fresh_game();g.screen=WORLD;clear_traffic();
  do{g.x=40+rnd()*(WORLD_W-80);g.y=40+rnd()*(WORLD_H-80);}while(!foot_free(g.x,g.y)||cm_lift(g.x,g.y,0)>1);
  g.lift=0;g.viewYaw=rnd()*6.28f;g.a=0;
  float stick=0,target=0,lastBody=0;int keys=run%3==0?0:B_CROSS,since=99;float lastSpeed=0;
  for(int f=0;f<600;f++){
   if(f%45==0){target=(rnd()*2-1)*PI;since=0;}
   float d=angle_delta(target,stick);stick+=clampf(d,-.22f,.22f);
   float mag=.85f+rnd()*.3f,noise=(rnd()-.5f)*.08f; /* analogico real: radio e imprecision */
   float ax=sinf(stick+noise)*mag,ay=-cosf(stick+noise)*mag;
   if(since==0&&rnd()<.5f){ax=ay=0;} /* a veces la palanca pasa por el centro */
   float px0,pz0;physics_project(g.x,g.y,&px0,&pz0);
   float want=g.viewYaw+stick;
   game_tick(keys,ax,ay,1.f/60);
   if(g.screen!=WORLD){g.screen=WORLD;continue;}
   since++;
   float px1,pz1;physics_project(g.x,g.y,&px1,&pz1);
   float mv=hypotf(px1-px0,pz1-pz0);
   if(f<20||!g.moveActive)continue;pushed++;
   float body=geo_heading(g.x,g.y,g.a);
   if(fabsf(angle_delta(body,lastBody))>.35f&&f>21)bodyFlick++;lastBody=body;
   float expect=g.footSpeed/60.f*.9f;
   if(mv<expect*.25f){
     float tx,ty;physics_unproject(px0+cosf(want)*3,pz0+sinf(want)*3,&tx,&ty);
     if(foot_free(tx,ty)&&lift_ok(tx,ty))openStall++;
   }else{
     float got=atan2f(pz1-pz0,px1-px0);
     if(fabsf(angle_delta(got,want))>PI*.6f)wrongWay++;
     if(since>4&&since<20){chErr+=fabsf(angle_delta(got,want));afterChange++;}
     if(lastSpeed>0&&mv<lastSpeed*.6f)speedDips++;
     lastSpeed=mv;
   }
  }
 }
 printf("frames %ld | atascos con camino libre %ld (%.2f%%) | contra-sentido %ld (%.2f%%) | error tras cambiar la palanca %.1f grados | tirones cuerpo %ld | caidas de velocidad %ld (%.2f%%)\n",
  pushed,openStall,100.*openStall/pushed,wrongWay,100.*wrongWay/pushed,chErr/afterChange*57.3,bodyFlick,speedDips,100.*speedDips/pushed);
 assert(100.*openStall/pushed<.5);assert(100.*wrongWay/pushed<1);assert(chErr/afterChange<.25f);
 puts("PASS: control a pie fluido con analogico simulado.");
 return 0;
}
