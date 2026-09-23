/* v2.26 (Claude): busca "paredes invisibles" comparando la colision del juego con
   la geometria que de verdad dibuja el renderizador (mismo codigo que la PSP).
   Para cada celda de la ciudad se genera la malla sin recorte de camara y se
   rasteriza en planta (espacio proyectado) la altura maxima dibujada. Un punto
   que la colision marca como solido pero donde no se dibuja nada que sobresalga
   del suelo es una pared invisible. Escribe build/collision_audit.ppm.
   Compilar: gcc -O2 -DNARCADE_3D -DAB_NOPLAYER -DAB_NOPEOPLE -DAB_NOFX -Isrc tools/qa_invisible_walls_v226.c -lm */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
/* Colision del juego (copia fiel de game.c: solid() + terminales). */
static int game_solid(float x,float y){
 if(x<8||y<8||x>2560-8||y>2240-8)return 1;
 int ly=(int)y%320;
 if(x>1396&&x<1460&&ly>86){int bridge=0;for(int row=0;row<7;row++)if(fabsf(y-cm_footbridge_z(row))<13.f)bridge=1;if(!bridge)return 1;}
 return cm_solid(x,y)||cm_obstacle(x,y);
}
#define GW 2700
#define GH 3400
static float *hmax,*hmin;
static void put(int x,int z,float y){if(x<0||z<0||x>=GW||z>=GH)return;int i=z*GW+x;if(y>hmax[i])hmax[i]=y;if(y<hmin[i])hmin[i]=y;}
static void line3(Vertex a,Vertex b){
 float dx=b.x-a.x,dz=b.z-a.z;int n=(int)(fmaxf(fabsf(dx),fabsf(dz))*2)+1;if(n>4000)return;
 for(int i=0;i<=n;i++){float t=(float)i/n;put((int)floorf(a.x+dx*t),(int)floorf(a.z+dz*t),a.y+(b.y-a.y)*t);}
}
static void tri(Vertex a,Vertex b,Vertex c){
 line3(a,b);line3(b,c);line3(c,a);
 float minx=fminf(a.x,fminf(b.x,c.x)),maxx=fmaxf(a.x,fmaxf(b.x,c.x)),minz=fminf(a.z,fminf(b.z,c.z)),maxz=fmaxf(a.z,fmaxf(b.z,c.z));
 if(maxx-minx>700||maxz-minz>700)return;
 float den=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);if(fabsf(den)<1e-6f)return;
 for(int z=(int)floorf(minz);z<=(int)ceilf(maxz);z++)for(int x=(int)floorf(minx);x<=(int)ceilf(maxx);x++){
  float px=x+.5f,pz=z+.5f;
  float w1=((b.z-c.z)*(px-c.x)+(c.x-b.x)*(pz-c.z))/den,w2=((c.z-a.z)*(px-c.x)+(a.x-c.x)*(pz-c.z))/den,w3=1-w1-w2;
  if(w1<-.01f||w2<-.01f||w3<-.01f)continue;
  put(x,z,a.y*w1+b.y*w2+c.y*w3);
 }
}
int main(void){
 hmax=malloc(sizeof(float)*GW*GH);hmin=malloc(sizeof(float)*GW*GH);
 for(long i=0;i<(long)GW*GH;i++){hmax[i]=-1e9f;hmin[i]=1e9f;}
 R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 int overflows=0;
 /* Visor cada 80 unidades: solo se guarda la geometria cercana (detalle completo, sin LOD). */
 for(float vz=40;vz<2240;vz+=80)for(float vx=40;vx<2560;vx+=80){
  s.x=vx;s.z=vz;s.yaw=0;r3_draw(NULL,&s);if(overflow)overflows++;
  float cx,cz;geo_project(vx,vz,&cx,&cz);
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i+2<used[m];i+=3){
   Vertex *v=&mesh[m][i];
   float mx=(v[0].x+v[1].x+v[2].x)/3,mz=(v[0].z+v[1].z+v[2].z)/3;
   if(fabsf(mx-cx)>170||fabsf(mz-cz)>200)continue; /* solo lo cercano a este visor */
   if(m==GRASS&&v[0].y>200)continue;
   tri(v[0],v[1],v[2]);
  }
 }
 /* Auditoria: puntos solidos para el juego sin nada dibujado encima del suelo. */
 long solidPts=0,invisible=0,walkThrough=0;static long cellInv[64];
 static unsigned char img[GH/2][GW/2][3];
 for(float y=10;y<2230;y+=1)for(float x=10;x<2550;x+=1){
  float gx,gz;geo_project(x,y,&gx,&gz);int ix=(int)floorf(gx),iz=(int)floorf(gz);
  if(ix<1||iz<1||ix>=GW-1||iz>=GH-1)continue;
  float top=-1e9f,ground=geo_height(x,y);
  for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++){int i=(iz+dz)*GW+ix+dx;if(hmax[i]>top)top=hmax[i];}
  int drawn=top>ground+2.5f,coll=game_solid(x,y);
  int ox=ix/2,oz=iz/2;if(ox>=GW/2||oz>=GH/2)continue;
  unsigned char *p=img[oz][ox];
  if(coll){solidPts++;if(!drawn&&!(x>1390&&x<1466)&&x>12&&y>12&&x<2548&&y<2228){invisible++;int c=(int)(y/320)*8+(int)(x/320);cellInv[c]++;p[0]=255;p[1]=0;p[2]=0;}else if(!p[0]||p[1]){p[0]=90;p[1]=90;p[2]=90;}}
  else if(!p[0]||p[1]==p[0]){p[0]=p[1]=p[2]=drawn?200:30;if(drawn){p[0]=60;p[1]=120;p[2]=220;walkThrough++;}}
 }
 for(int c=0;c<56;c++)if(cellInv[c]>30)printf("celda bx%d bz%d: %ld puntos invisibles\n",c%8,c/8,cellInv[c]);
 FILE *f=fopen("build/collision_audit.ppm","wb");fprintf(f,"P6 %d %d 255\n",GW/2,GH/2);fwrite(img,1,sizeof(img),f);fclose(f);
 printf("overflows %d | puntos solidos %ld | PAREDES INVISIBLES fuera del rio y del borde %ld (%.2f%%) | dibujado pero atravesable %ld\n",
  overflows,solidPts,invisible,100.*invisible/solidPts,walkThrough);
 return 0;
}
