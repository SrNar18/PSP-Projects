"""Build a genuine ISO-9660 PSP disc image from the static ELF executable."""
import pathlib, subprocess, shutil, io, hashlib
import pycdlib
root=pathlib.Path(__file__).resolve().parents[1]
stage=root/'build/umd';stage.mkdir(parents=True,exist_ok=True)
(stage/'PSP_GAME/SYSDIR').mkdir(parents=True,exist_ok=True)
(stage/'PSP_GAME/USRDIR').mkdir(exist_ok=True)
subprocess.run(['mksfoex','-s','CATEGORY=UG','-s','DISC_ID=NARC00001','-s','DISC_VERSION=2.50','-s','PSP_SYSTEM_VER=6.00','-d','BOOTABLE=1','-d','PARENTAL_LEVEL=5','-d','REGION=32768','Narcade 3D',str(stage/'PSP_GAME/PARAM.SFO')],check=True)
# v1.2: el cargador de disco del CFW solo admite, ademas de ejecutables de UMD firmados, ELFs
# ESTATICOS (asi funcionan los EBOOT.BIN "descifrados"). Se usa narcade_static.elf (make -f Makefile.iso).
shutil.copy(root/'narcade_static.elf',stage/'PSP_GAME/SYSDIR/EBOOT.BIN')
shutil.copy(root/'narcade_static.elf',stage/'PSP_GAME/SYSDIR/BOOT.BIN')
shutil.copy(root/'assets/ICON0.png',stage/'PSP_GAME/ICON0.PNG')
shutil.copy(root/'assets/PIC1.png',stage/'PSP_GAME/PIC1.PNG')
shutil.copy(root/'assets/SND0.AT3',stage/'PSP_GAME/SND0.AT3')  # musica en la XMB (tools/make_snd0.py)
(stage/'UMD_DATA.BIN').write_bytes(b'NARC-00001|E658BD244F5EED20|0001|G')
(stage/'PSP_GAME/USRDIR/README.TXT').write_text('Narcade 3D 2.50 - made by Naresz. Original homebrew. Assets embedded in executable.\n')
for k in range(3):
 src=root/f'assets/title-slides/slide{k}.rgb565'
 if src.exists():shutil.copy(src,stage/'PSP_GAME/USRDIR'/f'TITLE{k}.BIN')  # v2.50: portada animada
for src in sorted((root/'assets/radio').glob('RADIO*.BIN')):shutil.copy(src,stage/'PSP_GAME/USRDIR'/src.name)  # v2.52: emisoras de los carros (tools/build_radio.py)
for k,name in enumerate(['valley','street','rooftop']):shutil.copy(root/f'assets/loading-v241/{name}.rgb565',stage/'PSP_GAME/USRDIR'/f'LOAD{k}.BIN')  # v2.41: ilustraciones de carga (se leen en la carga, no ocupan RAM)
for source,dest in [('AVISOS.txt','NOTICES.TXT'),('tools/PSPSDK-LICENSE.txt','SDK.TXT'),('tools/Newlib-LICENSE.txt','NEWLIB.TXT'),('tools/Allura-LICENSE.txt','ALLURA.TXT'),('tools/DejaVu-LICENSE.txt','DEJAVU.TXT'),('tools/fonts/Oxanium-OFL.txt','OXANIUM.TXT'),('tools/fonts/Rajdhani-OFL.txt','RAJDHANI.TXT')]:
 shutil.copy(root/source,stage/'PSP_GAME/USRDIR'/dest)
iso=pycdlib.PyCdlib();iso.new(interchange_level=1,vol_ident='NARCADE',sys_ident='PSP GAME',pub_ident_str='NARESZ')
for d in ['/PSP_GAME','/PSP_GAME/SYSDIR','/PSP_GAME/USRDIR']:iso.add_directory(d)
for p in sorted(stage.rglob('*')):
 if p.is_file():iso.add_file(str(p),iso_path='/'+p.relative_to(stage).as_posix()+';1')
out=root/'Narcade.iso';iso.write(str(out));iso.close()
check=pycdlib.PyCdlib();check.open(str(out));buf=io.BytesIO();check.get_file_from_iso_fp(buf,iso_path='/PSP_GAME/SYSDIR/EBOOT.BIN;1');assert buf.getvalue()==(root/'narcade_static.elf').read_bytes();check.close()
print(out.name,out.stat().st_size,'bytes; executable verified')
print('SHA256',hashlib.sha256(out.read_bytes()).hexdigest())
