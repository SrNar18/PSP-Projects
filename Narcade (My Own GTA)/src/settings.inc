typedef struct {uint32_t magic,version;int language,brightness,music,effects,fog;uint32_t check;} PrefsDisk;
static uint32_t prefs_check(const PrefsDisk *p){return 0x39ab62d1u^p->magic^p->version^(unsigned)p->language*17u^(unsigned)p->brightness*103u^(unsigned)p->music*701u^(unsigned)p->effects*1009u^(unsigned)p->fog*65537u;}
static void settings_apply(void){
#ifdef NARCADE_3D
    r3_set_brightness(prefsBrightness-5);
    r3_set_distance_fog(prefsFog);
#endif
}
static void settings_defaults(void){prefsLanguage=1; /* v2.44 (Claude): ingles por defecto en una instalacion nueva */prefsBrightness=5;prefsMusic=10;prefsEffects=10;prefsFog=0;settings_apply();}
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
    rect(0,0,W,H,RGB(8,11,18));header("AJUSTES","Idioma, imagen y sonido");
    const char *labels[]={"IDIOMA","BRILLO","MUSICA","EFECTOS","NIEBLA LEJANA","RESTAURAR AJUSTES"};
    rect(15,60,450,176,RGB(17,24,32));outline(15,60,450,176,RGB(65,87,95));
    rect(15,60,118,2,TEAL);rect(133,60,333,1,RGB(44,67,75));
    for(int row=0;row<6;row++){int y=68+row*27;int active=g.menu==row;uint32_t col=active?GOLD:WHITE;
        rect(24,y-4,432,25,active?RGB(51,44,38):RGB(23,32,41));
        if(active){rect(24,y-4,3,25,GOLD);outline(24,y-4,432,25,RGB(116,87,58));rect(446,y-3,8,2,GOLD);}
        /* The small control symbols are drawn at native resolution, not font glyphs. */
        if(row==0){circle(43,y+8,6,col);line(37,y+8,49,y+8,RGB(23,32,41));line(43,y+2,43,y+14,RGB(23,32,41));}
        else if(row==1){circle(43,y+8,4,col);for(int n=0;n<8;n++){float a=n*PI/4;line(43+(int)(cosf(a)*6),y+8+(int)(sinf(a)*6),43+(int)(cosf(a)*8),y+8+(int)(sinf(a)*8),col);}}
        else if(row==2){line(42,y+3,42,y+13,col);line(42,y+3,48,y+1,col);circle(39,y+13,2,col);circle(45,y+11,2,col);}
        else if(row==3){rect(37,y+6,4,5,col);line(41,y+6,46,y+2,col);line(41,y+10,46,y+14,col);line(47,y+4,50,y+8,col);line(50,y+8,47,y+12,col);}
        else if(row==4){line(36,y+11,50,y+11,col);line(38,y+7,48,y+7,col);line(41,y+3,45,y+3,col);}
        else{circle(43,y+8,6,col);line(43,y+8,46,y+4,RGB(23,32,41));}
        text(61,y,labels[row],col,1);
        if(row==0)text(310,y,prefsLanguage?"English":"Espanol",col,1);
        else if(row<4){int value=row==1?prefsBrightness:row==2?prefsMusic:prefsEffects;
            rect(302,y+6,104,6,RGB(47,65,73));rect(302,y+6,value*104/10,6,active?GOLD:TEAL);char b[16];snprintf(b,sizeof b,"%d%%",value*10);text(414,y,b,col,1);}
        else if(row==4)text(310,y,prefsFog?"ACTIVADA":"DESACTIVADA",col,1);
        else text(410,y,"X",col,1);
    }
    text(26,235,g.noticeT>0?g.notice:"Los ajustes se conservan entre partidas.",g.noticeT>0?CORAL:MUTED,1);
    footer("ARRIBA/ABAJO elegir  < > cambiar  O volver");
}
