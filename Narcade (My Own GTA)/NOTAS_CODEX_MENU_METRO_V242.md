# Recursos de Codex listos para Claude

Rama: `codex/metro-modern-ui`. Arte y tipografia preparados primero, como pidio Naresz. El trabajo del metro sigue en esta misma rama.

## Menu y trofeos

- Cinco tarjetas originales: Continuar (0), Nueva partida (1), Ajustes (2), Creditos (3), Trofeos (4). Ajustes es una tarjeta vertical a la derecha; Creditos/Trofeos debajo de las dos superiores. La navegacion sigue su posicion.
- `assets/menu-*-v242.png` son vistas previas; sus `.565` estan integrados en `assets.S`. El generador es `tools/build_title_ui.py`.
- Oxanium para titulos y Rajdhani para texto pequeno. Fuentes originales y licencias SIL OFL en `tools/fonts`; `tools/build_ui_fonts.py` genera `src/ui_font.h`. El texto se dibuja antialiasado y conserva el avance de 7 px para no romper los paneles existentes.
- Creditos y pantalla de trofeos funcionan como pantallas independientes. No activar PAUSE/WORLD desde estas pantallas por START.
- **Contrato para tu funcionalidad de trofeos**: `GameTrophyInfo`, `GameTrophyProvider` y `game_set_trophy_provider()` en `src/game.h`. Registra tu proveedor al iniciar el juego; recibe indice 0..4 y devuelve 1 con titulo, descripcion, progreso actual, objetivo y desbloqueado. Sin proveedor, el menu muestra PROXIMAMENTE, sin inventar desbloqueos. Tu parte sigue siendo los eventos, objetivos, persistencia y avisos de desbloqueo. Puedes elegir tus propios cinco trofeos; los nombres por defecto son provisionales.
- Traduce tus titulos/descripciones en el catalogo como el resto de textos. El menu ya llama `locale_text` mediante `text`.
- Tu correccion de bordes de las dos tarjetas antiguas se sustituye por bordes calculados con las dimensiones exactas de las cinco nuevas.
- Mantengo fuera de mi rama tu cambio del intervalo de carga a cinco segundos y la funcionalidad de trofeos.

## Decision expresa del usuario: Luna

**Naresz** pidio el 28-09-2026 que la mujer del retrato que acabo de generar sea **Luna** dentro del lore. Debo comunicartelo en la revision de bugs **porque el usuario me pidio expresamente que te lo dijera**.

El original es `assets/luna-menu-source.png`. Pelo oscuro rizado, chaqueta negra streetwear, luces magenta/cian y Medellin nocturna. Se usa como arte de Ajustes. El modelo/NPC dentro del mundo se implementara en una tarea futura, tal como indico el usuario; esta entrega no lo implementa.

Consultar tambien `NOTA_LORE_LUNA_PARA_CLAUDE.md`. No confundir el retrato del menu con un modelo 3D ya incorporado.
