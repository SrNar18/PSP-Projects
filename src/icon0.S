/* Portada ICON0 embebida para el icono de las partidas guardadas (dialogo de la Memory Stick). */
.section .rodata
.balign 16
.global icon0_png
icon0_png:
.incbin "assets/ICON0.png"
.global icon0_png_end
icon0_png_end:
.section .note.GNU-stack,"",@progbits
