"""Genera assets/SND0.AT3 (musica de la XMB al seleccionar el juego) a partir de un audio.

Uso: python tools/make_snd0.py <audio> <inicio_s> <fin_s> [--atracdenc RUTA] [--ffmpeg RUTA]

- Recorta con ffmpeg (fade in/out), 44.1 kHz estereo 16 bit.
- Codifica ATRAC3 LP4 66 kbps (frames de 192 bytes, el estandar de los SND0.AT3) con atracdenc, que escribe contenedor OMA.
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

def rm_frames(rm: bytes):
    """Extrae los frames ATRAC3 de un .rm de atracdenc (interleaver 'genr', sub_packet_h=1: frames consecutivos)."""
    assert rm[:4] == b'.RMF', 'no es RealMedia'
    i = rm.find(b'.ra' + bytes([0xFD])); assert i > 0, 'sin cabecera .ra'
    frame = struct.unpack('>H', rm[i + 44:i + 46])[0]  # sub_packet_size = bytes por frame ATRAC3
    pos = 0; frames = []
    while pos < len(rm) - 10:
        cid = rm[pos:pos + 4]; sz = struct.unpack('>I', rm[pos + 4:pos + 8])[0]
        if cid == b'DATA':
            npk = struct.unpack('>I', rm[pos + 10:pos + 14])[0]; q = pos + 18
            for _ in range(npk):
                if q + 12 > len(rm): break
                ln = struct.unpack('>H', rm[q + 2:q + 4])[0]
                if ln < 12 or q + ln > len(rm): break
                payload = rm[q + 12:q + ln]
                for k in range(0, len(payload) - frame + 1, frame): frames.append(payload[k:k + frame])
                q += ln
            break
        pos += sz
    data = bytearray(b''.join(frames))
    # Los frames ATRAC3 en RealMedia van XOR-scrambled con 0x537F6103 (big-endian, por palabra de 32 bits);
    # en RIFF/WAVE (y en la PSP) se almacenan sin ese XOR.
    key = bytes.fromhex('537F6103')
    for i in range(0, len(data) - 3, 4):
        data[i] ^= key[0]; data[i + 1] ^= key[1]; data[i + 2] ^= key[2]; data[i + 3] ^= key[3]
    return frame, bytes(data)

def frames_to_at3(frame: int, data: bytes, joint: int) -> bytes:
    srate = 44100; nframes = len(data) // frame; samples = nframes * 1024
    fmt = struct.pack('<HHIIHHH', 0x0270, 2, srate, frame * srate // 1024, frame, 0, 14)
    # extradata canonica (validada por el parser del firmware): 1, 0x1000, modo, modo(igual), 1
    fmt += struct.pack('<HIHHI', 1, 0x1000, joint, joint, 1)
    delay = 0x400  # retardo del codificador ATRAC3 (primer sample util)
    fact = struct.pack('<II', samples, delay)
    # chunk smpl con un bucle sobre todo el archivo, como los SND0.AT3 de Sony (at3tool -wholeloop)
    smpl = struct.pack('<9I', 0, 0, 22676, 60, 0, 0, 0, 1, 24) + struct.pack('<6I', 0, 0, delay, samples - 1, 0, 0)
    body = (b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'fact' + struct.pack('<I', len(fact)) + fact
            + b'smpl' + struct.pack('<I', len(smpl)) + smpl + b'data' + struct.pack('<I', len(data)) + data)
    return b'RIFF' + struct.pack('<I', len(body)) + body

def oma_plus_to_at3(oma: bytes) -> bytes:
    """OMA ATRAC3plus -> RIFF/WAVE extensible (0xFFFE + GUID), el formato de los SND0.AT3 oficiales."""
    assert oma[:4] == b'EA3'; hdr = struct.unpack('>H', oma[4:6])[0]
    assert oma[32] == 1, 'no es ATRAC3plus'
    params = oma[33:36]; frame = ((int.from_bytes(params, 'big') & 0x3FF) * 8) + 8
    data = oma[hdr:]; data = data[:len(data) // frame * frame]; nframes = len(data) // frame; samples = nframes * 2048
    srate = 44100; chans = 2
    fmt = struct.pack('<HHIIHHH', 0xFFFE, chans, srate, frame * srate // 2048, frame, 0, 34)
    fmt += struct.pack('<HI', 0x0800, 3) + bytes.fromhex('BFAA23E958CB7144A119FFFA01E4CE62') + struct.pack('<H', 1) + params[1:3] + bytes(8)
    delay = 0x866
    fact = struct.pack('<II', samples, delay)
    smpl = struct.pack('<9I', 0, 0, 22676, 60, 0, 0, 0, 1, 24) + struct.pack('<6I', 0, 0, delay, samples - 1, 0, 0)
    body = (b'WAVE' + b'fmt ' + struct.pack('<I', len(fmt)) + fmt + b'fact' + struct.pack('<I', len(fact)) + fact
            + b'smpl' + struct.pack('<I', len(smpl)) + smpl + b'data' + struct.pack('<I', len(data)) + data)
    return b'RIFF' + struct.pack('<I', len(body)) + body

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
    ap.add_argument('--ffmpeg'); ap.add_argument('--codec', default='atrac3', help='atrac3 = LP2 132 kbps (este atracdenc.exe no soporta LP4)'); ap.add_argument('--lp2', dest='lp4', action='store_false', help='usar 132 kbps (LP2) en vez de 66 kbps (LP4)'); ap.add_argument('--out', default=str(ROOT / 'assets/SND0.AT3'))
    a = ap.parse_args()
    ff = find_ffmpeg(a.ffmpeg); dur = a.end - a.start
    with tempfile.TemporaryDirectory() as td:
        wav = pathlib.Path(td) / 'clip.wav'; oma = pathlib.Path(td) / 'clip.oma'
        subprocess.run([ff, '-hide_banner', '-loglevel', 'error', '-y', '-ss', str(a.start), '-to', str(a.end), '-i', a.audio,
                        '-ac', '2', '-ar', '44100', '-sample_fmt', 's16',
                        '-af', f'afade=t=in:st=0:d=0.5,afade=t=out:st={dur - 0.7:.2f}:d=0.7', str(wav)], check=True)
        if a.codec == 'atrac3plus':
            subprocess.run([a.atracdenc, '-e', 'atrac3plus', '-i', str(wav), '-o', str(oma)], check=True, stdout=subprocess.DEVNULL)
            at3 = oma_plus_to_at3(oma.read_bytes())
        elif a.lp4:
            # ATRAC3 LP4 66 kbps (frames de 192 bytes, joint stereo): el formato clasico de SND0.AT3.
            # atracdenc solo lo permite via contenedor RealMedia; '--bitrate 64' produce frames de 192 bytes.
            rm = pathlib.Path(td) / 'clip.rm'
            subprocess.run([a.atracdenc, '-e', 'atrac3', '--bitrate', '64', '-i', str(wav), '-o', str(rm)], check=True, stdout=subprocess.DEVNULL)
            frame, data = rm_frames(rm.read_bytes()); assert frame == 192, frame
            at3 = frames_to_at3(frame, data, 1)
        else:
            subprocess.run([a.atracdenc, '-e', a.codec, '-i', str(wav), '-o', str(oma)], check=True, stdout=subprocess.DEVNULL)
            at3 = oma_to_at3(oma.read_bytes())
    pathlib.Path(a.out).write_bytes(at3)
    print(f'{a.out}: {len(at3)} bytes, {dur:.1f} s, ' + ('ATRAC3plus (tasa fija de atracdenc)' if a.codec == 'atrac3plus' else 'ATRAC3 ' + ('LP4 66 kbps' if a.lp4 else 'LP2 132 kbps')) + ' 44.1 kHz')

if __name__ == '__main__':
    main()
