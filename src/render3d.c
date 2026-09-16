/* Narcade 3D: original low-poly meshes, batched by material, PSP GE backend.
 * No allocation in the frame loop. Texture/vertex memory stays bounded.
 */
#include "render3d.h"
#include <math.h>
#include <string.h>
#ifndef R3_HOST
#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspge.h>
#endif

#define PI 3.14159265358979323846f
#define MAT_COUNT 20
#define MAX_VERTICES 4092
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

static Point point(float x,float y,float z){Point p={x,y,z};return p;}
static uint32_t shade(uint32_t c,float f){return COLOR((int)((c&255)*f),(int)(((c>>8)&255)*f),(int)(((c>>16)&255)*f));}
static void camera(const R3Scene *s){
    eye=point(s->x-cosf(s->yaw)*s->cameraDistance,s->driving?54:43,s->z-sinf(s->yaw)*s->cameraDistance);
    target=point(s->x+cosf(s->yaw)*25,s->driving?10:14,s->z+sinf(s->yaw)*25);
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
}
static float plane_distance(const Vertex *v,int k){return planes[k][0]*v->x+planes[k][1]*v->y+planes[k][2]*v->z+planes[k][3];}
static Vertex interpolate(Vertex a,Vertex b,float t){
    Vertex v={a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t,a.color,a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};return v;
}
static void polygon(int mat,Vertex *input,int count){
    Vertex buffers[2][16];memcpy(buffers[0],input,count*sizeof(Vertex));int src=0;
    /* PSP rejects large triangles crossing its near/guard planes. Clip in
       world space before submission, including UV interpolation at cuts. */
    for(int k=0;clipEnabled&&k<6&&count>=3;k++){
        int out=0;Vertex a=buffers[src][count-1];float da=plane_distance(&a,k);
        for(int j=0;j<count;j++){
            Vertex b=buffers[src][j];float db=plane_distance(&b,k);
            if((da>=0)!=(db>=0))buffers[src^1][out++]=interpolate(a,b,da/(da-db));
            if(db>=0)buffers[src^1][out++]=b;
            a=b;da=db;
        }
        count=out;src^=1;
    }
    if(count<3)return;
    int needed=(count-2)*3;
    if(used[mat]+needed>MAX_VERTICES){overflow++;return;}
    Vertex *p=mesh[mat]+used[mat];used[mat]+=needed;
    for(int j=1;j<count-1;j++){*p++=buffers[src][0];*p++=buffers[src][j];*p++=buffers[src][j+1];}
}
static void quad(int m,Point a,Point b,Point c,Point d,uint32_t color,float u,float v){
    Vertex p[4]={{0,0,color,a.x,a.y,a.z},{u,0,color,b.x,b.y,b.z},{u,v,color,c.x,c.y,c.z},{0,v,color,d.x,d.y,d.z}};
    polygon(m,p,4);
}
static Point local(float x,float y,float z,float cx,float cz,float a){
    float c=cosf(a),s=sinf(a);return point(cx+x*c-z*s,y,cz+x*s+z*c);
}
/* Object forward is +X. Faces have UVs with their top at v=0. */
static void box(float x,float z,float bottom,float length,float width,float height,float angle,int side,int top,uint32_t color){
    float l=length*.5f,w=width*.5f,h=bottom+height;
    Point p[8]={local(-l,bottom,-w,x,z,angle),local(l,bottom,-w,x,z,angle),local(l,bottom,w,x,z,angle),local(-l,bottom,w,x,z,angle),
        local(-l,h,-w,x,z,angle),local(l,h,-w,x,z,angle),local(l,h,w,x,z,angle),local(-l,h,w,x,z,angle)};
    quad(side,p[4],p[5],p[1],p[0],shade(color,.82f),1,1);
    quad(side,p[6],p[7],p[3],p[2],color,1,1);
    quad(side,p[5],p[6],p[2],p[1],shade(color,.94f),1,1);
    quad(side,p[7],p[4],p[0],p[3],shade(color,.72f),1,1);
    quad(top,p[7],p[6],p[5],p[4],color,1,1);
}
static void ground(int mat,float x,float z,float w,float d,float y,uint32_t color,float repeat){
    quad(mat,point(x,y,z),point(x+w,y,z),point(x+w,y,z+d),point(x,y,z+d),color,repeat,repeat);
}
static int nearby(float x,float z,float range){float dx=x-view->x,dz=z-view->z;return dx*dx+dz*dz<range*range;}
static int park(int x,int z){return (x==2&&z==3)||(x==2&&z==1)||(x==0&&z==2)||(x==4&&z==5)||(x==4&&z==6);}
static void tree(float x,float z){
    box(x,z,0,3,3,25,0,ROOF,ROOF,COLOR(104,85,61));
    box(x,z,19,20,20,13,.35f,GRASS,GRASS,COLOR(190,220,155));
    box(x,z,30,13,13,10,-.2f,GRASS,GRASS,COLOR(216,239,176));
}
static void city(void){
    const uint32_t white=0xffffffffu;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320,z=bz*320;
        if(!nearby(x+160,z+160,790))continue;
        ground(ROAD,x,z,320,320,0,white,12);
        ground(SIDEWALK,x+86,z+86,234,234,.35f,white,12);
        for(int k=0;k<320;k+=40){
            ground(CAR_PAINT,x+k,z+42,19,1.2f,.15f,COLOR(244,208,100),1);
            ground(CAR_PAINT,x+42,z+k,1.2f,19,.15f,COLOR(244,208,100),1);
        }
        for(int k=0;k<5;k++){
            ground(CAR_PAINT,x+9+k*13,z+75,7,5,.2f,white,1);
            ground(CAR_PAINT,x+75,z+9+k*13,5,7,.2f,white,1);
        }
        if(park(bx,bz)){
            ground(GRASS,x+95,z+95,194,186,.5f,white,8);
            ground(SIDEWALK,x+100,z+173,185,14,.6f,white,6);
            ground(SIDEWALK,x+185,z+99,13,179,.6f,white,6);
            for(int k=0;k<4;k++)tree(x+112+k%2*157,z+115+k/2*139);
        }else{
            /* A continuous podium matches solid(): no walkable-looking gaps. */
            box(x+191,z+187,0,194,186,6,0,BRICK,ROOF,white);
            for(int k=0;k<4;k++){
                float xx=x+143+(k%2)*96,zz=z+141+(k/2)*91;
                int floors=2+(bx*7+bz*3+k)%3,mat=(bx+bz+k)%2?BRICK:STUCCO;
                for(int f=0;f<floors;f++)box(xx,zz,6+f*24,94,89,24,0,f?mat:SHOP,ROOF,white);
                box(xx,zz,6+floors*24,97,92,3,0,ROOF,ROOF,white);
                if((bx+bz+k)%3==0)box(xx+19,zz+15,9+floors*24,11,11,12,0,SIDEWALK,SIDEWALK,white);
                if(bx<2&&k>1)quad(MURAL,point(xx-45,25,zz+44.6f),point(xx+45,25,zz+44.6f),point(xx+45,7,zz+44.6f),point(xx-45,7,zz+44.6f),white,1,1);
            }
            tree(x+305,z+126);tree(x+305,z+265);
        }
        box(x+82,z+59,0,1.5f,1.5f,34,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));
        box(x+79,z+59,33,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
    }
    /* River is impassable except at the original street bridges. */
    for(int bz=0;bz<7;bz++)if(nearby(1428,bz*320+160,850)){
        ground(WATER,1396,bz*320+86,64,234,.8f,white,7);
        ground(ROAD,1392,bz*320,72,86,1,white,4);
        box(1395,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);
        box(1461,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);
    }
    /* Hills beyond the edge of the playable city. */
    for(int i=0;i<14;i++){
        float x=-400+i*270,z=-130,peak=140+(i%4)*65;
        quad(GRASS,point(x,0,z),point(x+150,peak,z-230),point(x+280,0,z),point(x,0,z),COLOR(104,154,127),2,2);
    }
}
static void car(const R3Car *c){
    if(!nearby(c->x,c->z,560))return;
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
    /* Reserve enough space before adding a pedestrian. The protagonist is
       submitted first; dense crowds must not truncate his outfit. Allow
       extra vertices for polygons split by the camera clip planes. */
    const int mats[]={JACKET,JACKET_BACK,JEANS,SLEEVE,SKIN,HAIR,FACE,CAR_PAINT};
    for(unsigned i=0;i<sizeof(mats)/sizeof(mats[0]);i++)if(used[mats[i]]+648>MAX_VERTICES)return;
    uint32_t tint=style<0?0xffffffffu:COLOR(175+(style*23)%80,170+(style*41)%85,160+(style*19)%95);
    float phase=view->time*8.5f+style,swing=walking?sinf(phase):0,bob=walking?fabsf(cosf(phase))*.3f:0;
    for(int s=-1;s<=1;s+=2){
        float stride=s*swing*3.4f,lift=walking?fmaxf(0,s*swing)*1.7f:0;
        Point hip=point(0,12+bob,s*2.2f),knee=point(stride*.55f-.6f,7+lift,s*2.3f),ankle=point(stride,2+lift,s*2.3f);
        limb(hip,knee,2.1f,1.65f,JEANS,0xffffffffu,x,z,angle);
        limb(knee,ankle,1.7f,1.35f,JEANS,0xffffffffu,x,z,angle);
        Point shoe=local(stride+1,0,s*2.3f,x,z,angle);
        box(shoe.x,shoe.z,.45f+lift,5.3f,3.3f,1.4f,angle,CAR_PAINT,CAR_PAINT,COLOR(47,48,46));
        box(shoe.x,shoe.z,.15f+lift,5.5f,3.4f,.45f,angle,CAR_PAINT,CAR_PAINT,COLOR(196,193,175));
        Point shoulder=point(0,20+bob,s*5.1f),elbow=point(-stride*.5f,15.8f+bob,s*5.5f),wrist=point(1-stride,12.7f+bob,s*5.2f);
        limb(shoulder,elbow,1.8f,1.35f,SLEEVE,tint,x,z,angle);
        limb(elbow,wrist,1.45f,1.05f,SLEEVE,tint,x,z,angle);
        limb(wrist,point(wrist.x+.3f,wrist.y-1.8f,wrist.z),1.1f,.8f,SKIN,0xffffffffu,x,z,angle);
    }
    /* Fitted waist, chest, shoulders; different front/back UV material. */
    float heights[4]={11,14,19.5f,21},rx[4]={2.1f,2.6f,3,2.1f},rz[4]={3.8f,4,4.8f,3.4f};
    for(int level=0;level<3;level++)for(int k=0;k<8;k++){
        Point p=local(ringC[k]*rx[level+1],heights[level+1]+bob,ringS[k]*rz[level+1],x,z,angle);
        Point q=local(ringC[k+1]*rx[level+1],heights[level+1]+bob,ringS[k+1]*rz[level+1],x,z,angle);
        Point r=local(ringC[k+1]*rx[level],heights[level]+bob,ringS[k+1]*rz[level],x,z,angle);
        Point s=local(ringC[k]*rx[level],heights[level]+bob,ringS[k]*rz[level],x,z,angle);
        int mat=ringC[k]+ringC[k+1]>0?JACKET:JACKET_BACK;
        Vertex v[4]={{.5f+ringS[k]*.5f,1-(heights[level+1]-11)/10,tint,p.x,p.y,p.z},{.5f+ringS[k+1]*.5f,1-(heights[level+1]-11)/10,tint,q.x,q.y,q.z},{.5f+ringS[k+1]*.5f,1-(heights[level]-11)/10,tint,r.x,r.y,r.z},{.5f+ringS[k]*.5f,1-(heights[level]-11)/10,tint,s.x,s.y,s.z}};
        polygon(mat,v,4);
    }
    limb(point(0,20.5f+bob,0),point(0,23+bob,0),1.35f,1.3f,SKIN,0xffffffffu,x,z,angle);
    /* Rounded jaw, cheeks, cranium. Face appears only on the forward surface. */
    float hy[6]={22.1f,23,25.2f,27.5f,28.6f,29},hr[6]={.4f,.78f,1,1,.75f,.05f};
    for(int level=0;level<5;level++)for(int k=0;k<8;k++){
        Point p=local(.2f+ringC[k]*2.6f*hr[level+1],hy[level+1]+bob,ringS[k]*2.35f*hr[level+1],x,z,angle);
        Point q=local(.2f+ringC[k+1]*2.6f*hr[level+1],hy[level+1]+bob,ringS[k+1]*2.35f*hr[level+1],x,z,angle);
        Point r=local(.2f+ringC[k+1]*2.6f*hr[level],hy[level]+bob,ringS[k+1]*2.35f*hr[level],x,z,angle);
        Point s=local(.2f+ringC[k]*2.6f*hr[level],hy[level]+bob,ringS[k]*2.35f*hr[level],x,z,angle);
        int mat=(k==0||k==7)?FACE:level>=2?HAIR:SKIN;
        float vt=1-(hy[level+1]-22.1f)/6.9f,vb=1-(hy[level]-22.1f)/6.9f;
        Vertex v[4]={{.5f+ringS[k]*.68f,vt,0xffffffffu,p.x,p.y,p.z},{.5f+ringS[k+1]*.68f,vt,0xffffffffu,q.x,q.y,q.z},{.5f+ringS[k+1]*.68f,vb,0xffffffffu,r.x,r.y,r.z},{.5f+ringS[k]*.68f,vb,0xffffffffu,s.x,s.y,s.z}};
        polygon(mat,v,4);
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
static void person(float x,float z,float angle,int style,int walking){
    if(style>=0){simple_person(x,z,angle,style,walking);return;}
    /* v2.2 (Claude): proporciones humanas. Altura 29.2 = ~7 cabezas; hombros 8.4 de ancho (antes 11.3),
       torso 4.8 de fondo (antes 7.2), cuello visible, brazos con codo y manos, piernas separadas con rodilla.
       Se conservan materiales, UVs y la animacion de andar (step/lift/bob). */
    float phase=view->time*8.5f,swing=walking?sinf(phase):0;
    float bob=walking?fabsf(cosf(phase))*.22f:0;
    for(int side=-1;side<=1;side+=2){
        float step=side*swing*3.0f,lift=walking?fmaxf(0,side*swing)*1.4f:0;
        float lz=side*1.85f;
        /* Pantalon: bajo, pantorrilla, rodilla, muslo, cadera. */
        BodyRing pants[]={
            {step,1.7f+lift,lz,1.55f,1.50f},
            {step*.9f,3.6f+lift,lz,1.42f,1.38f},
            {step*.65f,7.0f+lift*.7f,lz,1.55f,1.50f},
            {step*.3f,10.5f+bob,lz,1.80f,1.70f},
            {step*.1f,12.6f+bob,lz,2.00f,1.85f},
            {0,13.8f+bob,lz,2.10f,1.90f}};
        cloth(pants,6,JEANS,JEANS,0xffffffffu,x,z,angle);
        /* Zapatilla: suela de goma y empeine redondeado. */
        BodyRing sole[]={{step+.55f,.15f+lift,lz,2.45f,1.20f},{step+.55f,.60f+lift,lz,2.50f,1.25f}};
        cloth(sole,2,SIDEWALK,SIDEWALK,COLOR(238,234,219),x,z,angle);
        BodyRing shoe[]={{step+.55f,.60f+lift,lz,2.40f,1.18f},{step+.45f,1.25f+lift,lz,2.15f,1.10f},{step-.05f,2.15f+lift,lz,1.35f,.95f}};
        cloth(shoe,3,SLEEVE,SLEEVE,0xffffffffu,x,z,angle);
        /* Brazo: manga corta desde el hombro, antebrazo con ligera flexion de codo y mano. */
        float arm=-step*.45f,az=side*5.15f;
        BodyRing sleeve[]={{arm*.9f,17.4f+bob,az,1.05f,1.00f},{arm*.6f,19.6f+bob,az,1.15f,1.10f},{arm*.2f,21.4f+bob,az-side*.15f,1.25f,1.18f},{0,22.35f+bob,az-side*.6f,.85f,.75f},{0,22.6f+bob,az-side*.9f,.2f,.2f}};
        cloth(sleeve,5,SLEEVE,SLEEVE,0xffffffffu,x,z,angle);
        BodyRing forearm[]={{arm+.9f,13.5f+bob,az+side*.1f,.80f,.76f},{arm+.55f,15.5f+bob,az+side*.05f,.90f,.85f},{arm*.9f,17.6f+bob,az,.98f,.94f}};
        cloth(forearm,3,SKIN,SKIN,0xffffffffu,x,z,angle);
        BodyRing hand[]={{arm+1.05f,11.6f+bob,az+side*.1f,.55f,.48f},{arm+1.1f,12.4f+bob,az+side*.1f,.80f,.62f},{arm+.95f,13.7f+bob,az+side*.1f,.72f,.62f}};
        cloth(hand,3,SKIN,SKIN,0xffffffffu,x,z,angle);
    }
    /* Camiseta: bajo, cintura, pecho, hombros, cuello. */
    BodyRing shirt[]={{0,12.4f+bob,0,2.35f,3.70f},{0,13.0f+bob,0,2.40f,3.75f},{0,16.5f+bob,0,2.35f,3.55f},{0,20.0f+bob,0,2.50f,3.95f},{0,21.9f+bob,0,2.20f,4.20f},{0,22.75f+bob,0,1.35f,1.55f}};
    cloth(shirt,6,JACKET,JACKET_BACK,0xffffffffu,x,z,angle);
    BodyRing neck[]={{0,22.5f+bob,0,.95f,.90f},{.15f,24.5f+bob,0,.92f,.88f}};
    cloth(neck,2,SKIN,SKIN,0xffffffffu,x,z,angle);
    /* Cabeza: menton, mandibula, pomulos, craneo redondeado. */
    BodyRing head[]={{.35f,24.1f+bob,0,.85f,.85f},{.30f,24.8f+bob,0,1.50f,1.32f},{.12f,25.9f+bob,0,1.85f,1.65f},{0,27.3f+bob,0,1.90f,1.70f},{-.1f,28.5f+bob,0,1.55f,1.45f},{-.1f,29.05f+bob,0,.65f,.7f},{-.1f,29.2f+bob,0,.02f,.02f}};
    for(int j=0;j<6;j++)for(int k=0;k<12;k++){
        int front=k<3||k>=9;
        int mat=front?(j>=4?HAIR:FACE):(j>=2?HAIR:SKIN);
        float vt=1-(head[j+1].y-24.1f-bob)/5.1f,vb=1-(head[j].y-24.1f-bob)/5.1f;
        Vertex v[4]={body_vertex(head[j+1],k,vt,0xffffffffu,x,z,angle),body_vertex(head[j+1],k+1,vt,0xffffffffu,x,z,angle),body_vertex(head[j],k+1,vb,0xffffffffu,x,z,angle),body_vertex(head[j],k,vb,0xffffffffu,x,z,angle)};
        polygon(mat,v,4);
    }
    for(int side=-1;side<=1;side+=2){
        BodyRing ear[]={{0,25.5f+bob,side*1.62f,.18f,.16f},{0,26.1f+bob,side*1.78f,.34f,.22f},{-.1f,26.7f+bob,side*1.66f,.18f,.14f}};
        cloth(ear,3,SKIN,SKIN,0xffffffffu,x,z,angle);
    }
    Point nose=local(2.30f,26.2f+bob,0,x,z,angle);
    Point a=local(1.80f,26.95f+bob,0,x,z,angle),b=local(1.80f,25.95f+bob,-.32f,x,z,angle),c=local(1.80f,25.95f+bob,.32f,x,z,angle);
    Vertex nv[3]={{.5f,.5f,0xffffffffu,nose.x,nose.y,nose.z},{.5f,.5f,0xffffffffu,a.x,a.y,a.z},{.5f,.5f,0xffffffffu,b.x,b.y,b.z}};
    polygon(SKIN,nv,3);nv[1]=(Vertex){.5f,.5f,0xffffffffu,c.x,c.y,c.z};polygon(SKIN,nv,3);
}
static void landmarks(void){
    for(int i=0;i<30;i++){
        float x=view->hubs[i][0],z=view->hubs[i][1];if(!nearby(x,z,400))continue;
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
    /* 2 x 8888 frame + depth + 20 x 128-square 565 textures = 2,048,000 bytes. */
    textureBase=(void*)((uintptr_t)sceGeEdramGetAddr()+512*272*10);
    memcpy((void*)((uintptr_t)textureBase|0x40000000),textures3d_data,MAT_COUNT*128*128*2);
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
    view=s;overflow=0;memset(used,0,sizeof(used));camera(s);city();
    for(int i=0;i<s->carCount;i++)car(&s->cars[i]);
    if(!s->driving)person(s->x,s->z,s->angle,-1,s->moving);
    for(int i=0;i<s->personCount;i++)person(s->people[i].x,s->people[i].z,s->people[i].angle,s->people[i].style,1);
    landmarks();
#ifndef R3_HOST
    sceKernelDcacheWritebackAll();sceGuStart(GU_DIRECT,commands);
    sceGuDrawBufferList(GU_PSM_8888,(void*)((uintptr_t)fb&0x001fffff),512);
    sceGuClearColor(COLOR(155,189,192));sceGuClearDepth(0);sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
    sceGuEnable(GU_DEPTH_TEST);sceGuDepthMask(GU_FALSE);sceGuDisable(GU_BLEND);sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_TEXTURE_2D);sceGuTexMode(GU_PSM_5650,0,0,1);
    sceGuTexFunc(GU_TFX_MODULATE,GU_TCC_RGB);sceGuTexFilter(GU_LINEAR,GU_LINEAR);
    sceGuTexWrap(GU_REPEAT,GU_REPEAT);sceGuTexScale(1,1);sceGuTexOffset(0,0);sceGuShadeModel(GU_SMOOTH);
    sceGuEnable(GU_FOG);sceGuFog(300,690,COLOR(155,189,192));
    sceGumMatrixMode(GU_PROJECTION);sceGumLoadIdentity();sceGumPerspective(62,480.0f/272,2,760);
    ScePspFVector3 e={eye.x,eye.y,eye.z},t={target.x,target.y,target.z};
    ScePspFVector3 up={0,1,0};sceGumMatrixMode(GU_VIEW);sceGumLoadIdentity();sceGumLookAt(&e,&t,&up);
    sceGumMatrixMode(GU_MODEL);sceGumLoadIdentity();
    for(int m=0;m<MAT_COUNT;m++)if(used[m]){
        sceGuTexImage(0,128,128,128,(const char*)textureBase+m*128*128*2);
        sceGumDrawArray(GU_TRIANGLES,GU_TEXTURE_32BITF|GU_COLOR_8888|GU_VERTEX_32BITF|GU_TRANSFORM_3D,used[m],0,mesh[m]);
    }
    sceGuDisable(GU_FOG);sceGuFinish();sceGuSync(0,0);
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
