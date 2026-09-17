# Prompt para ChatGPT / Codex — Narcade PSP, continuar desde v2.7.1 (18-sep-2026)

Trabajas sobre la carpeta `C:\Users\carmo\Downloads\Narcade_v1.1_fuente` (repo `SrNar18/narcade-psp`, rama `main`,
ultimo commit `afb7662`). El juego corre en una PSP E-1000 con CFW PRO 6.60 (EBOOT.PBP en `ms0:/PSP/GAME/NARCADE/`
e ISO en `ms0:/ISO/`). Compila con `python tools/build_windows.py` (toolchain en `build/pspdev`); genera EBOOT.PBP y
Narcade.iso. Emulador de pruebas: `build/ppsspp` (scripts `build/quicklook.sh`, `build/tour.sh X Y nombre`,
`build/ab.sh etiqueta "-DFLAGS"`, `build/metrotest.sh`).

## Lee primero
`NOTAS_CLAUDE_PARA_CODEX.md` (secciones 10 a 12) explica todo lo hecho por Claude y las reglas de hardware:
- NUNCA `PSP_DISPLAY_SETBUF_IMMEDIATE` ni `sceDmacMemcpy`; swap = `SetFrameBuf(NEXTFRAME)` + `WaitVblankStart`.
- El ISO necesita un ELF estatico como EBOOT.BIN (`Makefile.iso` / build_windows.py ya lo hacen).
- El guardado usa AUTOSAVE/AUTOLOAD con menu de ranuras propio (pantalla `SLOTS` en game.c). No volver al dialogo
  de lista de Sony (LISTSAVE/LISTLOAD): en la consola era lentisimo y dejaba restos.

## Estado actual (v2.7.1)
- Trazado urbano irregular: `src/citymap.h` es la UNICA fuente de verdad (colision, minimapa, render). Manzanas
  dobles, partidas, triangulares (Av. Oriental diagonal), tuneles bajo edificios, Estadio, Plaza Botero, Pueblito
  Paisa, Metro (viaducto, estaciones, tren en el que el jugador se sube con []), Metrocable.
- Render: `src/render3d.c` + `city3d.inc` (helpers), `city26.inc` (ciudad), `shapes.inc` (cilindros, conos, bovedas,
  ruedas, carrocerias por secciones, arboles), `daylight.inc` (dia/noche, cielo, sombras, luces).
- 37 materiales: 21 en VRAM (128px), 8 NPC + 8 nuevos en RAM (64px, `tools/extra_textures.py`). Regenerar con
  `python tools/textures3d.py`. Cada material cuesta 196 KB de malla estatica: no anadir sin motivo.
- SELECT = zoom de camara (4 niveles). Mapa en PAUSA > MAPA.
- Rendimiento en PPSSPP: ~35 ms en el Centro (zona mas densa), 22-28 ms en el resto. Palancas: `far`/`cityMid`
  en `city_v26()`, distancia de arboles (170) en `tree()`, `nearby` de coches (500) y peatones (270).

## Lo que pide el usuario ahora (en orden)
1. Mejorar texturas y formas: menos cajas, mas variedad. Ya hay primitivas en `shapes.inc`; usalas en lugar de
   `box()` donde tenga sentido (peatones, mobiliario, detalles de fachada, puentes, estadio).
2. Fluidez en la consola real: apuntar a <=33 ms en el Centro. Medir con `NARCADE_PROFILE=1` (overlay de ms) o
   `build/ab.sh` antes y despues de cada cambio. No romper el LOD ni el descarte por esfera de `box()`/`prism()`.
3. Revisar en consola: edificios flotantes (los cimientos ya bajan al terreno mas bajo), texturas que parpadean
   (plano cercano 5, marcas viales a .5) y objetos atravesables (`cm_obstacle()` en citymap.h decide que es solido).

## Reglas
- No toques `citymap.h` sin actualizar a la vez colision y render: todo lo solido que dibujes debe existir ahi.
- No uses assets con copyright (modelos de GTA, etc.). Solo propios o CC0.
- Deja una nota al final de `NOTAS_CLAUDE_PARA_CODEX.md` con lo que cambies, y commit + push a `main`.
- Cuando termines, no instales en la PSP: dile al usuario que los binarios estan listos (EBOOT.PBP y Narcade.iso).
