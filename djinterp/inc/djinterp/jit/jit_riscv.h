/******************************************************************************
* djinterp [jit]                                                   jit_riscv.h
*
* djinterp RISC-V (RV32I / RV64I) JIT encoder (fixed-width 32-bit words):
*   The RISC-V half of the djinterp JIT. Unlike x86/x86-64 or ARM A32/A64, the
* RV32I and RV64I base integer encodings are identical -- the register width
* (XLEN) is not encoded in an instruction -- so one module serves both, and
* RV64 differs only by adding instructions. The shared base ISA (plus the M-
* extension MUL) is in sections D-K; the RV64I-only instructions (word ops and
* the wider loads/stores) are isolated in section L and must not be emitted for
* an RV32 target. All base encodings were verified on both riscv32 and riscv64
* via llvm-mc; RV64-only ones on riscv64.
*
*   Six formats -- R (reg,reg,reg), I (reg,reg,imm), S (store), B (branch), U
* (upper-immediate), J (jump) -- are assembled internally from the opcode and
* funct fields. Immediates are sign-checked where the format bounds them.
* Branch and jump targets are d_jit_label (see jit.h).
*
*   Registers are x0-x31; x0 is always zero. Standard ABI aliases (ra, sp, a0,
* t0, s0/fp, ...) are provided for readable call sites.
*
*   NAMING CONVENTION:
*     D_JIT_RISCV_REG_[NAME] - a register operand number (0-31)
*     D_JIT_RISCV_OP_[NAME]  - a 7-bit base opcode (the opcode field)
*     d_jit_riscv_emit_*     - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_riscv.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_RISCV_
#define DJINTERP_JIT_RISCV_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_RISCV_ENCODING
//   feature: 1, indicating the RISC-V encoder is available.
#ifndef D_JIT_RISCV_ENCODING
    #define D_JIT_RISCV_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   x0-x31 as numbers 0-31 (x0 reads as zero), with the standard integer ABI
// aliases. fp is an alias of s0 (x8).

#define D_JIT_RISCV_REG_X0      0
#define D_JIT_RISCV_REG_X1      1
#define D_JIT_RISCV_REG_X2      2
#define D_JIT_RISCV_REG_X3      3
#define D_JIT_RISCV_REG_X4      4
#define D_JIT_RISCV_REG_X5      5
#define D_JIT_RISCV_REG_X6      6
#define D_JIT_RISCV_REG_X7      7
#define D_JIT_RISCV_REG_X8      8
#define D_JIT_RISCV_REG_X9      9
#define D_JIT_RISCV_REG_X10     10
#define D_JIT_RISCV_REG_X11     11
#define D_JIT_RISCV_REG_X12     12
#define D_JIT_RISCV_REG_X13     13
#define D_JIT_RISCV_REG_X14     14
#define D_JIT_RISCV_REG_X15     15
#define D_JIT_RISCV_REG_X16     16
#define D_JIT_RISCV_REG_X17     17
#define D_JIT_RISCV_REG_X18     18
#define D_JIT_RISCV_REG_X19     19
#define D_JIT_RISCV_REG_X20     20
#define D_JIT_RISCV_REG_X21     21
#define D_JIT_RISCV_REG_X22     22
#define D_JIT_RISCV_REG_X23     23
#define D_JIT_RISCV_REG_X24     24
#define D_JIT_RISCV_REG_X25     25
#define D_JIT_RISCV_REG_X26     26
#define D_JIT_RISCV_REG_X27     27
#define D_JIT_RISCV_REG_X28     28
#define D_JIT_RISCV_REG_X29     29
#define D_JIT_RISCV_REG_X30     30
#define D_JIT_RISCV_REG_X31     31

// ABI aliases
#define D_JIT_RISCV_REG_ZERO   0
#define D_JIT_RISCV_REG_RA     1
#define D_JIT_RISCV_REG_SP     2
#define D_JIT_RISCV_REG_GP     3
#define D_JIT_RISCV_REG_TP     4
#define D_JIT_RISCV_REG_T0     5
#define D_JIT_RISCV_REG_T1     6
#define D_JIT_RISCV_REG_T2     7
#define D_JIT_RISCV_REG_S0     8
#define D_JIT_RISCV_REG_FP     8
#define D_JIT_RISCV_REG_S1     9
#define D_JIT_RISCV_REG_A0     10
#define D_JIT_RISCV_REG_A1     11
#define D_JIT_RISCV_REG_A2     12
#define D_JIT_RISCV_REG_A3     13
#define D_JIT_RISCV_REG_A4     14
#define D_JIT_RISCV_REG_A5     15
#define D_JIT_RISCV_REG_A6     16
#define D_JIT_RISCV_REG_A7     17
#define D_JIT_RISCV_REG_S2     18
#define D_JIT_RISCV_REG_S3     19
#define D_JIT_RISCV_REG_S4     20
#define D_JIT_RISCV_REG_S5     21
#define D_JIT_RISCV_REG_S6     22
#define D_JIT_RISCV_REG_S7     23
#define D_JIT_RISCV_REG_S8     24
#define D_JIT_RISCV_REG_S9     25
#define D_JIT_RISCV_REG_S10    26
#define D_JIT_RISCV_REG_S11    27
#define D_JIT_RISCV_REG_T3     28
#define D_JIT_RISCV_REG_T4     29
#define D_JIT_RISCV_REG_T5     30
#define D_JIT_RISCV_REG_T6     31


// ===========================================================================
// C.   BASE OPCODES
// ===========================================================================
//   The 7-bit opcode field of each instruction group; the emitters combine
// these with the funct3/funct7 selectors internally. Verified encodings.

// D_JIT_RISCV_OP_OP
//   constant: register-register ALU (add/sub/sll/.../and, mul).
#define D_JIT_RISCV_OP_OP         0x33

// D_JIT_RISCV_OP_OP_IMM
//   constant: register-immediate ALU (addi/andi/slli/...).
#define D_JIT_RISCV_OP_OP_IMM     0x13

// D_JIT_RISCV_OP_LOAD
//   constant: loads (lb/lh/lw/lbu/lhu, and RV64 lwu/ld).
#define D_JIT_RISCV_OP_LOAD       0x03

// D_JIT_RISCV_OP_STORE
//   constant: stores (sb/sh/sw, and RV64 sd).
#define D_JIT_RISCV_OP_STORE      0x23

// D_JIT_RISCV_OP_BRANCH
//   constant: conditional branches (beq/bne/blt/bge/bltu/bgeu).
#define D_JIT_RISCV_OP_BRANCH     0x63

// D_JIT_RISCV_OP_JAL
//   constant: jump and link (J-type).
#define D_JIT_RISCV_OP_JAL        0x6F

// D_JIT_RISCV_OP_JALR
//   constant: jump and link register (I-type).
#define D_JIT_RISCV_OP_JALR       0x67

// D_JIT_RISCV_OP_LUI
//   constant: load upper immediate (U-type).
#define D_JIT_RISCV_OP_LUI        0x37

// D_JIT_RISCV_OP_AUIPC
//   constant: add upper immediate to pc (U-type).
#define D_JIT_RISCV_OP_AUIPC      0x17

// D_JIT_RISCV_OP_OP_32
//   constant: RV64 word register-register ALU (addw/subw/...).
#define D_JIT_RISCV_OP_OP_32      0x3B

// D_JIT_RISCV_OP_OP_IMM_32
//   constant: RV64 word register-immediate ALU (addiw/slliw/...).
#define D_JIT_RISCV_OP_OP_IMM_32  0x1B


// ===========================================================================
// D.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands into the correct format and emits the 32-bit word.
// Register arguments are D_JIT_RISCV_REG_* numbers; branch/jump targets are
// d_jit_label. Each returns 0 on success, -1 on an emit, immediate-range, or
// alignment error. Immediates for the I/S formats are signed 12-bit
// (-2048..2047); shift amounts are 0-31 (0-63 is RV64-only).

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// register-register ALU (R-type)
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_add
//   function: add rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_add(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_sub
//   function: sub rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sub(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_sll
//   function: sll rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sll(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_slt
//   function: slt rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_slt(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_sltu
//   function: sltu rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sltu(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_riscv_emit_xor
//   function: xor rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_xor(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_srl
//   function: srl rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_srl(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_sra
//   function: sra rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sra(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_or
//   function: or rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_or(d_jit_buffer* _buf, int _rd, int _rs1,
                                    int _rs2);

// d_jit_riscv_emit_and
//   function: and rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_and(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_riscv_emit_mul
//   function: mul rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_mul(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// ---------------------------------------------------------------------------
// register-immediate ALU (I-type)
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_addi
//   function: addi rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_addi(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_riscv_emit_slti
//   function: slti rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_slti(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_riscv_emit_sltiu
//   function: sltiu rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_sltiu(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int32_t _imm);

// d_jit_riscv_emit_xori
//   function: xori rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_xori(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_riscv_emit_ori
//   function: ori rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_ori(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_riscv_emit_andi
//   function: andi rd, rs1, #_imm (signed 12-bit).
D_NODISCARD int d_jit_riscv_emit_andi(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// ---------------------------------------------------------------------------
// shift-immediate
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_slli
//   function: slli rd, rs1, #_shamt.
D_NODISCARD int d_jit_riscv_emit_slli(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shamt);

// d_jit_riscv_emit_srli
//   function: srli rd, rs1, #_shamt.
D_NODISCARD int d_jit_riscv_emit_srli(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shamt);

// d_jit_riscv_emit_srai
//   function: srai rd, rs1, #_shamt.
D_NODISCARD int d_jit_riscv_emit_srai(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shamt);

// ---------------------------------------------------------------------------
// loads
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_lb
//   function: lb rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lb(d_jit_buffer* _buf, int _rd, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_lh
//   function: lh rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lh(d_jit_buffer* _buf, int _rd, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_lw
//   function: lw rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lw(d_jit_buffer* _buf, int _rd, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_lbu
//   function: lbu rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lbu(d_jit_buffer* _buf, int _rd, int _base,
                                     int32_t _off);

// d_jit_riscv_emit_lhu
//   function: lhu rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lhu(d_jit_buffer* _buf, int _rd, int _base,
                                     int32_t _off);

// ---------------------------------------------------------------------------
// stores
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_sb
//   function: sb src, _off(base) -- store (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_sb(d_jit_buffer* _buf, int _src, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_sh
//   function: sh src, _off(base) -- store (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_sh(d_jit_buffer* _buf, int _src, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_sw
//   function: sw src, _off(base) -- store (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_sw(d_jit_buffer* _buf, int _src, int _base,
                                    int32_t _off);

// ---------------------------------------------------------------------------
// upper immediates (U-type)
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_lui
//   function: lui rd, #_imm20 -- rd = _imm20 << 12 (low 20 bits used).
D_NODISCARD int d_jit_riscv_emit_lui(d_jit_buffer* _buf, int _rd,
                                     uint32_t _imm20);

// d_jit_riscv_emit_auipc
//   function: auipc rd, #_imm20 -- rd = pc + (_imm20 << 12).
D_NODISCARD int d_jit_riscv_emit_auipc(d_jit_buffer* _buf, int _rd,
                                       uint32_t _imm20);

// ---------------------------------------------------------------------------
// branches (+/-4 KB; targets are d_jit_label, see jit.h)
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_beq
//   function: beq rs1, rs2, _target -- branch if rs1 == rs2.
D_NODISCARD int d_jit_riscv_emit_beq(d_jit_buffer* _buf, int _rs1, int _rs2,
                                     d_jit_label* _target);

// d_jit_riscv_emit_bne
//   function: bne rs1, rs2, _target -- branch if rs1 != rs2.
D_NODISCARD int d_jit_riscv_emit_bne(d_jit_buffer* _buf, int _rs1, int _rs2,
                                     d_jit_label* _target);

// d_jit_riscv_emit_blt
//   function: blt rs1, rs2, _target -- branch if rs1 < signed rs2.
D_NODISCARD int d_jit_riscv_emit_blt(d_jit_buffer* _buf, int _rs1, int _rs2,
                                     d_jit_label* _target);

// d_jit_riscv_emit_bge
//   function: bge rs1, rs2, _target -- branch if rs1 >= signed rs2.
D_NODISCARD int d_jit_riscv_emit_bge(d_jit_buffer* _buf, int _rs1, int _rs2,
                                     d_jit_label* _target);

// d_jit_riscv_emit_bltu
//   function: bltu rs1, rs2, _target -- branch if rs1 < unsigned rs2.
D_NODISCARD int d_jit_riscv_emit_bltu(d_jit_buffer* _buf, int _rs1, int _rs2,
                                      d_jit_label* _target);

// d_jit_riscv_emit_bgeu
//   function: bgeu rs1, rs2, _target -- branch if rs1 >= unsigned rs2.
D_NODISCARD int d_jit_riscv_emit_bgeu(d_jit_buffer* _buf, int _rs1, int _rs2,
                                      d_jit_label* _target);

// ---------------------------------------------------------------------------
// jumps
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_jal
//   function: jal rd, _target -- jump and link; rd gets the return address
// (+/-1 MB). Use REG_RA for a call, REG_ZERO for a plain jump.
D_NODISCARD int d_jit_riscv_emit_jal(d_jit_buffer* _buf, int _rd,
                                     d_jit_label* _target);

// d_jit_riscv_emit_jalr
//   function: jalr rd, _off(rs1) -- indirect jump to (rs1 + _off) & ~1.
D_NODISCARD int d_jit_riscv_emit_jalr(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _off);

// d_jit_riscv_emit_j
//   function: j _target -- unconditional jump (jal with rd = zero).
D_NODISCARD int d_jit_riscv_emit_j(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_riscv_emit_call
//   function: jal ra, _target -- direct call within +/-1 MB.
D_NODISCARD int d_jit_riscv_emit_call(d_jit_buffer* _buf,
                                      d_jit_label* _target);

// d_jit_riscv_emit_ret
//   function: ret (jalr zero, 0(ra)).
D_NODISCARD int d_jit_riscv_emit_ret(d_jit_buffer* _buf);

// d_jit_riscv_emit_jr
//   function: jr rs1 (jalr zero, 0(rs1)) -- indirect jump.
D_NODISCARD int d_jit_riscv_emit_jr(d_jit_buffer* _buf, int _rs1);

// d_jit_riscv_emit_beqz
//   function: beqz rs1, _target (beq rs1, zero).
D_NODISCARD int d_jit_riscv_emit_beqz(d_jit_buffer* _buf, int _rs1,
                                      d_jit_label* _target);

// d_jit_riscv_emit_bnez
//   function: bnez rs1, _target (bne rs1, zero).
D_NODISCARD int d_jit_riscv_emit_bnez(d_jit_buffer* _buf, int _rs1,
                                      d_jit_label* _target);

// ---------------------------------------------------------------------------
// pseudo-instructions
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_nop
//   function: nop (addi zero, zero, 0).
D_NODISCARD int d_jit_riscv_emit_nop(d_jit_buffer* _buf);

// d_jit_riscv_emit_mv
//   function: mv rd, rs (addi rd, rs, 0).
D_NODISCARD int d_jit_riscv_emit_mv(d_jit_buffer* _buf, int _rd, int _rs);

// d_jit_riscv_emit_li
//   function: load a signed 32-bit immediate into rd: one addi when it fits in
// 12 bits, otherwise lui followed by addi (valid on RV32 and RV64).
D_NODISCARD int d_jit_riscv_emit_li(d_jit_buffer* _buf, int _rd, int32_t _imm);

// ---------------------------------------------------------------------------
// L.  RV64I-ONLY (do not emit for an RV32 target)
// ---------------------------------------------------------------------------

// d_jit_riscv_emit_lwu
//   function: lwu rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_lwu(d_jit_buffer* _buf, int _rd, int _base,
                                     int32_t _off);

// d_jit_riscv_emit_ld
//   function: ld rd, _off(base) -- load (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_ld(d_jit_buffer* _buf, int _rd, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_sd
//   function: sd src, _off(base) -- store (signed 12-bit offset).
D_NODISCARD int d_jit_riscv_emit_sd(d_jit_buffer* _buf, int _src, int _base,
                                    int32_t _off);

// d_jit_riscv_emit_addiw
//   function: addiw rd, rs1, #_imm -- 32-bit add-immediate (RV64).
D_NODISCARD int d_jit_riscv_emit_addiw(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int32_t _imm);

// d_jit_riscv_emit_slliw
//   function: slliw rd, rs1, #_shamt -- 32-bit shift (RV64, shamt 0-31).
D_NODISCARD int d_jit_riscv_emit_slliw(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shamt);

// d_jit_riscv_emit_srliw
//   function: srliw rd, rs1, #_shamt -- 32-bit shift (RV64, shamt 0-31).
D_NODISCARD int d_jit_riscv_emit_srliw(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shamt);

// d_jit_riscv_emit_sraiw
//   function: sraiw rd, rs1, #_shamt -- 32-bit shift (RV64, shamt 0-31).
D_NODISCARD int d_jit_riscv_emit_sraiw(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shamt);

// d_jit_riscv_emit_addw
//   function: addw rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_addw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_riscv_emit_subw
//   function: subw rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_subw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_riscv_emit_sllw
//   function: sllw rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sllw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_riscv_emit_srlw
//   function: srlw rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_srlw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_riscv_emit_sraw
//   function: sraw rd, rs1, rs2.
D_NODISCARD int d_jit_riscv_emit_sraw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_riscv_reg_name
//   function: the ABI name ("zero","ra","sp","a0",...) or "?".
const char* d_jit_riscv_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_RISCV_
