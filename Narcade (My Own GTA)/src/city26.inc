/* city26.inc — Trazado irregular real (Claude, v2.6). Se incluye desde render3d.c despues de city3d.inc.
 *
 * La huella la define src/citymap.h (misma fuente que solid() en game.c): manzanas fusionadas (dobles), partidas
 * por una calle central, triangulares/trapezoidales junto a la Av. Oriental, tuneles bajo edificios puente,
 * Estadio, Plaza Botero, Pueblito Paisa, viaducto y estaciones del Metro y Metrocable.
 * Reutiliza los helpers de city3d.inc (casas, tejados, jardines, puentes del rio, pasarelas) y los tipos de manzana
 * por barrio; los rellenos se adaptan al tamano real de cada parcela.
 */
static void cylinder(float x,float z,float bottom,float r,float height,int n,float rot,int side,int top,uint32_t color);
static void frustum(float x,float z,float bottom,float r0,float r1,float height,int n,float rot,int mat,uint32_t color);
static void vault(float x,float z,float L,float W,float bottom,float h,int mat,uint32_t color);
static void chamfered_rect(CmParcel *p,float x0,float z0,float x1,float z1,float c);
static void bench(float x,float z,float angle);static void lamp_post(float x,float z,float angle);
#define parcel_centroid cm_centroid
#define parcel_shrunk cm_shrunk
/* Prisma vertical sobre un poligono convexo: lados iluminados por su normal, tapa iluminada como techo. */
static void prism(const CmParcel *p,float bottom,float height,int side,int top,uint32_t color){
    if(p->n<3)return;
    float savedGround=fixedGround;float cx,cz;parcel_centroid(p,&cx,&cz);float h=bottom+height;
    if(geographic&&!rigid&&fixedGround>-999999){if(bottom==0){float low=geo_height(cx,cz);for(int i=0;i<p->n;i++){float hh=geo_height(p->x[i],p->z[i]);if(hh<low)low=hh;}bottom=low-fixedGround-1;}} /* nivel impuesto; cimiento hasta el terreno mas bajo */
    else if(geographic&&!rigid){
        float low=geo_height(cx,cz),high=low;
        for(int i=0;i<p->n;i++){float hh=geo_height(p->x[i],p->z[i]);if(hh<low)low=hh;if(hh>high)high=hh;}
        /* muestreo interior: en ladera el terreno puede subir dentro de la parcela mas que en sus esquinas */
        for(int i=0;i<p->n;i++){float hh=geo_height((p->x[i]+cx)*.5f,(p->z[i]+cz)*.5f);if(hh>high)high=hh;}
        fixedGround=high;if(bottom==0)bottom=low-high-1; /* cimiento hasta el terreno mas bajo; la tapa no cambia */
    }
    {float rr=0;for(int i=0;i<p->n;i++){float dx=p->x[i]-cx,dz=p->z[i]-cz,d=dx*dx+dz*dz;if(d>rr)rr=d;}if(geographic&&!rigid&&!volume_visible(cx,cz,bottom,h,sqrtf(rr))){fixedGround=savedGround;return;}}
    uint32_t sc=emissive_color(side,color);
    litAlready=1;
    /* Solo las parcelas que cruzan una franja de transicion del valle (z local <86 de una
       fila) se deforman en pantalla; el resto se dibuja como siempre (sin coste extra). */
    int warped=0;{float z0=1e9f,z1=-1e9f;for(int i=0;i<p->n;i++){z0=fminf(z0,p->z[i]);z1=fmaxf(z1,p->z[i]);}
        warped=geographic&&!rigid&&floorf((z0-86.f)/320.f)!=floorf((z1-86.f)/320.f);}
    for(int i=0;i<p->n;i++){int j=(i+1)%p->n;
        float ex=p->x[j]-p->x[i],ez=p->z[j]-p->z[i],len=sqrtf(ex*ex+ez*ez);if(len<.5f)continue;
        float nx=ez/len,nz=-ex/len;float mx=(p->x[i]+p->x[j])*.5f-cx,mz=(p->z[i]+p->z[j])*.5f-cz;if(mx*nx+mz*nz<0){nx=-nx;nz=-nz;}
        /* v2.26 (Claude): en el valle deformado un lado largo (manzanas dobles de 500) se
           dibujaba recto en pantalla mientras la colision seguia la curva del mapa: quedaba
           un triangulo de pared invisible. Los lados largos se trocean cada ~40 unidades. */
        int parts=(warped&&len>60)?(int)ceilf(len/40):1;
        for(int k=0;k<parts;k++){
            float t0=(float)k/parts,t1=(float)(k+1)/parts;
            float x0=p->x[i]+ex*t0,z0=p->z[i]+ez*t0,x1=p->x[i]+ex*t1,z1=p->z[i]+ez*t1;
            Point a=point(x0,h,z0),b=point(x1,h,z1),c=point(x1,bottom,z1),d=point(x0,bottom,z0);
            float u=len/parts/54;if(u<1)u=1;float v=height/24;if(v<1)v=1;
            if(height>14)quad2(side,a,b,c,d,lit_color(sc,nx,0,nz),u,v);else quad(side,a,b,c,d,lit_color(sc,nx,0,nz),u,v);
            if(height>18)facade_lights(d,c,height,nx,nz,side);
        }
    }
    Vertex tv[8];uint32_t tc=lit_color(color,0,1,0);
    float spanZ=0,spanX=0;for(int i=0;i<p->n;i++){spanZ=fmaxf(spanZ,fabsf(p->z[i]-cz));spanX=fmaxf(spanX,fabsf(p->x[i]-cx));}
    if(warped&&cm_is_rect(p)&&(spanZ>60||spanX>60)){
        /* tapa rectangular en franjas para que siga la misma curva que la colision */
        float x0=p->x[0],x1=p->x[1],z0=p->z[0],z1=p->z[2];
        int nzp=(int)ceilf((z1-z0)/40),nxp=(int)ceilf((x1-x0)/40);if(nzp<1)nzp=1;if(nxp<1)nxp=1;
        for(int a=0;a<nzp;a++)for(int b=0;b<nxp;b++){
            float za=z0+(z1-z0)*a/nzp,zb=z0+(z1-z0)*(a+1)/nzp,xa=x0+(x1-x0)*b/nxp,xb=x0+(x1-x0)*(b+1)/nxp;
            Vertex q[4]={{(xa-cx)/90,(za-cz)/90,tc,xa,h,za},{(xb-cx)/90,(za-cz)/90,tc,xb,h,za},{(xb-cx)/90,(zb-cz)/90,tc,xb,h,zb},{(xa-cx)/90,(zb-cz)/90,tc,xa,h,zb}};
            polygon(top,q,4);
        }
    }else{
    for(int i=0;i<p->n;i++){Vertex vv={(p->x[i]-cx)/90,(p->z[i]-cz)/90,tc,p->x[i],h,p->z[i]};tv[i]=vv;}
    polygon(top,tv,p->n);
    }
    litAlready=0;fixedGround=savedGround;
}
/* Losa transitable que SIGUE el terreno (parques y plazas poligonales): abanico desde el centroide subdividido en 4,
   con faldon perimetral. Sin fixedGround: cada vertice toma su altura de terreno, como ground(). */
static void slab_draped(const CmParcel *p,float y,int mat,uint32_t color){
    if(p->n<3)return;float cx,cz;parcel_centroid(p,&cx,&cz);
    float savedGround=fixedGround;fixedGround=-1000000;
    litAlready=1;uint32_t tc=lit_color(color,0,1,0),sc=lit_color(color,0,0,1);
    for(int i=0;i<p->n;i++){int j=(i+1)%p->n;
        float ax=p->x[i],az=p->z[i],bx=p->x[j],bz=p->z[j];
        float m1x=(cx+ax)*.5f,m1z=(cz+az)*.5f,m2x=(ax+bx)*.5f,m2z=(az+bz)*.5f,m3x=(cx+bx)*.5f,m3z=(cz+bz)*.5f;
        float sc=(mat==COBBLE)?18.f:60.f;
        #define SV(X,Z) {((X)-cx)/sc,((Z)-cz)/sc,tc,X,y,Z}
        Vertex t1[3]={SV(cx,cz),SV(m1x,m1z),SV(m3x,m3z)},t2[3]={SV(m1x,m1z),SV(ax,az),SV(m2x,m2z)},t3[3]={SV(m3x,m3z),SV(m2x,m2z),SV(bx,bz)},t4[3]={SV(m1x,m1z),SV(m2x,m2z),SV(m3x,m3z)};
        #undef SV
        polygon(mat,t1,3);polygon(mat,t2,3);polygon(mat,t3,3);polygon(mat,t4,3);
        uint32_t kc=lit_color(COLOR(226,222,214),0,0,1);Vertex sk[4]={{0,0,kc,ax,y,az},{1,0,kc,bx,y,bz},{1,1,kc,bx,-3,bz},{0,1,kc,ax,-3,az}};polygon(CONCRETE,sk,4); /* bordillo */
    }
    litAlready=0;fixedGround=savedGround;
}
/* Exposed, terrain-following curb for rectangular parcels. The sidewalk top
   remains at 1.2 so existing pedestrian and vehicle heights stay unchanged. */
static void rect_curbs(const CmParcel *p){
    float x0=p->x[0]-8,x1=p->x[1]+8,z0=p->z[0]-8,z1=p->z[2]+8;
    float sx[4]={x0,x1,x1,x0},sz[4]={z0,z0,z1,z1};
    float saved=fixedGround;fixedGround=-1000000;
    litAlready=1;
    for(int e=0;e<4;e++){
        int next=(e+1)&3;
        float ax=sx[e],az=sz[e],bx=sx[next],bz=sz[next];
        int parts=(int)(hypotf(bx-ax,bz-az)/40)+1;
        for(int k=0;k<parts;k++){
            float t=(float)k/parts,u=(float)(k+1)/parts;
            float x=ax+(bx-ax)*t,z=az+(bz-az)*t;
            float xx=ax+(bx-ax)*u,zz=az+(bz-az)*u;
            uint32_t c=lit_color(COLOR(221,211,193),az==bz?0:1,0,ax==bx?0:1);
            quad(CONCRETE,point(x,1.24f,z),point(xx,1.24f,zz),point(xx,-.12f,zz),point(x,-.12f,z),c,1,1);
        }
    }
    litAlready=0;fixedGround=saved;
}
/* Caja con altura ABSOLUTA (no relativa al terreno): cabinas y colgadores del Metrocable. */
static void abox(float x,float z,float y,float length,float width,float height,float angle,int mat,uint32_t color){
    float savedGround=fixedGround;fixedGround=0;
    float l=length*.5f,w=width*.5f,ca=cosf(angle),sa=sinf(angle);Point p[8];
    static const float sx[4]={-1,1,1,-1},sz[4]={-1,-1,1,1};
    for(int k=0;k<8;k++){float lx=sx[k&3]*l,lz=sz[k&3]*w;p[k]=point(x+lx*ca-lz*sa,(k<4)?y:y+height,z+lx*sa+lz*ca);}
    litAlready=1;uint32_t sc=emissive_color(mat,color);
    quad(mat,p[4],p[5],p[1],p[0],lit_color(sc,sa,0,-ca),1,1);quad(mat,p[6],p[7],p[3],p[2],lit_color(sc,-sa,0,ca),1,1);
    quad(mat,p[5],p[6],p[2],p[1],lit_color(sc,ca,0,sa),1,1);quad(mat,p[7],p[4],p[0],p[3],lit_color(sc,-ca,0,-sa),1,1);
    quad(mat,p[7],p[6],p[5],p[4],lit_color(color,0,1,0),1,1);
    quad(mat,p[0],p[1],p[2],p[3],lit_color(sc,0,-1,0),1,1); /* suspended cabins need a visible underside */
    litAlready=0;fixedGround=savedGround;
}
/* Nivel (altura de terreno) de una parcela: maximo en vertices, centro y medios; todo lo que se apoya en el podio usa este nivel. */
static float parcel_level(const CmParcel *p){
    float cx,cz;parcel_centroid(p,&cx,&cz);float high=geo_height(cx,cz);
    for(int i=0;i<p->n;i++){float hh=geo_height(p->x[i],p->z[i]);if(hh>high)high=hh;hh=geo_height((p->x[i]+cx)*.5f,(p->z[i]+cz)*.5f);if(hh>high)high=hh;}
    return high;
}
static int city_floor_bias(int d){return d==6?2:d==4?3:d==1?1:0;}
/* Facades stay wholly inside the solid parcel. The patterned frontage is
   visible from the street without creating a walk-through shop illusion. */
static void street_front(float cx,float z0,float w,int mat,unsigned h,int far){
    if(far||w<26)return;
    uint32_t trim=(mat==EATERY)?COLOR(181,68,55):(mat==WORKSHOP_FRONT)?COLOR(51,113,165):
                  (mat==OFFICE_FRONT)?COLOR(66,145,173):COLOR(36,161,150);
    box(cx,z0+3,6,w-4,4,18,0,CONCRETE,CONCRETE,COLOR(215,209,193));
    if(cityMid)return;
    /* Four distinct storefronts per material. Explicit front-face UV order
       makes the names readable from the street, without mirrored box faces. */
    float u=(h&1)?.5f:0,v=(h&2)?.5f:0,eps=.007f;
    uint32_t tint=lit_color(0xffffffffu,0,0,-1);
    Vertex sign[4]={{u+eps,v+eps,tint,cx-w*.5f+2,23.8f,z0+.85f},
        {u+.5f-eps,v+eps,tint,cx+w*.5f-2,23.8f,z0+.85f},
        {u+.5f-eps,v+.5f-eps,tint,cx+w*.5f-2,6.2f,z0+.85f},
        {u+eps,v+.5f-eps,tint,cx-w*.5f+2,6.2f,z0+.85f}};
    litAlready=1;polygon(mat,sign,4);litAlready=0;
    box(cx,z0+4.5f,17.4f,w-3,5,1.2f,0,AWNING,AWNING,trim);
    if(w>42){
        float xx=cx+w*.23f-5;
        box(xx,z0+1.5f,7,2,1,13,0,METAL,METAL,COLOR(225,210,175));
    }
    (void)h;
}
/* Relleno de una parcela rectangular con edificios del tipo de la manzana. Parcelas muy grandes se subdividen. */
static void fill_rect(int bx,int bz,float x0,float z0,float x1,float z1,int type,int far,unsigned h,int depth){
    float w=x1-x0,d=z1-z0;int dist=city_district((x0+x1)*.5f,(z0+z1)*.5f);
    if(w<24||d<24)return;
    if((w>300||d>290)&&depth<2){ /* Adjoining halves share the same party wall. */
        if(w>=d){float m=(x0+x1)*.5f;fill_rect(bx,bz,x0,z0,m,z1,type,far,h,depth+1);fill_rect(bx,bz,m,z0,x1,z1,city_block_type(bx+1,bz),far,h>>5,depth+1);}
        else{float m=(z0+z1)*.5f;fill_rect(bx,bz,x0,z0,x1,m,type,far,h,depth+1);fill_rect(bx,bz,x0,m,x1,z1,city_block_type(bx,bz+1),far,h>>5,depth+1);}
        return;
    }
    float cx=(x0+x1)*.5f,cz=(z0+z1)*.5f;int bias=city_floor_bias(dist);
    /* tipos grandes que no caben degradan a un tipo menudo */
    if(type==CB_TOWER&&(w<110||d<110))type=CB_CLASSIC;
    if(type==CB_COMMERCIAL&&(w<150||d<120))type=CB_CLASSIC;
    if(type==CB_CHURCH&&(w<150||d<170))type=CB_COLONIAL;
    if(type==CB_MARKET&&(w<170||d<110))type=CB_COMUNA;
    if(type==CB_OFFICE&&(w<95||d<95))type=CB_RETAIL;
    if(type==CB_WORKSHOP&&(w<70||d<70))type=CB_RETAIL;
    if(type==CB_RESTAURANT&&(w<68||d<70))type=CB_RETAIL;
    switch(type){
    case CB_RETAIL:{
        int units=w>150?3:w>95?2:1;float uw=w/units;
        for(int i=0;i<units;i++){
            unsigned hk=h^city_hash(bx,bz,320+i+depth*11);
            float xx=x0+uw*(i+.5f),ww=uw+.08f,zz=(z0+z1)*.5f;
            int mat=(hk&3)==0?EATERY:RETAIL;
            int floors=1+(hk>>5)%3;
            box(xx,zz,6,ww,d-4,20,0,CONCRETE,ROOF,COLOR(220,213,199));
            if(floors>1)box(xx,zz,26,ww,d-4,(floors-1)*22,0,STUCCO,ROOF,cityPastel[(hk>>9)&7]);
            box(xx,zz,6+20+(floors-1)*22,ww+1,d-15,2.2f,0,ROOF,ROOF,COLOR(119,99,88));
            street_front(xx,z0,ww,mat,hk,far);
            if(!far&&!cityMid)box(xx,z1-7,10,ww*.65f,2,12,0,GLASS,GLASS,COLOR(112,171,186));
        }
        break;}
    case CB_RESTAURANT:{
        uint32_t wall=cityPastel[(h>>7)&7];
        box(cx,cz,6,w-12,d-18,40,0,STUCCO,ROOF,wall);
        box(cx,cz,46,w-10,d-16,4,0,ROOF,ROOF,COLOR(145,73,63));
        street_front(cx,z0+7,w-13,EATERY,h,far);
        if(!far){
            box(cx,z0+14,24,w-18,13,2.5f,0,AWNING,AWNING,COLOR(194,80,60));
            if(!cityMid)for(int i=-1;i<=1;i++){
                float xx=cx+i*(w-27)/3;
                box(xx,z0+17,6,2.2f,2.2f,18,0,METAL,METAL,COLOR(92,58,47));
                box(xx,z0+23,6,7,7,2,0,ROOF,ROOF,COLOR(123,76,49));
            }
        }
        break;}
    case CB_OFFICE:{
        int floors=5+(h>>4)%5+city_floor_bias(dist);float tw=fminf(w-24,116),td=fminf(d-24,116);
        box(cx,cz,6,w-12,d-12,22,0,CONCRETE,CONCRETE,COLOR(208,218,220));
        box(cx,cz,28,tw,td,floors*21,0,CURTAIN,ROOF,0xffffffffu);
        box(cx,cz,28+floors*21,tw+5,td+5,3,0,METAL,METAL,COLOR(92,145,162));
        street_front(cx,z0+4,w-14,OFFICE_FRONT,h,far);
        if(!far&&!cityMid){
            for(int side=-1;side<=1;side+=2)box(cx+side*(tw*.5f+1),cz,31,1.8f,td,21*floors-3,0,METAL,METAL,COLOR(168,196,204));
            rooftop(cx,cz,31+floors*21,h);
        }
        break;}
    case CB_WORKSHOP:{
        box(cx,cz,6,w-12,d-12,31,0,CONCRETE,CONCRETE,COLOR(160,181,190));
        street_front(cx,z0+4,w-14,WORKSHOP_FRONT,h,far);
        for(int k=0;k<3;k++){
            float zz=z0+17+(k+.5f)*(d-34)/3;
            box(cx,zz,37,w-10,(d-34)/3-2,2+(k&1)*5,0,METAL,METAL,COLOR(88,128,154));
        }
        if(!far&&!cityMid){
            box(cx,z1-8,6,w*.6f,3,17,0,METAL,METAL,COLOR(64,91,107));
            for(int k=0;k<3;k++)box(x0+18+k*(w-36)/2,z0+8,25,5,2,2,0,CAR_PAINT,CAR_PAINT,COLOR(248,194,84));
        }
        break;}
    case CB_COMUNA:{int cols=(int)(w/58),rows=(int)(d/56);if(cols<1)cols=1;if(rows<1)rows=1;float cw=w/cols,cd=d/rows;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,20+r*cols+c+depth*40)^h;
            float hx=x0+cw*(c+.5f),hz=z0+cd*(r+.5f),hL=cw+.08f,hW=cd+.08f,base=6;int fl=1+(hk>>2)%3;
            comuna_house(hx,hz,hL,hW,base,fl,far,hk);
            if(!far&&(c==0||c==cols-1))residential_facade(c==0?x0:x1,hz,hW,base,fl,20,c==0?-PI*.5f:PI*.5f,0);
            if(!far&&(hk&1))gable_roof(hx,hz,hL-3,hW-3,base+fl*20+1.5f,base+fl*20+5,COLOR(185,169,155));}
        if(!far&&!cityMid&&bx<3)quad(MURAL,point(cx-40,24,z0+.5f),point(cx+40,24,z0+.5f),point(cx+40,7,z0+.5f),point(cx-40,7,z0+.5f),0xffffffffu,1,1);
        break;}
    case CB_COLONIAL:{int cols=(int)(w/47);if(cols<1)cols=1;float cw=w/cols;int rows=d>=125?2:1;float W=rows==2?54:d-10;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,40+r*cols+c+depth*40)^h;
            colonial_house(x0+cw*(c+.5f),rows==2?(r?z1-W*.5f-5:z0+W*.5f+5):cz,cw+.08f,W,1+((hk>>2)&1),cityPastel[hk&7],far,hk);}
        if(!far&&rows==2){float gw=w-40;if(gw>30){garden(x0+20,z0+64,gw,d-128>20?d-128:20,h,far);tree(x0+30,cz);tree(x1-30,cz);}}
        break;}
    case CB_TOWER:{int floors=6+(h>>3)%5+bias;float bw=w-14,bd=d-14;if(bw>120)bw=120;if(bd>110)bd=110;float tw=bw*.74f,td=bd*.76f;
        int round=(h>>7)&1;float rr=(tw<td?tw:td)*.52f;
        if(far){if(round)cylinder(cx,cz,6,rr,floors*24,8,.39f,CURTAIN,ROOF,0xffffffffu);else box(cx,cz,6,tw,td,floors*24,0,CURTAIN,ROOF,0xffffffffu);break;}
        box(cx,cz,6,bw,bd,10,0,SHOP,CONCRETE,0xffffffffu);
        if(round){ /* v2.7: torre cilindrica (octogono) de muro cortina con corona */
            cylinder(cx,cz,16,rr,floors*24,8,.39f,CURTAIN,ROOF,0xffffffffu);
            frustum(cx,cz,16+floors*24,rr*1.04f,rr*.6f,8,8,.39f,METAL,COLOR(200,204,210));
            cylinder(cx,cz,24+floors*24,rr*.6f,10,8,.39f,METAL,METAL,COLOR(200,204,210));
        }else{
            CmParcel cp;chamfered_rect(&cp,cx-tw*.5f,cz-td*.5f,cx+tw*.5f,cz+td*.5f,7);
            prism(&cp,16,floors*24,CURTAIN,ROOF,0xffffffffu);
            for(int f=2;f<floors;f+=3)box(cx,cz,16+f*24-1,tw+2,td+2,1.6f,0,METAL,METAL,COLOR(210,212,216)); /* fajas */
            box(cx,cz,16+floors*24,tw*.68f,td*.66f,14,0,MODERN,ROOF,0xffffffffu);
        }
        box(cx,cz,30+floors*24,3,3,22,0,CAR_PAINT,CAR_PAINT,COLOR(180,30,30));
        if(w>150){tree(x0+14,z0+14);tree(x1-14,z0+14);tree(x0+14,z1-14);tree(x1-14,z1-14);garden(x0+8,cz-20,w-bw>40?(w-bw)*.5f-10:20,40,h,far);}
        break;}
    case CB_COMMERCIAL:{float aw=w*.5f,ad=d-12,bwid=w-aw-12,bd=d*.36f;float ax=x0+6+aw*.5f,bxx=x1-6-bwid*.5f,bzz=z0+6+bd*.5f;
        if(far){box(ax,cz,6,aw,ad,36,0,STUCCO,ROOF,cityPastel[h&7]);box(bxx,bzz,6,bwid,bd,36,0,STUCCO,ROOF,cityPastel[h&7]);break;}
        for(int f=0;f<2;f++){box(ax,cz,6+f*18,aw,ad,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:0xffffffffu);box(bxx,bzz,6+f*18,bwid,bd,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:0xffffffffu);}
        box(ax,cz,42,aw+3,ad+3,2,0,ROOF,ROOF,COLOR(120,110,100));box(bxx,bzz,42,bwid+3,bd+3,2,0,ROOF,ROOF,COLOR(120,110,100));
        rooftop(ax,cz,44,h);rooftop(bxx,bzz,44,h>>4);
        for(int k=0;k<4;k++)box(x0+14+k*(aw-20)/4,z0,18,(aw-28)/4,10,.8f,-.3f,AWNING,AWNING,0xffffffffu);
        float px0=x0+aw+8,pz0=z0+bd+14,pw=x1-px0-4,pd=z1-pz0-4;if(pw>20&&pd>20)ground(ROAD,px0,pz0,pw,pd,6.3f,COLOR(150,150,150),3);
        box(bxx,z1-4,6,bwid*.5f,3,1.5f,0,CAR_PAINT,CAR_PAINT,COLOR(230,60,50));
        break;}
    case CB_CHURCH:{float nw=w*.42f,nd=d*.62f;float zz=cz+d*.08f;
        if(far){box(cx,zz,6,nw,nd,40,0,STUCCO,ROOF,COLOR(242,232,204));box(cx-nw*.38f,zz-nd*.6f,6,22,22,80,0,STUCCO,ROOF,COLOR(242,232,204));break;}
        box(cx,zz,6,nw,nd,34,0,STUCCO,ROOF,COLOR(242,232,204));gable_roof(cx,zz,nw,nd,40,58,COLOR(150,90,70));
        /* v2.7: cupula sobre el crucero y chapitel piramidal en el campanario */
        cylinder(cx,zz+nd*.15f,40,nw*.3f,8,8,.39f,STUCCO,ROOF,COLOR(242,232,204));
        frustum(cx,zz+nd*.15f,48,nw*.32f,nw*.18f,7,8,.39f,METAL,COLOR(120,150,140));frustum(cx,zz+nd*.15f,55,nw*.18f,0,7,8,.39f,METAL,COLOR(120,150,140));
        box(cx,zz+nd*.15f,61,1.2f,1.2f,6,0,CAR_PAINT,CAR_PAINT,COLOR(90,80,60));
        float tx=cx-nw*.38f,tz=zz-nd*.6f;box(tx,tz,6,22,22,74,0,STUCCO,ROOF,COLOR(242,232,204));frustum(tx,tz,80,16,0,14,4,PI*.25f,ROOF,COLOR(150,90,70));box(tx,tz,93,1.5f,1.5f,8,0,CAR_PAINT,CAR_PAINT,COLOR(90,80,60));
        for(int k=0;k<3;k++)box(cx-nw*.5f+1,zz-30+k*30,20,1,4,12,0,GLASS,GLASS,COLOR(120,80,200));
        ground(SIDEWALK,x0+6,z0+6,w-12,d*.25f,6.2f,COLOR(226,222,214),6);tree(x0+22,z0+24);tree(x1-22,z0+24);garden(x1-60,z0+8,44,26,h,far);
        for(int k=0;k<3;k++)box(cx-50+k*50,z0+40,6,10,3,2,0,ROOF,ROOF,COLOR(110,80,50));
        break;}
    case CB_MARKET:{float nw=w-14,nd=d*.64f;
        if(far){box(cx,cz-d*.08f,6,nw,nd,26,0,BRICK,ROOF,COLOR(220,200,180));break;}
        box(cx,cz-d*.08f,6,nw,nd,22,0,BRICK,ROOF,COLOR(220,200,180));vault(cx,cz-d*.08f,nw+2,nd+2,28,15,METAL,COLOR(190,200,210)); /* v2.7: boveda metalica */
        for(int k=0;k<5;k++)box(x0+20+k*(nw-30)/4,z1-12,6,26,10,9,0,CAR_PAINT,CAR_PAINT,cityPastel[(h>>k)&7]);
        box(cx-nw*.3f,cz-d*.08f,6,12,12,30,0,BRICK,BRICK,COLOR(180,160,140));
        break;}
    default:{int cols=(int)(w/63),rows=d>125?2:1;if(cols<1)cols=1;float cw=w/cols,cd=rows==2?fminf(66,d*.40f):d;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,60+r*cols+c+depth*40)^h;
            float edge0=x0+cw*c+(c?(city_rand(bx,bz,190+c)-.5f)*cw*.32f:0);
            float edge1=x0+cw*(c+1)+(c+1<cols?(city_rand(bx,bz,191+c)-.5f)*cw*.32f:0);
            float xx=(edge0+edge1)*.5f,zz=rows==2?(r?z1-cd*.5f:z0+cd*.5f):cz;
            int floors=2+(hk%3)+bias;int mat=(hk>>2)&1?BRICK:STUCCO;uint32_t tint=mat==STUCCO?cityPastel[(hk>>3)&7]:cityBrick[(hk>>3)&3];float L=edge1-edge0+.08f,Wd=cd;
            if(far){box(xx,zz,6,L,Wd,floors*24,0,mat,ROOF,tint);continue;}
            if(cols==1&&((hk>>9)&3)==0&&L>40&&Wd>40){ /* Only isolated buildings chamfer their shared walls. */
                CmParcel cp;chamfered_rect(&cp,xx-L*.5f,zz-Wd*.5f,xx+L*.5f,zz+Wd*.5f,7);
                prism(&cp,6,12,SHOP,ROOF,0xffffffffu);prism(&cp,18,floors*24-12,(hk&4)?MODERN:CONCRETE,ROOF,0xffffffffu);
                prism(&cp,6+floors*24,2,METAL,METAL,COLOR(200,204,210));rooftop(xx,zz,8+floors*24,hk>>8);continue;
            }
            int frontMat=((hk>>13)%5==0)?EATERY:((hk>>13)%5==1)?OFFICE_FRONT:RETAIL;
            box(xx,zz,6,L,Wd,20,0,CONCRETE,CONCRETE,COLOR(221,213,194));
            box(xx,zz,26,L,Wd,floors*24-20,0,mat,ROOF,tint);
            if(!far&&!cityMid){
                residential_facade(xx,zz-Wd*.5f,L,26,floors-1,24,0,0);
                if(c==0||c==cols-1)residential_facade(c==0?x0:x1,zz,Wd,26,floors-1,24,c==0?-PI*.5f:PI*.5f,0);
            }
            if(r==0)street_front(xx,z0,L,frontMat,hk,far);
            if(!cityMid&&(hk&64))for(int f=1;f<floors&&f<4;f+=2){
                float face=zz+(r?1:-1)*(Wd*.5f+2),level=6+f*24;
                box(xx,face,level,L*.60f,4,1.2f,0,CONCRETE,CONCRETE,COLOR(207,201,192));
                box(xx,face+(r?2:-2),level+1.2f,L*.60f,.6f,6,0,METAL,METAL,COLOR(60,81,89));
                for(int side=-1;side<=1;side+=2)box(xx+side*L*.30f,face,level+1.2f,.6f,4,6,0,METAL,METAL,COLOR(60,81,89));
            }
            box(xx,zz,6+floors*24,L+3,Wd+3,3,0,ROOF,ROOF,COLOR(120,110,100));rooftop(xx,zz,9+floors*24,hk>>8);
            if((dist==0||dist==5)&&r==rows-1)quad(MURAL,point(xx-L*.45f,25,zz+Wd*.5f+.6f),point(xx+L*.45f,25,zz+Wd*.5f+.6f),point(xx+L*.45f,7,zz+Wd*.5f+.6f),point(xx-L*.45f,7,zz+Wd*.5f+.6f),0xffffffffu,1,1);
        }
        if(rows==2&&!far){ground(GRASS,x0+8,z0+cd+2,w-16,d-2*cd-4,6.2f,COLOR(158,181,145),4);if(!cityMid){tree(x0+w*.3f,cz);tree(x0+w*.7f,cz);}}
        break;}
    }
}
/* Parcela irregular (triangulo/trapecio junto a la avenida): edificio prisma escalonado. */
static void fill_poly_mass(const CmParcel *p,int bx,int bz,int far,unsigned h){
    int dist=city_district(p->x[0],p->z[0]);int floors=3+(h>>4)%4+city_floor_bias(dist);float area=cm_area(p);
    if(area<2500)floors=2;
    int mat=(dist==6||dist==4)?((h&1)?CURTAIN:MODERN):((h&1)?BRICK:STUCCO);uint32_t tint=(mat==CURTAIN||mat==MODERN)?0xffffffffu:mat==BRICK?cityBrick[(h>>2)&3]:cityPastel[(h>>2)&7];
    CmParcel a,b;a=*p; /* Shared parcel edges are party walls, not empty gaps. */
    if(far){prism(&a,6,floors*24,mat,ROOF,tint);return;}
    prism(&a,6,12,SHOP,ROOF,0xffffffffu);            /* planta comercial */
    prism(&a,18,floors*24-12,mat,ROOF,tint);
    prism(&a,6+floors*24,2,ROOF,ROOF,COLOR(120,110,100));
    if(area>6000){parcel_shrunk(p,26,&b);prism(&b,8+floors*24,36,mat==CURTAIN?CURTAIN:CONCRETE,ROOF,0xffffffffu);}
    float cx,cz;parcel_centroid(p,&cx,&cz);if(!cityMid)rooftop(cx,cz,area>6000?44+floors*24:9+floors*24,h>>6);
    (void)bx;(void)bz;
}
/* A diagonal parcel is a row of buildings, not one enormous triangular tower.
   Clipping keeps every frontage inside the collision parcel. */
static void fill_poly(const CmParcel *p,int bx,int bz,int far,unsigned h){
    if(cm_area(p)<8500){fill_poly_mass(p,bx,bz,far,h);return;}
    float x0=p->x[0],x1=x0,z0=p->z[0],z1=z0;
    for(int i=1;i<p->n;i++){x0=fminf(x0,p->x[i]);x1=fmaxf(x1,p->x[i]);z0=fminf(z0,p->z[i]);z1=fmaxf(z1,p->z[i]);}
    int alongX=x1-x0>z1-z0;float lo=alongX?x0:z0,hi=alongX?x1:z1;
    for(int i=0;i<3;i++){
        float a=lo+(hi-lo)*i/3,b=lo+(hi-lo)*(i+1)/3;CmParcel piece=*p;
        cm_clip_halfplane(&piece,alongX?a:0,alongX?0:a,alongX?1:0,alongX?0:1);
        cm_clip_halfplane(&piece,alongX?b:0,alongX?0:b,alongX?-1:0,alongX?0:-1);
        if(piece.n>=3&&cm_area(&piece)>600)fill_poly_mass(&piece,bx,bz,far,h^city_hash(bx,bz,240+i));
    }
}
/* Parque en parcela (rectangular o no): cesped, senderos, arboles, jardines, fuente. */
static void fill_park(const CmParcel *p,int far,unsigned h){
    CmParcel g;parcel_shrunk(p,3,&g);if(cm_is_rect(p))ground(GRASS,g.x[0],g.z[0],g.x[1]-g.x[0],g.z[2]-g.z[0],1.8f,0xffffffffu,6);else slab_draped(&g,1.8f,GRASS,0xffffffffu);
    float cx,cz;parcel_centroid(p,&cx,&cz);
    if(cm_is_rect(p)){float x0=p->x[0],z0=p->z[0],x1=p->x[1],z1=p->z[2],w=x1-x0,d=z1-z0;
        ground(SIDEWALK,x0+6,cz-7,w-12,14,2,0xffffffffu,6);ground(SIDEWALK,cx-6,z0+6,13,d-12,2,0xffffffffu,6);
        if(far)return;
        for(int k=0;k<4;k++)tree(x0+18+(k%2)*(w-36),z0+20+(k/2)*(d-40));
        if(w>120&&d>120){garden(x0+10,z0+10,w*.35f,d*.3f,h,far);garden(x1-10-w*.35f,z1-10-d*.34f,w*.35f,d*.34f,h>>3,far);}
        if(!cityMid){for(int k=0;k<(int)(w/19)-1;k++)box(x0+10+k*19,z0+2,1.8f,8,4,5,0,GRASS,GRASS,COLOR(50,110,55));
            cylinder(cx+6,cz+7,2,6,1,8,0,CONCRETE,CONCRETE,0xffffffffu);cylinder(cx+6,cz+7,3,4.5f,3.5f,8,.39f,WATER,WATER,0xffffffffu);
            for(int k=0;k<3;k++)bench(x0+30+k*(w-60)*.5f,cz-16,0);}
    }else{ if(far)return;tree(cx,cz);CmParcel s;parcel_shrunk(p,18,&s);for(int i=0;i<p->n&&i<4;i++)tree(s.x[i],s.z[i]); }
}
/* A public parking court beside a planted pocket park. Its fountain stays
   at the same shared collision coordinates as the other parks. */
static void fill_parking_park(const CmParcel *p,int far,unsigned h){
    if(!cm_is_rect(p)){fill_park(p,far,h);return;}
    float x0=p->x[0],x1=p->x[1],z0=p->z[0],z1=p->z[2],cx=(x0+x1)*.5f,cz=(z0+z1)*.5f;
    ground(ROAD,x0+3,z0+3,x1-x0-6,cz-z0-17,1.8f,COLOR(173,181,188),4);
    ground(GRASS,x0+3,cz-12,x1-x0-6,z1-cz+9,1.8f,COLOR(174,204,148),4);
    ground(SIDEWALK,x0+3,cz-15,x1-x0-6,8,2,0xffffffffu,4);
    if(far)return;
    for(float zz=z0+12;zz<cz-23;zz+=18){
        ground(FLAT,x0+7,zz,25,.7f,1.9f,COLOR(236,224,171),1);
        ground(FLAT,x1-32,zz,25,.7f,1.9f,COLOR(236,224,171),1);
    }
    tree(x0+18,z1-20);tree(x1-18,z1-20);
    cylinder(cx+6,cz+7,2,6,1,8,0,CONCRETE,CONCRETE,0xffffffffu);
    cylinder(cx+6,cz+7,3,4.5f,3.5f,8,.39f,WATER,WATER,0xffffffffu);
    if(!cityMid){bench(x0+30,cz+30,0);bench(x1-30,cz+30,0);}
}
/* Plaza Botero: plaza dura con esculturas de bronce sobre pedestales, palmas y fuente. */
static void fill_plaza(const CmParcel *p,int far,unsigned h){
    float cx,cz;parcel_centroid(p,&cx,&cz);
    CmParcel s;parcel_shrunk(p,2,&s);slab_draped(&s,1.7f,COBBLE,0xffffffffu);
    if(far)return;
    CmParcel g;parcel_shrunk(p,22,&g);
    for(int i=0;i<g.n;i++){float sx=(g.x[i]+cx)*.5f,sz=(g.z[i]+cz)*.5f;
        box(sx,sz,1.6f,8,8,3,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));
        box(sx,sz,4.6f,5,4,6,(i*.7f),CAR_PAINT,CAR_PAINT,COLOR(70,60,40));box(sx,sz,10.6f,3.4f,3.4f,3,0,CAR_PAINT,CAR_PAINT,COLOR(70,60,40)); /* figura rotunda */
        if(!cityMid)tree(g.x[i],g.z[i]);}
    cylinder(cx,cz,1.6f,8,1.2f,8,0,CONCRETE,CONCRETE,0xffffffffu);cylinder(cx,cz,2.8f,6,3.5f,8,.39f,WATER,WATER,0xffffffffu);cylinder(cx,cz,2.8f,1.4f,8,6,0,CONCRETE,CONCRETE,0xffffffffu); /* fuente redonda */
    for(int k=0;k<4;k++)bench(cx-30+k*20,cz+24,0);
    cylinder(cx+34,cz-20,1.6f,6,9,6,0,STUCCO,ROOF,cityPastel[h&7]);frustum(cx+34,cz-20,10.6f,8,0,6,6,0,ROOF,COLOR(150,90,70)); /* kiosco */
    (void)h;
}
/* Pueblito Paisa sobre el cerro Nutibara: cerro escalonado, plazuela, iglesia y casitas coloniales. */
static void fill_pueblito(const CmParcel *p,int far,unsigned h){
    CmParcel a,b,c;parcel_shrunk(p,0,&a);parcel_shrunk(p,20,&b);parcel_shrunk(p,42,&c);
    prism(&a,0,9,GRASS,GRASS,COLOR(150,190,120));prism(&b,9,9,GRASS,GRASS,COLOR(160,200,130));prism(&c,18,8,SIDEWALK,SIDEWALK,COLOR(220,205,180));
    float cx,cz;parcel_centroid(p,&cx,&cz);
    if(far){box(cx,cz-26,26,30,30,30,0,STUCCO,ROOF,COLOR(250,246,236));return;}
    for(int i=0;i<b.n;i++)tree((a.x[i]+b.x[i])*.5f,(a.z[i]+b.z[i])*.5f);
    box(cx,cz-28,26,30,26,22,0,STUCCO,ROOF,COLOR(250,246,236));gable_roof(cx,cz-28,30,26,48,58,COLOR(190,90,60));
    box(cx+18,cz-28,26,9,9,38,0,STUCCO,ROOF,COLOR(250,246,236));box(cx+18,cz-28,64,11,11,2,0,ROOF,ROOF,COLOR(190,90,60));
    for(int k=0;k<6;k++){float ang=PI*.15f+k*PI*.34f;float hx=cx+cosf(ang)*44,hz=cz+10+sinf(ang)*30;unsigned hk=city_hash(k,7,3);
        box(hx,hz,26,20,16,12,0,STUCCO,ROOF,cityPastel[hk&7]);gable_roof(hx,hz,20,16,38,44,(hk&8)?COLOR(190,90,60):COLOR(150,110,90));}
    box(cx,cz+2,26,8,8,1,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));box(cx,cz+2,27,5,5,3,0,WATER,WATER,0xffffffffu);
    (void)h;
}
/* Estadio Atanasio Girardot: anillo de graderias, cancha, torres de iluminacion. */
static void fill_stadium(const CmParcel *p,int far,unsigned h){
    float cx,cz;parcel_centroid(p,&cx,&cz);float A=(p->x[1]-p->x[0])*.5f-18,B=(p->z[2]-p->z[0])*.5f-12;
    ground(GRASS,cx-80,cz-46,160,92,6.5f,COLOR(120,200,110),4);
    if(!far){for(int k=0;k<5;k++)ground(FLAT,cx-80+k*40,cz-46,1.2f,92,7.1f,COLOR(214,220,209),1);}
    int segs=far?14:26;
    for(int k=0;k<segs;k++){float t=k*2*PI/segs,t2=(k+1)*2*PI/segs;float x=cx+cosf(t)*A,z=cz+sinf(t)*B;float x2=cx+cosf(t2)*A,z2=cz+sinf(t2)*B;
        float ang=atan2f(z2-z,x2-x);float len=sqrtf((x2-x)*(x2-x)+(z2-z)*(z2-z))*1.08f;x=(x+x2)*.5f;z=(z+z2)*.5f;
        box(x,z,6,len,26,22,ang,STUCCO,SIDEWALK,COLOR(236,236,232));
        if(!far)box(x+cosf(t)*4,z+sinf(t)*4,28,len*.98f,18,12,ang,SIDEWALK,ROOF,COLOR(70,140,90));
    }
    if(far)return;
    for(int k=0;k<4;k++){float x=cx+(k%2?A-10:-A+10),z=cz+(k/2?B+2:-B-2);box(x,z,6,3,3,64,0,CAR_PAINT,CAR_PAINT,COLOR(120,124,128));box(x,z,70,12,4,6,0,SIDEWALK,SIDEWALK,COLOR(230,230,210));}
    (void)h;
}
/* Edificio puente sobre el paso (tunel) de una manzana doble. */
static void tunnel_building(int bx,float z0,float z1,int far){
    float x=bx*320+351,cz=(z0+z1)*.5f,d=(z1-z0)*.72f;
    box(x,cz,60,150,d,34,0,GLASS,ROOF,COLOR(190,215,225));
    for(int k=0;k<4;k++)box(x+(k&1?42:-42),cz+(k&2?d*.3f:-d*.3f),6,10,10,54,0,STUCCO,STUCCO,COLOR(200,200,205));
    if(far)return;
    box(x,cz,94,120,d*.7f,16,0,STUCCO,ROOF,COLOR(200,200,205));
    /* No decorative paint beneath the building: this is a tunnel, not a crossing. */
}
/* Metro de Medellin a escala del jugador (citymap.h: CM_METRO_*): viaducto sobre pilares, estaciones con anden a 30,
   escalera de 60 de recorrido en la orilla, y tren de dos coches (paneles bajos + pilares de ventana: se ve la ciudad
   desde dentro cuando el jugador va a bordo). */
static void rail_canopy(float x,float z,float width,float length,float y,float rise){
    float saved=fixedGround;if(saved<=-999999)fixedGround=geo_height(x,z);litAlready=1;
    for(int k=0;k<4;k++){
        float a=PI*k/4,b=PI*(k+1)/4;
        float xa=x+cosf(a)*width*.5f,xb=x+cosf(b)*width*.5f,ya=y+sinf(a)*rise,yb=y+sinf(b)*rise;
        quad(METAL,point(xa,ya,z-length*.5f),point(xa,ya,z+length*.5f),point(xb,yb,z+length*.5f),point(xb,yb,z-length*.5f),lit_color(COLOR(210,224,215),cosf((a+b)*.5f),sinf((a+b)*.5f),0),length/45,.5f);
        quad(METAL,point(xb,yb,z-length*.5f),point(xb,yb,z+length*.5f),point(xa,ya,z+length*.5f),point(xa,ya,z-length*.5f),lit_color(COLOR(129,155,154),0,-1,0),length/45,.5f);
    }
    litAlready=0;fixedGround=saved;
}
static void metro_train(void){
    for(int c=0;c<2;c++){float zc=view->metroZ+(c-.5f)*(CM_TRAIN_CAR+6);
        if(!nearby(CM_METRO_X,zc,700))continue;
        float previousGround=fixedGround;fixedGround=geo_height(CM_METRO_X,zc);
        float fl=CM_PLAT_H-.6f;
        box(CM_METRO_X,zc,fl,24,CM_TRAIN_CAR,1.2f,0,SIDEWALK,SIDEWALK,COLOR(120,120,124));                 /* piso */
        box(CM_METRO_X,zc,fl-3,20,CM_TRAIN_CAR-8,3,0,CAR_PAINT,CAR_PAINT,COLOR(60,62,66));                  /* bajos */
        for(int sd=-1;sd<=1;sd+=2){float x=CM_METRO_X+sd*11.6f;
            box(x,zc,fl+1.2f,.8f,CM_TRAIN_CAR,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));                  /* panel bajo */
            box(x,zc,fl+5.4f,1.0f,CM_TRAIN_CAR,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80));               /* franja verde */
            for(int k=-2;k<=2;k++){float zz=zc+k*22;box(x,zz,fl+9.2f,.8f,2,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));} /* pilares de ventana */
            for(int door=-1;door<=1;door+=2)for(int panel=-1;panel<=1;panel+=2){
                float dz=zc+door*33+panel*(2.4f+view->metroDoors*5);
                box(x+sd*.1f,dz,fl+1.2f,.9f,4.7f,17,0,METAL,METAL,COLOR(226,232,226));
                if(!view->inMetro)box(x+sd*.65f,dz,fl+10,.12f,3.2f,5.4f,0,GLASS,GLASS,COLOR(101,143,147));
            }
            box(x,zc,fl+18.2f,.8f,CM_TRAIN_CAR,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));             /* dintel */
        }
        if(!view->inMetro)for(int side=-1;side<=1;side+=2)
            for(int k=-2;k<2;k++)box(CM_METRO_X+side*11.65f,zc+k*22+11,fl+9.3f,.35f,19,8.5f,0,GLASS,GLASS,COLOR(112,151,165));
        rail_canopy(CM_METRO_X,zc,25,CM_TRAIN_CAR+1,fl+20,2.8f);
        if(view_distance(CM_METRO_X,zc)<250){
            box(CM_METRO_X,zc,fl+22.5f,13,24,2,0,METAL,METAL,COLOR(157,168,168));
            for(int bogie=-1;bogie<=1;bogie+=2)box(CM_METRO_X,zc+bogie*34,fl-4,21,13,3,0,METAL,METAL,COLOR(58,63,67));
        }
        for(int e=-1;e<=1;e+=2){float ze=zc+e*CM_TRAIN_CAR*.5f;                                                   /* testeros abiertos: panel bajo, pilares y dintel */
            box(CM_METRO_X,ze,fl+1.2f,24,1,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X,ze,fl+5.4f,24.2f,1.2f,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80));
            box(CM_METRO_X-7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X+7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));
            box(CM_METRO_X,ze,fl+18.2f,24,1,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));}
        if(view->inMetro||view_distance(CM_METRO_X,zc)<200){
            for(int k=-2;k<=2;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*8,zc+k*20,fl+1.2f,5,12,4,0,CAR_PAINT,CAR_PAINT,COLOR(40,90,160)); /* asientos */
            for(int k=-2;k<=2;k++)box(CM_METRO_X,zc+k*20+10,fl+1.2f,1,1,18,0,CAR_PAINT,CAR_PAINT,COLOR(200,200,200));       /* barras */
        }
        fixedGround=previousGround;
    }
}
static void metro(int bz,int far){
    float z=bz*320;
    for(int k=0;k<5;k++)cylinder(CM_METRO_X,z+32+k*64,0,4,CM_METRO_DECK,6,0,CONCRETE,CONCRETE,0xffffffffu);
    box(CM_METRO_X,z+160,CM_METRO_DECK,14,320,3,0,SIDEWALK,ROAD,COLOR(190,186,180));
    if(!far){box(CM_METRO_X-3,z+160,CM_METRO_DECK+3,.8f,320,.6f,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));box(CM_METRO_X+3,z+160,CM_METRO_DECK+3,.8f,320,.6f,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));
        if(!cm_metro_station(bz)){box(CM_METRO_X-6.5f,z+160,CM_METRO_DECK+3,1,320,3,0,SIDEWALK,SIDEWALK,COLOR(210,206,196));box(CM_METRO_X+6.5f,z+160,CM_METRO_DECK+3,1,320,3,0,SIDEWALK,SIDEWALK,COLOR(210,206,196));}}
    if(cm_metro_station(bz)){
        box(CM_METRO_X,z+160,CM_PLAT_H-1.2f,36,200,1.2f,0,SIDEWALK,SIDEWALK,COLOR(226,222,214));                /* anden 60..260 */
        for(int k=0;k<3;k++)for(int sd=-1;sd<=1;sd+=2)cylinder(CM_METRO_X+sd*14,z+80+k*80,0,2.4f,CM_PLAT_H-1.2f,6,0,CONCRETE,CONCRETE,0xffffffffu); /* pilares del anden */
        for(int k=0;k<4;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*17,z+70+k*60,CM_PLAT_H,2,2,24,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));
        rail_canopy(CM_METRO_X,z+160,44,208,CM_PLAT_H+24,5);
        if(!far)for(int k=0;k<4;k++)box(CM_METRO_X,z+74+k*57,CM_PLAT_H+25,38,1.2f,1.1f,0,METAL,METAL,COLOR(56,80,74));
        for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*17.5f,z+160,CM_PLAT_H,.6f,200,.4f,0,CAR_PAINT,CAR_PAINT,COLOR(250,200,60)); /* linea amarilla de borde */
        /* escalera: 10 peldanos de 6 x 3.02 subiendo hacia el norte (z 318 -> 258), x 1465..1476 */
        for(int k=0;k<10;k++)box(1470.5f,z+315-k*6,0,11,6,(k+1)*(CM_PLAT_H/10),0,STUCCO,SIDEWALK,COLOR(226,222,214));
        for(int sd=-1;sd<=1;sd+=2)for(int k=0;k<4;k++){float zz=z+312-k*18;box(1470.5f+sd*5.8f,zz,(318-(zz-z))*(CM_PLAT_H/60)+1,1,1,10,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));}
        box(1470.5f,z+257,CM_PLAT_H-1.2f,11,4,1.2f,0,SIDEWALK,SIDEWALK,COLOR(226,222,214));                    /* rellano hacia el anden */
        if(!far){box(CM_METRO_X,z+58,CM_PLAT_H+2,22,1.2f,5,0,CAR_PAINT,CAR_PAINT,COLOR(60,130,90));box(1470.5f,z+322,0,10,1,12,0,CAR_PAINT,CAR_PAINT,COLOR(60,130,90));} /* carteles */
    }
}
/* Metrocable: pilonas, cable y cabinas entre (CM_CABLE_X0,Z0) y (CM_CABLE_X1,Z1). */
static void metrocable(int far){
    enum {N=5};float hs[N+1],px[N+1],pz[N+1];
    for(int i=0;i<=N;i++){cm_cable_node(i,&px[i],&pz[i]);hs[i]=geo_height(px[i],pz[i])+96;
        if(nearby(px[i],pz[i],700)){
            float bottom=(i==0||i==N)?46:0;
            box(px[i],pz[i],bottom,5,5,96-bottom,0,METAL,METAL,COLOR(118,143,152));
            if(!far){
                float a=i<N?atan2f(pz[i+1]-pz[i],px[i+1]-px[i]):atan2f(pz[i]-pz[i-1],px[i]-px[i-1]);
                box(px[i],pz[i],94,17,2,2,a+PI*.5f,METAL,METAL,COLOR(55,83,96));
            }
        }}
    float dx=CM_CABLE_X1-CM_CABLE_X0,dz=CM_CABLE_Z1-CM_CABLE_Z0,l=sqrtf(dx*dx+dz*dz),nx=-dz/l,nz=dx/l;
    float savedGround=fixedGround;fixedGround=0;
    for(int i=0;i<N;i++){if(!nearby((px[i]+px[i+1])*.5f,(pz[i]+pz[i+1])*.5f,760))continue;
        for(int s=-1;s<=1;s+=2){float ox=nx*3*s,oz=nz*3*s;
            quad(CAR_PAINT,point(px[i]+ox-nx*.5f,hs[i],pz[i]+oz-nz*.5f),point(px[i+1]+ox-nx*.5f,hs[i+1],pz[i+1]+oz-nz*.5f),point(px[i+1]+ox+nx*.5f,hs[i+1],pz[i+1]+oz+nz*.5f),point(px[i]+ox+nx*.5f,hs[i],pz[i]+oz+nz*.5f),COLOR(60,60,64),1,1);}
    }
    fixedGround=savedGround;
    if(far)return;
    for(int k=0;k<6;k++){float t=fmodf(view->time*.03f+k/6.0f,1.0f);int s=k&1;if(s)t=1-t;
        int i=(int)(t*N);if(i>=N)i=N-1;float tt=t*N-i;float x=px[i]+(px[i+1]-px[i])*tt+nx*3*(s?1:-1),z=pz[i]+(pz[i+1]-pz[i])*tt+nz*3*(s?1:-1),y=hs[i]+(hs[i+1]-hs[i])*tt;
        if(!nearby(x,z,700))continue;
        float ang=atan2f(pz[i+1]-pz[i],px[i+1]-px[i]);
        static const uint32_t cabs[4]={COLOR(31,171,172),COLOR(233,151,64),COLOR(211,86,104),COLOR(80,151,213)};
        uint32_t body=cabs[k&3];
        abox(x,z,y-10,8,6,8,ang,CAR_PAINT,body);
        abox(x,z,y-2,9,7,1.2f,ang,METAL,COLOR(230,231,213));
        abox(x,z,y-11,8,6,1,ang,METAL,COLOR(52,64,68));
        abox(x,z,y-.8f,.8f,.8f,2,ang,METAL,COLOR(59,66,70));
        for(int side=-1;side<=1;side+=2){
            float ox=-sinf(ang)*side*3.1f,oz=cosf(ang)*side*3.1f;
            abox(x+ox,z+oz,y-8.3f,5.9f,.24f,4.7f,ang,GLASS,COLOR(92,170,193));
            abox(x+ox,z+oz,y-8.3f,.45f,.4f,5,ang,METAL,COLOR(224,228,220));
        }
    }
}
/* Avenida diagonal: separador ajardinado con arboles y farolas. */
static void diagonal_avenue(void){
    float dx=CM_DIAG_X1-CM_DIAG_X0,dz=CM_DIAG_Z1-CM_DIAG_Z0,l=sqrtf(dx*dx+dz*dz),ux=dx/l,uz=dz/l,nx=-uz,nz=ux;
    int segs=(int)(l/60);
    for(int k=0;k<segs;k++){float t0=(float)k/segs,t1=(float)(k+1)/segs;float x0=CM_DIAG_X0+dx*t0,z0=CM_DIAG_Z0+dz*t0,x1=CM_DIAG_X0+dx*t1,z1=CM_DIAG_Z0+dz*t1;
        float mx=(x0+x1)*.5f,mz=(z0+z1)*.5f;
        if(!nearby(mx,mz,700))continue;
        if(cm_on_grid_road(x0,z0)||cm_on_grid_road(x1,z1)||cm_on_grid_road(mx,mz))continue;
        quad(GRASS,point(x0-nx*3,.45f,z0-nz*3),point(x1-nx*3,.45f,z1-nz*3),point(x1+nx*3,.45f,z1+nz*3),point(x0+nx*3,.45f,z0+nz*3),COLOR(140,190,120),1,2);
        quad(SIDEWALK,point(x0-nx*3.6f,.4f,z0-nz*3.6f),point(x1-nx*3.6f,.4f,z1-nz*3.6f),point(x1-nx*3,.4f,z1-nz*3),point(x0-nx*3,.4f,z0-nz*3),0xffffffffu,1,1);
        quad(SIDEWALK,point(x0+nx*3,.4f,z0+nz*3),point(x1+nx*3,.4f,z1+nz*3),point(x1+nx*3.6f,.4f,z1+nz*3.6f),point(x0+nx*3.6f,.4f,z0+nz*3.6f),0xffffffffu,1,1);
        if(view_distance(mx,mz)<430&&(k&1))tree(mx,mz);
        if(!(k%3)){box(x0-nx*2,z0-nz*2,.4f,1.2f,1.2f,30,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(x0-nx*2,z0-nz*2,29,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));}
    }
}
/* v2.7: farola con brazo curvo (dos tramos) y cabeza; banco con respaldo y patas. */
static void lamp_post(float x,float z,float angle){
    float saved=fixedGround;if(saved<=-999999)fixedGround=geo_height(x,z);
    box(x,z,0,1.6f,1.6f,30,angle,METAL,METAL,COLOR(120,124,128));
    Point a=local(2.5f,0,0,x,z,angle);box(a.x,a.z,29.2f,6,1.2f,1.2f,angle,METAL,METAL,COLOR(120,124,128));
    Point b=local(6.5f,0,0,x,z,angle);box(b.x,b.z,30.5f,3,1.2f,1.2f,angle+.5f,METAL,METAL,COLOR(120,124,128));
    Point h=local(7.5f,0,0,x,z,angle);frustum(h.x,h.z,28.6f,1.2f,2.6f,2.4f,6,0,METAL,COLOR(60,64,70));box(h.x,h.z,27.9f,3,3,.8f,angle,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
    fixedGround=saved;
}
static void bench(float x,float z,float angle){
    box(x,z,3.2f,10,3.2f,.8f,angle,ROOF,ROOF,COLOR(120,85,50));
    Point b=local(0,0,-1.9f,x,z,angle);box(b.x,b.z,4,10,.7f,3.6f,angle,ROOF,ROOF,COLOR(120,85,50));
    for(int e=-1;e<=1;e+=2){Point p=local(e*4.2f,0,0,x,z,angle);box(p.x,p.z,0,.8f,3,3.2f,angle,METAL,METAL,COLOR(60,62,66));}
}
/* Two pedestrian crossings link the riverside promenades between the road
   bridges. Their clear centre and parapets share citymap.h collision bounds. */
static void river_footbridge(int bz){
    float cz=cm_footbridge_z(bz);if(cz<0)return;
    box(1428,cz,-5,72,32,7,0,SIDEWALK,SIDEWALK,COLOR(202,194,177));
    ground(COBBLE,1392,cz-13,72,26,2.1f,COLOR(226,217,194),4);
    for(int side=-1;side<=1;side+=2){
        float zz=cz+side*14.5f;
        box(1428,zz,2,72,2,2,0,CONCRETE,CONCRETE,COLOR(200,191,170));
        for(int k=0;k<7;k++)box(1394+k*11,zz,4,1.5f,1.5f,7,0,METAL,METAL,COLOR(64,74,77));
        box(1428,zz,11,72,1.5f,1.5f,0,METAL,METAL,COLOR(72,81,84));
    }
    box(1400,cz,-12,6,34,7,0,BRICK,BRICK,COLOR(185,175,158));
    box(1456,cz,-12,6,34,7,0,BRICK,BRICK,COLOR(185,175,158));
}
/* Each terminal replaces the ordinary building on its parcel. No geometry
   intersects a storefront, and the existing parcel remains its solid base. */
static void cable_station_building(const CmParcel *p,int terminal,int far){
    float cx,cz;parcel_centroid(p,&cx,&cz);
    float w=fminf(p->x[1]-p->x[0]-12,48.f),d=fminf(p->z[2]-p->z[0]-18,68.f);
    uint32_t paint=terminal?COLOR(228,182,84):COLOR(66,174,181);
    box(cx,cz,6,w,d,34,0,MODERN,CONCRETE,COLOR(218,230,222));
    box(cx,cz,40,w+2,d+3,4,0,METAL,METAL,paint);
    if(far)return;
    for(int side=-1;side<=1;side+=2){
        box(cx+side*(w*.5f+.1f),cz,11,.8f,d-8,24,0,GLASS,GLASS,COLOR(85,155,178));
        box(cx+side*(w*.5f+1.f),cz,34,2,d-6,2,0,METAL,METAL,paint);
    }
    box(cx,cz-d*.5f+.9f,13,w-10,1.5f,20,0,GLASS,GLASS,COLOR(83,149,169));
    box(cx,cz-d*.5f+.4f,35,w-7,1.4f,4,0,CAR_PAINT,CAR_PAINT,paint);
    box(cx,cz,44,w*.55f,d*.6f,3,0,METAL,METAL,COLOR(69,88,96));
}
static int road_mark_clear(float x,float z,float w,float d){
    for(int i=0;i<5;i++){
        float px=x+(i==1||i==2?w:i==4?w*.5f:0);
        float pz=z+(i==2||i==3?d:i==4?d*.5f:0);
        if(!cm_on_grid_road(px,pz)||cm_on_diag(px,pz))return 0;
    }
    return 1;
}
static void city_cell_v26(int bx,int bz,int far,uint32_t white){
    float x=bx*320,z=bz*320;unsigned f=cm_flags(bx,bz),h=city_hash(bx,bz,7);int type=city_block_type(bx,bz);
    ground(ROAD,x,z,320,320,0,white,12);
    if(cm_roundabout(bx,bz)){
        float cx=x+42,cz=z+42;
        /* The surrounding 86-unit junction remains a usable driving lane. */
        cylinder(cx,cz,.6f,12,2.2f,16,0,SIDEWALK,SIDEWALK,COLOR(216,207,187));
        cylinder(cx,cz,2.8f,9,1.5f,16,0,GRASS,GRASS,COLOR(95,147,84));
        if(!far){
            for(int k=0;k<8;k++){
                float a=(k+.5f)*PI*.25f;
                cylinder(cx+cosf(a)*8,cz+sinf(a)*8,4.3f,1.2f,2.2f,6,0,GRASS,GRASS,COLOR(48,108,55));
            }
        }
    }
    int westMerged=(cm_flags(bx-1,bz)&CM_MERGE_E)!=0,northMerged=(cm_flags(bx,bz-1)&CM_MERGE_S)!=0;
    if(!far){
        /* Leave junctions and diagonal crossings clear. Stable dash phase
           and colour belong to the street, not a random building block. */
        for(int k=96;k<304;k+=32){
            if(!northMerged&&road_mark_clear(x+k,z+41.4f,16,1.2f))
                ground(FLAT,x+k,z+41.4f,16,1.2f,.45f,COLOR(244,208,100),1);
            if(!westMerged&&road_mark_clear(x+41.4f,z+k,1.2f,16))
                ground(FLAT,x+41.4f,z+k,1.2f,16,.45f,COLOR(244,208,100),1);
        }
        if(!cityMid){if((bx==0||bx==3||bx==5)&&(bz==0||bz==3))traffic_light(x+82,z+82,0);if((h>>9)%3==0&&!westMerged)bus_stop(x+116,z+80);}
        if(cityMid){box(x+82,z+59,0,1.5f,1.5f,34,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(x+79,z+59,33,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));}
        else lamp_post(x+84,z+59,PI);
        if(!cityMid&&(h&1024))cylinder(x+90,z+70,1.2f,2.2f,5,6,0,METAL,METAL,COLOR(70,120,80)); /* v2.7: papelera */
    }
    const CmParcel *ps;int n=cm_parcels(bx,bz,&ps);
    for(int i=0;i<n;i++){
        const CmParcel *p=&ps[i];uint32_t swc=((f&CM_PLAZA)||type==CB_TOWER)?COLOR(226,222,214):white;
        /* acera: sigue el terreno (ground) en parcelas rectangulares; prisma en las poligonales */
        if(cm_is_rect(p)){
            ground(SIDEWALK,p->x[0]-8,p->z[0]-8,p->x[1]-p->x[0]+16,p->z[2]-p->z[0]+16,1.2f,swc,8);
            if(!far)rect_curbs(p);
        }
        else{CmParcel sw;parcel_shrunk(p,-8,&sw);if(p->kind==1)slab_draped(&sw,1.2f,SIDEWALK,swc);else prism(&sw,0,1.2f,SIDEWALK,SIDEWALK,swc);}
        /* parcelas edificadas: todo comparte el nivel del podio (sin z-fighting en ladera). Las transitables (parques,
           plazas) siguen el terreno, porque el jugador camina sobre ellas a la altura del terreno. */
        float savedLevel=fixedGround;if(p->kind==0)fixedGround=parcel_level(p);
        if(p->kind==0&&(cm_point_in(p,CM_CABLE_X0,CM_CABLE_Z0)||cm_point_in(p,CM_CABLE_X1,CM_CABLE_Z1))){
            prism(p,0,6,CONCRETE,CONCRETE,COLOR(203,209,204));
            cable_station_building(p,cm_point_in(p,CM_CABLE_X1,CM_CABLE_Z1),far);
        }
        else if(f&CM_PLAZA)fill_plaza(p,far,h);
        else if(p->kind==1&&bx==4&&bz==6)fill_parking_park(p,far,h);
        else if(p->kind==1)fill_park(p,far,h);
        else if(f&CM_PUEBLITO)fill_pueblito(p,far,h);
        else if(f&CM_STADIUM){prism(p,0,6,SIDEWALK,SIDEWALK,white);fill_stadium(p,far,h);}
        else{
            uint32_t podium=type==CB_COMUNA?COLOR(190,180,170):type==CB_TOWER?COLOR(215,212,205):white;
            prism(p,0,6,type==CB_COMUNA?BRICK:CONCRETE,type==CB_TOWER?CONCRETE:ROOF,type==CB_COMUNA?podium:0xffffffffu);
            if(cm_is_rect(p)){int t=(n>1&&city_hash(bx,bz,90+i)%4==0)?city_block_type(bx,bz+1):type;fill_rect(bx,bz,p->x[0],p->z[0],p->x[1],p->z[2],t,far,h^city_hash(bx,bz,100+i),0);}
            else fill_poly(p,bx,bz,far,h^city_hash(bx,bz,100+i));
        }
        fixedGround=savedLevel;
    }
    if((f&CM_TUNNEL)&&(f&CM_MERGE_E)&&n>=2)tunnel_building(bx,ps[0].z[0],ps[0].z[2],far);
    /* Pasarela hacia la manzana del este (cruza la calle a 38 de altura). */
    if(!far&&bx<7&&bx!=3&&!(f&CM_MERGE_E)&&n==1&&cm_is_rect(&ps[0])&&(h>>12)%4==0&&(type==CB_TOWER||type==CB_COMMERCIAL||type==CB_CLASSIC)&&!cm_park(bx+1,bz)){
        const CmParcel *pe;int ne=cm_parcels(bx+1,bz,&pe);int t2=city_block_type(bx+1,bz);
        if(ne>=1&&cm_is_rect(&pe[0])&&(t2==CB_TOWER||t2==CB_COMMERCIAL||t2==CB_CLASSIC))skybridge(ps[0].x[1],pe[0].x[0],z+187,10);
    }
}
/* v2.31 (Claude): ver CACHE DE LA CIUDAD en render3d.c. */
static void replay_range(const unsigned char *p,const unsigned char *end){
    while(p<end){RecHdr h;memcpy(&h,p,4);p+=4;
        if(h.kind==0){Vertex tmp[16];memcpy(tmp,p,h.count*sizeof(Vertex));p+=h.count*sizeof(Vertex);polygon_emit(h.mat,tmp,h.count,h.lit);}
        else if(h.kind<3){Vertex t[3];memcpy(t,p,sizeof t);p+=sizeof t;fx_triangle(t[0],t[1],t[2],h.kind==2);}
        else{float q[3];memcpy(q,p,sizeof q);p+=sizeof q;int nc=noClip;noClip=0;litAlready=0;traffic_light(q[0],q[1],q[2]);noClip=nc;}
    }
}
static void cache_replay(const CellCache *c){
    float savedGround=fixedGround;int savedLit=litAlready,savedRigid=rigid;
    fixedGround=-1000000;rigid=0;
    const unsigned char *base=cachePool+c->off;
    for(unsigned k=0;k<c->nchunks;k++){const CacheChunk *ch=&chunkPool[c->chunk0+k];
        /* una prueba por trozo: fuera de la vista se salta entero; dentro del todo, sin recorte */
        int inside=1,out=0;
        for(int q=0;q<6;q++){float d=planes[q][0]*ch->x+planes[q][1]*ch->y+planes[q][2]*ch->z+planes[q][3];if(d<-ch->r){out=1;break;}if(d<ch->r)inside=0;}
        if(out)continue;
        noClip=inside;replay_range(base+ch->off,base+ch->off+ch->len);noClip=0;
    }
    fixedGround=savedGround;litAlready=savedLit;rigid=savedRigid;
}
/* Divide lo grabado de una manzana en trozos de ~100 vertices con su esfera envolvente. */
static int cache_chunks(CellCache *cc){
    const unsigned char *base=cachePool+cc->off,*p=base,*end=base+cc->len;
    cc->chunk0=chunkUsed;cc->nchunks=0;
    while(p<end){
        if(chunkUsed>=CHUNK_MAX)return 0;
        CacheChunk *ch=&chunkPool[chunkUsed];const unsigned char *st=p;int verts=0;
        float mnx=1e9f,mny=1e9f,mnz=1e9f,mxx=-1e9f,mxy=-1e9f,mxz=-1e9f;
        while(p<end&&verts<100){RecHdr h;memcpy(&h,p,4);p+=4;int n=h.kind==0?h.count:h.kind<3?3:0;
            if(h.kind==3){float q[3];memcpy(q,p,sizeof q);p+=sizeof q;float gx,gz;geo_project(q[0],q[1],&gx,&gz);float gy=geo_height(q[0],q[1]);
                mnx=fminf(mnx,gx-4);mxx=fmaxf(mxx,gx+4);mnz=fminf(mnz,gz-4);mxz=fmaxf(mxz,gz+4);mny=fminf(mny,gy);mxy=fmaxf(mxy,gy+34);continue;}
            for(int i=0;i<n;i++){Vertex v;memcpy(&v,p+i*sizeof(Vertex),sizeof v);
                mnx=fminf(mnx,v.x);mxx=fmaxf(mxx,v.x);mny=fminf(mny,v.y);mxy=fmaxf(mxy,v.y);mnz=fminf(mnz,v.z);mxz=fmaxf(mxz,v.z);}
            p+=n*sizeof(Vertex);verts+=n;}
        ch->off=(unsigned)(st-base);ch->len=(unsigned)(p-st);
        ch->x=(mnx+mxx)*.5f;ch->y=(mny+mxy)*.5f;ch->z=(mnz+mxz)*.5f;
        ch->r=sqrtf((mxx-mnx)*(mxx-mnx)+(mxy-mny)*(mxy-mny)+(mxz-mnz)*(mxz-mnz))*.5f+.5f;
        chunkUsed++;cc->nchunks++;
    }
    return 1;
}
typedef void (*PieceFn)(int,int,int,uint32_t);
static void cached_piece(int slot,int lod,PieceFn fn,int bx,int bz,int far,uint32_t white,int *budget){
    CellCache *cc=&cellCache[slot][lod];
    float age=fabsf(cc->day-dayT);if(age>.5f)age=1-age;
    if(cc->valid&&(age<.006f||*budget<=0)){cache_replay(cc);return;}
    if(*budget<=0){fn(bx,bz,far,white);return;}
    (*budget)--;
    if(cacheUsed+262144>CITY_CACHE_BYTES||chunkUsed+2000>CHUNK_MAX){memset(cellCache,0,sizeof cellCache);cacheUsed=0;chunkUsed=0;} /* lleno: se vacia y se rehace poco a poco */
    unsigned start=cacheUsed;float savedGround=fixedGround;int savedLit=litAlready;
    cacheRec=1;recFail=0;recDist=lod==0?100.f:lod==1?300.f:500.f;recRange=lod==0?0.f:lod==1?250.f:420.f;
    fn(bx,bz,far,white);
    cacheRec=0;fixedGround=savedGround;litAlready=savedLit;
    if(recFail){cacheUsed=start;cc->valid=0;fn(bx,bz,far,white);return;}
    unsigned len=cacheUsed-start;
    if(cc->valid&&len<=cc->len&&cc->off+cc->len<=start){memmove(cachePool+cc->off,cachePool+start,len);cc->len=len;cacheUsed=start;} /* reutiliza su hueco */
    else{cc->off=start;cc->len=len;}
    cc->day=dayT;cc->valid=1;
    if(!cache_chunks(cc)){cc->valid=0;fn(bx,bz,far,white);return;}
    cache_replay(cc);
}
static void cell_piece(int bx,int bz,int far,uint32_t white){city_cell_v26(bx,bz,far,white);}
static float river_centre(float z){return 1428+6*sinf(z/145.f)+2*sinf(z/53.f);}
static void river_channel(int bz,uint32_t white){
    /* Shared endpoints, including beneath every bridge: no disconnected rows.
       Dense strips also follow the geographic bends instead of spanning them. */
    for(int k=0;k<20;k++){
        float z0=bz*320+k*16,z1=z0+16,c0=river_centre(z0),c1=river_centre(z1);
        quad(WATER,point(c0-24,1.4f,z0),point(c1-24,1.4f,z1),point(c1+24,1.4f,z1),point(c0+24,1.4f,z0),white,2,1);
        if(z1<=bz*320+86)continue; /* The road deck occupies this bank section. */
        z0=fmaxf(z0,bz*320+86);c0=river_centre(z0);
        quad(CONCRETE,point(1396,1.5f,z0),point(1396,1.5f,z1),point(c1-24,1.5f,z1),point(c0-24,1.5f,z0),COLOR(166,163,145),1,1);
        quad(CONCRETE,point(c0+24,1.5f,z0),point(c1+24,1.5f,z1),point(1460,1.5f,z1),point(1460,1.5f,z0),COLOR(166,163,145),1,1);
        for(int side=0;side<2;side++){
            if(fabsf((z0+z1)*.5f-cm_footbridge_z(bz))<24)continue;
            float xx=side?1461:1395;
            quad(SIDEWALK,point(xx,0,z0),point(xx,0,z1),point(xx,8,z1),point(xx,8,z0),COLOR(203,202,184),1,1);
            quad(SIDEWALK,point(xx-1,8,z0),point(xx-1,8,z1),point(xx+1,8,z1),point(xx+1,8,z0),white,1,1);
        }
    }
}
static void river_row(int bx,int bz,int far,uint32_t white){(void)bx;
    river_channel(bz,white);
    river_bridge(bz,white);
    river_footbridge(bz);
    ground(SIDEWALK,1462,bz*320+86,44,234,1.2f,COLOR(226,222,214),6);
    if(!far)for(int k=0;k<3;k++){tree(1388,bz*320+120+k*60);if(!cm_metro_station(bz))tree(1466,bz*320+150+k*60);}
    metro(bz,far);
}
static void city_v26(void){
    const uint32_t white=0xffffffffu;int cacheBudget=1;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320,z=bz*320;
        if(!nearby(x+160,z+160,720))continue;
        if(!nearby(x+160,z+160,400)&&!sphere_visible(x+160,z+160,460))continue;
        float dist=view_distance(x+160,z+160);int far=dist>380;cityMid=dist>230;
        {static char cellTag[20];cellTag[0]='c';cellTag[1]='0'+bx;cellTag[2]=',';cellTag[3]='0'+bz;cellTag[4]=far?'L':(cityMid?'M':'c');cellTag[5]=0;r3_phase(cellTag);r3_trace_cell(cellTag);}
        cached_piece(bz*8+bx,far?2:cityMid?1:0,cell_piece,bx,bz,far,white,&cacheBudget);cityMid=0;
    }
    diagonal_avenue();
    /* Rio con puentes reales en cada calle transversal, paseo de la orilla este y Metro. */
    for(int bz=0;bz<7;bz++)if(nearby(1428,bz*320+160,850)){
        int far=view_distance(1428,bz*320+160)>430;
        cached_piece(56+bz,far?2:0,river_row,0,bz,far,white,&cacheBudget); /* v2.31: tramo de rio/Metro en cache */
    }
    if(nearby(CM_METRO_X,view->metroZ,800))metro_train();
    {float cx=(CM_CABLE_X0+CM_CABLE_X1)*.5f,cz=(CM_CABLE_Z0+CM_CABLE_Z1)*.5f;if(nearby(cx,cz,1100))metrocable(view_distance(cx,cz)>600);}
    for(int side=0;side<2;side++)for(int row=0;row<28;row++){
        float x=side?2560:-320,z=row*80;
        if(nearby(x,z,1000))ground(GRASS,x,z,320,80,0,COLOR(125,167,132),5);
    }
}
