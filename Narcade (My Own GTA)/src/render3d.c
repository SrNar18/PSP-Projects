/* Narcade 3D: original low-poly meshes, batched by material, PSP GE backend.
 * No allocation in the frame loop. Texture/vertex memory stays bounded.
 */
#include "render3d.h"
#include "world_geo.h"
#include "citymap.h" /* v2.6 (Claude): trazado irregular compartido con game.c */
#include <math.h>
#include <string.h>
#ifndef R3_HOST
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspge.h>
#include <pspdisplay.h>
#endif

#define PI 3.14159265358979323846f
#define MAT_COUNT 43 /* 21 VRAM + 8 NPC + 12 city + 2 weapon RAM materials */
#define VRAM_MATERIALS 21
#ifndef MAX_VERTICES /* v2.26: las pruebas en PC pueden ampliarlo */
#define MAX_VERTICES 6144 /* v2.13.2 (Claude): 8190 -> 6144 = 2 MB menos de RAM estatica (mesh). El pico medido
                                por material es ~5.000; si algun material se pasa solo se descartan poligonos. */
#endif
#include "player_mesh.h"
#define COLOR(r,g,b) (0xff000000u | (r) | ((g)<<8) | ((b)<<16))
enum { ROAD,SIDEWALK,BRICK,STUCCO,SHOP,ROOF,GRASS,WATER,JACKET,JEANS,FACE,WHEEL,CAR_SIDE,CAR_PAINT,GLASS,MURAL,JACKET_BACK,SLEEVE,SKIN,HAIR };
enum { FLAT=20 }; /* Existing opaque white tile, tintable without car-paint glints. */
enum { BARK=29,LEAVES,METAL,CONCRETE,CURTAIN,AWNING,COBBLE,MODERN }; /* v2.7: materiales 64px en RAM (tools/extra_textures.py) */
enum { RETAIL=37,EATERY,OFFICE_FRONT,WORKSHOP_FRONT };
enum { WEAPON_METAL=41,WEAPON_WOOD };
typedef struct { float u,v; uint32_t color; float x,y,z; } Vertex;
typedef struct { float x,y,z; } Point;
static Vertex __attribute__((aligned(16))) mesh[MAT_COUNT][MAX_VERTICES];
static int used[MAT_COUNT];
/* v2.31 (Claude): CACHE DE LA CIUDAD. Medido en la consola real (grabadora v2.29): el juego
   iba a ~9 fps porque cada fotograma se reconstruia en la CPU toda la ciudad visible
   (el 54% del tiempo de dibujo; la logica apenas cuenta). Ahora cada manzana se construye
   una vez por nivel de detalle y se guarda ya proyectada; cada fotograma solo se
   recorta y se envia. Los semaforos se guardan como "dinamicos" (se dibujan en vivo) y
   las manzanas se reconstruyen de una en una para seguir la luz del ciclo dia/noche.
   Si el espacio se agota, esa manzana se dibuja como antes (nunca falta nada). */
#ifndef CITY_CACHE_BYTES
#define CITY_CACHE_BYTES (5u*512u*1024u) /* 2,5 MB (v2.34: el suelo troceado en las transiciones ocupa mas); limite de RAM ~20 MB */
#endif
typedef struct{unsigned char kind,mat,count,lit;}RecHdr; /* kind: 0 poligono, 1 luz, 2 sombra, 3 semaforo */
typedef struct{unsigned off,len;float day;int valid;unsigned chunk0,nchunks,used;}CellCache; /* v2.36: used = ultimo fotograma en que se dibujo */
static unsigned r3Frame;
typedef struct{unsigned off,len;float x,y,z,r;}CacheChunk; /* trozo de ~100 vertices con esfera envolvente (off relativo a la manzana) */
#define CHUNK_MAX 7000
static CacheChunk chunkPool[CHUNK_MAX];static unsigned chunkUsed;
static int noClip; /* v2.31: el trozo entero cae dentro de la vista: sin pruebas de recorte */
static unsigned char __attribute__((aligned(16))) cachePool[CITY_CACHE_BYTES];
static unsigned cacheUsed;static int cacheRec,recFail;static float recDist,recRange;
unsigned long r3CacheNew,r3CacheLight,r3CacheReset; /* diagnostico: reconstrucciones por manzana nueva/LOD, por luz y vaciados */
static CellCache cellCache[64][3]; /* 56 manzanas + 7 tramos de rio/Metro */
static void cache_put(const void *p,unsigned n){
    if(recFail)return;if(cacheUsed+n>CITY_CACHE_BYTES){recFail=1;return;}
    memcpy(cachePool+cacheUsed,p,n);cacheUsed+=n;
}
#ifndef R3_HOST
static unsigned int __attribute__((aligned(16))) commands[65536];
#ifndef R3_HOST
/* v2.37 (Claude): la escena 3D se envia sin esperar al GE; la CPU dibuja el HUD mientras tanto.
   Toda reutilizacion de la lista (y de las mallas en el siguiente r3_draw) espera antes aqui. */
static int geBusy;
static void ge_wait(void){if(geBusy){sceGuSync(0,0);geBusy=0;}}
#endif
extern const unsigned char textures3d_data[];
static void *textureBase;
#endif
static const R3Scene *view;
static float planes[6][4];
static Point eye,target;
static int overflow;
static float r3FogNear=420.f; /* v2.39: inicio de la niebla de distancia */
static int distanceFog;
void r3_set_distance_fog(int enabled){distanceFog=enabled!=0;}
/* Opaque horizon silhouettes, rendered behind the playable city. They hide
   the empty far plane without washing out world textures with distance fog. */
static Vertex __attribute__((aligned(16))) horizonMesh[96*12];
static void horizon_build(void);
static int clipEnabled=1;
static int geographic=0,rigid=0;
static float objectX,objectZ,objectYaw;
static float fixedGround=-1000000;
static float npcFall=0,npcPhase=0,npcFlee=0;

static Point point(float x,float y,float z){Point p={x,y,z};return p;}
static uint32_t shade(uint32_t c,float f){return COLOR((int)((c&255)*f),(int)(((c>>8)&255)*f),(int)(((c>>16)&255)*f));}
static void camera(const R3Scene *s){
    float px,pz;geo_project(s->x,s->z,&px,&pz);float yaw=s->yaw;
    float h=s->camBase!=0?s->camBase:geo_height(s->x,s->z)+s->lift; /* v2.6: anden del Metro; v2.30: base suavizada */
    eye=point(px-cosf(yaw)*s->cameraDistance,h+(s->eyeHeight>0?s->eyeHeight:(s->driving?54:43)),pz-sinf(yaw)*s->cameraDistance);
    float lx,lz;geo_unproject(eye.x,eye.z,&lx,&lz);eye.y=fmaxf(eye.y,s->camClear!=0?s->camClear:geo_height(lx,lz)+12);
    target=point(px+cosf(yaw)*25,h+(s->driving?10:11),pz+sinf(yaw)*25);
    if(s->inMetro){ /* v2.6: camara dentro del coche del Metro, mirando en el sentido de la marcha */
        float dir=s->metroDir>0?1:-1,rideZ=s->metroZ+dir*(CM_TRAIN_CAR*.5f+3-26);float mx,mz;geo_project(CM_METRO_X,rideZ,&mx,&mz);float base=geo_height(CM_METRO_X,rideZ)+CM_PLAT_H;
        eye=point(mx-3,base+13,mz);float tx,tz;geo_project(CM_METRO_X,s->metroZ+dir*160,&tx,&tz);target=point(tx,geo_height(CM_METRO_X,s->metroZ+dir*160)+CM_PLAT_H+9,tz); /* altura de la via bajo la camara y a lo lejos */
    }else if(s->lift-s->jump>15){ /* en el anden: camara baja y cercana para no ver la marquesina desde arriba */
        float d=s->cameraDistance<48?s->cameraDistance:48;
        eye=point(px-cosf(yaw)*d,h+16,pz-sinf(yaw)*d);target=point(px+cosf(yaw)*25,h+9,pz+sinf(yaw)*25);
    }
#ifdef NARCADE_TOPVIEW
    /* solo pruebas: vista aerea oblicua para revisar el trazado (NARCADE_EXTRA_CFLAGS=-DNARCADE_TOPVIEW) */
    eye=point(px-cosf(yaw)*260,h+NARCADE_TOPVIEW,pz-sinf(yaw)*260);target=point(px+cosf(yaw)*60,h,pz+sinf(yaw)*60);
#endif
    if(s->aiming&&!s->driving&&!s->inMetro){
        float cp=cosf(yaw),sp=sinf(yaw),pitch=tanf(s->cameraPitch),d=geo_clamp(s->cameraDistance,10,38);
        float base=geo_height(s->x,s->z)+s->lift;
        eye=point(px-cp*d-sp*5,base+14,pz-sp*d+cp*5);
        float lx,lz;geo_unproject(eye.x,eye.z,&lx,&lz);eye.y=fmaxf(eye.y,geo_height(lx,lz)+2);
        target=point(px+cp*60-sp*5,base+10+pitch*60,pz+sp*60+cp*5);
    }
    float fx=target.x-eye.x,fy=target.y-eye.y,fz=target.z-eye.z;
    float n=sqrtf(fx*fx+fy*fy+fz*fz);fx/=n;fy/=n;fz/=n;
    float rx=-fz,rz=fx,rn=sqrtf(rx*rx+rz*rz);rx/=rn;rz/=rn;
    float ux=-rz*fy,uy=rz*fx-rx*fz,uz=rx*fy;
    float tv=tanf(31*PI/180),th=tv*480/272;
    float normals[6][3]={{fx,fy,fz},{-fx,-fy,-fz},{fx*th+rx,fy*th,fz*th+rz},{fx*th-rx,fy*th,fz*th-rz},{fx*tv+ux,fy*tv+uy,fz*tv+uz},{fx*tv-ux,fy*tv-uy,fz*tv-uz}};
    for(int k=0;k<6;k++){
        memcpy(planes[k],normals[k],3*sizeof(float));
        planes[k][3]=-normals[k][0]*eye.x-normals[k][1]*eye.y-normals[k][2]*eye.z;
    }
    planes[0][3]-=5.1f;planes[1][3]+=710; /* v2.6: acorde con sceGumPerspective(...,5,720) */
    /* Sphere distances require unit normals, including the side planes. */
    for(int k=0;k<6;k++){
        float length=sqrtf(planes[k][0]*planes[k][0]+planes[k][1]*planes[k][1]+planes[k][2]*planes[k][2]);
        for(int j=0;j<4;j++)planes[k][j]/=length;
    }
}
int r3_target_screen(float x,float z,float height,float *sx,float *sy){
    float px,pz;geo_project(x,z,&px,&pz);Point p=point(px-eye.x,geo_height(x,z)+height-eye.y,pz-eye.z);
    float fx=target.x-eye.x,fy=target.y-eye.y,fz=target.z-eye.z,n=sqrtf(fx*fx+fy*fy+fz*fz);fx/=n;fy/=n;fz/=n;
    float rx=-fz,rz=fx,rn=hypotf(rx,rz);rx/=rn;rz/=rn;
    float ux=-rz*fy,uy=rz*fx-rx*fz,uz=rx*fy;
    float depth=p.x*fx+p.y*fy+p.z*fz;if(depth<=5)return 0;
    float focal=136/tanf(31*PI/180);
    *sx=240+(p.x*rx+p.z*rz)*focal/depth;*sy=136-(p.x*ux+p.y*uy+p.z*uz)*focal/depth;
    return *sx>12&&*sx<468&&*sy>12&&*sy<250;
}
static float plane_distance(const Vertex *v,int k){return planes[k][0]*v->x+planes[k][1]*v->y+planes[k][2]*v->z+planes[k][3];}
static Vertex interpolate(Vertex a,Vertex b,float t){
    Vertex v={a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t,a.color,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};return v;
}
/* v2.5 (Claude): iluminacion por hora del dia (src/daylight.inc) y ciudad irregular (src/city3d.inc). */
static uint32_t lit_color(uint32_t c,float nx,float ny,float nz);
static uint32_t day_scale(uint32_t c);
static uint32_t emissive_color(int mat,uint32_t c);
static void facade_lights(Point a,Point b,float height,float nx,float nz,int material);
static int litAlready=0; /* box()/ground() ya iluminan por cara: polygon() no vuelve a atenuar */
/* Optimizacion (Claude): en modo rigido todos los vertices de un objeto comparten proyeccion, rumbo y altura;
   antes se recalculaban (geo_project + geo_heading con atan2, cos, sin) para CADA vertice. Se cachean por objeto. */
static float rigX=1e30f,rigZ=1e30f,rigYaw=1e30f,rigGX,rigGZ,rigCos,rigSin,rigH;
static int rigMode=-1;
static Point rigBX,rigBY,rigBZ;
static Point unit(Point p){float n=sqrtf(p.x*p.x+p.y*p.y+p.z*p.z);return point(p.x/n,p.y/n,p.z/n);}
static Point cross3(Point a,Point b){return point(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static float projected_height(float x,float z){float lx,lz;geo_unproject(x,z,&lx,&lz);return geo_height(lx,lz);}
static float playerLift=0; /* v2.6: altura extra del jugador (anden del Metro) */
static int metroFlexible;static float metroBaseGround; /* train only; never city cache */
static void rigid_cache(void){
    if(objectX==rigX&&objectZ==rigZ&&objectYaw==rigYaw&&rigid==rigMode)return;
    rigMode=rigid;
    rigX=objectX;rigZ=objectZ;rigYaw=objectYaw;
    geo_project(objectX,objectZ,&rigGX,&rigGZ);
    float da=geo_heading(objectX,objectZ,objectYaw)-objectYaw;rigCos=cosf(da);rigSin=sinf(da);
    rigH=geo_height(objectX,objectZ)+playerLift;
    if(rigid==2){
        float a=geo_heading(objectX,objectZ,objectYaw),c=cosf(a),s=sinf(a);
        float pitch=(projected_height(rigGX+c*14,rigGZ+s*14)-projected_height(rigGX-c*14,rigGZ-s*14))/28;
        float roll=(projected_height(rigGX-s*8,rigGZ+c*8)-projected_height(rigGX+s*8,rigGZ-c*8))/16;
        Point forward=unit(point(c,geo_clamp(pitch,-.22f,.22f),s));
        Point up=unit(cross3(point(-s,geo_clamp(roll,-.22f,.22f),c),forward));
        Point right=unit(cross3(forward,up));float oc=cosf(objectYaw),os=sinf(objectYaw);
        rigBX=point(forward.x*oc-right.x*os,forward.y*oc-right.y*os,forward.z*oc-right.z*os);
        rigBZ=point(forward.x*os+right.x*oc,forward.y*os+right.y*oc,forward.z*os+right.z*oc);rigBY=up;
    }
}
static void polygon_emit(int mat,Vertex *input,int count,int lit);
static void polygon(int mat,Vertex *input,int count){
    Vertex buffers[2][16];memcpy(buffers[0],input,count*sizeof(Vertex));int src=0;
    if(geographic)for(int i=0;i<count;i++){
        Vertex *v=&buffers[0][i];float gx,gz;
        if(rigid){
            rigid_cache();
            float dx=v->x-objectX,dz=v->z-objectZ;
            if(rigid==1&&npcFall>0){
                float c=cosf(objectYaw),q=sinf(objectYaw),f=dx*c+dz*q,l=-dx*q+dz*c;
                float sn=sinf(npcFall*PI*.5f),cs=cosf(npcFall*PI*.5f),nf=f*cs+v->y*sn;
                v->y=fmaxf(.15f,v->y*cs-f*sn+2.8f*sn);dx=nf*c-l*q;dz=nf*q+l*c;
            }
            if(rigid==2){float h=v->y;
                v->x=rigGX+rigBX.x*dx+rigBY.x*h+rigBZ.x*dz;
                v->z=rigGZ+rigBX.z*dx+rigBY.z*h+rigBZ.z*dz;
                v->y=rigH+rigBX.y*dx+rigBY.y*h+rigBZ.y*dz;
            }else{v->x=rigGX+rigCos*dx-rigSin*dz;v->z=rigGZ+rigSin*dx+rigCos*dz;v->y+=rigH;}
        }else{geo_project(v->x,v->z,&gx,&gz);
            /* The moving train spans more than one terrain terrace. Project
               each skin vertex at the rail centreline below it, rather than
               lifting both long cars abruptly when their centres cross a ramp. */
            if(metroFlexible)v->y+=geo_height(CM_METRO_X,v->z)-metroBaseGround;
            v->y+=fixedGround>-999999?fixedGround:geo_height(v->x,v->z);v->x=gx;v->z=gz;}
    }
    if(cacheRec){RecHdr h={0,(unsigned char)mat,(unsigned char)count,(unsigned char)litAlready};cache_put(&h,4);cache_put(buffers[0],count*sizeof(Vertex));return;}
    polygon_emit(mat,buffers[0],count,litAlready);
}
static void polygon_emit(int mat,Vertex *input,int count,int lit){
    /* v2.41 (Claude): ruta rapida. Un poligono ya iluminado de un trozo de cache entero dentro de la vista
       (noClip) va directo de la cache a la malla, sin copias intermedias ni pruebas de planos. */
    if(noClip&&lit&&mat!=WATER&&count>=3){int needed=(count-2)*3;if(used[mat]+needed>MAX_VERTICES){overflow++;return;}
        Vertex *p=mesh[mat]+used[mat];used[mat]+=needed;const Vertex *v0=input;
        for(int j=1;j<count-1;j++){*p++=*v0;*p++=input[j];*p++=input[j+1];}return;}
    Vertex buffers[2][16];memcpy(buffers[0],input,count*sizeof(Vertex));int src=0;
    /* Animate after cache replay, so a cached river never freezes its flow.
       World-space UVs join the current across every strip and row. */
    if(mat==WATER){float flow=fmodf(view->time*.055f,1.f);
        for(int i=0;i<count;i++){buffers[0][i].u=buffers[0][i].x/24.f;buffers[0][i].v=buffers[0][i].z/36.f+flow;}}
    /* PSP rejects large triangles crossing its near/guard planes. Clip in
       world space before submission, including UV interpolation at cuts.
       Optimizacion (Claude): primero una prueba trivial por plano; si todos los vertices quedan fuera de un plano se
       descarta el poligono entero, y solo se recorta contra los planos que realmente cruza. */
    if(clipEnabled&&!noClip&&count>=3){
        unsigned crossing=0;
        for(int k=0;k<6;k++){
            int inside=0;
            for(int j=0;j<count;j++)if(plane_distance(&buffers[0][j],k)>=0)inside++;
            if(inside==0)return;
            if(inside<count)crossing|=1u<<k;
        }
        for(int k=0;crossing&&k<6&&count>=3;k++){
            if(!(crossing&(1u<<k)))continue;
            int out=0;Vertex a=buffers[src][count-1];float da=plane_distance(&a,k);
            for(int j=0;j<count;j++){
                Vertex b=buffers[src][j];float db=plane_distance(&b,k);
                if((da>=0)!=(db>=0))buffers[src^1][out++]=interpolate(a,b,da/(da-db));
                if(db>=0)buffers[src^1][out++]=b;
                a=b;da=db;
            }
            count=out;src^=1;
        }
    }
    if(count<3)return;
    if(!lit)for(int j=0;j<count;j++)buffers[src][j].color=day_scale(buffers[src][j].color);
    int needed=(count-2)*3;
    if(used[mat]+needed>MAX_VERTICES){overflow++;return;}
    Vertex *p=mesh[mat]+used[mat];used[mat]+=needed;
    for(int j=1;j<count-1;j++){*p++=buffers[src][0];*p++=buffers[src][j];*p++=buffers[src][j+1];}
}
/* Optimizacion (Claude): descarte por esfera envolvente contra el frustum (en coordenadas proyectadas). */
static int sphere_visible(float x,float z,float radius){
    if(!clipEnabled||cacheRec)return 1; /* offline mesh export / grabacion de cache */
    float gx,gz;geo_project(x,z,&gx,&gz);float gy=geo_height(x,z)+radius*.4f;
    for(int k=0;k<6;k++)if(planes[k][0]*gx+planes[k][1]*gy+planes[k][2]*gz+planes[k][3]<-radius)return 0;
    return 1;
}
static float view_distance(float x,float z){if(cacheRec)return recDist;float dx=x-view->x,dz=z-view->z;return sqrtf(dx*dx+dz*dz);}
/* Floors and roofs have an absolute elevation. Testing them with a sphere
   centred near the ground incorrectly removed middle/upper building sections. */
static int volume_visible(float x,float z,float bottom,float top,float planRadius){
    if(!clipEnabled||cacheRec)return 1;
    float gx,gz;geo_project(x,z,&gx,&gz);
    float gy=(fixedGround>-999999?fixedGround:geo_height(x,z))+(bottom+top)*.5f;
    float r=hypotf(planRadius*2,(top-bottom)*.5f)+4;
    for(int k=0;k<6;k++)if(planes[k][0]*gx+planes[k][1]*gy+planes[k][2]*gz+planes[k][3]<-r)return 0;
    return 1;
}
/* v2.6 (Claude): cara lateral con degradado vertical (oclusion ambiental falsa: mas oscura abajo). a,b arriba; c,d abajo. */
static void quad2(int m,Point a,Point b,Point c,Point d,uint32_t color,float u,float v){
    uint32_t low=shade(color,.78f);
    Vertex p[4]={{0,0,color,a.x,a.y,a.z},{u,0,color,b.x,b.y,b.z},{u,v,low,c.x,c.y,c.z},{0,v,low,d.x,d.y,d.z}};
    polygon(m,p,4);
}
static void quad(int m,Point a,Point b,Point c,Point d,uint32_t color,float u,float v){
    Vertex p[4]={{0,0,color,a.x,a.y,a.z},{u,0,color,b.x,b.y,b.z},{u,v,color,c.x,c.y,c.z},{0,v,color,d.x,d.y,d.z}};
    polygon(m,p,4);
}
/* Optimizacion (Claude): la misma rotacion se repite miles de veces por fotograma (todos los vertices de un
   objeto comparten angulo). Cachear seno/coseno del ultimo angulo evita dos llamadas trigonometricas por vertice. */
static float localA=1e30f,localC=1,localS=0;
#define PERSON_SCALE 0.52f /* v2.6: 1.75 m = 15 unidades; un piso (24) = 1.6 personas, como en la realidad */
#define PERSON_SCALE_OLD 0.62f /* v2.5 (Claude): personas a escala de la ciudad (un piso = 1.35 personas, coche = 2 personas) */
static float localScale=1; /* factor aplicado a las coordenadas locales (personas) */
static Point local(float x,float y,float z,float cx,float cz,float a){
    if(a!=localA){localA=a;localC=cosf(a);localS=sinf(a);}
    x*=localScale;y*=localScale;z*=localScale;
    return point(cx+x*localC-z*localS,y,cz+x*localS+z*localC);
}
/* Object forward is +X. Faces have UVs with their top at v=0. */
static void box(float x,float z,float bottom,float length,float width,float height,float angle,int side,int top,uint32_t color){
    /* v2.33 (Claude): una caja larga que cruza una franja de transicion del valle se dibujaba
       recta en pantalla y se salia de su manzana (edificios y puente sobre la calzada). Se
       trocea a lo largo de z para que siga la misma curva que el mapa. */
    if(geographic&&!rigid&&!metroFlexible&&angle==0&&width>40){float z0=z-width*.5f,z1=z+width*.5f;
        if(floorf((z0-86.f)/320.f)!=floorf((z1-86.f)/320.f)){
            /* cortes cada ~14 solo dentro de las franjas de transicion (z local 0..86); fuera, una pieza */
            float cut[48];int nc=0;cut[nc++]=z0;
            for(int r=(int)floorf(z0/320.f);r<=(int)floorf(z1/320.f)&&nc<44;r++){
                float t0=fmaxf(z0,r*320.f),t1=fminf(z1,r*320.f+86.f);if(t1<=t0)continue;
                int k=(int)ceilf((t1-t0)/14.f);for(int i=0;i<=k&&nc<46;i++){float c=t0+(t1-t0)*i/k;if(c>cut[nc-1]+.01f&&c<z1-.01f)cut[nc++]=c;}
            }
            cut[nc++]=z1;
            for(int i=0;i+1<nc;i++)box(x,(cut[i]+cut[i+1])*.5f,bottom,length,cut[i+1]-cut[i],height,0,side,top,color);
            return;}}
    float l=length*.5f,w=width*.5f,h=bottom+height;
    float savedGround=fixedGround;
    if(geographic&&!rigid){
        if(fixedGround>-999999){ /* v2.6: nivel de parcela impuesto desde fuera (podio) */
            if(bottom==0){float low=geo_height(x,z);for(int a=-1;a<=1;a+=2)for(int b=-1;b<=1;b+=2){Point p=local(a*l,0,b*w,x,z,angle);float hh=geo_height(p.x,p.z);if(hh<low)low=hh;}bottom=low-fixedGround-1;} /* cimiento hasta el terreno mas bajo: nada flota */
        }
        else{
        float low=geo_height(x,z),high=low;
        for(int a=-1;a<=1;a+=2)for(int b=-1;b<=1;b+=2){Point p=local(a*l,0,b*w,x,z,angle);float hh=geo_height(p.x,p.z);low=fminf(low,hh);high=fmaxf(high,hh);}
        fixedGround=high;
        if(bottom==0)bottom=low-high-1; /* Foundation reaches the downhill ground. */
        }
    }
    /* v2.6: descarte por esfera de la caja entera antes de generar 5 poligonos (la mitad de una celda visible queda
       fuera del encuadre). Texturas: repeticion segun la proporcion de la cara para no estirar postes, tableros, vias. */
    if(geographic&&!rigid&&!volume_visible(x,z,bottom,h,hypotf(l,w))){fixedGround=savedGround;return;}
    float ul=1,uw=1,vl=1,vw=1;
    if(length>height*2.2f)ul=length/(height>8?height:8)*.5f;if(height>length*2.2f)vl=height/(length>3?length:3)*.5f;
    if(width>height*2.2f)uw=width/(height>8?height:8)*.5f;if(height>width*2.2f)vw=height/(width>3?width:3)*.5f;
    if(ul>8)ul=8;if(uw>8)uw=8;if(vl>8)vl=8;if(vw>8)vw=8;float ut=1,vt=1;
    if((side==BRICK||side==STUCCO||side==SHOP||side==CURTAIN||side==MODERN||
        side==RETAIL||side==EATERY||side==OFFICE_FRONT||side==WORKSHOP_FRONT)&&height>14){
        ul=fmaxf(1,length/54);uw=fmaxf(1,width/54);vl=vw=fmaxf(1,height/24);
    }
    if(length>width*2.2f)ut=length/(width>4?width:4)*.5f;if(width>length*2.2f)vt=width/(length>4?length:4)*.5f;if(ut>8)ut=8;if(vt>8)vt=8;
    Point p[8]={local(-l,bottom,-w,x,z,angle),local(l,bottom,-w,x,z,angle),local(l,bottom,w,x,z,angle),local(-l,bottom,w,x,z,angle),
        local(-l,h,-w,x,z,angle),local(l,h,-w,x,z,angle),local(l,h,w,x,z,angle),local(-l,h,w,x,z,angle)};
    /* v2.5: cada cara segun su orientacion respecto al sol (angle rota las normales locales). */
    float ca=cosf(angle),sa=sinf(angle);uint32_t sc=emissive_color(side,color);
    litAlready=1;
    int ao=height>14&&bottom<=6.5f;  /* degradado solo en volumenes que arrancan del suelo/podio */
    void (*q)(int,Point,Point,Point,Point,uint32_t,float,float)=ao?quad2:quad;
    q(side,p[4],p[5],p[1],p[0],lit_color(sc,sa,0,-ca),ul,vl);   /* cara -z */
    q(side,p[6],p[7],p[3],p[2],lit_color(sc,-sa,0,ca),ul,vl);   /* cara +z */
    q(side,p[5],p[6],p[2],p[1],lit_color(sc,ca,0,sa),uw,vw);    /* cara +x */
    q(side,p[7],p[4],p[0],p[3],lit_color(sc,-ca,0,-sa),uw,vw);  /* cara -x */
    quad(top,p[7],p[6],p[5],p[4],lit_color(color,0,1,0),ut,vt);
    if(!rigid&&height>18){facade_lights(p[0],p[1],height,sa,-ca,side);facade_lights(p[3],p[2],height,-sa,ca,side);facade_lights(p[1],p[2],height,ca,sa,side);facade_lights(p[0],p[3],height,-ca,-sa,side);}
    litAlready=0;
    fixedGround=savedGround;
}
#include "street_surface.inc"
static void ground(int mat,float x,float z,float w,float d,float y,uint32_t color,float repeat){
    /* Subdivide at terrain lattice boundaries: roads genuinely climb hills. */
    for(float zz=z;zz<z+d-.001f;){float endz=fminf(z+d,geo_next_z(zz));if(endz<=zz+.001f)endz=fminf(z+d,zz+.01f);
        /* v2.34 (Claude): dentro de la franja de transicion del valle (z local 0..86) el borde de la
           ciudad se curva mucho: tramos de 8 en vez de uno solo de 86. Antes quedaba un hueco sin
           suelo en el borde oeste (justo donde empieza la partida, junto al taller de Luna). */
        {float lz=zz-floorf(zz/320.f)*320.f;if(lz<86.f)endz=fminf(endz,zz+8.f);}
        for(float xx=x;xx<x+w-.001f;){float endx=fminf(x+w,(floorf(xx/80)+1)*80);if(endx<=xx+.001f)endx=fminf(x+w,xx+80);
            uint32_t lc=lit_color(color,0,1,0);float ou=0,ov=0;if(mat==WATER){ou=view->time*.045f;ov=view->time*.11f;} /* v2.7: el agua fluye */
            Vertex v[4]={{(xx-x)/w*repeat+ou,(zz-z)/d*repeat+ov,lc,xx,y,zz},{(endx-x)/w*repeat+ou,(zz-z)/d*repeat+ov,lc,endx,y,zz},
                         {(endx-x)/w*repeat+ou,(endz-z)/d*repeat+ov,lc,endx,y,endz},{(xx-x)/w*repeat+ou,(endz-z)/d*repeat+ov,lc,xx,y,endz}};
            /* World-space phase and fixed tile size prevent adjacent parcels
               from changing the apparent paving scale as the camera moves. */
            if(mat==ROAD||mat==SIDEWALK){float scale=mat==ROAD?64.f:28.f;
                for(int k=0;k<4;k++){v[k].u=v[k].x/scale;v[k].v=v[k].z/scale;v[k].color=street_tint(lc,v[k].x,v[k].z);}}
            litAlready=1;polygon(mat,v,4);litAlready=0;xx=endx;
        }zz=endz;
    }
}
static int nearby(float x,float z,float range){if(cacheRec)return range>=recRange;float dx=x-view->x,dz=z-view->z;return dx*dx+dz*dz<range*range;}
static int park(int x,int z){return cm_park(x,z);}
static int cityMid; /* LOD intermedio (definido en city3d.inc) */
static void tree_shape(float x,float z,int kind,int simple); /* shapes.inc */
static int treeKind=-1; /* -1: por hash de posicion; 0 frondoso, 1 palma, 2 cipres */
static void tree(float x,float z){
    float savedGround=fixedGround;if(savedGround<=-999999)fixedGround=geo_height(x,z);
    int kind=treeKind;
    if(kind<0){unsigned h=(unsigned)(x*1.7f)*2654435761u^(unsigned)(z*2.3f)*40503u;h^=h>>11;unsigned r=h%100;kind=r<58?0:r<80?1:2;}
    tree_shape(x,z,kind,cityMid||view_distance(x,z)>170);
    fixedGround=savedGround;
}
static void (*phaseFn2)(const char*)=0;
static void r3_phase(const char *s){if(phaseFn2)phaseFn2(s);}
static void (*traceFn2)(const char*)=0;static int *traceFrames2=0;
static void r3_trace_cell(const char *s){if(traceFn2&&traceFrames2&&*traceFrames2>0)traceFn2(s);}
static void fx_triangle(Vertex a,Vertex b,Vertex c,int shadow);static float dayT; /* v2.31: usados por la cache (definidos en daylight.inc) */
#include "city3d.inc"
#include "city26.inc"
#include "shapes.inc"
#include "daylight.inc"
static void city(void){city_v26();}
#include "car_detail.inc"
static void car(const R3Car *c){
    if(!nearby(c->x,c->z,500))return;
    if(!sphere_visible(c->x,c->z,30))return;
    float dist=view_distance(c->x,c->z);
    static const uint32_t colors[]={COLOR(93,196,173),COLOR(245,198,75),COLOR(221,106,86),COLOR(232,230,211),COLOR(108,157,207),COLOR(167,124,182),
        COLOR(48,88,112),COLOR(159,64,65),COLOR(195,204,200),COLOR(142,112,80),COLOR(82,126,91),COLOR(72,74,87)};
    uint32_t paint=c->police?COLOR(207,226,229):colors[(unsigned)c->paint%12];
    int type=c->police?0:c->type%6;
    if(dist>170||used[METAL]>5400||used[CAR_PAINT]>5200||used[CAR_SIDE]>4800){ /* preserve material capacity in dense traffic */
        box(c->x,c->z,3,36,18,7,c->angle,CAR_PAINT,CAR_PAINT,paint);box(c->x,c->z,10,20,15,7,c->angle,CAR_SIDE,CAR_PAINT,paint);
        if(dist<=170)for(int side=-1;side<=1;side+=2)for(int end=-1;end<=1;end+=2){Point p=local(end*18.3f,0,side*5.5f,c->x,c->z,c->angle);
            box(p.x,p.z,5.5f,.8f,3.6f,1.8f,c->angle,FLAT,FLAT,end>0?COLOR(255,244,192):COLOR(255,61,42));}
        return;
    }
    /* v2.7 (Claude): carrocerias por secciones (perfil lateral real: capo, parabrisas inclinado, techo, luneta,
       maletero; laterales achaflanados) en 6 siluetas: sedan, hatchback, pickup, furgoneta, deportivo, SUV. */
    #define P CAR_PAINT
    #define G GLASS
    #define CP (200+CAR_PAINT)
    #define CG (200+GLASS)
    static const HullSec sedan[8]={{-18,8,8},{-16,10.5f,8.6f},{-9,11,8.8f},{-4,16.5f,8},{5,16.8f,8},{10,11.5f,8.8f},{17,10,8.4f},{18,8,7.6f}};
    static const unsigned char sedanM[7]={P,P,CG,CP,CG,P,P};
    static const HullSec hatch[7]={{-18,8,8},{-16,15,8.4f},{-8,16.5f,8.2f},{3,16.5f,8.2f},{9,11.5f,8.8f},{17,10,8.4f},{18,8,7.6f}};
    static const unsigned char hatchM[6]={P,CG,CP,CG,P,P};
    static const HullSec pickup[7]={{-19,9,8.6f},{-6,9,8.6f},{-6,17,8.4f},{2,17,8.4f},{7,11.5f,8.8f},{17,10.5f,8.6f},{19,8,7.8f}};
    static const unsigned char pickupM[6]={P,CP,CP,CG,P,P};
    static const HullSec van[6]={{-20,9,9},{-19,20,9},{2,20.5f,9},{12,15,9},{19,11,8.8f},{20,8,8}};
    static const unsigned char vanM[5]={P,CP,CG,P,P};
    static const HullSec sport[8]={{-18,7,8.5f},{-15,10,9},{-6,10.5f,9},{-1,14.5f,8.4f},{6,14.5f,8.4f},{11,9.5f,9},{18,8,8.4f},{18.5f,6,7.5f}};
    static const unsigned char sportM[7]={P,P,CG,CP,CG,P,P};
    static const HullSec suv[7]={{-18,9,8.8f},{-17,18,8.8f},{-2,18.5f,8.6f},{7,18,8.6f},{12,13,9},{17.5f,12,8.8f},{18,9,8}};
    static const unsigned char suvM[6]={P,CG,CP,CG,P,P};
    #undef P
    #undef G
    #undef CP
    #undef CG
    const HullSec *prof;const unsigned char *mats;int n;float floor=3,wr=3.6f;
    switch(type){
        case 1:prof=hatch;mats=hatchM;n=7;break;
        case 2:prof=pickup;mats=pickupM;n=7;break;
        case 3:prof=van;mats=vanM;n=6;wr=3.8f;break;
        case 4:prof=sport;mats=sportM;n=8;floor=2.6f;wr=3.3f;break;
        case 5:prof=suv;mats=suvM;n=7;floor=4;wr=4.2f;break;
        default:prof=sedan;mats=sedanM;n=8;break;
    }
    hull(c->x,c->z,c->angle,prof,n,mats,floor,paint,dist<100?wr:0);
    /* bajos oscuros */
    box(c->x,c->z,floor-1.2f,prof[n-1].x-prof[0].x-4,prof[0].w*1.7f,1.2f,c->angle,METAL,METAL,COLOR(40,40,42));
    if(dist>300)return; /* LOD: sin ruedas detalladas a lo lejos */
    float spin=view->time*c->speed*.08f;
    for(int s=-1;s<=1;s+=2)for(int e=-1;e<=1;e+=2)wheel(c->x,c->z,c->angle,e*11,wr,s*(prof[0].w+.4f),wr,2.6f,spin);
    for(int s=-1;s<=1;s+=2){
        Point p=local(prof[n-1].x+.3f,0,s*5.5f,c->x,c->z,c->angle);
        box(p.x,p.z,floor+2.5f,.8f,3.6f,1.8f,c->angle,FLAT,FLAT,COLOR(255,244,192));
        p=local(prof[0].x-.3f,0,s*5.5f,c->x,c->z,c->angle);
        box(p.x,p.z,floor+2.5f,.8f,3.6f,1.8f,c->angle,FLAT,FLAT,COLOR(255,61,42));
    }
    if(used[METAL]>3000)return; /* lights remain visible when trim is omitted */
    car_windscreen_trim(c->x,c->z,c->angle,prof,n,mats,paint);
    for(int side=-1;side<=1;side+=2){
        car_arch(c->x,c->z,c->angle,prof,n,floor,-11.f,wr,side,paint);
        car_arch(c->x,c->z,c->angle,prof,n,floor,11.f,wr,side,paint);
        car_lens_detail(c->x,c->z,c->angle,prof,n,floor,side);
    }
    /* Mirrors sit beside the forward window, following each cabin's width. */
    float mirrorX=type==3?9:type==2?6:type==4?9:type==5?10:8;
    for(int s=-1;s<=1;s+=2){Point p=car_skin(c->x,c->z,c->angle,prof,n,floor,mirrorX,11.8f,s,1.25f);
        box(p.x,p.z,p.y,1.9f,2.4f,1.15f,c->angle,METAL,METAL,shade(paint,.83f));}
    /* Modelled bumpers, grille, plates and handles remain straight when the
       body narrows; their details no longer depend on a stretched door bitmap. */
    for(int end=-1;end<=1;end+=2){
        Point bumper=local(end*18.5f,0,0,c->x,c->z,c->angle);
        box(bumper.x,bumper.z,3.3f,1.6f,15.8f,2.1f,c->angle,FLAT,FLAT,shade(paint,.55f));
        Point plate=local(end*19.5f,0,0,c->x,c->z,c->angle);
        box(plate.x,plate.z,5.8f,.55f,4.7f,1.5f,c->angle,FLAT,FLAT,COLOR(229,214,155));
    }
    Point grille=local(19.1f,0,0,c->x,c->z,c->angle);
    box(grille.x,grille.z,7.3f,.45f,7.3f,2.3f,c->angle,FLAT,FLAT,COLOR(33,39,41));
    /* Door shut lines and recessed handles are attached to the correct side
       surface. Their x positions follow each model's cabin, not a shared box. */
    static const float seamX[6][3]={{-9,0,10},{-14,0,9},{-6,7,99},{-9,7,99},{-1,11,99},{-14,0,11}};
    static const float handleX[6][2]={{-2,7},{-4,6},{4,99},{-3,8},{7,99},{-4,8}};
    for(int side=-1;side<=1;side+=2){
        for(int k=0;k<3;k++)if(seamX[type][k]<90)car_seam(c->x,c->z,c->angle,prof,n,floor,seamX[type][k],side);
        for(int k=0;k<2;k++)if(handleX[type][k]<90){
            float hx=handleX[type][k],hy=10.25f;
            Point a=car_skin(c->x,c->z,c->angle,prof,n,floor,hx-1.35f,hy-.38f,side,.26f);
            Point b=car_skin(c->x,c->z,c->angle,prof,n,floor,hx+1.35f,hy-.38f,side,.26f);
            Point d=car_skin(c->x,c->z,c->angle,prof,n,floor,hx-1.35f,hy+.38f,side,.26f);
            Point e=car_skin(c->x,c->z,c->angle,prof,n,floor,hx+1.35f,hy+.38f,side,.26f);
            quad(METAL,a,b,e,d,day_scale(COLOR(48,55,58)),1,1);
            a=car_skin(c->x,c->z,c->angle,prof,n,floor,hx-1.06f,hy+.06f,side,.37f);
            b=car_skin(c->x,c->z,c->angle,prof,n,floor,hx+1.06f,hy+.06f,side,.37f);
            d=car_skin(c->x,c->z,c->angle,prof,n,floor,hx-1.06f,hy+.29f,side,.37f);
            e=car_skin(c->x,c->z,c->angle,prof,n,floor,hx+1.06f,hy+.29f,side,.37f);
            quad(METAL,a,b,e,d,day_scale(COLOR(216,223,223)),1,1);
        }
    }
    if(type==2){ /* bed rails give the pickup a distinct open cargo section */
        for(int side=-1;side<=1;side+=2){Point rail=local(-12,0,side*8.3f,c->x,c->z,c->angle);
            box(rail.x,rail.z,11,12,1,1.2f,c->angle,METAL,METAL,shade(paint,.65f));}
    }else if(type==5){for(int side=-1;side<=1;side+=2){Point rail=local(0,0,side*6.2f,c->x,c->z,c->angle);
        box(rail.x,rail.z,19,19,.75,.75,c->angle,METAL,METAL,COLOR(74,82,84));}}
    else if(type==4){Point spoiler=local(-16,0,0,c->x,c->z,c->angle);
        box(spoiler.x,spoiler.z,12.5f,2,14,1,c->angle,CAR_PAINT,CAR_PAINT,shade(paint,.8f));}
    if(c->police)box(c->x,c->z,prof[3].y+.6f,3,13,2,c->angle,CAR_PAINT,CAR_PAINT,((int)(view->time*5)&1)?COLOR(255,65,49):COLOR(45,147,255));
}
static const float ringC[9]={1,.7071068f,0,-.7071068f,-1,-.7071068f,0,.7071068f,1};
static const float ringS[9]={0,.7071068f,1,.7071068f,0,-.7071068f,-1,-.7071068f,0};
/* Tapered eight-sided segments with real elbow/knee joints, not floating boxes. */
static void limb(Point a,Point b,float r0,float r1,int mat,uint32_t tint,float x,float z,float angle){
    float dx=b.x-a.x,dy=b.y-a.y,n=sqrtf(dx*dx+dy*dy);
    float tx=n>.001f?dy/n:1,ty=n>.001f?-dx/n:0;
    for(int k=0;k<8;k++){
        Point p=local(a.x+tx*ringC[k]*r0,a.y+ty*ringC[k]*r0,a.z+ringS[k]*r0,x,z,angle);
        Point q=local(a.x+tx*ringC[k+1]*r0,a.y+ty*ringC[k+1]*r0,a.z+ringS[k+1]*r0,x,z,angle);
        Point r=local(b.x+tx*ringC[k+1]*r1,b.y+ty*ringC[k+1]*r1,b.z+ringS[k+1]*r1,x,z,angle);
        Point s=local(b.x+tx*ringC[k]*r1,b.y+ty*ringC[k]*r1,b.z+ringS[k]*r1,x,z,angle);
        quad(mat,p,q,r,s,shade(tint,.78f+.2f*fabsf(ringC[k])),1,1);
    }
}
static void simple_person_body(float x,float z,float angle,int style,int walking);
static void simple_person(float x,float z,float angle,int style,int walking){
    if(!nearby(x,z,270))return;
    if(!sphere_visible(x,z,22))return;
    localScale=PERSON_SCALE;simple_person_body(x,z,angle,style,walking);localScale=1;
}
static void simple_person_body(float x,float z,float angle,int style,int walking){
    /* Reserve enough space before adding a pedestrian. The protagonist is
       submitted first; dense crowds must not truncate his outfit. Allow
       extra vertices for polygons split by the camera clip planes. */
    const int mats[]={JEANS,SKIN,HAIR,CAR_PAINT,21,22,23,24,25,26,27,28};
    for(unsigned i=0;i<sizeof(mats)/sizeof(mats[0]);i++)if(used[mats[i]]+648>MAX_VERTICES)return;
    int variant=style%8,woman=variant<4,top=21+variant%6,face=woman?27:28;
    const uint32_t skins[]={COLOR(246,216,187),COLOR(182,129,92),COLOR(225,178,139),COLOR(140,93,67)};
    uint32_t skin=skins[(style/2)%4];
    uint32_t tint=COLOR(210+(style*13)%45,205+(style*23)%50,205+(style*17)%50);
    uint32_t pants=variant%3==0?COLOR(150,168,170):variant%3==1?COLOR(90,101,117):COLOR(212,209,183);
    float shoulderWidth=woman?4.1f:variant==6?5.9f:5.1f;
    float hipWidth=woman?2.45f:2.2f,legWidth=variant==2||variant==5?2.15f:1.65f;
    float phase=npcPhase,swing=walking?sinf(phase)*(npcFlee>0?1.5f:1):0,bob=walking?fabsf(cosf(phase))*.3f:0;
    for(int s=-1;s<=1;s+=2){
        float stride=s*swing*3.4f,lift=walking?fmaxf(0,s*swing)*1.7f:0;
        Point hip=point(0,12+bob,s*hipWidth),knee=point(stride*.55f+.6f,7+lift,s*hipWidth),ankle=point(stride,2+lift,s*hipWidth);
        limb(hip,knee,legWidth+.3f,legWidth,JEANS,pants,x,z,angle);
        limb(knee,ankle,legWidth,legWidth*.8f,JEANS,pants,x,z,angle);
        Point shoe=local(stride+1,0,s*2.3f,x,z,angle);
        box(shoe.x,shoe.z,.45f+lift,5.3f,3.3f,1.4f,angle,CAR_PAINT,CAR_PAINT,COLOR(47,48,46));
        box(shoe.x,shoe.z,.15f+lift,5.5f,3.4f,.45f,angle,CAR_PAINT,CAR_PAINT,COLOR(196,193,175));
        Point shoulder=point(0,20+bob,s*shoulderWidth),elbow=point(-stride*.5f,15.8f+bob,s*(shoulderWidth+.3f)),wrist=point(1-stride,12.7f+bob,s*shoulderWidth);
        limb(shoulder,elbow,woman?1.45f:1.8f,1.25f,top,tint,x,z,angle);
        limb(elbow,wrist,1.25f,.9f,variant%2?top:SKIN,variant%2?tint:skin,x,z,angle);
        limb(wrist,point(wrist.x+.3f,wrist.y-1.8f,wrist.z),.95f,.7f,SKIN,skin,x,z,angle);
    }
    /* Fitted waist, chest, shoulders; different front/back UV material. */
    float heights[4]={11,14,19.5f,21},rx[4]={2.1f,2.6f,3,2.1f},rz[4]={3.8f,4,4.8f,3.4f};
    if(woman){rx[0]=2.3f;rx[1]=2.1f;rx[2]=2.65f;rz[0]=4.5f;rz[1]=3.25f;rz[2]=4.0f;rz[3]=3.0f;}
    if(variant==6){for(int i=0;i<4;i++){rx[i]*=1.28f;rz[i]*=1.15f;}}
    if(variant==1||variant==3){heights[0]=8.5f;rx[0]=3.3f;rz[0]=5.0f;} /* tunic / long jacket */
    for(int level=0;level<3;level++)for(int k=0;k<8;k++){
        Point p=local(ringC[k]*rx[level+1],heights[level+1]+bob,ringS[k]*rz[level+1],x,z,angle);
        Point q=local(ringC[k+1]*rx[level+1],heights[level+1]+bob,ringS[k+1]*rz[level+1],x,z,angle);
        Point r=local(ringC[k+1]*rx[level],heights[level]+bob,ringS[k+1]*rz[level],x,z,angle);
        Point s=local(ringC[k]*rx[level],heights[level]+bob,ringS[k]*rz[level],x,z,angle);
        int mat=top;
        Vertex v[4]={{.5f+ringS[k]*.5f,1-(heights[level+1]-11)/10,tint,p.x,p.y,p.z},{.5f+ringS[k+1]*.5f,1-(heights[level+1]-11)/10,tint,q.x,q.y,q.z},{.5f+ringS[k+1]*.5f,1-(heights[level]-11)/10,tint,r.x,r.y,r.z},{.5f+ringS[k]*.5f,1-(heights[level]-11)/10,tint,s.x,s.y,s.z}};
        polygon(mat,v,4);
    }
    limb(point(0,20.5f+bob,0),point(0,23+bob,0),1.25f,1.2f,SKIN,skin,x,z,angle);
    /* Rounded jaw, cheeks, cranium. Face appears only on the forward surface. */
    float hy[6]={22.1f,23,25.2f,27.5f,28.6f,29},hr[6]={.4f,.78f,1,1,.75f,.05f};
    for(int level=0;level<5;level++)for(int k=0;k<8;k++){
        Point p=local(.2f+ringC[k]*2.6f*hr[level+1],hy[level+1]+bob,ringS[k]*2.35f*hr[level+1],x,z,angle);
        Point q=local(.2f+ringC[k+1]*2.6f*hr[level+1],hy[level+1]+bob,ringS[k+1]*2.35f*hr[level+1],x,z,angle);
        Point r=local(.2f+ringC[k+1]*2.6f*hr[level],hy[level]+bob,ringS[k+1]*2.35f*hr[level],x,z,angle);
        Point s=local(.2f+ringC[k]*2.6f*hr[level],hy[level]+bob,ringS[k]*2.35f*hr[level],x,z,angle);
        int mat=(k==0||k==7)?face:level>=2?HAIR:SKIN;
        float vt=1-(hy[level+1]-22.1f)/6.9f,vb=1-(hy[level]-22.1f)/6.9f;
        uint32_t headTint=mat==HAIR?(variant%3==0?COLOR(116,77,47):COLOR(65,53,47)):skin;
        Vertex v[4]={{.5f+ringS[k]*.68f,vt,headTint,p.x,p.y,p.z},{.5f+ringS[k+1]*.68f,vt,headTint,q.x,q.y,q.z},{.5f+ringS[k+1]*.68f,vb,headTint,r.x,r.y,r.z},{.5f+ringS[k]*.68f,vb,headTint,s.x,s.y,s.z}};
        polygon(mat,v,4);
    }
    if(woman){
        if(variant%2==0){ /* ponytail with independent lower swing */
            limb(point(-2,26+bob,0),point(-3.6f,23+bob,0),1.4f,1.3f,HAIR,COLOR(110,79,53),x,z,angle);
            limb(point(-3.6f,23+bob,0),point(-3.8f+swing*.3f,19+bob,.2f),1.3f,.6f,HAIR,COLOR(110,79,53),x,z,angle);
        }else for(int side=-1;side<=1;side+=2)
            limb(point(-.7f,26+bob,side*2.2f),point(-.8f,21.5f+bob,side*2.5f),1.2f,1.1f,HAIR,COLOR(74,57,46),x,z,angle);
    }
    if(variant==4||variant==7){ /* cap, including forward brim */
        Point cap=local(0,0,0,x,z,angle);box(cap.x,cap.z,27.5f+bob,4.6f,4.8f,1.4f,angle,top,top,tint);
        Point brim=local(2.5f,0,0,x,z,angle);box(brim.x,brim.z,27.4f+bob,3,4.8f,.35f,angle,top,top,tint);
    }
    if(variant==2||variant==7){ /* backpack and straps */
        Point bag=local(-3.1f,0,0,x,z,angle);box(bag.x,bag.z,13+bob,3,5.4f,6,angle,25,25,COLOR(120,128,139));
        for(int side=-1;side<=1;side+=2)limb(point(-1,20+bob,side*2.5f),point(2.8f,15+bob,side*2.5f),.35f,.35f,25,0xffffffffu,x,z,angle);
    }
}
/* Twelve-sided connected rings: smooth cloth silhouettes and continuous UVs.
   Rings run bottom to top; x is forward, z is lateral. */
typedef struct {float x,y,z,rx,rz;} BodyRing;
static const float bodyC[13]={1,.8660254f,.5f,0,-.5f,-.8660254f,-1,-.8660254f,-.5f,0,.5f,.8660254f,1};
static const float bodyS[13]={0,.5f,.8660254f,1,.8660254f,.5f,0,-.5f,-.8660254f,-1,-.8660254f,-.5f,0};
static Vertex body_vertex(BodyRing r,int k,float v,uint32_t tint,float x,float z,float angle){
    Point p=local(r.x+bodyC[k]*r.rx,r.y,r.z+bodyS[k]*r.rz,x,z,angle);
    /* Smooth per-vertex diffuse shading, avoiding eight flat-looking panels. */
    float light=.83f+.12f*bodyC[k]-.05f*bodyS[k];
    Vertex out={.5f+.5f*bodyS[k],v,shade(tint,light),p.x,p.y,p.z};return out;
}
static void cloth(const BodyRing *r,int count,int front,int back,uint32_t tint,float x,float z,float angle){
    float height=r[count-1].y-r[0].y;
    for(int j=0;j<count-1;j++)for(int k=0;k<12;k++){
        float a=1-(r[j+1].y-r[0].y)/height,b=1-(r[j].y-r[0].y)/height;
        Vertex v[4]={body_vertex(r[j+1],k,a,tint,x,z,angle),body_vertex(r[j+1],k+1,a,tint,x,z,angle),
                     body_vertex(r[j],k+1,b,tint,x,z,angle),body_vertex(r[j],k,b,tint,x,z,angle)};
        polygon(bodyC[k]+bodyC[k+1]>=0?front:back,v,4);
    }
    /* Closed shoulders/hem, so looking down never reveals an empty torso. */
    for(int end=0;end<2;end++)for(int k=0;k<12;k++){
        BodyRing a=r[end?count-1:0];Point p=local(a.x,a.y,a.z,x,z,angle);
        Vertex v[3]={{.5f,.5f,tint,p.x,p.y,p.z},body_vertex(a,k,end?0:1,tint,x,z,angle),body_vertex(a,k+1,end?0:1,tint,x,z,angle)};
        polygon(back,v,3);
    }
}
#include "human_gait.inc"
#include "character_pose.inc"
static Point player_pose(Point p,int bone){
    float motion=poseMotion,wave=poseWave,run=poseRun;
    if(view->jump>0&&bone>0&&bone<3){float tuck=geo_clamp(view->jump/7,0,1)*geo_clamp(1-p.y/15,0,1);p.x-=tuck*1.5f;p.y+=tuck*2.5f;}
    int side=(bone&1)?-1:1;
    /* Animacion procedural (Claude v2.4): ademas del paso, contragiro de hombros y cadera, brazos con codo,
       inclinacion hacia delante al correr y arco del pie. Todo en funcion de la fase de marcha ya existente. */
    if(bone==1||bone==2){
        p=gait_leg(p,bone-1);
    }else if(bone>=3){
        int posed=view->climb>0||view->aiming||(view->weapon>0&&(bone==3||view->weapon>=3));
        if(posed){p=pose_arm(p,bone);}else{
        float weight=geo_clamp((24-p.y)/12,0,1);
        /* Brazo: balanceo opuesto a la pierna, mas amplio al correr; el codo se dobla y sube (antebrazo adelantado). */
        float swing=-footStep[(bone-3)&1]*(.43f+.19f*run);

        /* Rotate the forearm around the elbow instead of stretching it up. */
        if(p.y<16.5f){float dy=p.y-16.5f,xx=p.x;
            p.x=xx*elbowCos-dy*elbowSin;p.y=16.5f+xx*elbowSin+dy*elbowCos;
        }
        p.x+=swing*weight;
        p.y+=fmaxf(0,swing)*.35f*weight*weight;
        p.z-=side*weight*run*.8f; /* los brazos se cierran hacia el cuerpo al correr */
        }
    }
    float upper=geo_clamp((p.y-13)/12,0,1);
    /* Contragiro de hombros respecto a la cadera (torsion del torso). */
    if(bone==0||bone>=3){float tw=-wave*.055f*motion*upper;float nx=p.x-p.z*tw,nz=p.z+p.x*tw;p.x=nx;p.z=nz;}
    p.y+=poseBreath*upper;
    if(bone!=1&&bone!=2){p.y+=poseBob;p.z+=poseSway;}
    /* Inclinacion hacia delante proporcional a la velocidad. */
    p.x+=(run*1.65f+geo_clamp(motion,0,1)*.25f)*upper;return p;
}
/* Tabla de vertices distintos del jugador (posicion, uv, color, hueso). Se construye una vez (tabla hash). */
#define PLAYER_UNIQUE_MAX 2048
static unsigned short playerUnique[PLAYER_UNIQUE_MAX],playerIndex[PLAYER_VERTEX_COUNT];static int playerUniqueCount=-1;
static void player_index_build(void){
    if(playerUniqueCount>=0)return;
    static short table[8192];for(int i=0;i<8192;i++)table[i]=-1;
    playerUniqueCount=0;
    for(int i=0;i<PLAYER_VERTEX_COUNT;i++){
        const PlayerVertex *a=&player_mesh[i];
        unsigned h=(unsigned)(a->x*73.f)*2654435761u^(unsigned)(a->y*151.f)*40503u^(unsigned)(a->z*97.f)*2246822519u^a->bone*97u^a->color;
        h=(h^(h>>13))&8191;int found=-1;
        for(int k=0;k<8192;k++){int slot=(h+k)&8191;int u=table[slot];
            if(u<0){if(playerUniqueCount<PLAYER_UNIQUE_MAX){table[slot]=(short)playerUniqueCount;playerUnique[playerUniqueCount]=(unsigned short)i;found=playerUniqueCount++;}else found=0;break;}
            const PlayerVertex *b=&player_mesh[playerUnique[u]];
            if(a->x==b->x&&a->y==b->y&&a->z==b->z&&a->u==b->u&&a->v==b->v&&a->color==b->color&&a->bone==b->bone&&a->mat==b->mat){found=u;break;}
        }
        playerIndex[i]=(unsigned short)found;
    }
}
static void weapon_part(Point grip,float fx,float fy,float fz,float length,float height,float width,uint32_t color,float x,float z,float angle){
 Point p[8];
 for(int i=0;i<8;i++){
  float xx=fx+((i&1)?1:-1)*length*.5f;
  float yy=fy+((i&2)?1:-1)*height*.5f;
  float zz=fz+((i&4)?1:-1)*width*.5f;
  float pitch=view->aiming?view->aimPitch:0;
  if(view->weapon==7)pitch=view->punch>0?-sinf(PI*geo_clamp(1-view->punch/.32f,0,1))*1.3f:0;
  float c=cosf(pitch),sn=sinf(pitch);
  p[i]=local(grip.x+xx*c-yy*sn,grip.y+xx*sn+yy*c,grip.z+zz,x,z,angle);
 }
 const int faces[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
 int material=color==COLOR(145,89,48)?WEAPON_WOOD:color==COLOR(41,45,49)?WHEEL:WEAPON_METAL;
 for(int f=0;f<6;f++)quad(material,p[faces[f][0]],p[faces[f][1]],p[faces[f][2]],p[faces[f][3]],shade(color,f==3?1:.72f+.04f*f),1,1);
}
#include "weapon_detail.inc"
static void equipped_weapon(float x,float z,float angle){
 int id=view->weapon;if(id<1||id>7)return;
 Point grip=player_pose(point(.95f,12.8f,-3.95f),3);
 uint32_t steel=COLOR(105,115,123),dark=COLOR(41,45,49),wood=COLOR(145,89,48);
 /* Trigger guard, safety, rear sight and grip grooves are actual geometry. */
 weapon_part(grip,1,-.5f,0,1.3f,.18f,1.15f,steel,x,z,angle);
 weapon_part(grip,1.5f,.25f,0,.18f,1.4f,1.05f,dark,x,z,angle);
 if(id!=7){weapon_part(grip,-.4f,1.9f,0,.35f,.35f,.6f,dark,x,z,angle);
  for(int k=0;k<3;k++)weapon_part(grip,-.58f,-.25f-k*.45f,0,.13f,.16f,1.08f,steel,x,z,angle);}
 if(view->recoil>.09f&&view->aiming&&id<7){float muzzle=id<3?3.7f:id==3?7.2f:id==4?13.3f:id==5?15.3f:11.3f;
  weapon_part(grip,muzzle,1.25f,0,1.5f,.8f,.8f,COLOR(255,211,76),x,z,angle);}

 if(id==7){
  weapon_part(grip,0,2.2f,0,.65f,5,.65f,wood,x,z,angle);
  weapon_part(grip,0,9,0,1.25f,9,1.25f,wood,x,z,angle);return;
 }
 weapon_part(grip,0,-.2f,0,1.15f,2.2f,1.0f,dark,x,z,angle);
 if(id<3){
  weapon_part(grip,1.2f,1.1f,0,id==1?4:4.8f,.95f,.85f,steel,x,z,angle);
  if(id==2)weapon_tube(grip,-.05f,1.45f,.7f,.72f,x,z,angle);
  weapon_tube(grip,2.3f,id==1?3.25f:3.65f,1.1f,.32f,x,z,angle);
 }else{
  float len=id==3?6:id==5?13:id==6?15:11;
  weapon_part(grip,2,1,0,5,1.6f,1.3f,steel,x,z,angle);
  weapon_tube(grip,len*.20f,len*.90f,1.25f,.27f,x,z,angle);
  weapon_part(grip,-2.7f,.5f,0,3.2f,1.9f,1.3f,id==4||id==5?wood:dark,x,z,angle);
  weapon_part(grip,4,.55f,0,3.5f,1.05f,1.4f,id==4||id==5?wood:dark,x,z,angle);
  if(id==3||id==4)weapon_part(grip,1.8f,-1,0,1.1f,3.8f,.85f,dark,x,z,angle);
  if(id==6)weapon_part(grip,2,2.5f,0,4.5f,.8f,.8f,dark,x,z,angle);
 }
}
static void person(float x,float z,float angle,int style,int walking){
    if(style>=0){simple_person(x,z,angle,style,walking);return;}
    pose_prepare();
    /* Optimizacion (Claude): ruta rapida para los ~2.300 triangulos del jugador. Son pequenos y estan siempre
       delante de la camara, asi que no pasan por polygon() (sin recorte de 6 planos ni copias): se posan, rotan,
       proyectan con la transformacion rigida cacheada y se escriben directamente en el lote de su material. */
    if(geographic&&rigid)rigid_cache();
    if(angle!=localA){localA=angle;localC=cosf(angle);localS=sinf(angle);}
    /* v2.6 (Claude): la malla viene sin indices (7.032 vertices) pero solo tiene ~1.400 distintos. Se posan y
       transforman una vez los distintos y los triangulos se copian por indice: 5x menos trabajo (13 ms -> ~3 ms). */
    player_index_build();
    static Vertex posed[PLAYER_UNIQUE_MAX];
    for(int u=0;u<playerUniqueCount;u++){
        const PlayerVertex *a=&player_mesh[playerUnique[u]];Point q=player_pose(player_surface(a),a->bone);
        q.x*=PERSON_SCALE;q.y*=PERSON_SCALE;q.z*=PERSON_SCALE;
        float wx=x+q.x*localC-q.z*localS,wz=z+q.x*localS+q.z*localC,wy=q.y;
        if(geographic){
            if(rigid){float dx=wx-objectX,dz=wz-objectZ;wx=rigGX+rigCos*dx-rigSin*dz;wz=rigGZ+rigSin*dx+rigCos*dz;wy+=rigH;}
            else{float gx,gz;geo_project(wx,wz,&gx,&gz);wy+=geo_height(wx,wz);wx=gx;wz=gz;}
        }
        posed[u]=(Vertex){a->u,a->v,day_scale(a->color),wx,wy,wz};
    }
    for(int i=0;i<PLAYER_VERTEX_COUNT;i+=3){
        int mat=player_mesh[i].mat;
        if(used[mat]+3>MAX_VERTICES){overflow++;continue;}
        Vertex *out=mesh[mat]+used[mat];
        out[0]=posed[playerIndex[i]];out[1]=posed[playerIndex[i+1]];out[2]=posed[playerIndex[i+2]];
        used[mat]+=3;
    }
    localScale=PERSON_SCALE;
    for(int side=-1;side<=1;side+=2){
        Point foot=player_pose(point(0,0,side*1.65f),side<0?1:2);float step=foot.x,lift=foot.y;
        float lateral=side*1.65f;
        BodyRing sole[]={{step+.4f,.12f+lift,lateral,2.65f,1.27f},{step+.4f,.55f+lift,lateral,2.7f,1.3f}};
        cloth(sole,2,20,20,COLOR(217,213,198),x,z,angle);
        BodyRing shoe[]={{step+.4f,.55f+lift,lateral,2.60f,1.25f},{step+.3f,1.05f+lift,lateral,2.45f,1.20f},{step-.3f,1.85f+lift,lateral,1.35f,1.02f}};
        cloth(shoe,3,20,20,COLOR(237,232,217),x,z,angle);
        /* Dark outsole and instep laces make the streetwear sneakers readable. */
        BodyRing tread[]={{step+.4f,.08f+lift,lateral,2.60f,1.24f},{step+.4f,.20f+lift,lateral,2.66f,1.28f}};
        cloth(tread,2,20,20,COLOR(58,61,61),x,z,angle);
        for(int k=0;k<3;k++){
            float xx=step-.4f+k*.45f,yy=1.86f-k*.13f+lift;
            quad(20,local(xx,yy,lateral-.66f,x,z,angle),local(xx+.12f,yy,lateral-.66f,x,z,angle),
                    local(xx+.12f,yy,lateral+.66f,x,z,angle),local(xx,yy,lateral+.66f,x,z,angle),COLOR(80,84,83),1,1);
        }
    }
    equipped_weapon(x,z,angle);
    localScale=1;
}
static void landmarks(void){
    for(int i=0;i<30;i++){
        static float termX[30],termZ[30],termSrcX[30],termSrcZ[30]; /* v2.36: terminal en la acera (cm_terminal_pos) */
        if(termSrcX[i]!=view->hubs[i][0]||termSrcZ[i]!=view->hubs[i][1]){termSrcX[i]=view->hubs[i][0];termSrcZ[i]=view->hubs[i][1];cm_terminal_pos(termSrcX[i],termSrcZ[i],&termX[i],&termZ[i]);}
        float x=termX[i],z=termZ[i];if(!nearby(x,z,400))continue;
        /* An actual textured computer terminal at each interaction point. */
        /* v2.7: terminal de barrio: pedestal hexagonal, pantalla inclinada y visera */
        /* v2.36: un poco mas estrecho (pedestal 3,6) para caber en la acera de 8 sin meterse en el edificio */
        cylinder(x,z,0,3.6f,9,6,0,CONCRETE,METAL,0xffffffffu);
        box(x,z,9,2.4f,6.4f,7,0,GLASS,METAL,COLOR(111,249,210));
        frustum(x,z,16,4.2f,1.6f,3,6,0,METAL,COLOR(80,84,90));
        /* v2.36 (Claude): el vinilo se dibuja donde el juego lo recoge (punto+15,+38); antes salia 18 mas a la derecha */
        if(i<24&&!(view->collected&(1u<<i)))box(view->hubs[i][0]+15,view->hubs[i][1]+38,5+sinf(view->time*2),1,7,7,view->time,WHEEL,WHEEL,0xffffffffu);
    }
    if(view->target>=0){
        float x=view->targetX,z=view->targetZ;
        float pulse=1+sinf(view->time*3)*.12f;
        for(int k=0;k<20;k++){
            float a=k*2*PI/20,b=(k+1)*2*PI/20;
            quad(CAR_PAINT,point(x+cosf(a)*17*pulse,.9f,z+sinf(a)*17*pulse),point(x+cosf(b)*17*pulse,.9f,z+sinf(b)*17*pulse),point(x+cosf(b)*20*pulse,.9f,z+sinf(b)*20*pulse),point(x+cosf(a)*20*pulse,.9f,z+sinf(a)*20*pulse),COLOR(235,255,97),1,1);
        }
        box(x,z,35+sinf(view->time*3)*3,5,5,5,view->time,CAR_PAINT,CAR_PAINT,COLOR(239,255,94));
    }
}
/* v2.9 (Claude): fija el buffer de dibujo del GE. Los dialogos de sceUtility dibujan en el buffer de dibujo ACTUAL del GE,
   no en el que se muestra: si el ultimo game_draw fue al buffer oculto, el dialogo pintaba ahi (invisible) y la copia
   al visible arrastraba los restos de todos los fotogramas (el "slot pegado"). */
/* v2.9.1: estado interno de sceGu para los dialogos del sistema (ver psp_main.c savedata_dialog). */
void r3_gu_buffers(uint32_t *draw,uint32_t *disp){
#ifndef R3_HOST
    ge_wait(),sceGuStart(GU_DIRECT,commands);
    sceGuDrawBuffer(GU_PSM_8888,(void*)((uintptr_t)draw&0x001fffff),512);sceGuDispBuffer(480,272,(void*)((uintptr_t)disp&0x001fffff),512);
    sceGuOffset(2048-240,2048-136);sceGuViewport(2048,2048,480,272);sceGuScissor(0,0,480,272);sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();sceGuSync(0,0);sceDisplayWaitVblankStart();
    /* v2.10.1 (Claude): CLAVE. El juego dibuja por CPU y presenta con sceDisplaySetFrameBuf, asi que r3_init nunca
       habilito la salida del GU. Sin sceGuDisplay(GU_TRUE), sceGuSwapBuffers() no cambia lo que se ve: el dialogo de
       Sony pintaba en un buffer que nunca llegaba a pantalla y quedaba el fotograma anterior "pegado". Es justo lo
       que faltaba respecto al teclado de PSP-IA, que si lo habilita. */
    sceGuDisplay(GU_TRUE);
#else
    (void)draw;(void)disp;
#endif
}
void r3_gu_display(int on){
#ifndef R3_HOST
    sceGuDisplay(on?GU_TRUE:GU_FALSE);
#else
    (void)on;
#endif
}
void r3_gu_idle(void){
#ifndef R3_HOST
    ge_wait(),sceGuStart(GU_DIRECT,commands);sceGuFinish();sceGuSync(0,0);
#endif
}
void r3_gu_swap(void){
#ifndef R3_HOST
    sceGuSwapBuffers();
#endif
}
void r3_set_draw_buffer(uint32_t *fb){
#ifndef R3_HOST
    ge_wait(),sceGuStart(GU_DIRECT,commands);sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);sceGuFinish();sceGuSync(0,0);
#else
    (void)fb;
#endif
}
/* v2.13.4 (Claude): trazas por fase; psp_main instala un callback que escribe en ms0:/NARCADE_DEBUG.TXT durante los
   primeros fotogramas tras cargar partida, para saber que subsistema se traga la consola. */
static void (*traceFn)(const char*)=0;static int traceFrames=0;
static void (*phaseFn)(const char*)=0;
void r3_phase_hook(void (*fn)(const char*)){phaseFn=fn;phaseFn2=fn;}
#define PHASE(s) do{if(phaseFn)phaseFn(s);}while(0)
#define TRACE(s) do{PHASE(s);if(traceFn&&traceFrames>0)traceFn(s);}while(0)
void r3_trace(void (*fn)(const char*),int frames){traceFn=fn;traceFrames=frames;traceFn2=fn;traceFrames2=&traceFrames;}
int r3_overflow(void){return overflow;}
/* v2.37 (Claude): pantalla de carga real. Construye en la cache de ciudad las manzanas que vera la
   camara (sin dibujar nada) hasta 6 piezas por llamada; devuelve cuantas construyo (0 = listo).
   Antes la cache se llenaba a 1 pieza por fotograma ya en juego: segundos de fotogramas lentos. */
int r3_prewarm(const R3Scene *s){
    unsigned long before=r3CacheNew+r3CacheLight;
#ifndef R3_HOST
    ge_wait();
#endif
    view=s;overflow=0;memset(used,0,sizeof(used));r3Frame+=2;day_update(s->time);glowUsed=0;shadowUsed=0;geographic=1;rigid=0;
    camera(s);r3WarmBudget=6;prefetchOnly=1;city();prefetchOnly=0;r3WarmBudget=0;
    memset(used,0,sizeof(used));overflow=0;geographic=0;
    return (int)(r3CacheNew+r3CacheLight-before);
}
int r3_used(int m){return m<MAT_COUNT?used[m]:0;}
void r3_init(void){
#ifndef R3_HOST
    /* 2 x 8888 frame + depth + 21 x 128-square 565 textures = 2,080,768 bytes. */
    textureBase=(void*)((uintptr_t)sceGeEdramGetAddr()+512*272*10);
    memcpy((void*)((uintptr_t)textureBase|0x40000000),textures3d_data,VRAM_MATERIALS*128*128*2);
    sceKernelDcacheWritebackAll();
    sceGuInit();sceGuStart(GU_DIRECT,commands);
    sceGuDrawBuffer(GU_PSM_8888,(void*)0,512);
    sceGuDispBuffer(480,272,(void*)(512*272*4),512);
    sceGuDepthBuffer((void*)(512*272*8),512);
    sceGuOffset(2048-240,2048-136);sceGuViewport(2048,2048,480,272);
    sceGuDepthRange(65535,0);sceGuDepthFunc(GU_GEQUAL);
    sceGuScissor(0,0,480,272);sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_CLIP_PLANES);sceGuDisable(GU_CULL_FACE);
    sceGuFinish();sceGuSync(0,0);
#endif
}
static void horizon_build(void){
 for(int k=0;k<96;k++){
  float a=k*2*PI/96,b=(k+1)*2*PI/96;
  float h0=18+42*powf(fabsf(cosf(a)),2)+12*sinf(a*7)+5*cosf(a*13);
  float h1=18+42*powf(fabsf(cosf(b)),2)+12*sinf(b*7)+5*cosf(b*13);
  float x0=eye.x+cosf(a)*300,z0=eye.z+sinf(a)*300,x1=eye.x+cosf(b)*300,z1=eye.z+sinf(b)*300;
  uint32_t mountain=day_scale(COLOR(47,85,67));
  Vertex *v=horizonMesh+k*12;
  v[0]=(Vertex){0,0,mountain,x0,eye.y-220,z0};v[1]=(Vertex){0,0,mountain,x1,eye.y-220,z1};v[2]=(Vertex){0,0,mountain,x1,eye.y+h1,z1};
  v[3]=v[0];v[4]=v[2];v[5]=(Vertex){0,0,mountain,x0,eye.y+h0,z0};
  float top=eye.y-9+(k*37%23),bottom=eye.y-220;
  uint32_t building=day_scale(COLOR(74+(k%4)*10,78+(k%3)*8,83));
  v[6]=(Vertex){0,0,building,x0,bottom,z0};v[7]=(Vertex){0,0,building,x1,bottom,z1};v[8]=(Vertex){0,0,building,x1,top,z1};
  v[9]=v[6];v[10]=v[8];v[11]=(Vertex){0,0,building,x0,top,z0};
 }
}
/* v2.30 (Claude): marcas de tiempo por etapa para medir en la consola real
   (inicio, ciudad construida, resto de geometria, GE terminado). */
unsigned r3ProfT[4];
#ifndef R3_HOST
#define R3PROF(i) (r3ProfT[i]=sceKernelGetSystemTimeLow())
#else
#define R3PROF(i) ((void)0)
#endif
void r3_draw(uint32_t *fb,const R3Scene *s){
    R3PROF(0);
    TRACE("r3:inicio");
    view=s;overflow=0;memset(used,0,sizeof(used));r3Frame++;day_update(s->time);glowUsed=0;shadowUsed=0;geographic=1;rigid=0;
    TRACE("r3:camara");camera(s);
#ifndef AB_NOCITY
    TRACE("r3:ciudad");city();TRACE("r3:ciudad-ok");R3PROF(1);
#endif
    rigid=1;
    for(int i=0;i<s->wallCount;i++){objectX=s->walls[i][0];objectZ=s->walls[i][1];objectYaw=s->walls[i][2];rigX=-1e9f;
        box(objectX,objectZ,0,24,2.4f,18,objectYaw,CONCRETE,CONCRETE,COLOR(225,215,195));
        box(objectX,objectZ,18,25,3.1f,.7f,objectYaw,METAL,METAL,COLOR(160,173,168));
    }
    TRACE("r3:coches");rigid=2;
#ifndef AB_NOCARS
    int order[64],count=s->carCount<64?s->carCount:64;float distance[64];
    for(int i=0;i<count;i++){int j=i;float d=view_distance(s->cars[i].x,s->cars[i].z);
        while(j>0&&distance[j-1]>d){distance[j]=distance[j-1];order[j]=order[j-1];j--;}
        distance[j]=d;order[j]=i;}
    for(int j=0;j<count;j++){int i=order[j];objectX=s->cars[i].x;objectZ=s->cars[i].z;objectYaw=s->cars[i].angle;car(&s->cars[i]);}
#endif
    TRACE("r3:jugador");rigid=1;
    objectX=s->x;objectZ=s->z;objectYaw=s->angle;playerLift=s->lift;rigX=-1e9f; /* invalidar cache */
#ifndef AB_NOPLAYER
    if(!s->driving&&!s->inMetro)person(s->x,s->z,s->angle,-1,s->moving);
#endif
    playerLift=0;rigX=-1e9f;
#ifndef AB_NOPEOPLE
    for(int i=0;i<s->personCount;i++){
        const R3Person *p=&s->people[i];objectX=p->x;objectZ=p->z;objectYaw=p->angle;npcFall=p->fall;npcPhase=p->phase;npcFlee=p->flee;
        person(objectX,objectZ,objectYaw,p->style,p->fall==0);
        npcFall=0;
        if(p->hit>0&&nearby(p->x,p->z,220)){
            for(int k=0;k<5;k++){float t=.32f-p->hit;Point a=point(p->x+(k-2)*t*12,9-t*t*45+(k%2)*.7f,p->z+t*15);
                quad(FLAT,a,point(a.x+.35f,a.y,a.z),point(a.x+.35f,a.y+.4f,a.z),point(a.x,a.y+.4f,a.z),COLOR(170,30,35),1,1);}
        }
    }
    npcFall=0;
#endif
    rigid=0;
    if(s->shotTime>0){
        float gx,gz,px,pz;geo_project(s->shotX,s->shotZ,&gx,&gz);geo_project(s->x,s->z,&px,&pz);
        float yaw=geo_heading(s->x,s->z,s->angle);Point start=point(px+cosf(yaw)*7,geo_height(s->x,s->z)+s->lift+10,pz+sinf(yaw)*7);
        Point end=point(gx,s->shotHeight,gz);int saved=geographic;geographic=0;
        quad(FLAT,start,point(start.x,start.y+.09f,start.z),point(end.x,end.y+.09f,end.z),end,COLOR(247,216,143),1,1);
        quad(FLAT,point(end.x-.3f,end.y-.3f,end.z),point(end.x+.3f,end.y-.3f,end.z),point(end.x+.3f,end.y+.3f,end.z),point(end.x-.3f,end.y+.3f,end.z),COLOR(245,220,166),1,1);
        geographic=saved;
    }
    TRACE("r3:hubs");landmarks();
    /* v2.5: sombras proyectadas, faros, farolas y nubes (en coordenadas logicas; geo_point proyecta). */
    for(int i=0;i<s->carCount;i++)if(nearby(s->cars[i].x,s->cars[i].z,320)){cast_shadow(s->cars[i].x,s->cars[i].z,11,9);headlights(s->cars[i].x,s->cars[i].z,s->cars[i].angle);}
    if(!s->driving&&!s->inMetro&&s->lift<1)cast_shadow(s->x,s->z,3,17);
    for(int i=0;i<s->personCount;i++)if(nearby(s->people[i].x,s->people[i].z,220))cast_shadow(s->people[i].x,s->people[i].z,2.5f,17);
#ifndef AB_NOFX
    TRACE("r3:efectos");building_shadows();street_lamps_glow();clouds(s->time);
#endif
    geographic=0;
#ifndef R3_HOST
    R3PROF(2);TRACE("r3:ge-inicio");sceKernelDcacheWritebackAll();ge_wait(),sceGuStart(GU_DIRECT,commands);
    sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);
    sceGuClearColor(skyColor);sceGuClearDepth(0);sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);sceGuDisable(GU_BLEND);sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_5650,0,0,1);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);sceGuTexScale(1,1);sceGuTexOffset(0,0);sceGuShadeModel(GU_SMOOTH);
    sceGumMatrixMode(GU_PROJECTION);sceGumLoadIdentity();sceGumPerspective(62,480.0f/272,5,720); /* v2.6: plano cercano 5 (antes 2): 2.5x mas precision de profundidad, menos parpadeo (z-fighting) */
    ScePspFVector3 e={eye.x,eye.y,eye.z},t={target.x,target.y,target.z};
    ScePspFVector3 up={0,1,0};sceGumMatrixMode(GU_VIEW);sceGumLoadIdentity();sceGumLookAt(&e,&t,&up);
    sceGumMatrixMode(GU_MODEL);sceGumLoadIdentity();
    /* Clear sky and an opaque mountain/city backdrop. No distance fog. */
    {
        static Vertex __attribute__((aligned(16))) skyMesh[16*6];
        for(int k=0;k<16;k++){float a0=k*2*PI/16,a1=(k+1)*2*PI/16;float R=270;
            float x0=eye.x+cosf(a0)*R,z0=eye.z+sinf(a0)*R,x1=eye.x+cosf(a1)*R,z1=eye.z+sinf(a1)*R;float yb=eye.y-90,yt=eye.y+140;
            Vertex *q=skyMesh+k*6;
            q[0]=(Vertex){0,0,horizonColor,x0,yb,z0};q[1]=(Vertex){0,0,horizonColor,x1,yb,z1};q[2]=(Vertex){0,0,skyColor,x1,yt,z1};
            q[3]=(Vertex){0,0,skyColor,x1,yt,z1};q[4]=(Vertex){0,0,skyColor,x0,yt,z0};q[5]=(Vertex){0,0,horizonColor,x0,yb,z0};
        }
        sceKernelDcacheWritebackRange(skyMesh,sizeof(skyMesh));
        sceGuDisable(GU_TEXTURE_2D);sceGuDepthMask(GU_TRUE);sceGuDisable(GU_DEPTH_TEST);
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,16*6,0,skyMesh);
        horizon_build();sceKernelDcacheWritebackRange(horizonMesh,sizeof(horizonMesh));
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,96*12,0,horizonMesh);
        sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);sceGuEnable(GU_TEXTURE_2D);
    }
#ifndef NARCADE_TOPVIEW
    /* v2.39 (Claude): niebla de distancia hacia el color del horizonte. Sin ella lo lejano entraba y
       salia de golpe en el plano lejano (720) y en los cambios de detalle: ahora aparece poco a poco.
       La hace el GE por vertice: sin coste de CPU. */
    if(distanceFog){sceGuFog(r3FogNear,700.f,horizonColor&0x00ffffffu);sceGuEnable(GU_FOG);}
    else sceGuDisable(GU_FOG);
#endif
#ifndef AB_NODRAW
    for(int m=0;m<MAT_COUNT;m++)if(used[m]){
        int streetMip=m==ROAD||m==SIDEWALK;
        int foliage=m==LEAVES;
        sceGuTexMode(foliage?GU_PSM_4444:GU_PSM_5650,streetMip,0,1);
        sceGuTexFunc(GU_TFX_MODULATE,foliage?GU_TCC_RGBA:GU_TCC_RGB);
        if(foliage){sceGuEnable(GU_ALPHA_TEST);sceGuAlphaFunc(GU_GEQUAL,128,255);}
        else sceGuDisable(GU_ALPHA_TEST);
        sceGuTexFilter(streetMip?GU_LINEAR_MIPMAP_LINEAR:GU_LINEAR,GU_LINEAR);
        if(m<VRAM_MATERIALS)sceGuTexImage(0,128,128,128,(const char*)textureBase+m*128*128*2);
        else if(m>=WEAPON_METAL)sceGuTexImage(0,64,64,64,textures3d_data+753664+14*8192+(m-WEAPON_METAL)*8192);
        else sceGuTexImage(0,64,64,64,textures3d_data+VRAM_MATERIALS*128*128*2+(m-VRAM_MATERIALS)*64*64*2);
        if(streetMip)sceGuTexImage(1,64,64,64,textures3d_data+753664+12*8192+m*64*64*2);
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,used[m],0,mesh[m]);
    }
#endif
    sceGuDisable(GU_ALPHA_TEST);sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);
    /* v2.5: pase de sombras (oscurece) y pase aditivo (luces, nubes, sol). Sin textura ni niebla. */
    sceGuDisable(GU_FOG);sceGuDisable(GU_TEXTURE_2D);sceGuEnable(GU_BLEND);sceGuDepthMask(GU_TRUE);
    if(shadowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,shadowUsed,0,shadowMesh);}
    if(glowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffff);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,glowUsed,0,glowMesh);}
    sceGuDisable(GU_BLEND);sceGuDepthMask(GU_FALSE);sceGuEnable(GU_TEXTURE_2D);
    sceGuFinish();geBusy=1;R3PROF(3); /* sin sceGuSync: ver ge_wait */
    TRACE("r3:ge-fin");if(traceFrames>0)traceFrames--;
#else
    (void)fb;
#endif
}
void r3_shutdown(void){
#ifndef R3_HOST
    sceGuTerm();
#endif
}
static int displayBrightness;
void r3_set_brightness(int level){displayBrightness=level<-5?-5:level>5?5:level;}
void r3_overlay_bands(uint32_t *fb,const uint32_t *rgba,const unsigned char *bands);
void r3_overlay(uint32_t *fb,const uint32_t *rgba){static const unsigned char all[17]={1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};r3_overlay_bands(fb,rgba,all);}
/* v2.41 (Claude): solo se componen las franjas de 16 filas que tienen HUD (antes toda la pantalla, 480x272
   RGBA leida de la RAM y mezclada cada fotograma). */
void r3_overlay_bands(uint32_t *fb,const uint32_t *rgba,const unsigned char *bands){
#ifndef R3_HOST
    typedef struct {float u,v,x,y,z;} SpriteVertex;
    sceKernelDcacheWritebackAll();ge_wait(),sceGuStart(GU_DIRECT,commands);
    sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);
    sceGuDisable(GU_DEPTH_TEST);sceGuDisable(GU_FOG);sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_TEXTURE_2D);sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    sceGuTexMode(GU_PSM_8888,0,0,0);sceGuTexImage(0,512,512,512,rgba);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexScale(1,1);sceGuTexOffset(0,0);sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    /* Narrow sprites avoid the GE's large textured-sprite cache penalty. */
    for(int b=0;b<17;){if(!bands[b]){b++;continue;}int e=b;while(e<17&&bands[e])e++;
     float y0=b*16.f,y1=e*16.f>272?272.f:e*16.f;b=e;
     for(int x=0;x<480;x+=32){
        int end=x+32>480?480:x+32;
        SpriteVertex *v=sceGuGetMemory(2*sizeof(*v));
        v[0]=(SpriteVertex){x,y0,x,y0,0};v[1]=(SpriteVertex){end,y1,end,y1,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
     }}
    if(displayBrightness){
        typedef struct {uint32_t color;float x,y,z;} ToneVertex;
        ToneVertex *tone=sceGuGetMemory(2*sizeof(*tone));
        uint32_t tint=((unsigned)(displayBrightness<0?-displayBrightness:displayBrightness)*12u<<24)|(displayBrightness>0?0xffffffu:0);
        tone[0]=(ToneVertex){tint,0,0,0};tone[1]=(ToneVertex){tint,480,272,0};
        sceGuDisable(GU_TEXTURE_2D);sceGuDrawArray(GU_SPRITES,GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,tone);
    }
    sceGuFinish();sceGuSync(0,0);
#else
    (void)fb;(void)rgba;(void)bands;
#endif
}
