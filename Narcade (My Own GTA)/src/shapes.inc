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
/* Three horizontal contours: tucked sill, full shoulder, tapered cabin. */
static float car_width_at(float width,float top,float floor,float y){
    float belt=fminf(top,floor+6.0f);
    if(y<=belt)return width*(.90f+.10f*fmaxf(0,(y-floor)/fmaxf(belt-floor,.01f)));
    return width*(1-.14f*fminf(1,(y-belt)/fmaxf(top-belt,.01f)));
}
static float car_arch_bottom(float x,float floor,float wheelRadius){
    float low=floor;if(wheelRadius<=0)return low;
    for(int e=-1;e<=1;e+=2){float d=x-e*11.f,r=wheelRadius+.7f;
        if(fabsf(d)<r)low=fmaxf(low,wheelRadius+sqrtf(fmaxf(0,r*r-d*d)));}
    return low;
}
/* Immutable local body patches: 39KB, no heap allocation in the frame loop.
   Each of six types has close and medium detail; lights/wheels remain dynamic. */
typedef struct{float u,v,x,y,z;unsigned char tone;}HullVertex;
typedef struct{HullVertex v[4];}HullPatch;
typedef struct{const HullSec *profile;int close,count;HullPatch patch[40];}HullCache;
static HullCache hullCache[12];
static int hullWarmOnly;
static void hull_sides(float cx,float cz,float angle,const HullSec *s,int n,float floor,uint32_t paint,float wheelRadius){
    HullCache *cache=0;int close=wheelRadius>0;
    for(int k=0;k<12;k++)if(hullCache[k].profile==s&&hullCache[k].close==close){cache=&hullCache[k];break;}
    if(!cache){
        for(int k=0;k<12;k++)if(!hullCache[k].profile){cache=&hullCache[k];break;}
        if(!cache)return; /* Only the six fixed internal profiles are supported. */
        cache->profile=s;cache->close=close;cache->count=0;
        for(int i=0;i<n-1;i++){const HullSec *a=&s[i],*b=&s[i+1];float dx=b->x-a->x,dy=b->y-a->y;
        float cuts[16];int nc=0;cuts[nc++]=a->x;
        if(wheelRadius>0)for(int e=-1;e<=1;e+=2)for(int k=0;k<=4;k++){
            float q=e*11.f+(wheelRadius+.7f)*(k*.5f-1);
            if(q>a->x+.001f&&q<b->x-.001f){int j=nc;while(j>0&&cuts[j-1]>q){cuts[j]=cuts[j-1];j--;}cuts[j]=q;nc++;}}
        cuts[nc++]=b->x;
        for(int k=0;k<nc-1;k++){
            float xx[2]={cuts[k],cuts[k+1]},topY[2],low[2],belt[2],ww[2];
            for(int j=0;j<2;j++){float t=(xx[j]-a->x)/fmaxf(.001f,dx);
                topY[j]=a->y+dy*t;ww[j]=a->w+(b->w-a->w)*t;
                low[j]=fminf(topY[j],car_arch_bottom(xx[j],floor,wheelRadius));belt[j]=fminf(topY[j],floor+6);}
            int bands=wheelRadius>0?2:1;
            for(int band=0;band<bands;band++){
                float lo0=band?fmaxf(low[0],belt[0]):low[0],lo1=band?fmaxf(low[1],belt[1]):low[1];
                float hi0=band?topY[0]:fmaxf(low[0],belt[0]),hi1=band?topY[1]:fmaxf(low[1],belt[1]);
                if(bands==1){lo0=low[0];lo1=low[1];hi0=topY[0];hi1=topY[1];}
                if(hi0-lo0<.001f&&hi1-lo1<.001f)continue;
                Point p[4]={point(xx[0],hi0,car_width_at(ww[0],topY[0],floor,hi0)),
                    point(xx[1],hi1,car_width_at(ww[1],topY[1],floor,hi1)),
                    point(xx[1],lo1,car_width_at(ww[1],topY[1],floor,lo1)),
                    point(xx[0],lo0,car_width_at(ww[0],topY[0],floor,lo0))};
                if(cache->count<40){HullPatch *patch=&cache->patch[cache->count++];
                    for(int j=0;j<4;j++)patch->v[j]=(HullVertex){(xx[j==1||j==2]+20)/40.f,(20-p[j].y)/20.f,p[j].x,p[j].y,p[j].z,(unsigned char)fminf(3,fmaxf(0,(p[j].y-floor)*.3f))};}
            }
        }
        }
    }
    if(hullWarmOnly)return;
    float ca=cosf(angle),sa=sinf(angle);
    uint32_t colors[2][4],base[2]={lit_color(paint,-sa,.3f,ca),lit_color(paint,sa,.3f,-ca)};
    for(int side=0;side<2;side++)for(int tone=0;tone<4;tone++)colors[side][tone]=shade(base[side],.82f+.06f*tone);
    for(int k=0;k<cache->count;k++)for(int side=-1;side<=1;side+=2){Vertex v[4];
        for(int j=0;j<4;j++){HullVertex *h=&cache->patch[k].v[j];Point p=local(h->x,h->y,side*h->z,cx,cz,angle);
            v[j]=(Vertex){h->u,h->v,colors[side>0][h->tone],p.x,p.y,p.z};}
        polygon(CAR_SIDE,v,4);
    }
}
static void hull(float cx,float cz,float angle,const HullSec *s,int n,const unsigned char *topMat,float floor,uint32_t paint,float wheelRadius){
    uint32_t glass=0xffffffffu;
    for(int i=0;i<n-1;i++){
        const HullSec *a=&s[i],*b=&s[i+1];int tm=topMat[i];int cabin=tm>=200;if(cabin)tm-=200; /* >=200: tramo de cabina (laterales con puertas/ventanas) */
        float dx=b->x-a->x,dy=b->y-a->y,len=sqrtf(dx*dx+dy*dy);float nx=-dy/len,ny=dx/len; /* normal del techo en plano x-y local */
        Point tl0=local(a->x,a->y,-a->w*.86f,cx,cz,angle),tr0=local(a->x,a->y,a->w*.86f,cx,cz,angle),tl1=local(b->x,b->y,-b->w*.86f,cx,cz,angle),tr1=local(b->x,b->y,b->w*.86f,cx,cz,angle);
        float ca=cosf(angle),sa=sinf(angle);
        uint32_t tc=lit_color(tm==GLASS?glass:paint,nx*ca,ny,nx*sa);
        litAlready=1;
        Vertex top[4]={{0,0,tc,tl0.x,tl0.y,tl0.z},{1,0,tc,tr0.x,tr0.y,tr0.z},{1,1,tc,tr1.x,tr1.y,tr1.z},{0,1,tc,tl1.x,tl1.y,tl1.z}};polygon(tm,top,4);
        /* Metallic reflections share car-local UVs across all body sections.
           Vertical UV follows actual height, so the shoulder and sill cannot
           bend when the bonnet or roof changes height. */
        /* Shared longitudinal UVs keep paint detail aligned across doors,
           bonnet and rear quarter instead of restarting each section. */
        /* Open wheel wells are absent geometry, not painted dark circles.
           Split only at arch knots, preserving shared longitudinal UVs. */
        if(i==0)hull_sides(cx,cz,angle,s,n,floor,paint,wheelRadius);
        litAlready=0;
        for(int side=-1;side<=1;side+=2){
            float y0=floor+2.4f,y1=y0+.45f;
            float a0=car_width_at(a->w,a->y,floor,y0)+.08f;
            float b0=car_width_at(b->w,b->y,floor,y0)+.08f;
            float a1=car_width_at(a->w,a->y,floor,y1)+.08f;
            float b1=car_width_at(b->w,b->y,floor,y1)+.08f;
            quad(METAL,local(a->x,y1,side*a1,cx,cz,angle),local(b->x,y1,side*b1,cx,cz,angle),
                local(b->x,y0,side*b0,cx,cz,angle),local(a->x,y0,side*a0,cx,cz,angle),shade(paint,.48f),1,1);
        }
        if(cabin&&(a->y>12||b->y>12))for(int side=-1;side<=1;side+=2){
            float ya=fminf(a->y-.35f,11.5f),yb=fminf(b->y-.35f,11.5f);
            float wa=car_width_at(a->w,a->y,floor,ya)+.13f;
            float wb=car_width_at(b->w,b->y,floor,yb)+.13f;
            Point p0=local(a->x,a->y-.15f,side*(a->w*.86f+.13f),cx,cz,angle);
            Point p1=local(b->x,b->y-.15f,side*(b->w*.86f+.13f),cx,cz,angle);
            Point p2=local(b->x,yb,side*wb,cx,cz,angle);
            Point p3=local(a->x,ya,side*wa,cx,cz,angle);
            quad(METAL,p0,p1,p2,p3,lit_color(COLOR(35,45,49),side*-sa,.2f,side*ca),1,1);
            float inset=fminf(.42f,(b->x-a->x)*.15f);
            Point g0=local(a->x+inset,a->y-.48f,side*(a->w*.86f+.22f),cx,cz,angle);
            Point g1=local(b->x-inset,b->y-.48f,side*(b->w*.86f+.22f),cx,cz,angle);
            Point g2=local(b->x-inset,yb+.22f,side*(wb+.10f),cx,cz,angle);
            Point g3=local(a->x+inset,ya+.22f,side*(wa+.10f),cx,cz,angle);
            quad(GLASS,g0,g1,g2,g3,lit_color(COLOR(87,120,139),side*-sa,.2f,side*ca),1,1);
            /* Slim B-pillar on the long cabin panels, following the taper of
               this actual body instead of a painted line across the texture. */
            if(dx>7&&a->y>13&&b->y>13){
                float mx=a->x+dx*.53f,top=a->y+(b->y-a->y)*.53f;
                float mw=a->w+(b->w-a->w)*.53f;
                float low=fminf(top-.35f,11.55f);
                Point pa=local(mx-.35f,top-.43f,side*(mw*.86f+.39f),cx,cz,angle);
                Point pb=local(mx+.35f,top-.43f,side*(mw*.86f+.39f),cx,cz,angle);
                Point pc=local(mx+.35f,low,side*(car_width_at(mw,top,floor,low)+.46f),cx,cz,angle);
                Point pd=local(mx-.35f,low,side*(car_width_at(mw,top,floor,low)+.46f),cx,cz,angle);
                quad(METAL,pa,pb,pc,pd,lit_color(COLOR(29,36,40),side*-sa,.2f,side*ca),1,1);
            }
        }
    }
    /* tapas delantera y trasera */
    for(int e=0;e<2;e++){const HullSec *a=e?&s[n-1]:&s[0];float ca=cosf(angle),sa=sinf(angle);uint32_t c=lit_color(paint,e?ca:-ca,.2f,e?sa:-sa);
        Point p0=local(a->x,a->y,-a->w*.86f,cx,cz,angle),p1=local(a->x,a->y,a->w*.86f,cx,cz,angle),p2=local(a->x,floor,a->w*.9f,cx,cz,angle),p3=local(a->x,floor,-a->w*.9f,cx,cz,angle);
        litAlready=1;Vertex q[4]={{0,0,c,p0.x,p0.y,p0.z},{1,0,c,p1.x,p1.y,p1.z},{1,1,c,p2.x,p2.y,p2.z},{0,1,c,p3.x,p3.y,p3.z}};polygon(CAR_PAINT,q,4);litAlready=0;}
}
/* Point on the actual sloped side of a car body, at car-local x and height.
   Trim is attached to this surface instead of a fixed-width box silhouette. */
static Point car_skin(float cx,float cz,float angle,const HullSec *s,int n,float floor,float x,float y,int side,float offset){
    int i=0;while(i<n-2&&x>s[i+1].x)i++;
    float t=(x-s[i].x)/fmaxf(.01f,s[i+1].x-s[i].x);t=fmaxf(0,fminf(1,t));
    float top=s[i].y+(s[i+1].y-s[i].y)*t;
    float width=s[i].w+(s[i+1].w-s[i].w)*t;
    return local(x,y,side*(car_width_at(width,top,floor,y)+offset),cx,cz,angle);
}
static void car_seam(float cx,float cz,float angle,const HullSec *s,int n,float floor,float x,int side){
    float low=floor+2.4f,high=fminf(10.8f,s[n-1].y+2.8f);
    Point a=car_skin(cx,cz,angle,s,n,floor,x-.10f,low,side,.18f);
    Point b=car_skin(cx,cz,angle,s,n,floor,x+.10f,low,side,.18f);
    Point c=car_skin(cx,cz,angle,s,n,floor,x+.10f,high,side,.18f);
    Point d=car_skin(cx,cz,angle,s,n,floor,x-.10f,high,side,.18f);
    quad(METAL,a,b,c,d,day_scale(COLOR(47,53,56)),1,1);
}
/* Arboles: tipo por hash de posicion (0 frondoso, 1 palma, 2 cipres). */
static void foliage_lobe(float x,float z,float bottom,float radius,float height,unsigned seed,uint32_t tint){
    /* Alpha-tested branch sprays keep sky visible inside the crown. */
    /* Crossed sprays have open space inside the crown. No closed green shell.
       Three cards replace twelve lobe faces, reducing the foliage vertex cost. */
    float rot=(seed&255)*(2*PI/255);int saved=litAlready;litAlready=1;
    for(int k=0;k<3;k++){
        float a=rot+k*PI/3,ex=cosf(a)*radius,ez=sinf(a)*radius;
        int light=200+((tint>>8)&255)/10+(seed&7);
        uint32_t c=lit_color(COLOR(light,light+8,light-5),cosf(a)*.35f,.8f,sinf(a)*.35f);
        quad(LEAVES,point(x-ex,bottom,z-ez),point(x+ex,bottom+.6f,z+ez),
            point(x+ex*.82f,bottom+height,z+ez*.82f),point(x-ex*.85f,bottom+height-.5f,z-ez*.85f),c,1,1);
    }
    litAlready=saved;
}

static void tree_shape(float x,float z,int kind,int simple){
    unsigned h=(unsigned)(x*7.3f)*2654435761u^(unsigned)(z*3.1f)*40503u;float rot=(h&255)/255.f*6.28f;float s=.85f+((h>>8)&127)/127.f*.4f;
    if(kind==1){ /* palma esbelta: copa abierta incluso en el LOD lejano */
        float lean=((h>>3)&7)*.5f-1.75f;
        cylinder(x,z,0,.70f*s,10*s,5,rot,BARK,BARK,COLOR(139,114,83));
        cylinder(x+lean*.45f,z+lean*.22f,10*s,.59f*s,9*s,5,rot,BARK,BARK,COLOR(150,124,88));
        cylinder(x+lean,z+lean*.5f,19*s,.44f*s,5*s,5,rot,BARK,BARK,COLOR(158,132,96));
        float top=24*s;Point c=point(x+lean,top,z+lean*.5f);
        litAlready=1;uint32_t lc=lit_color(COLOR(110,190,90),0,1,0);
        int fronds=simple?6:9;
        for(int k=0;k<fronds;k++){float a=rot+k*2*PI/fronds;float ex=cosf(a),ez=sinf(a);
            float length=(k&1?9.5f:11.5f)*s;
            Point tip=point(c.x+ex*length,top-(simple?3:5)*s,c.z+ez*length),mid=point(c.x+ex*length*.52f,top+1.8f*s,c.z+ez*length*.52f);
            Point l=point(mid.x-ez*1.9f*s,mid.y,mid.z+ex*1.9f*s),r=point(mid.x+ez*1.9f*s,mid.y,mid.z-ex*1.9f*s);
            Vertex f1[3]={{.5f,0,lc,c.x,c.y+1,c.z},{0,.5f,lc,l.x,l.y,l.z},{.5f,1,lc,tip.x,tip.y,tip.z}};polygon(LEAVES,f1,3);
            Vertex f2[3]={{.5f,0,lc,c.x,c.y+1,c.z},{.5f,1,lc,tip.x,tip.y,tip.z},{1,.5f,lc,r.x,r.y,r.z}};polygon(LEAVES,f2,3);}
        litAlready=0;
        /* The former ROOF cube sat inside the BARK trunk cap and flickered.
           The tapered trunk now meets the open fronds without an extra cap. */
        return;
    }
    if(kind==2){ /* cipres / pino */
        cylinder(x,z,0,1.2f*s,7*s,5,rot,BARK,BARK,COLOR(120,95,70));
        frustum(x,z,5*s,6*s,3.2f*s,9*s,6,rot,LEAVES,COLOR(70,130,75));
        if(!simple)frustum(x,z,13*s,4*s,0,10*s,6,rot+.3f,LEAVES,COLOR(90,150,85));else frustum(x,z,13*s,3.2f*s,0,9*s,6,rot,LEAVES,COLOR(90,150,85));
        return;
    }
    /* Frondoso: tronco expuesto, ramas diagonales y copa irregular formada
       por grupos de hojas, no por un unico cono verde. */
    cylinder(x,z,0,1.55f*s,15*s,simple?4:6,rot,BARK,BARK,COLOR(135,107,77));
    if(simple){frustum(x,z,11*s,8*s,6*s,9*s,6,rot,LEAVES,COLOR(92,146,73));frustum(x,z,19*s,6*s,1*s,7*s,6,rot,LEAVES,COLOR(116,164,88));return;}
    for(int k=0;k<5;k++){
        float a=rot+k*2*PI/5,ex=cosf(a),ez=sinf(a),r=(6+(k&1)*2)*s;
        Point base=point(x+ex*.8f*s,(10+(k%2)*2)*s,z+ez*.8f*s);
        Point tip=point(x+ex*r,(17+(k%3)*2)*s,z+ez*r);
        uint32_t bark=lit_color(COLOR(105,79,55),ex,.2f,ez);
        quad(BARK,point(base.x-ez*s,base.y,base.z+ex*s),point(base.x+ez*s,base.y,base.z-ex*s),
             point(tip.x+ez*.35f*s,tip.y,tip.z-ex*.35f*s),point(tip.x-ez*.35f*s,tip.y,tip.z+ex*.35f*s),bark,1,1);
        quad(BARK,point(base.x,base.y-s,base.z),point(base.x,base.y+s,base.z),
             point(tip.x,tip.y+.35f*s,tip.z),point(tip.x,tip.y-.35f*s,tip.z),bark,1,1);
        uint32_t leaf=k&1?COLOR(87,150,77):COLOR(114,174,91);
        foliage_lobe(tip.x,tip.z,tip.y-4*s,5.6f*s,10*s,h+(unsigned)k*17u,leaf);
    }
    foliage_lobe(x,z,13.5f*s,7.4f*s,12*s,h^0x537u,COLOR(91,155,75));
    foliage_lobe(x+1.1f*s,z-.8f*s,20*s,5.5f*s,8*s,h^0x822u,COLOR(119,177,92));
}
