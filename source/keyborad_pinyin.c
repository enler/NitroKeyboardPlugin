#include <nds/ndstypes.h>
#include <stdlib.h>
#include "fs.h"
#include "keyboard.h"

#define MAX_INPUT_LETTER_NUM 6
#define MAX_CANDIDATE_NUM 10

typedef struct {
    u32 pinyinNum;
    u32 candidateOffset;
    u32 maxCandidateNum;
    u32 reserved;
} PinyinTableHeader;

typedef struct {
    u32 pinyin;
    u16 candidateOffset;
    u16 candidateNum;
} PinyinTableEntry;

typedef struct {
    PinyinTableHeader header;
    PinyinTableEntry *entries;
    FSFile file;
    u8 inputLetter[6];
    int inputLetterNum;
    u16 *candidate;
    int candidateNum;
    int candidateOffset;
    KeyboardInputMethodInterface interface;
} PinyinInputMethod;

PinyinInputMethod *gPinyinInputMethod;

static bool OnKeyPressed(struct VirtualKeyboard *keyboard, struct Key *key);
static bool OnKeyDraw(const struct VirtualKeyboard *keyboard, const struct Key *key);
static bool OnInputStringDraw(struct VirtualKeyboard *keyboard, struct TextBox *textBox);

KeyboardInputMethodInterface * GetPinyinInputMethodInterface() {
    if (!gPinyinInputMethod)
        return NULL;
    else 
        return &gPinyinInputMethod->interface;
}

void InitPinyinInputMethod() {
    gPinyinInputMethod = malloc(sizeof(PinyinInputMethod));
    memset(gPinyinInputMethod, 0, sizeof(PinyinInputMethod));
    gPinyinInputMethod->interface.OnKeyPressed = OnKeyPressed;
    gPinyinInputMethod->interface.OnKeyDraw = OnKeyDraw;
    gPinyinInputMethod->interface.OnInputStringDraw = OnInputStringDraw;
    FS_InitFile(&gPinyinInputMethod->file);
    if(FS_OpenFile(&gPinyinInputMethod->file, "/keyboard/pinyin_table.bin")) {
        FS_ReadFile(&gPinyinInputMethod->file, &gPinyinInputMethod->header, sizeof(PinyinTableHeader));
        gPinyinInputMethod->entries = malloc(sizeof(PinyinTableEntry) * gPinyinInputMethod->header.pinyinNum);
        FS_SeekFile(&gPinyinInputMethod->file, sizeof(PinyinTableHeader), 0);
        FS_ReadFile(&gPinyinInputMethod->file, gPinyinInputMethod->entries, sizeof(PinyinTableEntry) * gPinyinInputMethod->header.pinyinNum);
        gPinyinInputMethod->candidate = malloc(sizeof(u16) * gPinyinInputMethod->header.maxCandidateNum);
    }
}

void DeinitPinyinInputMethod() {
    if(gPinyinInputMethod) {
        FS_CloseFile(&gPinyinInputMethod->file);
        if(gPinyinInputMethod->entries) {
            free(gPinyinInputMethod->entries);
        }
        if(gPinyinInputMethod->candidate) {
            free(gPinyinInputMethod->candidate);
        }
        free(gPinyinInputMethod);
    }
}

int SearchPinyinTable() {
    u32 pinyin = 0;
    for (int i = 0; i < gPinyinInputMethod->inputLetterNum; i++) {
        pinyin |= (((gPinyinInputMethod->inputLetter[i] - 'a' + 1) & 0x1F) << (25 - i * 5));
    }
    int left = 0;
    int right = gPinyinInputMethod->header.pinyinNum - 1;
    while(left <= right) {
        int mid = (left + right) / 2;
        if(gPinyinInputMethod->entries[mid].pinyin == pinyin) {
            return mid;
        } else if(gPinyinInputMethod->entries[mid].pinyin < pinyin) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

void LoadPinyinCandidate() {
    int index = SearchPinyinTable();
    if(index != -1) {
        FS_SeekFile(&gPinyinInputMethod->file, gPinyinInputMethod->header.candidateOffset + gPinyinInputMethod->entries[index].candidateOffset * sizeof(u16), 0);
        FS_ReadFile(&gPinyinInputMethod->file, gPinyinInputMethod->candidate, gPinyinInputMethod->entries[index].candidateNum * sizeof(u16));
        gPinyinInputMethod->candidateNum = gPinyinInputMethod->entries[index].candidateNum;
        gPinyinInputMethod->candidateOffset = 0;
    } else {
        gPinyinInputMethod->candidateNum = 0;
        gPinyinInputMethod->candidateOffset = 0;
    }
}

void OnSwitchCandidate(bool isNextPage) {
    if (gPinyinInputMethod->candidateNum <= MAX_CANDIDATE_NUM ||
        (isNextPage && gPinyinInputMethod->candidateOffset + MAX_CANDIDATE_NUM >= gPinyinInputMethod->candidateNum) ||
        (!isNextPage && gPinyinInputMethod->candidateOffset == 0)) {
        return;
    }
    if(isNextPage) {
        gPinyinInputMethod->candidateOffset += MAX_CANDIDATE_NUM;
    } else {
        gPinyinInputMethod->candidateOffset -= MAX_CANDIDATE_NUM;
    }
}

static bool OnKeyPressed(struct VirtualKeyboard *keyboard, struct Key *key) {
    if(key->code >= KEYCODE_a && key->code <= KEYCODE_z) {
        if(gPinyinInputMethod->inputLetterNum < MAX_INPUT_LETTER_NUM) {
            gPinyinInputMethod->inputLetter[gPinyinInputMethod->inputLetterNum++] = key->code;
            LoadPinyinCandidate();
        }
        return true;
    } 
    else if(key->code == KEYCODE_BACKSPACE) {
        if(gPinyinInputMethod->inputLetterNum > 0) {
            gPinyinInputMethod->inputLetterNum--;
            LoadPinyinCandidate();
            return true;
        }
    }
    else if (key->code == KEYCODE_ENTER) {
        if (gPinyinInputMethod->inputLetterNum > 0) {
            for (int i = 0; i < gPinyinInputMethod->inputLetterNum; i++)
            {
                TryAddKeycodeToInput(gPinyinInputMethod->inputLetter[i]);
            }
            gPinyinInputMethod->inputLetterNum = 0;
            gPinyinInputMethod->candidateNum = 0;
            return true;
        }
    }
    else if (key->code == KEYCODE_SHIFT || key->code == KEYCODE_CAPS_LOCK) {
        gPinyinInputMethod->inputLetterNum = 0;
        gPinyinInputMethod->candidateNum = 0;
    }
    else if (key->code == KEYCODE_MINUS) {
        OnSwitchCandidate(false);
        return true;
    }
    else if (key->code == KEYCODE_EQUAL) {
        OnSwitchCandidate(true);
        return true;
    }
    int index = -1;
    if (key->code >= KEYCODE_1 && key->code <= KEYCODE_9) {
        index = key->code - KEYCODE_1;
    } else if (key->code == KEYCODE_0) {
        index = 9;
    }
    
    if (index >= 0 && index + gPinyinInputMethod->candidateOffset < gPinyinInputMethod->candidateNum) {
        TryAddCharToInput(gPinyinInputMethod->candidate[index + gPinyinInputMethod->candidateOffset]);
        gPinyinInputMethod->inputLetterNum = 0;
        gPinyinInputMethod->candidateNum = 0;
        return true;
    }
    return false;
}

static bool OnKeyDraw(const struct VirtualKeyboard *keyboard, const struct Key *key) {
    if (gPinyinInputMethod->candidateNum == 0) {
        return false;
    }
    int palIndex, x, y, adv;
    int index = -1;
    if (key->code >= KEYCODE_1 && key->code <= KEYCODE_9) {
        index = key->code - KEYCODE_1;
    } else if (key->code == KEYCODE_0) {
        index = 9;
    }
    if (index >= 0) {
        index += gPinyinInputMethod->candidateOffset;
        if (index < gPinyinInputMethod->candidateNum) {
            u16 charCode = gPinyinInputMethod->candidate[index];
            glImage glyph;
            if (GetExternalGlyph(charCode, &glyph, &palIndex, &adv)) {
                glSetActiveTexture(glyph.textureID);
                glAssignColorTable(0, keyboard->externalGlyphKeyPalIds[palIndex]);
                x = keyboard->x + key->x + (key->width - adv + 1) / 2;
                y = keyboard->y + key->y + (key->height - glyph.height + 1) / 2;
                glSprite(x, y, GL_FLIP_NONE, &glyph);
                return true;
            }
        }
    }
    return false;
}

static bool OnInputStringDraw(struct VirtualKeyboard *keyboard, struct TextBox *textBox) {
    if (gPinyinInputMethod->inputLetterNum == 0) {
        return false;
    }
    int inputStringWidth = 0;
    int inputLettersWidth = 0;
    glImage glyph;
    int palIndex, adv;
    int x = 0;

    for (int i = 0; i < gPinyinInputMethod->inputLetterNum; i++) {
        glImage *defaultGlyph = GetDefaultGlyph(gPinyinInputMethod->inputLetter[i]);
        if (defaultGlyph) {
            inputLettersWidth += defaultGlyph->width;
            inputLettersWidth++;
        }
    }

    for (int i = textBox->length - 1; i >= 0; i--) {
        if (GetExternalGlyph(textBox->text[i], &glyph, &palIndex, &adv)) {
            inputStringWidth += adv;
            if (inputStringWidth + inputLettersWidth > textBox->width) {
                break;
            }
        }
    }

    x = inputStringWidth;

    for (int i = 0; i < gPinyinInputMethod->inputLetterNum; i++) {
        glImage *defaultGlyph = GetDefaultGlyph(gPinyinInputMethod->inputLetter[i]);
        if (defaultGlyph) {
            glSetActiveTexture(defaultGlyph->textureID);
            SetDefaultKeysPalette(keyboard->glyphTexPalId);
            glSprite(textBox->x + x, textBox->y + keyboard->glyphBaseline - defaultGlyph->height, GL_FLIP_NONE, defaultGlyph);
            x += defaultGlyph->width;
            x++;
        }
    }
    return false;
}
