#ifndef ARM_INSTRUCTION_H
#define ARM_INSTRUCTION_H

#define REG_R0 0
#define REG_R1 1
#define REG_R2 2
#define REG_R3 3
#define REG_R4 4
#define REG_R5 5
#define REG_R6 6
#define REG_R7 7
#define REG_R8 8
#define REG_R9 9
#define REG_R10 10
#define REG_R11 11
#define REG_R12 12
#define REG_SP 13
#define REG_LR 14
#define REG_PC 15

#define REG_R0_F (1 << REG_R0)
#define REG_R1_F (1 << REG_R1)
#define REG_R2_F (1 << REG_R2)
#define REG_R3_F (1 << REG_R3)
#define REG_R4_F (1 << REG_R4)
#define REG_R5_F (1 << REG_R5)
#define REG_R6_F (1 << REG_R6)
#define REG_R7_F (1 << REG_R7)
#define REG_R8_F (1 << REG_R8)
#define REG_R9_F (1 << REG_R9)
#define REG_R10_F (1 << REG_R10)
#define REG_R11_F (1 << REG_R11)
#define REG_R12_F (1 << REG_R12)
#define REG_SP_F (1 << REG_SP)
#define REG_LR_F (1 << REG_LR)
#define REG_PC_F (1 << REG_PC)


#define COND_EQ 0x0  // Equal
#define COND_NE 0x1  // Not equal
#define COND_CS 0x2  // Carry set/unsigned higher or same
#define COND_CC 0x3  // Carry clear/unsigned lower
#define COND_MI 0x4  // Minus/negative
#define COND_PL 0x5  // Plus/positive or zero
#define COND_VS 0x6  // Overflow
#define COND_VC 0x7  // No overflow
#define COND_HI 0x8  // Unsigned higher
#define COND_LS 0x9  // Unsigned lower or same
#define COND_GE 0xA  // Signed greater than or equal
#define COND_LT 0xB  // Signed less than
#define COND_GT 0xC  // Signed greater than
#define COND_LE 0xD  // Signed less than or equal
#define COND_AL 0xE  // Always (unconditional)

#define NOP 0xE1A00000
#define NOP_T 0x46C0
#define BX_PC_T 0x4778

#define CALC_PC_RELATIVE_OFFSET_A(current_pc, target) (((s32)(target) - 8 - (s32)(current_pc)) >> 2)
#define CALC_PC_RELATIVE_OFFSET_T(current_pc, target) (((s32)(target) - 4 - (s32)(current_pc)) >> 1)
#define CALC_PC_RELATIVE_OFFSET_ALIGNED_T(current_pc, target) ((((s32)(target) & ~3) - 4 - ((s32)(current_pc) & ~3)) >> 2)

#define MAKE_BRANCH(src, dest) ((CALC_PC_RELATIVE_OFFSET_A(src, dest) & 0xFFFFFF) | 0xEA000000)
#define MAKE_BRANCH_LINK(src, dest) ((CALC_PC_RELATIVE_OFFSET_A(src, dest) & 0xFFFFFF) | 0xEB000000)
#define MAKE_BRANCH_LINK_EXCHANGE(src, dest) ((CALC_PC_RELATIVE_OFFSET_A(src, dest) & 0xFFFFFF) | (((CALC_PC_RELATIVE_OFFSET_A(src, dest) >> 1) & 1) << 24) | 0xFA000000)
#define MAKE_BRANCH_T(src, dest) (((((s32)(dest) - 4 - (s32)(src)) >> 1) & 0x7FF) | 0xE000)
#define MAKE_BARNCH_COND_T(src, dest, cond) (((((s32)(dest) - 4 - (s32)(src)) >> 1) & 0xFF) | ((cond) << 8) | 0xD000)
#define MAKE_BRANCH_LINK_T_H(src, dest) (((((s32)(dest) - 4 - (s32)(src)) >> 12) & 0x7FF) | 0xF000)
#define MAKE_BRANCH_LINK_T_L(src, dest) (((((s32)(dest) - 4 - (s32)(src)) >> 1) & 0x7FF) | 0xF800)
#define MAKE_BRANCH_LINK_EXCHAGE_T_H(src, dest) ((((((s32)(dest) & ~3) - 4 - ((s32)(src) & ~3)) >> 12) & 0x7FF) | 0xF000)
#define MAKE_BRANCH_LINK_EXCHAGE_T_L(src, dest) ((((((s32)(dest) & ~3) - 4 - ((s32)(src) & ~3)) >> 1) & 0x7FF) | 0xE800)

#define MAKE_PUSH_T(reg_flags) (0xB400 | (((reg_flags) & REG_LR_F) ? 0x80 : 0) | ((reg_flags) & 0x7F))
#define MAKE_POP_T(reg_flags) (0xBC00 | (((reg_flags) & REG_PC_F) ? 0x80 : 0) | ((reg_flags) & 0x7F))
#define MAKE_ADD_RD_RS_RN_T(rd, rs, rn) (0x1800 | (rn) << 6 | (rs) << 3 | (rd))
#define MAKE_MOV_RD_RS_T(rd, rs) (0x4600 | ((rs) << 3) | ((rd) & 7) | ((rd) & 8) << 4)
#define MAKE_ADD_RD_PC_OFF_T(rd, current_pc, addr) ((CALC_PC_RELATIVE_OFFSET_ALIGNED_T(current_pc, addr) & 0xFF) | (((rd) & 7) << 8) | 0xA000)
#define MAKE_LDR_PC_OFF_T(rd, current_pc, addr) ((CALC_PC_RELATIVE_OFFSET_ALIGNED_T(current_pc, addr) & 0xFF) | (((rd) & 7) << 8) | 0x4800)



static inline u32 CreateSubsInstruction(u8 rd, u8 rs, u16 imm) {
    u32 instruction = 0xE2500000; // Base opcode for SUBS (immediate)
    instruction |= (rd & 0xF) << 12; // Set destination register
    instruction |= (rs & 0xF) << 16; // Set source register
    instruction |= imm & 0xFFF; // Set immediate value
    return instruction;
}

static inline u32 CreateCmpInstruction(u8 rs, u16 imm) {
    u32 instruction = 0xE3500000; // Base opcode for CMP (immediate)
    instruction |= (rs & 0xF) << 16; // Set source register
    instruction |= imm & 0xFFF; // Set immediate value
    return instruction;
}

static inline u32 CreateBInstruction(u32 address, u32 target, u8 cond) {
    s32 offset = (s32)(target - address - 8) >> 2; // Calculate the offset
    u32 instruction = 0x0A000000; // Base opcode for B (branch)
    instruction |= (cond & 0xF) << 28; // Set condition code
    instruction |= offset & 0xFFFFFF; // Set offset
    return instruction;
}

#endif