#include <nds/ndstypes.h>
#include <nitro/fs.h>
#include <nitro/heap.h>
#include <nitro/pad.h>
#include <nitro/twl.h>
#include "keyboard.h"
#include "hook.h"

typedef struct {
    void *inputContextAlt;
} GameInternalContext;

static GameInternalContext gInternalContext;

static u16 gBppConvTable[256];

const struct KeycodeConvItem gKeycodeConvTable[] = {
    {KEYCODE_FULLWIDTH_SPACE, 0x8140},
    {KEYCODE_CHINESE_PERIOD, 0x8142},
    {KEYCODE_FULLWIDTH_EXCLAMATION, 0x8149},
    {KEYCODE_FULLWIDTH_PERCENT, 0x8193},
    {KEYCODE_FULLWIDTH_LEFT_PAREN, 0x8169},
    {KEYCODE_FULLWIDTH_RIGHT_PAREN, 0x816A},
    {KEYCODE_FULLWIDTH_PLUS, 0x817B},
    {KEYCODE_FULLWIDTH_COMMA, 0x8143},
    {KEYCODE_FULLWIDTH_MINUS, 0x817C},
    {KEYCODE_FULLWIDTH_PERIOD, 0x8144},
    {KEYCODE_FULLWIDTH_SLASH, 0x815E},
    {KEYCODE_FULLWIDTH_COLON, 0x8146},
    {KEYCODE_FULLWIDTH_SEMICOLON, 0x8147},
    {KEYCODE_FULLWIDTH_EQUAL, 0x8181},
    {KEYCODE_FULLWIDTH_QUESTION, 0x8148},
    {KEYCODE_FULLWIDTH_AT, 0x8197},
    {KEYCODE_FULLWIDTH_TILDE, 0x8160},
};

void *(*Orig_FS_LoadOverlay)(u32 target, u32 id);
void *(*Orig_InputContext$$ctor)(void *this);
void *(*Orig_InputContext$$dtor)(void *this);

void *GetGameInternalGlyph(void *context, u16 charCode, u32 a3);

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

void *Hook_InputContext$$ctor(void *this) {
    gInternalContext.inputContextAlt = this;
    return Orig_InputContext$$ctor(this);
}

void *Hook_InputContext$$dtor(void *this) {
    if (this == gInternalContext.inputContextAlt)
        gInternalContext.inputContextAlt = NULL;
    return Orig_InputContext$$dtor(this);
}

void *Hook_FS_LoadOverlay(u32 target, u32 id) {
    static HookARMEntry entries[] = {
        {
            .functionAddr = (void *)0x224A35C,
            .origFunctionRef = (void **)&Orig_InputContext$$ctor,
            .hookFunction = Hook_InputContext$$ctor
        },
        {
            .functionAddr = (void*)0x224A400,
            .origFunctionRef = (void**)&Orig_InputContext$$dtor,
            .hookFunction = Hook_InputContext$$dtor
        }
    };
    void *result = Orig_FS_LoadOverlay(target, id);
    if (id == 12) {
        for (int i = 0; i < ARRAY_SIZE(entries); i++)
            HookFunction(&entries[i]);
    }
    else if (id == 15)
        PATCH_WORD(0x02000154, 0xDA000003);
    return result;
}

static void * Alloc(u32 size) {
    if (!OS_IsRunOnTwl())
        return (void*)0x225EB40;
    else
        return FndAllocFromExpHeapEx((void*)0x20CC33C, size, 4);
}

static void Free(void * ptr) {
    if (OS_IsRunOnTwl())
        FndFreeToExpHeap((void*)0x20CC33C, ptr);
}

static void OnOverlayLoaded() {
    static bool initialized = false;
    if (initialized)
        return;
    static HookARMEntry entry = {
        .functionAddr = (void*)FS_LoadOverlay,
        .origFunctionRef = (void**)&Orig_FS_LoadOverlay,
        .hookFunction = Hook_FS_LoadOverlay
    };
    InitBppConvTable();
    HookFunction(&entry);
    initialized = true;
}

static bool ShouldShowKeyboard() {
    return KEY_PRESSED(KEY_R | KEY_X) && gInternalContext.inputContextAlt;
}

static int GetMaxInputLength() {
    return 5;
}

static bool GetGlyph(u16 charCode, u8 *output, int *advance) {
    u8 *glyph = GetGameInternalGlyph((void*)0x20CC488, charCode, 0);
    if (!glyph)
        return false;
    memset(output, 0, 64);
    for (int i = 0; i < 12; i++) {
        u16 line = 0;
        int index = i / 2 * 3;
        if ((i & 1) == 0)
            line = (glyph[index] << 4) | (glyph[index + 1] >> 4);
        else
            line = (glyph[index + 1] << 8 & 0xF00) | glyph[index + 2];
        line <<= 4;
        u16 lineLeft = gBppConvTable[line >> 8];
        u16 lineRight = gBppConvTable[line & 0xFF];
        index = (i + 2) * 4;
        output[index] = lineLeft & 0xFF;
        output[index + 1] = lineLeft >> 8;
        output[index + 2] = lineRight & 0xFF;
        output[index + 3] = lineRight >> 8;
        *advance = 12;
    }
    return true;
}

static bool KeycodeToChar(u16 keycode, u16 * output) {
    if (keycode >= KEYCODE_FULLWIDTH_0 && keycode <= KEYCODE_FULLWIDTH_9) {
        *output = 0x824F + keycode - KEYCODE_FULLWIDTH_0;
        return true;
    }
    if (keycode >= KEYCODE_FULLWIDTH_A && keycode <= KEYCODE_FULLWIDTH_Z) {
        *output = 0x8260 + keycode - KEYCODE_FULLWIDTH_A;
        return true;
    }
    if (keycode >= KEYCODE_FULLWIDTH_a && keycode <= KEYCODE_FULLWIDTH_z) {
        *output = 0x8281 + keycode - KEYCODE_FULLWIDTH_a;
        return true;
    }
    int charCode = FindCustomCharCode(gKeycodeConvTable, ARRAY_SIZE(gKeycodeConvTable), keycode);
    if (charCode != -1) {
        *output = charCode;
        return true;
    }
    return false;
}

static bool CanContinueInput(u16 * inputText, int length, u16 nextChar) {
    return true;
}

static void OnInputFinished(u16 * inputText, int length, bool isCanceled) {
    if (isCanceled)
        return;
    if (gInternalContext.inputContextAlt) {
        u16 *inputTextAlt = (u16*)((u32)gInternalContext.inputContextAlt + 0x390);
        s16 *inputPoses = (s16*)((u32)gInternalContext.inputContextAlt + 0x3A2);
        u8 *flag = (u8*)((u32)gInternalContext.inputContextAlt + 0x3B2);
        u8 *textLength = (u8*)((u32)gInternalContext.inputContextAlt + 0x3B4);
        for (int i = 0; i < length; i++) {
            inputTextAlt[i] = inputText[i] >> 8 | inputText[i] << 8;
            inputPoses[i] = 1;
        }
        *(u8*)&inputTextAlt[length] = 0;
        *flag |= 1;
        *textLength = length < GetMaxInputLength() ? length : GetMaxInputLength() - 1;
    }
}

KeyboardGameInterface * GetKeyboardGameInterface() {
    static KeyboardGameInterface gameInterface = {
        .Alloc = Alloc,
        .Free = Free,
        .OnOverlayLoaded = OnOverlayLoaded,
        .ShouldShowKeyboard = ShouldShowKeyboard,
        .GetMaxInputLength = GetMaxInputLength,
        .GetGlyph = GetGlyph,
        .KeycodeToChar = KeycodeToChar,
        .CanContinueInput = CanContinueInput,
        .OnInputFinished = OnInputFinished
    };
    return &gameInterface;
}