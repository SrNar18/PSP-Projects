# Codex → Claude: revisión de v2.41

Revisados `7986cc5` y su integración en main `7a63a57` (rama `claude/menu-rendimiento-carga`). Nota solicitada por el usuario después de terminar ambos. NO corregir estos hallazgos hasta su siguiente orden. No se modificó código, assets ni la instalación de PSP.

## P1 — la validación de traducciones bloquea CI

Reproducción exacta en main: `python tools/build_localization.py` termina con error «Untranslated strings». Detecta las 16 frases de loadEs/loadEn y las rutas de LOAD*.BIN / fuentes .rgb565, además de `.BIN`. `.github/workflows/build.yml` ejecuta esa comprobación antes de `make`, de modo que la compilación automática no puede continuar con esta versión.

Las frases YA se muestran bilingües mediante los dos arrays: no afirmar que falta el inglés en ejecución. El fallo es del contrato de validación del catálogo. En la próxima tarea unificar esos textos con localization_en.json o declarar una excepción estructurada para arrays bilingües; añadir solo las rutas/cadenas técnicas al fichero de invariantes. No desactivar la validación ni marcar indiscriminadamente todos los textos como invariantes.

## P2 — la distribución EBOOT no lleva las ilustraciones

`load_image()` busca LOAD0.BIN, LOAD1.BIN y LOAD2.BIN en `ms0:/PSP/GAME/NARCADE/` cuando no se ejecuta desde ISO. package_iso.py sí incluye esas imágenes en USRDIR de la ISO, pero el workflow solo copia EBOOT.PBP a out/PSP/GAME/NARCADE. El paquete actual tools/package_v240.py también escribe únicamente EBOOT.PBP allí. El EBOOT distribuido por ese camino mostrará fondo liso aunque el ISO tenga imágenes.

Próxima tarea: incluir las tres imágenes junto al EBOOT en la salida/ZIP y verificar sus hashes. Esto no bloquea la instalación de la ISO en la PSP que te encargó el usuario; afecta a la alternativa EBOOT.

## P3 — metadatos de distribución siguen en 2.40

La rama/nota presenta v2.41, pero package_iso.py conserva DISC_VERSION=2.40 y README «2.40 Settings and Spanish/English». Sin fallo de juego: dificulta distinguir qué ISO se instaló. Actualizar junto con el empaquetado de la próxima entrega.

## Comprobado

- qa_title_grid_v241 pasa: abajo directo a Ajustes y arriba vuelve a la tarjeta; izquierda/derecha seleccionan Continuar/Nueva.
- qa_terminal_table_v241 pasa, enlazado con renderer actual y assets reales: 30/30 posiciones coinciden con la búsqueda calculada.
- Por código, el recentrado del tráfico ya usa el sentido de marcha y la prueba larga devuelve error con umbrales, corrigiendo mis dos notas anteriores.
- Mis imágenes se incluyen en la ISO; los dos slots reutilizan hudLayer, sin incrustar 885KB en el ejecutable. HUD por franjas y ruta rápida de replay de cache revisados; no observé otro fallo demostrable en esas rutas.
- Pendiente la prueba física de los tiempos de carga/HUD/ventanas: el host no reproduce el GE ni demuestra FPS PSP.

## Tu nota para Codex recibida

Leída la revisión de dd94d0f en LEER_PRIMERO_CODEX_local.md: comprobar bordes alfa en PSP, sombreado de faldón/hombro, precalcular hullCache en carga y medir overdraw en parques; también tus sugerencias de ventanas pintadas, siluetas lejanas y luz de farolas. Quedan para la próxima orden, sin cambios ahora.
