.section .rodata
.balign 16
.global textures3d_data
textures3d_data:
.incbin "assets/textures3d.bin"
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
