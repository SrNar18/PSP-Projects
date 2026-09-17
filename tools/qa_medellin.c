#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 game_init();fresh_game();finishdialog();g.screen=WORLD;g.car=-1;g.x=700;g.y=42;g.viewYaw=-PI*.5f;
 for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
 for(int i=0;i<180;i++)game_tick(0,1,0,1.f/60);
 assert(g.x>850&&fabsf(g.y-42)<.01f);assert(fabsf(g.viewYaw)<.001f);
 game_tick(0,0,0,1.f/60);float yaw=g.viewYaw,x=g.x;
 for(int i=0;i<60;i++)game_tick(B_R,.15f,0,1.f/60);
 assert(fabsf(g.viewYaw-yaw)<.001f&&g.x==x);
 game_tick(0,0,0,1.f/60);game_tick(0,0,-1,1.f/60);assert(g.x>x);
 puts("PASS: stick follows heading, held lateral input stays straight, deadzone stable, L/R no camera rotation.");
 for(int zi=0;zi<=28;zi++)for(int xi=0;xi<=32;xi++){
  float a,b,x=xi*80,z=zi*80,rx,rz;geo_project(x,z,&a,&b);geo_unproject(a,b,&rx,&rz);
  assert(fabsf(rx-x)<.01f&&fabsf(rz-z)<.01f);assert(isfinite(geo_height(x,z)));
 }
 assert(geo_height(100,1022)>geo_height(1428,1022)+100);
 assert(geo_height(2400,382)>geo_height(1428,382)+180);
 for(int i=0;i<30;i++)assert(free_at(locations[i].x,locations[i].y,5));
 assert(foot_free(62,1022));assert(!foot_free(80,1022));
 g.cars[0]=(Car){700,42,0,0,100,0,1,0};assert(!foot_free(710,42));assert(foot_free(700,70));
 puts("PASS: geographic forward/inverse, western/eastern hill elevations, mission hubs, terminal offset and car/player collision.");
 g.screen=MAP;game_draw(frame,480);FILE *f=fopen("build/medellin-map.ppm","wb");assert(f);fprintf(f,"P6\n480 272\n255\n");
 for(int i=0;i<480*272;i++){unsigned c=frame[i];fputc(c&255,f);fputc((c>>8)&255,f);fputc((c>>16)&255,f);}fclose(f);
 return 0;
}
