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
#define GLOW_MAX 6000
static Vertex __attribute__((aligned(16))) glowMesh[GLOW_MAX];static int glowUsed=0;
static Vertex __attribute__((aligned(16))) shadowMesh[GLOW_MAX/2];static int shadowUsed=0;
static float dayT=.35f;              /* 0 = medianoche, .25 = amanecer, .5 = mediodia, .75 = atardecer */
static float sunX=0,sunY=1,sunZ=0;   /* direccion HACIA el sol */
static float sunR=1,sunG=1,sunB=1;   /* color de la luz directa */
static float ambR=.5f,ambG=.5f,ambB=.55f;
static float night=0;                /* 0 dia ... 1 noche cerrada */
static uint32_t skyColor=COLOR(155,189,192);
static uint32_t horizonColor=COLOR(205,222,214); /* v2.6: bruma del horizonte (mas clara y calida que el cenit); tambien color de niebla */
static float lerpf(float a,float b,float t){return a+(b-a)*t;}
static float smooth01(float t){t=t<0?0:t>1?1:t;return t*t*(3-2*t);}
static void day_update(float time){
    dayT=fmodf(time/DAY_SECONDS+.30f,1.0f);if(dayT<0)dayT+=1;   /* la partida empieza a media manana */
#ifdef NARCADE_HOUR
    dayT=NARCADE_HOUR; /* solo pruebas: hora fija (0 medianoche .. .5 mediodia) */
#endif
    float ang=(dayT-.25f)*2*PI;                                   /* amanecer en el este, atardecer en el oeste */
    float elev=sinf(ang)*.85f+.28f;if(elev>1)elev=1;              /* sesgo: el sol pasa ~62% del ciclo sobre el horizonte (noche corta) */
    sunX=cosf(ang)*.8f;sunY=elev;sunZ=.35f;
    float n=sqrtf(sunX*sunX+sunY*sunY+sunZ*sunZ);sunX/=n;sunY/=n;sunZ/=n;
    night=1-smooth01((elev+.12f)/.30f);                          /* transicion en el crepusculo */
    float low=1-smooth01((elev-.05f)/.35f);                       /* sol bajo: luz calida y rasante */
    /* Stronger warm key and cooler shadow fill: the city's materials retain
       their hue instead of converging on the same pale grey at sunset. */
    sunR=lerpf(1.10f,1.38f,low);sunG=lerpf(1.02f,.78f,low);sunB=lerpf(.98f,.50f,low);
    float dusk=low*(1-night);
    ambR=lerpf(lerpf(.47f,.43f,dusk),.22f,night);ambG=lerpf(lerpf(.50f,.40f,dusk),.25f,night);ambB=lerpf(lerpf(.57f,.55f,dusk),.36f,night);
    /* cielo: azul claro de dia, naranja/violeta al amanecer y atardecer, azul noche */
    float r=lerpf(lerpf(88,195,dusk),14,night),g=lerpf(lerpf(154,100,dusk),18,night),b=lerpf(lerpf(211,168,dusk),40,night);
    skyColor=COLOR((int)r,(int)g,(int)b);
    /* horizonte: bruma clara de dia, naranja intenso al amanecer/atardecer, gris-azul apagado de noche */
    float hr=lerpf(lerpf(125,251,dusk),22,night),hg=lerpf(lerpf(184,128,dusk),30,night),hb=lerpf(lerpf(225,112,dusk),58,night);
    horizonColor=COLOR((int)hr,(int)hg,(int)hb);
}
static uint32_t lit_color(uint32_t c,float nx,float ny,float nz){
    /* v2.50 (Claude): difusa con "wrap" suave (las caras de canto no pasan de golpe a ambiente) */
    float d=(nx*sunX+ny*sunY+nz*sunZ+.15f)*(1/1.15f);if(d<0)d=0;
    float direct=d*(1-night)*(.95f);                              /* difusa */
    float fr=ambR+direct*sunR,fg=ambG+direct*sunG,fb=ambB+direct*sunB;
    /* v2.50: rebote calido del suelo en las fachadas a la sombra (antes quedaban grises y planas) */
    if(ny<.5f&&ny>-.5f){float bounce=(1-d)*.09f*(1-night);fr+=bounce*1.10f;fg+=bounce*.95f;fb+=bounce*.75f;}
    /* v2.39 (Claude): luz de luna de noche (fria, tenue, desde el noroeste alto). Antes de noche solo habia
       ambiente: todas las caras igual de oscuras y la ciudad se veia plana. */
    if(night>0){float m=nx*-.42f+ny*.78f+nz*-.46f;if(m>0){m*=night*.30f;fr+=m*.60f;fg+=m*.70f;fb+=m*.95f;}}
    /* y las caras que miran abajo (voladizos, bajos) reciben menos luz del cielo */
    if(ny<0){float k=1+ny*.18f;fr*=k;fg*=k;fb*=k;}
    /* el cielo ilumina un poco las caras que miran arriba */
    float sky=(ny>0?ny:0)*.12f*(1-night);fr+=sky;fg+=sky;fb+=sky*1.2f;
    float R=(c&255)*fr,G=((c>>8)&255)*fg,B=((c>>16)&255)*fb;
    /* v2.50: hombro suave por encima de 196 en lugar de recortar a 255: los blancos al sol no se queman
       ni pierden el matiz (antes fachadas claras y aceras quedaban en blanco plano). */
    #define LIT_SHOULDER(v) ((v)<196?(v):196+59*((v)-196)/((v)-196+59))
    R=LIT_SHOULDER(R);G=LIT_SHOULDER(G);B=LIT_SHOULDER(B);
    #undef LIT_SHOULDER
    return COLOR((int)R,(int)G,(int)B);
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
    if(mat==CURTAIN||mat==MODERN||mat==OFFICE_FRONT){float w=night*.14f;int R=(int)(((c&255))*(1-w)+240*w),G=(int)((((c>>8)&255))*(1-w)+220*w),B=(int)((((c>>16)&255))*(1-w)+160*w);return COLOR(R,G,B);}
    if(mat==SHOP||mat==GLASS||mat==RETAIL||mat==EATERY){float w=night*.38f;int R=(int)(((c&255))*(1-w)+250*w),G=(int)((((c>>8)&255))*(1-w)+225*w),B=(int)((((c>>16)&255))*(1-w)+150*w);return COLOR(R,G,B);}
    return c;
}
/* Effects need the same near-plane clipping as world geometry. Preserve alpha
   gradients at clipping intersections rather than discarding an entire pool. */
static Vertex fx_lerp(Vertex a,Vertex b,float t){
    Vertex v=interpolate(a,b,t);v.color=0;
    for(int shift=0;shift<32;shift+=8){float av=(a.color>>shift)&255,bv=(b.color>>shift)&255;v.color|=(uint32_t)(av+(bv-av)*t)<<shift;}return v;
}
static void fx_triangle(Vertex a,Vertex b,Vertex c,int shadow){
    if(cacheRec){RecHdr h={(unsigned char)(shadow?2:1),0,3,0};cache_put(&h,4);cache_put(&a,sizeof a);cache_put(&b,sizeof b);cache_put(&c,sizeof c);return;}
    Vertex buf[2][12];buf[0][0]=a;buf[0][1]=b;buf[0][2]=c;int count=3,src=0;
    for(int k=0;k<6;k++){
        int inside=0;for(int i=0;i<count;i++)inside+=plane_distance(&buf[src][i],k)>=0;
        if(!inside)return;if(inside==count)continue;
        int out=0;Vertex p=buf[src][count-1];float pd=plane_distance(&p,k);
        for(int i=0;i<count;i++){Vertex q=buf[src][i];float qd=plane_distance(&q,k);
            if((pd>=0)!=(qd>=0))buf[src^1][out++]=fx_lerp(p,q,pd/(pd-qd));
            if(qd>=0)buf[src^1][out++]=q;p=q;pd=qd;
        }count=out;src^=1;if(count<3)return;
    }
    int *used=shadow?&shadowUsed:&glowUsed,limit=shadow?GLOW_MAX/2:GLOW_MAX;
    Vertex *mesh=shadow?shadowMesh:glowMesh;int needed=(count-2)*3;if(*used+needed>limit)return;
    for(int i=1;i<count-1;i++){mesh[(*used)++]=buf[src][0];mesh[(*used)++]=buf[src][i];mesh[(*used)++]=buf[src][i+1];}
}
static Vertex fx_vertex(Point p,uint32_t color){Vertex v={0,0,color,p.x,p.y,p.z};return v;}
static void glow_quad(Point a,Point b,Point c,Point d,uint32_t color){
    fx_triangle(fx_vertex(a,color),fx_vertex(b,color),fx_vertex(c,color),0);
    fx_triangle(fx_vertex(a,color),fx_vertex(c,color),fx_vertex(d,color),0);
}
static void shadow_quad(Point a,Point b,Point c,Point d,uint32_t color){
    fx_triangle(fx_vertex(a,color),fx_vertex(b,color),fx_vertex(c,color),1);
    fx_triangle(fx_vertex(a,color),fx_vertex(c,color),fx_vertex(d,color),1);
}
/* Convierte coordenadas logicas a proyectadas (mismo camino que polygon() en modo no rigido). */
static Point geo_point(float x,float y,float z){float gx,gz;geo_project(x,z,&gx,&gz);return point(gx,y+geo_height(x,z),gz);}
static Point fx_building(Point p){float gx,gz;geo_project(p.x,p.z,&gx,&gz);return point(gx,p.y+(fixedGround>-999999?fixedGround:geo_height(p.x,p.z)),gz);}
static void facade_lights(Point a,Point b,float height,float nx,float nz,int material){
    if(night<.05f||!geographic||rigid||height<18)return;
    if(material!=BRICK&&material!=STUCCO&&material!=CURTAIN&&material!=MODERN)return;
    float mx=(a.x+b.x)*.5f,mz=(a.z+b.z)*.5f;if(!nearby(mx,mz,210))return;
    float dx=b.x-a.x,dz=b.z-a.z,len=hypotf(dx,dz);if(len<22)return;
    int cols=(int)(len/27);if(cols>6)cols=6;int rows=(int)(height/24);if(rows>4)rows=4;if(rows<1)rows=1;
    for(int row=0;row<rows;row++)for(int col=0;col<cols;col++){
        unsigned hash=city_hash((int)mx+col,(int)mz+row,17);if(hash%3==0)continue;
        float t=(col+.5f)/cols,w=fminf(7,len/cols*.28f)/len;
        float y=a.y+row*24+9,hh=fminf(8,height-row*24-10);if(hh<2)continue;
        /* v2.48 (Claude): 0,8 fuera de la pared (antes 0,25: a ~200 de distancia eran 2 pasos de profundidad y la
           ventana iluminada parpadeaba contra la fachada) */
        Point p=point(a.x+dx*(t-w)+nx*.8f,y,a.z+dz*(t-w)+nz*.8f);
        Point q=point(a.x+dx*(t+w)+nx*.8f,y,a.z+dz*(t+w)+nz*.8f);
        Point r=q,s=p;r.y+=hh;s.y+=hh;
        uint32_t rgb=hash&1?COLOR(255,204,130):COLOR(170,202,228);
        uint32_t color=((uint32_t)(int)(night*125)<<24)|(rgb&0xffffff);
        glow_quad(fx_building(p),fx_building(q),fx_building(r),fx_building(s),color);
    }
}
static void soft_ground(float x,float z,float rx,float rz,float angle,uint32_t color,int shadow){
    float c=cosf(angle),s=sinf(angle);Point centre=geo_point(x,shadow?.38f:.55f,z);
    for(int k=0;k<12;k++){
        float a=k*2*PI/12,b=(k+1)*2*PI/12;
        Point p=geo_point(x+cosf(a)*rx*c-sinf(a)*rz*s,shadow?.38f:.55f,z+cosf(a)*rx*s+sinf(a)*rz*c);
        Point q=geo_point(x+cosf(b)*rx*c-sinf(b)*rz*s,shadow?.38f:.55f,z+cosf(b)*rx*s+sinf(b)*rz*c);
        fx_triangle(fx_vertex(centre,color),fx_vertex(p,color&0xffffff),fx_vertex(q,color&0xffffff),shadow);
    }
}
static void sky_disc(Point centre,float rx,float ry,uint32_t color){
    Point f=unit(point(target.x-eye.x,target.y-eye.y,target.z-eye.z));
    Point r=unit(point(-f.z,0,f.x)),u=cross3(r,f);
    for(int k=0;k<12;k++){
        float a=k*2*PI/12,b=(k+1)*2*PI/12;
        Point p=point(centre.x+r.x*cosf(a)*rx+u.x*sinf(a)*ry,centre.y+u.y*sinf(a)*ry,centre.z+r.z*cosf(a)*rx+u.z*sinf(a)*ry);
        Point q=point(centre.x+r.x*cosf(b)*rx+u.x*sinf(b)*ry,centre.y+u.y*sinf(b)*ry,centre.z+r.z*cosf(b)*rx+u.z*sinf(b)*ry);
        fx_triangle(fx_vertex(centre,color),fx_vertex(p,color&0xffffff),fx_vertex(q,color&0xffffff),0);
    }
}
/* Sombra proyectada de un objeto: elipse oscura en el suelo, desplazada y alargada segun el sol. */
static void cast_shadow(float x,float z,float radius,float height){
    float len=sunY>.15f?height*(1-sunY)/sunY:height*5.6f;if(len>radius*4)len=radius*4;
    float dx=-sunX,dz=-sunZ;float n=sqrtf(dx*dx+dz*dz);if(n<.001f){dx=1;dz=0;n=1;}dx/=n;dz/=n;
    len*=1-night;float alpha=.25f+(1-night)*.35f;
    uint32_t c=((uint32_t)(int)(alpha*255)<<24)|0x000000;
    soft_ground(x+dx*len*.35f,z+dz*len*.35f,radius+len*.65f,radius,atan2f(dz,dx),c,1);
}
/* Charco de luz en el suelo (farolas, faros): rombo aditivo. */
static void building_shadows(void){
    if(night>.8f||sunY<.03f)return;
    float length=fminf(42,24/sunY),dx=-sunX*length,dz=-sunZ*length;
    uint32_t nearColor=(uint32_t)((1-night)*65)<<24;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        if(!nearby(bx*320+160,bz*320+160,280))continue;
        const CmParcel *parcels;int n=cm_parcels(bx,bz,&parcels);
        for(int i=0;i<n;i++){const CmParcel *p=&parcels[i];if(p->kind)continue;float cx,cz;cm_centroid(p,&cx,&cz);
            for(int k=0;k<p->n;k++){int j=(k+1)%p->n;float mx=(p->x[k]+p->x[j])*.5f,mz=(p->z[k]+p->z[j])*.5f;
                if((mx-cx)*dx+(mz-cz)*dz<=0)continue;
                Point a=geo_point(p->x[k],.28f,p->z[k]),b=geo_point(p->x[j],.28f,p->z[j]);
                Point c=geo_point(p->x[j]+dx,.28f,p->z[j]+dz),d=geo_point(p->x[k]+dx,.28f,p->z[k]+dz);
                fx_triangle(fx_vertex(a,nearColor),fx_vertex(b,nearColor),fx_vertex(c,0),1);
                fx_triangle(fx_vertex(a,nearColor),fx_vertex(c,0),fx_vertex(d,0),1);
            }
        }
    }
}
static void light_pool(float x,float z,float rx,float rz,float angle,uint32_t color){
    soft_ground(x,z,rx,rz,angle,color,0);
}
/* Projected road point: headlights keep a straight bearing through the
   geographical bend while each control point still rests on the terrain. */
static Point beam_ground(float px,float pz){float wx,wz;geo_unproject(px,pz,&wx,&wz);return point(px,geo_height(wx,wz)+.57f,pz);}
/* Cono de faros delante de un coche (dos haces + el propio faro encendido). */
static void headlights(float x,float z,float angle){
    if(night<.2f||!nearby(x,z,170))return; /* distant car LOD has no individual lamps */
    float c=cosf(angle),s=sinf(angle),heading=geo_heading(x,z,angle);
    float fc=cosf(heading),fs=sinf(heading);
    uint32_t rgb=COLOR(255,239,188)&0x00ffffff;
    uint32_t bright=((uint32_t)(int)(108*night)<<24)|rgb;
    uint32_t soft=((uint32_t)(int)(57*night)<<24)|rgb;
    uint32_t clear=rgb;
    float centreX,centreZ;geo_project(x,z,&centreX,&centreZ);
    for(int side=-1;side<=1;side+=2){
        float hx=x+c*18-s*side*6,hz=z+s*18+c*side*6;
        /* The body is rigid in projected space. Projecting the lamp offset
           independently bent its attachment away from that body on curves. */
        float px=centreX+fc*18-fs*side*6,pz=centreZ+fs*18+fc*side*6;
        float reach=64;for(float d=8;d<64;d+=8)if(cm_solid(hx+c*d,hz+s*d)){reach=d;break;}
        Point root=beam_ground(px+fc*2,pz+fs*2);
        Point mid=beam_ground(px+fc*reach*.48f,pz+fs*reach*.48f);
        Point left=beam_ground(mid.x-fs*8,mid.z+fc*8);
        Point right=beam_ground(mid.x+fs*8,mid.z-fc*8);
        Point tip=beam_ground(px+fc*reach,pz+fs*reach);
        fx_triangle(fx_vertex(root,bright),fx_vertex(left,clear),fx_vertex(mid,soft),0);
        fx_triangle(fx_vertex(root,bright),fx_vertex(mid,soft),fx_vertex(right,clear),0);
        fx_triangle(fx_vertex(mid,soft),fx_vertex(left,clear),fx_vertex(tip,clear),0);
        fx_triangle(fx_vertex(mid,soft),fx_vertex(tip,clear),fx_vertex(right,clear),0);
        Point f=point(px,geo_height(hx,hz)+9,pz);
        glow_quad(point(f.x-fs*1.6f,f.y-1.4f,f.z+fc*1.6f),point(f.x+fs*1.6f,f.y-1.4f,f.z-fc*1.6f),point(f.x+fs*1.6f,f.y+1.4f,f.z-fc*1.6f),point(f.x-fs*1.6f,f.y+1.4f,f.z+fc*1.6f),((uint32_t)(int)(230*night)<<24)|COLOR(255,250,220)&0x00ffffff);
    }
}
/* Nubes: planos translucidos a gran altura que derivan con el viento; forma irregular por hash. */
static void clouds(float time){
    /* Keep cloud groups around the camera instead of scattering them across
       the world grid. The old x/z range often left the visible sky empty. */
    float wind=time*.009f;
    float sunset=(1-smooth01((sunY-.05f)/.35f))*(1-night);
    for(int i=0;i<9;i++){
        unsigned h=city_hash(i*7,13,3);
        float angle=(i+wind*(1+(h>>10&3)*.15f))*2*PI/9;
        float distance=440+(h&63),y=eye.y+115+(h>>22&47);
        float w=43+(h>>5&31),d=21+(h>>9&15);
        float cx=eye.x+cosf(angle)*distance,cz=eye.z+sinf(angle)*distance;
        float bright=lerpf(1,.55f,night);int a=(int)(lerpf(170,75,night));
        uint32_t col=((uint32_t)a<<24)|COLOR((int)(255*bright),(int)((250-48*sunset)*bright),(int)((252-76*sunset)*bright))&0x00ffffff;
        uint32_t underside=((uint32_t)(a/3)<<24)|COLOR((int)((183+23*sunset)*bright),(int)((192-46*sunset)*bright),(int)((203-36*sunset)*bright))&0x00ffffff;
        sky_disc(point(cx,y-8,cz),w*.54f,d*.34f,underside);
        sky_disc(point(cx-w*.20f,y+3,cz),w*.34f,d*.63f,col);
        sky_disc(point(cx+w*.20f,y+5,cz),w*.31f,d*.59f,col);
        sky_disc(point(cx,y+10,cz),w*.39f,d*.72f,col);
    }
    /* disco solar / luna */
    float sx=eye.x+sunX*600,sy=eye.y+sunY*600,sz=eye.z+sunZ*600;
    if(sunY>-.1f){uint32_t sc=(200u<<24)|COLOR(255,(int)lerpf(240,180,1-sunY),(int)lerpf(200,90,1-sunY))&0x00ffffff;float r=26;
        uint32_t halo=((uint32_t)(int)(42+50*sunset)<<24)|COLOR(255,(int)(205-42*sunset),(int)(143-44*sunset))&0x00ffffff;
        sky_disc(point(sx,sy,sz),r*3.0f,r*3.0f,halo);
        sky_disc(point(sx,sy,sz),r,r,sc);}
    float mx=eye.x-sunX*600,my=eye.y-sunY*600,mz=eye.z-sunZ*600;
    if(-sunY>-.1f){uint32_t mc=(170u<<24)|COLOR(220,225,240)&0x00ffffff;float r=16;
        sky_disc(point(mx,my,mz),r,r,mc);}
}
/* Farolas encendidas de noche: charco de luz en cada esquina de manzana visible. */
static void street_lamps_glow(void){
    if(night<.2f)return;
    uint32_t warm=((uint32_t)(int)(120*night)<<24)|COLOR(255,225,150)&0x00ffffff;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        /* v2.34 (Claude): el charco de luz va donde esta la farola (movida a la acera en v2.33) */
        int ci=bz*8+bx;if(!furnMemoDone[ci]||!furnMemo[ci].ok[2])continue;
        float x=furnMemo[ci].x[2],z=furnMemo[ci].z[2];
        if(!nearby(x,z,420)||!sphere_visible(x,z,40))continue;
        /* The actual fixture hangs 7.5 units west of its pole (lamp_post PI).
           Illuminate the road below the fixture, not the pole's base. */
        float headX=x-7.5f;
        light_pool(headX,z,26,20,0,warm);
        Point l=geo_point(headX,28.3f,z);
        glow_quad(point(l.x-4,l.y-1.5f,l.z-3),point(l.x+4,l.y-1.5f,l.z-3),point(l.x+4,l.y+1.5f,l.z+3),point(l.x-4,l.y+1.5f,l.z+3),((uint32_t)(int)(240*night)<<24)|COLOR(255,240,190)&0x00ffffff);
    }
}
