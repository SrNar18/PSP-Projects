/* v2.30 (Claude): coste de dibujo por fotograma segun la zona. Cuenta los
   vertices que el renderizador envia a la PSP (mismo codigo) mirando a lo largo
   de calles llanas (este-oeste) y de calles en pendiente (norte-sur). Si en las
   cuestas se dibuja mucho mas, la PSP baja de fotogramas: tirones. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
static int total(void){int t=0;for(int m=0;m<MAT_COUNT;m++)t+=used[m];return t;}
int main(void){
 R3Scene s={0};s.cameraDistance=50;s.eyeHeight=34;s.target=-1;
 long flatSum=0,slopeSum=0;int flatN=0,slopeN=0,flatMax=0,slopeMax=0;
 for(int row=0;row<7;row++)for(float x=100;x<2500;x+=60){ /* calles E-O (llanas) */
  for(int d=0;d<2;d++){s.x=x;s.z=row*320+42;s.yaw=geo_heading(s.x,s.z,d?PI:0);r3_draw(NULL,&s);int t=total();flatSum+=t;flatN++;if(t>flatMax)flatMax=t;}
 }
 for(int col=0;col<8;col++)for(float z=100;z<2150;z+=60){ /* calles N-S (pendiente) */
  for(int d=0;d<2;d++){s.x=col*320+42;s.z=z;s.yaw=geo_heading(s.x,s.z,d?-PI*.5f:PI*.5f);r3_draw(NULL,&s);int t=total();slopeSum+=t;slopeN++;if(t>slopeMax)slopeMax=t;}
 }
 printf("vertices por fotograma | llano: medio %ld max %d | pendiente: medio %ld max %d\n",flatSum/flatN,flatMax,slopeSum/slopeN,slopeMax);
 return 0;
}
