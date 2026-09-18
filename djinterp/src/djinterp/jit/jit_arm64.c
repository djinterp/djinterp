/******************************************************************************
* djinterp [jit]                                                   jit_arm64.c
*
* djinterp AArch64 (ARM64) JIT encoder -- implementation (jit_arm64.h).
*   Each emitter packs register and immediate fields into a verified base
* opcode and emits one 32-bit little-endian word. Branch displacements are
* patched by the two relocations at the top, which the shared label facility
* in jit.h invokes on bind.
*
* path:      /inc/djinterp/jit/jit_arm64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_arm64.h"


// ===========================================================================
// I.   RELOCATIONS + FIELD HELPERS
// ===========================================================================

// d_jit_arm64_reloc_br26
//   function (internal): patch the imm26 field of a B/BL at _at so it reaches
// _target. The offset is PC-relative to the branch itself and scaled by 4.
static int d_jit_arm64_reloc_br26(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    long long imm;
    uint32_t  w;
    if (disp & 3) { return -1; }
    imm = disp >> 2;
    if (imm < -(1LL << 25) || imm > (1LL << 25) - 1) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0)   { return -1; }
    w = (w & ~0x03FFFFFFu) | ((uint32_t)imm & 0x03FFFFFFu);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_arm64_reloc_br19
//   function (internal): patch the imm19 field (bits [23:5]) of a B.cond /
// CBZ / CBNZ at _at so it reaches _target (PC-relative, scaled by 4).
static int d_jit_arm64_reloc_br19(d_jit_buffer* _buf, size_t _at,
                                  size_t _target)
{
    long long disp = (long long)_target - (long long)_at;
    long long imm;
    uint32_t  w;
    if (disp & 3) { return -1; }
    imm = disp >> 2;
    if (imm < -(1LL << 18) || imm > (1LL << 18) - 1) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0)   { return -1; }
    w = (w & ~(0x7FFFFu << 5)) | (((uint32_t)imm & 0x7FFFFu) << 5);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_arm64_rrr
//   function (internal): OR the Rd, Rn, Rm register fields into a base opcode.
static uint32_t d_jit_arm64_rrr(uint32_t _base, int _rd, int _rn, int _rm)
{
    return _base
         | ((uint32_t)(_rd & 0x1F) << D_JIT_ARM64_RD_SHIFT)
         | ((uint32_t)(_rn & 0x1F) << D_JIT_ARM64_RN_SHIFT)
         | ((uint32_t)(_rm & 0x1F) << D_JIT_ARM64_RM_SHIFT);
}

// d_jit_arm64_emit_movw
//   function (internal): shared movz/movk/movn encoder. _shift is 0/16/32/48.
static int d_jit_arm64_emit_movw(d_jit_buffer* _buf, uint32_t _base, int _xd,
                                 uint16_t _imm16, unsigned _shift)
{
    uint32_t w;
    if (_shift != 0 && _shift != 16 && _shift != 32 && _shift != 48) {
        return -1;
    }
    w = _base
      | ((uint32_t)(_shift / 16) << D_JIT_ARM64_HW_SHIFT)
      | ((uint32_t)_imm16 << D_JIT_ARM64_IMM16_SHIFT)
      | ((uint32_t)(_xd & 0x1F) << D_JIT_ARM64_RD_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

// d_jit_arm64_emit_addsub_imm
//   function (internal): shared add/sub/subs-immediate encoder.
static int d_jit_arm64_emit_addsub_imm(d_jit_buffer* _buf, uint32_t _base,
                                       int _xd, int _xn, uint32_t _imm12,
                                       int _lsl12)
{
    uint32_t w;
    if (_imm12 > 0xFFF) { return -1; }
    w = _base
      | (_lsl12 ? D_JIT_ARM64_SHIFT12_BIT : 0u)
      | (_imm12 << D_JIT_ARM64_IMM12_SHIFT)
      | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT)
      | ((uint32_t)(_xd & 0x1F) << D_JIT_ARM64_RD_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

// d_jit_arm64_emit_branch
//   function (internal): emit a branch base word (imm field zero), then record
// the reference for _reloc to patch on bind.
static int d_jit_arm64_emit_branch(d_jit_buffer* _buf, uint32_t _word,
                                   d_jit_label* _target, d_jit_reloc_fn _reloc)
{
    size_t at = _buf->size;
    if (d_jit_emit_u32(_buf, _word) != 0) { return -1; }
    return d_jit_label_reference(_buf, _target, at, _reloc);
}


// ===========================================================================
// II.  NO-OP AND REGISTER MOVE
// ===========================================================================

int d_jit_arm64_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, D_JIT_ARM64_OP_NOP);
}

int d_jit_arm64_emit_mov_reg(d_jit_buffer* _buf, int _xd, int _xm)
{
    /* mov Xd, Xm == orr Xd, XZR, Xm */
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_ORR_REG, _xd,
                                 D_JIT_ARM64_REG_XZR, _xm);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// III. MOVE-WIDE IMMEDIATES
// ===========================================================================

int d_jit_arm64_emit_movz(d_jit_buffer* _buf, int _xd, uint16_t _imm16,
                         unsigned _shift)
{
    return d_jit_arm64_emit_movw(_buf, D_JIT_ARM64_OP_MOVZ, _xd, _imm16,
                                _shift);
}

int d_jit_arm64_emit_movk(d_jit_buffer* _buf, int _xd, uint16_t _imm16,
                         unsigned _shift)
{
    return d_jit_arm64_emit_movw(_buf, D_JIT_ARM64_OP_MOVK, _xd, _imm16,
                                _shift);
}

int d_jit_arm64_emit_movn(d_jit_buffer* _buf, int _xd, uint16_t _imm16,
                         unsigned _shift)
{
    return d_jit_arm64_emit_movw(_buf, D_JIT_ARM64_OP_MOVN, _xd, _imm16,
                                _shift);
}

int d_jit_arm64_emit_mov_imm(d_jit_buffer* _buf, int _xd, uint64_t _value)
{
    unsigned k;
    /* low halfword via movz (also handles value == 0) */
    uint16_t lo = (uint16_t)(_value & 0xFFFF);
    if (d_jit_arm64_emit_movz(_buf, _xd, lo, 0) != 0) { return -1; }
    /* remaining non-zero halfwords via movk */
    for (k = 1; k < 4; ++k) {
        uint16_t chunk = (uint16_t)((_value >> (16 * k)) & 0xFFFF);
        if (chunk != 0) {
            if (d_jit_arm64_emit_movk(_buf, _xd, chunk, 16 * k) != 0) {
                return -1;
            }
        }
    }
    return 0;
}


// ===========================================================================
// IV.  ARITHMETIC AND LOGIC (REGISTER)
// ===========================================================================

int d_jit_arm64_emit_add_reg(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_ADD_REG, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_sub_reg(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_SUB_REG, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_and_reg(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_AND_REG, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_orr_reg(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_ORR_REG, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_eor_reg(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_EOR_REG, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_mul(d_jit_buffer* _buf, int _xd, int _xn, int _xm)
{
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_MUL, _xd, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_cmp_reg(d_jit_buffer* _buf, int _xn, int _xm)
{
    /* cmp Xn, Xm == subs XZR, Xn, Xm */
    uint32_t w = d_jit_arm64_rrr(D_JIT_ARM64_OP_SUBS_REG,
                                 D_JIT_ARM64_REG_XZR, _xn, _xm);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// V.   ARITHMETIC (IMMEDIATE)
// ===========================================================================

int d_jit_arm64_emit_add_imm(d_jit_buffer* _buf, int _xd, int _xn,
                         uint32_t _imm12, int _lsl12)
{
    return d_jit_arm64_emit_addsub_imm(_buf, D_JIT_ARM64_OP_ADD_IMM, _xd, _xn,
                                       _imm12, _lsl12);
}

int d_jit_arm64_emit_sub_imm(d_jit_buffer* _buf, int _xd, int _xn,
                         uint32_t _imm12, int _lsl12)
{
    return d_jit_arm64_emit_addsub_imm(_buf, D_JIT_ARM64_OP_SUB_IMM, _xd, _xn,
                                       _imm12, _lsl12);
}

int d_jit_arm64_emit_cmp_imm(d_jit_buffer* _buf, int _xn, uint32_t _imm12,
                             int _lsl12)
{
    /* cmp Xn, #imm == subs XZR, Xn, #imm */
    return d_jit_arm64_emit_addsub_imm(_buf, D_JIT_ARM64_OP_SUBS_IMM,
                                       D_JIT_ARM64_REG_XZR, _xn, _imm12,
                                       _lsl12);
}


// ===========================================================================
// VI.  LOAD / STORE (UNSIGNED SCALED OFFSET)
// ===========================================================================

int d_jit_arm64_emit_ldr(d_jit_buffer* _buf, int _xt, int _xn,
                         uint32_t _offset)
{
    uint32_t w, imm12;
    if ((_offset & 7u) != 0 || _offset > 32760u) { return -1; }
    imm12 = _offset >> 3;
    w = D_JIT_ARM64_OP_LDR_IMM
      | (imm12 << D_JIT_ARM64_IMM12_SHIFT)
      | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT)
      | ((uint32_t)(_xt & 0x1F) << D_JIT_ARM64_RD_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_str(d_jit_buffer* _buf, int _xt, int _xn,
                         uint32_t _offset)
{
    uint32_t w, imm12;
    if ((_offset & 7u) != 0 || _offset > 32760u) { return -1; }
    imm12 = _offset >> 3;
    w = D_JIT_ARM64_OP_STR_IMM
      | (imm12 << D_JIT_ARM64_IMM12_SHIFT)
      | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT)
      | ((uint32_t)(_xt & 0x1F) << D_JIT_ARM64_RD_SHIFT);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// VII. BRANCHES
// ===========================================================================

int d_jit_arm64_emit_b(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_arm64_emit_branch(_buf, D_JIT_ARM64_OP_B, _target,
                                   d_jit_arm64_reloc_br26);
}

int d_jit_arm64_emit_bl(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_arm64_emit_branch(_buf, D_JIT_ARM64_OP_BL, _target,
                                   d_jit_arm64_reloc_br26);
}

int d_jit_arm64_emit_b_cond(d_jit_buffer* _buf, int _cc, d_jit_label* _target)
{
    uint32_t w = D_JIT_ARM64_OP_B_COND | ((uint32_t)_cc & 0xF);
    return d_jit_arm64_emit_branch(_buf, w, _target, d_jit_arm64_reloc_br19);
}

int d_jit_arm64_emit_cbz(d_jit_buffer* _buf, int _xt, d_jit_label* _target)
{
    uint32_t w = D_JIT_ARM64_OP_CBZ | ((uint32_t)(_xt & 0x1F));
    return d_jit_arm64_emit_branch(_buf, w, _target,
                                   d_jit_arm64_reloc_br19);
}

int d_jit_arm64_emit_cbnz(d_jit_buffer* _buf, int _xt, d_jit_label* _target)
{
    uint32_t w = D_JIT_ARM64_OP_CBNZ | ((uint32_t)(_xt & 0x1F));
    return d_jit_arm64_emit_branch(_buf, w, _target,
                                   d_jit_arm64_reloc_br19);
}


// ===========================================================================
// VIII. RETURNS AND INDIRECT BRANCHES
// ===========================================================================

int d_jit_arm64_emit_ret(d_jit_buffer* _buf)
{
    /* ret == ret x30 */
    uint32_t w = D_JIT_ARM64_OP_RET
               | ((uint32_t)D_JIT_ARM64_REG_LR << D_JIT_ARM64_RN_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_ret_reg(d_jit_buffer* _buf, int _xn)
{
    uint32_t w = D_JIT_ARM64_OP_RET
               | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_br(d_jit_buffer* _buf, int _xn)
{
    uint32_t w = D_JIT_ARM64_OP_BR
               | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm64_emit_blr(d_jit_buffer* _buf, int _xn)
{
    uint32_t w = D_JIT_ARM64_OP_BLR
               | ((uint32_t)(_xn & 0x1F) << D_JIT_ARM64_RN_SHIFT);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// IX.  DIAGNOSTICS
// ===========================================================================

const char* d_jit_arm64_reg_name(int _reg)
{
    static const char* const names[32] = {
        "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
        "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
        "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
        "x24", "x25", "x26", "x27", "x28", "x29", "x30", "sp"
    };
    return (_reg >= 0 && _reg < 32) ? names[_reg] : "?";
}
