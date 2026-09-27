/* v2.37 (Claude): pantalla de carga real. Tras cargar partida, cuanto cuesta que la ciudad visible
   quede en cache: sin precarga (1 pieza por fotograma en juego) frente a r3_prewarm en la carga. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <time.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
int main(int argc,char **argv){
 int warm=argc>1;R3Scene s={0};s.cameraDistance=50;s.eyeHeight=34;s.target=-1;s.x=1342;s.z=1022;s.yaw=.7f;s.time=10;
 double t0=now();int calls=0,built=0,k;
 if(warm)while(calls<14&&(k=r3_prewarm(&s))>0){calls++;built+=k;}
 double tw=now()-t0;
 double first=0,worst=0;int slow=0;
 for(int f=0;f<120;f++){unsigned long b=r3CacheNew+r3CacheLight;double a=now();r3_draw(NULL,&s);double d=now()-a;
  if(f<30)first+=d;if(d>worst)worst=d;if(r3CacheNew+r3CacheLight>b)slow=f+1;}
 printf("%s: precarga %d llamadas %d piezas %.1f ms | primeros 30 fotogramas %.1f ms | peor %.2f ms | ultima reconstruccion en el fotograma %d",
  warm?"CON precarga":"SIN precarga",calls,built,tw,first,worst,slow);putchar(10);
 return warm&&(slow>10||calls>=14)?1:0;
}
