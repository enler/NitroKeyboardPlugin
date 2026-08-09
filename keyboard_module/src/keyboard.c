#include <nds.h>
#include "nitro/fs.h"
#include "keyboard.h"
#include "keyboard_style.h"
#include "touch.h"

#define TEXTBOX_CURSOR_BLINK_INTERVAL 15

const u8 KeyboardMap[] = {
    KEYCODE_1, KEYCODE_2, KEYCODE_3, KEYCODE_4, KEYCODE_5, KEYCODE_6, KEYCODE_7, KEYCODE_8, KEYCODE_9, KEYCODE_0, KEYCODE_MINUS, KEYCODE_EQUAL,
    KEYCODE_q, KEYCODE_w, KEYCODE_e, KEYCODE_r, KEYCODE_t, KEYCODE_y, KEYCODE_u, KEYCODE_i, KEYCODE_o, KEYCODE_p,
    KEYCODE_a, KEYCODE_s, KEYCODE_d, KEYCODE_f, KEYCODE_g, KEYCODE_h, KEYCODE_j, KEYCODE_k, KEYCODE_l,
    KEYCODE_z, KEYCODE_x, KEYCODE_c, KEYCODE_v, KEYCODE_b, KEYCODE_n, KEYCODE_m, KEYCODE_COMMA, KEYCODE_PERIOD, KEYCODE_SLASH, 
    KEYCODE_SEMICOLON, KEYCODE_QUOTE, KEYCODE_SPACE, KEYCODE_LEFT_BRACKET, KEYCODE_RIGHT_BRACKET
};

const u8 KeyboardMapUppercase[] = {
    KEYCODE_1, KEYCODE_2, KEYCODE_3, KEYCODE_4, KEYCODE_5, KEYCODE_6, KEYCODE_7, KEYCODE_8, KEYCODE_9, KEYCODE_0, KEYCODE_MINUS, KEYCODE_EQUAL,
    KEYCODE_Q, KEYCODE_W, KEYCODE_E, KEYCODE_R, KEYCODE_T, KEYCODE_Y, KEYCODE_U, KEYCODE_I, KEYCODE_O, KEYCODE_P,
    KEYCODE_A, KEYCODE_S, KEYCODE_D, KEYCODE_F, KEYCODE_G, KEYCODE_H, KEYCODE_J, KEYCODE_K, KEYCODE_L,
    KEYCODE_Z, KEYCODE_X, KEYCODE_C, KEYCODE_V, KEYCODE_B, KEYCODE_N, KEYCODE_M, KEYCODE_COMMA, KEYCODE_PERIOD, KEYCODE_SLASH, 
    KEYCODE_SEMICOLON, KEYCODE_QUOTE, KEYCODE_SPACE, KEYCODE_LEFT_BRACKET, KEYCODE_RIGHT_BRACKET
};

const u8 KeyboardMapShift[] = {
    KEYCODE_EXCLAMATION, KEYCODE_AT, KEYCODE_HASH, KEYCODE_DOLLAR, KEYCODE_PERCENT, KEYCODE_CARET, KEYCODE_AMPERSAND, KEYCODE_ASTERISK, KEYCODE_LEFT_PAREN, KEYCODE_RIGHT_PAREN, KEYCODE_UNDERSCORE, KEYCODE_PLUS,
    KEYCODE_Q, KEYCODE_W, KEYCODE_E, KEYCODE_R, KEYCODE_T, KEYCODE_Y, KEYCODE_U, KEYCODE_I, KEYCODE_O, KEYCODE_P,
    KEYCODE_A, KEYCODE_S, KEYCODE_D, KEYCODE_F, KEYCODE_G, KEYCODE_H, KEYCODE_J, KEYCODE_K, KEYCODE_L,
    KEYCODE_Z, KEYCODE_X, KEYCODE_C, KEYCODE_V, KEYCODE_B, KEYCODE_N, KEYCODE_M, KEYCODE_LESS, KEYCODE_GREATER, KEYCODE_QUESTION, 
    KEYCODE_COLON, KEYCODE_TILDE, KEYCODE_SPACE, KEYCODE_LEFT_BRACE, KEYCODE_RIGHT_BRACE
};

const u16 gKeysTexPal[] = {
    RGB15(0, 0, 0),
    KEY_GLYPH_COLOR,
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
};

const u16 gFunctionKeysTexPal[] = {
    RGB15(0, 0, 0),
    FUNCTION_KEY_GLYPH_COLOR,
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
};

const u16 gGlyphTexPal[] = {
    RGB15(0, 0, 0),
    TEXTBOX_GLYPH_COLOR,
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
};

static VirtualKeyboard *gVirtualKeyboard;

typedef struct {
    u16 charCode;
    int x;
    int advance;
    int cursorBefore;
    int cursorAfter;
    bool useDefaultGlyph;
    bool underline;
    bool hasGlyph;
    glImage *defaultGlyph;
    glImage externalGlyph;
    int externalPalIndex;
} TextBoxLayoutItem;

typedef struct {
    TextBoxLayoutItem *items;
    int length;
    int cursorIndex;
    int cursorX;
    int drawFrom;
    int drawOffset;
    int totalWidth;
} TextBoxLayout;

static TextBoxLayoutItem *gTextBoxLayoutItems;
static int gTextBoxLayoutCapacity;

void InitializeKeyboard(const KeyboardGameInterface *gameInterface) {
    InitKeyboardFont();
    RegisterGlyphLoader(gameInterface->LoadGlyph);
    gVirtualKeyboard = malloc(sizeof(VirtualKeyboard));
    memset(gVirtualKeyboard, 0, sizeof(VirtualKeyboard));
    gVirtualKeyboard->gameInterface = gameInterface;
    int rowOffsets[] = {0, (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) / 2, (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 3 / 2, (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 2};
    int rowLengths[] = {12, 10, 9, 10, 3};

    int keyIndex = 0;
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < rowLengths[row]; col++) {
            gVirtualKeyboard->normalKeys[keyIndex].x = col * (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) + rowOffsets[row];
            gVirtualKeyboard->normalKeys[keyIndex].y = row * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING);
            gVirtualKeyboard->normalKeys[keyIndex].width = KEY_BUTTON_WIDTH;
            gVirtualKeyboard->normalKeys[keyIndex].height = KEY_BUTTON_HEIGHT;
            gVirtualKeyboard->normalKeys[keyIndex].code = KeyboardMap[keyIndex];
            keyIndex++;
        }
    }

    gVirtualKeyboard->normalKeys[43].width = KEY_BUTTON_WIDTH * 5 + KEY_BUTTON_SPACING * 4;
    for (int i = 44; i < 46; i++) {
        gVirtualKeyboard->normalKeys[i].x = gVirtualKeyboard->normalKeys[43].x + gVirtualKeyboard->normalKeys[43].width + KEY_BUTTON_SPACING + (i - 44) * (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING);
        gVirtualKeyboard->normalKeys[i].y = 4 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING);
        gVirtualKeyboard->normalKeys[i].width = KEY_BUTTON_WIDTH;
        gVirtualKeyboard->normalKeys[i].height = KEY_BUTTON_HEIGHT;
        gVirtualKeyboard->normalKeys[i].code = KeyboardMap[i];
    }

    Key functionKeys[] = {
        {0, 2 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), KEY_BUTTON_WIDTH, KEY_BUTTON_HEIGHT, KEYCODE_CAPS_LOCK},
        {0, 3 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 3 / 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_SHIFT},
        {(KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 21 / 2, 1 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 3 / 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_BACKSPACE},
        {(KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 10, 2 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_ENTER},
        {KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING, 4 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), KEY_BUTTON_WIDTH, KEY_BUTTON_HEIGHT, KEYCODE_LANGUAGE_CHINISE}
    };

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->functionKeys); i++) {
        gVirtualKeyboard->functionKeys[i] = functionKeys[i];
    }

    gVirtualKeyboard->x = 8;
    gVirtualKeyboard->y = 60;

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->normalKeys); i++) {
        gVirtualKeyboard->normalKeys[i].glyph = GetDefaultGlyph(gVirtualKeyboard->normalKeys[i].code);
    }

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->functionKeys); i++) {
        gVirtualKeyboard->functionKeys[i].glyph = GetDefaultGlyph(gVirtualKeyboard->functionKeys[i].code);
    }


    gVirtualKeyboard->inputTextBox.x = 8;
    gVirtualKeyboard->inputTextBox.y = 16;
    gVirtualKeyboard->inputTextBox.width = 240;
    gVirtualKeyboard->inputTextBox.height = 16;
    gVirtualKeyboard->inputTextBox.maxLength = gameInterface->GetMaxInputLength();
    gVirtualKeyboard->inputTextBox.text = malloc(gVirtualKeyboard->inputTextBox.maxLength * sizeof(u16));
    gVirtualKeyboard->inputTextBox.length = 0;
    gVirtualKeyboard->inputTextBox.cursorPosition = 0;
    gVirtualKeyboard->inputTextBox.cursorBlinkCounter = 0;
    if (gameInterface->GetInitialInputText) {
        const u16 *initialText = NULL;
        int initialLength = gameInterface->GetInitialInputText(&initialText);
        if (initialText && initialLength > 0) {
            if (initialLength > gVirtualKeyboard->inputTextBox.maxLength)
                initialLength = gVirtualKeyboard->inputTextBox.maxLength;
            for (int i = 0; i < initialLength; i++)
                gVirtualKeyboard->inputTextBox.text[i] = initialText[i];
            gVirtualKeyboard->inputTextBox.length = initialLength;
            gVirtualKeyboard->inputTextBox.cursorPosition = initialLength;
        }
    }

    gVirtualKeyboard->language = KEYBOARD_LANG_CHS;
    gVirtualKeyboard->glyphBaseline = KEY_BUTTON_HEIGHT - (KEY_BUTTON_HEIGHT - 12) / 2;

    glGenTextures(1, &gVirtualKeyboard->keyTexPalId);
    glBindTexture(0, gVirtualKeyboard->keyTexPalId);
    glColorTableEXT(0, 0, 4, 0, 0, gKeysTexPal);

    glGenTextures(1, &gVirtualKeyboard->functionKeyTexPalId);
    glBindTexture(0, gVirtualKeyboard->functionKeyTexPalId);
    glColorTableEXT(0, 0, 4, 0, 0, gFunctionKeysTexPal);

    glGenTextures(1, &gVirtualKeyboard->glyphTexPalId);
    glBindTexture(0, gVirtualKeyboard->glyphTexPalId);
    glColorTableEXT(0, 0, 4, 0, 0, gGlyphTexPal);

    CreateExternalFontPalette(gVirtualKeyboard->externalGlyphKeyPalIds, KEY_GLYPH_COLOR, KEY_BG_COLOR);
    CreateExternalFontPalette(gVirtualKeyboard->externalGlyphTextBoxPalIds, TEXTBOX_GLYPH_COLOR, TEXTBOX_BG_COLOR);
#if ENABLE_KEYBOARD_PINYIN_EX
    CreateExternalFontPalette(gVirtualKeyboard->externalGlyphCandidatePalIds, TEXTBOX_GLYPH_COLOR, CANDIDATE_BG_COLOR);
#endif
}

void FinalizeKeyboard(bool isCancelled) {
    if (gVirtualKeyboard->inputTextBox.length == 0)
        isCancelled = true;
    gVirtualKeyboard->gameInterface->OnInputFinished(gVirtualKeyboard->inputTextBox.text, gVirtualKeyboard->inputTextBox.length, isCancelled);
    DeinitKeyboardFont();
    if (gTextBoxLayoutItems) {
        free(gTextBoxLayoutItems);
        gTextBoxLayoutItems = NULL;
        gTextBoxLayoutCapacity = 0;
    }
    free(gVirtualKeyboard->inputTextBox.text);
    free(gVirtualKeyboard);
    gVirtualKeyboard = NULL;
}

static int ClampCompositionStart(const TextBox *textBox, const TextComposition *composition) {
    if (composition->start < 0)
        return 0;
    if (composition->start > textBox->length)
        return textBox->length;
    return composition->start;
}

static int ClampTextBoxCursorPosition(const TextBox *textBox, int cursorPosition) {
    if (cursorPosition < 0)
        return 0;
    if (cursorPosition > textBox->length)
        return textBox->length;
    return cursorPosition;
}

static void ResetTextBoxCursorBlink(TextBox *textBox) {
    textBox->cursorBlinkCounter = 0;
}

static bool EnsureTextBoxLayoutCapacity(int capacity) {
    if (capacity <= 0)
        return true;
    if (capacity <= gTextBoxLayoutCapacity)
        return true;

    TextBoxLayoutItem *items = realloc(gTextBoxLayoutItems, sizeof(TextBoxLayoutItem) * capacity);
    if (!items)
        return false;

    gTextBoxLayoutItems = items;
    gTextBoxLayoutCapacity = capacity;
    return true;
}

static void LoadTextBoxLayoutItemGlyph(TextBoxLayoutItem *item) {
    item->advance = 0;
    item->hasGlyph = false;
    item->defaultGlyph = NULL;
    item->externalPalIndex = 0;

    if (item->useDefaultGlyph) {
        item->defaultGlyph = GetDefaultGlyph(item->charCode);
        if (item->defaultGlyph) {
            item->advance = item->defaultGlyph->width + 1;
            item->hasGlyph = true;
        }
        return;
    }

    item->hasGlyph = GetExternalGlyph(item->charCode, &item->externalGlyph, &item->externalPalIndex, &item->advance);
    if (!item->hasGlyph)
        item->advance = 0;
}

static void AddTextBoxLayoutItem(TextBoxLayout *layout,
                                 u16 charCode,
                                 bool useDefaultGlyph,
                                 bool underline,
                                 int cursorBefore,
                                 int cursorAfter) {
    TextBoxLayoutItem *item = &layout->items[layout->length++];
    memset(item, 0, sizeof(TextBoxLayoutItem));
    item->charCode = charCode;
    item->x = layout->totalWidth;
    item->cursorBefore = cursorBefore;
    item->cursorAfter = cursorAfter;
    item->useDefaultGlyph = useDefaultGlyph;
    item->underline = underline;
    LoadTextBoxLayoutItemGlyph(item);
    layout->totalWidth += item->advance;
}

static int GetTextBoxLayoutSlotX(const TextBoxLayout *layout, int index) {
    if (index <= 0 || layout->length == 0)
        return 0;
    if (index >= layout->length)
        return layout->totalWidth;
    return layout->items[index].x;
}

static void FinishTextBoxLayoutScroll(const TextBox *textBox, TextBoxLayout *layout) {
    layout->cursorX = GetTextBoxLayoutSlotX(layout, layout->cursorIndex);
    layout->drawFrom = 0;

    for (int i = layout->cursorIndex - 1; i >= 0; i--) {
        if (layout->cursorX - layout->items[i].x > textBox->width - 1) {
            layout->drawFrom = i + 1;
            break;
        }
    }

    layout->drawOffset = GetTextBoxLayoutSlotX(layout, layout->drawFrom);
}

static bool BuildTextBoxLayout(VirtualKeyboard *keyboard, TextBoxLayout *layout) {
    TextBox *textBox = &keyboard->inputTextBox;
    KeyboardInputMethodInterface *inputMethodInterface = keyboard->inputMethodInterface[keyboard->language];
    TextComposition composition;
    memset(layout, 0, sizeof(TextBoxLayout));
    memset(&composition, 0, sizeof(TextComposition));

    bool hasComposition = inputMethodInterface &&
                          inputMethodInterface->GetComposition &&
                          inputMethodInterface->GetComposition(keyboard, &composition) &&
                          composition.text &&
                          composition.length > 0;
    int compositionStart = hasComposition ? ClampCompositionStart(textBox, &composition) : textBox->length;
    int displayLength = textBox->length + (hasComposition ? composition.length : 0);
    if (!EnsureTextBoxLayoutCapacity(displayLength))
        return false;

    layout->items = gTextBoxLayoutItems;
    int cursorPosition = ClampTextBoxCursorPosition(textBox, textBox->cursorPosition);

    for (int i = 0; i < compositionStart; i++) {
        AddTextBoxLayoutItem(layout, textBox->text[i], false, false, i, i + 1);
    }

    if (hasComposition) {
        for (int i = 0; i < composition.length; i++) {
            AddTextBoxLayoutItem(layout,
                                 composition.text[i],
                                 composition.useDefaultGlyph,
                                 composition.underline,
                                 compositionStart,
                                 compositionStart);
        }
    }

    for (int i = compositionStart; i < textBox->length; i++) {
        AddTextBoxLayoutItem(layout, textBox->text[i], false, false, i, i + 1);
    }

    layout->cursorIndex = cursorPosition;
    if (hasComposition && cursorPosition >= compositionStart)
        layout->cursorIndex += composition.length;
    if (layout->cursorIndex > layout->length)
        layout->cursorIndex = layout->length;

    FinishTextBoxLayoutScroll(textBox, layout);
    return true;
}

static void DrawTextBoxLayoutItem(VirtualKeyboard *keyboard,
                                  const TextBox *textBox,
                                  const TextBoxLayoutItem *item,
                                  int x) {
    if (!item->hasGlyph)
        return;

    if (item->useDefaultGlyph) {
        glSetActiveTexture(item->defaultGlyph->textureID);
        SetDefaultKeysPalette(keyboard->glyphTexPalId);
        glSprite(textBox->x + x,
                 textBox->y + keyboard->glyphBaseline - item->defaultGlyph->height,
                 GL_FLIP_NONE,
                 item->defaultGlyph);
        return;
    }

    glSetActiveTexture(item->externalGlyph.textureID);
    glAssignColorTable(0, keyboard->externalGlyphTextBoxPalIds[item->externalPalIndex]);
    glSprite(textBox->x + x,
             textBox->y + (textBox->height - item->externalGlyph.height) / 2,
             GL_FLIP_NONE,
             &item->externalGlyph);
}

void DrawInputTextBox() {
    TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    TextBoxLayout layout;
    bool hasLayout = BuildTextBoxLayout(gVirtualKeyboard, &layout);
    int cursorDrawX = 0;

    glBoxFilled(textBox->x, textBox->y, textBox->x + textBox->width - 1, textBox->y + textBox->height - 1, TEXTBOX_BG_COLOR);
    DrawKeyboardOutlineBox(
        textBox->x - 1,
        textBox->y - 1,
        textBox->x + textBox->width,
        textBox->y + textBox->height,
        TEXTBOX_BORDER_COLOR
    );

    if (hasLayout) {
        for (int i = layout.drawFrom; i < layout.length; i++) {
            TextBoxLayoutItem *item = &layout.items[i];
            int x = item->x - layout.drawOffset;
            if (x + item->advance > textBox->width)
                break;

            if (item->underline && item->advance > 0) {
                int underlineY = textBox->y + textBox->height - 1;
                glLine(textBox->x + x, underlineY, textBox->x + x + item->advance - 1, underlineY, TEXTBOX_COMPOSITION_COLOR);
            }
            DrawTextBoxLayoutItem(gVirtualKeyboard, textBox, item, x);
        }

        int cursorX = layout.cursorX - layout.drawOffset;
        if (cursorX < 0)
            cursorX = 0;
        if (cursorX >= textBox->width)
            cursorX = textBox->width - 1;
        cursorDrawX = cursorX > 0 ? cursorX - 1 : 0;
    }

    if ((textBox->cursorBlinkCounter / TEXTBOX_CURSOR_BLINK_INTERVAL) == 0) {
        int x = textBox->x + cursorDrawX;
        glLine(x, textBox->y, x, textBox->y + textBox->height - 1, TEXTBOX_CURSOR_COLOR);
    }

    textBox->cursorBlinkCounter++;
    if (textBox->cursorBlinkCounter >= TEXTBOX_CURSOR_BLINK_INTERVAL * 2)
        textBox->cursorBlinkCounter = 0;
}

static bool IsFunctionKey(KeyCode keyCode) {
    return keyCode == KEYCODE_CAPS_LOCK ||
        keyCode == KEYCODE_SHIFT ||
        keyCode == KEYCODE_BACKSPACE ||
        keyCode == KEYCODE_ENTER ||
        keyCode == KEYCODE_LANGUAGE_CHINISE ||
        keyCode == KEYCODE_LANGUAGE_ENGLISH;
}

void DrawKey(Key *key) {
    KeyboardInputMethodInterface *inputMethodInterface = gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];

    int x = key->x + gVirtualKeyboard->x;
    int y = key->y + gVirtualKeyboard->y;

    bool isActive = key->isPressed ||
        (key->code == KEYCODE_SHIFT && gVirtualKeyboard->isShifted) ||
        (key->code == KEYCODE_CAPS_LOCK && gVirtualKeyboard->isCapsLocked);
    bool isFunctionKey = IsFunctionKey(key->code);

    u16 color = isActive
        ? KEY_BG_COLOR_PRESSED
        : (isFunctionKey ? FUNCTION_KEY_BG_COLOR : KEY_BG_COLOR);
    u16 borderColor = isFunctionKey
        ? FUNCTION_KEY_BORDER_COLOR
        : KEY_BORDER_COLOR;

    glBoxFilled(x, y, x + key->width - 1, y + key->height - 1, color);
    DrawKeyboardOutlineBox(x, y, x + key->width - 1, y + key->height - 1, borderColor);

    if (inputMethodInterface && inputMethodInterface->OnKeyDraw &&
        inputMethodInterface->OnKeyDraw(gVirtualKeyboard, key)) {
        return;
    }

    if (key->glyph) {
        int glyphX = x + (key->width + 1 - key->glyph->width) / 2;
        int glyphY = y + gVirtualKeyboard->glyphBaseline - key->glyph->height;
        SetDefaultKeysPalette(isFunctionKey
            ? gVirtualKeyboard->functionKeyTexPalId
            : gVirtualKeyboard->keyTexPalId);
        glSprite(glyphX, glyphY, GL_FLIP_NONE, key->glyph);
    }
}

void DrawInputMethodGlobal() {
    KeyboardInputMethodInterface *inputMethodInterface =
        gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];
    if (inputMethodInterface && inputMethodInterface->OnGlobalDraw) {
        inputMethodInterface->OnGlobalDraw(gVirtualKeyboard);
    }
}

void DrawKeyboard() {
    glBoxFilled(0, 0, 255, 191, KEYBOARD_BG_COLOR);
    DrawKeyboardOutlineBox(0, 0, 255, 191, KEYBOARD_OUTLINE_COLOR);
    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->normalKeys); i++) {
        Key key = gVirtualKeyboard->normalKeys[i];
        DrawKey(&key);
    }

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->functionKeys); i++) {
        Key key = gVirtualKeyboard->functionKeys[i];
        DrawKey(&key);
    }
    DrawInputMethodGlobal();
    DrawInputTextBox();
}

void SwitchKeyboardLayer(u8 * keyboardMap) {
    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->normalKeys); i++) {
        gVirtualKeyboard->normalKeys[i].code = keyboardMap[i];
        gVirtualKeyboard->normalKeys[i].glyph = GetDefaultGlyph(keyboardMap[i]);
    }
}

void TryAddCharToInput(u16 charCode) {
    TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    KeyboardGameInterface *gameInterface = gVirtualKeyboard->gameInterface;
    if (textBox->length >= textBox->maxLength)
        return;
    if (!gameInterface || !gameInterface->CanContinueInput || gameInterface->CanContinueInput(textBox->text, textBox->length, charCode)) {
        int insertPosition = ClampTextBoxCursorPosition(textBox, textBox->cursorPosition);
        if (insertPosition < textBox->length) {
            for (int i = textBox->length; i > insertPosition; i--)
                textBox->text[i] = textBox->text[i - 1];
        }
        textBox->text[insertPosition] = charCode;
        textBox->length++;
        textBox->cursorPosition = insertPosition + 1;
        ResetTextBoxCursorBlink(textBox);
    }
}

void TryAddKeycodeToInput(KeyCode keyCode) {
    TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    KeyboardGameInterface *gameInterface = gVirtualKeyboard->gameInterface;
    if (textBox->length >= textBox->maxLength)
        return;
    u16 charCode = keyCode;
    if (!gameInterface || !gameInterface->KeycodeToChar || gameInterface->KeycodeToChar(keyCode, &charCode)) {
        TryAddCharToInput(charCode);
    }
}

static int ProcessTextBoxTouch(int x, int y) {
    TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    if (x < textBox->x || x >= textBox->x + textBox->width ||
        y < textBox->y || y >= textBox->y + textBox->height) {
        return KEY_STATE_NOT_PRESSED;
    }

    if (gVirtualKeyboard->currentKey) {
        gVirtualKeyboard->currentKey->isPressed = false;
        gVirtualKeyboard->currentKey->isHeld = false;
        gVirtualKeyboard->currentKey = NULL;
    }

    if (gVirtualKeyboard->isPressed)
        return KEY_STATE_HELD;

    TextBoxLayout layout;
    if (!BuildTextBoxLayout(gVirtualKeyboard, &layout))
        return KEY_STATE_PRESSED;

    int localX = x - textBox->x;
    int targetCursorPosition = textBox->length;
    if (layout.drawFrom < layout.length)
        targetCursorPosition = layout.items[layout.drawFrom].cursorBefore;

    for (int i = layout.drawFrom; i < layout.length; i++) {
        TextBoxLayoutItem *item = &layout.items[i];
        int itemX = item->x - layout.drawOffset;
        if (itemX + item->advance > textBox->width)
            break;
        if (item->advance <= 0)
            continue;

        if (localX < itemX + item->advance / 2) {
            targetCursorPosition = item->cursorBefore;
            break;
        }
        targetCursorPosition = item->cursorAfter;
    }

    textBox->cursorPosition = ClampTextBoxCursorPosition(textBox, targetCursorPosition);
    ResetTextBoxCursorBlink(textBox);
    return KEY_STATE_PRESSED;
}

void ProcessKey(Key *key, int x, int y, int *result, KeyboardInputMethodInterface *inputMethodInterface) {
    int keyX = key->x + gVirtualKeyboard->x;
    int keyY = key->y + gVirtualKeyboard->y;

    
    if (x >= keyX && x < keyX + key->width && y >= keyY && y < keyY + key->height) {
        if (gVirtualKeyboard->currentKey && gVirtualKeyboard->currentKey != key) {
            gVirtualKeyboard->currentKey->isPressed = false;
            gVirtualKeyboard->currentKey->isHeld = false;
            *result |= KEY_STATE_RELEASED;
        }
        gVirtualKeyboard->currentKey = key;
        if (!key->isPressed) {
            key->isPressed = true;
            *result |= KEY_STATE_PRESSED;
            if (gVirtualKeyboard->isPressed)
                return;
            if (inputMethodInterface && inputMethodInterface->OnKeyPressed &&
                inputMethodInterface->OnKeyPressed(gVirtualKeyboard, key)) {
                return;
            }
            if (key->code == KEYCODE_SHIFT) {
                gVirtualKeyboard->isShifted = !gVirtualKeyboard->isShifted;
                gVirtualKeyboard->isCapsLocked = false;
                SwitchKeyboardLayer(gVirtualKeyboard->isShifted ? KeyboardMapShift : KeyboardMap);
            }
            else if (key->code == KEYCODE_CAPS_LOCK) {
                gVirtualKeyboard->isCapsLocked = !gVirtualKeyboard->isCapsLocked;
                gVirtualKeyboard->isShifted = false;
                SwitchKeyboardLayer(gVirtualKeyboard->isCapsLocked ? KeyboardMapUppercase : KeyboardMap);
            }
            else if (key->code == KEYCODE_BACKSPACE) {
                TextBox *textBox = &gVirtualKeyboard->inputTextBox;
                textBox->cursorPosition = ClampTextBoxCursorPosition(textBox, textBox->cursorPosition);
                if (textBox->cursorPosition > 0)
                {
                    int deletePosition = textBox->cursorPosition - 1;
                    if (deletePosition < textBox->length - 1) {
                        for (int i = deletePosition; i < textBox->length - 1; i++)
                            textBox->text[i] = textBox->text[i + 1];
                    }
                    textBox->length--;
                    textBox->cursorPosition = deletePosition;
                    textBox->text[textBox->length] = 0;
                    ResetTextBoxCursorBlink(textBox);
                }
                else if (textBox->length == 0)
                {
                    *result |= KEY_STATE_EXIT;
                }
            }
            else if (key->code & KEYCODE_FLAG_LANGUAGE) {
                gVirtualKeyboard->isShifted = false;
                gVirtualKeyboard->isCapsLocked = false;
                gVirtualKeyboard->language++;
                if (gVirtualKeyboard->language >= KEYBOARD_LANG_MAX)
                {
                    gVirtualKeyboard->language = KEYBOARD_LANG_CHS;
                }
                key->code = KEYCODE_FLAG_LANGUAGE | gVirtualKeyboard->language;
                key->glyph = GetDefaultGlyph(key->code);
                SwitchKeyboardLayer(KeyboardMap);
            }
            else if (key->code == KEYCODE_ENTER) {
                if (gVirtualKeyboard->inputTextBox.length > 0)
                {
                    *result |= KEY_STATE_FINISHED;
                }
                else
                {
                    *result |= KEY_STATE_EXIT;
                }
            }
            else {
                TryAddKeycodeToInput(key->code);
            }
        }
        else {
            key->isHeld = true;
            *result |= KEY_STATE_HELD;
        }
    }
}

int ProcessInputMethodGlobalTouch(int x, int y) {
    KeyboardInputMethodInterface *inputMethodInterface =
        gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];
    if (!inputMethodInterface || !inputMethodInterface->OnGlobalTouch) {
        return KEY_STATE_NOT_PRESSED;
    }

    int result = inputMethodInterface->OnGlobalTouch(gVirtualKeyboard, x, y);
    if (result != KEY_STATE_NOT_PRESSED && gVirtualKeyboard->currentKey) {
        gVirtualKeyboard->currentKey->isPressed = false;
        gVirtualKeyboard->currentKey->isHeld = false;
        gVirtualKeyboard->currentKey = NULL;
    }
    return result;
}

int ProcessKeyTouch(int x, int y) {
    int result = KEY_STATE_NOT_PRESSED;
    KeyboardInputMethodInterface *inputMethodInterface = gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];

    result = ProcessInputMethodGlobalTouch(x, y);
    if (result != KEY_STATE_NOT_PRESSED)
        return result;

    result = ProcessTextBoxTouch(x, y);
    if (result != KEY_STATE_NOT_PRESSED)
        return result;

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->normalKeys) && result == KEY_STATE_NOT_PRESSED; i++) {
        ProcessKey(&gVirtualKeyboard->normalKeys[i], x, y, &result, inputMethodInterface);
    }

    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->functionKeys) && result == KEY_STATE_NOT_PRESSED; i++) {
        ProcessKey(&gVirtualKeyboard->functionKeys[i], x, y, &result, inputMethodInterface);
    }

    if (result == KEY_STATE_NOT_PRESSED && gVirtualKeyboard->currentKey) {
        gVirtualKeyboard->currentKey->isPressed = false;
        gVirtualKeyboard->currentKey->isHeld = false;
        gVirtualKeyboard->currentKey = NULL;
        result = KEY_STATE_RELEASED;
    }

    return result;
}

void ClearKeyboardState() {
    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->normalKeys); i++) {
        gVirtualKeyboard->normalKeys[i].isPressed = false;
        gVirtualKeyboard->normalKeys[i].isHeld = false;
    }
    for (int i = 0; i < ARRAY_SIZE(gVirtualKeyboard->functionKeys); i++) {
        gVirtualKeyboard->functionKeys[i].isPressed = false;
        gVirtualKeyboard->functionKeys[i].isHeld = false;
    }
}

int HandleKeyboardInput() {
    int x, y, keyState;
    switch (gVirtualKeyboard->state) {
        case 0:
            if (GetCalibratedPoint(&x, &y)) {
                keyState = ProcessKeyTouch(x, y);
                gVirtualKeyboard->isPressed = true;
                if (keyState & KEY_STATE_FINISHED)
                    return 2;
                else if (keyState & KEY_STATE_EXIT)
                    return 3;
                else if (keyState & KEY_STATE_PRESSED)
                {
                    gVirtualKeyboard->state++;
                    return 1;
                }
            } 
            else {
                gVirtualKeyboard->isPressed = false;
            }
            break;
        case 1:
            if (GetCalibratedPoint(&x, &y)) {
                keyState = ProcessKeyTouch(x, y);
                if (keyState == KEY_STATE_RELEASED || keyState == KEY_STATE_NOT_PRESSED) {
                    gVirtualKeyboard->state++;
                    return 0;
                }
                else if (keyState & KEY_STATE_PRESSED) {
                    return 1;
                }
                break;
            } 
            else {
                gVirtualKeyboard->isPressed = false;
            }
        case 2:
            ClearKeyboardState();
            gVirtualKeyboard->state = 0;
            return 1;
        
    }
    return 0;
}

void RegisterKeyboardInputMethod(int language, KeyboardInputMethodInterface *inputMethodInterface) {
    gVirtualKeyboard->inputMethodInterface[language] = inputMethodInterface;
}
