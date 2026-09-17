# Narcade v2.6 — movimiento y piernas

También incluye ocho variantes de peatón (cuatro femeninas y cuatro masculinas),
con 344–418 triángulos, seis materiales nuevos de ropa y dos de rostro.
Los 42 peatones combinan estas variantes con tonos de piel y colores.
Las ocho texturas originales se generan mediante patrones de tejido y dibujo
procedural, a 64×64 RGB565: 64 KB adicionales, alojados en RAM principal.
La copia de texturas a VRAM permanece limitada a los 21 materiales originales
para no desbordar los 2 MB de memoria gráfica.
Prueba específica: ocho mallas exportadas y 42 estilos en 16 poses cada uno,
sin vértices inválidos ni desbordamientos. La vista de conjunto es un render
de las mallas efectivas del juego, no arte conceptual.
Se conservaron los nuevos archivos city3d.inc y daylight.inc encontrados en
el proyecto durante el trabajo. Se corrigió el producto entero con signo del
hash de ciudad para evitar overflow indefinido detectado por el compilador.

Se mantiene el modelo v3 del usuario. La asignación por substring `'-l' in name`
clasificaba `pants-leg-r` como izquierda. El importador usa ahora tokens completos
de lado; la malla se regeneró desde el GLB original. Las pruebas requieren
vértices de pantalón en ambos huesos y en lados opuestos del cuerpo.

X mantenida: trote a 111 unidades/s. Sin X: 72. Tres pulsaciones con intervalo
de 0,07–0,45 s activan sprint a 150, renovado por pulsaciones posteriores.
La velocidad se interpola. Son velocidades objetivo antes de aplicar intensidad
del joystick y pendiente. Los estados transitorios no modifican el guardado.

El ciclo de animación utiliza distancia realmente recorrida. Cada pie tiene
su propia fase de apoyo y recuperación; al correr disminuye el tiempo de apoyo,
aumenta la elevación, se flexionan codos y rodillas y se inclina el torso.
Se calculan seno/coseno y trayectorias una vez por pose, no por vértice.
Es animación procedural del juego, no captura de movimiento ni GLB con esqueleto.

Pruebas ejecutadas:

- Comparación integrada de avance al caminar, mantener X y pulsar X.
- Mantener X indefinidamente no activa sprint. Caducar el ritmo recupera
  trote o marcha; pulsaciones lentas o estando quieto no acumulan sprint.
- Pausar borra el ritmo. Empujar contra una pared no avanza el ciclo.
- 48 poses de caminar/trotar/correr, pies alternados, sin pies bajo el suelo,
  topología estable y 2.692 triángulos, incluida geometría de zapatillas.
- Regresión: campaña, guardados, menús, 12.000 entradas aleatorias, contactos
  de vehículos, cámara, rampas, 896 escenas y 512 comprobaciones de suelo.
- Compilación PSP y comparación del ejecutable contenido en la ISO.
- Arranque de la ISO final en PPSSPP 1.20.4.

Registro: QA-v2.6.txt. No se ha probado esta versión en la PSP física ni se
ha medido su rendimiento sostenido en hardware. Los GIF incluidos son vistas
de estudio de las poses del motor y permiten comparar los tres ritmos.
