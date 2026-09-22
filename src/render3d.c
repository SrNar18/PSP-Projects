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
#define MAT_COUNT 41 /* 21 VRAM + 8 NPC + 12 urban/detail materials in RAM */
#define VRAM_MATERIALS 21
#define MAX_VERTICES 6144 /* v2.13.2 (Claude): 8190 -> 6144 = 2 MB menos de RAM estatica (mesh). El pico medido
                                por material es ~5.000; si algun material se pasa solo se descartan poligonos. */
#include "player_mesh.h"
#define COLOR(r,g,b) (0xff000000u | (r) | ((g)<<8) | ((b)<<16))
enum { ROAD,SIDEWALK,BRICK,STUCCO,SHOP,ROOF,GRASS,WATER,JACKET,JEANS,FACE,WHEEL,CAR_SIDE,CAR_PAINT,GLASS,MURAL,JACKET_BACK,SLEEVE,SKIN,HAIR };
enum { BARK=29,LEAVES,METAL,CONCRETE,CURTAIN,AWNING,COBBLE,MODERN }; /* v2.7: materiales 64px en RAM (tools/extra_textures.py) */
enum { RETAIL=37,EATERY,OFFICE_FRONT,WORKSHOP_FRONT };
typedef struct { float u,v; uint32_t color; float x,y,z; } Vertex;
typedef struct { float x,y,z; } Point;
static Vertex __attribute__((aligned(16))) mesh[MAT_COUNT][MAX_VERTICES];
static int used[MAT_COUNT];
#ifndef R3_HOST
static unsigned int __attribute__((aligned(16))) commands[65536];
extern const unsigned char textures3d_data[];
static void *textureBase;
#endif
static const R3Scene *view;
static float planes[6][4];
static Point eye,target;
static int overflow;
/* Opaque horizon silhouettes, rendered behind the playable city. They hide
   the empty far plane without washing out world textures with distance fog. */
static Vertex __attribute__((aligned(16))) horizonMesh[96*12];
static void horizon_build(void);
static int clipEnabled=1;
static int geographic=0,rigid=0;
static float objectX,objectZ,objectYaw;
static float fixedGround=-1000000;

static Point point(float x,float y,float z){Point p={x,y,z};return p;}
static uint32_t shade(uint32_t c,float f){return COLOR((int)((c&255)*f),(int)(((c>>8)&255)*f),(int)(((c>>16)&255)*f));}
static void camera(const R3Scene *s){
    float px,pz;geo_project(s->x,s->z,&px,&pz);float yaw=s->yaw;
    float h=geo_height(s->x,s->z)+s->lift; /* v2.6: anden del Metro */
    eye=point(px-cosf(yaw)*s->cameraDistance,h+(s->eyeHeight>0?s->eyeHeight:(s->driving?54:43)),pz-sinf(yaw)*s->cameraDistance);
    float lx,lz;geo_unproject(eye.x,eye.z,&lx,&lz);eye.y=fmaxf(eye.y,geo_height(lx,lz)+12);
    target=point(px+cosf(yaw)*25,h+(s->driving?10:11),pz+sinf(yaw)*25);
    if(s->inMetro){ /* v2.6: camara dentro del coche del Metro, mirando en el sentido de la marcha */
        float dir=s->metroDir>0?1:-1;float mx,mz;geo_project(CM_METRO_X,s->metroZ+dir*(CM_TRAIN_CAR*.5f+3-26),&mx,&mz);float base=geo_height(CM_METRO_X,s->metroZ)+CM_PLAT_H;
        eye=point(mx-3,base+13,mz);float tx,tz;geo_project(CM_METRO_X,s->metroZ+dir*160,&tx,&tz);target=point(tx,base+9,tz); /* coche delantero, mirando por el testero */
    }else if(s->lift>15){ /* en el anden: camara baja y cercana para no ver la marquesina desde arriba */
        float d=s->cameraDistance<48?s->cameraDistance:48;
        eye=point(px-cosf(yaw)*d,h+16,pz-sinf(yaw)*d);target=point(px+cosf(yaw)*25,h+9,pz+sinf(yaw)*25);
    }
#ifdef NARCADE_TOPVIEW
    /* solo pruebas: vista aerea oblicua para revisar el trazado (NARCADE_EXTRA_CFLAGS=-DNARCADE_TOPVIEW) */
    eye=point(px-cosf(yaw)*260,h+NARCADE_TOPVIEW,pz-sinf(yaw)*260);target=point(px+cosf(yaw)*60,h,pz+sinf(yaw)*60);
#endif
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
static void polygon(int mat,Vertex *input,int count){
    Vertex buffers[2][16];memcpy(buffers[0],input,count*sizeof(Vertex));int src=0;
    if(geographic)for(int i=0;i<count;i++){
        Vertex *v=&buffers[0][i];float gx,gz;
        if(rigid){
            rigid_cache();
            float dx=v->x-objectX,dz=v->z-objectZ;
            if(rigid==2){float h=v->y;
                v->x=rigGX+rigBX.x*dx+rigBY.x*h+rigBZ.x*dz;
                v->z=rigGZ+rigBX.z*dx+rigBY.z*h+rigBZ.z*dz;
                v->y=rigH+rigBX.y*dx+rigBY.y*h+rigBZ.y*dz;
            }else{v->x=rigGX+rigCos*dx-rigSin*dz;v->z=rigGZ+rigSin*dx+rigCos*dz;v->y+=rigH;}
        }else{geo_project(v->x,v->z,&gx,&gz);v->y+=fixedGround>-999999?fixedGround:geo_height(v->x,v->z);v->x=gx;v->z=gz;}
    }
    /* PSP rejects large triangles crossing its near/guard planes. Clip in
       world space before submission, including UV interpolation at cuts.
       Optimizacion (Claude): primero una prueba trivial por plano; si todos los vertices quedan fuera de un plano se
       descarta el poligono entero, y solo se recorta contra los planos que realmente cruza. */
    if(clipEnabled&&count>=3){
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
    if(!litAlready)for(int j=0;j<count;j++)buffers[src][j].color=day_scale(buffers[src][j].color);
    int needed=(count-2)*3;
    if(used[mat]+needed>MAX_VERTICES){overflow++;return;}
    Vertex *p=mesh[mat]+used[mat];used[mat]+=needed;
    for(int j=1;j<count-1;j++){*p++=buffers[src][0];*p++=buffers[src][j];*p++=buffers[src][j+1];}
}
/* Optimizacion (Claude): descarte por esfera envolvente contra el frustum (en coordenadas proyectadas). */
static int sphere_visible(float x,float z,float radius){
    if(!clipEnabled)return 1; /* offline mesh export */
    float gx,gz;geo_project(x,z,&gx,&gz);float gy=geo_height(x,z)+radius*.4f;
    for(int k=0;k<6;k++)if(planes[k][0]*gx+planes[k][1]*gy+planes[k][2]*gz+planes[k][3]<-radius)return 0;
    return 1;
}
static float view_distance(float x,float z){float dx=x-view->x,dz=z-view->z;return sqrtf(dx*dx+dz*dz);}
/* Floors and roofs have an absolute elevation. Testing them with a sphere
   centred near the ground incorrectly removed middle/upper building sections. */
static int volume_visible(float x,float z,float bottom,float top,float planRadius){
    if(!clipEnabled)return 1;
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
static void ground(int mat,float x,float z,float w,float d,float y,uint32_t color,float repeat){
    /* Subdivide at terrain lattice boundaries: roads genuinely climb hills. */
    for(float zz=z;zz<z+d-.001f;){float endz=fminf(z+d,geo_next_z(zz));if(endz<=zz+.001f)endz=fminf(z+d,zz+.01f);
        for(float xx=x;xx<x+w-.001f;){float endx=fminf(x+w,(floorf(xx/80)+1)*80);if(endx<=xx+.001f)endx=fminf(x+w,xx+80);
            uint32_t lc=lit_color(color,0,1,0);float ou=0,ov=0;if(mat==WATER){ou=view->time*.045f;ov=view->time*.11f;} /* v2.7: el agua fluye */
            Vertex v[4]={{(xx-x)/w*repeat+ou,(zz-z)/d*repeat+ov,lc,xx,y,zz},{(endx-x)/w*repeat+ou,(zz-z)/d*repeat+ov,lc,endx,y,zz},
                         {(endx-x)/w*repeat+ou,(endz-z)/d*repeat+ov,lc,endx,y,endz},{(xx-x)/w*repeat+ou,(endz-z)/d*repeat+ov,lc,xx,y,endz}};
            litAlready=1;polygon(mat,v,4);litAlready=0;xx=endx;
        }zz=endz;
    }
}
static int nearby(float x,float z,float range){float dx=x-view->x,dz=z-view->z;return dx*dx+dz*dz<range*range;}
static int park(int x,int z){return cm_park(x,z);}
static int cityMid; /* LOD intermedio (definido en city3d.inc) */
static void tree_shape(float x,float z,int kind,int simple); /* shapes.inc */
static int treeKind=-1; /* -1: por hash de posicion; 0 frondoso, 1 palma, 2 cipres */
static void tree(float x,float z){
    int kind=treeKind;
    if(kind<0){unsigned h=(unsigned)(x*1.7f)*2654435761u^(unsigned)(z*2.3f)*40503u;h^=h>>11;unsigned r=h%100;kind=r<58?0:r<80?1:2;}
    tree_shape(x,z,kind,cityMid||view_distance(x,z)>170);
}
#include "city3d.inc"
#include "city26.inc"
#include "shapes.inc"
#include "daylight.inc"
static void city(void){city_v26();}
static void car(const R3Car *c){
    if(!nearby(c->x,c->z,500))return;
    if(!sphere_visible(c->x,c->z,30))return;
    float dist=view_distance(c->x,c->z);
    static const uint32_t colors[]={COLOR(93,196,173),COLOR(245,198,75),COLOR(221,106,86),COLOR(232,230,211),COLOR(108,157,207),COLOR(167,124,182)};
    uint32_t paint=c->police?COLOR(207,226,229):colors[c->type%6];
    int type=c->police?0:c->type%6;
    if(dist>380){ /* LOD lejano: dos cajas */
        box(c->x,c->z,3,36,18,7,c->angle,CAR_PAINT,CAR_PAINT,paint);box(c->x,c->z,10,20,15,7,c->angle,CAR_SIDE,CAR_PAINT,paint);return;
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
    hull(c->x,c->z,c->angle,prof,n,mats,floor,paint);
    /* bajos oscuros */
    box(c->x,c->z,floor-1.2f,prof[n-1].x-prof[0].x-4,prof[0].w*1.7f,1.2f,c->angle,CAR_PAINT,CAR_PAINT,COLOR(40,40,42));
    if(dist>300)return; /* LOD: sin ruedas detalladas a lo lejos */
    float spin=view->time*c->speed*.08f;
    for(int s=-1;s<=1;s+=2)for(int e=-1;e<=1;e+=2)wheel(c->x,c->z,c->angle,e*11,wr,s*(prof[0].w+.4f),wr,2.6f,spin);
    for(int s=-1;s<=1;s+=2){
        Point p=local(prof[n-1].x+.3f,0,s*5.5f,c->x,c->z,c->angle);
        box(p.x,p.z,floor+2.5f,.8f,3.6f,1.8f,c->angle,CAR_PAINT,CAR_PAINT,COLOR(255,244,192));
        p=local(prof[0].x-.3f,0,s*5.5f,c->x,c->z,c->angle);
        box(p.x,p.z,floor+2.5f,.8f,3.6f,1.8f,c->angle,CAR_PAINT,CAR_PAINT,COLOR(255,61,42));
    }
    /* retrovisores y matricula */
    for(int s=-1;s<=1;s+=2){Point p=local(6,0,s*(prof[0].w+1.2f),c->x,c->z,c->angle);box(p.x,p.z,11.5f,1.8f,2.2f,1.2f,c->angle,CAR_PAINT,CAR_PAINT,paint);}
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
    float phase=view->time*8.5f+style,swing=walking?sinf(phase):0,bob=walking?fabsf(cosf(phase))*.3f:0;
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
/* Optimizacion (Claude): las ~7.000 llamadas por fotograma compartian los mismos senos/cosenos de la marcha;
   se calculan una vez por fotograma. sin(pi*w) se aproxima con 4w(1-w) (error < 6%, invisible en la rodilla). */
static float poseWave,poseRun,poseMotion,poseBreath,poseBob;
static float footStep[2],footLift[2];
static float elbowSin,elbowCos;
static void pose_prepare(void){
    poseMotion=geo_clamp(view->motion,0,2.1f);poseWave=sinf(view->gaitPhase);
    poseRun=geo_clamp((poseMotion-1)/1.08f,0,1);
    elbowSin=sinf(poseRun*1.05f);elbowCos=cosf(poseRun*1.05f);
    if(view->weapon>0){elbowSin=sinf(1.12f);elbowCos=cosf(1.12f);}
    float moving=geo_clamp(poseMotion,0,1);
    poseBreath=sinf(view->time*2.2f)*.08f*(1-moving);
    poseBob=(1-cosf(view->gaitPhase*2))*(.13f+.22f*poseRun)*moving;
    /* Contact: planted foot travels backwards. Recovery: bent knee and foot
       lift only while swinging forwards. Calculated once, not per vertex. */
    for(int i=0;i<2;i++){
        float phase=fmodf(view->gaitPhase/(2*PI)+i*.5f,1);
        if(phase<0)phase+=1;
        float support=.62f-.16f*poseRun,stride=(4.2f+3.5f*poseRun)*moving;
        if(phase<support){footStep[i]=stride*(1-2*phase/support);footLift[i]=0;}
        else{float t=(phase-support)/(1-support),ease=t*t*(3-2*t);
            footStep[i]=stride*(-1+2*ease);
            footLift[i]=sinf(PI*t)*(1.5f+3.8f*poseRun)*moving;
        }
    }
}
static Point player_pose(Point p,int bone){
    float motion=poseMotion,wave=poseWave,run=poseRun;
    int side=(bone&1)?-1:1;
    /* Animacion procedural (Claude v2.4): ademas del paso, contragiro de hombros y cadera, brazos con codo,
       inclinacion hacia delante al correr y arco del pie. Todo en funcion de la fase de marcha ya existente. */
    if(bone==1||bone==2){
        float weight=geo_clamp(1-p.y/14.8f,0,1);
        float lift=footLift[bone-1],step=footStep[bone-1];
        /* Flexible knee blend preserves a continuous baggy pant surface. */
        p.x+=step*weight+lift*.42f*(4*weight*(1-weight));
        p.y+=lift*weight;
        /* Arco del pie: el pie que avanza se eleva mas en mitad del paso (pierna casi recta al apoyar). */
        p.y+=lift*.12f*(4*weight*(1-weight));
        /* La cadera gira ligeramente con la pierna que avanza. */
        float hip=geo_clamp((p.y-6)/8,0,1)*wave*.06f*motion;p.z+=p.x*hip;
    }else if(bone>=3){
        float weight=geo_clamp((24-p.y)/12,0,1);
        /* Brazo: balanceo opuesto a la pierna, mas amplio al correr; el codo se dobla y sube (antebrazo adelantado). */
        float swing=-footStep[(bone-3)&1]*.62f;
        if(view->weapon>0)swing*=.12f;
        /* Rotate the forearm around the elbow instead of stretching it up. */
        if(p.y<16.5f){float dy=p.y-16.5f,xx=p.x;
            p.x=xx*elbowCos-dy*elbowSin;p.y=16.5f+xx*elbowSin+dy*elbowCos;
        }
        p.x+=swing*weight;
        p.y+=fmaxf(0,swing)*.35f*weight*weight;
        p.z-=side*weight*run*.8f; /* los brazos se cierran hacia el cuerpo al correr */
    }
    float upper=geo_clamp((p.y-13)/12,0,1);
    /* Contragiro de hombros respecto a la cadera (torsion del torso). */
    if(bone==0||bone>=3){float tw=-wave*.10f*motion*upper;float nx=p.x-p.z*tw,nz=p.z+p.x*tw;p.x=nx;p.z=nz;}
    p.y+=poseBreath*upper;
    p.y+=poseBob*geo_clamp(p.y/4,0,1);
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
            if(a->x==b->x&&a->y==b->y&&a->z==b->z&&a->u==b->u&&a->v==b->v&&a->color==b->color&&a->bone==b->bone){found=u;break;}
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
  p[i]=local(grip.x+xx,grip.y+yy,grip.z+zz,x,z,angle);
 }
 const int faces[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
 for(int f=0;f<6;f++)quad(20,p[faces[f][0]],p[faces[f][1]],p[faces[f][2]],p[faces[f][3]],shade(color,f==3?1:.72f+.04f*f),1,1);
}
static void equipped_weapon(float x,float z,float angle){
 int id=view->weapon;if(id<1||id>7)return;
 Point grip=player_pose(point(.95f,12.8f,-3.95f),3);
 uint32_t steel=COLOR(105,115,123),dark=COLOR(41,45,49),wood=COLOR(145,89,48);
 if(id==7){
  weapon_part(grip,0,2.2f,0,.65f,5,.65f,wood,x,z,angle);
  weapon_part(grip,0,9,0,1.25f,9,1.25f,wood,x,z,angle);return;
 }
 weapon_part(grip,0,-.2f,0,1.15f,2.2f,1.0f,dark,x,z,angle);
 if(id<3){
  weapon_part(grip,1.2f,1.1f,0,id==1?4:4.8f,.95f,.85f,steel,x,z,angle);
  if(id==2)weapon_part(grip,.7f,.7f,0,1.5f,1.4f,1.4f,dark,x,z,angle);
 }else{
  float len=id==3?6:id==5?13:id==6?15:11;
  weapon_part(grip,2,1,0,5,1.6f,1.3f,steel,x,z,angle);
  weapon_part(grip,len*.55f,1.25f,0,len*.70f,.48f,.48f,dark,x,z,angle);
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
        const PlayerVertex *a=&player_mesh[playerUnique[u]];Point q=player_pose(point(a->x,a->y,a->z),a->bone);
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
        float x=view->hubs[i][0]+18,z=view->hubs[i][1];if(!nearby(x,z,400))continue;
        /* An actual textured computer terminal at each interaction point. */
        /* v2.7: terminal de barrio: pedestal hexagonal, pantalla inclinada y visera */
        cylinder(x,z,0,5.5f,9,6,0,CONCRETE,METAL,0xffffffffu);
        box(x,z,9,2.4f,9,7,0,GLASS,METAL,COLOR(111,249,210));
        frustum(x,z,16,6,2,3,6,0,METAL,COLOR(80,84,90));
        if(i<24&&!(view->collected&(1u<<i)))box(x+15,z+38,5+sinf(view->time*2),1,7,7,view->time,WHEEL,WHEEL,0xffffffffu);
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
    sceGuStart(GU_DIRECT,commands);
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
    sceGuStart(GU_DIRECT,commands);sceGuFinish();sceGuSync(0,0);
#endif
}
void r3_gu_swap(void){
#ifndef R3_HOST
    sceGuSwapBuffers();
#endif
}
void r3_set_draw_buffer(uint32_t *fb){
#ifndef R3_HOST
    sceGuStart(GU_DIRECT,commands);sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);sceGuFinish();sceGuSync(0,0);
#else
    (void)fb;
#endif
}
/* v2.13.4 (Claude): trazas por fase; psp_main instala un callback que escribe en ms0:/NARCADE_DEBUG.TXT durante los
   primeros fotogramas tras cargar partida, para saber que subsistema se traga la consola. */
static void (*traceFn)(const char*)=0;static int traceFrames=0;
#define TRACE(s) do{if(traceFn&&traceFrames>0)traceFn(s);}while(0)
void r3_trace(void (*fn)(const char*),int frames){traceFn=fn;traceFrames=frames;}
int r3_overflow(void){return overflow;}
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
void r3_draw(uint32_t *fb,const R3Scene *s){
    TRACE("r3:inicio");
    view=s;overflow=0;memset(used,0,sizeof(used));day_update(s->time);glowUsed=0;shadowUsed=0;geographic=1;rigid=0;
    TRACE("r3:camara");camera(s);
#ifndef AB_NOCITY
    TRACE("r3:ciudad");city();TRACE("r3:ciudad-ok");
#endif
    TRACE("r3:coches");rigid=2;
#ifndef AB_NOCARS
    for(int i=0;i<s->carCount;i++){objectX=s->cars[i].x;objectZ=s->cars[i].z;objectYaw=s->cars[i].angle;car(&s->cars[i]);}
#endif
    TRACE("r3:jugador");rigid=1;
    objectX=s->x;objectZ=s->z;objectYaw=s->angle;playerLift=s->lift;rigX=-1e9f; /* invalidar cache */
#ifndef AB_NOPLAYER
    if(!s->driving&&!s->inMetro)person(s->x,s->z,s->angle,-1,s->moving);
#endif
    playerLift=0;rigX=-1e9f;
#ifndef AB_NOPEOPLE
    for(int i=0;i<s->personCount;i++){objectX=s->people[i].x;objectZ=s->people[i].z;objectYaw=s->people[i].angle;person(objectX,objectZ,objectYaw,s->people[i].style,1);}
#endif
    rigid=0;
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
    TRACE("r3:ge-inicio");sceKernelDcacheWritebackAll();sceGuStart(GU_DIRECT,commands);
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
    sceGuDisable(GU_FOG);
#endif
#ifndef AB_NODRAW
    for(int m=0;m<MAT_COUNT;m++)if(used[m]){
        if(m<VRAM_MATERIALS)sceGuTexImage(0,128,128,128,(const char*)textureBase+m*128*128*2);
        else sceGuTexImage(0,64,64,64,textures3d_data+VRAM_MATERIALS*128*128*2+(m-VRAM_MATERIALS)*64*64*2);
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,used[m],0,mesh[m]);
    }
#endif
    /* v2.5: pase de sombras (oscurece) y pase aditivo (luces, nubes, sol). Sin textura ni niebla. */
    sceGuDisable(GU_FOG);sceGuDisable(GU_TEXTURE_2D);sceGuEnable(GU_BLEND);sceGuDepthMask(GU_TRUE);
    if(shadowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,shadowUsed,0,shadowMesh);}
    if(glowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffff);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,glowUsed,0,glowMesh);}
    sceGuDisable(GU_BLEND);sceGuDepthMask(GU_FALSE);sceGuEnable(GU_TEXTURE_2D);
    sceGuFinish();sceGuSync(0,0);
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
void r3_overlay(uint32_t *fb,const uint32_t *rgba){
#ifndef R3_HOST
    typedef struct {float u,v,x,y,z;} SpriteVertex;
    sceKernelDcacheWritebackAll();sceGuStart(GU_DIRECT,commands);
    sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);
    sceGuDisable(GU_DEPTH_TEST);sceGuDisable(GU_FOG);sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_TEXTURE_2D);sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);
    sceGuTexMode(GU_PSM_8888,0,0,0);sceGuTexImage(0,512,512,512,rgba);
    sceGuTexFunc(GU_TFX_REPLACE,GU_TCC_RGBA);sceGuTexFilter(GU_NEAREST,GU_NEAREST);
    sceGuTexScale(1,1);sceGuTexOffset(0,0);sceGuTexWrap(GU_CLAMP,GU_CLAMP);
    /* Narrow sprites avoid the GE's large textured-sprite cache penalty. */
    for(int x=0;x<480;x+=32){
        int end=x+32>480?480:x+32;
        SpriteVertex *v=sceGuGetMemory(2*sizeof(*v));
        v[0]=(SpriteVertex){x,0,x,0,0};v[1]=(SpriteVertex){end,272,end,272,0};
        sceGuDrawArray(GU_SPRITES,GU_TEXTURE_32BITF|GU_VERTEX_32BITF|GU_TRANSFORM_2D,2,0,v);
    }
    sceGuFinish();sceGuSync(0,0);
#else
    (void)fb;(void)rgba;
#endif
}
