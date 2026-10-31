/*******************************************************************************
* djinterp [c/jit]                                                     jit_ppc.h
*
* djinterp PowerPC (PPC32 / PPC64) JIT encoder (fixed-width 32-bit words):
*   The PowerPC half of the djinterp JIT. Like MIPS and RISC-V, the PPC32 and
* PPC64 base encodings are identical (register width is not encoded), so
* one module serves both; PPC64 adds the doubleword instructions (ld/std, the
* rld* rotates, mulld/divd, sld/srd, extsw/cntlzd), isolated in section L and
* invalid on a 32-bit target.
*
*   NO DELAY SLOTS: unlike MIPS, a PowerPC branch takes effect immediately; the
* instruction after a branch is NOT executed unconditionally. Nothing is
* required of the caller.
*
*   CONDITION MODEL: PowerPC does not branch on registers directly. A compare
* (cmpw/cmpwi/...) sets a 4-bit field of the condition register (CR0-CR7); a
* conditional branch tests one bit of that field. So 'branch if r3 == 0' is
* cmpwi CR0, r3, 0 followed by beq CR0, target. The bit order within a CR field
* is LT(0), GT(1), EQ(2), SO(3); beq/bne/blt/bge/bgt/ble build the right test.
*
*   ENDIANNESS: instruction words are emitted little-endian (the ppc64le
* convention that dominates Linux on POWER, and matching the buffer's emit
* primitives). A big-endian target (AIX, most PPC32, embedded, consoles) would
* require byte-swapped emission.
*
*   Forms used: D (opcode, two regs, 16-bit imm), X / XO (opcode 31 + a 10-bit
* extended opcode), M / MD (rotate-and-mask), I (b/bl), B (bc), XL (blr/bctr),
* DS (ld/std), XFX (mtlr/mflr). Branch targets are dj_jit_label. All encodings
* verified via llvm-mc (powerpc64le / powerpc64).
*
*   Registers are the 32 GPRs (R0-R31); R0 reads as literal 0 in the base
* operand of addi and the load/store forms. Condition fields are CR0-CR7; link
* and count registers are reached through mtlr/mflr and mtctr/mfctr.
*
*   NAMING CONVENTION:
*     D_JIT_PPC_REG_[n]    - a GPR operand number (0-31)
*     D_JIT_PPC_CR[n]      - a condition-register field number (0-7)
*     d_jit_ppc_emit_*     - pack and emit one instruction
*
*   Requires:  jit.h (dj_jit_buffer, the emit primitives, the label facility).
*
*
* path:      /inc/djinterp/c/modules/jit/jit_ppc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.16
*                                                            revised: 2026.10.23
*******************************************************************************/

#ifndef DJINTERP_C_JIT_JIT_PPC_H
#define DJINTERP_C_JIT_JIT_PPC_H 1

// djinterp
#include "jit.h"
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // RE_STD_HAS_INT64: its floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict C++98
// wherever int64_t is `long long`: a 32-bit target, 64-bit Windows or macOS --
// the header compiles to nothing (the owner's ruling of 2026.10.03 on round 3's
// question 1, (a)).
#if RE_STD_HAS_INT64


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_PPC_ENCODING
//   feature: 1, indicating the PowerPC encoder is available.
#ifndef D_JIT_PPC_ENCODING
    #define D_JIT_PPC_ENCODING 1
#endif


// ===========================================================================
// B.   REGISTER AND CONDITION-FIELD NUMBERS
// ===========================================================================
//   The 32 general-purpose registers R0-R31, and the eight condition-register
// fields CR0-CR7. R0 denotes the literal value 0 (not the register) when used
// as the base of addi/addis or a load/store; CR0 is the field the '.' record
// forms and the simple compares target by default.

#define D_JIT_PPC_REG_R0     0
#define D_JIT_PPC_REG_R1     1
#define D_JIT_PPC_REG_R2     2
#define D_JIT_PPC_REG_R3     3
#define D_JIT_PPC_REG_R4     4
#define D_JIT_PPC_REG_R5     5
#define D_JIT_PPC_REG_R6     6
#define D_JIT_PPC_REG_R7     7
#define D_JIT_PPC_REG_R8     8
#define D_JIT_PPC_REG_R9     9
#define D_JIT_PPC_REG_R10    10
#define D_JIT_PPC_REG_R11    11
#define D_JIT_PPC_REG_R12    12
#define D_JIT_PPC_REG_R13    13
#define D_JIT_PPC_REG_R14    14
#define D_JIT_PPC_REG_R15    15
#define D_JIT_PPC_REG_R16    16
#define D_JIT_PPC_REG_R17    17
#define D_JIT_PPC_REG_R18    18
#define D_JIT_PPC_REG_R19    19
#define D_JIT_PPC_REG_R20    20
#define D_JIT_PPC_REG_R21    21
#define D_JIT_PPC_REG_R22    22
#define D_JIT_PPC_REG_R23    23
#define D_JIT_PPC_REG_R24    24
#define D_JIT_PPC_REG_R25    25
#define D_JIT_PPC_REG_R26    26
#define D_JIT_PPC_REG_R27    27
#define D_JIT_PPC_REG_R28    28
#define D_JIT_PPC_REG_R29    29
#define D_JIT_PPC_REG_R30    30
#define D_JIT_PPC_REG_R31    31

#define D_JIT_PPC_CR0        0
#define D_JIT_PPC_CR1        1
#define D_JIT_PPC_CR2        2
#define D_JIT_PPC_CR3        3
#define D_JIT_PPC_CR4        4
#define D_JIT_PPC_CR5        5
#define D_JIT_PPC_CR6        6
#define D_JIT_PPC_CR7        7


// ===========================================================================
// C.   INSTRUCTION EMITTERS
// ===========================================================================
//   Each packs its operands into the correct form and emits one 32-bit word.
// The destination is the first operand, following PowerPC mnemonic order (for
// the logical and shift forms the architecture places that register in the RA
// field; the encoders handle it). Returns 0 on success, -1 on an emit,
// immediate-range, or branch-range error. D-form immediates are signed 16-bit
// unless noted unsigned; DS-form displacements must be a multiple of 4.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// ---------------------------------------------------------------------------
// integer arithmetic (XO-form)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_add
//   function: add rt, ra, rb (rt = ra + rb).
D_NODISCARD int d_jit_ppc_emit_add(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_subf
//   function: subf rt, ra, rb (rt = rb - ra).
D_NODISCARD int d_jit_ppc_emit_subf(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_addc
//   function: addc rt, ra, rb (carry out).
D_NODISCARD int d_jit_ppc_emit_addc(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_adde
//   function: adde rt, ra, rb (extend carry).
D_NODISCARD int d_jit_ppc_emit_adde(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_subfc
//   function: subfc rt, ra, rb (rt = rb - ra, carry).
D_NODISCARD int d_jit_ppc_emit_subfc(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_subfe
//   function: subfe rt, ra, rb (rt = ~ra + rb + CA).
D_NODISCARD int d_jit_ppc_emit_subfe(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_neg
//   function: neg rt, ra (rt = -ra).
D_NODISCARD int d_jit_ppc_emit_neg(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra
);

// d_jit_ppc_emit_mullw
//   function: mullw rt, ra, rb (low 32 of product).
D_NODISCARD int d_jit_ppc_emit_mullw(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_mulhw
//   function: mulhw rt, ra, rb (high 32, signed).
D_NODISCARD int d_jit_ppc_emit_mulhw(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_mulhwu
//   function: mulhwu rt, ra, rb (high 32, unsigned).
D_NODISCARD int d_jit_ppc_emit_mulhwu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_divw
//   function: divw rt, ra, rb (signed 32/32).
D_NODISCARD int d_jit_ppc_emit_divw(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_divwu
//   function: divwu rt, ra, rb (unsigned 32/32).
D_NODISCARD int d_jit_ppc_emit_divwu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// ---------------------------------------------------------------------------
// arithmetic with immediate (D-form; ra = R0 means literal 0)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_addi
//   function: addi rt, ra, #si (rt = (ra|0) + si).
D_NODISCARD int d_jit_ppc_emit_addi(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _si
);

// d_jit_ppc_emit_addis
//   function: addis rt, ra, #si (rt = (ra|0) + (si<<16)).
D_NODISCARD int d_jit_ppc_emit_addis(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _si
);

// d_jit_ppc_emit_addic
//   function: addic rt, ra, #si (carry out).
D_NODISCARD int d_jit_ppc_emit_addic(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _si
);

// d_jit_ppc_emit_subfic
//   function: subfic rt, ra, #si (rt = si - ra).
D_NODISCARD int d_jit_ppc_emit_subfic(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _si
);

// d_jit_ppc_emit_mulli
//   function: mulli rt, ra, #si (low 32 of ra*si).
D_NODISCARD int d_jit_ppc_emit_mulli(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _si
);

// ---------------------------------------------------------------------------
// logical (X-form: ra is the destination)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_and
//   function: and ra, rs, rb (ra = rs & rb).
D_NODISCARD int d_jit_ppc_emit_and(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_or
//   function: or ra, rs, rb (ra = rs | rb).
D_NODISCARD int d_jit_ppc_emit_or(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_xor
//   function: xor ra, rs, rb (ra = rs ^ rb).
D_NODISCARD int d_jit_ppc_emit_xor(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_nand
//   function: nand ra, rs, rb (ra = ~(rs & rb)).
D_NODISCARD int d_jit_ppc_emit_nand(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_nor
//   function: nor ra, rs, rb (ra = ~(rs | rb)).
D_NODISCARD int d_jit_ppc_emit_nor(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_andc
//   function: andc ra, rs, rb (ra = rs & ~rb).
D_NODISCARD int d_jit_ppc_emit_andc(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_orc
//   function: orc ra, rs, rb (ra = rs | ~rb).
D_NODISCARD int d_jit_ppc_emit_orc(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_eqv
//   function: eqv ra, rs, rb (ra = ~(rs ^ rb)).
D_NODISCARD int d_jit_ppc_emit_eqv(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// ---------------------------------------------------------------------------
// logical with immediate (D-form, unsigned; ra is the destination)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_andi_
//   function: andi. ra, rs, #ui (sets CR0).
D_NODISCARD int d_jit_ppc_emit_andi_(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// d_jit_ppc_emit_andis_
//   function: andis. ra, rs, #ui (ui<<16, sets CR0).
D_NODISCARD int d_jit_ppc_emit_andis_(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// d_jit_ppc_emit_ori
//   function: ori ra, rs, #ui (ra = rs | ui).
D_NODISCARD int d_jit_ppc_emit_ori(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// d_jit_ppc_emit_oris
//   function: oris ra, rs, #ui (ra = rs | (ui<<16)).
D_NODISCARD int d_jit_ppc_emit_oris(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// d_jit_ppc_emit_xori
//   function: xori ra, rs, #ui (ra = rs ^ ui).
D_NODISCARD int d_jit_ppc_emit_xori(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// d_jit_ppc_emit_xoris
//   function: xoris ra, rs, #ui (ra = rs ^ (ui<<16)).
D_NODISCARD int d_jit_ppc_emit_xoris(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    uint32_t       _ui
);

// ---------------------------------------------------------------------------
// sign-extend / count-leading-zeros (X-form, ra is the destination)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_extsb
//   function: extsb ra, rs (sign-extend byte).
D_NODISCARD int d_jit_ppc_emit_extsb(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// d_jit_ppc_emit_extsh
//   function: extsh ra, rs (sign-extend halfword).
D_NODISCARD int d_jit_ppc_emit_extsh(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// d_jit_ppc_emit_cntlzw
//   function: cntlzw ra, rs (count leading zero bits, 32).
D_NODISCARD int d_jit_ppc_emit_cntlzw(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// ---------------------------------------------------------------------------
// shifts (X-form register; ra is the destination)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_slw
//   function: slw ra, rs, rb (shift left word).
D_NODISCARD int d_jit_ppc_emit_slw(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_srw
//   function: srw ra, rs, rb (shift right word, logical).
D_NODISCARD int d_jit_ppc_emit_srw(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_sraw
//   function: sraw ra, rs, rb (shift right word, arithmetic).
D_NODISCARD int d_jit_ppc_emit_sraw(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_srawi
//   function: srawi ra, rs, #_sh (arithmetic right shift, _sh 0-31).
D_NODISCARD int d_jit_ppc_emit_srawi(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh
);

// ---------------------------------------------------------------------------
// rotate-and-mask (M-form)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_rlwinm
//   function: rlwinm ra, rs, sh, mb, me -- rotate rs left sh, keep mask
//   [mb:me].
// Covers word shifts, extracts, and clears (_sh/_mb/_me each 0-31).
D_NODISCARD int d_jit_ppc_emit_rlwinm(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _mb,
    unsigned       _me
);

// d_jit_ppc_emit_rlwimi
//   function: rlwimi ra, rs, sh, mb, me -- rotate-insert under mask [mb:me].
D_NODISCARD int d_jit_ppc_emit_rlwimi(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _mb,
    unsigned       _me
);

// d_jit_ppc_emit_rlwnm
//   function: rlwnm ra, rs, rb, mb, me -- rotate by rb, keep mask [mb:me].
D_NODISCARD int d_jit_ppc_emit_rlwnm(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb,
    unsigned       _mb,
    unsigned       _me
);

// ---------------------------------------------------------------------------
// loads (D-form; ra = R0 means base 0)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_lbz
//   function: lbz rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lbz(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lhz
//   function: lhz rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lhz(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lha
//   function: lha rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lha(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lwz
//   function: lwz rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lwz(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lbzu
//   function: lbzu rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lbzu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lwzu
//   function: lwzu rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lwzu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lbzx
//   function: lbzx rt, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_lbzx(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_lhzx
//   function: lhzx rt, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_lhzx(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_lwzx
//   function: lwzx rt, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_lwzx(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// ---------------------------------------------------------------------------
// stores (D-form; ra = R0 means base 0)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_stb
//   function: stb rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_stb(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_sth
//   function: sth rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_sth(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_stw
//   function: stw rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_stw(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_stwu
//   function: stwu rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_stwu(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_stbx
//   function: stbx rs, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_stbx(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_sthx
//   function: sthx rs, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_sthx(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_stwx
//   function: stwx rs, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_stwx(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int            _rb
);

// ---------------------------------------------------------------------------
// compares (set a CR field; _crf is CR0-CR7)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_cmpw
//   function: cmpw crf, ra, rb (signed 32-bit compare).
D_NODISCARD int d_jit_ppc_emit_cmpw(
    dj_jit_buffer* _buf,
    int            _crf,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_cmpwi
//   function: cmpwi crf, ra, #si (signed 32-bit compare).
D_NODISCARD int d_jit_ppc_emit_cmpwi(
    dj_jit_buffer* _buf,
    int            _crf,
    int            _ra,
    int32_t        _si
);

// d_jit_ppc_emit_cmplw
//   function: cmplw crf, ra, rb (unsigned 32-bit compare).
D_NODISCARD int d_jit_ppc_emit_cmplw(
    dj_jit_buffer* _buf,
    int            _crf,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_cmplwi
//   function: cmplwi crf, ra, #ui (unsigned 32-bit compare).
D_NODISCARD int d_jit_ppc_emit_cmplwi(
    dj_jit_buffer* _buf,
    int            _crf,
    int            _ra,
    uint32_t       _ui
);

// ---------------------------------------------------------------------------
// branches (I-/B-form PC-relative to dj_jit_label; XL-form via LR/CTR)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_b
//   function: b _target -- unconditional branch (+/-32 MB).
D_NODISCARD int d_jit_ppc_emit_b(
    dj_jit_buffer* _buf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_bl
//   function: bl _target -- branch and link (call); sets LR.
D_NODISCARD int d_jit_ppc_emit_bl(
    dj_jit_buffer* _buf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_bc
//   function: bc bo, bi, _target -- raw conditional branch (+/-32 KB).
// bo selects the test, bi the CR bit (4*crf + LT0/GT1/EQ2/SO3).
D_NODISCARD int d_jit_ppc_emit_bc(
    dj_jit_buffer* _buf,
    unsigned       _bo,
    unsigned       _bi,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_beq
//   function: beq crf, _target (branch if crf shows == 0).
D_NODISCARD int d_jit_ppc_emit_beq(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_bne
//   function: bne crf, _target (branch if crf shows != 0).
D_NODISCARD int d_jit_ppc_emit_bne(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_blt
//   function: blt crf, _target (branch if crf shows < 0).
D_NODISCARD int d_jit_ppc_emit_blt(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_bge
//   function: bge crf, _target (branch if crf shows >= 0).
D_NODISCARD int d_jit_ppc_emit_bge(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_bgt
//   function: bgt crf, _target (branch if crf shows > 0).
D_NODISCARD int d_jit_ppc_emit_bgt(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_ble
//   function: ble crf, _target (branch if crf shows <= 0).
D_NODISCARD int d_jit_ppc_emit_ble(
    dj_jit_buffer* _buf,
    int            _crf,
    dj_jit_label*  _target
);

// d_jit_ppc_emit_blr
//   function: blr -- return (branch to LR).
D_NODISCARD int d_jit_ppc_emit_blr(
    dj_jit_buffer* _buf
);

// d_jit_ppc_emit_bctr
//   function: bctr -- jump to CTR.
D_NODISCARD int d_jit_ppc_emit_bctr(
    dj_jit_buffer* _buf
);

// d_jit_ppc_emit_bctrl
//   function: bctrl -- call via CTR; sets LR.
D_NODISCARD int d_jit_ppc_emit_bctrl(
    dj_jit_buffer* _buf
);

// ---------------------------------------------------------------------------
// link / count register moves (XFX-form)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_mflr
//   function: mflr rt (rt = LR).
D_NODISCARD int d_jit_ppc_emit_mflr(
    dj_jit_buffer* _buf,
    int            _rt
);

// d_jit_ppc_emit_mtlr
//   function: mtlr rs (LR = rs).
D_NODISCARD int d_jit_ppc_emit_mtlr(
    dj_jit_buffer* _buf,
    int            _rs
);

// d_jit_ppc_emit_mfctr
//   function: mfctr rt (rt = CTR).
D_NODISCARD int d_jit_ppc_emit_mfctr(
    dj_jit_buffer* _buf,
    int            _rt
);

// d_jit_ppc_emit_mtctr
//   function: mtctr rs (CTR = rs).
D_NODISCARD int d_jit_ppc_emit_mtctr(
    dj_jit_buffer* _buf,
    int            _rs
);

// ---------------------------------------------------------------------------
// pseudo-instructions
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_mr
//   function: mr ra, rs -- move register (or ra, rs, rs).
D_NODISCARD int d_jit_ppc_emit_mr(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// d_jit_ppc_emit_li
//   function: load a 32-bit immediate into rt: addi when it fits a signed
// 16-bit field, else lis+ori (sign-extended into a 64-bit reg on PPC64).
D_NODISCARD int d_jit_ppc_emit_li(
    dj_jit_buffer* _buf,
    int            _rt,
    int32_t        _imm
);

// d_jit_ppc_emit_lis
//   function: lis rt, #_ui (rt = ui << 16).
D_NODISCARD int d_jit_ppc_emit_lis(
    dj_jit_buffer* _buf,
    int            _rt,
    uint32_t       _ui
);

// d_jit_ppc_emit_nop
//   function: nop (ori 0, 0, 0 = 0x60000000).
D_NODISCARD int d_jit_ppc_emit_nop(
    dj_jit_buffer* _buf
);

// ---------------------------------------------------------------------------
// L.  PPC64-ONLY (invalid on a 32-bit target)
// ---------------------------------------------------------------------------

// d_jit_ppc_emit_mulld
//   function: mulld rt, ra, rb (low 64 of product).
D_NODISCARD int d_jit_ppc_emit_mulld(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_mulhd
//   function: mulhd rt, ra, rb (high 64, signed).
D_NODISCARD int d_jit_ppc_emit_mulhd(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_divd
//   function: divd rt, ra, rb (signed 64/64).
D_NODISCARD int d_jit_ppc_emit_divd(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_divdu
//   function: divdu rt, ra, rb (unsigned 64/64).
D_NODISCARD int d_jit_ppc_emit_divdu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_extsw
//   function: extsw ra, rs (sign-extend word to 64).
D_NODISCARD int d_jit_ppc_emit_extsw(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// d_jit_ppc_emit_cntlzd
//   function: cntlzd ra, rs (count leading zeros, 64).
D_NODISCARD int d_jit_ppc_emit_cntlzd(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs
);

// d_jit_ppc_emit_sld
//   function: sld ra, rs, rb (shift left doubleword).
D_NODISCARD int d_jit_ppc_emit_sld(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_srd
//   function: srd ra, rs, rb (shift right dword, logical).
D_NODISCARD int d_jit_ppc_emit_srd(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_srad
//   function: srad ra, rs, rb (shift right dword, arithmetic).
D_NODISCARD int d_jit_ppc_emit_srad(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    int            _rb
);

// d_jit_ppc_emit_sradi
//   function: sradi ra, rs, #_sh (arithmetic right shift, _sh 0-63).
D_NODISCARD int d_jit_ppc_emit_sradi(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh
);

// d_jit_ppc_emit_rldicl
//   function: rldicl ra, rs, sh, mb -- 64-bit rotate, clear left of mb
// (_sh/_mb 0-63). The doubleword workhorse for shifts and masks.
D_NODISCARD int d_jit_ppc_emit_rldicl(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _mb
);

// d_jit_ppc_emit_rldicr
//   function: rldicr ra, rs, sh, me -- 64-bit rotate, clear right of me.
D_NODISCARD int d_jit_ppc_emit_rldicr(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _me
);

// d_jit_ppc_emit_rldic
//   function: rldic ra, rs, sh, mb -- 64-bit rotate then clear.
D_NODISCARD int d_jit_ppc_emit_rldic(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _mb
);

// d_jit_ppc_emit_rldimi
//   function: rldimi ra, rs, sh, mb -- 64-bit rotate-insert under mask.
D_NODISCARD int d_jit_ppc_emit_rldimi(
    dj_jit_buffer* _buf,
    int            _ra,
    int            _rs,
    unsigned       _sh,
    unsigned       _mb
);

// d_jit_ppc_emit_ld
//   function: ld rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_ld(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_ldu
//   function: ldu rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_ldu(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_lwa
//   function: lwa rt, _d(ra).
D_NODISCARD int d_jit_ppc_emit_lwa(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_ldx
//   function: ldx rt, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_ldx(
    dj_jit_buffer* _buf,
    int            _rt,
    int            _ra,
    int            _rb
);

// d_jit_ppc_emit_std
//   function: std rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_std(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_stdu
//   function: stdu rs, _d(ra).
D_NODISCARD int d_jit_ppc_emit_stdu(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int32_t        _d
);

// d_jit_ppc_emit_stdx
//   function: stdx rs, ra, rb (indexed).
D_NODISCARD int d_jit_ppc_emit_stdx(
    dj_jit_buffer* _buf,
    int            _rs,
    int            _ra,
    int            _rb
);

// ---------------------------------------------------------------------------
// diagnostics
// ---------------------------------------------------------------------------

// d_jit_ppc_reg_name
//   function: the GPR name ("r0".."r31") or "?".
const char* d_jit_ppc_reg_name(
    int _reg
);

D_EXTERN_C_END


#endif  // RE_STD_HAS_INT64

#endif  // DJINTERP_C_JIT_JIT_PPC_H
