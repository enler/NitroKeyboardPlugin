#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <gl2d.h>

#define KEY_BUTTON_WIDTH 19
#define KEY_BUTTON_HEIGHT 20
#define KEY_BUTTON_SPACING 1

#define KEYBOARD_LANG_CHS 0
#define KEYBOARD_LANG_ENG 1
#define KEYBOARD_LANG_MAX 2

#define EXTERNAL_FONT_PALETTE_SIZE 2

#define KEYBOARD_HEAP_SIZE (24 * 1024)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

typedef enum {
    KEYCODE_NONE = 0,
    // 大写字母
    KEYCODE_A = 65, // ASCII 'A'
    KEYCODE_B = 66, // ASCII 'B'
    KEYCODE_C = 67, // ASCII 'C'
    KEYCODE_D = 68, // ASCII 'D'
    KEYCODE_E = 69, // ASCII 'E'
    KEYCODE_F = 70, // ASCII 'F'
    KEYCODE_G = 71, // ASCII 'G'
    KEYCODE_H = 72, // ASCII 'H'
    KEYCODE_I = 73, // ASCII 'I'
    KEYCODE_J = 74, // ASCII 'J'
    KEYCODE_K = 75, // ASCII 'K'
    KEYCODE_L = 76, // ASCII 'L'
    KEYCODE_M = 77, // ASCII 'M'
    KEYCODE_N = 78, // ASCII 'N'
    KEYCODE_O = 79, // ASCII 'O'
    KEYCODE_P = 80, // ASCII 'P'
    KEYCODE_Q = 81, // ASCII 'Q'
    KEYCODE_R = 82, // ASCII 'R'
    KEYCODE_S = 83, // ASCII 'S'
    KEYCODE_T = 84, // ASCII 'T'
    KEYCODE_U = 85, // ASCII 'U'
    KEYCODE_V = 86, // ASCII 'V'
    KEYCODE_W = 87, // ASCII 'W'
    KEYCODE_X = 88, // ASCII 'X'
    KEYCODE_Y = 89, // ASCII 'Y'
    KEYCODE_Z = 90, // ASCII 'Z'
    // 小写字母
    KEYCODE_a = 97, // ASCII 'a'
    KEYCODE_b = 98, // ASCII 'b'
    KEYCODE_c = 99, // ASCII 'c'
    KEYCODE_d = 100, // ASCII 'd'
    KEYCODE_e = 101, // ASCII 'e'
    KEYCODE_f = 102, // ASCII 'f'
    KEYCODE_g = 103, // ASCII 'g'
    KEYCODE_h = 104, // ASCII 'h'
    KEYCODE_i = 105, // ASCII 'i'
    KEYCODE_j = 106, // ASCII 'j'
    KEYCODE_k = 107, // ASCII 'k'
    KEYCODE_l = 108, // ASCII 'l'
    KEYCODE_m = 109, // ASCII 'm'
    KEYCODE_n = 110, // ASCII 'n'
    KEYCODE_o = 111, // ASCII 'o'
    KEYCODE_p = 112, // ASCII 'p'
    KEYCODE_q = 113, // ASCII 'q'
    KEYCODE_r = 114, // ASCII 'r'
    KEYCODE_s = 115, // ASCII 's'
    KEYCODE_t = 116, // ASCII 't'
    KEYCODE_u = 117, // ASCII 'u'
    KEYCODE_v = 118, // ASCII 'v'
    KEYCODE_w = 119, // ASCII 'w'
    KEYCODE_x = 120, // ASCII 'x'
    KEYCODE_y = 121, // ASCII 'y'
    KEYCODE_z = 122, // ASCII 'z'
    // 数字
    KEYCODE_0 = 48, // ASCII '0'
    KEYCODE_1 = 49, // ASCII '1'
    KEYCODE_2 = 50, // ASCII '2'
    KEYCODE_3 = 51, // ASCII '3'
    KEYCODE_4 = 52, // ASCII '4'
    KEYCODE_5 = 53, // ASCII '5'
    KEYCODE_6 = 54, // ASCII '6'
    KEYCODE_7 = 55, // ASCII '7'
    KEYCODE_8 = 56, // ASCII '8'
    KEYCODE_9 = 57, // ASCII '9'
    // 符号
    KEYCODE_EXCLAMATION = 33, // ASCII '!'
    KEYCODE_AT = 64, // ASCII '@'
    KEYCODE_HASH = 35, // ASCII '#'
    KEYCODE_DOLLAR = 36, // ASCII '$'
    KEYCODE_PERCENT = 37, // ASCII '%'
    KEYCODE_CARET = 94, // ASCII '^'
    KEYCODE_AMPERSAND = 38, // ASCII '&'
    KEYCODE_ASTERISK = 42, // ASCII '*'
    KEYCODE_LEFT_PAREN = 40, // ASCII '('
    KEYCODE_RIGHT_PAREN = 41, // ASCII ')'
    KEYCODE_MINUS = 45, // ASCII '-'
    KEYCODE_UNDERSCORE = 95, // ASCII '_'
    KEYCODE_EQUAL = 61, // ASCII '='
    KEYCODE_PLUS = 43, // ASCII '+'
    KEYCODE_LEFT_BRACE = 123, // ASCII '{'
    KEYCODE_RIGHT_BRACE = 125, // ASCII '}'
    KEYCODE_LEFT_BRACKET = 91, // ASCII '['
    KEYCODE_RIGHT_BRACKET = 93, // ASCII ']'
    KEYCODE_PIPE = 124, // ASCII '|'
    KEYCODE_BACKSLASH = 92, // ASCII '\'
    KEYCODE_COLON = 58, // ASCII ':'
    KEYCODE_SEMICOLON = 59, // ASCII ';'
    KEYCODE_QUOTE = 39, // ASCII '''
    KEYCODE_DOUBLE_QUOTE = 34, // ASCII '"'
    KEYCODE_COMMA = 44, // ASCII ','
    KEYCODE_PERIOD = 46, // ASCII '.'
    KEYCODE_SLASH = 47, // ASCII '/'
    KEYCODE_QUESTION = 63, // ASCII '?'
    KEYCODE_TILDE = 126, // ASCII '~'
    KEYCODE_GRAVE = 96, // ASCII '`'
    KEYCODE_LESS = 60, // ASCII '<'
    KEYCODE_GREATER = 62, // ASCII '>'
    // 控制键
    KEYCODE_ENTER = 13, // ASCII Carriage Return
    KEYCODE_SPACE = 32, // ASCII Space
    KEYCODE_BACKSPACE = 8, // ASCII Backspace
    KEYCODE_SHIFT,
    KEYCODE_CTRL,
    KEYCODE_ALT,
    KEYCODE_CAPS_LOCK = 20,
    KEYCODE_ESC = 27, // ASCII Escape
    // 大写字母
    KEYCODE_FULLWIDTH_A = 0xFF21, // full width 'Ａ'
    KEYCODE_FULLWIDTH_B = 0xFF22, // full width 'Ｂ'
    KEYCODE_FULLWIDTH_C = 0xFF23, // full width 'Ｃ'
    KEYCODE_FULLWIDTH_D = 0xFF24, // full width 'Ｄ'
    KEYCODE_FULLWIDTH_E = 0xFF25, // full width 'Ｅ'
    KEYCODE_FULLWIDTH_F = 0xFF26, // full width 'Ｆ'
    KEYCODE_FULLWIDTH_G = 0xFF27, // full width 'Ｇ'
    KEYCODE_FULLWIDTH_H = 0xFF28, // full width 'Ｈ'
    KEYCODE_FULLWIDTH_I = 0xFF29, // full width 'Ｉ'
    KEYCODE_FULLWIDTH_J = 0xFF2A, // full width 'Ｊ'
    KEYCODE_FULLWIDTH_K = 0xFF2B, // full width 'Ｋ'
    KEYCODE_FULLWIDTH_L = 0xFF2C, // full width 'Ｌ'
    KEYCODE_FULLWIDTH_M = 0xFF2D, // full width 'Ｍ'
    KEYCODE_FULLWIDTH_N = 0xFF2E, // full width 'Ｎ'
    KEYCODE_FULLWIDTH_O = 0xFF2F, // full width 'Ｏ'
    KEYCODE_FULLWIDTH_P = 0xFF30, // full width 'Ｐ'
    KEYCODE_FULLWIDTH_Q = 0xFF31, // full width 'Ｑ'
    KEYCODE_FULLWIDTH_R = 0xFF32, // full width 'Ｒ'
    KEYCODE_FULLWIDTH_S = 0xFF33, // full width 'Ｓ'
    KEYCODE_FULLWIDTH_T = 0xFF34, // full width 'Ｔ'
    KEYCODE_FULLWIDTH_U = 0xFF35, // full width 'Ｕ'
    KEYCODE_FULLWIDTH_V = 0xFF36, // full width 'Ｖ'
    KEYCODE_FULLWIDTH_W = 0xFF37, // full width 'Ｗ'
    KEYCODE_FULLWIDTH_X = 0xFF38, // full width 'Ｘ'
    KEYCODE_FULLWIDTH_Y = 0xFF39, // full width 'Ｙ'
    KEYCODE_FULLWIDTH_Z = 0xFF3A, // full width 'Ｚ'
    // 小写字母
    KEYCODE_FULLWIDTH_a = 0xFF41, // full width 'ａ'
    KEYCODE_FULLWIDTH_b = 0xFF42, // full width 'ｂ'
    KEYCODE_FULLWIDTH_c = 0xFF43, // full width 'ｃ'
    KEYCODE_FULLWIDTH_d = 0xFF44, // full width 'ｄ'
    KEYCODE_FULLWIDTH_e = 0xFF45, // full width 'ｅ'
    KEYCODE_FULLWIDTH_f = 0xFF46, // full width 'ｆ'
    KEYCODE_FULLWIDTH_g = 0xFF47, // full width 'ｇ'
    KEYCODE_FULLWIDTH_h = 0xFF48, // full width 'ｈ'
    KEYCODE_FULLWIDTH_i = 0xFF49, // full width 'ｉ'
    KEYCODE_FULLWIDTH_j = 0xFF4A, // full width 'ｊ'
    KEYCODE_FULLWIDTH_k = 0xFF4B, // full width 'ｋ'
    KEYCODE_FULLWIDTH_l = 0xFF4C, // full width 'ｌ'
    KEYCODE_FULLWIDTH_m = 0xFF4D, // full width 'ｍ'
    KEYCODE_FULLWIDTH_n = 0xFF4E, // full width 'ｎ'
    KEYCODE_FULLWIDTH_o = 0xFF4F, // full width 'ｏ'
    KEYCODE_FULLWIDTH_p = 0xFF50, // full width 'ｐ'
    KEYCODE_FULLWIDTH_q = 0xFF51, // full width 'ｑ'
    KEYCODE_FULLWIDTH_r = 0xFF52, // full width 'ｒ'
    KEYCODE_FULLWIDTH_s = 0xFF53, // full width 'ｓ'
    KEYCODE_FULLWIDTH_t = 0xFF54, // full width 'ｔ'
    KEYCODE_FULLWIDTH_u = 0xFF55, // full width 'ｕ'
    KEYCODE_FULLWIDTH_v = 0xFF56, // full width 'ｖ'
    KEYCODE_FULLWIDTH_w = 0xFF57, // full width 'ｗ'
    KEYCODE_FULLWIDTH_x = 0xFF58, // full width 'ｘ'
    KEYCODE_FULLWIDTH_y = 0xFF59, // full width 'ｙ'
    KEYCODE_FULLWIDTH_z = 0xFF5A, // full width 'ｚ'
    // 数字
    KEYCODE_FULLWIDTH_0 = 0xFF10, // full width '０'
    KEYCODE_FULLWIDTH_1 = 0xFF11, // full width '１'
    KEYCODE_FULLWIDTH_2 = 0xFF12, // full width '２'
    KEYCODE_FULLWIDTH_3 = 0xFF13, // full width '３'
    KEYCODE_FULLWIDTH_4 = 0xFF14, // full width '４'
    KEYCODE_FULLWIDTH_5 = 0xFF15, // full width '５'
    KEYCODE_FULLWIDTH_6 = 0xFF16, // full width '６'
    KEYCODE_FULLWIDTH_7 = 0xFF17, // full width '７'
    KEYCODE_FULLWIDTH_8 = 0xFF18, // full width '８'
    KEYCODE_FULLWIDTH_9 = 0xFF19, // full width '９'
    // 符号
    KEYCODE_FULLWIDTH_EXCLAMATION = 0xFF01, // full width '！'
    KEYCODE_FULLWIDTH_AT = 0xFF20, // full width '＠'
    KEYCODE_FULLWIDTH_HASH = 0xFF03, // full width '＃'
    KEYCODE_FULLWIDTH_DOLLAR = 0xFF04, // full width '＄'
    KEYCODE_FULLWIDTH_PERCENT = 0xFF05, // full width '％'
    KEYCODE_FULLWIDTH_CARET = 0xFF3E, // full width '＾'
    KEYCODE_FULLWIDTH_AMPERSAND = 0xFF06, // full width '＆'
    KEYCODE_FULLWIDTH_ASTERISK = 0xFF0A, // full width '＊'
    KEYCODE_FULLWIDTH_LEFT_PAREN = 0xFF08, // full width '（'
    KEYCODE_FULLWIDTH_RIGHT_PAREN = 0xFF09, // full width '）'
    KEYCODE_FULLWIDTH_MINUS = 0xFF0D, // full width '－'
    KEYCODE_FULLWIDTH_UNDERSCORE = 0xFF3F, // full width '＿'
    KEYCODE_FULLWIDTH_EQUAL = 0xFF1D, // full width '＝'
    KEYCODE_FULLWIDTH_PLUS = 0xFF0B, // full width '＋'
    KEYCODE_FULLWIDTH_LEFT_BRACE = 0xFF5B, // full width '｛'
    KEYCODE_FULLWIDTH_RIGHT_BRACE = 0xFF5D, // full width '｝'
    KEYCODE_FULLWIDTH_LEFT_BRACKET = 0xFF3B, // full width '［'
    KEYCODE_FULLWIDTH_RIGHT_BRACKET = 0xFF3D, // full width '］'
    KEYCODE_FULLWIDTH_PIPE = 0xFF5C, // full width '｜'
    KEYCODE_FULLWIDTH_BACKSLASH = 0xFF3C, // full width '＼'
    KEYCODE_FULLWIDTH_COLON = 0xFF1A, // full width '：'
    KEYCODE_FULLWIDTH_SEMICOLON = 0xFF1B, // full width '；'
    KEYCODE_FULLWIDTH_QUOTE = 0xFF07, // full width '＇'
    KEYCODE_FULLWIDTH_DOUBLE_QUOTE = 0xFF02, // full width '＂'
    KEYCODE_FULLWIDTH_COMMA = 0xFF0C, // full width '，'
    KEYCODE_FULLWIDTH_PERIOD = 0xFF0E, // full width '．'
    KEYCODE_FULLWIDTH_SLASH = 0xFF0F, // full width '／'
    KEYCODE_FULLWIDTH_QUESTION = 0xFF1F, // full width '？'
    KEYCODE_FULLWIDTH_TILDE = 0xFF5E, // full width '～'
    KEYCODE_FULLWIDTH_GRAVE = 0xFF40, // full width '｀'
    KEYCODE_FULLWIDTH_LESS = 0xFF1C, // full width '＜'
    KEYCODE_FULLWIDTH_GREATER = 0xFF1E, // full width '＞'

    KEYCODE_FULLWIDTH_SPACE = 0x3000, // full width space '　'
    KEYCODE_CHINESE_PERIOD = 0x3002, // Chinese period '。'

    KEYCODE_FLAG_LANGUAGE = 1 << 16,
    KEYCODE_LANGUAGE_CHINISE = KEYCODE_FLAG_LANGUAGE | KEYBOARD_LANG_CHS,
    KEYCODE_LANGUAGE_ENGLISH = KEYCODE_FLAG_LANGUAGE | KEYBOARD_LANG_ENG,
} KeyCode;

typedef enum {
    KEY_STATE_NOT_PRESSED =  0,
    KEY_STATE_PRESSED = 1 << 0,
    KEY_STATE_HELD = 1 << 1,
    KEY_STATE_RELEASED = 1 << 2,
    KEY_STATE_FINISHED = 1 << 3,
    KEY_STATE_EXIT = 1 << 4
} KeyState;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    KeyCode code;
    u16 charCode;
    bool isPressed;
    bool isHeld;
    glImage *glyph;
} Key;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    u16 *text;
    int maxLength;
    int length;
} TextBox;

typedef struct {
    u16 keyCode;
    u16 charCode;
} KeycodeConvItem;

typedef struct VirtualKeyboard VirtualKeyboard;

typedef struct {
    void * (*Alloc)(u32 size);
    void (*Free)(void *ptr);
    void (*OnOverlayLoaded)();
    bool (*ShouldShowKeyboard)();
    int (*GetMaxInputLength)();
    bool (*LoadGlyph)(u16 charCode, u8 *output, int *advance);
    bool (*KeycodeToChar)(u16 keycode, u16 *output);
    bool (*CanContinueInput)(u16 *inputText, int length, u16 nextChar);
    void (*OnInputFinished)(u16 *inputText, int length, bool isCanceled);
} KeyboardGameInterface;

typedef struct {
    bool (*OnKeyPressed)(VirtualKeyboard *keyboard, Key *key);
    bool (*OnKeyDraw)(const VirtualKeyboard *keyboard, const Key *key);
    bool (*OnInputStringDraw)(VirtualKeyboard *keyboard, TextBox *textBox);
} KeyboardInputMethodInterface;

typedef struct VirtualKeyboard {
    Key normalKeys[46];
    Key functionKeys[5];
    Key *currentKey;    
    int x;
    int y;
    int glyphBaseline;
    bool isShifted;
    bool isCapsLocked;
    bool isPressed;
    int language;
    int state;
    int keyTexPalId;
    int glyphTexPalId;
    int externalGlyphKeyPalIds[EXTERNAL_FONT_PALETTE_SIZE];
    int externalGlyphTextBoxPalIds[EXTERNAL_FONT_PALETTE_SIZE];
    TextBox inputTextBox;
    const KeyboardGameInterface *gameInterface;
    KeyboardInputMethodInterface *inputMethodInterface[KEYBOARD_LANG_MAX];
} VirtualKeyboard;

void StartKeyboardMonitorThread();

void InitHeap(void *start, u32 size);

void InitializeKeyboard(const KeyboardGameInterface *gameInterface);
void FinalizeKeyboard(bool isCancelled);
void DrawKeyboard();
int HandleKeyboardInput();
void TryAddCharToInput(u16 charCode);
void TryAddKeycodeToInput(KeyCode keyCode);
void RegisterKeyboardInputMethod(int lang, KeyboardInputMethodInterface *inputMethodInterface);

static inline u16 HalfToFullWidth(u16 halfWidth) {
    if (halfWidth >= 0x21 && halfWidth <= 0x7e) {
        return halfWidth + 0xfee0;
    }
    if (halfWidth == 0x20) {
        return 0x3000;
    }
    return 0x0000;
}

static inline int FindCustomCharCode(const KeycodeConvItem *table, int tableSize, u16 keyCode) {
    int left = 0;
    int right = tableSize - 1;
    while (left <= right) {
        int mid = (left + right) / 2;
        if (table[mid].keyCode == keyCode) {
            return table[mid].charCode;
        }
        else if (table[mid].keyCode < keyCode) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }
    return -1;
}

void InitPinyinInputMethod();
void DeinitPinyinInputMethod();
KeyboardInputMethodInterface *GetPinyinInputMethodInterface();

void InitKeyboardFont();
void DeinitKeyboardFont();
glImage *GetDefaultGlyph(KeyCode code);
void SetDefaultKeysPalette(int palId);
void CreateExternalFontPalette(int *paletteIds, u16 textColor, u16 bgColor);
void RegisterGlyphLoader(bool (*LoadGlyph)(u16 charCode, u8 *output, int *advance));
bool GetExternalGlyph(u16 charCode, glImage *glyphImage, int *palIndex, int *advance);

KeyboardGameInterface *GetKeyboardGameInterface();

#endif