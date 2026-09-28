#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
int main(void){
 R3Scene s={0};view=&s;geographic=1;clipEnabled=0;s.x=CM_METRO_X;s.z=480;s.time=100;day_update(100);
 memset(used,0,sizeof used);geographic=0;rail_canopy(0,0,8,30,54,2);assert(used[METAL]==24);geographic=1;
 for(int bz=0;bz<7;bz++)for(int far=0;far<2;far++){
  memset(used,0,sizeof used);overflow=0;fixedGround=-1000000;s.z=bz*320+160;metro(bz,far);assert(!overflow);
  assert(used[METAL]>=12); /* rails survive far LOD, not just station roofs */
  int pavementOverTrack=0;
  for(int i=0;i<used[SIDEWALK];i+=3){int over=1;
   for(int j=0;j<3;j++){Vertex *v=&mesh[SIDEWALK][i+j];float x,z;geo_unproject(v->x,v->z,&x,&z);
    if(!isfinite(v->y)||fabsf(x-CM_METRO_X)>10||v->y<geo_height(x,z)+CM_PLAT_H-.05f)over=0;}
   pavementOverTrack+=over;
  }
  assert(!pavementOverTrack);
  if(cm_metro_station(bz))for(int i=0;i<used[METAL];i+=3){
   float x=0,z=0,y=0;for(int j=0;j<3;j++){Vertex *v=&mesh[METAL][i+j];x+=v->x/3;z+=v->z/3;y+=v->y/3;}
   float wx,wz;geo_unproject(x,z,&wx,&wz);float relative=y-geo_height(wx,wz);
   if(fabsf(wx-CM_METRO_X)<10&&relative>CM_PLAT_H+20){
    /* Centre HVAC reaches 54.9; outer curved roof reaches at most 53.2. */
    float required=CM_PLAT_H+(fabsf(wx-CM_METRO_X)<=6.5f?24.7f:23.0f)+.05f;
    if(relative<required){printf("clearance row%d far%d x%.3f z%.3f rel%.3f need%.3f\n",bz,far,wx,wz,relative,required);return 3;}
   }
  }
 }
 for(int bz=1;bz<=5;bz+=2){
  float z=cm_station_z(bz);assert(cm_on_platform(1470.5f,z));assert(cm_on_platform(1500,z));assert(!cm_on_platform(CM_METRO_X,z));
  assert(cm_lift(1470.5f,z,1)==CM_PLAT_H);assert(cm_lift(CM_METRO_X,z,1)==0);
  float last=-1;for(int k=0;k<=120;k++){float zz=bz*320+318-k*.5f,lift=cm_lift(1470.5f,zz,k>0&&last>15);assert(last<0||(lift>=last&&lift-last<.3f));last=lift;}
  assert(fabsf(last-CM_PLAT_H)<.01f);
 }
 puts("PASS: all seven track rows, both LODs, three split platforms, clear track bed and continuous stairs.");return 0;
}
