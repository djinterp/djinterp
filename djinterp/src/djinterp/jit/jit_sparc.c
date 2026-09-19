/******************************************************************************
* djinterp [jit]                                                   jit_sparc.c
*
* djinterp SPARC (V8 / V9) JIT encoder -- implementation (jit_sparc.h).
*   SPARC instructions are big-endian, so d_jit_sparc_emit_word writes the word
* most-significant-byte first via the byte primitive (correct on any host),
* rather than the buffer's little-endian u32 helper. The relocations patch
* branch displacements (Bicc disp22, BPcc disp19, call disp30), each relative
* to the branch and scaled by 4, with big-endian read-modify-write. Control
* transfers have delay slots, which the caller fills.
*
* path:      /inc/djinterp/jit/jit_sparc.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_sparc.h"


// ===========================================================================
// I.   BIG-ENDIAN EMIT + RELOCATIONS + FORM ENCODERS
// ===========================================================================

// d_jit_sparc_emit_word
//   function (internal): append one instruction word big-endian (MSB first).
static int d_jit_sparc_emit_word(d_jit_buffer* _buf, uint32_t _w)
{
    if (d_jit_emit_u8(_buf, (unsigned char)(_w >> 24)) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, (unsigned char)(_w >> 16)) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, (unsigned char)(_w >> 8))  != 0) { return -1; }
    return d_jit_emit_u8(_buf, (unsigned char)_w);
}

// big-endian read-modify-write of the instruction word at _at, replacing the
// low _maskbits of displacement; shared by the three branch relocations.
static int d_jit_sparc_patch_disp(d_jit_buffer* _buf, size_t _at,
                                  uint32_t _mask, long long _off,
                                  long long _lo, long long _hi)
{
    uint32_t w;
    if (_off < _lo || _off > _hi)   { return -1; }
    if (_at + 4 > _buf->size)       { return -1; }
    w = ((uint32_t)_buf->code[_at]     << 24)
      | ((uint32_t)_buf->code[_at + 1] << 16)
      | ((uint32_t)_buf->code[_at + 2] << 8)
      |  (uint32_t)_buf->code[_at + 3];
    w = (w & ~_mask) | ((uint32_t)_off & _mask);
    _buf->code[_at]     = (unsigned char)(w >> 24);
    _buf->code[_at + 1] = (unsigned char)(w >> 16);
    _buf->code[_at + 2] = (unsigned char)(w >> 8);
    _buf->code[_at + 3] = (unsigned char)w;
    return 0;
}

// d_jit_sparc_reloc_disp22 (Bicc, +/-8 MB).
//   PC-relative to the branch, scaled by 4.
static int d_jit_sparc_reloc_disp22(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    if (disp & 3) { return -1; }
    return d_jit_sparc_patch_disp(_buf, _at, 0x003FFFFFu, disp >> 2,
                                  -2097152, 2097151);
}

// d_jit_sparc_reloc_disp19 (BPcc, +/-1 MB).
//   PC-relative to the branch, scaled by 4.
static int d_jit_sparc_reloc_disp19(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    if (disp & 3) { return -1; }
    return d_jit_sparc_patch_disp(_buf, _at, 0x0007FFFFu, disp >> 2,
                                  -262144, 262143);
}

// d_jit_sparc_reloc_disp30 (call, +/-2 GB).
//   PC-relative to the branch, scaled by 4.
static int d_jit_sparc_reloc_disp30(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    if (disp & 3) { return -1; }
    return d_jit_sparc_patch_disp(_buf, _at, 0x3FFFFFFFu, disp >> 2,
                                  -536870912, 536870911);
}

static int d_jit_sparc_fits_simm13(int32_t _v)
{
    return (_v >= -4096 && _v <= 4095);
}

// form encoders (op is the 2-bit format selector: 2 = arith, 3 = ld/st)
static uint32_t d_jit_sparc_f3r(uint32_t _op, int _rd, uint32_t _op3,
                                int _rs1, int _rs2)
{
    return (_op << 30)
         | (((uint32_t)_rd  & 0x1F) << 25)
         | ((_op3 & 0x3Fu) << 19)
         | (((uint32_t)_rs1 & 0x1F) << 14)
         |  ((uint32_t)_rs2 & 0x1F);
}

static uint32_t d_jit_sparc_f3i(uint32_t _op, int _rd, uint32_t _op3,
                                int _rs1, int32_t _imm)
{
    return (_op << 30)
         | (((uint32_t)_rd  & 0x1F) << 25)
         | ((_op3 & 0x3Fu) << 19)
         | (((uint32_t)_rs1 & 0x1F) << 14)
         | (1u << 13)
         |  ((uint32_t)_imm & 0x1FFFu);
}

static uint32_t d_jit_sparc_shift(uint32_t _op3, int _rd, int _rs1,
                                  unsigned _x, unsigned _i, uint32_t _pay)
{
    return (2u << 30)
         | (((uint32_t)_rd  & 0x1F) << 25)
         | ((_op3 & 0x3Fu) << 19)
         | (((uint32_t)_rs1 & 0x1F) << 14)
         | ((_i & 1u) << 13)
         | ((_x & 1u) << 12)
         |  (_pay & 0x3Fu);
}

static uint32_t d_jit_sparc_sethi_w(int _rd, uint32_t _imm22)
{
    return (((uint32_t)_rd & 0x1F) << 25) | (4u << 22) | (_imm22 & 0x3FFFFFu);
}

// branch/call emit helpers: append the base word and record the reloc
static int d_jit_sparc_br(d_jit_buffer* _buf, uint32_t _word,
                          d_jit_reloc_fn _fn, d_jit_label* _t)
{
    size_t at = _buf->size;
    if (d_jit_sparc_emit_word(_buf, _word) != 0) { return -1; }
    return d_jit_label_reference(_buf, _t, at, _fn);
}


// ===========================================================================
// II.  ARITHMETIC / LOGICAL
// ===========================================================================

int d_jit_sparc_emit_add(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 0, _rs1, _rs2));
}

int d_jit_sparc_emit_addcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 16, _rs1, _rs2));
}

int d_jit_sparc_emit_addx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 8, _rs1, _rs2));
}

int d_jit_sparc_emit_addxcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 24, _rs1, _rs2));
}

int d_jit_sparc_emit_sub(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 4, _rs1, _rs2));
}

int d_jit_sparc_emit_subcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 20, _rs1, _rs2));
}

int d_jit_sparc_emit_subx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 12, _rs1, _rs2));
}

int d_jit_sparc_emit_subxcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 28, _rs1, _rs2));
}

int d_jit_sparc_emit_and(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 1, _rs1, _rs2));
}

int d_jit_sparc_emit_andcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 17, _rs1, _rs2));
}

int d_jit_sparc_emit_andn(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 5, _rs1, _rs2));
}

int d_jit_sparc_emit_andncc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 21, _rs1, _rs2));
}

int d_jit_sparc_emit_or(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 2, _rs1, _rs2));
}

int d_jit_sparc_emit_orcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 18, _rs1, _rs2));
}

int d_jit_sparc_emit_orn(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 6, _rs1, _rs2));
}

int d_jit_sparc_emit_orncc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 22, _rs1, _rs2));
}

int d_jit_sparc_emit_xor(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 3, _rs1, _rs2));
}

int d_jit_sparc_emit_xorcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 19, _rs1, _rs2));
}

int d_jit_sparc_emit_xnor(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 7, _rs1, _rs2));
}

int d_jit_sparc_emit_xnorcc(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 23, _rs1, _rs2));
}

int d_jit_sparc_emit_addi(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 0, _rs1, _imm));
}

int d_jit_sparc_emit_subi(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 4, _rs1, _imm));
}

int d_jit_sparc_emit_andi(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 1, _rs1, _imm));
}

int d_jit_sparc_emit_ori(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 2, _rs1, _imm));
}

int d_jit_sparc_emit_xori(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 3, _rs1, _imm));
}

int d_jit_sparc_emit_addcci(d_jit_buffer* _buf, int _rd, int _rs1,
                            int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 16, _rs1, _imm));
}

int d_jit_sparc_emit_subcci(d_jit_buffer* _buf, int _rd, int _rs1,
                            int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 20, _rs1, _imm));
}


// ===========================================================================
// III. SHIFTS (32-BIT)
// ===========================================================================

int d_jit_sparc_emit_sll(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(37, _rd, _rs1, 0, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_slli(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shcnt)
{
    if (_shcnt > 31) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(37, _rd, _rs1, 0, 1, _shcnt));
}

int d_jit_sparc_emit_srl(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(38, _rd, _rs1, 0, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_srli(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shcnt)
{
    if (_shcnt > 31) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(38, _rd, _rs1, 0, 1, _shcnt));
}

int d_jit_sparc_emit_sra(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(39, _rd, _rs1, 0, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_srai(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shcnt)
{
    if (_shcnt > 31) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(39, _rd, _rs1, 0, 1, _shcnt));
}


// ===========================================================================
// IV.  MULTIPLY / DIVIDE (V8, VIA %y)
// ===========================================================================

int d_jit_sparc_emit_umul(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 10, _rs1, _rs2));
}

int d_jit_sparc_emit_smul(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 11, _rs1, _rs2));
}

int d_jit_sparc_emit_udiv(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 14, _rs1, _rs2));
}

int d_jit_sparc_emit_sdiv(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 15, _rs1, _rs2));
}

int d_jit_sparc_emit_rd_y(d_jit_buffer* _buf, int _rd)
{
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_f3r(2, _rd, 0x28, 0, 0));
}

int d_jit_sparc_emit_wr_yi(d_jit_buffer* _buf, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, 0, 0x30, _rs1, _imm));
}


// ===========================================================================
// V.   SETHI / SET
// ===========================================================================

int d_jit_sparc_emit_sethi(d_jit_buffer* _buf, int _rd, uint32_t _imm22)
{
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_sethi_w(_rd, _imm22));
}

int d_jit_sparc_emit_set(d_jit_buffer* _buf, int _rd, int32_t _imm)
{
    if (d_jit_sparc_fits_simm13(_imm)) {
        return d_jit_sparc_emit_word(_buf,
            d_jit_sparc_f3i(2, _rd, 0x02, 0, _imm));   /* or %g0, imm, rd */
    }
    {
        uint32_t u    = (uint32_t)_imm;
        uint32_t hi22 = (u >> 10) & 0x3FFFFFu;
        uint32_t lo10 = u & 0x3FFu;
        if (d_jit_sparc_emit_word(_buf, d_jit_sparc_sethi_w(_rd, hi22)) != 0) {
            return -1;
        }
        if (lo10 == 0u) { return 0; }
        return d_jit_sparc_emit_word(_buf,
            d_jit_sparc_f3i(2, _rd, 0x02, _rd, (int32_t)lo10));  /* or rd,lo */
    }
}


// ===========================================================================
// VI.  LOADS / STORES (IMMEDIATE ADDRESSING)
// ===========================================================================

int d_jit_sparc_emit_ldub(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 1, _rs1, _imm));
}

int d_jit_sparc_emit_ldsb(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 9, _rs1, _imm));
}

int d_jit_sparc_emit_lduh(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 2, _rs1, _imm));
}

int d_jit_sparc_emit_ldsh(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 10, _rs1, _imm));
}

int d_jit_sparc_emit_ld(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 0, _rs1, _imm));
}

int d_jit_sparc_emit_stb(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 5, _rs1, _imm));
}

int d_jit_sparc_emit_sth(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 6, _rs1, _imm));
}

int d_jit_sparc_emit_st(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 4, _rs1, _imm));
}


// ===========================================================================
// VII. CONTROL TRANSFER
// ===========================================================================

int d_jit_sparc_emit_bicc(d_jit_buffer* _buf, unsigned _cond,
                          d_jit_label* _target)
{
    uint32_t w = ((_cond & 0xFu) << 25) | (2u << 22);   /* op=0, op2=Bicc */
    return d_jit_sparc_br(_buf, w, d_jit_sparc_reloc_disp22, _target);
}

int d_jit_sparc_emit_ba(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_A, _target);
}

int d_jit_sparc_emit_be(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_E, _target);
}

int d_jit_sparc_emit_bne(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_NE, _target);
}

int d_jit_sparc_emit_bl(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_L, _target);
}

int d_jit_sparc_emit_ble(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_LE, _target);
}

int d_jit_sparc_emit_bg(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_G, _target);
}

int d_jit_sparc_emit_bge(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_GE, _target);
}

int d_jit_sparc_emit_bgu(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_GU, _target);
}

int d_jit_sparc_emit_bleu(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_LEU, _target);
}

int d_jit_sparc_emit_bgeu(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_GEU, _target);
}

int d_jit_sparc_emit_blu(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_emit_bicc(_buf, D_JIT_SPARC_COND_LU, _target);
}

int d_jit_sparc_emit_call(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_sparc_br(_buf, (1u << 30),
                          d_jit_sparc_reloc_disp30, _target);
}

int d_jit_sparc_emit_jmpl(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 0x38, _rs1, _imm));
}

int d_jit_sparc_emit_ret(d_jit_buffer* _buf)
{
    /* jmpl %i7 + 8, %g0 */
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_f3i(2, 0, 0x38, 31, 8));
}

int d_jit_sparc_emit_retl(d_jit_buffer* _buf)
{
    /* jmpl %o7 + 8, %g0 */
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_f3i(2, 0, 0x38, 15, 8));
}


// ===========================================================================
// VIII. REGISTER-WINDOW AND PSEUDO-INSTRUCTIONS
// ===========================================================================

int d_jit_sparc_emit_save_i(d_jit_buffer* _buf, int _rd, int _rs1,
                            int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, _rd, 0x3C, _rs1, _imm));
}

int d_jit_sparc_emit_restore(d_jit_buffer* _buf)
{
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_f3r(2, 0, 0x3D, 0, 0));
}

int d_jit_sparc_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_sparc_emit_word(_buf, 0x01000000u);   /* sethi 0, %g0 */
}

int d_jit_sparc_emit_mov(d_jit_buffer* _buf, int _rd, int _rs)
{
    /* or %g0, rs, rd */
    return d_jit_sparc_emit_word(_buf, d_jit_sparc_f3r(2, _rd, 0x02, 0, _rs));
}

int d_jit_sparc_emit_cmp(d_jit_buffer* _buf, int _rs1, int _rs2)
{
    /* subcc rs1, rs2, %g0 */
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, 0, 0x14, _rs1, _rs2));
}

int d_jit_sparc_emit_cmpi(d_jit_buffer* _buf, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(2, 0, 0x14, _rs1, _imm));
}


// ===========================================================================
// L.   V9-ONLY (INVALID ON A PURE V8 TARGET)
// ===========================================================================

int d_jit_sparc_emit_ldsw(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 8, _rs1, _imm));
}

int d_jit_sparc_emit_ldx(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 11, _rs1, _imm));
}

int d_jit_sparc_emit_stx(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _imm)
{
    if (!d_jit_sparc_fits_simm13(_imm)) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3i(3, _rd, 14, _rs1, _imm));
}

int d_jit_sparc_emit_sllx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(37, _rd, _rs1, 1, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_sllxi(d_jit_buffer* _buf, int _rd, int _rs1,
                           unsigned _shcnt)
{
    if (_shcnt > 63) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(37, _rd, _rs1, 1, 1, _shcnt));
}

int d_jit_sparc_emit_srlx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(38, _rd, _rs1, 1, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_srlxi(d_jit_buffer* _buf, int _rd, int _rs1,
                           unsigned _shcnt)
{
    if (_shcnt > 63) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(38, _rd, _rs1, 1, 1, _shcnt));
}

int d_jit_sparc_emit_srax(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(39, _rd, _rs1, 1, 0, (uint32_t)_rs2));
}

int d_jit_sparc_emit_sraxi(d_jit_buffer* _buf, int _rd, int _rs1,
                           unsigned _shcnt)
{
    if (_shcnt > 63) { return -1; }
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_shift(39, _rd, _rs1, 1, 1, _shcnt));
}

int d_jit_sparc_emit_mulx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 9, _rs1, _rs2));
}

int d_jit_sparc_emit_udivx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 13, _rs1, _rs2));
}

int d_jit_sparc_emit_sdivx(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_sparc_emit_word(_buf,
        d_jit_sparc_f3r(2, _rd, 45, _rs1, _rs2));
}

int d_jit_sparc_emit_bpcc(d_jit_buffer* _buf, unsigned _cond, int _cc,
                          d_jit_label* _target)
{
    /* op=0, op2=BPcc(1), cc in [21:20], predict-taken (p=1) */
    uint32_t w = ((_cond & 0xFu) << 25) | (1u << 22)
               | (((uint32_t)_cc & 3u) << 20) | (1u << 19);
    return d_jit_sparc_br(_buf, w, d_jit_sparc_reloc_disp19, _target);
}


// ===========================================================================
// IX.  DIAGNOSTICS
// ===========================================================================

const char* d_jit_sparc_reg_name(int _reg)
{
    static const char* const names[32] = {
        "%g0", "%g1", "%g2", "%g3", "%g4", "%g5", "%g6", "%g7",
        "%o0", "%o1", "%o2", "%o3", "%o4", "%o5", "%o6", "%o7",
        "%l0", "%l1", "%l2", "%l3", "%l4", "%l5", "%l6", "%l7",
        "%i0", "%i1", "%i2", "%i3", "%i4", "%i5", "%i6", "%i7"
    };
    return (_reg >= 0 && _reg < 32) ? names[_reg] : "?";
}
