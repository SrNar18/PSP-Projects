/* Five-panel navigation, child screens and trophy provider; stride/HUD safety. */
#define main campaign_main
#include "qa.c"
#undef main
static int sample(int i,GameTrophyInfo *out){*out=(GameTrophyInfo){"PRIMEROS PASOS","VIDA DE BARRIO",i?3:10,10,i==0};return 1;}
int main(void){
 game_init();tap(B_CROSS);tap(B_DOWN);assert(g.menu==3);tap(B_CROSS);assert(g.screen==CREDITS);snap("credits-v242");
 tap(B_START);assert(g.screen==TITLE&&g.menu==3);tap(B_RIGHT);assert(g.menu==4);tap(B_CROSS);assert(g.screen==TROPHIES);snap("trophies-v242-placeholder");
 tap(B_DOWN);assert(trophySelection==1);game_set_trophy_provider(sample);snap("trophies-v242-provider");
 tap(B_CIRCLE);assert(g.menu==4&&g.screen==TITLE);tap(B_UP);assert(g.menu==1);tap(B_RIGHT);assert(g.menu==2);tap(B_CROSS);assert(g.screen==SETTINGS);
 tap(B_CIRCLE);assert(g.menu==2);tap(B_LEFT);assert(g.menu==1);tap(B_LEFT);assert(g.menu==0);snap("menu-v242-es");
 prefsLanguage=1;snap("menu-v242-en");g.screen=CREDITS;snap("credits-v242-en");
 static uint32_t strided[512*272];game_draw(strided,512);for(int y=0;y<272;y++)for(int x=480;x<512;x++)assert(strided[y*512+x]==0);
 puts("PASS: five-panel spatial navigation, credits, trophy provider, English and 512-pixel stride.");return 0;
}
