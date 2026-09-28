/* v2.42 (Claude): trofeos. Progreso global del jugador (no de una partida): se guarda en ms0:/NARCADE.TRO,
   fuera de las carpetas SAVEDATA de Sony, con .tmp/.bak como los ajustes. Cinco trofeos para esta fase.
   API para los menus (Codex): trophy_count(), trophy_info(), trophy_unlocked_count(). Visual presentation is owned by game.c. */
enum{TR_WALK,TR_DRIVE,TR_KILLS,TR_GHOST,TR_VINYL,TR_COUNT};
typedef struct{const char *name,*desc;float goal;}TrophyDef;
static const TrophyDef trophyDefs[TR_COUNT]={
 {"PIES EN LA CALLE","Recorre 50 manzanas a pie.",50},
 {"REY DEL ASFALTO","Conduce 200 manzanas.",200},
 {"MALA FAMA","Derriba a 50 personas.",50},
 {"FANTASMA","Pierde a la policia con 3 estrellas o mas.",1},
 {"CAZADOR DE VINILOS","Encuentra 5 vinilos escondidos.",5}};
#define TROPHY_BLOCK 320.f /* una manzana de la rejilla, en unidades del mundo */
typedef struct{uint32_t magic,version;float walked,driven;int kills,ghost,vinyls;uint32_t unlocked,check;}TrophyDisk;
static struct{float walked,driven;int kills,ghost,vinyls;uint32_t unlocked;int dirty;float lastX,lastY;int lastCar,tracking;
 int popup;float popupT,saveT;}tro={0,0,0,0,0,0,0,0,0,-1,0,-1,0,0};
static uint32_t trophy_check(const TrophyDisk *d){const unsigned char *p=(const unsigned char*)d;uint32_t h=2166136261u;
 for(size_t i=0;i<offsetof(TrophyDisk,check);i++){h^=p[i];h*=16777619u;}return h;}
static void trophy_path(char *path,size_t cap){
#ifdef _PSP_FW_VERSION
 raw_snprintf(path,cap,"ms0:/NARCADE.TRO");
#else
 raw_snprintf(path,cap,"%s.tro",g.savepath);
#endif
}
static void trophies_load(void){
 memset(&tro,0,sizeof tro);tro.lastCar=-1;tro.popup=-1;
 char path[300];trophy_path(path,sizeof path);FILE *f=fopen(path,"rb");if(!f)return;
 TrophyDisk d;int ok=fread(&d,1,sizeof d,f)==sizeof d;fclose(f);
 if(!ok||d.magic!=0x4f52544eu||d.version!=1||d.check!=trophy_check(&d))return;
 if(!(d.walked>=0&&d.walked<1e9f&&d.driven>=0&&d.driven<1e9f))return;
 tro.walked=d.walked;tro.driven=d.driven;tro.kills=d.kills;tro.ghost=d.ghost;tro.vinyls=d.vinyls;tro.unlocked=d.unlocked&((1u<<TR_COUNT)-1);
}
static int trophies_save(void){
 TrophyDisk d;memset(&d,0,sizeof d);d.magic=0x4f52544eu;d.version=1;d.walked=tro.walked;d.driven=tro.driven;d.kills=tro.kills;
 d.ghost=tro.ghost;d.vinyls=tro.vinyls;d.unlocked=tro.unlocked;d.check=trophy_check(&d);
 char path[300],temp[320],backup[320];trophy_path(path,sizeof path);raw_snprintf(temp,sizeof temp,"%s.tmp",path);raw_snprintf(backup,sizeof backup,"%s.bak",path);
 FILE *f=fopen(temp,"wb");if(!f)return 0;int ok=fwrite(&d,1,sizeof d,f)==sizeof d;if(fclose(f))ok=0;if(!ok){remove(temp);return 0;}
 remove(backup);rename(path,backup);if(rename(temp,path)){rename(backup,path);return 0;}tro.dirty=0;return 1;
}
static float trophy_progress(int i){
 switch(i){case TR_WALK:return tro.walked/TROPHY_BLOCK;case TR_DRIVE:return tro.driven/TROPHY_BLOCK;case TR_KILLS:return (float)tro.kills;
  case TR_GHOST:return (float)tro.ghost;default:return (float)tro.vinyls;}
}
static void trophy_try(int i){
 if(tro.unlocked&(1u<<i))return;if(trophy_progress(i)<trophyDefs[i].goal)return;
 tro.unlocked|=1u<<i;tro.popup=i;tro.popupT=4.5f;menu_click();trophies_save(); /* un desbloqueo se guarda al momento */
}
/* eventos desde el juego */
static void trophy_kill(void){tro.kills++;tro.dirty=1;trophy_try(TR_KILLS);}
static void trophy_escape(int stars){if(stars>=3){tro.ghost++;tro.dirty=1;trophy_try(TR_GHOST);}}
/* cada fotograma de juego: distancia a pie / en carro (sin saltos de teletransporte: hospital, metro, respawn) */
static void trophies_tick(float dt){
 if(tro.popupT>0){tro.popupT-=dt;if(tro.popupT<=0)tro.popup=-1;}
 int inCar=g.car>=0;float d=tro.tracking?dist(g.x,g.y,tro.lastX,tro.lastY):0;
 if(tro.tracking&&inCar==(tro.lastCar>=0)&&!g.inMetro&&d<dt*400.f+1.f){
  if(inCar)tro.driven+=d;else tro.walked+=d;if(d>0)tro.dirty=1;}
 tro.lastX=g.x;tro.lastY=g.y;tro.lastCar=g.car;tro.tracking=1;
 int v=cachecount();if(v>tro.vinyls){tro.vinyls=v;tro.dirty=1;}
 trophy_try(TR_WALK);trophy_try(TR_DRIVE);trophy_try(TR_VINYL);
 tro.saveT+=dt;if(tro.dirty&&tro.saveT>60){tro.saveT=0;trophies_save();} /* el recorrido se guarda como mucho cada minuto */
}
static void trophies_flush(void){if(tro.dirty)trophies_save();tro.tracking=0;}
int trophy_count(void){return TR_COUNT;}
int trophy_unlocked_count(void){int n=0;for(int i=0;i<TR_COUNT;i++)n+=(tro.unlocked>>i)&1;return n;}
/* nombre y descripcion ya traducidos; progreso 0..1 */
int trophy_info(int i,const char **name,const char **desc,float *progress){
 if(i<0||i>=TR_COUNT)return 0;if(name)*name=locale_text(trophyDefs[i].name);if(desc)*desc=locale_text(trophyDefs[i].desc);
 if(progress){float p=trophy_progress(i)/trophyDefs[i].goal;*progress=p>1?1:p<0?0:p;}return (tro.unlocked>>i)&1;
}
/* aviso de desbloqueo (arriba al centro, entra deslizando) */
static void trophy_popup_draw(void){
 if(tro.popup<0||tro.popupT<=0)return;
 float t=tro.popupT,slide=t>4.2f?(4.5f-t)/.3f:t<.3f?t/.3f:1;int y=(int)(-40+54*slide);
 const char *name=locale_text(trophyDefs[tro.popup].name);int w=(int)strlen(name)*7+76;if(w<230)w=230;int x=W/2-w/2;
 rect(x,y,w,38,INK);rect(x,y,4,38,GOLD);outline(x,y,w,38,RGB(120,98,40));
 circle(x+22,y+19,11,GOLD);circle(x+22,y+19,7,INK);circle(x+22,y+19,4,GOLD);
 text(x+40,y+6,"TROFEO DESBLOQUEADO",GOLD,1);text(x+40,y+21,trophyDefs[tro.popup].name,WHITE,1);
}
