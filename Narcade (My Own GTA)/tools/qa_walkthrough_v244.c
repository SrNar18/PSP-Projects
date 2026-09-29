/* v2.44 (Claude): volumenes dibujados (build/solids.csv, de qa_solid_export_v244) por los que el personaje
   puede pasar. Muestrea el interior de cada volumen y prueba foot_free(). Agrupa por linea de origen. */
#include <stdio.h>
#include "../src/game.c"
typedef struct{int line,mat,n;float x,z,l,w,b,h;}Grp;static Grp gr[400];static int ng;
int main(void){FILE *f=fopen("build/solids.csv","r");if(!f){puts("falta build/solids.csv");return 1;}
 game_init();fresh_game();for(int i=0;i<CAR_COUNT;i++){g.cars[i].x=-5000-i*50;g.cars[i].y=-5000;}
 float x,z,l,w,a,b,h;int line,mat,total=0,bad=0;
 while(fscanf(f,"%f,%f,%f,%f,%f,%f,%f,%d,%d\n",&x,&z,&l,&w,&a,&b,&h,&line,&mat)==9){total++;
  float c=cosf(a),s=sinf(a);int walk=0,pts=0;
  for(int i=-1;i<=1;i++)for(int j=-1;j<=1;j++){float u=i*(l*.5f-1.2f),v=j*(w*.5f-1.2f);if(l<3)u=0;if(w<3)v=0;
   float px=x+u*c-v*s,pz=z+u*s+v*c;if(px<5||pz<5||px>WORLD_W-5||pz>WORLD_H-5)continue;pts++;if(free_at(px,pz,2))walk++;}
  if(pts&&walk*2>pts){bad++;int k;for(k=0;k<ng;k++)if(gr[k].line==line)break;
   if(k==ng&&ng<400){gr[ng]=(Grp){line,mat,0,x,z,l,w,b,h};ng++;}if(k<ng)gr[k].n++;}}
 fclose(f);
 /* prismas (edificios, podios, muros de parcela) */
 f=fopen("build/prisms.csv","r");int n;
 while(f&&fscanf(f,"%d",&n)==1){float px_[8],pz_[8];for(int k=0;k<n;k++)if(fscanf(f,",%f,%f",&px_[k],&pz_[k])!=2)n=0;
  if(fscanf(f,",%f,%f,%d,%d\n",&b,&h,&line,&mat)!=4)break;if(n<3)continue;total++;
  float cx=0,cz=0;for(int k=0;k<n;k++){cx+=px_[k];cz+=pz_[k];}cx/=n;cz/=n;int walk=0,pts=0;
  for(int k=-1;k<n;k++){float qx=k<0?cx:cx+(px_[k]-cx)*.7f,qz=k<0?cz:cz+(pz_[k]-cz)*.7f;pts++;if(free_at(qx,qz,2))walk++;}
  if(walk*2>pts){bad++;int q;for(q=0;q<ng;q++)if(gr[q].line==line)break;
   if(q==ng&&ng<400){gr[ng]=(Grp){line,mat,0,cx,cz,0,0,b,h};ng++;}if(q<ng)gr[q].n++;}}
 if(f)fclose(f);
 for(int k=0;k<ng;k++){printf("L%d mat %d: %d volumenes traspasables, p.ej. (%.0f,%.0f) %.0fx%.0f base %.1f alto %.1f",gr[k].line,gr[k].mat,gr[k].n,gr[k].x,gr[k].z,gr[k].l,gr[k].w,gr[k].b,gr[k].h);putchar(10);}
 printf("TOTAL: %d de %d volumenes solidos se pueden atravesar",bad,total);putchar(10);return bad?1:0;}
