/* v2.35 (Claude): elementos sueltos encimados o incrustados. Se genera toda la ciudad con el
   renderizador real y un registro de cada elemento suelto (arboles, farolas, bancos, paradas,
   semaforos, papeleras, quioscos, fuentes, pilares...). Busca:
   1) dos elementos ocupando el mismo sitio (circulos que se solapan);
   2) elementos a ras de suelo metidos en un edificio (parcela edificada).
   Escribe build/props.csv para qa_parked_v235.c (coches aparcados y terminales).
   Requiere la copia etiquetada que generan mkaud.py + mkprops.py (registro prop_add). */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static float in_building(float x,float z,float r){ /* cuanto se mete el circulo en una parcela edificada */
 float worst=0;
 for(float d=r;d>=0;d-=.5f){int k8=d>0?16:1;
  for(int k=0;k<k8;k++){float a=k*(2*PI/k8);if(cm_parcel_at(x+cosf(a)*d,z+sinf(a)*d)==0){float pen=r-d+ (d==0?r:0);if(pen>worst)worst=pen;}}}
 return worst;
}
int main(void){
 R3Scene s={0};s.cameraDistance=50;s.target=-1;clipEnabled=0;
 /* los puntos de mision (coche aparcado y terminal) se leen de src/story.h, como hace el juego */
 {FILE *st=fopen("src/story.h","r");char ln[512];int n=0;
  while(st&&n<30&&fgets(ln,sizeof ln,st)){char *q=strstr(ln,"{\"");if(!q)continue;q=strchr(q+2,'"');if(!q)continue;int x,y;if(sscanf(q+1,",%d,%d}",&x,&y)==2){s.hubs[n][0]=(float)x;s.hubs[n][1]=(float)y;n++;}}
  if(st)fclose(st);printf("puntos de mision leidos: %d",n);putchar(10);}
 for(float vz=40;vz<2240;vz+=160)for(float vx=40;vx<2560;vx+=160){s.x=vx;s.z=vz;s.yaw=0;r3_draw(NULL,&s);}
 printf("elementos registrados: %d",nprops);putchar(10);
 int pairs=0,embedded=0;
 for(int i=0;i<nprops;i++)for(int j=i+1;j<nprops;j++){Prop *a=&props[i],*b=&props[j];
  float d=hypotf(a->x-b->x,a->z-b->z);if(d<.6f)continue; /* misma pieza compuesta (tronco+copa, quiosco...) */
  if(d<a->r+b->r-.5f){pairs++;if(pairs<=60){printf("SOLAPE %s(L%d r%.1f) (%.0f,%.0f) <-> %s(L%d r%.1f) (%.0f,%.0f) dist %.1f",a->kind,a->line%10000,a->r,a->x,a->z,b->kind,b->line%10000,b->r,b->x,b->z,d);putchar(10);}}
 }
 for(int i=0;i<nprops;i++){Prop *a=&props[i];if(a->bottom>2)continue;
  /* arboles de jardines interiores (sobre el podio) y del Pueblito: crecen dentro de la parcela a proposito */
  if(cm_parcel_at(a->x,a->z)==0){int tree=!strcmp(a->kind,"arbol");for(int k=0;k<nprops&&!tree;k++)if(!strcmp(props[k].kind,"arbol")&&hypotf(props[k].x-a->x,props[k].z-a->z)<.6f)tree=1;if(tree)continue;} /* tronco del mismo arbol */
  float pen;
  if(!strcmp(a->kind,"parada")){ /* forma real: 18 x 7 (marquesina a lo largo de x) */
   pen=0;for(float dx=-8.5f;dx<=8.5f;dx+=1)for(float dz=-3;dz<=3;dz+=1)if(cm_parcel_at(a->x+dx,a->z+dz)==0)pen=2;
  }else pen=in_building(a->x,a->z,a->r);
  if(pen>1){embedded++;if(embedded<=40){printf("INCRUSTADO %s(L%d r%.1f) en (%.0f,%.0f): %.1f dentro de un edificio",a->kind,a->line%10000,a->r,a->x,a->z,pen);putchar(10);}}}
 FILE *f=fopen("build/props.csv","w");for(int i=0;i<nprops;i++)fprintf(f,"%s,%.2f,%.2f,%.2f,%d\n",props[i].kind,props[i].x,props[i].z,props[i].r,props[i].line%10000);fclose(f);
 printf("TOTAL: %d pares solapados, %d incrustados en edificios",pairs,embedded);putchar(10);
 return 0;
}
