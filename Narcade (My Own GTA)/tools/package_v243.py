"""Package Narcade v2.43 ISO/PBP with loading art, notices and review notes."""
from pathlib import Path
import hashlib,shutil,zipfile
root=Path(__file__).resolve().parents[1]
release=root/'release/Narcade_v2.43';release.mkdir(parents=True,exist_ok=True)
iso=release/'Narcade_v2.43.iso';shutil.copy2(root/'Narcade.iso',iso)
files=['AVISOS.txt','NOTAS_CODEX_MENU_METRO_V242.md','NOTA_LORE_LUNA_PARA_CLAUDE.md','REVISION_CODEX_CLAUDE_V242.md',
       'build/metro-v243-preview.png','build/menu-v242-es.png','build/menu-v242-en.png',
       'tools/fonts/Oxanium-OFL.txt','tools/fonts/Rajdhani-OFL.txt']
for source in files:shutil.copy2(root/source,release/Path(source).name)
(release/'LEEME.txt').write_text('''Narcade v2.43 - made by Naresz.
Menu moderno de cinco tarjetas, creditos, tipografias Oxanium/Rajdhani y metro corregido.
Integra Claude v2.42: cinco trofeos persistentes e imagenes de carga cada cinco segundos.

ISO: copia ISO/Narcade_v2.43.iso a la carpeta ISO de tu PSP con CFW.
PBP: extrae PSP/GAME/NARCADE/ completo; LOAD0..2.BIN deben quedar junto a EBOOT.PBP.
Se mantienen el identificador del juego y las partidas existentes.

El retrato de Ajustes representa a Luna por decision expresa de Naresz.
Su modelo/NPC 3D se incorporara en una futura tarea.

Compilacion PSP y pruebas de codigo en PC aprobadas. Las vistas incluidas son
previsualizaciones de interfaz/malla en PC, no capturas de consola.
PPSSPP no respondio al depurador en esta sesion; queda pendiente probar en PSP real.
''',encoding='utf-8')
pbp=root/'EBOOT.PBP'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
(release/'SHA256.txt').write_text(f'{sha(iso)}  {iso.name}\n{sha(pbp)}  PSP/GAME/NARCADE/EBOOT.PBP\n',encoding='ascii')
archive=root/'release/Narcade_v2.43_PSP.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 z.write(iso,f'ISO/{iso.name}');z.write(pbp,'PSP/GAME/NARCADE/EBOOT.PBP')
 for k,name in enumerate(('valley','street','rooftop')):z.write(root/f'assets/loading-v241/{name}.rgb565',f'PSP/GAME/NARCADE/LOAD{k}.BIN')
 for file in sorted(release.iterdir()):
  if file!=iso:z.write(file,f'{release.name}/{file.name}')
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 assert z.read(f'ISO/{iso.name}')==iso.read_bytes()
 assert z.read('PSP/GAME/NARCADE/EBOOT.PBP')==pbp.read_bytes()
 for k,name in enumerate(('valley','street','rooftop')):assert z.read(f'PSP/GAME/NARCADE/LOAD{k}.BIN')==(root/f'assets/loading-v241/{name}.rgb565').read_bytes()
print(archive,archive.stat().st_size,'bytes; ISO/PBP/loading assets/ZIP verified')
