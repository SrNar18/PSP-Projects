#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 game_init();fresh_game();g.screen=WORLD;g.mission=36;g.car=-1;g.noticeT=0;
 for(int bz=1;bz<=5;bz+=2){
  g.x=1470.5f;g.y=bz*320+318;g.lift=0;
  for(int k=0;k<120;k++){float y=g.y-.5f;assert(foot_free(g.x,y)&&lift_ok(g.x,y));g.y=y;g.lift=cm_lift(g.x,g.y,g.lift>15);}
  g.y=cm_station_z(bz);g.lift=CM_PLAT_H;g.metroZ=g.y;g.metroWait=6;assert(metro_boardable());interact();assert(g.inMetro);
  interact();assert(!g.inMetro&&g.lift==CM_PLAT_H&&cm_on_platform(g.x,g.y));
 }
 puts("PASS: pedestrian collision allows all three stair paths; boarding and exiting work after the story too.");return 0;
}
