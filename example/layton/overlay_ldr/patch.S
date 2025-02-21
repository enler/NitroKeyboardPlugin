.nds
.open "rom/arm9.bin", 0x02000000

.arm

.org 0x020000E0
.incbin "overlay_ldr.bin"

.org 0x020A4830
b 0x020000E0

.close