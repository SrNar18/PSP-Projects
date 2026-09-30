# Nota de Codex para Claude — v2.46 (30-09-2026)

Rama `codex/cover-light-metro-v246`. Partí de `main` después de la v2.45 y después integré tu `claude/bugs-parpadeo-metro` ya fusionada a `main`; no edité tu rama ni tus cambios locales. En el conflicto del tren conservé tu posición adelantada de las puertas y mis franjas exteriores/cristales opacos.

## Cambios de esta ronda

- Portada nueva en `assets/title-cover-v246.png`, generada tomando como referencia el estilo de las tarjetas de Historia y la imagen de Luna. El personaje está de espaldas, con iluminación magenta/cian; la composición sigue siendo Medellín con metro y río. `tools/build_title_ui.py` produce el RGB565 de PSP desde esa imagen, pero conserva el fondo anterior del menú de tarjetas, que al usuario le gusta.
- La portada ya no dibuja el gran panel azul. Logo transparente arriba a la izquierda; invitación de inicio y crédito `made by Naresz` siguen como letras nítidas y localizables del juego. `tools/build_logo_v244.py` rellena los píxeles oscuros residuales de la N azul; el mismo recurso corregido se usa en portada, Historia, créditos y trofeos.
- Tarjeta nueva de Trofeos: `assets/trophies-menu-source-v246.png`, mesa con armas, mapa y medalla, en la dirección visual del resto de Historia. No cambié lógica ni navegación del menú.
- Día/noche: cielo de día más azul y claro, horizonte menos gris, luz rasante más ámbar y cielo magenta al atardecer. Se calculan con las interpolaciones continuas existentes; no hay filtro cálido fijo en todas las horas.
- Metro: la franja verde lateral ahora queda fuera del panel de la carrocería en vez de cruzarlo, que causaba parpadeo de profundidad. Los cristales opacos tintados usan METAL en lugar de GLASS para que no aparezcan huecos por la transparencia de esa textura; FLAT saturaba la malla en una de las nueve vistas de estación.

## Verificación

- Suite de personaje y render: 14 pruebas, incluido ciclo día/noche de 512 escenas y 480 fotogramas del tren, pasan.
- `qa_title_ui`: portada, X/Start y navegación pasan. Vistas previas a 480×272 en `build/title-v246-preview.png` y `build/menu-v246-preview.png`.
- `qa_metro_seams_v245`: tablero/carriles continuos y 35 soportes pasan.
- `qa_metro_views`: las nueve vistas de estaciones caben en las mallas. Vista compuesta en `build/metro-v243-preview.png` (el nombre heredado es del generador).
- ISO compilado y comprobado por `tools/package_iso.py`. Falta validación en PSP física de colores/legibilidad y del parpadeo durante el movimiento.

## Hallazgos para tu próxima inspección

- En la vista anterior a integrar tu rama se veía un prisma azul sobre un tejado marrón junto al viaducto. Tu cambio al edificio puente ya aborda el cubo flotante; comprueba en PSP si el volumen final se percibe apoyado.
- Una estación queda muy cerca de las fachadas (vista 2/B y 2/C); parte de la cámara queda tapada por edificios. Revisa en PSP si el tren o el andén se atraviesan visualmente desde la calle.
- El cambio de material de cristales quita los huecos de textura, pero merece comprobar desde dentro y fuera del vagón en PSP, con puertas abiertas y cerradas.

Deja en tu nota cualquier artefacto de geometría o luz que encuentres; lo incorporaré en la siguiente orden del usuario.
