# Narcade v2.38: trabajo visual de Codex

Rama visual `codex/cars-metro-lighting-trees`, PR #3, integrada junto con `claude/rendimiento-estrellas`. Esta entrega no modifica el formato de guardado.

## Cambios de Codex

- Corregidos los cuatro hallazgos previos de Claude: límite de alcance de las piernas, balanceo de pelvis, conservación del alfa de los materiales y ruta portátil del compilador QA.
- Coches: seis perfiles de carrocería conservados y detallados con pasos de rueda, contornos de cristales, limpiaparabrisas, molduras y ópticas separadas. Se mantiene la geometría de manijas, puertas, espejos y placas adaptada a cada perfil. La nueva pintura utiliza variación de reflejos a partir de una referencia generada para el proyecto, combinada con los detalles procedurales. No es una fotografía descargada ni la reproducción de una marca. El paquete de texturas sigue ocupando 884736 bytes.
- Metro: cámara a la altura real del punto de viaje; piel del tren adaptada al desnivel de la vía, cristales de cabina y ventanas laterales. El cristal de cada puerta viaja con su panel, con ventanillas fijas cortas a ambos lados que no tapan la abertura.
- Luces: los dos haces del coche mantienen una dirección recta en el espacio proyectado al pasar por curvas; su superficie sigue el terreno. El punto luminoso y el charco de luz de la farola se sitúan bajo su brazo, en lugar de bajo el poste.
- Árboles cercanos: copas redondeadas con varios grupos de hojas de radios y tonos distintos, conservando las ramas y los modelos lejanos ligeros.
- Tráfico denso: los detalles de carrocería reservan espacio para las mallas de los coches restantes; un atasco de veinte coches cabe en los materiales sin descartar polígonos.

## Validación

Once suites host aprobadas en la integración de ambas ramas: controles, marcha, combate, armas, 512 escenas de ciudad/metro/día/noche, 720 poses de marcha y las seis carrocerías/tres árboles/puertas del metro. También pasan `qa_wanted_v237.c` y `qa_load_warm_v237.c`. Compilación PSP y extracción/verificación del ejecutable dentro de la ISO aprobadas. No se afirma validación en PSP física ni una cifra de FPS de consola.

La vista `urban-visual-preview.png` usa las mallas y materiales del juego en un rasterizador de PC; no es una captura de PSP. Para regenerarla, compilar `tools/export_urban_visual.c`, ejecutarlo y correr `tools/preview_urban_visual.py` con NumPy y Pillow instalados. La referencia de pintura está en `assets/reference/automotive-clearcoat-reference-v237.png`; se generó con imagegen integrado, solicitando pintura automotriz plateada fotorealista con reflejos suaves de cielo y ciudad, sin puertas ni emblemas.

## Para Claude

Ver `REVISION_CODEX_RAMA_CLAUDE_2026-09-28.md`. Los nuevos hallazgos se dejan documentados, sin corregirlos, como pidió el usuario. Conviene probar faros en curvas de noche, puertas abiertas del metro y rendimiento con varios coches y árboles próximos en una PSP real.
