/* Narcade - original PSP homebrew. All game logic also builds on desktop for QA. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#ifdef NARCADE_3D
#include "render3d.h"
#endif

#define W 480
#define H 272
#include "world_geo.h"
#include "citymap.h" /* v2.6 (Claude): trazado irregular compartido con render3d */
#define WORLD_W 2560
#define WORLD_H 2240
#define CAR_COUNT 64
#define PI 3.14159265358979323846f
#define RGB(r,g,b) (0xff000000u|((uint32_t)(b)<<16)|((uint32_t)(g)<<8)|(r))
#define INK RGB(12,23,31)
#define PANEL RGB(21,38,46)
#define WHITE RGB(234,240,222)
#define MUTED RGB(139,166,164)
#define LIME RGB(215,240,109)
#define TEAL RGB(66,207,193)
#define CORAL RGB(245,121,99)
#define GOLD RGB(251,197,94)
typedef struct {const char *name; int x,y;} Location;
enum {K_TALK,K_DRIVE,K_CODE,K_CIRCUIT,K_MEMORY,K_TUNE,K_RHYTHM,K_LOCK,K_STEALTH,K_RACE,K_CHASE,K_CHOICE,K_ENDING};
typedef struct {int kind,loc,seed,par; const char *text;} Step;
typedef struct {const char *title,*who,*intro,*outro;int count,reward;Step steps[6];} Mission;
#include "story.h"
#include "assets.h"
#include "title_ui.h"
#include "ui_font.h"
enum {TITLE,WORLD,DIALOG,MINI,MAP,PAUSE,JOURNAL,CHOICE,SETTINGS,CREDITS,TROPHIES};
typedef struct {float x,y,a,speed,hp;int type,parked,police;} Car;
typedef struct {float x,y,v,phase;int vertical;} Ped;
typedef struct {
 uint32_t magic,version; int mission,step,cash,reputation,ending;
 float x,y,health,playtime; uint32_t caches; int jobs,station;uint32_t check;
} Save;
typedef struct {
 int kind,seed,cursor,digits[4],answer[4],tiles[16],connected[16],seq[12],input,phase,score,hits,miss;
 float t,value,target,hold,x,y;int lane,notes[24],hit[24];
} Puzzle;
static struct {
 int screen,back,mission,step,cash,reputation,ending,car,station,menu,titleStage,seenIntro,dialogAction,mapSel,journalPage;
 int jobs,side,checkpoint,route[6],saveOK,active; uint32_t caches,prev,pressed,held;
 float x,y,a,health,heat,escape,clock,playtime,timer,noticeT,hitCD,cameraX,cameraY,screenT,raceTime,missionTimer;
 float viewYaw,walking,inputYaw,gaitPhase,motion,stickAngle,cameraVelocity,cameraDistance,followScale,frameYaw,bodyYaw,wallGuide,carCamYaw,stickAnchor,carScreenYaw,stickFX,stickFY,stickHX[2],stickHY[2],camBase,camClear,calX,calY;int carCamInit,carYawOwner;int stickActive;
 float tapAge,sprintTime,footSpeed,footTravel,footFiltered,footStall,moveGap;int runTaps;
 float stamina;int exhausted;
 float trafficYield[CAR_COUNT],hornCooldown[CAR_COUNT],blockedTime[CAR_COUNT],wallTime[CAR_COUNT];
 int pauseTab,pauseBack;
 int weapon,weaponChoice,weaponWheel;float weaponHold;
 float lift,metroZ,metroWait;int metroDir,inMetro;float moveYaw,steerSmooth;int moveActive; /* v2.9: rumbo suavizado a pie */int zoom; /* v2.6.2: SELECT alterna 4 distancias de camara (1 = por defecto) */ /* v2.6: anden/escaleras y Metro (no se guardan) */
 int hudDistrict,hudStepKey; float hudDistrictT,hudObjectiveT;
 char notice[160],dialog[640],speaker[60],savepath[256];
 Car cars[CAR_COUNT]; Ped peds[42]; Puzzle p;
} g;
#include "localization.inc"
static int prefsBrightness=5,prefsFog,prefsDirty;
static volatile int prefsMusic=10,prefsEffects=10;
static void settings_load(void),settings_finish(void),settings_tick(void),settings_draw(void);
static GameTrophyProvider trophyProvider;
void game_set_trophy_provider(GameTrophyProvider provider){trophyProvider=provider;}
static int trophySelection;
static uint32_t *fb;static int pitch;
#ifdef NARCADE_3D
static uint32_t *renderTarget;
#endif
static volatile int audioStation=0,audioEnabled=0,audioAmbient=0;
static volatile unsigned hornEvent=0;
static volatile unsigned menuEvent=0;
static volatile int hornGain=0,hornPan=0,trafficGain=0,crowdGain=0;
extern const short city_bed[],city_engine[],city_horn[];
#define CITY_BED_SAMPLES 352800
#define CITY_ENGINE_SAMPLES 22050
#define CITY_HORN_SAMPLES 12127
static unsigned audioPos=0;static int audioLast=-1;
static unsigned pendingCross=0;
void game_latch_cross(unsigned buttons){pendingCross|=buttons&B_CROSS;}
static uint32_t rng=137;
static uint32_t random_u(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float clampf(float a,float lo,float hi){return a<lo?lo:(a>hi?hi:a);}
static float dist(float x,float y,float u,float v){float a=x-u,b=y-v;return sqrtf(a*a+b*b);}
static int wrapi(int x,int n){return (x%n+n)%n;}
static int pressed(int b){return (g.pressed&b)!=0;}
static int held(int b){return (g.held&b)!=0;}
static void notice(const char *s){strncpy(g.notice,s,sizeof(g.notice)-1);g.notice[sizeof(g.notice)-1]=0;g.noticeT=4;}
static void menu_click(void){menuEvent++;}
static const char *weaponNames[8]={"PUNOS","PISTOLA","REVOLVER","SUBFUSIL","AK","ESCOPETA","RIFLE","BATE"};
static int weapon_menu(float ax,float ay,float dt){
 if(g.screen!=WORLD||g.car>=0||g.inMetro){g.weaponWheel=0;g.weaponHold=0;return 0;}
 if(held(B_L)){
  g.weaponHold+=dt;
  if(!g.weaponWheel&&g.weaponHold>=.18f){g.weaponWheel=1;g.weaponChoice=g.weapon;}
  if(g.weaponWheel){
   if(ax*ax+ay*ay>.30f*.30f){float a=atan2f(ax,-ay);g.weaponChoice=wrapi((int)floorf(a/(PI/4)+.5f),8);}
   return 1;
  }
 }else{
  g.weaponHold=0;
  if(g.weaponWheel){g.weapon=g.weaponChoice;g.weaponWheel=0;g.moveActive=0;g.stickActive=0;notice(weaponNames[g.weapon]);return 1;}
 }
 return 0;
}
/* v1.1: dibujo optimizado para PSP real: recorrido por filas y trazado de pixel sin recorte por llamada. */
/* v2.41 (Claude): franjas de 16 filas con HUD en este fotograma: solo esas se limpian y se componen en la GPU. */
static unsigned char hudBand[17];
static inline void px(int x,int y,uint32_t c){if((unsigned)x<(unsigned)W&&(unsigned)y<(unsigned)H){fb[y*pitch+x]=c;hudBand[y>>4]=1;}}
static void rect(int x,int y,int w,int h,uint32_t c){int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>W?W:x+w,y1=y+h>H?H:y+h;if(x1<=x0||y1<=y0)return;for(int b=y0>>4;b<=(y1-1)>>4;b++)hudBand[b]=1;int n=x1-x0;for(int j=y0;j<y1;j++){uint32_t *row=fb+j*pitch+x0;for(int i=0;i<n;i++)row[i]=c;}}
static void line(int x,int y,int x1,int y1,uint32_t c){int dx=abs(x1-x),sx=x<x1?1:-1,dy=-abs(y1-y),sy=y<y1?1:-1,e=dx+dy;for(;;){px(x,y,c);if(x==x1&&y==y1)break;int z=2*e;if(z>=dy){e+=dy;x+=sx;}if(z<=dx){e+=dx;y+=sy;}}}
static void circle(int x,int y,int r,uint32_t c){for(int yy=-r;yy<=r;yy++){int xx=(int)sqrtf((float)(r*r-yy*yy));rect(x-xx,y+yy,xx*2+1,1,c);}}
static void outline(int x,int y,int w,int h,uint32_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static void font_text(int x,int y,const char *s,uint32_t c,int scale,int heading){
 const UiFont *f=&uiFonts[scale>1?(scale>2?3:2):heading];int start=x;
 for(;*s;s++){unsigned char ch=*s;if(ch=='\n'){y+=12*scale;x=start;continue;}if(ch<32||ch>126)ch='?';
  unsigned off=f->offset+(ch-32)*f->w*f->h;
  for(int yy=0;yy<f->h;yy++){int py=y+yy;if((unsigned)py>=H)continue;
   for(int xx=0;xx<f->w;xx++){int sx=x+xx;if((unsigned)sx>=W)continue;
    unsigned p=off+yy*f->w+xx;unsigned a=((uiGlyphs[p>>1]>>(4*(p&1)))&15)*17;if(a<8)continue;
    uint32_t old=fb[py*pitch+sx];
    px(sx,py,RGB(((old&255)*(255-a)+(c&255)*a+127)/255,
      (((old>>8)&255)*(255-a)+((c>>8)&255)*a+127)/255,
      (((old>>16)&255)*(255-a)+((c>>16)&255)*a+127)/255));
   }
  }x+=7*scale;
 }
}
static void text_raw(int x,int y,const char *s,uint32_t c,int scale){font_text(x,y,s,c,scale,0);}
static void text(int x,int y,const char *s,uint32_t c,int scale){text_raw(x,y,locale_text(s),c,scale);}
static int textwrap_raw(int x,int y,int width,const char *s,uint32_t c){int limit=width/7,lines=0;while(*s){while(*s==' ')s++;if(!*s)break;int n=0,last=-1;while(s[n]&&s[n]!='\n'&&n<limit){if(s[n]==' ')last=n;n++;}if(s[n]&&s[n]!='\n'&&last>0)n=last;char b[100];int k=n<99?n:99;memcpy(b,s,k);b[k]=0;text_raw(x,y+lines*13,b,c,1);s+=n;if(*s=='\n'||*s==' ')s++;lines++;}return lines*13;}
static int textwrap(int x,int y,int width,const char *s,uint32_t c){return textwrap_raw(x,y,width,locale_text(s),c);}
static void label(int x,int y,const char *s,uint32_t c){s=locale_text(s);rect(x-4,y-2,(int)strlen(s)*7+8,15,INK);text_raw(x,y,s,c,1);}
static void header(const char *a,const char *b){rect(0,0,W,48,INK);rect(16,16,4,20,LIME);font_text(28,12,locale_text(a),WHITE,1,1);text(28,29,b,MUTED,1);}
static void footer(const char *s){rect(0,252,W,20,INK);text(12,257,s,MUTED,1);}
static const Step *step_now(void){return &missions[g.mission<36?g.mission:35].steps[g.step];}
static int parkblock(int bx,int by){return cm_park(bx,by);}
#include "climb_walls.inc"
static int world_solid(float x,float y){
 if(x<8||y<8||x>WORLD_W-8||y>WORLD_H-8)return 1;
 int ly=(int)y%320;
 if(x>1396&&x<1460&&ly>86){
  int bridge=0;for(int row=0;row<7;row++)if(fabsf(y-cm_footbridge_z(row))<13.f)bridge=1;
  if(!bridge)return 1;
 }
 /* v2.6: manzanas irregulares (fusionadas, partidas, triangulares): la huella la define citymap.h */
 return cm_solid(x,y)||cm_obstacle(x,y);
}
static int solid(float x,float y){return world_solid(x,y)||combat_wall_at(x,y,0);}
static int free_at(float x,float y,int radius){
 /* A circular footprint matches the character's body. The old 3x3 square
    extended 41% farther at its corners and snagged diagonal facades. */
 if(solid(x,y))return 0;
 for(int k=0;k<8;k++){
  float a=k*(PI*.25f);
  if(world_solid(x+cosf(a)*radius,y+sinf(a)*radius))return 0;
 }
 return 1;
}
static void physics_project(float x,float y,float *a,float *b){
#ifdef NARCADE_3D
 geo_project(x,y,a,b);
#else
 *a=x;*b=y;
#endif
}
static void physics_unproject(float x,float y,float *a,float *b){
#ifdef NARCADE_3D
 geo_unproject(x,y,a,b);
#else
 *a=x;*b=y;
#endif
}
static float physics_heading(const Car *c){
#ifdef NARCADE_3D
 return geo_heading(c->x,c->y,c->a);
#else
 return c->a;
#endif
}
static float logical_heading_from_projected(float x,float y,float angle){
 float gx,gz,tx,ty;physics_project(x,y,&gx,&gz);
 physics_unproject(gx+cosf(angle)*.5f,gz+sinf(angle)*.5f,&tx,&ty); /* v2.24: sonda corta = direccion local exacta */
 return atan2f(ty-y,tx-x);
}
/* v2.25: rumbo en pantalla con sonda corta (geo_heading usa 4 unidades y falla en los pliegues). */
static float proj_heading(float x,float y,float angle){
 float a,b,c,d;physics_project(x,y,&a,&b);physics_project(x+cosf(angle)*.5f,y+sinf(angle)*.5f,&c,&d);return atan2f(d-b,c-a);
}
/* v2.6: escaleras y anden del Metro. Arriba solo se puede estar sobre el anden o la escalera; no hay saltos bruscos. */
static int lift_ok(float x,float y){
 int up=g.lift>15;float nl=cm_lift(x,y,up);
 if(up&&nl<1)return 0;                 /* borde del anden */
 if(fabsf(nl-g.lift)>6)return 0;       /* no se entra a la escalera por el lateral */
 return 1;
}
/* v2.36 (Claude): posicion del terminal de cada punto de mision (en la acera, cm_terminal_pos). */
static void terminal_xy(int i,float *x,float *y){
 static float tx[30],ty[30];static int done;
 if(!done){done=1;for(int k=0;k<30;k++)cm_terminal_pos(locations[k].x,locations[k].y,&tx[k],&ty[k]);}
 *x=tx[i];*y=ty[i];
}
/* Cerca del punto de mision o de su terminal (el terminal ya no esta en el centro del punto). */
/* v2.37 (Claude): nivel de busqueda al estilo GTA. g.heat son estrellas (entero visible = floor).
   Delitos leves suman fracciones y solo llegan a 1 estrella si se repiten; matar a un civil da 1
   (si ya te buscan, sube 1 mas desde la tercera muerte); a un policia, al menos 2. Las patrullas
   solo persiguen desde 1 estrella completa. */
static void trophy_kill(void);static void trophy_escape(int stars); /* trophies.inc */
static int cop_near(float r){for(int i=60;i<64;i++)if(i!=g.car&&g.cars[i].hp>0&&dist(g.x,g.y,g.cars[i].x,g.cars[i].y)<r)return 1;return 0;}
static int wantedKills;
static void wanted_crime(int kind){ /* 0 leve, 1 muerte de civil, 2 policia, 3 robo de carro, 4 robo de patrulla */
 float h=g.heat;int seen=cop_near(300);g.escape=0;
 if(kind==0)h+=seen?.34f:.12f;
 else if(kind==1){trophy_kill();wantedKills++;h=h<1?1:(wantedKills%3==0?floorf(h)+1:h+.2f);}
 else if(kind==2)h=fmaxf(2,floorf(h)+1);
 else if(kind==3){if(seen)h=fmaxf(h,1);else h+=.2f;}
 else h=fmaxf(h,2);
 if(h<1)wantedKills=0;
 g.heat=clampf(h,0,5);
}
static int wanted_stars(void){return (int)(g.heat+.001f);}
static int cachecount(void);
#include "trophies.inc" /* v2.42 (Claude) */
static int near_hub(int i,float radius){float tx,ty;terminal_xy(i,&tx,&ty);return dist(g.x,g.y,locations[i].x,locations[i].y)<radius||dist(g.x,g.y,tx,ty)<24;}
static int foot_free(float x,float y){
 if(!free_at(x,y,5)||combat_wall_at(x,y,5))return 0;
 for(int i=0;i<CAR_COUNT;i++){
  const Car *c=&g.cars[i];float px,py,cx,cy;physics_project(x,y,&px,&py);physics_project(c->x,c->y,&cx,&cy);float dx=px-cx,dy=py-cy;if(fabsf(dx)>27||fabsf(dy)>27)continue;
  float angle=physics_heading(c),ca=cosf(angle),sa=sinf(angle);if(fabsf(dx*ca+dy*sa)<22&&fabsf(-dx*sa+dy*ca)<13)return 0;
 }
 /* v2.25: el terminal visible es un cilindro de radio 5.5; la caja de 20x18 era una pared invisible. */
 /* v2.36: terminal en la acera (cm_terminal_pos), calculado una vez */
 for(int i=0;i<30;i++){float tx,ty;terminal_xy(i,&tx,&ty);tx=x-tx;ty=y-ty;if(tx*tx+ty*ty<7.5f*7.5f)return 0;} /* pedestal 3,6 + cuerpo */
 return 1;
}
static float angle_delta(float a,float b){float d=a-b;while(d>PI)d-=2*PI;while(d<-PI)d+=2*PI;return d;}
static void camera_follow(float target,float dt){
 /* v2.29 (Claude): seguimiento continuo y sin zona muerta. La version anterior
    se paraba si faltaban menos de 0,10 rad y arrancaba al superarlos, y recortaba
    la velocidad de golpe al llegar al limite: con el joystick (giros finos y
    continuos) la camara iba a saltos. Ahora la velocidad deseada es proporcional a
    lo que falta, con un tope suave (tanh), y la velocidad real la alcanza de forma
    progresiva: giro de camara suave en cualquier caso. */
 float fs=g.car>=0||g.followScale<=0?1:g.followScale;
 float omega=(g.car>=0?6.5f:g.footSpeed<56?3.1f:g.footSpeed<86?4.1f:5.2f)*fs;
 float limit=(g.car>=0?3.6f:g.footSpeed<56?2.2f:g.footSpeed<86?2.6f:3.0f)*fs;
 float d=angle_delta(target,g.viewYaw);
 float want=limit>0?limit*tanhf(omega*d/limit):0;
 g.cameraVelocity+=(want-g.cameraVelocity)*(1-expf(-dt*12));
 g.viewYaw+=g.cameraVelocity*dt;
}
/* v2.30 (Claude): altura de la camara suavizada. El terreno sube por tramos rectos
   (4 por cuesta): en cada union la camara cambiaba de velocidad vertical de golpe,
   y el "suelo minimo" detras de la camara la empujaba hacia arriba a saltos. Ambos
   se siguen ahora con un muelle critico (sin sobrepaso), mas notable en coche. */
static float camBaseVel,camClearVel;
static void spring(float *x,float *v,float target,float w,float dt){
 float d=*x-target,e=expf(-w*dt),vv=*v;*x=target+(d+(vv+w*d)*dt)*e;*v=(vv-w*(vv+w*d)*dt)*e;
}
static float cam_distance(void);static float cam_eye(void);
static void camera_heights(float dt){
#ifdef NARCADE_3D
 if(g.inMetro||g.lift>1){g.camBase=g.camClear=0;return;}
 float base=geo_height(g.x,g.y);
 float px,pz;geo_project(g.x,g.y,&px,&pz);float d=g.cameraDistance>0?g.cameraDistance:cam_distance();
 float lx,lz;geo_unproject(px-cosf(g.viewYaw)*d,pz-sinf(g.viewYaw)*d,&lx,&lz);float clear=geo_height(lx,lz)+12;
 if(g.camBase==0||fabsf(g.camBase-base)>40){g.camBase=base;camBaseVel=0;g.camClear=clear;camClearVel=0;return;}
 float w=g.car>=0?4.5f:9.f; /* en coche se va mas rapido: mas suavizado */
 spring(&g.camBase,&camBaseVel,base,w,dt);spring(&g.camClear,&camClearVel,clear,w*.8f,dt);
#else
 (void)dt;
#endif
}
/* v2.6.2: niveles de camara (SELECT): 0 lejana, 1 normal (por defecto), 2 cercana, 3 muy cercana. */
static const float camFootDist[4]={65,50,38,28},camFootEye[4]={43,34,26,20},camCarDist[4]={95,78,62,50},camCarEye[4]={54,46,38,32};
static float cam_distance(void){return g.car>=0?camCarDist[g.zoom&3]:camFootDist[g.zoom&3];}
static float cam_eye(void){return g.car>=0?camCarEye[g.zoom&3]:camFootEye[g.zoom&3];}
static void camera_clearance(float dt){
#ifdef NARCADE_3D
 float gx,gz;geo_project(g.x,g.y,&gx,&gz);float desired=cam_distance();if(g.inMetro){g.cameraDistance=30;return;}
 for(float d=8;d<desired;d+=2){
  float x,y;geo_unproject(gx-cosf(g.viewYaw)*d,gz-sinf(g.viewYaw)*d,&x,&y);
  /* v2.26 (Claude): solo los edificios y el borde acercan la camara; farolas,
     pilares y fuentes la hacian saltar adelante y atras al pasar junto a ellos. */
  if(x<8||y<8||x>WORLD_W-8||y>WORLD_H-8||cm_solid(x,y)){desired=fmaxf(10,d-4);break;}
 }
 /* Acercarse rapido pero en unos fotogramas (no de golpe); alejarse despacio. */
 if(g.cameraDistance<=0)g.cameraDistance=desired;
 else if(desired<g.cameraDistance)g.cameraDistance+=(desired-g.cameraDistance)*(1-expf(-dt*16));
 else g.cameraDistance+=(desired-g.cameraDistance)*(1-expf(-dt*3));
#else
 (void)dt;
#endif
}
/* Four separating axes for two oriented 36x18 car footprints. */
static int car_overlap(const Car *a,const Car *b,float *nx,float *ny,float *depth){
 if(fabsf(b->y-a->y)>42)return 0;
 float ax,ay,bx,by;physics_project(a->x,a->y,&ax,&ay);physics_project(b->x,b->y,&bx,&by);
 float dx=bx-ax,dy=by-ay,best=1e9f;
 if(fabsf(dx)>42||fabsf(dy)>42)return 0;
 float aa=physics_heading(a),ba=physics_heading(b);float ac=cosf(aa),as=sinf(aa),bc=cosf(ba),bs=sinf(ba);
 float axes[4][2]={{ac,as},{-as,ac},{bc,bs},{-bs,bc}};
 for(int k=0;k<4;k++){
  float x=axes[k][0],y=axes[k][1];
  float ra=18*fabsf(x*ac+y*as)+9*fabsf(-x*as+y*ac);
  float rb=18*fabsf(x*bc+y*bs)+9*fabsf(-x*bs+y*bc);
  float projection=dx*x+dy*y,overlap=ra+rb-fabsf(projection);
  if(overlap<=0)return 0;
  if(overlap<best){best=overlap;*nx=projection<0?-x:x;*ny=projection<0?-y:y;}
 }
 *depth=best;return 1;
}
/* v2.34 (Claude): para los coches, el borde del mundo solo cuenta a 2 unidades. En las franjas
   de transicion del valle la huella girada del coche del carril mas externo tocaba el limite de 8
   del peaton y el coche se quedaba parado para siempre. */
static int car_solid(float x,float y){
 if(x<-40||y<-40||x>WORLD_W+40||y>WORLD_H+40)return 1;
 if(x>=8&&y>=8&&x<=WORLD_W-8&&y<=WORLD_H-8)return solid(x,y);
 return cm_solid(x,y)||cm_obstacle(x,y);
}
static int car_free_at(const Car *c,float x,float y){
 Car test=*c;test.x=x;test.y=y;float angle=physics_heading(&test),ca=cosf(angle),sa=sinf(angle),gx,gy;physics_project(x,y,&gx,&gy);
 for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++){
  float lx,ly;physics_unproject(gx+ca*i*18-sa*j*9,gy+sa*i*18+ca*j*9,&lx,&ly);
  if(car_solid(lx,ly))return 0;
 }
 return 1;
}
static void car_push(const Car *c,float x,float y,float *outX,float *outY){float gx,gy;physics_project(c->x,c->y,&gx,&gy);physics_unproject(gx+x,gy+y,outX,outY);}
/* v2.39 (Claude): al separar coches, un civil en marcha no puede ser empujado fuera de la calzada
   (antes un choque lo desplazaba 26 unidades de lado y seguia recto por un parque). */
static int car_sep_ok(const Car *c,float x,float y){
 if(!car_free_at(c,x,y))return 0;
 if(c->parked||c->police||(g.car>=0&&c==&g.cars[g.car]))return 1;
 return cm_on_road(x,y);
}
static int contact_slide(Car *a,const Car *b,float nx,float ny,float depth){
 /* If a wall blocks the shortest correction, try a small tangential escape.
    Only accept a free position that actually reduces penetration. */
 float directions[4][2]={{nx-ny,ny+nx},{nx+ny,ny-nx},{-ny,nx},{ny,-nx}};
 for(int k=0;k<4;k++){
  float length=sqrtf(directions[k][0]*directions[k][0]+directions[k][1]*directions[k][1]);
  Car candidate=*a;car_push(a,directions[k][0]/length*(depth+.3f),directions[k][1]/length*(depth+.3f),&candidate.x,&candidate.y);
  if(!car_sep_ok(&candidate,candidate.x,candidate.y))continue;
  float dx,dy,d;
  if(!car_overlap(&candidate,b,&dx,&dy,&d)||d<depth-.05f){a->x=candidate.x;a->y=candidate.y;return 1;}
 }
 return 0;
}
static void separate_cars(void){
 /* Resolve independently of damage cooldown. Several passes settle pile-ups. */
 for(int pass=0;pass<8;pass++){
  int changed=0;
  for(int i=0;i<CAR_COUNT;i++)for(int j=i+1;j<CAR_COUNT;j++){
   Car *a=&g.cars[i],*b=&g.cars[j];float nx,ny,depth;
   if(!car_overlap(a,b,&nx,&ny,&depth))continue;
   float push=depth+.3f,ax,ay,bx,by,aax,aay,bbx,bby;
   car_push(a,-nx*push*.5f,-ny*push*.5f,&ax,&ay);car_push(b,nx*push*.5f,ny*push*.5f,&bx,&by);
   car_push(a,-nx*push,-ny*push,&aax,&aay);car_push(b,nx*push,ny*push,&bbx,&bby);
   int moveA=car_sep_ok(a,ax,ay),moveB=car_sep_ok(b,bx,by);
   if(a->parked&&!b->parked&&car_sep_ok(b,bbx,bby)){b->x=bbx;b->y=bby;changed=1;}
   else if(b->parked&&!a->parked&&car_sep_ok(a,aax,aay)){a->x=aax;a->y=aay;changed=1;}
   else if(moveA&&moveB&&!a->parked&&!b->parked){a->x=ax;a->y=ay;b->x=bx;b->y=by;changed=1;}
   else if(!a->parked&&car_sep_ok(a,aax,aay)){a->x=aax;a->y=aay;changed=1;}
   else if(!b->parked&&car_sep_ok(b,bbx,bby)){b->x=bbx;b->y=bby;changed=1;}
   else if((!a->parked&&contact_slide(a,b,-nx,-ny,depth))||(!b->parked&&contact_slide(b,a,nx,ny,depth)))changed=1;
   float aa=physics_heading(a),ba=physics_heading(b);
   float an=cosf(aa)*nx+sinf(aa)*ny,bn=cosf(ba)*nx+sinf(ba)*ny;
   float closing=a->speed*an-b->speed*bn,impact=fmaxf(0,closing);
   if(pass==0&&(i==g.car||j==g.car)&&impact>25&&g.hitCD<=0){
    g.cars[g.car].hp-=impact*.025f;if(g.cars[i==g.car?j:i].police||cop_near(200))wanted_crime(0);g.hitCD=.7f;
   }
   /* Resolve the inward normal velocity only. Side rubbing and reversing
      away must retain their tangential/escaping motion. */
   if(pass==0&&closing>0){
    /* Low-speed contact stops against a parked car; hard impacts add a
       controlled rebound. Parked vehicles act as immovable obstacles. */
    float restitution=clampf((impact-48.f)/210.f,0,.42f);
    float den=(a->parked?0:an*an)+(b->parked?0:bn*bn);
    if(den>.001f){float impulse=closing*(1+restitution)/den;if(!a->parked)a->speed-=impulse*an;if(!b->parked)b->speed+=impulse*bn;}
    if(a->parked)a->speed=0;if(b->parked)b->speed=0;
    /* v2.34 (Claude): solo cede el coche que va hacia el otro. Antes cedian los dos y, si seguian
       en contacto, se volvian a ceder cada fotograma: quedaban parados para siempre (atascos). */
    {int ya=an>.2f&&!a->parked,yb=bn<-.2f&&!b->parked;if(ya&&yb){if(i<j)ya=0;else yb=0;}
     if(ya&&i!=g.car)g.trafficYield[i]=.45f;if(yb&&j!=g.car)g.trafficYield[j]=.45f;}
   }
  }
  if(!changed)break;
 }
 if(g.car>=0){g.x=g.cars[g.car].x;g.y=g.cars[g.car].y;}
}
/* v2.6: tren del Metro (va y vuelve por x=CM_METRO_X, para 6 s en cada estacion) y viaje del jugador a bordo. */
static float metroSpeed=0;
static void metro_update(float dt){
 if(g.metroDir==0){g.metroDir=1;g.metroZ=cm_station_z(1);g.metroWait=6;metroSpeed=0;}
 if(g.metroWait>0){g.metroWait=fmaxf(0,g.metroWait-dt);metroSpeed=0;}
 else{
  float remaining=g.metroDir>0?2180-g.metroZ:g.metroZ-60;
  for(int bz=1;bz<=5;bz+=2){float d=(cm_station_z(bz)-g.metroZ)*g.metroDir;if(d>.01f&&d<remaining)remaining=d;}
  float wanted=fminf(130,sqrtf(fmaxf(0,2*45*remaining)));
  metroSpeed+=clampf(wanted-metroSpeed,-45*dt,35*dt);
  float before=g.metroZ;g.metroZ+=g.metroDir*metroSpeed*dt;
  for(int bz=1;bz<=5;bz+=2){float sz=cm_station_z(bz);if(before!=sz&&(before-sz)*(g.metroZ-sz)<=0){g.metroZ=sz;g.metroWait=6;metroSpeed=0;break;}}
  if(g.metroZ<60){g.metroZ=60;g.metroDir=1;g.metroWait=2;}if(g.metroZ>2180){g.metroZ=2180;g.metroDir=-1;g.metroWait=2;}
 }
 if(g.inMetro){g.x=CM_METRO_X;g.y=g.metroZ;g.lift=CM_PLAT_H+1;g.a=g.metroDir>0?PI*.5f:-PI*.5f;g.viewYaw=geo_heading(g.x,g.y,g.a);g.cameraVelocity=0;g.walking=0;}
}
static int metro_boardable(void){return !g.inMetro&&g.car<0&&g.lift>15&&g.metroWait>0&&cm_station_near(g.metroZ,2)==(int)floorf(g.y/320)&&cm_on_platform(g.x,g.y);}
static int district(float x,float y){if(x>1950&&y<640)return 7;if(x<760&&y<650)return 8;if(x<640&&y<1500)return 0;if(x<1100&&y<1450)return 1;if(x<1250)return 2;if(y<650)return 3;if(x>1600&&y>1500)return 4;if(x>1800)return 5;return 6;}
static const char *districts[]={"SAN JAVIER / COMUNA 13","LAURELES / ESTADIO","BELEN","ARANJUEZ / CASTILLA","EL POBLADO","VILLA HERMOSA / BUENOS AIRES","LA CANDELARIA / RIO","POPULAR / SANTO DOMINGO","ROBLEDO / DOCE DE OCTUBRE"};
static uint32_t carcolors[]={RGB(62,169,154),RGB(230,188,69),RGB(167,80,73),RGB(179,191,183),RGB(79,121,160),RGB(116,91,147)};
static void map_grid_build(void);
static void combat_init_peds(void);
static void world_cars_init(void);static void world_peds_init(void);
static void world_init(void){
 map_grid_build();
 world_cars_init();world_peds_init();
}
static void world_cars_init(void){
 rng=729;for(int i=0;i<CAR_COUNT;i++){
  Car *c=&g.cars[i];int bx=random_u()%8,by=random_u()%7;
  c->type=i%6;c->police=i>=60;c->hp=100;c->speed=0;c->parked=i<38;
  c->x=bx*320+42;c->y=by*320+42;
  c->a=(i%2)?PI*.5f:0;
  if(c->parked){c->x=bx*320+70;c->y=by*320+145+(i%3)*37;c->a=PI*.5f;}
 }
 // One car beside every mission hub: progression never depends on a random spawn.
 for(int i=0;i<30;i++){g.cars[i].x=locations[i].x+54;g.cars[i].y=locations[i].y+18;g.cars[i].a=0;g.cars[i].parked=1;}
 for(int i=0;i<CAR_COUNT;i++)if(!car_free_at(&g.cars[i],g.cars[i].x,g.cars[i].y)){
  Car *c=&g.cars[i];float bx=floorf(c->x/320)*320,by=floorf(c->y/320)*320;int found=0;
  for(int k=0;k<10&&!found;k++){
   float xx=bx+52+k*22,yy=by+42;c->a=0;
   if(car_free_at(c,xx,yy)){c->x=xx;c->y=yy;found=1;}
  }
  for(int k=0;k<10&&!found;k++){ /* v2.6: si la calle norte esta fusionada, probar la calle oeste */
   float xx=bx+42,yy=by+52+k*22;c->a=PI*.5f;
   if(car_free_at(c,xx,yy)){c->x=xx;c->y=yy;found=1;}
  }
 }
 g.cars[1].type=0;
 /* Ten long, obstruction-free two-way corridors. Keep civilian traffic on
    the right-hand lane, with several car lengths of initial separation. */
 for(int i=30;i<60;i++){
  Car *c=&g.cars[i];int route=(i-30)%10,place=(i-30)/10;
  static const int rows[2]={0,3},cols[3]={0,3,5};
  if(route<4){int row=rows[route/2],east=(route&1)==0;
   c->a=east?0:PI;c->x=170+place*790;c->y=row*320+42+(east?14:-14);
  }else{int col=cols[(route-4)/2],south=(route&1)==0;
   c->a=south?PI*.5f:-PI*.5f;c->x=col*320+42+(south?-14:14);c->y=170+place*690;
  }
  c->parked=0;c->speed=0;
 }
}
static void world_peds_init(void){
 combat_init_peds();
 for(int i=0;i<42;i++){
  /* v2.34 (Claude): el peaton se coloca en el centro de una acera real (4 antes del borde de la
     parcela; las manzanas empiezan entre 86 y 100). Antes iba en x local 82, que en la mayoria de
     manzanas era calzada (84% del tiempo en la calle, medido). */
  Ped *p=&g.peds[i];int ok=0;
  for(int k=0;k<40&&!ok;k++){float bx=(random_u()%8)*320.f;p->y=(random_u()%7)*320+100+(random_u()%180);
   for(float lx=78;lx<=112;lx+=1)if(cm_parcel_at(bx+lx,p->y)>=0){p->x=bx+lx-4;ok=cm_parcel_at(p->x,p->y)<0;break;}}
  if(!ok){p->x=-1000;p->y=-1000;p->v=0;} /* sin sitio: no se dibuja en la ciudad */
  g.peds[i].v=(i%2?1:-1)*13;g.peds[i].vertical=1;g.peds[i].phase=i;}
}
int cachecount_public(void);
static void fill_save(Save *s);
static uint32_t savecheck(const Save *s){uint32_t h=2166136261u;const unsigned char *p=(const unsigned char*)s;for(size_t i=0;i<offsetof(Save,check);i++){h^=p[i];h*=16777619u;}return h;}
void game_set_save_path(const char *p){raw_snprintf(g.savepath,sizeof(g.savepath),"%s",p);settings_load();trophies_load();}
int game_save(void){
 trophies_flush(); /* v2.42: los trofeos van en su propio archivo */
 if(!g.active)return 1;
 Save s;fill_save(&s);
 char tmp[300],bak[300];raw_snprintf(tmp,sizeof(tmp),"%s.tmp",g.savepath);raw_snprintf(bak,sizeof(bak),"%s.bak",g.savepath);
 FILE *f=fopen(tmp,"wb");if(!f){g.saveOK=0;return 0;}int ok=fwrite(&s,1,sizeof(s),f)==sizeof(s);if(fclose(f))ok=0;
 if(!ok){remove(tmp);g.saveOK=0;return 0;}
 remove(bak);rename(g.savepath,bak);if(rename(tmp,g.savepath)){rename(bak,g.savepath);g.saveOK=0;return 0;}g.saveOK=1;return 1;
}
/* v2.2 (Claude): guardado nativo de PSP. El bucle principal consulta game_take_request() y abre el
   dialogo de la Memory Stick (sceUtilitySavedata); los datos van y vienen con export/import. */
static int nativeSave=0,saveRequest=0;
void game_set_native_savedata(int on){nativeSave=on;}
void game_set_lowmem2(const char *msg){notice(msg);} /* diagnostico */
/* v2.13.2: pantalla de carga (dibujo directo, sin depender del bucle principal) y reinicio del mundo tras cargar. */
/* v2.13.3 (Claude): carga por ETAPAS. Cada llamada hace un trozo de trabajo acotado y dibuja la barra; asi ninguna
   llamada monopoliza la CPU varios segundos (el hilo de audio y el sistema siguen atendidos) y, si algo falla, el
   registro de psp_main dice exactamente en que etapa. Devuelve 0 mientras queda trabajo, 1 cuando termina. */
static const char *LOAD_STEPS[]={"Leyendo la partida","Trazando calles y manzanas","Repartiendo el trafico",
    "Colocando peatones","Dibujando el mapa de la ciudad","Preparando la camara","Construyendo la ciudad visible","Listo"};
#ifdef NARCADE_3D
static void build_scene(R3Scene *sp);
#endif
enum{LOAD_WARM_MAX=14}; /* v2.37: etapas 6..19 construyen la ciudad visible (6 piezas cada una) */
/* v2.41 (Claude): pantalla de carga ilustrada (arte de Codex, assets/loading-v241). Las imagenes NO van en el
   ejecutable (885 KB; vamos justos de memoria): el ISO las lleva en PSP_GAME/USRDIR/LOADn.BIN (RGB565 512x288)
   y se leen durante la carga en la capa del HUD, que en ese momento no se usa. Cada frase describe el trabajo
   que se hace mientras se ve (la etapa siguiente); la barra sigue las etapas reales. */
static uint32_t __attribute__((aligned(16))) hudLayer[512*512];
static unsigned char hudPrev[17]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
#define LOAD_IMG_W 512
#define LOAD_IMG_H 288
static uint16_t *const loadSlot[2]={(uint16_t*)hudLayer,(uint16_t*)hudLayer+LOAD_IMG_W*LOAD_IMG_H};
static int loadShown=-1,loadPrev=-1,loadOk[3],loadSlotOf[3]={-1,-1,-1},loadFade=256;
static float loadZoom=1,loadBar,loadClock;
void game_load_clock(float seconds){loadClock=seconds;} /* segundos desde el inicio de la carga (psp_main) */
static int load_image(int k){ /* lee LOADk.BIN en la ranura libre; 0 si no esta (fondo liso) */
 static const char *const dirs[]={"disc0:/PSP_GAME/USRDIR/","ms0:/PSP/GAME/NARCADE/"};
 static const char *const host[]={"assets/loading-v241/valley.rgb565","assets/loading-v241/street.rgb565","assets/loading-v241/rooftop.rgb565"};
 int slot=loadShown>=0&&loadSlotOf[loadShown]==0?1:0;
 for(int d=0;d<3;d++){char path[96];
  if(d<2){strcpy(path,dirs[d]);size_t n=strlen(path);path[n]='L';path[n+1]='O';path[n+2]='A';path[n+3]='D';path[n+4]=(char)('0'+k);strcpy(path+n+5,".BIN");}
  else strcpy(path,host[k]);
  FILE *f=fopen(path,"rb");if(!f)continue;size_t got=fread(loadSlot[slot],2,LOAD_IMG_W*LOAD_IMG_H,f);fclose(f);
  if(got==LOAD_IMG_W*LOAD_IMG_H){for(int q=0;q<3;q++)if(loadSlotOf[q]==slot)loadSlotOf[q]=-1;loadSlotOf[k]=slot;return 1;}}
 return 0;
}
static inline uint32_t rgb565_to_8888(uint16_t c){unsigned r=c&31,g5=(c>>5)&63,b=(c>>11)&31; /* formato PSP 5650: rojo en los bits bajos */return 0xff000000u|(((b<<3)|(b>>2))<<16)|(((g5<<2)|(g5>>4))<<8)|((r<<3)|(r>>2));}
/* imagen con zoom centrado (1 = recorte 480x272 del centro), brillo 0..256 y mezcla opcional sobre lo que hay */
static void load_blit(uint32_t *dst,int stride,const uint16_t *img,float zoom,int light,int blend){
 float sw=480.f/zoom,sh=272.f/zoom,x0=(LOAD_IMG_W-sw)*.5f,y0=(LOAD_IMG_H-sh)*.5f;
 int fx0=(int)(x0*65536),fdx=(int)(sw/480.f*65536),fy0=(int)(y0*65536),fdy=(int)(sh/272.f*65536);
 for(int y=0;y<272;y++){const uint16_t *row=img+((fy0+y*fdy)>>16)*LOAD_IMG_W;uint32_t *out=dst+y*stride;int fx=fx0;
  for(int x=0;x<480;x++,fx+=fdx){uint32_t c=rgb565_to_8888(row[fx>>16]);
   unsigned r=(c&255)*light>>8,g=((c>>8)&255)*light>>8,b=((c>>16)&255)*light>>8;
   if(blend){uint32_t o=out[x];r=(r*blend+(o&255)*(256-blend))>>8;g=(g*blend+((o>>8)&255)*(256-blend))>>8;b=(b*blend+((o>>16)&255)*(256-blend))>>8;}
   out[x]=0xff000000u|(b<<16)|(g<<8)|r;}}
}
static const char *const loadEs[]={"Recuperando tu historia","Trazando las calles y manzanas de Medellin","Arrancando los motores del trafico",
 "Llenando las aceras de vecinos","Dibujando el mapa del barrio","Cargando cartuchos y ajustando la mira","Encendiendo las luces de la ciudad","Listo. Buena suerte, Nico"};
static void load_screen_draw(uint32_t *pixels,int stride,int stage,int anim){
 fb=pixels;pitch=stride;
 static int drawnStage;if(anim)stage=drawnStage;else drawnStage=stage; /* la animacion repite la etapa ya dibujada (incluido el final) */
 if(stage==0&&!anim){loadShown=loadPrev=-1;loadOk[0]=loadOk[1]=loadOk[2]=0;loadSlotOf[0]=loadSlotOf[1]=loadSlotOf[2]=-1;loadFade=256;loadZoom=1;loadBar=0;}
 int warmEnd=6+LOAD_WARM_MAX,finished=stage>=warmEnd;
 int caption=finished?7:stage+1<6?stage+1:6;            /* trabajo que se hace mientras se ve este cuadro */
 /* v2.42 (Claude): cada ilustracion dura al menos 5 s de reloj real; si la carga es rapida solo se ven una o dos */
 int want=(int)(loadClock/5.f);if(want>2)want=2;
 if(!anim&&want!=loadShown){loadPrev=loadShown;loadOk[want]=load_image(want);loadShown=want;loadFade=0;loadZoom=1;}
 {static float shownAt;if(loadFade==0)shownAt=loadClock;loadZoom=1.f+.066f*fminf(1.f,(loadClock-shownAt)/5.f);} /* zoom lento durante los 5 s de cada imagen */if(loadFade<256)loadFade=loadFade+43>256?256:loadFade+43;
 float target=finished?1.f:(float)(stage+1)/(warmEnd+1);
 if(finished)loadBar=1;else if(loadBar<target)loadBar=fminf(target,loadBar+fmaxf(.015f,(target-loadBar)*.45f));
 if(loadShown>=0&&loadOk[loadShown]){
  int prevOk=loadFade<256&&loadPrev>=0&&loadOk[loadPrev]&&loadSlotOf[loadPrev]>=0;
  if(prevOk)load_blit(pixels,stride,loadSlot[loadSlotOf[loadPrev]],1.066f,210,0);else if(loadFade<256)rect(0,0,W,H,INK);
  load_blit(pixels,stride,loadSlot[loadSlotOf[loadShown]],loadZoom,210,loadFade<256?loadFade:0);
 }else{rect(0,0,W,H,INK);text(W/2-7*3,90,"NARCADE",LIME,1);}
 rect(0,H-58,W,58,RGB(7,13,23));rect(0,H-58,W,2,LIME);
 text(16,H-50,"NARCADE",LIME,1);
 text(16,H-32,loadEs[caption],WHITE,1); /* text() traduce con el catalogo (localization_en.json) */
 int bw=W-32;rect(16,H-12,bw,5,PANEL);rect(16,H-12,(int)(bw*loadBar),5,LIME);
 char b[16];int pct=(int)(loadBar*100+.5f);b[0]=0;if(pct>=100){b[0]='1';b[1]='0';b[2]='0';b[3]='%';b[4]=0;}else{b[0]=(char)(pct>=10?'0'+pct/10:' ');b[1]=(char)('0'+pct%10);b[2]='%';b[3]=0;}
 text(W-16-(int)strlen(b)*7,H-50,b,MUTED,1);
}
/* fotogramas de animacion entre etapas (zoom y fundido siguen sin tocar la carga) */
/* devuelve 1 mientras quede fundido por terminar: la carga no sigue con una imagen a medio fundir */
int game_load_anim(uint32_t *pixels,int stride,int stage){load_screen_draw(pixels,stride,stage,1);return loadFade<256;}
/* la capa del HUD tuvo imagenes: el primer fotograma de juego la limpia entera */
void game_load_finish(void){memset(hudPrev,1,sizeof hudPrev);}
int game_load_stage(uint32_t *pixels,int stride,int stage){
 static int warmDone;if(stage==0)warmDone=0;else if(warmDone)return 1;
 switch(stage){
  case 0: break;                                   /* la partida ya esta aplicada */
  case 1: cm_build(); break;                       /* parcelas de las 56 celdas (cache) */
  case 2: world_cars_init(); break;                /* coches */
  case 3: world_peds_init(); break;                /* peatones */
  case 4: map_grid_build(); break;                 /* rejilla del minimapa (640x560) */
  case 5: g.cameraDistance=0;g.cameraVelocity=0;g.moveActive=0;g.stickActive=0;g.noticeT=0;g.screenT=0;break;
  default:
#ifdef NARCADE_3D
   if(stage<6+LOAD_WARM_MAX&&!warmDone){static R3Scene sc;build_scene(&sc);if(r3_prewarm(&sc)==0){warmDone=1;stage=6+LOAD_WARM_MAX;}break;}
#endif
   return 1;
 }
 load_screen_draw(pixels,stride,stage,0);
 return 0;
}
void game_world_reset(void){world_init();}
void game_player_pos(int *x,int *y){*x=(int)g.x;*y=(int)g.y;}
void game_set_lowmem(int kb){char b[96];snprintf(b,sizeof(b),"Sin memoria para el menu de la Memory Stick (%d KB libres).",kb);notice(b);} /* v2.13.1 */
int game_take_request(void){int r=saveRequest;saveRequest=0;return r;}
void game_request_result(int req,int ok){
 if(req==1)notice(ok?"Partida guardada en la Memory Stick.":"Guardado cancelado.");
 else if(req==3){if(ok){game_save();g.screen=TITLE;g.menu=0;}else notice("Guardado cancelado. Sigues en la partida.");}
 else if(req==2&&!ok){g.screen=TITLE;g.menu=0;notice("Carga cancelada.");}}
static void fill_save(Save *s){memset(s,0,sizeof(*s));s->magic=0x4e415243;s->version=1;s->mission=g.mission;s->step=g.step;s->cash=g.cash;s->reputation=g.reputation;s->ending=g.ending;s->x=g.x;s->y=g.y;s->health=g.health;s->playtime=g.playtime;s->caches=g.caches;s->jobs=g.jobs;s->station=g.station;s->check=savecheck(s);
 if(s->mission<36&&s->step>=missions[s->mission].count){s->mission++;s->step=0;s->check=savecheck(s);}}
int game_export_save(void *buf,int cap){if(!g.active||cap<(int)sizeof(Save))return 0;fill_save((Save*)buf);return sizeof(Save);}
void game_save_summary(char *title,int titlecap,char *detail,int detailcap){
 int m=g.mission<36?g.mission:35;snprintf(title,titlecap,"Mision %02d/36 - %s",g.mission<36?g.mission+1:36,g.mission<36?missions[m].title:"Historia completada");
 int h=(int)g.playtime/3600,mi=((int)g.playtime/60)%60;
 snprintf(detail,detailcap,"$%d  -  %d vinilos  -  %d encargos\nTiempo de juego %d:%02d\nmade by Naresz",g.cash,cachecount_public(),g.jobs,h,mi);}
static int apply_save(const Save *sp){Save s=*sp;g.lift=0;g.inMetro=0;
 if(s.magic!=0x4e415243||s.version!=1||s.check!=savecheck(&s)||s.mission<0||s.mission>36||s.step<0||s.step>=6||s.cash<0||s.cash>100000000||s.station<0||s.station>4||!isfinite(s.x)||!isfinite(s.y)||!isfinite(s.health)||!isfinite(s.playtime)||s.playtime<0)return 0;
 if(s.mission<36&&s.step>=missions[s.mission].count)return 0;
 g.mission=s.mission;g.step=s.step;g.cash=s.cash;g.reputation=s.reputation;g.ending=s.ending;g.x=clampf(s.x,10,WORLD_W-10);g.y=clampf(s.y,10,WORLD_H-10);g.health=clampf(s.health,1,100);g.playtime=s.playtime;g.caches=s.caches;g.jobs=s.jobs;g.station=s.station;g.car=-1;g.heat=0;g.side=0;g.raceTime=0;g.missionTimer=0;g.active=1;g.stamina=100;g.exhausted=0;g.sprintTime=0;g.runTaps=0;g.footSpeed=0;g.footTravel=0;g.footFiltered=0;g.footStall=0;g.motion=0;g.moveActive=0;g.stickActive=0;g.a=-PI*.5f;g.viewYaw=geo_heading(g.x,g.y,g.a);g.cameraVelocity=0;return 1;}
/* v2.13.6 (Claude): el tamano debe coincidir EXACTAMENTE. Una partida guardada por una version con otro formato
   pasaba las comprobaciones de rango y se cargaba con estado incoherente; en la consola eso acabo reiniciandola
   al dibujar la ciudad (en el emulador no). Mejor rechazarla con un aviso claro. */
int game_import_save(const void *buf,int len){
 if(len!=(int)sizeof(Save)){notice("Esa partida es de una version anterior del juego y no se puede cargar.");return 0;}
 if(!apply_save((const Save*)buf)){notice("Los datos de la partida estan danados.");return 0;}
 g.screen=WORLD;notice("Partida cargada desde la Memory Stick.");return 1;}
static int loadfile(const char *p){Save s;FILE *f=fopen(p,"rb");if(!f)return 0;size_t n=fread(&s,1,sizeof(s),f);fclose(f);
 if(n!=sizeof(s))return 0;return apply_save(&s);}
#if 0
static int loadfile_old(const char *p){Save s;FILE *f=fopen(p,"rb");if(!f)return 0;size_t n=fread(&s,1,sizeof(s),f);fclose(f);
 if(n!=sizeof(s)||s.magic!=0x4e415243||s.version!=1||s.check!=savecheck(&s)||s.mission<0||s.mission>36||s.step<0||s.step>=6||s.cash<0||s.cash>100000000||s.station<0||s.station>4||!isfinite(s.x)||!isfinite(s.y)||!isfinite(s.health)||!isfinite(s.playtime)||s.playtime<0)return 0;
 if(s.mission<36&&s.step>=missions[s.mission].count)return 0;
 g.mission=s.mission;g.step=s.step;g.cash=s.cash;g.reputation=s.reputation;g.ending=s.ending;g.x=clampf(s.x,10,WORLD_W-10);g.y=clampf(s.y,10,WORLD_H-10);g.health=clampf(s.health,1,100);g.playtime=s.playtime;g.caches=s.caches;g.jobs=s.jobs;g.station=s.station;g.car=-1;g.heat=0;g.side=0;g.raceTime=0;g.missionTimer=0;g.active=1;g.stamina=100;g.exhausted=0;g.sprintTime=0;g.runTaps=0;return 1;
}
#endif
static int load_game(void){if(loadfile(g.savepath))return 1;char b[300];raw_snprintf(b,sizeof(b),"%s.bak",g.savepath);return loadfile(b);}
static void fresh_game(void);
void game_continue(void){if(load_game()){g.screen=WORLD;notice("Partida cargada. SELECT camara / O cuaderno.");}else fresh_game();}
static void dialog(const char *who,const char *s,int action){strncpy(g.speaker,who,sizeof(g.speaker)-1);g.speaker[sizeof(g.speaker)-1]=0;strncpy(g.dialog,s,sizeof(g.dialog)-1);g.dialog[sizeof(g.dialog)-1]=0;g.dialogAction=action;g.screen=DIALOG;g.screenT=0;}
static void start_mission(void){g.missionTimer=0;g.checkpoint=0;g.raceTime=0;g.seenIntro=1;if(g.mission<36)dialog(missions[g.mission].who,missions[g.mission].intro,0);}
static void fresh_game(void){g.weapon=0;g.weaponWheel=0;g.weaponHold=0;g.stamina=100;g.exhausted=0;g.runTaps=0;g.tapAge=10;g.sprintTime=0;g.footSpeed=0;g.footTravel=0;g.footFiltered=0;g.footStall=0;g.moveGap=0;g.stickActive=0;g.cameraVelocity=0;g.cameraDistance=50;g.zoom=1;g.viewYaw=-PI*.5f;g.motion=0;g.gaitPhase=0;memset(g.trafficYield,0,sizeof(g.trafficYield));g.active=1;g.mission=0;g.step=0;g.cash=350;g.reputation=0;g.ending=0;g.x=62;g.y=1022;g.health=100;g.heat=0;g.car=-1;g.station=0;g.caches=0;g.jobs=0;g.side=0;g.playtime=0;g.lift=0;g.inMetro=0;g.metroDir=0;
#ifdef NARCADE_SPAWN_X
 g.x=NARCADE_SPAWN_X;g.y=NARCADE_SPAWN_Y;g.lift=cm_on_platform(g.x,g.y)?CM_PLAT_H:0; /* solo pruebas: NARCADE_EXTRA_CFLAGS="-DNARCADE_SPAWN_X=.. -DNARCADE_SPAWN_Y=.." */
#endif
 g.a=-PI*.5f;g.viewYaw=geo_heading(g.x,g.y,g.a);world_init();start_mission();}
static void advance(void){
 if(g.side){g.cash+=180;g.jobs++;g.reputation++;g.side=0;g.raceTime=0;notice("ENCARGO COMPLETO  +$180  +1 reputacion");g.screen=WORLD;game_save();return;}
 g.step++;g.checkpoint=0;g.raceTime=0;g.missionTimer=0;g.screen=WORLD;
 if(g.step>=missions[g.mission].count){g.cash+=missions[g.mission].reward;g.reputation+=3;dialog("MISION COMPLETADA",missions[g.mission].outro,2);}else{notice("Objetivo completado. Consulta la nueva marca amarilla.");game_save();}
}
static void end_dialog(void){int a=g.dialogAction;g.screen=WORLD;if(a==1)advance();if(a==2){g.mission++;g.step=0;game_save();if(g.mission<36)start_mission();else{g.heat=0;dialog("NARCADE / FIN DE LA HISTORIA",g.ending==1?"Vera entrega el expediente a la justicia. Los vecinos conservan sus datos. Sara vuelve a casa. La historia termina, pero la ciudad sigue abierta: encuentra los 24 vinilos, realiza encargos y recorre Medellin. made by Naresz.":"Mara publica las pruebas sin exponer los datos privados. Los barrios guardan copias y vigilan su ciudad. Sara vuelve a casa. La historia termina, pero puedes seguir explorando, reunir los 24 vinilos y realizar encargos. made by Naresz.",0);}}}
void game_init(void){memset(&g,0,sizeof(g));g.stamina=100;g.zoom=1;g.hudDistrict=-1;g.screen=TITLE;g.car=-1;g.health=100;g.cash=350;g.x=62;g.y=1022;g.viewYaw=-PI*.5f;strcpy(g.savepath,"NARCADE.SAV");settings_load();trophies_load();world_init();}

/* Mini-games: all are real state machines and require player input. */
static const char *puzzleNames[]={"","","PC / CLAVE DE CUATRO DIGITOS","PC / CIRCUITO AISLADO","PC / MEMORIA DEL REGISTRO","RADIO / SINTONIA FINA","CONSOLA / SESION DE RITMO","ESTUCHE / CIERRE MECANICO","ACCESO / PATIO VIGILADO"};
static int rot(int v){return ((v<<1)&15)|((v>>3)&1);}
static int circuit_connected(void){Puzzle *p=&g.p;memset(p->connected,0,sizeof(p->connected));if(!(p->tiles[0]&8))return 0;int q[16],n=1;p->connected[0]=1;q[0]=0;int dx[]={0,1,0,-1},dy[]={-1,0,1,0};for(int z=0;z<n;z++){int c=q[z];for(int d=0;d<4;d++){int x=c%4+dx[d],y=c/4+dy[d];if(x<0||y<0||x>3||y>3)continue;int k=y*4+x;if((p->tiles[c]&(1<<d))&&(p->tiles[k]&(1<<((d+2)%4)))&&!p->connected[k]){p->connected[k]=1;q[n++]=k;}}}return p->connected[15]&&(p->tiles[15]&2);}
static void puzzle_start(int kind,int seed){Puzzle *p=&g.p;memset(p,0,sizeof(*p));p->kind=kind;p->seed=seed;rng=seed*9871u+823u;p->x=28;p->y=142;
 int v=(seed*37+1423)%10000;for(int i=3;i>=0;i--){p->answer[i]=v%10;v/=10;}
 // Solvable snake: input at W of 0, output at E of 15, shuffled rotations.
 int solution[]={10,10,10,12,6,10,10,9,3,10,10,12,10,10,10,3};
 for(int i=0;i<16;i++){p->tiles[i]=solution[i];int r=random_u()%4;while(r--)p->tiles[i]=rot(p->tiles[i]);}
 for(int i=0;i<12;i++)p->seq[i]=random_u()%4;
 p->target=18+(seed*13)%64;p->value=50;
 for(int i=0;i<24;i++)p->notes[i]=random_u()%4;
 g.screen=MINI;g.screenT=0;
}
static void puzzle_success(void){dialog("ACCESO CONSEGUIDO","Bien. El objetivo esta resuelto y la informacion ha quedado en tu cuaderno. Continuemos con la siguiente pista.",1);}
static void puzzle_tick(float ax,float ay,float dt){
 Puzzle *p=&g.p;p->t+=dt;
 if(pressed(B_CIRCLE)){g.screen=WORLD;notice("Puedes volver a intentarlo cuando quieras.");return;}
 if(p->kind==K_CODE){
  if(pressed(B_LEFT))p->cursor=wrapi(p->cursor-1,4);if(pressed(B_RIGHT))p->cursor=(p->cursor+1)%4;
  if(pressed(B_UP))p->digits[p->cursor]=(p->digits[p->cursor]+1)%10;if(pressed(B_DOWN))p->digits[p->cursor]=wrapi(p->digits[p->cursor]-1,10);
  if(pressed(B_CROSS)){int ok=1;for(int i=0;i<4;i++)if(p->digits[i]!=p->answer[i])ok=0;if(ok)puzzle_success();else{p->miss++;notice("La clave no coincide. Revisa las cuatro pistas.");}}
 }else if(p->kind==K_CIRCUIT){
  if(pressed(B_LEFT))p->cursor=wrapi(p->cursor-1,16);if(pressed(B_RIGHT))p->cursor=(p->cursor+1)%16;if(pressed(B_UP))p->cursor=wrapi(p->cursor-4,16);if(pressed(B_DOWN))p->cursor=(p->cursor+4)%16;
  if(pressed(B_CROSS))p->tiles[p->cursor]=rot(p->tiles[p->cursor]);if(circuit_connected())puzzle_success();
 }else if(p->kind==K_MEMORY){
  int length=4+p->phase;if(p->t<(float)length*.8f+1)return;
  int k=-1;if(pressed(B_UP))k=0;if(pressed(B_RIGHT))k=1;if(pressed(B_DOWN))k=2;if(pressed(B_LEFT))k=3;
  if(pressed(B_SQUARE)){p->input=0;p->t=0;return;}
  if(k>=0){if(k==p->seq[p->input]){p->input++;if(p->input==length){p->phase++;p->input=0;p->t=0;if(p->phase==3)puzzle_success();}}else{p->input=0;p->t=0;p->miss++;notice("Secuencia distinta. Mira de nuevo el patron.");}}
 }else if(p->kind==K_TUNE){
  float move=(held(B_RIGHT)-held(B_LEFT))+ax;p->value=clampf(p->value+move*dt*24,0,100);
  float target=p->target+sinf(p->t*.75f)*4;
  if(fabsf(p->value-target)<4&&held(B_CROSS))p->hold+=dt;else p->hold=fmaxf(0,p->hold-dt*.65f);
  if(p->hold>4.0f)puzzle_success();
 }else if(p->kind==K_LOCK){
  p->value=(sinf(p->t*(2.0f+p->phase*.35f))+1)*50;
  if(pressed(B_CROSS)){if(fabsf(p->value-p->target)<11){p->phase++;p->target=20+random_u()%60;if(p->phase==4)puzzle_success();}else{p->miss++;notice("Cerca. Pulsa X dentro de la zona verde.");}}
 }else if(p->kind==K_RHYTHM){
  const int keys[]={B_LEFT,B_UP,B_DOWN,B_RIGHT};
  for(int i=0;i<24;i++){float at=2.0f+i*.57f;if(!p->hit[i]&&p->t>at+.22f){p->hit[i]=-1;p->miss++;}}
  for(int k=0;k<4;k++)if(pressed(keys[k])){int best=-1;float diff=.23f;for(int i=0;i<24;i++)if(!p->hit[i]&&p->notes[i]==k){float d=fabsf(p->t-(2.0f+i*.57f));if(d<diff){best=i;diff=d;}}if(best>=0){p->hit[best]=1;p->hits++;}else p->miss++;}
  if(p->t>16.0f){if(p->hits>=14)puzzle_success();else{p->t=0;p->hits=0;p->miss=0;memset(p->hit,0,sizeof(p->hit));notice("Necesitas 14 aciertos. Repetimos la sesion.");}}
 }else if(p->kind==K_STEALTH){
  p->x=clampf(p->x+(ax+held(B_RIGHT)-held(B_LEFT))*dt*76,25,452);p->y=clampf(p->y+(ay+held(B_DOWN)-held(B_UP))*dt*76,79,226);
  for(int i=0;i<5;i++){float xx=90+i*68,cy=150+sinf(p->t*(.85f+i*.08f)+i*1.7f+p->seed)*47;
   if(fabsf(p->x-xx)<12&&fabsf(p->y-cy)>26){p->x=28;p->y=142;p->miss++;notice("Te detectaron. Busca el hueco entre los haces.");break;}}
  if(p->x>447)puzzle_success();
 }
}

static void race_start(int side){g.side=side;g.checkpoint=0;g.raceTime=side?140:step_now()->par;int origin=side?g.jobs:step_now()->seed;int bx=(int)g.x/320,by=(int)g.y/320;
 // Each waypoint is a real hub. Route order is stable and shown on the map.
 int near[6]={13,22,4,21,17,1};for(int i=0;i<6;i++)g.route[i]=near[(i+origin)%6];(void)bx;(void)by;
 notice("RUTA INICIADA: cruza los seis aros amarillos en carro.");}
static void interact(void){
 if(g.inMetro){if(g.metroWait>0){int bz=cm_station_near(g.metroZ,2);if(bz>=0){g.inMetro=0;g.x=CM_METRO_X-14;g.y=cm_station_z(bz);g.lift=CM_PLAT_H;g.a=PI;notice("Bajaste del Metro. Escalera al sur del anden.");}}return;}
 if(metro_boardable()){g.inMetro=1;notice("METRO: viaje en marcha. R+[] para bajar en la siguiente estacion.");return;}
 if(g.lift>15){if(g.metroWait<=0||cm_station_near(g.metroZ,2)!=(int)floorf(g.y/320))notice("Espera el Metro en el anden: para 6 segundos en cada estacion.");return;} /* en el anden no se toman carros de la calle */
 if(g.mission<36&&!g.side){const Step *s=step_now();const Location *l=&locations[s->loc];if(near_hub(s->loc,58)){
  if(s->kind==K_DRIVE){if(g.car<0){notice("Este objetivo requiere llegar en un carro.");return;}advance();return;}
  if(s->kind==K_RACE){if(g.car<0){notice("Consigue un carro antes de iniciar el recorrido.");return;}if(g.raceTime<=0)race_start(0);return;}
  if(s->kind==K_CHASE){if(g.missionTimer<=0){g.heat=3;g.missionTimer=.01f;for(int i=60;i<64;i++){Car *c=&g.cars[i];c->x=clampf(g.x+(i%2?340:-340),42,WORLD_W-278);c->y=floorf(g.y/320)*320+42;c->a=i%2?PI:0;c->parked=0;}notice("Sobrevive y alejate de las patrullas hasta perderlas.");}return;}
  if(s->kind==K_CHOICE){g.screen=CHOICE;g.menu=0;return;}
  if(s->kind==K_ENDING||s->kind==K_TALK){dialog(missions[g.mission].who,s->text,1);return;}
  if(g.car>=0){notice("Baja del carro con TRIANGULO para interactuar.");return;}
  puzzle_start(s->kind,s->seed);return;
 }}
 // Optional collections and services stay available throughout the story.
 for(int i=0;i<24;i++){float x=locations[i].x+15,y=locations[i].y+38;if(!(g.caches&(1u<<i))&&dist(g.x,g.y,x,y)<25){g.caches|=1u<<i;g.cash+=60;g.reputation++;notice("VINILO ENCONTRADO +$60. Coleccion registrada.");game_save();return;}}
 if(near_hub(1,65)){int cost=g.car>=0?100:30;if(g.cash>=cost){g.cash-=cost;g.health=100;if(g.car>=0)g.cars[g.car].hp=100;g.heat=0;notice("Luna: listo. Motor reparado y carro repintado.");game_save();}else notice("Necesitas $100 para reparar el carro, o $30 a pie.");return;}
 if(near_hub(0,65)){g.health=100;g.heat=0;notice(game_save()?"REFUGIO: salud recuperada y partida guardada.":"No se pudo guardar. Revisa el espacio de la Memory Stick.");return;}
 if(near_hub(12,65)){if(g.car>=0&&!g.side&&!g.raceTime){race_start(1);return;}notice("ENCARGOS: llega en carro para hacer seis entregas ($180).");return;}
 notice("Acercate al objetivo amarillo, un vinilo o un servicio.");
}

static void enter_exit(void){
 if(g.car>=0){Car *c=&g.cars[g.car];if(fabsf(c->speed)>45){notice("Frena antes de bajar del carro.");return;}float xx=c->x+cosf(c->a+PI*.5f)*23,yy=c->y+sinf(c->a+PI*.5f)*23;if(!free_at(xx,yy,5)){xx=c->x-cosf(c->a+PI*.5f)*23;yy=c->y-sinf(c->a+PI*.5f)*23;}if(!free_at(xx,yy,5)){notice("No hay espacio para bajar. Mueve el carro.");return;}g.x=xx;g.y=yy;c->speed=0;c->parked=1;g.car=-1;return;}
 int best=-1;float d=43;for(int i=0;i<CAR_COUNT;i++){float dd=dist(g.x,g.y,g.cars[i].x,g.cars[i].y);if(dd<d&&g.cars[i].hp>0){best=i;d=dd;}}
 if(best>=0){g.car=best;g.steerSmooth=0;g.moveActive=0;g.x=g.cars[best].x;g.y=g.cars[best].y;g.cars[best].parked=0;
  if(best!=1){wanted_crime(g.cars[best].police?4:3);notice("CARRO TOMADO. X acelera / [] frena / L-R radio.");}else notice("Luna: cuidalo. X acelera / [] frena / L-R radio.");
 }else notice("Acercate a un carro. TRIANGULO para tomarlo.");
}
static void foot_pace(int moving,float dt){
 g.tapAge+=dt;g.sprintTime=fmaxf(0,g.sprintTime-dt);
 if(!moving){g.moveGap+=dt;g.footSpeed*=expf(-dt*4); /* v2.25: pasar la palanca por el centro no frena en seco */if(g.moveGap>.38f){g.runTaps=0;g.tapAge=10;g.sprintTime=0;}return;}
 g.moveGap=0;
 if(pressed(B_CROSS)){
  /* Once running, an irregular tap renews the sprint directly. Previously
     a single missed interval forced three new presses and repeated slowdown. */
  if(g.sprintTime>0)g.runTaps=3;
  else if(g.tapAge<=.60f)g.runTaps++;else g.runTaps=1;
  g.runTaps=g.runTaps>3?3:g.runTaps;g.tapAge=0;
  if(g.runTaps>=3&&!g.exhausted&&g.stamina>0)g.sprintTime=1.55f;
 }
 if(g.tapAge>.60f&&g.sprintTime<=0)g.runTaps=0;
 float wanted=g.exhausted?74.f:g.sprintTime>0?105.f:(held(B_CROSS)||(g.runTaps>=2&&g.tapAge<.6f))?74.f:42.f;
 if(g.footSpeed<24)g.footSpeed=24; /* no step-frame stall after a brief stick release */
 g.footSpeed+=(wanted-g.footSpeed)*(1-expf(-dt*7));
}
static void stamina_tick(float dt){
 if(g.car<0&&!g.inMetro&&g.footTravel>.01f&&g.sprintTime>0&&!g.exhausted){
  g.stamina=fmaxf(0,g.stamina-dt*(100.f/6));
  if(g.stamina<=0){g.exhausted=1;g.sprintTime=0;g.runTaps=0;notice("Cansado: trota mientras recuperas la estamina.");}
 }else{
  g.stamina=fminf(100,g.stamina+dt*(g.footTravel>.01f?12.5f:18.f));
  if(g.exhausted&&g.stamina>=100){g.exhausted=0;g.runTaps=0;g.tapAge=10;notice("Estamina recuperada. Pulsa X repetidamente para correr.");}
 }
}
static int traffic_blocked(const Car *c){
 if(g.lift>8||g.inMetro)return 0;
 float cx,cz,px,pz;physics_project(c->x,c->y,&cx,&cz);physics_project(g.x,g.y,&px,&pz);
 float a=physics_heading(c),dx=px-cx,dz=pz-cz;
 float forward=dx*cosf(a)+dz*sinf(a),side=-dx*sinf(a)+dz*cosf(a);
 return forward>4&&forward<(g.car<0?62:78)&&fabsf(side)<(g.car<0?13:21);
}
static void request_horn(int i){
 float cx,cz,px,pz;physics_project(g.cars[i].x,g.cars[i].y,&cx,&cz);physics_project(g.x,g.y,&px,&pz);
 float d=hypotf(cx-px,cz-pz);
 hornGain=(int)(220*clampf(1-d/240,.1f,1));
 hornPan=(int)(100*clampf((-(cx-px)*sinf(g.viewYaw)+(cz-pz)*cosf(g.viewYaw))/fmaxf(d,1),-1,1));
 hornEvent++;
}
static void city_audio_update(void){
 float traffic=0,crowd=0;
 for(int i=0;i<CAR_COUNT;i++)if(!g.cars[i].parked)traffic+=clampf(1-dist(g.x,g.y,g.cars[i].x,g.cars[i].y)/180,0,1)*(.3f+fabsf(g.cars[i].speed)/200);
 for(int i=0;i<42;i++)crowd+=clampf(1-dist(g.x,g.y,g.peds[i].x,g.peds[i].y)/100,0,1);
 trafficGain=(int)(clampf(traffic,0,2)*90);crowdGain=(int)(clampf(crowd,0,3)*20);
}
/* Fixed lane corridors avoid impossible turns through merged blocks and the
   small roundabouts. Signals alternate every four seconds at every junction. */
static int traffic_red(const Car *c,float *stopDistance){
 int horizontal=fabsf(cosf(c->a))>.7f;
 float pos=horizontal?c->x:c->y,dir=horizontal?cosf(c->a):sinf(c->a);
 float n=(pos-42)/320.f;
 int next=dir>0?(int)floorf(n)+1:(int)ceilf(n)-1;
 float junction=next*320.f+42.f,dist=(junction-pos)*dir;
 int bx=horizontal?next:(int)floorf(c->x/320),bz=horizontal?(int)floorf(c->y/320):next;
 *stopDistance=dist-29.f;
 if(bx<0||bx>7||bz<0||bz>6||dist<29||dist>110)return 0;
 int horizontalGreen=(((int)(g.clock/4)+bx+bz)&1)==0;
 return horizontal?!horizontalGreen:horizontalGreen;
}
static int traffic_ahead(int index,float distance){
 const Car *c=&g.cars[index];float a=physics_heading(c),gx,gz;physics_project(c->x,c->y,&gx,&gz);
 for(int j=0;j<CAR_COUNT;j++)if(j!=index){float x,z;physics_project(g.cars[j].x,g.cars[j].y,&x,&z);
  float dx=x-gx,dz=z-gz,forward=dx*cosf(a)+dz*sinf(a),side=-dx*sinf(a)+dz*cosf(a);
  /* v2.34 (Claude): solo frenan los coches que van en el mismo sentido (o aparcados). Los del carril
     contrario, que en la proyeccion del valle quedan muy cerca, se veian "delante" y los dos se
     esperaban para siempre (atasco frente a frente). */
  if(!g.cars[j].parked&&j!=g.car&&cosf(angle_delta(physics_heading(&g.cars[j]),a))<.3f)continue;
  /* v2.34: ademas, en el mapa, a menos de 12 del carril (los carriles distan 28). Cerca del borde
     oeste la proyeccion junta cosas que en el mapa estan lejos (coches aparcados, otros carriles). */
  {float lx=g.cars[j].x-c->x,ly=g.cars[j].y-c->y;if(fabsf(-lx*sinf(c->a)+ly*cosf(c->a))>12)continue;}
  if(forward>8&&forward<distance&&fabsf(side)<19)return 1;
 }
 return 0;
}
/* v2.34 (Claude): un coche civil atascado contra algo (sobre todo en las franjas donde el valle
   deforma las calles) reaparece en otro carril libre de la ciudad, lejos del jugador y fuera de su
   vista. Antes se quedaban parados para siempre (31 atascos en 15 minutos de simulacion). */
/* En pantalla: delante de la camara (±70 grados) y a menos de 450. */
static int in_view(float x,float y){
 float px,pz,cx,cz;physics_project(g.x,g.y,&px,&pz);physics_project(x,y,&cx,&cz);
 float dx=cx-px,dz=cz-pz,d=hypotf(dx,dz);if(d>450)return 0;if(d<30)return 1;
 return fabsf(angle_delta(atan2f(dz,dx),g.viewYaw))<1.22f;
}
static int respawn_civilian(int i){
 Car *c=&g.cars[i];
 for(int tries=0;tries<24;tries++){
  Car t=*c;int east=random_u()&1,vertical=random_u()&1;
  if(vertical){int col=random_u()%8;t.a=east?PI*.5f:-PI*.5f;t.x=col*320+42+(east?-14:14);t.y=150+(random_u()%1900);}
  else{int row=random_u()%7;t.a=east?0:PI;t.x=150+(random_u()%2260);t.y=row*320+42+(east?14:-14);}
  if(dist(t.x,t.y,g.x,g.y)<420||in_view(t.x,t.y))continue;
  if(!car_free_at(&t,t.x,t.y))continue;
  int crowded=0;for(int j=0;j<CAR_COUNT;j++)if(j!=i&&dist(g.cars[j].x,g.cars[j].y,t.x,t.y)<70)crowded=1;
  if(crowded)continue;
  c->x=t.x;c->y=t.y;c->a=t.a;c->speed=0;g.wallTime[i]=0;return 1;
 }
 return 0;
}
static void civilian_traffic_tick(int i,float dt){
 Car *c=&g.cars[i];float stopDist=0;int red=traffic_red(c,&stopDist);
 int blocked=traffic_blocked(c)||traffic_ahead(i,52+fmaxf(0,c->speed)*.18f);
 float target=(red||blocked)?0:64+(i%4)*6;
 c->speed+=clampf(target-c->speed,-210*dt,85*dt);
 if(c->speed<.05f)c->speed=0;
 float distance=c->speed*dt;
 if(red&&stopDist>=0&&distance>stopDist){distance=stopDist;c->speed=0;}
 float xx=c->x+cosf(c->a)*distance,yy=c->y+sinf(c->a)*distance;
 /* v2.39 (Claude): un choque puede desplazarlo de lado; vuelve poco a poco al centro de su carril
    (42+-14 en la rejilla). Antes seguia recto desplazado y acababa en un parque. */
 if(c->speed>1){int horizontal=fabsf(cosf(c->a))>.7f;float cur=horizontal?yy:xx;
  float lane=roundf((cur-42)/320.f)*320.f+42.f;float want=lane+(horizontal?(cosf(c->a)>0?14.f:-14.f):(sinf(c->a)>0?-14.f:14.f)); /* v2.41: carril segun el sentido (como respawn_civilian), no segun el lado en que quedo */
  float step=clampf(want-cur,-25*dt,25*dt);if(horizontal)yy+=step;else xx+=step;}
 if(car_free_at(c,xx,yy)){c->x=xx;c->y=yy;g.wallTime[i]=0;}
 else{c->speed=0;if(target>0){g.wallTime[i]+=dt;if(g.wallTime[i]>3&&(dist(c->x,c->y,g.x,g.y)>250||!in_view(c->x,c->y)))respawn_civilian(i);}}
 /* Loop at the outside edge only when the entry lane is clear. */
 /* v2.34: el morro (18) llega al borde antes que el centro: se da la vuelta a 32 del borde,
    mirando solo el eje por el que circula (los carriles junto al borde no deben reaparecer). */
 int horiz=fabsf(cosf(c->a))>.7f;
 if(horiz?(c->x<32||c->x>2528):(c->y<32||c->y>2208)){
  Car next=*c;if(cosf(c->a)>.7f)next.x=70;else if(cosf(c->a)<-.7f)next.x=2490;
  else if(sinf(c->a)>.7f)next.y=70;else next.y=2170;
  if(car_free_at(&next,next.x,next.y)&&!traffic_ahead(i,55)){c->x=next.x;c->y=next.y;c->speed=0;}
 }
}
#include "combat.inc"
static void combat_init_peds(void){combat_reset();combat_walls_init();}
static void world_tick(float ax,float ay,float dt){
 g.footTravel=0;
 if(pressed(B_SELECT)){g.zoom=(g.zoom+1)&3;notice(g.zoom==0?"Camara: lejana":g.zoom==1?"Camara: normal":g.zoom==2?"Camara: cercana":"Camara: muy cercana");} /* v2.6.2: zoom; el mapa esta en PAUSA > MAPA */
 if(pressed(B_TRI))enter_exit();
#ifdef NARCADE_3D
 if(g.car>=0)g.stickActive=0;
 if(g.car>=0){if(pressed(B_L))g.station=wrapi(g.station-1,5);if(pressed(B_R))g.station=(g.station+1)%5;}
#else
 if(pressed(B_L))g.station=wrapi(g.station-1,5);if(pressed(B_R))g.station=(g.station+1)%5;
#endif
 combat_tick(ax,ay,dt);
 if(pressed(B_CIRCLE)&&!combat.aiming){g.screen=JOURNAL;g.journalPage=0;return;}
 if(g.car<0&&!g.inMetro&&!combat.aiming&&combat.vault<=0){
  /* v2.25 (Claude): controlador a pie reescrito desde cero. Todo se calcula en
     el espacio que se ve (proyectado), y se pasa al mapa con geo_unproject, que
     es la inversa exacta: sin sondas ni factores de escala que den tirones al
     cruzar las calles este-oeste.
     Marco de entrada: la palanca se interpreta respecto a g.frameYaw. Con la
     palanca quieta el marco queda fijo, asi la camara puede colocarse detras
     sin torcer el rumbo (sin circulos en diagonal). Cuando el jugador mueve la
     palanca, el marco se acerca a la camara en proporcion a cuanto la movio:
     cada cambio de direccion es relativo a lo que se ve en pantalla. */
  /* v2.29 (Claude): mediana de las 3 ultimas lecturas del analogico por eje. Una
     lectura suelta disparada (pico de un fotograma, habitual en las palancas de
     PSP) ya no mueve al personaje; los cambios reales pasan con 1 fotograma de
     retraso. La cruceta no pasa por aqui. */
  /* v2.31 (Claude): calibracion automatica del centro. La grabacion de la consola del
     jugador mostro la palanca en reposo en (121,115) y no en (128,128): una desviacion
     de 0,11, casi el umbral de parada. Mientras no se empuja, el centro se aprende
     poco a poco y se descuenta. */
  if(!g.stickActive&&hypotf(ax-g.calX,ay-g.calY)<.22f){float k=1-expf(-dt*1.5f);g.calX+=(ax-g.calX)*k;g.calY+=(ay-g.calY)*k;}
  ax-=g.calX;ay-=g.calY;
  {float mx=ax,my=ay;
   #define MED3(a,b,c) fmaxf(fminf(a,b),fminf(fmaxf(a,b),c))
   ax=MED3(mx,g.stickHX[0],g.stickHX[1]);ay=MED3(my,g.stickHY[0],g.stickHY[1]);
   #undef MED3
   g.stickHX[1]=g.stickHX[0];g.stickHX[0]=mx;g.stickHY[1]=g.stickHY[0];g.stickHY[0]=my;}
  float sx=ax+(held(B_RIGHT)-held(B_LEFT)),sy=ay+(held(B_DOWN)-held(B_UP));
  float raw=hypotf(sx,sy);
  /* v2.29 (Claude): margen amplio para empezar (0,20) y dejar de andar (0,12). Antes
     la zona muerta (0,18) y el umbral (0,22) dejaban una franja estrecha: empujando
     suave el personaje arrancaba y paraba sin parar (177 veces en la prueba). */
  if(raw>.20f)g.stickActive=1;else if(raw<.12f)g.stickActive=0;
  /* v2.28 (Claude): modelo pedido por el jugador: la direccion es SIEMPRE
     "camara + palanca" y la camara sigue al movimiento (tambien hacia los lados),
     asi que al mantener la palanca a un lado el personaje y la camara van girando
     juntos. Solo al caminar hacia la camara (palanca atras) la camara no gira.
     La palanca decide la direccion; la velocidad es completa como con la cruceta:
     la presion variable del pulgar hacia acelerar y frenar a golpes (medido: 3-7
     veces mas tirones de velocidad que con la cruceta). */
  float mag=0;
  if(g.stickActive){
   /* v2.29 (Claude): filtro CONTINUO sobre la posicion de la palanca (vector), no
      sobre el angulo. El filtro anterior tenia dos velocidades con un corte brusco:
      al girar la palanca poco a poco saltaba de una a otra y el giro iba a tirones.
      Este filtro es mas rapido cuanto mas se mueve la palanca (giros al instante) y
      promedia el temblor cuando esta quieta; y cerca del centro, donde el angulo es
      poco fiable, pesa menos cada lectura. */
   if(!g.moveActive){g.stickFX=sx;g.stickFY=sy;}
   else{
    float ex=sx-g.stickFX,ey=sy-g.stickFY,e=hypotf(ex,ey);
    float k=(10.f+70.f*e)*clampf(raw/.45f,.35f,1.f);
    float a=1-expf(-dt*k);g.stickFX+=ex*a;g.stickFY+=ey*a;
    /* un giro brusco de un lado a otro pasa por el centro: seguir a la palanca */
    if(hypotf(g.stickFX,g.stickFY)<.15f){g.stickFX=sx;g.stickFY=sy;}
   }
   g.stickAngle=atan2f(g.stickFX,-g.stickFY);
   mag=1;g.moveActive=1;
  }else g.moveActive=0;
  g.frameYaw=g.viewYaw;g.wallGuide=0;g.stickAnchor=g.stickAngle;
  float dirP=g.viewYaw+g.stickAngle;
  g.walking=g.moveActive;foot_pace(g.walking,dt);
  float bx,bz;physics_project(g.x,g.y,&bx,&bz);
  if(g.moveActive){
   g.inputYaw=g.moveYaw=logical_heading_from_projected(g.x,g.y,dirP);
   float speed=g.footSpeed*mag;
   {float lx,lz;physics_unproject(bx+cosf(dirP)*4,bz+sinf(dirP)*4,&lx,&lz);
    float grade=(geo_height(lx,lz)-geo_height(g.x,g.y))/4;speed/=sqrtf(1+fminf(grade*grade,.35f));}
   /* Barrido en pasos cortos. Si el paso choca, se prueban desviaciones de
      13 a 77 grados a ambos lados con longitud * cos: deslizamiento suave y
      proporcional a lo que se empuja contra la pared, nunca hacia atras. */
   float dist=speed*dt,px=bx,pz=bz,slideOff=0;
   int steps=(int)ceilf(dist/3.f);if(steps<1)steps=1;float seg=dist/steps;
   for(int s=0;s<steps;s++){
    int moved=0;
#define FOOT_TRY(OFF,NX,NZ) (physics_unproject(px+cosf(dirP+(OFF))*seg*cosf(OFF),pz+sinf(dirP+(OFF))*seg*cosf(OFF),&(NX),&(NZ)),foot_free((NX),(NZ))&&lift_ok((NX),(NZ)))
    for(int k=0;k<=6&&!moved;k++)for(int sd=-1;sd<=1&&!moved;sd+=2){
     if(!k&&sd<0)continue;
     float off=sd*k*(PI/14.f),nx,nz;
     if(FOOT_TRY(off,nx,nz)){
      /* v2.26 (Claude): afinar el angulo de deslizamiento por biseccion entre el
         ultimo bloqueado y el primero libre. Con saltos de 13 grados la velocidad
         junto a una pared cambiaba a golpes (cos 13, 26, 39...): eso eran tirones. */
      if(k>0){float lo=sd*(k-1)*(PI/14.f),hi=off;
       for(int it=0;it<5;it++){float mid=(lo+hi)*.5f,mx,mz;if(FOOT_TRY(mid,mx,mz)){hi=mid;nx=mx;nz=mz;}else lo=mid;}
       off=hi;}
      g.x=nx;g.y=nz;physics_project(g.x,g.y,&px,&pz);moved=1;slideOff=off;
     }
    }
#undef FOOT_TRY
    if(!moved)break;
   }
   (void)slideOff; /* v2.28: la camara sigue al deslizamiento; ya no hace falta guia */
   g.lift=cm_lift(g.x,g.y,g.lift>15);
  }
  float ax2,az2;physics_project(g.x,g.y,&ax2,&az2);
  float mvx=ax2-bx,mvz=az2-bz;g.footTravel=hypotf(mvx,mvz);
  /* Cuerpo: mira hacia donde realmente avanza (o a donde se pide si esta
     parado contra algo), girando rapido pero sin saltos de un fotograma. */
  if(g.moveActive){
   float target=g.footTravel>.05f?atan2f(mvz,mvx):dirP;
   g.bodyYaw+=clampf(angle_delta(target,g.bodyYaw)*(1-expf(-dt*18)),-14*dt,14*dt);
   g.a=logical_heading_from_projected(g.x,g.y,g.bodyYaw);
  }else g.bodyYaw=proj_heading(g.x,g.y,g.a);
#ifdef NARCADE_3D
  /* La camara se coloca detras solo al avanzar; de lado o hacia atras no gira. */
  if(g.moveActive&&g.footTravel>.05f){
   /* delante y a los lados la camara sigue del todo; hacia atras deja de girar */
   float fw=clampf(cosf(g.stickAngle)*1.3f+1.f,0,1);
   if(fw>0){g.followScale=fw;camera_follow(g.bodyYaw,dt);g.followScale=1;}
  }
#endif
 }else if(g.car>=0){ /* v2.6: a bordo del Metro no hay coche ni paseo */
  foot_pace(0,dt);
  Car *c=&g.cars[g.car];float oldAngle=c->a,steer=clampf(ax+held(B_RIGHT)-held(B_LEFT),-1,1);
  if(held(B_CROSS))c->speed+=130*dt;else if(held(B_SQUARE))c->speed-=190*dt;else c->speed*=powf(.44f,dt);
  c->speed=clampf(c->speed,-72,220+(c->type==4?32:0));if(c->hp<25)c->speed=clampf(c->speed,-50,120);
  /* Centre the wheel as soon as the stick is released: lingering steering
     made the car keep turning after a direction change. */
  g.steerSmooth=steer;
  /* Steering belongs solely to the player; traffic routes never set this car's heading. */
  float grip=clampf((fabsf(c->speed)+8)/24,0,1);
  /* v2.30 (Claude): volante mas sensible y mas dependiente de la velocidad: a 60
     gira ~30% mas que antes, a 120 ~13% mas y a tope casi igual (cuanto mas rapido,
     mas cuesta girar). Parado no gira (grip). */
  float turn=g.steerSmooth*dt*(4.0f/(1.f+fabsf(c->speed)/130.f))*(c->speed<0?-1:1)*grip;
  /* v2.25 (Claude): el giro se aplica en pantalla (espacio proyectado): la misma
     presion del volante gira lo mismo en cualquier zona del valle. En espacio
     del mapa, las zonas deformadas giraban mucho mas o mucho menos. */
  /* Rumbo en pantalla guardado aparte: solo cambia cuando el jugador gira (sin
     deriva por convertir ida y vuelta entre mapa y pantalla). */
  if(g.carYawOwner!=g.car+1){g.carScreenYaw=proj_heading(c->x,c->y,c->a);g.carYawOwner=g.car+1;}
  float h0=g.carScreenYaw,applied=1;
#define CAR_TURNED(px,py,f) (turn==0?oldAngle:logical_heading_from_projected((px),(py),h0+turn*(f)))
  c->a=CAR_TURNED(c->x,c->y,1);
  if(!car_free_at(c,c->x,c->y)){
   /* Near a facade, keep the largest steering angle that still fits.
      Reverting the whole turn made the wheel appear unresponsive. */
   float lo=0,hi=1;
   for(int k=0;k<5;k++){float mid=(lo+hi)*.5f;c->a=CAR_TURNED(c->x,c->y,mid);if(car_free_at(c,c->x,c->y))lo=mid;else hi=mid;}
   c->a=lo>0?CAR_TURNED(c->x,c->y,lo):oldAngle;applied=lo;
   /* v2.24 (Claude): pegado a una pared el giro quedaba bloqueado del todo.
      Si no cabe ninguna fraccion, se prueba el giro completo separando el
      coche 1-3 unidades de la pared (nunca atraviesa edificios). */
   if(lo<.05f&&fabsf(turn)>1e-4f){
    int done=0;
    for(float r=1.f;r<=3.f&&!done;r+=1.f)for(int k=0;k<8&&!done;k++){
     float oa=k*(PI*.25f),ox=c->x+cosf(oa)*r,oy=c->y+sinf(oa)*r;
     c->a=CAR_TURNED(ox,oy,1);if(car_free_at(c,ox,oy)){c->x=ox;c->y=oy;done=1;applied=1;}
    }
    if(!done){c->a=lo>0?CAR_TURNED(c->x,c->y,lo):oldAngle;applied=lo;}
   }
  }
  /* v2.27 (Claude): el coche del jugador avanza recto EN PANTALLA. Antes avanzaba con
     rumbo fijo en el mapa, y como el valle deforma el mapa (las calles norte-sur se
     desplazan cientos de unidades de un cruce a otro), al llegar a cada cruce el coche
     giraba solo siguiendo la calle: parecia una regla de carril. Ahora solo gira
     cuando el jugador gira. Al rozar una pared se desliza sin cambiar la direccion
     del morro. */
  g.carScreenYaw=h0+turn*applied;
  float distance=c->speed*dt,hp=g.carScreenYaw;
  int steps=(int)ceilf(fabsf(distance)/3.f);if(steps<1)steps=1;int scraped=0;
  for(int step=0;step<steps;step++){
   float seg=distance/steps,px,pz;physics_project(c->x,c->y,&px,&pz);
#define CAR_TRY(OFF,T) (physics_unproject(px+cosf(hp+(OFF))*seg*cosf(OFF),pz+sinf(hp+(OFF))*seg*cosf(OFF),&(T).x,&(T).y),   (T).a=logical_heading_from_projected((T).x,(T).y,hp),car_free_at(&(T),(T).x,(T).y))
   Car t=*c;int slid=-1;
   if(CAR_TRY(0,t))slid=0;
   else for(int k=1;k<=4&&slid<0;k++)for(int sd=-1;sd<=1&&slid<0;sd+=2){
    float off=sd*k*(PI/12.f);
    if(CAR_TRY(off,t)){
     /* angulo minimo libre por biseccion: deslizamiento continuo, sin golpes */
     float lo=sd*(k-1)*(PI/12.f),hi=off;Car m=*c;
     for(int it=0;it<5;it++){float mid=(lo+hi)*.5f;if(CAR_TRY(mid,m)){hi=mid;t=m;}else lo=mid;}
     slid=k;
     if(!scraped){float keep=cosf(hi);c->speed*=.35f+.65f*keep;if(k>=3)c->hp-=fabsf(c->speed)*.004f;scraped=1;}
    }
   }
#undef CAR_TRY
   if(slid>=0){c->x=t.x;c->y=t.y;c->a=t.a;if(slid>0)distance=c->speed*dt;continue;}
   {float impact=fabsf(c->speed);c->hp-=impact*.025f;c->speed=impact>65?-c->speed*.13f:0;g.hitCD=.12f;break;}
  }
#undef CAR_TURNED
  g.x=c->x;g.y=c->y;
  if(c->hp<=0){c->hp=20;c->speed=0;c->parked=1;g.car=-1;g.health-=25;
   /* v2.34 (Claude): bajar por un lado libre, como con TRIANGULO. Antes el jugador quedaba en el
      centro del coche y el coche, sin aparcar, se iba solo como si fuera trafico. */
   float xx=c->x+cosf(c->a+PI*.5f)*23,yy=c->y+sinf(c->a+PI*.5f)*23;
   if(!free_at(xx,yy,5)){xx=c->x-cosf(c->a+PI*.5f)*23;yy=c->y-sinf(c->a+PI*.5f)*23;}
   if(!free_at(xx,yy,5)){xx=c->x+cosf(c->a)*30;yy=c->y+sinf(c->a)*30;}
   if(!free_at(xx,yy,5)){xx=c->x-cosf(c->a)*30;yy=c->y-sinf(c->a)*30;}
   g.x=xx;g.y=yy;notice("Motor averiado. Busca otro carro o ve al taller.");}
 }
 for(int i=0;i<CAR_COUNT;i++){
  g.hornCooldown[i]=fmaxf(0,g.hornCooldown[i]-dt);
  if(i==g.car)continue;Car *c=&g.cars[i];if(c->parked&&!c->police)continue;
  if(traffic_blocked(c)){
   g.blockedTime[i]+=dt;c->speed=0;
   if(g.blockedTime[i]>.65f&&g.hornCooldown[i]<=0){request_horn(i);g.hornCooldown[i]=7+(i%5);}
   continue;
  }
  g.blockedTime[i]=0;
  if(g.trafficYield[i]>0){g.trafficYield[i]=fmaxf(0,g.trafficYield[i]-dt);c->speed=0;continue;}
  /* v2.34 (Claude): sin persecucion, las patrullas circulan como el trafico normal (carriles y
     semaforos). Antes iban rectas y giraban al azar: acababan fuera del mapa o dando vueltas contra
     una pared para siempre. Al acabar una persecucion se alinean con el eje mas cercano. */
  if(!c->police||wanted_stars()<1||i-60>wanted_stars()){if(c->police){c->a=roundf(c->a/(PI*.5f))*(PI*.5f);c->a=fmodf(c->a,2*PI);}civilian_traffic_tick(i,dt);continue;}
  if(c->police&&wanted_stars()>=1){
   float dd=dist(g.x,g.y,c->x,c->y);
   // Pursuers use the street grid, choosing the next junction toward the player.
   float jx=floorf(c->x/320)*320+42,jy=floorf(c->y/320)*320+42;
   if(dist(c->x,c->y,jx,jy)<10){float dx=g.x-c->x,dy=g.y-c->y;
    if(fabsf(dx)>fabsf(dy))c->a=dx>0?0:PI;else c->a=dy>0?PI*.5f:-PI*.5f;
   }
   if(dd<130){float desired=atan2f(g.y-c->y,g.x-c->x);float xx=c->x+cosf(desired)*dt*105,yy=c->y+sinf(desired)*dt*105;if(free_at(xx,yy,9))c->a=desired;}
   c->speed=95+wanted_stars()*12;
   if(dd<25&&g.hitCD<=0){g.health-=g.car<0?12:4;if(g.car>=0)g.cars[g.car].hp-=5;g.hitCD=1;}
  }else{c->speed=c->police?45:48+i%4*9;
   float lx=fmodf(c->x,320),ly=fmodf(c->y,320);
   if(fabsf(lx-42)<2&&fabsf(ly-42)<2&&((int)(g.clock*10)+i)%4==0)c->a+=PI*.5f;
  }
  float xx=c->x+cosf(c->a)*c->speed*dt,yy=c->y+sinf(c->a)*c->speed*dt;
  if(car_free_at(c,xx,yy)){c->x=xx;c->y=yy;}else{c->speed=0;c->a=fmodf(c->a+PI*.5f,2*PI);}
 }
 separate_cars();
 city_audio_update();
 metro_update(dt);
#ifdef NARCADE_3D
 /* v2.25 (Claude): la camara del coche sigue un rumbo suavizado (~0,2 s) y medido
    con sonda corta: las curvas de las calles en los pliegues del valle y los saltos
    de geo_heading (sonda de 4) ya no la hacen bailar. Los giros reales se siguen. */
 if(g.car>=0){
  float h=proj_heading(g.cars[g.car].x,g.cars[g.car].y,g.cars[g.car].a);
  if(!g.carCamInit){g.carCamYaw=h;g.carCamInit=1;}
  else g.carCamYaw+=angle_delta(h,g.carCamYaw)*(1-expf(-dt*5));
  camera_follow(g.carCamYaw,dt);
 }else{g.carCamInit=0;g.carYawOwner=0;}
 if(g.car<0&&!g.walking)g.cameraVelocity=0;
 camera_clearance(dt);
 camera_heights(dt);
#endif
 combat_peds_tick(dt);
 /* v2.37: fuera de la vista de las patrullas las estrellas parpadean; si aguantas 6 s + 5 s por
    estrella sin que te vean, se pierde la busqueda entera (GTA). Delitos leves se olvidan solos. */
 if(wanted_stars()>=1){float nearest=100000;for(int i=60;i<64;i++)if(i!=g.car&&g.cars[i].hp>0)nearest=fminf(nearest,dist(g.x,g.y,g.cars[i].x,g.cars[i].y));if(nearest>210){g.escape+=dt;if(g.escape>6+5*wanted_stars()){trophy_escape(wanted_stars());g.heat=0;wantedKills=0;g.escape=0;}}else g.escape=0;}
 else if(g.heat>0){g.heat=fmaxf(0,g.heat-dt*.02f);if(g.heat<=0)wantedKills=0;}
 if(g.mission<36&&step_now()->kind==K_CHASE&&g.missionTimer>0){g.missionTimer+=dt;if(g.missionTimer>step_now()->par&&g.heat<.01f)advance();}
 if(g.raceTime>0){g.raceTime-=dt;int l=g.route[g.checkpoint];if(g.car>=0&&dist(g.x,g.y,locations[l].x,locations[l].y)<66){g.checkpoint++;g.raceTime+=3;if(g.checkpoint==6){g.raceTime=0;advance();}else notice("PUNTO ALCANZADO +3 segundos. Sigue el siguiente aro.");}
  if(g.raceTime<=0&&g.checkpoint<6){g.raceTime=0;g.side=0;g.checkpoint=0;notice("Tiempo agotado. Vuelve al inicio para repetir el recorrido.");}
 }
 if(g.health<=0){combat.jump=combat.velocity=combat.vault=combat.punch=combat.alert=0;combat.aiming=0;combat.target=-1;g.health=100;g.heat=0;g.car=-1;g.x=locations[14].x;g.y=locations[14].y;g.cash=g.cash>100?g.cash-100:0;g.raceTime=0;g.side=0;g.missionTimer=0;notice("HOSPITAL: te recuperaste. El objetivo sigue disponible.");game_save();}
 if(pressed(B_SQUARE)&&g.car<0){if(held(B_R))interact();else combat_jump();}
 /* Animate distance actually travelled: pushing into a wall no longer runs
    the feet in place. Keep phase continuous through all three gaits. */
 stamina_tick(dt);
 float actualSpeed=g.footTravel/dt;
 g.footFiltered+=(actualSpeed-g.footFiltered)*(1-expf(-dt*7));
 if(g.footTravel<.005f)g.footStall+=dt;else g.footStall=0;
 float motionTarget=g.car<0?clampf(g.footFiltered/42,0,2.1f):0;
 g.motion+=(motionTarget-g.motion)*(1-expf(-dt*9));
 if(g.car<0&&g.footFiltered>.2f&&g.footStall<.11f)
  g.gaitPhase=fmodf(g.gaitPhase+g.footFiltered*dt*(.275f-.055f*clampf(g.motion-1,0,1.1f)),2*PI);
 if(pressed(B_UP)&&g.car>=0)interact();
}

void game_tick(unsigned buttons,float ax,float ay,float dt){
 int oldScreen=g.screen,oldMenu=g.menu,oldTitleStage=g.titleStage,oldTab=g.pauseTab,oldMap=g.mapSel,oldJournal=g.journalPage,oldCursor=g.p.cursor;
 dt=clampf(dt,.001f,.10f); /* v2.31: hasta 0,1 s por paso (a pocos fps el tope de 0,05 ponia el juego en camara lenta); el movimiento va en subpasos */g.pressed=(buttons&~g.prev)|(g.screen==WORLD?pendingCross:0);pendingCross=0;g.held=buttons;g.prev=buttons;g.clock+=dt;g.screenT+=dt;g.noticeT=fmaxf(0,g.noticeT-dt);if(g.screen==WORLD){g.hudDistrictT=fmaxf(0,g.hudDistrictT-dt);g.hudObjectiveT=fmaxf(0,g.hudObjectiveT-dt);}g.hitCD=fmaxf(0,g.hitCD-dt);
 /* v2.24: zona muerta radial. La zona por eje anulaba cada eje por separado y
    "pegaba" la palanca a las 4 direcciones rectas (+-10 grados sin respuesta). */
 {float r=hypotf(ax,ay);
  /* v2.29: a pie el mundo aplica su propio margen (ver world_tick); fuera del mundo
     o en coche se mantiene la zona muerta radial. */
  if(r<.18f&&!(g.screen==WORLD&&g.car<0))ax=ay=0;
  }
 if(g.screen!=WORLD){g.moveActive=0;g.steerSmooth=0;g.stickActive=0;g.runTaps=0;g.tapAge=10;g.sprintTime=0;g.footSpeed=0;g.footFiltered=0;g.footStall=0;}
 if(pressed(B_START)&&g.screen!=TITLE&&g.screen!=PAUSE&&g.screen!=SETTINGS&&g.screen!=CREDITS&&g.screen!=TROPHIES){
  g.weaponWheel=0;g.weaponHold=0;
  g.pauseBack=g.screen;g.pauseTab=0;g.menu=0;g.mapSel=g.mission<36?step_now()->loc:0;g.screen=PAUSE;menu_click();return;
 }
 if(g.screen==TITLE){
  if(!g.titleStage){
   if(pressed(B_CROSS)||pressed(B_START)){g.titleStage=1;g.menu=0;g.screenT=0;}
  }else{
   if(pressed(B_CIRCLE)){g.titleStage=0;g.screenT=0;}
   else{
    /* Five-panel spatial navigation. Existing IDs 0/1/2 stay compatible. */
    {static const int down[5]={3,4,2,3,4},up[5]={0,1,2,0,1},left[5]={0,0,1,3,3},right[5]={1,2,2,4,2};
     if(pressed(B_DOWN))g.menu=down[g.menu];else if(pressed(B_UP))g.menu=up[g.menu];
     else if(pressed(B_LEFT))g.menu=left[g.menu];else if(pressed(B_RIGHT))g.menu=right[g.menu];}
    if(pressed(B_CROSS)||pressed(B_START)){
     if(g.menu==0){if(nativeSave)saveRequest=2;else game_continue();}
     else if(g.menu==2){g.screen=SETTINGS;g.menu=0;menu_click();}
     else if(g.menu==3){g.screen=CREDITS;menu_click();}
     else if(g.menu==4){g.screen=TROPHIES;trophySelection=0;menu_click();}
     else{g.menu=0;dialog("NUEVA HISTORIA","Empezar una historia nueva sustituye el progreso anterior al guardar. X confirma. Pulsa O para volver al inicio.",3);}
    }
   }
  }
 }
 else if(g.screen==SETTINGS){if(pressed(B_CIRCLE)||pressed(B_START)){settings_finish();g.screen=TITLE;g.menu=2;menu_click();}else settings_tick();}
 else if(g.screen==CREDITS||g.screen==TROPHIES){
  if(pressed(B_CIRCLE)||pressed(B_START)){g.menu=g.screen==CREDITS?3:4;g.screen=TITLE;menu_click();}
  else if(g.screen==TROPHIES&&(pressed(B_UP)||pressed(B_DOWN))){trophySelection=wrapi(trophySelection+pressed(B_DOWN)-pressed(B_UP),GAME_TROPHY_COUNT);menu_click();}
 }
 else if(g.screen==DIALOG){if(g.dialogAction==3&&pressed(B_CIRCLE))g.screen=TITLE;else if(pressed(B_CROSS)&&g.screenT>.12f){if(g.dialogAction==3)fresh_game();else end_dialog();}}
 else if(g.screen==WORLD){if(!weapon_menu(ax,ay,dt)){g.playtime+=dt;world_tick(ax,ay,dt);trophies_tick(dt);}}
 else if(g.screen==MINI){g.playtime+=dt;puzzle_tick(ax,ay,dt);}
 else if(g.screen==MAP){if(pressed(B_SELECT)||pressed(B_CIRCLE))g.screen=WORLD;if(pressed(B_LEFT)||pressed(B_UP))g.mapSel=wrapi(g.mapSel-1,30);if(pressed(B_RIGHT)||pressed(B_DOWN))g.mapSel=(g.mapSel+1)%30;}
 else if(g.screen==JOURNAL){if(pressed(B_CIRCLE))g.screen=WORLD;if(pressed(B_R)||pressed(B_RIGHT))g.journalPage=(g.journalPage+1)%3;if(pressed(B_L)||pressed(B_LEFT))g.journalPage=wrapi(g.journalPage-1,3);}
 else if(g.screen==PAUSE){
  if(pressed(B_START)||pressed(B_CIRCLE)){settings_finish();g.screen=g.pauseBack;menu_click();return;}
  if(pressed(B_L)||(g.pauseTab!=3&&pressed(B_LEFT))){settings_finish();g.pauseTab=wrapi(g.pauseTab-1,6);g.menu=0;}
  if(pressed(B_R)||(g.pauseTab!=3&&pressed(B_RIGHT))){settings_finish();g.pauseTab=(g.pauseTab+1)%6;g.menu=0;}
  int delta=pressed(B_DOWN)-pressed(B_UP);
  if(g.pauseTab==0)g.mapSel=wrapi(g.mapSel+delta,30);
  else if(g.pauseTab==1)g.journalPage=wrapi(g.journalPage+delta,3);
  else if(g.pauseTab==2)g.menu=wrapi(g.menu+delta,2);
  else if(g.pauseTab==3)settings_tick();
  else if(g.pauseTab==4)trophySelection=wrapi(trophySelection+delta,GAME_TROPHY_COUNT);
  if(pressed(B_CROSS)){
   if(g.pauseTab==2&&g.menu==0){if(nativeSave&&g.active)saveRequest=1;else notice(game_save()?"Partida guardada correctamente.":"No se pudo guardar. Revisa la Memory Stick.");}
   else if(g.pauseTab==2&&g.menu==1)g.screen=g.pauseBack;
   else if(g.pauseTab==5){if(nativeSave&&g.active)saveRequest=3;else if(game_save()){g.screen=TITLE;g.menu=0;}else notice("No se pudo guardar. Sigues en la partida.");}
  }
 }else if(g.screen==CHOICE){if(pressed(B_UP)||pressed(B_DOWN))g.menu=1-g.menu;if(pressed(B_CROSS)){g.ending=g.menu+1;advance();}}
 if((oldScreen==TITLE&&g.screen==TITLE&&(oldMenu!=g.menu||oldTitleStage!=g.titleStage))||
    (oldScreen==PAUSE&&g.screen==PAUSE&&(oldMenu!=g.menu||oldTab!=g.pauseTab||oldMap!=g.mapSel||oldJournal!=g.journalPage))||
    (oldScreen==MAP&&oldMap!=g.mapSel)||(oldScreen==JOURNAL&&oldJournal!=g.journalPage)||
    (oldScreen==MINI&&oldCursor!=g.p.cursor)||(oldScreen==CHOICE&&oldMenu!=g.menu))menu_click();
 int rhythm=(g.screen==MINI&&g.p.kind==K_RHYTHM);audioStation=rhythm?3:g.station;audioEnabled=(g.car>=0&&g.station<4&&g.screen!=TITLE&&g.screen!=CREDITS&&g.screen!=TROPHIES)||rhythm;
 audioAmbient=g.screen==WORLD;
 if(rhythm)audioPos=(unsigned)(g.p.t*44100)%(track_length[3]*2);
}

static void car_draw(const Car *c,float ox,float oy,int chosen){
 int x=(int)(c->x-ox),y=(int)(c->y-oy);if(x<-32||y<-32||x>512||y>300)return;
 float ca=cosf(c->a),sa=sinf(c->a);uint32_t color=c->police?RGB(41,71,87):carcolors[c->type];
 for(int yy=-21;yy<=21;yy++)for(int xx=-21;xx<=21;xx++){float u=xx*ca+yy*sa,v=-xx*sa+yy*ca;if(fabsf(u)<19&&fabsf(v)<10)px(x+xx+2,y+yy+3,RGB(13,23,27));}
 for(int yy=-21;yy<=21;yy++)for(int xx=-21;xx<=21;xx++){float u=xx*ca+yy*sa,v=-xx*sa+yy*ca;uint32_t col=color;int draw=0;
  if(fabsf(u)<18&&fabsf(v)<9){draw=1;if((u>5&&u<10)||(u<-6&&u>-11))col=RGB(21,48,62);if(fabsf(v)>7&&fabsf(u)>11)col=RGB(19,24,27);if(u>15&&fabsf(v)>4)col=GOLD;if(u<-15&&fabsf(v)>4)col=CORAL;}
  if(c->police&&fabsf(u)<2&&fabsf(v)<8){draw=1;col=(v>0)?TEAL:CORAL;}
  if(draw)px(x+xx,y+yy,col);
 }
 if(chosen)circle(x,y,2,LIME);
}
static void person_draw(float wx,float wy,float ox,float oy,int player,int n){int x=wx-ox,y=wy-oy;if(x<-10||y<-10||x>490||y>282)return;circle(x+2,y+4,5,RGB(27,36,37));if(player){rect(x-5,y-9,10,15,INK);}rect(x-3,y-3,6,9,player?TEAL:carcolors[n%6]);rect(x-2,y+5,2,3,INK);rect(x+1,y+5,2,3,INK);circle(x,y-4,3,RGB(208,154,115));if(player)rect(x-2,y-7,5,2,INK);}
static void tree(int x,int y,int n){rect(x-1,y,3,13,RGB(92,74,49));circle(x+3,y,13,RGB(27,59,49));circle(x-3,y-5,11,n%2?RGB(57,99,65):RGB(50,86,61));rect(x-5,y-10,4,2,RGB(105,139,83));}
static void world_draw(void){
 float ox=clampf(g.x-240,0,WORLD_W-W),oy=clampf(g.y-137,0,WORLD_H-H);g.cameraX=ox;g.cameraY=oy;
 rect(0,0,W,H,RGB(46,57,60));
 int bx0=(int)ox/320,by0=(int)oy/320;
 for(int by=by0;by<=by0+2&&by<7;by++)for(int bx=bx0;bx<=bx0+2&&bx<8;bx++){
  int x=bx*320-ox,y=by*320-oy;
  rect(x+86,y+86,234,234,RGB(109,114,103));rect(x+91,y+91,208,203,RGB(81,91,83));
  for(int i=0;i<320;i+=36){rect(x+i,y+42,17,2,RGB(170,164,127));rect(x+42,y+i,2,17,RGB(170,164,127));}
  for(int j=0;j<5;j++){rect(x+9+j*13,y+75,7,4,RGB(189,191,168));rect(x+75,y+9+j*13,4,7,RGB(189,191,168));}
  if(parkblock(bx,by)){
   rect(x+99,y+99,188,180,RGB(57,91,68));rect(x+118,y+173,160,14,RGB(170,159,128));rect(x+185,y+106,13,160,RGB(170,159,128));
   if(bx==0&&by==2){rect(x+121,y+123,138,123,RGB(155,100,78));outline(x+132,y+134,116,101,WHITE);line(x+190,y+134,x+190,y+235,WHITE);circle(x+190,y+185,20,RGB(172,119,83));}
   if(bx==2&&by==1){rect(x+118,y+111,151,152,RGB(66,113,71));outline(x+131,y+122,125,131,WHITE);line(x+131,y+188,x+256,y+188,WHITE);outline(x+159,y+122,68,29,WHITE);outline(x+159,y+224,68,29,WHITE);}
   for(int i=0;i<4;i++)tree(x+112+(i%2)*157,y+115+(i/2)*139,i);
  }else{
   uint32_t roofs[]={RGB(117,109,100),RGB(133,115,96),RGB(96,117,119),RGB(121,124,113),RGB(121,98,96)};
   for(int k=0;k<4;k++){int xx=x+98+(k%2)*96,yy=y+98+(k/2)*91;
    rect(xx+5,yy+7,88,83,RGB(47,55,55));rect(xx,yy,87,81,roofs[(bx+by+k)%5]);rect(xx+3,yy+3,81,74,roofs[(bx+by+k+1)%5]);
    rect(xx+17,yy+18,24,16,RGB(74,87,85));rect(xx+19,yy+16,20,14,RGB(159,167,151));line(xx+20,yy+20,xx+36,yy+20,MUTED);
    for(int w=0;w<5;w++){rect(xx+8+w*15,yy+75,7,4,(w+bx+by)%3==0?GOLD:RGB(59,77,80));}
    rect(xx+63,yy+12,13,23,RGB(71,84,85));line(xx+60,yy+45,xx+75,yy+45,RGB(87,87,76));
   }
   if(bx<2){rect(x+99,y+268,180,9,TEAL);for(int k=0;k<10;k++)rect(x+104+k*17,y+269,7,6,k%2?CORAL:GOLD);}
   tree(x+303,y+120,bx);tree(x+304,y+263,by);
  }
  // Street lamps and warm pool markers give the grid a readable scale.
  rect(x+82,y+59,2,13,INK);rect(x+80,y+56,6,3,GOLD);
 }
 int rx=1396-ox;rect(rx,0,64,H,RGB(34,83,94));for(int yy=-(int)oy%26;yy<H;yy+=26)for(int xx=0;xx<3;xx++)rect(rx+8+xx*19,yy+(xx%2)*8,13,1,RGB(64,115,120));
 for(int by=by0;by<=by0+2;by++){int y=by*320-oy;rect(rx-4,y,72,86,RGB(54,64,64));rect(rx-4,y,72,4,MUTED);rect(rx-4,y+82,72,4,MUTED);for(int x=rx;x<rx+64;x+=24)rect(x,y+42,13,2,GOLD);}
 for(int i=0;i<30;i++){int x=locations[i].x-ox,y=locations[i].y-oy;if(x<-35||x>510||y<-35||y>305)continue;
  if(i==0||i==1||i==12){circle(x,y,11,INK);outline(x-7,y-7,14,14,TEAL);text(x-3,y-5,i==0?"S":i==1?"T":"$",TEAL,1);}
  else{rect(x-7,y-6,14,11,INK);rect(x-5,y-4,10,6,TEAL);rect(x-8,y+6,16,2,MUTED);}
 }
 for(int i=0;i<24;i++)if(!(g.caches&(1u<<i))){int x=locations[i].x+15-ox,y=locations[i].y+38-oy;circle(x,y,6,INK);circle(x,y,3,CORAL);rect(x,y,1,1,GOLD);}
 for(int i=0;i<42;i++)person_draw(g.peds[i].x,g.peds[i].y,ox,oy,0,i);
 for(int i=0;i<CAR_COUNT;i++)car_draw(&g.cars[i],ox,oy,i==g.car);
 if(g.car<0)person_draw(g.x,g.y,ox,oy,1,0);
 int target=-1;if(g.raceTime>0)target=g.route[g.checkpoint];else if(g.mission<36)target=step_now()->loc;
 if(target>=0){int x=locations[target].x-ox,y=locations[target].y-oy;int sx=clampf(x,15,465),sy=clampf(y,68,211);
  if(x>8&&x<472&&y>60&&y<220){int r=17+(int)(sinf(g.clock*4)*3);outline(x-r,y-r,r*2,r*2,LIME);rect(x-1,y-r-8,3,5,LIME);}else{circle(sx,sy,8,LIME);float a=atan2f(y-sy,x-sx);line(sx,sy,sx+cosf(a)*6,sy+sinf(a)*6,INK);}
 }
}
static int cachecount(void){int n=0;for(int i=0;i<24;i++)if(g.caches&(1u<<i))n++;return n;}
int cachecount_public(void){return cachecount();}
/* v2.3 (Claude): HUD minimo. Sin barra superior ni franja inferior permanentes.
   - Esquina superior derecha: barra de vida (y del carro), dinero y busqueda.
   - Arriba al centro: nombre del barrio, solo unos segundos al entrar en uno nuevo.
   - Abajo al centro: la frase del objetivo, solo al empezar cada objetivo (se relee en START > cuaderno/mapa),
     los avisos del juego y la accion contextual al llegar al objetivo.
   - Abajo a la izquierda: minimapa circular con calles, rio, objetivo y posicion. */
static void text_center(int cx,int y,const char *s,uint32_t c,int scale){s=locale_text(s);text_raw(cx-(int)strlen(s)*7*scale/2,y,s,c,scale);}
static void box_center(int cx,int y,int w,int h,uint32_t c){rect(cx-w/2,y,w,h,c);}
/* v2.6: el minimapa se dibuja cada fotograma (3.200 muestras); el trazado se precalcula en una rejilla de 4 unidades
   (640x560 bytes = 358 KB) para no evaluar poligonos por pixel. Codigos: 0 fuera, 1 rio, 2 calle, 3 acera/plaza, 4 parque, 5 edificio. */
#define MG_STEP 4
static unsigned char mapGrid[WORLD_H/MG_STEP][WORLD_W/MG_STEP];static int mapGridBuilt=0;
static void map_grid_build(void){
 if(mapGridBuilt)return;
 for(int j=0;j<WORLD_H/MG_STEP;j++)for(int i=0;i<WORLD_W/MG_STEP;i++){
  float wx=i*MG_STEP+MG_STEP*.5f,wy=j*MG_STEP+MG_STEP*.5f;int ly=(int)wy%320;unsigned char c;
  if(wx>1396&&wx<1460&&ly>86)c=1;
  else{int kind=cm_parcel_at(wx,wy);if(kind<0)c=cm_on_road(wx,wy)?2:3;else c=kind==1?4:5;}
  int h=(int)geo_clamp(geo_height(wx,wy)*.14f,0,40); /* sombreado por altura precalculado (bits 3..7) */
  mapGrid[j][i]=(unsigned char)(c|((h/2)<<3));
 }
 mapGridBuilt=1;
}
static uint32_t map_ground(float gx,float gz){
 float wx,wy;geo_unproject(gx,gz,&wx,&wy);
 if(wx<0||wy<0||wx>=WORLD_W||wy>=WORLD_H)return RGB(20,30,34);
 if(!mapGridBuilt)map_grid_build();
 unsigned char v=mapGrid[(int)wy/MG_STEP][(int)wx/MG_STEP],c=v&7;int h=(v>>3)*2;
 if(c==1)return RGB(39,127,147);if(c==2)return RGB(119,128,123);if(c==3)return RGB(150,152,140);
 return c==4?RGB(57+h,91+h,68):RGB(51+h,67+h,54);
}
static void map_point(float x,float y,float sc,int ox,int oy,int *mx,int *my){float gx,gz;geo_project(x,y,&gx,&gz);*mx=ox+(int)(gx*sc);*my=oy+(int)(gz*sc);}
static void minimap(int cx,int cy,int r){
 const float scale=6.0f; /* unidades de mundo por pixel */
 float gx,gz;geo_project(g.x,g.y,&gx,&gz);
 circle(cx,cy,r+4,RGB(7,14,23));
 for(int dy=-r;dy<=r;dy+=2)for(int dx=-r;dx<=r;dx+=2){ /* v2.6: bloques 2x2 (4x menos muestras; costaba 5 ms) */
  if(dx*dx+dy*dy>r*r)continue;
  uint32_t c=map_ground(gx+dx*scale,gz+dy*scale);
  px(cx+dx,cy+dy,c);
  if((dx+1)*(dx+1)+dy*dy<=r*r)px(cx+dx+1,cy+dy,c);
  if(dx*dx+(dy+1)*(dy+1)<=r*r)px(cx+dx,cy+dy+1,c);
  if((dx+1)*(dx+1)+(dy+1)*(dy+1)<=r*r)px(cx+dx+1,cy+dy+1,c);
 }
 for(int i=0;i<CAR_COUNT;i++){if(!g.cars[i].police||wanted_stars()<1)continue;float pxp,pyp;geo_project(g.cars[i].x,g.cars[i].y,&pxp,&pyp);int dx=(int)((pxp-gx)/scale),dy=(int)((pyp-gz)/scale);if(dx*dx+dy*dy<(r-2)*(r-2))rect(cx+dx-1,cy+dy-1,3,3,CORAL);}
 if(g.mission<36||g.raceTime>0){int t=g.raceTime>0?g.route[g.checkpoint]:step_now()->loc;float pxp,pyp;geo_project(locations[t].x,locations[t].y,&pxp,&pyp);float dx=(pxp-gx)/scale,dy=(pyp-gz)/scale;float d=sqrtf(dx*dx+dy*dy);
  if(d>r-6){dx=dx/d*(r-6);dy=dy/d*(r-6);}circle(cx+(int)dx,cy+(int)dy,4,INK);circle(cx+(int)dx,cy+(int)dy,2,GOLD);}
 /* Crisp pointer: a filled chevron with dark keyline remains readable on roads. */
 float a=geo_heading(g.x,g.y,g.car>=0?g.cars[g.car].a:g.a),co=cosf(a),si=sinf(a);
 int tx=cx+(int)(co*8),ty=cy+(int)(si*8);
 int lx=cx+(int)(-co*5-si*4),ly=cy+(int)(-si*5+co*4);
 int rx=cx+(int)(-co*5+si*4),ry=cy+(int)(-si*5-co*4);
 for(int yy=cy-8;yy<=cy+8;yy++)for(int xx=cx-8;xx<=cx+8;xx++){
  int e0=(xx-tx)*(ly-ty)-(yy-ty)*(lx-tx);
  int e1=(xx-lx)*(ry-ly)-(yy-ly)*(rx-lx);
  int e2=(xx-rx)*(ty-ry)-(yy-ry)*(tx-rx);
  if((e0>=0&&e1>=0&&e2>=0)||(e0<=0&&e1<=0&&e2<=0))px(xx,yy,WHITE);
 }
 line(tx,ty,lx,ly,INK);line(lx,ly,rx,ry,INK);line(rx,ry,tx,ty,INK);
 /* Double bezel and north tick, inspired by the legibility of classic GTA HUDs. */
 for(int k=0;k<128;k++){float t=k*PI*2/128;
  int dx=(int)(cosf(t)*(r+2)),dy=(int)(sinf(t)*(r+2));
  px(cx+dx,cy+dy,k%16<2?RGB(90,218,225):RGB(90,118,132));
  dx=(int)(cosf(t)*(r+4));dy=(int)(sinf(t)*(r+4));px(cx+dx,cy+dy,RGB(8,15,26));
 }
 line(cx,cy-r-5,cx,cy-r-1,LIME);
}
#ifdef NARCADE_PROFILE
static float profileMs=0,profileMax=0;void game_set_profile(float ms){profileMs=ms;if(ms>profileMax)profileMax=ms;if(g.clock<.5f)profileMax=0;}
#endif
static void hud(void){
#ifdef NARCADE_SPAWN_X
 {char dbg[96];snprintf(dbg,sizeof(dbg),"x%d y%d lift%d metro%d z%d w%d sp%d ff%d lo%d s%d",(int)g.x,(int)g.y,(int)g.lift,g.inMetro,(int)g.metroZ,(int)g.metroWait,(int)g.footSpeed,foot_free(g.x,g.y-3),lift_ok(g.x,g.y-3),solid(g.x-5,g.y-8));text(6,4,dbg,WHITE,1);snprintf(dbg,sizeof(dbg),"ovf%d side%d stucco%d paint%d road%d roof%d",r3_overflow(),r3_used(1),r3_used(3),r3_used(13),r3_used(0),r3_used(5));text(6,14,dbg,WHITE,1);text(6,4,dbg,WHITE,1);}
#endif
 char b[180];
#ifdef NARCADE_PROFILE
 snprintf(b,sizeof(b),"%.1f ms  (max %.1f)",profileMs,profileMax);rect(4,40,140,14,INK);text(8,42,b,GOLD,1);
#endif
 /* Barrio nuevo: aviso temporal arriba. */
 int d=district(g.x,g.y);if(g.screen==WORLD&&d!=g.hudDistrict){g.hudDistrict=d;g.hudDistrictT=2.2f;}
 if(g.hudDistrictT>0){int w=(int)strlen(districts[d])*7+24;box_center(W/2,10,w,19,INK);text_center(W/2,13,districts[d],TEAL,1);}
 /* Objetivo nuevo: frase temporal abajo. */
 if(g.screen==WORLD&&g.mission<36){int key=g.mission*8+g.step+1;if(key!=g.hudStepKey){g.hudStepKey=key;g.hudObjectiveT=5.0f;}}
 /* Esquina superior derecha: vida, carro, dinero, busqueda. */
 rect(W-122,8,112,5,RGB(40,48,50));rect(W-122,8,(int)(g.health*1.12f),5,g.health>30?TEAL:CORAL);
 if(g.car>=0){rect(W-122,15,112,3,RGB(40,48,50));rect(W-122,15,(int)(g.cars[g.car].hp*1.12f),3,GOLD);}
 else if(!g.inMetro){rect(W-122,15,112,4,RGB(40,48,50));rect(W-122,15,(int)(g.stamina*1.12f),4,g.exhausted?CORAL:GOLD);}
 snprintf(b,sizeof(b),"$%d",g.cash);text(W-10-(int)strlen(b)*7,21,b,WHITE,1);
 if(wanted_stars()>=1){int blink=g.escape>0&&((int)(g.clock*4)&1);for(int i=0;i<5;i++)rect(W-122+i*10,36,7,4,wanted_stars()>i?(blink?RGB(120,70,60):CORAL):RGB(40,48,50));}
 /* Cronometros de ruta / huida. */
 if(g.raceTime>0){snprintf(b,sizeof(b),"RUTA %d/6   %ds",g.checkpoint+1,(int)g.raceTime);box_center(W/2,34,(int)strlen(b)*7+16,17,INK);text_center(W/2,37,b,GOLD,1);}
 else if(g.mission<36&&step_now()->kind==K_CHASE&&g.missionTimer>0){snprintf(b,sizeof(b),"ALEJATE Y PIERDE LA BUSQUEDA / %ds",(int)fmaxf(0,step_now()->par-g.missionTimer));box_center(W/2,34,(int)strlen(b)*7+16,17,INK);text_center(W/2,37,b,CORAL,1);}
 /* Abajo al centro: aviso > accion contextual > frase del objetivo. */
 const char *line=NULL;uint32_t col=WHITE;
 if(g.noticeT>0){line=g.notice;col=WHITE;}
 else if(g.inMetro){line=g.metroWait>0?"R+[] BAJAR DEL METRO":"METRO EN MARCHA";col=LIME;}
 else if(metro_boardable()){line="R+[] SUBIR AL METRO";col=LIME;}
 else if(g.mission<36){const Step *st=step_now();
  if(!g.side&&near_hub(st->loc,58)){const char *act=st->kind==K_DRIVE?"ENTREGAR":st->kind==K_RACE?"INICIAR RUTA":st->kind==K_CHASE?"INICIAR HUIDA":st->kind==K_TALK||st->kind==K_ENDING?"HABLAR":"INTERACTUAR";snprintf(b,sizeof(b),"%s %s",g.car>=0?"ARRIBA":"R+[]",act);line=b;col=LIME;}
  else if(g.hudObjectiveT>0){snprintf(b,sizeof(b),"%s  /  %s",locations[st->loc].name,st->text);line=b;col=LIME;}}
 if(!line&&!combat.aiming&&combat_wall_at(g.x,g.y,14))line="[] ESCALAR MURO";
 if(line){ /* a la derecha del minimapa: zona util x=92..470 (378 px, 51 caracteres por linea) */
  line=locale_text(line);int n=(int)strlen(line);int cw=51;int lines=(n+cw-1)/cw;int w=lines>1?378:n*7+20;int cx=92+378/2;
  box_center(cx,H-14-lines*13,w,lines*13+8,INK);
  if(lines==1)text_raw(cx-n*7/2,H-10-13,line,col,1);else textwrap_raw(cx-w/2+10,H-10-lines*13,w-20,line,col);}
 /* Minimapa. */
#ifndef AB_NOMINIMAP
 minimap(46,H-46,32);
#endif
}
static uint32_t title_color(unsigned short c){return RGB((c&31)*255/31,((c>>5)&63)*255/63,((c>>11)&31)*255/31);}
static void title_image(const unsigned short *src,int sw,int sh,int dx,int dy){
 for(int y=0;y<sh;y++){int sy=dy+y;if((unsigned)sy>=H)continue;
  for(int x=0;x<sw;x++){int sx=dx+x;if((unsigned)sx<W)fb[sy*pitch+sx]=title_color(src[y*sw+x]);}}
}
/* One brand mark from the PSP icon, composited over both title screens. */
static void title_logo(const unsigned short *src,int sw,int sh,int dx,int dy){
 for(int y=0;y<sh;y++){int py=dy+y;if((unsigned)py>=H)continue;
  for(int x=0;x<sw;x++){int px0=dx+x;if((unsigned)px0>=W)continue;
   unsigned p=src[y*sw+x],a=p>>12;if(!a)continue;
   unsigned rr=(p&15)*17,gg=((p>>4)&15)*17,bb=((p>>8)&15)*17;
   uint32_t *dst=&fb[py*pitch+px0],old=*dst;
   *dst=RGB((((old&255)*(15-a)+rr*a)+7)/15,
            (((((old>>8)&255)*(15-a)+gg*a)+7)/15),
            (((((old>>16)&255)*(15-a)+bb*a)+7)/15));
  }}
}
/* UI letterforms are rendered by Pillow from Oxanium/Rajdhani/Allura at
   native PSP resolution. Alpha compositing preserves antialiased edges. */
static void title_label(int id,int x,int y,uint32_t color){
 if(prefsLanguage)id+=TL_EN_LOGO;
 const TitleLabel *l=&titleLabels[id];
 for(int yy=0;yy<l->height;yy++){int py=y+yy;if((unsigned)py>=H)continue;
  for(int xx=0;xx<l->width;xx++){int px=x+xx;if((unsigned)px>=W)continue;
   unsigned a=title_labels_v213[l->offset+yy*l->width+xx];if(!a)continue;
   uint32_t *dst=&fb[py*pitch+px],old=*dst;
   unsigned r=(((old&255)*(255-a))+((color&255)*a)+127)/255;
   unsigned g0=((((old>>8)&255)*(255-a))+(((color>>8)&255)*a)+127)/255;
   unsigned b=((((old>>16)&255)*(255-a))+(((color>>16)&255)*a)+127)/255;
   *dst=RGB(r,g0,b);
  }
 }
}
typedef struct {int x,y,w,h,label;const unsigned short *image;} MenuPanel;
static const MenuPanel menuPanels[5]={
 {14,64,146,94,TL_CONTINUE,menu_continue_v242},
 {174,64,146,94,TL_NEW,menu_new_v242},
 {334,64,132,164,TL_SETTINGS,menu_settings_v242},
 {14,172,146,56,TL_CREDITS,menu_credits_v242},
 {174,172,146,56,TL_TROPHIES,menu_trophies_v242}
};
static void title_card(int id){
 const MenuPanel *p=&menuPanels[id];int selected=g.menu==id;
 /* Compiled image dimensions are exact: use the same extents for selection. */
 title_image(p->image,p->w,p->h,p->x,p->y);
 title_label(p->label,p->x+8,p->y+p->h-24,WHITE);
 outline(p->x-2,p->y-2,p->w+4,p->h+4,selected?LIME:RGB(64,77,96));
 if(selected){outline(p->x-3,p->y-3,p->w+6,p->h+6,LIME);rect(p->x,p->y,22,2,LIME);}
}
static void title_draw(void){
 if(!g.titleStage){
  title_image(title_cover_v213,W,H,0,0);
  rect(0,0,W,3,RGB(31,191,205));
  title_logo(narcade_logo_cover_v244,420,143,18,15);
  title_label(TL_COVER_SUB,26,157,RGB(188,220,230));
  rect(0,174,275,98,RGB(7,14,25));
  rect(20,190,4,48,RGB(84,218,230));
  uint32_t prompt=sinf(g.clock*3.2f)>-.45f?WHITE:RGB(116,165,176);
  title_label(TL_PRESS,33,194,prompt);
  title_label(TL_PRESS_SUB,34,218,RGB(146,184,195));
  title_label(TL_MADE_BY,356,234,WHITE);title_label(TL_CREDIT,397,224,WHITE);
  rect(20,253,122,2,RGB(64,145,161));
 }else{
  title_image(title_menu_v213,W,H,0,0);
  rect(0,0,W,4,RGB(68,204,218));
  rect(0,0,W,65,RGB(7,13,23));
  title_logo(narcade_logo_small_v244,115,39,14,5);
  title_label(TL_STORY,185,13,WHITE);
  title_label(TL_STORY_KICKER,14,43,RGB(153,186,196));
  for(int i=0;i<5;i++)title_card(i);
  title_label(TL_FOOTER,14,249,RGB(190,212,219));
  title_label(TL_MADE_BY,356,252,MUTED);title_label(TL_CREDIT,395,246,WHITE);
 }
 if(g.noticeT>0){rect(0,246,W,26,INK);textwrap(12,247,W-24,g.notice,CORAL);}
}
static void credits_draw(void){
 title_image(title_menu_v213,W,H,0,0);header("CREDITOS","UN PROYECTO ORIGINAL PARA PSP");title_logo(narcade_logo_small_v244,115,39,344,4);
 font_text(24,61,locale_text("CREADO POR NARESZ"),LIME,2,1);
 text(24,97,"Idea, direccion creativa y universo: Naresz",WHITE,1);
 text(24,122,"ASISTENCIA DE DESARROLLO",TEAL,1);
 text(24,141,"Codex (OpenAI) / Claude Code (Anthropic)",WHITE,1);
 text(24,165,"Inspirado en Medellin y el valle de Aburra",WHITE,1);
 text(24,186,"Personajes originales: Nico, Luna y su comunidad",MUTED,1);
 text(24,207,"Oxanium / Rajdhani: tipografias SIL OFL",MUTED,1);
 text(24,226,"Version en desarrollo / gracias por jugar",GOLD,1);
 footer("O VOLVER");
}
static void trophies_draw(void){
 title_image(title_menu_v213,W,H,0,0);header("TROFEOS","CADA HISTORIA DEJA SU HUELLA");title_logo(narcade_logo_small_v244,115,39,344,4);
 for(int i=0;i<GAME_TROPHY_COUNT;i++){
  GameTrophyInfo info={trophyDefs[i].name,trophyDefs[i].desc,(int)trophy_progress(i),(int)trophyDefs[i].goal,(tro.unlocked>>i)&1};
  int valid=1;if(trophyProvider)valid=trophyProvider(i,&info);
  if(!valid)info=(GameTrophyInfo){trophyDefs[i].name,"Sistema de trofeos en preparacion",0,1,0};
  int y=57+i*35;uint32_t accent=info.unlocked?GOLD:TEAL;
  rect(14,y,452,32,PANEL);
  if(i==trophySelection)outline(14,y,452,32,accent);
  circle(30,y+16,7,info.unlocked?GOLD:MUTED);text(45,y+3,info.title?info.title:trophyDefs[i].name,WHITE,1);
  text(45,y+17,info.unlocked?"DESBLOQUEADO":(valid?"EN PROGRESO":"PROXIMAMENTE"),info.unlocked?GOLD:MUTED,1);
  float amount=info.unlocked?1:!trophyProvider?clampf(trophy_progress(i)/trophyDefs[i].goal,0,1):info.target>0?clampf((float)info.current/info.target,0,1):0;
  rect(339,y+21,114,3,INK);rect(339,y+21,(int)(114*amount),3,accent);
  if(valid){char b[40];snprintf(b,sizeof b,"%d/%d",info.current,info.target);text(339,y+4,b,MUTED,1);}
  if(i==trophySelection&&info.detail){char detail[64];raw_snprintf(detail,sizeof detail,"%s",locale_text(info.detail));text_raw(17,237,detail,MUTED,1);}
 }
 footer("ARRIBA/ABAJO EXPLORAR  O VOLVER");
}
static void mini_draw(void){
 Puzzle *p=&g.p;char b[180];rect(0,0,W,H,INK);header("NARCADE / INTERACCION",puzzleNames[p->kind]);
 if(p->kind==K_CODE){
  text(20,57,"LEE LAS PISTAS. ARMA LA CLAVE EN ORDEN.",TEAL,1);
  for(int i=0;i<4;i++){int x=66+i*90;rect(x,80,70,60,PANEL);outline(x,80,70,60,p->cursor==i?LIME:MUTED);snprintf(b,sizeof(b),"%d",p->digits[i]);text(x+22,88,b,WHITE,3);text(x+29,66,"^",p->cursor==i?LIME:INK,1);}
  const char *clues[]={"los lados que tiene un circulo", "el unico sol que ilumina el valle", "las orillas del rio", "las partes de la llave de Sara", "los lados de una cancha rectangular", "los dedos de una mano abierta", "los controles de cada ruta", "los dias de una semana", "los lados de un octagono", "tres veces el numero de nodos"};
  for(int i=0;i<4;i++){snprintf(b,sizeof(b),"%d. %s",i+1,clues[p->answer[i]]);text(23,153+i*20,b,MUTED,1);}
  footer("< > posicion  ARRIBA/ABAJO digito  X probar  O salir");
 }else if(p->kind==K_CIRCUIT){
  circuit_connected();for(int i=0;i<16;i++){int x=48+(i%4)*43,y=62+(i/4)*43,cx=x+19,cy=y+19;rect(x,y,38,38,PANEL);outline(x,y,38,38,i==p->cursor?LIME:RGB(43,69,75));int v=p->tiles[i];uint32_t col=p->connected[i]?TEAL:MUTED;circle(cx,cy,4,col);if(v&1)rect(cx-2,y,5,20,col);if(v&2)rect(cx,cy-2,19,5,col);if(v&4)rect(cx-2,cy,5,19,col);if(v&8)rect(x,cy-2,20,5,col);}
  text(11,75,"IN",LIME,1);text(221,204,"OUT",LIME,1);
  text(259,67,"RESTABLECE EL PC",WHITE,1);textwrap(259,94,205,"Gira las piezas. Une la entrada izquierda de arriba con la salida derecha de abajo.",MUTED);
  textwrap(259,163,200,"Las piezas turquesa tienen corriente. No es necesario usar todas.",TEAL);footer("CRUCETA mover  X girar pieza  O salir");
 }else if(p->kind==K_MEMORY){
  int len=4+p->phase;int watching=p->t<len*.8f+1;snprintf(b,sizeof(b),"RONDA %d/3 / %d PASOS",p->phase+1,len);text(24,61,b,TEAL,1);text(24,82,watching?"OBSERVA LA SECUENCIA":"REPITE CON LA CRUCETA",WHITE,1);
  const char *names[]={"ARRIBA","DERECHA","ABAJO","IZQUIERDA"};
  for(int i=0;i<4;i++){int x=24+i*113;int lit=watching&&p->t>.5f&&(int)((p->t-.5f)/.8f)<len&&p->seq[(int)((p->t-.5f)/.8f)]==i&&fmodf(p->t-.5f,.8f)<.57f;rect(x,117,102,69,lit?TEAL:PANEL);text(x+10,143,names[i],lit?INK:WHITE,1);}
  if(!watching){snprintf(b,sizeof(b),"%d de %d correctos",p->input,len);text(150,206,b,LIME,1);}else text(123,206,"Espera a que termine el patron",MUTED,1);
  footer("CRUCETA responder  [] volver a ver  O salir");
 }else if(p->kind==K_TUNE){
  float target=p->target+sinf(p->t*.75f)*4;int good=fabsf(target-p->value)<4;
  text(25,62,"ENCUENTRA LA BANDA Y SOSTEN X",TEAL,1);rect(28,111,420,38,PANEL);rect(28+target*4.2f-17,111,34,38,RGB(44,102,84));rect(27+p->value*4.2f,104,3,52,good?LIME:CORAL);
  for(int i=0;i<=10;i++)rect(28+i*42,155,1,6,MUTED);
  snprintf(b,sizeof(b),"SENAL %d%%  /  ENLACE %.1f de 4.0 s",(int)(100-clampf(fabsf(p->value-target)*4,0,100)),p->hold);text(57,180,b,WHITE,1);rect(58,208,360,7,PANEL);rect(58,208,p->hold*90,7,TEAL);
  footer("< > ajustar  Mantener X dentro de la banda  O salir");
 }else if(p->kind==K_LOCK){
  snprintf(b,sizeof(b),"CIERRE %d/4",p->phase+1);text(24,64,b,TEAL,1);textwrap(24,88,430,"Pulsa X cuando el indicador cruce la zona verde. Cada cierre cambia el punto de ajuste.",MUTED);
  rect(30,147,420,38,PANEL);rect(30+(p->target-11)*4.2f,147,92,38,RGB(51,117,88));rect(30+p->value*4.2f,137,3,58,LIME);footer("X ajustar cierre  O salir");
 }else if(p->kind==K_RHYTHM){
  const char *labels[]={"<","^","v",">"};for(int k=0;k<4;k++){int x=108+k*67;rect(x,61,51,175,PANEL);rect(x,208,51,3,TEAL);text(x+21,221,labels[k],WHITE,1);}
  for(int i=0;i<24;i++)if(p->hit[i]==0){float at=2.0f+i*.57f;int y=209-(at-p->t)*84;if(y>=60&&y<=216){int x=108+p->notes[i]*67;rect(x+5,y-6,41,12,LIME);}}
  snprintf(b,sizeof(b),"%d/14",p->hits);text(19,99,"ACIERTOS",TEAL,1);text(19,119,b,WHITE,1);text(397,98,"SIGUE",MUTED,1);text(397,114,"EL",MUTED,1);text(397,130,"PULSO",MUTED,1);footer("CRUCETA: pulsa al cruzar la linea turquesa  O salir");
 }else if(p->kind==K_STEALTH){
  text(24,55,"CRUZA POR LOS HUECOS EN MOVIMIENTO",TEAL,1);rect(18,76,444,158,PANEL);
  for(int i=0;i<5;i++){int x=90+i*68;float cy=150+sinf(p->t*(.85f+i*.08f)+i*1.7f+p->seed)*47;rect(x-3,76,6,(int)cy-26-76,CORAL);rect(x-3,cy+26,6,234-(cy+26),CORAL);circle(x,cy-31,4,GOLD);circle(x,cy+31,4,GOLD);}
  rect(449,79,10,147,RGB(45,112,91));person_draw(p->x,p->y,0,0,1,0);footer("CRUCETA o ANALOGICO mover  Evita los haces rojos  O salir");
 }
 if(g.noticeT>0){rect(10,234,460,16,INK);text(15,236,g.notice,MUTED,1);}
}
static void map_draw(void){
 char b[180];rect(0,0,W,H,INK);header("MEDELLIN / VALLE Y LADERAS","Contorno urbano real; calles y relieve simplificados");
 float sc=.055f;int ox=42,oy=59;
 for(int py=0;py<185;py++)for(int pxp=0;pxp<180;pxp++)px(ox+pxp,oy+py,map_ground(pxp/sc,py/sc));
 text(23,61,"N",WHITE,1);line(26,91,26,77,WHITE);line(26,77,23,82,WHITE);line(26,77,29,82,WHITE);
 for(int i=0;i<30;i++){int x,y;map_point(locations[i].x,locations[i].y,sc,ox,oy,&x,&y);circle(x,y,2,MUTED);if(i==g.mapSel)outline(x-4,y-4,9,9,WHITE);}
 if(g.mission<36){int l=g.raceTime>0?g.route[g.checkpoint]:step_now()->loc;int mx,my;map_point(locations[l].x,locations[l].y,sc,ox,oy,&mx,&my);
  /* Icono de objetivo: chincheta amarilla con "!" sobre el punto. */
  circle(mx,my-9,6,INK);circle(mx,my-9,5,LIME);line(mx-3,my-5,mx,my,INK);line(mx+3,my-5,mx,my,INK);line(mx-2,my-5,mx,my-1,LIME);line(mx+2,my-5,mx,my-1,LIME);rect(mx,my-12,1,4,INK);rect(mx,my-7,1,1,INK);}
 /* Jugador: flecha coral con su orientacion. */
 {int pxp,pyp;map_point(g.x,g.y,sc,ox,oy,&pxp,&pyp);float a=geo_heading(g.x,g.y,g.car>=0?g.cars[g.car].a:g.a);circle(pxp,pyp,3,INK);circle(pxp,pyp,2,CORAL);line(pxp,pyp,pxp+(int)(cosf(a)*6),pyp+(int)(sinf(a)*6),CORAL);}
 text(245,61,"LUGAR SELECCIONADO",TEAL,1);textwrap(245,83,222,locations[g.mapSel].name,WHITE);
 snprintf(b,sizeof(b),"Distancia: %d m",(int)dist(g.x,g.y,locations[g.mapSel].x,locations[g.mapSel].y));text(245,111,b,MUTED,1);
 text(245,134,"CHINCHETA: objetivo actual",LIME,1);text(245,150,"CORAL: tu posicion",CORAL,1);
 textwrap(245,170,220,districts[district(locations[g.mapSel].x,locations[g.mapSel].y)],TEAL);
 snprintf(b,sizeof(b),"Relieve: +%d unidades",(int)geo_height(locations[g.mapSel].x,locations[g.mapSel].y));text(245,204,b,MUTED,1);
 text(245,222,"Claro: ladera / azul: rio",MUTED,1);
 footer("CRUCETA recorrer lugares  SELECT u O volver");
}
static void journal_draw(void){
 char b[180];rect(0,0,W,H,INK);header("CUADERNO DE NICO",g.journalPage==0?"La pista actual":g.journalPage==1?"Personas y progreso":"Controles y servicios");
 if(g.journalPage==0){if(g.mission<36){text(21,59,chapters[g.mission/6],TEAL,1);text(21,81,missions[g.mission].title,WHITE,1);textwrap(21,107,435,step_now()->text,LIME);textwrap(21,156,435,missions[g.mission].intro,MUTED);}else textwrap(22,72,430,"La historia termino. La ciudad sigue abierta. Puedes conducir, coleccionar los 24 vinilos y aceptar entregas en el mercado de Belen.",LIME);}
 else if(g.journalPage==1){
  snprintf(b,sizeof(b),"HISTORIA %d/36   VINILOS %d/24",g.mission,cachecount());text(21,62,b,LIME,1);snprintf(b,sizeof(b),"REPUTACION %d   ENCARGOS %d   TIEMPO %dh %02dm",g.reputation,g.jobs,(int)g.playtime/3600,((int)g.playtime/60)%60);text(21,82,b,MUTED,1);
  const char *lines[]={"Nico / Mensajero. Una firma falsa cambio su vida.","Sara / Hermana de Nico. Investiga a Prisma.","Vera / Periodista. Sigue los documentos.","Luna / Mecanica. Conoce cada ruta de la ciudad.","Mara / DJ de Radio Ladera. Escucha lo oculto.","Tiza / Artista. Las paredes guardan sus pistas."};for(int i=0;i<6;i++)text(21,115+i*20,lines[i],WHITE,1);
 }else{
  const char *lines[]={"A PIE: mover. X trotar; pulsar X correr.","TRIANGULO: entrar o salir de un carro cercano.","[] saltar. R+[] interactuar. ARRIBA en carro.","EN CARRO: X gas, [] freno/reversa, < > girar.","R apuntar + O atacar. L rueda de armas.","O: mensajes. SELECT: camara. START: menu.","TALLER: reparacion $100. REFUGIO: curar/guardar.","MERCADO: encargos en carro. VINILOS: R+[] recoger.","Guardado por objetivo. Menu permite guardar.","Busqueda: alejate. Los minijuegos se reintentan."};for(int i=0;i<10;i++)text(21,58+i*18,lines[i],i%2?MUTED:WHITE,1);
 }
 footer("L/R paginas  O volver");
}
static void pause_draw(void){
 const char *tabs[]={"MAPA","MENSAJES","PARTIDA","AJUSTES","TROFEOS","SALIR"};char b[100];
 if(g.pauseTab==0)map_draw();
 else if(g.pauseTab==1)journal_draw();
 else if(g.pauseTab==3)settings_draw();
 else if(g.pauseTab==4)trophies_draw(); /* v2.42 (Claude) */
 else{
  rect(0,0,W,H,INK);rect(18,70,444,161,PANEL);
  if(g.pauseTab==2){
   text(34,84,"TU PARTIDA",TEAL,2);
   snprintf(b,sizeof(b),"Historia %d/36   Dinero $%d",g.mission,g.cash);text(34,117,b,MUTED,1);
   const char *actions[]={"GUARDAR PARTIDA","VOLVER AL JUEGO"};
   for(int i=0;i<2;i++){if(g.menu==i)rect(28,143+i*34,424,29,RGB(43,62,64));text(39,151+i*34,actions[i],g.menu==i?LIME:WHITE,1);}
  }else{
   text(34,83,"MENU PRINCIPAL",TEAL,2);
   textwrap(34,120,395,"Se guardara tu progreso antes de volver al titulo de Narcade. Podras continuar desde el ultimo objetivo guardado.",MUTED);
   rect(28,179,424,32,RGB(43,62,64));text(40,189,"X  GUARDAR Y VOLVER AL TITULO",LIME,1);
  }
 }
 rect(0,0,W,54,INK);title_logo(narcade_logo_small_v244,115,39,14,2);text(300,15,"JUEGO EN PAUSA",MUTED,1);
 for(int i=0;i<6;i++){int x=8+i*78;rect(x,34,74,19,i==g.pauseTab?LIME:PANEL);text(x+(74-(int)strlen(locale_text(tabs[i]))*7)/2,38,tabs[i],i==g.pauseTab?INK:MUTED,1);}
 if(g.noticeT>0&&g.pauseTab>=2&&g.pauseTab!=4){rect(12,231,456,18,INK);text(18,235,g.notice,g.saveOK?TEAL:CORAL,1);}
 footer(g.pauseTab==3?"L/R seccion  ARRIBA/ABAJO elegir  < > cambiar  O volver":g.pauseTab==0?"L/R seccion  ARRIBA/ABAJO lugar  START/O volver":g.pauseTab==1?"L/R seccion  ARRIBA/ABAJO pagina  START/O volver":g.pauseTab==4?"L/R seccion  START/O volver":"L/R seccion  ARRIBA/ABAJO elegir  X aceptar  O volver");
}
#include "settings.inc"
#include "weapon_icons.h"
static void weapon_icon(int x,int y,int id,uint32_t color){
 (void)color;
 for(int yy=0;yy<32;yy++)for(int xx=0;xx<64;xx++){
  unsigned v=weapon_icons[id][yy*64+xx];if(!v)continue;
  px(x+xx-32,y+yy-16,RGB((v&31)*255/31,((v>>5)&63)*255/63,((v>>11)&31)*255/31));
 }
}
static void weapon_wheel_draw(void){
 /* Solid, high-contrast sectors remain legible on the 480x272 display. */
 for(int y=-105;y<=105;y++)for(int x=-105;x<=105;x++){
  int r=x*x+y*y;if(r<42*42||r>105*105)continue;
  int slot=wrapi((int)floorf(atan2f((float)x,(float)-y)/(PI/4)+.5f),8);
  px(240+x,133+y,slot==g.weaponChoice?RGB(51,94,92):RGB(16,28,36));
 }
 for(int i=0;i<8;i++){
  float a=i*PI/4;int x=240+(int)(sinf(a)*77),y=133-(int)(cosf(a)*77);
  weapon_icon(x,y,i,i==g.weaponChoice?LIME:WHITE);
  float edge=a+PI/8;line(240+(int)(sinf(edge)*43),133-(int)(cosf(edge)*43),240+(int)(sinf(edge)*105),133-(int)(cosf(edge)*105),MUTED);
 }
 circle(240,133,40,INK);text_center(240,125,weaponNames[g.weaponChoice],LIME,1);
 text_center(240,142,"L",TEAL,1);
 label(153,8,"SELECCIONAR ARMA",WHITE);
 footer("MANTEN L + JOYSTICK elegir / SUELTA L equipar");
}
static void hud_clear_bands(uint32_t *layer){ /* limpia lo dibujado el fotograma anterior y empieza el registro */
 for(int b=0;b<17;b++)if(hudPrev[b]){int y0=b*16,y1=y0+16>H?H:y0+16;memset(layer+y0*512,0,(size_t)(y1-y0)*512*sizeof(uint32_t));}
 memset(hudBand,0,sizeof hudBand);
}
#ifdef NARCADE_3D
static void build_scene(R3Scene *sp){
 memset(sp,0,sizeof(*sp));sp->x=g.x;sp->z=g.y;sp->angle=g.a;sp->yaw=g.viewYaw;sp->time=g.clock;
 sp->camBase=g.camBase;sp->camClear=g.camClear;
 sp->driving=g.car>=0;sp->moving=g.walking&&g.screen==WORLD;sp->cameraDistance=g.cameraDistance>0?g.cameraDistance:cam_distance();sp->eyeHeight=cam_eye();
 sp->wallCount=climbWallCount;for(int i=0;i<climbWallCount;i++){sp->walls[i][0]=climbWall[i][0];sp->walls[i][1]=climbWall[i][1];sp->walls[i][2]=logical_heading_from_projected(climbWall[i][0],climbWall[i][1],0);}
 sp->shotX=combat.shotX;sp->shotZ=combat.shotY;sp->shotHeight=combat.shotHeight;sp->shotTime=combat.shotTime;
 sp->weapon=g.weapon;sp->aiming=combat.aiming;sp->cameraPitch=combat.pitch;sp->aimPitch=combat.pitch;if(combat.aiming&&combat.target<0&&g.weapon>0&&g.weapon<7){float origin,slope;combat_camera_ray(&origin,&slope);sp->aimPitch=atanf(slope);}sp->recoil=combat.recoil;sp->punch=combat.punch;sp->combo=combat.combo;sp->jump=combat.jump;sp->climb=combat.vault;
 sp->motion=g.motion;sp->gaitPhase=g.gaitPhase;sp->lift=g.lift+combat.jump;sp->metroZ=g.metroZ;sp->metroDir=g.metroDir;sp->inMetro=g.inMetro;
 sp->metroDoors=cm_station_near(g.metroZ,2)>=0?clampf(fminf((6-g.metroWait)/.7f,g.metroWait/.7f),0,1):0;
 /* Obstruction distance is maintained in projected space by camera_clearance. */
 sp->carCount=CAR_COUNT;sp->personCount=42;sp->collected=g.caches;
 for(int i=0;i<CAR_COUNT;i++)sp->cars[i]=(R3Car){g.cars[i].x,g.cars[i].y,g.cars[i].a,g.cars[i].speed,g.cars[i].type,g.cars[i].police,(i*7+g.cars[i].type*3)%12};
 for(int i=0;i<42;i++){CombatPed *p=&combat.ped[i];sp->people[i]=(R3Person){g.peds[i].x,g.peds[i].y,p->yaw,i,p->health,p->flee,p->fall,p->hit,p->phase};}
 for(int i=0;i<30;i++){sp->hubs[i][0]=locations[i].x;sp->hubs[i][1]=locations[i].y;}
 sp->target=g.raceTime>0?g.route[g.checkpoint]:g.mission<36?step_now()->loc:-1;
 if(sp->target>=0){sp->targetX=locations[sp->target].x;sp->targetZ=locations[sp->target].y;}
}
#endif
static void draw_frame(uint32_t *pixels,int stride){fb=pixels;pitch=stride;
 if(g.screen==TITLE){title_draw();return;}if(g.screen==CREDITS){credits_draw();return;}if(g.screen==TROPHIES){trophies_draw();return;}if(g.screen==SETTINGS){settings_draw();return;}if(g.screen==MINI){mini_draw();return;}if(g.screen==MAP){map_draw();return;}if(g.screen==JOURNAL){journal_draw();return;}if(g.screen==PAUSE){pause_draw();return;}
#ifdef NARCADE_3D
 static R3Scene scene;build_scene(&scene);
 r3_draw(renderTarget,&scene);
#ifndef R3_HOST
 hud_clear_bands(fb); /* v2.37: se limpia la capa del HUD mientras el GE pinta la escena (v2.41: solo sus franjas) */
#endif
#else
 world_draw();
#endif
 if(g.weaponWheel){weapon_wheel_draw();return;}
 hud();combat_hud();trophy_popup_draw();
 if(g.screen==DIALOG){rect(16,70,448,144,INK);outline(16,70,448,144,RGB(67,92,93));rect(16,70,4,144,LIME);text(31,83,g.speaker,TEAL,1);textwrap(31,106,416,g.dialog,WHITE);text(31,196,g.dialogAction==3?"X empezar de nuevo  O cancelar":"X continuar",LIME,1);}
 if(g.screen==CHOICE){rect(16,62,448,173,INK);text(30,76,"TU DECISION / EL FUTURO DEL EXPEDIENTE",TEAL,1);textwrap(30,98,412,"Las dos opciones protegen los datos privados. Elige quien llevara las pruebas a la ciudad.",MUTED);const char *items[]={"VERA: entregar el expediente a la justicia","MARA: publicar tambien una memoria vecinal"};for(int i=0;i<2;i++){rect(25,146+i*35,429,28,i==g.menu?PANEL:INK);text(31,154+i*35,items[i],i==g.menu?LIME:WHITE,1);}text(31,219,"ARRIBA/ABAJO elegir  X confirmar",MUTED,1);}
}
void game_draw(uint32_t *pixels,int stride){
#ifdef NARCADE_3D
 renderTarget=pixels;
#ifndef R3_HOST
 uint32_t *overlay=hudLayer;
 int world=!(g.screen==TITLE||g.screen==MINI||g.screen==MAP||g.screen==JOURNAL||g.screen==PAUSE||g.screen==SETTINGS||g.screen==CREDITS||g.screen==TROPHIES);
 if(!world){memset(overlay,0,512*272*sizeof(uint32_t));memset(hudPrev,1,sizeof hudPrev);}
 draw_frame(overlay,512);
 if(!world)memset(hudBand,1,sizeof hudBand);
 r3_overlay_bands(pixels,overlay,hudBand);memcpy(hudPrev,hudBand,sizeof hudPrev);
#else
 draw_frame(pixels,stride);
#endif
#else
 draw_frame(pixels,stride);
#endif
}
void game_audio(short *stereo,unsigned frames){
 int musicVolume=prefsMusic,effectsVolume=prefsEffects;int station=audioStation;if(station>3)station=0;if(station!=audioLast){audioPos=0;audioLast=station;}
 static unsigned bedPos=0,enginePos=0,hornPos=CITY_HORN_SAMPLES*2,seenHorn=0;
 static unsigned seenMenu=0,menuSample=4000,seenShot=0,shotSample=7000,noise=1234567;static int soundKind=0;
 static int hGain=0,hPan=0,fade=0,engineLevel=0;
 if(seenHorn!=hornEvent){seenHorn=hornEvent;if(audioAmbient){hornPos=0;hGain=hornGain;hPan=hornPan;}}
 if(seenMenu!=menuEvent){seenMenu=menuEvent;menuSample=0;}
 if(seenShot!=shotEvent){seenShot=shotEvent;shotSample=0;soundKind=shotKind;}
 for(unsigned i=0;i<frames;i++){
  int music=audioEnabled?radio_data[track_start[station]+audioPos/2]:0;
  if(audioEnabled){audioPos++;if(audioPos>=(unsigned)track_length[station]*2)audioPos=0;}
  int target=audioAmbient?256:0;if(fade<target)fade++;else if(fade>target)fade--;
  if((i&127)==0){if(engineLevel<trafficGain)engineLevel++;else if(engineLevel>trafficGain)engineLevel--;}
  int base=city_bed[bedPos/4]*(110+crowdGain)/256;
  int baseRight=city_bed[((bedPos/4)+137)%CITY_BED_SAMPLES]*(110+crowdGain)/256;
  int engine=city_engine[enginePos/4]*engineLevel/256;
  int horn=hornPos<CITY_HORN_SAMPLES*2?city_horn[hornPos++/2]*hGain/256:0;
  bedPos=(bedPos+1)%(CITY_BED_SAMPLES*4);enginePos=(enginePos+1)%(CITY_ENGINE_SAMPLES*4);
  int click=0;
  if(menuSample<3969){ /* 90 ms: soft attack, two rounded partials, no XMB sample. */
   float t=menuSample++/44100.f,env=(1-expf(-t*1100))*expf(-t*46);
   float phase=2*PI*(810*t-1050*t*t);
   click=(int)(2900*env*(sinf(phase)+.24f*sinf(phase*1.5f)));
  }
  int attack=0;
  if(shotSample<6500){unsigned sample=shotSample++;noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;
   static short tone[64];static int toneReady;
   if(!toneReady){for(int k=0;k<64;k++)tone[k]=(short)(sinf(2*PI*k/64)*8192);toneReady=1;}
   int env=(6500-(int)sample)*256/6500;env=env*env/256;
   int body=tone[(sample*(soundKind?115:82)*64/44100)&63];
   int hiss=((int)(noise&65535)-32768)*(soundKind?55:8)/256;
   attack=(body+hiss)*env/256;
  }
  int l=music+(base+engine+horn*(128-hPan)/128)*fade/256+click+attack;
  int r=music+(baseRight+engine+horn*(128+hPan)/128)*fade/256+click+attack;
  l=l*effectsVolume/10+music*(musicVolume-effectsVolume)/10;r=r*effectsVolume/10+music*(musicVolume-effectsVolume)/10;
  stereo[i*2]=(short)(l<-32768?-32768:l>32767?32767:l);
  stereo[i*2+1]=(short)(r<-32768?-32768:r>32767?32767:r);
 }
}
