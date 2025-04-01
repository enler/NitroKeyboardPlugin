#include <nds/ndstypes.h>
#include <nds/arm9/background.h>
#include <stddef.h>
#include "nitro/font.h"
#include "nitro/fs.h"
#include "nitro/heap.h"
#include "nitro/pad.h"
#include "hook.h"
#include "keyboard.h"

#define IMPORT __attribute__((naked))

#define max(a,b) ((a) > (b) ? (a) : (b))

IMPORT bool TP_GetUserInfo(void *calibrate) {}
IMPORT void TP_SetCalibrateParam(const void *param) {}

static void *gNamingContextAlt;

static u16 gBppConvTable[256];

void (*Orig_NamingBegin)(void **);
void (*Orig_NamingQuit)(void **);

void InitBppConvTable() {
    for (int i = 0; i < 256; i++) {
        u16 value = 0;
        for (int j = 0; j < 8; j++) {
            if (i & (1 << j))
                value |= 0x4000 >> (j * 2);
        }
        gBppConvTable[i] = value;
    }
}

void Hook_NamingBegin(void *namingContext) {
    gNamingContextAlt = namingContext;
    Orig_NamingBegin(namingContext);
}

void Hook_NamingQuit(void *namingContext) {
    gNamingContextAlt = NULL;
    Orig_NamingQuit(namingContext);
}

static void* Alloc(u32 size) {
    return FndAllocFromExpHeapEx(*(void**)0x020FADD4, size, -4);
}

static void Free(void *ptr) {
    FndFreeToExpHeap(*(void**)0x020FADD4, ptr);
}

static void OnOverlayLoaded() {
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    InitBppConvTable();

    Orig_NamingBegin = *(void **)0x02094B14;
    Orig_NamingQuit = *(void **)0x02094B24;

    *(void **)0x02094B14 = Hook_NamingBegin;
    *(void **)0x02094B24 = Hook_NamingQuit;

    u32 calibrate[2];
    TP_GetUserInfo(&calibrate);
    TP_SetCalibrateParam(calibrate);
}

static bool ShouldShowKeyboard() {
    return KEY_PRESSED(KEY_R | KEY_X) && gNamingContextAlt;
}

static int GetMaxInputLength() {
    return *(u32*)0x020C18F4;
}

static void ConvertGlyph(const u8 *glyphCell, u8 *output, s32 cellWidth, s32 cellHeight, const NitroGlyphMetrics* metrics) {
    int i = 0;
    int remainBitCount = 8;
    int bitOffset = 0;
    u32 line = 0;
    u32 mask = ((1 << metrics->width) - 1) << (16 - metrics->width - metrics->left);

    while (i < cellHeight) {
        int rightShift = max(remainBitCount - (cellWidth - bitOffset), 0);
        int bitsLength = (remainBitCount - rightShift);
        int leftShift = 16 - bitOffset - bitsLength;
        line = (*glyphCell >> rightShift << leftShift) | line;
        bitOffset += bitsLength;
        remainBitCount -= bitsLength;
        if (remainBitCount == 0) {
            remainBitCount = 8;
            glyphCell++;
        }
        if (bitOffset == cellWidth) {
            if (metrics->left >= 0)
                line >>= metrics->left;
            else
                line <<= metrics->left;
            line = line & mask;
            u16 lineLeft = gBppConvTable[line >> 8];
            u16 lineRight = gBppConvTable[line & 0xFF];
            output[0] = lineLeft & 0xFF;
            output[1] = lineLeft >> 8;
            output[2] = lineRight & 0xFF;
            output[3] = lineRight >> 8;
            output += 4;
            line = 0;
            bitOffset = 0;
            i++;
        }
    }
}

static bool LoadGlyph(u16 charCode, u8 *output, int *advance) {
    const u8 *glyphCell;
    NitroGlyphMetrics metrics;
    const NitroFontInfoSection *font = *(NitroFontInfoSection **)0x20fed74;

    memset(output, 0, 64);
    if (LoadGlyphData(font, charCode, &glyphCell, &metrics)) {
        int top = (16 - font->glyphSection->cellHeight) / 2;
        ConvertGlyph(glyphCell, output + top * 4, font->glyphSection->cellWidth, font->glyphSection->cellHeight, &metrics);
        *advance = metrics.advance;
        return true;
    }
    return false;
}

static bool KeycodeToChar(u16 keycode, u16 *output) {
    if (keycode == KEYCODE_AMPERSAND || 
        keycode == KEYCODE_LESS ||
        keycode == KEYCODE_FULLWIDTH_QUOTE ||
        keycode == KEYCODE_FULLWIDTH_DOUBLE_QUOTE )
        return false;
    *output = keycode;
    return true;
}

static bool CanContinueInput(u16 *inputText, int length, u16 nextChar) {
    return true;
}

static void OnInputFinished(u16 *inputText, int length, bool isCanceled) {
    u32 *currentPos = (u32 *)0x020C18F8;
    u32 *nameCharsUtf8 = (u32 *)0x020C1A68;
    u32 *needsScreenUpdate = ((u32 *)gNamingContextAlt + 1);
    if (!isCanceled) {
        for (int i = 0; i < length; i++) {
            u16 charCode = inputText[i];
            if (charCode < 0x80) 
                nameCharsUtf8[i] = charCode;
            else if (charCode < 0x800) {
                nameCharsUtf8[i] = ((0xC0 | ((charCode >> 6) & 0x1F)) << 8) | 
                                    (0x80 | (charCode & 0x3F));
            }
            else {
                nameCharsUtf8[i] = ((0xE0 | ((charCode >> 12) & 0x0F)) << 16) |
                                   ((0x80 | ((charCode >> 6) & 0x3F)) << 8) |
                                    (0x80 | (charCode & 0x3F));
            }
            *needsScreenUpdate = 1;
        }
        *currentPos = length;
    }
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