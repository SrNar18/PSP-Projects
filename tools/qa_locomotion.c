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
 setup();move(0,90);assert(g.footSpeed>71&&g.footSpeed<73);float walk=g.x-700;
 setup();move(B_CROSS,90);assert(g.footSpeed>110&&g.footSpeed<112);assert(!g.sprintTime);float trot=g.x-700;assert(trot>walk*1.4f);
 setup();for(int i=0;i<90;i++)move(i%12==0?B_CROSS:0,1);
 assert(g.sprintTime>0&&g.footSpeed>148);assert(g.x-700>trot);
 move(B_CROSS,75);assert(g.sprintTime==0&&g.footSpeed>110&&g.footSpeed<112);
 move(0,75);assert(g.footSpeed>71&&g.footSpeed<73);
 puts("PASS: walking < held-X jogging < repeated-X sprint; hold never auto-sprints; rhythm expiry returns to jogging/walking.");
 setup();for(int i=0;i<120;i++)move(i%40==0?B_CROSS:0,1);assert(!g.sprintTime);
 for(int i=0;i<60;i++)game_tick(i%12==0?B_CROSS:0,0,0,1.f/60);
 assert(!g.sprintTime&&g.runTaps==0&&g.footSpeed==0);
 move(B_CROSS,20);assert(!g.sprintTime);
 game_tick(B_START,0,0,1.f/60);game_tick(0,0,0,1.f/60);assert(g.runTaps==0&&!g.sprintTime);
 puts("PASS: slow isolated taps, stationary taps and pause cannot accumulate a sprint.");
 setup();g.x=200;g.y=200;move(B_CROSS,90);
 assert(g.footTravel==0&&g.motion==0&&g.gaitPhase==0);
 puts("PASS: blocked movement does not cycle the walking animation.");
 return 0;
}
