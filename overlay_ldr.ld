OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)

SECTIONS
{
        .text : {
            *overlay_ldr/*.o(.text*)
        }
        . = ALIGN(4);

        .rodata : { 
            *overlay_ldr/*.o(.rodata*)
        }
        . = ALIGN(4);

        .data : { 
            *overlay_ldr/*.o(.data*)
        }
        . = ALIGN(4);
        
        .bss : { 
            *overlay_ldr/*.o(.bss*)
        }



        /DISCARD/ :
        {
            *(*)
        }
}
