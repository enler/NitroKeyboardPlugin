    .syntax unified
    .section .text.entry, "ax", %progbits
    .arm
    .align 2

    .global KeyboardModule_Entry
    .type KeyboardModule_Entry, %function
KeyboardModule_Entry:
    push {r4-r7, lr}
    sub r4, pc, #12

    ldr r0, KeyboardModule_LinkAddress
    sub r6, r4, r0

    ldr r0, KeyboardModule_RelocOffset
    add r5, r4, r0
    ldr r7, KeyboardModule_RelocCount

.Lreloc_loop:
    cmp r7, #0
    beq .Lreloc_done

    ldr r0, [r5]
    add r0, r4, r0
    ldr r1, [r0]
    add r1, r1, r6
    str r1, [r0]

    add r5, r5, #4
    subs r7, r7, #1
    b .Lreloc_loop

.Lreloc_done:
    mov r0, r4
    ldr r1, KeyboardModule_ModuleSize
    ldr r3, .Ldc_flush_range
    blx r3

    ldr r3, .Lrun
    blx r3

    pop {r4-r7, pc}

    .align 2
    .global KeyboardModule_LinkAddress
KeyboardModule_LinkAddress:
    .word 0
    .global KeyboardModule_RelocOffset
KeyboardModule_RelocOffset:
    .word 0
    .global KeyboardModule_RelocCount
KeyboardModule_RelocCount:
    .word 0
    .global KeyboardModule_ModuleSize
KeyboardModule_ModuleSize:
    .word 0
.Ldc_flush_range:
    .word armDCacheFlush
.Lrun:
    .word KeyboardModule_Run

    .size KeyboardModule_Entry, . - KeyboardModule_Entry
