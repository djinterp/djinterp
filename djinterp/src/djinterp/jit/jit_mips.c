/******************************************************************************
* djinterp [jit]                                                    jit_mips.c
*
* djinterp MIPS (MIPS32 / MIPS64) JIT encoder -- implementation (jit_mips.h).
*   Emitters assemble each instruction from its opcode/funct fields via three
* format encoders and emit one 32-bit little-endian word. Branch displacements
* are patched by the relocation at the top (PC-relative to the delay slot,
* scaled by 4), invoked by the label facility in jit.h. Callers add the delay
* slot after every branch and jump; nothing is inserted here.
*
* path:      /inc/djinterp/jit/jit_mips.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_mips.h"


// ===========================================================================
// I.   RELOCATION + FORMAT ENCODERS
// ===========================================================================

// d_jit_mips_reloc_branch
//   function (internal): patch the 16-bit offset of a PC-relative branch at
// _at so it reaches _target. MIPS measures the offset from the delay slot
// (_at + 4) in instruction-sized (4-byte) units.
static int d_jit_mips_reloc_branch(d_jit_buffer* _buf, size_t _at,
                                   size_t _target)
{
    long long disp = (long long)_target - (long long)(_at + 4);
    long long off;
    uint32_t  w;
    if (disp & 3)                    { return -1; }
    off = disp >> 2;
    if (off < -32768 || off > 32767) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0) { return -1; }
    w = (w & ~0xFFFFu) | ((uint32_t)off & 0xFFFFu);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_mips_fits_imm16
//   function (internal): 1 when _v fits a signed 16-bit immediate.
static int d_jit_mips_fits_imm16(int32_t _v)
{
    return (_v >= -32768 && _v <= 32767);
}

// format encoders: pack the register/immediate fields for one instruction
static uint32_t d_jit_mips_r(uint32_t _funct, int _rs, int _rt, int _rd,
                             unsigned _sa)
{
    /* opcode 0 (SPECIAL) is implicit in the zero high bits */
    return (((uint32_t)_rs & 0x1F) << 21)
         | (((uint32_t)_rt & 0x1F) << 16)
         | (((uint32_t)_rd & 0x1F) << 11)
         | ((_sa & 0x1Fu) << 6)
         | (_funct & 0x3Fu);
}

static uint32_t d_jit_mips_r2(uint32_t _funct, int _rs, int _rt, int _rd)
{
    return ((uint32_t)D_JIT_MIPS_OP_SPECIAL2 << 26)
         | (((uint32_t)_rs & 0x1F) << 21)
         | (((uint32_t)_rt & 0x1F) << 16)
         | (((uint32_t)_rd & 0x1F) << 11)
         | (_funct & 0x3Fu);
}

static uint32_t d_jit_mips_i(uint32_t _op, int _rs, int _rt, int32_t _imm)
{
    return ((_op & 0x3Fu) << 26)
         | (((uint32_t)_rs & 0x1F) << 21)
         | (((uint32_t)_rt & 0x1F) << 16)
         | ((uint32_t)_imm & 0xFFFFu);
}

static int d_jit_mips_emit_branch(d_jit_buffer* _buf, uint32_t _word,
                                  d_jit_label* _target)
{
    size_t at = _buf->size;
    if (d_jit_emit_u32(_buf, _word) != 0) { return -1; }
    return d_jit_label_reference(_buf, _target, at,
                                 d_jit_mips_reloc_branch);
}


// ===========================================================================
// II.  REGISTER ALU (R-TYPE)
// ===========================================================================

int d_jit_mips_emit_add(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_ADD, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_addu(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_ADDU, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_sub(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SUB, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_subu(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SUBU, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_and(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_AND, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_or(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_OR, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_xor(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_XOR, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_nor(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_NOR, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_slt(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SLT, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_sltu(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SLTU, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_mul(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r2(D_JIT_MIPS_FN_MUL, _rs, _rt, _rd));
}


// ===========================================================================
// III. SHIFTS
// ===========================================================================

int d_jit_mips_emit_sll(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SLL, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_srl(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SRL, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_sra(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SRA, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_sllv(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SLLV, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_srlv(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SRLV, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_srav(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_SRAV, _rs, _rt, _rd, 0));
}


// ===========================================================================
// IV.  MULTIPLY / DIVIDE (HI/LO)
// ===========================================================================

int d_jit_mips_emit_mult(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_MULT, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_multu(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_MULTU, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_div(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DIV, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_divu(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DIVU, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_mfhi(d_jit_buffer* _buf, int _rd)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_MFHI, 0, 0, _rd, 0));
}

int d_jit_mips_emit_mflo(d_jit_buffer* _buf, int _rd)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_MFLO, 0, 0, _rd, 0));
}


// ===========================================================================
// V.   REGISTER-IMMEDIATE (I-TYPE)
// ===========================================================================

int d_jit_mips_emit_addi(d_jit_buffer* _buf, int _rt, int _rs,
                        int32_t _imm)
{
    if (!d_jit_mips_fits_imm16(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_ADDI, _rs, _rt, _imm));
}

int d_jit_mips_emit_addiu(d_jit_buffer* _buf, int _rt, int _rs,
                        int32_t _imm)
{
    if (!d_jit_mips_fits_imm16(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_ADDIU, _rs, _rt, _imm));
}

int d_jit_mips_emit_slti(d_jit_buffer* _buf, int _rt, int _rs,
                        int32_t _imm)
{
    if (!d_jit_mips_fits_imm16(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SLTI, _rs, _rt, _imm));
}

int d_jit_mips_emit_sltiu(d_jit_buffer* _buf, int _rt, int _rs,
                        int32_t _imm)
{
    if (!d_jit_mips_fits_imm16(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SLTIU, _rs, _rt, _imm));
}

int d_jit_mips_emit_andi(d_jit_buffer* _buf, int _rt, int _rs,
                        uint32_t _imm)
{
    return d_jit_emit_u32(_buf, d_jit_mips_i(
        D_JIT_MIPS_OP_ANDI, _rs, _rt, (int32_t)(_imm & 0xFFFFu)));
}

int d_jit_mips_emit_ori(d_jit_buffer* _buf, int _rt, int _rs,
                        uint32_t _imm)
{
    return d_jit_emit_u32(_buf, d_jit_mips_i(
        D_JIT_MIPS_OP_ORI, _rs, _rt, (int32_t)(_imm & 0xFFFFu)));
}

int d_jit_mips_emit_xori(d_jit_buffer* _buf, int _rt, int _rs,
                        uint32_t _imm)
{
    return d_jit_emit_u32(_buf, d_jit_mips_i(
        D_JIT_MIPS_OP_XORI, _rs, _rt, (int32_t)(_imm & 0xFFFFu)));
}

int d_jit_mips_emit_lui(d_jit_buffer* _buf, int _rt, uint32_t _imm16)
{
    return d_jit_emit_u32(_buf, d_jit_mips_i(
        D_JIT_MIPS_OP_LUI, 0, _rt, (int32_t)(_imm16 & 0xFFFFu)));
}


// ===========================================================================
// VI.  LOADS / STORES
// ===========================================================================

int d_jit_mips_emit_lb(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LB, _base, _rt, _off));
}

int d_jit_mips_emit_lh(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LH, _base, _rt, _off));
}

int d_jit_mips_emit_lw(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LW, _base, _rt, _off));
}

int d_jit_mips_emit_lbu(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LBU, _base, _rt, _off));
}

int d_jit_mips_emit_lhu(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LHU, _base, _rt, _off));
}

int d_jit_mips_emit_sb(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SB, _base, _rt, _off));
}

int d_jit_mips_emit_sh(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SH, _base, _rt, _off));
}

int d_jit_mips_emit_sw(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SW, _base, _rt, _off));
}


// ===========================================================================
// VII. BRANCHES (PC-RELATIVE)
// ===========================================================================

int d_jit_mips_emit_beq(d_jit_buffer* _buf, int _rs, int _rt, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BEQ, _rs, _rt, 0), _t);
}

int d_jit_mips_emit_bne(d_jit_buffer* _buf, int _rs, int _rt, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BNE, _rs, _rt, 0), _t);
}

int d_jit_mips_emit_blez(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BLEZ, _rs, 0, 0), _t);
}

int d_jit_mips_emit_bgtz(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BGTZ, _rs, 0, 0), _t);
}

int d_jit_mips_emit_bltz(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_REGIMM, _rs, D_JIT_MIPS_RT_BLTZ, 0), _t);
}

int d_jit_mips_emit_bgez(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_REGIMM, _rs, D_JIT_MIPS_RT_BGEZ, 0), _t);
}

int d_jit_mips_emit_beqz(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BEQ, _rs, 0, 0), _t);
}

int d_jit_mips_emit_bnez(d_jit_buffer* _buf, int _rs, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BNE, _rs, 0, 0), _t);
}

int d_jit_mips_emit_b(d_jit_buffer* _buf, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_BEQ, 0, 0, 0), _t);
}

int d_jit_mips_emit_bal(d_jit_buffer* _buf, d_jit_label* _t)
{
    return d_jit_mips_emit_branch(_buf, d_jit_mips_i(
        D_JIT_MIPS_OP_REGIMM, 0, D_JIT_MIPS_RT_BGEZAL, 0), _t);
}


// ===========================================================================
// VIII. JUMPS
// ===========================================================================

int d_jit_mips_emit_jr(d_jit_buffer* _buf, int _rs)
{
    return d_jit_emit_u32(_buf, d_jit_mips_r(D_JIT_MIPS_FN_JR, _rs, 0, 0, 0));
}

int d_jit_mips_emit_jalr(d_jit_buffer* _buf, int _rd, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_JALR, _rs, 0, _rd, 0));
}

int d_jit_mips_emit_j(d_jit_buffer* _buf, uint32_t _target_addr)
{
    uint32_t w = ((uint32_t)D_JIT_MIPS_OP_J << 26)
               | ((_target_addr >> 2) & 0x3FFFFFFu);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_mips_emit_jal(d_jit_buffer* _buf, uint32_t _target_addr)
{
    uint32_t w = ((uint32_t)D_JIT_MIPS_OP_JAL << 26)
               | ((_target_addr >> 2) & 0x3FFFFFFu);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// IX.  PSEUDO-INSTRUCTIONS
// ===========================================================================

int d_jit_mips_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, 0x00000000u);   /* sll zero, zero, 0 */
}

int d_jit_mips_emit_move(d_jit_buffer* _buf, int _rd, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_ADDU, _rs, 0, _rd, 0));
}

int d_jit_mips_emit_li(d_jit_buffer* _buf, int _rt, int32_t _imm)
{
    uint32_t u  = (uint32_t)_imm;
    uint32_t hi = u >> 16;
    uint32_t lo = u & 0xFFFFu;
    if (hi == 0u) {                      /* 0 .. 0xFFFF: ori */
        return d_jit_emit_u32(_buf, d_jit_mips_i(
            D_JIT_MIPS_OP_ORI, D_JIT_MIPS_REG_ZERO, _rt, (int32_t)lo));
    }
    if (hi == 0xFFFFu && (lo & 0x8000u)) {  /* negative fits addiu */
        return d_jit_emit_u32(_buf, d_jit_mips_i(
            D_JIT_MIPS_OP_ADDIU, D_JIT_MIPS_REG_ZERO, _rt,
            (int32_t)(int16_t)lo));
    }
    if (d_jit_emit_u32(_buf, d_jit_mips_i(
            D_JIT_MIPS_OP_LUI, D_JIT_MIPS_REG_ZERO, _rt, (int32_t)hi)) != 0) {
        return -1;
    }
    if (lo == 0u) { return 0; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_ORI, _rt, _rt, (int32_t)lo));
}


// ===========================================================================
// X.   MIPS64-ONLY (INVALID ON MIPS32)
// ===========================================================================

int d_jit_mips_emit_dadd(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DADD, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_daddu(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DADDU, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_dsub(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSUB, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_dsubu(d_jit_buffer* _buf, int _rd, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSUBU, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_daddiu(d_jit_buffer* _buf, int _rt, int _rs,
                        int32_t _imm)
{
    if (!d_jit_mips_fits_imm16(_imm)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_DADDIU, _rs, _rt, _imm));
}

int d_jit_mips_emit_dsll(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSLL, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsrl(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRL, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsra(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRA, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsll32(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSLL32, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsrl32(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRL32, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsra32(d_jit_buffer* _buf, int _rd, int _rt,
                        unsigned _sa)
{
    if (_sa > 31) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRA32, 0, _rt, _rd, _sa));
}

int d_jit_mips_emit_dsllv(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSLLV, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_dsrlv(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRLV, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_dsrav(d_jit_buffer* _buf, int _rd, int _rt, int _rs)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DSRAV, _rs, _rt, _rd, 0));
}

int d_jit_mips_emit_dmult(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DMULT, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_dmultu(d_jit_buffer* _buf, int _rs, int _rt)
{
    return d_jit_emit_u32(_buf,
        d_jit_mips_r(D_JIT_MIPS_FN_DMULTU, _rs, _rt, 0, 0));
}

int d_jit_mips_emit_ld(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LD, _base, _rt, _off));
}

int d_jit_mips_emit_sd(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_SD, _base, _rt, _off));
}

int d_jit_mips_emit_lwu(d_jit_buffer* _buf, int _rt, int _base,
                        int32_t _off)
{
    if (!d_jit_mips_fits_imm16(_off)) { return -1; }
    return d_jit_emit_u32(_buf,
        d_jit_mips_i(D_JIT_MIPS_OP_LWU, _base, _rt, _off));
}


// ===========================================================================
// XI.  DIAGNOSTICS
// ===========================================================================

const char* d_jit_mips_reg_name(int _reg)
{
    static const char* const names[32] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
        "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7",
        "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };
    return (_reg >= 0 && _reg < 32) ? names[_reg] : "?";
}
