#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>
int main(void){R3Scene s={0};s.x=1342;s.z=1022;s.cameraDistance=50;s.eyeHeight=34;s.time=100;r3_prewarm(&s);
 int count=0;for(int i=0;i<12;i++){assert(hullCache[i].profile&&hullCache[i].count>0);count++;}
 assert(count==12&&!hullWarmOnly);puts("PASS: all six car profiles and both LODs are prepared during loading.");return 0;}
