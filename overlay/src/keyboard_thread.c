#include <nds/ndstypes.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <calico/arm/common.h>
#include <nds/debug.h>
#include "nitro/thread.h"
#include "hook.h"
#include "keyboard.h"
#include "touch.h"

static bool gKeyboardVisible;
extern u32 OrigLauncherThreadLR;
extern u32 OrigLauncherThreadPC;
extern OSContext LanucherThreadContext;
extern void *OSi_IrqThreadQueue[];

void JumpFromLauncherThread();
void *MpuGetDTCMRegion();

void OS_SaveContext();
__attribute__((weak)) extern void *SVC_WaitVBlankIntr;
__attribute__((weak)) extern u32 SVC_WaitVBlankIntr_Caller;
void Hook_SVC_WaitVBlankIntr();

OSThread backboardThread;
u32 stack[512 / sizeof(u32)];

void WaitVBlankIntr() {
    vu32 *irqCheckFlags = (vu32 *)MpuGetDTCMRegion() + 0x3FF8 / sizeof(u32);
    swiDelay(1);
    ArmIrqState state = armIrqLockByPsr();
    *irqCheckFlags &= ~IRQ_VBLANK;
    armIrqUnlockByPsr(state);
    while (!(*irqCheckFlags & IRQ_VBLANK))
        OS_SleepThread(OSi_IrqThreadQueue);
}

bool GetBranchLinkAddr(u32 lr, u32 *addr) {
    if (lr >= 0x1FF8000 && lr < 0x23E0000) {
        if (lr & 1) {  // Thumb mode
            lr &= ~1;
            lr -= 4;
            u16 instr1 = *(u16 *)lr;
            u16 instr2 = *(u16 *)(lr + 2);
            
            // Check for Thumb BLX
            if ((instr1 & 0xF800) == 0xF000 && (instr2 & 0xE800) == 0xE800) {
                // Extract immediate value and compute target address
                s32 offset = (s32)(((instr1 & 0x7FF) << 12) | ((instr2 & 0x7FF) << 1)) << 9 >> 9;
                *addr = (lr + 4) + offset;
                if (!(instr2 & 0x1000)) {
                    *addr &= ~3;
                }
                return true;
            }
        }
        else {  // ARM mode
            lr -= 4;
            u32 instr = *(u32 *)lr;
            
            // Check for ARM BL
            if ((instr & 0x0F000000) == 0x0B000000) {
                s32 offset = (s32)(instr & 0x00FFFFFF) << 8 >> 6;
                *addr = (lr + 8) + offset;
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
    u32 addr, mode;
    u32 caller = (u32)&SVC_WaitVBlankIntr_Caller;
    void * SVC_WaitVBlankIntrPtr = &SVC_WaitVBlankIntr;
    mode = 0;
    if (caller && SVC_WaitVBlankIntrPtr) {
        int ime = enterCriticalSection();
        ForceMakingBranchLink((void*)(caller - 4), Hook_SVC_WaitVBlankIntr);
        leaveCriticalSection(ime);
        mode = 1;
    }
    for (;;) {
        switch (state)
        {
        case 0:
            if (interface->ShouldShowKeyboard())
                state++;
            else
                OS_SleepThread(OSi_IrqThreadQueue);
            break;
        case 1:
            int ime = enterCriticalSection();
            if (mode == 1) {
                gKeyboardVisible = true;
                state++;
            }
            else {
                if (context->lr >= 0x01FF8000 && context->lr < 0x023E0000) {
                    if (GetBranchLinkAddr(context->lr, &addr)) {
                        if (addr == (u32)OS_SaveContext) {
                            gKeyboardVisible = true;
                            OrigLauncherThreadLR = context->lr;
                            OrigLauncherThreadPC = context->pc_plus4 - 4;
                            context->lr = (u32)JumpFromLauncherThread;
                            state++;
                        }
                    }
                }
            }
            leaveCriticalSection(ime);
            OS_SleepThread(OSi_IrqThreadQueue);
            break;
        case 2:
            OS_SleepThread(NULL);
            state = 0;
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
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    void *heap = interface->Alloc(KEYBOARD_HEAP_SIZE);

    if (!heap) {
        gKeyboardVisible = false;
        OS_WakeupThreadDirect(&backboardThread);
        return;
    }

    InitHeap(heap, KEYBOARD_HEAP_SIZE);

    u32 i;

    u32 dispcnt = REG_DISPCNT;
    u32 disp3dcnt = GFX_CONTROL;
    u16 bg0cnt = REG_BG0CNT;
    u16 bg1cnt = REG_BG1CNT;
    u16 bg2cnt = REG_BG2CNT;
    u16 bg3cnt = REG_BG3CNT;

    u8 vramCRs[10];
    for (i = 0; i < 10; i++) {
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
    glEnable(GL_TEXTURE_2D);
    
    // enable antialiasing
    glEnable(GL_ANTIALIAS);

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

    // GFX_CLEAR_COLOR = 0x3F003FFF;
    // GFX_CLEAR_DEPTH = 0x7FFF;


    glMatrixMode(GL_PROJECTION);
    glLoadMatrix4x4(&matrixProjection);

    ResetTPData();
    FinalizeKeyboard(result != 2);
    //    glFlush(0);

    interface->Free(heap);
    gKeyboardVisible = false;
    OS_WakeupThreadDirect(&backboardThread);
}

void Hook_SVC_WaitVBlankIntr() {
    void (* volatile func)() = (void (* volatile)())&SVC_WaitVBlankIntr;
    func();
    if (gKeyboardVisible)
        LanucherThreadExt();
}

void StartKeyboardMonitorThread() {
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    interface->OnOverlayLoaded();
    OS_CreateThread(&backboardThread, MonitorThreadEntry, 0, stack + ARRAY_SIZE(stack), sizeof(stack), 8);
    OS_WakeupThreadDirect(&backboardThread);
}