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
/* v2.33 (Claude): mobiliario sin invadir la calzada. La acera son las 8 unidades que rodean
   cada parcela, y las manzanas no empiezan todas en el mismo sitio (86-100): las posiciones
   fijas dejaban farolas, semaforos, paradas y papeleras en medio de la calle. */
static int block_at(float x,float z){return cm_parcel_at(x,z)>=0;}
/* Todo el circulo de radio m cae dentro de una parcela (para objetos de plazas y parques). */
static int inside_block(float x,float z,float m){
    if(!block_at(x,z))return 0;
    for(int k=0;k<12;k++){float a=k*(PI/6);if(!block_at(x+cosf(a)*m,z+sinf(a)*m))return 0;}
    return 1;
}
/* Punto de acera mas cercano para un objeto de radio c (c<4): fuera de toda parcela en su
   radio y con una parcela a menos de 8-c. Devuelve 0 si no hay acera cerca (no se dibuja). */
typedef struct{float x[4],z[4];signed char ok[4];}FurnMemo; /* 0 semaforo, 1 parada, 2 farola, 3 papelera */
static FurnMemo furnMemo[56];static unsigned char furnMemoDone[56];
static float takenX[16],takenZ[16],takenR[16],spotPad;static int nTaken; /* objetos ya colocados en la manzana; spotPad: brazo de la farola */
static int sidewalk_spot(float *x,float *z,float c){
    for(float r=0;r<=36;r+=1.5f)for(int k=0,n=r==0?1:(int)r+6;k<n;k++){ /* v2.36: busqueda mas ligera */
        float a=k*(2*PI/n),px=*x+cosf(a)*r,pz=*z+sinf(a)*r;
        if(block_at(px,pz)||cm_diag_dist(px,pz)<CM_DIAG_HALF+c+.5f)continue; /* v2.37: sin sobresalir a la diagonal */
        int busy=0;for(int q=0;q<nTaken;q++)if(hypotf(px-takenX[q],pz-takenZ[q])<c+spotPad+takenR[q]+1.f)busy=1; /* sin objetos encimados */
        if(busy)continue;
        int clear=1,near=0;
        for(int q=0;q<12&&clear;q++){float b=q*(PI/6);
            if(block_at(px+cosf(b)*c,pz+sinf(b)*c))clear=0;
            if(block_at(px+cosf(b)*(8-c),pz+sinf(b)*(8-c)))near=1;}
        if(clear&&near){*x=px;*z=pz;if(nTaken<16){takenX[nTaken]=px;takenZ[nTaken]=pz;takenR[nTaken]=c+spotPad;nTaken++;}return 1;}
    }
    return 0;
}
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
        int parts=(warped&&len>24)?(int)ceilf(len/12):1; /* v2.33: trozos de 12 (antes 40) */
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
        int nzp=(int)ceilf((z1-z0)/12),nxp=(int)ceilf((x1-x0)/40);if(nzp<1)nzp=1;if(nxp<1)nxp=1;
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
    /* v2.39 (Claude): visto desde la calle (mirando hacia +z) la derecha es -x: la u crece hacia -x.
       Antes crecia hacia +x y los nombres se leian en espejo ("MIGA" -> "AGIM"). */
    Vertex sign[4]={{u+.5f-eps,v+eps,tint,cx-w*.5f+2,23.8f,z0+.35f},
        {u+eps,v+eps,tint,cx+w*.5f-2,23.8f,z0+.35f},
        {u+eps,v+.5f-eps,tint,cx+w*.5f-2,6.2f,z0+.35f},
        {u+.5f-eps,v+.5f-eps,tint,cx-w*.5f+2,6.2f,z0+.35f}};
    litAlready=1;polygon(mat,sign,4);litAlready=0;
    box(cx,z0-.4f,17.4f,w-3,5,1.2f,0,AWNING,AWNING,trim); /* v2.41: toldo saliente (z0-2,9..z0+2,1); letrero a 0,65 de la fachada (antes 0,15: parpadeaba) */ /* v2.33 (Claude): sin caras coplanares (parpadeo): tools/qa_zfight_v233.c */
    if(w>42){
        float xx=cx+w*.23f-5;
        box(xx,z0+1.1f,7,2,1,13,0,METAL,METAL,COLOR(225,210,175)); /* v2.33: por delante de la fachada */
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
            {int nb=w>=110?3:w>=80?2:1;float span=w-60; /* v2.35 (Claude): en parques estrechos los 3 bancos caian en el mismo sitio */
             for(int k=0;k<nb;k++)bench(nb==1?(x0+x1)*.5f:x0+30+k*span/(nb-1),cz-16,0);}}
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
    if(!cityMid){if(x1-x0>=74){bench(x0+30,cz+30,0);bench(x1-30,cz+30,0);}else bench((x0+x1)*.5f,cz+30,0);} /* v2.35: sin bancos encimados */
}
/* Plaza Botero: plaza dura con esculturas de bronce sobre pedestales, palmas y fuente. */
static void fill_plaza(const CmParcel *p,int far,unsigned h){
    float cx,cz;parcel_centroid(p,&cx,&cz);
    CmParcel s;parcel_shrunk(p,2,&s);slab_draped(&s,1.7f,COBBLE,0xffffffffu);
    if(far)return;
    CmParcel g;parcel_shrunk(p,22,&g);
    for(int i=0;i<g.n;i++){float sx=(g.x[i]+cx)*.5f,sz=(g.z[i]+cz)*.5f;
        if(!inside_block(sx,sz,6))continue; /* v2.33: nada fuera de la plaza */
        box(sx,sz,1.6f,8,8,3,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));
        box(sx,sz,4.6f,5,4,6,(i*.7f),CAR_PAINT,CAR_PAINT,COLOR(70,60,40));box(sx,sz,10.6f,3.4f,3.4f,3,0,CAR_PAINT,CAR_PAINT,COLOR(70,60,40)); /* figura rotunda */
        if(!cityMid&&inside_block(g.x[i],g.z[i],3))tree(g.x[i],g.z[i]);}
    cylinder(cx,cz,1.6f,8,1.2f,8,0,CONCRETE,CONCRETE,0xffffffffu);cylinder(cx,cz,2.8f,6,3.5f,8,.39f,WATER,WATER,0xffffffffu);cylinder(cx,cz,2.8f,1.4f,8,6,0,CONCRETE,CONCRETE,0xffffffffu); /* fuente redonda */
    for(int k=0;k<4;k++)if(inside_block(cx-30+k*20,cz+24,6))bench(cx-30+k*20,cz+24,0); /* v2.33: bancos solo dentro de la plaza */
    if(inside_block(cx+34,cz-20,10)){cylinder(cx+34,cz-20,1.6f,6,9,6,0,STUCCO,ROOF,cityPastel[h&7]);frustum(cx+34,cz-20,10.6f,8,0,6,6,0,ROOF,COLOR(150,90,70));} /* kiosco */
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
static Point metro_point(float x,float z,float y){
 float gx,gz;geo_project(x,z,&gx,&gz);
 return point(gx,geo_height(CM_METRO_X,z)+y,gz);
}
static float metro_next_z(float z,float end){
 float row=floorf(z/320.f),local=z-row*320.f;
 float limit=local<86.f?row*320.f+86.f:geo_next_z(z);
 float step=local<86.f?12.f:48.f;
 float next=fminf(end,fminf(z+step,limit));
 return next<=z+.001f?fminf(end,z+step):next;
}
/* Adjacent rows share exactly the same projected edge coordinates and rail
   height. A long box used its highest corner as one flat foundation, creating
   seams whenever the route crosses a valley terrace. */
static void metro_ribbon(float x,float z0,float z1,float width,float bottom,float height,int side,int top,uint32_t color){
 int saved=geographic;geographic=0;
 float left=x-width*.5f,right=x+width*.5f,upper=bottom+height;
 for(float a=z0;a<z1-.001f;){
  float b=metro_next_z(a,z1);
  Point la=metro_point(left,a,upper),lb=metro_point(left,b,upper);
  Point ra=metro_point(right,a,upper),rb=metro_point(right,b,upper);
  Point lda=metro_point(left,a,bottom),ldb=metro_point(left,b,bottom);
  Point rda=metro_point(right,a,bottom),rdb=metro_point(right,b,bottom);
  quad(top,la,lb,rb,ra,color,width/12.f,(b-a)/12.f);
  quad(side,lda,ldb,lb,la,shade(color,.75f),1,(b-a)/12.f);
  quad(side,rb,ra,rda,rdb,shade(color,.68f),1,(b-a)/12.f);
  if(a<=z0+.001f)quad(side,la,ra,rda,lda,shade(color,.71f),1,1);
  if(b>=z1-.001f)quad(side,lb,ldb,rdb,rb,shade(color,.71f),1,1);
  a=b;
 }
 geographic=saved;
}
static void rail_canopy(float x,float z,float width,float length,float y,float rise){
    /* Station roofs follow every curve and terrace at the same 16-unit
       sampling as the rail deck. Moving carriage roofs keep their existing
       flexible topology and UVs. */
    if(geographic&&!metroFlexible){int saved=geographic;geographic=0;
        float z0=z-length*.5f,z1=z+length*.5f;litAlready=1;
        for(float za=z0;za<z1-.001f;){
            float zb=metro_next_z(za,z1);
            for(int k=0;k<4;k++){
                float a=PI*k/4,b=PI*(k+1)/4;
                float xa=x+cosf(a)*width*.5f,xb=x+cosf(b)*width*.5f;
                float ya=y+sinf(a)*rise,yb=y+sinf(b)*rise;
                quad(GLASS,metro_point(xa,za,ya),metro_point(xa,zb,ya),
                    metro_point(xb,zb,yb),metro_point(xb,za,yb),
                    lit_color(COLOR(111,153,167),cosf((a+b)*.5f),sinf((a+b)*.5f),0),1,1);
            }
            za=zb;
        }
        litAlready=0;geographic=saved;return;
    }

    float saved=fixedGround;if(saved<=-999999)fixedGround=geo_height(x,z);litAlready=1;
    for(int k=0;k<4;k++){
        float a=PI*k/4,b=PI*(k+1)/4;
        float xa=x+cosf(a)*width*.5f,xb=x+cosf(b)*width*.5f,ya=y+sinf(a)*rise,yb=y+sinf(b)*rise;
        quad(METAL,point(xa,ya,z-length*.5f),point(xa,ya,z+length*.5f),point(xb,yb,z+length*.5f),point(xb,yb,z-length*.5f),lit_color(COLOR(210,224,215),cosf((a+b)*.5f),sinf((a+b)*.5f),0),length/45,.5f);
        /* GE culling is disabled: one surface is already visible from both sides.
           The old reversed duplicate fought for the same depth and shimmered. */
    }
    litAlready=0;fixedGround=saved;
}
static void metro_train(void){
    for(int c=0;c<2;c++){float zc=view->metroZ+(c-.5f)*(CM_TRAIN_CAR+6);
        if(!nearby(CM_METRO_X,zc,700))continue;
        float previousGround=fixedGround;fixedGround=geo_height(CM_METRO_X,zc);
        metroBaseGround=fixedGround;metroFlexible=1;
        float fl=CM_PLAT_H+.2f;
        box(CM_METRO_X,zc,fl+.15f,20,CM_TRAIN_CAR,1.05f,0,SIDEWALK,SIDEWALK,COLOR(120,120,124));                 /* piso (v2.33: separado de los rieles) */
        box(CM_METRO_X,zc,fl-.6f,16,CM_TRAIN_CAR-8,.6f,0,CAR_PAINT,CAR_PAINT,COLOR(60,62,66));                  /* bajos (v2.33: bajo el tope de los rieles) */
        for(int sd=-1;sd<=1;sd+=2){float x=CM_METRO_X+sd*9.6f;
            box(x,zc,fl+1.2f,.8f,CM_TRAIN_CAR,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));                  /* panel bajo */
            box(x,zc,fl+5.4f,1.0f,CM_TRAIN_CAR-.4f,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80));               /* franja verde (v2.33: sin testeros coplanares) */
            for(int k=-2;k<=2;k++){float zz=zc+k*22;box(x,zz,fl+9.2f,.8f,2,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));} /* pilares de ventana */
            for(int door=-1;door<=1;door+=2)for(int panel=-1;panel<=1;panel+=2){
                float dz=zc+door*33+panel*(2.4f+view->metroDoors*5);
                box(x+sd*.25f,dz,fl+1.2f,.9f,4.7f,17,0,FLAT,FLAT,COLOR(218,229,222));
                box(x+sd*.77f,dz+panel*2.23f,fl+1.3f,.08f,.16f,16.4f,0,FLAT,FLAT,COLOR(39,52,53)); /* rubber door seam */
                box(x+sd*.80f,dz-panel*1.4f,fl+7.8f,.10f,.26f,1.1f,0,FLAT,FLAT,COLOR(78,99,96)); /* recessed grab handle */
                if(!view->inMetro)box(x+sd*.8f,dz,fl+10,.12f,3.2f,5.4f,0,GLASS,GLASS,COLOR(101,143,147)); /* v2.33: por fuera de la puerta */
            }
            box(x,zc,fl+18.2f,.8f,CM_TRAIN_CAR,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));             /* dintel */
        }
        /* Only the two fixed central windows stay on the shell. The outer
           spans are sliding doors, whose glass belongs to the moving panels
           above. The former four uninterrupted panes covered an open door. */
        if(!view->inMetro)for(int side=-1;side<=1;side+=2)
            for(int k=-1;k<=0;k++)box(CM_METRO_X+side*9.65f,zc+k*22+11,fl+9.3f,.35f,19,8.5f,0,GLASS,GLASS,COLOR(112,151,165));
        /* Short fixed panes flank each 9.4-unit doorway. They fill the rest
           of its 20-unit bay without drawing glass across the actual opening. */
        if(!view->inMetro)for(int side=-1;side<=1;side+=2)
            for(int door=-1;door<=1;door+=2)for(int flank=-1;flank<=1;flank+=2)
                box(CM_METRO_X+side*9.65f,zc+door*33+flank*7.55f,fl+9.3f,.35f,4.7f,8.5f,0,GLASS,GLASS,COLOR(112,151,165));
        rail_canopy(CM_METRO_X,zc,21,CM_TRAIN_CAR+1,fl+20,2.8f);
        if(view_distance(CM_METRO_X,zc)<250){
            box(CM_METRO_X,zc,fl+22.5f,13,24,2,0,METAL,METAL,COLOR(157,168,168));
            for(int bogie=-1;bogie<=1;bogie+=2)box(CM_METRO_X,zc+bogie*34,fl-.8f,17,13,1.2f,0,METAL,METAL,COLOR(58,63,67));
        }
        for(int e=-1;e<=1;e+=2){float ze=zc+e*CM_TRAIN_CAR*.5f;                                                   /* testeros abiertos: panel bajo, pilares y dintel */
            box(CM_METRO_X,ze,fl+1.2f,18.6f,1,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X,ze,fl+5.4f,18.8f,1.2f,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80)); /* v2.33: entre los paneles laterales */
            box(CM_METRO_X-7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X+7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));
            box(CM_METRO_X,ze,fl+18.2f,18.6f,1,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));
            if(!view->inMetro)box(CM_METRO_X,ze+e*.65f,fl+10.1f,13.6f,.35f,6.4f,0,GLASS,GLASS,COLOR(105,150,168));}
        if(view->inMetro||view_distance(CM_METRO_X,zc)<200){
            for(int k=-2;k<=2;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*6.5f,zc+k*20,fl+1.2f,5,12,4,0,CAR_PAINT,CAR_PAINT,COLOR(40,90,160)); /* asientos */
            for(int k=-2;k<=2;k++)box(CM_METRO_X,zc+k*20+10,fl+1.2f,1,1,18,0,CAR_PAINT,CAR_PAINT,COLOR(200,200,200));       /* barras */
        }
        metroFlexible=0;fixedGround=previousGround;
    }
}
/* Two-sided original station marker, built into the cached FLAT mesh.
   Pixel cells are geometry, not untranslated UI: METRO is the network's name. */
static void metro_marker(float x,float z,float bottom,int number){
 static const unsigned char letters[5][7]={
  {17,27,21,21,17,17,17},{31,16,16,30,16,16,31},{31,4,4,4,4,4,4},
  {30,17,17,30,20,18,17},{14,17,17,17,17,17,14}};
 box(x,z,bottom,20,.8f,5.8f,0,FLAT,FLAT,COLOR(10,48,54));
 for(int face=-1;face<=1;face+=2)for(int k=0;k<5;k++)for(int row=0;row<7;row++)for(int col=0;col<5;col++)
  if(letters[k][row]&(1<<(4-col))){float xx=x+face*(-8+(k*6+col)*.55f),zz=z+face*.43f,yy=bottom+4.9f-row*.55f;
   quad(FLAT,point(xx,yy,zz),point(xx+face*.48f,yy,zz),point(xx+face*.48f,yy+.48f,zz),point(xx,yy+.48f,zz),COLOR(225,246,186),1,1);}
 /* Color-coded station identifier, independent of the menu language. */
 for(int k=0;k<number;k++)box(x+8,z,bottom+.8f+k*.95f,.8f,1.0f,.5f,0,FLAT,FLAT,COLOR(250,200,60));
}
static void metro(int bz,int far){
 float z=bz*320;
 /* Supports stay off the east/west intersection (local z 0..86). Their
    48-unit rhythm and two continuous under-deck girders make the elevated
    structure read as a viaduct even where it spans a road without a pillar. */
 for(int k=0;k<5;k++)cylinder(CM_METRO_X,z+96+k*48,0,4,CM_METRO_DECK,6,0,CONCRETE,CONCRETE,0xffffffffu);
 for(int side=-1;side<=1;side+=2)
  metro_ribbon(CM_METRO_X+side*5.2f,z,z+320,1.3f,CM_METRO_DECK-4,4,CONCRETE,CONCRETE,COLOR(106,115,115));
 metro_ribbon(CM_METRO_X,z,z+320,18,CM_METRO_DECK,3,CONCRETE,SIDEWALK,COLOR(133,140,139));
 /* Rails remain visible in both LODs. A continuous silver cap replaces dark
    car paint; the station never draws pavement over this central track bed. */
 for(int side=-1;side<=1;side+=2)
  metro_ribbon(CM_METRO_X+side*6,z,z+320,.65f,CM_METRO_DECK+3.06f,.55f,METAL,FLAT,COLOR(206,220,222));
 if(!far){
  for(int k=0;k<16;k++)box(CM_METRO_X,z+10+k*20,CM_METRO_DECK+3.01f,13,1.1f,.16f,0,FLAT,FLAT,COLOR(60,70,69));
  if(!cm_metro_station(bz))for(int side=-1;side<=1;side+=2)
   metro_ribbon(CM_METRO_X+side*8.2f,z,z+320,.4f,CM_METRO_DECK+3,2,CONCRETE,CONCRETE,COLOR(153,163,158));
 }
 if(cm_metro_station(bz)){
  for(int side=-1;side<=1;side+=2){
   /* Left side extends to the original stair landing; right ends before buildings at x1506. */
   float width=side<0?9.5f:8.5f,cx=CM_METRO_X+side*(10.5f+width*.5f);
   metro_ribbon(cx,z+60,z+260,width,CM_PLAT_H-1.2f,1.2f,SIDEWALK,SIDEWALK,COLOR(226,222,214));
   metro_ribbon(CM_METRO_X+side*10.9f,z+60,z+260,.55f,CM_PLAT_H+.02f,.12f,FLAT,FLAT,COLOR(250,200,60));
   metro_ribbon(CM_METRO_X+side*16.5f,z+61,z+259,.55f,CM_PLAT_H,5.5f,METAL,METAL,COLOR(73,111,108));
  }
  for(int k=0;k<3;k++)for(int side=-1;side<=1;side+=2)cylinder(CM_METRO_X+side*15,z+80+k*80,0,2.4f,CM_PLAT_H-1.2f,6,0,CONCRETE,CONCRETE,0xffffffffu);
  for(int k=0;k<4;k++)for(int side=-1;side<=1;side+=2)box(CM_METRO_X+side*16.6f,z+70+k*60,CM_PLAT_H,1.2f,1.2f,26,0,METAL,METAL,COLOR(113,143,144));
  /* Platform canopies leave the central track open: the follow camera can
     see trains/rails instead of looking through one opaque full-width roof. */
  for(int side=-1;side<=1;side+=2){
   float roofX=CM_METRO_X+side*15;
   rail_canopy(roofX,z+160,8,208,CM_PLAT_H+24,2);
   /* Continuous dark fascia hides the raw canopy edges and identifies the
      platform as a station rather than an exposed piece of track. */
   metro_ribbon(CM_METRO_X+side*20.8f,z+56,z+264,.8f,CM_PLAT_H+23.6f,1.4f,METAL,METAL,COLOR(44,82,93));
   metro_ribbon(roofX,z+56,z+264,.55f,CM_PLAT_H+25.6f,.55f,METAL,METAL,COLOR(209,230,226));
  }
  if(!far){
   for(int side=-1;side<=1;side+=2)for(int k=0;k<4;k++)
    metro_ribbon(CM_METRO_X+side*15,z+74+k*57-.6f,z+74+k*57+.6f,8,CM_PLAT_H+24.2f,1.1f,METAL,METAL,COLOR(56,80,74));
   for(int side=-1;side<=1;side+=2)for(int k=0;k<3;k++){
    box(CM_METRO_X+side*15,z+96+k*64,CM_PLAT_H+2.5f,3,10,.8f,0,FLAT,FLAT,COLOR(50,137,121));
    box(CM_METRO_X+side*15,z+96+k*64,CM_PLAT_H,1,7,2.5f,0,METAL,METAL,COLOR(68,86,87));
   }
  }
  /* A narrower stair no longer intersects the body/doors of a stopped train. */
  for(int k=0;k<20;k++)box(1470.5f,z+316.5f-k*3,0,8,3,(k+1)*(CM_PLAT_H/20),0,STUCCO,SIDEWALK,COLOR(226,222,214));
  for(int side=-1;side<=1;side+=2)for(int k=0;k<4;k++){float zz=z+312-k*18;box(1470.5f+side*4.3f,zz,(318-(zz-z))*(CM_PLAT_H/60)+1,.6f,.6f,8,0,METAL,METAL,COLOR(113,143,144));}
  /* Leave half a unit between landing and final stair: their side planes used
     to overlap at x=1466 and shimmer in the PSP depth buffer. */
  box(1470.5f,z+255.5f,CM_PLAT_H-1.2f,8,4,1.2f,0,SIDEWALK,SIDEWALK,COLOR(226,222,214));
  if(!far){
   /* Ground entrance sign stands on its own posts, below the viaduct;
      no floating board or frame crosses the train/roof equipment envelope. */
   for(int side=-1;side<=1;side+=2)box(1470.5f+side*9.4f,z+314,0,.7f,.7f,24.8f,0,METAL,METAL,COLOR(113,143,144));
   metro_marker(1470.5f,z+314,19,(bz+1)/2);
  }
 }
}
/* Metrocable: pilonas, cable y cabinas entre (CM_CABLE_X0,Z0) y (CM_CABLE_X1,Z1). */
static void metrocable(int far){
    enum {N=5};float hs[N+1],px[N+1],pz[N+1];
    /* v2.34 (Claude): primero todas las pilonas. Antes la cruceta de cada una se orientaba hacia
       la siguiente leyendo px[i+1] antes de calcularlo (valor sin inicializar: angulo al azar). */
    for(int i=0;i<=N;i++){cm_cable_node(i,&px[i],&pz[i]);hs[i]=geo_height(px[i],pz[i])+96;}
    for(int i=0;i<=N;i++){
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
            float ox=-sinf(ang)*side*3.3f,oz=cosf(ang)*side*3.3f; /* v2.33: cristal separado de la cabina */
            abox(x+ox,z+oz,y-8.3f,5.9f,.24f,4.7f,ang,GLASS,COLOR(92,170,193));
            abox(x+ox,z+oz,y-8.45f,.45f,.4f,5.2f,ang,METAL,COLOR(224,228,220)); /* v2.33: sin base coplanar con el cristal */
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
    ground(COBBLE,1392,cz-13,72,26,2.3f,COLOR(226,217,194),4); /* v2.33: separado del tablero */
    for(int side=-1;side<=1;side+=2){
        float zz=cz+side*14.5f;
        box(1428,zz,2,71.6f,2,2,0,CONCRETE,CONCRETE,COLOR(200,191,170)); /* v2.33: sin extremos coplanares con el tablero */
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
        /* v2.33 (Claude): cada objeto se lleva al punto de acera mas cercano (ver sidewalk_spot).
           Las posiciones no cambian: se calculan una vez por manzana y se recuerdan (la busqueda
           es cara y las manzanas se reconstruyen al cambiar el nivel de detalle o la luz). */
        {FurnMemo *memo=furnMemo;unsigned char *memoDone=furnMemoDone;
         int ci=bz*8+bx;
         if(!memoDone[ci]){memoDone[ci]=1;nTaken=0;
            /* v2.35 (Claude): el coche aparcado y el terminal de cada punto de mision ocupan su sitio:
               antes la parada de bus o la papelera caian encima del coche aparcado. */
            for(int hb=0;hb<30&&nTaken<6;hb++){float hx=view->hubs[hb][0],hz=view->hubs[hb][1];
                if(hx==0&&hz==0)continue;if(hx<x-60||hx>x+380||hz<z-60||hz>z+380)continue;
                takenX[nTaken]=hx+54;takenZ[nTaken]=hz+18;takenR[nTaken]=12;nTaken++;
                {float tx,tz;cm_terminal_pos(hx,hz,&tx,&tz);takenX[nTaken]=tx;takenZ[nTaken]=tz;takenR[nTaken]=6;nTaken++;}}
            float tx=x+82,tz=z+82;memo[ci].ok[0]=(bx==0||bx==3||bx==5)&&(bz==0||bz==3)&&sidewalk_spot(&tx,&tz,1.5f);memo[ci].x[0]=tx;memo[ci].z[0]=tz;
            float sx=x+116,sz=z+80;spotPad=6;memo[ci].ok[1]=(h>>9)%3==0&&!westMerged&&sidewalk_spot(&sx,&sz,3.6f);spotPad=0;memo[ci].x[1]=sx;memo[ci].z[1]=sz; /* v2.35: la marquesina mide 18: reserva su largo */
            if(memo[ci].ok[1])for(float dx=-9;dx<=9&&memo[ci].ok[1];dx+=1.5f)for(float dz=-3.5f;dz<=3.5f;dz+=1.75f)if(block_at(sx+dx,sz+dz)){memo[ci].ok[1]=0;break;} /* v2.35: el rectangulo entero en la acera */
            float lx=x+84,lz=z+59;spotPad=6;memo[ci].ok[2]=sidewalk_spot(&lx,&lz,1.4f);spotPad=0;memo[ci].x[2]=lx;memo[ci].z[2]=lz;
            float px=x+90,pz=z+70;memo[ci].ok[3]=(h&1024)&&sidewalk_spot(&px,&pz,1.3f);memo[ci].x[3]=px;memo[ci].z[3]=pz;}
         if(!cityMid&&memo[ci].ok[0])traffic_light(memo[ci].x[0],memo[ci].z[0],0);
         if(!cityMid&&memo[ci].ok[1])bus_stop(memo[ci].x[1],memo[ci].z[1]);
         if(memo[ci].ok[2]){float lx=memo[ci].x[2],lz=memo[ci].z[2];
            if(cityMid){box(lx,lz,0,1.5f,1.5f,28,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(lx-7.5f,lz,27.5f,8,3,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));}
            else lamp_post(lx,lz,PI);}
         if(!cityMid&&memo[ci].ok[3])cylinder(memo[ci].x[3],memo[ci].z[3],1.2f,2.2f,5,6,0,METAL,METAL,COLOR(70,120,80));} /* v2.7: papelera */
    }
    const CmParcel *ps;int n=cm_parcels(bx,bz,&ps);
    for(int i=0;i<n;i++){
        const CmParcel *p=&ps[i];uint32_t swc=((f&CM_PLAZA)||type==CB_TOWER)?COLOR(226,222,214):white;
        /* acera: sigue el terreno (ground) en parcelas rectangulares; prisma en las poligonales */
        if(cm_is_rect(p)){
            ground(SIDEWALK,p->x[0]-8,p->z[0]-8,p->x[1]-p->x[0]+16,p->z[2]-p->z[0]+16,1.2f,swc,8);
            if(!far)rect_curbs(p);
        }
        /* v2.44 (Claude): la acera de las parcelas irregulares SIEMPRE sigue el terreno. Antes, alrededor de los
           edificios era un prisma plano a la altura del punto mas alto: en ladera quedaba un bloque elevado en el
           lado bajo que el jugador (que camina a la altura del terreno) atravesaba ("aceras en las faldas"). */
        else{CmParcel sw;parcel_shrunk(p,-8,&sw);slab_draped(&sw,1.2f,SIDEWALK,swc);}
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
        if(h.kind==0){polygon_emit(h.mat,(Vertex*)p,h.count,h.lit);p+=h.count*sizeof(Vertex);} /* v2.41: sin copia intermedia (polygon_emit ya no escribe en su entrada) */
        else if(h.kind<3){Vertex t[3];memcpy(t,p,sizeof t);p+=sizeof t;fx_triangle(t[0],t[1],t[2],h.kind==2);}
        else{float q[3];memcpy(q,p,sizeof q);p+=sizeof q;int nc=noClip;noClip=0;litAlready=0;traffic_light(q[0],q[1],q[2]);noClip=nc;}
    }
}
/* Retint the cached road/sidewalk records only when daylight has changed
   perceptibly. They are subsequently copied to the mesh unchanged, avoiding
   the old per-vertex multiply on every frame. The cache is rebuilt regularly
   from source geometry, so 8-bit rounding never accumulates indefinitely. */
static void cache_relight_ground(CellCache *c,uint32_t now){
 float light[3]={(float)(now&255),(float)((now>>8)&255),(float)((now>>16)&255)};
 float gain[3];int changed=0;
 for(int i=0;i<3;i++){
  gain[i]=light[i]/fmaxf(c->groundLight[i],1.f);
  if(fabsf(gain[i]-1.f)>.004f)changed=1;
 }
 if(!changed)return;
 unsigned char *p=cachePool+c->off,*end=p+c->len;
 while(p<end){RecHdr h;memcpy(&h,p,4);p+=4;
  if(h.kind==0){
   if((h.mat==ROAD||h.mat==SIDEWALK)&&h.lit)for(int j=0;j<h.count;j++){
    Vertex *v=(Vertex*)(p+j*sizeof(Vertex));uint32_t col=v->color;
    int r=(int)((col&255)*gain[0]+.5f),g=(int)(((col>>8)&255)*gain[1]+.5f),b=(int)(((col>>16)&255)*gain[2]+.5f);
    v->color=COLOR(r>255?255:r,g>255?255:g,b>255?255:b);
   }
   p+=h.count*sizeof(Vertex);
  }else if(h.kind<3)p+=3*sizeof(Vertex);else p+=3*sizeof(float);
 }
 for(int i=0;i<3;i++)c->groundLight[i]=light[i];
}
static void cache_replay(CellCache *c){
    float savedGround=fixedGround;int savedLit=litAlready,savedRigid=rigid;
    uint32_t now=lit_color(COLOR(160,160,160),0,1,0);
    cache_relight_ground(c,now);
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
/* v2.36 (Claude): cache llena -> se quitan solo las piezas que no se han dibujado en los ultimos 2 s
   y se compacta lo demas (antes se vaciaba entera y durante unos fotogramas se construia todo en
   vivo: los peores tirones al correr). Devuelve 1 si quedo sitio. */
static int cache_compact(void){
    enum{NS=64*3};int idx[NS],n=0;
    for(int i=0;i<NS;i++){CellCache *c=&cellCache[i/3][i%3];if(!c->valid)continue;
        if(r3Frame-c->used>120){c->valid=0;continue;}idx[n++]=i;}
    /* por desplazamiento en el pool (insercion: n es pequeno) */
    for(int a=1;a<n;a++){int v=idx[a],b=a-1;while(b>=0&&cellCache[idx[b]/3][idx[b]%3].off>cellCache[v/3][v%3].off){idx[b+1]=idx[b];b--;}idx[b+1]=v;}
    unsigned w=0;for(int a=0;a<n;a++){CellCache *c=&cellCache[idx[a]/3][idx[a]%3];if(c->off!=w)memmove(cachePool+w,cachePool+c->off,c->len);c->off=w;w+=c->len;}
    cacheUsed=w;
    for(int a=1;a<n;a++){int v=idx[a],b=a-1;while(b>=0&&cellCache[idx[b]/3][idx[b]%3].chunk0>cellCache[v/3][v%3].chunk0){idx[b+1]=idx[b];b--;}idx[b+1]=v;}
    unsigned cw=0;for(int a=0;a<n;a++){CellCache *c=&cellCache[idx[a]/3][idx[a]%3];if(c->chunk0!=cw)memmove(chunkPool+cw,chunkPool+c->chunk0,c->nchunks*sizeof(CacheChunk));c->chunk0=cw;cw+=c->nchunks;}
    chunkUsed=cw;
    return cacheUsed+262144<=CITY_CACHE_BYTES&&chunkUsed+2000<=CHUNK_MAX;
}
static int prefetchOnly;static int r3WarmBudget; /* v2.37: >0 durante la pantalla de carga */ /* v2.36: construir la pieza en la cache sin dibujarla (anticipo) */
static void cached_piece(int slot,int lod,PieceFn fn,int bx,int bz,int far,uint32_t white,int *budget){
    CellCache *cc=&cellCache[slot][lod];
    float age=fabsf(cc->day-dayT);if(age>.5f)age=1-age;
    /* v2.36: la luz solo se rehace si cambio de verdad (0,02 de dia = ~10 s) y como mucho una pieza
       cada 4 fotogramas: antes el 90% de las reconstrucciones eran por luz (tirones al correr). */
    if(cc->valid&&(age<.02f||*budget<=0||(r3Frame&3))){cc->used=r3Frame;cache_replay(cc);return;}
    if(*budget<=0){if(!prefetchOnly)fn(bx,bz,far,white);return;}
    (*budget)--;if(cc->valid)r3CacheLight++;else r3CacheNew++;
    if(cacheUsed+262144>CITY_CACHE_BYTES||chunkUsed+2000>CHUNK_MAX){if(!cache_compact()){memset(cellCache,0,sizeof cellCache);cacheUsed=0;chunkUsed=0;r3CacheReset++;}} /* lleno: compactar; solo si no basta se vacia */
    unsigned start=cacheUsed;float savedGround=fixedGround;int savedLit=litAlready;int oldValid=cc->valid;unsigned oldC0=cc->chunk0,oldN=cc->nchunks;
    cacheRec=1;recFail=0;recDist=lod==0?100.f:lod==1?300.f:500.f;recRange=lod==0?0.f:lod==1?250.f:420.f;
    fn(bx,bz,far,white);
    cacheRec=0;fixedGround=savedGround;litAlready=savedLit;
    if(recFail){cacheUsed=start;cc->valid=0;if(!prefetchOnly)fn(bx,bz,far,white);return;}
    unsigned len=cacheUsed-start;
    if(cc->valid&&len<=cc->len&&cc->off+cc->len<=start){memmove(cachePool+cc->off,cachePool+start,len);cc->len=len;cacheUsed=start;} /* reutiliza su hueco */
    else{cc->off=start;cc->len=len;}
    cc->day=dayT;uint32_t sample=lit_color(COLOR(160,160,160),0,1,0);
    cc->groundLight[0]=(float)(sample&255);cc->groundLight[1]=(float)((sample>>8)&255);cc->groundLight[2]=(float)((sample>>16)&255);
    cc->valid=1;
    if(!cache_chunks(cc)){cc->valid=0;if(!prefetchOnly)fn(bx,bz,far,white);return;}
    /* v2.36: reutilizar sus trozos antiguos si caben (antes cada reconstruccion llenaba la tabla) */
    if(oldValid&&cc->nchunks<=oldN&&oldC0+oldN<=cc->chunk0){memcpy(chunkPool+oldC0,chunkPool+cc->chunk0,cc->nchunks*sizeof(CacheChunk));chunkUsed=cc->chunk0;cc->chunk0=oldC0;}
    cc->used=r3Frame;if(!prefetchOnly)cache_replay(cc);
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
    ground(SIDEWALK,1462,bz*320+86,35.5f,234,1.2f,COLOR(226,222,214),6); /* v2.33: hasta 1497,5; desde 1498 es la acera de las manzanas (misma altura: parpadeaba) */
    if(!far)for(int k=0;k<3;k++){tree(1388,bz*320+120+k*60);if(!cm_metro_station(bz))tree(1466,bz*320+150+k*60);}
    metro(bz,far);
}
static void city_v26(void){
    const uint32_t white=0xffffffffu;int cacheBudget=r3WarmBudget>0?r3WarmBudget:1;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320,z=bz*320;
        if(!nearby(x+160,z+160,720))continue;
        if(!nearby(x+160,z+160,400)&&!sphere_visible(x+160,z+160,460))continue;
        float dist=view_distance(x+160,z+160);int far=dist>380;cityMid=dist>230;
        /* v2.36: margen de 25 en los cambios de detalle: si ya hay en cache el nivel vecino se sigue
           usando (antes una manzana en el limite se reconstruia una y otra vez al moverse) */
        {int lod=far?2:cityMid?1:0,alt=-1;
         if(fabsf(dist-230)<25)alt=lod==0?1:0;else if(fabsf(dist-380)<25)alt=lod==1?2:1;
         if(alt>lod&&!cellCache[bz*8+bx][lod].valid&&cellCache[bz*8+bx][alt].valid){lod=alt;far=lod==2;cityMid=lod>=1;}}
        {static char cellTag[20];cellTag[0]='c';cellTag[1]='0'+bx;cellTag[2]=',';cellTag[3]='0'+bz;cellTag[4]=far?'L':(cityMid?'M':'c');cellTag[5]=0;r3_phase(cellTag);r3_trace_cell(cellTag);}
        cached_piece(bz*8+bx,far?2:cityMid?1:0,cell_piece,bx,bz,far,white,&cacheBudget);cityMid=0;
    }
    /* v2.36 (Claude): anticipo. Si este fotograma no construyo nada, se prepara en la cache la
       manzana cercana (<300) que va a necesitar el detalle maximo y aun no lo tiene. Asi al cruzar
       de manzana corriendo ya esta lista y no hay un fotograma largo (el tiron al correr). */
    if(cacheBudget>0&&!(r3Frame&1)){
        int best=-1;float bd=1e9f;
        for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){float d=view_distance(bx*320+160,bz*320+160);
            if(d<300&&d<bd&&!cellCache[bz*8+bx][0].valid){bd=d;best=bz*8+bx;}}
        if(best>=0){int bx=best%8,bz=best/8;{int sp=prefetchOnly;prefetchOnly=1;cityMid=0;
            cached_piece(best,0,cell_piece,bx,bz,0,white,&cacheBudget);prefetchOnly=sp;cityMid=0;}}
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
        if(nearby(x,z,1000)){
            /* v2.33 (Claude): franjas en rebanadas de 8 a lo largo de z. La proyeccion es lineal en x
               (una sola rebanada lo cubre exacto) pero en las transiciones de fila el borde del valle
               salta mucho en z: con la teselacion de 64 el cesped de la ladera se montaba sobre la
               calle del borde oeste (zona donde empieza la partida). */
            uint32_t gc=lit_color(COLOR(125,167,132),0,1,0);litAlready=1;
            for(int k=0;k<10;k++){float z0=z+k*8,z1=z0+8;
                float v0=k*.125f,v1=v0+.125f; /* textura continua entre rebanadas */
                Vertex q[4]={{0,v0,gc,x,0,z0},{5,v0,gc,x+320,0,z0},{5,v1,gc,x+320,0,z1},{0,v1,gc,x,0,z1}};polygon(GRASS,q,4);}
            litAlready=0;
        }
    }
}
