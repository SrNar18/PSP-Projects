#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/game.c"
static void tap(unsigned button){game_tick(0,0,0,.02f);game_tick(button,0,0,.02f);game_tick(0,0,0,.02f);}
static void snapshot(const char *path){
    static uint32_t pixels[512*272];game_draw(pixels,512);FILE *f=fopen(path,"wb");assert(f);
    fprintf(f,"P6\n480 272\n255\n");for(int y=0;y<272;y++)for(int x=0;x<480;x++){
        uint32_t c=pixels[y*512+x];unsigned char rgb[3]={c&255,(c>>8)&255,(c>>16)&255};fwrite(rgb,1,3,f);}
    fclose(f);
}
int main(void){
    remove("build/settings-qa.sav.cfg");remove("build/settings-qa.sav.cfg.bak");
    game_init();game_set_save_path("build/settings-qa.sav");g.titleStage=1;
    snapshot("build/menu-es.ppm");tap(B_DOWN);tap(B_DOWN);assert(g.menu==2);tap(B_CROSS);assert(g.screen==SETTINGS);
    snapshot("build/settings-es.ppm");tap(B_CROSS);assert(game_language()==1);
    assert(!strcmp(game_localize("GUARDAR PARTIDA"),"SAVE GAME"));
    for(int i=0;i<36;i++){
        assert(strcmp(locale_text(missions[i].title),missions[i].title));
        assert(strcmp(locale_text(missions[i].intro),missions[i].intro));
        assert(strcmp(locale_text(missions[i].outro),missions[i].outro));
        for(int j=0;j<missions[i].count;j++)assert(strcmp(locale_text(missions[i].steps[j].text),missions[i].steps[j].text));
    }
    char line[200];snprintf(line,sizeof line,"Mision %02d/36 - %s",1,missions[0].title);
    assert(!strcmp(line,"Mission 01/36 - A delivery gone wrong"));
    snprintf(line,sizeof line,"SENAL %d%%  /  ENLACE %.1f de 4.0 s",75,2.5);
    assert(!strcmp(line,"SIGNAL 75%  /  LINK 2.5 of 4.0 s"));
    struct {char out[5];char guard;} small={{0},'Q'};int n=snprintf(small.out,sizeof small.out,"%s",missions[0].title);
    assert(n==(int)strlen("A delivery gone wrong")&&small.guard=='Q'&&small.out[4]==0);
    tap(B_DOWN);for(int i=0;i<15;i++)tap(B_RIGHT);assert(prefsBrightness==10);
    for(int i=0;i<15;i++)tap(B_LEFT);assert(prefsBrightness==0);for(int i=0;i<5;i++)tap(B_RIGHT);
    tap(B_DOWN);tap(B_LEFT);assert(prefsMusic==9);tap(B_DOWN);tap(B_DOWN);tap(B_RIGHT);assert(prefsFog==1);snapshot("build/settings-en.ppm");
    tap(B_CIRCLE);assert(g.screen==TITLE&&g.menu==2);snapshot("build/menu-en.ppm");
    settings_defaults();settings_load();assert(prefsLanguage==1&&prefsBrightness==5&&prefsMusic==9&&prefsFog==1);
    fresh_game();g.screen=WORLD;tap(B_START);assert(g.screen==PAUSE);
    tap(B_R);tap(B_R);tap(B_R);assert(g.pauseTab==3);tap(B_CROSS);assert(prefsLanguage==0);
    tap(B_DOWN);tap(B_RIGHT);assert(g.pauseTab==3&&prefsBrightness==6);
    tap(B_START);assert(g.screen==WORLD);settings_load();assert(prefsBrightness==6&&prefsLanguage==0);
    FILE *f=fopen("build/settings-qa.sav.cfg","wb");assert(f);fputs("invalid",f);fclose(f);settings_load();
    assert(!prefsLanguage&&prefsBrightness==5&&prefsMusic==10&&prefsEffects==10&&prefsFog==0);
    remove("build/settings-qa.sav.cfg");remove("build/settings-qa.sav.cfg.bak");
    puts("PASS: title/pause settings, all 36 missions in English, formats, truncation, brightness bounds, persistence and corrupt-config recovery.");
    return 0;
}
