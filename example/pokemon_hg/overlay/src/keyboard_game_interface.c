#include <nds/ndstypes.h>
#include <nds/arm9/background.h>
#include <stddef.h>
#include "nitro/fs.h"
#include "nitro/heap.h"
#include "nitro/pad.h"
#include "keyboard.h"

#define IMPORT __attribute__((naked))

IMPORT void DrawTextToWindow(u8 *, u16 *, u32, u32, u32, u32, u32, u32) {}
IMPORT void FillTextWindow(u8 *, u8) {}
IMPORT void MoveCharCursor(u8 *, u16, u16) {}
IMPORT int OnHandleNamingEvent(u32, u32) {}
IMPORT int DeinitNamingContext(u32, u32) {}


extern u8 *HW_ARENA_INFO_BUF[];

typedef struct {
    u32 glyphOffset;
    u32 widthOffset;
    u32 glyphNum;
} PMFontHeader;

typedef struct {
    FSFile fontFile;
    PMFontHeader header;
    void *namingContextAlt;
    int glyphNum;
} GameInternalContext;

typedef struct {
    u16 charCode;
    u8 width;
    u8 flag;
    u8 data[12 * 12 / 8];
} CustomGlyphEntry;

static GameInternalContext gInternalContext;

static u16 gBppConvTable[256];

const struct KeycodeConvItem KeycodeConvTable[] = {
    {KEYCODE_SPACE, 0x01DE},
    {KEYCODE_EXCLAMATION, 0x01AB},
    {KEYCODE_HASH, 0x01C0},
    {KEYCODE_DOLLAR, 0x01A8},
    {KEYCODE_PERCENT, 0x01D2},
    {KEYCODE_AMPERSAND, 0x01C2},
    {KEYCODE_LEFT_PAREN, 0x01B9},
    {KEYCODE_RIGHT_PAREN, 0x01BA},
    {KEYCODE_ASTERISK, 0x01BF},
    {KEYCODE_PLUS, 0x01BD},
    {KEYCODE_COMMA, 0x01AD},
    {KEYCODE_MINUS, 0x01BE},
    {KEYCODE_PERIOD, 0x01AE},
    {KEYCODE_SLASH, 0x01B1},
    {KEYCODE_COLON, 0x01C4},
    {KEYCODE_SEMICOLON, 0x01C5},
    {KEYCODE_EQUAL, 0x01C1},
    {KEYCODE_QUESTION, 0x01AC},
    {KEYCODE_AT, 0x01D0},
    {KEYCODE_UNDERSCORE, 0x01E9},
    {KEYCODE_TILDE, 0x01C3},
    {KEYCODE_FULLWIDTH_SPACE, 0x01FB},
    {KEYCODE_CHINESE_PERIOD, 0x00E4},
    {KEYCODE_FULLWIDTH_EXCLAMATION, 0x00E1},
    {KEYCODE_FULLWIDTH_PERCENT, 0x0106},
    {KEYCODE_FULLWIDTH_AMPERSAND, 0x0120},
    {KEYCODE_FULLWIDTH_LEFT_PAREN, 0x00EC},
    {KEYCODE_FULLWIDTH_RIGHT_PAREN, 0x00ED},
    {KEYCODE_FULLWIDTH_PLUS, 0x00F0},
    {KEYCODE_FULLWIDTH_COMMA, 0x00F9},
    {KEYCODE_FULLWIDTH_PERIOD, 0x00F8},
    {KEYCODE_FULLWIDTH_SLASH, 0x00E7},
    {KEYCODE_FULLWIDTH_COLON, 0x00F6},
    {KEYCODE_FULLWIDTH_SEMICOLON, 0x00F7},
    {KEYCODE_FULLWIDTH_LESS, 0x01FC},
    {KEYCODE_FULLWIDTH_EQUAL, 0x00F4},
    {KEYCODE_FULLWIDTH_GREATER, 0x01FD},
    {KEYCODE_FULLWIDTH_QUESTION, 0x00E2},
    {KEYCODE_FULLWIDTH_AT, 0x0104},
    {KEYCODE_FULLWIDTH_UNDERSCORE, 0x01EA},
    {KEYCODE_FULLWIDTH_TILDE, 0x00F5},
};

void InitBppConvTable() {
    for (int i = 0; i < 256; i++)
    {
        u16 value = 0;
        for (int j = 0; j < 8; j++)
        {
            if (i & (1 << j))
                value |= 0x01 << (j * 2);
        }
        gBppConvTable[i] = value;
    }
}

static int Hook_OnHandleNamingEvent(u32 a1, u32 a2) {
    int result = OnHandleNamingEvent(a1, a2);
    gInternalContext.namingContextAlt = *(void**)(a1 + 0x1C);
    return result;
}

static int Hook_DeinitNamingContext(u32 a1, u32 a2){
    int result = DeinitNamingContext(a1, a2);
    gInternalContext.namingContextAlt = NULL;
    return result;
}

static void InitInternalContext() {
    static bool initialized = false;
    if (initialized)
        return;
    FS_InitFile(&gInternalContext.fontFile);
    if (FS_OpenFile(&gInternalContext.fontFile, "a/0/1/6")) {
        FS_SeekFile(&gInternalContext.fontFile, 0x84, 0);
        FS_ReadFile(&gInternalContext.fontFile, &gInternalContext.header, sizeof(PMFontHeader));
    }
    *(void**)0x2101D94 = Hook_OnHandleNamingEvent;
    *(void**)0x2101D98 = Hook_DeinitNamingContext;
    initialized = true;
}

static void* Alloc(u32 size) {
    if (HW_ARENA_INFO_BUF[0] + size <= HW_ARENA_INFO_BUF[9]) {
        return HW_ARENA_INFO_BUF[0];
    }
    return NULL;
}

static void Free(void *ptr) {
    // do nothing
}

static void OnOverlayLoaded() {
    InitInternalContext();
}

static bool ShouldShowKeyboard() {
    return KEY_PRESSED(KEY_R | KEY_X) && gInternalContext.namingContextAlt;
}

static int GetMaxInputLength() {
    if (gInternalContext.namingContextAlt) {
        return ((u32*)gInternalContext.namingContextAlt)[3];
    }
    return 5;
}

u32 Reverse2bitUnits(u32 pixelData) {
    u32 result = 0;
    for (int i = 0; i < 16; ++i) {
        u8 two_bits = (pixelData >> (i * 2)) & 0x3;
        result |= (two_bits << ((15 - i) * 2));
    }
    return result;
}

static bool LoadGlyph(u16 charCode, u8 *output, int *advance) {
    charCode--;
    u16 glyphData[16 * 16 / 4 / sizeof(u16)];
    u8 width;
    FS_SeekFile(&gInternalContext.fontFile, 0x84 + gInternalContext.header.glyphOffset + charCode * 16 * 16 / 4, 0);
    FS_ReadFile(&gInternalContext.fontFile, glyphData, sizeof(glyphData));
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 8; j++) {
            u32 pixelData = (glyphData[i * 16 + j] << 16) | glyphData[i * 16 + j + 8];
            pixelData = (pixelData & 0x55555555) & ((pixelData >> 1) ^ pixelData);
            pixelData = Reverse2bitUnits(pixelData);
            memcpy(output, &pixelData, sizeof(u32));
            output += 4;
        }
    }
    if (charCode <= gInternalContext.header.glyphNum) {
        FS_SeekFile(&gInternalContext.fontFile, 0x84 + gInternalContext.header.widthOffset + charCode, 0);
        FS_ReadFile(&gInternalContext.fontFile, &width, sizeof(u8));
        *advance = width;
    }
    else {
        *advance = 12;
    }
    
    return true;
}

static bool KeycodeToChar(u16 keycode, u16 *output) {
    if (keycode >= KEYCODE_0 && keycode <= KEYCODE_9) {
        *output = keycode - KEYCODE_0 + 0x0121;
        return true;
    }
    else if (keycode >= KEYCODE_A && keycode <= KEYCODE_Z) {
        *output = keycode - KEYCODE_A + 0x012B;
        return true;
    }
    else if (keycode >= KEYCODE_a && keycode <= KEYCODE_z) {
        *output = keycode - KEYCODE_a + 0x0145;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_0 && keycode <= KEYCODE_FULLWIDTH_9) {
        *output = keycode - KEYCODE_FULLWIDTH_0 + 0x00A2;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_A && keycode <= KEYCODE_FULLWIDTH_Z) {
        *output = keycode - KEYCODE_FULLWIDTH_A + 0x00AC;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_a && keycode <= KEYCODE_FULLWIDTH_z) {
        *output = keycode - KEYCODE_FULLWIDTH_a + 0x00C6;
        return true;
    }
    int charCode = FindCustomCharCode(KeycodeConvTable, ARRAY_SIZE(KeycodeConvTable), keycode);
    if (charCode != -1) {
        *output = charCode;
        return true;
    }
    return false;
}

static bool CanContinueInput(u16 *inputText, int length, u16 nextChar) {
    return true;
}

void OnInputFinished(u16 *inputText, int length, bool isCanceled) {
    u8 *namingContext = (u8 *)gInternalContext.namingContextAlt;
    if (!namingContext)
        return;
    if (!isCanceled) {
        u16 *name = (u16*)(namingContext + 216);
        memcpy(name, inputText, length * sizeof(u16));
        name[length] = 0xFFFF;
        u16 *nameLength = (u16*)(namingContext + 344);
        *nameLength = length;
        FillTextWindow(namingContext + 1000, 1);
        DrawTextToWindow(namingContext + 1000, name, 0, 0, 12, 0, 0x000E0F01, 0);
        MoveCharCursor(namingContext + 868, *nameLength, *(u32*)(namingContext + 12));
    }

    int bg0H = *(int *)(namingContext + 0x468);
    int bg0V = *(int *)(namingContext + 0x46C);
    REG_BG0HOFS = 512 + bg0H;
    REG_BG0VOFS = 512 + bg0V;
    return;
}

KeyboardGameInterface * GetKeyboardGameInterface() {
    static KeyboardGameInterface gameInterface = {
        .Alloc = Alloc,
        .Free = Free,
        .OnOverlayLoaded = OnOverlayLoaded,
        .ShouldShowKeyboard = ShouldShowKeyboard,
        .GetMaxInputLength = GetMaxInputLength,
        .LoadGlyph = LoadGlyph,
        .KeycodeToChar = KeycodeToChar,
        .CanContinueInput = CanContinueInput,
        .OnInputFinished = OnInputFinished
    };
    return &gameInterface;
}