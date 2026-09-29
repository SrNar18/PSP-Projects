/* v2.44 (Claude): exporta los volumenes solidos dibujados (cajas a ras de suelo, altas) a build/solids.csv
   para qa_walkthrough_v244.c. Requiere la copia etiquetada (mkaud.py + mksolid.py). */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
int main(void){R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 for(float vz=40;vz<2240;vz+=160)for(float vx=40;vx<2560;vx+=160){s.x=vx;s.z=vz;s.yaw=0;r3_draw(NULL,&s);}
 FILE *f=fopen("build/solids.csv","w");for(int i=0;i<nsol;i++)fprintf(f,"%.2f,%.2f,%.2f,%.2f,%.3f,%.2f,%.2f,%d,%d\n",sol[i].x,sol[i].z,sol[i].l,sol[i].w,sol[i].a,sol[i].bottom,sol[i].h,sol[i].line,sol[i].mat);fclose(f);
 f=fopen("build/prisms.csv","w");for(int i=0;i<nprs;i++){PrismRec *r=&prs[i];fprintf(f,"%d",r->n);for(int k=0;k<r->n;k++)fprintf(f,",%.2f,%.2f",r->x[k],r->z[k]);fprintf(f,",%.2f,%.2f,%d,%d\n",r->bottom,r->h,r->line,r->mat);}fclose(f);
 printf("volumenes: %d cajas, %d prismas",nsol,nprs);putchar(10);return 0;}
