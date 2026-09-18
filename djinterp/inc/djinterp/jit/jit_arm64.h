/******************************************************************************
* djinterp [jit]                                                   jit_arm64.h
*
* djinterp AArch64 (ARM64) JIT encoder (fixed-width A64 instructions):
*   The 64-bit ARM half of the djinterp JIT. AArch64 instructions are all one
* 32-bit little-endian word, so there is no ModR/M layer: each emitter packs
* register and immediate fields into a base opcode and emits the word with
* d_jit_emit_u32. This module is entirely separate from the 32-bit ARM encoder
* (jit_arm.h) -- the A64 and A32 instruction sets share neither encodings nor a
* register model -- exactly as x86 and x86-64 are separate modules.
*
*   Operations are 64-bit (X registers). The 32-bit (W) variants differ only
* only by clearing the sf bit (31), and are not exposed here. Each opcode
* below was cross-checked against llvm-mc -show-encoding.
*
*   Register numbers are 0-30 for X0-X30; number 31 is SP or the zero register
* XZR by instruction (SP in add/sub-immediate and load/store base positions,
* XZR elsewhere). Aliases: FP = X29, LR = X30.
*
*   NAMING CONVENTION:
*     D_JIT_ARM64_REG_[NAME] - a register operand number (0-31)
*     D_JIT_ARM64_CC_[COND]  - a condition code (0-15) for B.cond
*     D_JIT_ARM64_OP_[NAME]  - a 32-bit base opcode (fields zeroed)
*     D_JIT_ARM64_*_SHIFT    - the bit position of a register/immediate field
*     d_jit_arm64_emit_*     - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_arm64.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_ARM64_
#define DJINTERP_JIT_ARM64_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_ARM64_ENCODING
//   feature: 1, indicating the AArch64 encoder is available.
#ifndef D_JIT_ARM64_ENCODING
    #define D_JIT_ARM64_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   X0-X30 are numbers 0-30. Number 31 encodes SP in add/sub-immediate and
// load/store base positions, and the zero register XZR everywhere else; both
// spellings are provided for readable call sites.

#define D_JIT_ARM64_REG_X0      0
#define D_JIT_ARM64_REG_X1      1
#define D_JIT_ARM64_REG_X2      2
#define D_JIT_ARM64_REG_X3      3
#define D_JIT_ARM64_REG_X4      4
#define D_JIT_ARM64_REG_X5      5
#define D_JIT_ARM64_REG_X6      6
#define D_JIT_ARM64_REG_X7      7
#define D_JIT_ARM64_REG_X8      8
#define D_JIT_ARM64_REG_X9      9
#define D_JIT_ARM64_REG_X10     10
#define D_JIT_ARM64_REG_X11     11
#define D_JIT_ARM64_REG_X12     12
#define D_JIT_ARM64_REG_X13     13
#define D_JIT_ARM64_REG_X14     14
#define D_JIT_ARM64_REG_X15     15
#define D_JIT_ARM64_REG_X16     16
#define D_JIT_ARM64_REG_X17     17
#define D_JIT_ARM64_REG_X18     18
#define D_JIT_ARM64_REG_X19     19
#define D_JIT_ARM64_REG_X20     20
#define D_JIT_ARM64_REG_X21     21
#define D_JIT_ARM64_REG_X22     22
#define D_JIT_ARM64_REG_X23     23
#define D_JIT_ARM64_REG_X24     24
#define D_JIT_ARM64_REG_X25     25
#define D_JIT_ARM64_REG_X26     26
#define D_JIT_ARM64_REG_X27     27
#define D_JIT_ARM64_REG_X28     28
#define D_JIT_ARM64_REG_X29     29
#define D_JIT_ARM64_REG_X30     30
#define D_JIT_ARM64_REG_SP     31
#define D_JIT_ARM64_REG_XZR    31
#define D_JIT_ARM64_REG_FP     29     // frame pointer alias (X29)
#define D_JIT_ARM64_REG_LR     30     // link register alias (X30)


// ===========================================================================
// C.   CONDITION CODES
// ===========================================================================
//   The 4-bit condition field for B.cond (and the basis of the ARM condition
// system generally). AL is "always".

// D_JIT_ARM64_CC_EQ: equal (Z=1)
#define D_JIT_ARM64_CC_EQ     0x0
// D_JIT_ARM64_CC_NE: not equal (Z=0)
#define D_JIT_ARM64_CC_NE     0x1
// D_JIT_ARM64_CC_CS: carry set / unsigned higher or same (alias HS)
#define D_JIT_ARM64_CC_CS     0x2
// D_JIT_ARM64_CC_CC: carry clear / unsigned lower (alias LO)
#define D_JIT_ARM64_CC_CC     0x3
// D_JIT_ARM64_CC_MI: minus / negative (N=1)
#define D_JIT_ARM64_CC_MI     0x4
// D_JIT_ARM64_CC_PL: plus / non-negative (N=0)
#define D_JIT_ARM64_CC_PL     0x5
// D_JIT_ARM64_CC_VS: overflow set (V=1)
#define D_JIT_ARM64_CC_VS     0x6
// D_JIT_ARM64_CC_VC: overflow clear (V=0)
#define D_JIT_ARM64_CC_VC     0x7
// D_JIT_ARM64_CC_HI: unsigned higher
#define D_JIT_ARM64_CC_HI     0x8
// D_JIT_ARM64_CC_LS: unsigned lower or same
#define D_JIT_ARM64_CC_LS     0x9
// D_JIT_ARM64_CC_GE: signed greater or equal
#define D_JIT_ARM64_CC_GE     0xA
// D_JIT_ARM64_CC_LT: signed less than
#define D_JIT_ARM64_CC_LT     0xB
// D_JIT_ARM64_CC_GT: signed greater than
#define D_JIT_ARM64_CC_GT     0xC
// D_JIT_ARM64_CC_LE: signed less or equal
#define D_JIT_ARM64_CC_LE     0xD
// D_JIT_ARM64_CC_AL: always
#define D_JIT_ARM64_CC_AL     0xE
// aliases
#define D_JIT_ARM64_CC_HS     D_JIT_ARM64_CC_CS
#define D_JIT_ARM64_CC_LO     D_JIT_ARM64_CC_CC


// ===========================================================================
// D.   FIELD POSITIONS
// ===========================================================================
//   Common field bit positions shared across the A64 formats used here.

// D_JIT_ARM64_RD_SHIFT
//   constant: destination register Rd, bits [4:0].
#define D_JIT_ARM64_RD_SHIFT    0

// D_JIT_ARM64_RN_SHIFT
//   constant: first source / base register Rn, bits [9:5].
#define D_JIT_ARM64_RN_SHIFT    5

// D_JIT_ARM64_RM_SHIFT
//   constant: second source register Rm, bits [20:16].
#define D_JIT_ARM64_RM_SHIFT    16

// D_JIT_ARM64_IMM12_SHIFT
//   constant: 12-bit immediate (add/sub, scaled ldr/str), bits [21:10].
#define D_JIT_ARM64_IMM12_SHIFT 10

// D_JIT_ARM64_IMM16_SHIFT
//   constant: 16-bit immediate (movz/movk/movn), bits [20:5].
#define D_JIT_ARM64_IMM16_SHIFT 5

// D_JIT_ARM64_HW_SHIFT
//   constant: move-wide halfword-shift selector hw, bits [22:21].
#define D_JIT_ARM64_HW_SHIFT    21

// D_JIT_ARM64_IMM19_SHIFT
//   constant: 19-bit branch offset (B.cond/CBZ), bits [23:5].
#define D_JIT_ARM64_IMM19_SHIFT 5

// D_JIT_ARM64_SHIFT12_BIT
//   constant: the add/sub-immediate 'LSL #12' flag, bit 22.
#define D_JIT_ARM64_SHIFT12_BIT   0x00400000


// ===========================================================================
// E.   BASE OPCODES
// ===========================================================================
//   Each is a complete 32-bit instruction word with all operand fields zero;
// the emitters OR the register and immediate fields in. Verified encodings.

// D_JIT_ARM64_OP_NOP
//   constant: nop.
#define D_JIT_ARM64_OP_NOP      0xD503201F

// D_JIT_ARM64_OP_MOVZ
//   constant: movz Xd, #imm16, lsl #(hw*16)  -- zero other bits.
#define D_JIT_ARM64_OP_MOVZ     0xD2800000

// D_JIT_ARM64_OP_MOVK
//   constant: movk Xd, #imm16, lsl #(hw*16)  -- keep other bits.
#define D_JIT_ARM64_OP_MOVK     0xF2800000

// D_JIT_ARM64_OP_MOVN
//   constant: movn Xd, #imm16, lsl #(hw*16)  -- move NOT.
#define D_JIT_ARM64_OP_MOVN     0x92800000

// D_JIT_ARM64_OP_ADD_REG
//   constant: add  Xd, Xn, Xm.
#define D_JIT_ARM64_OP_ADD_REG  0x8B000000

// D_JIT_ARM64_OP_SUB_REG
//   constant: sub  Xd, Xn, Xm.
#define D_JIT_ARM64_OP_SUB_REG  0xCB000000

// D_JIT_ARM64_OP_SUBS_REG
//   constant: subs Xd, Xn, Xm (sets flags; CMP is Rd=XZR).
#define D_JIT_ARM64_OP_SUBS_REG 0xEB000000

// D_JIT_ARM64_OP_AND_REG
//   constant: and  Xd, Xn, Xm.
#define D_JIT_ARM64_OP_AND_REG  0x8A000000

// D_JIT_ARM64_OP_ORR_REG
//   constant: orr  Xd, Xn, Xm (MOV Xd,Xm is orr Xd,XZR,Xm).
#define D_JIT_ARM64_OP_ORR_REG  0xAA000000

// D_JIT_ARM64_OP_EOR_REG
//   constant: eor  Xd, Xn, Xm.
#define D_JIT_ARM64_OP_EOR_REG  0xCA000000

// D_JIT_ARM64_OP_MUL
//   constant: mul  Xd, Xn, Xm (madd with Ra = XZR).
#define D_JIT_ARM64_OP_MUL      0x9B007C00

// D_JIT_ARM64_OP_ADD_IMM
//   constant: add  Xd, Xn, #imm12 (optional LSL #12).
#define D_JIT_ARM64_OP_ADD_IMM  0x91000000

// D_JIT_ARM64_OP_SUB_IMM
//   constant: sub  Xd, Xn, #imm12 (optional LSL #12).
#define D_JIT_ARM64_OP_SUB_IMM  0xD1000000

// D_JIT_ARM64_OP_SUBS_IMM
//   constant: subs Xd, Xn, #imm12 (sets flags; CMP is Rd=XZR).
#define D_JIT_ARM64_OP_SUBS_IMM 0xF1000000

// D_JIT_ARM64_OP_LDR_IMM
//   constant: ldr  Xt, [Xn, #imm12*8] (unsigned scaled offset).
#define D_JIT_ARM64_OP_LDR_IMM  0xF9400000

// D_JIT_ARM64_OP_STR_IMM
//   constant: str  Xt, [Xn, #imm12*8] (unsigned scaled offset).
#define D_JIT_ARM64_OP_STR_IMM  0xF9000000

// D_JIT_ARM64_OP_B
//   constant: b    label (imm26, offset from this instruction /4).
#define D_JIT_ARM64_OP_B        0x14000000

// D_JIT_ARM64_OP_BL
//   constant: bl   label (imm26).
#define D_JIT_ARM64_OP_BL       0x94000000

// D_JIT_ARM64_OP_B_COND
//   constant: b.cond label (imm19 at [23:5], cond at [3:0]).
#define D_JIT_ARM64_OP_B_COND   0x54000000

// D_JIT_ARM64_OP_CBZ
//   constant: cbz  Xt, label (imm19 at [23:5], Rt at [4:0]).
#define D_JIT_ARM64_OP_CBZ      0xB4000000

// D_JIT_ARM64_OP_CBNZ
//   constant: cbnz Xt, label (imm19 at [23:5], Rt at [4:0]).
#define D_JIT_ARM64_OP_CBNZ     0xB5000000

// D_JIT_ARM64_OP_RET
//   constant: ret  Xn (Rn at [9:5]; RET alone is Xn = X30).
#define D_JIT_ARM64_OP_RET      0xD65F0000

// D_JIT_ARM64_OP_BR
//   constant: br   Xn (indirect branch).
#define D_JIT_ARM64_OP_BR       0xD61F0000

// D_JIT_ARM64_OP_BLR
//   constant: blr  Xn (indirect call).
#define D_JIT_ARM64_OP_BLR      0xD63F0000


// ===========================================================================
// F.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands into the base opcode and emits the 32-bit word.
// Register arguments are D_JIT_ARM64_REG_* numbers; branch targets are
// d_jit_label (see jit.h). Each returns 0 on success, -1 on an emit or
// range/alignment error.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// no-op and register move
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_nop
//   function: nop.
D_NODISCARD int d_jit_arm64_emit_nop(d_jit_buffer* _buf);

// d_jit_arm64_emit_mov_reg
//   function: mov Xd, Xm (orr Xd, XZR, Xm).
D_NODISCARD int d_jit_arm64_emit_mov_reg(d_jit_buffer* _buf, int _xd, int _xm);

// ---------------------------------------------------------------------------
// move-wide immediates
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_movz
//   function: movz Xd, #_imm16, lsl #_shift. _shift must be 0, 16, 32, or 48.
D_NODISCARD int d_jit_arm64_emit_movz(d_jit_buffer* _buf, int _xd,
                                      uint16_t _imm16, unsigned _shift);

// d_jit_arm64_emit_movk
//   function: movk Xd, #_imm16, lsl #_shift. _shift must be 0, 16, 32, or 48.
D_NODISCARD int d_jit_arm64_emit_movk(d_jit_buffer* _buf, int _xd,
                                      uint16_t _imm16, unsigned _shift);

// d_jit_arm64_emit_movn
//   function: movn Xd, #_imm16, lsl #_shift. _shift must be 0, 16, 32, or 48.
D_NODISCARD int d_jit_arm64_emit_movn(d_jit_buffer* _buf, int _xd,
                                      uint16_t _imm16, unsigned _shift);

// d_jit_arm64_emit_mov_imm
//   function: load an arbitrary 64-bit constant into Xd, as a movz followed by
// movk for each remaining non-zero 16-bit halfword (1-4 instructions).
D_NODISCARD int d_jit_arm64_emit_mov_imm(d_jit_buffer* _buf, int _xd,
                                         uint64_t _value);

// ---------------------------------------------------------------------------
// arithmetic and logic (register)
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_add_reg
//   function: add Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_add_reg(d_jit_buffer* _buf, int _xd, int _xn,
                                         int _xm);

// d_jit_arm64_emit_sub_reg
//   function: sub Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_sub_reg(d_jit_buffer* _buf, int _xd, int _xn,
                                         int _xm);

// d_jit_arm64_emit_and_reg
//   function: and Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_and_reg(d_jit_buffer* _buf, int _xd, int _xn,
                                         int _xm);

// d_jit_arm64_emit_orr_reg
//   function: orr Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_orr_reg(d_jit_buffer* _buf, int _xd, int _xn,
                                         int _xm);

// d_jit_arm64_emit_eor_reg
//   function: eor Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_eor_reg(d_jit_buffer* _buf, int _xd, int _xn,
                                         int _xm);

// d_jit_arm64_emit_mul
//   function: mul Xd, Xn, Xm.
D_NODISCARD int d_jit_arm64_emit_mul(d_jit_buffer* _buf, int _xd, int _xn,
                                     int _xm);

// d_jit_arm64_emit_cmp_reg
//   function: cmp Xn, Xm (subs XZR, Xn, Xm).
D_NODISCARD int d_jit_arm64_emit_cmp_reg(d_jit_buffer* _buf, int _xn, int _xm);

// ---------------------------------------------------------------------------
// arithmetic (immediate)
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_add_imm
//   function: add Xd, Xn, #_imm12. _imm12 is 0-4095; _lsl12 != 0 shifts it
// left by 12. Use REG_SP for Xd/Xn to address the stack pointer.
D_NODISCARD int d_jit_arm64_emit_add_imm(d_jit_buffer* _buf, int _xd, int _xn,
                                         uint32_t _imm12, int _lsl12);

// d_jit_arm64_emit_sub_imm
//   function: sub Xd, Xn, #_imm12. _imm12 is 0-4095; _lsl12 != 0 shifts it
// left by 12. Use REG_SP for Xd/Xn to address the stack pointer.
D_NODISCARD int d_jit_arm64_emit_sub_imm(d_jit_buffer* _buf, int _xd, int _xn,
                                         uint32_t _imm12, int _lsl12);

// d_jit_arm64_emit_cmp_imm
//   function: cmp Xn, #_imm12 (subs XZR, Xn, #_imm12).
D_NODISCARD int d_jit_arm64_emit_cmp_imm(d_jit_buffer* _buf, int _xn,
                                         uint32_t _imm12, int _lsl12);

// ---------------------------------------------------------------------------
// load / store (unsigned scaled offset)
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_ldr
//   function: ldr Xt, [Xn, #_offset]. _offset is a byte offset and must be a
// multiple of 8 in the range 0-32760.
D_NODISCARD int d_jit_arm64_emit_ldr(d_jit_buffer* _buf, int _xt, int _xn,
                                     uint32_t _offset);

// d_jit_arm64_emit_str
//   function: str Xt, [Xn, #_offset]. _offset is a byte offset and must be a
// multiple of 8 in the range 0-32760.
D_NODISCARD int d_jit_arm64_emit_str(d_jit_buffer* _buf, int _xt, int _xn,
                                     uint32_t _offset);

// ---------------------------------------------------------------------------
// branches (targets are d_jit_label; see jit.h)
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_b
//   function: b _target -- unconditional branch (+/-128 MB).
D_NODISCARD int d_jit_arm64_emit_b(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_arm64_emit_bl
//   function: bl _target -- branch with link (call).
D_NODISCARD int d_jit_arm64_emit_bl(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_arm64_emit_b_cond
//   function: b.cond _target (+/-1 MB). _cc is a D_JIT_ARM64_CC_* value.
D_NODISCARD int d_jit_arm64_emit_b_cond(d_jit_buffer* _buf, int _cc,
                                        d_jit_label* _target);

// d_jit_arm64_emit_cbz
//   function: cbz Xt, _target -- branch if Xt == 0 (+/-1 MB).
D_NODISCARD int d_jit_arm64_emit_cbz(d_jit_buffer* _buf, int _xt,
                                     d_jit_label* _target);

// d_jit_arm64_emit_cbnz
//   function: cbnz Xt, _target -- branch if Xt != 0 (+/-1 MB).
D_NODISCARD int d_jit_arm64_emit_cbnz(d_jit_buffer* _buf, int _xt,
                                      d_jit_label* _target);

// ---------------------------------------------------------------------------
// returns and indirect branches
// ---------------------------------------------------------------------------

// d_jit_arm64_emit_ret
//   function: ret (return to X30 / LR).
D_NODISCARD int d_jit_arm64_emit_ret(d_jit_buffer* _buf);

// d_jit_arm64_emit_ret_reg
//   function: ret Xn (return to an explicit register).
D_NODISCARD int d_jit_arm64_emit_ret_reg(d_jit_buffer* _buf, int _xn);

// d_jit_arm64_emit_br
//   function: br Xn -- indirect branch.
D_NODISCARD int d_jit_arm64_emit_br(d_jit_buffer* _buf, int _xn);

// d_jit_arm64_emit_blr
//   function: blr Xn -- indirect call.
D_NODISCARD int d_jit_arm64_emit_blr(d_jit_buffer* _buf, int _xn);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_arm64_reg_name
//   function: the name "x0".."x30", "sp", or "?" for a register number.
const char* d_jit_arm64_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_ARM64_
