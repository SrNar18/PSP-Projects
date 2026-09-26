#include <math.h>
#include <stdio.h>
#include "../src/game.c"
static void clear_traffic(void){for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;}}
int main(void){
 int stalls=0,backwards=0,blocked=0,paths=0;
 for(int row=0;row<7;row++)for(int col=0;col<8;col++)for(int dir=0;dir<4;dir++){
  game_init();fresh_game();g.screen=WORLD;clear_traffic();
  g.x=col*320+42;g.y=row*320+42;g.lift=cm_lift(g.x,g.y,0);
  if(!foot_free(g.x,g.y))continue;
  g.viewYaw=geo_heading(g.x,g.y,0);g.cameraVelocity=0;
  float ax=dir&1?-.7f:.7f,ay=dir&2?.7f:-.7f;
  float previousX=g.x,previousY=g.y,initialX=g.x,initialY=g.y;
  for(int t=0;t<45;t++){
   float p0x,p0z;physics_project(g.x,g.y,&p0x,&p0z);
   game_tick(0,ax,ay,1.f/60);
   float mx=g.x-previousX,my=g.y-previousY;
   /* v2.25 (Claude): "hacia atras" se mide en pantalla (espacio proyectado),
      que es donde el controlador calcula el rumbo. */
   float p1x,p1z;physics_project(g.x,g.y,&p1x,&p1z);float dirP=g.frameYaw+g.stickAngle+g.wallGuide;
   if((p1x-p0x)*cosf(dirP)+(p1z-p0z)*sinf(dirP)<-.05f)backwards++;
   if(t>=2&&hypotf(mx,my)<.001f&&g.x>20&&g.x<WORLD_W-20){stalls++;if(t>=2&&t<10)blocked++;} /* v2.29: 2 fotogramas de arranque del filtro de picos */
   previousX=g.x;previousY=g.y;
  }
  paths++;
  if(hypotf(g.x-initialX,g.y-initialY)<4)printf("STUCK cell %d,%d dir %d at %.1f,%.1f heading %.2f\n",col,row,dir,g.x,g.y,g.moveYaw);
 }
 printf("paths=%d stalls=%d earlyBlocked=%d backwards=%d\n",paths,stalls,blocked,backwards);
 return backwards||blocked?1:0;
}
