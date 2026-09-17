/* daylight.inc — Ciclo dia/noche, sol, sombras, nubes y luces nocturnas (Claude, v2.5). Incluido desde render3d.c.
 *
 * - day_update(time): un dia completo cada DAY_SECONDS segundos reales. Calcula direccion del sol, color de la luz,
 *   ambiente, color del cielo/niebla y factor de "noche".
 * - lit_color(color, nx,ny,nz): color de vertice iluminado por cara (ambiente + difusa Lambert + calidez del sol).
 *   box() y ground() lo usan; asi cada fachada se ilumina segun su orientacion y la hora, y las sombras propias
 *   cambian a lo largo del dia.
 * - day_scale(color): atenuacion global (noche) aplicada en polygon() a todo lo demas.
 * - Pase "glow" (aditivo, sin textura, dibujado tras la geometria): nubes, sombras proyectadas (sustractivas),
 *   charcos de luz de farolas, ventanas encendidas y conos de los faros de los coches.
 */
#ifndef DAY_SECONDS
#define DAY_SECONDS 480.0f /* segundos reales por dia completo */
#endif
#define GLOW_MAX 3000
static Vertex __attribute__((aligned(16))) glowMesh[GLOW_MAX];static int glowUsed=0;
static Vertex __attribute__((aligned(16))) shadowMesh[GLOW_MAX/2];static int shadowUsed=0;
static float dayT=.35f;              /* 0 = medianoche, .25 = amanecer, .5 = mediodia, .75 = atardecer */
static float sunX=0,sunY=1,sunZ=0;   /* direccion HACIA el sol */
static float sunR=1,sunG=1,sunB=1;   /* color de la luz directa */
static float ambR=.5f,ambG=.5f,ambB=.55f;
static float night=0;                /* 0 dia ... 1 noche cerrada */
static uint32_t skyColor=COLOR(155,189,192);
static float lerpf(float a,float b,float t){return a+(b-a)*t;}
static float smooth01(float t){t=t<0?0:t>1?1:t;return t*t*(3-2*t);}
static void day_update(float time){
    dayT=fmodf(time/DAY_SECONDS+.30f,1.0f);if(dayT<0)dayT+=1;   /* la partida empieza a media manana */
    float ang=(dayT-.25f)*2*PI;                                   /* amanecer en el este, atardecer en el oeste */
    float elev=sinf(ang)*.85f+.28f;if(elev>1)elev=1;              /* sesgo: el sol pasa ~62% del ciclo sobre el horizonte (noche corta) */
    sunX=cosf(ang)*.8f;sunY=elev;sunZ=.35f;
    float n=sqrtf(sunX*sunX+sunY*sunY+sunZ*sunZ);sunX/=n;sunY/=n;sunZ/=n;
    night=1-smooth01((elev+.12f)/.30f);                          /* transicion en el crepusculo */
    float low=1-smooth01((elev-.05f)/.35f);                       /* sol bajo: luz calida y rasante */
    sunR=lerpf(1.05f,1.15f,low);sunG=lerpf(1.0f,.72f,low);sunB=lerpf(.92f,.45f,low);
    float dusk=low*(1-night);
    ambR=lerpf(lerpf(.50f,.42f,dusk),.16f,night);ambG=lerpf(lerpf(.52f,.36f,dusk),.18f,night);ambB=lerpf(lerpf(.58f,.42f,dusk),.30f,night);
    /* cielo: azul claro de dia, naranja/violeta al amanecer y atardecer, azul noche */
    float r=lerpf(lerpf(155,236,dusk),14,night),g=lerpf(lerpf(189,140,dusk),18,night),b=lerpf(lerpf(192,110,dusk),40,night);
    skyColor=COLOR((int)r,(int)g,(int)b);
}
static uint32_t lit_color(uint32_t c,float nx,float ny,float nz){
    float d=nx*sunX+ny*sunY+nz*sunZ;if(d<0)d=0;
    float direct=d*(1-night)*(.75f);                              /* difusa */
    float fr=ambR+direct*sunR,fg=ambG+direct*sunG,fb=ambB+direct*sunB;
    /* el cielo ilumina un poco las caras que miran arriba */
    float sky=(ny>0?ny:0)*.12f*(1-night);fr+=sky;fg+=sky;fb+=sky*1.2f;
    int R=(int)((c&255)*fr),G=(int)(((c>>8)&255)*fg),B=(int)(((c>>16)&255)*fb);
    if(R>255)R=255;if(G>255)G=255;if(B>255)B=255;return COLOR(R,G,B);
}
/* Atenuacion global para lo que no pasa por lit_color (coches, personas, detalles). */
static uint32_t day_scale(uint32_t c){
    if(night<=0)return c;
    float f=1-night*.62f,fb=1-night*.45f;
    return COLOR((int)((c&255)*f),(int)(((c>>8)&255)*f),(int)(((c>>16)&255)*fb));
}
/* Materiales que emiten luz de noche (escaparates, torres de cristal, farolas). */
static uint32_t emissive_color(int mat,uint32_t c){
    if(night<=0)return c;
    if(mat==SHOP||mat==GLASS){float w=night*.9f;int R=(int)(((c&255))*(1-w)+250*w),G=(int)((((c>>8)&255))*(1-w)+225*w),B=(int)((((c>>16)&255))*(1-w)+150*w);return COLOR(R,G,B);}
    return c;
}
static void glow_quad(Point a,Point b,Point c,Point d,uint32_t color){
    if(glowUsed+6>GLOW_MAX)return;
    Vertex *p=glowMesh+glowUsed;glowUsed+=6;
    Vertex va={0,0,color,a.x,a.y,a.z},vb={1,0,color,b.x,b.y,b.z},vc={1,1,color,c.x,c.y,c.z},vd={0,1,color,d.x,d.y,d.z};
    p[0]=va;p[1]=vb;p[2]=vc;p[3]=va;p[4]=vc;p[5]=vd;
}
static void shadow_quad(Point a,Point b,Point c,Point d,uint32_t color){
    if(shadowUsed+6>GLOW_MAX/2)return;
    Vertex *p=shadowMesh+shadowUsed;shadowUsed+=6;
    Vertex va={0,0,color,a.x,a.y,a.z},vb={1,0,color,b.x,b.y,b.z},vc={1,1,color,c.x,c.y,c.z},vd={0,1,color,d.x,d.y,d.z};
    p[0]=va;p[1]=vb;p[2]=vc;p[3]=va;p[4]=vc;p[5]=vd;
}
/* Convierte coordenadas logicas a proyectadas (mismo camino que polygon() en modo no rigido). */
static Point geo_point(float x,float y,float z){float gx,gz;geo_project(x,z,&gx,&gz);return point(gx,y+geo_height(x,z),gz);}
/* Sombra proyectada de un objeto: elipse oscura en el suelo, desplazada y alargada segun el sol. */
static void cast_shadow(float x,float z,float radius,float height){
    if(night>.85f)return;
    float len=sunY>.15f?height*(1-sunY)/sunY:height*5.6f;if(len>radius*4)len=radius*4;
    float dx=-sunX,dz=-sunZ;float n=sqrtf(dx*dx+dz*dz);if(n<.001f){dx=1;dz=0;n=1;}dx/=n;dz/=n;
    float px=-dz,pz=dx;float alpha=(1-night)*(.55f-.25f*(1-sunY));
    uint32_t c=((int)(alpha*255)<<24)|0x000000;
    Point a=geo_point(x-px*radius,0,z-pz*radius),b=geo_point(x+px*radius,0,z+pz*radius);
    Point cc=geo_point(x+px*radius*.7f+dx*(radius+len),0,z+pz*radius*.7f+dz*(radius+len)),d=geo_point(x-px*radius*.7f+dx*(radius+len),0,z-pz*radius*.7f+dz*(radius+len));
    a.y+=.35f;b.y+=.35f;cc.y+=.35f;d.y+=.35f;
    shadow_quad(a,b,cc,d,c);
}
/* Charco de luz en el suelo (farolas, faros): rombo aditivo. */
static void light_pool(float x,float z,float rx,float rz,float angle,uint32_t color){
    float c=cosf(angle),s=sinf(angle);
    Point p[4]={geo_point(x+(-rx)*c-(-rz)*s,0,z+(-rx)*s+(-rz)*c),geo_point(x+(rx)*c-(-rz)*s,0,z+(rx)*s+(-rz)*c),geo_point(x+(rx)*c-(rz)*s,0,z+(rx)*s+(rz)*c),geo_point(x+(-rx)*c-(rz)*s,0,z+(-rx)*s+(rz)*c)};
    for(int i=0;i<4;i++)p[i].y+=.45f;
    glow_quad(p[0],p[1],p[2],p[3],color);
}
/* Cono de faros delante de un coche (dos haces + el propio faro encendido). */
static void headlights(float x,float z,float angle){
    if(night<.2f)return;
    float c=cosf(angle),s=sinf(angle);uint32_t beam=((int)(110*night)<<24)|COLOR(255,240,190)&0x00ffffff;
    for(int side=-1;side<=1;side+=2){
        float ox=x+c*18-s*side*6,oz=z+s*18+c*side*6;             /* posicion del faro */
        Point a=geo_point(ox-s*side*2,0,oz+c*side*2),b=geo_point(ox+s*side*2,0,oz-c*side*2);
        Point d=geo_point(ox+c*70-s*side*16,0,oz+s*70+c*side*16),cc=geo_point(ox+c*70+s*side*4,0,oz+s*70-c*side*4);
        a.y+=.5f;b.y+=.5f;cc.y+=.5f;d.y+=.5f;
        glow_quad(a,b,cc,d,beam);
        Point f=geo_point(ox,9,oz);
        glow_quad(point(f.x-s*1.6f,f.y-1.4f,f.z+c*1.6f),point(f.x+s*1.6f,f.y-1.4f,f.z-c*1.6f),point(f.x+s*1.6f,f.y+1.4f,f.z-c*1.6f),point(f.x-s*1.6f,f.y+1.4f,f.z+c*1.6f),((int)(230*night)<<24)|COLOR(255,250,220)&0x00ffffff);
    }
}
/* Nubes: planos translucidos a gran altura que derivan con el viento; forma irregular por hash. */
static void clouds(float time){
    float wind=time*3.5f;
    for(int i=0;i<18;i++){
        unsigned h=city_hash(i*7,13,3);
        float cx=fmodf((h&1023)*3.2f+wind*(1+(h>>10&3)*.2f),3600.0f)-500,cz=(h>>12&1023)*2.4f-200;
        float y=260+(h>>22&63),w=120+(h>>5&127),d=70+(h>>9&63);
        if(!nearby(cx,cz,1100))continue;
        float bright=lerpf(1,.30f,night);int a=(int)(lerpf(150,110,night));
        uint32_t col=(a<<24)|COLOR((int)(255*bright),(int)(250*bright),(int)(252*bright))&0x00ffffff;
        float gx,gz;geo_project(cx,cz,&gx,&gz);
        glow_quad(point(gx-w*.5f,y,gz-d*.5f),point(gx+w*.5f,y,gz-d*.5f),point(gx+w*.5f,y,gz+d*.5f),point(gx-w*.5f,y,gz+d*.5f),col);
        /* segundo plano mas pequeno y alto: volumen */
        glow_quad(point(gx-w*.3f,y+14,gz-d*.3f),point(gx+w*.35f,y+14,gz-d*.3f),point(gx+w*.3f,y+14,gz+d*.35f),point(gx-w*.3f,y+14,gz+d*.3f),col);
    }
    /* disco solar / luna */
    float sx=eye.x+sunX*600,sy=eye.y+sunY*600,sz=eye.z+sunZ*600;
    if(sunY>-.1f){uint32_t sc=(200<<24)|COLOR(255,(int)lerpf(240,180,1-sunY),(int)lerpf(200,90,1-sunY))&0x00ffffff;float r=26;
        glow_quad(point(sx-r,sy-r,sz),point(sx+r,sy-r,sz),point(sx+r,sy+r,sz),point(sx-r,sy+r,sz),sc);}
    float mx=eye.x-sunX*600,my=eye.y-sunY*600,mz=eye.z-sunZ*600;
    if(-sunY>-.1f){uint32_t mc=(170<<24)|COLOR(220,225,240)&0x00ffffff;float r=16;
        glow_quad(point(mx-r,my-r,mz),point(mx+r,my-r,mz),point(mx+r,my+r,mz),point(mx-r,my+r,mz),mc);}
}
/* Farolas encendidas de noche: charco de luz en cada esquina de manzana visible. */
static void street_lamps_glow(void){
    if(night<.2f)return;
    uint32_t warm=((int)(120*night)<<24)|COLOR(255,225,150)&0x00ffffff;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        float x=bx*320+82,z=bz*320+59;
        if(!nearby(x,z,420)||!sphere_visible(x,z,40))continue;
        light_pool(x,z,26,20,0,warm);
        Point l=geo_point(x,34,z);
        glow_quad(point(l.x-4,l.y-1.5f,l.z-3),point(l.x+4,l.y-1.5f,l.z-3),point(l.x+4,l.y+1.5f,l.z+3),point(l.x-4,l.y+1.5f,l.z+3),((int)(240*night)<<24)|COLOR(255,240,190)&0x00ffffff);
    }
}
