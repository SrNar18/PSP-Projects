# Revisión de Codex de `claude/aceras-distancia-ventanas` (v2.48)

Revisé el commit `0d0ab90` y su integración en `main` (`4fb47f4`), incluidos `city26.inc`, `city3d.inc`, `daylight.inc` y `render3d.c`. La entrega conjunta v2.49 ya incorpora estos cambios; no encontré conflictos con los cambios de personaje e interfaz de Codex.

## Resultado

- No encontré un bug confirmado que requiera corrección inmediata. La subdivisión de `slab_draped` por quiebres del terreno coincide con el cálculo de `geo_height`; el radio lejano de cámara y ciudad usa `R3_FAR`; los carriles del metro continúan en ambos niveles de detalle.
- Tras la integración, las 14 baterías de `qa_character_suite.py` pasaron, incluidas 512 vistas de ciudad/día-noche/metro sin desbordamiento. Pico conjunto: 53.286 vértices de mundo, 780 de brillo y 387 de sombras. Pasa asimismo la compilación PSP y la validación de la ISO.
- `qa_sprint_cost_v236` en PC: 6.000 fotogramas, media 0,569 ms, p95 0,831 ms, p99 1,022 ms, máximo 4,432 ms; 196 construcciones de LOD, 1.051 cambios de luz, 0 vaciados de caché. Esto mide generación de geometría en PC, no FPS de la PSP.

## Comprobación pendiente en PSP física

Probar una carrera continua por Candelaria Río y mirar a lo largo de las vías del metro, de día y de noche. Buscar caída de FPS, entrada brusca de ventanas al pasar de LOD lejano a medio, o franja del tren que parpadee. El radio de dibujo pasó de 720 a 1100 y el presupuesto de METAL/CONCRETE queda ajustado; las pruebas sintéticas no sustituyen esa inspección en la consola. Si aparece un defecto, registrar posición aproximada, hora del juego y si se está a pie o en coche antes de modificar geometría.

La observación previa sobre la franja verde del tren fue atendida en v2.48 (posición x+sd*0,68); no la marco como bug pendiente sin reproducirlo en la PSP. No corregí código de Claude durante esta revisión.
