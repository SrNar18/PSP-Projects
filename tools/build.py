"""Compila PSP-IA con el toolchain pspdev de Narcade y genera EBOOT.PBP + PSP_IA.iso (ELF estatico como EBOOT.BIN)."""
import os, pathlib, subprocess, sys, shutil, io, hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
NARC=pathlib.Path(r'C:\Users\carmo\Downloads\Narcade_v1.1_fuente')
sdk=NARC/'build/pspdev';os.chdir(ROOT)
env=os.environ.copy();env['PATH']=str(sdk/'bin')+os.pathsep+env['PATH']
def posix(p):return pathlib.Path(p).resolve().as_posix()
def run(tool,*args):
    print(tool,*args,flush=True);subprocess.run([str(sdk/'bin'/f'{tool}.exe'),*map(str,args)],env=env,check=True)
inc=posix(sdk/'psp/sdk/include');lib=posix(sdk/'psp/sdk/lib')
flags=['-O2','-G0','-Wall','-Wextra','-std=gnu99','-D_PSP_FW_VERSION=600','-Isrc',f'-I{inc}',f'-I{posix(sdk/"psp/include")}']+os.environ.get('PSPIA_CFLAGS','').split()
(ROOT/'build').mkdir(exist_ok=True)
tq=os.environ.get('PSPIA_TEST','');tc=os.environ.get('PSPIA_CHAT','')
(ROOT/'src/testquery.h').write_text((('#define TEST_QUERY "'+tq+'"'+chr(10)) if tq else '')+(('#define TEST_CHAT "'+tc+'"'+chr(10)) if tc else '')+'/* pruebas */'+chr(10))
objs=[]
for src in ['main.c','search.c']+(['chat.c'] if (ROOT/'src/chat.c').exists() else []):
    obj='build/'+pathlib.Path(src).stem+'.o';objs.append(obj);run('psp-gcc',*flags,'-c','src/'+src,'-o',obj)
libs=['-lpspgu','-lpspge','-lpsputility','-lpsppower','-lpspdisplay','-lpspctrl','-lz','-lm']
ld=[f'-L{lib}',f'-L{posix(sdk/"psp/lib")}','-Wl,-zmax-page-size=128']
run('psp-gcc',*flags,*ld,f'-specs={lib}/prxspecs',f'-Wl,-q,-T{lib}/linkfile.prx',*objs,f'{lib}/prxexports.o',*libs,'-o','build/pspia.elf')
run('psp-fixup-imports','build/pspia.elf');run('psp-prxgen','build/pspia.elf','build/pspia.prx')
run('mksfoex','-d','MEMSIZE=0','PSP-IA','build/PARAM.SFO')
icon=ROOT/'assets/ICON0.png';pic=ROOT/'assets/PIC1.png'
run('pack-pbp','out/EBOOT.PBP','build/PARAM.SFO',str(icon) if icon.exists() else 'NULL','NULL','NULL',str(pic) if pic.exists() else 'NULL','NULL','build/pspia.prx','NULL')
run('psp-gcc',*flags,*ld,*objs,*libs,'-o','build/pspia_static.elf');run('psp-fixup-imports','build/pspia_static.elf')
# ISO
sys.path.insert(0,str(NARC/'build/python-deps'));import pycdlib
stage=ROOT/'build/umd';shutil.rmtree(stage,ignore_errors=True);(stage/'PSP_GAME/SYSDIR').mkdir(parents=True);(stage/'PSP_GAME/USRDIR').mkdir()
run('mksfoex','-s','CATEGORY=UG','-s','DISC_ID=PSIA00001','-s','DISC_VERSION=1.00','-s','PSP_SYSTEM_VER=6.00','-d','BOOTABLE=1','-d','PARENTAL_LEVEL=1','-d','REGION=32768','PSP-IA',str(stage/'PSP_GAME/PARAM.SFO'))
shutil.copy(ROOT/'build/pspia_static.elf',stage/'PSP_GAME/SYSDIR/EBOOT.BIN');shutil.copy(ROOT/'build/pspia_static.elf',stage/'PSP_GAME/SYSDIR/BOOT.BIN')
if icon.exists():shutil.copy(icon,stage/'PSP_GAME/ICON0.PNG')
if pic.exists():shutil.copy(pic,stage/'PSP_GAME/PIC1.PNG')
(stage/'UMD_DATA.BIN').write_bytes(b'PSIA-00001|0000000000000000|0001|G')
(stage/'PSP_GAME/USRDIR/README.TXT').write_text('PSP-IA: asistente de conocimiento offline. Datos en ms0:/IA (Wikipedia en espanol, CC BY-SA).\n')
iso=pycdlib.PyCdlib();iso.new(interchange_level=1,vol_ident='PSPIA',sys_ident='PSP GAME',pub_ident_str='NARESZ')
for d in ['/PSP_GAME','/PSP_GAME/SYSDIR','/PSP_GAME/USRDIR']:iso.add_directory(d)
for p in sorted(stage.rglob('*')):
    if p.is_file():iso.add_file(str(p),iso_path='/'+p.relative_to(stage).as_posix()+';1')
out=ROOT/'out/PSP_IA.iso';iso.write(str(out));iso.close()
print(out,out.stat().st_size,'bytes')
