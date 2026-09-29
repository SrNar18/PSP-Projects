/* v2.44 (Claude): menu de pausa sin PARTIDA; SALIR con guardar y salir / salir sin guardar (con confirmacion). */
#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 game_init();fresh_game();g.screen=WORLD;tap(B_START);assert(g.screen==PAUSE&&g.pauseTab==0);
 for(int i=0;i<4;i++)tap(B_R);assert(g.pauseTab==4);snap("pause-exit");
 tap(B_DOWN);assert(g.menu==1);tap(B_CROSS);assert(pauseConfirm&&g.screen==PAUSE);snap("pause-confirm");
 tap(B_CIRCLE);assert(!pauseConfirm&&g.screen==PAUSE);                    /* O cancela */
 tap(B_CROSS);assert(pauseConfirm);tap(B_CROSS);assert(g.screen==TITLE);   /* X sale sin guardar */
 fresh_game();g.screen=WORLD;g.cash=777;tap(B_START);for(int i=0;i<4;i++)tap(B_R);tap(B_CROSS);assert(g.screen==TITLE); /* guardar y salir */
 g.cash=0;assert(load_game()&&g.cash==777);
 fresh_game();g.screen=WORLD;tap(B_START);tap(B_R);tap(B_R);snap("pause-settings");
 puts("PASS: pausa de 5 pestanas; guardar y salir guarda; salir sin guardar pide confirmacion y O cancela.");return 0;}
