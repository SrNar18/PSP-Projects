# Codex → Claude: visuales v2.44 (29–30 septiembre de 2026)

Trabajo aislado en `codex/logo-cars-character-hud`, basado en `main` 08eb647. No cambié idioma inicial, zoom de carga, colisiones ni lógica del menú de pausa, que el usuario te asignó. Conservamos los nombres de los personajes actuales; **el retrato de la mujer es Luna** por petición expresa del usuario, pero todavía no se implementa su modelo 3D.

## Cambios

- Logo: extraje el distintivo original de `assets/icon-source.png` con transparencia, en dos tamaños RGBA4444, mediante `tools/build_logo_v244.py`. Se ve en portada, tarjetas, créditos, trofeos y cabecera de pausa. Captura de 480×272 revisada visualmente. El diseño no procede de GTA.
- Carros: colores de producción menos saturados, reflejo más visible en las texturas neutrales, cristal más oscuro, pilar central adaptado a cada carrocería y seis techos distintos en el LOD lejano. Se conservan perfiles, puertas y ruedas 3D. Recomiendo revisar sus proporciones en PSP antes de cambiar hitboxes; el LOD distante todavía simplifica mucho las ruedas.
- Palmera: tronco más esbelto en tres tramos, nueve frondas cercanas y seis lejanas; eliminé el cono sólido de la copa distante.
- Nico: pelo ampliado alrededor del cuero cabelludo y manga más holgada sobre el brazo, con piel reducida bajo la tela. La malla y el rig siguen siendo los originales; comprobar silueta en PSP al girar y correr.
- Radar: marco doble, indicador de norte, marcador de objetivo y flecha orientada, rellena y contorneada para legibilidad. Inspiración funcional: el [manual oficial de GTA San Andreas](https://media.rockstargames.com/rockstargames-newsite/img/manuals/en_us/GTA_SA_PS3_MANUAL_ENG.pdf) usa un radar con destinos reconocibles; no se copiaron sus gráficos.
- Pavimento: el color de ROAD/SIDEWALK se ajusta al reproducir una pieza cacheada para que el ciclo de luz no salte por sectores. La caché geométrica y su ritmo de reconstrucción no cambian. Verificar el efecto y FPS en consola.
- Nota anterior: `ui_font.h` ahora almacena alfa de 4 bits (119.700 → 59.850 bytes). La captura de menú se mantuvo legible. La superficie SIDEWALK/STUCCO en (1466,579) dejó de solaparse tras separar el descansillo del último peldaño; `qa_zfight_v233` ya no lista ese par. Permanece el par antiguo bajo el puente (zona 1388,2006), de baja prioridad según tu nota.
- Aviso de trofeo: título con Oxanium; cuerpo conserva Rajdhani.

## Comprobaciones

Pasaron compilación PSP (`make`, EBOOT.PBP), `qa_character_suite.py` (14 suites), `qa_title_ui`, `qa_metro_station`, `qa_urban_visual` y `qa_zfight_v233`. Este último todavía da pares antiguos fuera de la estación. Render de portada y tarjetas inspeccionado a resolución nativa. `narcade.elf` mide 19.165.032 bytes entre text/data/bss; sigue siendo importante vigilar el margen de RAM. Medición PC de `qa_cpucost_v230`: 0,789 ms/fotograma de geometría, que no equivale a FPS de PSP. La consola física aún debe comprobar palmeras, costuras de Nico, color nocturno y legibilidad del radar.

## Integración de ramas

Vi en tu checkout `assets/logo-narcade.png`, `src/logo_narcade.h` y `tools/build_logo.py` sin seguimiento todavía. También se basan en la portada original. Al fusionar, usar una sola extracción y un solo juego de tamaños para evitar duplicar memoria y pequeñas diferencias entre portada, pausa y carga. Mis fuentes originales PNG y RGBA4444 están en `assets/narcade-logo-*-v244.*`; el script los regenera. Tus cambios de pausa/carga y el inglés inicial son tuyos; adaptaré únicamente la llamada al logo que requiera la integración, sin cambiar su comportamiento.

Sin otros bugs nuevos confirmados en esta ronda. Cuando termines tu rama, revisaré tu diff y dejaré una nota de hallazgos sin corregirlos hasta la siguiente orden del usuario.
