#include <nds/arm9/videoGL.h>
//---------------------------------------------------------------------------------
void __wrap_glInit_C(void) {
//---------------------------------------------------------------------------------
	int i;

//	powerOn(POWER_3D_CORE | POWER_MATRIX);	// enable 3D core & geometry engine

	glGlob = glGetGlobals();

	if( glGlob->isActive )
		return;

	// Allocate the designated layout for each memory block
	glGlob->vramBlocks[ 0 ] = vramBlock_Construct( (uint8*)VRAM_A, (uint8*)VRAM_B );
	glGlob->vramBlocks[ 1 ] = vramBlock_Construct( (uint8*)VRAM_E, (uint8*)VRAM_F );

	glGlob->vramLock[ 0 ] = 0;
	glGlob->vramLock[ 1 ] = 0;

	// init texture globals

	glGlob->clearColor = 0;

	glGlob->activeTexture = 0;
	glGlob->activePalette = 0;
	glGlob->texCount = 1;
	glGlob->palCount = 1;
	glGlob->deallocTexSize = 0;
	glGlob->deallocPalSize = 0;

	// Clean out all this crap
	DynamicArrayInit( &glGlob->texturePtrs, 16 );
	DynamicArrayInit( &glGlob->palettePtrs, 16 );
	DynamicArrayInit( &glGlob->deallocTex, 16 );
	DynamicArrayInit( &glGlob->deallocPal, 16 );

	for(i = 0; i < 16; i++) {
		DynamicArraySet( &glGlob->texturePtrs, i, (void*)0 );
		DynamicArraySet( &glGlob->palettePtrs, i, (void*)0 );
		DynamicArraySet( &glGlob->deallocTex, i, (void*)0 );
		DynamicArraySet( &glGlob->deallocPal, i, (void*)0 );
	}

	while (GFX_STATUS & (1<<27)); // wait till gfx engine is not busy

	// Clear the FIFO
	GFX_STATUS |= (1<<29);

	// Clear overflows from list memory
//	glResetMatrixStack();

	// prime the vertex/polygon buffers
	glFlush(0);

	// reset the control bits
	GFX_CONTROL = 0;

	GFX_TEX_FORMAT = 0;
	GFX_POLY_FORMAT = 0;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glGlob->isActive = 1;
}

//---------------------------------------------------------------------------------
void __wrap_glResetTextures(void) {
//---------------------------------------------------------------------------------
	int i;

	glGlob->activeTexture = 0;
	glGlob->activePalette = 0;
	glGlob->texCount = 1;
	glGlob->palCount = 1;
	glGlob->deallocTexSize = 0;
	glGlob->deallocPalSize = 0;

	// Any textures in use will be clean of all their data
	for(i = 0; i < (int)glGlob->texturePtrs.cur_size; i++) {
		gl_texture_data* texture = (gl_texture_data*)DynamicArrayGet( &glGlob->texturePtrs, i );
		if( texture ) {
			free( texture );
			DynamicArraySet(&glGlob->texturePtrs, i, (void*)0 );
		}
	}

	// Any palettes in use will be cleaned of all their data
	for( i = 0; i < (int)glGlob->palettePtrs.cur_size; i++ ) {
		gl_palette_data* palette = (gl_palette_data*)DynamicArrayGet( &glGlob->palettePtrs, i );
		if( palette ) {
			free( palette );
			DynamicArraySet( &glGlob->palettePtrs, i, (void*)0 );
		}
	}

	// Clean out both blocks
	for( i = 0; i < 2; i++ ) {
        vramBlock_terminate(glGlob->vramBlocks[i]);
        free(glGlob->vramBlocks[i]);
        glGlob->vramBlocks[i] = NULL;
    }

    DynamicArrayDelete( &glGlob->texturePtrs );
    DynamicArrayDelete( &glGlob->palettePtrs );
    DynamicArrayDelete( &glGlob->deallocTex );
    DynamicArrayDelete( &glGlob->deallocPal );

    memset( glGlob, 0, sizeof(gl_hidden_globals));
}