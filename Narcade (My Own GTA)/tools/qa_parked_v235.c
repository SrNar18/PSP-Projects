/* v2.35 (Claude): coches aparcados y terminales de mision contra el mobiliario (build/props.csv,
   generado por qa_props_v235.c), los edificios y entre si. */
#include <stdio.h>
#include "../src/game.c"
typedef struct{char kind[16];float x,z,r;int line;}P;
static P pr[2000];static int np;
int main(void){
 FILE *f=fopen("build/props.csv","r");if(!f){puts("falta build/props.csv");return 1;}
 char kind[16];float x,z,r;int line;while(fscanf(f,"%15[^,],%f,%f,%f,%d\n",kind,&x,&z,&r,&line)==5&&np<2000){snprintf(pr[np].kind,16,"%s",kind);pr[np].x=x;pr[np].z=z;pr[np].r=r;pr[np].line=line;np++;}fclose(f);
 game_init();fresh_game();
 int issues=0;
 for(int i=0;i<CAR_COUNT;i++){Car *c=&g.cars[i];if(!c->parked)continue;
  if(!car_free_at(c,c->x,c->y)){issues++;printf("COCHE %d aparcado dentro de un edificio/obstaculo en (%.0f,%.0f)",i,c->x,c->y);putchar(10);}
  for(int k=0;k<np;k++){float d=dist(c->x,c->y,pr[k].x,pr[k].z);if(d<pr[k].r+11){issues++;printf("COCHE %d (%.0f,%.0f) encima de %s (L%d) en (%.0f,%.0f) dist %.1f",i,c->x,c->y,pr[k].kind,pr[k].line,pr[k].x,pr[k].z,d);putchar(10);}}
  for(int j=i+1;j<CAR_COUNT;j++){if(!g.cars[j].parked)continue;float nx,ny,dd;if(car_overlap(c,&g.cars[j],&nx,&ny,&dd)){issues++;printf("COCHES %d y %d aparcados uno encima del otro en (%.0f,%.0f)",i,j,c->x,c->y);putchar(10);}}
  for(int t=0;t<30;t++){float d=dist(c->x,c->y,locations[t].x+18,locations[t].y);if(d<16){issues++;printf("COCHE %d sobre el terminal %d dist %.1f",i,t,d);putchar(10);}}
 }
 for(int t=0;t<30;t++){float tx=locations[t].x+18,tz=locations[t].y;
  if(cm_parcel_at(tx,tz)==0){issues++;printf("TERMINAL %d (%s) dentro de un edificio en (%.0f,%.0f)",t,locations[t].name,tx,tz);putchar(10);}
  for(int k=0;k<np;k++){float d=dist(tx,tz,pr[k].x,pr[k].z);if(d>.6f&&d<pr[k].r+6){issues++;printf("TERMINAL %d (%s) encima de %s (L%d) dist %.1f",t,locations[t].name,pr[k].kind,pr[k].line,d);putchar(10);}}
  for(int u=t+1;u<30;u++)if(dist(tx,tz,locations[u].x+18,locations[u].y)<14){issues++;printf("TERMINALES %d y %d encimados",t,u);putchar(10);}
 }
 printf("TOTAL problemas: %d",issues);putchar(10);
 return 0;
}
