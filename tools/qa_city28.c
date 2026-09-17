#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);exit(1);}}while(0)
int main(void){
 R3Scene s={0};s.x=1250;s.z=1030;s.cameraDistance=50;s.eyeHeight=34;s.target=-1;s.metroZ=1120;s.metroDir=1;
 view=&s;geographic=1;rigid=0;day_update(0);
 /* Separate elevated floors must not be culled using a ground-level sphere. */
 for(int floor=0;floor<12;floor++){
  s.lift=floor*24;s.yaw=0;camera(&s);memset(used,0,sizeof(used));fixedGround=geo_height(s.x+40,s.z);
  box(s.x+40,s.z,floor*24,20,26,24,0,BRICK,ROOF,0xffffffffu);CHECK(used[BRICK]>0);
  memset(used,0,sizeof(used));CmParcel p;cm_rect(&p,s.x+30,s.z-13,s.x+50,s.z+13,0);
  prism(&p,floor*24,24,BRICK,ROOF,0xffffffffu);CHECK(used[BRICK]>0);fixedGround=-1000000;
 }
 puts("PASS: every one of 12 elevated floors remains visible as box AND polygon prism.");
 s.lift=0;int maxV=0,maxGlow=0,maxShadow=0;
 for(int loc=0;loc<8;loc++)for(int hour=0;hour<8;hour++)for(int heading=0;heading<8;heading++){
  s.x=loc<4?1250:1468;s.z=loc<4?1030+loc*320:480+(loc-4)*320;s.yaw=heading*PI/4;s.time=(hour/8.f-.30f)*DAY_SECONDS;
  s.metroZ=s.z;s.metroDoors=hour%2;s.inMetro=loc==7;s.personCount=8;
  for(int i=0;i<8;i++)s.people[i]=(R3Person){s.x+20+i*3,s.z+25,0,i};
  r3_draw(NULL,&s);CHECK(!overflow);int total=0;for(int m=0;m<MAT_COUNT;m++)total+=used[m];if(total>maxV)maxV=total;
  CHECK(glowUsed<=GLOW_MAX&&shadowUsed<=GLOW_MAX/2);
  if(glowUsed>maxGlow)maxGlow=glowUsed;if(shadowUsed>maxShadow)maxShadow=shadowUsed;
  for(int group=0;group<2;group++){Vertex *v=group?shadowMesh:glowMesh;int n=group?shadowUsed:glowUsed;
   for(int i=0;i<n;i++){CHECK(isfinite(v[i].x)&&isfinite(v[i].y)&&isfinite(v[i].z));for(int k=0;k<6;k++)CHECK(plane_distance(&v[i],k)>-.015f);}
  }
 }
 printf("PASS: 512 city/metro/day-night scenes; no material overflow; clipped FX. Peaks: %d world, %d glow, %d shadow vertices.\n",maxV,maxGlow,maxShadow);
 for(int i=0;i<=1000;i++){day_update(i*DAY_SECONDS/1000.f);CHECK(night>=0&&night<=1);CHECK(isfinite(sunX)&&isfinite(sunY)&&isfinite(sunZ));}
 puts("PASS: continuous day-night cycle stays finite and bounded.");return 0;
}
