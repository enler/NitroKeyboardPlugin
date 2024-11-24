OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

ENTRY(_start)

SECTIONS
{
        .text : {
            *source/*.o(.text*)
            *libc.a:libc_a-memcpy-stub.o(.text.memcpy)
            *libc.a:libc_a-memset.o(.text.memset)
            *libgcc.a:_divsi3.o(.text)
            *libgcc.a:_dvmd_tls.o(.text)
            *libnds9.a:biosCalls.o(.text.swiCopy)
            *libnds9.a:dynamicArray.o(.text.DynamicArrayGet)
            *libnds9.a:dynamicArray.o(.text.DynamicArrayInit)
            *libnds9.a:dynamicArray.o(.text.DynamicArraySet)
            *libnds9.a:dynamicArray.o(.text.DynamicArrayDelete)
            *libnds9.a:gl2d.o(.text.glBegin2D)
            *libnds9.a:gl2d.o(.text.glBoxFilled)
            *libnds9.a:gl2d.o(.text.glEnd2D)
            *libnds9.a:gl2d.o(.text.glLoadTileSet)
            *libnds9.a:gl2d.o(.text.glLoadSpriteSet)
            *libnds9.a:gl2d.o(.text.glSprite)
            *libnds9.a:gl2d.o(.text.glTexImage2D)
            *libnds9.a:gl2d.o(.text.SetOrtho)
            *libnds9.a:video.o(.text.vramRestoreBanks_EFG)
            *libnds9.a:video.o(.text.vramRestorePrimaryBanks)
            *libnds9.a:videoGL.o(.text.glBindTexture)
            *libnds9.a:videoGL.o(.text.glColorTableEXT)
            *libnds9.a:videoGL.o(.text.glGenTextures)
            *libnds9.a:videoGL.o(.text.glGetGlobals)
            *libnds9.a:videoGL.o(.text.glTexImage2D)
            *libnds9.a:videoGL.o(.text.removePaletteFromTexture.part.0)
            *libnds9.a:videoGL.o(.text.vramBlock_allocateBlock)
            *libnds9.a:videoGL.o(.text.vramBlock_allocateSpecial)
            *libnds9.a:videoGL.o(.text.vramBlock_Construct)
            *libnds9.a:videoGL.o(.text.vramBlock_deallocateBlock)
            *libnds9.a:videoGL.o(.text.vramBlock__allocateBlock)
            *libnds9.a:videoGL.o(.text.vramBlock__deallocateBlock.part.0)
            *libnds9.a:videoGL.o(.text.vramBlock_examineSpecial)
            *libnds9.a:videoGL.o(.text.vramBlock_init)
            *libnds9.a:videoGL.o(.text.vramGetBank)
            *libnds9.a:videoGL.o(.text.vramBlock_terminate)
            *libnds9.a:videoGL.o(.text.glAssignColorTable)
            *libnds9.a:videoGL.o(.text.glGetTexturePointer)
        }
        . = ALIGN(4);

        .rodata : { 
            *source/*.o(.rodata*)
            *libnds9.a:videoGL.o(.rodata)
        }
        . = ALIGN(4);

        .data : { 
            *source/*.o(.data)
            *libnds9.a:videoGL.o(.data.glGlob)
        }
        . = ALIGN(4);

        __init_functions_begin__ = .;
        .init_functions : {*source/misc.o(.init_functions)}
        __init_functions_end__ = .;
        . = ALIGN(4);
        __ram_size__ = . - ADDR(.text);
        
        .bss : { 
            *source/*.o(.bss*)
            *libnds9.a:gl2d.o(.bss.gCurrentTexture)
            *libnds9.a:gl2d.o(.bss.g_depth)
            *libnds9.a:videoGL.o(.bss.glGlobalData)
            *libnds9.a:videoGL.o(.bss.diffuse_ambient.1)
            *libnds9.a:videoGL.o(.bss.specular_emission.0)
        }
        __bss_size__ = . - ADDR(.bss);
        __size__ = . - ADDR(.text);

        /DISCARD/ :
        {
            *(*)
        }
}
