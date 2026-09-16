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
