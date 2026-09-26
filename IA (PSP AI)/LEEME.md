# PSP-IA — asistente de conocimiento offline para PSP

Homebrew para PSP (CFW PRO 6.60, probado para E-1000) que responde preguntas **sin internet** usando la Wikipedia en
español guardada en la Memory Stick, y que además incluye un **modelo de lenguaje diminuto** (≈5 M parámetros,
entrenado en el PC del usuario) que genera texto en la propia PSP como modo "charla" experimental.

## Instalación en la PSP
1. Copia la carpeta `out/IA` a la raíz de la Memory Stick: `ms0:/IA/` (ia.txt, ia.doc, ia.idx, ia.pst, ia.hdr, lm.bin).
2. ISO: copia `out/PSP_IA.iso` a `ms0:/ISO/` (modo UMD ISO: Inferno). O EBOOT: `out/EBOOT.PBP` a `ms0:/PSP/GAME/PSPIA/`.

## Uso
- **X**: teclado de la PSP para escribir la pregunta ("quien fue Simon Bolivar", "capital de Australia").
- **L/R**: otro resultado. **Arriba/Abajo**: leer el artículo. **O**: borrar. **SELECT**: modo charla.
- Modo charla: escribe el comienzo de una frase ("Medellin es") y el modelo la continúa. Inventa: es un juguete.

## Cómo funciona
- `tools/build_index.py`: lee los parquet de `wikimedia/wikipedia` (20231101.es, 3,3 GB), se queda con título +
  primeros ~1400 caracteres de cada artículo (zlib), y construye un índice invertido: hash FNV-1a de cada palabra
  normalizada (título peso 3, primeras 90 palabras peso 1, sin stopwords) → postings (delta varint + peso).
- `src/search.c`: en la PSP todo va por seeks a disco (la consola tiene 64 MB): búsqueda binaria del término en
  `ia.idx`, lectura de postings, acumulación de puntuaciones idf·peso en una tabla hash de 256k entradas, top-8,
  descompresión del artículo y elección de la frase que más palabras comparte con la pregunta.
- `tools/train_lm.py`: GPT a nivel de byte (6 capas, d=256, 8 cabezas, contexto 192) entrenado con PyTorch en la
  RTX 3060 sobre 300 MB de resúmenes; exportado a int8 con escala por fila (`lm.bin`, ~5 MB).
- `src/chat.c`: inferencia en C con caché K/V, muestreo top-p; ~10 MFLOP por byte generado.
- `src/main.c`: interfaz (framebuffer 8888 por CPU, fuente de mapa de bits generada por `tools/make_font.py`),
  teclado `sceUtilityOsk`. Los diálogos del sistema dibujan en el buffer de dibujo del GE: se apunta al visible.

## Compilar
`venv/Scripts/python tools/build.py` (usa el toolchain pspdev de `Narcade_v1.1_fuente/build/pspdev`).
Prueba automática de consulta: `PSPIA_TEST="quien fue Andres Bello" venv/Scripts/python tools/build.py`.

## Licencias
Texto de Wikipedia: CC BY-SA 4.0 (autores de Wikipedia). Código: del autor del proyecto.
