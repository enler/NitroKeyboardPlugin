#include <nds/ndstypes.h>
#include <nds/arm9/background.h>
#include <stddef.h>
#include "nitro/fs.h"
#include "nitro/heap.h"
#include "nitro/pad.h"
#include "hook.h"
#include "keyboard.h"


#define IMPORT __attribute__((naked))

typedef struct {
    FSFile fontFile;
    void *tpHandwritingContextAlt;
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

static u32 (*Orig_HandleTPHandwriting)(void *arg);

extern u8 gUsingBoldFont;

const struct KeycodeConvItem KeycodeConvTable[] = {
    {KEYCODE_FULLWIDTH_SPACE, 0x8140},
    {KEYCODE_CHINESE_PERIOD, 0x8142},
    {KEYCODE_FULLWIDTH_EXCLAMATION, 0x8149},
    {KEYCODE_FULLWIDTH_HASH, 0x8194},
    {KEYCODE_FULLWIDTH_DOLLAR, 0x8190},
    {KEYCODE_FULLWIDTH_PERCENT, 0x8193},
    {KEYCODE_FULLWIDTH_AMPERSAND, 0x8195},
    {KEYCODE_FULLWIDTH_LEFT_PAREN, 0x8169},
    {KEYCODE_FULLWIDTH_RIGHT_PAREN, 0x816A},
    {KEYCODE_FULLWIDTH_ASTERISK, 0x8196},
    {KEYCODE_FULLWIDTH_PLUS, 0x817B},
    {KEYCODE_FULLWIDTH_COMMA, 0x8143},
    {KEYCODE_FULLWIDTH_MINUS, 0x817C},
    {KEYCODE_FULLWIDTH_PERIOD, 0x8144},
    {KEYCODE_FULLWIDTH_SLASH, 0x815E},
    {KEYCODE_FULLWIDTH_COLON, 0x8146},
    {KEYCODE_FULLWIDTH_SEMICOLON, 0x8147},
    {KEYCODE_FULLWIDTH_LESS, 0x8183},
    {KEYCODE_FULLWIDTH_EQUAL, 0x8181},
    {KEYCODE_FULLWIDTH_GREATER, 0x8184},
    {KEYCODE_FULLWIDTH_QUESTION, 0x8148},
    {KEYCODE_FULLWIDTH_AT, 0x8197},
    {KEYCODE_FULLWIDTH_LEFT_BRACKET, 0x816D},
    {KEYCODE_FULLWIDTH_BACKSLASH, 0x815F},
    {KEYCODE_FULLWIDTH_RIGHT_BRACKET, 0x816E},
    {KEYCODE_FULLWIDTH_CARET, 0x814F},
    {KEYCODE_FULLWIDTH_UNDERSCORE, 0x8151},
    {KEYCODE_FULLWIDTH_GRAVE, 0x814D},
    {KEYCODE_FULLWIDTH_LEFT_BRACE, 0x816F},
    {KEYCODE_FULLWIDTH_PIPE, 0x8162},
    {KEYCODE_FULLWIDTH_RIGHT_BRACE, 0x8170},
    {KEYCODE_FULLWIDTH_TILDE, 0x8160}
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

u32 Hook_HandleTPHandwriting(void *arg) {
    gInternalContext.tpHandwritingContextAlt = arg;
    u8 usingBoldFont = gUsingBoldFont;
    gUsingBoldFont = 0;
    u32 result = Orig_HandleTPHandwriting(arg);
    gInternalContext.tpHandwritingContextAlt = NULL;
    gUsingBoldFont = usingBoldFont;
    return result;
}

static void InitInternalContext() {
    static bool initialized = false;
    static HookDataEntry handleTPHandwritingHookData = {
        .functionAddr = (void*)0x020413D4,
        .hookFunction = Hook_HandleTPHandwriting,
        .origFunctionRef = (void**)&Orig_HandleTPHandwriting
    };
    if (initialized)
        return;
    InitBppConvTable();
    if (FS_OpenFile(&gInternalContext.fontFile, "keyboard/font.bin"))
    {
        FS_ReadFile(&gInternalContext.fontFile, &gInternalContext.glyphNum, sizeof(u32));
    }
    HookFunction(&handleTPHandwritingHookData);
    initialized = true;
}

static void* Alloc(u32 size) {
    return OS_AllocFromHeap(0, -1, size);
}

static void Free(void *ptr) {
    OS_FreeToHeap(0,-1, ptr);
}

static void OnOverlayLoaded() {
    InitInternalContext();
}

static bool ShouldShowKeyboard() {
    return KEY_PRESSED(KEY_R | KEY_X) && gInternalContext.tpHandwritingContextAlt;
}

static int GetMaxInputLength() {
    if (gInternalContext.tpHandwritingContextAlt)
        return *((int*)gInternalContext.tpHandwritingContextAlt + 42);
    else
        return 0;
}

u32 reverse_2bit_units(u32 pixelData) {
    u32 result = 0;
    for (int i = 0; i < 16; ++i) {
        u8 two_bits = (pixelData >> (i * 2)) & 0x3;
        result |= (two_bits << ((15 - i) * 2));
    }
    return result;
}

static bool GetGlyphFromCustomFont(u16 charCode, u8 *output, int *advance) {
    memset(output, 0, 64);
    // binary search in font file
    int left = 0;
    int right = gInternalContext.glyphNum - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        CustomGlyphEntry entry;
        FS_SeekFile(&gInternalContext.fontFile, 4 + mid * sizeof(CustomGlyphEntry), 0);
        FS_ReadFile(&gInternalContext.fontFile, &entry, sizeof(CustomGlyphEntry));
        if (entry.charCode == charCode) {
            for (int i = 0; i < 12; i++) {
                u16 line = 0;
                int index = i / 2 * 3;
                if ((i & 1) == 0)
                    line = entry.data[index] | (entry.data[index + 1] << 8 & 0xF00);
                else 
                    line = entry.data[index + 1] >> 4 | (entry.data[index + 2] << 4);
                u16 lineLeft = gBppConvTable[line & 0xFF];
                u16 lineRight = gBppConvTable[line >> 8];
                index = (i + 2) * 4;
                output[index] = lineLeft & 0xFF;
                output[index + 1] = lineLeft >> 8;
                output[index + 2] = lineRight & 0xFF;
                output[index + 3] = lineRight >> 8;
            }
            *advance = entry.width;
            return true;
        }
        else if (entry.charCode < charCode) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }
    return false;
}

static bool KeycodeToChar(u16 keycode, u16 *output) {
    if (keycode >= KEYCODE_SPACE && keycode <= KEYCODE_TILDE) {
        *output = keycode;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_0 && keycode <= KEYCODE_FULLWIDTH_9) {
        *output = keycode - KEYCODE_FULLWIDTH_0 + 0x824F;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_A && keycode <= KEYCODE_FULLWIDTH_Z) {
        *output = keycode - KEYCODE_FULLWIDTH_A + 0x8260;
        return true;
    }
    else if (keycode >= KEYCODE_FULLWIDTH_a && keycode <= KEYCODE_FULLWIDTH_z) {
        *output = keycode - KEYCODE_FULLWIDTH_a + 0x8281;
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
    int max = *((int *)gInternalContext.tpHandwritingContextAlt + 42);
    int i = 0;
    void **inputCharEntities = (void **)((u8*)gInternalContext.tpHandwritingContextAlt + 0x94);
    for (i = 0; i < length; i++) {
        *(u16 *)((u8 *)inputCharEntities[i] + 0x8a) = 1;
        *(u32 *)((u8 *)inputCharEntities[i] + 0x8c) = inputText[i];
    }
    for (; i < max; i++) {
        *(u16 *)((u8 *)inputCharEntities[i] + 0x8a) = 1;
        *(u32 *)((u8 *)inputCharEntities[i] + 0x8c) = 0x8140;
    }
    return;
}

KeyboardGameInterface * GetKeyboardGameInterface() {
    static KeyboardGameInterface gameInterface = {
        .Alloc = Alloc,
        .Free = Free,
        .OnOverlayLoaded = OnOverlayLoaded,
        .ShouldShowKeyboard = ShouldShowKeyboard,
        .GetMaxInputLength = GetMaxInputLength,
        .GetGlyph = GetGlyphFromCustomFont,
        .KeycodeToChar = KeycodeToChar,
        .CanContinueInput = CanContinueInput,
        .OnInputFinished = OnInputFinished
    };
    return &gameInterface;
}