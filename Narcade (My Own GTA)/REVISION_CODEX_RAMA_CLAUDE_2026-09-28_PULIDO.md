# Revisión de Codex para Claude — pulido 2.39 integrado en 2.40

Revisado el commit `5aff527`, integrado desde main `a5d8495`. No se corrigen estos hallazgos nuevos hasta la próxima orden.

## P2: el recentrado puede llevar un coche al carril contrario

En `civilian_tick`, el objetivo `lane+(cur>=lane?14:-14)` elige el lado por la posición actual, no por el sentido de circulación. Un coche hacia el este desplazado por un choque al otro lado de la mediana termina estabilizado en el carril contrario.

Reproducción host con el renderer real, resto de coches aparcados lejos: coche 3 en (500,359), ángulo 0, velocidad 40, hp 100; 60 pasos de 1/60 s. Resultado z=348, siendo su carril z=376 y el opuesto z=348. Elegir el carril según sentido, no según el lado actual, en la próxima tarea.

## P3: la prueba larga puede dar verde con problemas de tráfico

`qa_traffic_long_v234` devuelve 0 aunque cuente atascos o salida de calzada. Ejecutada 15 minutos simulados con NARCADE_3D/R3_HOST y objetos reales del renderer: 5 coches atascados más de 30 s, 0 fotogramas dentro de edificios, 30 fuera de calzada y 0 solapes. Definir umbrales y devolver error en casos indebidos antes de usarla como regresión automática. Comparar siempre con la base antes de atribuir un atasco nuevo.

## Integración y validación

Las trece suites de personaje, combate, ciudad, ajustes y prioridad de coches pasan. La corrección de policías muertos, el alineamiento del puente, UV de letreros y luz lunar están integrados. La niebla reintroducida por Claude se ofrece como ajuste y queda desactivada de inicio porque el jugador pidió anteriormente no verla. Las tres observaciones de Claude sobre los coches/farolas de Codex se corrigieron; detalles en NOTAS_CODEX_AJUSTES_IDIOMAS.md.

La prueba de consola física sigue pendiente; los ensayos host y compilación no equivalen a medir FPS reales en PSP.
