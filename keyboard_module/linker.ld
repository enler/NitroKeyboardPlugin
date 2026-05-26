OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(KeyboardModule_Entry)

SECTIONS
{
    __module_link_address__ = ADDR(.text);

    .text : {
        KEEP(*build/keyboard_module/module_entry.o(.text.entry))
        *build/keyboard_module/*.o(.text*)
        *nitro.a:import.o(.text)
        *libgcc.a:_divsi3.o(.text)
        *libgcc.a:_dvmd_tls.o(.text)
        *libcalico_ds9.a:arm-shims.32.o(.text.armIrqLockByPsr)
        *libcalico_ds9.a:arm-shims.32.o(.text.armIrqUnlockByPsr)
        *libcalico_ds9.a:bios.o(.text.svcCpuSet)
        *libcalico_ds9.a:bios.o(.text.svcWaitByLoop)
        *libnds9.a:dynamicArray.o(.text.DynamicArrayDelete)
        *libnds9.a:dynamicArray.o(.text.DynamicArrayGet)
        *libnds9.a:dynamicArray.o(.text.DynamicArrayInit)
        *libnds9.a:dynamicArray.o(.text.DynamicArraySet)
        *libnds9.a:gl2d.o(.text.SetOrtho)
        *libnds9.a:gl2d.o(.text.glBegin2D)
        *libnds9.a:gl2d.o(.text.glBoxFilled)
        *libnds9.a:gl2d.o(.text.glEnd2D)
        *libnds9.a:gl2d.o(.text.glLine)
        *libnds9.a:gl2d.o(.text.glLoadSpriteSet)
        *libnds9.a:gl2d.o(.text.glLoadTileSet)
        *libnds9.a:gl2d.o(.text.glSprite)
        *libnds9.a:gl2d.o(.text.glTexImage2D)
        *libnds9.a:video.o(.text.vramRestoreBanks_EFG)
        *libnds9.a:video.o(.text.vramRestorePrimaryBanks)
        *libnds9.a:videoGL.o(.text.glAssignColorTable)
        *libnds9.a:videoGL.o(.text.glBindTexture)
        *libnds9.a:videoGL.o(.text.glColorTableEXT)
        *libnds9.a:videoGL.o(.text.glGenTextures)
        *libnds9.a:videoGL.o(.text.glGetGlobals)
        *libnds9.a:videoGL.o(.text.glGetTexturePointer)
        *libnds9.a:videoGL.o(.text.glTexImage2D)
        *libnds9.a:videoGL.o(.text.removePaletteFromTexture.part.0)
        *libnds9.a:videoGL.o(.text.vramBlock_Construct)
        *libnds9.a:videoGL.o(.text.vramBlock__allocateBlock)
        *libnds9.a:videoGL.o(.text.vramBlock__deallocateBlock.part.0)
        *libnds9.a:videoGL.o(.text.vramBlock_allocateBlock)
        *libnds9.a:videoGL.o(.text.vramBlock_allocateSpecial)
        *libnds9.a:videoGL.o(.text.vramBlock_deallocateBlock)
        *libnds9.a:videoGL.o(.text.vramBlock_examineSpecial)
        *libnds9.a:videoGL.o(.text.vramBlock_init)
        *libnds9.a:videoGL.o(.text.vramBlock_terminate)
        *libnds9.a:videoGL.o(.text.vramGetBank)
    }
    . = ALIGN(4);

    .rodata : {
        *build/keyboard_module/*.o(.rodata*)
        *libnds9.a:videoGL.o(.rodata)
    }
    . = ALIGN(4);

    .data : {
        *build/keyboard_module/*.o(.data*)
        *libnds9.a:videoGL.o(.data.glGlob)
    }
    . = ALIGN(4);

    __module_image_size__ = . - ADDR(.text);

    .bss : {
        *build/keyboard_module/*.o(.bss*)
        *libnds9.a:gl2d.o(.bss.gCurrentTexture)
        *libnds9.a:gl2d.o(.bss.g_depth)
        *libnds9.a:videoGL.o(.bss.diffuse_ambient.1)
        *libnds9.a:videoGL.o(.bss.glGlobalData)
        *libnds9.a:videoGL.o(.bss.specular_emission.0)
        *libnds9.a:videoGL_base.o(.bss.glCurClearColor)
        *(COMMON)
    }
    . = ALIGN(4);
    __module_bss_size__ = . - ADDR(.bss);
    __module_size__ = . - ADDR(.text);

    /DISCARD/ :
    {
        *(*)
    }
}
