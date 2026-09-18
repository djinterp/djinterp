/******************************************************************************
* djinterp [jit]                                                     jit_arm.h
*
* djinterp AArch32 / ARM (A32) JIT encoder (fixed-width ARM instructions):
*   The 32-bit ARM half of the djinterp JIT, targeting the A32 instruction set
* (ARMv7). Every instruction is one 32-bit little-endian word; each emitter
* packs fields into a base opcode and emits it with d_jit_emit_u32. It is
* wholly separate from the AArch64 encoder (jit_arm64.h) -- A32 and A64 share
* neither encodings nor a register model -- exactly as x86 and x86-64 are
* separate modules.
*
*   Every A32 instruction carries a 4-bit condition in bits [31:28]; these
* emitters use AL (always) except b.cond. Data-processing immediate forms take
* an ARM 'modified immediate' (8-bit value rotated right by an even amount) and
* return -1 for a constant that cannot be so expressed; emit_mov_imm32 loads
* any 32-bit constant via movw/movt. Base opcodes cross-checked against
* llvm-mc -show-encoding.
*
*   Registers are R0-R15; aliases SP = R13, LR = R14, PC = R15.
*
*   NAMING CONVENTION:
*     D_JIT_ARM_REG_[NAME] - a register operand number (0-15)
*     D_JIT_ARM_CC_[COND]  - a condition code (0-15) for b.cond
*     D_JIT_ARM_OP_[NAME]  - a 32-bit base opcode (fields zeroed, cond = AL)
*     d_jit_arm_emit_*     - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_arm.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_ARM_
#define DJINTERP_JIT_ARM_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_ARM_ENCODING
//   feature: 1, indicating the A32 (ARM) encoder is available.
#ifndef D_JIT_ARM_ENCODING
    #define D_JIT_ARM_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   R0-R15 are numbers 0-15. R13/R14/R15 have the usual aliases.

#define D_JIT_ARM_REG_R0       0
#define D_JIT_ARM_REG_R1       1
#define D_JIT_ARM_REG_R2       2
#define D_JIT_ARM_REG_R3       3
#define D_JIT_ARM_REG_R4       4
#define D_JIT_ARM_REG_R5       5
#define D_JIT_ARM_REG_R6       6
#define D_JIT_ARM_REG_R7       7
#define D_JIT_ARM_REG_R8       8
#define D_JIT_ARM_REG_R9       9
#define D_JIT_ARM_REG_R10      10
#define D_JIT_ARM_REG_R11      11
#define D_JIT_ARM_REG_R12      12
#define D_JIT_ARM_REG_R13      13
#define D_JIT_ARM_REG_R14      14
#define D_JIT_ARM_REG_R15      15
#define D_JIT_ARM_REG_SP      13     // stack pointer alias (R13)
#define D_JIT_ARM_REG_LR      14     // link register alias (R14)
#define D_JIT_ARM_REG_PC      15     // program counter alias (R15)


// ===========================================================================
// C.   CONDITION CODES
// ===========================================================================
//   The 4-bit condition field (bits [31:28]) for b.cond. AL is "always".

// D_JIT_ARM_CC_EQ: equal
#define D_JIT_ARM_CC_EQ     0x0
// D_JIT_ARM_CC_NE: not equal
#define D_JIT_ARM_CC_NE     0x1
// D_JIT_ARM_CC_CS: carry set / unsigned higher or same (alias HS)
#define D_JIT_ARM_CC_CS     0x2
// D_JIT_ARM_CC_CC: carry clear / unsigned lower (alias LO)
#define D_JIT_ARM_CC_CC     0x3
// D_JIT_ARM_CC_MI: minus / negative
#define D_JIT_ARM_CC_MI     0x4
// D_JIT_ARM_CC_PL: plus / non-negative
#define D_JIT_ARM_CC_PL     0x5
// D_JIT_ARM_CC_VS: overflow set
#define D_JIT_ARM_CC_VS     0x6
// D_JIT_ARM_CC_VC: overflow clear
#define D_JIT_ARM_CC_VC     0x7
// D_JIT_ARM_CC_HI: unsigned higher
#define D_JIT_ARM_CC_HI     0x8
// D_JIT_ARM_CC_LS: unsigned lower or same
#define D_JIT_ARM_CC_LS     0x9
// D_JIT_ARM_CC_GE: signed greater or equal
#define D_JIT_ARM_CC_GE     0xA
// D_JIT_ARM_CC_LT: signed less than
#define D_JIT_ARM_CC_LT     0xB
// D_JIT_ARM_CC_GT: signed greater than
#define D_JIT_ARM_CC_GT     0xC
// D_JIT_ARM_CC_LE: signed less or equal
#define D_JIT_ARM_CC_LE     0xD
// D_JIT_ARM_CC_AL: always
#define D_JIT_ARM_CC_AL     0xE
#define D_JIT_ARM_CC_HS      D_JIT_ARM_CC_CS
#define D_JIT_ARM_CC_LO      D_JIT_ARM_CC_CC


// ===========================================================================
// D.   FIELD POSITIONS
// ===========================================================================
// D_JIT_ARM_RD_SHIFT
//   constant: destination register Rd (data-processing), bits [15:12].
#define D_JIT_ARM_RD_SHIFT   12

// D_JIT_ARM_RN_SHIFT
//   constant: first source / base register Rn, bits [19:16].
#define D_JIT_ARM_RN_SHIFT   16

// D_JIT_ARM_RM_SHIFT
//   constant: second source register Rm, bits [3:0].
#define D_JIT_ARM_RM_SHIFT   0

// D_JIT_ARM_COND_SHIFT
//   constant: condition field, bits [31:28].
#define D_JIT_ARM_COND_SHIFT 28


// ===========================================================================
// E.   BASE OPCODES
// ===========================================================================
//   Complete 32-bit words (cond = AL, operand fields zero); emitters OR the
// register and immediate fields in. Verified encodings.

// D_JIT_ARM_OP_MOV_REG
//   constant: mov Rd, Rm.
#define D_JIT_ARM_OP_MOV_REG 0xE1A00000

// D_JIT_ARM_OP_MOV_IMM
//   constant: mov Rd, #modified_imm.
#define D_JIT_ARM_OP_MOV_IMM 0xE3A00000

// D_JIT_ARM_OP_MOVW
//   constant: movw Rd, #imm16  (imm4:imm12; zero-extends).
#define D_JIT_ARM_OP_MOVW    0xE3000000

// D_JIT_ARM_OP_MOVT
//   constant: movt Rd, #imm16  (sets top halfword).
#define D_JIT_ARM_OP_MOVT    0xE3400000

// D_JIT_ARM_OP_ADD_REG
//   constant: add Rd, Rn, Rm.
#define D_JIT_ARM_OP_ADD_REG 0xE0800000

// D_JIT_ARM_OP_SUB_REG
//   constant: sub Rd, Rn, Rm.
#define D_JIT_ARM_OP_SUB_REG 0xE0400000

// D_JIT_ARM_OP_AND_REG
//   constant: and Rd, Rn, Rm.
#define D_JIT_ARM_OP_AND_REG 0xE0000000

// D_JIT_ARM_OP_ORR_REG
//   constant: orr Rd, Rn, Rm.
#define D_JIT_ARM_OP_ORR_REG 0xE1800000

// D_JIT_ARM_OP_EOR_REG
//   constant: eor Rd, Rn, Rm.
#define D_JIT_ARM_OP_EOR_REG 0xE0200000

// D_JIT_ARM_OP_ADD_IMM
//   constant: add Rd, Rn, #modified_imm.
#define D_JIT_ARM_OP_ADD_IMM 0xE2800000

// D_JIT_ARM_OP_SUB_IMM
//   constant: sub Rd, Rn, #modified_imm.
#define D_JIT_ARM_OP_SUB_IMM 0xE2400000

// D_JIT_ARM_OP_CMP_REG
//   constant: cmp Rn, Rm (sets flags; Rd = 0).
#define D_JIT_ARM_OP_CMP_REG 0xE1500000

// D_JIT_ARM_OP_CMP_IMM
//   constant: cmp Rn, #modified_imm.
#define D_JIT_ARM_OP_CMP_IMM 0xE3500000

// D_JIT_ARM_OP_MUL
//   constant: mul Rd, Rn, Rm (Rd[19:16], Rn[3:0], Rm[11:8]).
#define D_JIT_ARM_OP_MUL     0xE0000090

// D_JIT_ARM_OP_LDR_IMM
//   constant: ldr Rt, [Rn, #imm12]  (P=1, U=1, add offset).
#define D_JIT_ARM_OP_LDR_IMM 0xE5900000

// D_JIT_ARM_OP_STR_IMM
//   constant: str Rt, [Rn, #imm12].
#define D_JIT_ARM_OP_STR_IMM 0xE5800000

// D_JIT_ARM_OP_B
//   constant: b label   (imm24, offset from PC+8 /4).
#define D_JIT_ARM_OP_B       0xEA000000

// D_JIT_ARM_OP_BL
//   constant: bl label  (imm24).
#define D_JIT_ARM_OP_BL      0xEB000000

// D_JIT_ARM_OP_B_COND
//   constant: b.cond: OR the condition into bits [31:28].
#define D_JIT_ARM_OP_B_COND  0x0A000000

// D_JIT_ARM_OP_BX
//   constant: bx Rm     (branch/exchange; bx lr returns).
#define D_JIT_ARM_OP_BX      0xE12FFF10

// D_JIT_ARM_OP_NOP
//   constant: nop (hint).
#define D_JIT_ARM_OP_NOP     0xE320F000

// D_JIT_ARM_OP_PUSH
//   constant: push {list} (stmdb sp!, register bitmask [15:0]).
#define D_JIT_ARM_OP_PUSH    0xE92D0000

// D_JIT_ARM_OP_POP
//   constant: pop  {list} (ldmia sp!, register bitmask [15:0]).
#define D_JIT_ARM_OP_POP     0xE8BD0000


// ===========================================================================
// F.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands into the base opcode and emits the 32-bit word.
// Register arguments are D_JIT_ARM_REG_* numbers; branch targets are
// d_jit_label (see jit.h). Each returns 0 on success, -1 on an emit,
// range/alignment, or non-encodable-immediate error.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// no-op and moves
// ---------------------------------------------------------------------------

// d_jit_arm_emit_nop
//   function: nop.
D_NODISCARD int d_jit_arm_emit_nop(d_jit_buffer* _buf);

// d_jit_arm_emit_mov_reg
//   function: mov Rd, Rm.
D_NODISCARD int d_jit_arm_emit_mov_reg(d_jit_buffer* _buf, int _rd, int _rm);

// d_jit_arm_emit_mov_imm
//   function: mov Rd, #_imm when _imm is an ARM modified immediate; otherwise
// returns -1 (use d_jit_arm_emit_mov_imm32).
D_NODISCARD int d_jit_arm_emit_mov_imm(d_jit_buffer* _buf, int _rd,
                                       uint32_t _imm);

// d_jit_arm_emit_mov_imm32
//   function: load any 32-bit constant into Rd -- movw the low halfword, then
// movt the high halfword when it is non-zero (1-2 instructions).
D_NODISCARD int d_jit_arm_emit_mov_imm32(d_jit_buffer* _buf, int _rd,
                                         uint32_t _imm);

// d_jit_arm_emit_movw
//   function: movw Rd, #_imm16 (zero-extends into Rd).
D_NODISCARD int d_jit_arm_emit_movw(d_jit_buffer* _buf, int _rd,
                                    uint16_t _imm16);

// d_jit_arm_emit_movt
//   function: movt Rd, #_imm16 (sets the top halfword of Rd).
D_NODISCARD int d_jit_arm_emit_movt(d_jit_buffer* _buf, int _rd,
                                    uint16_t _imm16);

// ---------------------------------------------------------------------------
// arithmetic and logic (register)
// ---------------------------------------------------------------------------

// d_jit_arm_emit_add_reg
//   function: add Rd, Rn, Rm.
D_NODISCARD int d_jit_arm_emit_add_reg(d_jit_buffer* _buf, int _rd, int _rn,
                                       int _rm);

// d_jit_arm_emit_sub_reg
//   function: sub Rd, Rn, Rm.
D_NODISCARD int d_jit_arm_emit_sub_reg(d_jit_buffer* _buf, int _rd, int _rn,
                                       int _rm);

// d_jit_arm_emit_and_reg
//   function: and Rd, Rn, Rm.
D_NODISCARD int d_jit_arm_emit_and_reg(d_jit_buffer* _buf, int _rd, int _rn,
                                       int _rm);

// d_jit_arm_emit_orr_reg
//   function: orr Rd, Rn, Rm.
D_NODISCARD int d_jit_arm_emit_orr_reg(d_jit_buffer* _buf, int _rd, int _rn,
                                       int _rm);

// d_jit_arm_emit_eor_reg
//   function: eor Rd, Rn, Rm.
D_NODISCARD int d_jit_arm_emit_eor_reg(d_jit_buffer* _buf, int _rd, int _rn,
                                       int _rm);

// d_jit_arm_emit_mul
//   function: mul Rd, Rn, Rm (Rd = Rn * Rm).
D_NODISCARD int d_jit_arm_emit_mul(d_jit_buffer* _buf, int _rd, int _rn,
                                   int _rm);

// d_jit_arm_emit_cmp_reg
//   function: cmp Rn, Rm.
D_NODISCARD int d_jit_arm_emit_cmp_reg(d_jit_buffer* _buf, int _rn, int _rm);

// ---------------------------------------------------------------------------
// arithmetic (modified immediate)
// ---------------------------------------------------------------------------

// d_jit_arm_emit_add_imm
//   function: add Rd, Rn, #_imm (modified immediate; -1 if not encodable).
D_NODISCARD int d_jit_arm_emit_add_imm(d_jit_buffer* _buf, int _rd, int _rn,
                                       uint32_t _imm);

// d_jit_arm_emit_sub_imm
//   function: sub Rd, Rn, #_imm (modified immediate; -1 if not encodable).
D_NODISCARD int d_jit_arm_emit_sub_imm(d_jit_buffer* _buf, int _rd, int _rn,
                                       uint32_t _imm);

// d_jit_arm_emit_cmp_imm
//   function: cmp Rn, #_imm (modified immediate; -1 if not encodable).
D_NODISCARD int d_jit_arm_emit_cmp_imm(d_jit_buffer* _buf, int _rn,
                                       uint32_t _imm);

// ---------------------------------------------------------------------------
// load / store (immediate offset)
// ---------------------------------------------------------------------------

// d_jit_arm_emit_ldr
//   function: ldr Rt, [Rn, #_offset]. _offset is a byte offset, 0-4095.
D_NODISCARD int d_jit_arm_emit_ldr(d_jit_buffer* _buf, int _rt, int _rn,
                                   uint32_t _offset);

// d_jit_arm_emit_str
//   function: str Rt, [Rn, #_offset]. _offset is a byte offset, 0-4095.
D_NODISCARD int d_jit_arm_emit_str(d_jit_buffer* _buf, int _rt, int _rn,
                                   uint32_t _offset);

// ---------------------------------------------------------------------------
// stack (register-list)
// ---------------------------------------------------------------------------

// d_jit_arm_emit_push
//   function: push {list} -- _reglist is a 16-bit mask (bit r selects Rr).
D_NODISCARD int d_jit_arm_emit_push(d_jit_buffer* _buf, unsigned _reglist);

// d_jit_arm_emit_pop
//   function: pop {list} -- _reglist is a 16-bit mask (bit r selects Rr).
D_NODISCARD int d_jit_arm_emit_pop(d_jit_buffer* _buf, unsigned _reglist);

// ---------------------------------------------------------------------------
// branches (targets are d_jit_label; see jit.h)
// ---------------------------------------------------------------------------

// d_jit_arm_emit_b
//   function: b _target -- unconditional branch (+/-32 MB).
D_NODISCARD int d_jit_arm_emit_b(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_arm_emit_bl
//   function: bl _target -- branch with link (call).
D_NODISCARD int d_jit_arm_emit_bl(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_arm_emit_b_cond
//   function: b.cond _target. _cc is a D_JIT_ARM_CC_* value.
D_NODISCARD int d_jit_arm_emit_b_cond(d_jit_buffer* _buf, int _cc,
                                      d_jit_label* _target);

// d_jit_arm_emit_bx
//   function: bx Rm -- branch/exchange; bx LR is the standard return.
D_NODISCARD int d_jit_arm_emit_bx(d_jit_buffer* _buf, int _rm);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_arm_reg_name
//   function: the name "r0".."r15" (or "?"). SP/LR/PC print as r13/r14/r15.
const char* d_jit_arm_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_ARM_
