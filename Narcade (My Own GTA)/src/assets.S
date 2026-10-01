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
.global menu_new_v242
menu_new_v242:
.incbin "assets/menu-new-v242.565"
.balign 16
.global menu_continue_v242
menu_continue_v242:
.incbin "assets/menu-continue-v242.565"
.balign 16
.global menu_settings_v242
menu_settings_v242:
.incbin "assets/menu-settings-v242.565"
.balign 16
.global hud_meter_frame_v253
hud_meter_frame_v253:
.incbin "assets/hud-meter-frame-v253.4444"
.balign 16
.global menu_credits_v242
menu_credits_v242:
.incbin "assets/menu-credits-v242.565"
.balign 16
.global menu_trophies_v242
menu_trophies_v242:
.incbin "assets/menu-trophies-v242.565"
.balign 16
.global title_labels_v213
title_labels_v213:
.incbin "assets/title-labels-v213.a8"
.balign 16
.balign 16
.global narcade_logo_small_v244
narcade_logo_small_v244:
.incbin "assets/narcade-logo-small-v244.4444"
#ifndef _WIN32
.section .note.GNU-stack,"",@progbits
#endif
