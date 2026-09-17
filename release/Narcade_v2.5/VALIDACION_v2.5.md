# Narcade 3D v2.5 — validación

Fecha: 17 de septiembre de 2026. Base: trabajo de Claude hasta `cefb11c`.

## Resultado

Compilados EBOOT.PBP y una ISO con ELF estático. La herramienta de empaquetado
reabre la ISO y compara su ejecutable con el ELF generado. Compilación final
sin el indicador de diagnóstico NARCADE_PROFILE.

SHA-256 de Narcade.iso:
`37abc7301307a52313e09cb83be9ba2cecdaaf39a08bd2cd6be980fd11dec0fc`

## Pruebas automáticas

- Campaña: 36 misiones, 146 objetivos y siete familias de minijuegos.
- Guardados, recuperación de guardado corrupto, menús Start y pausa.
- 12.000 fotogramas de entradas aleatorias. Esta ejecución no usa sanitizadores.
- Contactos frontales, laterales, centros coincidentes, pared y 32 orientaciones.
- Roce lateral conserva avance; marcha atrás sale de un solapamiento.
- Joystick mantenido, zona muerta, seguimiento y ausencia de giro con L/R.
- Calles horizontales y cruces planos, rampas verticales continuas por debajo
  del 12% de pendiente en el espacio proyectado.
- Base ortonormal de los coches: dimensiones constantes en rampas y giros.
- 896 escenas, recorte contra seis planos y sin desbordamiento de materiales.
- Prueba adicional de cobertura real del suelo bajo el jugador en 512
  combinaciones de posición/orientación cerca del taller.
- 32 poses de caminar/correr, topología estable, 2.692 triángulos del personaje.

Registro de salida: QA-v2.5.txt. Pruebas nativas con GCC en Windows, sin simular
la temporización ni todos los detalles del hardware de PSP.

## Revisión visual

PPSSPP 1.20.4: arranque de la ISO, nueva historia, mundo, desplazamiento a pie
y entrada en coche. Durante esta revisión se reprodujo un hueco en el suelo;
se corrigió el descarte prematuro de manzanas y se repitió el recorrido con la
ISO final. La captura psp-v2.5-world.png corresponde a la ISO final sin overlay.

Las imágenes nico-streetwear-preview.png y los GIF son vistas de estudio de
la malla y las poses exportadas del motor; no son capturas de la consola.
La animación del GLB v3 se realiza proceduralmente en el juego: no se entrega
un GLB con esqueleto ni un proyecto Blender animado.

## Pendiente

Probar esta versión en la PSP E1000 del usuario, especialmente sensación de
cámara, rendimiento sostenido y contactos con varios coches simultáneos.
No se promete una tasa de FPS basándose únicamente en tiempos de dibujo del
emulador. El audio XMB y los ajustes de compatibilidad anteriores se conservan;
no se ha confirmado aquí el audio XMB en hardware.
