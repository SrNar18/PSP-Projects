# Narcade 2.33 — personaje, armas y combate (Codex)

Fecha: 27 de septiembre de 2026. Rama: `codex/character-combat`.
Base: `0f4cf17`, producción 2.32. Se trabajó en un worktree dentro de `build/worktrees/character-combat`, no en el checkout de Claude.

## Coordinación

Durante la entrega, el checkout principal estaba en `claude/revision-bugs`, con cambios sin commit en ciudad, luces, tráfico, `game.c` y `render3d.c`. Se leyeron sus diferencias pero NO se modificaron, copiaron ni confirmaron. Esta entrega se integra contra `origin/main`; Claude deberá incorporar ese main a su rama antes de publicar la 2.34. Si hay conflicto en `game.c`/`render3d.c`, conservar ambos sistemas: sus correcciones de tráfico/ciudad y las llamadas a los nuevos módulos de personaje. No sustituir archivos completos por una copia anterior.

Se mantienen el controlador de movimiento 2.25–2.31, mediana y calibración del joystick, proyección/inversa del valle, cámara normal, tráfico y ritmo de vblank de Claude. No se modificó `psp_main.c`. La pausa y los diálogos Sony siguen usando el flujo anterior.

## Controles

- Caminar con joystick/cruceta. Mantener X: trotar. Pulsaciones repetidas de X: sprint con estamina, como en la base de Claude.
- Mantener L: rueda. Elegir con joystick y soltar L: equipar.
- Mantener R a pie: pose de apuntado/guardia. Durante el apuntado se detiene la locomoción; el joystick controla la cámara y la altura de la mira.
- NPC vivo visible en el cono frontal: fijación automática con retícula y pequeña barra de vida. Sin candidato visible: retícula central y apuntado libre.
- R + círculo: disparar. Con puños/bate, mantener círculo repite los golpes, limitados por alcance y cadencia.
- Cuadrado a pie: saltar; ante los muros escalables de parques, saltar/agarrarse y pasar al otro lado.
- R + cuadrado: interacción de historia, vinilos y Metro. Se actualizan los avisos contextuales y la ayuda. En coche siguen cuadrado para frenar y ARRIBA para interactuar.
- Círculo sin R: mensajes. START/SELECT y radio siguen funcionando.

## Organización del código

`src/combat.inc` se incluye antes de `world_tick`. Mantiene un estado independiente de `g` y 42 registros de NPC: vida, huida, caída, impacto, orientación y fase de animación. Se reinicia desde `world_peds_init`, también en la carga por etapas. Las firmas y el contenido binario de `Save` NO cambian; las partidas existentes siguen siendo compatibles. Las muertes de peatones no se escriben en el guardado: al reconstruir el mundo reaparecen vivos.

El apuntado usa unidades proyectadas, evitando reintroducir los fallos de rumbo en diagonales. La fijación comprueba obstáculos. Los disparos son hitscan con altura: comprueban edificios, terreno, coches y muros de parques antes del NPC. Cada arma tiene daño/cadencia/alcance propios; la escopeta tiene menos alcance. La munición es ilimitada en esta versión. Los ataques causan huida, impactos de sangre breves, caída al agotar 100 de vida y aumento de búsqueda hasta cinco estrellas. La alerta impide que la búsqueda desaparezca inmediatamente.

La cámara de apuntado tiene distancia limitada por la cámara de obstáculos y una altura mínima sobre el terreno. El rayo libre coincide con la línea central real de esa cámara, incluyendo el desplazamiento lateral del hombro; no se confunde el suelo bajo el jugador con el suelo bajo el blanco. `r3_target_screen` proyecta la retícula del NPC con la misma cámara y FOV del renderer.

`src/character_pose.inc` conserva la malla importada de `personaje-v3.glb`. Corrige la envolvente del pelo y reduce la parte del cráneo que debía quedar cubierta. La piel debajo de las mangas se recoge y se amplía ligeramente la manga. Ambos materiales reciben la misma pose de brazo. La tabla de vértices únicos ahora compara también el material, para que una corrección de piel no se reutilice por error como manga/pelo. Caben 1.392 vértices únicos en los 2.048 existentes.

Los brazos usan dos segmentos con codo y mano: pistola/revólver a una mano; subfusil, AK, escopeta y rifle con mano de apoyo; bate con ambas manos y swing. Se suavizan las trayectorias de los pies y se reduce la torsión del torso/balanceo al caminar. La fase de marcha sigue avanzando por distancia realmente recorrida, sin tocar el controlador de Claude. Saltos recogen las piernas; la escalada eleva los brazos.

Los NPC heridos corren alejándose del jugador y prueban direcciones alternativas ante obstáculos. Los muertos dejan de caminar y el renderer inclina el cuerpo hasta el suelo. No hay ragdoll ni mutilación. Sangre, flash y trazador son geometría limitada, sin asignaciones por fotograma.

`src/climb_walls.inc` sitúa tres muros en parcelas de parque existentes. Tienen hitbox y tamaño compartidos en coordenadas proyectadas: 24 × 2,4 × 18 unidades, más remate. Son aproximadamente más altos que el personaje. La escalada dura 0,95 s y comprueba aterrizaje libre y desnivel; no permite atravesar edificios ni pilares del Metro. No se habilita trepar cualquier fachada o llegar a cualquier azotea. La cámara no interpreta la altura del salto como si fuese un andén del Metro.

## Armas y materiales

`src/weapon_detail.inc` añade cañones octogonales con boca oscura. Los modelos tienen guardamonte, miras, estrías, culata, cargador y detalles según el arma. `tools/weapon_icons.py` genera ilustraciones originales de 64 × 32 RGB565 para la rueda (`src/weapon_icons.h`), con metal/madera y detalles legibles.

`tools/weapon_textures.py` genera dos tiles originales de 64 × 64: acero cepillado y madera acabada. Se añaden DESPUÉS de los dos mipmaps antiguos de carretera/andén en `assets/textures3d.bin`. Los primeros 868.352 bytes del atlas anterior se conservan. Materiales 41/42 tienen una dirección especial al cargar; los offsets de ciudad y mipmaps NO se desplazan. `tools/textures3d.py` también añade estos tiles al regenerar todo. Los previews de personaje leen 43 materiales.

VRAM, `MAX_VERTICES=6144` y cache de ciudad de 2 MB se mantienen. Se añaden dos lotes de geometría para armas en RAM (aprox. 295 KB) y 16 KB de tiles, además del pequeño estado de combate/iconos. Revisar memoria antes de aumentar materiales/cache nuevamente. El sonido de disparos/golpes se mezcla con el audio existente; ruido propio y tabla de tono, sin exponenciales ni senos por muestra del disparo.

## Validación y reproducción

`tools/qa_character_suite.py --cc <gcc de w64devkit>` compila y ejecuta nueve pruebas:

1. `qa_combat`: fijación, daño, cadencia, huida, muerte, búsqueda, apuntado libre con altura, puños repetidos y alcance, coches/edificios bloqueando balas, salto/aterrizaje y escalada sólida.
2. `qa_gait_v217`: caminar y trotar continuos.
3. `qa_thumb_v229`: mismas medidas de pulgar/ruido de Claude.
4. `qa_controls_v218`: giros en las tres marchas, cámara, conducción libre y colisiones.
5. `qa_weapons`: ocho direcciones, soltar L, pausa y vehículos.
6. `qa`: 36 misiones, 146 objetivos, siete familias de minijuegos, guardado/recuperación corrupta, audio y 12.000 entradas aleatorias.
7. `qa_character_combat`: 1.152 poses, mano de apoyo, vértices finitos, proyección de mira, cámara sobre terreno y cache de jugador.
8. `qa_weapon_mesh`: siete modelos y 504 poses.
9. `qa_city28`: 512 vistas ciudad/Metro/día-noche, plantas altas visibles y sin overflow de materiales.

`tools/export_character_combat.c` y `preview_character_combat.py` producen una inspección de la malla real con sus texturas. Es un render de estudio en PC, NO una captura de PSP. La rueda se revisa además mediante el framebuffer host.

La ISO se compila con el SDK PSP existente y `package_iso.py` comprueba que `EBOOT.BIN` coincide byte a byte con el ELF estático. `tools/character_layout.c` permite una inspección de estado de sólo lectura en PPSSPP; `psp_character_smoke.py` envía controles reales, sin modificar memoria. Se usa una instancia aislada y otra Memory Stick. Comprueba apuntar, golpe mantenido, salto/aterrizaje, rueda/equipar y disparo sin abrir mensajes accidentalmente. El emulador oculto no permite validar visualmente la salida GPU: la revisión de apariencia realizada es la de la malla en PC. Queda pendiente el aspecto final y FPS en PSP física.

Entrega en `release/Narcade_v2.33` y ZIP hermano. El README explica los controles, instalación y límites. Se incluyen SHA256, ISO, PBP en el ZIP, previews identificados y esta nota.

Compilacion final: secciones estaticas de `narcade_static.elf` = 18.240.616 bytes (mas heap PSP de 1 MB existente). ISO de 16.082.944 bytes, SHA256 `e400ad957883891d9a0fccd80aa20f3d9e1abf4e759366ad42def2f085de2a81`. Tambien se comprueba cancelar salto/escalada al entrar en un vehiculo o recuperarse en el hospital, evitando volver a la posicion de una escalada anterior.
