#ifndef HOOK_H
#define HOOK_H

#include <nds/arm9/cache.h>

typedef struct {
    void * functionAddr;
	void ** origFunctionRef;
    u32 ldrInstruction;
    void *hookFunction;
    u32 oldInstruction;
	u32 jumpInstruction;
    u32 extraData;
} HookARMEntry;

typedef struct {
    void * functionAddr;
    void ** origFunctionRef;
    u32 ldrInstruction;
    void *hookFunction;
    u16 *oldInstructions;
} HookThumbEntry;

void HookFunction(HookARMEntry * entry);
void ForceMakingBranchLink(void *origAddr, void *targetAddr);
void HookFunctionThumb(HookThumbEntry *entry);
void SetupHookMemory(void *memory, u32 size);

#define PATCH_WORD(addr, value) do { \
    *(vu32 *)(addr) = (vu32)(value); \
    DC_FlushRange((void *)(addr), sizeof(u32)); \
} while (0)

#endif