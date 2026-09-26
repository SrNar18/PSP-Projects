#define main campaign_main
#include "qa.c"
#undef main

int main(void){
    game_init();
    assert(g.screen==TITLE&&!g.titleStage);
    snap("title-v213-cover");
    tap(B_CROSS);
    assert(g.screen==TITLE&&g.titleStage&&g.menu==0);
    snap("title-v213-menu");
    tap(B_RIGHT);assert(g.menu==1);
    snap("title-v213-new-selected");
    tap(B_CIRCLE);assert(g.screen==TITLE&&!g.titleStage);
    tap(B_START);assert(g.titleStage&&g.menu==0);
    tap(B_RIGHT);tap(B_CROSS);assert(g.screen==DIALOG&&g.dialogAction==3);
    tap(B_CIRCLE);assert(g.screen==TITLE);
    game_set_native_savedata(1);g.titleStage=1;g.menu=0;
    tap(B_CROSS);assert(game_take_request()==2);
    game_request_result(2,0);assert(g.screen==TITLE&&g.titleStage);
    puts("PASS: cover gate, START/X, card navigation, new-story confirmation and back.");
}
