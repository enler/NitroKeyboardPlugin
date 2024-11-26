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

.section .init_functions
    .word OverlayInit