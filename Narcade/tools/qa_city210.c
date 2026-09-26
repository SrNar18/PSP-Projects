#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 for(int fps=20;fps<=60;fps+=10){
  game_init();g.stamina=100;g.tapAge=10;
  for(int f=0;f<fps;f++){g.pressed=f%(fps/5)==0?B_CROSS:0;g.held=g.pressed;foot_pace(1,1.f/fps);}
  assert(g.sprintTime>0);
  /* A realistic irregular cadence, including a long gap and missed samples,
     must not require another three presses while the run remains active. */
  for(int f=0;f<fps*4;f++){
   g.pressed=f%(fps*8/10)==0?B_CROSS:0;g.held=g.pressed;
   foot_pace(f%fps!=fps/2,1.f/fps);
   assert(g.sprintTime>0&&g.footSpeed>100);
  }
  g.pressed=0;g.held=B_CROSS;
  for(int f=0;f<fps*2;f++)foot_pace(1,1.f/fps);
  assert(g.sprintTime==0&&g.footSpeed>73&&g.footSpeed<75);
 }
 puts("PASS: irregular running cadence and brief stick dropout at 20/30/40/50/60 Hz; holding returns to jog.");
 game_init();fresh_game();g.screen=WORLD;g.prev=0;
 for(int i=0;i<3;i++){game_latch_cross(B_CROSS);game_tick(0,0,-1,.05f);for(int j=0;j<3;j++)game_tick(0,0,-1,.05f);}
 assert(g.sprintTime>0&&!(g.held&B_CROSS));
 puts("PASS: short taps captured between rendered frames initiate sprint even when the current button is released.");
 game_init();g.car=-1;g.lift=0;g.inMetro=0;
 Car c={700,42,0,40,100,0,0,0};
 car_push(&c,40,0,&g.x,&g.y);assert(traffic_blocked(&c));
 car_push(&c,-40,0,&g.x,&g.y);assert(!traffic_blocked(&c));
 car_push(&c,40,30,&g.x,&g.y);assert(!traffic_blocked(&c));
 car_push(&c,40,0,&g.x,&g.y);g.lift=30;assert(!traffic_blocked(&c));
 puts("PASS: horns triggered only by an obstacle ahead in lane, not behind, sidewalk or elevated Metro.");
 game_init();g.screen=WORLD;g.mission=36;g.active=1;g.car=-1;
 for(int i=0;i<CAR_COUNT;i++)g.cars[i]=(Car){-10000-i*100,-10000,0,0,100,0,1,0};
 g.cars[0]=c;car_push(&c,40,0,&g.x,&g.y);unsigned before=hornEvent;
 for(int i=0;i<120;i++)game_tick(0,0,0,.05f);
 assert(hornEvent==before+1&&g.cars[0].speed==0);
 car_push(&c,40,30,&g.x,&g.y);game_tick(0,0,0,.05f);assert(g.cars[0].speed>0);
 puts("PASS: blocked traffic stops, honks once in six seconds and resumes after player clears lane.");
 short samples[2048];audioEnabled=0;audioAmbient=1;unsigned initial=hornEvent;
 for(int f=0;f<1000;f++)game_audio(samples,1024);
 assert(hornEvent==initial);g.cars[0]=c;request_horn(0);assert(hornEvent==initial+1);
 game_audio(samples,1024);long energy=0;for(int i=0;i<2048;i++)energy+=abs(samples[i]);assert(energy>0);
 puts("PASS: no timer horns in 23 seconds of ambience; event horn and PCM mix active.");
 return 0;
}
