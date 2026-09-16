"""Genera assets/SND0.AT3 (musica de la XMB al seleccionar el juego) a partir de un audio.

Uso: python tools/make_snd0.py <audio> <inicio_s> <fin_s> [--atracdenc RUTA] [--ffmpeg RUTA]

- Recorta con ffmpeg (fade in/out), 44.1 kHz estereo 16 bit.
- Codifica ATRAC3 LP2 132 kbps con atracdenc, que escribe contenedor OMA.
- Reempaqueta los frames en RIFF/WAVE con fmt 0x0270 (formato que lee la XMB de la PSP).
"""
import argparse, pathlib, shutil, struct, subprocess, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]

def find_ffmpeg(explicit):
    if explicit: return explicit
    if shutil.which('ffmpeg'): return 'ffmpeg'
    try:
        import imageio_ffmpeg; return imageio_ffmpeg.get_ffmpeg_exe()
    except ImportError:
        sys.exit('ffmpeg no encontrado: instala con  python -m pip install imageio-ffmpeg')

def oma_to_at3(oma: bytes) -> bytes:
    assert oma[:4] == b'EA3\x01', 'no es OMA'
    hdr = struct.unpack('>H', oma[4:6])[0]
    codec = oma[32]; params = int.from_bytes(oma[33:36], 'big')
    assert codec == 0, 'solo ATRAC3 (LP2/LP4)'
    joint = (params >> 17) & 1
    srate = [320, 441, 480, 882, 960][(params >> 13) & 7] * 100
    frame = (params & 0x3FF) * 8
    data = oma[hdr:]
    data = data[:len(data) // frame * frame]
    nframes = len(data) // frame
    samples = nframes * 1024
    fmt = struct.pack('<HHIIHHH', 0x0270, 2, srate, frame * srate // 1024, frame, 0, 14)
    # extradata canonica de at3tool: version 1, muestras/canal 0x1000, coding mode (x2), frame factor 1, 0
    fmt += struct.pack('<HIHHHH', 1, 0x1000, joint, joint, 1, 0)
    fact = struct.pack('<II', samples, 0)
    body = b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'fact' + struct.pack('<I', len(fact)) + fact + b'data' + struct.pack('<I', len(data)) + data
    return b'RIFF' + struct.pack('<I', len(body)) + body

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('audio'); ap.add_argument('start', type=float); ap.add_argument('end', type=float)
    ap.add_argument('--atracdenc', default=str(ROOT / 'build/atracdenc/atracdenc.exe'))
    ap.add_argument('--ffmpeg'); ap.add_argument('--codec', default='atrac3', help='atrac3 = LP2 132 kbps (este atracdenc.exe no soporta LP4)'); ap.add_argument('--out', default=str(ROOT / 'assets/SND0.AT3'))
    a = ap.parse_args()
    ff = find_ffmpeg(a.ffmpeg); dur = a.end - a.start
    with tempfile.TemporaryDirectory() as td:
        wav = pathlib.Path(td) / 'clip.wav'; oma = pathlib.Path(td) / 'clip.oma'
        subprocess.run([ff, '-hide_banner', '-loglevel', 'error', '-y', '-ss', str(a.start), '-to', str(a.end), '-i', a.audio,
                        '-ac', '2', '-ar', '44100', '-sample_fmt', 's16',
                        '-af', f'afade=t=in:st=0:d=0.5,afade=t=out:st={dur - 0.7:.2f}:d=0.7', str(wav)], check=True)
        subprocess.run([a.atracdenc, '-e', a.codec, '-i', str(wav), '-o', str(oma)], check=True, stdout=subprocess.DEVNULL)
        at3 = oma_to_at3(oma.read_bytes())
    pathlib.Path(a.out).write_bytes(at3)
    print(f'{a.out}: {len(at3)} bytes, {dur:.1f} s, ATRAC3 ({a.codec}) 44.1 kHz')

if __name__ == '__main__':
    main()
