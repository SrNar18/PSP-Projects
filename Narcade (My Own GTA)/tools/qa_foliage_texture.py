"""Verify the actual packed PSP alpha tile, not just its PNG preview."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parents[1]
blob=(root/'assets/textures3d.bin').read_bytes()
assert len(blob)==884736, 'Texture layout/size changed'
tile=blob[753664+8192:753664+2*8192]
alpha=[v>>12 for (v,) in struct.iter_unpack('<H',tile)]
holes=sum(a<8 for a in alpha)/len(alpha)
assert .25<holes<.80, f'Foliage coverage incorrect: holes={holes}'
assert min(alpha)==0 and max(alpha)==15
print(f'PASS: actual 8KB RGBA4444 foliage tile has {holes:.1%} alpha-test gaps, unchanged texture budget.')
