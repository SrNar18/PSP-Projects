"""Package the tested 2.35 ISO/PBP and handoff notes without touching other branches."""
from pathlib import Path
import hashlib,shutil,zipfile
root=Path(__file__).resolve().parents[1];release=root/'release/Narcade_v2.35';release.mkdir(parents=True,exist_ok=True)
shutil.copy2(root/'Narcade.iso',release/'Narcade_v2.35.iso')
shutil.copy2(root/'NOTAS_CODEX_PARA_CLAUDE_v2.35.md',release/'NOTAS_CODEX_PARA_CLAUDE_v2.35.md')
shutil.copy2(root/'AVISOS.txt',release/'AVISOS.txt')
for name in ['PSPSDK-LICENSE.txt','Newlib-LICENSE.txt','Allura-LICENSE.txt','DejaVu-LICENSE.txt']:shutil.copy2(root/'tools'/name,release/name)
for source,target in [('character-combat-preview.png','personaje-inspeccion-PC.png'),('weapon-wheel.png','rueda-armas-inspeccion-PC.png')]:
 shutil.copy2(root/'build'/source,release/target)
(release/'LEEME.txt').write_text('''NARCADE 3D v2.35 — made by Naresz

INSTALACION (elige una opcion):
1. ISO: copia Narcade_v2.35.iso a ms0:/ISO/ de la PSP con CFW.
2. PBP: extrae PSP/GAME/NARCADE del ZIP a la Memory Stick.
Las partidas anteriores siguen usando el panel oficial de Sony y el mismo formato.

CONTROLES A PIE:
Joystick/cruceta: mover. X mantenida: trotar; X repetida: correr con estamina.
L mantenida: rueda de armas; joystick elige y soltar L equipa.
R mantenida: apuntar. NPC cercano visible: fijacion. Sin NPC: mira libre con joystick.
R + circulo: disparar; con punos/bate, mantener circulo para golpes repetidos.
Cuadrado: saltar. Frente a un muro escalable de parque: agarrarse y pasar al otro lado.
R + cuadrado: interactuar con misiones/vinilos/Metro.
Circulo sin apuntar: mensajes. START: pausa/mapa/guardar. SELECT: distancia de camara.
Triangulo: entrar/salir del coche. En coche: X acelera, cuadrado frena, joystick gira, L/R radio.

NOVEDADES:
Poses de marcha mas suaves, pelo/mangas corregidos y agarre de armas a una o dos manos.
Modelos con detalles y cañones redondeados, acero/madera propios e iconos nuevos.
NPC con vida, impactos de sangre breves, huida y caida; la violencia aumenta las estrellas.
Salto y escalada en tres muros de parques. Las fachadas altas no son escalables.
Municion ilimitada. El estado de los peatones se reinicia al cargar/reconstruir el mundo.

VERIFICACION:
Compilacion PSP, pruebas de controles/combate/guardado/misiones y entrada real en PPSSPP.
Las imagenes adjuntas son inspecciones en PC de la malla y la rueda, no capturas de PSP.
El aspecto y FPS de esta version aun requieren prueba en una PSP fisica.
La nota MD adjunta explica el codigo y la integracion con el trabajo de Claude.
''',encoding='utf-8')
iso=release/'Narcade_v2.35.iso';pbp=root/'EBOOT.PBP'
checks=hashlib.sha256(iso.read_bytes()).hexdigest()+'  Narcade_v2.35.iso\n'+hashlib.sha256(pbp.read_bytes()).hexdigest()+'  PSP/GAME/NARCADE/EBOOT.PBP (dentro del ZIP)\n'
(release/'SHA256.txt').write_text(checks,encoding='utf-8')
zip_path=root/'release/Narcade_v2.35_PSP.zip'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 z.write(iso,'ISO/Narcade_v2.35.iso');z.write(pbp,'PSP/GAME/NARCADE/EBOOT.PBP')
 for f in sorted(release.iterdir()):
  if f!=iso:z.write(f,'Narcade_v2.35/'+f.name)
with zipfile.ZipFile(zip_path) as z:
 assert z.testzip() is None
 assert z.read('ISO/Narcade_v2.35.iso')==iso.read_bytes()
 assert z.read('PSP/GAME/NARCADE/EBOOT.PBP')==pbp.read_bytes()
print(zip_path,zip_path.stat().st_size,'bytes; ISO and PBP verified')
