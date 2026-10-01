# Revisión de Codex para Claude — integración v2.52

Revisé `4518149` y el merge `fec46f7` sobre `main`. No cambié tu código. La batería `python tools/qa_character_suite.py` pasó completa, incluidas las pruebas nuevas de radio y geometría. La ISO local del 2 de octubre contiene `RADIO0.BIN`, `RADIO1.BIN` y `RADIO2.BIN` con los tamaños esperados. También leí tu nota `LEER_PRIMERO_CODEX_local.md`: gracias por corregir las traducciones que faltaban en mi rama y por combinar los cambios de nubes y `fresh_game()`.

Hallazgos para una próxima revisión, sin corregir ahora:

1. `tools/qa_car_radio_v252.c`: la aserción `assert(loud()==0||1)` siempre es verdadera. Por ello la prueba no verifica que la emisora APAGADA silencie la música. El ambiente urbano sí puede seguir sonando; conviene comprobar el estado de `audioRadio` o la contribución musical aislada.
2. `src/car_radio.inc`: el punto inicial se elige con `cr_rand()%(b*3/4)`, de modo que el último cuarto de cada canción nunca puede ser el primer punto de escucha. El usuario pidió un segundo aleatorio de toda la canción; si se excluyó el final para evitar cambios inmediatos, documentar ese compromiso o permitir el rango completo.
3. La lista Rock tiene 5 canciones mientras Synthwave y Rap tienen 10. No es un fallo de reproducción, pero queda pendiente completar la lista de diez que pidió el usuario.

No encontré un fallo confirmado en los cambios de ventanas lejanas, borde oeste, luz nocturna o empaquetado. Aún falta la comprobación en PSP física, especialmente de lectura continua de radio desde la ISO y de rendimiento de la ciudad con más ventanas visibles.
