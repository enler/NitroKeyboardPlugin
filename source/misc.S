    .include "function.inc"
    .section .text
    .syntax unified
    
    arm_func_start JumpFromLauncherThread
JumpFromLauncherThread:
    ldr lr, =OrigLauncherThreadPC
    ldr lr, [lr]
    push {lr}
    push {r0-r12}
    bl LanucherThreadExt
    pop {r0-r12}
    ldr lr, =OrigLauncherThreadLR
    ldr lr, [lr]
    pop {pc}


    .section .data
    .align 2, 0
.global OrigLauncherThreadLR
OrigLauncherThreadLR:
    .word 0
.global OrigLauncherThreadPC
OrigLauncherThreadPC:
    .word 0

.section .init_functions
    .word OverlayInit