/* v2.52 (Claude): the valley outline no longer makes sharp jogs. Every north/south street (and the metro ring on
   x=42 and x=1962) shifts sideways at most 40 render units where it crosses an east/west street, and
   geo_unproject stays the exact inverse of geo_project. */
#include <stdio.h>
#include <assert.h>
#include "../src/world_geo.h"
int main(void){
 float worst=0;
 for(int col=0;col<8;col++){float x=col*320+42;
  for(float z=0;z<2239;z+=1){float a,b,c,d;geo_project(x,z,&a,&b);geo_project(x,z+1,&c,&d);
   float slope=fabsf(c-a)/(d-b);if(slope>worst)worst=slope;}}
 /* 40 units of smoothstep over 86*1.5 render units: peak slope 1.5*40/129 = 0.47 */
 assert(worst<.5f);
 for(float x=-200;x<2700;x+=37)for(float z=0;z<2239;z+=29){float gx,gz,x2,z2;geo_project(x,z,&gx,&gz);geo_unproject(gx,gz,&x2,&z2);assert(fabsf(x2-x)<.05f&&fabsf(z2-z)<.01f);}
 printf("PASS: north/south streets bend at most %.0f degrees at a junction (before v2.52: up to 80); exact inverse.\n",atanf(worst)*57.29578f);
 return 0;
}
