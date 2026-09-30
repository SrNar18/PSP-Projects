# Narcade v2.51 — recursos y revisión de Codex para Claude

Rama `codex/title-neighborhood-visuals`, actualizada con tu `main` 6ffb729. Tu carrusel de tres escenas, paneo y fundido ya está en código. Conecté tus `tools/build_title_slides.py` con mis dos PNG fuente y generé `assets/title-slides/slide1.rgb565` y `slide2.rgb565`; la imagen actual permanece como `slide0`.

## Nuevas escenas listas para PSP

- `assets/title-car-barrio-v251-source.png` / `.png` / `.565`: carro deportivo en primer plano, visto desde abajo en tres cuartos, dentro de una cuadra de casas, sin panorama ni atardecer.
- `assets/title-parques-barrio-v251-source.png` / `.png` / `.565`: dos hombres jugando parqués y fumando frente a una casa de barrio, sin panorama de la ciudad.
- Cada `.565` mide exactamente 480×272, RGB565 little-endian, 261120 bytes. `tools/build_title_scenes_v251.py` reproduce ambos paquetes a partir de los PNG fuente. La esquina superior izquierda queda libre para el logo existente y se oscureció suavemente la zona inferior izquierda para `TL_PRESS`. No se incrustó texto ni marca en las escenas.
- Tu ruta ISO carga `TITLE0.BIN`, `TITLE1.BIN`, `TITLE2.BIN`, todos de 512×288 RGB565. Mi script adicional produce vistas nativas 480×272 para inspección y otros empaquetadores. `tools/package_iso.py` ya incluye las tres diapositivas; comprobé sus tres tamaños de 294912 bytes dentro de la ISO.

## Otros cambios de mi rama

- Cinco iconos específicos en la lista de trofeos: huellas, volante, diana, estrella y vinilo.
- Ajustes: controles con pictogramas, selección más clara y barras de valor más legibles.
- Nubes: redistribuidas alrededor de la cámara a la altura de cielo visible; antes la nube podía generarse fuera del campo visible. Montañas: relieve con detalle fino y colores diferenciados en cresta/base.
- Piel del cuello y brazos de Nico calentada para aproximarse al color medio de la cara pintada. Se regeneró `textures3d.bin`; la cara y el cuerpo conservan su malla original.

## Revisión y límites

La compilación PSP y la creación de ISO pasan. Antes de integrar tu carrusel, `qa_character_suite.py` pasó completa; después de integrar `claude/portada-metro-ventanas`, todas las baterías salvo `qa` pasan (incluidas 512 escenas de ciudad y 48 vistas de metro). **Bug reproducible en PC para tu revisión:** `tools/qa.c` llama `game_draw(frame,W)` con un framebuffer de paso 480; en `R3_HOST`, tu nueva `r3_slide_image()` escribe con paso fijo 512 (`target[y*512+x]`) y provoca `SIGSEGV` en el primer `snap("01-title")`. Confirmado por backtrace de GDB: `r3_slide_image -> draw_frame -> main`. La ruta PSP usa paso 512, así que no afirmo que la consola tenga el mismo fallo. Corrige la ruta host/contrato de paso cuando te corresponda. No toqué esa lógica por ser tu ámbito.

La primera ejecución de CI tras fusionar tu portada paró en `build_localization.py` porque faltaban tres rutas/formatos internos del carrusel en `assets/localization_invariant.json` (`assets/title-slides/`, `%sTITLE%d.BIN`, `%sslide%d.rgb565`). Añadí esas invariantes técnicas y el validador ya pasa; los textos visibles siguen traducidos.

No he visto en PSP física la translucidez de las nubes ni la concordancia definitiva de piel: compruébalas en la consola. Tu metro circular sigue pasando las pruebas específicas. Los problemas que reporta ahora el usuario sobre ventanas que aparecen tarde, escaleras atravesables, carretera donde desaparecen coches, estaciones y luz quedan en tu ámbito; no los modifiqué.
