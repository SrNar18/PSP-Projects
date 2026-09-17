"""Portable Windows build using a locally extracted pspdev-win toolchain.

Run: python tools/build_windows.py [--sdk build/pspdev]
Equivalent to Makefile and Makefile.iso, without installing MSYS make.
"""
import argparse, os, pathlib, subprocess, sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--sdk',default='build/pspdev');args=p.parse_args()
sdk=(ROOT/args.sdk).resolve();os.chdir(ROOT)
env=os.environ.copy();env['PATH']=str(sdk/'bin')+os.pathsep+env['PATH']
def posix(path):
    path=pathlib.Path(path).resolve().as_posix()
    return path
def run(tool,*args):
    print(tool,*args,flush=True)
    subprocess.run([str(sdk/'bin'/f'{tool}.exe'),*map(str,args)],env=env,check=True)
inc=posix(sdk/'psp/sdk/include');lib=posix(sdk/'psp/sdk/lib')
flags=['-O2','-G0','-Wall','-Wextra','-Wno-misleading-indentation','-std=gnu99','-DNARCADE_3D','-D_PSP_FW_VERSION=600','-Isrc',f'-I{inc}',f'-I{posix(sdk/"psp/include")}']
if os.environ.get('NARCADE_PROFILE'): flags.append('-DNARCADE_PROFILE')  # overlay de tiempo por fotograma
sources=['psp_main.c','game.c','render3d.c','assets.S','textures3d.S','icon0.S']
objects=[]
for source in sources:
    obj='build/'+pathlib.Path(source).stem+'.o';objects.append(obj)
    run('psp-gcc',*flags,'-c','src/'+source,'-o',obj)
libs=['-lpspgum','-lpspgu','-lpsputility','-lpspaudiolib','-lpspaudio','-lpsppower','-lm','-lpspdebug','-lpspdisplay','-lpspge','-lpspctrl','-lpspnet','-lpspnet_apctl']
ld=[f'-L{lib}',f'-L{posix(sdk/"psp/lib")}','-Wl,-zmax-page-size=128']
run('psp-gcc',*flags,*ld,f'-specs={lib}/prxspecs',f'-Wl,-q,-T{lib}/linkfile.prx',*objects,f'{lib}/prxexports.o',*libs,'-o','narcade.elf')
run('psp-fixup-imports','narcade.elf');run('psp-prxgen','narcade.elf','narcade.prx')
run('mksfoex','-d','MEMSIZE=0','Narcade 3D','PARAM.SFO')
run('pack-pbp','EBOOT.PBP','PARAM.SFO','assets/ICON0.png','NULL','NULL','assets/PIC1.png','assets/SND0.AT3','narcade.prx','NULL')
run('psp-gcc',*flags,*ld,*objects,*libs,'-o','narcade_static.elf')
run('psp-fixup-imports','narcade_static.elf')
run('psp-size','narcade.elf','narcade_static.elf')
env['PYTHONPATH']=str(ROOT/'build/python-deps')+os.pathsep+env.get('PYTHONPATH','')
subprocess.run([sys.executable,'tools/package_iso.py'],env=env,check=True)
