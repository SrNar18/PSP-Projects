/* Integration checks. These helpers are not compiled into the PSP game. */
#include <assert.h>
#include "../src/game.c"
#ifdef _WIN32
#undef assert
#define assert(x) do { if(!(x)){fprintf(stderr,"FAIL: %s at %s:%d\n",#x,__FILE__,__LINE__);exit(1);} } while(0)
#endif
static uint32_t frame[W*H];
static void snap(const char *name){game_draw(frame,W);char path[256];snprintf(path,sizeof(path),"build/%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n480 272\n255\n");for(int i=0;i<W*H;i++){unsigned char rgb[3]={frame[i]&255,(frame[i]>>8)&255,(frame[i]>>16)&255};fwrite(rgb,1,3,f);}fclose(f);}
static void tick(unsigned b){game_tick(b,0,0,1.0f/60);}
static void tap(unsigned b){tick(0);tick(b);tick(0);}
static void finishdialog(void){for(int i=0;i<12;i++)tick(0);tap(B_CROSS);}
static void solve(void){
 Puzzle *p=&g.p;int kind=p->kind;
 if(kind==K_CODE){for(int k=0;k<4;k++){while(p->digits[p->cursor]!=p->answer[p->cursor])tap(B_UP);if(k<3)tap(B_RIGHT);}tap(B_CROSS);}
 if(kind==K_CIRCUIT){int want[]={10,10,10,12,6,10,10,9,3,10,10,12,10,10,10,3};for(int i=0;i<16&&g.screen==MINI;i++){int tries=0;while(p->tiles[p->cursor]!=want[p->cursor]&&g.screen==MINI){tap(B_CROSS);assert(++tries<=4);}if(g.screen==MINI)tap(B_RIGHT);}}
 if(kind==K_MEMORY){while(g.screen==MINI){int n=4+p->phase;while(p->t<n*.8f+1.1f)tick(0);int keys[]={B_UP,B_RIGHT,B_DOWN,B_LEFT};for(int i=0;i<n&&g.screen==MINI;i++)tap(keys[p->seq[i]]);}}
 if(kind==K_TUNE){for(int i=0;i<2500&&g.screen==MINI;i++){float target=p->target+sinf((p->t+1.0f/60)*.75f)*4;float delta=target-p->value;game_tick(B_CROSS,clampf(delta*2.5f,-1,1),0,1.0f/60);}assert(g.screen==DIALOG);}
 if(kind==K_LOCK){for(int i=0;i<8000&&g.screen==MINI;i++){float val=(sinf((p->t+1.0f/60)*(2.0f+p->phase*.35f))+1)*50;tick(fabsf(val-p->target)<6&&!(g.prev&B_CROSS)?B_CROSS:0);}assert(g.screen==DIALOG);}
 if(kind==K_RHYTHM){int keys[]={B_LEFT,B_UP,B_DOWN,B_RIGHT};for(int i=0;i<24;i++){float at=2+i*.57f;while(p->t<at-.018f)tick(0);tap(keys[p->notes[i]]);}while(g.screen==MINI&&p->t<17)tick(0);assert(g.screen==DIALOG);}
 if(kind==K_STEALTH){for(int i=0;i<15000&&g.screen==MINI;i++){int next=(int)((p->x-77)/68);if(p->x<77)next=0;if(next>4)next=4;float cx=90+next*68;float cy=150+sinf((p->t+.08f)*(.85f+next*.08f)+next*1.7f+p->seed)*47;float dy=cy-p->y;float dx=1;if(fabsf(dy)>16&&p->x<cx-14)dx=0;game_tick(0,dx,clampf(dy*.15f,-1,1),1.0f/60);}assert(g.screen==DIALOG);}
 assert(g.screen==DIALOG);
}
int main(void){
 game_init();game_set_save_path("build/QA.SAV");remove(g.savepath);remove("build/QA.SAV.bak");snap("01-title");
 assert(game_save()==1);assert(fopen(g.savepath,"rb")==NULL); // title must never destroy an existing save
 fresh_game();finishdialog();snap("02-city");
 // Every target has open access for a player and a vehicle.
 for(int i=0;i<30;i++)assert(free_at(locations[i].x,locations[i].y,10));
 // Car acquisition, actual acceleration, turning, collision and exit.
 g.x=g.cars[1].x-24;g.y=g.cars[1].y;tap(B_TRI);assert(g.car==1);float sx=g.x;
 for(int i=0;i<50;i++)tick(B_CROSS);assert(g.x>sx+20);snap("03-driving");
 g.cars[1].speed=0;tap(B_TRI);assert(g.car==-1);
 // Round trip and recovery from a corrupted primary save.
 g.cash=1234;assert(game_save());g.cash=2345;assert(game_save());FILE *f=fopen(g.savepath,"wb");fputs("damaged",f);fclose(f);g.cash=0;assert(load_game());assert(g.cash==1234);
 g.screen=WORLD;tap(B_SELECT);assert(g.screen==MAP);snap("04-map");tap(B_SELECT);
 int total=0,miniCounts[13]={0};
 for(int m=0;m<36;m++){
  g.mission=m;g.step=0;g.screen=WORLD;g.health=100;g.heat=0;g.side=0;g.raceTime=0;
  for(int s=0;s<missions[m].count;s++){
   assert(g.mission==m&&g.step==s);const Step *st=step_now();int kind=st->kind;g.car=-1;g.x=locations[st->loc].x;g.y=locations[st->loc].y;g.screen=WORLD;
   if(kind==K_DRIVE||kind==K_RACE||kind==K_CHASE){g.car=1;g.cars[1].x=g.x;g.cars[1].y=g.y;g.cars[1].speed=0;}
   interact();miniCounts[kind]++;
   if(kind>=K_CODE&&kind<=K_STEALTH){assert(g.screen==MINI);if(miniCounts[kind]==1){char name[40];snprintf(name,sizeof(name),"mini-%d",kind);snap(name);}solve();finishdialog();}
   else if(kind==K_RACE){assert(g.raceTime>0);for(int i=0;i<6;i++){int l=g.route[g.checkpoint];g.x=g.cars[1].x=locations[l].x;g.y=g.cars[1].y=locations[l].y;tick(0);}assert(g.raceTime==0);}
   else if(kind==K_CHASE){assert(g.missionTimer>0);g.heat=0;g.hitCD=1;g.missionTimer=st->par+1;tick(0);}
   else if(kind==K_CHOICE){assert(g.screen==CHOICE);tap(B_DOWN);tap(B_CROSS);assert(g.ending==2);}
   else if(kind==K_TALK||kind==K_ENDING)finishdialog();
   total++;
   if(s==missions[m].count-1){assert(g.screen==DIALOG&&g.dialogAction==2);assert(game_save());assert(load_game());assert(g.mission==m+1&&g.step==0);g.screen=WORLD;}else {if(g.step!=s+1)fprintf(stderr,"mission=%d step=%d kind=%d actual=%d car=%d pos=%f,%f\n",m,s,kind,g.step,g.car,g.x,g.y);assert(g.step==s+1);}
  }
 }
 assert(total==146&&g.mission==36);printf("PASS: 36 missions, %d objectives; all seven puzzle families solved through input.\n",total);
 for(int i=2;i<=8;i++)printf("  mini kind %d: %d solved\n",i,miniCounts[i]);
 g.screen=WORLD;g.car=1;g.x=locations[12].x;g.y=locations[12].y;g.cars[1].x=g.x;g.cars[1].y=g.y;interact();assert(g.side&&g.raceTime>0);int cash=g.cash;for(int i=0;i<6;i++){int l=g.route[g.checkpoint];g.x=g.cars[1].x=locations[l].x;g.y=g.cars[1].y=locations[l].y;tick(0);}assert(g.jobs==1&&g.cash==cash+180);
 short samples[2048];audioStation=1;audioEnabled=1;game_audio(samples,1024);int nonzero=0;for(int i=0;i<2048;i++)nonzero|=samples[i];assert(nonzero);
 printf("PASS: native simulation, saves, corrupted-save recovery, car controls, side job and PCM audio.\n");
 // Random input with ASan/UBSan enabled catches indexing and render bounds issues.
 for(int i=0;i<12000;i++){unsigned keys[]={0,B_START,B_SELECT,B_CIRCLE,B_TRI,B_CROSS,B_SQUARE,B_UP,B_DOWN,B_LEFT,B_RIGHT,B_L,B_R};game_tick(keys[random_u()%13],(int)(random_u()%3)-1,(int)(random_u()%3)-1,1.0f/30);if(i%13==0)game_draw(frame,W);}
 puts("PASS: 12,000 randomized input frames (sanitizers when enabled by the compiler).");return 0;
}
