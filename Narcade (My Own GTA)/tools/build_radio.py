"""v2.52 (Claude): emisoras de radio de los carros a partir de las listas de canciones.

Cada carpeta NN_Genero de la fuente (por defecto ~/Downloads/Narcade_Radio) es una emisora.
Salida: assets/radio/RADIO<n>.BIN, una por emisora, en el orden de las carpetas.

Formato (little endian):
  0  'NRD1'   4  u32 canciones   8  u32 frecuencia (22050)   12  u32 0   16  char genero[16]
  32 canciones x 48 bytes: u32 primer bloque, u32 bloques, char titulo[40]
  4096 bloques IMA ADPCM de 1028 bytes (2048 muestras mono), ver tools/radio_adpcm.c.
Los archivos no se suben a git (pesan ~2 MB por cancion); package_iso.py los copia si existen.
"""
import os, pathlib, struct, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path.home() / 'Downloads' / 'Narcade_Radio'
OUT = ROOT / 'assets' / 'radio'
RATE, BLOCK, BLOCK_BYTES, DATA = 22050, 2048, 4 + 1024, 4096


def ffmpeg():
    try:
        import imageio_ffmpeg
        return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        return 'ffmpeg'


def encoder():
    exe = ROOT / 'build' / ('radio_adpcm.exe' if os.name == 'nt' else 'radio_adpcm')
    if not exe.exists():
        exe.parent.mkdir(exist_ok=True)
        subprocess.check_call(['gcc', '-O2', '-o', str(exe), str(ROOT / 'tools' / 'radio_adpcm.c')])
    return str(exe)


def encode(mp3):
    pcm = subprocess.run([ffmpeg(), '-v', 'error', '-i', str(mp3), '-ac', '1', '-ar', str(RATE),
                          '-af', 'loudnorm=I=-16:TP=-1.5:LRA=11', '-f', 's16le', '-'],
                         check=True, stdout=subprocess.PIPE).stdout
    return subprocess.run([encoder()], input=pcm, check=True, stdout=subprocess.PIPE).stdout


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    genres = sorted(d for d in SRC.iterdir() if d.is_dir() and d.name[:2].isdigit())
    for n, folder in enumerate(genres):
        songs = sorted(folder.glob('*.mp3'))[:16]
        table, blob, first = [], bytearray(), 0
        for song in songs:
            data = encode(song)
            blocks = len(data) // BLOCK_BYTES
            title = song.stem.replace('_', ' ').encode('ascii', 'replace')[:39]
            table.append(struct.pack('<II40s', first, blocks, title))
            blob += data
            first += blocks
            print(f'  {folder.name}: {song.stem} {blocks * BLOCK / RATE:.0f} s')
        genre = folder.name[3:].encode('ascii', 'replace')[:15]
        head = struct.pack('<4sIII16s', b'NRD1', len(songs), RATE, 0, genre) + b''.join(table)
        out = OUT / f'RADIO{n}.BIN'
        out.write_bytes(head.ljust(DATA, b'\0') + blob)
        print(f'{out.name}: {folder.name}, {len(songs)} canciones, {out.stat().st_size / 1e6:.1f} MB')


if __name__ == '__main__':
    main()
