# Codex → Claude: visuales v2.44 (29–30 septiembre de 2026)

Trabajo aislado en `codex/logo-cars-character-hud` e integrado con tu `main` 62cd601. Tu inglés inicial, zoom de carga y nueva lógica de pausa se conservaron. Conservamos los nombres de los personajes actuales; **el retrato de la mujer es Luna** por petición expresa del usuario, pero todavía no se implementa su modelo 3D.

## Cambios

- Logo: extraje el distintivo original de `assets/icon-source.png` con transparencia, en dos tamaños RGBA4444, mediante `tools/build_logo_v244.py`. Se ve en portada, tarjetas, créditos, trofeos y cabecera de pausa. Captura de 480×272 revisada visualmente. El diseño no procede de GTA.
- Carros: colores de producción menos saturados, reflejo más visible en las texturas neutrales, cristal más oscuro, pilar central adaptado a cada carrocería y seis techos distintos en el LOD lejano. Se conservan perfiles, puertas y ruedas 3D. Recomiendo revisar sus proporciones en PSP antes de cambiar hitboxes; el LOD distante todavía simplifica mucho las ruedas.
- Revisé tus dos capturas de carros v2.44 y añadí lunetas traseras para las seis carrocerías. Bajé las barras de techo y el alerón para que no parezcan piezas flotantes. Las grandes superficies traseras y las ruedas lejanas aún merecen revisión física en PSP; no cambié dimensiones de colisión.
- Palmera: tronco más esbelto en tres tramos, nueve frondas cercanas y seis lejanas; eliminé el cono sólido de la copa distante.
- Nico: pelo ampliado alrededor del cuero cabelludo y manga más holgada sobre el brazo, con piel reducida bajo la tela. La malla y el rig siguen siendo los originales; comprobar silueta en PSP al girar y correr.
- Radar: marco doble, indicador de norte, marcador de objetivo y flecha orientada, rellena y contorneada para legibilidad. Inspiración funcional: el [manual oficial de GTA San Andreas](https://media.rockstargames.com/rockstargames-newsite/img/manuals/en_us/GTA_SA_PS3_MANUAL_ENG.pdf) usa un radar con destinos reconocibles; no se copiaron sus gráficos.
- Pavimento: el color de ROAD/SIDEWALK se ajusta al reproducir una pieza cacheada para que el ciclo de luz no salte por sectores. La caché geométrica y su ritmo de reconstrucción no cambian. Verificar el efecto y FPS en consola.
- Nota anterior: `ui_font.h` ahora almacena alfa de 4 bits (119.700 → 59.850 bytes). La captura de menú se mantuvo legible. La superficie SIDEWALK/STUCCO en (1466,579) dejó de solaparse tras separar el descansillo del último peldaño; `qa_zfight_v233` ya no lista ese par. Permanece el par antiguo bajo el puente (zona 1388,2006), de baja prioridad según tu nota.
- Aviso de trofeo: título con Oxanium; cuerpo conserva Rajdhani.
- Por tu nota de colisiones de la estación, añadí `cm_metro_support()` para los 18 soportes estructurales: bloquean al jugador a nivel de calle y al carro; se puede caminar sobre la plataforma elevada. La prueba `qa_metro_support_v244.c` verifica ambos niveles.

## Comprobaciones

Pasaron compilación PSP de PBP e ISO, `qa_character_suite.py` (14 suites), `qa_title_ui`, `qa_pause_exit_v244`, `qa_metro_support_v244`, `qa_metro_station`, `qa_car_lod_v244`, `qa_urban_visual` y `qa_zfight_v233`. Este último todavía da pares antiguos fuera de la estación, sobre todo el puente (1396,2006). Render de portada y tarjetas inspeccionado a resolución nativa. `narcade.elf` medía 19.165.032 bytes entre text/data/bss antes de integrar tu rama; sigue siendo importante vigilar el margen de RAM. Medición PC de `qa_cpucost_v230`: 0,789 ms/fotograma de geometría antes de la integración, que no equivale a FPS de PSP. La consola física aún debe comprobar palmeras, costuras de Nico, lunetas, color nocturno, legibilidad del radar y rendimiento. `release/Narcade_v2.44_PSP.zip` incluye ISO, PBP y los tres fondos de carga.

## Integración de ramas

Tu rama añadió `assets/logo-narcade.png`, `src/logo_narcade.h` y `tools/build_logo.py`; al fusionar los sustituí por un único recurso compartido en `assets/narcade-logo-*-v244.*`. `logo_draw()` conserva tus llamadas en pausa/carga y usa mi versión pequeña. El script `tools/build_logo_v244.py` regenera ambos tamaños. Se mantienen tu inglés inicial y el comportamiento de pausa/carga. No es necesario incorporar tus tres recursos de logo eliminados al repetir la integración.

No encontré otros bugs nuevos confirmados en tu rama. El solapamiento antiguo del puente (1396,2006) persiste; quedaría para la siguiente tarea si el usuario decide priorizarlo.
