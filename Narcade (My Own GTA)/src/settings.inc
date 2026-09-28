typedef struct {uint32_t magic,version;int language,brightness,music,effects,fog;uint32_t check;} PrefsDisk;
static uint32_t prefs_check(const PrefsDisk *p){return 0x39ab62d1u^p->magic^p->version^(unsigned)p->language*17u^(unsigned)p->brightness*103u^(unsigned)p->music*701u^(unsigned)p->effects*1009u^(unsigned)p->fog*65537u;}
static void settings_apply(void){
#ifdef NARCADE_3D
    r3_set_brightness(prefsBrightness-5);
    r3_set_distance_fog(prefsFog);
#endif
}
static void settings_defaults(void){prefsLanguage=0;prefsBrightness=5;prefsMusic=10;prefsEffects=10;prefsFog=0;settings_apply();}
static void settings_path(char *path,size_t cap){
#ifdef _PSP_FW_VERSION
    raw_snprintf(path,cap,"ms0:/NARCADE.CFG"); /* Outside Sony's SAVEDATA folders. */
#else
    raw_snprintf(path,cap,"%s.cfg",g.savepath);
#endif
}
static void settings_load(void){
    settings_defaults();prefsDirty=0;char path[300];settings_path(path,sizeof path);
    FILE *f=fopen(path,"rb");
#ifdef _PSP_FW_VERSION
    if(!f){raw_snprintf(path,sizeof path,"%s.cfg",g.savepath);f=fopen(path,"rb");} /* Read v2.40 prefs once for migration. */
#endif
    if(!f)return;PrefsDisk p;int ok=fread(&p,1,sizeof p,f)==sizeof p&&fgetc(f)==EOF;fclose(f);
    if(ok&&p.magic==0x4643524eu&&p.version==1&&p.check==prefs_check(&p)&&p.language>=0&&p.language<=1&&p.brightness>=0&&p.brightness<=10&&p.music>=0&&p.music<=10&&p.effects>=0&&p.effects<=10&&p.fog>=0&&p.fog<=1){
        prefsLanguage=p.language;prefsBrightness=p.brightness;prefsMusic=p.music;prefsEffects=p.effects;prefsFog=p.fog;settings_apply();}
    prefsDirty=0;
}
static int settings_save(void){
    PrefsDisk p={0x4643524eu,1,prefsLanguage,prefsBrightness,prefsMusic,prefsEffects,prefsFog,0};p.check=prefs_check(&p);
    char path[300],temp[320],backup[320];settings_path(path,sizeof path);raw_snprintf(temp,sizeof temp,"%s.tmp",path);raw_snprintf(backup,sizeof backup,"%s.bak",path);
    FILE *f=fopen(temp,"wb");if(!f)return 0;int ok=fwrite(&p,1,sizeof p,f)==sizeof p;if(fclose(f))ok=0;if(!ok){remove(temp);return 0;}
    remove(backup);rename(path,backup);if(rename(temp,path)){rename(backup,path);return 0;}prefsDirty=0;return 1;
}
static void settings_finish(void){if(prefsDirty&&!settings_save())notice("No se pudieron guardar los ajustes.");}
static void settings_tick(void){
    int old=g.menu;g.menu=wrapi(g.menu+pressed(B_DOWN)-pressed(B_UP),6);
    int delta=pressed(B_RIGHT)-pressed(B_LEFT),changed=0;
    if(g.menu==0&&(delta||pressed(B_CROSS))){prefsLanguage=1-prefsLanguage;g.noticeT=0;changed=1;}
    else if(delta&&g.menu>=1&&g.menu<=3){int value=g.menu==1?prefsBrightness:g.menu==2?prefsMusic:prefsEffects;
        int next=(int)clampf(value+delta,0,10);changed=next!=value;
        if(g.menu==1)prefsBrightness=next;else if(g.menu==2)prefsMusic=next;else prefsEffects=next;}
    else if(g.menu==4&&(delta||pressed(B_CROSS))){prefsFog=1-prefsFog;changed=1;}
    else if(g.menu==5&&pressed(B_CROSS)){settings_defaults();g.noticeT=0;changed=1;}
    if(changed){prefsDirty=1;settings_apply();}if(old!=g.menu||changed)menu_click();
}
static void settings_draw(void){
    rect(0,0,W,H,INK);header("AJUSTES","Idioma, imagen y sonido");
    const char *labels[]={"IDIOMA","BRILLO","MUSICA","EFECTOS","NIEBLA LEJANA","RESTAURAR AJUSTES"};
    for(int row=0;row<6;row++){int y=66+row*28;uint32_t col=g.menu==row?LIME:WHITE;
        rect(18,y-5,444,25,g.menu==row?RGB(40,59,67):PANEL);text(30,y,labels[row],col,1);
        if(row==0)text(287,y,prefsLanguage?"English":"Espanol",col,1);
        else if(row<4){int value=row==1?prefsBrightness:row==2?prefsMusic:prefsEffects;
            rect(267,y+4,124,5,RGB(68,83,87));rect(267,y+4,value*124/10,5,col);char b[16];snprintf(b,sizeof b,"%d%%",value*10);text(400,y,b,col,1);}
        else if(row==4)text(287,y,prefsFog?"ACTIVADA":"DESACTIVADA",col,1);
        else text(389,y,"X",col,1);
    }
    text(26,233,g.noticeT>0?g.notice:"Los ajustes se conservan entre partidas.",g.noticeT>0?CORAL:MUTED,1);
    footer("ARRIBA/ABAJO elegir  < > cambiar  O volver");
}
