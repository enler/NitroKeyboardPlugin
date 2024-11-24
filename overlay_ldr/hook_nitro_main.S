    .include "function.inc"
    .section .text
    .syntax unified

    thumb_func_start Hook_NitroMain
Hook_NitroMain:
    push {r0, r2}
    ldr r0, =ArenaLo
    ldr r1, [r0]
    ldr r2, =OverlaySize
    adds r1, r2
    str r1, [r0]
    pop {r0, r2}
    ldr r1, =NitroMain
    bx r1