#define main campaign_main
#include "qa.c"
#undef main
static void hold_l(float ax,float ay){for(int i=0;i<20;i++)game_tick(B_L,ax,ay,1.f/60);}
int main(void){
 game_init();fresh_game();finishdialog();g.screen=WORLD;
 game_tick(B_L,0,0,.05f);game_tick(0,0,0,.05f);assert(!g.weaponWheel&&g.weapon==0);
 for(int i=0;i<8;i++){
  float a=i*PI/4;hold_l(0,0);assert(g.weaponWheel);
  float x=g.x,y=g.y,stamina=g.stamina,time=g.playtime;
  hold_l(sinf(a),-cosf(a));assert(g.weaponChoice==i);
  assert(g.x==x&&g.y==y&&g.stamina==stamina&&g.playtime==time);
  hold_l(.05f,0);assert(g.weaponChoice==i);
  game_tick(0,0,0,1.f/60);assert(!g.weaponWheel&&g.weapon==i);
 }
 hold_l(0,0);snap("weapon-wheel");
 game_tick(B_START|B_L,0,0,.02f);assert(g.screen==PAUSE&&!g.weaponWheel&&g.weapon==7);
 g.screen=WORLD;g.car=0;hold_l(0,0);assert(!g.weaponWheel);
 g.car=-1;g.inMetro=1;hold_l(0,0);assert(!g.weaponWheel);
 puts("PASS: short press ignored; eight directions; centre deadzone; release equips; world frozen; pause cancels; no wheel in vehicles.");
 return 0;
}
