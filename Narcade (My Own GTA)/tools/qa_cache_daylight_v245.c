/* Cached street colors change only when daylight changes, without a per-frame mesh pass. */
#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
int main(void){
 cacheUsed=0;RecHdr h={0,ROAD,3,1};Vertex v[3]={{0}};
 for(int i=0;i<3;i++)v[i].color=COLOR(80,100,120);
 cache_put(&h,sizeof h);cache_put(v,sizeof v);
 CellCache c={0};c.off=0;c.len=cacheUsed;
 c.groundLight[0]=80;c.groundLight[1]=100;c.groundLight[2]=120;
 cache_relight_ground(&c,COLOR(120,150,180));
 Vertex *cached=(Vertex*)(cachePool+sizeof h);
 for(int i=0;i<3;i++)assert((cached[i].color&0xffffff)==(COLOR(120,150,180)&0xffffff));
 uint32_t once=cached[0].color;
 for(int i=0;i<120;i++)cache_relight_ground(&c,COLOR(120,150,180));
 assert(cached[0].color==once);
 cache_relight_ground(&c,COLOR(80,100,120));
 assert((cached[0].color&0xffffff)==(COLOR(80,100,120)&0xffffff));
 puts("PASS: street cache relights once per daylight change; repeated frames preserve color.");
 return 0;
}
