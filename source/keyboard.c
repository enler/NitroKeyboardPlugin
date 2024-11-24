#include <nds.h>
#include <gl2d.h>
#include "keyboard.h"
#include "fs.h"
#include "touch.h"

#define KEYBOARD_BG_COLOR RGB15(30 >> 3, 144 >> 3, 255 >> 3)
#define KEY_GLYPH_COLOR RGB15(56 >> 3, 8 >> 3, 120 >> 3)
#define KEY_BG_COLOR RGB15(135 >> 3, 206 >> 3, 250 >> 3)
#define KEY_BG_COLOR_PRESSED RGB15(31, 31, 31)
#define TEXTBOX_GLYPH_COLOR RGB15(31, 31, 31)
#define TEXTBOX_BG_COLOR RGB15(0, 86 >> 3, 179 >> 3)

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

const u16 gGlyphTexPal[] = {
    RGB15(0, 0, 0),
    TEXTBOX_GLYPH_COLOR,
    RGB15(0, 0, 0),
    RGB15(0, 0, 0),
};

static struct VirtualKeyboard *gVirtualKeyboard;

void InitializeKeyboard(const KeyboardGameInterface *gameInterface) {
    InitKeyboardFont();
    SetGetGlyph(gameInterface->GetGlyph);
    gVirtualKeyboard = malloc(sizeof(struct VirtualKeyboard));
    memset(gVirtualKeyboard, 0, sizeof(struct VirtualKeyboard));
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

    struct Key functionKeys[] = {
        {0, 2 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), KEY_BUTTON_WIDTH, KEY_BUTTON_HEIGHT, KEYCODE_CAPS_LOCK},
        {0, 3 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 3 / 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_SHIFT},
        {(KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 21 / 2, 1 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 3 / 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_BACKSPACE},
        {(KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 10, 2 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), (KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING) * 2 - 1, KEY_BUTTON_HEIGHT, KEYCODE_ENTER},
        {KEY_BUTTON_WIDTH + KEY_BUTTON_SPACING, 4 * (KEY_BUTTON_HEIGHT + KEY_BUTTON_SPACING), KEY_BUTTON_WIDTH, KEY_BUTTON_HEIGHT, KEYCODE_LANGUAGE_CHINISE}
    };

    for (int i = 0; i < 5; i++) {
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
    gVirtualKeyboard->inputTextBox.text = malloc(gameInterface->GetMaxInputLength() * sizeof(u16));
    gVirtualKeyboard->inputTextBox.length = 0;

    gVirtualKeyboard->language = KEYBOARD_LANG_CHS;
    gVirtualKeyboard->glyphBaseline = KEY_BUTTON_HEIGHT - (KEY_BUTTON_HEIGHT - 12) / 2;

    glGenTextures(1, &gVirtualKeyboard->keyTexPalId);
    glBindTexture(0, gVirtualKeyboard->keyTexPalId);
    glColorTableEXT(0, 0, 4, 0, 0, gKeysTexPal);

    glGenTextures(1, &gVirtualKeyboard->glyphTexPalId);
    glBindTexture(0, gVirtualKeyboard->glyphTexPalId);
    glColorTableEXT(0, 0, 4, 0, 0, gGlyphTexPal);

    CreateExternalFontPalette(gVirtualKeyboard->externalGlyphKeyPalIds, KEY_GLYPH_COLOR, KEY_BG_COLOR);
    CreateExternalFontPalette(gVirtualKeyboard->externalGlyphTextBoxPalIds, TEXTBOX_GLYPH_COLOR, TEXTBOX_BG_COLOR);
}

void FinalizeKeyboard(bool shouldSetInputStringToGame) {
    if (shouldSetInputStringToGame) {
        gVirtualKeyboard->gameInterface->OnInputComplete(gVirtualKeyboard->inputTextBox.text, gVirtualKeyboard->inputTextBox.length);
    }
    DeinitKeyboardFont();
    free(gVirtualKeyboard->inputTextBox.text);
    free(gVirtualKeyboard);
    gVirtualKeyboard = NULL;
}

void DrawInputTextBox() {
    KeyboardInputMethodInterface *inputMethodInterface = gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];
    struct TextBox textBox = gVirtualKeyboard->inputTextBox;
    glBoxFilled(textBox.x, textBox.y, textBox.x + textBox.width - 1, textBox.y + textBox.height - 1, TEXTBOX_BG_COLOR);
    if (inputMethodInterface && inputMethodInterface[gVirtualKeyboard->language].OnInputStringDraw(gVirtualKeyboard, &textBox)) {
        return;
    }
    SetDefaultKeysPalette(gVirtualKeyboard->glyphTexPalId);
    int drawFrom = 0;
    int width = 0;
    glImage glyph;
    int palIndex;
    int advance;
    for (int i = textBox.length - 1; i >= 0; i--) {
        if (GetExternalGlyph(textBox.text[i], &glyph, &palIndex, &advance)) {
            width += advance;
            if (width > textBox.width) {
                drawFrom = i + 1;
                break;
            }
        }
    }

    width = 0;

    for (int i = drawFrom; i < textBox.length; i++) {
        if (GetExternalGlyph(textBox.text[i], &glyph, &palIndex, &advance)) {
            glSetActiveTexture(glyph.textureID);
            glAssignColorTable(0, gVirtualKeyboard->externalGlyphTextBoxPalIds[palIndex]);
            glSprite(textBox.x + width, textBox.y + (textBox.height - glyph.height) / 2, GL_FLIP_NONE, &glyph);
            width += advance;
        }
    }
}

void DrawKey(struct Key *key) {
    KeyboardInputMethodInterface *inputMethodInterface = gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];

    int x = key->x + gVirtualKeyboard->x;
    int y = key->y + gVirtualKeyboard->y;

    bool isActive = key->isPressed ||
        (key->code == KEYCODE_SHIFT && gVirtualKeyboard->isShifted) ||
        (key->code == KEYCODE_CAPS_LOCK && gVirtualKeyboard->isCapsLocked);

    u16 color = isActive ? KEY_BG_COLOR_PRESSED : KEY_BG_COLOR;

    glBoxFilled(x, y, x + key->width - 1, y + key->height - 1, color);

    if (inputMethodInterface && inputMethodInterface->OnKeyDraw(gVirtualKeyboard, key)) {
        return;
    }

    if (key->glyph) {
        int glyphX = x + (key->width + 1 - key->glyph->width) / 2;
        int glyphY = y + gVirtualKeyboard->glyphBaseline - key->glyph->height;
        glSprite(glyphX, glyphY, GL_FLIP_NONE, key->glyph);
    }
}

void DrawKeyboard() {
    glBoxFilled(0, 0, 256, 192, KEYBOARD_BG_COLOR);
    SetDefaultKeysPalette(gVirtualKeyboard->keyTexPalId);
    for (int i = 0; i < sizeof(gVirtualKeyboard->normalKeys) / sizeof(gVirtualKeyboard->normalKeys[0]); i++) {
        struct Key key = gVirtualKeyboard->normalKeys[i];
        DrawKey(&key);
    }

    for (int i = 0; i < sizeof(gVirtualKeyboard->functionKeys) / sizeof(gVirtualKeyboard->functionKeys[0]); i++) {
        struct Key key = gVirtualKeyboard->functionKeys[i];
        DrawKey(&key);
    }
    DrawInputTextBox();
}

void SwitchKeyboardLayer(u8 * keyboardMap) {
    for (int i = 0; i < sizeof(gVirtualKeyboard->normalKeys) / sizeof(gVirtualKeyboard->normalKeys[0]); i++) {
        gVirtualKeyboard->normalKeys[i].code = keyboardMap[i];
        gVirtualKeyboard->normalKeys[i].glyph = GetDefaultGlyph(keyboardMap[i]);
    }
}

void TryAddCharToInput(u16 charCode) {
    struct TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    KeyboardGameInterface *gameInterface = gVirtualKeyboard->gameInterface;
    if (textBox->length >= textBox->maxLength)
        return;
    if (!gameInterface || !gameInterface->CanContinueInput || gameInterface->CanContinueInput(textBox->text, textBox->length, charCode)) {
            textBox->text[textBox->length++] = charCode;
    }
}

void TryAddKeycodeToInput(KeyCode keyCode) {
    struct TextBox *textBox = &gVirtualKeyboard->inputTextBox;
    KeyboardGameInterface *gameInterface = gVirtualKeyboard->gameInterface;
    if (textBox->length >= textBox->maxLength)
        return;
    u16 charCode = keyCode;
    if (!gameInterface || !gameInterface->KeycodeToChar || gameInterface->KeycodeToChar(keyCode, &charCode)) {
        TryAddCharToInput(charCode);
    }
}

void ProcessKey(struct Key *key, int x, int y, int *result, KeyboardInputMethodInterface *inputMethodInterface) {
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
            if (inputMethodInterface && inputMethodInterface->OnKeyPressed(gVirtualKeyboard, key)) {
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
                struct TextBox *textBox = &gVirtualKeyboard->inputTextBox;
                if (textBox->length > 0)
                {
                    textBox->text[textBox->length - 1] = 0;
                    textBox->length--;
                }
                else
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

int ProcessKeyTouch(int x, int y) {
    int result = KEY_STATE_NOT_PRESSED;
    KeyboardInputMethodInterface *inputMethodInterface = gVirtualKeyboard->inputMethodInterface[gVirtualKeyboard->language];

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
    for (int i = 0; i < sizeof(gVirtualKeyboard->normalKeys) / sizeof(gVirtualKeyboard->normalKeys[0]); i++) {
        gVirtualKeyboard->normalKeys[i].isPressed = false;
        gVirtualKeyboard->normalKeys[i].isHeld = false;
    }
    for (int i = 0; i < sizeof(gVirtualKeyboard->functionKeys) / sizeof(gVirtualKeyboard->functionKeys[0]); i++) {
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