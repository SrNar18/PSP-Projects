/* v2.49 (Claude): dibujo del Metro circular (ver metro_loop.h). Cintas por tramos cuyos vertices toman la altura del
   terreno en cada punto (continuas en las rampas y en las esquinas, sin escalones), pilares, estaciones y tren.
   El viaducto y las estaciones van dentro de la pieza cacheada de cada manzana; el tren se dibuja cada fotograma. */
#define ML_STEP 16.f
static void ml_frame(float s,float off,float *x,float *z,float *tx,float *tz){
    float px,pz;ml_point(s,&px,&pz,tx,tz);*x=px-*tz*off;*z=pz+*tx*off; /* off>0: a la derecha del sentido de marcha */
}
/* Cinta entre s0 y s1 a desplazamiento off: tapa, dos costados y (si under) la cara inferior. */
static void ml_ribbon(float s0,float s1,float off,float width,float y0,float y1,int side,int top,uint32_t color,int under){
    float ax,az,bx,bz,tx,tz,ux,uz;ml_frame(s0,off,&ax,&az,&tx,&tz);ml_frame(s1,off,&bx,&bz,&ux,&uz);
    float h=width*.5f;
    Point al=point(ax+tz*h,y1,az-tx*h),ar=point(ax-tz*h,y1,az+tx*h),bl=point(bx+uz*h,y1,bz-ux*h),br=point(bx-uz*h,y1,bz+ux*h);
    Point al0=point(al.x,y0,al.z),ar0=point(ar.x,y0,ar.z),bl0=point(bl.x,y0,bl.z),br0=point(br.x,y0,br.z);
    float len=fmaxf(1,s1-s0),v=(y1-y0)/12.f;if(v<1)v=1;
    int saved=litAlready;litAlready=1;
    quad(top,al,bl,br,ar,lit_color(color,0,1,0),1,len/24.f);
    quad(side,al0,bl0,bl,al,lit_color(color,tz,0,-tx),len/24.f,v);
    quad(side,ar,br,br0,ar0,lit_color(color,-tz,0,tx),len/24.f,v);
    if(under)quad(side,al0,ar0,br0,bl0,shade(lit_color(color,0,-1,0),.8f),1,len/24.f);
    litAlready=saved;
}
static void ml_cap(float s,float off,float width,float y0,float y1,int mat,uint32_t color){ /* testero vertical transversal */
    float x,z,tx,tz;ml_frame(s,off,&x,&z,&tx,&tz);float h=width*.5f;
    quad(mat,point(x+tz*h,y1,z-tx*h),point(x-tz*h,y1,z+tx*h),point(x-tz*h,y0,z+tx*h),point(x+tz*h,y0,z-tx*h),color,1,1);
}
static int ml_in_cell(float x,float z,int bx,int bz){return x>=bx*320.f&&x<bx*320.f+320&&z>=bz*320.f&&z<bz*320.f+320;}

static void ml_station(int i,int far){
    float sc=ml_station_s(i),s0=sc-ML_PLAT_HALF,s1=sc+ML_PLAT_HALF;
    /* andenes laterales, borde amarillo y barandilla exterior */
    for(int sd=-1;sd<=1;sd+=2){
        for(float s=s0;s<s1-.01f;s+=ML_STEP){float e=fminf(s+ML_STEP,s1);
            ml_ribbon(s,e,sd*16.f,12,ML_DECK-3,ML_FLOOR,CONCRETE,SIDEWALK,COLOR(214,210,202),1);
            if(!far){ml_ribbon(s,e,sd*10.6f,.8f,ML_FLOOR,ML_FLOOR+.25f,FLAT,FLAT,COLOR(250,200,60),0);
                ml_ribbon(s,e,sd*21.6f,.4f,ML_FLOOR,ML_FLOOR+4,METAL,METAL,COLOR(70,96,100),0);}
            /* cubierta: una sola nave sobre andenes y via, con faldon */
            if(sd<0){ml_ribbon(s,e,0,46,ML_FLOOR+21,ML_FLOOR+22.4f,METAL,ROOF,COLOR(196,202,204),1);
                if(!far){ml_ribbon(s,e,-23,1,ML_FLOOR+19,ML_FLOOR+22.4f,METAL,METAL,COLOR(36,112,92),0);
                    ml_ribbon(s,e,23,1,ML_FLOOR+19,ML_FLOOR+22.4f,METAL,METAL,COLOR(36,112,92),0);}}
        }
        /* pilares de la cubierta */
        if(!far)for(float s=s0+6;s<s1;s+=22){float x,z,tx,tz;ml_frame(s,sd*20.8f,&x,&z,&tx,&tz);
            float saved=fixedGround;fixedGround=geo_height(x,z);box(x,z,ML_FLOOR,1.3f,1.3f,21,atan2f(tz,tx),METAL,METAL,COLOR(96,112,114));fixedGround=saved;}
        /* soportes del anden hasta el suelo */
        for(float s=s0+10;s<s1;s+=40){float x,z,tx,tz;ml_frame(s,sd*16.f,&x,&z,&tx,&tz);cylinder(x,z,0,2.6f,ML_DECK-3,6,0,CONCRETE,CONCRETE,0xffffffffu);}
    }
    /* cartel con el nombre (placa verde en el faldon lateral interior) */
    if(!far){float x,z,tx,tz;ml_frame(sc,23.6f,&x,&z,&tx,&tz);float saved=fixedGround;fixedGround=geo_height(x,z);
        box(x,z,ML_FLOOR+15.5f,26,.5f,3.4f,atan2f(tz,tx),FLAT,FLAT,COLOR(30,120,86));
        box(x-tz*.3f,z+tx*.3f,ML_FLOOR+16.3f,4,.4f,1.8f,atan2f(tz,tx),FLAT,FLAT,COLOR(240,240,236));fixedGround=saved;}
    /* pasarela del anden interior a la escalera y escalera hasta la acera interior (hacia atras en s) */
    ml_ribbon(s0-4,s0+6,(16+ML_STAIR_OFF)*.5f+3,ML_STAIR_OFF-16+8,ML_FLOOR-1.2f,ML_FLOOR,CONCRETE,SIDEWALK,COLOR(214,210,202),1);
    {int steps=far?5:14;float run=ML_STAIR_FOOT-ML_PLAT_HALF-4;
        for(int k=0;k<steps;k++){float a=(float)k/steps,b=(float)(k+1)/steps;float sa=s0-4-run*a,sb=s0-4-run*b;
            float hgt=ML_FLOOR*(1-b)+.4f;ml_ribbon(sb,sa,ML_STAIR_OFF,6,0,hgt,CONCRETE,SIDEWALK,COLOR(206,202,194),0);}
        if(!far)for(int sd=-1;sd<=1;sd+=2)for(int k=0;k<4;k++){float a=(k+.5f)/4;float s=s0-4-run*a;float x,z,tx,tz;ml_frame(s,ML_STAIR_OFF+sd*3.3f,&x,&z,&tx,&tz);
            float saved=fixedGround;fixedGround=geo_height(x,z);box(x,z,ML_FLOOR*(1-a)+.4f,.4f,.4f,4,0,METAL,METAL,COLOR(96,112,114));fixedGround=saved;}}
    /* portico de entrada a pie de calle */
    {float ex,ez,tx,tz;ml_frame(sc-ML_STAIR_FOOT-4,ML_STAIR_OFF,&ex,&ez,&tx,&tz);float saved=fixedGround;fixedGround=geo_height(ex,ez);
        box(ex,ez,0,2,9,12,atan2f(tz,tx),METAL,METAL,COLOR(36,112,92));box(ex,ez,12,3,9.5f,2.2f,atan2f(tz,tx),FLAT,FLAT,COLOR(240,240,236));fixedGround=saved;}
}
/* Viaducto, pilares y estaciones cuyo centro de tramo cae en la manzana (bx,bz). */
static void metro_loop_cell(int bx,int bz,int far){
    float L=ml_length();
    /* tramos de 32 en recta y 16 en las esquinas (64 lejos); costados en STUCCO gris para repartir el presupuesto
       de vertices (CONCRETE se desbordaba mirando a lo largo del anillo) */
    for(float s=0;s<L;){float px,pz,tx,tz;int leg=ml_point(s+1,&px,&pz,&tx,&tz);float st=leg>=4?16.f:far?64.f:32.f;
        float e=fminf(s+st,L);float mx,mz;ml_point((s+e)*.5f,&mx,&mz,&tx,&tz);
        if(ml_in_cell(mx,mz,bx,bz)){
            ml_ribbon(s,e,0,18,ML_DECK-3,ML_DECK,STUCCO,SIDEWALK,COLOR(158,162,162),1);                 /* tablero */
            if(!far&&!cityMid){ml_ribbon(s,e,-8.6f,.6f,ML_DECK,ML_DECK+1.6f,CONCRETE,CONCRETE,COLOR(176,180,178),0);   /* pretiles */
                ml_ribbon(s,e,8.6f,.6f,ML_DECK,ML_DECK+1.6f,CONCRETE,CONCRETE,COLOR(176,180,178),0);
                for(int sd=-1;sd<=1;sd+=2)ml_ribbon(s,e,sd*3.4f,.5f,ML_DECK,ML_DECK+.7f,METAL,FLAT,COLOR(206,214,216),0);} /* carriles */
        }
        s=e;
    }
    /* pilares: los mismos puntos que ml_pillar_at (colision) */
    const float lines[4][2]={{ML_Z0,0},{ML_X1,1},{ML_Z1,0},{ML_X0,1}};
    for(int leg=0;leg<4;leg++){int vert=(int)lines[leg][1];float lo=vert?ML_Z0+ML_R:ML_X0+ML_R,hi=vert?ML_Z1-ML_R:ML_X1-ML_R;
        for(float p=lo;p<=hi;p+=48){float px=vert?lines[leg][0]:p,pz=vert?p:lines[leg][0];
            if(!ml_in_cell(px,pz,bx,bz)||!ml_pillar_ok(px,pz,leg))continue;
            cylinder(px,pz,0,3.2f,ML_DECK-3,far?6:8,0,CONCRETE,CONCRETE,0xffffffffu);
            if(!far){float saved=fixedGround;fixedGround=geo_height(px,pz);box(px,pz,ML_DECK-5,vert?6:13,vert?13:6,2,0,CONCRETE,CONCRETE,COLOR(160,164,162));fixedGround=saved;}}}
    for(int i=0;i<ML_STATIONS;i++){float x,z,tx,tz;ml_point(ml_station_s(i),&x,&z,&tx,&tz);if(ml_in_cell(x,z,bx,bz))ml_station(i,far);}
}
/* Tren de tres vagones con cabeza en s (frente del primer vagon). */
static void metro_loop_train(float head){
    float cx,cz,tx,tz;ml_point(head-ML_CARS*ML_CAR*.5f,&cx,&cz,&tx,&tz);if(!nearby(cx,cz,900))return;
    int close=view_distance(cx,cz)<260;
    for(int c=0;c<ML_CARS;c++){float f=head-c*(ML_CAR+2),b=f-ML_CAR;
        for(float s=b;s<f-.01f;s+=10){float e=fminf(s+10,f);
            ml_ribbon(s,e,0,8,ML_FLOOR-1.6f,ML_FLOOR,METAL,METAL,COLOR(58,63,67),1);                        /* bastidor */
            ml_ribbon(s,e,0,10,ML_FLOOR,ML_FLOOR+8,CAR_PAINT,CAR_PAINT,COLOR(238,238,234),0);              /* panel bajo */
            ml_ribbon(s,e,0,10.5f,ML_FLOOR+5.4f,ML_FLOOR+7,FLAT,FLAT,COLOR(50,150,80),0);                  /* franja verde (0,25 por fuera) */
            ml_ribbon(s,e,0,9.8f,ML_FLOOR+8,ML_FLOOR+15,METAL,METAL,COLOR(62,88,104),0);                   /* ventanas */
            ml_ribbon(s,e,0,10.2f,ML_FLOOR+15,ML_FLOOR+17,CAR_PAINT,CAR_PAINT,COLOR(232,232,228),0);       /* techo */
            if(close)for(int sd=-1;sd<=1;sd+=2)ml_ribbon(s+1,s+2.2f,sd*5.05f,.3f,ML_FLOOR+8,ML_FLOOR+15,CAR_PAINT,CAR_PAINT,COLOR(238,238,234),0); /* montantes */
        }
        uint32_t endc=c==0?COLOR(36,48,56):COLOR(238,238,234);
        ml_cap(f,0,10.2f,ML_FLOOR,ML_FLOOR+17,c==0?METAL:CAR_PAINT,endc);ml_cap(b,0,10.2f,ML_FLOOR,ML_FLOOR+17,CAR_PAINT,COLOR(238,238,234));
        if(c==0){ml_cap(f+.3f,0,6,ML_FLOOR+3,ML_FLOOR+5,FLAT,COLOR(255,240,190));}                          /* faros */
    }
}
