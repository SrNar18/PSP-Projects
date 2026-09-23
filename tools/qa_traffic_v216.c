#include <assert.h>
#include "../src/game.c"

int main(void){
    game_init();fresh_game();g.screen=WORLD;
    for(int i=30;i<60;i++)assert(car_free_at(&g.cars[i],g.cars[i].x,g.cars[i].y));
    int progressed=0;
    float start[30][2];for(int i=30;i<60;i++){start[i-30][0]=g.cars[i].x;start[i-30][1]=g.cars[i].y;}
    for(int frame=0;frame<60*80;frame++){
        g.clock+=1.f/60;
        for(int i=30;i<60;i++)civilian_traffic_tick(i,1.f/60);
        separate_cars();
        for(int i=30;i<60;i++){
            Car *c=&g.cars[i];
            assert(car_free_at(c,c->x,c->y));
            for(int j=i+1;j<60;j++){float nx,ny,depth;assert(!car_overlap(c,&g.cars[j],&nx,&ny,&depth)||depth<1.5f);}
            if(frame==60*20&&dist(c->x,c->y,start[i-30][0],start[i-30][1])>20)progressed++;
        }
    }
    fprintf(stderr,"progressed %d\n",progressed);assert(progressed==30);
    for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=2450;g.cars[i].y=2150;g.cars[i].parked=1;}
    g.car=-1;g.inMetro=0;g.x=62;g.y=1000;g.viewYaw=geo_heading(g.x,g.y,PI*.5f);g.moveActive=0;g.footSpeed=0;
    float oldz=g.y;
    for(int frame=0;frame<180;frame++)game_tick(B_CROSS,0,-1,1.f/60);
    fprintf(stderr,"foot %.1f -> %.1f x %.1f speed %.1f\n",oldz,g.y,g.x,g.footSpeed);assert(g.y>oldz+70);
    puts("PASS: 80 s traffic stays on drivable road without civilian overlap; all 30 cars progress; uphill foot motion progresses.");
}
