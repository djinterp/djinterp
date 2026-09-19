/******************************************************************************
* djinterp [jit]                                                   jit_s390x.c
*
* djinterp s390x (IBM z/Architecture) JIT encoder -- implementation.
*   Instructions are variable length (2/4/6 bytes) and big-endian, so each form
* encoder fills a byte array most-significant-byte first and appends it whole.
* The eight helpers cover the formats used here: RR, RRE, RI, RIL, RX, RXY, RS,
* and RSY. The RXY/RSY 20-bit displacement is split into a 12-bit low part and
* an 8-bit high part. The relocations patch a branch's halfword displacement
* -- ri16 for the RI relative branches, ril32 for the RIL ones -- a signed
* count of halfwords from the branch, with a big-endian read-modify-write.
*
* path:      /inc/djinterp/jit/jit_s390x.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_s390x.h"


// ===========================================================================
// I.   BIG-ENDIAN VARIABLE-LENGTH EMIT + RELOCATIONS + FORM ENCODERS
// ===========================================================================

static int d_jit_s390x_rr(d_jit_buffer* _buf, unsigned _op8, int _r1, int _r2)
{
    unsigned char b[2];
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_r2 & 0xF));
    return d_jit_emit(_buf, b, 2);
}

static int d_jit_s390x_rre(d_jit_buffer* _buf, unsigned _op16, int _r1,
                           int _r2)
{
    unsigned char b[4];
    b[0] = (unsigned char)(_op16 >> 8);
    b[1] = (unsigned char)(_op16 & 0xFF);
    b[2] = 0x00;
    b[3] = (unsigned char)(((_r1 & 0xF) << 4) | (_r2 & 0xF));
    return d_jit_emit(_buf, b, 4);
}

static int d_jit_s390x_ri(d_jit_buffer* _buf, unsigned _op8, int _r1,
                          unsigned _op4, uint16_t _imm)
{
    unsigned char b[4];
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_op4 & 0xF));
    b[2] = (unsigned char)((_imm >> 8) & 0xFF);
    b[3] = (unsigned char)(_imm & 0xFF);
    return d_jit_emit(_buf, b, 4);
}

static int d_jit_s390x_ril(d_jit_buffer* _buf, unsigned _op8, int _r1,
                           unsigned _op4, uint32_t _imm)
{
    unsigned char b[6];
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_op4 & 0xF));
    b[2] = (unsigned char)((_imm >> 24) & 0xFF);
    b[3] = (unsigned char)((_imm >> 16) & 0xFF);
    b[4] = (unsigned char)((_imm >> 8) & 0xFF);
    b[5] = (unsigned char)(_imm & 0xFF);
    return d_jit_emit(_buf, b, 6);
}

static int d_jit_s390x_rx(d_jit_buffer* _buf, unsigned _op8, int _r1,
                          uint32_t _d, int _x, int _b)
{
    unsigned char by[4];
    by[0] = (unsigned char)_op8;
    by[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_x & 0xF));
    by[2] = (unsigned char)(((_b & 0xF) << 4) | ((_d >> 8) & 0xF));
    by[3] = (unsigned char)(_d & 0xFF);
    return d_jit_emit(_buf, by, 4);
}

// RXY/RSY share the 20-bit displacement split (12-bit low, 8-bit high).
static int d_jit_s390x_rxy(d_jit_buffer* _buf, unsigned _op8, int _r1,
                           int32_t _d, int _x, int _b, unsigned _op2)
{
    unsigned char by[6];
    uint32_t d20 = (uint32_t)_d & 0xFFFFFu;
    uint32_t dl  = d20 & 0xFFFu;
    uint32_t dh  = (d20 >> 12) & 0xFFu;
    by[0] = (unsigned char)_op8;
    by[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_x & 0xF));
    by[2] = (unsigned char)(((_b & 0xF) << 4) | ((dl >> 8) & 0xF));
    by[3] = (unsigned char)(dl & 0xFF);
    by[4] = (unsigned char)dh;
    by[5] = (unsigned char)_op2;
    return d_jit_emit(_buf, by, 6);
}

static int d_jit_s390x_rs(d_jit_buffer* _buf, unsigned _op8, int _r1,
                          int _r3, uint32_t _d, int _b)
{
    unsigned char b[4];
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_r3 & 0xF));
    b[2] = (unsigned char)(((_b & 0xF) << 4) | ((_d >> 8) & 0xF));
    b[3] = (unsigned char)(_d & 0xFF);
    return d_jit_emit(_buf, b, 4);
}

static int d_jit_s390x_rsy(d_jit_buffer* _buf, unsigned _op8, int _r1,
                           int _r3, int32_t _d, int _b, unsigned _op2)
{
    unsigned char by[6];
    uint32_t d20 = (uint32_t)_d & 0xFFFFFu;
    uint32_t dl  = d20 & 0xFFFu;
    uint32_t dh  = (d20 >> 12) & 0xFFu;
    by[0] = (unsigned char)_op8;
    by[1] = (unsigned char)(((_r1 & 0xF) << 4) | (_r3 & 0xF));
    by[2] = (unsigned char)(((_b & 0xF) << 4) | ((dl >> 8) & 0xF));
    by[3] = (unsigned char)(dl & 0xFF);
    by[4] = (unsigned char)dh;
    by[5] = (unsigned char)_op2;
    return d_jit_emit(_buf, by, 6);
}

// relative branch relocations: displacement is a signed halfword count from
// the branch instruction, patched big-endian into the immediate field.
static int d_jit_s390x_reloc_ri16(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long off = (long long)_target - (long long)_at;
    long long h;
    if (off & 1)              { return -1; }
    h = off / 2;
    if (h < -32768 || h > 32767) { return -1; }
    if (_at + 4 > _buf->size)  { return -1; }
    _buf->code[_at + 2] = (unsigned char)((h >> 8) & 0xFF);
    _buf->code[_at + 3] = (unsigned char)(h & 0xFF);
    return 0;
}

static int d_jit_s390x_reloc_ril32(d_jit_buffer* _buf, size_t _at,
                                   size_t _target)
{
    long long off = (long long)_target - (long long)_at;
    long long h;
    if (off & 1)              { return -1; }
    h = off / 2;
    if (h < -2147483647LL - 1 || h > 2147483647LL) { return -1; }
    if (_at + 6 > _buf->size)  { return -1; }
    _buf->code[_at + 2] = (unsigned char)((h >> 24) & 0xFF);
    _buf->code[_at + 3] = (unsigned char)((h >> 16) & 0xFF);
    _buf->code[_at + 4] = (unsigned char)((h >> 8) & 0xFF);
    _buf->code[_at + 5] = (unsigned char)(h & 0xFF);
    return 0;
}

// RI/RIL relative-branch emit: append the base form, then record the reloc.
static int d_jit_s390x_bri(d_jit_buffer* _buf, unsigned _op8, int _field,
                           unsigned _op4, d_jit_label* _t)
{
    unsigned char b[4];
    size_t at = _buf->size;
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_field & 0xF) << 4) | (_op4 & 0xF));
    b[2] = 0; b[3] = 0;
    if (d_jit_emit(_buf, b, 4) != 0) { return -1; }
    return d_jit_label_reference(_buf, _t, at, d_jit_s390x_reloc_ri16);
}

static int d_jit_s390x_bril(d_jit_buffer* _buf, unsigned _op8, int _field,
                            unsigned _op4, d_jit_label* _t)
{
    unsigned char b[6];
    size_t at = _buf->size;
    b[0] = (unsigned char)_op8;
    b[1] = (unsigned char)(((_field & 0xF) << 4) | (_op4 & 0xF));
    b[2] = 0; b[3] = 0; b[4] = 0; b[5] = 0;
    if (d_jit_emit(_buf, b, 6) != 0) { return -1; }
    return d_jit_label_reference(_buf, _t, at, d_jit_s390x_reloc_ril32);
}

static int d_jit_s390x_fits_s16(int32_t _v)
{ return (_v >= -32768 && _v <= 32767); }


// ===========================================================================
// II.  REGISTER MOVES / LOAD-AND-TEST / EXTEND
// ===========================================================================

int d_jit_s390x_emit_lr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x18, _r1, _r2);
}

int d_jit_s390x_emit_ltr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x12, _r1, _r2);
}

int d_jit_s390x_emit_lcr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x13, _r1, _r2);
}

int d_jit_s390x_emit_lgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB904, _r1, _r2);
}

int d_jit_s390x_emit_lgfr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB914, _r1, _r2);
}

int d_jit_s390x_emit_llgfr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB916, _r1, _r2);
}

int d_jit_s390x_emit_ltgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB902, _r1, _r2);
}

int d_jit_s390x_emit_lcgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB903, _r1, _r2);
}


// ===========================================================================
// III. LOAD IMMEDIATE / ADDRESS
// ===========================================================================

int d_jit_s390x_emit_lhi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0x8, (uint16_t)_imm);
}

int d_jit_s390x_emit_lghi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0x9, (uint16_t)_imm);
}

int d_jit_s390x_emit_lgfi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0x1, (uint32_t)_imm);
}

int d_jit_s390x_emit_load_imm(d_jit_buffer* _buf, int _r1, int64_t _imm)
{
    if (_imm >= -32768 && _imm <= 32767) {
        return d_jit_s390x_emit_lghi(_buf, _r1, (int32_t)_imm);
    }
    if (_imm >= -2147483647LL - 1 && _imm <= 2147483647LL) {
        return d_jit_s390x_emit_lgfi(_buf, _r1, (int32_t)_imm);
    }
    if (d_jit_s390x_emit_iihf(_buf, _r1,
            (uint32_t)((uint64_t)_imm >> 32)) != 0) { return -1; }
    return d_jit_s390x_emit_iilf(_buf, _r1,
            (uint32_t)((uint64_t)_imm & 0xFFFFFFFFu));
}

int d_jit_s390x_emit_larl(d_jit_buffer* _buf, int _r1, d_jit_label* _target)
{
    return d_jit_s390x_bril(_buf, 0xC0, _r1, 0x0, _target);
}


// ===========================================================================
// IV.  ARITHMETIC (r1 = r1 op r2; immediate forms)
// ===========================================================================

int d_jit_s390x_emit_ar(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x1A, _r1, _r2);
}

int d_jit_s390x_emit_sr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x1B, _r1, _r2);
}

int d_jit_s390x_emit_agr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB908, _r1, _r2);
}

int d_jit_s390x_emit_sgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB909, _r1, _r2);
}

int d_jit_s390x_emit_msr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB252, _r1, _r2);
}

int d_jit_s390x_emit_msgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB90C, _r1, _r2);
}

int d_jit_s390x_emit_msgfr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB91C, _r1, _r2);
}

int d_jit_s390x_emit_ahi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xA, (uint16_t)_imm);
}

int d_jit_s390x_emit_aghi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xB, (uint16_t)_imm);
}

int d_jit_s390x_emit_mhi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xC, (uint16_t)_imm);
}

int d_jit_s390x_emit_mghi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xD, (uint16_t)_imm);
}

int d_jit_s390x_emit_afi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC2, _r1, 0x9, (uint32_t)_imm);
}

int d_jit_s390x_emit_agfi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC2, _r1, 0x8, (uint32_t)_imm);
}


// ===========================================================================
// V.   DIVIDE (r1:r1+1 even/odd pair)
// ===========================================================================

int d_jit_s390x_emit_dsgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB90D, _r1, _r2);
}

int d_jit_s390x_emit_dsgfr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB91D, _r1, _r2);
}

int d_jit_s390x_emit_dlgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB987, _r1, _r2);
}


// ===========================================================================
// VI.  LOGICAL (r1 = r1 op r2; immediate forms act on 32-bit halves)
// ===========================================================================

int d_jit_s390x_emit_nr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x14, _r1, _r2);
}

int d_jit_s390x_emit_or(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x16, _r1, _r2);
}

int d_jit_s390x_emit_xr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x17, _r1, _r2);
}

int d_jit_s390x_emit_ngr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB980, _r1, _r2);
}

int d_jit_s390x_emit_ogr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB981, _r1, _r2);
}

int d_jit_s390x_emit_xgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB982, _r1, _r2);
}

int d_jit_s390x_emit_nilf(d_jit_buffer* _buf, int _r1, uint32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0xB, _imm);
}

int d_jit_s390x_emit_oilf(d_jit_buffer* _buf, int _r1, uint32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0xD, _imm);
}

int d_jit_s390x_emit_xilf(d_jit_buffer* _buf, int _r1, uint32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0x7, _imm);
}

int d_jit_s390x_emit_iihf(d_jit_buffer* _buf, int _r1, uint32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0x8, _imm);
}

int d_jit_s390x_emit_iilf(d_jit_buffer* _buf, int _r1, uint32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC0, _r1, 0x9, _imm);
}


// ===========================================================================
// VII. SHIFTS
// ===========================================================================

int d_jit_s390x_emit_sll(d_jit_buffer* _buf, int _r1, unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rs(_buf, 0x89, _r1, 0, _count, 0);
}

int d_jit_s390x_emit_srl(d_jit_buffer* _buf, int _r1, unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rs(_buf, 0x88, _r1, 0, _count, 0);
}

int d_jit_s390x_emit_sra(d_jit_buffer* _buf, int _r1, unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rs(_buf, 0x8A, _r1, 0, _count, 0);
}

int d_jit_s390x_emit_sllg(d_jit_buffer* _buf, int _r1, int _r3,
                          unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rsy(_buf, 0xEB, _r1, _r3, _count, 0, 0x0D);
}

int d_jit_s390x_emit_srlg(d_jit_buffer* _buf, int _r1, int _r3,
                          unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rsy(_buf, 0xEB, _r1, _r3, _count, 0, 0x0C);
}

int d_jit_s390x_emit_srag(d_jit_buffer* _buf, int _r1, int _r3,
                          unsigned _count)
{
    if (_count > 63) { return -1; }
    return d_jit_s390x_rsy(_buf, 0xEB, _r1, _r3, _count, 0, 0x0A);
}


// ===========================================================================
// VIII. COMPARE
// ===========================================================================

int d_jit_s390x_emit_cr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x19, _r1, _r2);
}

int d_jit_s390x_emit_clr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x15, _r1, _r2);
}

int d_jit_s390x_emit_cgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB920, _r1, _r2);
}

int d_jit_s390x_emit_clgr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rre(_buf, 0xB921, _r1, _r2);
}

int d_jit_s390x_emit_chi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xE, (uint16_t)_imm);
}

int d_jit_s390x_emit_cghi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    if (!d_jit_s390x_fits_s16(_imm)) { return -1; }
    return d_jit_s390x_ri(_buf, 0xA7, _r1, 0xF, (uint16_t)_imm);
}

int d_jit_s390x_emit_cfi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC2, _r1, 0xD, (uint32_t)_imm);
}

int d_jit_s390x_emit_cgfi(d_jit_buffer* _buf, int _r1, int32_t _imm)
{
    return d_jit_s390x_ril(_buf, 0xC2, _r1, 0xC, (uint32_t)_imm);
}


// ===========================================================================
// IX.  STORAGE -- RX (12-bit unsigned displacement)
// ===========================================================================

int d_jit_s390x_emit_l(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                       int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x58, _r1, (uint32_t)_disp,
                          _index, _base);
}

int d_jit_s390x_emit_st(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x50, _r1, (uint32_t)_disp,
                          _index, _base);
}

int d_jit_s390x_emit_a(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                       int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x5A, _r1, (uint32_t)_disp,
                          _index, _base);
}

int d_jit_s390x_emit_s(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                       int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x5B, _r1, (uint32_t)_disp,
                          _index, _base);
}

int d_jit_s390x_emit_c(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                       int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x59, _r1, (uint32_t)_disp,
                          _index, _base);
}

int d_jit_s390x_emit_la(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < 0 || _disp > 4095) { return -1; }
    return d_jit_s390x_rx(_buf, 0x41, _r1, (uint32_t)_disp,
                          _index, _base);
}


// ===========================================================================
// X.   STORAGE -- RXY (20-bit signed displacement)
// ===========================================================================

int d_jit_s390x_emit_lg(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x04);
}

int d_jit_s390x_emit_stg(d_jit_buffer* _buf, int _r1, int32_t _disp,
                         int _index, int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x24);
}

int d_jit_s390x_emit_ag(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x08);
}

int d_jit_s390x_emit_sg(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x09);
}

int d_jit_s390x_emit_cg(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x20);
}

int d_jit_s390x_emit_ly(d_jit_buffer* _buf, int _r1, int32_t _disp, int _index,
                        int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x58);
}

int d_jit_s390x_emit_sty(d_jit_buffer* _buf, int _r1, int32_t _disp,
                         int _index, int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x50);
}

int d_jit_s390x_emit_lay(d_jit_buffer* _buf, int _r1, int32_t _disp,
                         int _index, int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x71);
}

int d_jit_s390x_emit_lgf(d_jit_buffer* _buf, int _r1, int32_t _disp,
                         int _index, int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x14);
}

int d_jit_s390x_emit_llgf(d_jit_buffer* _buf, int _r1, int32_t _disp,
                          int _index, int _base)
{
    if (_disp < -524288 || _disp > 524287) { return -1; }
    return d_jit_s390x_rxy(_buf, 0xE3, _r1, _disp, _index, _base,
                           0x16);
}


// ===========================================================================
// XI.  CONTROL TRANSFER
// ===========================================================================

int d_jit_s390x_emit_brc(d_jit_buffer* _buf, unsigned _mask,
                         d_jit_label* _target)
{
    return d_jit_s390x_bri(_buf, 0xA7, (int)_mask, 0x4, _target);
}

int d_jit_s390x_emit_brcl(d_jit_buffer* _buf, unsigned _mask,
                          d_jit_label* _target)
{
    return d_jit_s390x_bril(_buf, 0xC0, (int)_mask, 0x4, _target);
}

int d_jit_s390x_emit_je(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_EQ, _target);
}

int d_jit_s390x_emit_jne(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_NE, _target);
}

int d_jit_s390x_emit_jlt(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_LT, _target);
}

int d_jit_s390x_emit_jgt(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_GT, _target);
}

int d_jit_s390x_emit_jle(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_LE, _target);
}

int d_jit_s390x_emit_jge(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_GE, _target);
}

int d_jit_s390x_emit_jmp(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_s390x_emit_brc(_buf, D_JIT_S390X_CC_ALWAYS, _target);
}

int d_jit_s390x_emit_brasl(d_jit_buffer* _buf, int _r1, d_jit_label* _target)
{
    return d_jit_s390x_bril(_buf, 0xC0, _r1, 0x5, _target);
}

int d_jit_s390x_emit_basr(d_jit_buffer* _buf, int _r1, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x0D, _r1, _r2);
}

int d_jit_s390x_emit_bcr(d_jit_buffer* _buf, unsigned _mask, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x07, (int)_mask, _r2);
}

int d_jit_s390x_emit_br(d_jit_buffer* _buf, int _r2)
{
    return d_jit_s390x_rr(_buf, 0x07, 0xF, _r2);   /* bcr 15, r2 */
}

int d_jit_s390x_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_s390x_rr(_buf, 0x07, 0x0, 0);     /* bcr 0, 0 */
}


// ===========================================================================
// XII. DIAGNOSTICS
// ===========================================================================

const char* d_jit_s390x_reg_name(int _reg)
{
    static const char* const names[16] = {
        "%r0",  "%r1",  "%r2",  "%r3",  "%r4",  "%r5",  "%r6",  "%r7",
        "%r8",  "%r9",  "%r10", "%r11", "%r12", "%r13", "%r14", "%r15"
    };
    return (_reg >= 0 && _reg < 16) ? names[_reg] : "?";
}
