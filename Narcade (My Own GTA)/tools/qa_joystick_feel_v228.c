/* v2.28 (Claude): tirones con joystick vs cruceta. Misma ruta (recto, giro a la
   derecha mantenido, diagonal) con cruceta y con analogico realista (pulgar que
   no empuja a tope y varia la presion + ruido de +-3 cuentas). Mide, por
   fotograma, lo que se ve: variacion de velocidad en pantalla, de rumbo del
   cuerpo y de giro de camara. Cuanto mas cerca del valor de la cruceta, mejor. */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static unsigned rs=31337;static float rnd(void){rs=rs*1103515245u+12345u;return (rs>>8)/16777216.f;}
typedef struct{double speedJ,bodyJ,camJ;long n;float dist;}M;
static M run(int analog,int keys){
 M m={0};
 game_init();fresh_game();g.screen=WORLD;
 for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
 g.x=1002;g.y=1180;g.lift=0;g.viewYaw=proj_heading(g.x,g.y,PI*.5f);
 float lpx,lpz,lv=-1,lb=0,lc=0,lcv=0;physics_project(g.x,g.y,&lpx,&lpz);
 for(int f=0;f<420;f++){
  float angle=f<120?0:f<240?1.5708f:.7854f; /* recto, derecha mantenida, diagonal */
  if(analog){float thumb=.62f+.18f*sinf(f*.05f)+.05f*sinf(f*.37f),q=127.f;
   float ax=roundf(sinf(angle)*thumb*q+(rnd()-.5f)*6)/q,ay=roundf(-cosf(angle)*thumb*q+(rnd()-.5f)*6)/q;
   game_tick(keys,ax,ay,1.f/60);}
  else{unsigned b=keys;if(sinf(angle)>.38f)b|=B_RIGHT;if(cosf(angle)>.38f)b|=B_UP;game_tick(b,0,0,1.f/60);}
  float px,pz;physics_project(g.x,g.y,&px,&pz);float v=hypotf(px-lpx,pz-lpz);
  float b=proj_heading(g.x,g.y,g.a),c=g.viewYaw,cv=angle_delta(c,lc);
  if(f>20&&f!=120&&f!=121&&f!=240&&f!=241){m.speedJ+=fabsf(v-lv);m.bodyJ+=fabsf(angle_delta(b,lb));m.camJ+=fabsf(cv-lcv);m.n++;}
  m.dist+=v;lv=v;lb=b;lc=c;lcv=cv;lpx=px;lpz=pz;
 }
 return m;
}
int main(void){
 const char *names[]={"caminar","trotar (X)"};int keys[]={0,B_CROSS};
 for(int k=0;k<2;k++){
  M d=run(0,keys[k]),a=run(1,keys[k]);
  printf("%-10s | cruceta: tiron velocidad %.4f cuerpo %.4f camara %.5f dist %.0f | joystick: velocidad %.4f cuerpo %.4f camara %.5f dist %.0f\n",names[k],
   d.speedJ/d.n,d.bodyJ/d.n,d.camJ/d.n,d.dist,a.speedJ/a.n,a.bodyJ/a.n,a.camJ/a.n,a.dist);
 }
 return 0;
}
