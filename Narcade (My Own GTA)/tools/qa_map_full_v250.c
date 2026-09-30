#define main campaign_main
#include "qa.c"
#undef main

int main(void){
    game_init();fresh_game();g.screen=PAUSE;g.pauseTab=0;
    const float scale=.04765f;const int ox=58,oy=68;
    for(int row=0;row<7;row++)for(int side=0;side<2;side++){
        int x,y;map_point(side?WORLD_W-1:0,row*320+160,scale,ox,oy,&x,&y);
        assert(x>=ox&&x<ox+123&&y>=oy&&y<oy+161);
    }
    int x,y;map_point(WORLD_W-1,WORLD_H-1,scale,ox,oy,&x,&y);
    assert(x>=ox&&x<ox+123&&y>=oy&&y<oy+161);
    snap("map-full-v250");
    puts("PASS: northern, southern, eastern and western city boundaries fit the Start map.");
    return 0;
}
