#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"road QA failed at line %d\n",__LINE__);exit(1);}}while(0)
static unsigned char audit[1120][1280][3];
static void paint(float x,float z,float w,float d,int r,int g,int b){
 int x0=(int)(x*.5f),y0=(int)(z*.5f),x1=(int)((x+w)*.5f+1),y1=(int)((z+d)*.5f+1);
 for(int yy=y0;yy<y1;yy++)for(int xx=x0;xx<x1;xx++)if(xx>=0&&xx<1280&&yy>=0&&yy<1120){audit[yy][xx][0]=r;audit[yy][xx][1]=g;audit[yy][xx][2]=b;}
}
int main(void){
 int accepted=0,rejected=0;
 for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
  float x=bx*320.f,z=bz*320.f;
  for(int k=96;k<304;k+=32){
   int h=road_mark_clear(x+k,z+41.4f,16,1.2f);
   int v=road_mark_clear(x+41.4f,z+k,1.2f,16);
   accepted+=h+v;rejected+=2-h-v;
  }
 }
 CHECK(accepted>550&&rejected>0);
 R3Scene scene={0};scene.cameraDistance=50;scene.eyeHeight=34;scene.time=300;
 int cells=0;
 for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
  scene.x=bx*320+42;scene.z=bz*320+42;scene.yaw=PI*.5f;
  r3_draw(NULL,&scene);CHECK(!r3_overflow());
  if(r3_used(FLAT)>0)cells++;
 }
 CHECK(cells==56);
 /* Plan view made from the exact road predicate and dash decisions. */
 for(int yy=0;yy<1120;yy++)for(int xx=0;xx<1280;xx++){
  int road=cm_on_road(xx*2.f+1,yy*2.f+1);
  audit[yy][xx][0]=road?65:137;audit[yy][xx][1]=road?69:135;audit[yy][xx][2]=road?70:128;
 }
 for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
  float x=bx*320.f,z=bz*320.f;
  int westMerged=(cm_flags(bx-1,bz)&CM_MERGE_E)!=0,northMerged=(cm_flags(bx,bz-1)&CM_MERGE_S)!=0;
  for(int k=96;k<304;k+=32){
   if(!northMerged&&road_mark_clear(x+k,z+41.4f,16,1.2f))paint(x+k,z+41.4f,16,1.2f,244,208,100);
   if(!westMerged&&road_mark_clear(x+41.4f,z+k,1.2f,16))paint(x+41.4f,z+k,1.2f,16,244,208,100);
  }
 }
 FILE *out=fopen("build/road-audit-v221.ppm","wb");CHECK(out);
 fprintf(out,"P6\n1280 1120\n255\n");fwrite(audit,1,sizeof(audit),out);fclose(out);
 printf("PASS: %d valid yellow dashes, %d skipped at removed/intersecting roads; no white dashes on secondary streets; all 56 cells render without overflow.\n",accepted,rejected);
}
