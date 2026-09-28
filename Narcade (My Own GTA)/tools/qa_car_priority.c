#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
static uint32_t car_hash(void){
    uint32_t h=2166136261u;int materials[]={CAR_PAINT,CAR_SIDE,METAL,WHEEL,GLASS,FLAT};
    for(unsigned k=0;k<sizeof materials/sizeof materials[0];k++){
        int m=materials[k];const unsigned char *p=(const unsigned char*)mesh[m];
        for(unsigned n=0;n<(unsigned)used[m]*sizeof(Vertex);n++)h=(h^p[n])*16777619u;
    }return h;
}
int main(void){
    R3Scene s={0};s.x=1342;s.z=1022;s.cameraDistance=50;s.eyeHeight=34;s.time=100;s.target=-1;s.carCount=20;
    for(int i=0;i<20;i++)s.cars[i]=(R3Car){s.x+35+i*5.1f,s.z+13+(i%4)*9.3f,.2f,0,i%6,0,i%12};
    for(int i=0;i<20&&r3_prewarm(&s);i++);
    r3_draw(NULL,&s);uint32_t expected=car_hash();assert(!r3_overflow());
    for(int i=0;i<10;i++){R3Car t=s.cars[i];s.cars[i]=s.cars[19-i];s.cars[19-i]=t;}
    r3_draw(NULL,&s);assert(!r3_overflow());assert(expected==car_hash());
    /* Even a trim-constrained nearby car keeps front/rear lamp geometry. */
    memset(used,0,sizeof used);geographic=0;rigid=0;clipEnabled=0;used[METAL]=3102;
    s.x=0;s.z=0;view=&s;R3Car c={0};car(&c);if(used[FLAT]<120){fprintf(stderr,"missing lamps: %d vertices\n",used[FLAT]);return 1;}
    puts("PASS: reversing traffic indices preserves car meshes; constrained nearby cars retain four lamps.");return 0;
}
