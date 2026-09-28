# Narcade v2.43 — menu moderno y metro

Trabajo de Codex en `codex/metro-modern-ui`, integrando Claude v2.42. Creador: Naresz.

## Cambios

- Cinco tarjetas: Continuar, Nueva partida, Ajustes, Creditos y Trofeos. Portada, menus, pausa, HUD y texto usan Oxanium para titulos y Rajdhani para texto pequeno; la firma Naresz conserva la caligrafia original. Bordes exactos, letras antialiasadas, arte preparado a resolucion PSP.
- Creditos a Naresz, Codex/OpenAI, Claude Code/Anthropic, Medellin/valle de Aburra y personajes originales. Espanol e ingles, tambien en los trofeos.
- Los cinco trofeos de Claude funcionan en el menu principal y la pausa. No se reimplementaron sus eventos ni su guardado. El proveedor opcional de game.h permite pruebas/variantes, pero no es necesario registrar nada: el menu usa el sistema existente por defecto.
- Retrato original de Luna para Ajustes. **El usuario decidio que es Luna y pidio que se lo comunicara a Claude al revisar bugs.** Su modelo/NPC se implementara mas adelante; no esta incorporado por esta entrega.
- Vias del metro visibles en ambos LODs. Se separan los dos andenes; ya no cubren el trazado central. El tren es mas estrecho, el suelo/equipos inferiores dejan libre la plataforma y sus puertas se mantienen sobre su propia carroceria.
- Tres estaciones con acceso por escalera mas detallada, rellano, bordes amarillos, bancos y senal de entrada METRO apoyada en postes. La via central queda abierta y las cubiertas siguen la curva del mapa para no invadir trenes/edificios por un tramo recto largo.
- Se eliminaron las caras coplanares duplicadas de las cubiertas: causaban parpadeo. Culling esta desactivado en el renderizador: una cara ya es visible desde ambos lados. Los travesanos respetan la altura del tren y su equipo del techo.
- Indicacion de subir/bajar del metro tambien tras completar la historia.
- Dos notas previas de Claude corregidas: sombreado vertical de carroceria y precalculo de las doce variantes de su cache durante la carga. Las observaciones sobre halos/overdraw de hojas siguen pendientes de medicion en PSP real.

## Validacion

Compilacion PSP e ISO; catalogo de 552 traducciones; navegacion de cinco tarjetas, creditos, trofeos, idiomas, stride 512; ajustes/persistencia; audio de menus; cinco trofeos; 480 fotogramas del tren con puertas abiertas/cerradas; siete filas de vias y ambos LODs; tres accesos peatonales y subida/bajada; nueve escenas completas de estaciones sin desbordar la malla; seis carrocerias y atasco de veinte coches; precarga de seis perfiles y dos LODs.

Las imagenes de `build/menu-v242-*.png` son vistas de la interfaz generadas con `game_draw` en PC. `build/metro-v243-preview.png` renderiza la malla real para inspeccion en PC; no es una captura de PSP. PPSSPP no abrio su puerto de depuracion en este entorno tras dos intentos, por lo que no se afirma validacion visual en emulador ni consola fisica. La prueba en PSP queda pendiente.

## Recursos y fuentes

Oxanium y Rajdhani se incluyen en `tools/fonts` con licencias SIL OFL; sus avisos tambien van en ISO/ZIP. `tools/build_ui_fonts.py` y `tools/build_title_ui.py` permiten reconstruir las fuentes/arte de interfaz sin fuentes instaladas en Windows.

`assets/luna-menu-source.png` se genero con la herramienta integrada ImageGen (sin CLI/API externa). Brief: retrato original de una mujer colombiana adulta, pelo oscuro rizado, chaqueta negra streetwear, aros plateados, mirando a la izquierda; Medellin nocturna, luces magenta/cian y montanas al fondo; composicion vertical 3:4 con espacio oscuro inferior para texto, sin letras/logos ni personajes de GTA. Su nombre Luna se asigno DESPUES por decision del usuario.

Las otras tarjetas reutilizan el arte original de carga de Narcade, compilado/cortado a sus dimensiones nativas; no contienen arte ni personajes de GTA.

## Coordinacion

Consultar `REVISION_CODEX_CLAUDE_V242.md`: hallazgo menor sobre distancia al salir del metro, pendiente para la proxima orden. No se modifica aqui la funcionalidad de trofeos de Claude. El primer aviso de recursos estuvo publicado en 1d2c8a9 antes de terminar el metro.
