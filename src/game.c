/* Narcade - original PSP homebrew. All game logic also builds on desktop for QA. */
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#define W 480
#define H 272
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
enum {TITLE,WORLD,DIALOG,MINI,MAP,PAUSE,JOURNAL,CHOICE};
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
 int screen,back,mission,step,cash,reputation,ending,car,station,menu,seenIntro,dialogAction,mapSel,journalPage;
 int jobs,side,checkpoint,route[6],saveOK,active; uint32_t caches,prev,pressed,held;
 float x,y,a,health,heat,escape,clock,playtime,timer,noticeT,hitCD,cameraX,cameraY,screenT,raceTime,missionTimer;
 char notice[160],dialog[640],speaker[60],savepath[256];
 Car cars[CAR_COUNT]; Ped peds[42]; Puzzle p;
} g;
static uint32_t *fb;static int pitch;
static volatile int audioStation=0,audioEnabled=0;
static unsigned audioPos=0;static int audioLast=-1;
static uint32_t rng=137;
static uint32_t random_u(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
static float clampf(float a,float lo,float hi){return a<lo?lo:(a>hi?hi:a);}
static float dist(float x,float y,float u,float v){float a=x-u,b=y-v;return sqrtf(a*a+b*b);}
static int wrapi(int x,int n){return (x%n+n)%n;}
static int pressed(int b){return (g.pressed&b)!=0;}
static int held(int b){return (g.held&b)!=0;}
static void notice(const char *s){snprintf(g.notice,sizeof(g.notice),"%s",s);g.noticeT=4;}
static void rect(int x,int y,int w,int h,uint32_t c){int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>W?W:x+w,y1=y+h>H?H:y+h;for(int j=y0;j<y1;j++)for(int i=x0;i<x1;i++)fb[j*pitch+i]=c;}
static void line(int x,int y,int x1,int y1,uint32_t c){int dx=abs(x1-x),sx=x<x1?1:-1,dy=-abs(y1-y),sy=y<y1?1:-1,e=dx+dy;for(;;){rect(x,y,1,1,c);if(x==x1&&y==y1)break;int z=2*e;if(z>=dy){e+=dy;x+=sx;}if(z<=dx){e+=dx;y+=sy;}}}
static void circle(int x,int y,int r,uint32_t c){for(int yy=-r;yy<=r;yy++){int xx=(int)sqrtf((float)(r*r-yy*yy));rect(x-xx,y+yy,xx*2+1,1,c);}}
static void outline(int x,int y,int w,int h,uint32_t c){rect(x,y,w,1,c);rect(x,y+h-1,w,1,c);rect(x,y,1,h,c);rect(x+w-1,y,1,h,c);}
static void text(int x,int y,const char *s,uint32_t c,int scale){int start=x;for(;*s;s++){unsigned char ch=*s;if(ch=='\n'){y+=12*scale;x=start;continue;}if(ch<32||ch>126)ch='?';for(int yy=0;yy<12;yy++){unsigned row=font_bits[(ch-32)*12+yy];for(int xx=0;xx<7;xx++)if(row&(1<<xx))rect(x+xx*scale,y+yy*scale,scale,scale,c);}x+=7*scale;}}
static int textwrap(int x,int y,int width,const char *s,uint32_t c){int limit=width/7,lines=0;while(*s){while(*s==' ')s++;if(!*s)break;int n=0,last=-1;while(s[n]&&s[n]!='\n'&&n<limit){if(s[n]==' ')last=n;n++;}if(s[n]&&s[n]!='\n'&&last>0)n=last;char b[100];int k=n<99?n:99;memcpy(b,s,k);b[k]=0;text(x,y+lines*13,b,c,1);s+=n;if(*s=='\n'||*s==' ')s++;lines++;}return lines*13;}
static void label(int x,int y,const char *s,uint32_t c){rect(x-4,y-2,(int)strlen(s)*7+8,15,INK);text(x,y,s,c,1);}
static void header(const char *a,const char *b){rect(0,0,W,48,INK);rect(16,16,4,20,LIME);text(28,12,a,WHITE,1);text(28,29,b,MUTED,1);}
static void footer(const char *s){rect(0,252,W,20,INK);text(12,257,s,MUTED,1);}
static const Step *step_now(void){return &missions[g.mission<36?g.mission:35].steps[g.step];}
static int parkblock(int bx,int by){return (bx==2&&by==3)||(bx==2&&by==1)||(bx==0&&by==2)||(bx==4&&by==5)||(bx==4&&by==6);}
static int solid(float x,float y){
 if(x<8||y<8||x>WORLD_W-8||y>WORLD_H-8)return 1;
 int lx=(int)x%320,ly=(int)y%320,bx=(int)x/320,by=(int)y/320;
 if(x>1396&&x<1460&&ly>86)return 1;
 return !parkblock(bx,by)&&lx>94&&lx<288&&ly>94&&ly<280;
}
static int free_at(float x,float y,int radius){return !solid(x-radius,y-radius)&&!solid(x+radius,y-radius)&&!solid(x-radius,y+radius)&&!solid(x+radius,y+radius);}
static int district(float x,float y){if(x<640&&y<1000)return 0;if(x<960&&y<1550)return 1;if(x<1100)return 2;if(y<630)return 3;if(x>1760&&y>1380)return 4;if(x>1600)return 5;return 6;}
static const char *districts[]={"COMUNA 13","LAURELES / ESTADIO","BELEN","NORTE / ARANJUEZ","EL POBLADO","LADERA ORIENTAL","CENTRO / RIO"};
static uint32_t carcolors[]={RGB(62,169,154),RGB(230,188,69),RGB(167,80,73),RGB(179,191,183),RGB(79,121,160),RGB(116,91,147)};
static void world_init(void){
 rng=729;for(int i=0;i<CAR_COUNT;i++){
  Car *c=&g.cars[i];int bx=random_u()%8,by=random_u()%7;
  c->type=i%6;c->police=i>=60;c->hp=100;c->speed=0;c->parked=i<38;
  c->x=bx*320+42;c->y=by*320+42;
  c->a=(i%2)?PI*.5f:0;
  if(c->parked){c->x=bx*320+70;c->y=by*320+145+(i%3)*37;c->a=PI*.5f;}
 }
 // One car beside every mission hub: progression never depends on a random spawn.
 for(int i=0;i<30;i++){g.cars[i].x=locations[i].x+54;g.cars[i].y=locations[i].y-31;g.cars[i].a=0;g.cars[i].parked=1;}
 g.cars[1].type=0;
 for(int i=0;i<42;i++){g.peds[i].x=(random_u()%8)*320+82;g.peds[i].y=(random_u()%7)*320+100+(random_u()%180);g.peds[i].v=(i%2?1:-1)*13;g.peds[i].vertical=1;g.peds[i].phase=i;}
}
static uint32_t savecheck(const Save *s){uint32_t h=2166136261u;const unsigned char *p=(const unsigned char*)s;for(size_t i=0;i<offsetof(Save,check);i++){h^=p[i];h*=16777619u;}return h;}
void game_set_save_path(const char *p){snprintf(g.savepath,sizeof(g.savepath),"%s",p);}
int game_save(void){
 if(!g.active)return 1;
 Save s;memset(&s,0,sizeof(s));s.magic=0x4e415243;s.version=1;s.mission=g.mission;s.step=g.step;s.cash=g.cash;s.reputation=g.reputation;s.ending=g.ending;s.x=g.x;s.y=g.y;s.health=g.health;s.playtime=g.playtime;s.caches=g.caches;s.jobs=g.jobs;s.station=g.station;s.check=savecheck(&s);
 if(s.mission<36&&s.step>=missions[s.mission].count){s.mission++;s.step=0;s.check=savecheck(&s);}
 char tmp[300],bak[300];snprintf(tmp,sizeof(tmp),"%s.tmp",g.savepath);snprintf(bak,sizeof(bak),"%s.bak",g.savepath);
 FILE *f=fopen(tmp,"wb");if(!f){g.saveOK=0;return 0;}int ok=fwrite(&s,1,sizeof(s),f)==sizeof(s);if(fclose(f))ok=0;
 if(!ok){remove(tmp);g.saveOK=0;return 0;}
 remove(bak);rename(g.savepath,bak);if(rename(tmp,g.savepath)){rename(bak,g.savepath);g.saveOK=0;return 0;}g.saveOK=1;return 1;
}
static int loadfile(const char *p){Save s;FILE *f=fopen(p,"rb");if(!f)return 0;size_t n=fread(&s,1,sizeof(s),f);fclose(f);
 if(n!=sizeof(s)||s.magic!=0x4e415243||s.version!=1||s.check!=savecheck(&s)||s.mission<0||s.mission>36||s.step<0||s.step>=6||s.cash<0||s.cash>100000000||s.station<0||s.station>4||!isfinite(s.x)||!isfinite(s.y)||!isfinite(s.health)||!isfinite(s.playtime)||s.playtime<0)return 0;
 if(s.mission<36&&s.step>=missions[s.mission].count)return 0;
 g.mission=s.mission;g.step=s.step;g.cash=s.cash;g.reputation=s.reputation;g.ending=s.ending;g.x=clampf(s.x,10,WORLD_W-10);g.y=clampf(s.y,10,WORLD_H-10);g.health=clampf(s.health,1,100);g.playtime=s.playtime;g.caches=s.caches;g.jobs=s.jobs;g.station=s.station;g.car=-1;g.heat=0;g.side=0;g.raceTime=0;g.missionTimer=0;g.active=1;return 1;
}
static int load_game(void){if(loadfile(g.savepath))return 1;char b[300];snprintf(b,sizeof(b),"%s.bak",g.savepath);return loadfile(b);}
static void dialog(const char *who,const char *s,int action){snprintf(g.speaker,sizeof(g.speaker),"%s",who);snprintf(g.dialog,sizeof(g.dialog),"%s",s);g.dialogAction=action;g.screen=DIALOG;g.screenT=0;}
static void start_mission(void){g.missionTimer=0;g.checkpoint=0;g.raceTime=0;g.seenIntro=1;if(g.mission<36)dialog(missions[g.mission].who,missions[g.mission].intro,0);}
static void fresh_game(void){g.active=1;g.mission=0;g.step=0;g.cash=350;g.reputation=0;g.ending=0;g.x=62;g.y=1022;g.health=100;g.heat=0;g.car=-1;g.station=0;g.caches=0;g.jobs=0;g.side=0;g.playtime=0;world_init();start_mission();}
static void advance(void){
 if(g.side){g.cash+=180;g.jobs++;g.reputation++;g.side=0;g.raceTime=0;notice("ENCARGO COMPLETO  +$180  +1 reputacion");g.screen=WORLD;game_save();return;}
 g.step++;g.checkpoint=0;g.raceTime=0;g.missionTimer=0;g.screen=WORLD;
 if(g.step>=missions[g.mission].count){g.cash+=missions[g.mission].reward;g.reputation+=3;dialog("MISION COMPLETADA",missions[g.mission].outro,2);}else{notice("Objetivo completado. Consulta la nueva marca amarilla.");game_save();}
}
static void end_dialog(void){int a=g.dialogAction;g.screen=WORLD;if(a==1)advance();if(a==2){g.mission++;g.step=0;game_save();if(g.mission<36)start_mission();else{g.heat=0;dialog("NARCADE / FIN DE LA HISTORIA",g.ending==1?"Vera entrega el expediente a la justicia. Los vecinos conservan sus datos. Sara vuelve a casa. La historia termina, pero la ciudad sigue abierta: encuentra los 24 vinilos, realiza encargos y recorre Medellin. made by Naresz.":"Mara publica las pruebas sin exponer los datos privados. Los barrios guardan copias y vigilan su ciudad. Sara vuelve a casa. La historia termina, pero puedes seguir explorando, reunir los 24 vinilos y realizar encargos. made by Naresz.",0);}}}
void game_init(void){memset(&g,0,sizeof(g));g.screen=TITLE;g.car=-1;g.health=100;g.cash=350;g.x=62;g.y=1022;strcpy(g.savepath,"NARCADE.SAV");world_init();}

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
 if(g.mission<36&&!g.side){const Step *s=step_now();const Location *l=&locations[s->loc];if(dist(g.x,g.y,l->x,l->y)<58){
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
 if(dist(g.x,g.y,locations[1].x,locations[1].y)<65){int cost=g.car>=0?100:30;if(g.cash>=cost){g.cash-=cost;g.health=100;if(g.car>=0)g.cars[g.car].hp=100;g.heat=0;notice("Luna: listo. Motor reparado y carro repintado.");game_save();}else notice("Necesitas $100 para reparar el carro, o $30 a pie.");return;}
 if(dist(g.x,g.y,locations[0].x,locations[0].y)<65){g.health=100;g.heat=0;notice(game_save()?"REFUGIO: salud recuperada y partida guardada.":"No se pudo guardar. Revisa el espacio de la Memory Stick.");return;}
 if(dist(g.x,g.y,locations[12].x,locations[12].y)<65){if(g.car>=0&&!g.side&&!g.raceTime){race_start(1);return;}notice("ENCARGOS: llega en carro para hacer seis entregas ($180).");return;}
 notice("Acercate al objetivo amarillo, un vinilo o un servicio.");
}

static void enter_exit(void){
 if(g.car>=0){Car *c=&g.cars[g.car];if(fabsf(c->speed)>45){notice("Frena antes de bajar del carro.");return;}float xx=c->x+cosf(c->a+PI*.5f)*23,yy=c->y+sinf(c->a+PI*.5f)*23;if(!free_at(xx,yy,5)){xx=c->x-cosf(c->a+PI*.5f)*23;yy=c->y-sinf(c->a+PI*.5f)*23;}if(!free_at(xx,yy,5)){notice("No hay espacio para bajar. Mueve el carro.");return;}g.x=xx;g.y=yy;c->speed=0;c->parked=1;g.car=-1;return;}
 int best=-1;float d=43;for(int i=0;i<CAR_COUNT;i++){float dd=dist(g.x,g.y,g.cars[i].x,g.cars[i].y);if(dd<d&&g.cars[i].hp>0){best=i;d=dd;}}
 if(best>=0){g.car=best;g.x=g.cars[best].x;g.y=g.cars[best].y;g.cars[best].parked=0;
  if(best!=1){g.heat=fmaxf(g.heat,g.cars[best].police?3:1.2f);notice("CARRO TOMADO. X acelera / [] frena / L-R radio.");}else notice("Luna: cuidalo. X acelera / [] frena / L-R radio.");
 }else notice("Acercate a un carro. TRIANGULO para tomarlo.");
}
static void world_tick(float ax,float ay,float dt){
 if(pressed(B_START)){g.screen=PAUSE;g.menu=0;return;}if(pressed(B_SELECT)){g.screen=MAP;g.mapSel=g.mission<36?step_now()->loc:0;return;}
 if(pressed(B_TRI))enter_exit();
 if(pressed(B_L))g.station=wrapi(g.station-1,5);if(pressed(B_R))g.station=(g.station+1)%5;
 if(pressed(B_CIRCLE)){g.screen=JOURNAL;g.journalPage=0;return;}
 if(g.car<0){float dx=ax+(held(B_RIGHT)-held(B_LEFT)),dy=ay+(held(B_DOWN)-held(B_UP));float n=sqrtf(dx*dx+dy*dy);if(n>.1f){dx/=fmaxf(1,n);dy/=fmaxf(1,n);g.a=atan2f(dy,dx);float speed=held(B_CROSS)?111:72;
  if(free_at(g.x+dx*speed*dt,g.y,5))g.x+=dx*speed*dt;if(free_at(g.x,g.y+dy*speed*dt,5))g.y+=dy*speed*dt;}
 }else{
  Car *c=&g.cars[g.car];float steer=clampf(ax+held(B_RIGHT)-held(B_LEFT),-1,1);
  if(held(B_CROSS))c->speed+=130*dt;else if(held(B_SQUARE))c->speed-=190*dt;else c->speed*=powf(.44f,dt);
  c->speed=clampf(c->speed,-72,220+(c->type==4?32:0));if(c->hp<25)c->speed=clampf(c->speed,-50,120);
  c->a+=steer*dt*(1.4f+fabsf(c->speed)/130)*(c->speed<0?-1:1)*clampf(fabsf(c->speed)/25,0,1);
  float xx=c->x+cosf(c->a)*c->speed*dt,yy=c->y+sinf(c->a)*c->speed*dt;
  if(free_at(xx,yy,10)){c->x=xx;c->y=yy;}else{c->hp-=fabsf(c->speed)*.025f;c->speed*=-.24f;g.hitCD=.12f;}
  for(int i=0;i<CAR_COUNT;i++)if(i!=g.car&&dist(c->x,c->y,g.cars[i].x,g.cars[i].y)<21&&g.hitCD<=0){float impact=fabsf(c->speed);c->hp-=impact*.035f;c->speed*=-.3f;g.hitCD=.7f;g.heat=fminf(5,g.heat+.4f);}
  g.x=c->x;g.y=c->y;
  if(c->hp<=0){c->hp=20;c->speed=0;g.car=-1;g.health-=25;g.x=c->x;g.y=c->y;notice("Motor averiado. Busca otro carro o ve al taller.");}
 }
 for(int i=0;i<CAR_COUNT;i++){
  if(i==g.car)continue;Car *c=&g.cars[i];if(c->parked&&!c->police)continue;
  if(c->police&&g.heat>0){
   float dd=dist(g.x,g.y,c->x,c->y);
   // Pursuers use the street grid, choosing the next junction toward the player.
   float jx=floorf(c->x/320)*320+42,jy=floorf(c->y/320)*320+42;
   if(dist(c->x,c->y,jx,jy)<10){float dx=g.x-c->x,dy=g.y-c->y;
    if(fabsf(dx)>fabsf(dy))c->a=dx>0?0:PI;else c->a=dy>0?PI*.5f:-PI*.5f;
   }
   if(dd<130){float desired=atan2f(g.y-c->y,g.x-c->x);float xx=c->x+cosf(desired)*dt*105,yy=c->y+sinf(desired)*dt*105;if(free_at(xx,yy,9))c->a=desired;}
   c->speed=95+g.heat*12;
   if(dd<25&&g.hitCD<=0){g.health-=g.car<0?12:4;if(g.car>=0)g.cars[g.car].hp-=5;g.hitCD=1;}
  }else{c->speed=c->police?45:48+i%4*9;
   float lx=fmodf(c->x,320),ly=fmodf(c->y,320);
   if(fabsf(lx-42)<2&&fabsf(ly-42)<2&&((int)(g.clock*10)+i)%4==0)c->a+=PI*.5f;
  }
  float xx=c->x+cosf(c->a)*c->speed*dt,yy=c->y+sinf(c->a)*c->speed*dt;
  if(free_at(xx,yy,9)){c->x=xx;c->y=yy;}else c->a+=PI*.5f;
 }
 for(int i=0;i<42;i++){Ped *p=&g.peds[i];p->y+=p->v*dt;int local=(int)p->y%320;if(local<91||local>283)p->v=-p->v;}
 if(g.heat>0){float nearest=100000;for(int i=60;i<64;i++)if(i!=g.car)nearest=fminf(nearest,dist(g.x,g.y,g.cars[i].x,g.cars[i].y));if(nearest>210){g.escape+=dt;if(g.escape>4)g.heat=fmaxf(0,g.heat-dt*.18f);}else g.escape=0;}
 if(g.mission<36&&step_now()->kind==K_CHASE&&g.missionTimer>0){g.missionTimer+=dt;if(g.missionTimer>step_now()->par&&g.heat<.01f)advance();}
 if(g.raceTime>0){g.raceTime-=dt;int l=g.route[g.checkpoint];if(g.car>=0&&dist(g.x,g.y,locations[l].x,locations[l].y)<66){g.checkpoint++;g.raceTime+=3;if(g.checkpoint==6){g.raceTime=0;advance();}else notice("PUNTO ALCANZADO +3 segundos. Sigue el siguiente aro.");}
  if(g.raceTime<=0&&g.checkpoint<6){g.raceTime=0;g.side=0;g.checkpoint=0;notice("Tiempo agotado. Vuelve al inicio para repetir el recorrido.");}
 }
 if(g.health<=0){g.health=100;g.heat=0;g.car=-1;g.x=locations[14].x;g.y=locations[14].y;g.cash=g.cash>100?g.cash-100:0;g.raceTime=0;g.side=0;g.missionTimer=0;notice("HOSPITAL: te recuperaste. El objetivo sigue disponible.");game_save();}
 if(pressed(B_SQUARE)&&g.car<0)interact();
 if(pressed(B_UP)&&g.car>=0)interact();
}

void game_tick(unsigned buttons,float ax,float ay,float dt){
 dt=clampf(dt,.001f,.05f);g.pressed=buttons&~g.prev;g.held=buttons;g.prev=buttons;g.clock+=dt;g.screenT+=dt;g.noticeT=fmaxf(0,g.noticeT-dt);g.hitCD=fmaxf(0,g.hitCD-dt);
 if(fabsf(ax)<.18f)ax=0;if(fabsf(ay)<.18f)ay=0;
 if(g.screen==TITLE){if(pressed(B_UP)||pressed(B_DOWN))g.menu=1-g.menu;if(pressed(B_CROSS)){if(g.menu==0){if(load_game()){g.screen=WORLD;notice("Partida cargada. SELECT mapa / O cuaderno.");}else fresh_game();}else{g.menu=0;dialog("NUEVA HISTORIA","Empezar una historia nueva sustituye el progreso anterior al guardar. X confirma. Pulsa O para volver al inicio.",3);}}}
 else if(g.screen==DIALOG){if(g.dialogAction==3&&pressed(B_CIRCLE))g.screen=TITLE;else if(pressed(B_CROSS)&&g.screenT>.12f){if(g.dialogAction==3)fresh_game();else end_dialog();}}
 else if(g.screen==WORLD){g.playtime+=dt;world_tick(ax,ay,dt);}
 else if(g.screen==MINI){g.playtime+=dt;puzzle_tick(ax,ay,dt);}
 else if(g.screen==MAP){if(pressed(B_SELECT)||pressed(B_CIRCLE))g.screen=WORLD;if(pressed(B_LEFT)||pressed(B_UP))g.mapSel=wrapi(g.mapSel-1,30);if(pressed(B_RIGHT)||pressed(B_DOWN))g.mapSel=(g.mapSel+1)%30;}
 else if(g.screen==JOURNAL){if(pressed(B_CIRCLE))g.screen=WORLD;if(pressed(B_R)||pressed(B_RIGHT))g.journalPage=(g.journalPage+1)%3;if(pressed(B_L)||pressed(B_LEFT))g.journalPage=wrapi(g.journalPage-1,3);}
 else if(g.screen==PAUSE){
  if(pressed(B_UP))g.menu=wrapi(g.menu-1,5);if(pressed(B_DOWN))g.menu=(g.menu+1)%5;
  if(pressed(B_START)||pressed(B_CIRCLE))g.screen=WORLD;
  if(pressed(B_CROSS)){switch(g.menu){case 0:g.screen=WORLD;break;case 1:notice(game_save()?"Partida guardada.":"No se pudo guardar. Revisa la Memory Stick.");g.screen=WORLD;break;case 2:g.car=-1;g.x=62;g.y=1022;g.health=100;g.heat=0;g.raceTime=0;g.side=0;g.missionTimer=0;g.screen=WORLD;notice("De vuelta en el refugio. Conservas el progreso.");break;case 3:g.screen=JOURNAL;g.journalPage=2;break;case 4:if(game_save())g.screen=TITLE;else notice("No se pudo guardar. Elige volver si deseas continuar.");break;}}
 }else if(g.screen==CHOICE){if(pressed(B_UP)||pressed(B_DOWN))g.menu=1-g.menu;if(pressed(B_CROSS)){g.ending=g.menu+1;advance();}}
 int rhythm=(g.screen==MINI&&g.p.kind==K_RHYTHM);audioStation=rhythm?3:g.station;audioEnabled=(g.car>=0&&g.station<4&&g.screen!=TITLE)||rhythm;
 if(rhythm)audioPos=(unsigned)(g.p.t*44100)%(track_length[3]*2);
}

static void car_draw(const Car *c,float ox,float oy,int chosen){
 int x=(int)(c->x-ox),y=(int)(c->y-oy);if(x<-32||y<-32||x>512||y>300)return;
 float ca=cosf(c->a),sa=sinf(c->a);uint32_t color=c->police?RGB(41,71,87):carcolors[c->type];
 for(int yy=-21;yy<=21;yy++)for(int xx=-21;xx<=21;xx++){float u=xx*ca+yy*sa,v=-xx*sa+yy*ca;if(fabsf(u)<19&&fabsf(v)<10)rect(x+xx+2,y+yy+3,1,1,RGB(13,23,27));}
 for(int yy=-21;yy<=21;yy++)for(int xx=-21;xx<=21;xx++){float u=xx*ca+yy*sa,v=-xx*sa+yy*ca;uint32_t col=color;int draw=0;
  if(fabsf(u)<18&&fabsf(v)<9){draw=1;if((u>5&&u<10)||(u<-6&&u>-11))col=RGB(21,48,62);if(fabsf(v)>7&&fabsf(u)>11)col=RGB(19,24,27);if(u>15&&fabsf(v)>4)col=GOLD;if(u<-15&&fabsf(v)>4)col=CORAL;}
  if(c->police&&fabsf(u)<2&&fabsf(v)<8){draw=1;col=(v>0)?TEAL:CORAL;}
  if(draw)rect(x+xx,y+yy,1,1,col);
 }
 if(chosen)circle(x,y,2,LIME);
}
static void person_draw(float wx,float wy,float ox,float oy,int player,int n){int x=wx-ox,y=wy-oy;if(x<-10||y<-10||x>490||y>282)return;circle(x+2,y+4,5,RGB(27,36,37));rect(x-3,y-3,6,9,player?TEAL:carcolors[n%6]);rect(x-2,y+5,2,3,INK);rect(x+1,y+5,2,3,INK);circle(x,y-4,3,RGB(208,154,115));if(player)rect(x-2,y-7,5,2,INK);}
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
static void hud(void){char b[180];rect(0,0,W,31,INK);text(11,7,"NARCADE",LIME,1);snprintf(b,sizeof(b),"$%d",g.cash);text(82,7,b,WHITE,1);
 text(178,7,districts[district(g.x,g.y)],MUTED,1);for(int i=0;i<5;i++)rect(396+i*15,8,10,10,g.heat>i?CORAL:RGB(47,62,66));
 rect(10,24,110,3,RGB(66,67,65));rect(10,24,(int)(g.health*1.1f),3,TEAL);
 if(g.car>=0){rect(123,24,100,3,RGB(66,67,65));rect(123,24,g.cars[g.car].hp,3,GOLD);}
 if(g.mission<36){snprintf(b,sizeof(b),"%02d/36  %s",g.mission+1,missions[g.mission].title);label(12,37,b,WHITE);
  const Step *s=step_now();if(dist(g.x,g.y,locations[s->loc].x,locations[s->loc].y)<58){const char *act=s->kind==K_DRIVE?"ENTREGAR":s->kind==K_RACE?"INICIAR RUTA":s->kind==K_CHASE?"INICIAR HUIDA":s->kind==K_TALK||s->kind==K_ENDING?"HABLAR":"INTERACTUAR";snprintf(b,sizeof(b),"%s %s",g.car>=0?"ARRIBA":"[]",act);label(152,200,b,LIME);}}
 else label(12,37,"HISTORIA COMPLETA / CIUDAD ABIERTA",LIME);
 if(g.raceTime>0){snprintf(b,sizeof(b),"RUTA %d/6   %ds",g.checkpoint+1,(int)g.raceTime);label(290,57,b,GOLD);}
 else if(g.mission<36&&step_now()->kind==K_CHASE&&g.missionTimer>0){snprintf(b,sizeof(b),"ALEJATE Y PIERDE LA BUSQUEDA / %ds",(int)fmaxf(0,step_now()->par-g.missionTimer));label(12,57,b,CORAL);}
 rect(0,218,W,54,INK);
 if(g.mission<36){const Step *s=step_now();int target=g.raceTime>0?g.route[g.checkpoint]:s->loc;snprintf(b,sizeof(b),"%s / %dm",locations[target].name,(int)dist(g.x,g.y,locations[target].x,locations[target].y));text(11,222,b,LIME,1);}
 else text(11,222,"Explora. Reune vinilos. Encargos en el mercado.",LIME,1);
 if(g.noticeT>0)textwrap(11,237,460,g.notice,WHITE);
 else if(g.car>=0){text(11,237,"X gas  [] freno  TRI salir  ARRIBA interactuar",WHITE,1);text(11,252,g.station<4?track_names[g.station]:"RADIO APAGADA / L-R cambiar",MUTED,1);}
 else{text(11,237,"[] interactuar  TRI tomar carro  X correr",WHITE,1);text(11,252,"SELECT mapa   O cuaderno   START pausa",MUTED,1);}
}

static void signature(int x,int y){for(int yy=0;yy<74;yy++)for(int xx=0;xx<180;xx++){int a=signature_data[yy*180+xx];if(a>60)rect(x+xx,y+yy,1,1,a>160?LIME:MUTED);}}
static void title_draw(void){
 rect(0,0,W,H,INK);
 for(int x=0;x<W;x++){int y=79+(int)(sin(x*.017)*19+cos(x*.038)*13);rect(x,y,1,H-y,RGB(28,57,61));int y2=119+(int)(sin(x*.023+2)*18);rect(x,y2,1,H-y2,RGB(37,71,70));}
 for(int i=0;i<24;i++){int x=i*22-5,y=128+(i*31)%65;rect(x,y,19,150,RGB(20,38,44));for(int yy=y+6;yy<236;yy+=13)for(int xx=x+4;xx<x+17;xx+=7)if((xx+yy+i)%3)rect(xx,yy,3,4,(xx+i)%2?GOLD:RGB(111,153,135));}
 line(0,79,480,142,RGB(95,125,124));for(int i=0;i<3;i++){int x=75+i*164+(int)(g.clock*5)%164;int y=79+x*63/480;rect(x,y,1,8,MUTED);rect(x-7,y+8,15,11,TEAL);rect(x-5,y+10,4,4,INK);rect(x+1,y+10,4,4,INK);}
 label(24,16,"UNA HISTORIA ORIGINAL EN MEDELLIN",TEAL);
 text(22,36,"NARCADE",INK,6);text(18,31,"NARCADE",WHITE,6);
 rect(21,98,249,3,LIME);label(23,110,"CIUDAD ABIERTA / PSP / v1.0",WHITE);
 rect(14,169,222,76,INK);text(28,181,g.menu==0?"> CONTINUAR / EMPEZAR":"  CONTINUAR / EMPEZAR",g.menu==0?LIME:MUTED,1);text(28,204,g.menu==1?"> NUEVA HISTORIA":"  NUEVA HISTORIA",g.menu==1?LIME:MUTED,1);text(28,226,"X confirmar",TEAL,1);
 rect(288,165,178,80,INK);text(312,176,"made by",WHITE,1);signature(287,184);
 footer("36 misiones  /  7 minijuegos  /  radio original");
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
 char b[180];rect(0,0,W,H,INK);header("MEDELLIN / MAPA DE LA CIUDAD","Una interpretacion ficticia, sin escala geografica real");
 float sc=.079f;int ox=20,oy=62;
 for(int by=0;by<7;by++)for(int bx=0;bx<8;bx++){int x=ox+bx*320*sc,y=oy+by*320*sc;rect(x,y,24,24,RGB(54,71,73));rect(x+7,y+7,15,15,parkblock(bx,by)?RGB(67,107,73):RGB(96,108,100));}
 rect(ox+1396*sc,oy,5,177,TEAL);
 for(int i=0;i<30;i++){int x=ox+locations[i].x*sc,y=oy+locations[i].y*sc;circle(x,y,2,MUTED);if(i==g.mapSel)outline(x-4,y-4,9,9,WHITE);}
 if(g.mission<36){int l=g.raceTime>0?g.route[g.checkpoint]:step_now()->loc;circle(ox+locations[l].x*sc,oy+locations[l].y*sc,4,LIME);}
 circle(ox+g.x*sc,oy+g.y*sc,3,CORAL);
 text(245,61,"LUGAR SELECCIONADO",TEAL,1);textwrap(245,83,222,locations[g.mapSel].name,WHITE);
 snprintf(b,sizeof(b),"Distancia: %d m",(int)dist(g.x,g.y,locations[g.mapSel].x,locations[g.mapSel].y));text(245,111,b,MUTED,1);
 text(245,139,"AMARILLO: objetivo",LIME,1);text(245,156,"CORAL: tu posicion",CORAL,1);text(245,174,"S: refugio / guardar",MUTED,1);text(245,191,"T: taller / reparar",MUTED,1);text(245,208,"$: encargos / mercado",MUTED,1);
 footer("CRUCETA recorrer lugares  SELECT u O volver");
}
static void journal_draw(void){
 char b[180];rect(0,0,W,H,INK);header("CUADERNO DE NICO",g.journalPage==0?"La pista actual":g.journalPage==1?"Personas y progreso":"Controles y servicios");
 if(g.journalPage==0){if(g.mission<36){text(21,59,chapters[g.mission/6],TEAL,1);text(21,81,missions[g.mission].title,WHITE,1);textwrap(21,107,435,step_now()->text,LIME);textwrap(21,156,435,missions[g.mission].intro,MUTED);}else textwrap(22,72,430,"La historia termino. La ciudad sigue abierta. Puedes conducir, coleccionar los 24 vinilos y aceptar entregas en el mercado de Belen.",LIME);}
 else if(g.journalPage==1){
  snprintf(b,sizeof(b),"HISTORIA %d/36   VINILOS %d/24",g.mission,cachecount());text(21,62,b,LIME,1);snprintf(b,sizeof(b),"REPUTACION %d   ENCARGOS %d   TIEMPO %dh %02dm",g.reputation,g.jobs,(int)g.playtime/3600,((int)g.playtime/60)%60);text(21,82,b,MUTED,1);
  const char *lines[]={"Nico / Mensajero. Una firma falsa cambio su vida.","Sara / Hermana de Nico. Investiga a Prisma.","Vera / Periodista. Sigue los documentos.","Luna / Mecanica. Conoce cada ruta de la ciudad.","Mara / DJ de Radio Ladera. Escucha lo oculto.","Tiza / Artista. Las paredes guardan sus pistas."};for(int i=0;i<6;i++)text(21,115+i*20,lines[i],WHITE,1);
 }else{
  const char *lines[]={"A PIE: analogico/cruceta mover. X correr.","TRIANGULO: entrar o salir de un carro cercano.","[]: interactuar a pie. ARRIBA: desde el carro.","EN CARRO: X gas, [] freno/reversa, < > girar.","L/R: emisora. Hay cuatro, mas radio apagada.","O: cuaderno. SELECT: mapa. START: pausa.","TALLER: reparacion $100. REFUGIO: curar/guardar.","MERCADO: encargos en carro. VINILOS: [] recoger.","Guardado por objetivo. Pausa permite guardar.","Busqueda: alejate. Los minijuegos se reintentan."};for(int i=0;i<10;i++)text(21,58+i*18,lines[i],i%2?MUTED:WHITE,1);
 }
 footer("L/R paginas  O volver");
}
void game_draw(uint32_t *pixels,int stride){fb=pixels;pitch=stride;
 if(g.screen==TITLE){title_draw();return;}if(g.screen==MINI){mini_draw();return;}if(g.screen==MAP){map_draw();return;}if(g.screen==JOURNAL){journal_draw();return;}
 world_draw();hud();
 if(g.screen==DIALOG){rect(16,70,448,144,INK);outline(16,70,448,144,RGB(67,92,93));rect(16,70,4,144,LIME);text(31,83,g.speaker,TEAL,1);textwrap(31,106,416,g.dialog,WHITE);text(31,196,g.dialogAction==3?"X empezar de nuevo  O cancelar":"X continuar",LIME,1);}
 if(g.screen==PAUSE){rect(99,56,282,179,INK);text(119,69,"NARCADE / PAUSA",TEAL,1);const char *items[]={"Volver a la ciudad","Guardar partida","Volver al refugio","Ver controles","Guardar y volver al titulo"};for(int i=0;i<5;i++){if(i==g.menu)rect(111,96+i*26,258,23,PANEL);text(121,102+i*26,items[i],i==g.menu?LIME:MUTED,1);}}
 if(g.screen==CHOICE){rect(16,62,448,173,INK);text(30,76,"TU DECISION / EL FUTURO DEL EXPEDIENTE",TEAL,1);textwrap(30,98,412,"Las dos opciones protegen los datos privados. Elige quien llevara las pruebas a la ciudad.",MUTED);const char *items[]={"VERA: entregar el expediente a la justicia","MARA: publicar tambien una memoria vecinal"};for(int i=0;i<2;i++){rect(25,146+i*35,429,28,i==g.menu?PANEL:INK);text(31,154+i*35,items[i],i==g.menu?LIME:WHITE,1);}text(31,219,"ARRIBA/ABAJO elegir  X confirmar",MUTED,1);}
}
void game_audio(short *stereo,unsigned frames){
 int station=audioStation;if(station>3)station=0;if(station!=audioLast){audioPos=0;audioLast=station;}
 for(unsigned i=0;i<frames;i++){short s=audioEnabled?radio_data[track_start[station]+audioPos/2]:0;stereo[i*2]=s;stereo[i*2+1]=s;audioPos++;if(audioPos>=(unsigned)track_length[station]*2)audioPos=0;}
}
