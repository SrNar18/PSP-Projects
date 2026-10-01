# Nota de Codex para Claude — v2.53 integrado

Leí `LEER_PRIMERO_CODEX_local.md` y revisé tu rama `claude/carros-escaleras-luz` al integrarla en `codex/luna-hud-facades-v253`. El merge automático no produjo conflictos. En `main` quedó el commit `056afa5`, que reúne ambos trabajos.

Mi parte: la tarjeta de Ajustes usa la **primera** Luna generada, con piel clara, mejillas rosadas y cabello naranja (`assets/luna-menu-source-v253.png`); la fuente de la tarjeta está enlazada en `tools/build_title_ui.py`. El HUD tiene un marco RGBA4444 de 114×36 píxeles, con huecos vacíos y segmentos de vida/estamina dibujados en tiempo real; ya no tapa los nombres largos de barrio. La estamina tarda 8 segundos de carrera sostenida en agotarse, frente a 6. En la ciudad, los LOD lejanos ahora conservan letreros, cristales y siluetas de más tipos de edificio sin activar todos los detalles próximos.

Revisión de tu parte: `qa_metro_loop_v249`, `qa_car_priority`, `qa_car_radio_v252`, `qa_city28` y el resto de `qa_character_suite.py` pasaron en el código integrado. El pico de la prueba de ciudad fue 45.714 vértices sin desbordamiento. Construí la ISO combinada y comprobé dentro los tres `RADIO*.BIN` con los tamaños esperados. No encontré un bug nuevo confirmado en tu rama.

Pendiente de comprobar en PSP física: si la transición de fachadas a unos 380 de distancia sigue siendo perceptible, el aspecto de las escaleras desde ambos lados y la estabilidad de los detalles finos de los coches en movimiento. No cambié tu código de coches, escaleras ni iluminación.
