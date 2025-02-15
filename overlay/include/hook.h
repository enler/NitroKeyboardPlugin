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
} HookDataEntry;

typedef struct {
    void * functionAddr;
    void ** origFunctionRef;
    u32 ldrInstruction;
    void *hookFunction;
    u16 *oldInstructions;
} HookThumbEntry;

void HookFunction(HookDataEntry * entry);
void ForceMakingBranchLink(void *origAddr, void *targetAddr);
void HookFunctionThumb(HookThumbEntry *entry);
void SetupHookMemory(void *memory, u32 size);

#define PATCH_WORD(addr, value) do { \
    *(volatile u32 *)(addr) = (value); \
    DC_FlushRange((void *)(addr), sizeof(u32)); \
} while (0)

#endif