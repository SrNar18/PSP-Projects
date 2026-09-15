"""Package tested artifacts without toolchains, emulator data or test saves."""
from pathlib import Path
import zipfile,hashlib,json
root=Path(__file__).resolve().parents[1]
out=root.parent/'Narcade_PSP.zip'
entries={}
def add(p,name):
 p=Path(p)
 assert p.is_file(),p
 entries[name]=p
add(root/'Narcade.iso','Narcade/Narcade.iso')
add(root/'EBOOT.PBP','Narcade/PSP/GAME/NARCADE/EBOOT.PBP')
add(root/'LEEME.txt','Narcade/LEEME.txt')
add(root/'AVISOS.txt','Narcade/AVISOS.txt')
for base in ['src','tools','assets']:
 for p in sorted((root/base).rglob('*')):
  if p.is_file() and p.suffix in ['.c','.h','.S','.py','.bin','.ttf','.txt','.png']:
   add(p,'Narcade/Fuente/'+str(p.relative_to(root)))
for name in ['Makefile','COMPILAR.md','CAMPANA.json','AVISOS.txt']:
 add(root/name,'Narcade/Fuente/'+name)
for name in ['QA-results.txt','VALIDACION.txt']:
 add(root/'build'/name,'Narcade/Pruebas/'+name)
for name in ['01-boot','02-story','04-world','05-movement','06-pause','07-saved','08-map']:
 add(root/'build/emulator'/(name+'.png'),'Narcade/Pruebas/PSP-'+name+'.png')
for p in sorted((root/'build').glob('mini-*.png')):
 add(p,'Narcade/Pruebas/motor-'+p.name)
checks={name:hashlib.sha256(p.read_bytes()).hexdigest() for name,p in entries.items()}
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=8) as z:
 for name,p in entries.items():z.write(p,name)
 z.writestr('Narcade/SHA256.json',json.dumps(checks,indent=2)+'\n')
with zipfile.ZipFile(out) as z:
 assert z.testzip() is None
 assert z.read('Narcade/Narcade.iso')==(root/'Narcade.iso').read_bytes()
 assert len(z.namelist())==len(set(z.namelist()))
print(out.name,out.stat().st_size,'bytes;',len(entries),'files; ZIP verified')
