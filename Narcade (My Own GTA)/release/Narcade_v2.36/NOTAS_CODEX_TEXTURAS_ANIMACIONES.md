# Codex: texturas y animaciones, 27 septiembre 2026

Rama: `codex/textures-human-gait`, base `86a5bcf` de main.
Alcance pedido: pavimento, texturas del personaje y marcha mas humana.
Las casetas, cache/rendimiento, trafico, colisiones y controles pertenecen a Claude.

## Cambios
- `src/human_gait.inc`: apoyos y recuperacion separados, rodilla de dos segmentos,
  transicion del tejido alrededor de la rodilla, balanceo de pelvis/torso y apoyo
  del talon/punta. La pierna apoyada no recibe el rebote del torso. Los calculos
  trigonometricos y articulaciones se preparan una vez por fotograma, sin asignaciones.
- `render3d.c`: incluye el nuevo rig y aplica colores continuos al pavimento.
  No se ha editado `game.c`: joystick, velocidades, estamina, conduccion y guardado
  conservan la revision de movimiento ya validada por el usuario.
- `street_surface.inc`: variacion de tono +/-6% en coordenadas del mundo, continua
  entre parcelas. Queda registrada en la cache de Claude; sin poligonos ni draw calls
  nuevos. Asfalto: baldosa de 64 unidades (antes 36), misma escala en toda la ciudad.
- `road_materials.py`: asfalto con arido fino y sin grietas/manchas identificables
  repetidas. Anden de hormigon con juntas finas y borde suave, no rejilla oscura.
- `surface_materials.py`: camiseta charcoal de algodon, cuello, costuras y detalle
  NR/Narcade; denim ancho con sarga, costura doble y bolsillo; piel y pelo coherentes.
  Se conserva el GLB del usuario, UVs y los detalles fotograficos de fachadas.
- `textures3d.py`: genera los ocho materiales modificados y las dos mipmaps existentes.
  Texturas totales: exactamente 884736 bytes, mismas IDs, VRAM y dimensiones.
  NPC, fachadas, coches y armas conservan sus bytes. No se anade ninguna textura.

## Validacion
- Suite anterior: controles, combate, guardado/campana, camara, armas y 512 escenas
  de ciudad/dia/noche sin overflow. Tambien `qa_human_gait.c`: 720 fotogramas,
  pies sobre el suelo, apoyo al caminar, continuidad entre fases, reposo y color
  continuo al cruzar el borde de las parcelas. Integrado en qa_character_suite.py.
- Compilacion PSP con SDK Windows, ISO con ELF estatico verificado.
- Inspeccion PC de la malla exacta: `export_human_gait.c` y `preview_human_gait.py`
  producen una hoja y GIF. Son inspecciones de la malla, no capturas de una PSP.
- Verificar en PSP fisica la percepcion de las tres marchas y el pavimento.
  La animacion sigue siendo procedural sobre la malla suministrada: no es mocap.
  No se promete sincronizacion perfecta de cada apoyo con los metros recorridos:
  la fase/velocidad que determina el juego se ha conservado para no alterar controles.
- PPSSPP aislado: depurador responde pero devuelve "CPU not started" al abrir
  esta ISO, igual que en la entrega anterior. No se afirma validacion de ejecucion
  ni de FPS. Se conserva la compilacion PSP y las comprobaciones host como evidencia.

## Reproducir
`python tools/textures3d.py` (Pillow), `python tools/build_windows.py --sdk RUTA_SDK`.
`python tools/qa_character_suite.py --cc RUTA_GCC` (10 suites).
Para inspeccion, compilar tools/export_human_gait.c con GCC -O2 -std=gnu99 -lm,
crear build/, ejecutar el exportador y despues python tools/preview_human_gait.py
(Pillow/numpy). Mantener los materiales fotograficos de ciudad existentes.

## Revision cruzada
Cuando Claude publique y termine `claude/casetas-rendimiento`, revisar su diff contra
86a5bcf y la integracion en main. No diagnosticar como fallo un cambio aun incompleto.
Especificamente: casetas/terminales deben respetar el mismo trazado en dibujo y
colision; las optimizaciones no deben ocultar suelo/metro ni romper los presupuestos,
los efectos de combate ni las transiciones de luz. Registrar hallazgos con linea,
reproduccion y evidencia, y no corregirlos fuera del alcance de esta rama.
