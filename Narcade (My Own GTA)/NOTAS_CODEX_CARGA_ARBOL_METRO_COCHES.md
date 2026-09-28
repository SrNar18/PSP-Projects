# Codex → Claude: imágenes, árboles, metro y coches

Rama `codex/loading-art-urban-detail`, desde main 0c5c763. Las imágenes se publicaron primero en eceefde; ver NOTA_CARGA_IMAGENES_PARA_CLAUDE.md. Tu navegación de menú, carga real, rendimiento, ventanas y tráfico se mantienen como tu tarea.

## Tu revisión 2.40

1. El formateador rechaza conversiones no admitidas ANTES de consumir va_args: retorna -1 y cadena vacía. `%ld`, `%zu`, `%*d` comprobados por qa_settings; tools/build_localization.py valida los formatos de llamadas snprintf de game/combat/settings durante CI. La cadena técnica debe usar raw_snprintf, que soporta formatos nativos completos. No ampliar el macro sin pruebas.
2. Todas las rutas, nombres de archivos y copias de savepath usan raw_snprintf; nunca buscan traducción. Test con ruta igual a una clave española confirma que queda intacta.
3. text_raw y textwrap_raw reciben cadenas ya traducidas. label, text_center, líneas HUD y etapa de carga traducen una sola vez antes de medir/dibujar.
4. La navegación en cuadrícula te corresponde por la orden actual. Al integrar, ajustar qa_settings (ahora usa dos DOWN para la lista vieja) a un DOWN.
5. Preferencias nuevas en PSP en `ms0:/NARCADE.CFG`, con .tmp/.bak seguros, fuera de SAVEDATA. Si no existe lee el .cfg antiguo como compatibilidad; no altera datos/checksum de las partidas. No he podido probar el gestor Sony en consola física. Host mantiene ruta del archivo .sav para aislar tests.

## Visuales y memoria

- Follaje nuevo original generado con imagegen, fondo transparente real y espacios internos. Fuente en assets/reference/foliage-cutout-v241.png. Compilador convierte SOLO LEAVES a RGBA4444 (8KB igual que antes), resto RGB565. Renderer configura GU_TCC_RGBA y GU_ALPHA_TEST GEQUAL 128 para LEAVES, restaura estados antes de sombras/luces. No blending ni ordenación de hojas. GU según documentación PSPSDK https://pspdev.github.io/pspsdk/group__GU.html.
- Las copas cercanas usan 3 tarjetas cruzadas por grupo en lugar de 12 caras de una envolvente cerrada. Conservar esto para evitar bloques sólidos y reducir geometría; las ramas siguen modeladas. Atlas total SIN aumento: 884736 bytes; VRAM sin aumento. QA verifica alfa de la textura REAL empaquetada: 66,7% píxeles recortables.
- Coches: seis perfiles conservados, laterales con hombro y faldón recogido, pasos de rueda abiertos en la malla cercana (menos de 100); medios simplifican la carrocería y lejanos conservan LOD 170. UV de panel continuo por posición/altura; tiradores y molduras ajustados a la nueva superficie. Parachoques, matrícula y parrilla usan FLAT para evitar rayas falsas de metal. Prioridad de coches por distancia y faros bajo presión se conservan.
- Parches de carrocería estáticos por tipo/LOD, ~39KB de RAM, sin allocation por fotograma; no regeneran recortes/arcos cada frame. Medición misma máquina 6000 frames sprint: base media .527 / p95 .759 / max4.249 ms; nueva .571 / p95 .823 / max4.168 ms. Incremento de media ~8%, máximo comparable, SOLO PC; medir en PSP antes de afirmar FPS. Si necesitas más margen, acercar umbral detallado 100 antes de eliminar luces o texturas.
- Metro: box NO corta el tren flexible contra la rejilla fija del terreno. Esos cortes cambiaban continuamente los UV y repetían textura según posición. El tren conserva conformación vertical al rail y UV/topología estable. Puertas con panel claro, juntas y tiradores; cristales pertenecen al panel móvil, no tapan la entrada abierta. No se toca velocidad/recorrido/colisiones.

## Validación

qa_character_suite incluye 14 suites, con nueva qa_metro_motion: 480 frames atravesando terrazas con puertas cerradas/abiertas, UV/topología idénticos y sin desbordamiento. qa_car_priority, 20 coches, 512 escenas día/noche y combate incluidos. qa_foliage_texture comprueba el binario. Compilación PSP, previsualización PC de las mallas/texturas; falta prueba física.

Si publicas una nueva rama, se revisará al terminar. Hallazgos nuevos quedarán en nota para próxima orden, sin tocar tus tareas ahora.
