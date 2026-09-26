# Compilar Narcade

El paquete incluye los recursos ya generados. No requiere un motor comercial.

1. Instala [PSPDEV](https://pspdev.github.io/installation.html), `make` y Python 3.
2. Añade `$PSPDEV/bin` al `PATH` según las instrucciones de PSPDEV.
3. En esta carpeta ejecuta `make`. Produce `narcade.prx` y `EBOOT.PBP`.
4. Instala `pycdlib` con `python3 -m pip install pycdlib`.
5. Ejecuta `python3 tools/package_iso.py`. Produce `Narcade.iso` y verifica que
   el ejecutable incorporado sea idéntico al PRX compilado.

La compilación de entrega usó PSPDEV v20260901 y GCC 15.2.0. El objetivo es
PSP de 32 MB; no se activa la extensión de memoria de los modelos posteriores.
El framebuffer es de 480 × 272, doble buffer, color de 32 bits. El programa
solicita 333 MHz y usa audio PCM mono de 22.050 Hz duplicado a estéreo/44.100 Hz.
Las melodías están integradas en el ejecutable y funcionan desde una ISO.

`tools/story.py` es la fuente editable de las 36 misiones. Genera `src/story.h`
y `CAMPANA.json`. Para regenerar los recursos opcionalmente, instala `Pillow`
y `numpy` y ejecuta `tools/assets.py`; necesita las fuentes DejaVu indicadas
en el script y la copia de Allura incluida. Los archivos de `assets/` ya
permiten compilar sin repetir ese paso.

Pruebas del motor, en Linux con GCC:

```sh
mkdir -p build
gcc -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I src tools/qa.c src/assets.S -lm -o build/qa
ASAN_OPTIONS=detect_leaks=0 ./build/qa
```

El test recorre los 146 objetivos y resuelve los minijuegos mediante sus
eventos de entrada. Para cubrir la campaña sin reproducir cada trayecto,
coloca al jugador en los destinos. Comprueba también adquisición y conducción
real de un carro, guardado, recuperación de corrupción, audio, encargos y
12.000 cuadros de entradas aleatorias. No sustituye una partida manual ni
pruebas físicas de PSP. LeakSanitizer se desactivó porque el entorno de
ejecución no permite su inspección de procesos; AddressSanitizer y UBSan
permanecieron activos.

`tools/emulator_qa.py` documenta la sesión de captura con PPSSPP y Xvfb. Sus
rutas de herramientas son propias del entorno de creación y deben adaptarse
para ejecutarlo en otro computador. No se necesita ese script para jugar.
