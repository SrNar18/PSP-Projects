#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);exit(1);}}while(0)
int main(void){
 R3Scene s={0};view=&s;s.cameraDistance=60;s.x=1002;s.z=1060;s.target=-1;day_update(0);geographic=0;rigid=0;clipEnabled=0;
 for(int w=0;w<8;w++)for(int aim=0;aim<2;aim++)for(int gait=0;gait<3;gait++)for(int f=0;f<24;f++){
  s.weapon=w;s.aiming=aim;s.motion=gait;s.gaitPhase=f*PI/12;s.aimPitch=sinf(f*PI/12)*.6f;s.punch=w==0?.32f*f/24:0;pose_prepare();
  memset(used,0,sizeof(used));overflow=0;localScale=PERSON_SCALE;equipped_weapon(0,0,0);localScale=1;CHECK(!overflow);
  for(int m=0;m<MAT_COUNT;m++)for(int v=0;v<used[m];v++)CHECK(isfinite(mesh[m][v].x)&&isfinite(mesh[m][v].y)&&isfinite(mesh[m][v].z));
  for(int u=0;u<PLAYER_VERTEX_COUNT;u++){const PlayerVertex *a=&player_mesh[u];Point p=player_pose(player_surface(a),a->bone);CHECK(isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z));}
  if(w>=3&&w<=6){Point r=pose_hand(3),l=pose_hand(4);CHECK(l.x>r.x+1&&fabsf(l.z-r.z)<2);}
 }
 puts("PASS 1152 weapon/aim/gait poses; finite body and gun vertices; supporting hand reaches fore-end");
 s.aiming=1;s.yaw=.3f;s.aimPitch=.25f;s.cameraPitch=.25f;s.motion=0;camera(&s);float x,z,sx,sy;geo_project(s.x,s.z,&x,&z);
 geo_unproject(x+cosf(s.yaw)*90-sinf(s.yaw)*5,z+sinf(s.yaw)*90+cosf(s.yaw)*5,&x,&z);
 float height=eye.y+(target.y-eye.y)*128/98-geo_height(x,z);
 CHECK(r3_target_screen(x,z,height,&sx,&sy));CHECK(fabsf(sx-240)<.01f&&fabsf(sy-136)<.05f);
 puts("PASS HUD crosshair matches renderer camera projection");
 for(int i=0;i<20;i++){s.cameraPitch=-.65f+i*1.3f/19;camera(&s);float lx,lz;geo_unproject(eye.x,eye.z,&lx,&lz);CHECK(eye.y>=geo_height(lx,lz)+1.99f);}
 player_index_build();CHECK(playerUniqueCount<PLAYER_UNIQUE_MAX);printf("PASS aim camera stays above terrain; %d distinct player vertices fit cache\n",playerUniqueCount);
 return 0;
}
