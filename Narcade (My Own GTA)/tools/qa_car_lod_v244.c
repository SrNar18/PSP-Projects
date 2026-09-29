#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
int main(void){
 R3Scene s={0};view=&s;clipEnabled=0;day_update(100);
 float height[6];
 for(int type=0;type<6;type++){
  memset(used,0,sizeof used);R3Car c={0};c.x=240;c.type=type;car(&c);
  if(used[GLASS]<6||used[CAR_PAINT]<6||overflow)return 1;
  height[type]=0;for(int i=0;i<used[GLASS];i++)if(mesh[GLASS][i].y>height[type])height[type]=mesh[GLASS][i].y;
 }
 if(!(height[3]>height[4]+5&&height[5]>height[0]))return 2;
 puts("PASS: six distant car types retain painted bodies and distinct window/roof silhouettes.");
 return 0;
}
