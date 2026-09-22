#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
int main(void){
 R3Scene s={0};view=&s;geographic=0;rigid=0;clipEnabled=0;day_update(0);
 for(int id=1;id<=7;id++)for(int gait=0;gait<3;gait++)for(int frame=0;frame<24;frame++){
  s.weapon=id;s.motion=gait;s.gaitPhase=frame*PI/12;pose_prepare();
  memset(used,0,sizeof(used));overflow=0;localScale=PERSON_SCALE;equipped_weapon(0,0,0);localScale=1;
  assert(used[20]>0&&!overflow);
  Point grip=player_pose(point(.95f,12.8f,-3.95f),3);
  for(int i=0;i<used[20];i++){
   Vertex v=mesh[20][i];assert(isfinite(v.x)&&isfinite(v.y)&&isfinite(v.z));
   assert(fabsf(v.x-grip.x*PERSON_SCALE)<12&&fabsf(v.z-grip.z*PERSON_SCALE)<2);
  }
 }
 s.weapon=0;memset(used,0,sizeof(used));equipped_weapon(0,0,0);assert(!used[20]);
 puts("PASS: seven meshes, 504 gait poses, finite vertices, attached to grip; fists render no weapon.");return 0;
}
