/* v2.29 (Claude): tirones SOLO con joystick. Pulgar realista en tres estilos:
   A) arcos lentos (gira la palanca adelante y atras) con ruido de +-3 cuentas;
   B) empuje suave (inclinacion que ronda el umbral, 0.15-0.40);
   C) cambios rapidos de direccion.
   Mide lo que se nota como tiron: arranques/paradas, aceleracion angular del
   rumbo (cambio del giro entre fotogramas) y cambios de velocidad en pantalla. */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static unsigned rs=2024;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
static void thumb(int style,int f,float *ax,float *ay){
 float ang,mag;
 if(style==0){ang=.9f*sinf(f*.025f);mag=.72f+.06f*sinf(f*.11f);}
 else if(style==1){ang=.3f*sinf(f*.02f);mag=.27f+.12f*sinf(f*.07f)+.04f*sinf(f*.31f);}
 else{int seg=(f/40)%4;float t[4]={0,1.2f,-.9f,.4f};ang=t[seg];mag=.85f;}
 float q=127.f;
 *ax=roundf(sinf(ang)*mag*q+(rnd()-.5f)*6)/q;*ay=roundf(-cosf(ang)*mag*q+(rnd()-.5f)*6)/q;
}
int main(void){
 const char *names[]={"A arcos lentos","B empuje suave","C cambios rapidos"};
 double totT=0,totA=0,totV=0;
 for(int style=0;style<3;style++){
  long toggles=0,n=0;double angAcc=0,spd=0;
  for(int run=0;run<20;run++){
   game_init();fresh_game();g.screen=WORLD;
   for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
   g.x=1002;g.y=1180;g.lift=0;g.viewYaw=proj_heading(g.x,g.y,PI*.5f)+run*.3f;
   float lpx,lpz,lh=0,lw=0,lv=0;int la=0;physics_project(g.x,g.y,&lpx,&lpz);
   for(int f=0;f<360;f++){
    float ax,ay;thumb(style,f+run*37,&ax,&ay);
    game_tick(run%2?B_CROSS:0,ax,ay,1.f/60);
    float px,pz;physics_project(g.x,g.y,&px,&pz);float v=hypotf(px-lpx,pz-lpz);
    float h=atan2f(pz-lpz,px-lpx),w=angle_delta(h,lh);
    if(f>30){if(g.moveActive!=la)toggles++;
     if(v>.1f&&lv>.1f){angAcc+=fabsf(w-lw);spd+=fabsf(v-lv);n++;}}
    la=g.moveActive;lh=h;lw=w;lv=v;lpx=px;lpz=pz;
    if(g.x<60||g.x>2500||g.y<60||g.y>2180){g.x=1002;g.y=1180;physics_project(g.x,g.y,&lpx,&lpz);lv=0;}
   }
  }
  printf("%-18s | arranques/paradas %4ld | tiron de giro %.4f rad/fot | tiron de velocidad %.4f\n",names[style],toggles,angAcc/n,spd/n);
  totT+=toggles;totA+=angAcc/n;totV+=spd/n;
 }
 printf("TOTAL arranques/paradas %.0f | giro %.4f | velocidad %.4f\n",totT,totA,totV);
 return 0;
}
