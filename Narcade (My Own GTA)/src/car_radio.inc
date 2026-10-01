/* v2.52 (Claude): radio de los carros con las canciones reales.
   Cada lista de canciones (una carpeta por genero, tools/build_radio.py) es una emisora RADIO<n>.BIN en
   disc0:/PSP_GAME/USRDIR/ o junto al EBOOT. No cabe en memoria (~2 MB por cancion): un hilo de baja prioridad
   (psp_main.c) llama a game_radio_pump(), que lee bloques IMA ADPCM y los deja en un anillo; el audio solo lee
   del anillo. En PC se bombea desde game_update.
   - Al subir a un carro sin emisora se elige una al azar (nunca APAGADA); cada carro recuerda la suya.
   - Cada sintonia (subir o cambiar con IZQUIERDA/DERECHA) empieza una cancion al azar en un segundo al azar.
   - La ultima opcion es APAGADA. Al acabar una cancion sigue otra al azar de la misma emisora. */
#define CR_MAX 6
#define CR_TRACKS 16
#define CR_BLOCK 2048
#define CR_BLOCK_BYTES 1028
#define CR_DATA 4096
#define CR_RING 16384 /* 0,74 s a 22050 Hz */
typedef struct{uint32_t first,blocks;char title[40];}CrTrack;
typedef struct{int count;char genre[16];CrTrack track[CR_TRACKS];}CrStation;
static CrStation crStation[CR_MAX];static int crStations=-1;   /* emisoras con canciones (sin contar APAGADA) */
static signed char carRadio[CAR_COUNT];                         /* emisora de cada carro; -1 = sin sintonizar */
static volatile int crWant=-1;static volatile unsigned crWantSeq=0; /* pedido del juego al hilo de radio */
static short crRing[CR_RING];static volatile unsigned crHead=0,crTail=0,crCutPos=0,crCutSeq=0;
static int crPlaying=-1,crTrack=0;static unsigned crSeq=0,crBlock=0,crRand=0x9e3779b9u;static FILE *crFile=0;
static float crShowT=0;static char crLabel[72];
static const char *const crDirs[]={"disc0:/PSP_GAME/USRDIR/","ms0:/PSP/GAME/NARCADE/","assets/radio/"};
static unsigned cr_rand(void){crRand=crRand*1664525u+1013904223u;return crRand>>8;}
static FILE *cr_open(int n){
    for(int d=0;d<3;d++){char path[96];raw_snprintf(path,sizeof path,"%sRADIO%d.BIN",crDirs[d],n);FILE *f=fopen(path,"rb");if(f)return f;}
    return 0;
}
static uint32_t cr_u32(const unsigned char *p){return p[0]|p[1]<<8|p[2]<<16|(uint32_t)p[3]<<24;}
static void car_radio_load(void){
    crStations=0;
    for(int n=0;n<CR_MAX;n++){FILE *f=cr_open(n);if(!f)break;unsigned char h[32+48*CR_TRACKS];size_t got=fread(h,1,sizeof h,f);fclose(f);
        if(got<32||memcmp(h,"NRD1",4)!=0)break;CrStation *s=&crStation[crStations];int count=(int)cr_u32(h+4);if(count<1)break;if(count>CR_TRACKS)count=CR_TRACKS;
        memcpy(s->genre,h+16,15);s->genre[15]=0;s->count=0;
        for(int k=0;k<count&&32+48*(k+1)<=(int)got;k++){CrTrack *t=&s->track[s->count];t->first=cr_u32(h+32+48*k);t->blocks=cr_u32(h+36+48*k);
            memcpy(t->title,h+40+48*k,39);t->title[39]=0;if(t->blocks>8)s->count++;}
        if(s->count>0)crStations++;else break;}
}
/* nombre inventado segun el genero de la carpeta */
static const char *car_radio_name(int st){
    if(st<0||st>=crStations)return "RADIO APAGADA";
    const char *g=crStation[st].genre;
    if(!strcmp(g,"Synthwave"))return "NEON 88.1 FM";
    if(!strcmp(g,"Rap"))return "BARRIO 104.5 FM";
    if(!strcmp(g,"Rock"))return "ASFALTO 97.3 FM";
    return g;
}
static void cr_pick(int avoid){CrStation *s=&crStation[crPlaying];int t=(int)(cr_rand()%(unsigned)s->count);
    if(s->count>1&&t==avoid)t=(t+1+(int)(cr_rand()%(unsigned)(s->count-1)))%s->count;crTrack=t;}
/* hilo de radio (PSP) o game_update (PC): atiende cambios de emisora y rellena el anillo */
void game_radio_pump(void){
    if(crStations<0)return;
    unsigned seq=crWantSeq;
    if(seq!=crSeq){crSeq=seq;if(crFile){fclose(crFile);crFile=0;}crPlaying=crWant;
        crCutPos=crHead;crCutSeq++;                                     /* el audio descarta lo que quedaba */
        if(crPlaying>=0&&crPlaying<crStations&&(crFile=cr_open(crPlaying))!=0){
            cr_pick(-1);uint32_t b=crStation[crPlaying].track[crTrack].blocks;crBlock=(unsigned)(cr_rand()%(b*3/4)); /* segundo al azar */
        }else crPlaying=-1;}
    if(crPlaying<0||!crFile)return;
    for(int n=0;n<8;n++){
        if(crHead-crTail>CR_RING-CR_BLOCK)break; /* lleno, o el audio aun no ha visto el corte */
        CrTrack *t=&crStation[crPlaying].track[crTrack];unsigned char blk[CR_BLOCK_BYTES];
        if(fseek(crFile,CR_DATA+(long)(t->first+crBlock)*CR_BLOCK_BYTES,SEEK_SET)!=0||fread(blk,1,CR_BLOCK_BYTES,crFile)!=CR_BLOCK_BYTES){crPlaying=-1;return;}
        static const int steps[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,
            130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,
            1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
            13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
        static const int adjust[8]={-1,-1,-1,-1,2,4,6,8};
        int pred=(short)(blk[0]|blk[1]<<8),index=blk[2]>88?88:blk[2];unsigned h=crHead;
        for(int i=0;i<CR_BLOCK;i++){int code=(blk[4+i/2]>>((i&1)*4))&15,step=steps[index],delta=step>>3;
            if(code&4)delta+=step;if(code&2)delta+=step>>1;if(code&1)delta+=step>>2;
            pred+=(code&8)?-delta:delta;if(pred>32767)pred=32767;if(pred<-32768)pred=-32768;
            index+=adjust[code&7];if(index<0)index=0;if(index>88)index=88;
            crRing[(h+i)%CR_RING]=(short)pred;}
        crHead=h+CR_BLOCK;
        if(++crBlock>=t->blocks){crBlock=0;cr_pick(crTrack);}           /* siguiente cancion al azar */
    }
}
/* audio (44100 Hz): una muestra de 22050 cada dos marcos, interpolando el marco intermedio */
static unsigned crCutSeen=0,crHalf=0;
static int car_radio_sample(void){
    if(crCutSeen!=crCutSeq){crCutSeen=crCutSeq;crTail=crCutPos;crHalf=0;}
    if(crHead-crTail<2||crHead-crTail>CR_RING)return 0;
    int a=crRing[crTail%CR_RING],b=crRing[(crTail+1)%CR_RING],v=crHalf?(a+b)/2:a;
    if(crHalf)crTail++;crHalf^=1;return v;
}
static void car_radio_tune(int st){
    if(crStations<0)car_radio_load();
    crWant=st<crStations?st:-1;crWantSeq++;
    if(crWant>=0)raw_snprintf(crLabel,sizeof crLabel,"%s",car_radio_name(crWant));
    else raw_snprintf(crLabel,sizeof crLabel,"%s",locale_text("RADIO APAGADA"));
    crShowT=3.5f;
}
/* al subir al carro i: su emisora, o una al azar la primera vez */
static void car_radio_enter(int i){
    if(crStations<0)car_radio_load();
    if(i<0||i>=CAR_COUNT)return;
    if(carRadio[i]<0||carRadio[i]>crStations)carRadio[i]=(signed char)(crStations>0?(int)(cr_rand()%(unsigned)crStations):0);
    car_radio_tune(carRadio[i]);
}
static void car_radio_exit(void){crWant=-1;crWantSeq++;crShowT=0;}
/* IZQUIERDA/DERECHA dentro del carro: emisoras + APAGADA */
static void car_radio_step(int dir){
    if(g.car<0)return;int n=crStations+1;int st=carRadio[g.car]<0?0:carRadio[g.car];st=((st+dir)%n+n)%n;carRadio[g.car]=(signed char)st;car_radio_tune(st);
}
/* cancion que suena ahora (para el HUD); NULL si apagada o aun sin datos */
static const char *car_radio_song(void){int p=crPlaying,t=crTrack;if(p<0||p>=crStations||crHead==crTail)return 0;return crStation[p].track[t].title;}
static void car_radio_reset(void){memset(carRadio,-1,sizeof carRadio);car_radio_exit();}
