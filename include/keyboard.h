#ifndef KEYBOARD_H
#define KEYBOARD_H

#define KEY_BUTTON_WIDTH 19
#define KEY_BUTTON_HEIGHT 20
#define KEY_BUTTON_SPACING 1

#define KEYBOARD_LANG_CHS 0
#define KEYBOARD_LANG_ENG 1
#define KEYBOARD_LANG_MAX 2

#define EXTERNAL_FONT_PALETTE_SIZE 2

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

struct Key {
    int x;
    int y;
    int width;
    int height;
    KeyCode code;
    u16 charCode;
    bool isPressed;
    bool isHeld;
    glImage *glyph;
};

struct TextBox {
    int x;
    int y;
    int width;
    int height;
    u16 *text;
    int maxLength;
    int length;
};

struct VirtualKeyboard;

typedef struct {
    void * (*Alloc)(u32 size);
    void (*Free)(void *ptr);
    bool (*ShouldShowKeyboard)();
    int (*GetMaxInputLength)();
    bool (*GetGlyph)(u16 charCode, u8 *output, int *advance);
    bool (*KeycodeToChar)(u16 keycode, u16 *output);
    void (*OnInputStarted)();
    bool (*CanContinueInput)(u16 *inputText, int length, u16 nextChar);
    void (*OnInputComplete)(u16 *inputText, int length);
} KeyboardGameInterface;

typedef struct {
    bool (*OnKeyPressed)(struct VirtualKeyboard *keyboard, struct Key *key);
    bool (*OnKeyDraw)(const struct VirtualKeyboard *keyboard, const struct Key *key);
    bool (*OnInputStringDraw)(struct VirtualKeyboard *keyboard, struct TextBox *textBox);
} KeyboardInputMethodInterface;

typedef struct VirtualKeyboard {
    struct Key normalKeys[46];
    struct Key functionKeys[6];
    struct Key *currentKey;    
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
    struct TextBox inputTextBox;
    const KeyboardGameInterface *gameInterface;
    KeyboardInputMethodInterface *inputMethodInterface[KEYBOARD_LANG_MAX];
};

void InitHeap(void *start, u32 size);

void InitializeKeyboard(const KeyboardGameInterface *gameInterface);
void FinalizeKeyboard(bool shouldSetInputStringToGame);
void DrawKeyboard();
int HandleKeyboardInput();
void TryAddCharToInput(u16 charCode);
void TryAddKeycodeToInput(KeyCode keyCode);
void RegisterKeyboardInputMethod(int lang, KeyboardInputMethodInterface *inputMethodInterface);

void InitPinyinInputMethod();
void DeinitPinyinInputMethod();
KeyboardInputMethodInterface *GetPinyinInputMethodInterface();

void InitKeyboardFont();
void DeinitKeyboardFont();
glImage *GetDefaultGlyph(KeyCode code);
void SetDefaultKeysPalette(int palId);
void CreateExternalFontPalette(int *paletteIds, u16 textColor, u16 bgColor);
void SetGetGlyph(bool (*GetGlyph)(u16 charCode, u8 *output, int *advance));
bool GetExternalGlyph(u16 charCode, glImage *glyphImage, int *palIndex, int *advance);

KeyboardGameInterface *GetKeyboardGameInterface();

#endif