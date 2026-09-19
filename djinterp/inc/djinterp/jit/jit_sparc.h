/******************************************************************************
* djinterp [jit]                                                   jit_sparc.h
*
* djinterp SPARC (V8 / V9) JIT encoder (fixed-width 32-bit, big-endian words):
*   The SPARC half of the djinterp JIT. As with the other RISC targets, the V8
* (32-bit) base encodings are valid on V9 (64-bit); V9 adds instructions
* -- ldx/stx, the 64-bit shifts sllx/srlx/srax, mulx/udivx/sdivx -- isolated
* in section L. So one module serves both; do not emit section-L forms for a
* pure V8 target.
*
*   BIG-ENDIAN: SPARC instructions are always big-endian, so this module emits
* each word most-significant-byte first (independent of the host and of the
* buffer's little-endian primitives). This is the first big-endian target.
*
*   BRANCH DELAY SLOTS: like MIPS, every control transfer (branch, call, jmpl)
* has a delay slot -- the next instruction always executes. These emitters emit
* only the transfer word; the caller MUST follow each with a delay-slot
* instruction (emit_nop, or a useful one). Nothing is auto-inserted.
* Unlike MIPS, a SPARC branch displacement is relative to the branch itself.
*
*   CONDITION MODEL: arithmetic 'cc' forms (addcc, subcc, ...) set the integer
* condition codes; a Bicc branch then tests them. cmp is subcc to %g0. V9 also
* has %xcc (64-bit) codes, reached through the BPcc branch (bpcc, section L).
*
*   Three instruction formats: 1 (call, 30-bit disp), 2 (sethi and branches),
* 3 (everything else: op=10 arithmetic, op=11 loads/stores; a register or a
* 13-bit signed immediate). Register forms place the destination LAST in SPARC
* mnemonics; this API takes it first, as the other djinterp encoders do. Branch
* targets are d_jit_label. Encodings verified via llvm-mc (sparc / sparcv9).
*
*   Registers: %g0-%g7 (0-7), %o0-%o7 (8-15), %l0-%l7 (16-23), %i0-%i7 (24-31);
* %g0 reads as 0. %sp is %o6, %fp is %i6. Return address is in %o7 (leaf, retl)
* or %i7 (after save, ret).
*
*   NAMING CONVENTION:
*     D_JIT_SPARC_REG_[name]  - a register operand number (0-31)
*     D_JIT_SPARC_COND_[cc]   - a 4-bit branch condition
*     D_JIT_SPARC_CC_[icc|xcc]- the condition-code bank for a V9 BPcc branch
*     d_jit_sparc_emit_*      - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_sparc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_SPARC_
#define DJINTERP_JIT_SPARC_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_SPARC_ENCODING
//   feature: 1, indicating the SPARC encoder is available.
#ifndef D_JIT_SPARC_ENCODING
    #define D_JIT_SPARC_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER, CONDITION, AND CC NUMBERS
// ===========================================================================
//   The 32 integer registers by window name. %g0 always reads as 0; writes to
// it are discarded. %sp/%fp and the return-address regs have aliases.

#define D_JIT_SPARC_REG_G0      0
#define D_JIT_SPARC_REG_G1      1
#define D_JIT_SPARC_REG_G2      2
#define D_JIT_SPARC_REG_G3      3
#define D_JIT_SPARC_REG_G4      4
#define D_JIT_SPARC_REG_G5      5
#define D_JIT_SPARC_REG_G6      6
#define D_JIT_SPARC_REG_G7      7

#define D_JIT_SPARC_REG_O0      8
#define D_JIT_SPARC_REG_O1      9
#define D_JIT_SPARC_REG_O2      10
#define D_JIT_SPARC_REG_O3      11
#define D_JIT_SPARC_REG_O4      12
#define D_JIT_SPARC_REG_O5      13
#define D_JIT_SPARC_REG_O6      14
#define D_JIT_SPARC_REG_O7      15

#define D_JIT_SPARC_REG_L0      16
#define D_JIT_SPARC_REG_L1      17
#define D_JIT_SPARC_REG_L2      18
#define D_JIT_SPARC_REG_L3      19
#define D_JIT_SPARC_REG_L4      20
#define D_JIT_SPARC_REG_L5      21
#define D_JIT_SPARC_REG_L6      22
#define D_JIT_SPARC_REG_L7      23

#define D_JIT_SPARC_REG_I0      24
#define D_JIT_SPARC_REG_I1      25
#define D_JIT_SPARC_REG_I2      26
#define D_JIT_SPARC_REG_I3      27
#define D_JIT_SPARC_REG_I4      28
#define D_JIT_SPARC_REG_I5      29
#define D_JIT_SPARC_REG_I6      30
#define D_JIT_SPARC_REG_I7      31

// aliases
#define D_JIT_SPARC_REG_SP      14
#define D_JIT_SPARC_REG_FP      30
#define D_JIT_SPARC_REG_RA_LEAF 15
#define D_JIT_SPARC_REG_RA      31

// branch conditions (integer)
#define D_JIT_SPARC_COND_A     0x8
#define D_JIT_SPARC_COND_N     0x0
#define D_JIT_SPARC_COND_NE    0x9
#define D_JIT_SPARC_COND_E     0x1
#define D_JIT_SPARC_COND_G     0xA
#define D_JIT_SPARC_COND_LE    0x2
#define D_JIT_SPARC_COND_GE    0xB
#define D_JIT_SPARC_COND_L     0x3
#define D_JIT_SPARC_COND_GU    0xC
#define D_JIT_SPARC_COND_LEU   0x4
#define D_JIT_SPARC_COND_GEU   0xD
#define D_JIT_SPARC_COND_LU    0x5
#define D_JIT_SPARC_COND_POS   0xE
#define D_JIT_SPARC_COND_NEG   0x6
#define D_JIT_SPARC_COND_VC    0xF
#define D_JIT_SPARC_COND_VS    0x7

// condition-code bank for V9 BPcc
#define D_JIT_SPARC_CC_ICC    0
#define D_JIT_SPARC_CC_XCC    2


// ===========================================================================
// C.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands and emits one 32-bit big-endian word. The
// destination register is the first argument (SPARC mnemonics write it last).
// Immediate ALU/memory forms take a signed 13-bit value; sethi takes 22 bits.
// Returns 0 on success, -1 on an emit, immediate-range, or branch-range error.
// Remember the delay slot after every branch, call, and jmpl.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// integer arithmetic / logical, register form (op=10)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_add
//   function: add rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_add(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_addcc
//   function: addcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_addcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_addx
//   function: addx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_addx(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_addxcc
//   function: addxcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_addxcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int _rs2);

// d_jit_sparc_emit_sub
//   function: sub rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sub(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_subcc
//   function: subcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_subcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_subx
//   function: subx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_subx(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_subxcc
//   function: subxcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_subxcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int _rs2);

// d_jit_sparc_emit_and
//   function: and rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_and(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_andcc
//   function: andcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_andcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_andn
//   function: andn rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_andn(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_andncc
//   function: andncc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_andncc(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int _rs2);

// d_jit_sparc_emit_or
//   function: or rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_or(d_jit_buffer* _buf, int _rd, int _rs1,
                                    int _rs2);

// d_jit_sparc_emit_orcc
//   function: orcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_orcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_orn
//   function: orn rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_orn(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_orncc
//   function: orncc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_orncc(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_xor
//   function: xor rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_xor(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_xorcc
//   function: xorcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_xorcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_xnor
//   function: xnor rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_xnor(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_xnorcc
//   function: xnorcc rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_xnorcc(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int _rs2);

// ---------------------------------------------------------------------------
// arithmetic / logical, immediate form (13-bit signed)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_addi
//   function: add rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_addi(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_subi
//   function: sub rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_subi(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_andi
//   function: and rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_andi(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ori
//   function: or rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ori(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_sparc_emit_xori
//   function: xor rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_xori(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_addcci
//   function: addcc rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_addcci(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int32_t _imm);

// d_jit_sparc_emit_subcci
//   function: subcc rs1, #imm, rd (signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_subcci(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int32_t _imm);

// ---------------------------------------------------------------------------
// shifts (32-bit): register and immediate (shcnt 0-31)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_sll
//   function: sll rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sll(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_slli
//   function: sll rs1, #shcnt, rd (shcnt 0-31).
D_NODISCARD int d_jit_sparc_emit_slli(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shcnt);

// d_jit_sparc_emit_srl
//   function: srl rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_srl(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_srli
//   function: srl rs1, #shcnt, rd (shcnt 0-31).
D_NODISCARD int d_jit_sparc_emit_srli(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shcnt);

// d_jit_sparc_emit_sra
//   function: sra rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sra(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int _rs2);

// d_jit_sparc_emit_srai
//   function: sra rs1, #shcnt, rd (shcnt 0-31).
D_NODISCARD int d_jit_sparc_emit_srai(d_jit_buffer* _buf, int _rd, int _rs1,
                                      unsigned _shcnt);

// ---------------------------------------------------------------------------
// multiply / divide (V8; result uses %y -- see rd_y / wr_yi)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_umul
//   function: umul rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_umul(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_smul
//   function: smul rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_smul(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_udiv
//   function: udiv rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_udiv(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_sdiv
//   function: sdiv rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sdiv(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_rd_y
//   function: rd %y, rd (read the Y register, e.g. umul high word).
D_NODISCARD int d_jit_sparc_emit_rd_y(d_jit_buffer* _buf, int _rd);

// d_jit_sparc_emit_wr_yi
//   function: wr rs1, #imm, %y (set Y, e.g. before udiv/sdiv).
D_NODISCARD int d_jit_sparc_emit_wr_yi(d_jit_buffer* _buf, int _rs1,
                                       int32_t _imm);

// ---------------------------------------------------------------------------
// sethi / high-and-low immediate load
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_sethi
//   function: sethi #imm22, rd (rd[31:10] = imm22, rd[9:0] = 0).
D_NODISCARD int d_jit_sparc_emit_sethi(d_jit_buffer* _buf, int _rd,
                                       uint32_t _imm22);

// d_jit_sparc_emit_set
//   function: load a 32-bit immediate into rd: a single or (mov) when it
// fits 13 bits, a single sethi when the low 10 bits are 0, else sethi+or.
D_NODISCARD int d_jit_sparc_emit_set(d_jit_buffer* _buf, int _rd,
                                     int32_t _imm);

// ---------------------------------------------------------------------------
// loads / stores (immediate addressing: [rs1 + simm13])
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_ldub
//   function: ldub [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ldub(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ldsb
//   function: ldsb [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ldsb(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_lduh
//   function: lduh [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_lduh(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ldsh
//   function: ldsh [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ldsh(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ld
//   function: ld [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ld(d_jit_buffer* _buf, int _rd, int _rs1,
                                    int32_t _imm);

// d_jit_sparc_emit_stb
//   function: stb rd, [rs1+imm] (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_stb(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_sparc_emit_sth
//   function: sth rd, [rs1+imm] (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_sth(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_sparc_emit_st
//   function: st rd, [rs1+imm] (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_st(d_jit_buffer* _buf, int _rd, int _rs1,
                                    int32_t _imm);

// ---------------------------------------------------------------------------
// control transfer (each needs a delay slot)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_bicc
//   function: Bicc cond, _target -- branch on icc (+/-8 MB). cond is a
// D_JIT_SPARC_COND_* value; the named wrappers below cover the common ones.
D_NODISCARD int d_jit_sparc_emit_bicc(d_jit_buffer* _buf, unsigned _cond,
                                      d_jit_label* _target);

// d_jit_sparc_emit_ba
//   function: ba _target (branch if always).
D_NODISCARD int d_jit_sparc_emit_ba(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_be
//   function: be _target (branch if equal).
D_NODISCARD int d_jit_sparc_emit_be(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bne
//   function: bne _target (branch if not equal).
D_NODISCARD int d_jit_sparc_emit_bne(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bl
//   function: bl _target (branch if less).
D_NODISCARD int d_jit_sparc_emit_bl(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_ble
//   function: ble _target (branch if less or equal).
D_NODISCARD int d_jit_sparc_emit_ble(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bg
//   function: bg _target (branch if greater).
D_NODISCARD int d_jit_sparc_emit_bg(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bge
//   function: bge _target (branch if greater or equal).
D_NODISCARD int d_jit_sparc_emit_bge(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bgu
//   function: bgu _target (branch if greater unsigned).
D_NODISCARD int d_jit_sparc_emit_bgu(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_bleu
//   function: bleu _target (branch if less-equal unsigned).
D_NODISCARD int d_jit_sparc_emit_bleu(d_jit_buffer* _buf,
                                      d_jit_label* _target);

// d_jit_sparc_emit_bgeu
//   function: bgeu _target (branch if gtr-equal unsigned).
D_NODISCARD int d_jit_sparc_emit_bgeu(d_jit_buffer* _buf,
                                      d_jit_label* _target);

// d_jit_sparc_emit_blu
//   function: blu _target (branch if less unsigned).
D_NODISCARD int d_jit_sparc_emit_blu(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_sparc_emit_call
//   function: call _target -- PC-relative call (30-bit word disp); %o7 =
// return address.
D_NODISCARD int d_jit_sparc_emit_call(d_jit_buffer* _buf,
                                      d_jit_label* _target);

// d_jit_sparc_emit_jmpl
//   function: jmpl rs1 + #imm, rd -- jump and link (rd = return address).
D_NODISCARD int d_jit_sparc_emit_jmpl(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ret
//   function: ret -- return from a non-leaf routine (jmpl %i7+8, %g0).
D_NODISCARD int d_jit_sparc_emit_ret(d_jit_buffer* _buf);

// d_jit_sparc_emit_retl
//   function: retl -- return from a leaf routine (jmpl %o7+8, %g0).
D_NODISCARD int d_jit_sparc_emit_retl(d_jit_buffer* _buf);

// ---------------------------------------------------------------------------
// register-window and pseudo-instructions
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_save_i
//   function: save rs1, #imm, rd -- open a register window (e.g.
//  save %sp, -framesize, %sp).
D_NODISCARD int d_jit_sparc_emit_save_i(d_jit_buffer* _buf, int _rd, int _rs1,
                                        int32_t _imm);

// d_jit_sparc_emit_restore
//   function: restore -- close the register window (restore %g0,%g0,%g0).
D_NODISCARD int d_jit_sparc_emit_restore(d_jit_buffer* _buf);

// d_jit_sparc_emit_nop
//   function: nop (sethi 0, %g0 = 0x01000000).
D_NODISCARD int d_jit_sparc_emit_nop(d_jit_buffer* _buf);

// d_jit_sparc_emit_mov
//   function: mov rs, rd -- move register (or %g0, rs, rd).
D_NODISCARD int d_jit_sparc_emit_mov(d_jit_buffer* _buf, int _rd, int _rs);

// d_jit_sparc_emit_cmp
//   function: cmp rs1, rs2 -- compare (subcc rs1, rs2, %g0).
D_NODISCARD int d_jit_sparc_emit_cmp(d_jit_buffer* _buf, int _rs1, int _rs2);

// d_jit_sparc_emit_cmpi
//   function: cmp rs1, #imm -- compare with a signed 13-bit immediate.
D_NODISCARD int d_jit_sparc_emit_cmpi(d_jit_buffer* _buf, int _rs1,
                                      int32_t _imm);

// ---------------------------------------------------------------------------
// L.  V9-ONLY (invalid on a pure 32-bit V8 target)
// ---------------------------------------------------------------------------

// d_jit_sparc_emit_ldsw
//   function: ldsw [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ldsw(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int32_t _imm);

// d_jit_sparc_emit_ldx
//   function: ldx [rs1+imm], rd (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_ldx(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_sparc_emit_stx
//   function: stx rd, [rs1+imm] (immediate offset, signed 13-bit).
D_NODISCARD int d_jit_sparc_emit_stx(d_jit_buffer* _buf, int _rd, int _rs1,
                                     int32_t _imm);

// d_jit_sparc_emit_sllx
//   function: sllx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sllx(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_sllxi
//   function: sllx rs1, #shcnt, rd (shcnt 0-63).
D_NODISCARD int d_jit_sparc_emit_sllxi(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shcnt);

// d_jit_sparc_emit_srlx
//   function: srlx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_srlx(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_srlxi
//   function: srlx rs1, #shcnt, rd (shcnt 0-63).
D_NODISCARD int d_jit_sparc_emit_srlxi(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shcnt);

// d_jit_sparc_emit_srax
//   function: srax rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_srax(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_sraxi
//   function: srax rs1, #shcnt, rd (shcnt 0-63).
D_NODISCARD int d_jit_sparc_emit_sraxi(d_jit_buffer* _buf, int _rd, int _rs1,
                                       unsigned _shcnt);

// d_jit_sparc_emit_mulx
//   function: mulx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_mulx(d_jit_buffer* _buf, int _rd, int _rs1,
                                      int _rs2);

// d_jit_sparc_emit_udivx
//   function: udivx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_udivx(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_sdivx
//   function: sdivx rs1, rs2, rd (rd is written last).
D_NODISCARD int d_jit_sparc_emit_sdivx(d_jit_buffer* _buf, int _rd, int _rs1,
                                       int _rs2);

// d_jit_sparc_emit_bpcc
//   function: BPcc cond, cc, _target -- V9 branch with prediction (+/-1
// MB). _cc selects %icc or %xcc (the 64-bit codes); predicted taken.
D_NODISCARD int d_jit_sparc_emit_bpcc(d_jit_buffer* _buf, unsigned _cond,
                                      int _cc, d_jit_label* _target);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_sparc_reg_name
//   function: the register name ("%g0".."%i7") or "?".
const char* d_jit_sparc_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_SPARC_
