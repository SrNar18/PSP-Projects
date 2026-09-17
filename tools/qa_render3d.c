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
 const char *names[]={"asphalt","sidewalk","brick","stucco","shop","roof","grass","water","jacket","jeans","face","wheel","car-side","car-paint","glass","mural","jacket-back","sleeve","skin","hair","flat"};
 R3Scene s={0};s.cameraDistance=65;view=&s;memset(used,0,sizeof(used));clipEnabled=0;person(0,0,0,-1,0);clipEnabled=1;
 fputs("# Narcade 2.6 adapted personaje-v3, Y up, +X forward\nmtllib nico.mtl\n",obj);
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
static void export_animation(void){
 R3Scene s={0};s.cameraDistance=65;view=&s;clipEnabled=0;geographic=0;rigid=0;
 FILE *f=fopen("build/player-animation.bin","wb");assert(f);unsigned frames=48,vertices=0;
 person(0,0,0,-1,0);memset(used,0,sizeof(used));person(0,0,0,-1,0);
 for(int m=0;m<MAT_COUNT;m++)vertices+=used[m];
 fwrite(&frames,4,1,f);fwrite(&vertices,4,1,f);
 for(unsigned frame=0;frame<frames;frame++){
  s.motion=frame<16?1:frame<32?111.f/72:150.f/72;s.gaitPhase=(frame%16)*2*PI/16;s.time=s.gaitPhase/8.5f;
  pose_prepare();
  Point left=player_pose(point(0,0,-1.65f),1),right=player_pose(point(0,0,1.65f),2);
  assert(left.y>=-.001f&&right.y>=-.001f);
  assert(fabsf(left.x-right.x)+fabsf(left.y-right.y)>.1f);
  if(frame%16==0)assert(left.x>0&&right.x<0);
  memset(used,0,sizeof(used));overflow=0;person(0,0,0,-1,1);assert(!overflow);
  unsigned count=0;
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i<used[m];i++){
   float xyz[]={mesh[m][i].x,mesh[m][i].y,mesh[m][i].z};assert(isfinite(xyz[0])&&isfinite(xyz[1])&&isfinite(xyz[2]));fwrite(xyz,4,3,f);count++;
  }
  assert(count==vertices);
 }
 fclose(f);clipEnabled=1;puts("PASS: 48 walk/jog/sprint poses, separate alternating feet, no foot below ground, stable topology.");
}
static int road_covers(float x,float z){
 for(int i=0;i<used[ROAD];i+=3){
  Vertex *a=&mesh[ROAD][i],*b=a+1,*c=a+2;
  float d=(b->z-c->z)*(a->x-c->x)+(c->x-b->x)*(a->z-c->z);
  if(fabsf(d)<.0001f)continue;
  float u=((b->z-c->z)*(x-c->x)+(c->x-b->x)*(z-c->z))/d;
  float v=((c->z-a->z)*(x-c->x)+(a->x-c->x)*(z-c->z))/d;
  if(u>=-.001f&&v>=-.001f&&u+v<=1.001f)return 1;
 }
 return 0;
}
int main(void){
 int legVertices[2]={0,0};float legZ[2]={0,0};
 for(int i=0;i<PLAYER_VERTEX_COUNT;i++){
  const PlayerVertex *v=&player_mesh[i];
  if(v->mat==JEANS&&v->bone>=1&&v->bone<=2){legVertices[v->bone-1]++;legZ[v->bone-1]+=v->z;}
 }
 assert(legVertices[0]>100&&legVertices[1]>100);
 assert(legZ[0]<0&&legZ[1]>0);
 puts("PASS: left AND right trouser meshes have independent limb assignments on opposite body sides.");
 rigid=2;
 for(int z=86;z<2240;z+=29)for(int angle=0;angle<8;angle++){
  objectX=382;objectZ=(float)z;objectYaw=angle*PI/4;rigid_cache();
  Point basis[]={rigBX,rigBY,rigBZ};
  for(int a=0;a<3;a++)for(int b=0;b<3;b++){
   float dot=basis[a].x*basis[b].x+basis[a].y*basis[b].y+basis[a].z*basis[b].z;
   assert(fabsf(dot-(a==b?1.f:0.f))<.00001f);
  }
  assert(rigBY.y>.95f);
 }
 rigid=0;puts("PASS: car basis preserves length, width and height on every ramp and heading (no shear/flattening).");
 R3Scene s={0};s.cameraDistance=65;s.target=-1;
 /* Workshop foreground disappeared with the previous block-sphere cull.
    Test actual triangle coverage, not merely a nonempty road batch. */
 for(int z=960;z<=1022;z+=2)for(int a=0;a<16;a++){
  s.x=62;s.z=(float)z;s.yaw=a*PI/8;
  r3_draw(NULL,&s);float xw,zw;geo_project(s.x,s.z,&xw,&zw);
  assert(road_covers(xw,zw));
 }
 puts("PASS: workshop foreground road covers the player at 512 positions/headings.");
 int scenes=0,maxVertices=0;
 for(int location=0;location<56;location++)for(int yaw=0;yaw<16;yaw++){
  s.x=62+(location%8)*320;s.z=62+(location/8)*320;s.yaw=yaw*PI/8;
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
 geographic=1;ground(ROAD,0,960,320,320,0,0xffffffffu,12);geographic=0;assert(used[ROAD]>0);
 for(int i=0;i<used[ROAD];i++)assert(plane_distance(&mesh[ROAD][i],0)>-.001f);
 printf("PASS: %d camera/location scenes, no material overflow, all vertices inside 6 clip planes, ground crossing near plane retained. Peak %d vertices.\n",scenes,maxVertices);
 s.x=62;s.z=62;s.yaw=0;s.personCount=42;
 for(int i=0;i<42;i++)s.people[i]=(R3Person){82+(i%7)*12,20+(i/7)*14,0,i%6};
 r3_draw(NULL,&s);assert(!overflow);assert(used[FACE]>0);
 puts("PASS: dense crowd, protagonist geometry retained, no material overflow.");
 export_player();export_animation();return 0;
}
