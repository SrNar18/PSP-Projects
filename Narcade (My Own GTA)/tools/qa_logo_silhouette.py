"""Guard the XMB logo's tapered N against an opaque sky cutout."""
from pathlib import Path
from PIL import Image
import struct

root = Path(__file__).resolve().parents[1]
image = Image.open(root / 'assets/narcade-logo-small-v244.png').convert('RGBA')
assert image.size == (115, 39)
assert image.getpixel((0, 30))[3] < 16
assert image.getpixel((5, 28))[3] < 16
assert image.getpixel((10, 25))[2] > 190  # blue painted diagonal survives
raw = (root / 'assets/narcade-logo-small-v244.4444').read_bytes()
assert len(raw) == 115 * 39 * 2
for i, (r, g, b, a) in enumerate(image.get_flattened_data()):
    encoded, = struct.unpack_from('<H', raw, i * 2)
    assert encoded == ((r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8) | ((a >> 4) << 12))
print('PASS: transparent N tip, painted diagonal, and PSP RGBA4444 match.')
