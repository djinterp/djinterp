/******************************************************************************
* djinterp [jit]                                                     jit_x64.c
*
* djinterp x86-64 JIT encoder -- implementation (jit_x64.h).
*   The instruction emitters: each encodes one x86-64 instruction from the
* module's byte constants and appends it to a d_jit_buffer. REX.B for extended
* registers is applied where needed.
*
* path:      /inc/djinterp/jit/jit_x64.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_x64.h"


// ===========================================================================
// I.   INTERNAL HELPERS
// ===========================================================================

// d_jit_x64_internal_needs_rexb
//   function (internal): 1 when a register number addresses r8-r15 and so
// needs its 4th bit carried in a REX.B prefix.
static int d_jit_x64_internal_needs_rexb(int _reg)
{
    return (_reg & 0x8) ? 1 : 0;
}


// ===========================================================================
// II.  EMITTERS
// ===========================================================================

int d_jit_x64_emit_prologue(d_jit_buffer* _buf)
{
    static const unsigned char seq[] = { D_JIT_X64_SEQ_PROLOGUE };
    return d_jit_emit(_buf, seq, sizeof seq);
}

int d_jit_x64_emit_epilogue(d_jit_buffer* _buf)
{
    static const unsigned char seq[] = { D_JIT_X64_SEQ_EPILOGUE };
    return d_jit_emit(_buf, seq, sizeof seq);
}

int d_jit_x64_emit_ret(d_jit_buffer* _buf)
{
    return d_jit_emit_u8(_buf, D_JIT_X64_OP_RET);
}

int d_jit_x64_emit_nop(d_jit_buffer* _buf, size_t _n)
{
    /* the canonical 1..9-byte NOP encodings, indexed by length */
    static const unsigned char n1[] = { D_JIT_X64_SEQ_NOP1 };
    static const unsigned char n2[] = { D_JIT_X64_SEQ_NOP2 };
    static const unsigned char n3[] = { D_JIT_X64_SEQ_NOP3 };
    static const unsigned char n4[] = { D_JIT_X64_SEQ_NOP4 };
    static const unsigned char n5[] = { D_JIT_X64_SEQ_NOP5 };
    static const unsigned char n6[] = { D_JIT_X64_SEQ_NOP6 };
    static const unsigned char n7[] = { D_JIT_X64_SEQ_NOP7 };
    static const unsigned char n8[] = { D_JIT_X64_SEQ_NOP8 };
    static const unsigned char n9[] = { D_JIT_X64_SEQ_NOP9 };
    static const unsigned char* const table[10] = {
        NULL, n1, n2, n3, n4, n5, n6, n7, n8, n9
    };
    while (_n > 0) {
        size_t chunk = (_n > 9) ? 9 : _n;
        if (d_jit_emit(_buf, table[chunk], chunk) != 0) { return -1; }
        _n -= chunk;
    }
    return 0;
}

int d_jit_x64_emit_mov_reg_imm32(d_jit_buffer* _buf, int _reg, uint32_t _imm)
{
    /* [REX.B] B8+rd id  -- writing r32 zero-extends into r64 */
    if (d_jit_x64_internal_needs_rexb(_reg)) {
        if (d_jit_emit_u8(_buf, D_JIT_X64_REX_B_BYTE) != 0) { return -1; }
    }
    uint8_t op = (uint8_t)(D_JIT_X64_OP_MOV_R_IMM32 + (_reg & 7));
    if (d_jit_emit_u8(_buf, op) != 0) { return -1; }
    return d_jit_emit_u32(_buf, _imm);
}

int d_jit_x64_emit_mov_reg_reg(d_jit_buffer* _buf, int _dst, int _src)
{
    /* REX.W [+R/+B] 89 /r  (r/m <- r, so rm=_dst, reg=_src) */
    uint8_t rex = (uint8_t)D_JIT_X64_REX(1,
                                         d_jit_x64_internal_needs_rexb(_src),
                                         0,
                                         d_jit_x64_internal_needs_rexb(_dst));
    uint8_t modrm = (uint8_t)D_JIT_X64_MODRM(D_JIT_X64_MOD_REG, _src, _dst);
    if (d_jit_emit_u8(_buf, rex) != 0)                      { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X64_OP_MOV_RM_R) != 0)    { return -1; }
    return d_jit_emit_u8(_buf, modrm);
}

int d_jit_x64_emit_add_reg_imm32(d_jit_buffer* _buf, int _reg, int32_t _imm)
{
    /* REX.W [+B] 81 /0 id */
    uint8_t rex = (uint8_t)D_JIT_X64_REX(1, 0, 0,
                                         d_jit_x64_internal_needs_rexb(_reg));
    uint8_t modrm = (uint8_t)D_JIT_X64_MODRM(D_JIT_X64_MOD_REG,
                                             D_JIT_X64_GRP1_ADD, _reg);
    if (d_jit_emit_u8(_buf, rex) != 0)                        { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X64_OP_GRP1_RM_IMM32) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, modrm) != 0)                      { return -1; }
    return d_jit_emit_u32(_buf, (uint32_t)_imm);
}

int d_jit_x64_emit_call_reg(d_jit_buffer* _buf, int _reg)
{
    /* [REX.B] FF /2 */
    uint8_t modrm = (uint8_t)D_JIT_X64_MODRM(D_JIT_X64_MOD_REG,
                                             D_JIT_X64_GRP5_CALL_RM, _reg);
    if (d_jit_x64_internal_needs_rexb(_reg)) {
        if (d_jit_emit_u8(_buf, D_JIT_X64_REX_B_BYTE) != 0) { return -1; }
    }
    if (d_jit_emit_u8(_buf, D_JIT_X64_OP_GRP5) != 0)        { return -1; }
    return d_jit_emit_u8(_buf, modrm);
}

const char* d_jit_x64_reg_name(int _reg)
{
    static const char* const names[16] = {
        "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
        "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"
    };
    return (_reg >= 0 && _reg < 16) ? names[_reg] : "?";
}
