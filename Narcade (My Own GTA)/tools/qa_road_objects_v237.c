/* v2.37 (Claude): objetos a ras de suelo (cajas, cilindros, cesped/aceras) encima de la calzada.
   Requiere la copia etiquetada (mkaud.py + mkroad.py): road_hit() registra cada primitiva. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
int main(void){
 R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 for(float vz=40;vz<2240;vz+=160)for(float vx=40;vx<2560;vx+=160){s.x=vx;s.z=vz;s.yaw=0;r3_draw(NULL,&s);}
 int total=0;for(int i=0;i<nhits;i++){total+=hits[i].n;printf("L%d %s mat %d: %d veces, p.ej. (%.0f,%.0f) %.0fx%.0f",hits[i].line,hits[i].kind,hits[i].mat,hits[i].n,hits[i].x,hits[i].z,hits[i].w,hits[i].d);putchar(10);}
 printf("TOTAL: %d objetos sobre la calzada en %d lineas",total,nhits);putchar(10);return total?1:0;
}
