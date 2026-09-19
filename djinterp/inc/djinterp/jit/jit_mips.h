/******************************************************************************
* djinterp [jit]                                                    jit_mips.h
*
* djinterp MIPS (MIPS32 / MIPS64) JIT encoder (fixed-width 32-bit words):
*   The MIPS half of the djinterp JIT. As with RISC-V, the MIPS32 and MIPS64
* base encodings are identical (register width is not encoded), so one module
* serves both; MIPS64 differs only by adding the doubleword (d*) instructions,
* which are isolated in section L and must not be emitted for a MIPS32 target.
*
*   IMPORTANT -- BRANCH DELAY SLOTS: every branch and jump on MIPS has a delay
* slot -- the single instruction after it always executes, before the transfer.
* These emitters emit only the branch word; the caller MUST follow every branch
* or jump with a delay-slot instruction (emit_nop for a bubble, or a useful one
* moved into it). Nothing is inserted automatically.
*
*   ENDIANNESS: words are emitted little-endian (the mipsel / mips64el
* convention of most embedded and Loongson targets, matching the buffer's
* emit primitives). A big-endian target needs byte-swapped emission.
*
*   Three formats: R (opcode 0 or SPECIAL2, a funct selects the operation), I
* (opcode, rs, rt, imm16), J (opcode, 26-bit index). Register-relative branches
* are PC-relative and take a d_jit_label (see jit.h); j/jal take an absolute
* target within the 256 MB region. All encodings verified via llvm-mc (mipsel
* and mips64el).
*
*   Registers are the 32 GPRs, as R0-R31 and by ABI name (zero, at, v0, a0, t0,
* s0, sp, ra, ...). R0 is always zero.
*
*   NAMING CONVENTION:
*     D_JIT_MIPS_REG_[NAME] - a register operand number (0-31)
*     D_JIT_MIPS_OP_[NAME]  - a 6-bit opcode field value
*     D_JIT_MIPS_FN_[NAME]  - a 6-bit funct field value (opcode 0 / SPECIAL2)
*     d_jit_mips_emit_*     - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_mips.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_MIPS_
#define DJINTERP_JIT_MIPS_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_MIPS_ENCODING
//   feature: 1, indicating the MIPS encoder is available.
#ifndef D_JIT_MIPS_ENCODING
    #define D_JIT_MIPS_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   The 32 general-purpose registers as R0-R31, plus the standard O32/N64 ABI
// aliases. R0 (zero) always reads as 0; writes to it are discarded. s8 is an
// alias of fp (R30).

#define D_JIT_MIPS_REG_R0      0
#define D_JIT_MIPS_REG_R1      1
#define D_JIT_MIPS_REG_R2      2
#define D_JIT_MIPS_REG_R3      3
#define D_JIT_MIPS_REG_R4      4
#define D_JIT_MIPS_REG_R5      5
#define D_JIT_MIPS_REG_R6      6
#define D_JIT_MIPS_REG_R7      7
#define D_JIT_MIPS_REG_R8      8
#define D_JIT_MIPS_REG_R9      9
#define D_JIT_MIPS_REG_R10     10
#define D_JIT_MIPS_REG_R11     11
#define D_JIT_MIPS_REG_R12     12
#define D_JIT_MIPS_REG_R13     13
#define D_JIT_MIPS_REG_R14     14
#define D_JIT_MIPS_REG_R15     15
#define D_JIT_MIPS_REG_R16     16
#define D_JIT_MIPS_REG_R17     17
#define D_JIT_MIPS_REG_R18     18
#define D_JIT_MIPS_REG_R19     19
#define D_JIT_MIPS_REG_R20     20
#define D_JIT_MIPS_REG_R21     21
#define D_JIT_MIPS_REG_R22     22
#define D_JIT_MIPS_REG_R23     23
#define D_JIT_MIPS_REG_R24     24
#define D_JIT_MIPS_REG_R25     25
#define D_JIT_MIPS_REG_R26     26
#define D_JIT_MIPS_REG_R27     27
#define D_JIT_MIPS_REG_R28     28
#define D_JIT_MIPS_REG_R29     29
#define D_JIT_MIPS_REG_R30     30
#define D_JIT_MIPS_REG_R31     31

// ABI aliases
#define D_JIT_MIPS_REG_ZERO   0
#define D_JIT_MIPS_REG_AT     1
#define D_JIT_MIPS_REG_V0     2
#define D_JIT_MIPS_REG_V1     3
#define D_JIT_MIPS_REG_A0     4
#define D_JIT_MIPS_REG_A1     5
#define D_JIT_MIPS_REG_A2     6
#define D_JIT_MIPS_REG_A3     7
#define D_JIT_MIPS_REG_T0     8
#define D_JIT_MIPS_REG_T1     9
#define D_JIT_MIPS_REG_T2     10
#define D_JIT_MIPS_REG_T3     11
#define D_JIT_MIPS_REG_T4     12
#define D_JIT_MIPS_REG_T5     13
#define D_JIT_MIPS_REG_T6     14
#define D_JIT_MIPS_REG_T7     15
#define D_JIT_MIPS_REG_S0     16
#define D_JIT_MIPS_REG_S1     17
#define D_JIT_MIPS_REG_S2     18
#define D_JIT_MIPS_REG_S3     19
#define D_JIT_MIPS_REG_S4     20
#define D_JIT_MIPS_REG_S5     21
#define D_JIT_MIPS_REG_S6     22
#define D_JIT_MIPS_REG_S7     23
#define D_JIT_MIPS_REG_T8     24
#define D_JIT_MIPS_REG_T9     25
#define D_JIT_MIPS_REG_K0     26
#define D_JIT_MIPS_REG_K1     27
#define D_JIT_MIPS_REG_GP     28
#define D_JIT_MIPS_REG_SP     29
#define D_JIT_MIPS_REG_FP     30
#define D_JIT_MIPS_REG_S8     30
#define D_JIT_MIPS_REG_RA     31


// ===========================================================================
// C.   OPCODE AND FUNCT FIELD VALUES
// ===========================================================================
//   The primary opcode (bits 31-26) and, for opcode 0 (SPECIAL) and SPECIAL2,
// the funct (bits 5-0). The emitters combine these internally; verified.

// -- primary opcodes (I-/J-type; opcode 0 is SPECIAL, 0x1C is SPECIAL2) --

#define D_JIT_MIPS_OP_SPECIAL   0x00
#define D_JIT_MIPS_OP_REGIMM    0x01
#define D_JIT_MIPS_OP_J         0x02
#define D_JIT_MIPS_OP_JAL       0x03
#define D_JIT_MIPS_OP_BEQ       0x04
#define D_JIT_MIPS_OP_BNE       0x05
#define D_JIT_MIPS_OP_BLEZ      0x06
#define D_JIT_MIPS_OP_BGTZ      0x07
#define D_JIT_MIPS_OP_ADDI      0x08
#define D_JIT_MIPS_OP_ADDIU     0x09
#define D_JIT_MIPS_OP_SLTI      0x0A
#define D_JIT_MIPS_OP_SLTIU     0x0B
#define D_JIT_MIPS_OP_ANDI      0x0C
#define D_JIT_MIPS_OP_ORI       0x0D
#define D_JIT_MIPS_OP_XORI      0x0E
#define D_JIT_MIPS_OP_LUI       0x0F
#define D_JIT_MIPS_OP_SPECIAL2  0x1C
#define D_JIT_MIPS_OP_DADDI     0x18
#define D_JIT_MIPS_OP_DADDIU    0x19
#define D_JIT_MIPS_OP_LB        0x20
#define D_JIT_MIPS_OP_LH        0x21
#define D_JIT_MIPS_OP_LW        0x23
#define D_JIT_MIPS_OP_LBU       0x24
#define D_JIT_MIPS_OP_LHU       0x25
#define D_JIT_MIPS_OP_LWU       0x27
#define D_JIT_MIPS_OP_SB        0x28
#define D_JIT_MIPS_OP_SH        0x29
#define D_JIT_MIPS_OP_SW        0x2B
#define D_JIT_MIPS_OP_LD        0x37
#define D_JIT_MIPS_OP_SD        0x3F

// -- SPECIAL (opcode 0) funct values --

#define D_JIT_MIPS_FN_SLL       0x00
#define D_JIT_MIPS_FN_SRL       0x02
#define D_JIT_MIPS_FN_SRA       0x03
#define D_JIT_MIPS_FN_SLLV      0x04
#define D_JIT_MIPS_FN_SRLV      0x06
#define D_JIT_MIPS_FN_SRAV      0x07
#define D_JIT_MIPS_FN_JR        0x08
#define D_JIT_MIPS_FN_JALR      0x09
#define D_JIT_MIPS_FN_MFHI      0x10
#define D_JIT_MIPS_FN_MFLO      0x12
#define D_JIT_MIPS_FN_MULT      0x18
#define D_JIT_MIPS_FN_MULTU     0x19
#define D_JIT_MIPS_FN_DIV       0x1A
#define D_JIT_MIPS_FN_DIVU      0x1B
#define D_JIT_MIPS_FN_ADD       0x20
#define D_JIT_MIPS_FN_ADDU      0x21
#define D_JIT_MIPS_FN_SUB       0x22
#define D_JIT_MIPS_FN_SUBU      0x23
#define D_JIT_MIPS_FN_AND       0x24
#define D_JIT_MIPS_FN_OR        0x25
#define D_JIT_MIPS_FN_XOR       0x26
#define D_JIT_MIPS_FN_NOR       0x27
#define D_JIT_MIPS_FN_SLT       0x2A
#define D_JIT_MIPS_FN_SLTU      0x2B
#define D_JIT_MIPS_FN_DADD      0x2C
#define D_JIT_MIPS_FN_DADDU     0x2D
#define D_JIT_MIPS_FN_DSUB      0x2E
#define D_JIT_MIPS_FN_DSUBU     0x2F
#define D_JIT_MIPS_FN_DSLL      0x38
#define D_JIT_MIPS_FN_DSRL      0x3A
#define D_JIT_MIPS_FN_DSRA      0x3B
#define D_JIT_MIPS_FN_DSLL32    0x3C
#define D_JIT_MIPS_FN_DSRL32    0x3E
#define D_JIT_MIPS_FN_DSRA32    0x3F
#define D_JIT_MIPS_FN_DSLLV     0x14
#define D_JIT_MIPS_FN_DSRLV     0x16
#define D_JIT_MIPS_FN_DSRAV     0x17
#define D_JIT_MIPS_FN_DMULT     0x1C
#define D_JIT_MIPS_FN_DMULTU    0x1D

// -- SPECIAL2 (opcode 0x1C) funct, and REGIMM (opcode 0x01) rt selectors --

#define D_JIT_MIPS_FN_MUL     0x02  // SPECIAL2: mul rd, rs, rt
#define D_JIT_MIPS_RT_BLTZ    0x00  // REGIMM rt: bltz
#define D_JIT_MIPS_RT_BGEZ    0x01  // REGIMM rt: bgez
#define D_JIT_MIPS_RT_BGEZAL  0x11  // REGIMM rt: bgezal (bal)


// ===========================================================================
// D.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands into the correct format and emits the 32-bit word.
// Register arguments are D_JIT_MIPS_REG_* numbers; PC-relative branch targets
// are d_jit_label. Returns 0 on success, -1 on an emit, immediate-range, or
// branch-range error. I-type immediates are 16-bit; shift amounts are 0-31.
// Remember the delay slot after every branch and jump.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// register ALU (R-type)
// ---------------------------------------------------------------------------

// d_jit_mips_emit_add
//   function: add rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_add(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_addu
//   function: addu rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_addu(d_jit_buffer* _buf, int _rd, int _rs,
                                     int _rt);

// d_jit_mips_emit_sub
//   function: sub rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_sub(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_subu
//   function: subu rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_subu(d_jit_buffer* _buf, int _rd, int _rs,
                                     int _rt);

// d_jit_mips_emit_and
//   function: and rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_and(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_or
//   function: or rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_or(d_jit_buffer* _buf, int _rd, int _rs,
                                   int _rt);

// d_jit_mips_emit_xor
//   function: xor rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_xor(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_nor
//   function: nor rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_nor(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_slt
//   function: slt rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_slt(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// d_jit_mips_emit_sltu
//   function: sltu rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_sltu(d_jit_buffer* _buf, int _rd, int _rs,
                                     int _rt);

// d_jit_mips_emit_mul
//   function: mul rd, rs, rt (3-operand multiply, low word to rd).
D_NODISCARD int d_jit_mips_emit_mul(d_jit_buffer* _buf, int _rd, int _rs,
                                    int _rt);

// ---------------------------------------------------------------------------
// shifts
// ---------------------------------------------------------------------------

// d_jit_mips_emit_sll
//   function: sll rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_sll(d_jit_buffer* _buf, int _rd, int _rt,
                                    unsigned _sa);

// d_jit_mips_emit_srl
//   function: srl rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_srl(d_jit_buffer* _buf, int _rd, int _rt,
                                    unsigned _sa);

// d_jit_mips_emit_sra
//   function: sra rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_sra(d_jit_buffer* _buf, int _rd, int _rt,
                                    unsigned _sa);

// d_jit_mips_emit_sllv
//   function: sllv rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_sllv(d_jit_buffer* _buf, int _rd, int _rt,
                                     int _rs);

// d_jit_mips_emit_srlv
//   function: srlv rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_srlv(d_jit_buffer* _buf, int _rd, int _rt,
                                     int _rs);

// d_jit_mips_emit_srav
//   function: srav rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_srav(d_jit_buffer* _buf, int _rd, int _rt,
                                     int _rs);

// ---------------------------------------------------------------------------
// multiply / divide (HI/LO)
// ---------------------------------------------------------------------------

// d_jit_mips_emit_mult
//   function: mult rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_mult(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_multu
//   function: multu rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_multu(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_div
//   function: div rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_div(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_divu
//   function: divu rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_divu(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_mfhi
//   function: mfhi rd (rd = HI).
D_NODISCARD int d_jit_mips_emit_mfhi(d_jit_buffer* _buf, int _rd);

// d_jit_mips_emit_mflo
//   function: mflo rd (rd = LO).
D_NODISCARD int d_jit_mips_emit_mflo(d_jit_buffer* _buf, int _rd);

// ---------------------------------------------------------------------------
// register-immediate (I-type)
// ---------------------------------------------------------------------------

// d_jit_mips_emit_addi
//   function: addi rt, rs, #_imm (signed 16-bit).
D_NODISCARD int d_jit_mips_emit_addi(d_jit_buffer* _buf, int _rt, int _rs,
                                     int32_t _imm);

// d_jit_mips_emit_addiu
//   function: addiu rt, rs, #_imm (signed 16-bit).
D_NODISCARD int d_jit_mips_emit_addiu(d_jit_buffer* _buf, int _rt, int _rs,
                                      int32_t _imm);

// d_jit_mips_emit_slti
//   function: slti rt, rs, #_imm (signed 16-bit).
D_NODISCARD int d_jit_mips_emit_slti(d_jit_buffer* _buf, int _rt, int _rs,
                                     int32_t _imm);

// d_jit_mips_emit_sltiu
//   function: sltiu rt, rs, #_imm (signed 16-bit).
D_NODISCARD int d_jit_mips_emit_sltiu(d_jit_buffer* _buf, int _rt, int _rs,
                                      int32_t _imm);

// d_jit_mips_emit_andi
//   function: andi rt, rs, #_imm (16-bit).
D_NODISCARD int d_jit_mips_emit_andi(d_jit_buffer* _buf, int _rt, int _rs,
                                     uint32_t _imm);

// d_jit_mips_emit_ori
//   function: ori rt, rs, #_imm (16-bit).
D_NODISCARD int d_jit_mips_emit_ori(d_jit_buffer* _buf, int _rt, int _rs,
                                    uint32_t _imm);

// d_jit_mips_emit_xori
//   function: xori rt, rs, #_imm (16-bit).
D_NODISCARD int d_jit_mips_emit_xori(d_jit_buffer* _buf, int _rt, int _rs,
                                     uint32_t _imm);

// d_jit_mips_emit_lui
//   function: lui rt, #_imm16 (rt = _imm16 << 16).
D_NODISCARD int d_jit_mips_emit_lui(d_jit_buffer* _buf, int _rt,
                                    uint32_t _imm16);

// ---------------------------------------------------------------------------
// loads / stores
// ---------------------------------------------------------------------------

// d_jit_mips_emit_lb
//   function: lb rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lb(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_lh
//   function: lh rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lh(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_lw
//   function: lw rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lw(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_lbu
//   function: lbu rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lbu(d_jit_buffer* _buf, int _rt, int _base,
                                    int32_t _off);

// d_jit_mips_emit_lhu
//   function: lhu rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lhu(d_jit_buffer* _buf, int _rt, int _base,
                                    int32_t _off);

// d_jit_mips_emit_sb
//   function: sb rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_sb(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_sh
//   function: sh rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_sh(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_sw
//   function: sw rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_sw(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// ---------------------------------------------------------------------------
// branches (PC-relative, +/-128 KB; targets are d_jit_label)
// NOTE: each must be followed by a delay-slot instruction.
// ---------------------------------------------------------------------------

// d_jit_mips_emit_beq
//   function: beq rs, rt, _target (branch if rs == rt).
D_NODISCARD int d_jit_mips_emit_beq(d_jit_buffer* _buf, int _rs, int _rt,
                                    d_jit_label* _target);

// d_jit_mips_emit_bne
//   function: bne rs, rt, _target (branch if rs != rt).
D_NODISCARD int d_jit_mips_emit_bne(d_jit_buffer* _buf, int _rs, int _rt,
                                    d_jit_label* _target);

// d_jit_mips_emit_blez
//   function: blez rs, _target (branch if rs <= 0, signed).
D_NODISCARD int d_jit_mips_emit_blez(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_bgtz
//   function: bgtz rs, _target (branch if rs > 0, signed).
D_NODISCARD int d_jit_mips_emit_bgtz(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_bltz
//   function: bltz rs, _target (branch if rs < 0, signed).
D_NODISCARD int d_jit_mips_emit_bltz(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_bgez
//   function: bgez rs, _target (branch if rs >= 0, signed).
D_NODISCARD int d_jit_mips_emit_bgez(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_beqz
//   function: beqz rs, _target (beq rs, zero).
D_NODISCARD int d_jit_mips_emit_beqz(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_bnez
//   function: bnez rs, _target (bne rs, zero).
D_NODISCARD int d_jit_mips_emit_bnez(d_jit_buffer* _buf, int _rs,
                                     d_jit_label* _target);

// d_jit_mips_emit_b
//   function: b _target -- unconditional branch (beq zero, zero).
D_NODISCARD int d_jit_mips_emit_b(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_mips_emit_bal
//   function: bal _target -- PC-relative call (bgezal zero); ra = return.
D_NODISCARD int d_jit_mips_emit_bal(d_jit_buffer* _buf, d_jit_label* _target);

// ---------------------------------------------------------------------------
// jumps (delay slot applies)
// ---------------------------------------------------------------------------

// d_jit_mips_emit_jr
//   function: jr rs -- jump to the address in rs (use REG_RA to return).
D_NODISCARD int d_jit_mips_emit_jr(d_jit_buffer* _buf, int _rs);

// d_jit_mips_emit_jalr
//   function: jalr rd, rs -- call via rs; rd gets the return address.
D_NODISCARD int d_jit_mips_emit_jalr(d_jit_buffer* _buf, int _rd, int _rs);

// d_jit_mips_emit_j
//   function: j _target_addr -- absolute jump; the target must share the
// current 256 MB region (index = (_target_addr >> 2) & 0x3FFFFFF).
D_NODISCARD int d_jit_mips_emit_j(d_jit_buffer* _buf, uint32_t _target_addr);

// d_jit_mips_emit_jal
//   function: jal _target_addr -- absolute call (same region constraint).
D_NODISCARD int d_jit_mips_emit_jal(d_jit_buffer* _buf, uint32_t _target_addr);

// ---------------------------------------------------------------------------
// pseudo-instructions
// ---------------------------------------------------------------------------

// d_jit_mips_emit_nop
//   function: nop (sll zero, zero, 0 = 0x00000000).
D_NODISCARD int d_jit_mips_emit_nop(d_jit_buffer* _buf);

// d_jit_mips_emit_move
//   function: move rd, rs (addu rd, rs, zero).
D_NODISCARD int d_jit_mips_emit_move(d_jit_buffer* _buf, int _rd, int _rs);

// d_jit_mips_emit_li
//   function: load a 32-bit immediate into rt: one ori/addiu when it fits in
// 16 bits, otherwise lui+ori (sign-extended into a 64-bit reg on MIPS64).
D_NODISCARD int d_jit_mips_emit_li(d_jit_buffer* _buf, int _rt, int32_t _imm);

// ---------------------------------------------------------------------------
// L.  MIPS64-ONLY (do not emit for a MIPS32 target)
// ---------------------------------------------------------------------------

// d_jit_mips_emit_dadd
//   function: dadd rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_dadd(d_jit_buffer* _buf, int _rd, int _rs,
                                     int _rt);

// d_jit_mips_emit_daddu
//   function: daddu rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_daddu(d_jit_buffer* _buf, int _rd, int _rs,
                                      int _rt);

// d_jit_mips_emit_dsub
//   function: dsub rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_dsub(d_jit_buffer* _buf, int _rd, int _rs,
                                     int _rt);

// d_jit_mips_emit_dsubu
//   function: dsubu rd, rs, rt.
D_NODISCARD int d_jit_mips_emit_dsubu(d_jit_buffer* _buf, int _rd, int _rs,
                                      int _rt);

// d_jit_mips_emit_daddiu
//   function: daddiu rt, rs, #_imm (signed 16-bit).
D_NODISCARD int d_jit_mips_emit_daddiu(d_jit_buffer* _buf, int _rt, int _rs,
                                       int32_t _imm);

// d_jit_mips_emit_dsll
//   function: dsll rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsll(d_jit_buffer* _buf, int _rd, int _rt,
                                     unsigned _sa);

// d_jit_mips_emit_dsrl
//   function: dsrl rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsrl(d_jit_buffer* _buf, int _rd, int _rt,
                                     unsigned _sa);

// d_jit_mips_emit_dsra
//   function: dsra rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsra(d_jit_buffer* _buf, int _rd, int _rt,
                                     unsigned _sa);

// d_jit_mips_emit_dsll32
//   function: dsll32 rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsll32(d_jit_buffer* _buf, int _rd, int _rt,
                                       unsigned _sa);

// d_jit_mips_emit_dsrl32
//   function: dsrl32 rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsrl32(d_jit_buffer* _buf, int _rd, int _rt,
                                       unsigned _sa);

// d_jit_mips_emit_dsra32
//   function: dsra32 rd, rt, #_sa (rd = rt shifted by _sa).
D_NODISCARD int d_jit_mips_emit_dsra32(d_jit_buffer* _buf, int _rd, int _rt,
                                       unsigned _sa);

// d_jit_mips_emit_dsllv
//   function: dsllv rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_dsllv(d_jit_buffer* _buf, int _rd, int _rt,
                                      int _rs);

// d_jit_mips_emit_dsrlv
//   function: dsrlv rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_dsrlv(d_jit_buffer* _buf, int _rd, int _rt,
                                      int _rs);

// d_jit_mips_emit_dsrav
//   function: dsrav rd, rt, rs (rd = rt shifted by rs).
D_NODISCARD int d_jit_mips_emit_dsrav(d_jit_buffer* _buf, int _rd, int _rt,
                                      int _rs);

// d_jit_mips_emit_dmult
//   function: dmult rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_dmult(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_dmultu
//   function: dmultu rs, rt (result in HI/LO).
D_NODISCARD int d_jit_mips_emit_dmultu(d_jit_buffer* _buf, int _rs, int _rt);

// d_jit_mips_emit_ld
//   function: ld rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_ld(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_sd
//   function: sd rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_sd(d_jit_buffer* _buf, int _rt, int _base,
                                   int32_t _off);

// d_jit_mips_emit_lwu
//   function: lwu rt, _off(base) (signed 16-bit offset).
D_NODISCARD int d_jit_mips_emit_lwu(d_jit_buffer* _buf, int _rt, int _base,
                                    int32_t _off);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_mips_reg_name
//   function: the ABI name ("zero","ra","sp","a0",...) or "?".
const char* d_jit_mips_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_MIPS_
