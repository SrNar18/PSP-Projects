"""Package the v2.5 build in the workspace, leaving previous releases alone."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
release = root / 'release' / 'Narcade_v2.5'
files = {
    'ISO/Narcade.iso': root / 'Narcade.iso',
    'PSP/GAME/NARCADE/EBOOT.PBP': root / 'EBOOT.PBP',
    'LEEME.txt': root / 'LEEME_v2.5.txt',
    'AVISOS.txt': root / 'AVISOS.txt',
    'VALIDACION_v2.5.md': root / 'VALIDACION_v2.5.md',
    'QA-v2.5.txt': root / 'build/QA-v2.5.txt',
    'Vista/psp-v2.5-world.png': root / 'assets/psp-v2.5-world.png',
    'Vista/nico-streetwear-preview.png': root / 'assets/nico-streetwear-preview.png',
    'Vista/nico-walk.gif': root / 'assets/nico-walk.gif',
    'Vista/nico-run.gif': root / 'assets/nico-run.gif',
}
for name, source in files.items():
    assert source.is_file(), source
    dest = release / name
    dest.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, dest)
checks = {name: hashlib.sha256(source.read_bytes()).hexdigest()
          for name, source in files.items()}
manifest = release / 'SHA256.json'
manifest.write_text(json.dumps(checks, indent=2) + '\n', encoding='utf-8')
out = release.parent / 'Narcade_v2.5_PSP.zip'
with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=8) as z:
    for name in files:
        z.write(release / name, 'Narcade_v2.5/' + name)
    z.write(manifest, 'Narcade_v2.5/SHA256.json')
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    for name, digest in checks.items():
        assert hashlib.sha256(z.read('Narcade_v2.5/' + name)).hexdigest() == digest
print(out)
print(f'{out.stat().st_size} bytes; all archived files verified against SHA-256')
