/******************************************************************************
* djinterp [jit]                                                     jit_ppc.c
*
* djinterp PowerPC (PPC32 / PPC64) JIT encoder -- implementation (jit_ppc.h).
*   Emitters assemble each instruction through the shared form encoders (X/XO,
* D, M, MD, DS) and emit one 32-bit little-endian word. The two relocations at
* the top patch branch displacements -- I-form (b/bl, +/-32 MB) and B-form (bc,
* +/-32 KB) -- both PC-relative to the branch and invoked by the label facility
* in jit.h. PowerPC has no delay slots, so nothing follows a branch implicitly.
*
* path:      /inc/djinterp/jit/jit_ppc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_ppc.h"


// ===========================================================================
// I.   RELOCATIONS + FORM ENCODERS
// ===========================================================================

// d_jit_ppc_reloc_b
//   function (internal): patch the 24-bit displacement of an I-form branch
// (b/bl) at _at so it reaches _target. PC-relative to the branch, scaled by 4.
static int d_jit_ppc_reloc_b(d_jit_buffer* _buf, size_t _at, size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    uint32_t  w;
    if (disp & 3)                            { return -1; }
    if (disp < -33554432 || disp > 33554428) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0) { return -1; }
    w = (w & ~0x03FFFFFCu) | ((uint32_t)disp & 0x03FFFFFCu);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_ppc_reloc_bc
//   function (internal): patch the 14-bit displacement of a B-form
// conditional branch (bc) at _at. PC-relative to the branch, scaled by 4.
static int d_jit_ppc_reloc_bc(d_jit_buffer* _buf, size_t _at, size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    uint32_t  w;
    if (disp & 3)                      { return -1; }
    if (disp < -32768 || disp > 32764) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0) { return -1; }
    w = (w & ~0x0000FFFCu) | ((uint32_t)disp & 0x0000FFFCu);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// form encoders: pack the fields of one instruction word
//   x   - X- and XO-form (opcode 31 + 10-bit extended opcode; fold OE into xo)
static uint32_t d_jit_ppc_x(uint32_t _op, int _a, int _b, int _c,
                            uint32_t _xo, int _rc)
{
    return ((_op & 0x3Fu) << 26)
         | (((uint32_t)_a & 0x1F) << 21)
         | (((uint32_t)_b & 0x1F) << 16)
         | (((uint32_t)_c & 0x1F) << 11)
         | ((_xo & 0x3FFu) << 1)
         | ((uint32_t)_rc & 1u);
}

//   d   - D-form (opcode, two registers, 16-bit immediate)
static uint32_t d_jit_ppc_d(uint32_t _op, int _a, int _b, int32_t _imm)
{
    return ((_op & 0x3Fu) << 26)
         | (((uint32_t)_a & 0x1F) << 21)
         | (((uint32_t)_b & 0x1F) << 16)
         | ((uint32_t)_imm & 0xFFFFu);
}

//   m   - M-form (rotate word: opcode, RS, RA, SH, MB, ME)
static uint32_t d_jit_ppc_m(uint32_t _op, int _rs, int _ra, unsigned _sh,
                            unsigned _mb, unsigned _me, int _rc)
{
    return ((_op & 0x3Fu) << 26)
         | (((uint32_t)_rs & 0x1F) << 21)
         | (((uint32_t)_ra & 0x1F) << 16)
         | ((_sh & 0x1Fu) << 11)
         | ((_mb & 0x1Fu) << 6)
         | ((_me & 0x1Fu) << 1)
         | ((uint32_t)_rc & 1u);
}

//   md  - MD-form (rotate doubleword: 6-bit SH split, 6-bit mask, 3-bit XO)
static uint32_t d_jit_ppc_md(int _rs, int _ra, unsigned _sh, unsigned _mask,
                             unsigned _xo, int _rc)
{
    uint32_t mf = (((_mask & 0x1Fu) << 1) | ((_mask >> 5) & 1u));
    return ((uint32_t)30 << 26)
         | (((uint32_t)_rs & 0x1F) << 21)
         | (((uint32_t)_ra & 0x1F) << 16)
         | ((_sh & 0x1Fu) << 11)
         | (mf << 5)
         | ((_xo & 7u) << 2)
         | (((_sh >> 5) & 1u) << 1)
         | ((uint32_t)_rc & 1u);
}

//   ds  - DS-form (ld/std family; displacement must be a multiple of 4)
static uint32_t d_jit_ppc_ds(uint32_t _op, int _rt, int _ra, int32_t _d,
                             unsigned _xo)
{
    return ((_op & 0x3Fu) << 26)
         | (((uint32_t)_rt & 0x1F) << 21)
         | (((uint32_t)_ra & 0x1F) << 16)
         | ((uint32_t)_d & 0xFFFCu)
         | (_xo & 3u);
}

static int d_jit_ppc_fits_s16(int32_t _v)
{
    return (_v >= -32768 && _v <= 32767);
}

// emit an I-form branch (b/bl) and record its relocation
static int d_jit_ppc_br(d_jit_buffer* _buf, unsigned _lk, d_jit_label* _t)
{
    size_t   at = _buf->size;
    uint32_t w  = ((uint32_t)18 << 26) | (_lk & 1u);
    if (d_jit_emit_u32(_buf, w) != 0) { return -1; }
    return d_jit_label_reference(_buf, _t, at, d_jit_ppc_reloc_b);
}

// emit a B-form conditional branch (bc) and record its relocation
static int d_jit_ppc_bcond(d_jit_buffer* _buf, unsigned _bo, unsigned _bi,
                           d_jit_label* _t)
{
    size_t   at = _buf->size;
    uint32_t w  = ((uint32_t)16 << 26) | ((_bo & 0x1Fu) << 21)
                | ((_bi & 0x1Fu) << 16);
    if (d_jit_emit_u32(_buf, w) != 0) { return -1; }
    return d_jit_label_reference(_buf, _t, at, d_jit_ppc_reloc_bc);
}


// ===========================================================================
// II.  ARITHMETIC (XO-FORM) AND IMMEDIATE (D-FORM)
// ===========================================================================

int d_jit_ppc_emit_add(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 266, 0));
}

int d_jit_ppc_emit_subf(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 40, 0));
}

int d_jit_ppc_emit_addc(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 10, 0));
}

int d_jit_ppc_emit_adde(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 138, 0));
}

int d_jit_ppc_emit_subfc(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 8, 0));
}

int d_jit_ppc_emit_subfe(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 136, 0));
}

int d_jit_ppc_emit_mullw(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 235, 0));
}

int d_jit_ppc_emit_mulhw(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 75, 0));
}

int d_jit_ppc_emit_mulhwu(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 11, 0));
}

int d_jit_ppc_emit_divw(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 491, 0));
}

int d_jit_ppc_emit_divwu(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 459, 0));
}

int d_jit_ppc_emit_neg(d_jit_buffer* _buf, int _rt, int _ra)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, 0, 104, 0));
}

int d_jit_ppc_emit_addi(d_jit_buffer* _buf, int _rt, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(14, _rt, _ra, _si));
}

int d_jit_ppc_emit_addis(d_jit_buffer* _buf, int _rt, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(15, _rt, _ra, _si));
}

int d_jit_ppc_emit_addic(d_jit_buffer* _buf, int _rt, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(12, _rt, _ra, _si));
}

int d_jit_ppc_emit_subfic(d_jit_buffer* _buf, int _rt, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(8, _rt, _ra, _si));
}

int d_jit_ppc_emit_mulli(d_jit_buffer* _buf, int _rt, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(7, _rt, _ra, _si));
}


// ===========================================================================
// III. LOGICAL
// ===========================================================================

int d_jit_ppc_emit_and(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 28, 0));
}

int d_jit_ppc_emit_or(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 444, 0));
}

int d_jit_ppc_emit_xor(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 316, 0));
}

int d_jit_ppc_emit_nand(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 476, 0));
}

int d_jit_ppc_emit_nor(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 124, 0));
}

int d_jit_ppc_emit_andc(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 60, 0));
}

int d_jit_ppc_emit_orc(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 412, 0));
}

int d_jit_ppc_emit_eqv(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 284, 0));
}

int d_jit_ppc_emit_andi_(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(28, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_andis_(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(29, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_ori(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(24, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_oris(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(25, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_xori(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(26, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_xoris(d_jit_buffer* _buf, int _ra, int _rs, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(27, _rs, _ra, (int32_t)(_ui & 0xFFFFu)));
}


// ===========================================================================
// IV.  SIGN-EXTEND / CLZ / SHIFTS
// ===========================================================================

int d_jit_ppc_emit_extsb(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, 0, 954, 0));
}

int d_jit_ppc_emit_extsh(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, 0, 922, 0));
}

int d_jit_ppc_emit_cntlzw(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, 0, 26, 0));
}

int d_jit_ppc_emit_slw(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 24, 0));
}

int d_jit_ppc_emit_srw(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 536, 0));
}

int d_jit_ppc_emit_sraw(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 792, 0));
}

int d_jit_ppc_emit_srawi(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh)
{
    if (_sh > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_ppc_x(31, _rs, _ra, (int)_sh, 824, 0));
}


// ===========================================================================
// V.   ROTATE-AND-MASK (M-FORM)
// ===========================================================================

int d_jit_ppc_emit_rlwinm(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                          unsigned _mb, unsigned _me)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_m(21, _rs, _ra, _sh, _mb, _me, 0));
}

int d_jit_ppc_emit_rlwimi(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                          unsigned _mb, unsigned _me)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_m(20, _rs, _ra, _sh, _mb, _me, 0));
}

int d_jit_ppc_emit_rlwnm(d_jit_buffer* _buf, int _ra, int _rs, int _rb,
                         unsigned _mb, unsigned _me)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_m(23, _rs, _ra, (unsigned)_rb, _mb, _me, 0));
}


// ===========================================================================
// VI.  LOADS / STORES
// ===========================================================================

int d_jit_ppc_emit_lbz(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(34, _rt, _ra, _d));
}

int d_jit_ppc_emit_lhz(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(40, _rt, _ra, _d));
}

int d_jit_ppc_emit_lha(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(42, _rt, _ra, _d));
}

int d_jit_ppc_emit_lwz(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(32, _rt, _ra, _d));
}

int d_jit_ppc_emit_lbzu(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(35, _rt, _ra, _d));
}

int d_jit_ppc_emit_lwzu(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(33, _rt, _ra, _d));
}

int d_jit_ppc_emit_lbzx(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 87, 0));
}

int d_jit_ppc_emit_lhzx(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 279, 0));
}

int d_jit_ppc_emit_lwzx(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 23, 0));
}

int d_jit_ppc_emit_stb(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(38, _rs, _ra, _d));
}

int d_jit_ppc_emit_sth(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(44, _rs, _ra, _d));
}

int d_jit_ppc_emit_stw(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(36, _rs, _ra, _d));
}

int d_jit_ppc_emit_stwu(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_d(37, _rs, _ra, _d));
}

int d_jit_ppc_emit_stbx(d_jit_buffer* _buf, int _rs, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 215, 0));
}

int d_jit_ppc_emit_sthx(d_jit_buffer* _buf, int _rs, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 407, 0));
}

int d_jit_ppc_emit_stwx(d_jit_buffer* _buf, int _rs, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 151, 0));
}


// ===========================================================================
// VII. COMPARES
// ===========================================================================

int d_jit_ppc_emit_cmpw(d_jit_buffer* _buf, int _crf, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_x(31, (_crf & 7) << 2, _ra, _rb, 0, 0));
}

int d_jit_ppc_emit_cmpwi(d_jit_buffer* _buf, int _crf, int _ra, int32_t _si)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(11, (_crf & 7) << 2, _ra, _si));
}

int d_jit_ppc_emit_cmplw(d_jit_buffer* _buf, int _crf, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_x(31, (_crf & 7) << 2, _ra, _rb, 32, 0));
}

int d_jit_ppc_emit_cmplwi(d_jit_buffer* _buf, int _crf, int _ra, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(10, (_crf & 7) << 2, _ra, (int32_t)(_ui & 0xFFFFu)));
}


// ===========================================================================
// VIII. BRANCHES
// ===========================================================================

int d_jit_ppc_emit_b(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_ppc_br(_buf, 0, _target);
}

int d_jit_ppc_emit_bl(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_ppc_br(_buf, 1, _target);
}

int d_jit_ppc_emit_bc(d_jit_buffer* _buf, unsigned _bo, unsigned _bi,
                      d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, _bo, _bi, _target);
}

int d_jit_ppc_emit_beq(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 12, (unsigned)((_crf & 7) * 4 + 2),
                           _target);
}

int d_jit_ppc_emit_bne(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 4, (unsigned)((_crf & 7) * 4 + 2),
                           _target);
}

int d_jit_ppc_emit_blt(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 12, (unsigned)((_crf & 7) * 4 + 0),
                           _target);
}

int d_jit_ppc_emit_bge(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 4, (unsigned)((_crf & 7) * 4 + 0),
                           _target);
}

int d_jit_ppc_emit_bgt(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 12, (unsigned)((_crf & 7) * 4 + 1),
                           _target);
}

int d_jit_ppc_emit_ble(d_jit_buffer* _buf, int _crf, d_jit_label* _target)
{
    return d_jit_ppc_bcond(_buf, 4, (unsigned)((_crf & 7) * 4 + 1),
                           _target);
}

int d_jit_ppc_emit_blr(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, 0x4E800020u);
}

int d_jit_ppc_emit_bctr(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, 0x4E800420u);
}

int d_jit_ppc_emit_bctrl(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, 0x4E800421u);
}


// ===========================================================================
// IX.  LINK / COUNT REGISTER MOVES
// ===========================================================================

int d_jit_ppc_emit_mflr(d_jit_buffer* _buf, int _rt)
{
    return d_jit_emit_u32(_buf, 0x7C0802A6u | (((uint32_t)_rt & 0x1F) << 21));
}

int d_jit_ppc_emit_mtlr(d_jit_buffer* _buf, int _rs)
{
    return d_jit_emit_u32(_buf, 0x7C0803A6u | (((uint32_t)_rs & 0x1F) << 21));
}

int d_jit_ppc_emit_mfctr(d_jit_buffer* _buf, int _rt)
{
    return d_jit_emit_u32(_buf, 0x7C0902A6u | (((uint32_t)_rt & 0x1F) << 21));
}

int d_jit_ppc_emit_mtctr(d_jit_buffer* _buf, int _rs)
{
    return d_jit_emit_u32(_buf, 0x7C0903A6u | (((uint32_t)_rs & 0x1F) << 21));
}


// ===========================================================================
// X.   PSEUDO-INSTRUCTIONS
// ===========================================================================

int d_jit_ppc_emit_mr(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rs, 444, 0));
}

int d_jit_ppc_emit_li(d_jit_buffer* _buf, int _rt, int32_t _imm)
{
    if (d_jit_ppc_fits_s16(_imm)) {
        return d_jit_emit_u32(_buf, d_jit_ppc_d(14, _rt, 0, _imm));
    }
    {
        uint32_t u  = (uint32_t)_imm;
        int32_t  hi = (int32_t)((u >> 16) & 0xFFFFu);
        int32_t  lo = (int32_t)(u & 0xFFFFu);
        if (d_jit_emit_u32(_buf, d_jit_ppc_d(15, _rt, 0, hi)) != 0) {
            return -1;
        }
        if (lo == 0) { return 0; }
        return d_jit_emit_u32(_buf, d_jit_ppc_d(24, _rt, _rt, lo));
    }
}

int d_jit_ppc_emit_lis(d_jit_buffer* _buf, int _rt, uint32_t _ui)
{
    return d_jit_emit_u32(_buf,
        d_jit_ppc_d(15, _rt, 0, (int32_t)(_ui & 0xFFFFu)));
}

int d_jit_ppc_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, 0x60000000u);   /* ori 0, 0, 0 */
}


// ===========================================================================
// XI.  PPC64-ONLY (INVALID ON A 32-BIT TARGET)
// ===========================================================================

int d_jit_ppc_emit_mulld(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 233, 0));
}

int d_jit_ppc_emit_mulhd(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 73, 0));
}

int d_jit_ppc_emit_divd(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 489, 0));
}

int d_jit_ppc_emit_divdu(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 457, 0));
}

int d_jit_ppc_emit_extsw(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, 0, 986, 0));
}

int d_jit_ppc_emit_cntlzd(d_jit_buffer* _buf, int _ra, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, 0, 58, 0));
}

int d_jit_ppc_emit_sld(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 27, 0));
}

int d_jit_ppc_emit_srd(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 539, 0));
}

int d_jit_ppc_emit_srad(d_jit_buffer* _buf, int _ra, int _rs, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 794, 0));
}

int d_jit_ppc_emit_sradi(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh)
{
    if (_sh > 63) { return -1; }
    return d_jit_emit_u32(_buf, ((uint32_t)31 << 26)
        | (((uint32_t)_rs & 0x1F) << 21)
        | (((uint32_t)_ra & 0x1F) << 16)
        | ((_sh & 0x1Fu) << 11)
        | ((uint32_t)413 << 2)
        | (((_sh >> 5) & 1u) << 1));
}

int d_jit_ppc_emit_rldicl(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                          unsigned _mx)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_md(_rs, _ra, _sh, _mx, 0, 0));
}

int d_jit_ppc_emit_rldicr(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                          unsigned _mx)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_md(_rs, _ra, _sh, _mx, 1, 0));
}

int d_jit_ppc_emit_rldic(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                         unsigned _mx)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_md(_rs, _ra, _sh, _mx, 2, 0));
}

int d_jit_ppc_emit_rldimi(d_jit_buffer* _buf, int _ra, int _rs, unsigned _sh,
                          unsigned _mx)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_md(_rs, _ra, _sh, _mx, 3, 0));
}

int d_jit_ppc_emit_ld(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    if ((_d & 3) != 0 || !d_jit_ppc_fits_s16(_d)) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_ppc_ds(58, _rt, _ra, _d, 0));
}

int d_jit_ppc_emit_ldu(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    if ((_d & 3) != 0 || !d_jit_ppc_fits_s16(_d)) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_ppc_ds(58, _rt, _ra, _d, 1));
}

int d_jit_ppc_emit_lwa(d_jit_buffer* _buf, int _rt, int _ra, int32_t _d)
{
    if ((_d & 3) != 0 || !d_jit_ppc_fits_s16(_d)) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_ppc_ds(58, _rt, _ra, _d, 2));
}

int d_jit_ppc_emit_ldx(d_jit_buffer* _buf, int _rt, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rt, _ra, _rb, 21, 0));
}

int d_jit_ppc_emit_std(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    if ((_d & 3) != 0 || !d_jit_ppc_fits_s16(_d)) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_ppc_ds(62, _rs, _ra, _d, 0));
}

int d_jit_ppc_emit_stdu(d_jit_buffer* _buf, int _rs, int _ra, int32_t _d)
{
    if ((_d & 3) != 0 || !d_jit_ppc_fits_s16(_d)) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_ppc_ds(62, _rs, _ra, _d, 1));
}

int d_jit_ppc_emit_stdx(d_jit_buffer* _buf, int _rs, int _ra, int _rb)
{
    return d_jit_emit_u32(_buf, d_jit_ppc_x(31, _rs, _ra, _rb, 149, 0));
}


// ===========================================================================
// XII. DIAGNOSTICS
// ===========================================================================

const char* d_jit_ppc_reg_name(int _reg)
{
    static const char* const names[32] = {
        "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",
        "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
        "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31"
    };
    return (_reg >= 0 && _reg < 32) ? names[_reg] : "?";
}
