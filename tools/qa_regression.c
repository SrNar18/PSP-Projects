/* Run original campaign tests plus concrete 2.1 bug regressions. */
#define main campaign_main
#include "qa.c"
#undef main

static void isolated_cars(void){
 for(int i=0;i<CAR_COUNT;i++)g.cars[i]=(Car){-10000-i*100,-10000,0,0,100,0,1,0};
 g.car=-1;
}
static void require_separated(void){
 float nx,ny,d;
 assert(!car_overlap(&g.cars[0],&g.cars[1],&nx,&ny,&d)||d<.01f);
 assert(car_free_at(&g.cars[0],g.cars[0].x,g.cars[0].y));
 assert(car_free_at(&g.cars[1],g.cars[1].x,g.cars[1].y));
}
int main(void){
 assert(campaign_main()==0);
 game_init();fresh_game();finishdialog();game_set_save_path("build/QA-menu.SAV");
 tap(B_START);assert(g.screen==PAUSE&&g.pauseTab==0);
 float playtime=g.playtime,x=g.x;for(int i=0;i<60;i++)tick(B_UP);
 assert(g.playtime==playtime&&g.x==x);
 tap(B_R);assert(g.pauseTab==1);tap(B_DOWN);assert(g.journalPage==1);
 tap(B_R);assert(g.pauseTab==2);tap(B_CROSS);assert(g.screen==PAUSE&&g.saveOK);
 tap(B_CIRCLE);assert(g.screen==WORLD);
 puzzle_start(K_CODE,3);tick(0);float puzzleTime=g.p.t;
 tap(B_START);assert(g.screen==PAUSE&&g.pauseBack==MINI);float stopped=g.p.t;
 for(int i=0;i<120;i++)tick(0);assert(g.p.t==stopped);
 tap(B_START);assert(g.screen==MINI&&g.p.t>=puzzleTime&&g.p.t<stopped+.04f);
 g.screen=WORLD;tap(B_START);tap(B_L);assert(g.pauseTab==3);tap(B_CROSS);assert(g.screen==TITLE&&g.menu==0);
 puts("PASS: Start map, messages, save, return to title, pause/resume minigame.");

 isolated_cars();g.cars[0]=(Car){150,42,0,100,100,0,0,0};g.cars[1]=(Car){175,42,0,0,100,0,1,0};
 g.car=0;g.hitCD=10;separate_cars();require_separated();assert(g.x==g.cars[0].x);
 /* Identical centers must still produce a deterministic separation axis. */
 g.cars[0].x=g.cars[1].x=200;g.cars[0].y=g.cars[1].y=42;separate_cars();require_separated();
 /* One car pinned against the building: all separation goes to free car. */
 g.cars[0]=(Car){70,145,PI*.5f,0,100,0,1,0};g.cars[1]=(Car){85,145,PI*.5f,0,100,0,1,0};
 separate_cars();require_separated();
 for(int i=0;i<32;i++){
  g.cars[0]=(Car){160,42,i*PI/16,100,100,0,0,0};
  g.cars[1]=(Car){176,42,(i+5)*PI/16,50,100,0,0,0};
  separate_cars();require_separated();
 }
 puts("PASS: head-on, side, identical-center, pinned-wall and 32 rotated car contacts; cooldown does not block separation.");
 return 0;
}
