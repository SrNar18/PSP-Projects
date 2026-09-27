# Revision de Codex a la rama de Claude — 27 septiembre 2026

Rama revisada: `claude/casetas-rendimiento`, commit `dde4c0e`.
Integrada por Claude en main `349ef34`; integrada sin conflictos en nuestra rama.
Se revisaron citymap.h, city26.inc, game.c, render3d.c y los dos diagnosticos.
No se han cambiado las correcciones de Claude: estos pendientes quedan para su proxima orden.

## Pendiente P2: aviso contextual no acompana al terminal movido

`src/game.c:1181` mantiene `dist(g.x,g.y,locations[st->loc].x,locations[st->loc].y)<58`
para mostrar R+[] INTERACTUAR/HABLAR. Pero `interact()` ahora usa `near_hub()`
(game.c:181), que acepta estar a menos de 24 del terminal nuevo.

Resultado: hay posiciones transitables junto a 12 terminales donde la interaccion
esta permitida y el HUD no ofrece su aviso. La comprobacion uso foot_free() y
near_hub() reales, no posiciones dentro de un edificio o terminal.

Reproduccion concreta: con objetivo en Estacion del Metro (location 4), caminar
hasta (1389.12,761.69). El terminal esta en (1386,746), distancia 16;
la distancia al marcador antiguo es 76.05. R+[] interactua, pero el criterio del
HUD no muestra la invitacion. Mismo caso para Archivo del Centro, Terminal del
Norte, Puente de San Juan, Jardin del Sur y Parque del Rio. Tambien se encontraron
casos junto a Cafe de Vera, Estadio, Antena Oriental, Plaza de la Luz, Servidor
Central y Observatorio.

Sugerencia para Claude: compartir near_hub(st->loc,58) con el HUD, y revisar las
indicaciones de los servicios que usen la posicion antigua. Mantener los marcadores
de las carreras en sus puntos actuales; no trasladar indiscriminadamente todos.

## Pendiente P2 de validacion (preexistente): el diagnostico nunca falla

`tools/qa_parked_v235.c:25` devuelve 0 aunque `issues` sea mayor que cero.
Un proceso/CI que solo mira el exit code puede declarar exito con solapes presentes.
Se recomienda devolver `issues ? 1 : 0`. La prueba sigue necesitando props.csv del
exportador instrumentado; explicar/generar esa dependencia desde un checkout limpio.
Este retorno ya era asi antes de esta rama; se registra porque la rama modifica el
mismo diagnostico para verificar los nuevos terminales.

## Verificado y limites

- 30 terminales: 32 muestras de radio 3.6 por pedestal, cero muestras dentro de
  una parcela; juego y dibujo comparten cm_terminal_pos. No se detecto que las
  nuevas posiciones/pedestales invadan edificios en esta comprobacion.
- Integracion de las diez suites host: controles, guardado/campana, combate, camara,
  armas, 512 escenas de ciudad/dia/noche y 720 fotogramas de marcha, todas aprobadas.
- Diagnostico de sprint original de Claude: 6000 fotogramas, 20 coches, 10 peatones,
  avance de hora x8. Integracion: 165 piezas nuevas/LOD, 827 por luz, 0 vaciados.
- Coste PC, una pasada del mismo recorrido: base 86a5bcf media 0.492 ms,
  p95 0.735, p99 1.006, max 10.844; integracion media 0.470, p95 0.738,
  p99 0.955, max 4.469. El p95 es practicamente igual; el peor pico observado baja.
  No son FPS de PSP ni una garantia de fluidez en hardware. Las cifras de PC son
  sensibles a la planificacion del SO; el contador base es cero porque se adapto
  solo para compilar con el diagnostico nuevo, no porque no reconstruyera ciudad.
- Se reviso la compactacion: conserva los bloques vivos ordenados por offsets,
  actualiza los offsets de pool/trozos y reutiliza espacio; no se detectaron fallos
  durante este recorrido. No se garantiza ausencia de todos los bugs de ciudad.
- Compilacion PSP e ISO estatica conjunta verificadas; prueba fisica pendiente.
