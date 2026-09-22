#define main campaign_main
#include "qa.c"
#undef main
static void setup(void){
 game_init();fresh_game();finishdialog();g.screen=WORLD;g.car=-1;
 g.x=700;g.y=42;g.viewYaw=0;g.mission=36;g.prev=0;
 for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
}
static void move(unsigned button,int frames){for(int i=0;i<frames;i++)game_tick(button,0,-1,1.f/60);}
int main(void){
 setup();move(0,90);assert(g.footSpeed>41&&g.footSpeed<43);float walk=g.x-700;
 setup();move(B_CROSS,90);assert(g.footSpeed>73&&g.footSpeed<75);assert(!g.sprintTime);float trot=g.x-700;assert(trot>walk*1.4f);
 setup();for(int i=0;i<90;i++)move(i%12==0?B_CROSS:0,1);
 assert(g.sprintTime>0&&g.footSpeed>98);assert(g.x-700>trot);
 move(B_CROSS,120);assert(g.sprintTime==0&&g.footSpeed>73&&g.footSpeed<75);
 move(0,75);assert(g.footSpeed>41&&g.footSpeed<43);
 puts("PASS: walking < held-X jogging < repeated-X sprint; hold never auto-sprints; rhythm expiry returns to jogging/walking.");
 setup();for(int i=0;i<120;i++)move(i%40==0?B_CROSS:0,1);assert(!g.sprintTime);
 for(int i=0;i<60;i++)game_tick(i%12==0?B_CROSS:0,0,0,1.f/60);
 assert(!g.sprintTime&&g.runTaps==0&&g.footSpeed==0);
 move(B_CROSS,20);assert(!g.sprintTime);
 game_tick(B_START,0,0,1.f/60);game_tick(0,0,0,1.f/60);assert(g.runTaps==0&&!g.sprintTime);
 puts("PASS: slow isolated taps, stationary taps and pause cannot accumulate a sprint.");
 setup();g.x=130;g.y=130;assert(solid(g.x,g.y));move(B_CROSS,90);
 assert(g.footTravel==0&&g.motion==0&&g.gaitPhase==0);
 puts("PASS: blocked movement does not cycle the walking animation.");
 setup();
 /* Exercise stamina independently of map obstacles at both supported rates. */
 for(int fps=30;fps<=60;fps+=30){
  g.stamina=100;g.exhausted=0;g.runTaps=0;g.tapAge=10;g.sprintTime=0;
  float dt=1.f/fps;int frames=0;
  while(!g.exhausted&&frames<fps*8){g.pressed=frames%(fps/5)==0?B_CROSS:0;g.held=g.pressed;foot_pace(1,dt);g.footTravel=1;stamina_tick(dt);frames++;}
  assert(g.exhausted&&g.stamina==0&&frames>fps*5&&frames<fps*8);
  for(int i=0;i<fps*7;i++){g.pressed=i%(fps/5)==0?B_CROSS:0;g.held=g.pressed;foot_pace(1,dt);g.footTravel=1;stamina_tick(dt);assert(g.exhausted&&g.sprintTime==0);}
  assert(g.footSpeed>73&&g.footSpeed<75&&g.stamina<100);
  for(int i=0;i<fps*2;i++){g.pressed=0;foot_pace(1,dt);g.footTravel=1;stamina_tick(dt);}
  assert(!g.exhausted&&g.stamina==100);
  for(int i=0;i<fps;i++){g.pressed=i%(fps/5)==0?B_CROSS:0;g.held=g.pressed;foot_pace(1,dt);g.footTravel=1;stamina_tick(dt);}
  assert(g.sprintTime>0&&g.stamina<100&&g.footSpeed>98);
 }
 puts("PASS: stamina drains in six sprint seconds, exhaustion locks sprint until full recovery, automatic jogging, repeatable at 30/60 Hz.");
 return 0;
}
