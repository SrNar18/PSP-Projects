/* v2.37 (Claude): nivel de busqueda al estilo GTA. */
#include <stdio.h>
#include <assert.h>
#include "../src/game.c"
static void reset(void){game_init();fresh_game();g.screen=WORLD;g.heat=0;wantedKills=0;for(int i=60;i<64;i++){g.cars[i].x=-3000;g.cars[i].y=-3000;}}
int main(void){
 reset();wanted_crime(0);assert(wanted_stars()==0);                      /* un golpe sin policia: nada */
 wanted_crime(0);wanted_crime(0);assert(wanted_stars()==0);
 reset();wanted_crime(1);assert(wanted_stars()==1);                      /* matar a un civil: 1 estrella */
 wanted_crime(1);assert(wanted_stars()==1);wanted_crime(1);assert(wanted_stars()==2); /* la tercera muerte sube */
 reset();wanted_crime(3);assert(wanted_stars()==0);                      /* robar un carro sin testigos */
 g.cars[60].x=g.x+100;g.cars[60].y=g.y;wanted_crime(3);assert(wanted_stars()==1); /* delante de la policia */
 reset();wanted_crime(4);assert(wanted_stars()==2);                      /* patrulla robada */
 /* huida: lejos de las patrullas se pierde de golpe tras 6+5*estrellas s */
 reset();wanted_crime(1);float t=0;while(wanted_stars()>0&&t<60){game_tick(0,0,0,1.f/30);g.screen=WORLD;for(int i=60;i<64;i++){g.cars[i].x=-3000;g.cars[i].y=-3000;}t+=1.f/30;}
 printf("1 estrella perdida en %.1f s",t);putchar(10);assert(t>10&&t<13);
 puts("PASS: estrellas estilo GTA (leves acumulan, 1 por muerte, patrulla 2, huida por tiempo fuera de vista)");return 0;
}
