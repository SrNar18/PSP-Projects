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
   game_tick(0,ax,ay,1.f/60);
   float mx=g.x-previousX,my=g.y-previousY;
   float wantX=cosf(g.moveYaw),wantY=sinf(g.moveYaw);
   if(mx*wantX+my*wantY<-.05f)backwards++;
   if(hypotf(mx,my)<.001f){stalls++;if(t<10)blocked++;}
   previousX=g.x;previousY=g.y;
  }
  paths++;
  if(hypotf(g.x-initialX,g.y-initialY)<4)printf("STUCK cell %d,%d dir %d at %.1f,%.1f heading %.2f\n",col,row,dir,g.x,g.y,g.moveYaw);
 }
 printf("paths=%d stalls=%d earlyBlocked=%d backwards=%d\n",paths,stalls,blocked,backwards);
 return backwards||blocked?1:0;
}
