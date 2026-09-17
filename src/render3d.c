/* Narcade 3D: original low-poly meshes, batched by material, PSP GE backend.
 * No allocation in the frame loop. Texture/vertex memory stays bounded.
 */
#include "render3d.h"
#include "world_geo.h"
#include <math.h>
#include <string.h>
#ifndef R3_HOST
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspge.h>
#endif

#define PI 3.14159265358979323846f
#define MAT_COUNT 29
#define VRAM_MATERIALS 21
#define MAX_VERTICES 8190
#include "player_mesh.h"
#define COLOR(r,g,b) (0xff000000u | (r) | ((g)<<8) | ((b)<<16))
enum { ROAD,SIDEWALK,BRICK,STUCCO,SHOP,ROOF,GRASS,WATER,JACKET,JEANS,FACE,WHEEL,CAR_SIDE,CAR_PAINT,GLASS,MURAL,JACKET_BACK,SLEEVE,SKIN,HAIR };
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
static int clipEnabled=1;
static int geographic=0,rigid=0;
static float objectX,objectZ,objectYaw;
static float fixedGround=-1000000;

static Point point(float x,float y,float z){Point p={x,y,z};return p;}
static uint32_t shade(uint32_t c,float f){return COLOR((int)((c&255)*f),(int)(((c>>8)&255)*f),(int)(((c>>16)&255)*f));}
static void camera(const R3Scene *s){
    float px,pz;geo_project(s->x,s->z,&px,&pz);float yaw=s->yaw;
    float h=geo_height(s->x,s->z);
    eye=point(px-cosf(yaw)*s->cameraDistance,h+(s->driving?54:43),pz-sinf(yaw)*s->cameraDistance);
    float lx,lz;geo_unproject(eye.x,eye.z,&lx,&lz);eye.y=fmaxf(eye.y,geo_height(lx,lz)+12);
    target=point(px+cosf(yaw)*25,h+(s->driving?10:14),pz+sinf(yaw)*25);
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
    planes[0][3]-=2.1f;planes[1][3]+=750;
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
static int litAlready=0; /* box()/ground() ya iluminan por cara: polygon() no vuelve a atenuar */
/* Optimizacion (Claude): en modo rigido todos los vertices de un objeto comparten proyeccion, rumbo y altura;
   antes se recalculaban (geo_project + geo_heading con atan2, cos, sin) para CADA vertice. Se cachean por objeto. */
static float rigX=1e30f,rigZ=1e30f,rigYaw=1e30f,rigGX,rigGZ,rigCos,rigSin,rigH;
static int rigMode=-1;
static Point rigBX,rigBY,rigBZ;
static Point unit(Point p){float n=sqrtf(p.x*p.x+p.y*p.y+p.z*p.z);return point(p.x/n,p.y/n,p.z/n);}
static Point cross3(Point a,Point b){return point(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static float projected_height(float x,float z){float lx,lz;geo_unproject(x,z,&lx,&lz);return geo_height(lx,lz);}
static void rigid_cache(void){
    if(objectX==rigX&&objectZ==rigZ&&objectYaw==rigYaw&&rigid==rigMode)return;
    rigMode=rigid;
    rigX=objectX;rigZ=objectZ;rigYaw=objectYaw;
    geo_project(objectX,objectZ,&rigGX,&rigGZ);
    float da=geo_heading(objectX,objectZ,objectYaw)-objectYaw;rigCos=cosf(da);rigSin=sinf(da);
    rigH=geo_height(objectX,objectZ);
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
static void quad(int m,Point a,Point b,Point c,Point d,uint32_t color,float u,float v){
    Vertex p[4]={{0,0,color,a.x,a.y,a.z},{u,0,color,b.x,b.y,b.z},{u,v,color,c.x,c.y,c.z},{0,v,color,d.x,d.y,d.z}};
    polygon(m,p,4);
}
/* Optimizacion (Claude): la misma rotacion se repite miles de veces por fotograma (todos los vertices de un
   objeto comparten angulo). Cachear seno/coseno del ultimo angulo evita dos llamadas trigonometricas por vertice. */
static float localA=1e30f,localC=1,localS=0;
static Point local(float x,float y,float z,float cx,float cz,float a){
    if(a!=localA){localA=a;localC=cosf(a);localS=sinf(a);}
    return point(cx+x*localC-z*localS,y,cz+x*localS+z*localC);
}
/* Object forward is +X. Faces have UVs with their top at v=0. */
static void box(float x,float z,float bottom,float length,float width,float height,float angle,int side,int top,uint32_t color){
    float l=length*.5f,w=width*.5f,h=bottom+height;
    float savedGround=fixedGround;
    if(geographic&&!rigid){
        float low=geo_height(x,z),high=low;
        for(int a=-1;a<=1;a+=2)for(int b=-1;b<=1;b+=2){Point p=local(a*l,0,b*w,x,z,angle);float hh=geo_height(p.x,p.z);low=fminf(low,hh);high=fmaxf(high,hh);}
        fixedGround=high;
        if(bottom==0)bottom=low-high-1; /* Foundation reaches the downhill ground. */
    }
    Point p[8]={local(-l,bottom,-w,x,z,angle),local(l,bottom,-w,x,z,angle),local(l,bottom,w,x,z,angle),local(-l,bottom,w,x,z,angle),
        local(-l,h,-w,x,z,angle),local(l,h,-w,x,z,angle),local(l,h,w,x,z,angle),local(-l,h,w,x,z,angle)};
    /* v2.5: cada cara segun su orientacion respecto al sol (angle rota las normales locales). */
    float ca=cosf(angle),sa=sinf(angle);uint32_t sc=emissive_color(side,color);
    litAlready=1;
    quad(side,p[4],p[5],p[1],p[0],lit_color(sc,sa,0,-ca),1,1);   /* cara -z */
    quad(side,p[6],p[7],p[3],p[2],lit_color(sc,-sa,0,ca),1,1);   /* cara +z */
    quad(side,p[5],p[6],p[2],p[1],lit_color(sc,ca,0,sa),1,1);    /* cara +x */
    quad(side,p[7],p[4],p[0],p[3],lit_color(sc,-ca,0,-sa),1,1);  /* cara -x */
    quad(top,p[7],p[6],p[5],p[4],lit_color(color,0,1,0),1,1);
    litAlready=0;
    fixedGround=savedGround;
}
static void ground(int mat,float x,float z,float w,float d,float y,uint32_t color,float repeat){
    /* Subdivide at terrain lattice boundaries: roads genuinely climb hills. */
    for(float zz=z;zz<z+d-.001f;){float endz=fminf(z+d,geo_next_z(zz));if(endz<=zz+.001f)endz=fminf(z+d,zz+.01f);
        for(float xx=x;xx<x+w-.001f;){float endx=fminf(x+w,(floorf(xx/80)+1)*80);if(endx<=xx+.001f)endx=fminf(x+w,xx+80);
            uint32_t lc=lit_color(color,0,1,0);
            Vertex v[4]={{(xx-x)/w*repeat,(zz-z)/d*repeat,lc,xx,y,zz},{(endx-x)/w*repeat,(zz-z)/d*repeat,lc,endx,y,zz},
                         {(endx-x)/w*repeat,(endz-z)/d*repeat,lc,endx,y,endz},{(xx-x)/w*repeat,(endz-z)/d*repeat,lc,xx,y,endz}};
            litAlready=1;polygon(mat,v,4);litAlready=0;xx=endx;
        }zz=endz;
    }
}
static int nearby(float x,float z,float range){float dx=x-view->x,dz=z-view->z;return dx*dx+dz*dz<range*range;}
static int park(int x,int z){return (x==2&&z==3)||(x==2&&z==1)||(x==0&&z==2)||(x==4&&z==5)||(x==4&&z==6);}
static void tree(float x,float z){
    box(x,z,0,3,3,25,0,ROOF,ROOF,COLOR(104,85,61));
    box(x,z,19,20,20,13,.35f,GRASS,GRASS,COLOR(190,220,155));
    box(x,z,30,13,13,10,-.2f,GRASS,GRASS,COLOR(216,239,176));
}
#include "city3d.inc"
#include "daylight.inc"
static void city(void){city_v25();}
static void car(const R3Car *c){
    if(!nearby(c->x,c->z,560))return;
    if(!sphere_visible(c->x,c->z,30))return;
    float dist=view_distance(c->x,c->z);
    static const uint32_t colors[]={COLOR(93,196,173),COLOR(245,198,75),COLOR(221,106,86),COLOR(232,230,211),COLOR(108,157,207),COLOR(167,124,182)};
    uint32_t paint=c->police?COLOR(207,226,229):colors[c->type%6];
    box(c->x,c->z,3,36,18,7,c->angle,CAR_PAINT,CAR_PAINT,paint);
    Point cabin[8]={local(-12,10,-8,c->x,c->z,c->angle),local(9,10,-8,c->x,c->z,c->angle),local(9,10,8,c->x,c->z,c->angle),local(-12,10,8,c->x,c->z,c->angle),
        local(-8,17,-6.7f,c->x,c->z,c->angle),local(4,17,-6.7f,c->x,c->z,c->angle),local(4,17,6.7f,c->x,c->z,c->angle),local(-8,17,6.7f,c->x,c->z,c->angle)};
    quad(CAR_SIDE,cabin[4],cabin[5],cabin[1],cabin[0],paint,1,1);
    quad(CAR_SIDE,cabin[6],cabin[7],cabin[3],cabin[2],paint,1,1);
    quad(GLASS,cabin[5],cabin[6],cabin[2],cabin[1],0xffffffffu,1,1);
    quad(GLASS,cabin[7],cabin[4],cabin[0],cabin[3],0xffffffffu,1,1);
    quad(CAR_PAINT,cabin[7],cabin[6],cabin[5],cabin[4],paint,1,1);
    if(dist>300)return; /* LOD: sin ruedas detalladas a lo lejos */
    for(int s=-1;s<=1;s+=2)for(int e=-1;e<=1;e+=2){
        float u=e*11,v=s*9.3f,rotation=view->time*c->speed*.08f;
        for(int k=0;k<8;k++){
            float a=k*PI/4,b=(k+1)*PI/4;
            Point p=local(u+cosf(a)*4,4+sinf(a)*4,v,c->x,c->z,c->angle);
            Point q=local(u+cosf(b)*4,4+sinf(b)*4,v,c->x,c->z,c->angle);
            Point center=local(u,4,v,c->x,c->z,c->angle);
            Vertex face[3]={{.5f,.5f,0xffffffffu,center.x,center.y,center.z},{.5f+cosf(a+rotation)*.49f,.5f-sinf(a+rotation)*.49f,0xffffffffu,p.x,p.y,p.z},{.5f+cosf(b+rotation)*.49f,.5f-sinf(b+rotation)*.49f,0xffffffffu,q.x,q.y,q.z}};
            polygon(WHEEL,face,3);
            quad(CAR_PAINT,p,q,local(u+cosf(b)*4,4+sinf(b)*4,v-s*2.3f,c->x,c->z,c->angle),local(u+cosf(a)*4,4+sinf(a)*4,v-s*2.3f,c->x,c->z,c->angle),COLOR(36,36,35),1,1);
        }
    }
    for(int s=-1;s<=1;s+=2){
        Point p=local(18.2f,0,s*6,c->x,c->z,c->angle);
        box(p.x,p.z,5,1,4,2,c->angle,CAR_PAINT,CAR_PAINT,COLOR(255,244,192));
        p=local(-18.2f,0,s*6,c->x,c->z,c->angle);
        box(p.x,p.z,5,1,4,2,c->angle,CAR_PAINT,CAR_PAINT,COLOR(255,61,42));
    }
    if(c->police)box(c->x,c->z,18,3,13,2,c->angle,CAR_PAINT,CAR_PAINT,((int)(view->time*5)&1)?COLOR(255,65,49):COLOR(45,147,255));
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
static void simple_person(float x,float z,float angle,int style,int walking){
    if(!nearby(x,z,330))return;
    if(!sphere_visible(x,z,22))return;
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
static void person(float x,float z,float angle,int style,int walking){
    if(style>=0){simple_person(x,z,angle,style,walking);return;}
    pose_prepare();
    /* Optimizacion (Claude): ruta rapida para los ~2.300 triangulos del jugador. Son pequenos y estan siempre
       delante de la camara, asi que no pasan por polygon() (sin recorte de 6 planos ni copias): se posan, rotan,
       proyectan con la transformacion rigida cacheada y se escriben directamente en el lote de su material. */
    if(geographic&&rigid)rigid_cache();
    if(angle!=localA){localA=angle;localC=cosf(angle);localS=sinf(angle);}
    for(int i=0;i<PLAYER_VERTEX_COUNT;i+=3){
        int mat=player_mesh[i].mat;
        if(used[mat]+3>MAX_VERTICES){overflow++;continue;}
        Vertex *out=mesh[mat]+used[mat];
        for(int j=0;j<3;j++){
            const PlayerVertex *a=&player_mesh[i+j];Point q=player_pose(point(a->x,a->y,a->z),a->bone);
            float wx=x+q.x*localC-q.z*localS,wz=z+q.x*localS+q.z*localC,wy=q.y;
            if(geographic){
                if(rigid){float dx=wx-objectX,dz=wz-objectZ;wx=rigGX+rigCos*dx-rigSin*dz;wz=rigGZ+rigSin*dx+rigCos*dz;wy+=rigH;}
                else{float gx,gz;geo_project(wx,wz,&gx,&gz);wy+=geo_height(wx,wz);wx=gx;wz=gz;}
            }
            out[j]=(Vertex){a->u,a->v,day_scale(a->color),wx,wy,wz};
        }
        used[mat]+=3;
    }
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
}
static void landmarks(void){
    for(int i=0;i<30;i++){
        float x=view->hubs[i][0]+18,z=view->hubs[i][1];if(!nearby(x,z,400))continue;
        /* An actual textured computer terminal at each interaction point. */
        box(x,z,0,10,8,10,0,SIDEWALK,SIDEWALK,0xffffffffu);
        box(x,z,10,2,10,8,0,GLASS,CAR_PAINT,COLOR(111,249,210));
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
void r3_draw(uint32_t *fb,const R3Scene *s){
    view=s;overflow=0;memset(used,0,sizeof(used));day_update(s->time);glowUsed=0;shadowUsed=0;geographic=1;rigid=0;camera(s);city();
    rigid=2;
    for(int i=0;i<s->carCount;i++){objectX=s->cars[i].x;objectZ=s->cars[i].z;objectYaw=s->cars[i].angle;car(&s->cars[i]);}
    rigid=1;
    objectX=s->x;objectZ=s->z;objectYaw=s->angle;
    if(!s->driving)person(s->x,s->z,s->angle,-1,s->moving);
    for(int i=0;i<s->personCount;i++){objectX=s->people[i].x;objectZ=s->people[i].z;objectYaw=s->people[i].angle;person(objectX,objectZ,objectYaw,s->people[i].style,1);}
    rigid=0;
    landmarks();
    /* v2.5: sombras proyectadas, faros, farolas y nubes (en coordenadas logicas; geo_point proyecta). */
    for(int i=0;i<s->carCount;i++)if(nearby(s->cars[i].x,s->cars[i].z,320)){cast_shadow(s->cars[i].x,s->cars[i].z,11,9);headlights(s->cars[i].x,s->cars[i].z,s->cars[i].angle);}
    if(!s->driving)cast_shadow(s->x,s->z,4,26);
    for(int i=0;i<s->personCount;i++)if(nearby(s->people[i].x,s->people[i].z,220))cast_shadow(s->people[i].x,s->people[i].z,3,24);
    street_lamps_glow();clouds(s->time);
    geographic=0;
#ifndef R3_HOST
    sceKernelDcacheWritebackAll();sceGuStart(GU_DIRECT,commands);
    sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);
    sceGuClearColor(skyColor);sceGuClearDepth(0);sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);sceGuDisable(GU_BLEND);sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_5650,0,0,1);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);sceGuTexScale(1,1);sceGuTexOffset(0,0);sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_FOG);sceGuFog(300,690,skyColor);
    sceGumMatrixMode(GU_PROJECTION);sceGumLoadIdentity();sceGumPerspective(62,480.0f/272,2,760);
    ScePspFVector3 e={eye.x,eye.y,eye.z},t={target.x,target.y,target.z};
    ScePspFVector3 up={0,1,0};sceGumMatrixMode(GU_VIEW);sceGumLoadIdentity();sceGumLookAt(&e,&t,&up);
    sceGumMatrixMode(GU_MODEL);sceGumLoadIdentity();
    for(int m=0;m<MAT_COUNT;m++)if(used[m]){
        if(m<VRAM_MATERIALS)sceGuTexImage(0,128,128,128,(const char*)textureBase+m*128*128*2);
        else sceGuTexImage(0,64,64,64,textures3d_data+VRAM_MATERIALS*128*128*2+(m-VRAM_MATERIALS)*64*64*2);
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,used[m],0,mesh[m]);
    }
    /* v2.5: pase de sombras (oscurece) y pase aditivo (luces, nubes, sol). Sin textura ni niebla. */
    sceGuDisable(GU_FOG);sceGuDisable(GU_TEXTURE_2D);sceGuEnable(GU_BLEND);sceGuDepthMask(GU_TRUE);
    if(shadowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_ONE_MINUS_SRC_ALPHA,0,0);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,shadowUsed,0,shadowMesh);}
    if(glowUsed){sceGuBlendFunc(GU_ADD,GU_SRC_ALPHA,GU_FIX,0,0xffffff);sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,glowUsed,0,glowMesh);}
    sceGuDisable(GU_BLEND);sceGuDepthMask(GU_FALSE);sceGuEnable(GU_TEXTURE_2D);
    sceGuFinish();sceGuSync(0,0);
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
