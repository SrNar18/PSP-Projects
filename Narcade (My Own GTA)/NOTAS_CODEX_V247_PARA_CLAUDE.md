# Narcade v2.47 — nota de Codex para Claude Code

Trabajo en `codex/character-ui-logo-v247`, creado desde `main` (`8918d48`, que ya incluye tu corrección del tanque). No modifiqué tu trabajo de aceras, distancia de renderizado, ventanas ni rendimiento.

## Entregado

- Logo NARCADE rehecho desde cero con Oxanium y antialias; elimina la antigua N con puntos/cuadrados oscuros. Conserva `narcade-logo-small-v244.*` como ruta de recurso por compatibilidad.
- Nico conserva el estilo de la portada y tarjetas: pelo corto y oscuro, tez cálida, camiseta ancha oscura y vaqueros holgados. La cara está repintada en `assets/face-painted-v247.png` tomando como referencia el hombre de `assets/loading-v241/street-source.jpg`. La malla tiene volumen en nariz/mentón y se redujo la piel que asomaba bajo las mangas.
- Los NPC tienen caras originales diferentes entre sí y diferentes de Nico, con variantes de silueta y vestuario. Hay reacciones de protección al apuntarles o herirlos. Nuevas fuentes en `assets/npc-face-*-painted-v247.png`.
- Rueda de armas repintada en grafito y ámbar, con ocho iconos de armas generados para el juego (`assets/weapon-icon-sheet-v247.png`).
- Interior del menú Start renovado: mapa con destino/distancia/distrito, mensajes en formato de tableta, tarjetas de ajustes, trofeos y salir. Textos nuevos localizados al inglés.

## Comprobaciones

- `python tools/qa_character_suite.py --cc <host gcc>`: 14/14 baterías aprobadas, incluidas 1152 poses de arma/apuntar/andar y 720 fotogramas de marcha/trote/carrera.
- `make` y `make -f Makefile.iso`: compilación PSP satisfactoria.
- `python tools/package_iso.py`: ISO válida, ejecutable estático verificado; ID `NARC00001` conservado para las partidas.
- Vistas de estudio comprobadas en `build/character-combat-preview.png` y `build/pause-map-v247.png`. Son renderizados de inspección, no capturas de PSP física.

## Para tu revisión siguiente

- Al integrar tu rama, prueba en PSP física la legibilidad de los textos del menú Start y la cara de Nico bajo las distintas horas del día. La previsualización de PC tiene otra iluminación/resolución.
- Los peatones de fondo todavía son simplificados por el presupuesto de polígonos de PSP. Sus caras y ropa tienen variedad, pero al acercarse aún pueden verse rígidos. Si tu cambio de radio aumenta mucho el número de peatones visibles, mide FPS antes de incrementar detalle.
- La ilustración femenina de la tarjeta de menú permanece identificada como Luna para una futura implementación. No se reutilizó su cara para todos los NPC.
