# Narcade v2.51 — recursos y revisión de Codex para Claude

Rama `codex/title-neighborhood-visuals`, basada en `main` 76450f7. **Aún no hay carrusel en código**: te corresponde integrar la alternancia, la transición y el zoom de la portada. El usuario pidió que la rotación empiece al entrar y que la imagen actual permanezca como una de las tres.

## Nuevas escenas listas para PSP

- `assets/title-car-barrio-v251-source.png` / `.png` / `.565`: carro deportivo en primer plano, visto desde abajo en tres cuartos, dentro de una cuadra de casas, sin panorama ni atardecer.
- `assets/title-parques-barrio-v251-source.png` / `.png` / `.565`: dos hombres jugando parqués y fumando frente a una casa de barrio, sin panorama de la ciudad.
- Cada `.565` mide exactamente 480×272, RGB565 little-endian, 261120 bytes. `tools/build_title_scenes_v251.py` reproduce ambos paquetes a partir de los PNG fuente. La esquina superior izquierda queda libre para el logo existente y se oscureció suavemente la zona inferior izquierda para `TL_PRESS`. No se incrustó texto ni marca en las escenas.
- Para mostrar las escenas, agrega los `.565` a `src/assets.S`, las declaraciones a `src/assets.h` y el estado de rotación al título. Reutiliza `title_logo`, `TL_PRESS`, `TL_PRESS_SUB` y los créditos encima de todas. Una pausa de varios segundos y un desplazamiento lateral con zoom leve evitan cambiar cada fotograma y el coste de escalado continuo en CPU. La compatibilidad del zoom en la PSP debe medirse; si provoca tirones, prioriza un fundido/deslizamiento simple.

## Otros cambios de mi rama

- Cinco iconos específicos en la lista de trofeos: huellas, volante, diana, estrella y vinilo.
- Ajustes: controles con pictogramas, selección más clara y barras de valor más legibles.
- Nubes: redistribuidas alrededor de la cámara a la altura de cielo visible; antes la nube podía generarse fuera del campo visible. Montañas: relieve con detalle fino y colores diferenciados en cresta/base.
- Piel del cuello y brazos de Nico calentada para aproximarse al color medio de la cara pintada. Se regeneró `textures3d.bin`; la cara y el cuerpo conservan su malla original.

## Revisión y límites

La compilación PSP y `qa_character_suite.py` pasan (incluidas 512 escenas de ciudad y 48 vistas de metro, sin overflow). No he visto en PSP física la translucidez de las nubes ni la concordancia definitiva de piel: compruébalas en la consola después de integrar. Tu rama v2.49 circular integrada pasó las pruebas, sin bug nuevo reproducible por mi lado. Los problemas que reporta ahora el usuario sobre ventanas que aparecen tarde, escaleras atravesables, carretera donde desaparecen coches, estaciones y luz quedan en tu ámbito; no los modifiqué.
