#include <nds/ndstypes.h>
#include "nitro/fs.h"
#include "keyboard.h"

#define EXTERNAL_GLYPH_WIDTH 16
#define EXTERNAL_GLYPH_HEIGHT 16
#define EXTERNAL_GLYPH_COUNT 64
#define EXTERNAL_FONT_TEXTURE_WIDTH 128
#define EXTERNAL_FONT_TEXTURE_HEIGHT 64

struct KeyGlyph {
    KeyCode charCode;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
};

typedef struct {
    u16 charCode;
    u8 palIdx;
    u8 advance;
    glImage *image;
} ExternalGlyph;

typedef struct {
    ExternalGlyph *glyphs;
    glImage *glyphImages;
    int textureId;
    int glyphCount;
    int glyphCapacity;
    bool (*GetGlyph)(u16 charCode, u8 *output, int *advance);
} ExternalFont;

typedef struct {
    glImage *keysGlyphs;
    int keysTextureId;
    int keyTexPalId;
    int glyphTexPalId;
    ExternalFont externalFont;
} KeyboardFont;

const struct KeyGlyph gDefaultKeyGlyphs[] = {
    { KEYCODE_BACKSPACE, 0, 51, 13, 7 },
    { KEYCODE_SHIFT, 0, 12, 25, 11 },
    { KEYCODE_ENTER, 25, 12, 29, 11 },
    { KEYCODE_CAPS_LOCK, 102, 0, 19, 11 },
    { KEYCODE_EXCLAMATION, 54, 12, 6, 10 },
    { KEYCODE_HASH, 60, 12, 6, 10 },
    { KEYCODE_DOLLAR, 30, 0, 6, 11 },
    { KEYCODE_PERCENT, 66, 12, 6, 10 },
    { KEYCODE_AMPERSAND, 72, 12, 6, 10 },
    { KEYCODE_QUOTE, 36, 0, 6, 11 },
    { KEYCODE_LEFT_PAREN, 42, 0, 6, 11 },
    { KEYCODE_RIGHT_PAREN, 48, 0, 6, 11 },
    { KEYCODE_ASTERISK, 108, 33, 6, 9 },
    { KEYCODE_PLUS, 78, 12, 6, 10 },
    { KEYCODE_COMMA, 13, 51, 6, 4 },
    { KEYCODE_MINUS, 12, 43, 6, 7 },
    { KEYCODE_PERIOD, 19, 51, 6, 4 },
    { KEYCODE_SLASH, 54, 0, 6, 11 },
    { KEYCODE_0, 84, 12, 6, 10 },
    { KEYCODE_1, 90, 12, 6, 10 },
    { KEYCODE_2, 96, 12, 6, 10 },
    { KEYCODE_3, 102, 12, 6, 10 },
    { KEYCODE_4, 108, 12, 6, 10 },
    { KEYCODE_5, 114, 12, 6, 10 },
    { KEYCODE_6, 120, 12, 6, 10 },
    { KEYCODE_7, 0, 23, 6, 10 },
    { KEYCODE_8, 6, 23, 6, 10 },
    { KEYCODE_9, 12, 23, 6, 10 },
    { KEYCODE_COLON, 120, 33, 6, 8 },
    { KEYCODE_SEMICOLON, 0, 43, 6, 8 },
    { KEYCODE_LESS, 60, 0, 6, 11 },
    { KEYCODE_EQUAL, 6, 43, 6, 8 },
    { KEYCODE_GREATER, 66, 0, 6, 11 },
    { KEYCODE_QUESTION, 18, 23, 6, 10 },
    { KEYCODE_AT, 24, 23, 6, 10 },
    { KEYCODE_A, 30, 23, 6, 10 },
    { KEYCODE_B, 36, 23, 6, 10 },
    { KEYCODE_C, 42, 23, 6, 10 },
    { KEYCODE_D, 48, 23, 6, 10 },
    { KEYCODE_E, 54, 23, 6, 10 },
    { KEYCODE_F, 60, 23, 6, 10 },
    { KEYCODE_G, 66, 23, 6, 10 },
    { KEYCODE_H, 72, 23, 6, 10 },
    { KEYCODE_I, 78, 23, 6, 10 },
    { KEYCODE_J, 84, 23, 6, 10 },
    { KEYCODE_K, 90, 23, 6, 10 },
    { KEYCODE_L, 96, 23, 6, 10 },
    { KEYCODE_M, 102, 23, 6, 10 },
    { KEYCODE_N, 108, 23, 6, 10 },
    { KEYCODE_O, 114, 23, 6, 10 },
    { KEYCODE_P, 120, 23, 6, 10 },
    { KEYCODE_Q, 0, 33, 6, 10 },
    { KEYCODE_R, 6, 33, 6, 10 },
    { KEYCODE_S, 12, 33, 6, 10 },
    { KEYCODE_T, 18, 33, 6, 10 },
    { KEYCODE_U, 24, 33, 6, 10 },
    { KEYCODE_V, 30, 33, 6, 10 },
    { KEYCODE_W, 36, 33, 6, 10 },
    { KEYCODE_X, 42, 33, 6, 10 },
    { KEYCODE_Y, 48, 33, 6, 10 },
    { KEYCODE_Z, 54, 33, 6, 10 },
    { KEYCODE_LEFT_BRACKET, 72, 0, 6, 11 },
    { KEYCODE_RIGHT_BRACKET, 78, 0, 6, 11 },
    { KEYCODE_CARET, 84, 0, 6, 11 },
    { KEYCODE_UNDERSCORE, 25, 51, 6, 2 },
    { KEYCODE_a, 18, 43, 6, 7 },
    { KEYCODE_b, 60, 33, 6, 10 },
    { KEYCODE_c, 24, 43, 6, 7 },
    { KEYCODE_d, 66, 33, 6, 10 },
    { KEYCODE_e, 30, 43, 6, 7 },
    { KEYCODE_f, 72, 33, 6, 10 },
    { KEYCODE_g, 36, 43, 6, 7 },
    { KEYCODE_h, 78, 33, 6, 10 },
    { KEYCODE_i, 84, 33, 6, 10 },
    { KEYCODE_j, 90, 33, 6, 10 },
    { KEYCODE_k, 96, 33, 6, 10 },
    { KEYCODE_l, 102, 33, 6, 10 },
    { KEYCODE_m, 42, 43, 6, 7 },
    { KEYCODE_n, 48, 43, 6, 7 },
    { KEYCODE_o, 54, 43, 6, 7 },
    { KEYCODE_p, 60, 43, 6, 7 },
    { KEYCODE_q, 66, 43, 6, 7 },
    { KEYCODE_r, 72, 43, 6, 7 },
    { KEYCODE_s, 78, 43, 6, 7 },
    { KEYCODE_t, 114, 33, 6, 9 },
    { KEYCODE_u, 84, 43, 6, 7 },
    { KEYCODE_v, 90, 43, 6, 7 },
    { KEYCODE_w, 96, 43, 6, 7 },
    { KEYCODE_x, 102, 43, 6, 7 },
    { KEYCODE_y, 108, 43, 6, 7 },
    { KEYCODE_z, 114, 43, 6, 7 },
    { KEYCODE_LEFT_BRACE, 90, 0, 6, 11 },
    { KEYCODE_RIGHT_BRACE, 96, 0, 6, 11 },
    { KEYCODE_TILDE, 0, 0, 6, 12 },
    { KEYCODE_LANGUAGE_CHINISE, 6, 0, 12, 12},
    { KEYCODE_LANGUAGE_ENGLISH, 18, 0, 12, 12}
};

KeyboardFont *gKeyboardFont;
void InitExternalFont();

void InitKeyboardFont() {
    gKeyboardFont = malloc(sizeof(KeyboardFont));
    memset(gKeyboardFont, 0, sizeof(KeyboardFont));

    gKeyboardFont->keysGlyphs = malloc(sizeof(glImage) * ARRAY_SIZE(gDefaultKeyGlyphs));
    u8 *keysTex = NULL;
    FSFile file;
    FS_InitFile(&file);
    if (FS_OpenFile(&file, "keyboard/keys.tex")) {
        u32 fileLen = FS_GetLength(&file);
        keysTex = malloc(fileLen);
        if (keysTex)
            FS_ReadFile(&file, keysTex, fileLen);
        FS_CloseFile(&file);
    }

    int * coords = malloc(sizeof(int) * ARRAY_SIZE(gDefaultKeyGlyphs) * 4);
    for (int i = 0; i < ARRAY_SIZE(gDefaultKeyGlyphs); i++) {
        coords[i * 4] = gDefaultKeyGlyphs[i].x;
        coords[i * 4 + 1] = gDefaultKeyGlyphs[i].y;
        coords[i * 4 + 2] = gDefaultKeyGlyphs[i].width;
        coords[i * 4 + 3] = gDefaultKeyGlyphs[i].height;
    }

    gKeyboardFont->keysTextureId = glLoadSpriteSet(gKeyboardFont->keysGlyphs, 
                    ARRAY_SIZE(gDefaultKeyGlyphs), 
                    coords,
                    GL_RGB4,
                    TEXTURE_SIZE_128,
                    TEXTURE_SIZE_64,
                    GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|TEXGEN_OFF|GL_TEXTURE_COLOR0_TRANSPARENT,
                    4,
                    NULL,
                    keysTex);

    free(coords);
    free(keysTex);

    InitExternalFont();
}

void InitExternalFont() {
    ExternalFont *externalFont = &gKeyboardFont->externalFont;
    externalFont->glyphs = malloc(EXTERNAL_GLYPH_COUNT * sizeof(ExternalGlyph));
    memset(externalFont->glyphs, 0, EXTERNAL_GLYPH_COUNT * sizeof(ExternalGlyph));
    externalFont->glyphCapacity = EXTERNAL_GLYPH_COUNT;
    externalFont->glyphImages = malloc(externalFont->glyphCapacity * sizeof(glImage) / 2);
    void *textureData = malloc(EXTERNAL_FONT_TEXTURE_WIDTH * EXTERNAL_FONT_TEXTURE_HEIGHT / 4);
    memset(textureData, 0, 128 * 64 / 4);
    externalFont->textureId = glLoadTileSet(externalFont->glyphImages,
                                            EXTERNAL_GLYPH_WIDTH,
                                            EXTERNAL_GLYPH_HEIGHT,
                                            128,
                                            64,
                                            GL_RGB4,
                                            TEXTURE_SIZE_128,
                                            TEXTURE_SIZE_64,
                                            GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
                                            0,
                                            NULL,
                                            textureData);

    for (int i = 0; i < externalFont->glyphCapacity / 2; i++) {
        externalFont->glyphs[i * 2].charCode = 0;
        externalFont->glyphs[i * 2].palIdx = 0;
        externalFont->glyphs[i * 2].image = &externalFont->glyphImages[i];

        externalFont->glyphs[i * 2 + 1].charCode = 0;
        externalFont->glyphs[i * 2 + 1].palIdx = 1;
        externalFont->glyphs[i * 2 + 1].image =  &externalFont->glyphImages[i];
    }
    free(textureData);
}

void DeinitKeyboardFont() {
    free(gKeyboardFont->keysGlyphs);
    free(gKeyboardFont->externalFont.glyphs);
    free(gKeyboardFont->externalFont.glyphImages);
    free(gKeyboardFont);
}

glImage *GetDefaultGlyph(KeyCode code) {
    // binary search in gDefaultKeyGlyphs, and get the glyph from keyGlyphs
    int left = 0;
    int right = ARRAY_SIZE(gDefaultKeyGlyphs) - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (gDefaultKeyGlyphs[mid].charCode == code) {
            return &gKeyboardFont->keysGlyphs[mid];
        } else if (gDefaultKeyGlyphs[mid].charCode < code) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return NULL;
}

void SetDefaultKeysPalette(int palId) {
    glSetActiveTexture(gKeyboardFont->keysTextureId);
    glAssignColorTable(0, palId);
}

void CreateExternalFontPalette(int * paletteIds, u16 textColor, u16 bgColor) {
    u16 palettes[EXTERNAL_FONT_PALETTE_SIZE][4] = {
        {bgColor, textColor, bgColor, textColor},
        {bgColor, bgColor, textColor, textColor}
    };
    for (int i = 0; i < EXTERNAL_FONT_PALETTE_SIZE; i++) {
        glGenTextures(1, &paletteIds[i]);
        glBindTexture(0, paletteIds[i]);
        glColorTableEXT(0, 0, 4, 0, 0, palettes[i]);
    }
}

void SetGetGlyph(bool (*GetGlyph)(u16 charCode, u8 *output, int *advance)) {
    gKeyboardFont->externalFont.GetGlyph = GetGlyph;
}

bool LoadExternalGlyph(u16 charCode, int index) {
    ExternalFont *externalFont = &gKeyboardFont->externalFont;
    if (!externalFont->GetGlyph) {
        return false;
    }
    u8 glyphData[EXTERNAL_GLYPH_WIDTH * EXTERNAL_GLYPH_HEIGHT / 4];
    int advance;
    if (!externalFont->GetGlyph(charCode, glyphData, &advance)) {
        return false;
    }
    externalFont->glyphs[index].charCode = charCode;
    externalFont->glyphs[index].advance = advance;
    u32 * texBuffer = glGetTexturePointer(externalFont->textureId);
    u8 vramACR = VRAM_A_CR;
    VRAM_A_CR = VRAM_ENABLE;
    u32 mask = 0x55555555 << externalFont->glyphs[index].palIdx;
    glImage *image = externalFont->glyphs[index].image;
    for (int i = 0; i < sizeof(glyphData) / 4; i++) {
        int texOffset = ((image->v_off + i) * 128 + image->u_off) / 16;
        texBuffer[texOffset] = texBuffer[texOffset] & ~mask | ((u32 *)glyphData)[i] << externalFont->glyphs[index].palIdx;
    }
    VRAM_A_CR = vramACR;
    return true;
}

bool GetExternalGlyph(u16 charCode, glImage *glyphImage, int *palIndex, int *advance) {
    // externalFont->glyphs is lru cache, so we need to check if the glyph is already loaded
    ExternalFont *externalFont = &gKeyboardFont->externalFont;
    for (int i = 0; i < externalFont->glyphCount; i++) {
        if (externalFont->glyphs[i].charCode == charCode) {
            *glyphImage = *externalFont->glyphs[i].image;
            *palIndex = externalFont->glyphs[i].palIdx;
            *advance = externalFont->glyphs[i].advance;
            // if loaded, get the glyph from the cache, and bring it to the front, then return true
            ExternalGlyph temp = externalFont->glyphs[i];
            for (int j = i; j > 0; j--) {
                externalFont->glyphs[j] = externalFont->glyphs[j - 1];
            }
            externalFont->glyphs[0] = temp;
            return true;
        }
    }

    int loadedIndex = externalFont->glyphCount < externalFont->glyphCapacity ? externalFont->glyphCount : externalFont->glyphCapacity - 1;
    if (!LoadExternalGlyph(charCode, loadedIndex)) {
        return false;
    }
    *glyphImage = *externalFont->glyphs[loadedIndex].image;
    *palIndex = externalFont->glyphs[loadedIndex].palIdx;
    *advance = externalFont->glyphs[loadedIndex].advance;
    ExternalGlyph temp = externalFont->glyphs[loadedIndex];
    for (int i = loadedIndex; i > 0; i--) {
        externalFont->glyphs[i] = externalFont->glyphs[i - 1];
    }
    externalFont->glyphs[0] = temp;
    if (externalFont->glyphCount < externalFont->glyphCapacity) {
        externalFont->glyphCount++;
    }
    return true;
}