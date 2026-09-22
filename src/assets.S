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
.balign 16
.global city_bed
city_bed:
.incbin "assets/city-bed.pcm"
.balign 16
.global city_engine
city_engine:
.incbin "assets/city-engine.pcm"
.balign 16
.global city_horn
city_horn:
.incbin "assets/city-horn.pcm"
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
