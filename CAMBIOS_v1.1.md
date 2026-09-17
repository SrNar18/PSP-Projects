# Narcade v1.1 — cambios para PSP E-1000 (Street) real

Hechos por Claude tras probar en la consola fisica con CFW PRO 6.60.

## Correcciones de hardware
1. **Parpadeo** (`src/psp_main.c`): se usaba `sceDisplaySetFrameBuf(..., NEXTFRAME)` tras
   `sceDisplayWaitVblankStart()` y se empezaba a redibujar el buffer que aun se mostraba.
   Ahora: dibujar -> volcar -> esperar vblank -> `SETBUF_IMMEDIATE`.
2. **Rendimiento** (`src/psp_main.c`, `src/game.c`): el juego escribia pixel a pixel en VRAM
   sin cache (muy lento en hardware; PPSSPP no lo penaliza). Ahora se dibuja en un backbuffer
   en RAM con cache y se copia a VRAM con memcpy. Se anadio `px()`
   para trazar pixeles sin pasar por `rect()` con recorte (coches, lineas, texto), y `rect()`
   recorre por filas. Copia a VRAM con memcpy (v1.1.1: se retiro sceDmacMemcpy porque la importacion impedia arrancar en la consola). Fondo XMB: assets/PIC1.png (480x272) en EBOOT e ISO.
3. **ISO firmada** (`tools/package_iso.py`): el modo disco de la PSP rechaza un EBOOT.BIN sin
   firmar ("datos danados"). `tools/prxencrypter.py` (port Python del PrxEncrypter de pspsdk;
   `pip install pycryptodome`) firma el PRX con cabecera ~PSP tag 0xADF305F0 y se usa para
   `PSP_GAME/SYSDIR/EBOOT.BIN` y `BOOT.BIN`. Limite: PRX <= 5.583.616 bytes.

## Compilar
    make                      # narcade.prx + EBOOT.PBP (WSL/Linux con PSPDEV)
    python3 tools/package_iso.py   # Narcade.iso firmada

## Instalar
- `PSP/GAME/NARCADE/EBOOT.PBP` -> `ms0:/PSP/GAME/NARCADE/`
- `Narcade.iso` -> `ms0:/ISO/`; en la XMB mantener SELECT > UMD ISO MODE > Inferno.
- En E-1000 ejecutar FastRecovery tras un apagado completo.

# v2.2 / v2.3 (Claude, 16-sep-2026) sobre el render 3D de Codex
- Guardado nativo: START > PARTIDA > Guardar abre el dialogo de la Memory Stick (4 ranuras,
  ms0:/PSP/SAVEDATA/NARC00001000x, icono de portada, titulo "Mision xx/36 - ..."). CONTINUAR en el
  titulo abre el dialogo de carga si hay ranuras; si no, sigue el flujo antiguo (PROGRESS.BIN).
  Implementado en psp_main.c (sceUtilitySavedata) + game_export_save/import en game.c; -lpsputility.
  Durante el dialogo NO se alternan buffers (el sistema dibuja sobre el visible). Nunca IMMEDIATE.
- Personaje: person() en render3d.c con proporciones humanas (hombros 8.4, cuello, codos, piernas).
- Portada ICON0 (assets/icon-source.png -> 144x80) y musica de la XMB SND0.AT3 (tools/make_snd0.py:
  ffmpeg recorta 0:11-0:31 de assets/xmb-music-source.mp3, atracdenc -> ATRAC3, reempaquetado RIFF).
- HUD minimo (game.c hud()): sin barra superior ni franja inferior. Vida/dinero arriba a la derecha;
  barrio arriba 2 s al cambiar; frase de objetivo abajo 5 s al empezar cada objetivo (se relee en
  START > cuaderno); minimapa circular abajo a la izquierda; chincheta "!" del objetivo en el mapa.
- Pendiente para Codex: el jugador aparece dentro de la caja del terminal del refugio y atraviesa
  coches (colision 2D vs escena 3D).
- SND0.AT3 (musica XMB): pendiente. La XMB real reproduce SND0 de Sony (ATRAC3plus) en el EBOOT de Narcade,
  pero rechaza todo lo generado con atracdenc (ver tools/make_snd0.py). Requiere at3tool.exe (Sony).

# v2.4 (Claude, 17-sep-2026): optimizacion de rendimiento del render 3D
Medido con overlay de perfil (NARCADE_PROFILE=1 python tools/build_windows.py) en PPSSPP, que emula los ciclos MIPS:
- Version de Codex del 17-sep: 138-156 ms/fotograma (6-7 fps). Optimizada: 19-22 ms (45-50 fps).
Tecnicas (render3d.c, world_geo.h):
1. Frustum culling por esfera envolvente: manzanas, coches y peatones fuera de la camara no generan geometria.
2. LOD por distancia (>430): sin marcas viales, pasos de cebra, farolas, murales ni arboles; edificios en un solo bloque.
   Coches a >300: sin ruedas.
3. Transformacion geografica rigida cacheada por objeto (antes geo_project+geo_heading con atan2 por VERTICE).
4. local(): seno/coseno cacheados por angulo. player_pose(): constantes de la marcha una vez por fotograma.
5. polygon(): aceptacion/rechazo trivial por plano; solo se recorta contra los planos que se cruzan.
6. Ruta rapida para los 2.344 triangulos del jugador (sin polygon(): sin recorte ni copias).
7. Tabla de alturas del terreno (rejilla de 80) en vez de 4 evaluaciones de geo_height_raw por consulta.
- Animacion procedural del jugador (player_pose): balanceo de brazos con codo (mas amplio al correr), contragiro
  hombros/cadera, arco del pie, inclinacion al correr. Sin coste medible.
- SND0.AT3 (musica XMB): causa real de los fallos del 16-sep encontrada en github.com/TotalKommando/psp-media-toolkit
  (FINDINGS.md, verificado en hardware): el chunk fact no puede declarar mas muestras de las que decodifican los
  frames (samples + delay + 368 <= frames*1024). tools/make_snd0.py ya aplica la regla; ATRAC3 LP4 66 kbps de
  atracdenc deberia sonar. PENDIENTE de confirmar en la E-1000.

## v2.5 — 17 septiembre 2026 (Codex)

- Terreno por terrazas: calles horizontales y cruces nivelados; rampas verticales
  continuas de pendiente inferior al 12%. Relieve artístico, no levantamiento real.
- Transformación ortonormal de los coches: inclinación sin deformar carrocería.
- Contactos: conservar velocidad tangencial y marcha atrás, escape junto a paredes
  y breve cesión del tráfico. Sin detener ambos vehículos por cualquier roce.
- Cámara amortiguada en coordenadas proyectadas, gesto de joystick estable y
  recuperación gradual de distancia tras obstáculos. L/R siguen siendo radio.
- Corregido descarte de suelo cercano: planos normalizados y culling conservador
  de manzanas. Regresión de cobertura de suelo con 512 posiciones/orientaciones.
- Modelo v3 y animación de Claude conservados; suelas y cordones añadidos.
  Exportación actual: 2.692 triángulos, OBJ/MTL y GIF de las poses reales del motor.
- Entrega: release/Narcade_v2.5_PSP.zip, ISO y PBP. Detalles en VALIDACION_v2.5.md.
  Probado en PPSSPP y pruebas nativas; pendiente de probar esta versión en PSP real.

# v2.5 (Claude, 17-sep-2026 noche)
- Ciudad irregular (src/city3d.inc): 7 tipos de manzana por barrio (comuna, colonial, torre, comercial en L, iglesia,
  mercado, clasico), aceras de ancho variable, marcas viales distintas, puentes con barandillas/arcos/farolas sobre el
  rio, pasarelas cubiertas entre manzanas, zonas verdes (arbustos, flores, setos, fuentes), semaforos, paradas de bus,
  toldos, balcones, azoteas. La red de calles y solid() NO cambian. LOD: 'far' (>430) y 'cityMid' (>260).
- Dia/noche (src/daylight.inc): dia de 8 min (DAY_SECONDS), sol con azimut/elevacion, iluminacion por cara en box()
  y ground() (lit_color), cielo/niebla por hora, nubes, sol/luna, sombras proyectadas (jugador, coches, peatones),
  farolas y ventanas encendidas de noche, faros de los coches con cono de luz. Pase aditivo tras la geometria.
- Escala humana: PERSON_SCALE 0.62 (jugador y NPCs); velocidades a pie 42/68/100 (antes 72/111/150).
- Control: direccion siempre relativa a la camara actual (sin anclaje); camara con seguimiento suave (omega 4.5).
- Coste: ~28-34 ms/fotograma en PPSSPP (30 fps) con todo lo anterior.
