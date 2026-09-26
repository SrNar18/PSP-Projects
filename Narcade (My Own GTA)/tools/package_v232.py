"""Package the verified PSP build, keeping previous releases intact."""
from pathlib import Path
import hashlib, shutil, zipfile
root=Path(__file__).resolve().parents[1]
dest=root/'release/Narcade_v2.32'
(dest/'PSP/GAME/NARCADE').mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'Narcade.iso',dest/'Narcade_v2.32.iso')
shutil.copy2(root/'EBOOT.PBP',dest/'PSP/GAME/NARCADE/EBOOT.PBP')
shutil.copy2(root/'assets/city-v232-preview.png',dest/'city-v232-preview.png')
shutil.copy2(root/'NOTAS_CLAUDE_PARA_CODEX.md',dest/'NOTAS_CLAUDE_PARA_CODEX.md')
(dest/'LEEME.txt').write_text('''Narcade 3D 2.32 - made by Naresz
ISO: copia Narcade_v2.32.iso a ISO/ en una PSP con CFW compatible.
EBOOT: copia PSP/GAME/NARCADE a PSP/GAME/ como alternativa a la ISO.
El guardado y la solucion del movimiento de Claude se conservan.
Mejoras: fachadas adosadas, balcones, parking ajardinado, rio continuo
con meandro y flujo animado; apoyos de jardines, tanques y mobiliario.
city-v232-preview.png es un render de inspeccion en PC de la malla real,
no una captura de la PSP. Falta verificar la apariencia en hardware.
''',encoding='utf-8')
checks=[]
for p in sorted(dest.rglob('*')):
    if p.is_file() and p.name!='SHA256.txt':
        checks.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.relative_to(dest).as_posix())
(dest/'SHA256.txt').write_text('\n'.join(checks)+'\n',encoding='utf-8')
out=dest.parent/'Narcade_v2.32_PSP.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=8) as z:
    for p in sorted(dest.rglob('*')):
        if p.is_file():z.write(p,p.relative_to(dest.parent))
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    assert z.read('Narcade_v2.32/Narcade_v2.32.iso')==(root/'Narcade.iso').read_bytes()
print(out,out.stat().st_size,'bytes; ZIP and embedded ISO verified')
