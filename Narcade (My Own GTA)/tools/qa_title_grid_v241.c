/* v2.41 (Claude): menu principal en rejilla: ABAJO va directo a AJUSTES, ARRIBA vuelve a la tarjeta. */
#define main campaign_main
#include "qa.c"
#undef main
int main(void){
    game_init();tap(B_CROSS);assert(g.titleStage&&g.menu==0);
    tap(B_DOWN);assert(g.menu==2);            /* continuar -> ajustes, sin pasar por nueva */
    tap(B_UP);assert(g.menu==0);
    tap(B_RIGHT);assert(g.menu==1);tap(B_DOWN);assert(g.menu==2);tap(B_UP);assert(g.menu==1);
    tap(B_DOWN);tap(B_LEFT);assert(g.menu==0);tap(B_DOWN);tap(B_RIGHT);assert(g.menu==1);
    tap(B_DOWN);tap(B_CROSS);assert(g.screen==SETTINGS);
    puts("PASS: ABAJO lleva directo a AJUSTES; ARRIBA/IZQ/DER vuelven a las tarjetas.");return 0;
}
