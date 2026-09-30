# Narcade v2.50 — nota de Codex para Claude Code

**Corrección posterior a la captura del usuario:** la primera extracción del logo convertía un trozo del cielo azul del arte fuente en un rectángulo opaco a la izquierda de la N. El recorte/umbral se corrigió en `codex/logo-xmb-cutout-fix`. Comprueba `build/title-logo-fixed.png` y `build/story-logo-fixed.png`: el extremo inferior izquierdo es transparente y la punta diagonal de la N sigue visible. `tools/qa_logo_silhouette.py` impide volver a empaquetar el bloque azul.

Rama `codex/xmb-logo-world-visuals`, desde `main` 74a8f99. Mi ámbito: logo del inicio/Historia, luz y color del mundo, mapa completo de Start y dos defectos visuales de tu nota. No modifiqué personaje, coches ni metro; esos están bajo tu trabajo actual.

## Cambios

- Recuperé el logo pintado de `assets/icon-source.png`, que es el de la portada de la XMB. `tools/build_logo_v244.py` extrae solo el neón vivo, excluye el cielo oscuro y conserva el trazo azul/cian y naranja/magenta. El recurso `narcade-logo-small-v244.*` sigue compartido por portada, Historia, créditos y pausa. Conservé ese nombre para no cambiar código de interfaz ni los recursos de Sony.
- En `daylight.inc`, luz solar más direccional, sombra de relleno más fría, cielo/horizonte menos gris, nubes teñidas por el atardecer y un halo solar ligero. Es luz por vértice y geometría transparente, compatible con la GE de PSP; no requiere shaders ni un filtro de CPU por fotograma.
- El mapa de Start ahora ajusta **la altura proyectada** (`z*1,5`) del valle. Antes tomaba 2240 como altura visible y recortaba el tercio sur; ahora se ven ambos extremos del área jugable. `qa_map_full_v250.c` comprueba que los límites norte, sur, este y oeste queden dentro del panel.
- Quité la pieza de material ROOF solapada con la punta BARK de las palmeras (tu nota de parpadeo) y separé el techo de cristal de los postes de una misma parada de bus. No toqué la posición de las marquesinas respecto de farolas/semáforos de otras celdas.

## Comprobación y límites

- Título e Historia renderizados a 480×272 en `build/title-cover-v250.png` y `build/title-menu-v250.png`. El mapa completo se inspeccionó en `build/map-full-v250.png`. Son vistas de PC, no capturas de PSP física.
- `qa_map_full_v250` y `qa_title_ui` pasan. `qa_character_suite.py` pasa (14 baterías); `qa_city28` repetido tras los últimos ajustes: 512 escenas sin desbordes, pico 53.286 vértices de mundo. `qa_sprint_cost_v236` en PC: 0,578 ms de media y p95 0,877 ms, frente a 0,569/0,831 ms antes de mis ajustes (variación pequeña; no representa los FPS de la PSP). La compilación PSP y la ISO se validaron.
- Comprueba luz de mañana/atardecer/noche en PSP y si los colores de las nubes se ven como esperamos. La previsualización de malla de PC no reproduce el blend de la GE. Comprueba también las superposiciones de marquesinas con farolas/semáforos de otras celdas que quedaron de tu nota anterior.
- Tu revisión de `codex/character-ui-logo-v247` advirtió coste de los NPC de 12 lados y posible estiramiento de caras. No los modifiqué porque el usuario te asignó ahora el diagnóstico y arreglo del personaje; conviene valorar LOD cercano/lejano al integrarlo.

Revisión técnica: la GE usa transformación e iluminación fija, textura única y combinadores simples; el sombreado especular de hardware es costoso. Por eso concentré el cambio en colores de luz por vértice, cielo y halo geométrico, conservando el presupuesto de renderizado. Referencias: https://www.ppsspp.org/docs/psp-hardware/gpu/ y https://www.ppsspp.org/docs/development/psp-internals/ge-performance/.
