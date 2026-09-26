#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL: %s line %d\n",#x,__LINE__);exit(1);}}while(0)
int main(void){
 const char *names[]={"asphalt","sidewalk","brick","stucco","shop","roof","grass","water","jacket","jeans","face","wheel","car-side","car-paint","glass","mural","jacket-back","sleeve","skin","hair","flat","npc-tee","npc-knit","npc-denim","npc-floral","npc-sport","npc-plaid","npc-face-woman","npc-face-man"};
 R3Scene scene={0};view=&scene;clipEnabled=0;geographic=0;rigid=0;day_update(0);
 for(int style=0;style<8;style++){
  memset(used,0,sizeof(used));overflow=0;simple_person(0,0,0,style,0);CHECK(!overflow);
  CHECK(used[style<4?27:28]>0);CHECK(used[21+style%6]>0);
  char path[80];snprintf(path,sizeof(path),"build/npc-%d.obj",style);FILE *f=fopen(path,"w");CHECK(f);
  int base=1,total=0;
  for(int m=0;m<MAT_COUNT;m++)if(used[m]){
   fprintf(f,"usemtl %s\n",names[m]);
   for(int i=0;i<used[m];i++){Vertex *p=&mesh[m][i];CHECK(isfinite(p->x)&&isfinite(p->y)&&isfinite(p->z));
    fprintf(f,"v %f %f %f %f %f %f\nvt %f %f\n",p->x,p->y,p->z,(p->color&255)/255.f,((p->color>>8)&255)/255.f,((p->color>>16)&255)/255.f,p->u,1-p->v);
   }
   for(int i=0;i<used[m];i+=3)fprintf(f,"f %d/%d %d/%d %d/%d\n",base+i,base+i,base+i+1,base+i+1,base+i+2,base+i+2);
   base+=used[m];total+=used[m]/3;
  }
  fclose(f);printf("PASS: NPC %d (%s), %d triangles, distinct cloth and face materials.\n",style,style<4?"woman":"man",total);
 }
 for(int style=0;style<42;style++)for(int phase=0;phase<16;phase++){
  memset(used,0,sizeof(used));overflow=0;scene.time=phase*.1f;simple_person(0,0,0,style,1);CHECK(!overflow);
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i<used[m];i++)CHECK(isfinite(mesh[m][i].x)&&isfinite(mesh[m][i].y)&&isfinite(mesh[m][i].z));
 }
 puts("PASS: all 42 pedestrian styles across 16 walking poses; finite vertices, no overflow.");
 return 0;
}
