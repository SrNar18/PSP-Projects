/* Build and inspect the exact CPU mesh stream submitted to PSP GE. */
#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#undef assert
#define assert(x) do { if(!(x)){fprintf(stderr,"FAIL: %s at %s:%d\n",#x,__FILE__,__LINE__);exit(1);} } while(0)
#endif

static void export_player(void){
 FILE *obj=fopen("assets/nico.obj","w"),*mtl=fopen("assets/nico.mtl","w");assert(obj&&mtl);
 const char *names[]={"asphalt","sidewalk","brick","stucco","shop","roof","grass","water","jacket","jeans","face","wheel","car-side","car-paint","glass","mural","jacket-back","sleeve","skin","hair"};
 R3Scene s={0};s.cameraDistance=65;view=&s;memset(used,0,sizeof(used));clipEnabled=0;person(0,0,0,-1,0);clipEnabled=1;
 fputs("# Narcade 2.1 original articulated character, Y up, +X forward\nmtllib nico.mtl\n",obj);
 int base=1,total=0;
 for(int m=0;m<MAT_COUNT;m++)if(used[m]){
  fprintf(mtl,"newmtl %s\nKd 1 1 1\nmap_Kd textures3d/%s.png\n\n",names[m],names[m]);
  fprintf(obj,"g %s\nusemtl %s\n",names[m],names[m]);
  for(int i=0;i<used[m];i++){Vertex *p=&mesh[m][i];fprintf(obj,"v %.5f %.5f %.5f %.5f %.5f %.5f\nvt %.5f %.5f\n",p->x,p->y,p->z,(p->color&255)/255.0f,((p->color>>8)&255)/255.0f,((p->color>>16)&255)/255.0f,p->u,1-p->v);}
  for(int i=0;i<used[m];i+=3)fprintf(obj,"f %d/%d %d/%d %d/%d\n",base+i,base+i,base+i+1,base+i+1,base+i+2,base+i+2);
  base+=used[m];total+=used[m]/3;
 }
 fclose(obj);fclose(mtl);printf("Nico export: %d triangles, OBJ+MTL, original UV materials.\n",total);
}
int main(void){
 R3Scene s={0};s.cameraDistance=65;s.target=-1;
 int scenes=0,maxVertices=0;
 for(int location=0;location<12;location++)for(int yaw=0;yaw<16;yaw++){
  s.x=62+(location%4)*320;s.z=62+(location/4)*320;s.yaw=yaw*PI/8;
  s.carCount=1;s.cars[0]=(R3Car){s.x+25,s.z,0,0,0,0};
  r3_draw(NULL,&s);assert(!overflow);assert(used[ROAD]>0);
  int total=0;
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i<used[m];i++){
   Vertex *v=&mesh[m][i];assert(isfinite(v->x)&&isfinite(v->y)&&isfinite(v->z)&&isfinite(v->u)&&isfinite(v->v));
   for(int k=0;k<6;k++)assert(plane_distance(v,k)>-.002f);
   total++;
  }
  if(total>maxVertices)maxVertices=total;
  scenes++;
 }
 /* Huge ground quad crossing the camera must be clipped, never discarded. */
 s.x=62;s.z=1022;s.yaw=-PI*.5f;camera(&s);memset(used,0,sizeof(used));
 ground(ROAD,0,960,320,320,0,0xffffffffu,12);assert(used[ROAD]>0);
 for(int i=0;i<used[ROAD];i++)assert(plane_distance(&mesh[ROAD][i],0)>-.001f);
 printf("PASS: %d camera/location scenes, no material overflow, all vertices inside 6 clip planes, ground crossing near plane retained. Peak %d vertices.\n",scenes,maxVertices);
 s.x=62;s.z=62;s.yaw=0;s.personCount=42;
 for(int i=0;i<42;i++)s.people[i]=(R3Person){82+(i%7)*12,20+(i/7)*14,0,i%6};
 r3_draw(NULL,&s);assert(!overflow);assert(used[FACE]>0);
 puts("PASS: dense crowd, protagonist geometry retained, no material overflow.");
 export_player();return 0;
}
