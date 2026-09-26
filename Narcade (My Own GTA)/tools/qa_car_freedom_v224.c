#include <stdio.h>
#include "../src/game.c"
static unsigned rs=777;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static void clear_traffic(void){for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}}
int main(){
 long stuckSteer=0,frames=0,hits=0,glancing=0,stuck=0,turnBlocked=0;double dist=0;
 for(int run=0;run<300;run++){
  game_init();fresh_game();g.screen=WORLD;clear_traffic();
  g.car=30;Car*c=&g.cars[30];c->parked=0;c->hp=100;c->speed=0;
  do{c->x=40+rnd()*(WORLD_W-80);c->y=40+rnd()*(WORLD_H-80);c->a=rnd()*6.28f;}while(!car_free_at(c,c->x,c->y));
  g.x=c->x;g.y=c->y;
  float steer=0;int stuckT=0,sst=0;
  for(int f=0;f<900;f++){
   if(f%50==0)steer=(rnd()<.4f)?0:(rnd()*2-1);
   float sp0=c->speed,a0=c->a,x0=c->x,y0=c->y;
   int keys=B_CROSS;if(stuckT>40){keys=B_SQUARE;} if(stuckT>90)stuckT=0;
   c->hp=100;g.health=100;
   game_tick(keys,steer,0,1.f/60);
   if(g.screen!=WORLD){g.screen=WORLD;}
   if(g.car!=30){g.car=30;}
   frames++;dist+=hypotf(c->x-x0,c->y-y0);
   if(sp0>30&&fabsf(c->speed)<sp0*.5f){hits++;
     /* glancing: a step turned 30deg toward either side would have been free */
     int ok=0;for(int sd=-1;sd<=1;sd+=2){Car t=*c;t.a=a0+sd*.5f;if(car_free_at(&t,x0+cosf(t.a)*3,y0+sinf(t.a)*3))ok=1;}
     if(ok)glancing++;}
   if(fabsf(steer)>.5f&&fabsf(angle_delta(c->a,a0))<1e-4f&&fabsf(c->speed)>1)turnBlocked++;
   if(fabsf(c->speed)<5)stuckT++;else stuckT=0; if(fabsf(c->speed)<5&&fabsf(steer)>.5f)sst++;else sst=0; if(sst==60)stuckSteer++;
   if(stuckT==40)stuck++;
  }
 }
 printf("frames %ld avgspeed %.1f  hits %ld (glancing that could slide: %ld)  stuck>0.7s %ld  steer-ignored frames %ld\n",frames,dist/frames*60,hits,glancing,stuck,turnBlocked,stuckSteer);
}
