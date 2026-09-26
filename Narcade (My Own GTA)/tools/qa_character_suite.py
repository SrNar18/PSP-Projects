"""Compile and run the character regressions with the installed host GCC."""
from pathlib import Path
import argparse, subprocess, concurrent.futures
p=argparse.ArgumentParser();p.add_argument('--cc',required=True);args=p.parse_args()
root=Path(__file__).resolve().parents[1];out=root/'build/character-qa';out.mkdir(parents=True,exist_ok=True)
flags=['-O2','-DNARCADE_3D','-DR3_HOST','-std=gnu99']
objects=[]
for source in ['render3d.c','assets.S','textures3d.S','icon0.S']:
 obj=out/(source.replace('.','_')+'.o');subprocess.run([args.cc,*flags,'-c',str(root/'src'/source),'-o',str(obj)],cwd=root,check=True);objects.append(str(obj))
game=['qa_combat','qa_gait_v217','qa_thumb_v229','qa_controls_v218','qa_weapons','qa']
renderer=['qa_character_combat','qa_weapon_mesh','qa_city28']
def test(name):
 exe=out/(name+'.exe');cmd=[args.cc,*flags,str(root/'tools'/(name+'.c'))]
 if name in game:cmd+=objects
 result=subprocess.run(cmd+['-lm','-o',str(exe)],cwd=root,capture_output=True,text=True)
 if not result.returncode:result=subprocess.run([str(exe)],cwd=root,capture_output=True,text=True,timeout=60)
 return name,result.returncode,result.stdout+result.stderr
failed=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 for name,code,log in pool.map(test,game+renderer):
  print(name, 'PASS' if code==0 else 'FAIL',log,flush=True)
  if code:failed.append(name)
if failed:raise SystemExit('Failed: '+', '.join(failed))
