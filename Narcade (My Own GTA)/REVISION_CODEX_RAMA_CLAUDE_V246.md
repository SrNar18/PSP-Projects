# Revisión de Codex para Claude — v2.46

Revisé `claude/bugs-parpadeo-metro` (`1d62902`) frente a `92ae5b9` y la integración final en `main` (`9b91dba`). No modifiqué el juego en esta revisión.

## Resultado

- Los cambios de aparcamiento, jardines, dovelas y edificio puente están presentes en `main`. `git diff --check` no mostró errores de formato.
- `qa_surface_v245` pasa: el jugador conserva las alturas esperadas de calzada, acera, puente, pasarela, plaza y parque. La nueva cota de los jardines no modifica `cm_surface`.
- `qa_metro_views` pasa en nueve perspectivas; `qa_metro_seams_v245` confirma continuidad del tablero y carriles. La prueba de 480 fotogramas del tren y la suite de ciudad/render pasan en la integración.

## Para la próxima orden del usuario

1. **P2, posible parpadeo que introduje al resolver el conflicto:** en `city26.inc`, la cara exterior del panel blanco está en `x = 10,0` y la interior de mi franja verde fina en `x = 10,11`. Tu nota recomienda separar planos paralelos al menos `0,25` a esa distancia por la precisión de 16 bits de la PSP. El test de mallas y la vista previa de PC no pueden confirmar ausencia de parpadeo en consola. Revisar sobre PSP y, si aparece, separar la franja ≥0,25 sin hundirla detrás de las puertas (tu puerta llega a `x = 10,5`).
2. **P2, solapes aún detectados:** `qa_zfight_v233` con `ZTOL=0.12` informa 3492 pares. Coinciden con los que dejaste anotados: marquesina/poste `(2087,353)`, palmeras `(747,1226)`, parada/farola `(1724,1045)` y bajos del puente `(1388,2006)`. El auditor sin etiquetas no permite atribuir cada triángulo a una línea exacta; usar tu copia instrumentada de `mkaud` antes de corregir.
3. **P3, inspección visual:** el edificio puente que sustituyó al cubo azul se ve más sólido en código, pero queda pendiente confirmar en PSP que sus cuatro pilares conectan de forma legible con el volumen y no invaden el paso. Las estaciones próximas a fachadas también pueden taparse con la cámara; comprobar desde la calle.

No encontré otro fallo nuevo atribuible a tu rama en esta revisión. La instalación en la PSP la haces tú, según la petición del usuario. El ZIP final integrado está en `release/Narcade_v2.46_PSP.zip` del checkout principal.
