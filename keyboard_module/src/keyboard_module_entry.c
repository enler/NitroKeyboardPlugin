#include <nds/ndstypes.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#include <nds/bios.h>
#include <nds/debug.h>
#include <nds/system.h>
#include <string.h>
#include "keyboard.h"
#include "touch.h"

void WaitVBlankIntr(void);
static void SetBrightness(u8 screen, s8 bright) {
    u16 mode = 1 << 14;

    if (bright < 0) {
        mode = 2 << 14;
        bright = -bright;
    }
    if (bright > 31) {
        bright = 31;
    }
    *(vu16 *)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

void KeyboardModule_Run(void) {
    KeyboardGameInterface *interface = GetKeyboardGameInterface();

    u32 i;

    u32 dispcnt = REG_DISPCNT;
    u32 disp3dcnt = GFX_CONTROL;
    u16 bg0cnt = REG_BG0CNT;
    u16 bg1cnt = REG_BG1CNT;
    u16 bg2cnt = REG_BG2CNT;
    u16 bg3cnt = REG_BG3CNT;

    u8 vramCRs[10];
    for (i = 0; i < 10; i++) {
        if (i == 7)
            continue;
        vramCRs[i] = *(vu8 *)(0x04000240 + i);
        *(vu8 *)(0x04000240 + i) = 0;
    }

    u8 *vramABackup = malloc(1024 * 4);
    u8 *vramEBackup = malloc(256);

    VRAM_A_CR = VRAM_ENABLE;
    VRAM_E_CR = VRAM_ENABLE;

    memcpy(vramABackup, VRAM_A, 1024 * 4);
    memcpy(vramEBackup, VRAM_E, 256);

    u16 powercnt = REG_POWERCNT;
    m4x4 matrixProjection;

    REG_DISPCNT = MODE_0_3D | DISPLAY_BG0_ACTIVE;
    REG_BG0CNT = 0;
    REG_BG1CNT = 0;
    REG_BG2CNT = 0;
    REG_BG3CNT = 0;
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    REG_POWERCNT |= POWER_3D_CORE | POWER_MATRIX;

    glGetFixed(GL_GET_MATRIX_PROJECTION, matrixProjection.m);

    glInit();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ANTIALIAS);
    glViewport(0, 0, 255, 191);

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);

    REG_POWERCNT &= ~POWER_SWAP_LCDS;

    SetBrightness(0, 0);

    InitializeKeyboard(interface);
    InitPinyinInputMethod();
    RegisterKeyboardInputMethod(KEYBOARD_LANG_CHS, GetPinyinInputMethodInterface());

    int state = 0;
    int result = 0;
    while (result != 2 && result != 3) {
        switch (state) {
        case 0:
            glBegin2D();
            DrawKeyboard();
            glEnd2D();
            glFlush(0);
            result = HandleKeyboardInput();
            state = 1;
            break;
        case 1:
            RequestSamplingTPData();
            state = 0;
            break;
        default:
            break;
        }
        WaitVBlankIntr();
    }

    i = 30;
    while (i--)
        WaitVBlankIntr();

    DeinitPinyinInputMethod();
    glResetTextures();

    REG_DISPCNT = dispcnt;
    GFX_CONTROL = disp3dcnt;
    REG_BG0CNT = bg0cnt;
    REG_BG1CNT = bg1cnt;
    REG_BG2CNT = bg2cnt;
    REG_BG3CNT = bg3cnt;

    REG_POWERCNT = powercnt;

    VRAM_A_CR = VRAM_ENABLE;
    VRAM_E_CR = VRAM_ENABLE;

    memcpy(VRAM_A, vramABackup, 1024 * 4);
    memcpy(VRAM_E, vramEBackup, 256);

    free(vramABackup);
    free(vramEBackup);

    for (i = 0; i < 10; i++) {
        if (i == 7)
            continue;
        *(vu8 *)(0x04000240 + i) = vramCRs[i];
    }

    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&matrixProjection);

    ResetTPData();
    FinalizeKeyboard(result != 2);
}
