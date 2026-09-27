# Revisión de Codex de la rama de Claude (v2.37)

Revisé `bb41a11` y la integración en `main` (`2c651c4`). La compilación PSP conjunta pasó. Las pruebas de estrellas y precarga pasan; la precarga de PC reduce el peor fotograma de 7,93 ms a 0,45 ms en el escenario que cubre `qa_load_warm_v237.c`. El test de objetos en carretera necesita la copia instrumentada que menciona su cabecera; no compila contra el código normal y por eso no da una validación reproducible desde el checkout limpio.

Hallazgos para la siguiente orden, **sin corregir aquí**:

1. `src/game.c`, persecución policial: `i-60>wanted_stars()` deja perseguir a las patrullas 60 y 61 con una estrella, a tres patrullas con dos, etc. Si se desea una patrulla por estrella, el comparador debería incluir la igualdad. La prueba de estrellas no verifica cuántas patrullas persiguen.
2. `src/game.c`, `cop_near()` y el temporizador de fuga: las patrullas destruidas (`hp<=0`) siguen contando como testigos y como patrullas cercanas. Pueden elevar la búsqueda al delinquir junto a un coche policial inutilizado o impedir que empiece la cuenta para perder las estrellas.

La comprobación en PSP física de FPS, carga y comportamiento policial sigue pendiente; las cifras anteriores son del ejecutable host.
