/* v2.41 (Claude): la tabla precalculada de casetas (citymap.h cmTerminalTable) coincide con la busqueda real.
   Si cambian el mapa, las parcelas o los puntos de mision, regenera la tabla (imprime la esperada al fallar). */
#include <stdio.h>
#include "../src/game.c"
int main(void){int bad=0;
 for(int i=0;i<30;i++){float x,z,tx,tz;cm_terminal_search(locations[i].x,locations[i].y,&x,&z);cm_terminal_pos(locations[i].x,locations[i].y,&tx,&tz);
  if(fabsf(x-tx)>.01f||fabsf(z-tz)>.01f){bad++;printf("    {%.0f.f,%.0f.f,%.9g,%.9g}, /* caseta %d: la tabla dice (%.2f,%.2f) */",(double)locations[i].x,(double)locations[i].y,(double)x,(double)z,i,tx,tz);putchar(10);}}
 if(bad){printf("FAIL: %d casetas desactualizadas en cmTerminalTable",bad);putchar(10);return 1;}
 puts("PASS: tabla de casetas = busqueda real (30/30); sin calculo diferido tras cargar.");return 0;}
