/* v2.49 (Claude): Metro circular. Un viaducto elevado sobre las calles del perimetro (x=42 y x=1962, z=42 y
   z=1642; son las que no atraviesan manzanas fusionadas), con esquinas redondeadas y seis estaciones. Este archivo es
   la geometria compartida por el juego (tren, subir/bajar, colision de pilares) y el dibujo (viaducto, estaciones,
   tren), para que todo coincida al milimetro. s = distancia recorrida sobre el anillo, en unidades logicas.
   Sentido de marcha: este por el norte, sur por el este, oeste por el sur y norte por el oeste. */
#ifndef NARCADE_METRO_LOOP_H
#define NARCADE_METRO_LOOP_H
#include <math.h>
#define ML_X0 42.f
#define ML_X1 1962.f
#define ML_Z0 42.f
#define ML_Z1 1642.f
#define ML_R 60.f          /* radio de las esquinas */
#define ML_DECK 26.f       /* cara superior del tablero sobre el terreno */
#define ML_FLOOR (ML_DECK+2.2f) /* suelo del tren y de los andenes */
#define ML_STATIONS 6
#define ML_CAR 46.f        /* largo de cada vagon */
#define ML_TRAIN_W 18.f    /* v2.50: ancho del tren (antes 10) */
#define ML_TRAIN_H 28.f    /* v2.50: alto sobre el suelo (antes 17; el personaje mide ~29 y los coches ~17) */
#define ML_DECK_W 26.f     /* ancho del tablero */
#define ML_PLAT_IN 11.f    /* borde interior del anden (desde el eje) */
#define ML_PLAT_W 12.f     /* ancho del anden */
#define ML_STAIR_W 11.f    /* ancho de la escalera (antes 6) */
#define ML_CARS 3
#define ML_PLAT_HALF 55.f   /* medio largo del anden */
#define ML_STAIR_OFF 32.f   /* desplazamiento lateral de la escalera (acera interior) */
#define ML_STAIR_FOOT 112.f /* la escalera baja desde el final del anden hasta aqui (en s, hacia atras) */
static const float ml_pi=3.14159265f;
static inline float ml_lh(void){return ML_X1-ML_X0-2*ML_R;}
static inline float ml_lv(void){return ML_Z1-ML_Z0-2*ML_R;}
static inline float ml_arc(void){return ml_pi*.5f*ML_R;}
static inline float ml_length(void){return 2*ml_lh()+2*ml_lv()+4*ml_arc();}
static inline float ml_wrap(float s){float L=ml_length();s=fmodf(s,L);return s<0?s+L:s;}
/* Punto y tangente (unitaria, en sentido de marcha) en la posicion s. leg: 0..3 recto (N,E,S,O), 4..7 esquina. */
static inline int ml_point(float s,float *x,float *z,float *tx,float *tz){
    float lh=ml_lh(),lv=ml_lv(),a=ml_arc();s=ml_wrap(s);
    const float segs[8]={lh,a,lv,a,lh,a,lv,a};
    /* tramos: N recto (este), esquina NE, E recto (sur), esquina SE, S recto (oeste), esquina SO, O recto (norte), esquina NO */
    const float cxs[4]={ML_X1-ML_R,ML_X1-ML_R,ML_X0+ML_R,ML_X0+ML_R},czs[4]={ML_Z0+ML_R,ML_Z1-ML_R,ML_Z1-ML_R,ML_Z0+ML_R};
    const float a0s[4]={-ml_pi*.5f,0,ml_pi*.5f,ml_pi};
    for(int k=0;k<8;k++){
        if(s<=segs[k]||k==7){
            if(!(k&1)){int leg=k>>1;float t=s;
                if(leg==0){*x=ML_X0+ML_R+t;*z=ML_Z0;*tx=1;*tz=0;}
                else if(leg==1){*x=ML_X1;*z=ML_Z0+ML_R+t;*tx=0;*tz=1;}
                else if(leg==2){*x=ML_X1-ML_R-t;*z=ML_Z1;*tx=-1;*tz=0;}
                else{*x=ML_X0;*z=ML_Z1-ML_R-t;*tx=0;*tz=-1;}
                return leg;}
            int c=k>>1;float ang=a0s[c]+fminf(s,a)/ML_R;
            *x=cxs[c]+cosf(ang)*ML_R;*z=czs[c]+sinf(ang)*ML_R;*tx=-sinf(ang);*tz=cosf(ang);return 4+c;
        }
        s-=segs[k];
    }
    return 0;
}
/* Estaciones: a mitad de manzana (lejos de los cruces), dos por lado largo y una por lado corto. */
static inline float ml_station_s(int i){
    float lh=ml_lh(),lv=ml_lv(),a=ml_arc();
    switch(i){
        case 0:return 522-(ML_X0+ML_R);                      /* norte, x=522 */
        case 1:return 1162-(ML_X0+ML_R);                     /* norte, x=1162 */
        case 2:return lh+a+(842-(ML_Z0+ML_R));               /* este, z=842 */
        case 3:return lh+a+lv+a+((ML_X1-ML_R)-1162);         /* sur, x=1162 */
        case 4:return lh+a+lv+a+((ML_X1-ML_R)-522);          /* sur, x=522 */
        default:return 2*lh+2*a+lv+a+((ML_Z1-ML_R)-842);     /* oeste, z=842 */
    }
}
static const char *const mlStationNames[ML_STATIONS]={"ESTACION ARANJUEZ","ESTACION PRADO","ESTACION BUENOS AIRES",
    "ESTACION POBLADO","ESTACION BELEN","ESTACION LAURELES"};
/* Entrada de la estacion a pie de calle: en la acera interior del anillo (la exterior del borde no tiene manzanas), junto a la escalera. */
static inline void ml_entrance(int i,float *x,float *z){
    float px,pz,tx,tz;ml_point(ml_station_s(i)-ML_STAIR_FOOT-8,&px,&pz,&tx,&tz); /* justo delante del pie de la escalera */
    float nx=-tz,nz=tx; /* normal interior (a la derecha del sentido de marcha) */
    *x=px+nx*ML_STAIR_OFF;*z=pz+nz*ML_STAIR_OFF;
}
/* Escalera de la estacion i como rectangulo alineado con los ejes (las estaciones estan en tramos rectos). */
static inline void ml_frame_pt(float s,float off,float *x,float *z){float px,pz,tx,tz;ml_point(s,&px,&pz,&tx,&tz);*x=px-tz*off;*z=pz+tx*off;}
static inline int ml_stairs_at(float x,float z,float r){
    for(int i=0;i<ML_STATIONS;i++){float s0=ml_station_s(i)-ML_PLAT_HALF-4,s1=ml_station_s(i)-ML_STAIR_FOOT;
        float ax,az,bx,bz;ml_frame_pt(s0,ML_STAIR_OFF-ML_STAIR_W*.5f,&ax,&az);ml_frame_pt(s1,ML_STAIR_OFF+ML_STAIR_W*.5f,&bx,&bz);
        float x0=fminf(ax,bx)-r,x1=fmaxf(ax,bx)+r,z0=fminf(az,bz)-r,z1=fmaxf(az,bz)+r;
        if(x>=x0&&x<=x1&&z>=z0&&z<=z1)return 1;}
    return 0;
}
/* Distancia (en s) desde a hasta b en sentido de marcha. */
static inline float ml_ahead(float a,float b){float d=ml_wrap(b)-ml_wrap(a);return d<0?d+ml_length():d;}
/* Pilares: cada 48 en los tramos rectos, nunca a menos de 50 de un cruce de calles ni en el cauce del rio. */
static inline int ml_pillar_ok(float x,float z,int leg){
    float c=leg==0||leg==2?x:z;float m=fmodf(c-42.f,320.f);if(m<0)m+=320;
    if(m<50.f||m>270.f)return 0;
    if(x>1385.f&&x<1470.f)return 0;
    return 1;
}
static inline int ml_pillar_at(float x,float z,float r){
    const float lines[4][2]={{ML_Z0,0},{ML_X1,1},{ML_Z1,0},{ML_X0,1}};
    for(int leg=0;leg<4;leg++){int vert=(int)lines[leg][1];float off=vert?x-lines[leg][0]:z-lines[leg][0];if(fabsf(off)>3.2f+r)continue;
        float along=vert?z:x;float lo=vert?ML_Z0+ML_R:ML_X0+ML_R;float hi=vert?ML_Z1-ML_R:ML_X1-ML_R;if(along<lo-r||along>hi+r)continue;
        float k=roundf((along-lo)/48.f);float p=lo+k*48.f;if(p<lo||p>hi)continue;
        float px=vert?lines[leg][0]:p,pz=vert?p:lines[leg][0];
        if(!ml_pillar_ok(px,pz,leg))continue;
        if((px-x)*(px-x)+(pz-z)*(pz-z)<(3.2f+r)*(3.2f+r))return 1;}
    return 0;
}
#endif
