/* Adjacent metro blocks must meet at the same projected rail/deck vertices. */
#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
static int contains(int mat,Point p){
 for(int i=0;i<used[mat];i++){
  Vertex *v=&mesh[mat][i];
  if(fabsf(v->x-p.x)<.001f&&fabsf(v->y-p.y)<.001f&&fabsf(v->z-p.z)<.001f)return 1;
 }
 return 0;
}
int main(void){
 R3Scene s={0};view=&s;s.time=100;s.x=CM_METRO_X;s.z=1000;
 geographic=1;clipEnabled=0;day_update(100);
 for(int row=0;row<6;row++){
  int present[2][6]={{0}};float z=(row+1)*320.f;
  for(int next=0;next<2;next++){
   memset(used,0,sizeof used);fixedGround=-1000000;metro(row+next,0);
   for(int side=-1;side<=1;side+=2){int k=side>0?1:0;
    present[next][k]=contains(SIDEWALK,metro_point(CM_METRO_X+side*7,z,CM_METRO_DECK+3));
    present[next][2+k]=contains(FLAT,metro_point(CM_METRO_X+side*6-.325f,z,CM_METRO_DECK+3.61f));
    present[next][4+k]=contains(FLAT,metro_point(CM_METRO_X+side*6+.325f,z,CM_METRO_DECK+3.61f));
   }
  }
  for(int k=0;k<6;k++)if(!present[0][k]||!present[1][k]){
   printf("FAIL: metro seam row %d vertex %d (%d/%d)\n",row,k,present[0][k],present[1][k]);return 1;
  }
 }
 puts("PASS: deck and both rails meet exactly across all six block boundaries.");
 return 0;
}
