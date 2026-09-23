# Notas de Claude para GPT Codex — estado del proyecto Narcade (17-sep-2026, actualizado 15:30)

Este documento resume TODO lo que Claude hizo en este repositorio después del checkpoint de Codex
(commit `5c49661`), qué quedó verificado en la **PSP E-1000 real**, qué quedó pendiente y con qué
evidencia. Léelo antes de tocar nada. El historial completo está en `git log` y en `CAMBIOS_v1.1.md`.

## 1. Contexto de hardware y entorno (verificado)

- Consola: PSP E-1000 (Street), CFW **PRO 6.60**, no permanente: tras un apagado completo hay que ejecutar
  `FastRecovery`. UMD ISO MODE debe estar en **Inferno** (menú VSH con SELECT) para que las ISO aparezcan.
- Compilación local en Windows: `python tools/build_windows.py` (toolchain pspdev en `build/pspdev`, dejada por Codex).
  Produce `EBOOT.PBP`, `narcade_static.elf` y `Narcade.iso`. También hay CI en GitHub Actions (`.github/workflows/build.yml`).
- Emulador: `build/ppsspp/PPSSPPWindows64.exe --debugger=19321 --windowed EBOOT.PBP`; `tools/ppsspp_check.py input <boton> <frames>`
  para pulsar botones; `build/shot.py <png>` captura la ventana del emulador (lo añadió Claude).
- Instalación en la consola: `ms0:/PSP/GAME/NARCADE/EBOOT.PBP` y `ms0:/ISO/Narcade.iso`. Copias de versiones previas en
  `Descargas/Narcade_v1.0_backup` y `Descargas/Narcade_v1.2_backup`.

## 2. Lo que Claude cambió (v2.2 / v2.3), todo probado en PPSSPP y v2.3 instalada en la consola

### 2.1 Guardado nativo con el diálogo de la Memory Stick
- `src/psp_main.c`: `savedata_dialog()` con `sceUtilitySavedata` (modos LISTSAVE / LISTLOAD), 4 ranuras
  `ms0:/PSP/SAVEDATA/NARC00001000{0..3}`, icono = `ICON0.png` embebido (`src/icon0.S`), título
  "Mision xx/36 - <titulo>", detalle con dinero/vinilos/tiempo. Clave `NARCADEKEY2026`.
- Flujo: START > PARTIDA > "Guardar partida" → `saveRequest=1`; "Guardar y volver al título" → 3;
  CONTINUAR en el título → 2 (solo abre el diálogo si existe alguna ranura; si no, `game_continue()` = flujo antiguo).
  El bucle principal llama `game_take_request()` cada fotograma y despacha a `handle_save_request()`.
- `src/game.c`: `fill_save()`, `apply_save()`, `game_export_save()`, `game_import_save()`, `game_save_summary()`,
  `game_set_native_savedata()`, `game_request_result()`. El autoguardado en `PROGRESS.BIN` se mantiene intacto.
  `tools/qa.c` sigue compilando: sin `game_set_native_savedata(1)` todo funciona como antes.
- **Regla dura**: mientras el diálogo está abierto NO se alternan buffers (el sistema dibuja sobre el buffer visible).
  Y **nunca** `PSP_DISPLAY_SETBUF_IMMEDIATE`: en esta consola deja la pantalla en negro (probado 3 veces).
- Enlaza con `-lpsputility` (Makefile, Makefile.iso, build_windows.py).

### 2.2 Personaje 3D (`src/render3d.c`, función `person()`)
- Reescrita con proporciones humanas: altura 29.2 ≈ 7 cabezas; hombros 8.4 de ancho (antes 11.3), torso 4.8 de fondo
  (antes 7.2), cuello visible, brazos con codo y manos, piernas separadas con rodilla, zapatilla con suela.
  Mismos materiales, mismas UV (`cloth()`), misma animación (step/lift/bob). Capturas: `build/emulator/nico_zoom*.png`.
- Pendiente para Codex (es de la escena, no del modelo): el jugador aparece **dentro de la caja del terminal** del
  refugio (hub 0) al empezar, y **atraviesa los coches** (la colisión sigue siendo la del mapa 2D).

### 2.3 HUD mínimo (`src/game.c`, `hud()` y `minimap()`)
- Eliminadas la barra superior y la franja inferior permanentes.
- Arriba derecha: barra de vida (y del coche si conduces), dinero, indicadores de búsqueda.
- Arriba centro: nombre del barrio **solo 2.2 s** al cambiar de barrio (`g.hudDistrict/hudDistrictT`).
- Abajo centro (a la derecha del minimapa, x=92..470): frase del objetivo **5 s** al empezar cada objetivo
  (`g.hudStepKey/hudObjectiveT`), los avisos (`g.notice`) y la acción contextual ("[] HABLAR") al llegar.
  Los temporizadores solo corren con `g.screen==WORLD` (no durante diálogos).
- Abajo izquierda: minimapa circular r=32 (6 unidades/px): calles, manzanas, parques, río, patrullas (si hay búsqueda),
  objetivo (verde, pegado al borde si está lejos) y flecha del jugador.
- Mapa (SELECT / START>MAPA): chincheta amarilla con "!" en el objetivo actual y flecha coral del jugador.

### 2.4 Portada y música de la XMB
- `assets/ICON0.png` (144×80) generado desde `assets/icon-source.png` (arte neón NARCADE). Va en el PBP, en la ISO
  y como icono de las partidas guardadas.
- `assets/SND0.AT3` + `tools/make_snd0.py`: **NO FUNCIONA EN LA CONSOLA** (sí decodifica en ffmpeg/PPSSPP). Ver §3.

### 2.5 Empaquetado
- `Makefile`: `PSP_EBOOT_SND0`, `src/icon0.o` en OBJS, `-lpsputility`. `Makefile.iso` igual (sin SND0).
- `tools/build_windows.py`: `icon0.S` en fuentes, `-lpsputility`, `SND0.AT3` en `pack-pbp`.
- `tools/package_iso.py`: copia `PSP_GAME/SND0.AT3` y `PIC1.PNG`.

## 3. Pendiente principal: música en la XMB (SND0.AT3)

**Evidencia obtenida en la PSP real (prueba A/B):** un `SND0.AT3` original de Sony (tomado de una partida guardada
de la consola, ATRAC3plus) empaquetado en el EBOOT de Narcade **sí suena** en la XMB. Ninguno de los generados con
software libre suena, aunque ffmpeg y el parser estricto de PPSSPP (`Core/Util/AtracTrack.cpp`) los aceptan:

| Intento | Códec | Resultado consola |
|---|---|---|
| atracdenc `-e atrac3` (OMA→RIFF) | ATRAC3 LP2 132 kbps | no suena |
| idem + extradata canónica (`1, 0x1000, modo, modo, 1`) | ATRAC3 LP2 | no suena |
| atracdenc `--bitrate 64` vía RealMedia, frames des-scrambled (XOR `0x537F6103`), joint=1, `smpl` + `fact` con retardo | ATRAC3 LP4 66 kbps | no suena |
| atracdenc `-e atrac3plus` (tasa fija 352 kbps, 10 s = 444 KB) con cabecera `0xFFFE`+GUID idéntica a la de Sony | ATRAC3plus | no suena |

Los tres SND0 de Sony hallados en la consola son todos **ATRAC3plus** (stereo 96 kbps / mono 64 y 192 kbps), con
`fact` de 8 bytes (samples, retardo 0x866–0xE8D) y chunk `smpl`. Conclusión: la XMB exige ATRAC3plus codificado por
el codificador de Sony; el codificador libre (`atracdenc`) produce un bitstream que el decodificador real rechaza.

**Caminos posibles**
1. `at3tool.exe` (SDK oficial de Sony, propietario, no incluido): `make_snd0.py` ya lo detecta en
   `build/at3tool/at3tool.exe` y lo usa con `-e -br 66 -wholeloop`. El usuario decidió no conseguirlo por ahora.
2. Media Go 3.2 está instalado en el PC (`C:\Program Files (x86)\Sony\Media Go`). Su interfaz ya no expone ATRAC,
   pero trae el codificador oficial en `FFPlugsLegacyLibs.dll` (DLL de 32 bits) con la API C
   `LEGACY_atrac_get_handle / set_codec_info / set_encode_algorithm / init_encode / encode / flush_encode / free_encode`
   (importada por `FileIO Plug-Ins\atracplug\atracplug.dll`). Sin documentación; haría falta ingeniería inversa de las
   firmas y un Python de 32 bits para llamarla con ctypes. No se intentó.
3. Dejar el juego sin música de XMB (estado actual). El `SND0.AT3` que hay en `assets/` es el ATRAC3plus de 10 s de
   atracdenc: inofensivo (la XMB lo ignora) pero inútil. Si prefieres, quítalo del `pack-pbp` y de `package_iso.py`.

## 4. Otras cosas hechas fuera del juego (por si aparecen en el stick)
- `ms0:/seplugins/RemoteJoyLite.prx` (v0.19) activo en `game.txt` y `pops.txt` (NO en vsh.txt) para transmitir la
  pantalla al PC por USB. Driver libusb-win32 en el PC. Suele congelarse tras el logo por el puerto USB 3.0 (xHCI).
- `ms0:/PSP/GAME/PSPMAN` (reproductor FLAC/MP3) con icono/fondo personalizados; `ms0:/ISO/Cuphead_PSP_v0.8.2.iso`.

## 5. Reglas que conviene mantener
- Nunca `PSP_DISPLAY_SETBUF_IMMEDIATE`; el orden correcto es `SetFrameBuf(NEXTFRAME)` → `WaitVblankStart`.
- No `sceDmacMemcpy` ni `-lpspdmac` (dejaba pantalla negra en la E-1000).
- La ISO usa ELF **estático** (`Makefile.iso`, `narcade_static.elf`) como `EBOOT.BIN`; un PRX sin firmar no arranca.
- Mantener `tools/qa.c` compilando en PC.
- Documentar cambios en `CAMBIOS_v1.1.md`.

## 8. Actualización 17-sep (tarde): v2.4

### 8.1 Rendimiento (commit `4d9d733`) — medido con `NARCADE_PROFILE=1 python tools/build_windows.py`
Tu versión del 17-sep tardaba **138–156 ms por fotograma** (6–7 fps) en PPSSPP (que emula ciclos MIPS). Causas:
la malla del jugador (7.032 vértices) pasaba cada uno por `player_pose` (5 trig), `local()` (2 trig), la proyección
geográfica rígida (`geo_project` + `geo_heading` con `atan2`, `cos`, `sin` **por vértice**) y un recorte contra 6
planos con copias; y la ciudad se regeneraba entera sin descartar lo que queda fuera de cámara.
Ahora: **19–22 ms (45–50 fps)**. Cambios en `render3d.c` y `world_geo.h` (todos comentados con "Optimizacion (Claude)"):
- `sphere_visible()`: frustum culling por manzana (r=300), coche (r=30) y peatón (r=22).
- LOD: manzanas a >430 sin marcas viales/cebras/farolas/murales/árboles y un solo bloque por edificio; coches a >300 sin ruedas.
- `rigid_cache()`: proyección/rumbo/altura por objeto, no por vértice. `local()` cachea seno/coseno por ángulo.
- `polygon()`: rechazo/aceptación trivial por plano; solo recorta contra planos cruzados.
- Ruta rápida del jugador dentro de `person()` (sin `polygon()`).
- `geo_height()`: tabla de nodos de la rejilla de 80 (44×40) construida perezosamente; mismo resultado numérico.
- `player_pose()`: constantes de la marcha una vez por fotograma; `sin(pi*w)` ≈ `4w(1-w)`.
Nada de esto cambia el aspecto de cerca. Si añades geometría nueva, pásala por `sphere_visible()` y usa `far` para LOD.

### 8.2 Animación (commit `d810d76`)
`player_pose()`: brazos con balanceo opuesto y codo (más amplio al correr), contragiro hombros/cadera (torsión
`tw=-wave*.10*motion*upper`), arco del pie, inclinación proporcional a la velocidad. La forma y texturas del
personaje vienen del GLB del usuario (`personaje-v3.glb` via `tools/import_character.py`); no las toqué.

### 8.3 SND0.AT3 — CAUSA ENCONTRADA (commit `046008f`)
Todos mis intentos del 16-sep fallaban por una regla no documentada que otro proyecto verificó en hardware
(github.com/TotalKommando/psp-media-toolkit, FINDINGS.md §2.3): **`fact.samples + delay + 368 <= frames*1024`**.
Yo declaraba `frames*1024`. `tools/make_snd0.py` ya recorta el `fact` (delay 1024). El `assets/SND0.AT3` actual es
ATRAC3 LP4 66 kbps (192 B/frame, joint stereo, `smpl` de bucle) del clip 0:11–0:31. **Pendiente de confirmar en la
E-1000** (el usuario se llevó la consola). Ese proyecto también confirma que el ATRAC3plus de atracdenc (352 kbps)
lo rechaza la XMB incluso con `fact` correcto, y que Media Go 3.x no sirve para codificar ATRAC.

### 8.4 Binarios listos para instalar
`Descargas/Narcade_v2.4_listo/EBOOT.PBP` y `Narcade.iso` (compilados 17-sep 15:2x). Instalación: `ms0:/PSP/GAME/NARCADE/`
y `ms0:/ISO/`.

## 9. Actualización 17-sep (noche)
- **v2.4 instalada en la PSP** (EBOOT e ISO, 18:32). Pendiente de que el usuario reporte: fluidez, música XMB, guardado.
- Aclaración al usuario: un juego de PS2 no se puede convertir a ISO de PSP (otra arquitectura); PS1 sí (PSX2PSP/POPS).
- Incidente de red del PC (sin relación con el juego): YouTube sin miniaturas. Diagnóstico: el proveedor perdía la ruta
  hacia la red de Google (tracert moría tras 185.1.119.28; Cloudflare OK; ping a Google 100 % perdido). Se descartaron
  PC, Wi-Fi, DNS, 360 Total Security y Razer Cortex. Se resolvió solo (proveedor). Análisis de malware independiente
  (Microsoft Safety Scanner) NO ejecutado aún: Defender está desactivado por 360; nada sospechoso en procesos, drivers,
  arranque, hosts ni proxy.
- Herramienta AT3: ya no hace falta `at3tool` si el SND0 con la regla del `fact` suena en la consola (§8.3). Si no
  sonara, el siguiente candidato abierto es github.com/liangchunn/atrac (codificadores RE en Rust).
- GTA San Andreas PSP (Sandstone): sigue sin publicarse (17-sep). El único repo que dice serlo está vacío.

## Entrega posterior de Codex: v2.5, 17-sep-2026

Se leyó esta nota y se trabajó sobre cefb11c, sin revertir los cambios de Claude.
Prioridad del usuario: terrazas horizontales, rampas verticales suaves, coches
sin aplastamiento ni bloqueo lateral y cámara coherente con joystick.
Detalles verificables: VALIDACION_v2.5.md, LEEME_v2.5.txt y build/QA-v2.5.txt.
Binarios entregables: release/Narcade_v2.5_PSP.zip (ISO + PBP).
SHA256 ISO: 37abc7301307a52313e09cb83be9ba2cecdaaf39a08bd2cd6be980fd11dec0fc.
No se ha instalado ni validado esta versión en la PSP física. Sin commits nuevos.
El render conserva batching, LOD, trigonometría cacheada y ruta rápida del jugador;
se cambió la tabla de alturas por terrazas y se corrigió el culling de suelo
que falló en PPSSPP. No reintroducir el descarte de manzanas con radio 300 sin
comprobar cobertura del suelo. La compilación entregada no tiene NARCADE_PROFILE.

## 13. Trabajo interrumpido de Codex — v2.8 en desarrollo (18-sep-2026)

El usuario pidió parar aquí y dejar esta transferencia. **No considerar esta sección una entrega final.** La ISO y el
EBOOT actuales se compilaron con `NARCADE_PROFILE`, `NARCADE_SPAWN_X=1250`, `NARCADE_SPAWN_Y=1030` y
`NARCADE_HOUR=0.45f`: son binarios de diagnóstico del Centro, no deben instalarse ni publicarse. SHA256 temporal de
`Narcade.iso`: `415b2c71c1362834d2b1931fccdc4718d6f2702ad5a7c107392129caffc4e42b`.

### 13.1 Texturas y forma urbana

- Se generaron dos atlas originales con ImageGen y se copiaron al proyecto:
  `assets/urban-atlas-v28.png` y `assets/urban-detail-atlas-v28.png`.
- `tools/textures3d.py` usa el primer atlas para los ocho materiales base (asfalto, acera, ladrillo, estuco, comercio,
  techo, césped y agua) y el segundo para los ocho materiales extra de v2.7 (corteza, hojas, metal, hormigón, muro
  cortina, toldo, adoquín y fachada moderna). Se regeneraron los PNG y `assets/textures3d.bin` (819.200 bytes).
- Las fachadas altas repiten textura por planta (`height/24`) y por módulo de 54 unidades para evitar estirar una sola
  planta sobre todo el edificio.
- Las manzanas clásicas ahora usan frentes contiguos de anchura irregular, plantas comerciales continuas y, cuando
  caben dos filas, un patio interior arbolado. Las parcelas diagonales grandes se dividen en tres cuerpos recortados a
  su polígono, en lugar de una única torre triangular enorme.
- Las casas de comuna apoyan desde un nivel común y usan tejados a dos aguas menos exagerados.
- Se corrigió overflow con signo en `city_hash`: los productos se realizan como `unsigned`.

### 13.2 Edificios que desaparecen o parecen flotar

- Causa encontrada: `box()` y `prism()` hacían culling con una esfera cuyo centro vertical estaba implícitamente cerca
  del suelo. Una planta alta podía descartarse aunque estuviese dentro del encuadre, dejando un hueco y otra planta
  visible arriba.
- `src/render3d.c` añade `volume_visible(x,z,bottom,top,planRadius)`, con centro y radio vertical reales.
  `box()` lo usa después de resolver `fixedGround`; `prism()` hace lo mismo y restaura `fixedGround` si descarta.
- `tools/qa_city28.c` comprueba 12 alturas consecutivas tanto con cajas como con prismas. Resultado: PASS.
- Todavía hace falta recorrer visualmente toda la ciudad en PPSSPP y verificar cimientos en laderas. No declarar el
  problema completamente cerrado solo por la prueba automática.

### 13.3 Metro

- `metro_update()` acelera y frena progresivamente: 35 u/s² al arrancar, 45 u/s² al frenar y velocidad máxima 130.
- `R3Scene.metroDoors` abre/cierra las puertas durante la parada. Cada puerta tiene dos paneles deslizantes y cristal.
- Tren y estaciones recibieron techos curvos propios (`rail_canopy`), bajos/bogies y equipos sobre el techo.
- Se investigó en fuente oficial: Línea A usa trenes de tres coches; documentos oficiales muestran tren blanco con
  franja verde/amarilla. El juego conserva dos coches por presupuesto geométrico, pero la apariencia sigue esa guía.
- Falta una prueba jugable completa: esperar, ver abrir puertas, subir, viajar, frenar, bajar y comprobar colisiones.

### 13.4 Día, noche, luces y sombras

- La noche tiene más iluminación ambiente para conservar lectura en la pantalla PSP.
- Los edificios cercanos generan ventanas encendidas individuales deterministas; se redujo el falso brillo de toda la
  fachada de `CURTAIN/MODERN/SHOP/GLASS`.
- Sol, luna y nubes ahora se dibujan con discos segmentados que miran a cámara, no cuadrados planos.
- Sombras de personas/coches y charcos de luz usan abanicos de 12 segmentos con borde alfa cero.
- Se añadieron sombras suaves de las parcelas edificadas cercanas y los faros se acortan al encontrar un edificio.
- Los efectos pasan por recorte de los seis planos mediante `fx_triangle`; antes podían atravesar la cámara.
- `GLOW_MAX` subió de 3000 a 6000. Revisar coste/memoria en PSP real antes de conservarlo definitivamente.
- `tools/qa_city28.c`: 512 combinaciones de ciudad/metro/hora/cámara, sin overflow; pico observado 29.532 vértices de
  mundo, 402 glow y 396 shadow; ciclo día/noche muestreado 1.001 veces, finito y acotado. PASS.

### 13.5 Caminar, trotar, correr y estamina

- Controles objetivo: caminar 42 sin X; mantener X = trotar 68; tres pulsaciones de X separadas 0,07–0,45 s = correr
  100. La fase de animación se adaptó a esas velocidades.
- Se añadió `g.stamina` (0..100) y `g.exhausted`. La carrera consume toda la barra en 6 s. Agotada, cancela el sprint y
  obliga a trotar. Recupera 12,5/s moviéndose o 18/s quieto; no permite correr otra vez hasta llegar a 100.
- El HUD muestra una barra amarilla de estamina debajo de la salud y roja cuando está agotado.
- Estamina y estado de agotamiento son transitorios: al iniciar/cargar se restauran a 100/normal y no cambian el formato
  del guardado.
- `tools/qa_locomotion.c` se actualizó a 42/68/100. Pasa marcha/trote/carrera, entradas lentas, pausa, pared y estamina
  a 30 y 60 Hz. El mensaje de prueba confirma agotamiento en seis segundos, bloqueo hasta recarga total y nuevo sprint.

### 13.6 Estado de pruebas y siguiente paso obligatorio

- PASS: `qa_locomotion`, `qa_render3d` y `qa_city28` en sus últimas ejecuciones.
- `qa_regression3d` FALLA en la prueba antigua `tap(B_SELECT); assert(g.screen==MAP)` porque Claude cambió SELECT a
  zoom y movió el mapa a PAUSA > MAPA. Es una prueba obsoleta, no evidencia de fallo del juego. Actualizar `tools/qa.c`
  para abrir START/PAUSE y seleccionar MAPA; después ejecutar la regresión completa.
- No se terminó la inspección visual de la compilación de perfil abierta en PPSSPP. No hay capturas finales ni medida
  fiable nueva de tiempo de dibujo.
- Antes de entregar: actualizar QA obsoleta, ejecutar todas las pruebas, inspeccionar día/noche/Centro/Metro/laderas,
  ajustar rendimiento si supera 33 ms, compilar de producción SIN flags `NARCADE_*`, verificar ISO y crear un paquete
  versionado nuevo. No sobrescribir la v2.5/v2.6 previa.
- Archivos modificados principales: `src/city26.inc`, `src/daylight.inc`, `src/game.c`, `src/render3d.c`,
  `src/render3d.h`, `src/psp_main.c`, `tools/textures3d.py`, `tools/qa_locomotion.c`, `tools/package_iso.py`.
  Nuevos: los dos atlas y `tools/qa_city28.c`. El árbol está sucio y contiene trabajo del usuario/Claude: preservarlo.

## 10. v2.5 (17-sep noche, commit `02e61b3`) — instalada en la PSP
- Ciudad irregular en `src/city3d.inc` (incluido desde `render3d.c`; `city()` solo llama a `city_v25()`), ciclo
  dia/noche en `src/daylight.inc`. Detalle en `CAMBIOS_v1.1.md`. Reglas: la red de calles y `solid()` no se tocan;
  todo lo nuevo pasa por `box()/ground()` (que ya iluminan por cara) o por el pase aditivo (`glow_quad`).
- Escala: `PERSON_SCALE 0.62` en `local()`/ruta rapida del jugador. Si cambias la malla del jugador, mantenla en
  unidades originales (0..29); la escala se aplica al dibujar.
- Control: `g.inputYaw=g.viewYaw` cada fotograma (sin anclaje); `camera_follow` con omega 4.5 y limite 2.1 rad/s.
- Velocidades a pie: 42/68/100. Tus NPC (npc-*.png, MAT_COUNT 29) siguen intactos.
- Pendiente de probar en consola: rendimiento real (~30 fps en PPSSPP), musica XMB (regla del fact), y el aspecto
  nocturno (faros/farolas) en la pantalla real.

## 11. v2.6 (17-sep noche, Claude) — trazado irregular real, Metro a escala, iluminacion, escala — instalada en la PSP
- **`src/citymap.h` es la unica fuente de verdad del trazado** (la incluyen `game.c` y `render3d.c`): celdas 8x7 con
  flags `cm_cells[bz][bx]` (MERGE_E/S = manzana doble, TUNNEL = paso de 48 bajo un edificio puente, SPLIT_X/Z = calle
  central de 40, PARK, PLAZA, STADIUM, PUEBLITO), Av. Oriental diagonal (1010,700)->(1330,1560) que recorta parcelas
  en triangulos/trapecios, parcelas = poligonos convexos `CmParcel` (cache `cm_build()`), `cm_solid()`/`cm_parcel_at()`,
  `cm_obstacle()` (pilares del Metro, pedestales y fuente de Plaza Botero, fuentes de parques) y geometria del Metro.
  **Si cambias donde se dibuja algo solido, cambialo aqui; `solid()` en game.c es `cm_solid||cm_obstacle` + rio + bordes.**
- Render: `src/city26.inc` (`city_v26()`), reutiliza los helpers de `city3d.inc`. `prism()` extruye poligonos;
  `fill_rect()` adapta los 7 tipos de manzana al tamano real de la parcela (subdivide las dobles); `fill_poly()` para
  parcelas triangulares; Estadio, Plaza Botero, Pueblito Paisa, Metro (viaducto x=1486, estaciones bz 1,3,5),
  Metrocable (1760,520)->(2460,120). Dentro de una parcela se fija `fixedGround=parcel_level(p)` para que podio,
  jardines y edificios compartan nivel en ladera (`box()`/`prism()` respetan un `fixedGround` ya fijado).
- **Metro jugable**: escalera de 60 (x 1465..1476, lz 258..318 de las filas de estacion) sube al anden (30.2). `g.lift`
  = `cm_lift()`; `lift_ok()` impide salir del anden por el borde. Tren: `metro_update()` (130 u/s, 6 s en cada estacion,
  va y vuelve). `[]` en el anden con el tren parado = subir (`g.inMetro`); `[]` parado en estacion = bajar. Camara
  dentro del coche delantero (`camera()` en render3d.c). `R3Scene` tiene `lift, metroZ, metroDir, inMetro`.
  Mientras `inMetro` no hay paseo ni coche (la rama de conduccion exige `g.car>=0`; antes indexaba `cars[-1]`).
- Iluminacion: cielo con degradado (cilindro `skyMesh` sin profundidad; `horizonColor` en daylight.inc) y niebla del
  color del horizonte; `quad2()` = degradado vertical (oclusion ambiental falsa) en caras de cajas/prismas altos.
- Escala: `PERSON_SCALE 0.52` (jugador 15 u = 1.75 m; piso 24 u). La camara a pie sigue en 43/11.
- Minimapa: rejilla precalculada `mapGrid` (paso 4) en `world_init()`; no evaluar poligonos por pixel (costaba ~3.5 ms).
- Pruebas: `build/tour.sh X Y nombre` (compila con `-DNARCADE_SPAWN_X/Y`, opcional `TOP=-DNARCADE_TOPVIEW=330` para vista
  aerea sin niebla) y `build/metrotest.sh`. Con `NARCADE_SPAWN_X` definido el HUD muestra posicion/lift/metro/overflow.
  Tiempo de fotograma en PPSSPP: ~33 ms (v2.5: 28-34).
- Pendiente de confirmar en consola: rendimiento real, subir al Metro, musica XMB.
- Peticion del usuario (17-sep): extraer modelos de GTA LCS/VCS con BLeeds/MDL viewer. **No se hizo**: son assets con
  copyright de Rockstar; usar solo modelos propios o CC0 (Kenney, Quaternius) via `tools/import_character.py`.

### 11.2 v2.6.2 (17-sep, noche) — instalada en la PSP
- **Guardar/cargar ya no usa el dialogo de lista de Sony** (LISTSAVE/LISTLOAD: lento y con restos de dibujo en la
  consola). Menu propio `SLOTS` en game.c (4 ranuras con titulo/detalle leidos de `PARAM.SFO` por `slot_info()` en
  psp_main.c) y el sistema solo hace **AUTOSAVE/AUTOLOAD** en silencio con `saveName` de la ranura elegida
  (`game_request_slot()`). Titulo > CONTINUAR abre el menu solo si hay alguna ranura; si no, historia nueva.
- **SELECT = zoom de camara** en 4 niveles (`g.zoom`, tablas `camFootDist/Eye`, `camCarDist/Eye`; por defecto 1 =
  normal 50/34; 0 lejana 65/43; 2 cercana; 3 muy cercana). `R3Scene.eyeHeight`. El mapa sigue en PAUSA > MAPA.
- **Edificios flotando**: con `fixedGround` impuesto (nivel de parcela), `box()`/`prism()` ahora bajan el cimiento
  hasta el terreno mas bajo de su huella (antes solo 1 unidad: en ladera se veia el hueco).

## 12. v2.7 (17/18-sep, Claude) — formas no cubicas, texturas nuevas — instalada en la PSP
- **8 materiales 64px nuevos en RAM** (`tools/extra_textures.py`, se generan en `tools/textures3d.py`; `MAT_COUNT 37`;
  enum `BARK,LEAVES,METAL,CONCRETE,CURTAIN,AWNING,COBBLE,MODERN` = indices 29..36). Regenerar con
  `python tools/textures3d.py` si se tocan (escribe `assets/textures3d.bin`). Cada material cuesta 196 KB de malla
  estatica (`mesh[MAT_COUNT][8190]`): no anadir a la ligera.
- **`src/shapes.inc`** (incluido tras city26.inc): `cylinder()` (prisma regular <=8 lados), `frustum()` (cono /
  piramide / tronco), `vault()` (boveda), `wheel()`, `hull()` (carroceria por secciones), `tree_shape()`,
  `chamfered_rect()`. Todas pasan por `polygon()` (recorte, proyeccion, modo rigido de coches, luz por normal).
- **Coches**: `car()` en render3d.c usa `hull()` con 6 siluetas (sedan, hatchback, pickup, furgoneta, deportivo, SUV)
  por `type`, ruedas con `wheel()`, retrovisores, bajos. LOD: >380 dos cajas; >300 sin ruedas.
- **Arboles**: `tree()` elige por hash: frondoso (tronco + troncos de cono LEAVES), palma (tronco en dos tramos
  inclinado + 7 frondas), cipres. Simplificados con `cityMid` o a mas de 170.
- **Edificios**: torres cilindricas o achaflanadas de muro cortina con corona; clasicos con esquinas achaflanadas
  (MODERN/CONCRETE); iglesia con cupula (troncos de cono) y chapitel piramidal; mercado con boveda metalica; casas de
  comuna con tejado de zinc a cuatro aguas; parcelas triangulares con CURTAIN/MODERN; podios de hormigon.
- **Mobiliario**: farolas con brazo (`lamp_post`), bancos con respaldo (`bench`), papeleras, fuentes redondas, kiosco,
  toldos con textura AWNING, Plaza Botero adoquinada (COBBLE, escala 18), pilares del Metro cilindricos.
- Rendimiento medido en PPSSPP en el Centro: ~35 ms (v2.6.1: 32) con mucha mas geometria; resto de la ciudad 22-28.
  Palancas si hace falta: `far`/`cityMid` en `city_v26()`, `tree()` (170), `nearby` de coches (500) y peatones (270).
- Pruebas: `-DNARCADE_HOUR=0.02f` fija la hora (noche) en compilaciones de prueba.

> **Continuación inmediata:** leer la sección 13 de este documento. Contiene la transferencia completa del trabajo
> v2.8 interrumpido el 18-sep-2026, el estado real de pruebas y la advertencia sobre los binarios de diagnóstico.

## 14. v2.8 terminada — guardado Sony, control, choques, portada y ambiente (18-sep-2026)

- Se eliminó por completo la pantalla propia de cuatro ranuras. Continuar usa
  `PSP_UTILITY_SAVEDATA_LISTLOAD` y Guardar/Guardar y salir usan
  `PSP_UTILITY_SAVEDATA_LISTSAVE`, con `saveNameList` de cuatro entradas. La selección,
  sobrescritura y cancelación se realizan en el panel oficial de Sony. Cancelar una carga
  conserva el título; cancelar un guardado conserva la pausa.
- La referencia del joystick queda anclada durante cada gesto y solo se recalcula al soltar
  o cambiar claramente su dirección. Esto evita que mantener izquierda/derecha durante una
  carrera describa un círculo por la rotación simultánea de la cámara.
- X mantenida trota; tres pulsaciones repetidas corren. La ventana tolera pulsaciones entre
  0,035 y 0,60 s y mantiene la carrera 0,70 s entre pulsaciones. Se conserva estamina,
  agotamiento y recuperación completa antes de volver a correr.
- Los choques frontales contra un carro estacionado detienen el vehículo a poca velocidad y
  aplican un rebote limitado y progresivo en impactos fuertes. El roce lateral conserva el
  movimiento tangencial y la separación geométrica sigue usando hasta ocho pasadas.
- La portada del XMB (`PIC1.png`) se usa ahora dentro del título mediante una versión indexada
  de 240x136 escalada a pantalla. Tiene exposición, reflejos y neón animados en tiempo real,
  manteniendo un coste pequeño en memoria y evitando un decodificador de vídeo.
- Durante el mundo se mezcla ambiente urbano estéreo procedural: aire de calle, tráfico grave
  y bocinas lejanas. Sigue sonando bajo la radio del coche y no añade una pista grande a la ISO.
- Pruebas superadas: campaña completa (146 objetivos), menús y solicitudes nativas de guardado,
  marcha/trote/carrera/estamina a 30 y 60 Hz, movimiento lateral, choques, 896 escenas de cámara,
  512 combinaciones ciudad/Metro/día-noche, 12 plantas elevadas, 42 peatones y animación separada
  de ambas piernas. Compilación final de producción sin `NARCADE_PROFILE`, spawn ni hora forzada.
- ISO final: 12.916.736 bytes. SHA-256:
  `c8fad9814a2a9ec6e186df5ac6c2799d38b74f72dd122b0aed1b2237759b7236`.

## 15. v2.9 (18-sep, Claude) — dialogo Sony sin restos, giro continuo, direccion progresiva
- **Dialogo de guardado de Sony (LISTSAVE/LISTLOAD) "con el slot anterior pegado" / invisible**: causa real encontrada.
  Los dialogos de sceUtility dibujan en el **buffer de dibujo actual del GE**, no en el que se muestra. Tras
  `game_draw(snap)` el GE apuntaba al buffer oculto: el dialogo pintaba ahi, la copia al visible arrastraba los restos
  acumulados (o, con el titulo de Codex, el dialogo era invisible y el juego parecia colgado). Arreglo:
  `r3_set_draw_buffer(fb)` (render3d.c) antes de cada `sceUtilitySavedataUpdate`, con el fondo restaurado por memcpy.
  El usuario QUIERE el dialogo de Sony: no volver al menu propio.
- **Control a pie**: direccion del stick relativa a la camara actual en cada fotograma (sin anclaje por gesto) +
  rumbo suavizado `g.moveYaw` con giro limitado (7.5/5.5/4.2 rad/s andando/trotando/corriendo; media vuelta = giro
  seco). Mantener izquierda/derecha = curva continua. Camara `camera_follow` mas agil (omega 5.5, 3.0 rad/s; en coche
  6.5 y 3.6 rad/s: antes la camara era mas lenta que el coche y el giro parecia "trabarse").
- **Coche**: direccion progresiva `g.steerSmooth` (sin saltos), agarre ligeramente menor a >140 de velocidad.
- Velocidades a pie 42 / 74 (X mantenida) / 105 (tres X). El trote ya funcionaba (medido 67); se separo mas del paso.
- Herramienta: `python tools/ppsspp_check.py hold up,cross 2.0` mantiene varios botones; `build/movetest.sh`,
  `build/dlgtest2.sh`, `build/dlgtest3.sh`.

### 15.1 v2.9.1 — dialogos del sistema: patron correcto (verificado en PSP-IA en consola real)
Los dialogos de sceUtility (guardado, teclado OSK) pintan CON TRANSPARENCIA sobre el buffer de DIBUJO que figura en
el estado interno de sceGu y solo repintan lo que cambia. Lo unico que funciona limpio en la consola es el patron de
los ejemplos del SDK: (1) `sceGuDrawBuffer`/`sceGuDispBuffer` (no las variantes `...List`) para fijar draw/disp;
(2) en cada fotograma restaurar el fondo (memcpy desde RAM) EN EL BUFFER DE DIBUJO y writeback de cache;
(3) lista GU vacia (Start/Finish/Sync); (4) `Update(1)` tambien en estado INIT; (5) WaitVblank; (6) `sceGuSwapBuffers()`.
Restaurar el buffer visible, alternar buffers a mano o no restaurar dejaban el slot anterior pegado, medio dialogo
invisible o parpadeo. Helpers en render3d.c: `r3_gu_buffers`, `r3_gu_idle`, `r3_gu_swap`. `exit_cb` llama a
`sceKernelExitGame()` directamente para que HOME funcione aunque haya un dialogo abierto.


## v2.10 — Codex, 22 septiembre 2026

Carrera: se lee uiMake de sceCtrlReadLatch y se entrega via game_latch_cross;
recoge pulsaciones completas entre fotogramas. Una carrera activa se renueva
con cada tap hasta 1,05 s; solo para iniciarla se piden tres taps cercanos.
El estado tolera 0,16 s de perdida de movimiento. No se ignoran colisiones ni
estamina. QA nuevo tools/qa_city210.c cubre ritmo irregular 20-60 Hz, latch,
obstaculos delante/lateral/altura y bocinas con cooldown.

Audio: tools/city_audio.py genera PCM original integrado por assets.S.
Capa ciudad 32 s/11025 Hz, motor 2 s/11025 Hz y bocina 0,55 s/22050 Hz.
Mezcla entera, estereo, volumen segun proximidad; sin bocina periodica.
Los NPC frenan ante el jugador en el carril y pitan tras 0,65 s, cooldown
7-11 s por coche. No se altera el formato de guardado.

Visuales: assets/materials-v210.png generado con image_gen; prompt completo
en assets/materials-v210-prompt.txt. tools/textures3d.py compila siete tiles
actualizados sin aumentar el layout de VRAM (819200 bytes). Conserva la
chaqueta principal, rostro y colores de carroceria. GU_FOG desactivado y
horizon_build dibuja 384 triangulos opacos de montanas/edificios tras el cielo,
sin escribir profundidad. Es decorado, no extension jugable.

Validacion: qa_city210, qa_locomotion, qa_regression3d y qa_city28 pasaron.
PPSSPP v1.20.4: arranque, paseo y dialogo Sony inspeccionados por capturas
VRAM usando configuracion aislada qa-software.ini. OpenGL oculto producia
capturas parciales; no se cuentan como validacion. PSP fisica no probada.
ISO de produccion sin flags de perfil/spawn/hora, 14473216 bytes.
SHA256 f19b0e951eb995f6ee07a32595fc7c0aeb33b06c618826e092adbf62114e50d5.
Entrega en release/Narcade_v2.10_PSP.zip y release/Narcade_v2.10/.

### 15.2 v2.10.1 (Claude) — la pieza que faltaba en el dialogo de guardado: sceGuDisplay(GU_TRUE)
El patron del SDK (§15.1) era correcto pero NO bastaba en la consola: `r3_init()` nunca habilita la salida del GU
(el juego dibuja por CPU y presenta con `sceDisplaySetFrameBuf`), y **sin `sceGuDisplay(GU_TRUE)` la llamada
`sceGuSwapBuffers()` no cambia lo que se ve**: el dialogo de Sony pintaba en un buffer que jamas llegaba a pantalla,
de ahi el menu "a medias" o con el fotograma anterior pegado. Diferencia exacta con el teclado OSK de PSP-IA, que si
lo habilita en su init. Ahora `r3_gu_buffers()` fija draw/disp + offset/viewport/scissor y habilita la salida; al
cerrar el dialogo, `r3_gu_display(0)` y un `sceDisplaySetFrameBuf` devuelven el control al bucle del juego.
No quitar `sceGuDisplay(GU_TRUE)` de ahi.


## v2.11 — rueda de armas, 22 septiembre 2026
A pie, mantener L 0,18 s abre rueda de ocho opciones. Joystick selecciona
por ángulo con zona muerta central; soltar L equipa. Un toque corto no abre.
La rueda congela el avance/estamina y Start la cierra para pausar. En coche
y Metro no abre. Arma en memoria transitoria: no cambia formato de partida.
Puños ocupan índice 0 arriba; índices 1..7 son pistola, revólver, subfusil,
AK, escopeta, rifle y bate. La mano derecha y el codo adoptan una pose de
sujeción; modelos de geometría ligera siguen la mano al caminar/correr.
No se ha implementado uso/disparo, munición ni daños.
QA: qa_weapons cubre los 8 sectores, centro, pausa, vehículo y congelación;
qa_weapon_mesh cubre 504 poses y estabilidad geométrica. ISO estática PSP
compilada sin flags de diagnóstico; SHA256 e0db79cb0dd8eb060fee8a6ee3f0787109543a07055e6b1a407ce843b0ae69f3.
No hay aún prueba en PSP física.

### 15.3 v2.13.1 (Claude) — la PSP se REINICIABA al cargar partida: memoria de la particion de usuario
Sintoma: en la consola (no en PPSSPP) el dialogo de Sony se abria y mostraba la lista, pero al elegir la ranura la
PSP se reiniciaba. Causa: la particion de usuario de un juego son **24 MB**. Estaticos actuales: text 6,65 MB +
bss 10,76 MB = 17,4 MB (mesh 8,06 MB, overlay 1 MB, background del dialogo 0,56 MB, mapGrid 0,36 MB, commands
0,26 MB, glow/shadow 0,21 MB, arte del titulo v2.13 ~1,3 MB en text). Con `PSP_HEAP_SIZE_KB(4096)` quedaban ~2,6 MB
libres y **sceUtilitySavedata necesita varios MB al cargar** (modulos + descifrado + icono): se quedaba sin memoria
y saltaba la excepcion. PPSSPP no aplica ese limite, por eso alli funcionaba.
Arreglo: `PSP_HEAP_SIZE_KB(1024)` (el juego no llama a malloc ni una vez: 0 en game.c/render3d.c/psp_main.c) y un
guardia `sceKernelMaxFreeMemSize()<1400 KB` que avisa por pantalla en vez de dejar caer el sistema.
**Antes de anadir mas datos estaticos (arte, audio, texturas, mallas) comprueba `psp-size`: text+bss+heap debe
quedar por debajo de ~20 MB.** Si hace falta espacio: `MAX_VERTICES` (8190) y `MAT_COUNT` (41) son los que mandan en
`mesh` (8 MB); bajar MAX_VERTICES a 6144 libera 2 MB y solo descarta poligonos en escenas extremas.

### 15.4 v2.13.2 a v2.13.6 (Claude) — la PSP se REINICIABA al cargar partida: diagnostico completo y lecciones
**Sintoma**: en la consola (nunca en PPSSPP) al elegir la ranura en el dialogo de Sony la PSP se reiniciaba. La misma
partida copiada a PPSSPP cargaba perfectamente.

**Como se diagnostico** (herramientas que quedan en el codigo y conviene reutilizar):
- `dbg()` en psp_main.c escribe pasos con marca de tiempo y memoria libre en `ms0:/NARCADE_DEBUG.TXT`. **Solo escribe
  si existe el fichero vacio `ms0:/NARCADE_DEBUG.ON`**: crealo para depurar en consola, borralo para jugar normal.
- Carga por etapas (`game_load_stage`) con barra de progreso: partida -> parcelas -> trafico -> peatones -> minimapa
  -> camara. Cada etapa se registra por separado.
- `r3_trace()` traza las fases del render (`r3:camara`, `r3:ciudad`, `r3:coches`, `r3:jugador`, `r3:hubs`,
  `r3:efectos`, `r3:ge-inicio`, `r3:ge-fin`) y cada celda de ciudad (`c3,4c`) en los primeros fotogramas.
- Hilo `watchdog` (prioridad 0x08) que vuelca la fase actual si se queda congelada: distingue CUELGUE de CRASH.

**Lo que dijeron los registros**: memoria libre 7,9 MB en todo momento (NO era falta de RAM), la partida se importaba
bien, todas las etapas terminaban y la consola moria siempre dentro de `city()` del primer fotograma, en menos de
0,6 s (crash, no cuelgue).

**Causa real**: la partida guardada provenia de una version anterior del juego. Pasaba las comprobaciones de rango y
se cargaba, dejando un estado incoherente que en la consola reventaba al dibujar la ciudad (PPSSPP lo toleraba).
Al empezar una partida nueva y sobrescribir el guardado, todo funciona.

**Blindaje anadido**: `game_import_save()` ahora exige que el tamano del bloque coincida EXACTAMENTE con
`sizeof(Save)` y avisa "Esa partida es de una version anterior del juego y no se puede cargar" en vez de cargarla.
**Si cambias la estructura `Save`, sube tambien `version` y manten este rechazo**: una partida vieja nunca debe llegar
al render. Ojo tambien con `savecheck()`: el checksum no detecta cambios de layout si los campos cuadran por casualidad.

**Otros arreglos de la tanda** (mantener):
- `PSP_HEAP_SIZE_KB(1024)` (el juego no usa malloc) y `MAX_VERTICES` 8190 -> 6144: de 21,4 MB a 16,4 MB de los 24 MB
  de la particion de usuario. Antes de anadir arte/audio/mallas, comprobar `psp-size` (text+bss+heap < ~20 MB).
- El audio del juego se detiene durante los dialogos de sceUtility y se restaura al cerrarlos.
- El dialogo se espera hasta el estado NONE (descarga completa del modulo) antes de que el juego vuelva a dibujar.
- Plugin RemoteJoyLite desactivado en la Memory Stick del usuario (`seplugins/game.txt` y `pops.txt`, copia en
  `game.txt.bak`): engancha el cambio de framebuffer y es un riesgo con los dialogos del sistema.
### 16. Narcade 2.15 (Codex) — materiales viales y bordillos

- `tools/road_materials.py` genera de forma determinista dos materiales sin recursos externos: asfalto con ruido periódico a varias escalas, árido fino y parches discretos; y losas de andén con juntas, variación entre piezas y desgaste. `tools/textures3d.py` los inserta en los dos materiales existentes del atlas PSP (128×128 RGB565), sin añadir mallas ni texturas nuevas en memoria.
- `src/city26.inc`: los andenes rectangulares ahora llevan caras verticales de hormigón sobre todo su perímetro, subdivididas para seguir la malla de relieve. Conservan la cota transitable 1.2 y no alteran la lógica de colisiones ni el formato de partida. Los andenes poligonales ya tenían faldón mediante `slab_draped`/`prism`.
- Versión de ISO 2.15 en `tools/package_iso.py`. Se mantienen intactos el guardado nativo y el rechazo de partidas incompatibles.

### 17. Narcade 2.16 (Codex) — movimiento, tráfico y mundo visual

- `src/game.c`: el desplazamiento a pie ahora se integra en coordenadas proyectadas, de manera que caminar cuesta arriba mantiene una velocidad coherente; la penalización por desnivel está limitada. La velocidad de caminar/trotar/correr no cae a cero por una breve pérdida de entrada y el margen de la secuencia de pulsaciones X es más tolerante.
- El tráfico civil ocupa corredores de doble sentido con carriles separados, conserva la dirección en ellos, frena por coches delante y alterna el paso en los semáforos cada cuatro segundos. Los vehículos aparcados de misión se apartaron del carril de circulación. La patrulla policial conserva su lógica de persecución. `tools/qa_traffic_v216.c` prueba 80 s sin solapamientos entre coches civiles y un tramo de ascenso a pie.
- `tools/car_materials.py` sustituye la antigua puerta/ventana dibujada que se deformaba sobre carrocerías. `src/shapes.inc` dibuja ventanillas 3D sobre paneles de pintura continuos; `src/render3d.c` añade parachoques, parrilla, matrícula, manillas y detalles distintos para pickup, SUV y deportivo.
- `tools/extra_textures.py` dibuja cuatro rótulos distintos por familia de tienda, restaurante y taller. `street_front()` usa UV directas para que los nombres se lean desde la calle sin reflejo horizontal.
- `src/shapes.inc` añade ramas y una copa frondosa irregular a los árboles cercanos; `src/daylight.inc` mejora silueta y sombreado inferior de las nubes.
- `Save` y su versión siguen intactos. Los materiales sustituyen ranuras ya existentes, sin aumentar el presupuesto de RAM/VRAM.

### 18. Narcade 2.17 (Codex) — caminar/trotar, estabilidad del suelo y pintura de coches

- `src/game.c`: zona muerta del joystick con histéresis, cámara con margen angular al caminar y seguimiento más lento a velocidades bajas. El paso usa velocidad recorrida filtrada y tolera breves interrupciones de un fotograma; se ajustan el apoyo, la zancada y la elevación del pie al caminar y trotar. El estado transitorio se reinicia al cargar, sin modificar `Save`.
- `src/render3d.c`: UV de carretera y andén ancladas a coordenadas del mundo y tamaño físico constante. Los dos materiales usan mipmap RGB565 de 64×64 en RAM (16 KB nuevos, sin consumo adicional de VRAM), para reducir el centelleo al avanzar. `tools/road_materials.py` suaviza el ruido de píxel del asfalto.
- `tools/car_materials.py`: pintura metálica con reflejos y árido fino; laterales con nervio de chapa, umbrales y pasos de rueda. `src/shapes.inc` conserva el patrón longitudinal a través de las piezas de la carrocería y da marco a las ventanillas. Doce acabados de color estables por vehículo; los detalles menores se simplifican si el lote PSP llega al límite de vértices.
- Pruebas nuevas: `tools/qa_gait_v217.c` (caminata/trote continuos y cámara estable) y `tools/qa_cars_v217.c` (24 coches cercanos sin desbordar materiales). La prueba de tráfico v2.16, la campaña, el guardado y las 512 vistas de ciudad también deben seguir pasando.

### 19. Narcade 2.18 (Codex) — dirección libre y giro a pie

- `src/game.c`: el coche conducido por el jugador sigue fuera de `civilian_traffic_tick`. La dirección responde antes a baja velocidad; un contacto con el entorno frena el desplazamiento sin deshacer un giro que cabe en el lugar. Los edificios siguen siendo sólidos y `separate_cars` mantiene las colisiones con otros vehículos.
- La dirección del joystick a pie se fija al comenzar cada gesto y cambia según el ángulo que mueve el jugador, sin que la rotación retrasada de la cámara vuelva a introducir giro por sí sola. Se aumentó la respuesta del giro al caminar, trotar y correr, y la orientación visual alcanza antes el rumbo de desplazamiento. Solo se usan campos transitorios existentes: `Save` no cambia.
- `tools/qa_controls_v218.c` comprueba un giro sostenido a las tres velocidades mientras sigue la cámara, y un coche conducido fuera del carril. ISO 2.18 en `tools/package_iso.py`.

### 20. Narcade 2.19 (Codex) — pintura y detalles 3D de los coches

- `tools/car_materials.py` genera una nueva pintura metálica original con reflejos amplios de cielo y entorno, brillo longitudinal, nervio lateral y sombra en los bajos. Se mantienen los dos materiales RGB565 existentes (128×128), sin incrementar VRAM. No se incrustó una fotografía de un modelo concreto porque las ventanas y puertas quedarían fuera de sitio en las otras carrocerías.
- `src/shapes.inc`: la textura lateral usa ahora UV ancladas a la altura real de la pieza; las líneas del panel ya no se doblan al pasar de capó a puerta. `car_skin()` calcula la superficie inclinada de cada perfil para colocar elementos sobre ella.
- `src/render3d.c`: las juntas de puertas, manillas con hueco y pieza brillante, y retrovisores siguen la cabina de cada una de las seis siluetas. El presupuesto de vértices METAL conserva margen para atascos; `tools/qa_cars_v217.c` comprueba 24 coches simultáneos.
- Atlas y binario de texturas regenerados. ISO 2.19. Formato `Save` sin cambios.
