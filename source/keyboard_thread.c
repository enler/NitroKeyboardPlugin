#include <nds/ndstypes.h>
#include <gl2d.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include "keyboard.h"
#include "thread.h"
#include "touch.h"

static vu32 userExit = 0;
extern u32 OrigLauncherThreadLR;
extern u32 OrigLauncherThreadPC;
extern OSContext LanucherThreadContext;

void JumpFromLauncherThread();

void OS_SaveContext();
void OS_WaitIrq(bool clear, u32 mask);

OSThread backboardThread;
u8 stack[512];

void WaitVBlankIntr() {
    swiDelay(1);
    OS_WaitIrq(true, IRQ_VBLANK);
}

bool GetBranchLinkAddr(u32 lr, u32 *addr) {
    if (lr >= 0x2000000 && lr <= 0x23E0000) {
        if (lr & 1) {  // Thumb mode
            lr &= ~1;
            lr -= 4;
            u16 instr1 = *(u16 *)lr;
            u16 instr2 = *(u16 *)(lr + 2);
            
            // Check for Thumb BLX
            if ((instr1 & 0xF800) == 0xF000 && (instr2 & 0xF800) == 0xE800) {
                // Extract immediate value and compute target address
                u32 S = (instr1 & 0x0400) >> 10;
                u32 imm10 = (instr1 & 0x03FF);
                u32 J1 = (instr2 & 0x2000) >> 13;
                u32 J2 = (instr2 & 0x0800) >> 11;
                u32 imm11 = (instr2 & 0x07FF);
                
                // Reconstruct signed immediate
                u32 I1 = ~(J1 ^ S) & 1;
                u32 I2 = ~(J2 ^ S) & 1;
                u32 imm32 = (S << 24) | (I1 << 23) | (I2 << 22) | (imm10 << 12) | (imm11 << 1);
                // Sign extend
                if (S) {
                    imm32 |= 0xFE000000;
                }
                
                *addr = (lr + 4) + imm32;
                return true;
            }
        }
        else {  // ARM mode
            lr -= 4;
            u32 instr = *(u32 *)lr;
            
            // Check for ARM BL
            if ((instr & 0x0F000000) == 0x0B000000) {
                u32 offset = instr & 0x00FFFFFF;
                // Sign extend 24-bit offset
                if (offset & 0x00800000) {
                    offset |= 0xFF000000;
                }
                *addr = (lr + 8) + (offset << 2);
                return true;
            }
        }
    }
    return false;  // Don't forget to return false if no branch found
}


void MonitorThreadEntry(void* arg) {
    u32 state = 0;
    OSContext *context = &LanucherThreadContext;
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    u32 addr;
    void *heap;
    for (;;)
    {
        switch(state) {
            case 0:
                if (interface->ShouldShowKeyboard())
                    state++;
                else
                    OS_Sleep(1);
                break;
            case 1:
                OrigLauncherThreadLR = context->lr;
                if (GetBranchLinkAddr(OrigLauncherThreadLR, &addr)) {
                    if (addr == OS_SaveContext)
                        state++;
                }
                OS_Sleep(1);
                break;
            case 2:
                if (heap = interface->Alloc(KEYBOARD_HEAP_SIZE)) {
                    InitHeap(heap, KEYBOARD_HEAP_SIZE);
                    userExit = 0;
                    OrigLauncherThreadPC = context->pc_plus4 - 4;
                    context->lr = (u32)JumpFromLauncherThread;
                    state++;
                }
                else
                    state = 0;
                break;
            case 3:
                if (userExit == 0)
                    OS_Sleep(1);
                else {
                    interface->Free(heap);
                    state = 0;
                }
                break;
        }
    }
}

void SetBrightness(u8 screen, s8 bright) {
    u16 mode = 1 << 14;

    if (bright < 0) {
        mode = 2 << 14;
        bright = -bright;
    }
    if (bright > 31) {
        bright = 31;
    }
    *(vu16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

void LanucherThreadExt() {
    u32 dispcnt = REG_DISPCNT;
    u32 disp3dcnt = GFX_CONTROL;
    u16 bg0cnt = REG_BG0CNT;
    u16 bg1cnt = REG_BG1CNT;
    u16 bg2cnt = REG_BG2CNT;
    u16 bg3cnt = REG_BG3CNT;

    u8 vramCRs[10];
    for (int i = 0; i < 10; i++) {
        if (i == 7) continue;
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

    u16 masterBright = *(vu16 *)0x0400106C;

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
    
    //enable textures
    glEnable( GL_TEXTURE_2D );
    
    // enable antialiasing
    glEnable( GL_ANTIALIAS );

    //this should work the same as the normal gl call
    glViewport(0,0,255,191);

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankE(VRAM_E_TEX_PALETTE);

    // If main screen is on auto, then force the bottom
    REG_POWERCNT &= ~POWER_SWAP_LCDS;

    SetBrightness(0, 0);
    // REG_MOSAIC = 0; // Register is write only, can't back up
    // REG_BLDCNT = 0; // Register is write only, can't back up
    // REG_BLDALPHA = 0; // Register is write only, can't back up
    // REG_BLDY = 0; // Register is write only, can't back up
    InitializeKeyboard(GetKeyboardGameInterface());
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
            state++;
            break;
        case 1:
            result = HandleKeyboardInput();
            if (result == 1)
                state = 0;
            break;
        default:
            break;
        }
        RequestSamplingTPData();
        WaitVBlankIntr();
    }

    OS_Sleep(500);

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

    for (int i = 0; i < 10; i++) {
        if (i == 7)
            continue;
        *(vu8 *)(0x04000240 + i) = vramCRs[i];
    }

    // GFX_CLEAR_COLOR = 0x3F003FFF;
    // GFX_CLEAR_DEPTH = 0x7FFF;


    glMatrixMode( GL_PROJECTION );
    glLoadMatrix4x4(&matrixProjection);

    ResetTPData();
    FinalizeKeyboard(result != 2);
    //    glFlush(0);

    userExit = 1;
}

void StartKeyboardMonitorThread() {
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    interface->OnOverlayLoaded();
    OS_CreateThread(&backboardThread, MonitorThreadEntry, 0, stack + sizeof(stack), sizeof(stack), 8);
    OS_WakeupThreadDirect(&backboardThread);
}