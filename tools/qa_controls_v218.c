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
 g.x=962;g.y=962;g.viewYaw=PI*.5f;
 if(sprint)g.sprintTime=10;
 for(int i=0;i<60;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,0,-1,1.f/60);}
 float initial=g.moveYaw;
 for(int i=0;i<70;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,1,0,1.f/60);}
 assert(fabsf(angle_delta(g.moveYaw,initial+PI*.5f))<.15f);
 float settled=g.moveYaw,px=g.x,py=g.y;
 for(int i=0;i<60;i++){if(sprint){g.sprintTime=10;g.stamina=100;}game_tick(pace,1,0,1.f/60);}
 assert(fabsf(angle_delta(g.moveYaw,settled))<.03f);
 fprintf(stderr,"foot motion: %.1f,%.1f -> %.1f,%.1f; speed %.1f\n",px,py,g.x,g.y,g.footSpeed);
 assert(hypotf(g.x-px,g.y-py)>18.f);
 printf("foot pace %d sprint %d: held turn stable at %.2f rad, camera %.2f rad\n",pace,sprint,settled,g.viewYaw);
}
int main(void){
 foot_turn(0,0);foot_turn(B_CROSS,0);foot_turn(B_CROSS,1);
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.car=30;Car *c=&g.cars[g.car];c->x=1002;c->y=1002;c->a=0;c->speed=0;c->parked=0;c->hp=100;
 assert(car_free_at(c,c->x,c->y));
 for(int i=0;i<70;i++)game_tick(B_CROSS,1,0,1.f/60);
 printf("player car: x %.1f y %.1f heading %.2f speed %.1f\n",c->x,c->y,c->a,c->speed);
 assert(c->y>1018.f); /* crosses the traffic lane freely */
 assert(c->a>.2f);
 assert(car_free_at(c,c->x,c->y));
 game_init();fresh_game();g.screen=WORLD;clear_traffic();
 g.car=30;c=&g.cars[g.car];c->x=1002;c->y=1002;c->a=0;c->speed=0;c->parked=0;c->hp=100;
 Car *other=&g.cars[31];other->x=1080;other->y=1002;other->a=0;other->speed=0;other->parked=1;
 for(int i=0;i<90;i++)game_tick(B_CROSS,0,0,1.f/60);
 float nx,ny,depth;
 assert(car_free_at(c,c->x,c->y));
 assert(!car_overlap(c,other,&nx,&ny,&depth)||depth<1.5f);
 assert(c->x<other->x);
 puts("PASS: walking/jogging/sprinting turns remain stable; player drives outside traffic lanes and collides with other cars.");
}
