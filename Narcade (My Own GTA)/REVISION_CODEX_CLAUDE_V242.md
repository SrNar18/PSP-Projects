# Revision de Codex de Claude v2.42

Revisado el commit b6d5d1c (`claude/trofeos-carga-menu`), integrado desde main 9b6717d. Esta nota deja los hallazgos para la proxima orden, sin modificar la funcionalidad de Claude.

## Hallazgo reproducido

**P3 — bajar del metro puede sumar distancia a pie de un teletransporte.** En `src/trophies.inc`, `trophies_tick` comprueba `!g.inMetro` del fotograma actual, pero no guarda si el anterior estaba en el metro. A 10 FPS, salir desplaza al jugador de x1486 a x1472; las 14 unidades quedan bajo `dt*400+1` y se suman a `tro.walked`. Reproduje la secuencia de entrar/actualizar, `interact()` para salir y actualizar: imprime `Metro exit adds 14.0 walking units`. Es un desvio pequeno, pero contradice la exclusion de viajes/teletransportes. Propuesta para la proxima orden: guardar `lastInMetro` o invalidar `tro.tracking` al entrar/salir. No lo he corregido.

## Comprobaciones

- `qa_trophies_v242` pasa: cinco objetivos, muertes, huida, distancia, persistencia y archivo corrupto.
- Se conserva el intervalo de cinco segundos de la pantalla de carga y el cierre sin espera artificial al completar la carga.
- El nuevo menu de Codex sustituye las tarjetas antiguas. Sus bordes usan el mismo ancho/alto que las imagenes, conservando el objetivo de tu correccion.
- El menu lee tus cinco trofeos reales por defecto. Tu logica de contadores, desbloqueos y guardado sigue en `trophies.inc`; su antigua funcion de dibujo se sustituyo por la nueva interfaz.

## Mensaje expreso del usuario: Luna

**Naresz me pidio expresamente que te dijera, al enviarte esta revision de bugs, que la mujer del nuevo retrato ES LUNA dentro del lore del juego.** Esta identidad procede de su decision, no de una invencion de Codex. El original es `assets/luna-menu-source.png` y aparece en la tarjeta Ajustes. La implementacion como modelo/NPC en el mundo queda para una futura tarea, tal como pidio el usuario.

Mas detalles: `NOTA_LORE_LUNA_PARA_CLAUDE.md` y `NOTAS_CODEX_MENU_METRO_V242.md`.
