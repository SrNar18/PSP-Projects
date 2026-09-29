/* v2.45 (Claude): el jugador pisa la superficie real (calzada, acera, puente, plaza, parque) y sube o baja
   los desniveles de forma suave; nunca queda hundido en lo elevado. */
#define main campaign_main
#include "qa.c"
#undef main
static float settle(float x,float y){g.x=x;g.y=y;for(int i=0;i<30;i++)surface_tick(1.f/60);return g.lift;}
int main(void){game_init();fresh_game();g.screen=WORLD;g.car=-1;g.inMetro=0;g.lift=0;
 assert(fabsf(settle(42,1022)-0)<.01f);                      /* calzada */
 float sx=0,sy=0;for(float x=86;x<100;x+=.5f)if(!cm_on_road(x,1100)&&cm_parcel_at(x,1100)<0){sx=x;sy=1100;break;}
 assert(sx>0&&fabsf(settle(sx,sy)-1.2f)<.01f);               /* acera */
 assert(fabsf(settle(1428,40)-1.6f)<.01f);                   /* puente */
 assert(fabsf(settle(1428,cm_footbridge_z(2))-2.3f)<.01f);   /* pasarela */
 int plaza=0,park=0;for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){const CmParcel *ps;int n=cm_parcels(bx,bz,&ps);
  for(int i=0;i<n;i++){if(ps[i].kind!=1)continue;float cx,cz;cm_centroid(&ps[i],&cx,&cz);float want=(cm_flags(bx,bz)&CM_PLAZA)?1.7f:1.8f;
   assert(fabsf(settle(cx,cz)-want)<.01f);if(want<1.75f)plaza++;else park++;}}
 assert(plaza&&park);
 /* el escalon es suave: un fotograma no sube de golpe */
 g.lift=0;g.x=42;g.y=1022;surface_tick(1.f/60);g.x=sx;g.y=sy;surface_tick(1.f/60);assert(g.lift>0&&g.lift<1.2f);
 puts("PASS: calzada 0, acera 1,2, puente 1,6, pasarela 2,3, plaza 1,7, parque 1,8; escalones suaves.");return 0;}
