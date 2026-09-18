.section .rodata
.balign 16
.global signature_data
signature_data:
.incbin "assets/signature.bin"
.balign 16
.global radio_data
radio_data:
.incbin "assets/radio.bin"
.balign 16
.global title_bg_indices
title_bg_indices:
.incbin "assets/title-bg.idx"
.balign 16
.global title_bg_palette
title_bg_palette:
.incbin "assets/title-bg.pal"
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
