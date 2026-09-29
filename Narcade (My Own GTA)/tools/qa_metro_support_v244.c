#include <assert.h>
#include "../src/game.c"
int main(void){
 game_init();fresh_game();
 for(int bz=1;bz<=5;bz+=2)for(int side=-1;side<=1;side+=2)
  for(int k=0;k<3;k++){
   float x=CM_METRO_X+side*15,z=bz*320+80+k*80;
   assert(cm_metro_support(x,z));
   g.lift=0;assert(world_solid(x,z));
   g.lift=CM_PLAT_H;assert(!world_solid(x,z));
   assert(car_solid(x,z));
  }
 puts("PASS: 18 station supports collide at ground and remain walkable above on the platform.");
 return 0;
}
