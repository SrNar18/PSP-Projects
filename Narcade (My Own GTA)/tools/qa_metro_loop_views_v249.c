/* v2.49 (Claude): el Metro circular cabe en el presupuesto de mallas: seis estaciones y seis esquinas vistas desde el
   suelo y a bordo, con el tren en la estacion, sin desbordes. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
int main(void){R3Scene s={0};s.cameraDistance=60;s.eyeHeight=40;s.target=-1;int shots=0;
 for(int i=0;i<ML_STATIONS*2;i++){float sp=i<ML_STATIONS?ml_station_s(i):ml_station_s(i-ML_STATIONS)+ml_length()/12;
  float x,z,tx,tz;ml_point(sp,&x,&z,&tx,&tz);s.x=x-tz*30;s.z=z+tx*30;s.metroZ=sp;
  for(int a=0;a<4;a++){s.yaw=a*1.5708f;s.inMetro=a==3;for(int k=0;k<8;k++)r3_prewarm(&s);r3_draw(NULL,&s);
   if(overflow){printf("FAIL: vista %d/%d desborda (FLAT %d METAL %d CONCRETE %d)\n",i,a,used[FLAT],used[METAL],used[CONCRETE]);return 1;}shots++;}}
 printf("PASS: %d vistas del Metro circular (estaciones, tramos y a bordo) sin desbordes.\n",shots);return 0;}
