"""Versioned release; preserve the previous v2.5 package."""
from pathlib import Path
import hashlib,json,shutil,zipfile
root=Path(__file__).resolve().parents[1]
dest=root/'release/Narcade_v2.6'
files={'ISO/Narcade.iso':root/'Narcade.iso',
       'PSP/GAME/NARCADE/EBOOT.PBP':root/'EBOOT.PBP',
       'LEEME.txt':root/'LEEME_v2.6.txt','AVISOS.txt':root/'AVISOS.txt',
       'VALIDACION_v2.6.md':root/'VALIDACION_v2.6.md',
       'QA-v2.6.txt':root/'build/QA-v2.6.txt'}
for name in ['nico-walk.gif','nico-jog.gif','nico-run.gif','npc-variety-preview.png']:
    files['Vista/'+name]=root/'assets'/name
checks={}
for name,source in files.items():
    assert source.is_file(),source
    target=dest/name;target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copy2(source,target);checks[name]=hashlib.sha256(source.read_bytes()).hexdigest()
manifest=dest/'SHA256.json';manifest.write_text(json.dumps(checks,indent=2)+'\n')
out=dest.parent/'Narcade_v2.6_PSP.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=8) as z:
    for name in files:z.write(dest/name,'Narcade_v2.6/'+name)
    z.write(manifest,'Narcade_v2.6/SHA256.json')
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    for name,digest in checks.items():assert hashlib.sha256(z.read('Narcade_v2.6/'+name)).hexdigest()==digest
print(out);print('Verified all archived SHA-256 hashes;',out.stat().st_size,'bytes')
