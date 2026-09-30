/* city3d.inc — Ciudad irregular (Claude, v2.5). Se incluye desde render3d.c dentro de city().
 *
 * REGLA: la red de calles (cada manzana de 320, calle en lx<86 || ly<86) y la huella solida de los edificios
 * (lx 94..288, ly 94..280, ver solid() en game.c) NO cambian: misiones, mapa, minimapa y colisiones dependen de
 * ellas. Toda la irregularidad es visual y vive dentro de esa huella o por encima de la altura del jugador (>32):
 *  - 7 tipos de manzana elegidos por barrio y por un hash determinista (bx,bz): comuna en ladera, casas coloniales,
 *    torre con plaza, comercial en L con parking, iglesia con plaza, mercado/bodega y el bloque clasico mejorado.
 *  - Aceras de distinto ancho, esquinas achaflanadas, plazoletas, marcas viales distintas por calle.
 *  - Puentes de verdad sobre el rio (tablero, barandillas, arcos, farolas) y pasarelas cubiertas entre manzanas.
 *  - Detalle urbano: toldos, balcones, tanques de agua, antenas, aires acondicionados, semaforos, paradas de bus.
 * Usa las primitivas de render3d.c (box, ground, quad, polygon, tree) y respeta el LOD 'far'.
 */
static unsigned city_hash(int bx,int bz,int salt){unsigned h=(unsigned)bx*73856093u^(unsigned)bz*19349663u^(unsigned)salt*83492791u;h^=h>>13;h*=0x5bd1e995u;h^=h>>15;return h;}
static float city_rand(int bx,int bz,int salt){return (city_hash(bx,bz,salt)&1023)/1023.0f;}
static int city_district(float x,float z){if(x<640&&z<1000)return 0;if(x<960&&z<1550)return 1;if(x<1100)return 2;if(z<630)return 3;if(x>1760&&z>1380)return 4;if(x>1600)return 5;return 6;}
/* cityMid (LOD intermedio) esta declarado en render3d.c antes de tree(): sin flores, tanques, semaforos ni balcones */
enum {CB_CLASSIC,CB_COMUNA,CB_COLONIAL,CB_TOWER,CB_COMMERCIAL,CB_CHURCH,CB_MARKET,
      CB_RETAIL,CB_RESTAURANT,CB_OFFICE,CB_WORKSHOP};
static int city_block_type(int bx,int bz){
    int d=city_district(bx*320+160,bz*320+160);unsigned h=city_hash(bx,bz,1)%100;
    switch(d){
        case 0: return h<40?CB_COMUNA:h<58?CB_COLONIAL:h<74?CB_RETAIL:h<86?CB_RESTAURANT:CB_MARKET;
        case 1: return h<25?CB_COLONIAL:h<43?CB_CLASSIC:h<60?CB_RETAIL:h<75?CB_RESTAURANT:h<90?CB_OFFICE:CB_CHURCH;
        case 2: return h<20?CB_COLONIAL:h<39?CB_RETAIL:h<57?CB_MARKET:h<74?CB_WORKSHOP:h<87?CB_RESTAURANT:CB_CLASSIC;
        case 3: return h<34?CB_COMUNA:h<49?CB_CLASSIC:h<66?CB_RETAIL:h<82?CB_WORKSHOP:CB_MARKET;
        case 4: return h<32?CB_TOWER:h<58?CB_OFFICE:h<74?CB_RESTAURANT:h<88?CB_RETAIL:CB_COMMERCIAL;
        case 5: return h<36?CB_COMUNA:h<54?CB_COLONIAL:h<72?CB_RETAIL:h<87?CB_RESTAURANT:CB_CLASSIC;
        default:return h<22?CB_TOWER:h<43?CB_OFFICE:h<59?CB_CLASSIC:h<74?CB_RETAIL:h<87?CB_COMMERCIAL:CB_RESTAURANT;
    }
}
static const uint32_t cityPastel[8]={COLOR(247,189,89),COLOR(225,106,83),COLOR(92,170,205),COLOR(243,223,175),COLOR(113,198,150),COLOR(231,126,164),COLOR(185,131,89),COLOR(100,145,204)};
static const uint32_t cityBrick[4]={COLOR(255,246,226),COLOR(236,195,165),COLOR(212,150,124),COLOR(255,215,190)};

/* Tejado a dos aguas centrado en (x,z): largo L (eje x), ancho W (eje z), base h, cumbrera r. */
static void gable_roof(float x,float z,float L,float W,float h,float r,uint32_t color){
    Point a=point(x-L*.5f-2,h,z-W*.5f-2),b=point(x+L*.5f+2,h,z-W*.5f-2),c=point(x+L*.5f+2,r,z),d=point(x-L*.5f-2,r,z);
    Point e=point(x-L*.5f-2,h,z+W*.5f+2),f=point(x+L*.5f+2,h,z+W*.5f+2);
    quad(ROOF,a,b,c,d,color,3,1.5f);quad(ROOF,c,f,e,d,shade(color,.85f),3,1.5f);
    Vertex g1[3]={{0,0,color,a.x,a.y,a.z},{.5f,1,color,d.x,d.y,d.z},{1,0,color,e.x,e.y,e.z}};polygon(STUCCO,g1,3);
    Vertex g2[3]={{0,0,color,f.x,f.y,f.z},{.5f,1,color,c.x,c.y,c.z},{1,0,color,b.x,b.y,b.z}};polygon(STUCCO,g2,3);
}
/* Detalle de azotea: tanque de agua, antena, aire acondicionado. */
static void rooftop(float x,float z,float top,unsigned h){
    if(cityMid)return;
    if(h&1)box(x+8*((h>>1)&1?1:-1),z+6,top,7,7,9,0,SIDEWALK,SIDEWALK,COLOR(120,120,116));
    if(h&4)box(x-10,z-8,top,1,1,16,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,90));
    if(h&8)box(x+12,z-12,top,6,4,3,0,SIDEWALK,SIDEWALK,COLOR(200,200,200));
}
/* Windows have actual frames and panes instead of being a flat brick wall.
   Only close LOD receives them; party-wall panes are naturally occluded. */
static void residential_facade(float x,float z,float width,float base,int floors,float storey,float angle,int door){
    if(cityMid||view_distance(x,z)>170)return;
    float win=fminf(10,width*.18f);
    for(int floor=0;floor<floors;floor++)for(int side=-1;side<=1;side+=2){
        float xx=side*width*.25f,y=base+floor*storey+8;
        quad(METAL,local(xx-win*.5f-1,y+10,-1.0f,x,z,angle),local(xx+win*.5f+1,y+10,-1.0f,x,z,angle),local(xx+win*.5f+1,y,-1.0f,x,z,angle),local(xx-win*.5f-1,y,-1.0f,x,z,angle),COLOR(74,79,75),1,1);
        quad(GLASS,local(xx-win*.5f,y+9,-1.5f,x,z,angle),local(xx+win*.5f,y+9,-1.5f,x,z,angle),local(xx+win*.5f,y+1,-1.5f,x,z,angle),local(xx-win*.5f,y+1,-1.5f,x,z,angle),COLOR(120,169,190),1,1);
    }
    /* v2.33: marcos a 0,4 y cristal a 0,6 (antes 0,12/0,25). v2.41 (Claude): 1,0 / 1,5 / puerta 1,2: con 16 bits de
       profundidad, a 100-170 de distancia 0,2-0,4 eran 2-4 pasos y seguian parpadeando (tools/qa_zfight_v233.c, ZTOL .3) */
    if(door)quad(ROOF,local(-3,base+15,-1.2f,x,z,angle),local(3,base+15,-1.2f,x,z,angle),local(3,base,-1.2f,x,z,angle),local(-3,base,-1.2f,x,z,angle),COLOR(105,82,61),1,1);
}
/* Casa colonial: 1-2 plantas, color pastel, balcon y tejado a dos aguas. */
static void colonial_house(float x,float z,float L,float W,int floors,uint32_t color,int far,unsigned h){
    float top=6+floors*22;
    if(far){box(x,z,6,L,W,floors*22,0,STUCCO,ROOF,color);return;}
    for(int f=0;f<floors;f++)box(x,z,6+f*22,L,W,22,0,STUCCO,ROOF,f?color:shade(color,.92f));
    gable_roof(x,z,L,W,top,top+9,(h&1)?COLOR(190,90,60):COLOR(150,110,90));
    if(cityMid)return;
    residential_facade(x,z-W*.5f,L,6,floors,22,0,1);
    residential_facade(x,z+W*.5f,L,6,floors,22,PI,0);
    if(floors>1){box(x,z-W*.5f-3,6+22,L*.6f,6,1.2f,0,SIDEWALK,SIDEWALK,COLOR(120,70,50));box(x,z-W*.5f-6,6+23,L*.6f,.6f,7,0,CAR_PAINT,CAR_PAINT,COLOR(70,50,40));}
    box(x-L*.25f,z-W*.5f-4,17,L*.4f,8,.8f,-.25f,CAR_PAINT,CAR_PAINT,cityPastel[(h>>3)&7]); /* toldo */
}
/* Casa de comuna: ladrillo, techo de zinc plano, escalera lateral. */
static void comuna_house(float x,float z,float L,float W,float base,int floors,int far,unsigned h){
    uint32_t tint=cityBrick[h&3];
    if(far){box(x,z,base,L,W,floors*20,0,BRICK,SIDEWALK,tint);return;}
    for(int f=0;f<floors;f++)box(x,z,base+f*20,L,W,20,0,(h>>f)&1?BRICK:STUCCO,SIDEWALK,(h>>f)&1?tint:cityPastel[(h>>(4+f))&7]);
    box(x,z,base+floors*20,L+3,W+3,1.5f,0,SIDEWALK,SIDEWALK,COLOR(150,152,150)); /* techo de zinc */
    if(cityMid)return;
    residential_facade(x,z-W*.5f,L,base,floors,20,0,1);
    residential_facade(x,z+W*.5f,L,base,floors,20,PI,0);
    if(h&32)box(x,z,base+floors*20+1.5f,6,6,6,0,CAR_PAINT,CAR_PAINT,COLOR(60,80,140)); /* tank supported by the roof */
}
/* Zona verde: cesped, arbustos redondeados y matas de flores de colores (deterministas por hash). */
static void garden(float x,float z,float w,float d,unsigned h,int far){
    float base=fixedGround>-999999?6.3f:2.15f; /* v2.46: 0,35 sobre el cesped del parque (1,8; antes 1,9 parpadeaba) */
    ground(GRASS,x,z,w,d,base,COLOR(200,230,190),3);
    if(far||cityMid)return;
    for(int k=0;k<3;k++){unsigned hk=city_hash((int)x+k,(int)z,9);float bx=x+6+(hk%97)/97.0f*(w-12),bz=z+6+((hk>>7)%97)/97.0f*(d-12);
        box(bx,bz,base,7+(hk>>14)%5,7+(hk>>17)%5,4+(hk>>20)%4,(hk>>10)%7*.4f,GRASS,GRASS,COLOR(60,120,60));}
    static const uint32_t flowers[5]={COLOR(240,90,90),COLOR(250,200,70),COLOR(230,120,200),COLOR(255,250,240),COLOR(120,120,230)};
    for(int k=0;k<6;k++){unsigned hk=city_hash((int)x,(int)z+k,11);float fx=x+4+(hk%89)/89.0f*(w-8),fz=z+4+((hk>>7)%89)/89.0f*(d-8);
        quad(CAR_PAINT,point(fx-2,base+1.3f,fz-2),point(fx+2,base+1.3f,fz-2),point(fx+2,base+1.3f,fz+2),point(fx-2,base+1.3f,fz+2),flowers[(hk>>14)%5],1,1);}
}
/* Semaforo en una esquina de manzana. */
static void traffic_light(float x,float z,float angle){
    if(cacheRec){RecHdr h={3,0,0,0};cache_put(&h,4);float q[3]={x,z,angle};cache_put(q,sizeof q);return;} /* dinamico: se dibuja en vivo */
    float saved=fixedGround;if(saved<=-999999)fixedGround=geo_height(x,z);
    box(x,z,0,1.2f,1.2f,28,angle,CAR_PAINT,CAR_PAINT,COLOR(70,74,76));
    box(x,z,22,3,3,9,angle,CAR_PAINT,CAR_PAINT,COLOR(40,40,40));
    int bx=(int)floorf(x/320),bz=(int)floorf(z/320);
    int horizontalGreen=(((int)(view->time/4)+bx+bz)&1)==0;
    box(x,z-1.7f,28,1.5f,.4f,1.5f,angle,CAR_PAINT,CAR_PAINT,horizontalGreen?COLOR(68,62,56):COLOR(255,60,40));
    box(x,z-1.7f,24,1.5f,.4f,1.5f,angle,CAR_PAINT,CAR_PAINT,horizontalGreen?COLOR(60,220,80):COLOR(46,65,48));
    box(x-1.7f,z,28,.4f,1.5f,1.5f,angle,CAR_PAINT,CAR_PAINT,horizontalGreen?COLOR(255,60,40):COLOR(68,62,56));
    box(x-1.7f,z,24,.4f,1.5f,1.5f,angle,CAR_PAINT,CAR_PAINT,horizontalGreen?COLOR(46,65,48):COLOR(60,220,80));
    fixedGround=saved;
}
/* Parada de bus: marquesina sobre postes (visual, en la acera). */
static void bus_stop(float x,float z){
    float saved=fixedGround;if(saved<=-999999)fixedGround=geo_height(x,z);
    box(x-6,z,0,1,1,14,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));box(x+6,z,0,1,1,14,0,CAR_PAINT,CAR_PAINT,COLOR(90,90,96));
    box(x,z,14,18,7,1.2f,0,GLASS,GLASS,COLOR(160,200,210));box(x,z+3,5,16,.6f,7,0,GLASS,GLASS,COLOR(180,210,220));
    fixedGround=saved;
}
/* Puente sobre el rio en la fila bz: tablero, barandillas, arcos y farolas. */
static void river_bridge(int bz,uint32_t white){
    float z=bz*320;
    ground(ROAD,1392,z,72,86,1.6f,white,4);
    box(1428,z+43,-6,80,90,7.2f,0,SIDEWALK,SIDEWALK,COLOR(196,190,178)); /* tablero visto desde el rio. v2.39: techo a 1,2 (antes 1,6 = misma altura que la calzada: parpadeaba) */
    for(int side=0;side<2;side++){float zz=side?z+85:z+1;
        box(1428,zz,1.6f,80,1.0f,1,0,SIDEWALK,SIDEWALK,COLOR(210,206,196)); /* v2.33 (Claude): sin caras coplanares (parpadeo): tools/qa_zfight_v233.c */
        for(int k=0;k<7;k++)box(1392+k*12,zz,1.6f,1.2f,1.2f,6,0,CAR_PAINT,CAR_PAINT,COLOR(80,84,88));
        box(1428,zz,7.2f,74,1,1,0,CAR_PAINT,CAR_PAINT,COLOR(80,84,88));
        box(1395,zz,7.2f,2,2,26,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(1395,zz,32,6,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
        box(1461,zz,7.2f,2,2,26,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(1461,zz,32,6,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
    }
    /* Arco: pilas en las orillas y dovelas escalonadas bajo el tablero. */
    box(1400,z+43,-12,8,92,12.4f,0,BRICK,BRICK,COLOR(200,190,180));box(1456,z+43,-12,8,92,12.4f,0,BRICK,BRICK,COLOR(200,190,180)); /* v2.39: 13 (antes 14: asomaban 0,4 por encima de la calzada) */
    for(int k=0;k<4;k++){float t=k/3.0f;box(1428,z+43,-7.4f-k*.5f,56-k*14,88-k*.6f,1.2f,0,BRICK,BRICK,COLOR(180,170,160)); /* v2.46: dovelas escalonadas 0,5 (antes 0,02: parpadeaban) */(void)t;} /* v2.41: bajo el tablero (-6) sin compartir plano con el ni entre si */ /* v2.33: 88 (tablero 90, pilas 92) */
}
/* Pasarela cubierta que cruza la calle entre dos manzanas (a 38 de altura, por encima de todo lo transitable). */
static void skybridge(float x0,float x1,float z,float w){
    float cx=(x0+x1)*.5f,len=x1-x0;
    box(cx,z,38,len,w,1.5f,0,SIDEWALK,SIDEWALK,COLOR(190,186,176));
    box(cx,z-w*.5f,39.5f,len,.8f,9,0,GLASS,GLASS,COLOR(170,205,215));box(cx,z+w*.5f,39.5f,len,.8f,9,0,GLASS,GLASS,COLOR(170,205,215));
    box(cx,z,48.5f,len+2,w+2,1.2f,0,ROOF,ROOF,COLOR(120,110,100));
}
static void city_block(int bx,int bz,int far,uint32_t white){
    float x=bx*320,z=bz*320;int type=city_block_type(bx,bz);unsigned h=city_hash(bx,bz,7);
    int d=city_district(x+160,z+160);
    /* --- calle y acera: ancho variable por manzana, esquinas achaflanadas, marcas distintas --- */
    ground(ROAD,x,z,320,320,0,white,12);
    float curb=(type==CB_COMUNA)?72:(type==CB_TOWER||type==CB_CHURCH)?60:(h&64)?78:86; /* acera mas ancha en plazas/torres */
    float sw=320-curb;
    ground(SIDEWALK,x+curb,z+curb,sw,sw,1.2f,(type==CB_TOWER)?COLOR(226,222,214):white,12);
    if(!far){
        int lane=(h>>8)&3; /* 0 amarilla discontinua, 1 doble amarilla, 2 blanca discontinua, 3 sin marca (calle de barrio) */
        if(lane<3)for(int k=0;k<320;k+=lane==1?320:40){
            uint32_t lc=lane==2?white:COLOR(244,208,100);float len=lane==1?318:19;
            ground(CAR_PAINT,x+k+1,z+42,len,1.2f,.15f,lc,1);ground(CAR_PAINT,x+42,z+k+1,1.2f,len,.15f,lc,1);
            if(lane==1){ground(CAR_PAINT,x+1,z+45,318,1.2f,.15f,lc,1);ground(CAR_PAINT,x+45,z+1,1.2f,318,.15f,lc,1);}
        }
        if(h&128)for(int k=0;k<5;k++){ground(CAR_PAINT,x+9+k*13,z+curb-11,7,5,.2f,white,1);ground(CAR_PAINT,x+curb-11,z+9+k*13,5,7,.2f,white,1);}
        if(!cityMid){if(h&256)traffic_light(x+curb-4,z+curb-4,0);
        if((h>>9)%3==0)bus_stop(x+curb+30,z+curb-6);}
        /* esquina achaflanada: la acera pierde un triangulo en la esquina */
        Vertex tri[3]={{0,0,white,x+curb,1.25f,z+curb},{1,0,white,x+curb+18,1.25f,z+curb},{0,1,white,x+curb,1.25f,z+curb+18}};
        (void)tri;
        box(x+82,z+59,0,1.5f,1.5f,34,0,CAR_PAINT,CAR_PAINT,COLOR(75,79,80));box(x+79,z+59,33,8,3,2,0,CAR_PAINT,CAR_PAINT,COLOR(255,236,167));
    }
    if(park(bx,bz)){
        ground(GRASS,x+95,z+95,194,186,.5f,white,8);
        ground(SIDEWALK,x+100,z+173,185,14,.6f,white,6);ground(SIDEWALK,x+185,z+99,13,179,.6f,white,6);
        if(!far){for(int k=0;k<4;k++)tree(x+112+k%2*157,z+115+k/2*139);
            garden(x+105,z+105,70,60,h,far);garden(x+205,z+195,72,70,h>>3,far);
            for(int k=0;k<10;k++){float sx=x+100+k*19;box(sx,z+96,.5f,8,4,5,0,GRASS,GRASS,COLOR(50,110,55));} /* seto */
            box(x+191,z+180,.6f,10,10,1,0,SIDEWALK,SIDEWALK,COLOR(200,196,186));box(x+191,z+180,1.6f,7,7,4,0,WATER,WATER,white); /* fuente */
            for(int k=0;k<3;k++)box(x+130+k*60,z+165,1,10,3,2,0,ROOF,ROOF,COLOR(110,80,50)); /* bancos */
        }
        return;
    }
    /* Podio continuo: coincide con solid(). */
    uint32_t podium=type==CB_COMUNA?COLOR(190,180,170):type==CB_TOWER?COLOR(215,212,205):white;
    box(x+191,z+187,0,194,186,6,0,type==CB_COMUNA?BRICK:SIDEWALK,type==CB_TOWER?SIDEWALK:ROOF,podium);
    switch(type){
    case CB_COMUNA:{ /* 3x3 casas de ladrillo a alturas distintas, escaleras, colores */
        for(int k=0;k<9;k++){
            unsigned hk=city_hash(bx,bz,20+k);float xx=x+128+(k%3)*63,zz=z+128+(k/3)*60;
            float base=6+(hk&3)*4;int floors=1+(hk>>2)%3;
            comuna_house(xx+((hk>>5)&3)-1.5f,zz+((hk>>7)&3)-1.5f,50+(hk>>9)%8,46+(hk>>11)%8,base,floors,far,hk);
        }
        if(!far){box(x+191,z+96,6,186,3,1,0,CAR_PAINT,CAR_PAINT,COLOR(60,60,64));garden(x+100,z+250,40,30,h,far);if(bx<3)quad(MURAL,point(x+150,24,z+94.5f),point(x+232,24,z+94.5f),point(x+232,7,z+94.5f),point(x+150,7,z+94.5f),white,1,1);}
        break;}
    case CB_COLONIAL:{ /* dos hileras de casas estrechas con patio central */
        for(int k=0;k<8;k++){
            unsigned hk=city_hash(bx,bz,40+k);int row=k/4,col=k%4;float xx=x+118+col*47,zz=row?z+245:z+129;
            colonial_house(xx,zz,44,54,1+((hk>>2)&1),cityPastel[hk&7],far,hk);
        }
        if(!far){garden(x+140,z+165,100,44,h,far);tree(x+165,z+187);tree(x+215,z+187);}
        break;}
    case CB_TOWER:{ /* torre con plaza y retranqueo */
        int floors=7+(h>>3)%5;float xx=x+191,zz=z+187;
        if(far){box(xx,zz,6,90,86,floors*24,0,GLASS,ROOF,COLOR(190,215,225));break;}
        box(xx,zz,6,120,110,10,0,SHOP,SIDEWALK,white); /* basamento comercial */
        for(int f=0;f<floors;f++){int glass=(f%3)!=2;box(xx,zz,16+f*24,88,84,24,0,glass?GLASS:STUCCO,ROOF,glass?COLOR(190,215,225):COLOR(200,200,205));}
        box(xx,zz,16+floors*24,60,56,14,0,STUCCO,ROOF,COLOR(200,200,205));box(xx,zz,30+floors*24,3,3,22,0,CAR_PAINT,CAR_PAINT,COLOR(180,30,30));
        for(int k=0;k<3;k++){tree(x+110+k*80,z+108);tree(x+110+k*80,z+270);}
        garden(x+100,z+120,60,40,h,far);garden(x+220,z+230,60,40,h>>5,far);
        box(x+191,z+110,6,40,12,3,0,SIDEWALK,SIDEWALK,COLOR(200,196,186)); /* banco largo */
        break;}
    case CB_COMMERCIAL:{ /* edificio en L + aparcamiento con plazas pintadas + toldos */
        if(far){box(x+150,z+187,6,110,186,36,0,STUCCO,ROOF,cityPastel[h&7]);box(x+235,z+120,6,100,52,36,0,STUCCO,ROOF,cityPastel[h&7]);break;}
        for(int f=0;f<2;f++){box(x+150,z+187,6+f*18,110,186,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:white);box(x+235,z+120,6+f*18,100,52,18,0,f?STUCCO:SHOP,ROOF,f?cityPastel[h&7]:white);}
        box(x+150,z+187,42,113,189,2,0,ROOF,ROOF,COLOR(120,110,100));box(x+235,z+120,42,103,55,2,0,ROOF,ROOF,COLOR(120,110,100));
        rooftop(x+150,z+187,44,h);rooftop(x+235,z+120,44,h>>4);
        for(int k=0;k<4;k++)box(x+108+k*26,z+94,18,22,10,.8f,-.3f,CAR_PAINT,CAR_PAINT,cityPastel[(h>>(k*2))&7]); /* toldos */
        ground(ROAD,x+206,z+150,80,132,6.3f,COLOR(150,150,150),3);
        for(int k=0;k<5;k++)ground(CAR_PAINT,x+206,z+152+k*30,80,1.2f,6.9f,white,1);
        box(x+240,z+270,6,40,3,1.5f,0,CAR_PAINT,CAR_PAINT,COLOR(230,60,50)); /* cartel */
        break;}
    case CB_CHURCH:{ /* iglesia con torre campanario y plaza */
        float xx=x+191,zz=z+200;
        if(far){box(xx,zz,6,80,120,40,0,STUCCO,ROOF,COLOR(242,232,204));box(xx-30,zz-70,6,22,22,80,0,STUCCO,ROOF,COLOR(242,232,204));break;}
        box(xx,zz,6,80,120,34,0,STUCCO,ROOF,COLOR(242,232,204));gable_roof(xx,zz,80,120,40,58,COLOR(150,90,70));
        box(xx-30,zz-72,6,22,22,74,0,STUCCO,ROOF,COLOR(242,232,204));box(xx-30,zz-72,80,26,26,3,0,ROOF,ROOF,COLOR(150,90,70));
        box(xx-30,zz-72,83,2,2,10,0,CAR_PAINT,CAR_PAINT,COLOR(90,80,60));
        for(int k=0;k<3;k++)box(xx-39,zz-30+k*40,20,1,4,12,0,GLASS,GLASS,COLOR(120,80,200)); /* vidrieras */
        ground(SIDEWALK,x+100,z+100,180,60,6.2f,COLOR(226,222,214),6);tree(x+120,z+120);tree(x+262,z+120);garden(x+236,z+104,44,30,h,far);
        for(int k=0;k<3;k++)box(x+140+k*50,z+135,6,10,3,2,0,ROOF,ROOF,COLOR(110,80,50));
        break;}
    case CB_MARKET:{ /* nave larga con tejado a dos aguas y puestos */
        if(far){box(x+191,z+187,6,180,120,26,0,BRICK,ROOF,COLOR(220,200,180));break;}
        box(x+191,z+187,6,180,120,22,0,BRICK,ROOF,COLOR(220,200,180));gable_roof(x+191,z+187,180,120,28,44,COLOR(110,120,130));
        for(int k=0;k<5;k++)box(x+110+k*40,z+270,6,26,10,9,0,CAR_PAINT,CAR_PAINT,cityPastel[(h>>k)&7]); /* puestos */
        box(x+191,z+124,6,12,12,30,0,BRICK,BRICK,COLOR(180,160,140)); /* chimenea */
        break;}
    default:{ /* clasico mejorado: 4 edificios con alturas y materiales distintos, balcones y azoteas */
        for(int k=0;k<4;k++){
            unsigned hk=city_hash(bx,bz,60+k);float xx=x+143+(k%2)*96,zz=z+141+(k/2)*91;
            int floors=2+(hk%4);int mat=(hk>>2)&1?BRICK:STUCCO;uint32_t tint=mat==STUCCO?cityPastel[(hk>>3)&7]:cityBrick[(hk>>3)&3];
            if(far){box(xx,zz,6,94,89,floors*24,0,mat,ROOF,tint);continue;}
            for(int f=0;f<floors;f++){box(xx,zz,6+f*24,94-(f==floors-1&&(hk&64)?12:0),89,24,0,f?mat:SHOP,ROOF,f?tint:white);
                if(!cityMid&&f&&(hk>>(6+f))&1)box(xx,zz-46,6+f*24+1,40,6,1,0,SIDEWALK,SIDEWALK,COLOR(120,120,116));} /* balcones */
            box(xx,zz,6+floors*24,97,92,3,0,ROOF,ROOF,COLOR(120,110,100));rooftop(xx,zz,9+floors*24,hk>>8);
            if(d==0||d==5){if(k>1)quad(MURAL,point(xx-45,25,zz+44.6f),point(xx+45,25,zz+44.6f),point(xx+45,7,zz+44.6f),point(xx-45,7,zz+44.6f),white,1,1);}
        }
        if(!far){tree(x+305,z+126);tree(x+305,z+265);}
        break;}
    }
    /* Pasarela hacia la manzana del este (cruza la calle a 38 de altura). */
    if(!far&&bx<7&&(h>>12)%4==0&&(type==CB_TOWER||type==CB_COMMERCIAL||type==CB_CLASSIC)&&!park(bx+1,bz)){int t2=city_block_type(bx+1,bz);if(t2==CB_TOWER||t2==CB_COMMERCIAL||t2==CB_CLASSIC)skybridge(x+288,x+320+94,z+187,10);}
}
static void city_v25(void){
    const uint32_t white=0xffffffffu;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320,z=bz*320;
        if(!nearby(x+160,z+160,790))continue;
        if(!nearby(x+160,z+160,400)&&!sphere_visible(x+160,z+160,420))continue;
        float dist=view_distance(x+160,z+160);int far=dist>430;cityMid=dist>260;
        city_block(bx,bz,far,white);cityMid=0;
    }
    /* Rio con puentes reales en cada calle transversal. */
    for(int bz=0;bz<7;bz++)if(nearby(1428,bz*320+160,850)){
        ground(WATER,1396,bz*320+86,64,234,1.4f,white,7);
        river_bridge(bz,white);
        box(1395,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);box(1461,bz*320+203,0,2,234,8,0,SIDEWALK,SIDEWALK,white);
        if(view_distance(1428,bz*320+160)<430)for(int k=0;k<3;k++){tree(1388,bz*320+120+k*60);tree(1468,bz*320+150+k*60);}
    }
    for(int side=0;side<2;side++)for(int row=0;row<28;row++){
        float x=side?2560:-320,z=row*80;
        if(nearby(x,z,1000))ground(GRASS,x,z,320,80,0,COLOR(125,167,132),5);
    }
}
