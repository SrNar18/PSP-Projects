/* v2.33 (Claude): objetos mal colocados sobre la calzada. Genera toda la ciudad con el
   renderizador real (sin recorte, detalle cercano), rasteriza en planta lo que sobresale
   entre 1,5 y 25 unidades del suelo (bancos, arboles, farolas, casetas, paradas...) y lo
   cruza con el mapa: un punto de calzada (fuera de toda parcela y de su acera de 9) con
   algo encima es un objeto en medio de la carretera. Agrupa por zonas y dibuja
   build/street_objects.ppm (rojo = objeto en calzada).
   Compilar: gcc -O2 -DNARCADE_3D -DAB_NOPLAYER -DAB_NOPEOPLE -DAB_NOFX -DMAX_VERTICES=60000 -Isrc tools/qa_street_objects_v233.c -lm */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#define GW 2700
#define GH 3400
static float *hobj;
static void put(int x,int z,float y){
 if(x<0||z<0||x>=GW||z>=GH)return;
 float lx,lz;geo_unproject(x+.5f,z+.5f,&lx,&lz);float gnd=geo_height(lx,lz);
 if(y>gnd+2.5f&&y<gnd+25.f){int i=z*GW+x;if(y-gnd>hobj[i])hobj[i]=y-gnd;}
}
static void line3(Vertex a,Vertex b){
 float dx=b.x-a.x,dz=b.z-a.z;int n=(int)(fmaxf(fabsf(dx),fabsf(dz))*2)+1;if(n>4000)return;
 for(int i=0;i<=n;i++){float t=(float)i/n;put((int)floorf(a.x+dx*t),(int)floorf(a.z+dz*t),a.y+(b.y-a.y)*t);}
}
static void tri(Vertex a,Vertex b,Vertex c){
 /* solo lo que apoya en el suelo: se ignoran copas de arboles, brazos de farolas y
    tableros elevados que pasan por encima de la calle */
 {float my=fminf(a.y,fminf(b.y,c.y)),lx,lz;geo_unproject((a.x+b.x+c.x)/3,(a.z+b.z+c.z)/3,&lx,&lz);if(my>geo_height(lx,lz)+4)return;}
 line3(a,b);line3(b,c);line3(c,a);
 float minx=fminf(a.x,fminf(b.x,c.x)),maxx=fmaxf(a.x,fmaxf(b.x,c.x)),minz=fminf(a.z,fminf(b.z,c.z)),maxz=fmaxf(a.z,fmaxf(b.z,c.z));
 if(maxx-minx>200||maxz-minz>200)return;
 float den=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);if(fabsf(den)<1e-6f)return;
 for(int z=(int)floorf(minz);z<=(int)ceilf(maxz);z++)for(int x=(int)floorf(minx);x<=(int)ceilf(maxx);x++){
  float px=x+.5f,pz=z+.5f;
  float w1=((b.z-c.z)*(px-c.x)+(c.x-b.x)*(pz-c.z))/den,w2=((c.z-a.z)*(px-c.x)+(a.x-c.x)*(pz-c.z))/den,w3=1-w1-w2;
  if(w1<-.01f||w2<-.01f||w3<-.01f)continue;
  put(x,z,a.y*w1+b.y*w2+c.y*w3);
 }
}
/* Calzada: fuera de toda parcela y de su acera (9 unidades), fuera del rio/Metro y del borde. */
static int is_road(float x,float z){
 if(x<40||z<40||x>2520||z>2200)return 0; /* el borde del mundo sube en ladera: no es calzada */
 if(x>1380&&x<1520)return 0; /* rio, orillas y Metro */
 for(int dz=-1;dz<=1;dz++)for(int dx=-1;dx<=1;dx++)if(cm_parcel_at(x+dx*9,z+dz*9)>=0)return 0;
 return 1;
}
int main(void){
 hobj=calloc((size_t)GW*GH,sizeof(float));
 R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 for(float vz=40;vz<2240;vz+=80)for(float vx=40;vx<2560;vx+=80){
  s.x=vx;s.z=vz;s.yaw=0;memset(cellCache,0,sizeof cellCache);cacheUsed=0;chunkUsed=0;r3_draw(NULL,&s);
  float cx,cz;geo_project(vx,vz,&cx,&cz);
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i+2<used[m];i+=3){
   Vertex *v=&mesh[m][i];float mx=(v[0].x+v[1].x+v[2].x)/3,mz=(v[0].z+v[1].z+v[2].z)/3;
   if(fabsf(mx-cx)>70||fabsf(mz-cz)>70)continue;
   tri(v[0],v[1],v[2]);
  }
 }
 static unsigned char img[GH/2][GW/2][3];
 enum{CW=40};static int bucket[2560/CW+1][2240/CW+1];static float bh[2560/CW+1][2240/CW+1];
 long bad=0;
 for(float y=20;y<2220;y+=1)for(float x=20;x<2540;x+=1){
  float gx,gz;geo_project(x,y,&gx,&gz);int ix=(int)gx,iz=(int)gz;if(ix<0||iz<0||ix>=GW||iz>=GH)continue;
  unsigned char *p=img[iz/2][ix/2];int road=is_road(x,y);
  if(road&&hobj[iz*GW+ix]>0){bad++;int bx=(int)(x/CW),bz=(int)(y/CW);bucket[bx][bz]++;if(hobj[iz*GW+ix]>bh[bx][bz])bh[bx][bz]=hobj[iz*GW+ix];p[0]=255;p[1]=0;p[2]=0;}
  else if(p[0]!=255){if(road){p[0]=p[1]=p[2]=70;}else if(cm_parcel_at(x,y)>=0){p[0]=40;p[1]=60;p[2]=90;}else{p[0]=p[1]=p[2]=150;}}
 }
 FILE *f=fopen("build/street_objects.ppm","wb");fprintf(f,"P6 %d %d 255\n",GW/2,GH/2);fwrite(img,1,sizeof img,f);fclose(f);
 int zones=0;
 for(int bz=0;bz<=2240/CW;bz++)for(int bx=0;bx<=2560/CW;bx++)if(bucket[bx][bz]>=4){zones++;
  printf("zona x %4d-%4d z %4d-%4d (celda %d,%d local %3d,%3d): %4d puntos, altura max %.1f\n",bx*CW,bx*CW+CW,bz*CW,bz*CW+CW,bx*CW/320,bz*CW/320,(bx*CW)%320,(bz*CW)%320,bucket[bx][bz],bh[bx][bz]);}
 printf("TOTAL: %ld puntos de calzada con objetos encima, %d zonas\n",bad,zones);
 return 0;
}
