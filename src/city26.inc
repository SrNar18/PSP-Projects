/* city26.inc — Trazado irregular real (Claude, v2.6). Se incluye desde render3d.c despues de city3d.inc.
 *
 * La huella la define src/citymap.h (misma fuente que solid() en game.c): manzanas fusionadas (dobles), partidas
 * por una calle central, triangulares/trapezoidales junto a la Av. Oriental, tuneles bajo edificios puente,
 * Estadio, Plaza Botero, Pueblito Paisa, viaducto y estaciones del Metro y Metrocable.
 * Reutiliza los helpers de city3d.inc (casas, tejados, jardines, puentes del rio, pasarelas) y los tipos de manzana
 * por barrio; los rellenos se adaptan al tamano real de cada parcela.
 */
#define parcel_centroid cm_centroid
#define parcel_shrunk cm_shrunk
/* Prisma vertical sobre un poligono convexo: lados iluminados por su normal, tapa iluminada como techo. */
static void prism(const CmParcel *p,float bottom,float height,int side,int top,uint32_t color){
    if(p->n<3)return;
    float savedGround=fixedGround;float cx,cz;parcel_centroid(p,&cx,&cz);float h=bottom+height;
    {float rr=0;for(int i=0;i<p->n;i++){float dx=p->x[i]-cx,dz=p->z[i]-cz,d=dx*dx+dz*dz;if(d>rr)rr=d;}if(geographic&&!rigid&&!sphere_visible(cx,cz,sqrtf(rr)+height*.5f+4))return;}
    if(geographic&&!rigid&&fixedGround>-999999){if(bottom==0)bottom=-1;} /* nivel de parcela ya impuesto */
    else if(geographic&&!rigid){
        float low=geo_height(cx,cz),high=low;
        for(int i=0;i<p->n;i++){float hh=geo_height(p->x[i],p->z[i]);if(hh<low)low=hh;if(hh>high)high=hh;}
        /* muestreo interior: en ladera el terreno puede subir dentro de la parcela mas que en sus esquinas */
        for(int i=0;i<p->n;i++){float hh=geo_height((p->x[i]+cx)*.5f,(p->z[i]+cz)*.5f);if(hh>high)high=hh;}
        fixedGround=high;if(bottom==0)bottom=low-high-1; /* cimiento hasta el terreno mas bajo; la tapa no cambia */
    }
    uint32_t sc=emissive_color(side,color);
    litAlready=1;
    for(int i=0;i<p->n;i++){int j=(i+1)%p->n;
        float ex=p->x[j]-p->x[i],ez=p->z[j]-p->z[i],len=sqrtf(ex*ex+ez*ez);if(len<.5f)continue;
        float nx=ez/len,nz=-ex/len;float mx=(p->x[i]+p->x[j])*.5f-cx,mz=(p->z[i]+p->z[j])*.5f-cz;if(mx*nx+mz*nz<0){nx=-nx;nz=-nz;}
        Point a=point(p->x[i],h,p->z[i]),b=point(p->x[j],h,p->z[j]),c=point(p->x[j],bottom,p->z[j]),d=point(p->x[i],bottom,p->z[i]);
        float u=len/90;if(u<1)u=1;float v=height/24;if(v<1)v=1;
        if(height>14)quad2(side,a,b,c,d,lit_color(sc,nx,0,nz),u,v);else quad(side,a,b,c,d,lit_color(sc,nx,0,nz),u,v);
    }
    Vertex tv[8];uint32_t tc=lit_color(color,0,1,0);
    for(int i=0;i<p->n;i++){Vertex vv={(p->x[i]-cx)/90,(p->z[i]-cz)/90,tc,p->x[i],h,p->z[i]};tv[i]=vv;}
    polygon(top,tv,p->n);
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
        #define SV(X,Z) {((X)-cx)/60,((Z)-cz)/60,tc,X,y,Z}
        Vertex t1[3]={SV(cx,cz),SV(m1x,m1z),SV(m3x,m3z)},t2[3]={SV(m1x,m1z),SV(ax,az),SV(m2x,m2z)},t3[3]={SV(m3x,m3z),SV(m2x,m2z),SV(bx,bz)},t4[3]={SV(m1x,m1z),SV(m2x,m2z),SV(m3x,m3z)};
        #undef SV
        polygon(mat,t1,3);polygon(mat,t2,3);polygon(mat,t3,3);polygon(mat,t4,3);
        Vertex sk[4]={{0,0,sc,ax,y,az},{1,0,sc,bx,y,bz},{1,1,sc,bx,-3,bz},{0,1,sc,ax,-3,az}};polygon(mat,sk,4);
    }
    litAlready=0;fixedGround=savedGround;
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
    quad(mat,p[7],p[6],p[5],p[4],lit_color(color,0,1,0),1,1);litAlready=0;fixedGround=savedGround;
}
/* Nivel (altura de terreno) de una parcela: maximo en vertices, centro y medios; todo lo que se apoya en el podio usa este nivel. */
static float parcel_level(const CmParcel *p){
    float cx,cz;parcel_centroid(p,&cx,&cz);float high=geo_height(cx,cz);
    for(int i=0;i<p->n;i++){float hh=geo_height(p->x[i],p->z[i]);if(hh>high)high=hh;hh=geo_height((p->x[i]+cx)*.5f,(p->z[i]+cz)*.5f);if(hh>high)high=hh;}
    return high;
}
static int city_floor_bias(int d){return d==6?2:d==4?3:d==1?1:0;}
/* Relleno de una parcela rectangular con edificios del tipo de la manzana. Parcelas muy grandes se subdividen. */
static void fill_rect(int bx,int bz,float x0,float z0,float x1,float z1,int type,int far,unsigned h,int depth){
    float w=x1-x0,d=z1-z0;int dist=city_district((x0+x1)*.5f,(z0+z1)*.5f);
    if(w<24||d<24)return;
    if((w>300||d>290)&&depth<2){ /* manzana doble: dos mitades con callejon interior (el podio ya lo pavimenta) */
        if(w>=d){float m=(x0+x1)*.5f;fill_rect(bx,bz,x0,z0,m-9,z1,type,far,h,depth+1);fill_rect(bx,bz,m+9,z0,x1,z1,city_block_type(bx+1,bz),far,h>>5,depth+1);}
        else{float m=(z0+z1)*.5f;fill_rect(bx,bz,x0,z0,x1,m-9,type,far,h,depth+1);fill_rect(bx,bz,x0,m+9,x1,z1,city_block_type(bx,bz+1),far,h>>5,depth+1);}
        if(!far&&!cityMid){float cx=(x0+x1)*.5f,cz=(z0+z1)*.5f;for(int k=0;k<3;k++)box(w>=d?cx:x0+30+k*(w-60)*.5f,w>=d?z0+30+k*(d-60)*.5f:cz,6,1.2f,1.2f,14,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));}
        return;
    }
    float cx=(x0+x1)*.5f,cz=(z0+z1)*.5f;int bias=city_floor_bias(dist);
    /* tipos grandes que no caben degradan a un tipo menudo */
    if(type==CB_TOWER&&(w<110||d<110))type=CB_CLASSIC;
    if(type==CB_COMMERCIAL&&(w<150||d<120))type=CB_CLASSIC;
    if(type==CB_CHURCH&&(w<150||d<170))type=CB_COLONIAL;
    if(type==CB_MARKET&&(w<170||d<110))type=CB_COMUNA;
    switch(type){
    case CB_COMUNA:{int cols=(int)(w/58),rows=(int)(d/56);if(cols<1)cols=1;if(rows<1)rows=1;float cw=w/cols,cd=d/rows;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,20+r*cols+c+depth*40)^h;
            comuna_house(x0+cw*(c+.5f)+((hk>>5)&3)-1.5f,z0+cd*(r+.5f)+((hk>>7)&3)-1.5f,cw-10-(hk>>9)%6,cd-10-(hk>>11)%6,6+(hk&3)*4,1+(hk>>2)%3,far,hk);}
        if(!far&&!cityMid&&bx<3)quad(MURAL,point(cx-40,24,z0+.5f),point(cx+40,24,z0+.5f),point(cx+40,7,z0+.5f),point(cx-40,7,z0+.5f),0xffffffffu,1,1);
        break;}
    case CB_COLONIAL:{int cols=(int)(w/47);if(cols<1)cols=1;float cw=w/cols;int rows=d>=125?2:1;float W=rows==2?54:d-10;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,40+r*cols+c+depth*40)^h;
            colonial_house(x0+cw*(c+.5f),rows==2?(r?z1-W*.5f-5:z0+W*.5f+5):cz,cw-4,W,1+((hk>>2)&1),cityPastel[hk&7],far,hk);}
        if(!far&&rows==2){float gw=w-40;if(gw>30){garden(x0+20,z0+64,gw,d-128>20?d-128:20,h,far);tree(x0+30,cz);tree(x1-30,cz);}}
        break;}
    case CB_TOWER:{int floors=6+(h>>3)%5+bias;float bw=w-14,bd=d-14;if(bw>120)bw=120;if(bd>110)bd=110;float tw=bw*.74f,td=bd*.76f;
        if(far){box(cx,cz,6,tw,td,floors*24,0,GLASS,ROOF,COLOR(190,215,225));break;}
        box(cx,cz,6,bw,bd,10,0,SHOP,SIDEWALK,0xffffffffu);
        for(int f=0;f<floors;f++){int glass=(f%3)!=2;box(cx,cz,16+f*24,tw,td,24,0,glass?GLASS:STUCCO,ROOF,glass?COLOR(190,215,225):COLOR(200,200,205));}
        box(cx,cz,16+floors*24,tw*.68f,td*.66f,14,0,STUCCO,ROOF,COLOR(200,200,205));box(cx,cz,30+floors*24,3,3,22,0,CAR_PAINT,CAR_PAINT,COLOR(180,30,30));
        if(w>150){tree(x0+14,z0+14);tree(x1-14,z0+14);tree(x0+14,z1-14);tree(x1-14,z1-14);garden(x0+8,cz-20,w-bw>40?(w-bw)*.5f-10:20,40,h,far);}
        break;}
    case CB_COMMERCIAL:{float aw=w*.5f,ad=d-12,bwid=w-aw-12,bd=d*.36f;float ax=x0+6+aw*.5f,bxx=x1-6-bwid*.5f,bzz=z0+6+bd*.5f;
        if(far){box(ax,cz,6,aw,ad,36,0,STUCCO,ROOF,cityPastel[h&7]);box(bxx,bzz,6,bwid,bd,36,0,STUCCO,ROOF,cityPastel[h&7]);break;}
        for(int f=0;f<2;f++){box(ax,cz,6+f*18,aw,ad,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:0xffffffffu);box(bxx,bzz,6+f*18,bwid,bd,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:0xffffffffu);}
        box(ax,cz,42,aw+3,ad+3,2,0,ROOF,ROOF,COLOR(120,110,100));box(bxx,bzz,42,bwid+3,bd+3,2,0,ROOF,ROOF,COLOR(120,110,100));
        rooftop(ax,cz,44,h);rooftop(bxx,bzz,44,h>>4);
        for(int k=0;k<4;k++)box(x0+14+k*(aw-20)/4,z0,18,(aw-28)/4,10,.8f,-.3f,CAR_PAINT,CAR_PAINT,cityPastel[(h>>(k*2))&7]);
        float px0=x0+aw+8,pz0=z0+bd+14,pw=x1-px0-4,pd=z1-pz0-4;if(pw>20&&pd>20){ground(ROAD,px0,pz0,pw,pd,6.3f,COLOR(150,150,150),3);for(int k=1;k<pd/30;k++)ground(CAR_PAINT,px0,pz0+k*30,pw,1.2f,6.9f,0xffffffffu,1);}
        box(bxx,z1-4,6,bwid*.5f,3,1.5f,0,CAR_PAINT,CAR_PAINT,COLOR(230,60,50));
        break;}
    case CB_CHURCH:{float nw=w*.42f,nd=d*.62f;float zz=cz+d*.08f;
        if(far){box(cx,zz,6,nw,nd,40,0,STUCCO,ROOF,COLOR(242,232,204));box(cx-nw*.38f,zz-nd*.6f,6,22,22,80,0,STUCCO,ROOF,COLOR(242,232,204));break;}
        box(cx,zz,6,nw,nd,34,0,STUCCO,ROOF,COLOR(242,232,204));gable_roof(cx,zz,nw,nd,40,58,COLOR(150,90,70));
        float tx=cx-nw*.38f,tz=zz-nd*.6f;box(tx,tz,6,22,22,74,0,STUCCO,ROOF,COLOR(242,232,204));box(tx,tz,80,26,26,3,0,ROOF,ROOF,COLOR(150,90,70));box(tx,tz,83,2,2,10,0,CAR_PAINT,CAR_PAINT,COLOR(90,80,60));
        for(int k=0;k<3;k++)box(cx-nw*.5f+1,zz-30+k*30,20,1,4,12,0,GLASS,GLASS,COLOR(120,80,200));
        ground(SIDEWALK,x0+6,z0+6,w-12,d*.25f,6.2f,COLOR(226,222,214),6);tree(x0+22,z0+24);tree(x1-22,z0+24);garden(x1-60,z0+8,44,26,h,far);
        for(int k=0;k<3;k++)box(cx-50+k*50,z0+40,6,10,3,2,0,ROOF,ROOF,COLOR(110,80,50));
        break;}
    case CB_MARKET:{float nw=w-14,nd=d*.64f;
        if(far){box(cx,cz-d*.08f,6,nw,nd,26,0,BRICK,ROOF,COLOR(220,200,180));break;}
        box(cx,cz-d*.08f,6,nw,nd,22,0,BRICK,ROOF,COLOR(220,200,180));gable_roof(cx,cz-d*.08f,nw,nd,28,44,COLOR(110,120,130));
        for(int k=0;k<5;k++)box(x0+20+k*(nw-30)/4,z1-12,6,26,10,9,0,CAR_PAINT,CAR_PAINT,cityPastel[(h>>k)&7]);
        box(cx-nw*.3f,cz-d*.08f,6,12,12,30,0,BRICK,BRICK,COLOR(180,160,140));
        break;}
    default:{int cols=(int)(w/96),rows=(int)(d/90);if(cols<1)cols=1;if(rows<1)rows=1;float cw=w/cols,cd=d/rows;
        for(int r=0;r<rows;r++)for(int c=0;c<cols;c++){unsigned hk=city_hash(bx,bz,60+r*cols+c+depth*40)^h;float xx=x0+cw*(c+.5f),zz=z0+cd*(r+.5f);
            int floors=2+(hk%4)+bias;int mat=(hk>>2)&1?BRICK:STUCCO;uint32_t tint=mat==STUCCO?cityPastel[(hk>>3)&7]:cityBrick[(hk>>3)&3];float L=cw-8,Wd=cd-8;
            if(far){box(xx,zz,6,L,Wd,floors*24,0,mat,ROOF,tint);continue;}
            for(int f=0;f<floors;f++){box(xx,zz,6+f*24,L-(f==floors-1&&(hk&64)?12:0),Wd,24,0,f?mat:SHOP,ROOF,f?tint:0xffffffffu);
                if(!cityMid&&f&&(hk>>(6+f))&1)box(xx,zz-Wd*.5f-2,6+f*24+1,L*.4f,6,1,0,SIDEWALK,SIDEWALK,COLOR(120,120,116));}
            box(xx,zz,6+floors*24,L+3,Wd+3,3,0,ROOF,ROOF,COLOR(120,110,100));rooftop(xx,zz,9+floors*24,hk>>8);
            if((dist==0||dist==5)&&r==rows-1)quad(MURAL,point(xx-L*.45f,25,zz+Wd*.5f+.6f),point(xx+L*.45f,25,zz+Wd*.5f+.6f),point(xx+L*.45f,7,zz+Wd*.5f+.6f),point(xx-L*.45f,7,zz+Wd*.5f+.6f),0xffffffffu,1,1);
        }
        break;}
    }
}
/* Parcela irregular (triangulo/trapecio junto a la avenida): edificio prisma escalonado. */
static void fill_poly(const CmParcel *p,int bx,int bz,int far,unsigned h){
    int dist=city_district(p->x[0],p->z[0]);int floors=3+(h>>4)%4+city_floor_bias(dist);float area=cm_area(p);
    if(area<2500)floors=2;
    int mat=(dist==6||dist==4)?((h&1)?GLASS:STUCCO):((h&1)?BRICK:STUCCO);uint32_t tint=mat==GLASS?COLOR(190,215,225):mat==BRICK?cityBrick[(h>>2)&3]:cityPastel[(h>>2)&7];
    CmParcel a,b;parcel_shrunk(p,10,&a);
    if(far){prism(&a,6,floors*24,mat,ROOF,tint);return;}
    prism(&a,6,12,SHOP,ROOF,0xffffffffu);            /* planta comercial */
    prism(&a,18,floors*24-12,mat,ROOF,tint);
    prism(&a,6+floors*24,2,ROOF,ROOF,COLOR(120,110,100));
    if(area>6000){parcel_shrunk(p,26,&b);prism(&b,8+floors*24,36,mat==GLASS?GLASS:STUCCO,ROOF,mat==GLASS?tint:COLOR(200,200,205));}
    float cx,cz;parcel_centroid(p,&cx,&cz);if(!cityMid)rooftop(cx,cz,area>6000?44+floors*24:9+floors*24,h>>6);
    (void)bx;(void)bz;
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
            box(cx+6,cz+7,2,10,10,1,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));box(cx+6,cz+7,3,7,7,4,0,WATER,WATER,0xffffffffu);
            for(int k=0;k<3;k++)box(x0+30+k*(w-60)*.5f,cz-16,2,10,3,2,0,ROOF,ROOF,COLOR(110,80,50));}
    }else{ if(far)return;tree(cx,cz);CmParcel s;parcel_shrunk(p,18,&s);for(int i=0;i<p->n&&i<4;i++)tree(s.x[i],s.z[i]); }
}
/* Plaza Botero: plaza dura con esculturas de bronce sobre pedestales, palmas y fuente. */
static void fill_plaza(const CmParcel *p,int far,unsigned h){
    float cx,cz;parcel_centroid(p,&cx,&cz);
    CmParcel s;parcel_shrunk(p,2,&s);slab_draped(&s,1.7f,SIDEWALK,COLOR(226,222,214));
    if(far)return;
    CmParcel g;parcel_shrunk(p,22,&g);
    for(int i=0;i<g.n;i++){float sx=(g.x[i]+cx)*.5f,sz=(g.z[i]+cz)*.5f;
        box(sx,sz,1.6f,8,8,3,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));
        box(sx,sz,4.6f,5,4,6,(i*.7f),CAR_PAINT,CAR_PAINT,COLOR(70,60,40));box(sx,sz,10.6f,3.4f,3.4f,3,0,CAR_PAINT,CAR_PAINT,COLOR(70,60,40)); /* figura rotunda */
        if(!cityMid)tree(g.x[i],g.z[i]);}
    box(cx,cz,1.6f,14,14,1,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));box(cx,cz,2.6f,10,10,4,0,WATER,WATER,0xffffffffu);
    for(int k=0;k<4;k++)box(cx-30+k*20,cz+24,1.6f,10,3,2,0,ROOF,ROOF,COLOR(110,80,50));
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
    if(!far){for(int k=0;k<5;k++)ground(CAR_PAINT,cx-80+k*40,cz-46,1.2f,92,7.1f,0xffffffffu,1);}
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
    for(int k=0;k<3;k++)ground(CAR_PAINT,x-6,z0+40+k*(d/3),12,1.2f,.2f,0xffffffffu,1); /* paso peatonal bajo el edificio */
}
/* Metro de Medellin a escala del jugador (citymap.h: CM_METRO_*): viaducto sobre pilares, estaciones con anden a 30,
   escalera de 60 de recorrido en la orilla, y tren de dos coches (paneles bajos + pilares de ventana: se ve la ciudad
   desde dentro cuando el jugador va a bordo). */
static void metro_train(void){
    for(int c=0;c<2;c++){float zc=view->metroZ+(c-.5f)*(CM_TRAIN_CAR+6);
        if(!nearby(CM_METRO_X,zc,700))continue;
        float fl=CM_PLAT_H-.6f;
        box(CM_METRO_X,zc,fl,24,CM_TRAIN_CAR,1.2f,0,SIDEWALK,SIDEWALK,COLOR(120,120,124));                 /* piso */
        box(CM_METRO_X,zc,fl-3,20,CM_TRAIN_CAR-8,3,0,CAR_PAINT,CAR_PAINT,COLOR(60,62,66));                  /* bajos */
        for(int sd=-1;sd<=1;sd+=2){float x=CM_METRO_X+sd*11.6f;
            box(x,zc,fl+1.2f,.8f,CM_TRAIN_CAR,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));                  /* panel bajo */
            box(x,zc,fl+5.4f,1.0f,CM_TRAIN_CAR,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80));               /* franja verde */
            for(int k=-2;k<=2;k++){float zz=zc+k*22;box(x,zz,fl+9.2f,.8f,2,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));} /* pilares de ventana */
            box(x,zc-33,fl+1.2f,.9f,10,18,0,CAR_PAINT,CAR_PAINT,COLOR(200,200,198));box(x,zc+33,fl+1.2f,.9f,10,18,0,CAR_PAINT,CAR_PAINT,COLOR(200,200,198)); /* puertas */
            box(x,zc,fl+18.2f,.8f,CM_TRAIN_CAR,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));             /* dintel */
        }
        box(CM_METRO_X,zc,fl+20,25,CM_TRAIN_CAR+1,1.6f,0,SIDEWALK,SIDEWALK,COLOR(230,230,226));             /* techo */
        for(int e=-1;e<=1;e+=2){float ze=zc+e*CM_TRAIN_CAR*.5f;                                                   /* testeros abiertos: panel bajo, pilares y dintel */
            box(CM_METRO_X,ze,fl+1.2f,24,1,8,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X,ze,fl+5.4f,24.2f,1.2f,1.6f,0,CAR_PAINT,CAR_PAINT,COLOR(50,150,80));
            box(CM_METRO_X-7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));box(CM_METRO_X+7,ze,fl+9.2f,2,1,9,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));
            box(CM_METRO_X,ze,fl+18.2f,24,1,1.8f,0,CAR_PAINT,CAR_PAINT,COLOR(240,240,236));}
        if(view->inMetro||view_distance(CM_METRO_X,zc)<200){
            for(int k=-2;k<=2;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*8,zc+k*20,fl+1.2f,5,12,4,0,CAR_PAINT,CAR_PAINT,COLOR(40,90,160)); /* asientos */
            for(int k=-2;k<=2;k++)box(CM_METRO_X,zc+k*20+10,fl+1.2f,1,1,18,0,CAR_PAINT,CAR_PAINT,COLOR(200,200,200));       /* barras */
        }
    }
}
static void metro(int bz,int far){
    float z=bz*320;
    for(int k=0;k<5;k++)box(CM_METRO_X,z+32+k*64,0,7,7,CM_METRO_DECK,0,STUCCO,STUCCO,COLOR(200,196,190));
    box(CM_METRO_X,z+160,CM_METRO_DECK,14,320,3,0,SIDEWALK,ROAD,COLOR(190,186,180));
    if(!far){box(CM_METRO_X-3,z+160,CM_METRO_DECK+3,.8f,320,.6f,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));box(CM_METRO_X+3,z+160,CM_METRO_DECK+3,.8f,320,.6f,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));
        if(!cm_metro_station(bz)){box(CM_METRO_X-6.5f,z+160,CM_METRO_DECK+3,1,320,3,0,SIDEWALK,SIDEWALK,COLOR(210,206,196));box(CM_METRO_X+6.5f,z+160,CM_METRO_DECK+3,1,320,3,0,SIDEWALK,SIDEWALK,COLOR(210,206,196));}}
    if(cm_metro_station(bz)){
        box(CM_METRO_X,z+160,CM_PLAT_H-1.2f,36,200,1.2f,0,SIDEWALK,SIDEWALK,COLOR(226,222,214));                /* anden 60..260 */
        for(int k=0;k<3;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*14,z+80+k*80,0,4,4,CM_PLAT_H-1.2f,0,STUCCO,STUCCO,COLOR(200,196,190)); /* pilares del anden */
        for(int k=0;k<4;k++)for(int sd=-1;sd<=1;sd+=2)box(CM_METRO_X+sd*17,z+70+k*60,CM_PLAT_H,2,2,24,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));
        box(CM_METRO_X,z+160,CM_PLAT_H+24,44,208,1.6f,0,SIDEWALK,SIDEWALK,COLOR(60,130,90));                         /* marquesina */
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
    for(int i=0;i<=N;i++){float t=(float)i/N;px[i]=CM_CABLE_X0+(CM_CABLE_X1-CM_CABLE_X0)*t;pz[i]=CM_CABLE_Z0+(CM_CABLE_Z1-CM_CABLE_Z0)*t;hs[i]=geo_height(px[i],pz[i])+48;
        if(nearby(px[i],pz[i],700))box(px[i],pz[i],0,4,4,60,0,CAR_PAINT,CAR_PAINT,COLOR(120,124,128));}
    float dx=CM_CABLE_X1-CM_CABLE_X0,dz=CM_CABLE_Z1-CM_CABLE_Z0,l=sqrtf(dx*dx+dz*dz),nx=-dz/l,nz=dx/l;
    float savedGround=fixedGround;fixedGround=0;
    for(int i=0;i<N;i++){if(!nearby((px[i]+px[i+1])*.5f,(pz[i]+pz[i+1])*.5f,760))continue;
        for(int s=-1;s<=1;s+=2){float ox=nx*3*s,oz=nz*3*s;
            quad(CAR_PAINT,point(px[i]+ox-nx*.5f,hs[i],pz[i]+oz-nz*.5f),point(px[i+1]+ox-nx*.5f,hs[i+1],pz[i+1]+oz-nz*.5f),point(px[i+1]+ox+nx*.5f,hs[i+1],pz[i+1]+oz+nz*.5f),point(px[i]+ox+nx*.5f,hs[i],pz[i]+oz+nz*.5f),COLOR(60,60,64),1,1);}
    }
    fixedGround=savedGround;
    box(CM_CABLE_X0,CM_CABLE_Z0,0,30,24,40,0,STUCCO,ROOF,COLOR(240,240,236));box(CM_CABLE_X1,CM_CABLE_Z1,0,30,24,40,0,STUCCO,ROOF,COLOR(240,240,236));
    if(far)return;
    float ang=atan2f(dz,dx);
    for(int k=0;k<6;k++){float t=fmodf(view->time*.03f+k/6.0f,1.0f);int s=k&1;if(s)t=1-t;
        int i=(int)(t*N);if(i>=N)i=N-1;float tt=t*N-i;float x=px[i]+(px[i+1]-px[i])*tt+nx*3*(s?1:-1),z=pz[i]+(pz[i+1]-pz[i])*tt+nz*3*(s?1:-1),y=hs[i]+(hs[i+1]-hs[i])*tt;
        if(!nearby(x,z,700))continue;
        abox(x,z,y-1.6f,.8f,.8f,1.6f,ang,CAR_PAINT,COLOR(60,60,64));abox(x,z,y-8.6f,6,5,7,ang,GLASS,COLOR(120,190,140));}
}
/* Avenida diagonal: separador ajardinado con arboles, lineas de carril y farolas. */
static void diagonal_avenue(void){
    float dx=CM_DIAG_X1-CM_DIAG_X0,dz=CM_DIAG_Z1-CM_DIAG_Z0,l=sqrtf(dx*dx+dz*dz),ux=dx/l,uz=dz/l,nx=-uz,nz=ux;
    int segs=(int)(l/60);
    for(int k=0;k<segs;k++){float t0=(float)k/segs,t1=(float)(k+1)/segs;float x0=CM_DIAG_X0+dx*t0,z0=CM_DIAG_Z0+dz*t0,x1=CM_DIAG_X0+dx*t1,z1=CM_DIAG_Z0+dz*t1;
        float mx=(x0+x1)*.5f,mz=(z0+z1)*.5f;
        if(!nearby(mx,mz,700))continue;
        quad(GRASS,point(x0-nx*3,.45f,z0-nz*3),point(x1-nx*3,.45f,z1-nz*3),point(x1+nx*3,.45f,z1+nz*3),point(x0+nx*3,.45f,z0+nz*3),COLOR(140,190,120),1,2);
        quad(SIDEWALK,point(x0-nx*3.6f,.4f,z0-nz*3.6f),point(x1-nx*3.6f,.4f,z1-nz*3.6f),point(x1-nx*3,.4f,z1-nz*3),point(x0-nx*3,.4f,z0-nz*3),0xffffffffu,1,1);
        quad(SIDEWALK,point(x0+nx*3,.4f,z0+nz*3),point(x1+nx*3,.4f,z1+nz*3),point(x1+nx*3.6f,.4f,z1+nz*3.6f),point(x0+nx*3.6f,.4f,z0+nz*3.6f),0xffffffffu,1,1);
        for(int s=-1;s<=1;s+=2){float ox=nx*15*s,oz=nz*15*s;
            quad(CAR_PAINT,point(mx-ux*9+ox-nx*.6f,.2f,mz-uz*9+oz-nz*.6f),point(mx+ux*9+ox-nx*.6f,.2f,mz+uz*9+oz-nz*.6f),point(mx+ux*9+ox+nx*.6f,.2f,mz+uz*9+oz+nz*.6f),point(mx-ux*9+ox+nx*.6f,.2f,mz-uz*9+oz+nz*.6f),0xffffffffu,1,1);}
        if(view_distance(mx,mz)<430&&(k&1))tree(mx,mz);
        if(!(k%3)){box(x0-nx*2,z0-nz*2,.4f,1.2f,1.2f,30,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(x0-nx*2,z0-nz*2,29,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));}
    }
}
static void city_cell_v26(int bx,int bz,int far,uint32_t white){
    float x=bx*320,z=bz*320;unsigned f=cm_flags(bx,bz),h=city_hash(bx,bz,7);int type=city_block_type(bx,bz);
    ground(ROAD,x,z,320,320,0,white,12);
    int westMerged=(cm_flags(bx-1,bz)&CM_MERGE_E)!=0,northMerged=(cm_flags(bx,bz-1)&CM_MERGE_S)!=0;
    if(!far){
        int lane=(h>>8)&3;
        if(lane<3)for(int k=0;k<320;k+=lane==1?320:40){
            uint32_t lc=lane==2?white:COLOR(244,208,100);float len=lane==1?318:19;
            if(!northMerged)ground(CAR_PAINT,x+k+1,z+42,len,1.2f,.5f,lc,1);
            if(!westMerged)ground(CAR_PAINT,x+42,z+k+1,1.2f,len,.5f,lc,1);
            if(lane==1){if(!northMerged)ground(CAR_PAINT,x+1,z+45,318,1.2f,.5f,lc,1);if(!westMerged)ground(CAR_PAINT,x+45,z+1,1.2f,318,.5f,lc,1);}
        }
        if(f&CM_SPLIT_X)for(int k=94;k<288;k+=40)ground(CAR_PAINT,x+189.4f,z+k,1.2f,19,.5f,white,1);
        if(f&CM_SPLIT_Z)for(int k=94;k<288;k+=40)ground(CAR_PAINT,x+k,z+185.4f,19,1.2f,.5f,white,1);
        if(h&128)for(int k=0;k<5;k++){ground(CAR_PAINT,x+9+k*13,z+75,7,5,.6f,white,1);ground(CAR_PAINT,x+75,z+9+k*13,5,7,.6f,white,1);}
        if(!cityMid){if(h&256)traffic_light(x+82,z+82,0);if((h>>9)%3==0&&!westMerged)bus_stop(x+116,z+80);}
        box(x+82,z+59,0,1.5f,1.5f,34,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(x+79,z+59,33,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
    }
    const CmParcel *ps;int n=cm_parcels(bx,bz,&ps);
    for(int i=0;i<n;i++){
        const CmParcel *p=&ps[i];uint32_t swc=((f&CM_PLAZA)||type==CB_TOWER)?COLOR(226,222,214):white;
        /* acera: sigue el terreno (ground) en parcelas rectangulares; prisma en las poligonales */
        if(cm_is_rect(p))ground(SIDEWALK,p->x[0]-8,p->z[0]-8,p->x[1]-p->x[0]+16,p->z[2]-p->z[0]+16,1.2f,swc,8);
        else{CmParcel sw;parcel_shrunk(p,-8,&sw);if(p->kind==1)slab_draped(&sw,1.2f,SIDEWALK,swc);else prism(&sw,0,1.2f,SIDEWALK,SIDEWALK,swc);}
        /* parcelas edificadas: todo comparte el nivel del podio (sin z-fighting en ladera). Las transitables (parques,
           plazas) siguen el terreno, porque el jugador camina sobre ellas a la altura del terreno. */
        float savedLevel=fixedGround;if(p->kind==0)fixedGround=parcel_level(p);
        if(f&CM_PLAZA)fill_plaza(p,far,h);
        else if(p->kind==1)fill_park(p,far,h);
        else if(f&CM_PUEBLITO)fill_pueblito(p,far,h);
        else if(f&CM_STADIUM){prism(p,0,6,SIDEWALK,SIDEWALK,white);fill_stadium(p,far,h);}
        else{
            uint32_t podium=type==CB_COMUNA?COLOR(190,180,170):type==CB_TOWER?COLOR(215,212,205):white;
            prism(p,0,6,type==CB_COMUNA?BRICK:SIDEWALK,type==CB_TOWER?SIDEWALK:ROOF,podium);
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
static void city_v26(void){
    const uint32_t white=0xffffffffu;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320,z=bz*320;
        if(!nearby(x+160,z+160,720))continue;
        if(!nearby(x+160,z+160,400)&&!sphere_visible(x+160,z+160,460))continue;
        float dist=view_distance(x+160,z+160);int far=dist>380;cityMid=dist>230;
        city_cell_v26(bx,bz,far,white);cityMid=0;
    }
    diagonal_avenue();
    /* Rio con puentes reales en cada calle transversal, paseo de la orilla este y Metro. */
    for(int bz=0;bz<7;bz++)if(nearby(1428,bz*320+160,850)){
        ground(WATER,1396,bz*320+86,64,234,1.4f,white,7);
        river_bridge(bz,white);
        box(1395,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);box(1461,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);
        ground(SIDEWALK,1462,bz*320+86,44,234,1.2f,COLOR(226,222,214),6);
        int far=view_distance(1428,bz*320+160)>430;
        if(!far)for(int k=0;k<3;k++){tree(1388,bz*320+120+k*60);if(!cm_metro_station(bz))tree(1466,bz*320+150+k*60);}
        metro(bz,far);
    }
    if(nearby(CM_METRO_X,view->metroZ,800))metro_train();
    {float cx=(CM_CABLE_X0+CM_CABLE_X1)*.5f,cz=(CM_CABLE_Z0+CM_CABLE_Z1)*.5f;if(nearby(cx,cz,1100))metrocable(view_distance(cx,cz)>600);}
    for(int side=0;side<2;side++)for(int row=0;row<28;row++){
        float x=side?2560:-320,z=row*80;
        if(nearby(x,z,1000))ground(GRASS,x,z,320,80,0,COLOR(125,167,132),5);
    }
}
