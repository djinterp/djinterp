/******************************************************************************
* djinterp [jit]                                                     jit_x86.c
*
* djinterp x86 (32-bit) JIT encoder -- implementation (jit_x86.h).
*   cdecl-oriented instruction emitters. No REX prefix exists in 32-bit mode,
* so encodings are a byte or two shorter than their x86-64 counterparts.
*
* path:      /inc/djinterp/jit/jit_x86.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_x86.h"


// ===========================================================================
// I.   EMITTERS
// ===========================================================================

int d_jit_x86_emit_prologue(d_jit_buffer* _buf)
{
    static const unsigned char seq[] = { D_JIT_X86_SEQ_PROLOGUE };
    return d_jit_emit(_buf, seq, sizeof seq);
}

int d_jit_x86_emit_epilogue(d_jit_buffer* _buf)
{
    static const unsigned char seq[] = { D_JIT_X86_SEQ_EPILOGUE };
    return d_jit_emit(_buf, seq, sizeof seq);
}

int d_jit_x86_emit_ret(d_jit_buffer* _buf)
{
    return d_jit_emit_u8(_buf, D_JIT_X86_OP_RET);
}

int d_jit_x86_emit_nop(d_jit_buffer* _buf, size_t _n)
{
    static const unsigned char n1[] = { D_JIT_X86_SEQ_NOP1 };
    static const unsigned char n2[] = { D_JIT_X86_SEQ_NOP2 };
    static const unsigned char n3[] = { D_JIT_X86_SEQ_NOP3 };
    static const unsigned char n4[] = { D_JIT_X86_SEQ_NOP4 };
    static const unsigned char n5[] = { D_JIT_X86_SEQ_NOP5 };
    static const unsigned char n6[] = { D_JIT_X86_SEQ_NOP6 };
    static const unsigned char n7[] = { D_JIT_X86_SEQ_NOP7 };
    static const unsigned char n8[] = { D_JIT_X86_SEQ_NOP8 };
    static const unsigned char n9[] = { D_JIT_X86_SEQ_NOP9 };
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

int d_jit_x86_emit_mov_reg_imm32(d_jit_buffer* _buf, int _reg, uint32_t _imm)
{
    /* B8+rd id */
    uint8_t op = (uint8_t)(D_JIT_X86_OP_MOV_R_IMM32 + (_reg & 7));
    if (d_jit_emit_u8(_buf, op) != 0) { return -1; }
    return d_jit_emit_u32(_buf, _imm);
}

int d_jit_x86_emit_mov_reg_reg(d_jit_buffer* _buf, int _dst, int _src)
{
    /* 89 /r  (r/m <- r, so rm=_dst, reg=_src) */
    uint8_t modrm = (uint8_t)D_JIT_X86_MODRM(D_JIT_X86_MOD_REG, _src, _dst);
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_MOV_RM_R) != 0) { return -1; }
    return d_jit_emit_u8(_buf, modrm);
}

int d_jit_x86_emit_add_reg_imm32(d_jit_buffer* _buf, int _reg, int32_t _imm)
{
    /* 81 /0 id */
    uint8_t modrm = (uint8_t)D_JIT_X86_MODRM(D_JIT_X86_MOD_REG,
                                             D_JIT_X86_GRP1_ADD, _reg);
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP1_RM_IMM32) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, modrm) != 0)                      { return -1; }
    return d_jit_emit_u32(_buf, (uint32_t)_imm);
}

int d_jit_x86_emit_load_arg(d_jit_buffer* _buf, int _reg, int _index)
{
    /* 8B /r with a disp8 off EBP: mov _reg, [ebp + 8 + 4*_index] */
    uint8_t modrm = (uint8_t)D_JIT_X86_MODRM(D_JIT_X86_MOD_DISP8, _reg,
                                             D_JIT_X86_REG_EBP);
    int disp = 8 + 4 * _index;
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_MOV_R_RM) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, modrm) != 0)                 { return -1; }
    return d_jit_emit_u8(_buf, (uint8_t)(disp & 0xFF));
}

const char* d_jit_x86_reg_name(int _reg)
{
    static const char* const names[8] = {
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
    };
    return (_reg >= 0 && _reg < 8) ? names[_reg] : "?";
}
