#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/game.c"

static float run_segment(unsigned buttons,int frames){
    float startX,startZ,endX,endZ;
    physics_project(g.x,g.y,&startX,&startZ);
    for(int i=0;i<frames;i++){
        float x,z;physics_project(g.x,g.y,&x,&z);
        game_tick(buttons,0,-1,1.f/60);
        float xx,zz;physics_project(g.x,g.y,&xx,&zz);
        if(i>45)assert(hypotf(xx-x,zz-z)>.22f);
        assert(fabsf(angle_delta(g.viewYaw,PI*.5f))<.12f);
    }
    physics_project(g.x,g.y,&endX,&endZ);
    return hypotf(endX-startX,endZ-startZ)/(frames/60.f);
}
int main(void){
    game_init();fresh_game();g.screen=WORLD;g.viewYaw=PI*.5f;
    g.x=62;g.y=970;
    for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-1000-i*100;g.cars[i].y=-1000;g.cars[i].parked=1;g.cars[i].police=0;}
    float walk=run_segment(0,150);
    float jog=run_segment(B_CROSS,150);
    assert(walk>25&&walk<55);
    assert(jog>55&&jog<90);
    printf("PASS: walk %.1f and jog %.1f world units/s; continuous frames and stable camera.\n",walk,jog);
}
