/* shapes.inc — Primitivas no cubicas (Claude, v2.7). Incluido desde render3d.c despues de city3d.inc.
 *
 * cylinder()  prisma regular de n lados (<=8) sobre el terreno (usa prism() de city26.inc: respeta fixedGround).
 * frustum()   tronco de cono/piramide entre dos radios (r1 puede ser 0 -> cono/piramide); copas, cupulas, tejados.
 * vault()     boveda de canon (mercados, naves).
 * wheel()     rueda: cilindro horizontal de 8 lados con tapas texturizadas (WHEEL) en coordenadas locales del coche.
 * hull()      carroceria por secciones: perfil lateral (x,y) con anchos, laterales achaflanados, materiales por tramo.
 * tree()      3 tipos: frondoso (tronco + 2 troncos de cono), palma (tronco inclinado + frondas), cipres (cono).
 * Todo pasa por quad()/polygon(): recorte, proyeccion geografica, modo rigido (coches) e iluminacion por normal.
 */
static void ring_points(float x,float z,float r,int n,float rot,float y,Point *out){
    for(int i=0;i<n;i++){float a=rot+i*2*PI/n;out[i]=point(x+cosf(a)*r,y,z+sinf(a)*r);}
}
static void chamfered_rect(CmParcel *p,float x0,float z0,float x1,float z1,float c){
    p->n=8;p->kind=0;
    p->x[0]=x0+c;p->z[0]=z0; p->x[1]=x1-c;p->z[1]=z0; p->x[2]=x1;p->z[2]=z0+c; p->x[3]=x1;p->z[3]=z1-c;
    p->x[4]=x1-c;p->z[4]=z1; p->x[5]=x0+c;p->z[5]=z1; p->x[6]=x0;p->z[6]=z1-c; p->x[7]=x0;p->z[7]=z0+c;
}
static void cylinder(float x,float z,float bottom,float r,float height,int n,float rot,int side,int top,uint32_t color){
    CmParcel p;p.n=n>8?8:n;p.kind=0;
    for(int i=0;i<p.n;i++){float a=rot+i*2*PI/p.n;p.x[i]=x+cosf(a)*r;p.z[i]=z+sinf(a)*r;}
    prism(&p,bottom,height,side,top,color);
}
/* Tronco de cono/piramide. Alturas relativas al terreno en (x,z) (o al fixedGround impuesto). */
static void frustum(float x,float z,float bottom,float r0,float r1,float height,int n,float rot,int mat,uint32_t color){
    if(n>8)n=8;
    float savedGround=fixedGround;
    if(geographic&&!rigid&&fixedGround<=-999999)fixedGround=geo_height(x,z);
    if(geographic&&!rigid&&!sphere_visible(x,z,(r0>r1?r0:r1)+height*.5f+2)){fixedGround=savedGround;return;}
    Point a[8],b[8];ring_points(x,z,r0,n,rot,bottom,a);ring_points(x,z,r1>0.3f?r1:0.001f,n,rot,bottom+height,b);
    float slope=(r0-r1)/(height>.1f?height:.1f);float ny=slope/sqrtf(1+slope*slope),nh=1/sqrtf(1+slope*slope);
    litAlready=1;uint32_t sc=emissive_color(mat,color);
    for(int i=0;i<n;i++){int j=(i+1)%n;float am=rot+(i+.5f)*2*PI/n;
        uint32_t c=lit_color(sc,cosf(am)*nh,ny,sinf(am)*nh);
        if(r1>0.3f){Vertex q[4]={{0,1,c,a[i].x,a[i].y,a[i].z},{1,1,c,a[j].x,a[j].y,a[j].z},{1,0,c,b[j].x,b[j].y,b[j].z},{0,0,c,b[i].x,b[i].y,b[i].z}};polygon(mat,q,4);}
        else{Vertex t[3]={{0,1,c,a[i].x,a[i].y,a[i].z},{1,1,c,a[j].x,a[j].y,a[j].z},{.5f,0,c,b[0].x,b[0].y,b[0].z}};polygon(mat,t,3);}
    }
    if(r1>0.3f){Vertex tv[8];uint32_t tc=lit_color(color,0,1,0);for(int i=0;i<n;i++){Vertex v={(b[i].x-x)/(2*r1)+.5f,(b[i].z-z)/(2*r1)+.5f,tc,b[i].x,b[i].y,b[i].z};tv[i]=v;}polygon(mat,tv,n);}
    litAlready=0;fixedGround=savedGround;
}
/* Boveda de canon a lo largo del eje x: base a 'bottom', anchura W (eje z), altura de flecha h, 5 segmentos. */
static void vault(float x,float z,float L,float W,float bottom,float h,int mat,uint32_t color){
    const int segs=5;float savedGround=fixedGround;
    if(geographic&&!rigid&&fixedGround<=-999999)fixedGround=geo_height(x,z);
    litAlready=1;
    for(int i=0;i<segs;i++){float a0=PI*i/segs,a1=PI*(i+1)/segs;
        float z0=z+cosf(a0)*W*.5f,y0=bottom+sinf(a0)*h,z1=z+cosf(a1)*W*.5f,y1=bottom+sinf(a1)*h;
        float am=(a0+a1)*.5f;uint32_t c=lit_color(color,0,sinf(am),cosf(am));
        Vertex q[4]={{0,0,c,x-L*.5f,y0,z0},{L/40,0,c,x+L*.5f,y0,z0},{L/40,1,c,x+L*.5f,y1,z1},{0,1,c,x-L*.5f,y1,z1}};polygon(mat,q,4);
    }
    /* timpanos */
    for(int e=-1;e<=1;e+=2){Vertex t[7];float xx=x+e*L*.5f;uint32_t c=lit_color(color,(float)e,0,0);
        for(int i=0;i<=segs;i++){float a=PI*i/segs;Vertex v={(cosf(a)+1)*.5f,1-sinf(a),c,xx,bottom+sinf(a)*h,z+cosf(a)*W*.5f};t[i]=v;}
        polygon(mat,t,segs+1);}
    litAlready=0;fixedGround=savedGround;
}
/* Rueda en coordenadas locales del coche (eje z local = ancho). Caps con textura WHEEL, banda oscura. */
static void wheel(float cx,float cz,float angle,float lx,float ly,float lz,float r,float w,float spin){
    const int n=8;Point in[8],out[8];
    for(int i=0;i<n;i++){float a=spin+i*2*PI/n;float px=lx+cosf(a)*r,py=ly+sinf(a)*r;in[i]=local(px,py,lz-w*.5f,cx,cz,angle);out[i]=local(px,py,lz+w*.5f,cx,cz,angle);}
    uint32_t rubber=day_scale(COLOR(38,38,40));
    for(int i=0;i<n;i++){int j=(i+1)%n;Vertex q[4]={{0,0,rubber,in[i].x,in[i].y,in[i].z},{1,0,rubber,in[j].x,in[j].y,in[j].z},{1,1,rubber,out[j].x,out[j].y,out[j].z},{0,1,rubber,out[i].x,out[i].y,out[i].z}};litAlready=1;polygon(CAR_PAINT,q,4);litAlready=0;}
    Vertex cap[8];uint32_t white=day_scale(0xffffffffu);
    for(int i=0;i<n;i++){float a=spin+i*2*PI/n;Vertex v={(cosf(a)+1)*.5f,(sinf(a)+1)*.5f,white,in[i].x,in[i].y,in[i].z};cap[i]=v;}litAlready=1;polygon(WHEEL,cap,n);
    for(int i=0;i<n;i++){float a=spin+i*2*PI/n;Vertex v={(cosf(a)+1)*.5f,(sinf(a)+1)*.5f,white,out[i].x,out[i].y,out[i].z};cap[i]=v;}polygon(WHEEL,cap,n);litAlready=0;
}
/* Carroceria por secciones. prof[i] = {x, ytop, halfwidth}; mats[i] = material del tramo i (techo) ; los laterales
   usan CAR_SIDE en tramos de cabina y CAR_PAINT en el resto. floor = altura del bajo. */
typedef struct {float x,y,w;} HullSec;
static void hull(float cx,float cz,float angle,const HullSec *s,int n,const unsigned char *topMat,float floor,uint32_t paint){
    uint32_t glass=0xffffffffu;
    for(int i=0;i<n-1;i++){
        const HullSec *a=&s[i],*b=&s[i+1];int tm=topMat[i];int cabin=tm>=200;if(cabin)tm-=200; /* >=200: tramo de cabina (laterales con puertas/ventanas) */
        float dx=b->x-a->x,dy=b->y-a->y,len=sqrtf(dx*dx+dy*dy);float nx=-dy/len,ny=dx/len; /* normal del techo en plano x-y local */
        Point tl0=local(a->x,a->y,-a->w*.86f,cx,cz,angle),tr0=local(a->x,a->y,a->w*.86f,cx,cz,angle),tl1=local(b->x,b->y,-b->w*.86f,cx,cz,angle),tr1=local(b->x,b->y,b->w*.86f,cx,cz,angle);
        Point bl0=local(a->x,floor,-a->w,cx,cz,angle),br0=local(a->x,floor,a->w,cx,cz,angle),bl1=local(b->x,floor,-b->w,cx,cz,angle),br1=local(b->x,floor,b->w,cx,cz,angle);
        float ca=cosf(angle),sa=sinf(angle);
        uint32_t tc=lit_color(tm==GLASS?glass:paint,nx*ca,ny,nx*sa);
        litAlready=1;
        Vertex top[4]={{0,0,tc,tl0.x,tl0.y,tl0.z},{1,0,tc,tr0.x,tr0.y,tr0.z},{1,1,tc,tr1.x,tr1.y,tr1.z},{0,1,tc,tl1.x,tl1.y,tl1.z}};polygon(tm,top,4);
        int sideMat=cabin?CAR_SIDE:CAR_PAINT;
        uint32_t lc=lit_color(paint,-sa,.3f,ca),rc=lit_color(paint,sa,.3f,-ca);
        float u0=0,u1=1;
        Vertex left[4]={{u0,0,lc,tl0.x,tl0.y,tl0.z},{u1,0,lc,tl1.x,tl1.y,tl1.z},{u1,1,lc,bl1.x,bl1.y,bl1.z},{u0,1,lc,bl0.x,bl0.y,bl0.z}};polygon(sideMat,left,4);
        Vertex right[4]={{u1,0,rc,tr1.x,tr1.y,tr1.z},{u0,0,rc,tr0.x,tr0.y,tr0.z},{u0,1,rc,br0.x,br0.y,br0.z},{u1,1,rc,br1.x,br1.y,br1.z}};polygon(sideMat,right,4);
        litAlready=0;
    }
    /* tapas delantera y trasera */
    for(int e=0;e<2;e++){const HullSec *a=e?&s[n-1]:&s[0];float ca=cosf(angle),sa=sinf(angle);uint32_t c=lit_color(paint,e?ca:-ca,.2f,e?sa:-sa);
        Point p0=local(a->x,a->y,-a->w*.86f,cx,cz,angle),p1=local(a->x,a->y,a->w*.86f,cx,cz,angle),p2=local(a->x,floor,a->w,cx,cz,angle),p3=local(a->x,floor,-a->w,cx,cz,angle);
        litAlready=1;Vertex q[4]={{0,0,c,p0.x,p0.y,p0.z},{1,0,c,p1.x,p1.y,p1.z},{1,1,c,p2.x,p2.y,p2.z},{0,1,c,p3.x,p3.y,p3.z}};polygon(CAR_PAINT,q,4);litAlready=0;}
}
/* Arboles: tipo por hash de posicion (0 frondoso, 1 palma, 2 cipres). */
static void tree_shape(float x,float z,int kind,int simple){
    unsigned h=(unsigned)(x*7.3f)*2654435761u^(unsigned)(z*3.1f)*40503u;float rot=(h&255)/255.f*6.28f;float s=.85f+((h>>8)&127)/127.f*.4f;
    if(kind==1){ /* palma: tronco en dos tramos ligeramente inclinado, frondas colgantes */
        float lean=((h>>3)&7)*.5f-1.75f;
        cylinder(x,z,0,1.3f*s,12*s,5,rot,BARK,BARK,COLOR(170,150,120));
        cylinder(x+lean,z+lean*.5f,12*s,1.1f*s,12*s,5,rot,BARK,BARK,COLOR(180,160,130));
        if(simple){frustum(x+lean,z+lean*.5f,24*s,7*s,0,4*s,6,rot,LEAVES,COLOR(120,190,90));return;}
        float top=24*s;Point c=point(x+lean,top,z+lean*.5f);
        litAlready=1;uint32_t lc=lit_color(COLOR(110,190,90),0,1,0);
        for(int k=0;k<7;k++){float a=rot+k*2*PI/7;float ex=cosf(a),ez=sinf(a);
            Point tip=point(c.x+ex*11*s,top-4*s,c.z+ez*11*s),mid=point(c.x+ex*5*s,top+2.2f*s,c.z+ez*5*s);
            Point l=point(mid.x-ez*2.4f*s,mid.y,mid.z+ex*2.4f*s),r=point(mid.x+ez*2.4f*s,mid.y,mid.z-ex*2.4f*s);
            Vertex f1[3]={{.5f,0,lc,c.x,c.y+1,c.z},{0,.5f,lc,l.x,l.y,l.z},{.5f,1,lc,tip.x,tip.y,tip.z}};polygon(LEAVES,f1,3);
            Vertex f2[3]={{.5f,0,lc,c.x,c.y+1,c.z},{.5f,1,lc,tip.x,tip.y,tip.z},{1,.5f,lc,r.x,r.y,r.z}};polygon(LEAVES,f2,3);}
        litAlready=0;
        box(c.x,c.z,top-2*s,2.5f*s,2.5f*s,2*s,rot,ROOF,ROOF,COLOR(150,110,60)); /* cocos */
        return;
    }
    if(kind==2){ /* cipres / pino */
        cylinder(x,z,0,1.2f*s,7*s,5,rot,BARK,BARK,COLOR(120,95,70));
        frustum(x,z,5*s,6*s,3.2f*s,9*s,6,rot,LEAVES,COLOR(70,130,75));
        if(!simple)frustum(x,z,13*s,4*s,0,10*s,6,rot+.3f,LEAVES,COLOR(90,150,85));else frustum(x,z,13*s,3.2f*s,0,9*s,6,rot,LEAVES,COLOR(90,150,85));
        return;
    }
    /* frondoso */
    cylinder(x,z,0,1.6f*s,12*s,simple?4:5,rot,BARK,BARK,COLOR(150,125,95));
    frustum(x,z,10*s,9.5f*s,6.5f*s,8*s,simple?5:6,rot,LEAVES,COLOR(140,200,120));
    if(!simple){frustum(x,z,17.5f*s,7*s,2.5f*s,6*s,6,rot+.4f,LEAVES,COLOR(160,215,130));frustum(x,z,23*s,3*s,0,3*s,4,rot,LEAVES,COLOR(175,225,140));}
    else frustum(x,z,17.5f*s,6.5f*s,0,7*s,5,rot,LEAVES,COLOR(160,215,130));
    if(!simple)frustum(x,z,8*s,3.5f*s,9.5f*s,2.5f*s,6,rot,LEAVES,COLOR(120,180,105)); /* parte baja de la copa */
}
