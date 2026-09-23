#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"road QA failed at line %d\n",__LINE__);exit(1);}}while(0)
int main(void){
 int accepted=0,rejected=0,split=0;
 for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
  float x=bx*320.f,z=bz*320.f;
  for(int k=96;k<304;k+=32){
   int h=road_mark_clear(x+k,z+41.4f,16,1.2f);
   int v=road_mark_clear(x+41.4f,z+k,1.2f,16);
   accepted+=h+v;rejected+=2-h-v;
  }
  unsigned f=cm_flags(bx,bz);
  if(f&CM_SPLIT_X)for(int k=94;k<288;k+=40)split+=road_mark_clear(x+191.4f,z+k,1.2f,19);
  if(f&CM_SPLIT_Z)for(int k=94;k<288;k+=40)split+=road_mark_clear(x+k,z+185.4f,19,1.2f);
 }
 CHECK(accepted>550&&rejected>0&&split>20);
 R3Scene scene={0};scene.cameraDistance=50;scene.eyeHeight=34;scene.time=300;
 int cells=0;
 for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
  scene.x=bx*320+42;scene.z=bz*320+42;scene.yaw=PI*.5f;
  r3_draw(NULL,&scene);CHECK(!r3_overflow());
  if(r3_used(FLAT)>0)cells++;
 }
 CHECK(cells==56);
 printf("PASS: %d valid base dashes, %d skipped at removed/intersecting roads, %d split-street dashes; all 56 cells render road paint without overflow.\n",accepted,rejected,split);
}
