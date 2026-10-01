/* v2.49 (Claude): Metro circular. Anillo continuo y cerrado; estaciones a mitad de manzana con la entrada en la acera;
   pilares con colision que coinciden con los dibujados y sin cortar cruces ni carriles; el tren da la vuelta completa
   parando en las seis estaciones; con TRIANGULO se sube en la entrada y se baja en otra estacion. */
#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 /* geometria */
 float px,pz,tx,tz,maxjump=0;ml_point(0,&px,&pz,&tx,&tz);float x0=px,z0=pz;
 for(float s=1;s<=ml_length()+.5f;s+=1){float x,z;ml_point(s,&x,&z,&tx,&tz);float d=hypotf(x-px,z-pz);if(d>maxjump)maxjump=d;px=x;pz=z;
  assert(fabsf(hypotf(tx,tz)-1)<1e-3f);}
 assert(maxjump<1.01f&&hypotf(px-x0,pz-z0)<.05f);
 game_init();fresh_game();g.screen=WORLD;
 for(int i=0;i<ML_STATIONS;i++){float ex,ez;ml_entrance(i,&ex,&ez);assert(cm_parcel_at(ex,ez)<0&&!ml_pillar_at(ex,ez,4));
  float x,z;ml_point(ml_station_s(i),&x,&z,&tx,&tz);float m=fmodf((tx!=0?x:z)-42,320);assert(m>100&&m<220);} /* a mitad de manzana */
 int pillars=0;for(float x=0;x<2560;x+=2)for(float z=0;z<2240;z+=2)if(ml_pillar_at(x,z,0)){pillars++;
  float m=fmodf(fabsf(z-ML_Z0)<4||fabsf(z-ML_Z1)<4?x-42:z-42,320);assert(m>=46&&m<=274);} /* nunca en un cruce */
 assert(pillars>0);assert(!free_at(ML_X0,ML_Z0+ML_R+48*2,2)||!ml_pillar_ok(ML_X0,ML_Z0+ML_R+48*2,3));
 /* civiles: los carriles (42+-14) quedan libres de pilares */
 for(float x=0;x<2560;x+=1){assert(!ml_pillar_at(x,ML_Z0-14,4)&&!ml_pillar_at(x,ML_Z0+14,4));}
 /* vuelta completa: paradas en las seis estaciones */
 int seen[ML_STATIONS]={0};float travelled=0,last=g.metroZ;
 for(int f=0;f<60*60*4;f++){metro_update(1.f/30);float d=ml_ahead(last,g.metroZ);if(d<ml_length()*.5f)travelled+=d;last=g.metroZ;
  if(g.metroWait>0&&metroStation>=0)seen[metroStation]=1;}
 for(int i=0;i<ML_STATIONS;i++)assert(seen[i]);assert(travelled>ml_length());
 /* subir en la estacion 0 y bajar en la 1 */
 g.metroDir=0;metro_update(1.f/30);assert(metroStation==0&&g.metroWait>0);
 float ex,ez;ml_entrance(0,&ex,&ez);g.x=ex;g.y=ez;g.car=-1;
 assert(metro_boardable());enter_exit();assert(g.inMetro);
 int got=0;for(int f=0;f<60*60;f++){metro_update(1.f/30);if(g.metroWait>0&&metroStation==1){got=1;break;}}
 assert(got);enter_exit();assert(!g.inMetro);ml_entrance(1,&ex,&ez);assert(dist(g.x,g.y,ex,ez)<1);
 assert(foot_free(g.x,g.y));
 /* v2.53: escaleras transitables. Por el centro de cada escalera la altura sube sin saltos (>6 seria un muro para
    lift_ok) desde la entrada hasta el anden; por el costado, a media altura, no se puede entrar; coches y peatones
    chocan con la escalera y el jugador no; desde el anden se sube al tren con TRIANGULO. */
 for(int i=0;i<ML_STATIONS;i++){float sc=ml_station_s(i),prev=0;int platform=0;
  for(float a=-ML_STAIR_FOOT-8;a<=0;a+=1){float x,z;ml_frame_pt(sc+a,a<=ML_STAIR_TOP?ML_STAIR_OFF:(a<=-ML_PLAT_HALF+6?ML_STAIR_OFF-6:ML_PLAT_IN+ML_PLAT_W*.5f),&x,&z);
   int st;float h=ml_walk_height(x,z,prev>15,&st);if(h<0)h=0;assert(fabsf(h-prev)<=6);prev=h;if(st==i)platform=1;}
  assert(platform&&prev==ML_FLOOR);
  float x,z;ml_frame_pt(sc-ML_STAIR_FOOT*.5f-ML_PLAT_HALF*.5f,ML_STAIR_OFF+ML_STAIR_W*.5f+4,&x,&z);float side=ml_walk_height(x,z,0,0);assert(side<0);
  ml_frame_pt(sc-ML_STAIR_FOOT*.5f-ML_PLAT_HALF*.5f,ML_STAIR_OFF,&x,&z);assert(ml_walk_height(x,z,0,0)>10);
  assert(world_solid(x,z));stairsWalkable=1;assert(!world_solid(x,z));stairsWalkable=0;}
 {fresh_game();g.screen=WORLD;g.metroDir=0;metro_update(1.f/30);assert(metroStation==0&&g.metroWait>0);
  float x,z;ml_frame_pt(ml_station_s(0),ML_PLAT_IN+ML_PLAT_W*.5f,&x,&z);g.x=x;g.y=z;g.lift=ML_FLOOR;g.car=-1;g.inMetro=0;
  assert(metro_boardable());enter_exit();assert(g.inMetro);}
 printf("PASS: anillo de %.0f continuo y cerrado; %d puntos de pilar fuera de cruces y carriles; vuelta con 6 paradas; subir y bajar con TRIANGULO.\n",ml_length(),pillars);
 return 0;
}
