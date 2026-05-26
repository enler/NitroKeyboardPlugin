#include <nds/ndstypes.h>
#include <nds/bios.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <calico/arm/common.h>
#include "nitro/fs.h"
#include "nitro/thread.h"
#include "hook.h"
#include "keyboard.h"

#define KEYBOARD_KMOD_PATH "keyboard/keyboard.kmod"

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

typedef void (*KeyboardModuleEntry)(void);

OSThread gMonitorThread;

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

void LanucherThreadExt() {
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    FSFile file;
    void *heap = NULL;
    u8 *module = NULL;

    do {
        FS_InitFile(&file);
        if (!FS_OpenFile(&file, KEYBOARD_KMOD_PATH))
            break;

        u32 moduleSize = FS_GetLength(&file);
        if (moduleSize == 0)
            break;

        u32 totalHeapSize = KEYBOARD_HEAP_SIZE + moduleSize;

        heap = interface->Alloc(totalHeapSize);
        if (!heap)
            break;

        InitHeap(heap, totalHeapSize);

        module = malloc(moduleSize);
        if (!module)
            break;

        if (FS_ReadFile(&file, module, moduleSize) != (s32)moduleSize)
            break;

        DC_FlushRange(module, moduleSize);

        ((KeyboardModuleEntry)module)();
    } while (0);

    if (module)
        free(module);
    if (heap)
        interface->Free(heap);
    FS_CloseFile(&file);
    gKeyboardVisible = false;
    OS_WakeupThreadDirect(&gMonitorThread);
}

void Hook_SVC_WaitVBlankIntr() {
    void (* volatile func)() = (void (* volatile)())&SVC_WaitVBlankIntr;
    func();
    if (gKeyboardVisible)
        LanucherThreadExt();
}

void StartKeyboardMonitorThread() {
    static u32 stack[512 / sizeof(u32)];
    KeyboardGameInterface *interface = GetKeyboardGameInterface();
    interface->OnOverlayLoaded();
    OS_CreateThread(&gMonitorThread, MonitorThreadEntry, 0, stack + ARRAY_SIZE(stack), sizeof(stack), 8);
    OS_WakeupThreadDirect(&gMonitorThread);
}
