/* citymap.h — Trazado urbano irregular de Narcade (Claude, v2.6).
 *
 * Fuente unica de verdad para colision (game.c solid()), minimapa/mapa y render 3D (city3d.inc, city_v26()).
 * Base: rejilla de 8x7 celdas de 320 unidades (calle base en lx<86 || lz<86). Sobre ella:
 *  - MERGE_E / MERGE_S: la calle entre dos celdas desaparece -> manzana doble (grande).
 *  - TUNNEL (solo con MERGE_E): la manzana doble deja un paso de 64 por donde iba la calle y un edificio
 *    puente lo cubre por encima -> "tunel" urbano.
 *  - SPLIT_X / SPLIT_Z: una calle de 52 atraviesa la celda por el medio -> dos manzanas pequenas.
 *  - Avenida diagonal (Av. Oriental) que cruza el Centro y genera parcelas triangulares/trapezoidales.
 *  - Parcelas = poligonos convexos (rectangulos recortados por la diagonal). La colision es "dentro de una parcela
 *    edificable"; todo lo demas (calles, aceras, plazas, parques) es transitable.
 *  - Celdas del rio (bx=4): la parcela empieza en x=1490 (orilla este); la orilla oeste es paseo y viaducto del Metro.
 *  - Especiales: Estadio (Laureles), Plaza Botero (Centro), Pueblito Paisa (cerro Nutibara), estaciones de Metro
 *    junto al rio y Metrocable en la ladera nororiental.
 * Inspirado en Medellin: comunas en las laderas oeste (San Javier) y este (nororiental), Laureles/Estadio al
 * oeste-centro, Belen al suroeste, Centro denso junto al rio, El Poblado con torres al sureste, Aranjuez al norte.
 *
 * Las parcelas se calculan una vez (cm_build) y se guardan en cache: cm_solid() es barato (minimapa lo llama por pixel).
 */
#ifndef NARCADE_CITYMAP_H
#define NARCADE_CITYMAP_H
#include <math.h>
#include "metro_loop.h" /* v2.49 (Claude) */
#define CM_MERGE_E 1
#define CM_MERGE_S 2
#define CM_SPLIT_X 4
#define CM_SPLIT_Z 8
#define CM_PARK 16
#define CM_STADIUM 32
#define CM_PLAZA 64
#define CM_PUEBLITO 128
#define CM_TUNNEL 256
#define CM_NONE 0
/* Flags por celda [bz][bx]. (Parques originales: (2,3) Nutibara, (2,1), (0,2), (4,5), (4,6).) */
static const unsigned short cm_cells[7][8]={
/* bz=0 */ {CM_NONE,              CM_SPLIT_Z,          CM_NONE,             CM_SPLIT_X,          CM_NONE,   CM_NONE,              CM_SPLIT_Z,          CM_NONE},
/* bz=1 */ {CM_SPLIT_Z,           CM_NONE,             CM_PARK,            CM_NONE,             CM_NONE,   CM_SPLIT_X,           CM_NONE,             CM_SPLIT_Z},
/* bz=2 */ {CM_PARK,              CM_SPLIT_X,          CM_NONE,             CM_NONE,             CM_NONE,   CM_SPLIT_Z,           CM_NONE,             CM_NONE},
/* bz=3 */ {CM_MERGE_E|CM_TUNNEL, CM_NONE,             CM_PUEBLITO,         CM_PLAZA,            CM_NONE,   CM_NONE,              CM_NONE,             CM_MERGE_S},
/* bz=4 */ {CM_NONE,              CM_MERGE_E|CM_STADIUM,CM_NONE,            CM_SPLIT_Z,          CM_NONE,   CM_MERGE_E|CM_TUNNEL, CM_NONE,             CM_NONE},
/* bz=5 */ {CM_SPLIT_X,           CM_NONE,             CM_MERGE_S,          CM_NONE,             CM_PARK,   CM_NONE,              CM_MERGE_E,          CM_NONE},
/* bz=6 */ {CM_NONE,              CM_SPLIT_Z,          CM_NONE,             CM_NONE,             CM_PARK,   CM_NONE,              CM_NONE,             CM_SPLIT_Z},
};
static inline unsigned cm_flags(int bx,int bz){if(bx<0||bx>7||bz<0||bz>6)return 0;return cm_cells[bz][bx];}
static inline int cm_park(int bx,int bz){return (cm_flags(bx,bz)&(CM_PARK|CM_PLAZA))!=0;}
/* Intersections reserved for small, drivable roundabouts. The island is
   represented by the same footprint in the renderer and collision code. */
static inline int cm_roundabout(int bx,int bz){
    /* v2.37 (Claude): desactivadas. El cruce esta en la franja de transicion del valle y la isla
       (circulo de radio 12) se veia aplastada: una acera torcida con arbustos en mitad de la calle,
       y su colision redonda era una pared invisible fuera de lo dibujado. */
    (void)bx;(void)bz;return 0;
}
static inline float cm_footbridge_z(int bz){return bz==2||bz==5?bz*320.f+190.f:-10000.f;}
/* Avenida diagonal (Av. Oriental): de (1010,700) a (1330,1560), anchura 56. */
#define CM_DIAG_X0 1010.f
#define CM_DIAG_Z0 700.f
#define CM_DIAG_X1 1330.f
#define CM_DIAG_Z1 1560.f
#define CM_DIAG_HALF 28.f
static inline float cm_diag_dist(float x,float z){
    float dx=CM_DIAG_X1-CM_DIAG_X0,dz=CM_DIAG_Z1-CM_DIAG_Z0,l2=dx*dx+dz*dz;
    float t=((x-CM_DIAG_X0)*dx+(z-CM_DIAG_Z0)*dz)/l2;if(t<0)t=0;if(t>1)t=1;
    float px=CM_DIAG_X0+dx*t-x,pz=CM_DIAG_Z0+dz*t-z;return sqrtf(px*px+pz*pz);
}
static inline int cm_on_diag(float x,float z){return cm_diag_dist(x,z)<CM_DIAG_HALF;}
/* Calle base de la rejilla en (x,z), teniendo en cuenta fusiones, tuneles y particiones. */
static inline int cm_on_grid_road(float x,float z){
    int bx=(int)floorf(x/320),bz=(int)floorf(z/320);float lx=x-bx*320,lz=z-bz*320;
    unsigned f=cm_flags(bx,bz),fw=cm_flags(bx-1,bz),fn=cm_flags(bx,bz-1);
    int westStreet=lx<86,northStreet=lz<86;
    /* la calle oeste de esta celda pertenece a la fusion con la celda de la izquierda; la norte, con la de arriba */
    if(westStreet&&(fw&CM_MERGE_E)&&lz>=86){ if((fw&CM_TUNNEL)&&lx>=7&&lx<71)return 1; return 0; }
    if(northStreet&&(fn&CM_MERGE_S)&&lx>=86)return 0;
    if(westStreet||northStreet)return 1;
    if((f&CM_SPLIT_X)&&lx>=166&&lx<218)return 1;
    if((f&CM_SPLIT_Z)&&lz>=160&&lz<212)return 1;
    return 0;
}
static inline int cm_on_road(float x,float z){return cm_on_grid_road(x,z)||cm_on_diag(x,z);}
/* Parcelas (poligonos convexos, hasta 8 vertices) de una celda, en coordenadas de mundo.
   kind: 0 edificable (solido), 1 parque/plaza (transitable). */
typedef struct {int n;float x[8],z[8];int kind;} CmParcel;
static inline void cm_clip_halfplane(CmParcel *p,float ax,float az,float nx,float nz){
    /* conserva la parte con (P-A).N >= 0 */
    CmParcel out;out.n=0;out.kind=p->kind;
    for(int i=0;i<p->n;i++){
        int j=(i+1)%p->n;float dx0=(p->x[i]-ax)*nx+(p->z[i]-az)*nz,dx1=(p->x[j]-ax)*nx+(p->z[j]-az)*nz;
        if(dx0>=0){out.x[out.n]=p->x[i];out.z[out.n]=p->z[i];out.n++;}
        if((dx0>=0)!=(dx1>=0)&&out.n<8){float t=dx0/(dx0-dx1);out.x[out.n]=p->x[i]+(p->x[j]-p->x[i])*t;out.z[out.n]=p->z[i]+(p->z[j]-p->z[i])*t;out.n++;}
    }
    *p=out;
}
static inline void cm_rect(CmParcel *p,float x0,float z0,float x1,float z1,int kind){p->n=4;p->x[0]=x0;p->z[0]=z0;p->x[1]=x1;p->z[1]=z0;p->x[2]=x1;p->z[2]=z1;p->x[3]=x0;p->z[3]=z1;p->kind=kind;}
static inline float cm_area(const CmParcel *p){float a=0;for(int i=0;i<p->n;i++){int j=(i+1)%p->n;a+=p->x[i]*p->z[j]-p->x[j]*p->z[i];}return fabsf(a)*.5f;}
static inline int cm_is_rect(const CmParcel *p){return p->n==4&&p->x[0]==p->x[3]&&p->x[1]==p->x[2]&&p->z[0]==p->z[1]&&p->z[2]==p->z[3];}
/* Parcelas que NACEN en la celda (bx,bz). Las fusiones extienden la parcela hacia el este/sur y la celda absorbida
   no genera la suya. */
static inline int cm_parcels_compute(int bx,int bz,CmParcel *out){
    unsigned f=cm_flags(bx,bz);int n=0;
    if(bx>0&&(cm_flags(bx-1,bz)&CM_MERGE_E))return 0;   /* absorbida por la celda oeste */
    if(bz>0&&(cm_flags(bx,bz-1)&CM_MERGE_S))return 0;   /* absorbida por la celda norte */
    float x=bx*320,z=bz*320;int kind=(f&(CM_PARK|CM_PLAZA))?1:0;
    /* Unequal setbacks break the repeated square silhouette while leaving
       the public road corridor and existing save coordinates untouched. */
    unsigned shape=(unsigned)(bx*73+bz*41+bx*bz*17);
    float x0=x+90+(shape%4)*3,z0=z+90+((shape>>2)%4)*3;
    float x1=x+278+((shape>>4)%5)*3,z1=z+270+((shape>>7)%5)*3;
    if(bx==4)x0=(bz==1||bz==3||bz==5)?1524:1506;       /* dar espacio a los andenes y cubiertas en las tres estaciones */
    if(f&CM_MERGE_E)x1=x+320+288;
    if(f&CM_MERGE_S)z1=z+320+280;
    if((f&CM_TUNNEL)&&(f&CM_MERGE_E)){cm_rect(&out[n++],x0,z0,x+327,z1,kind);cm_rect(&out[n++],x+391,z0,x1,z1,kind);}
    else if(f&CM_SPLIT_X){cm_rect(&out[n++],x0,z0,x+158,z1,kind);cm_rect(&out[n++],x+226,z0,x1,z1,kind);}
    else if(f&CM_SPLIT_Z){cm_rect(&out[n++],x0,z0,x1,z+152,kind);cm_rect(&out[n++],x0,z+220,x1,z1,kind);}
    else cm_rect(&out[n++],x0,z0,x1,z1,kind);
    /* recorte por la diagonal: cada parcela se divide en la parte a un lado y al otro de la avenida */
    float dx=CM_DIAG_X1-CM_DIAG_X0,dz=CM_DIAG_Z1-CM_DIAG_Z0,l=sqrtf(dx*dx+dz*dz);float nx=-dz/l,nz=dx/l; /* normal */
    int m=n;
    for(int i=0;i<m;i++){
        CmParcel a=out[i],b=out[i];
        int touch=0;for(int k=0;k<a.n;k++)if(cm_diag_dist(a.x[k],a.z[k])<CM_DIAG_HALF+150)touch=1;
        if(!touch)continue;
        float cxm=(out[i].x[0]+out[i].x[2])*.5f,czm=(out[i].z[0]+out[i].z[2])*.5f;
        if(cm_diag_dist(cxm,czm)>260)continue;
        float off=CM_DIAG_HALF+8;
        cm_clip_halfplane(&a,CM_DIAG_X0+nx*off,CM_DIAG_Z0+nz*off,nx,nz);       /* lado +N */
        cm_clip_halfplane(&b,CM_DIAG_X0-nx*off,CM_DIAG_Z0-nz*off,-nx,-nz);     /* lado -N */
        int wrote=0;
        if(a.n>=3&&cm_area(&a)>900){out[i]=a;wrote=1;}
        if(b.n>=3&&cm_area(&b)>900){if(wrote){if(n<8)out[n++]=b;}else{out[i]=b;wrote=1;}}
        if(!wrote)out[i].n=0;
    }
    int k=0;for(int i=0;i<n;i++)if(out[i].n>=3)out[k++]=out[i];
    return k;
}
/* Cache (una copia por unidad de compilacion; se rellena en la primera consulta). */
static CmParcel cm_cache[7][8][8];
static unsigned char cm_count[7][8];
static int cm_built=0;
static inline void cm_build(void){
    if(cm_built)return;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++)cm_count[bz][bx]=(unsigned char)cm_parcels_compute(bx,bz,cm_cache[bz][bx]);
    cm_built=1;
}
static inline int cm_parcels(int bx,int bz,const CmParcel **out){
    if(bx<0||bx>7||bz<0||bz>6)return 0;
    cm_build();*out=cm_cache[bz][bx];return cm_count[bz][bx];
}
static inline int cm_point_in(const CmParcel *p,float x,float z){
    int sign=0;
    for(int i=0;i<p->n;i++){int j=(i+1)%p->n;float c=(p->x[j]-p->x[i])*(z-p->z[i])-(p->z[j]-p->z[i])*(x-p->x[i]);
        if(fabsf(c)<1e-4f)continue;int s=c>0?1:-1;if(sign==0)sign=s;else if(s!=sign)return 0;}
    return 1;
}
/* Parcela que contiene (x,z): devuelve kind (0 edificio, 1 parque/plaza) o -1 si es espacio publico. */
static inline int cm_parcel_at(float x,float z){
    int bx=(int)floorf(x/320),bz=(int)floorf(z/320);
    for(int dz=-1;dz<=0;dz++)for(int dx=-1;dx<=0;dx++){
        const CmParcel *ps;int n=cm_parcels(bx+dx,bz+dz,&ps);
        for(int i=0;i<n;i++)if(cm_point_in(&ps[i],x,z))return ps[i].kind;
    }
    return -1;
}
/* Metro (a escala del jugador: 1 m ~ 8.7 unidades): viaducto por la orilla este del rio en x=1486 (tablero a 26),
   anden a 30.2 en las estaciones (bz 1,3,5), escalera de 60 de recorrido en la orilla (x 1465..1476) que sube hacia
   el norte hasta el anden. El tren (2 coches de 110) para en el centro de cada estacion. Metrocable: (1760,520)->(2460,120). */
#define CM_METRO_X 1486.f
#define CM_METRO_DECK 26.f
#define CM_PLAT_H 30.2f
#define CM_TRAIN_CAR 110.f
static inline int cm_metro_station(int bz){(void)bz;return 0;} /* v2.49: ya no hay estaciones en la orilla (Metro circular, metro_loop.h) */
static inline float cm_station_z(int bz){return bz*320+160.f;}
/* Concrete supports beneath the three elevated station platforms. Their
   walkable decks are above the tops of these supports, so callers decide
   whether the player is at ground level before treating them as obstacles. */
static inline int cm_metro_support(float x,float z){
 int row=(int)floorf(z/320.f);if(!cm_metro_station(row))return 0;
 float localZ=z-row*320.f;
 for(int side=-1;side<=1;side+=2)if(fabsf(x-(CM_METRO_X+side*15.f))<2.6f)
  for(int k=0;k<3;k++)if(fabsf(localZ-(80.f+k*80.f))<2.6f)return 1;
 return 0;
}
static inline int cm_on_platform(float x,float z){int bz=(int)floorf(z/320);float lz=z-bz*320;return cm_metro_station(bz)&&x>1466&&x<1505&&fabsf(x-CM_METRO_X)>10.5f&&lz>=60&&lz<=260;}
static inline int cm_on_stairs(float x,float z){int bz=(int)floorf(z/320);float lz=z-bz*320;return cm_metro_station(bz)&&x>1466&&x<1475&&lz>=258&&lz<=318;}
/* Elevacion peatonal en (x,z). up=1 si el jugador ya esta arriba (en el anden). */
static inline float cm_lift(float x,float z,int up){
    int bz=(int)floorf(z/320);float lz=z-bz*320;
    if(cm_on_stairs(x,z))return CM_PLAT_H*(318-lz)/60;
    if(up&&cm_on_platform(x,z))return CM_PLAT_H;
    return 0;
}
/* v2.45 (Claude): altura de la superficie transitable sobre el terreno en (x,z), la misma que dibuja la
   ciudad: calzada 0, acera 1,2, tablero de los puentes 1,6, plaza 1,7, cesped de parque 1,8, pasarela 2,3.
   Antes el jugador y los peatones iban a la altura del terreno y se hundian en todo lo elevado (atravesaban
   el puente y los desniveles de las plazas). */
static inline float cm_surface(float x,float z){
    int bz=(int)floorf(z/320);float lz=z-bz*320;
    if(x>=1392.f&&x<=1464.f){
        if(lz<86.f)return 1.6f;                                   /* puente de la calzada */
        if(fabsf(z-cm_footbridge_z(bz))<13.f)return 2.3f;           /* pasarela peatonal */
        return 0;                                                 /* cauce (no transitable) */
    }
    int k=cm_parcel_at(x,z);
    if(k>=0){int bx=(int)floorf(x/320);if(cm_flags(bx,bz)&CM_PLAZA)return 1.7f;return k==1?1.8f:0;}
    if(cm_on_road(x,z))return 0;
    return 1.2f;                                                  /* acera */
}
/* Estacion mas cercana al tren en z (o -1). */
static inline int cm_station_near(float z,float tol){for(int bz=1;bz<=5;bz+=2)if(fabsf(z-cm_station_z(bz))<tol)return bz;return -1;}
#define CM_CABLE_X0 1730.f
#define CM_CABLE_Z0 508.f
#define CM_CABLE_X1 2430.f
#define CM_CABLE_Z1 189.f
/* Stations occupy buildable parcels; intermediate pylons are set on open
   sidewalk plots rather than passing through houses or traffic lanes. */
static inline void cm_cable_node(int i,float *x,float *z){
    /* v2.33 (Claude): pilonas 1-4 movidas dentro de una manzana (antes 3 de ellas estaban
       en la calzada: tools/qa_street_objects_v233.c). */
    static const float nodes[6][2]={{CM_CABLE_X0,CM_CABLE_Z0},{1871.f,444.f},
        {2020.f,420.f},{2150.f,272.f},{2346.f,253.f},{CM_CABLE_X1,CM_CABLE_Z1}};
    if(i<0)i=0;if(i>5)i=5;*x=nodes[i][0];*z=nodes[i][1];
}
static inline void cm_centroid(const CmParcel *p,float *cx,float *cz){float sx=0,sz=0;for(int i=0;i<p->n;i++){sx+=p->x[i];sz+=p->z[i];}*cx=sx/p->n;*cz=sz/p->n;}
/* Parcela encogida (inset>0) o ensanchada (inset<0) moviendo cada vertice hacia/desde el centroide. */
static inline void cm_shrunk(const CmParcel *p,float inset,CmParcel *out){
    float cx,cz;cm_centroid(p,&cx,&cz);*out=*p;
    for(int i=0;i<p->n;i++){float dx=cx-p->x[i],dz=cz-p->z[i],l=sqrtf(dx*dx+dz*dz);if(l<1)continue;
        float m=inset*1.35f;if(m>l*.9f)m=l*.9f;out->x[i]=p->x[i]+dx/l*m;out->z[i]=p->z[i]+dz/l*m;}
}
/* Obstaculos menores que tambien son solidos (mismas posiciones que dibuja city26.inc): pilares y escaleras del
   Metro, pedestales y fuente de Plaza Botero, fuentes de los parques. */
/* v2.33: el circulo de radio m cae dentro de alguna parcela (igual que inside_block en city26.inc). */
static inline int cm_inside_margin(float x,float z,float m){
    if(cm_parcel_at(x,z)<0)return 0;
    for(int k=0;k<12;k++){float a=k*0.5235988f;if(cm_parcel_at(x+cosf(a)*m,z+sinf(a)*m)<0)return 0;}
    return 1;
}
static inline int cm_obstacle(float x,float z){
    for(int i=1;i<5;i++){float px,pz;cm_cable_node(i,&px,&pz);
        if(fabsf(x-px)<4.5f&&fabsf(z-pz)<4.5f)return 1;
    }
    if(x>=1392.f&&x<=1464.f)
        for(int row=0;row<7;row++){
            float dz=fabsf(z-cm_footbridge_z(row));
            if(dz>=13.f&&dz<=16.f)return 1; /* parapet */
        }
    for(int k=0;k<3;k++){static const int bx[3]={1,6,6},bz[3]={2,1,5};
        if(!cm_roundabout(bx[k],bz[k]))continue; /* v2.37: islas desactivadas */ float dx=x-(bx[k]*320+42),dz=z-(bz[k]*320+42);
        if(dx*dx+dz*dz<12.f*12.f)return 1;
    }
    if(ml_pillar_at(x,z,0))return 1; /* v2.49: pilares del Metro circular */
    /* v2.53: las escaleras del Metro van en world_solid (game.c): solidas para coches y peatones, transitables para el jugador */
    int bx=(int)floorf(x/320),bzc=(int)floorf(z/320);
    for(int dz=-1;dz<=0;dz++)for(int dx=-1;dx<=0;dx++){
        const CmParcel *ps;int n=cm_parcels(bx+dx,bzc+dz,&ps);unsigned f=cm_flags(bx+dx,bzc+dz);
        for(int i=0;i<n;i++){const CmParcel *p=&ps[i];if(p->kind!=1)continue;
            float cx,cz;cm_centroid(p,&cx,&cz);
            if(f&CM_PLAZA){
                if(fabsf(x-cx)<8&&fabsf(z-cz)<8)return 1;                       /* fuente */
                CmParcel g;cm_shrunk(p,22,&g);
                for(int k=0;k<g.n;k++){float sx=(g.x[k]+cx)*.5f,sz=(g.z[k]+cz)*.5f;if(fabsf(x-sx)<5&&fabsf(z-sz)<5&&cm_inside_margin(sx,sz,6))return 1;} /* pedestales (v2.33: solo los que se dibujan) */
            }else if(cm_is_rect(p)&&fabsf(x-(cx+6))<6&&fabsf(z-(cz+7))<6)return 1;  /* fuente del parque */
        }
    }
    return 0;
}
/* Colision de edificios: dentro de alguna parcela edificable (parques y plazas no son solidos). */
static inline int cm_solid(float x,float z){return cm_parcel_at(x,z)==0;}
/* v2.36 (Claude): terminal de cada punto de mision en la acera real. Antes estaba en
   (x+18, y) = z local 62, en medio de la calzada (las "casetas" en la carretera). Busca el punto
   mas cercano fuera de toda parcela en radio 3 y con una parcela a menos de 5, evitando el coche
   aparcado del punto (x+54, y+18). Lo usan el dibujo (render3d.c) y la colision (game.c). */
static inline int cm_block(float x,float z){return cm_parcel_at(x,z)>=0;}
/* v2.41 (Claude): la busqueda fina tarda ~150 ms en PC (varios segundos en PSP) y se hacia de forma diferida
   en el primer fotograma tras cargar (render) y otra vez al primer paso (juego): el parón tras el 100 %.
   Las posiciones son fijas: tabla precalculada; tools/qa_terminal_table_v241.c comprueba que coincide. */
static const float cmTerminalTable[30][4]={ /* lx,ly -> tx,tz: generado por tools/qa_terminal_table_v241.c */
    {62.f,1022.f,94.470871f,1056.05286f},
    {382.f,1342.f,409.949768f,1366.02087f},
    {1022.f,1022.f,1055.12878f,1056.31287f},
    {62.f,382.f,90.90979f,410.482025f},
    {1342.f,702.f,1386.f,746.f},
    {1342.f,1022.f,1386.f,1066.f},
    {62.f,702.f,90.7564468f,728.392212f},
    {702.f,382.f,729.274353f,415.748871f},
    {1342.f,62.f,1386.f,106.f},
    {1662.f,1342.f,1690.75647f,1368.39221f},
    {382.f,62.f,410.90979f,90.4820404f},
    {1982.f,1982.f,2010.38586f,2004.1897f},
    {382.f,1982.f,410.756439f,2008.39221f},
    {702.f,1022.f,734.470886f,1056.05286f},
    {1022.f,1662.f,1054.47083f,1696.05286f},
    {1982.f,702.f,2008.83081f,727.515015f},
    {62.f,62.f,90.3859024f,84.1897049f},
    {1022.f,1342.f,1054.47083f,1376.05286f},
    {1982.f,1662.f,2009.94983f,1686.02087f},
    {2302.f,382.f,2335.12891f,416.312805f},
    {1662.f,62.f,1689.27441f,95.7488708f},
    {1342.f,1342.f,1386.f,1386.f},
    {1022.f,702.f,1055.78577f,736.56604f},
    {2302.f,1982.f,2335.12891f,2016.31287f},
    {702.f,1662.f,730.90979f,1690.48206f},
    {1342.f,1982.f,1386.f,2026.f},
    {382.f,702.f,409.274353f,735.748901f},
    {1982.f,62.f,2010.50659f,94.3359222f},
    {1342.f,1662.f,1386.f,1706.f},
    {2302.f,1022.f,2335.12891f,1056.31287f},
};
static inline void cm_terminal_search(float lx,float ly,float *tx,float *tz);
static inline void cm_terminal_pos(float lx,float ly,float *tx,float *tz){
    for(int i=0;i<30;i++)if(cmTerminalTable[i][0]==lx&&cmTerminalTable[i][1]==ly){*tx=cmTerminalTable[i][2];*tz=cmTerminalTable[i][3];return;}
    cm_terminal_search(lx,ly,tx,tz);
}
static inline void cm_terminal_search(float lx,float ly,float *tx,float *tz){
    float ox=lx+18,oz=ly;
    for(float r=0;r<=40;r+=.5f)for(int k=0,n=r==0?1:(int)(r*2.f)+12;k<n;k++){ /* fina: solo se calcula una vez por terminal */
        float a=k*(6.2831853f/n),px=ox+cosf(a)*r,pz=oz+sinf(a)*r;
        if(cm_block(px,pz)||cm_on_diag(px,pz))continue;
        if(fabsf(px-(lx+54))<25&&fabsf(pz-(ly+18))<16)continue; /* el coche aparcado del punto (36x18) */
        int clear=1,near=0;
        for(int q=0;q<16&&clear;q++){float b=q*0.3926991f;
            if(cm_block(px+cosf(b)*3.8f,pz+sinf(b)*3.8f))clear=0;
            if(cm_block(px+cosf(b)*6,pz+sinf(b)*6))near=1;}
        if(clear&&near){*tx=px;*tz=pz;return;}
    }
    /* columna junto al rio (x 1300-1400): no hay manzana cerca; va en el paseo de la orilla oeste,
       pegado al muro del rio y lejos de sus arboles (z local 120/180/240) */
    if(ox>1300&&ox<1400){*tx=1386;*tz=ly+44;return;}
    *tx=ox;*tz=oz;
}
#endif
