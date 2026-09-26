/* v2.26 (Claude): tirones visibles al caminar. Mide, fotograma a fotograma, lo que
   el jugador ve: posicion del ojo de la camara, giro de la camara y avance en
   pantalla del personaje. Un tiron = cambio brusco respecto al fotograma anterior.
   Compilar: gcc -O1 -DNARCADE_3D -Isrc tools/qa_smooth_v226.c <stubs.c> -lm */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static unsigned rs=4242;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static void eye_of(float *ex,float *ey,float *ez){
 float px,pz;geo_project(g.x,g.y,&px,&pz);float d=g.cameraDistance>0?g.cameraDistance:cam_distance();
 float h=geo_height(g.x,g.y)+g.lift;*ex=px-cosf(g.viewYaw)*d;*ez=pz-sinf(g.viewYaw)*d;*ey=h+cam_eye();
 float lx,lz;geo_unproject(*ex,*ez,&lx,&lz);*ey=fmaxf(*ey,geo_height(lx,lz)+12);
}
int main(void){
 long frames=0,eyeJerk=0,yawJerk=0,distJerk=0,moveJerk=0,heightJerk=0;
 for(int run=0;run<300;run++){
  game_init();fresh_game();g.screen=WORLD;
  for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
  do{g.x=40+rnd()*(WORLD_W-80);g.y=40+rnd()*(WORLD_H-80);}while(!foot_free(g.x,g.y)||cm_lift(g.x,g.y,0)>1);
  g.lift=0;g.viewYaw=rnd()*6.28f;
  float stick=0,target=0;int keys=run%2?B_CROSS:0;
  float lex=0,ley=0,lez=0,lvx=0,lvy=0,lyaw=0,ld=0,lmv=0,lh=0;
  for(int f=0;f<480;f++){
   if(f%90==0)target=(rnd()*2-1)*1.2f;
   stick+=clampf(angle_delta(target,stick),-.08f,.08f);
   float px0,pz0;geo_project(g.x,g.y,&px0,&pz0);
   game_tick(keys,sinf(stick),-cosf(stick),1.f/60);
   if(g.screen!=WORLD){g.screen=WORLD;continue;}
   float ex,ey,ez;eye_of(&ex,&ey,&ez);float px1,pz1;geo_project(g.x,g.y,&px1,&pz1);
   float vx=ex-lex,vy=ey-ley,vz=ez-lez;
   float mv=hypotf(px1-px0,pz1-pz0),h=geo_height(g.x,g.y);
   if(f>30){frames++;
    /* aceleracion del ojo (cambio de velocidad entre fotogramas) */
    if(hypotf(vx-lvx,vz-lvy)>1.2f)eyeJerk++;
    if(fabsf(vy-(ley-lh))>1.0f&&0)heightJerk++;
    if(fabsf(angle_delta(g.viewYaw,lyaw))>.06f)yawJerk++;
    if(fabsf(g.cameraDistance-ld)>1.5f)distJerk++;
    if(lmv>.3f&&fabsf(mv-lmv)>lmv*.35f)moveJerk++;
   }
   lvx=vx;lvy=vz;lex=ex;ley=ey;lez=ez;lyaw=g.viewYaw;ld=g.cameraDistance;lmv=mv;lh=h;
  }
 }
 printf("fotogramas %ld | tirones ojo camara %ld (%.2f%%) | saltos de giro >3.4 grados/fot %ld (%.2f%%) | saltos distancia camara %ld (%.2f%%) | cambios bruscos de velocidad %ld (%.2f%%)\n",
  frames,eyeJerk,100.*eyeJerk/frames,yawJerk,100.*yawJerk/frames,distJerk,100.*distJerk/frames,moveJerk,100.*moveJerk/frames);
 return 0;
}
