#include <stdio.h>
#include <stdlib.h>
#include "../src/game.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s line %d\n",#x,__LINE__);exit(1);}}while(0)
static void setup(void){
 game_init();game_set_save_path("build/COMBAT_QA.SAV");fresh_game();g.screen=WORLD;g.x=1002;g.y=1060;g.viewYaw=geo_heading(g.x,g.y,PI*.5f);g.bodyYaw=g.viewYaw;g.a=PI*.5f;g.mission=36;
 for(int i=0;i<64;i++){g.cars[i].x=-5000-i*100;g.cars[i].y=-5000;g.cars[i].parked=1;}
 for(int i=0;i<42;i++){g.peds[i].x=-8000;g.peds[i].y=-8000;}
}
static void npc_ahead(float d,float side){float x,z;physics_project(g.x,g.y,&x,&z);physics_unproject(x+cosf(g.viewYaw)*d-sinf(g.viewYaw)*side,z+sinf(g.viewYaw)*d+cosf(g.viewYaw)*side,&g.peds[0].x,&g.peds[0].y);}
int main(void){
 setup();npc_ahead(35,0);g.weapon=1;CHECK(combat_line(g.x,g.y,g.peds[0].x,g.peds[0].y));
 combat_tick(0,0,.016f);g.held=B_R;combat_tick(0,0,.016f);CHECK(combat.aiming&&combat.target==0);
 g.held=B_R|B_CIRCLE;combat_tick(0,0,.016f);CHECK(combat.ped[0].health==74&&combat.ped[0].flee>0&&g.heat>0);
 combat_tick(0,0,.016f);CHECK(combat.ped[0].health==74);
 for(int i=0;i<3;i++)combat_tick(0,0,.3f);
 CHECK(combat.ped[0].health==0);combat_peds_tick(.5f);CHECK(combat.ped[0].fall>.5f);
 puts("PASS lock, damage, shot cooldown, fleeing, death and wanted escalation");
 setup();g.weapon=1;g.held=B_R;combat_tick(.8f,-.5f,.1f);CHECK(combat.target==-1&&combat.pitch>0);CHECK(isfinite(g.a));
 g.held=0;combat_tick(0,0,.016f);CHECK(!combat.aiming&&!g.stickActive);
 setup();npc_ahead(230,5);g.weapon=1;g.held=B_R;combat_tick(0,0,.016f);CHECK(combat.target==-1);
 float desired=(geo_height(g.peds[0].x,g.peds[0].y)+9-geo_height(g.x,g.y)-14)/(230+38);
 combat.pitch=atanf((desired*(60+38)+4)/60);
 g.held=B_R|B_CIRCLE;combat_tick(0,0,.016f);CHECK(combat.ped[0].health<100);
 puts("PASS free aiming, shoulder crosshair ray and releasing aim");
 setup();npc_ahead(8,0);g.weapon=0;g.held=B_R|B_CIRCLE;combat_tick(0,0,.016f);CHECK(combat.ped[0].health==82&&combat.punch>0);
 combat_tick(0,0,.43f);CHECK(combat.ped[0].health==64);
 setup();npc_ahead(26,0);g.weapon=0;g.held=B_R|B_CIRCLE;combat_tick(0,0,.016f);CHECK(combat.ped[0].health==100);
 puts("PASS held melee repeats, but fists cannot hit a distant pedestrian");
 setup();npc_ahead(90,0);float px,pz;physics_project(g.x,g.y,&px,&pz);physics_unproject(px+cosf(g.viewYaw)*45,pz+sinf(g.viewYaw)*45,&g.cars[0].x,&g.cars[0].y);g.cars[0].a=g.a;
 g.weapon=1;g.held=B_R|B_CIRCLE;combat_tick(0,0,.016f);CHECK(combat.ped[0].health==100);
 puts("PASS car between muzzle and pedestrian absorbs the shot");
 setup();const CmParcel *parcels;int n=cm_parcels(0,0,&parcels),found=0;
 for(int i=0;i<n;i++)if(parcels[i].kind==0){float x,z;cm_centroid(&parcels[i],&x,&z);CHECK(!combat_line(42,42,x,z));found=1;break;}CHECK(found);
 combat_jump();float maximum=0;for(int i=0;i<90;i++){combat_tick(0,0,1.f/60);maximum=fmaxf(maximum,combat.jump);}CHECK(maximum>6&&maximum<9&&combat.jump==0);
 game_tick(B_CIRCLE,0,0,.016f);CHECK(g.screen==JOURNAL);
 puts("PASS building blocks bullets, bounded jump lands, messages remain accessible");
 setup();CHECK(climbWallCount>0);float *w=climbWall[0];physics_unproject(w[2],w[3]-7,&g.x,&g.y);g.a=logical_heading_from_projected(g.x,g.y,PI*.5f);
 CHECK(foot_free(g.x,g.y));combat_jump();CHECK(combat.vault>0);
 for(int i=0;i<60;i++)combat_tick(0,0,1.f/60);
 CHECK(combat.vault==0&&combat.jump==0&&foot_free(g.x,g.y));physics_project(g.x,g.y,&px,&pz);CHECK(pz>w[3]);
 puts("PASS solid park wall can be climbed; landing is clear and the character returns to ground");
 setup();combat_jump();g.car=0;combat_tick(0,0,.016f);CHECK(combat.jump==0&&combat.vault==0);
 setup();combat.vault=.01f;combat.startX=g.x;combat.startY=g.y;combat.vaultX=g.x;combat.vaultY=g.y+8;g.health=0;
 game_tick(0,0,0,.016f);CHECK(combat.vault==0&&combat.jump==0);float hx=g.x,hy=g.y;game_tick(0,0,0,.016f);CHECK(dist(g.x,g.y,hx,hy)<1);
 puts("PASS vehicle boarding and hospital recovery cancel airborne/climbing state");
 return 0;
}
