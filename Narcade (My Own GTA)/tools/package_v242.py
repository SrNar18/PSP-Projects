"""Package the combined PSP ISO/PBP and reproducible review material."""
from pathlib import Path
import hashlib
import shutil
import zipfile

root = Path(__file__).resolve().parents[1]
release = root / 'release/Narcade_v2.42'
release.mkdir(parents=True, exist_ok=True)
iso = release / 'Narcade_v2.42.iso'
shutil.copy2(root / 'Narcade.iso', iso)
for source in (
    'AVISOS.txt',
    'NOTAS_CODEX_AJUSTES_IDIOMAS.md',
    'REVISION_CODEX_RAMA_CLAUDE_2026-09-28_PULIDO.md',
    'build/settings-preview.png',
):
    shutil.copy2(root / source, release / Path(source).name)

pbp = root / 'EBOOT.PBP'
(release / 'LEEME.txt').write_text(
    'Narcade v2.42 - made by Naresz.\n'
    'Incluye las ramas de Claude (rendimiento, carga, policia, objetos de carretera) '
    'y Codex (ajustes persistentes, ingles completo y correcciones de coches/farolas).\n'
    'Copia la ISO a ms0:/ISO/ en PSP con CFW o extrae PSP/GAME/NARCADE/ del ZIP.\n'
    'Preferencias: idioma, brillo, musica, efectos y niebla opcional. La imagen es una captura de interfaz en PC, no PSP.\n'
    'Pruebas host y compilacion PSP aprobadas; la prueba en consola fisica corresponde al jugador.\n',
    encoding='utf-8',
)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

(release / 'SHA256.txt').write_text(
    f'{sha(iso)}  {iso.name}\n{sha(pbp)}  PSP/GAME/NARCADE/EBOOT.PBP\n'
)
archive = root / 'release/Narcade_v2.42_PSP.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    z.write(iso, f'ISO/{iso.name}')
    z.write(pbp, 'PSP/GAME/NARCADE/EBOOT.PBP')
    for k, name in enumerate(('valley', 'street', 'rooftop')):  # ilustraciones de carga junto al EBOOT (Claude v2.42)
        z.write(root / f'assets/loading-v241/{name}.rgb565', f'PSP/GAME/NARCADE/LOAD{k}.BIN')
    for file in sorted(release.iterdir()):
        if file != iso:
            z.write(file, f'{release.name}/{file.name}')
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    assert z.read(f'ISO/{iso.name}') == iso.read_bytes()
    assert z.read('PSP/GAME/NARCADE/EBOOT.PBP') == pbp.read_bytes()
    for k, name in enumerate(('valley', 'street', 'rooftop')):
        assert z.read(f'PSP/GAME/NARCADE/LOAD{k}.BIN') == (root / f'assets/loading-v241/{name}.rgb565').read_bytes()
print(f'{archive} ({archive.stat().st_size} bytes); ISO/PBP/ZIP verified')
