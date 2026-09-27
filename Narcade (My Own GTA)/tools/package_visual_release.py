"""Package the visual branch, with honest PC inspection labels."""
from pathlib import Path
import hashlib,shutil,zipfile
root=Path(__file__).resolve().parents[1]
version='2.36'
release=root/'release'/('Narcade_v'+version);release.mkdir(parents=True,exist_ok=True)
iso=release/('Narcade_v'+version+'.iso');shutil.copy2(root/'Narcade.iso',iso)
for source in ('AVISOS.txt','tools/PSPSDK-LICENSE.txt','tools/Newlib-LICENSE.txt','tools/Allura-LICENSE.txt','tools/DejaVu-LICENSE.txt','NOTAS_CODEX_TEXTURAS_ANIMACIONES.md','REVISION_CODEX_RAMA_CLAUDE_2026-09-27.md','build/human-gait-sheet.png','build/human-gait.gif'):
 shutil.copy2(root/source,release/Path(source).name)
(release/'LEEME.txt').write_text("Narcade v2.36: texturas y animaciones de Codex.\nISO a ms0:/ISO/; o extraer PSP/GAME/NARCADE del ZIP a la Memory Stick.\nSe mantiene el formato de partidas y los controles existentes.\nLas imagenes y GIF son inspecciones PC de la malla, no capturas PSP.\nDiez suites host y compilacion PSP aprobadas; validacion fisica pendiente.\nEsta entrega conjunta incluye la rama de casetas/rendimiento publicada por Claude.\nConsulta la nota MD para alcance, reproduccion y limitaciones.\n",encoding='utf-8')
pbp=root/'EBOOT.PBP'
(release/'SHA256.txt').write_text(hashlib.sha256(iso.read_bytes()).hexdigest()+'  '+iso.name+'\n'+hashlib.sha256(pbp.read_bytes()).hexdigest()+'  PSP/GAME/NARCADE/EBOOT.PBP\n')
output=root/'release'/('Narcade_v'+version+'_PSP.zip')
with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 z.write(iso,'ISO/'+iso.name);z.write(pbp,'PSP/GAME/NARCADE/EBOOT.PBP')
 for f in sorted(release.iterdir()):
  if f!=iso:z.write(f,release.name+'/'+f.name)
with zipfile.ZipFile(output) as z:
 assert z.testzip() is None
 assert z.read('ISO/'+iso.name)==iso.read_bytes()
 assert z.read('PSP/GAME/NARCADE/EBOOT.PBP')==pbp.read_bytes()
print(output,output.stat().st_size,'bytes; ISO/PBP/ZIP verified')
