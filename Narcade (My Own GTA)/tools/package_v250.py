"""Package Narcade v2.50 for PSP, including the validated ISO and EBOOT."""
from pathlib import Path
import hashlib
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
release = root / 'release/Narcade_v2.50'
release.mkdir(parents=True, exist_ok=True)
iso = release / 'Narcade_v2.50.iso'
shutil.copy2(root / 'Narcade.iso', iso)
for source in ('AVISOS.txt', 'NOTAS_CODEX_V250_PARA_CLAUDE.md'):
    shutil.copy2(root / source, release / source)
(release / 'LEEME.txt').write_text(
    'Narcade v2.50 - made by Naresz.\n'
    'Logo original de la XMB restaurado, mapa completo de Start e iluminacion mejorada.\n'
    'ISO: copia ISO/Narcade_v2.50.iso a la carpeta ISO de tu PSP con CFW.\n'
    'PBP: extrae PSP/GAME/NARCADE/ completo; LOAD0..2.BIN deben quedar junto a EBOOT.PBP.\n'
    'Se conserva el identificador del juego y las partidas existentes.\n'
    'Compilacion y pruebas automatizadas aprobadas; verificar en PSP fisica.\n',
    encoding='utf-8')
pbp = root / 'EBOOT.PBP'
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
(release / 'SHA256.txt').write_text(
    f'{sha(iso)}  {iso.name}\n{sha(pbp)}  PSP/GAME/NARCADE/EBOOT.PBP\n', encoding='ascii')
archive = root / 'release/Narcade_v2.50_PSP.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    z.write(iso, f'ISO/{iso.name}')
    z.write(pbp, 'PSP/GAME/NARCADE/EBOOT.PBP')
    for k, name in enumerate(('valley', 'street', 'rooftop')):
        z.write(root / f'assets/loading-v241/{name}.rgb565', f'PSP/GAME/NARCADE/LOAD{k}.BIN')
    for file in sorted(release.iterdir()):
        if file != iso:
            z.write(file, f'{release.name}/{file.name}')
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert z.read(f'ISO/{iso.name}') == iso.read_bytes()
    assert z.read('PSP/GAME/NARCADE/EBOOT.PBP') == pbp.read_bytes()
print(archive, archive.stat().st_size, 'bytes; ISO/PBP/loading assets/ZIP verified')



