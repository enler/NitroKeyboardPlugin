#include <nds/ndstypes.h>
#include <stdlib.h>
#include <string.h>
#include "nitro/fs.h"
#include "keyboard.h"
#include "keyboard_style.h"

#define PINYIN_DB_PATH "/keyboard/pinyin_db.bin"
#define PINYIN_DB_VERSION 1
#define PINYIN_DB_HEADER_SIZE 32
#define PINYIN_DB_STARTUP_SIZE 4096
#define PINYIN_DB_SECTOR_SIZE 512
#define MAX_PINYIN_BLOCK_SIZE (16 * 1024)
#define MAX_PINYIN_PREFIX_LENGTH 6
#define MAX_PINYIN_LENGTH 20
#define MAX_LEVEL1_ENTRY_NUM 508
#define MAX_VISIBLE_CANDIDATE_NUM 32

#define CANDIDATE_BAR_GAP 8
#define CANDIDATE_BAR_HEIGHT 16
#define CANDIDATE_ARROW_WIDTH 16
#define CANDIDATE_TEXT_PADDING 2
#define CANDIDATE_WORD_SPACING 4
typedef struct {
    u8 magic[8];
    u16 version;
    u16 headerSize;
    u16 level1Num;
    u16 maxPrefixLength;
    u32 level1Offset;
    u32 dataOffset;
    u32 fileSize;
    u32 maxBlockSize;
} PinyinDbHeader;

typedef struct {
    u32 pinyin;
    u32 blockOffset;
} PinyinDbLevel1Entry;

typedef struct {
    u16 pinyinNum;
    u16 candidateNum;
    u32 candidateOffset;
} PinyinDbBlockHeader;

typedef struct {
    u32 pinyinOffset;
    u16 candidateOffset;
    u16 candidateNum;
} PinyinDbEntry;

typedef struct {
    PinyinDbHeader header;
    PinyinDbLevel1Entry entries[MAX_LEVEL1_ENTRY_NUM];
} PinyinDbStartup;

typedef union {
    u32 alignment;
    u8 data[MAX_PINYIN_BLOCK_SIZE];
} PinyinDbBlock;

typedef struct {
    int candidateOffset;
    int x;
    int width;
} PinyinCandidateLayoutItem;

typedef struct {
    PinyinCandidateLayoutItem items[MAX_VISIBLE_CANDIDATE_NUM];
    int itemNum;
    int nextPageStart;
} PinyinCandidateLayout;

typedef struct {
    PinyinDbStartup startup;
    PinyinDbBlock block;
    FSFile file;
    int blockIndex;
    int candidateOffset;
    int candidateNum;
    u8 inputLetter[MAX_PINYIN_LENGTH];
    u16 compositionText[MAX_PINYIN_LENGTH];
    int inputLetterNum;
    int candidatePageStart;
    PinyinCandidateLayout candidateLayout;
    KeyboardInputMethodInterface interface;
} PinyinInputMethodEx;

typedef char PinyinDbHeaderSizeCheck[sizeof(PinyinDbHeader) == 32 ? 1 : -1];
typedef char PinyinDbLevel1EntrySizeCheck[sizeof(PinyinDbLevel1Entry) == 8 ? 1 : -1];
typedef char PinyinDbBlockHeaderSizeCheck[sizeof(PinyinDbBlockHeader) == 8 ? 1 : -1];
typedef char PinyinDbEntrySizeCheck[sizeof(PinyinDbEntry) == 8 ? 1 : -1];
typedef char PinyinDbStartupSizeCheck[sizeof(PinyinDbStartup) == PINYIN_DB_STARTUP_SIZE ? 1 : -1];

PinyinInputMethodEx *gPinyinInputMethodEx;

static int GetPinyinCodeLength(u32 pinyin);
static bool IsValidPinyinDbMagic(const u8 *magic);
static bool ValidatePinyinDb(PinyinInputMethodEx *inputMethod);
static bool ValidatePinyinBlock(PinyinInputMethodEx *inputMethod, int blockIndex, int blockSize);
static int SearchLevel1Entry(u32 pinyin);
static bool LoadPinyinBlock(int blockIndex);
static bool OnPinyinKeyPressed(VirtualKeyboard *keyboard, Key *key);
static bool GetPinyinComposition(VirtualKeyboard *keyboard, TextComposition *composition);
static void DrawPinyinCandidates(const VirtualKeyboard *keyboard);
static int ProcessPinyinCandidateTouch(VirtualKeyboard *keyboard, int x, int y);

static int GetPinyinCodeLength(u32 pinyin) {
    if (pinyin & 0xc0000000) {
        return -1;
    }

    int length = 0;
    bool ended = false;
    for (int i = 0; i < MAX_PINYIN_PREFIX_LENGTH; i++) {
        int value = (pinyin >> (25 - i * 5)) & 0x1f;
        if (value == 0) {
            ended = true;
        }
        else if (ended || value > 26) {
            return -1;
        }
        else {
            length++;
        }
    }
    return length > 0 ? length : -1;
}

static bool IsValidPinyinDbMagic(const u8 *magic) {
    static const u8 expected[8] = {'P', 'Y', 'D', 'B', '1', 0, 0, 0};
    for (int i = 0; i < 8; i++) {
        if (magic[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

static bool ValidatePinyinDb(PinyinInputMethodEx *inputMethod) {
    PinyinDbHeader *header = &inputMethod->startup.header;
    if (!IsValidPinyinDbMagic(header->magic) ||
        header->version != PINYIN_DB_VERSION ||
        header->headerSize != PINYIN_DB_HEADER_SIZE ||
        header->level1Num == 0 ||
        header->level1Num > MAX_LEVEL1_ENTRY_NUM ||
        header->maxPrefixLength != MAX_PINYIN_PREFIX_LENGTH ||
        header->level1Offset != PINYIN_DB_HEADER_SIZE ||
        header->dataOffset != PINYIN_DB_STARTUP_SIZE ||
        header->fileSize != FS_GetLength(&inputMethod->file) ||
        header->fileSize > 0x7fffffff ||
        header->maxBlockSize == 0 ||
        header->maxBlockSize > MAX_PINYIN_BLOCK_SIZE ||
        header->maxBlockSize % PINYIN_DB_SECTOR_SIZE != 0) {
        return false;
    }

    u32 maximumBlockSize = 0;
    for (int i = 0; i < header->level1Num; i++) {
        PinyinDbLevel1Entry *entry = &inputMethod->startup.entries[i];
        u32 blockEnd = i + 1 < header->level1Num
            ? inputMethod->startup.entries[i + 1].blockOffset
            : header->fileSize;
        if (GetPinyinCodeLength(entry->pinyin) < 0 ||
            (i > 0 && inputMethod->startup.entries[i - 1].pinyin >= entry->pinyin) ||
            entry->blockOffset < header->dataOffset ||
            entry->blockOffset % PINYIN_DB_SECTOR_SIZE != 0 ||
            blockEnd <= entry->blockOffset ||
            blockEnd - entry->blockOffset > header->maxBlockSize ||
            (blockEnd - entry->blockOffset) % PINYIN_DB_SECTOR_SIZE != 0) {
            return false;
        }
        if (i == 0 && entry->blockOffset != header->dataOffset) {
            return false;
        }
        if (blockEnd - entry->blockOffset > maximumBlockSize) {
            maximumBlockSize = blockEnd - entry->blockOffset;
        }
    }
    return maximumBlockSize == header->maxBlockSize;
}

static int GetPinyinSuffixLength(const u8 *suffix, int maximumLength) {
    for (int i = 0; i < maximumLength; i++) {
        if (suffix[i] == 0) {
            return i;
        }
        if (suffix[i] < 'a' || suffix[i] > 'z') {
            return -1;
        }
    }
    return -1;
}

static int ComparePinyinString(const u8 *left, const u8 *right) {
    while (*left == *right && *left != 0) {
        left++;
        right++;
    }
    return (int)*left - (int)*right;
}

static bool ValidatePinyinBlock(PinyinInputMethodEx *inputMethod, int blockIndex, int blockSize) {
    u8 *block = inputMethod->block.data;
    PinyinDbBlockHeader *header = (PinyinDbBlockHeader *)block;
    PinyinDbEntry *entries = (PinyinDbEntry *)(block + sizeof(PinyinDbBlockHeader));
    int prefixLength = GetPinyinCodeLength(inputMethod->startup.entries[blockIndex].pinyin);
    u32 pinyinCursor = sizeof(PinyinDbBlockHeader) + header->pinyinNum * sizeof(PinyinDbEntry);

    if (header->pinyinNum == 0 ||
        header->candidateNum == 0 ||
        prefixLength < 0 ||
        pinyinCursor > (u32)blockSize ||
        header->candidateOffset < pinyinCursor ||
        header->candidateOffset > (u32)blockSize ||
        header->candidateOffset & 1) {
        return false;
    }

    u16 *candidateData = (u16 *)(block + header->candidateOffset);
    u32 candidateCapacity = (blockSize - header->candidateOffset) / sizeof(u16);
    u32 candidateCursor = 0;
    u32 candidateNum = 0;
    const u8 *previousSuffix = NULL;

    for (int i = 0; i < header->pinyinNum; i++) {
        PinyinDbEntry *entry = &entries[i];
        if (entry->pinyinOffset != pinyinCursor ||
            entry->pinyinOffset >= header->candidateOffset ||
            entry->candidateOffset != candidateCursor ||
            entry->candidateNum == 0) {
            return false;
        }

        u8 *suffix = block + entry->pinyinOffset;
        int suffixLength = GetPinyinSuffixLength(
            suffix, header->candidateOffset - entry->pinyinOffset
        );
        if (suffixLength < 0 ||
            prefixLength + suffixLength > MAX_PINYIN_LENGTH ||
            (previousSuffix && ComparePinyinString(previousSuffix, suffix) >= 0)) {
            return false;
        }
        previousSuffix = suffix;
        pinyinCursor += suffixLength + 1;

        for (int j = 0; j < entry->candidateNum; j++) {
            if (candidateCursor >= candidateCapacity) {
                return false;
            }
            u16 wordLength = candidateData[candidateCursor++];
            if (wordLength == 0 || candidateCursor + wordLength > candidateCapacity) {
                return false;
            }
            candidateCursor += wordLength;
        }
        candidateNum += entry->candidateNum;
    }

    if (pinyinCursor > header->candidateOffset ||
        header->candidateOffset - pinyinCursor > 1 ||
        candidateNum != header->candidateNum) {
        return false;
    }
    for (u32 i = header->candidateOffset + candidateCursor * sizeof(u16);
         i < (u32)blockSize; i++) {
        if (block[i] != 0) {
            return false;
        }
    }
    return true;
}

static int SearchLevel1Entry(u32 pinyin) {
    int left = 0;
    int right = gPinyinInputMethodEx->startup.header.level1Num - 1;
    while(left <= right) {
        int mid = (left + right) / 2;
        if(gPinyinInputMethodEx->startup.entries[mid].pinyin == pinyin) {
            return mid;
        } else if(gPinyinInputMethodEx->startup.entries[mid].pinyin < pinyin) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;
}

static bool LoadPinyinBlock(int blockIndex) {
    if (gPinyinInputMethodEx->blockIndex == blockIndex) {
        return true;
    }

    PinyinDbHeader *header = &gPinyinInputMethodEx->startup.header;
    u32 blockOffset = gPinyinInputMethodEx->startup.entries[blockIndex].blockOffset;
    u32 blockEnd = blockIndex + 1 < header->level1Num
        ? gPinyinInputMethodEx->startup.entries[blockIndex + 1].blockOffset
        : header->fileSize;
    int blockSize = blockEnd - blockOffset;

    if (!FS_SeekFile(&gPinyinInputMethodEx->file, blockOffset, 0) ||
        FS_ReadFile(
            &gPinyinInputMethodEx->file,
            gPinyinInputMethodEx->block.data,
            blockSize
        ) != blockSize ||
        !ValidatePinyinBlock(gPinyinInputMethodEx, blockIndex, blockSize)) {
        gPinyinInputMethodEx->blockIndex = -1;
        return false;
    }

    gPinyinInputMethodEx->blockIndex = blockIndex;
    return true;
}

static int ComparePinyinSuffix(const u8 *suffix, const char *query, int queryLength) {
    for (int i = 0; i < queryLength; i++) {
        if (suffix[i] < (u8)query[i]) {
            return -1;
        }
        if (suffix[i] > (u8)query[i]) {
            return 1;
        }
    }
    return 0;
}

KeyboardInputMethodInterface *GetPinyinInputMethodInterfaceEx() {
    if (!gPinyinInputMethodEx) {
        return NULL;
    }
    return &gPinyinInputMethodEx->interface;
}

void InitPinyinInputMethodEx() {
    if (gPinyinInputMethodEx) {
        return;
    }

    PinyinInputMethodEx *inputMethod = malloc(sizeof(PinyinInputMethodEx));
    if (!inputMethod) {
        return;
    }
    memset(inputMethod, 0, sizeof(PinyinInputMethodEx));
    inputMethod->blockIndex = -1;
    FS_InitFile(&inputMethod->file);

    if (!FS_OpenFile(&inputMethod->file, PINYIN_DB_PATH)) {
        free(inputMethod);
        return;
    }
    if (FS_ReadFile(
            &inputMethod->file,
            &inputMethod->startup,
            sizeof(PinyinDbStartup)
        ) != sizeof(PinyinDbStartup) ||
        !ValidatePinyinDb(inputMethod)) {
        FS_CloseFile(&inputMethod->file);
        free(inputMethod);
        return;
    }
    inputMethod->interface.OnKeyPressed = OnPinyinKeyPressed;
    inputMethod->interface.GetComposition = GetPinyinComposition;
    inputMethod->interface.OnGlobalDraw = DrawPinyinCandidates;
    inputMethod->interface.OnGlobalTouch = ProcessPinyinCandidateTouch;
    gPinyinInputMethodEx = inputMethod;
}

void DeinitPinyinInputMethodEx() {
    if(gPinyinInputMethodEx) {
        FS_CloseFile(&gPinyinInputMethodEx->file);
        free(gPinyinInputMethodEx);
        gPinyinInputMethodEx = NULL;
    }
}

int SearchPinyinDb(const char *pinyin, int pinyinLength) {
    if (!gPinyinInputMethodEx || !pinyin ||
        pinyinLength < 1 || pinyinLength > MAX_PINYIN_LENGTH) {
        return -1;
    }

    gPinyinInputMethodEx->candidateOffset = 0;
    gPinyinInputMethodEx->candidateNum = 0;
    for (int i = 0; i < pinyinLength; i++) {
        if (pinyin[i] < 'a' || pinyin[i] > 'z') {
            return -1;
        }
    }

    int blockIndex = -1;
    int prefixLength = pinyinLength < MAX_PINYIN_PREFIX_LENGTH
        ? pinyinLength : MAX_PINYIN_PREFIX_LENGTH;
    for (; prefixLength > 0; prefixLength--) {
        u32 prefix = 0;
        for (int i = 0; i < prefixLength; i++) {
            prefix |= ((pinyin[i] - 'a' + 1) & 0x1f) << (25 - i * 5);
        }
        blockIndex = SearchLevel1Entry(prefix);
        if (blockIndex >= 0) {
            break;
        }
    }
    if (blockIndex < 0) {
        return 0;
    }
    if (!LoadPinyinBlock(blockIndex)) {
        return -1;
    }

    PinyinDbBlockHeader *header = (PinyinDbBlockHeader *)gPinyinInputMethodEx->block.data;
    PinyinDbEntry *entries = (PinyinDbEntry *)(
        gPinyinInputMethodEx->block.data + sizeof(PinyinDbBlockHeader)
    );
    const char *query = pinyin + prefixLength;
    int queryLength = pinyinLength - prefixLength;

    int left = 0;
    int right = header->pinyinNum;
    while (left < right) {
        int mid = (left + right) / 2;
        const u8 *suffix = gPinyinInputMethodEx->block.data + entries[mid].pinyinOffset;
        if (ComparePinyinSuffix(suffix, query, queryLength) < 0) {
            left = mid + 1;
        }
        else {
            right = mid;
        }
    }
    int first = left;

    right = header->pinyinNum;
    while (left < right) {
        int mid = (left + right) / 2;
        const u8 *suffix = gPinyinInputMethodEx->block.data + entries[mid].pinyinOffset;
        if (ComparePinyinSuffix(suffix, query, queryLength) <= 0) {
            left = mid + 1;
        }
        else {
            right = mid;
        }
    }
    int last = left;
    if (first == last) {
        return 0;
    }

    gPinyinInputMethodEx->candidateOffset = entries[first].candidateOffset;
    for (int i = first; i < last; i++) {
        gPinyinInputMethodEx->candidateNum += entries[i].candidateNum;
    }
    return gPinyinInputMethodEx->candidateNum;
}

int GetPinyinDbCandidateNum() {
    if (!gPinyinInputMethodEx) {
        return 0;
    }
    return gPinyinInputMethodEx->candidateNum;
}

bool GetPinyinDbCandidate(int index, const u16 **candidate, int *candidateLength) {
    if (!gPinyinInputMethodEx || !candidate || !candidateLength ||
        index < 0 || index >= gPinyinInputMethodEx->candidateNum) {
        return false;
    }

    PinyinDbBlockHeader *header = (PinyinDbBlockHeader *)gPinyinInputMethodEx->block.data;
    u16 *candidateData = (u16 *)(
        gPinyinInputMethodEx->block.data + header->candidateOffset
    );
    int offset = gPinyinInputMethodEx->candidateOffset;
    for (int i = 0; i < index; i++) {
        offset += candidateData[offset] + 1;
    }
    *candidateLength = candidateData[offset];
    *candidate = &candidateData[offset + 1];
    return true;
}

static u16 *GetPinyinCandidateData() {
    if (!gPinyinInputMethodEx || gPinyinInputMethodEx->candidateNum == 0) {
        return NULL;
    }

    PinyinDbBlockHeader *header = (PinyinDbBlockHeader *)gPinyinInputMethodEx->block.data;
    return (u16 *)(gPinyinInputMethodEx->block.data + header->candidateOffset);
}

static void ClearPinyinUiState() {
    gPinyinInputMethodEx->inputLetterNum = 0;
    gPinyinInputMethodEx->candidateOffset = 0;
    gPinyinInputMethodEx->candidateNum = 0;
    gPinyinInputMethodEx->candidatePageStart = 0;
    memset(&gPinyinInputMethodEx->candidateLayout, 0, sizeof(PinyinCandidateLayout));
}

static void RefreshPinyinCandidates() {
    gPinyinInputMethodEx->candidatePageStart = 0;
    memset(&gPinyinInputMethodEx->candidateLayout, 0, sizeof(PinyinCandidateLayout));

    if (gPinyinInputMethodEx->inputLetterNum == 0) {
        gPinyinInputMethodEx->candidateOffset = 0;
        gPinyinInputMethodEx->candidateNum = 0;
        return;
    }

    if (SearchPinyinDb(
            (const char *)gPinyinInputMethodEx->inputLetter,
            gPinyinInputMethodEx->inputLetterNum
        ) < 0) {
        gPinyinInputMethodEx->candidateOffset = 0;
        gPinyinInputMethodEx->candidateNum = 0;
    }
}

static int MeasurePinyinCandidate(const u16 *candidate, int candidateLength, int maximumWidth) {
    int advanceWidth = 0;
    int renderedWidth = 0;
    for (int i = 0; i < candidateLength; i++) {
        glImage glyph;
        int palIndex;
        int advance;
        if (!GetExternalGlyph(candidate[i], &glyph, &palIndex, &advance) || advance <= 0) {
            return -1;
        }
        int glyphRight = advanceWidth + glyph.width;
        if (glyphRight > renderedWidth) {
            renderedWidth = glyphRight;
        }
        advanceWidth += advance;
        if (advanceWidth > renderedWidth) {
            renderedWidth = advanceWidth;
        }
        if (renderedWidth > maximumWidth) {
            return renderedWidth;
        }
    }
    return renderedWidth;
}

static void BuildPinyinCandidateLayout(const VirtualKeyboard *keyboard, int pageStart) {
    PinyinCandidateLayout *layout = &gPinyinInputMethodEx->candidateLayout;
    const TextBox *textBox = &keyboard->inputTextBox;
    u16 *candidateData = GetPinyinCandidateData();
    int candidateNum = gPinyinInputMethodEx->candidateNum;
    memset(layout, 0, sizeof(PinyinCandidateLayout));

    if (pageStart < 0) {
        pageStart = 0;
    }
    if (pageStart > candidateNum) {
        pageStart = candidateNum;
    }
    layout->nextPageStart = candidateNum;
    if (!candidateData || pageStart == candidateNum) {
        return;
    }

    int contentX = textBox->x + CANDIDATE_ARROW_WIDTH + CANDIDATE_TEXT_PADDING;
    int contentRight = textBox->x + textBox->width -
        CANDIDATE_ARROW_WIDTH - CANDIDATE_TEXT_PADDING;
    int contentWidth = contentRight - contentX;
    int remainingLength = textBox->maxLength - textBox->length;
    if (remainingLength <= 0) {
        return;
    }
    int candidateOffset = gPinyinInputMethodEx->candidateOffset;
    for (int i = 0; i < pageStart; i++) {
        candidateOffset += candidateData[candidateOffset] + 1;
    }

    int candidateIndex = pageStart;
    int drawX = contentX;
    while (candidateIndex < candidateNum) {
        if (layout->itemNum >= MAX_VISIBLE_CANDIDATE_NUM) {
            break;
        }

        int candidateLength = candidateData[candidateOffset];
        const u16 *candidate = &candidateData[candidateOffset + 1];
        int candidateWidth = -1;
        if (candidateLength <= remainingLength) {
            candidateWidth = MeasurePinyinCandidate(candidate, candidateLength, contentWidth);
        }

        if (candidateWidth > 0 && candidateWidth <= contentWidth) {
            if (drawX + candidateWidth > contentRight) {
                break;
            }

            PinyinCandidateLayoutItem *item = &layout->items[layout->itemNum++];
            item->candidateOffset = candidateOffset;
            item->x = drawX;
            item->width = candidateWidth;
            drawX += candidateWidth + CANDIDATE_WORD_SPACING;
        }

        candidateOffset += candidateLength + 1;
        candidateIndex++;
    }
    layout->nextPageStart = candidateIndex;
}

static int FindPreviousCandidatePage(const VirtualKeyboard *keyboard, int currentPageStart) {
    int pageStart = 0;
    while (pageStart < currentPageStart) {
        BuildPinyinCandidateLayout(keyboard, pageStart);
        PinyinCandidateLayout *layout = &gPinyinInputMethodEx->candidateLayout;
        if (layout->itemNum == 0 ||
            layout->nextPageStart <= pageStart ||
            layout->nextPageStart >= currentPageStart) {
            return pageStart;
        }
        pageStart = layout->nextPageStart;
    }
    return 0;
}

static void DrawPinyinCandidateArrow(
        const VirtualKeyboard *keyboard,
        int x,
        int y,
        KeyCode keyCode,
        bool enabled) {
    glBoxFilled(
        x,
        y,
        x + CANDIDATE_ARROW_WIDTH - 1,
        y + CANDIDATE_BAR_HEIGHT - 1,
        CANDIDATE_ARROW_BG_COLOR
    );
    if (!enabled) {
        return;
    }

    glImage *glyph = GetDefaultGlyph(keyCode);
    if (glyph) {
        SetDefaultKeysPalette(keyboard->functionKeyTexPalId);
        glSprite(
            x + (CANDIDATE_ARROW_WIDTH - glyph->width) / 2,
            y + (CANDIDATE_BAR_HEIGHT - glyph->height) / 2,
            GL_FLIP_NONE,
            glyph
        );
    }
}

static void DrawPinyinCandidateWord(
        const VirtualKeyboard *keyboard,
        const PinyinCandidateLayoutItem *item,
        int y) {
    u16 *candidateData = GetPinyinCandidateData();
    int candidateLength = candidateData[item->candidateOffset];
    const u16 *candidate = &candidateData[item->candidateOffset + 1];
    int x = item->x;

    for (int i = 0; i < candidateLength; i++) {
        glImage glyph;
        int palIndex;
        int advance;
        if (!GetExternalGlyph(candidate[i], &glyph, &palIndex, &advance)) {
            return;
        }
        glSetActiveTexture(glyph.textureID);
        glAssignColorTable(0, keyboard->externalGlyphCandidatePalIds[palIndex]);
        glSprite(
            x,
            y + (CANDIDATE_BAR_HEIGHT - glyph.height) / 2,
            GL_FLIP_NONE,
            &glyph
        );
        x += advance;
    }
}

static void DrawPinyinCandidates(const VirtualKeyboard *keyboard) {
    if (!gPinyinInputMethodEx ||
        gPinyinInputMethodEx->inputLetterNum == 0 ||
        gPinyinInputMethodEx->candidateNum == 0) {
        return;
    }

    BuildPinyinCandidateLayout(keyboard, gPinyinInputMethodEx->candidatePageStart);
    PinyinCandidateLayout *layout = &gPinyinInputMethodEx->candidateLayout;
    if (layout->itemNum == 0) {
        return;
    }

    const TextBox *textBox = &keyboard->inputTextBox;
    int barX = textBox->x;
    int barY = textBox->y + textBox->height + CANDIDATE_BAR_GAP;
    glBoxFilled(
        barX,
        barY,
        barX + textBox->width - 1,
        barY + CANDIDATE_BAR_HEIGHT - 1,
        CANDIDATE_BG_COLOR
    );

    DrawPinyinCandidateArrow(
        keyboard,
        barX,
        barY,
        KEYCODE_LEFT_TRIANGLE,
        gPinyinInputMethodEx->candidatePageStart > 0
    );
    DrawPinyinCandidateArrow(
        keyboard,
        barX + textBox->width - CANDIDATE_ARROW_WIDTH,
        barY,
        KEYCODE_RIGHT_TRIANGLE,
        layout->nextPageStart < gPinyinInputMethodEx->candidateNum
    );

    for (int i = 0; i < layout->itemNum; i++) {
        DrawPinyinCandidateWord(keyboard, &layout->items[i], barY);
    }

    glLine(
        barX + CANDIDATE_ARROW_WIDTH,
        barY,
        barX + CANDIDATE_ARROW_WIDTH,
        barY + CANDIDATE_BAR_HEIGHT - 1,
        CANDIDATE_BORDER_COLOR
    );
    glLine(
        barX + textBox->width - CANDIDATE_ARROW_WIDTH - 1,
        barY,
        barX + textBox->width - CANDIDATE_ARROW_WIDTH - 1,
        barY + CANDIDATE_BAR_HEIGHT - 1,
        CANDIDATE_BORDER_COLOR
    );
    DrawKeyboardOutlineBox(
        barX - 1,
        barY - 1,
        barX + textBox->width,
        barY + CANDIDATE_BAR_HEIGHT,
        CANDIDATE_BORDER_COLOR
    );
}

static int ProcessPinyinCandidateTouch(VirtualKeyboard *keyboard, int x, int y) {
    if (!gPinyinInputMethodEx ||
        gPinyinInputMethodEx->inputLetterNum == 0 ||
        gPinyinInputMethodEx->candidateNum == 0) {
        return KEY_STATE_NOT_PRESSED;
    }

    TextBox *textBox = &keyboard->inputTextBox;
    int barX = textBox->x;
    int barY = textBox->y + textBox->height + CANDIDATE_BAR_GAP;
    if (x < barX || x >= barX + textBox->width ||
        y < barY || y >= barY + CANDIDATE_BAR_HEIGHT) {
        return KEY_STATE_NOT_PRESSED;
    }

    BuildPinyinCandidateLayout(keyboard, gPinyinInputMethodEx->candidatePageStart);
    PinyinCandidateLayout *layout = &gPinyinInputMethodEx->candidateLayout;
    if (layout->itemNum == 0) {
        return KEY_STATE_NOT_PRESSED;
    }
    if (keyboard->isPressed) {
        return KEY_STATE_HELD;
    }

    if (x < barX + CANDIDATE_ARROW_WIDTH) {
        if (gPinyinInputMethodEx->candidatePageStart > 0) {
            gPinyinInputMethodEx->candidatePageStart = FindPreviousCandidatePage(
                keyboard,
                gPinyinInputMethodEx->candidatePageStart
            );
        }
        return KEY_STATE_PRESSED;
    }

    if (x >= barX + textBox->width - CANDIDATE_ARROW_WIDTH) {
        if (layout->nextPageStart < gPinyinInputMethodEx->candidateNum) {
            gPinyinInputMethodEx->candidatePageStart = layout->nextPageStart;
        }
        return KEY_STATE_PRESSED;
    }

    for (int i = 0; i < layout->itemNum; i++) {
        PinyinCandidateLayoutItem *item = &layout->items[i];
        if (x >= item->x && x < item->x + item->width) {
            u16 *candidateData = GetPinyinCandidateData();
            int candidateLength = candidateData[item->candidateOffset];
            const u16 *candidate = &candidateData[item->candidateOffset + 1];
            for (int j = 0; j < candidateLength; j++) {
                TryAddCharToInput(candidate[j]);
            }
            ClearPinyinUiState();
            return KEY_STATE_PRESSED;
        }
    }
    return KEY_STATE_PRESSED;
}

static bool OnPinyinKeyPressed(VirtualKeyboard *keyboard, Key *key) {
    (void)keyboard;

    if (key->code >= KEYCODE_a && key->code <= KEYCODE_z) {
        if (gPinyinInputMethodEx->inputLetterNum < MAX_PINYIN_LENGTH) {
            gPinyinInputMethodEx->inputLetter[gPinyinInputMethodEx->inputLetterNum++] = key->code;
            RefreshPinyinCandidates();
        }
        return true;
    }

    if (key->code == KEYCODE_BACKSPACE) {
        if (gPinyinInputMethodEx->inputLetterNum > 0) {
            gPinyinInputMethodEx->inputLetterNum--;
            RefreshPinyinCandidates();
            return true;
        }
        return false;
    }

    if (key->code == KEYCODE_ENTER) {
        if (gPinyinInputMethodEx->inputLetterNum > 0) {
            for (int i = 0; i < gPinyinInputMethodEx->inputLetterNum; i++) {
                TryAddKeycodeToInput(HalfToFullWidth(gPinyinInputMethodEx->inputLetter[i]));
            }
            ClearPinyinUiState();
            return true;
        }
        return false;
    }

    if (key->code == KEYCODE_SHIFT ||
        key->code == KEYCODE_CAPS_LOCK ||
        (key->code & KEYCODE_FLAG_LANGUAGE)) {
        ClearPinyinUiState();
        return false;
    }

    if (key->code >= KEYCODE_SPACE && key->code <= KEYCODE_TILDE) {
        ClearPinyinUiState();
        if (key->code == KEYCODE_PERIOD) {
            TryAddKeycodeToInput(KEYCODE_CHINESE_PERIOD);
        }
        else {
            TryAddKeycodeToInput(HalfToFullWidth(key->code));
        }
        return true;
    }
    return false;
}

static bool GetPinyinComposition(VirtualKeyboard *keyboard, TextComposition *composition) {
    if (!gPinyinInputMethodEx || gPinyinInputMethodEx->inputLetterNum == 0) {
        return false;
    }

    for (int i = 0; i < gPinyinInputMethodEx->inputLetterNum; i++) {
        gPinyinInputMethodEx->compositionText[i] = gPinyinInputMethodEx->inputLetter[i];
    }
    composition->text = gPinyinInputMethodEx->compositionText;
    composition->length = gPinyinInputMethodEx->inputLetterNum;
    composition->start = keyboard->inputTextBox.cursorPosition;
    composition->useDefaultGlyph = true;
    composition->underline = true;
    return true;
}
