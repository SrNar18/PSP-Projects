# PSP-IA — an offline AI assistant for the PSP

**PSP-IA** is a homebrew app for the Sony PSP that answers questions **without internet**.
It combines two things:

1. **Offline search over the whole Spanish Wikipedia** (about 1.7 million articles) stored
   on the Memory Stick, with a compact inverted index that the PSP queries from disk.
2. **A tiny language model** (~5 million parameters, byte-level GPT) that runs on the
   PSP itself for casual conversation ("hola" → "¡Hola! ¿Cómo estás? ¿En qué te puedo ayudar?"),
   with a typewriter-style text animation.

Tested on a **PSP E-1000 with CFW PRO 6.60**. The interface and data are in Spanish.

## Installing on the PSP

1. Build the data files (see below) and copy the `IA` data folder to the root of the
   Memory Stick: `ms0:/IA/` (index, article volumes, title table and `lm.bin`).
2. Copy `PSP_IA.iso` to `ms0:/ISO/` (Inferno ISO driver), **or** `EBOOT.PBP` to
   `ms0:/PSP/GAME/PSPIA/`.

The data files are **not** in this repository: the full Wikipedia build is tens of GB.

## Using it

- **X**: open the PSP keyboard and type a question ("quién fue Simón Bolívar").
- **L / R**: previous / next result · **Up / Down**: scroll · **Triangle**: full article.
- Greetings and small talk are routed to the language model; questions go to Wikipedia.

## How it works

| File | What it does |
|---|---|
| `tools/build_index.py`, `tools/build_index_full.py` | Read the `wikimedia/wikipedia` dump, compress articles (zlib) into volumes under 3.9 GB (FAT32 limit) and build an inverted index (FNV-1a word hashes → delta-varint postings). |
| `tools/make_titles.py` | Builds the title table used to re-rank results. |
| `tools/make_dialog.py`, `tools/train_lm.py` | Build a Spanish dialogue corpus and train the byte-level GPT (6 layers, d=256, 8 heads, context 192) with PyTorch; exported as int8 with per-row scales (`lm.bin`). |
| `tools/make_font.py` | Generates the bitmap font (`src/font.h`). |
| `tools/build.py` | Compiles the app with the PSPDEV toolchain from `../Narcade/build/pspdev` and packages `EBOOT.PBP` + `PSP_IA.iso`. |
| `tools/query.py` | Tests searches on the PC against the built index. |
| `src/search.c` | On-PSP search: binary search in the index, score accumulation, answer-sentence selection. |
| `src/chat.c` | Language-model inference in C with a K/V cache and top-p sampling. |
| `src/main.c` | User interface, Sony on-screen keyboard (`sceUtilityOsk`), chat/search router. |
| `LEEME.md` | Original notes (Spanish). |

## Licenses

Wikipedia text: CC BY-SA 4.0 (Wikipedia authors). Code: by the project author.
