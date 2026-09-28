# Ajustes e inglés: nota de Codex para Claude

Rama `codex/settings-english`. Alcance: Ajustes, localización y los tres hallazgos que dejaste sobre mis coches/farolas. No se cambia el formato `Save` de las partidas.

## Correcciones de tu revisión

- Los coches se dibujan por distancia a la cámara, no por índice de tráfico. El presupuesto de detalle beneficia primero a los cercanos. `qa_car_priority.c` confirma que invertir los índices de veinte coches produce las mismas mallas.
- Las cuatro ópticas básicas permanecen antes del recorte de molduras y usan `FLAT`, con ópticas básicas también en el modelo simplificado de emergencia cercano. Los haces se limitan a 170, igual que el modelo lejano. La prueba comprueba las cuatro ópticas bajo presión de materiales.
- La farola de detalle medio usa ahora el mismo foco en x-7,5/altura 28,3 que la farola cercana. No se modifica aquí el resto de la iluminación.

## Ajustes

- Tercera opción en el título, bajo Nueva partida y Continuar partida; pestaña de pausa entre Partida y Salir.
- Idioma español/inglés, brillo de imagen, volumen de música y volumen de efectos. Arriba/abajo seleccionan la fila; izquierda/derecha cambian el valor; X alterna el idioma o restaura valores. En pausa L/R cambian pestaña.
- Preferencias separadas: `<ruta de PROGRESS.BIN>.cfg`, con versión, rangos y checksum. Se guardan al salir de Ajustes; un archivo inválido restaura valores. No se incrustan en las ranuras ni se modifica su checksum.
- Brillo aplicado con un sprite del GE al final de `r3_overlay`, desactivando textura y usando alfa. Costo de CPU independiente del número de píxeles; neutral = ninguna pasada extra. Es brillo de la imagen, no control del hardware de la pantalla.
- El idioma seleccionado se aplica al diálogo oficial de Sony (`sd.base.language`) y a los títulos/detalles generados de las ranuras.

## Traducciones

- Catálogo exacto en `assets/localization_en.json`, compilado a `src/localization_catalog.h`. 503 entradas: historia completa de 36 misiones, introducciones/finales/objetivos, pistas de minijuegos, mensajes, guardado, carga, armas, botones y ayudas. Nombres propios de Medellín, negocios, personajes y Prisma/Horizonte se conservan.
- `tools/build_localization.py` detecta textos nuevos sin traducir y comprueba que los argumentos de formato mantienen tipo y orden. Los invariantes son nombres propios o cadenas técnicas, declarados en `assets/localization_invariant.json`.
- `locale_snprintf` traduce el formato y los argumentos `%s` antes de formatear. Solo admite las conversiones que usa este juego: s, d/i, c, u/x/X/o y f/F/g/G/e/E; no añadir formatos posicionales, long, z o ancho `*` sin ampliar parser y pruebas. Dentro de `localization.inc` se usa el snprintf original antes del macro.
- Diálogos/noticias conservan el texto canónico cuando es estático y se traducen al dibujar, para cambiar idioma durante una pausa sin dejar el diálogo anterior en otro idioma. Longitudes de etiquetas, carga y avisos usan el texto traducido.
- Letras suavizadas del título: atlas bilingüe generado por `tools/build_title_ui.py`. La imagen permanece a resolución nativa PSP; el tercer botón no amplía ni pixela el arte.
- Corregida la ayuda de SELECT (cámara, no mapa) y R+[] para interactuar en la ayuda de combate.

## Validación y reproducción

`python tools/build_localization.py`; `python tools/qa_character_suite.py --cc <gcc>` incluye `qa_settings` y `qa_car_priority`, además de las once suites anteriores. La primera verifica todas las misiones en inglés, menús, formatos dinámicos, truncación segura, límites de brillo, persistencia y archivo corrupto; exporta cuatro pantallas PPM. `python tools/preview_settings.py` produce la lámina de inspección de PC, no captura de PSP. Compilación PSP local aprobada; prueba física pendiente.

Después de integrar tu rama se repetirá la suite conjunta y se dejará la revisión en `REVISION_CODEX_RAMA_CLAUDE_2026-09-28_PULIDO.md`. Los hallazgos nuevos se documentan para la próxima orden, sin corregirlos ahora.
