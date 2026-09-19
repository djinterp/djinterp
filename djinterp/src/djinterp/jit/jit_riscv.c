/******************************************************************************
* djinterp [jit]                                                   jit_riscv.c
*
* djinterp RISC-V (RV32I / RV64I) JIT encoder -- implementation (jit_riscv.h).
*   Emitters assemble each instruction from its opcode and funct fields via the
* six format encoders, then emit one 32-bit little-endian word. Branch and jump
* displacements are patched by the two relocations at the top (the RISC-V B-
* and J-type immediate bit-scrambles), invoked by the label facility in jit.h.
*
* path:      /inc/djinterp/jit/jit_riscv.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_riscv.h"


// ===========================================================================
// I.   RELOCATIONS + FORMAT ENCODERS
// ===========================================================================

// d_jit_riscv_reloc_branch
//   function (internal): patch the B-type immediate of a branch at _at so it
// reaches _target. The offset is PC-relative to the branch and 2-aligned.
static int d_jit_riscv_reloc_branch(d_jit_buffer* _buf, size_t _at,
                                    size_t _target)
{
    long long off = (long long)_target - (long long)_at;
    uint32_t  uoff, imm, w;
    if (off & 1)                     { return -1; }
    if (off < -4096 || off > 4094)   { return -1; }
    uoff = (uint32_t)(long)off;
    imm = (((uoff >> 12) & 1u) << 31)
        | (((uoff >>  5) & 0x3Fu) << 25)
        | (((uoff >>  1) & 0x0Fu) << 8)
        | (((uoff >> 11) & 1u) << 7);
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0) { return -1; }
    w = (w & ~0xFE000F80u) | imm;
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_riscv_reloc_jal
//   function (internal): patch the J-type immediate of a JAL at _at so it
// reaches _target (PC-relative, 2-aligned).
static int d_jit_riscv_reloc_jal(d_jit_buffer* _buf, size_t _at,
                                 size_t _target)
{
    long long off = (long long)_target - (long long)_at;
    uint32_t  uoff, imm, w;
    if (off & 1)                           { return -1; }
    if (off < -1048576 || off > 1048574)   { return -1; }
    uoff = (uint32_t)(long)off;
    imm = (((uoff >> 20) & 1u) << 31)
        | (((uoff >>  1) & 0x3FFu) << 21)
        | (((uoff >> 11) & 1u) << 20)
        | (((uoff >> 12) & 0xFFu) << 12);
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0) { return -1; }
    w = (w & ~0xFFFFF000u) | imm;
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_riscv_fits_imm12
//   function (internal): 1 when _v fits a signed 12-bit immediate.
static int d_jit_riscv_fits_imm12(int32_t _v)
{
    return (_v >= -2048 && _v <= 2047);
}

// format encoders: pack the register/immediate fields for one instruction
static uint32_t d_jit_riscv_r(uint32_t _op, uint32_t _f3, uint32_t _f7,
                              int _rd, int _rs1, int _rs2)
{
    return _op
         | (((uint32_t)_rd  & 0x1F) << 7)
         | ((_f3 & 7u) << 12)
         | (((uint32_t)_rs1 & 0x1F) << 15)
         | (((uint32_t)_rs2 & 0x1F) << 20)
         | ((_f7 & 0x7Fu) << 25);
}

static uint32_t d_jit_riscv_i(uint32_t _op, uint32_t _f3, int _rd, int _rs1,
                              int32_t _imm)
{
    return _op
         | (((uint32_t)_rd  & 0x1F) << 7)
         | ((_f3 & 7u) << 12)
         | (((uint32_t)_rs1 & 0x1F) << 15)
         | (((uint32_t)_imm & 0xFFFu) << 20);
}

static uint32_t d_jit_riscv_s(uint32_t _op, uint32_t _f3, int _rs1, int _rs2,
                              int32_t _imm)
{
    uint32_t u = (uint32_t)_imm;
    return _op
         | ((u & 0x1Fu) << 7)
         | ((_f3 & 7u) << 12)
         | (((uint32_t)_rs1 & 0x1F) << 15)
         | (((uint32_t)_rs2 & 0x1F) << 20)
         | (((u >> 5) & 0x7Fu) << 25);
}

static uint32_t d_jit_riscv_u(uint32_t _op, int _rd, uint32_t _imm20)
{
    return _op
         | (((uint32_t)_rd & 0x1F) << 7)
         | ((_imm20 & 0xFFFFFu) << 12);
}

static uint32_t d_jit_riscv_shift(uint32_t _op, uint32_t _f3, uint32_t _top6,
                                  int _rd, int _rs1, unsigned _shamt)
{
    return _op
         | (((uint32_t)_rd  & 0x1F) << 7)
         | ((_f3 & 7u) << 12)
         | (((uint32_t)_rs1 & 0x1F) << 15)
         | (((uint32_t)_shamt & 0x3Fu) << 20)
         | ((_top6 & 0x3Fu) << 26);
}

static uint32_t d_jit_riscv_bbase(uint32_t _f3, int _rs1, int _rs2)
{
    return D_JIT_RISCV_OP_BRANCH
         | ((_f3 & 7u) << 12)
         | (((uint32_t)_rs1 & 0x1F) << 15)
         | (((uint32_t)_rs2 & 0x1F) << 20);
}

// d_jit_riscv_emit_branch / _emit_jump: emit a base word, then record the
// reference for the matching relocation to patch on bind.
static int d_jit_riscv_emit_branch(d_jit_buffer* _buf, uint32_t _word,
                                   d_jit_label* _target)
{
    size_t at = _buf->size;
    if (d_jit_emit_u32(_buf, _word) != 0) { return -1; }
    return d_jit_label_reference(_buf, _target, at,
                                 d_jit_riscv_reloc_branch);
}

static int d_jit_riscv_emit_jump(d_jit_buffer* _buf, int _rd,
                                 d_jit_label* _target)
{
    uint32_t w = D_JIT_RISCV_OP_JAL | (((uint32_t)_rd & 0x1F) << 7);
    size_t   at = _buf->size;
    if (d_jit_emit_u32(_buf, w) != 0) { return -1; }
    return d_jit_label_reference(_buf, _target, at, d_jit_riscv_reloc_jal);
}


// ===========================================================================
// II.  REGISTER-REGISTER ALU (R-TYPE)
// ===========================================================================

int d_jit_riscv_emit_add(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x0, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sub(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x0, 0x20, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sll(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x1, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_slt(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x2, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sltu(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x3, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_xor(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x4, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_srl(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x5, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sra(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x5, 0x20, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_or(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x6, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_and(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x7, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_mul(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP, 0x0, 0x01, _rd, _rs1, _rs2));
}


// ===========================================================================
// III. REGISTER-IMMEDIATE ALU (I-TYPE)
// ===========================================================================

int d_jit_riscv_emit_addi(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x0, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_slti(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x2, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_sltiu(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x3, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_xori(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x4, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_ori(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x6, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_andi(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x7, _rd, _rs1, _imm));
}


// ===========================================================================
// IV.  SHIFT-IMMEDIATE
// ===========================================================================

int d_jit_riscv_emit_slli(d_jit_buffer* _buf, int _rd, int _rs1,
                         unsigned _shamt)
{
    if (_shamt > 63) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_shift(
        D_JIT_RISCV_OP_OP_IMM, 0x1, 0x00, _rd, _rs1, _shamt));
}

int d_jit_riscv_emit_srli(d_jit_buffer* _buf, int _rd, int _rs1,
                         unsigned _shamt)
{
    if (_shamt > 63) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_shift(
        D_JIT_RISCV_OP_OP_IMM, 0x5, 0x00, _rd, _rs1, _shamt));
}

int d_jit_riscv_emit_srai(d_jit_buffer* _buf, int _rd, int _rs1,
                         unsigned _shamt)
{
    if (_shamt > 63) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_shift(
        D_JIT_RISCV_OP_OP_IMM, 0x5, 0x10, _rd, _rs1, _shamt));
}


// ===========================================================================
// V.   LOADS
// ===========================================================================

int d_jit_riscv_emit_lb(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x0, _rd, _base, _off));
}

int d_jit_riscv_emit_lh(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x1, _rd, _base, _off));
}

int d_jit_riscv_emit_lw(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x2, _rd, _base, _off));
}

int d_jit_riscv_emit_lbu(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x4, _rd, _base, _off));
}

int d_jit_riscv_emit_lhu(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x5, _rd, _base, _off));
}


// ===========================================================================
// VI.  STORES
// ===========================================================================

int d_jit_riscv_emit_sb(d_jit_buffer* _buf, int _src, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_s(D_JIT_RISCV_OP_STORE, 0x0, _base, _src, _off));
}

int d_jit_riscv_emit_sh(d_jit_buffer* _buf, int _src, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_s(D_JIT_RISCV_OP_STORE, 0x1, _base, _src, _off));
}

int d_jit_riscv_emit_sw(d_jit_buffer* _buf, int _src, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_s(D_JIT_RISCV_OP_STORE, 0x2, _base, _src, _off));
}


// ===========================================================================
// VII. UPPER IMMEDIATES (U-TYPE)
// ===========================================================================

int d_jit_riscv_emit_lui(d_jit_buffer* _buf, int _rd, uint32_t _imm20)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_u(D_JIT_RISCV_OP_LUI, _rd, _imm20));
}

int d_jit_riscv_emit_auipc(d_jit_buffer* _buf, int _rd, uint32_t _imm20)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_u(D_JIT_RISCV_OP_AUIPC, _rd, _imm20));
}


// ===========================================================================
// VIII. BRANCHES
// ===========================================================================

int d_jit_riscv_emit_beq(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x0, _rs1, _rs2), _target);
}

int d_jit_riscv_emit_bne(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x1, _rs1, _rs2), _target);
}

int d_jit_riscv_emit_blt(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x4, _rs1, _rs2), _target);
}

int d_jit_riscv_emit_bge(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x5, _rs1, _rs2), _target);
}

int d_jit_riscv_emit_bltu(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x6, _rs1, _rs2), _target);
}

int d_jit_riscv_emit_bgeu(d_jit_buffer* _buf, int _rs1, int _rs2,
                        d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x7, _rs1, _rs2), _target);
}


// ===========================================================================
// IX.  JUMPS
// ===========================================================================

int d_jit_riscv_emit_jal(d_jit_buffer* _buf, int _rd, d_jit_label* _target)
{
    return d_jit_riscv_emit_jump(_buf, _rd, _target);
}

int d_jit_riscv_emit_jalr(d_jit_buffer* _buf, int _rd, int _rs1, int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_JALR, 0x0, _rd, _rs1, _off));
}

int d_jit_riscv_emit_j(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_riscv_emit_jump(_buf, D_JIT_RISCV_REG_ZERO, _target);
}

int d_jit_riscv_emit_call(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_riscv_emit_jump(_buf, D_JIT_RISCV_REG_RA, _target);
}

int d_jit_riscv_emit_ret(d_jit_buffer* _buf)
{
    /* jalr zero, 0(ra) */
    return d_jit_emit_u32(_buf, d_jit_riscv_i(D_JIT_RISCV_OP_JALR, 0x0,
        D_JIT_RISCV_REG_ZERO, D_JIT_RISCV_REG_RA, 0));
}

int d_jit_riscv_emit_jr(d_jit_buffer* _buf, int _rs1)
{
    return d_jit_emit_u32(_buf, d_jit_riscv_i(D_JIT_RISCV_OP_JALR, 0x0,
        D_JIT_RISCV_REG_ZERO, _rs1, 0));
}

int d_jit_riscv_emit_beqz(d_jit_buffer* _buf, int _rs1, d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x0, _rs1, D_JIT_RISCV_REG_ZERO), _target);
}

int d_jit_riscv_emit_bnez(d_jit_buffer* _buf, int _rs1, d_jit_label* _target)
{
    return d_jit_riscv_emit_branch(_buf,
        d_jit_riscv_bbase(0x1, _rs1, D_JIT_RISCV_REG_ZERO), _target);
}


// ===========================================================================
// X.   PSEUDO-INSTRUCTIONS
// ===========================================================================

int d_jit_riscv_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x0,
        D_JIT_RISCV_REG_ZERO, D_JIT_RISCV_REG_ZERO, 0));
}

int d_jit_riscv_emit_mv(d_jit_buffer* _buf, int _rd, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x0, _rd, _rs, 0));
}

int d_jit_riscv_emit_li(d_jit_buffer* _buf, int _rd, int32_t _imm)
{
    long long hi;
    int32_t   lo;
    if (d_jit_riscv_fits_imm12(_imm)) {
        return d_jit_emit_u32(_buf, d_jit_riscv_i(
            D_JIT_RISCV_OP_OP_IMM, 0x0, _rd, D_JIT_RISCV_REG_ZERO, _imm));
    }
    /* lui high 20 bits (with carry from the signed low 12), then addi */
    hi = ((long long)_imm + 0x800) >> 12;
    lo = (int32_t)((long long)_imm - (hi << 12));
    if (d_jit_emit_u32(_buf, d_jit_riscv_u(
            D_JIT_RISCV_OP_LUI, _rd, (uint32_t)hi)) != 0) {
        return -1;
    }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM, 0x0, _rd, _rd, lo));
}


// ===========================================================================
// XI.  RV64I-ONLY (INVALID ON RV32)
// ===========================================================================

int d_jit_riscv_emit_lwu(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x6, _rd, _base, _off));
}

int d_jit_riscv_emit_ld(d_jit_buffer* _buf, int _rd, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_LOAD, 0x3, _rd, _base, _off));
}

int d_jit_riscv_emit_sd(d_jit_buffer* _buf, int _src, int _base,
                        int32_t _off)
{
    if (!d_jit_riscv_fits_imm12(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_s(D_JIT_RISCV_OP_STORE, 0x3, _base, _src, _off));
}

int d_jit_riscv_emit_addiw(d_jit_buffer* _buf, int _rd, int _rs1,
                         int32_t _imm)
{
    if (!d_jit_riscv_fits_imm12(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_riscv_i(D_JIT_RISCV_OP_OP_IMM_32, 0x0, _rd, _rs1, _imm));
}

int d_jit_riscv_emit_slliw(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shamt)
{
    if (_shamt > 31) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_r(
        D_JIT_RISCV_OP_OP_IMM_32, 0x1, 0x00, _rd, _rs1, (int)_shamt));
}

int d_jit_riscv_emit_srliw(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shamt)
{
    if (_shamt > 31) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_r(
        D_JIT_RISCV_OP_OP_IMM_32, 0x5, 0x00, _rd, _rs1, (int)_shamt));
}

int d_jit_riscv_emit_sraiw(d_jit_buffer* _buf, int _rd, int _rs1,
                          unsigned _shamt)
{
    if (_shamt > 31) { return -1; }
    return d_jit_emit_u32(_buf, d_jit_riscv_r(
        D_JIT_RISCV_OP_OP_IMM_32, 0x5, 0x20, _rd, _rs1, (int)_shamt));
}

int d_jit_riscv_emit_addw(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP_32, 0x0, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_subw(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP_32, 0x0, 0x20, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sllw(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP_32, 0x1, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_srlw(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP_32, 0x5, 0x00, _rd, _rs1, _rs2));
}

int d_jit_riscv_emit_sraw(d_jit_buffer* _buf, int _rd, int _rs1, int _rs2)
{
    return d_jit_emit_u32(_buf,
        d_jit_riscv_r(D_JIT_RISCV_OP_OP_32, 0x5, 0x20, _rd, _rs1, _rs2));
}


// ===========================================================================
// XII. DIAGNOSTICS
// ===========================================================================

const char* d_jit_riscv_reg_name(int _reg)
{
    static const char* const names[32] = {
        "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
        "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
        "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
        "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6"
    };
    return (_reg >= 0 && _reg < 32) ? names[_reg] : "?";
}
