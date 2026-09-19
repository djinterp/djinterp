/******************************************************************************
* djinterp [jit]                                                   jit_s390x.h
*
* djinterp s390x (IBM z/Architecture) JIT encoder (variable-length, big-endian)
*   The z/Architecture (64-bit s390x) half of the djinterp JIT. Unlike every
* other target so far, instructions are VARIABLE LENGTH -- 2, 4, or 6 bytes --
* and, like SPARC, BIG-ENDIAN. Each emitter writes its exact byte length most-
* significant-byte first, and the two branch relocations do big-endian
* read-modify-writes on the displacement field.
*
*   TWO-OPERAND FORM: the classic instructions accumulate in place, r1 = r1 op
* r2 (like x86), not the three-operand r1 = r2 op r3 of the RISC targets. The
* distinct-operand 'K' variants are not emitted here; compose with a copy (lgr)
* first when you need the source preserved.
*
*   ADDRESSING: storage operands are disp(index, base); pass index or base 0
* for 'none' (r0 as base or index means zero, not r0's contents). The RX forms
* take a 12-bit UNSIGNED displacement (0..4095); RXY and 64-bit forms take a
* 64-bit ops) take a 20-bit SIGNED displacement.
*
* 20-bit SIGNED displacement.
*
*   CONDITION CODE: most arithmetic and compare instructions set a 2-bit
* code; a relative branch then tests it through a 4-bit mask (one bit per code
* value. After compare: EQ=8, LT=4, GT=2, and their unions. Branch targets are
* d_jit_label; relative displacements count HALFWORDS from the branch itself.
*
*   Registers: r0-r15, 64-bit. r15 is the stack pointer; r14 the return addr.
* All encodings verified with llvm-mc (-triple=s390x).
*
*   NAMING CONVENTION:
*     D_JIT_S390X_REG_[name]  - a register operand number (0-15)
*     D_JIT_S390X_CC_[cond]   - a 4-bit branch condition mask
*     d_jit_s390x_emit_*      - pack and emit one instruction
*
*   Requires:  jit.h (d_jit_buffer, the emit primitives, the label facility).
*
* path:      /inc/djinterp/jit/jit_s390x.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_S390X_
#define DJINTERP_JIT_S390X_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_S390X_ENCODING
//   feature: 1, indicating the s390x encoder is available.
#ifndef D_JIT_S390X_ENCODING
    #define D_JIT_S390X_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER AND CONDITION-MASK NUMBERS
// ===========================================================================
//   The 16 general registers, 64-bit. r15 is the stack pointer and r14 the
// return address by convention; as an address base or index, r0 means zero.

#define D_JIT_S390X_REG_R0      0
#define D_JIT_S390X_REG_R1      1
#define D_JIT_S390X_REG_R2      2
#define D_JIT_S390X_REG_R3      3
#define D_JIT_S390X_REG_R4      4
#define D_JIT_S390X_REG_R5      5
#define D_JIT_S390X_REG_R6      6
#define D_JIT_S390X_REG_R7      7
#define D_JIT_S390X_REG_R8      8
#define D_JIT_S390X_REG_R9      9
#define D_JIT_S390X_REG_R10     10
#define D_JIT_S390X_REG_R11     11
#define D_JIT_S390X_REG_R12     12
#define D_JIT_S390X_REG_R13     13
#define D_JIT_S390X_REG_R14     14
#define D_JIT_S390X_REG_R15     15

#define D_JIT_S390X_REG_SP     15
#define D_JIT_S390X_REG_RA     14

// branch condition masks (bit per condition code: CC0=8 CC1=4 CC2=2 CC3=1)
//   After a compare, CC0 = equal, CC1 = low (<), CC2 = high (>).
#define D_JIT_S390X_CC_EQ     0x8   // equal
#define D_JIT_S390X_CC_NE     0x7   // not equal
#define D_JIT_S390X_CC_LT     0x4   // less
#define D_JIT_S390X_CC_GT     0x2   // greater
#define D_JIT_S390X_CC_LE     0xC   // less or equal
#define D_JIT_S390X_CC_GE     0xA   // greater or equal
#define D_JIT_S390X_CC_ALWAYS 0xF   // unconditional


// ===========================================================================
// C.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands and emits one 2-, 4-, or 6-byte big-endian
// instruction. Two-operand arithmetic accumulates into r1 (r1 = r1 op r2).
// Immediate widths: 'hi' forms take a signed 16-bit value, 'fi' forms a
// 32-bit value; load_imm synthesizes any 64-bit constant. Returns 0 on
// success, -1 on a register, immediate-range, displacement, or branch-range
// error.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// register moves, load-and-test, complement, sign/zero extend
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_lr
//   function: lr r1, r2 -- copy r2 to r1 (32-bit).
D_NODISCARD int d_jit_s390x_emit_lr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_lgr
//   function: lgr r1, r2 -- copy (64-bit).
D_NODISCARD int d_jit_s390x_emit_lgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_lgfr
//   function: lgfr r1, r2 -- sign-extend r2[32] into r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_lgfr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_llgfr
//   function: llgfr r1, r2 -- zero-extend r2[32] into r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_llgfr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_ltr
//   function: ltr r1, r2 -- copy and set condition code (32-bit).
D_NODISCARD int d_jit_s390x_emit_ltr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_ltgr
//   function: ltgr r1, r2 -- copy and set condition code (64-bit).
D_NODISCARD int d_jit_s390x_emit_ltgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_lcr
//   function: lcr r1, r2 -- load complement -r2 into r1 (32-bit).
D_NODISCARD int d_jit_s390x_emit_lcr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_lcgr
//   function: lcgr r1, r2 -- load complement -r2 into r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_lcgr(d_jit_buffer* _buf, int _r1, int _r2);

// ---------------------------------------------------------------------------
// load immediate / address
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_lhi
//   function: lhi r1, imm -- load signed 16-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_lhi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_lghi
//   function: lghi r1, imm -- load signed 16-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_lghi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// d_jit_s390x_emit_lgfi
//   function: lgfi r1, imm -- load signed 32-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_lgfi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// d_jit_s390x_emit_load_imm
//   function: load any 64-bit constant into r1 -- lghi when it fits 16
// bits, lgfi when it fits 32, else iihf+iilf.
D_NODISCARD int d_jit_s390x_emit_load_imm(d_jit_buffer* _buf, int _r1,
                                          int64_t _imm);

// d_jit_s390x_emit_larl
//   function: larl r1, _target -- load the address of _target (PC-relative
// long, halfword displacement) into r1.
D_NODISCARD int d_jit_s390x_emit_larl(d_jit_buffer* _buf, int _r1,
                                      d_jit_label* _target);

// ---------------------------------------------------------------------------
// arithmetic (two-operand: r1 = r1 op r2)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_ar
//   function: ar r1, r2 -- add (32-bit).
D_NODISCARD int d_jit_s390x_emit_ar(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_agr
//   function: agr r1, r2 -- add (64-bit).
D_NODISCARD int d_jit_s390x_emit_agr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_sr
//   function: sr r1, r2 -- subtract (32-bit).
D_NODISCARD int d_jit_s390x_emit_sr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_sgr
//   function: sgr r1, r2 -- subtract (64-bit).
D_NODISCARD int d_jit_s390x_emit_sgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_msr
//   function: msr r1, r2 -- multiply (32-bit).
D_NODISCARD int d_jit_s390x_emit_msr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_msgr
//   function: msgr r1, r2 -- multiply (64-bit).
D_NODISCARD int d_jit_s390x_emit_msgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_msgfr
//   function: msgfr r1, r2 -- multiply r1 by r2[32] (64-bit).
D_NODISCARD int d_jit_s390x_emit_msgfr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_ahi
//   function: ahi r1, imm -- add signed 16-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_ahi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_aghi
//   function: aghi r1, imm -- add signed 16-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_aghi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// d_jit_s390x_emit_afi
//   function: afi r1, imm -- add signed 32-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_afi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_agfi
//   function: agfi r1, imm -- add signed 32-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_agfi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// d_jit_s390x_emit_mhi
//   function: mhi r1, imm -- multiply by signed 16-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_mhi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_mghi
//   function: mghi r1, imm -- multiply by signed 16-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_mghi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// ---------------------------------------------------------------------------
// divide (dividend in the r1:r1+1 even/odd pair; r1 must be even)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_dsgr
//   function: dsgr r1, r2 -- signed 64-bit: r1<-remainder, r1+1<-quotient of
//   (r1:r1+1)/r2.
D_NODISCARD int d_jit_s390x_emit_dsgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_dsgfr
//   function: dsgfr r1, r2 -- signed 64/32-bit: divide r1+1 by r2[32].
D_NODISCARD int d_jit_s390x_emit_dsgfr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_dlgr
//   function: dlgr r1, r2 -- unsigned 64-bit divide of the r1:r1+1 pair by r2.
D_NODISCARD int d_jit_s390x_emit_dlgr(d_jit_buffer* _buf, int _r1, int _r2);

// ---------------------------------------------------------------------------
// logical (two-operand: r1 = r1 op r2; immediates act on low 32 bits)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_nr
//   function: nr r1, r2 -- AND (32-bit).
D_NODISCARD int d_jit_s390x_emit_nr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_ngr
//   function: ngr r1, r2 -- AND (64-bit).
D_NODISCARD int d_jit_s390x_emit_ngr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_or
//   function: or r1, r2 -- OR (32-bit).
D_NODISCARD int d_jit_s390x_emit_or(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_ogr
//   function: ogr r1, r2 -- OR (64-bit).
D_NODISCARD int d_jit_s390x_emit_ogr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_xr
//   function: xr r1, r2 -- XOR (32-bit).
D_NODISCARD int d_jit_s390x_emit_xr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_xgr
//   function: xgr r1, r2 -- XOR (64-bit).
D_NODISCARD int d_jit_s390x_emit_xgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_nilf
//   function: nilf r1, imm -- AND low 32 bits with a 32-bit mask.
D_NODISCARD int d_jit_s390x_emit_nilf(d_jit_buffer* _buf, int _r1,
                                      uint32_t _imm);

// d_jit_s390x_emit_oilf
//   function: oilf r1, imm -- OR low 32 bits with a 32-bit mask.
D_NODISCARD int d_jit_s390x_emit_oilf(d_jit_buffer* _buf, int _r1,
                                      uint32_t _imm);

// d_jit_s390x_emit_xilf
//   function: xilf r1, imm -- XOR low 32 bits with a 32-bit mask.
D_NODISCARD int d_jit_s390x_emit_xilf(d_jit_buffer* _buf, int _r1,
                                      uint32_t _imm);

// d_jit_s390x_emit_iihf
//   function: iihf r1, imm -- insert a 32-bit value into the high 32 bits.
D_NODISCARD int d_jit_s390x_emit_iihf(d_jit_buffer* _buf, int _r1,
                                      uint32_t _imm);

// d_jit_s390x_emit_iilf
//   function: iilf r1, imm -- insert a 32-bit value into the low 32 bits.
D_NODISCARD int d_jit_s390x_emit_iilf(d_jit_buffer* _buf, int _r1,
                                      uint32_t _imm);

// ---------------------------------------------------------------------------
// shifts (count is a 6-bit immediate; b=0 so no register amount)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_sll
//   function: sll r1, count -- shift left logical (32-bit).
D_NODISCARD int d_jit_s390x_emit_sll(d_jit_buffer* _buf, int _r1,
                                     unsigned _count);

// d_jit_s390x_emit_srl
//   function: srl r1, count -- shift right logical (32-bit).
D_NODISCARD int d_jit_s390x_emit_srl(d_jit_buffer* _buf, int _r1,
                                     unsigned _count);

// d_jit_s390x_emit_sra
//   function: sra r1, count -- shift right arithmetic (32-bit).
D_NODISCARD int d_jit_s390x_emit_sra(d_jit_buffer* _buf, int _r1,
                                     unsigned _count);

// d_jit_s390x_emit_sllg
//   function: sllg r1, r3, count -- shift left logical (64-bit) (r1 <- r3
//   shifted).
D_NODISCARD int d_jit_s390x_emit_sllg(d_jit_buffer* _buf, int _r1, int _r3,
                                      unsigned _count);

// d_jit_s390x_emit_srlg
//   function: srlg r1, r3, count -- shift right logical (64-bit) (r1 <- r3
//   shifted).
D_NODISCARD int d_jit_s390x_emit_srlg(d_jit_buffer* _buf, int _r1, int _r3,
                                      unsigned _count);

// d_jit_s390x_emit_srag
//   function: srag r1, r3, count -- shift right arithmetic (64-bit) (r1 <- r3
//   shifted).
D_NODISCARD int d_jit_s390x_emit_srag(d_jit_buffer* _buf, int _r1, int _r3,
                                      unsigned _count);

// ---------------------------------------------------------------------------
// compare (sets condition code; register and immediate)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_cr
//   function: cr r1, r2 -- compare signed (32-bit).
D_NODISCARD int d_jit_s390x_emit_cr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_cgr
//   function: cgr r1, r2 -- compare signed (64-bit).
D_NODISCARD int d_jit_s390x_emit_cgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_clr
//   function: clr r1, r2 -- compare unsigned (32-bit).
D_NODISCARD int d_jit_s390x_emit_clr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_clgr
//   function: clgr r1, r2 -- compare unsigned (64-bit).
D_NODISCARD int d_jit_s390x_emit_clgr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_chi
//   function: chi r1, imm -- compare with signed 16-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_chi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_cghi
//   function: cghi r1, imm -- compare with signed 16-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_cghi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// d_jit_s390x_emit_cfi
//   function: cfi r1, imm -- compare with signed 32-bit immediate (32-bit).
D_NODISCARD int d_jit_s390x_emit_cfi(d_jit_buffer* _buf, int _r1,
                                     int32_t _imm);

// d_jit_s390x_emit_cgfi
//   function: cgfi r1, imm -- compare with signed 32-bit immediate (64-bit).
D_NODISCARD int d_jit_s390x_emit_cgfi(d_jit_buffer* _buf, int _r1,
                                      int32_t _imm);

// ---------------------------------------------------------------------------
// storage: RX forms, 12-bit UNSIGNED displacement (0..4095)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_l
//   function: l r1, disp(index, base) -- load r1 (32-bit).
D_NODISCARD int d_jit_s390x_emit_l(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                   int _index, int _base);

// d_jit_s390x_emit_st
//   function: st r1, disp(index, base) -- store r1 (32-bit).
D_NODISCARD int d_jit_s390x_emit_st(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_a
//   function: a r1, disp(index, base) -- add storage to r1 (32-bit).
D_NODISCARD int d_jit_s390x_emit_a(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                   int _index, int _base);

// d_jit_s390x_emit_s
//   function: s r1, disp(index, base) -- subtract storage (32-bit).
D_NODISCARD int d_jit_s390x_emit_s(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                   int _index, int _base);

// d_jit_s390x_emit_c
//   function: c r1, disp(index, base) -- compare r1 with storage (32-bit).
D_NODISCARD int d_jit_s390x_emit_c(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                   int _index, int _base);

// d_jit_s390x_emit_la
//   function: la r1, disp(index, base) -- load the address disp(index,base)
//   into r1.
D_NODISCARD int d_jit_s390x_emit_la(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// ---------------------------------------------------------------------------
// storage: RXY forms, 20-bit SIGNED displacement
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_lg
//   function: lg r1, disp(index, base) -- load r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_lg(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_stg
//   function: stg r1, disp(index, base) -- store r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_stg(d_jit_buffer* _buf, int _r1,
                                     int32_t _disp, int _index, int _base);

// d_jit_s390x_emit_ag
//   function: ag r1, disp(index, base) -- add storage to r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_ag(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_sg
//   function: sg r1, disp(index, base) -- subtract storage (64-bit).
D_NODISCARD int d_jit_s390x_emit_sg(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_cg
//   function: cg r1, disp(index, base) -- compare r1 with storage (64-bit).
D_NODISCARD int d_jit_s390x_emit_cg(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_ly
//   function: ly r1, disp(index, base) -- load r1 (32-bit, long displacement).
D_NODISCARD int d_jit_s390x_emit_ly(d_jit_buffer* _buf, int _r1, int32_t _disp,
                                    int _index, int _base);

// d_jit_s390x_emit_sty
//   function: sty r1, disp(index, base) -- store r1 (32-bit, long
//   displacement).
D_NODISCARD int d_jit_s390x_emit_sty(d_jit_buffer* _buf, int _r1,
                                     int32_t _disp, int _index, int _base);

// d_jit_s390x_emit_lay
//   function: lay r1, disp(index, base) -- load the address disp(index,base)
//   into r1.
D_NODISCARD int d_jit_s390x_emit_lay(d_jit_buffer* _buf, int _r1,
                                     int32_t _disp, int _index, int _base);

// d_jit_s390x_emit_lgf
//   function: lgf r1, disp(index, base) -- load and sign-extend storage[32]
//   into r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_lgf(d_jit_buffer* _buf, int _r1,
                                     int32_t _disp, int _index, int _base);

// d_jit_s390x_emit_llgf
//   function: llgf r1, disp(index, base) -- load and zero-extend storage[32]
//   into r1 (64-bit).
D_NODISCARD int d_jit_s390x_emit_llgf(d_jit_buffer* _buf, int _r1,
                                      int32_t _disp, int _index, int _base);

// ---------------------------------------------------------------------------
// control transfer (relative displacements are halfwords from the branch)
// ---------------------------------------------------------------------------

// d_jit_s390x_emit_brc
//   function: brc mask, _target -- branch relative on condition (+/-64
// KB). mask is a D_JIT_S390X_CC_* value; the named wrappers cover the usual
// conditions.
D_NODISCARD int d_jit_s390x_emit_brc(d_jit_buffer* _buf, unsigned _mask,
                                     d_jit_label* _target);

// d_jit_s390x_emit_brcl
//   function: brcl mask, _target -- branch relative on condition long
// (+/-4 GB).
D_NODISCARD int d_jit_s390x_emit_brcl(d_jit_buffer* _buf, unsigned _mask,
                                      d_jit_label* _target);

// d_jit_s390x_emit_je
//   function: je _target -- brc, branch if equal.
D_NODISCARD int d_jit_s390x_emit_je(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jne
//   function: jne _target -- brc, branch if not equal.
D_NODISCARD int d_jit_s390x_emit_jne(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jlt
//   function: jlt _target -- brc, branch if less.
D_NODISCARD int d_jit_s390x_emit_jlt(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jgt
//   function: jgt _target -- brc, branch if greater.
D_NODISCARD int d_jit_s390x_emit_jgt(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jle
//   function: jle _target -- brc, branch if less or equal.
D_NODISCARD int d_jit_s390x_emit_jle(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jge
//   function: jge _target -- brc, branch if greater or equal.
D_NODISCARD int d_jit_s390x_emit_jge(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_jmp
//   function: jmp _target -- brc, branch if unconditionally.
D_NODISCARD int d_jit_s390x_emit_jmp(d_jit_buffer* _buf, d_jit_label* _target);

// d_jit_s390x_emit_brasl
//   function: brasl r1, _target -- branch relative and save (call); the
// return address lands in r1.
D_NODISCARD int d_jit_s390x_emit_brasl(d_jit_buffer* _buf, int _r1,
                                       d_jit_label* _target);

// d_jit_s390x_emit_basr
//   function: basr r1, r2 -- branch and save to the address in r2 (call
// through a register).
D_NODISCARD int d_jit_s390x_emit_basr(d_jit_buffer* _buf, int _r1, int _r2);

// d_jit_s390x_emit_bcr
//   function: bcr mask, r2 -- branch on condition to the address in r2.
D_NODISCARD int d_jit_s390x_emit_bcr(d_jit_buffer* _buf, unsigned _mask,
                                     int _r2);

// d_jit_s390x_emit_br
//   function: br r2 -- unconditional branch to r2 (bcr 15, r2); with r14,
// the standard return.
D_NODISCARD int d_jit_s390x_emit_br(d_jit_buffer* _buf, int _r2);

// d_jit_s390x_emit_nop
//   function: nop (bcr 0, 0 = 0x0700).
D_NODISCARD int d_jit_s390x_emit_nop(d_jit_buffer* _buf);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_s390x_reg_name
//   function: the register name ("%r0".."%r15") or "?".
const char* d_jit_s390x_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_S390X_
