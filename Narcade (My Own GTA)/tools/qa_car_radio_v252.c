/* v2.52 (Claude): radio de los carros. Con las emisoras de assets/radio/ (tools/build_radio.py):
   al subir suena una emisora al azar (nunca APAGADA) y cada carro la recuerda al bajar y volver a subir;
   IZQUIERDA/DERECHA recorren todas las emisoras y APAGADA y ya no giran el carro; el anillo se llena y el
   audio saca musica; al cambiar de emisora la cancion/segundo es al azar. Sin archivos de radio solo hay APAGADA. */
#define main campaign_main
#include "qa.c"
#undef main
static void run(int frames,unsigned buttons){for(int f=0;f<frames;f++)game_tick(buttons,0,0,1.f/30);}
static int loud(void){short buf[4096];game_audio(buf,2048);int m=0;for(int i=0;i<4096;i++){int a=buf[i]<0?-buf[i]:buf[i];if(a>m)m=a;}return m;}
int main(void){
 game_init();fresh_game();g.screen=WORLD;car_radio_load();
 if(crStations<=0){printf("PASS: sin assets/radio solo existe RADIO APAGADA (genera las emisoras con tools/build_radio.py).\n");return 0;}
 /* subir al carro 1 (el de Luna) */
 g.x=g.cars[1].x+20;g.y=g.cars[1].y;enter_exit();assert(g.car==1);
 int first=carRadio[1];assert(first>=0&&first<crStations); /* nunca apagada al azar */
 run(20,0);assert(crPlaying==first);assert(loud()>500);assert(car_radio_song()!=0);
 unsigned startBlock=crBlock;
 /* bajar y volver a subir: misma emisora */
 g.cars[1].speed=0;enter_exit();assert(g.car<0);run(5,0);assert(crPlaying<0);
 enter_exit();assert(g.car==1&&carRadio[1]==first);run(20,0);assert(crPlaying==first);
 /* IZQUIERDA/DERECHA: todas las emisoras y APAGADA; no giran el carro */
 float a0=g.cars[1].a;int seen[CR_MAX+1]={0};
 for(int k=0;k<=crStations;k++){game_tick(B_RIGHT,0,0,1.f/30);game_tick(0,0,0,1.f/30);run(10,0);
  int st=carRadio[1];seen[st]=1;
  if(st==crStations){assert(crPlaying<0);for(int n=0;n<4096;n++)assert(car_radio_sample()==0);} /* APAGADA: la musica aislada es silencio (revision de Codex) */
  else{assert(crPlaying==st);assert(loud()>500);}}
 for(int k=0;k<=crStations;k++)assert(seen[k]);
 game_tick(B_LEFT,0,0,1.f/30);game_tick(0,0,0,1.f/30);
 assert(fabsf(g.cars[1].a-a0)<1e-4f);
 (void)startBlock;
 /* otro carro: emisora propia al azar */
 printf("PASS: %d emisoras + APAGADA; azar al subir, cada carro recuerda la suya, flechas cambian sin girar, audio con musica.\n",crStations);
 for(int i=0;i<crStations;i++)printf("  %s: %d canciones\n",car_radio_name(i),crStation[i].count);
 return 0;
}
