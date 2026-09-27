#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL %s at %d\n",#c,__LINE__);exit(1);}}while(0)
int main(void){
 R3Scene s={0};view=&s;
 for(int g=0;g<3;g++){
  s.motion=g==0?1:g==1?1.7f:2.1f;
  for(int frame=0;frame<240;frame++){
   s.gaitPhase=2*PI*frame/240;pose_prepare();
   CHECK(footLift[0]>=0&&footLift[1]>=0);
   if(g==0)CHECK(footLift[0]<.0001f||footLift[1]<.0001f); /* walking has support */
   for(int i=0;i<2;i++){
    float dx=gaitHip[i].x-gaitAnkle[i].x,dy=gaitHip[i].y-gaitAnkle[i].y;
    CHECK(dx*dx+dy*dy<=14.66f*14.66f);
    Point heel=player_pose(point(-1,0,i==0?-1.65f:1.65f),i+1);
    CHECK(heel.y>=-.001f);
    /* Cross sections at knee retain their width, rather than scale with lift. */
    Point a=gait_leg(point(0,7.4f,-2),i),b=gait_leg(point(0,7.4f,2),i);
    CHECK(fabsf(a.z-b.z-(-4))<.001f);
    Point waist=gait_leg(point(0,14.8f,0),i);
    CHECK(fabsf(waist.z-poseSway)<.001f);
   }
  }
  /* There must be no teleport at recovery/contact or phase wrap. */
  float support=.62f-.14f*poseRun;
  for(int boundary=0;boundary<2;boundary++){
   float ph=boundary?2*PI:support*2*PI;
   s.gaitPhase=ph-.00001f;pose_prepare();Point before=gait_leg(point(2,0,-1.65f),0);
   s.gaitPhase=ph+.00001f;pose_prepare();Point after=gait_leg(point(2,0,-1.65f),0);
   CHECK(fabsf(before.x-after.x)<.002f&&fabsf(before.y-after.y)<.002f);
  }
 }
 s.motion=0;s.gaitPhase=0;s.time=0;pose_prepare();
 Point idle=gait_leg(point(.3f,7.4f,-2),0);CHECK(fabsf(idle.x-.3f)<.001f&&fabsf(idle.y-7.4f)<.001f);
 uint32_t first=street_tint(COLOR(200,200,200),192,320);
 CHECK(first==street_tint(COLOR(200,200,200),192,320));
 uint32_t near=street_tint(COLOR(200,200,200),192.001f,320);
 CHECK(abs((int)(first&255)-(int)(near&255))<=1);
  CHECK((street_tint(0x45808080u,190,310)>>24)==0x45);
 puts("PASS 720 walk/jog/run frames: grounded support, knee width, phase continuity, idle and continuous street colors.");
 return 0;
}
