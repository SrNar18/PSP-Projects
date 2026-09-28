/* v2.42 (Claude): trofeos. Desbloqueo, distancia sin teletransportes, persistencia y archivo danado. */
#define main campaign_main
#include "qa.c"
#undef main
int main(void){
 game_init();char path[300];trophy_path(path,sizeof path);remove(path);trophies_load();fresh_game();g.screen=WORLD;
 assert(trophy_count()==5&&trophy_unlocked_count()==0);
 for(int i=0;i<49;i++)trophy_kill();assert(!(tro.unlocked&(1u<<TR_KILLS)));trophy_kill();assert(tro.unlocked&(1u<<TR_KILLS));assert(tro.popup==TR_KILLS);
 trophy_escape(2);assert(!(tro.unlocked&(1u<<TR_GHOST)));trophy_escape(3);assert(tro.unlocked&(1u<<TR_GHOST));
 /* distancia a pie: pasos normales cuentan; un salto (hospital/metro) no */
 tro.tracking=0;g.car=-1;g.inMetro=0;trophies_tick(1.f/60);float w0=tro.walked;
 for(int i=0;i<600;i++){g.x+=1.f;trophies_tick(1.f/60);}assert(tro.walked-w0>590&&tro.walked-w0<610);
 g.x+=5000;trophies_tick(1.f/60);assert(tro.walked-w0<610);
 tro.walked=50*TROPHY_BLOCK-1;g.x+=2;trophies_tick(1.f/60);assert(tro.unlocked&(1u<<TR_WALK));
 const char *n,*d;float p;assert(trophy_info(TR_DRIVE,&n,&d,&p)==0&&p>=0&&p<1);
 /* persistencia */
 assert(trophies_save());uint32_t u=tro.unlocked;int k=tro.kills;memset(&tro,0,sizeof tro);trophies_load();assert(tro.unlocked==u&&tro.kills==k);
 FILE *f=fopen(path,"r+b");fseek(f,12,SEEK_SET);fputc(0x7f,f);fclose(f);trophies_load();assert(tro.unlocked==0&&tro.kills==0); /* danado: se ignora */
 remove(path);
 puts("PASS: 5 trofeos; muertes/huida/distancia desbloquean; saltos no cuentan; guardado, carga y archivo danado.");return 0;}
