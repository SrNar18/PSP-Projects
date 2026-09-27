/* v2.36 (Claude): coste del dibujo fotograma a fotograma corriendo a velocidad de sprint por
   las calles (105 u/s), con la hora avanzando. Los tirones al correr son picos: se mide la
   media, el percentil 95 y el maximo, y cuantas manzanas se reconstruyen en la cache. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
static int cmpd(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return x<y?-1:x>y;}
unsigned long rebuilds;
int main(void){
 R3Scene s={0};s.cameraDistance=50;s.eyeHeight=34;s.target=-1;s.carCount=20;s.personCount=10;
 enum{N=6000};static double t[N];
 /* recorrido: calles E-O y N-S alternas, sprint 1,75 unidades por fotograma */
 float x=62,z=42,dx=1,dz=0;int leg=0;
 for(int f=0;f<N;f++){
  x+=dx*1.75f;z+=dz*1.75f;
  if(dx>0&&x>2500){dx=0;dz=1;leg++;} else if(dz>0&&z>42+320*((leg+1)/2)&&z<2200&&leg%2==1){dz=0;dx=(leg/2)%2?1:-1;leg++;}
  if(dx<0&&x<62){dx=0;dz=1;leg++;}
  if(z>2190){z=42;}
  s.x=x;s.z=z;s.yaw=geo_heading(x,z,atan2f(dz,dx));s.time=f/60.f*8; /* hora x8: estresa la luz del dia */
  for(int i=0;i<20;i++){s.cars[i].x=x+(i%5)*60-120;s.cars[i].z=z+(i/5)*70-100;}
  unsigned long b0=r3CacheNew+r3CacheLight;double a=now();r3_draw(NULL,&s);t[f]=now()-a;
  if(f>60&&t[f]>2.5){printf("  pico f%d %.2f ms en (%.0f,%.0f) reconstrucciones en el fotograma %lu",f,t[f],x,z,r3CacheNew+r3CacheLight-b0);putchar(10);}
 }
 double sum=0;for(int i=0;i<N;i++)sum+=t[i];
 /* sin el arranque en frio (primeros 60 fotogramas: nada en cache) */
 for(int i=0;i<60;i++)t[i]=0;
 qsort(t,N,sizeof(double),cmpd);
 printf("reconstrucciones: nuevas/LOD %lu | por luz %lu | vaciados %lu",r3CacheNew,r3CacheLight,r3CacheReset);putchar(10);
 printf("sprint: media %.3f ms | p95 %.3f | p99 %.3f | max %.3f (PC)",sum/N,t[N*95/100],t[N*99/100],t[N-1]);putchar(10);
 return 0;
}
