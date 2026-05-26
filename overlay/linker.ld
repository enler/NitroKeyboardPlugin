OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)

SECTIONS
{
    .text : {
        *src/overlay_entry.o(.text)
        *src/*.o(.text*)
        *nitro.a:import.o(.text)
        *libc.a:libc_a-memcpy-stub.o(.text.memcpy)
        *libc.a:libc_a-memset.o(.text.memset)
        *libgcc.a:_divsi3.o(.text)
        *libgcc.a:_dvmd_tls.o(.text)
        *libcalico_ds9.a:arm-cache.32.o(.text.armDCacheFlush)
        *libcalico_ds9.a:arm-cache.32.o(.text.armDrainWriteBuffer)
        *libcalico_ds9.a:arm-shims.32.o(.text.armIrqLockByPsr)
        *libcalico_ds9.a:arm-shims.32.o(.text.armIrqUnlockByPsr)
        *libcalico_ds9.a:bios.o(.text.svcWaitByLoop)
    }
    . = ALIGN(4);

    .rodata : {
        *src/*.o(.rodata*)
    }
    . = ALIGN(4);

    .data : {
        *src/*.o(.data)
    }
    . = ALIGN(4);

    __init_functions_begin__ = .;
    .init_functions : {*src/misc.o(.init_functions)}
    __init_functions_end__ = .;
    . = ALIGN(4);
    __ram_size__ = . - ADDR(.text);
    
    .bss : {
        *src/*.o(.bss*)
    }
    . = ALIGN(4);
    __bss_size__ = . - ADDR(.bss);
    __size__ = . - ADDR(.text);

    /* DWARF debug sections.
       Symbols in the DWARF debugging sections are relative to the beginning
       of the section so we begin them at 0.  */

    /* DWARF 1 */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }

    /* GNU DWARF 1 extensions */
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }

    /* DWARF 1.1 and DWARF 2 */
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }

    /* DWARF 2 */
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }

    /DISCARD/ :
    {
        *(*)
    }
}
