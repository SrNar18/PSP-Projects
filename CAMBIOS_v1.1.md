# Narcade v1.1 — cambios para PSP E-1000 (Street) real

Hechos por Claude tras probar en la consola fisica con CFW PRO 6.60.

## Correcciones de hardware
1. **Parpadeo** (`src/psp_main.c`): se usaba `sceDisplaySetFrameBuf(..., NEXTFRAME)` tras
   `sceDisplayWaitVblankStart()` y se empezaba a redibujar el buffer que aun se mostraba.
   Ahora: dibujar -> volcar -> esperar vblank -> `SETBUF_IMMEDIATE`.
2. **Rendimiento** (`src/psp_main.c`, `src/game.c`): el juego escribia pixel a pixel en VRAM
   sin cache (muy lento en hardware; PPSSPP no lo penaliza). Ahora se dibuja en un backbuffer
   en RAM con cache y se copia a VRAM con `sceDmacMemcpy` (fallback memcpy). Se anadio `px()`
   para trazar pixeles sin pasar por `rect()` con recorte (coches, lineas, texto), y `rect()`
   recorre por filas. Requiere `-lpspdmac` (Makefile).
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
