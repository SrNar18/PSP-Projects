#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"city QA line %d\n",__LINE__);exit(1);}}while(0)
int main(void){
 R3Scene s={0};s.cameraDistance=55;s.eyeHeight=38;s.target=-1;s.metroZ=1120;s.time=.45f*DAY_SECONDS;
 float scenes[3][3]={{1020,800,0},{62,180,0},{1400,860,PI*.5f}};
 int picked[2]={0,0};
 for(int row=0;row<7;row++)for(int col=0;col<8;col++){
  int type=city_block_type(col,row),which=type==CB_CLASSIC?0:type==CB_COMUNA?1:-1;
  if(which>=0&&!picked[which]&&cm_flags(col,row)==0){scenes[which][0]=col*320+62;scenes[which][1]=row*320+180;picked[which]=1;}
 }
 for(int i=0;i<3;i++){
  s.x=scenes[i][0];s.z=scenes[i][1];s.yaw=scenes[i][2];
  s.cameraDistance=i==2?180:85;s.eyeHeight=i==2?145:62;r3_draw(NULL,&s);CHECK(!overflow);
  char path[80];snprintf(path,sizeof path,"build/city-v232-%d.bin",i);FILE *f=fopen(path,"wb");CHECK(f);
  fwrite(&eye,sizeof eye,1,f);fwrite(&target,sizeof target,1,f);
  for(int m=0;m<MAT_COUNT;m++){fwrite(&used[m],4,1,f);fwrite(mesh[m],sizeof(Vertex),used[m],f);}fclose(f);
 }
 for(int z=0;z<=2240;z++)CHECK(river_centre(z)-24>=1396&&river_centre(z)+24<=1460);
 for(int row=0;row<7;row++){
  s.x=1428;s.z=row*320+160;view=&s;camera(&s);memset(used,0,sizeof used);
  clipEnabled=0;geographic=1;fixedGround=-1000000;river_channel(row,0xffffffffu);CHECK(!overflow);
  CHECK(used[WATER]==120); /* twenty connected quads, including beneath bridges */
  clipEnabled=1;
 }
 puts("PASS: 3 actual-mesh previews, continuous river inside collision corridor; movement source unchanged.");
}
