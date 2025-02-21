#include <nds/ndstypes.h>
#include <stdlib.h>
#include "arm_instruction.h"
#include "hook.h"

static void *sHookMemory = NULL;
static u32 sHookMemorySize = 0;

void HookFunction(HookARMEntry * entry) {
	entry->oldInstruction = *(u32*)entry->functionAddr;
    if ((entry->oldInstruction & 0xFFFF0000) == 0xE59F0000) {
        entry->extraData = *(u32*)((u32)entry->functionAddr + 8 + (entry->oldInstruction & 0xFFF));
        entry->oldInstruction = entry->oldInstruction & ~0xFFF;
    }
    else if ((entry->oldInstruction & 0xFF000000) == 0xEA000000) {
        u32 targetAddr = (u32)entry->functionAddr + 8 + ((s32)entry->oldInstruction << 8 >> 6);
        entry->oldInstruction = MAKE_BRANCH(&entry->oldInstruction, targetAddr);
    }
	*(u32*)entry->functionAddr = MAKE_BRANCH(entry->functionAddr, &entry->ldrInstruction);
    entry->ldrInstruction = 0xE51FF004;
    entry->jumpInstruction = MAKE_BRANCH(&entry->jumpInstruction, (u32)entry->functionAddr + 4);
	if (entry->origFunctionRef)
		*entry->origFunctionRef = (void*)&entry->oldInstruction;
	DC_FlushRange(entry->functionAddr, sizeof(u32));
	DC_FlushRange(entry, sizeof(HookARMEntry));
}



void SetupHookMemory(void *memory, u32 size) {
    if ((u32)memory & 3) {
        u32 adjust = 4 - ((u32)memory & 3);
        sHookMemory = (void*)((u32)memory + adjust);
        sHookMemorySize = size - adjust;
    }
    else {
        sHookMemory = memory;
        sHookMemorySize = size;
    }
    sHookMemorySize &= ~3;
}

static void *AllocHookMemory(u32 size) {
    void *result = NULL;
    size = (size + 3) & ~3;
    if (size <= sHookMemorySize) {
        result = sHookMemory;
        sHookMemory = (void*)((u32)sHookMemory + size);
        sHookMemorySize -= size;
    }
    return result;
}

void HookFunctionThumb(HookThumbEntry * entry) {
    u16 *begin = (u16*)((u32)entry->functionAddr & ~1);
    u16 *end = (u16*)(((u32)entry->functionAddr & ~3) + 8);
    int require = 0;
    int offsets[5] = {-1, -1, -1, -1, -1};
    int *pOffsets = offsets;
    int offset = 0;
    while (begin < end) {
        *pOffsets++ = offset;
        if ((*begin & 0xF800) == 0x4800) { // ldr rd, [pc, imm]
            require += 2;
        }
        else if ((*begin & 0xF000) == 0xD000 && (*begin & 0x0E00) != 0x0E00) { // b{cond} label
            u32 dest = (u32)begin + 4 + ((s32)(*begin & 0xFF) << 24 >> 23);
            if (dest < ((u32)entry->functionAddr & ~1) || dest >= end)
                require += 6;
        }
        else if ((*begin & 0xF800) == 0xE000) { // b label
            u32 dest = (u32)begin + 4 + ((s32)(*begin & 0x3FF) << 22 >> 21);
            if (dest < ((u32)entry->functionAddr & ~1) || dest >= end)
                require += 6;
        }
        else if ((*begin & 0xF800 == 0xA000)) { // add rd, pc, imm
            require += 2;
        }
        else if ((*begin & 0xF800) == 0xF000 && (*(begin + 1) & 0xE800) == 0xE800) {// bl label, blx label
            require++;
            begin++;
            *pOffsets++ = ++offset;
        }
        else if ((*begin & 0xff78) == 0x4478) { // add rd, pc
            require += 5;
            offset += 3;
        }
        else if ((*begin & 0xff78) == 0x4678) { // mov rd, pc
            if ((*begin & 0x80) == 0x80) {
                require += 5;
                offset += 3;
            }
            else {
                require += 2;
            }
        }
        require++;
        begin++;
        offset++;
    }
    begin = (u16*)((u32)entry->functionAddr & ~1);
    require = require * sizeof(u16) + 12 - (require & 1) * sizeof(u16);

    if (!entry->oldInstructions)
        entry->oldInstructions = (u16*)AllocHookMemory(require);

    offset = 0;
    int tail = require / sizeof(u16);
    while (begin < end) {
        if ((*begin & 0xF800) == 0x4800) { // ldr rd, [pc, imm]
            u32 dest = (u32)begin + 4 + ((*begin & 0xFF) << 2);
            dest &= ~3;
            tail -= 2;
            *(u32*)&entry->oldInstructions[tail] = *(u32*)dest;
            u32 rd = (*begin & 0x0700) >> 8;
            entry->oldInstructions[offset++] = MAKE_LDR_PC_OFF_T(rd, offset * 2, tail * 2);
            begin++;
        }
        else if ((*begin & 0xF000) == 0xD000 && (*begin & 0x0E00) != 0x0E00) { // b{cond} label
            u32 dest = (u32)begin + 4 + ((s32)(*begin & 0xFF) << 24 >> 23);
            if (dest < ((u32)entry->functionAddr & ~1) || dest >= (u32)end) {
                tail -= 6;
                entry->oldInstructions[tail] = BX_PC_T;
                entry->oldInstructions[tail + 1] = NOP_T;
                *(u32 *)(&entry->oldInstructions[tail + 2]) = 0xE51FF004;
                *(u32 *)(&entry->oldInstructions[tail + 4]) = dest|1;
                entry->oldInstructions[offset] = MAKE_BARNCH_COND_T(offset * 2, tail * 2, (*begin & 0xF00) >> 8);
            }
            else {
                entry->oldInstructions[offset] = MAKE_BARNCH_COND_T(offset * 2, offsets[(dest - ((u32)entry->functionAddr & ~1)) / 2] * 2, (*begin & 0xF00) >> 8);
            }
            offset++;
            begin++;
        }
        else if ((*begin & 0xF800) == 0xE000) { // b label
            u32 dest = (u32)begin + 4 + ((s32)(*begin & 0x7FF) << 21 >> 20);
            if (dest < ((u32)entry->functionAddr & ~1) || dest >= (u32)end) {
                tail -= 6;
                entry->oldInstructions[tail] = BX_PC_T;
                entry->oldInstructions[tail + 1] = NOP_T;
                *(u32 *)(&entry->oldInstructions[tail + 2]) = 0xE51FF004;
                *(u32 *)(&entry->oldInstructions[tail + 4]) = dest|1;
                entry->oldInstructions[offset] = MAKE_BRANCH_T(offset * 2, tail * 2);
            }
            else {
                entry->oldInstructions[offset] = MAKE_BRANCH_T(offset * 2, offsets[(dest - ((u32)entry->functionAddr & ~1)) / 2] * 2);
            }
            offset++;
            begin++;
        }
        else if ((*begin & 0xF800 == 0xA000)) { // add rd, pc, imm
            u32 dest = ((u32)begin & ~3) + 4 + ((*begin & 0xFF) << 2);
            tail -= 2;
            *(u32*)&entry->oldInstructions[tail] = *(u32*)dest;
            u32 rd = (*begin & 0x0700) >> 8;
            entry->oldInstructions[offset++] = MAKE_LDR_PC_OFF_T(rd, offset * 2, tail * 2);
            begin++;
        }
        else if ((*begin & 0xF800) == 0xF000 && (*(begin + 1) & 0xE800) == 0xE800) {// bl label, blx label
            u32 dest = (u32)begin + 4 + (s32)((*begin & 0x7FF) << 12 | (*(begin + 1) & 0x7FF) << 1) << 9 >> 9;
            if (*(begin + 1) & 0x1000) {
                entry->oldInstructions[offset] = MAKE_BRANCH_LINK_T_H(&entry->oldInstructions[offset], dest);
                entry->oldInstructions[offset + 1] = MAKE_BRANCH_LINK_T_L(&entry->oldInstructions[offset], dest);
            }
            else {
                entry->oldInstructions[offset] = MAKE_BRANCH_LINK_EXCHAGE_T_H(&entry->oldInstructions[offset], dest);
                entry->oldInstructions[offset + 1] = MAKE_BRANCH_LINK_EXCHAGE_T_L(&entry->oldInstructions[offset], dest);
            }
            offset += 2;
            begin += 2;
        }
        else if ((*begin & 0xff78) == 0x4478) { // add rd, pc
            u8 rd = *begin & 0x7;
            u8 rt = rd == REG_R0 ? REG_R1 : REG_R0;
            tail -= 2;
            *(u32*)&entry->oldInstructions[tail] = (u32)begin + 4;
            entry->oldInstructions[offset++] = MAKE_PUSH_T(1 << rt);
            entry->oldInstructions[offset++] = MAKE_LDR_PC_OFF_T(rt, offset * 2, tail * 2);
            entry->oldInstructions[offset++] = MAKE_ADD_RD_RS_RN_T(rd, rd, rt);
            entry->oldInstructions[offset++] = MAKE_POP_T(1 << rt);
            begin++;
        }
        else if ((*begin & 0xff78) == 0x4678) { // mov rd, pc
            u32 rd = (*begin & 0x7) | (*begin & 0x80) >> 4;
            if (rd >= REG_R8) {
                tail -= 2;
                *(u32*)&entry->oldInstructions[tail] = (u32)begin + 4;
                entry->oldInstructions[offset++] = MAKE_PUSH_T(REG_R0_F);
                entry->oldInstructions[offset++] = MAKE_LDR_PC_OFF_T(REG_R0, offset * 2, tail * 2);
                entry->oldInstructions[offset++] = MAKE_MOV_RD_RS_T(rd, REG_R0);
                entry->oldInstructions[offset++] = MAKE_POP_T(REG_R0_F);
            }
            else {
                tail -= 2;
                *(u32*)&entry->oldInstructions[tail] = (u32)begin + 4;
                entry->oldInstructions[offset++] = MAKE_LDR_PC_OFF_T(rd, offset * 2, tail * 2);
            }
            begin++;
        }
        else {
            entry->oldInstructions[offset++] = *begin++;
        }
    }
    entry->oldInstructions[offset++] = BX_PC_T;
    if (offset & 1){
        entry->oldInstructions[offset++] = NOP_T;
    }
    *(u32*)&entry->oldInstructions[offset] = 0xE51FF004;
    *(u32*)&entry->oldInstructions[offset + 2] = (u32)begin | 1;

    DC_FlushRange(entry->oldInstructions, require);

    begin = (u16*)((u32)entry->functionAddr & ~1);
    *begin++ = BX_PC_T;
    if ((u32)begin % 4 != 0) {
        *begin++ = NOP_T;
    }
    *(u32*)begin = MAKE_BRANCH(begin, &entry->ldrInstruction);
    DC_FlushRange((void*)((u32)entry->functionAddr & ~3), 8);

    entry->ldrInstruction = 0xE51FF004;
    DC_FlushRange(&entry->ldrInstruction, sizeof(u32));

    if (entry->origFunctionRef)
        *entry->origFunctionRef = (void*)((u32)entry->oldInstructions | 1);
}

void ForceMakingBranchLink(void* origAddr, void* targetAddr) {
    int mode;
    if ((u32)origAddr & 1) {
        if ((u32)targetAddr & 1) {
            mode = 2;
        }
        else {
            mode = 3;
        }
    }
    else {
        if ((u32)targetAddr & 1) {
            mode = 1;
        }
        else {
            mode = 0;
        }
    }

    if (mode == 0 || mode == 1) {
        origAddr = (void*)((u32)origAddr & ~3);
    } else {
        origAddr = (void*)((u32)origAddr & ~1);
    }

    if (mode == 0 || mode == 3) {
        targetAddr = (void*)((u32)targetAddr & ~3);
    } else {
        targetAddr = (void*)((u32)targetAddr & ~1);
    }

    if (mode == 0)
        *(u32*)origAddr = MAKE_BRANCH_LINK(origAddr, targetAddr);
    else if (mode == 1)
        *(u32*)origAddr = MAKE_BRANCH_LINK_EXCHANGE(origAddr, targetAddr);
    else if (mode == 2) {
        *(u16*)origAddr = MAKE_BRANCH_LINK_T_H(origAddr, targetAddr);
        *(u16*)((u32)origAddr + 2) = MAKE_BRANCH_LINK_T_L(origAddr, targetAddr);
    }
    else {
        *(u16*)origAddr = MAKE_BRANCH_LINK_EXCHAGE_T_H(origAddr, targetAddr);
        *(u16*)((u32)origAddr + 2) = MAKE_BRANCH_LINK_EXCHAGE_T_L(origAddr, targetAddr);
    }

    DC_FlushRange((void *)((u32)origAddr & ~3), 8);
}