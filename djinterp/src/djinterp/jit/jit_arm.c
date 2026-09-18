/******************************************************************************
* djinterp [jit]                                                     jit_arm.c
*
* djinterp AArch32 / ARM (A32) JIT encoder -- implementation (jit_arm.h).
*   Each emitter packs register and immediate fields into a verified base
* opcode and emits one 32-bit little-endian word. Branch displacements are
* patched by the relocation at the top (PC base is the branch address plus 8,
* per the A32 pipeline), invoked by the shared label facility in jit.h.
*
* path:      /inc/djinterp/jit/jit_arm.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_arm.h"


// ===========================================================================
// I.   RELOCATION + ENCODING HELPERS
// ===========================================================================

// d_jit_arm_reloc_br24
//   function (internal): patch the imm24 field of a B/BL/B.cond at _at so it
// reaches _target. The A32 program counter reads as the instruction address
// plus 8, so the offset is measured from _at + 8 and scaled by 4.
static int d_jit_arm_reloc_br24(d_jit_buffer* _buf, size_t _at, size_t _target)
{
    long long disp = (long long)_target - (long long)(_at + 8);
    long long imm;
    uint32_t  w;
    if (disp & 3) { return -1; }
    imm = disp >> 2;
    if (imm < -(1LL << 23) || imm > (1LL << 23) - 1) { return -1; }
    if (d_jit_buffer_read_u32(_buf, _at, &w) != 0)   { return -1; }
    w = (w & ~0x00FFFFFFu) | ((uint32_t)imm & 0x00FFFFFFu);
    return d_jit_buffer_patch(_buf, _at, w, 4);
}

// d_jit_arm_encode_imm
//   function (internal): encode _v as an ARM modified immediate -- an 8-bit
// value rotated right by an even amount -- into *_op2 (the 12-bit operand2).
//   returns: 0 if encodable, -1 otherwise.
static int d_jit_arm_encode_imm(uint32_t _v, uint32_t* _op2)
{
    unsigned r;
    for (r = 0; r < 16; ++r) {
        unsigned k  = 2u * r;
        uint32_t rl = (k == 0) ? _v : ((_v << k) | (_v >> (32 - k)));
        if (rl <= 0xFFu) {
            *_op2 = ((uint32_t)r << 8) | rl;
            return 0;
        }
    }
    return -1;
}

// d_jit_arm_dp_reg
//   function (internal): assemble a data-processing register word.
static uint32_t d_jit_arm_dp_reg(uint32_t _base, int _rd, int _rn, int _rm)
{
    return _base
         | ((uint32_t)(_rn & 0xF) << D_JIT_ARM_RN_SHIFT)
         | ((uint32_t)(_rd & 0xF) << D_JIT_ARM_RD_SHIFT)
         | ((uint32_t)(_rm & 0xF) << D_JIT_ARM_RM_SHIFT);
}

// d_jit_arm_emit_dp_imm
//   function (internal): emit a data-processing modified-immediate word.
static int d_jit_arm_emit_dp_imm(d_jit_buffer* _buf, uint32_t _base, int _rd,
                                 int _rn, uint32_t _imm)
{
    uint32_t op2;
    if (d_jit_arm_encode_imm(_imm, &op2) != 0) { return -1; }
    return d_jit_emit_u32(_buf, _base
        | ((uint32_t)(_rn & 0xF) << D_JIT_ARM_RN_SHIFT)
        | ((uint32_t)(_rd & 0xF) << D_JIT_ARM_RD_SHIFT)
        | op2);
}

// d_jit_arm_emit_branch
//   function (internal): emit a branch base word, then record the reference.
static int d_jit_arm_emit_branch(d_jit_buffer* _buf, uint32_t _word,
                                 d_jit_label* _target)
{
    size_t at = _buf->size;
    if (d_jit_emit_u32(_buf, _word) != 0) { return -1; }
    return d_jit_label_reference(_buf, _target, at, d_jit_arm_reloc_br24);
}


// ===========================================================================
// II.  NO-OP AND MOVES
// ===========================================================================

int d_jit_arm_emit_nop(d_jit_buffer* _buf)
{
    return d_jit_emit_u32(_buf, D_JIT_ARM_OP_NOP);
}

int d_jit_arm_emit_mov_reg(d_jit_buffer* _buf, int _rd, int _rm)
{
    /* mov has no Rn; pass 0 */
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_MOV_REG, _rd, 0, _rm));
}

int d_jit_arm_emit_mov_imm(d_jit_buffer* _buf, int _rd, uint32_t _imm)
{
    return d_jit_arm_emit_dp_imm(_buf, D_JIT_ARM_OP_MOV_IMM, _rd, 0, _imm);
}

int d_jit_arm_emit_movw(d_jit_buffer* _buf, int _rd, uint16_t _imm16)
{
    uint32_t w = D_JIT_ARM_OP_MOVW
               | (((uint32_t)_imm16 >> 12) << 16)
               | ((uint32_t)(_rd & 0xF) << D_JIT_ARM_RD_SHIFT)
               | ((uint32_t)_imm16 & 0xFFFu);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm_emit_movt(d_jit_buffer* _buf, int _rd, uint16_t _imm16)
{
    uint32_t w = D_JIT_ARM_OP_MOVT
               | (((uint32_t)_imm16 >> 12) << 16)
               | ((uint32_t)(_rd & 0xF) << D_JIT_ARM_RD_SHIFT)
               | ((uint32_t)_imm16 & 0xFFFu);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm_emit_mov_imm32(d_jit_buffer* _buf, int _rd, uint32_t _imm)
{
    if (d_jit_arm_emit_movw(_buf, _rd, (uint16_t)(_imm & 0xFFFF)) != 0) {
        return -1;
    }
    if ((_imm >> 16) != 0) {
        return d_jit_arm_emit_movt(_buf, _rd, (uint16_t)(_imm >> 16));
    }
    return 0;
}


// ===========================================================================
// III. ARITHMETIC AND LOGIC (REGISTER)
// ===========================================================================

int d_jit_arm_emit_add_reg(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_ADD_REG, _rd, _rn, _rm));
}

int d_jit_arm_emit_sub_reg(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_SUB_REG, _rd, _rn, _rm));
}

int d_jit_arm_emit_and_reg(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_AND_REG, _rd, _rn, _rm));
}

int d_jit_arm_emit_orr_reg(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_ORR_REG, _rd, _rn, _rm));
}

int d_jit_arm_emit_eor_reg(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_EOR_REG, _rd, _rn, _rm));
}

int d_jit_arm_emit_mul(d_jit_buffer* _buf, int _rd, int _rn, int _rm)
{
    /* MUL: Rd[19:16], Rm(=_rn)[3:0], Rs(=_rm)[11:8]; Rd = _rn * _rm */
    uint32_t w = D_JIT_ARM_OP_MUL
               | ((uint32_t)(_rd & 0xF) << 16)
               | ((uint32_t)(_rm & 0xF) << 8)
               | ((uint32_t)(_rn & 0xF));
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm_emit_cmp_reg(d_jit_buffer* _buf, int _rn, int _rm)
{
    /* cmp has no Rd; pass 0 */
    return d_jit_emit_u32(_buf,
        d_jit_arm_dp_reg(D_JIT_ARM_OP_CMP_REG, 0, _rn, _rm));
}


// ===========================================================================
// IV.  ARITHMETIC (MODIFIED IMMEDIATE)
// ===========================================================================

int d_jit_arm_emit_add_imm(d_jit_buffer* _buf, int _rd, int _rn, uint32_t _imm)
{
    return d_jit_arm_emit_dp_imm(_buf, D_JIT_ARM_OP_ADD_IMM, _rd, _rn, _imm);
}

int d_jit_arm_emit_sub_imm(d_jit_buffer* _buf, int _rd, int _rn, uint32_t _imm)
{
    return d_jit_arm_emit_dp_imm(_buf, D_JIT_ARM_OP_SUB_IMM, _rd, _rn, _imm);
}

int d_jit_arm_emit_cmp_imm(d_jit_buffer* _buf, int _rn, uint32_t _imm)
{
    return d_jit_arm_emit_dp_imm(_buf, D_JIT_ARM_OP_CMP_IMM, 0, _rn, _imm);
}


// ===========================================================================
// V.   LOAD / STORE (IMMEDIATE OFFSET)
// ===========================================================================

int d_jit_arm_emit_ldr(d_jit_buffer* _buf, int _rt, int _rn,
                       uint32_t _offset)
{
    uint32_t w;
    if (_offset > 0xFFFu) { return -1; }
    w = D_JIT_ARM_OP_LDR_IMM
      | ((uint32_t)(_rn & 0xF) << D_JIT_ARM_RN_SHIFT)
      | ((uint32_t)(_rt & 0xF) << D_JIT_ARM_RD_SHIFT)
      | (_offset & 0xFFFu);
    return d_jit_emit_u32(_buf, w);
}

int d_jit_arm_emit_str(d_jit_buffer* _buf, int _rt, int _rn,
                       uint32_t _offset)
{
    uint32_t w;
    if (_offset > 0xFFFu) { return -1; }
    w = D_JIT_ARM_OP_STR_IMM
      | ((uint32_t)(_rn & 0xF) << D_JIT_ARM_RN_SHIFT)
      | ((uint32_t)(_rt & 0xF) << D_JIT_ARM_RD_SHIFT)
      | (_offset & 0xFFFu);
    return d_jit_emit_u32(_buf, w);
}


// ===========================================================================
// VI.  STACK (REGISTER-LIST)
// ===========================================================================

int d_jit_arm_emit_push(d_jit_buffer* _buf, unsigned _reglist)
{
    return d_jit_emit_u32(_buf, D_JIT_ARM_OP_PUSH | (_reglist & 0xFFFFu));
}

int d_jit_arm_emit_pop(d_jit_buffer* _buf, unsigned _reglist)
{
    return d_jit_emit_u32(_buf, D_JIT_ARM_OP_POP | (_reglist & 0xFFFFu));
}


// ===========================================================================
// VII. BRANCHES
// ===========================================================================

int d_jit_arm_emit_b(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_arm_emit_branch(_buf, D_JIT_ARM_OP_B, _target);
}

int d_jit_arm_emit_bl(d_jit_buffer* _buf, d_jit_label* _target)
{
    return d_jit_arm_emit_branch(_buf, D_JIT_ARM_OP_BL, _target);
}

int d_jit_arm_emit_b_cond(d_jit_buffer* _buf, int _cc, d_jit_label* _target)
{
    uint32_t w = D_JIT_ARM_OP_B_COND
               | ((uint32_t)(_cc & 0xF) << D_JIT_ARM_COND_SHIFT);
    return d_jit_arm_emit_branch(_buf, w, _target);
}

int d_jit_arm_emit_bx(d_jit_buffer* _buf, int _rm)
{
    return d_jit_emit_u32(_buf, D_JIT_ARM_OP_BX | (uint32_t)(_rm & 0xF));
}


// ===========================================================================
// VIII. DIAGNOSTICS
// ===========================================================================

const char* d_jit_arm_reg_name(int _reg)
{
    static const char* const names[16] = {
        "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
    };
    return (_reg >= 0 && _reg < 16) ? names[_reg] : "?";
}
