/* v2.33 (Claude): parpadeo entre objetos (z-fighting). Dos superficies en el mismo plano (o
   a menos de 0,35 unidades: el bufer de profundidad de la PSP es de 16 bits) que se solapan
   parpadean al mover la camara. Genera toda la ciudad con el renderizador real, agrupa los
   triangulos por plano y busca pares solapados que vienen de piezas distintas.
   Compilar (sobre la copia etiquetada que genera mkaud.py, para saber la linea de origen):
   gcc -O2 -DNARCADE_3D -DAB_NOPLAYER -DAB_NOPEOPLE -DAB_NOFX -DMAX_VERTICES=60000 -Isrc tools/qa_zfight_v233.c -lm */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#ifndef ZTOL
#define ZTOL .06f /* coplanares de verdad: parpadean a cualquier distancia */
#endif
#ifndef AUDIT_LINES
static int meshLineDummy;
#define LINE_OF(m,i) meshLineDummy
#else
#define LINE_OF(m,i) meshLine[m][i]
#endif
typedef struct{float a[3],b[3],c[3],n[3],d;int line,mat;}Tri;
static Tri *T;static int nt,cap;
static int cmp(const void *x,const void *y){const Tri *a=x,*b=y;
 int ka=(int)floorf(a->n[0]*20)*10000+(int)floorf(a->n[1]*20)*100+(int)floorf(a->n[2]*20),kb=(int)floorf(b->n[0]*20)*10000+(int)floorf(b->n[1]*20)*100+(int)floorf(b->n[2]*20);
 if(ka!=kb)return ka<kb?-1:1;return a->d<b->d?-1:a->d>b->d;}
/* 2D en el plano del triangulo: se descarta el eje dominante de la normal */
static void p2(const float *v,int ax,float *u,float *w){int i0=ax==0?1:0,i1=ax==2?1:2;*u=v[i0];*w=v[i1];}
static int sep(float (*A)[2],float (*B)[2]){
 for(int s=0;s<2;s++){float (*P)[2]=s?B:A;
  for(int e=0;e<3;e++){float ex=P[(e+1)%3][0]-P[e][0],ey=P[(e+1)%3][1]-P[e][1],nx=-ey,ny=ex;
   float a0=1e30f,a1=-1e30f,b0=1e30f,b1=-1e30f;
   for(int k=0;k<3;k++){float pa=A[k][0]*nx+A[k][1]*ny,pb=B[k][0]*nx+B[k][1]*ny;a0=fminf(a0,pa);a1=fmaxf(a1,pa);b0=fminf(b0,pb);b1=fmaxf(b1,pb);}
   float L=sqrtf(nx*nx+ny*ny)+1e-9f;if((a1-b0)/L<.3f||(b1-a0)/L<.3f)return 1; /* solape minimo 0,3 */
  }}
 return 0;}
int main(void){
 cap=1<<20;T=malloc(sizeof(Tri)*cap);
 R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 for(float vz=40;vz<2240;vz+=80)for(float vx=40;vx<2560;vx+=80){
  s.x=vx;s.z=vz;s.yaw=0;memset(cellCache,0,sizeof cellCache);cacheUsed=0;chunkUsed=0;r3_draw(NULL,&s);
  float cx,cz;geo_project(vx,vz,&cx,&cz);
  for(int m=0;m<MAT_COUNT;m++)for(int i=0;i+2<used[m];i+=3){
   Vertex *v=&mesh[m][i];float mx=(v[0].x+v[1].x+v[2].x)/3,mz=(v[0].z+v[1].z+v[2].z)/3;
   if(fabsf(mx-cx)>40||fabsf(mz-cz)>60)continue;
   float e1[3]={v[1].x-v[0].x,v[1].y-v[0].y,v[1].z-v[0].z},e2[3]={v[2].x-v[0].x,v[2].y-v[0].y,v[2].z-v[0].z};
   float n[3]={e1[1]*e2[2]-e1[2]*e2[1],e1[2]*e2[0]-e1[0]*e2[2],e1[0]*e2[1]-e1[1]*e2[0]};float L=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
   if(L<1.f)continue; /* area < 0,5 */
   for(int k=0;k<3;k++)n[k]/=L;
   /* sin invertir la normal: solo parpadean superficies que miran al MISMO lado (dos paredes
      espalda con espalda de casas adosadas quedan ocultas y no cuentan) */
   if(nt>=cap){cap*=2;T=realloc(T,sizeof(Tri)*cap);}
   Tri *t=&T[nt++];float *P[3]={t->a,t->b,t->c};for(int q=0;q<3;q++){P[q][0]=v[q].x;P[q][1]=v[q].y;P[q][2]=v[q].z;}
   memcpy(t->n,n,sizeof n);t->d=n[0]*v[0].x+n[1]*v[0].y+n[2]*v[0].z;t->line=LINE_OF(m,i);t->mat=m;
  }
 }
 qsort(T,nt,sizeof(Tri),cmp);
 typedef struct{int la,ma,lb,mb;long n;float x,z;}PairK;static PairK pk[4000];int npk=0;static long pairs[1][1];static long byLine[40000];static float sx[40000],sz[40000];long total=0;
 for(int i=0;i<nt;i++){
  for(int j=i+1;j<nt&&j<i+4000;j++){
   Tri *a=&T[i],*b=&T[j];
   if(fabsf(a->n[0]-b->n[0])+fabsf(a->n[1]-b->n[1])+fabsf(a->n[2]-b->n[2])>.02f)break;
   if(b->d-a->d>ZTOL)break;
   if(a->line==b->line&&a->mat==b->mat)continue; /* misma pieza: triangulos vecinos */
   int ax=fabsf(a->n[0])>fabsf(a->n[1])?(fabsf(a->n[0])>fabsf(a->n[2])?0:2):(fabsf(a->n[1])>fabsf(a->n[2])?1:2);
   float A[3][2],B[3][2];float *pa[3]={a->a,a->b,a->c},*pb[3]={b->a,b->b,b->c};
   for(int k=0;k<3;k++){p2(pa[k],ax,&A[k][0],&A[k][1]);p2(pb[k],ax,&B[k][0],&B[k][1]);}
   if(sep(A,B))continue;
   total++;int la=a->line%40000,lb=b->line%40000;byLine[la]++;byLine[lb]++;sx[la]=a->a[0];sz[la]=a->a[2];sx[lb]=b->a[0];sz[lb]=b->a[2];
   {int L1=la,M1=a->mat,L2=lb,M2=b->mat;if(L1>L2||(L1==L2&&M1>M2)){int t=L1;L1=L2;L2=t;t=M1;M1=M2;M2=t;}
    int f=-1;for(int q=0;q<npk;q++)if(pk[q].la==L1&&pk[q].ma==M1&&pk[q].lb==L2&&pk[q].mb==M2){f=q;break;}
    if(f<0&&npk<4000){f=npk++;pk[f].la=L1;pk[f].ma=M1;pk[f].lb=L2;pk[f].mb=M2;pk[f].n=0;pk[f].x=a->a[0];pk[f].z=a->a[2];}
    if(f>=0)pk[f].n++;}
   (void)pairs;
  }
 }
 printf("pares de superficies coplanares solapadas: %ld\n",total);
 for(int q=0;q<npk;q++)if(pk[q].n>=6){float lx,lz;geo_unproject(pk[q].x,pk[q].z,&lx,&lz);
  printf("PAR %d/%d(mat %d) <-> %d/%d(mat %d): %ld  en (%.0f,%.0f)\n",pk[q].la/10000,pk[q].la%10000,pk[q].ma,pk[q].lb/10000,pk[q].lb%10000,pk[q].mb,pk[q].n,lx,lz);}
 for(int k=0;k<40000;k++)if(byLine[k]>10){float lx,lz;geo_unproject(sx[k],sz[k],&lx,&lz);
  printf("LINEA %s:%d -> %ld pares, p.ej. en (%.0f,%.0f)",k/10000==1?"city26.inc":k/10000==2?"city3d.inc":"render3d/otro",k%10000,byLine[k],lx,lz);putchar(10);}
 return 0;
}
