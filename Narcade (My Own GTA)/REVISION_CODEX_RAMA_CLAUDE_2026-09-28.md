# Revisión de Codex de la rama de Claude (v2.37)

Revisé `bb41a11` y la integración en `main` (`2c651c4`). La compilación PSP conjunta pasó. Las pruebas de estrellas y precarga pasan; la precarga de PC reduce el peor fotograma de 7,93 ms a 0,45 ms en el escenario que cubre `qa_load_warm_v237.c`. El test de objetos en carretera necesita la copia instrumentada que menciona su cabecera; no compila contra el código normal y por eso no da una validación reproducible desde el checkout limpio.

Hallazgos para la siguiente orden, **sin corregir aquí**:

1. `src/game.c`, `cop_near()` y el temporizador de fuga: las patrullas destruidas (`hp<=0`) siguen contando como testigos y como patrullas cercanas. Pueden elevar la búsqueda al delinquir junto a un coche policial inutilizado o impedir que empiece la cuenta para perder las estrellas.
2. Reproducibilidad de QA: `qa_road_objects_v237.c` depende de `nhits` y `hits`, añadidos por una instrumentación externa. Convendría guardar el generador o las instrucciones completas de esa copia para que ambas IA puedan repetir la auditoría desde un checkout limpio.

## Cierre de la revisión

Revisión de Codex completa. Tras leer la nota de Claude (`LEER_PRIMERO_CODEX_local.md`, apartado v2.37), retiro el anterior comentario sobre el comparador de persecución: **una patrulla adicional por estrella es intencional**, no un bug. No cambiar ese comparador por este informe.

Las cuatro observaciones anteriores de Claude sobre mi rama de texturas/marcha se corrigieron en v2.38. No encontré todavía en las notas locales una revisión de Claude de la rama nueva `codex/cars-metro-lighting-trees` y de los ajustes finales de `codex/release-v238`; esa comprobación corresponde a Claude. Esta nota no afirma que él ya la haya realizado.

No se modifica código en este cierre. Dejar los hallazgos pendientes hasta que el usuario autorice la siguiente tarea.

La comprobación en PSP física de FPS, carga y comportamiento policial sigue pendiente; las cifras anteriores son del ejecutable host.
