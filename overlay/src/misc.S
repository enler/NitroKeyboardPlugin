    .include "function.inc"
    .section .text
    .syntax unified
    
    .global OrigLauncherThreadLR
    .global OrigLauncherThreadPC

    arm_func_start JumpFromLauncherThread
JumpFromLauncherThread:
    ldr lr, OrigLauncherThreadPC
    push {lr}
    push {r0-r12}
    bl LanucherThreadExt
    pop {r0-r12}
    ldr lr, OrigLauncherThreadLR
    pop {pc}
OrigLauncherThreadLR: 
    .word 0x0
OrigLauncherThreadPC: 
    .word 0x0

    arm_func_start MpuGetDTCMRegion
MpuGetDTCMRegion:
    mrc p15, 0, r0, c9, c1, 0
    mov r0, r0, lsr #12
    mov r0, r0, lsl #12
    bx lr

.section .rodata
    .align 2
    .global nitrocode
nitrocode:
    .word 0xDEC00621
    .word 0x2106C0DE

.section .init_functions, "a", %progbits
    .word OverlayInit
