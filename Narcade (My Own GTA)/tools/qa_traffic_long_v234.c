/* v2.34 (Claude): trafico NPC a largo plazo (15 minutos de juego, jugador quieto lejos).
   Busca atascos permanentes (coche civil parado mas de 30 s), coches dentro de edificios,
   fuera de la calzada o solapados, y policias fuera del mapa. */
#include <stdio.h>
#include "../src/game.c"
int main(void){
 game_init();fresh_game();g.screen=WORLD;g.x=-2000;g.y=-2000; /* jugador fuera: solo problemas del trafico */
 float still[CAR_COUNT]={0};int maxStuck=0,inside=0,offroad=0,overlap=0,stuckCars=0;long frames=0;
 static int flagged[CAR_COUNT];
 for(int f=0;f<60*60*15;f++){
  game_tick(0,0,0,1.f/60);if(g.screen!=WORLD){g.screen=WORLD;}
  frames++;
  for(int i=0;i<CAR_COUNT;i++){Car *c=&g.cars[i];if(c->parked||i==g.car)continue;
   if(fabsf(c->speed)<.5f)still[i]+=1.f/60;else still[i]=0;
   if(still[i]>30&&!flagged[i]){flagged[i]=1;stuckCars++;printf("ATASCO coche %d en (%.0f,%.0f) rumbo %.2f policia %d",i,c->x,c->y,c->a,c->police);putchar(10);}
   if(!car_free_at(c,c->x,c->y)){inside++;if(inside<5){printf("DENTRO coche %d (%.0f,%.0f)",i,c->x,c->y);putchar(10);}}
   if(!c->police&&!cm_on_road(c->x,c->y)){offroad++;if(offroad<5){printf("FUERA coche %d (%.0f,%.0f)",i,c->x,c->y);putchar(10);}}
   for(int j=i+1;j<CAR_COUNT;j++){if(g.cars[j].parked)continue;float nx,ny,d;if(car_overlap(c,&g.cars[j],&nx,&ny,&d)&&d>2){overlap++;}}
  }
 }
 printf("15 min: coches atascados >30s %d | fotogramas-coche dentro de edificios %d | fuera de calzada %d | solapes %d",stuckCars,inside,offroad,overlap);putchar(10);
 /* v2.41: umbrales (base 3D actual: ~5 atascos, ~30-300 fotogramas fuera). Falla si empeora. */
 if(inside>0||overlap>0||offroad>600||stuckCars>8){puts("FAIL: trafico por encima de los umbrales");return 1;}
 return 0;
}
