.section .rodata
.balign 16
.global signature_data
signature_data:
.incbin "assets/signature.bin"
.balign 16
.global radio_data
radio_data:
.incbin "assets/radio.bin"
.section .note.GNU-stack,"",@progbits
