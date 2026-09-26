#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
int main(void){
 R3Scene s={0};view=&s;geographic=0;rigid=0;clipEnabled=0;s.time=100;day_update(100);
 for(int i=0;i<9;i++){
  memset(used,0,sizeof(used));s.weapon=i<3?0:i==3?1:4;s.aiming=i>=3;s.motion=i<3?(i==0?1.f:i==1?1.7f:2.1f):0;s.gaitPhase=PI*.45f;
  s.aimPitch=i==5?.35f:0;s.recoil=0;s.jump=0;s.punch=0;
  float base=0,px=0,pz=0;
  if(i<6){geographic=0;rigid=0;person(0,0,0,-1,i<3);}else{
   s.x=1002;s.z=1060;geographic=1;rigid=1;objectX=s.x;objectZ=s.z;objectYaw=0;rigX=-1e9f;
   npcFall=i==6?0:i==7?.5f:1;npcFlee=i==6?10:0;npcPhase=.9f;
   person(s.x,s.z,0,5,i==6);npcFall=0;geo_project(s.x,s.z,&px,&pz);base=geo_height(s.x,s.z);
  }char name[120];snprintf(name,sizeof name,"build/character-combat-%d.bin",i);
  FILE *f=fopen(name,"wb");if(!f)return 1;
  float angle=i==2?3.14f:i==4?1.5f:.9f;
  eye=point(px+cosf(angle)*34,base+12,pz+sinf(angle)*34);target=point(px+2,base+8,pz);
  fwrite(&eye,sizeof eye,1,f);fwrite(&target,sizeof target,1,f);
  for(int m=0;m<MAT_COUNT;m++){fwrite(&used[m],4,1,f);fwrite(mesh[m],sizeof(Vertex),used[m],f);}fclose(f);
 }
 return 0;
}
