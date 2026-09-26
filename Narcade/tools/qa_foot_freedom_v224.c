#include <stdio.h>
#include <stdlib.h>
#include "../src/game.c"
static unsigned rs=12345;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static void clear_traffic(void){for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}}
int main(int argc,char**argv){
 int keepCars=argc>1;
 long slidable=0,frames=0,pushed=0,stall=0,misdir=0;long cause[8]={0};
 double errSum=0;
 for(int run=0;run<400;run++){
  game_init();fresh_game();g.screen=WORLD;if(!keepCars)clear_traffic();
  do{g.x=40+rnd()*(WORLD_W-80);g.y=40+rnd()*(WORLD_H-80);}while(!foot_free(g.x,g.y)||cm_lift(g.x,g.y,0)>1);
  g.lift=0;g.viewYaw=rnd()*6.28f;
  float stick=0,target=0;int keys=run%3==0?0:B_CROSS;
  for(int f=0;f<600;f++){
   if(f%40==0)target=(rnd()*2-1)*PI;
   float d=angle_delta(target,stick);stick+=clampf(d,-.25f,.25f); /* human rotates stick ~15 rad/s */
   float ax=sinf(stick),ay=-cosf(stick);
   float x0=g.x,y0=g.y,px0,py0;physics_project(x0,y0,&px0,&py0);
   float want=g.viewYaw+stick;
   game_tick(keys,ax,ay,1.f/60);
   if(g.screen!=WORLD){g.screen=WORLD;continue;}
   float px1,py1;physics_project(g.x,g.y,&px1,&py1);
   float mv=hypotf(px1-px0,py1-py0);frames++;
   if(f<10)continue;pushed++;
   float expect=g.footSpeed/60.f;
   if(mv<expect*.25f){stall++;
     /* classify: what blocks a small step in the wanted rendered direction? */
     float tx,ty;physics_unproject(px0+cosf(want)*3,py0+sinf(want)*3,&tx,&ty);
     int c=7;
     if(tx<8||ty<8||tx>WORLD_W-8||ty>WORLD_H-8)c=0;
     else if(!free_at(tx,ty,5)){ int s=0;for(int k=-1;k<8;k++){float a=k*(PI*.25f),r=k<0?0:5;float qx=tx+cosf(a)*r,qy=ty+sinf(a)*r;if(cm_solid(qx,qy))s|=1;else if(cm_obstacle(qx,qy))s|=2;else if(solid(qx,qy))s|=4;}c=s&1?1:s&2?2:3;}
     else if(!foot_free(tx,ty))c=4;
     else if(!lift_ok(tx,ty))c=5;
     else c=6; /* direction is open but player did not move */
     cause[c]++;
     { float wl=atan2f(ty-y0,tx-x0);int ok=0;for(int k=1;k<=5&&!ok;k++)for(int sd=-1;sd<=1;sd+=2){float a=wl+sd*k*PI/12;float qx=x0+cosf(a)*2,qy=y0+sinf(a)*2;if(foot_free(qx,qy)&&lift_ok(qx,qy)){ok=1;break;}} if(ok){slidable++; if(slidable<=12)printf("  atasco deslizable x%.0f y%.0f want%.2f causa%d fs%.0f\n",x0,y0,want,c,g.footSpeed);} }
   } else {
     float got=atan2f(py1-py0,px1-px0);float e=fabsf(angle_delta(got,want));errSum+=e;if(e>.6f)misdir++;
   }
  }
 }
 printf("stalls where a slide within 75deg was open: %ld\n",slidable);
 printf("frames %ld  stalls %ld (%.1f%%)  screen-misdirected>34deg %ld (%.1f%%) meanErr %.2f rad\n",pushed,stall,100.*stall/pushed,misdir,100.*misdir/pushed,errSum/(pushed-stall));
 const char*nm[]={"borde mundo","edificio (parcela)","obstaculo menor","puente/pasarela","coche/zona location","escalera/anden","DIRECCION LIBRE pero no avanzo","?"};
 for(int i=0;i<8;i++)if(cause[i])printf("  %-32s %ld\n",nm[i],cause[i]);
}
