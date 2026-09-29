#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
int main(void){
 R3Scene s={0};s.cameraDistance=125;s.eyeHeight=85;s.target=-1;s.time=100;
 for(int shot=0;shot<9;shot++){int bz=1+2*(shot/3);s.x=CM_METRO_X;s.z=cm_station_z(bz);s.metroZ=s.z;s.yaw=(shot%3)*PI*.65f;
  for(int k=0;k<8;k++)r3_prewarm(&s);r3_draw(NULL,&s);if(overflow){printf("FAIL: full station scene overflow %d (%d), FLAT %d, METAL %d\n",shot,overflow,used[FLAT],used[METAL]);return 1;}
  char path[80];snprintf(path,sizeof path,"build/metro-v243-%d.bin",shot);FILE *f=fopen(path,"wb");if(!f)return 2;
  fwrite(&eye,sizeof eye,1,f);fwrite(&target,sizeof target,1,f);for(int m=0;m<MAT_COUNT;m++){fwrite(&used[m],4,1,f);fwrite(mesh[m],sizeof(Vertex),used[m],f);}fclose(f);
 }
 puts("PASS: nine full-city station viewpoints fit the mesh budget.");return 0;
}
