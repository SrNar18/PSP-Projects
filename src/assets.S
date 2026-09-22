.section .rodata
.balign 16
.global radio_data
radio_data:
.incbin "assets/radio.bin"
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
.balign 16
.global title_cover_v213
title_cover_v213:
.incbin "assets/title-cover-v213.565"
.balign 16
.global title_menu_v213
title_menu_v213:
.incbin "assets/title-menu-v213.565"
.balign 16
.global title_card_new_v213
title_card_new_v213:
.incbin "assets/title-card-0-v213.565"
.balign 16
.global title_card_continue_v213
title_card_continue_v213:
.incbin "assets/title-card-1-v213.565"
.balign 16
.global title_labels_v213
title_labels_v213:
.incbin "assets/title-labels-v213.a8"
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
