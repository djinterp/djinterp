/******************************************************************************
* djinterp [jit]                                                     jit_x86.c
*
* djinterp x86 (32-bit) JIT encoder -- implementation (jit_x86.h).
*   A general ModR/M + SIB + displacement encoder drives every operand-taking
* instruction, so register-direct and all memory addressing forms
* ([base + index*scale + disp], absolute, index-only) share one code path.
* Branch emitters record their rel32 with the shared label facility in jit.h.
* Encodings were cross-checked against `as --32` / objdump.
*
* path:      /inc/djinterp/jit/jit_x86.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

// djinterp
#include "jit_x86.h"


// ===========================================================================
// I.   INTERNAL ENCODING HELPERS
// ===========================================================================

// d_jit_x86_fits_int8
//   function (internal): 1 when _v is representable as a signed 8-bit value.
static int d_jit_x86_fits_int8(int32_t _v)
{
    return (_v >= -128 && _v <= 127);
}

// d_jit_x86_emit_rm
//   function (internal): emit the ModR/M byte, plus a SIB byte and
// displacement where the addressing form requires them, for the register or
// opcode-extension digit _reg_field applied to r/m operand _rm. This is the
// core of the encoder; every form below was verified against the assembler.
static int d_jit_x86_emit_rm(d_jit_buffer* _buf, int _reg_field,
                             const d_jit_x86_operand* _rm)
{
    int rf = _reg_field & 7;
    int base, index, scale_field, mod, idx_field;

    if (_rm->kind == D_JIT_X86_KIND_REG) {
        return d_jit_emit_u8(_buf,
            (uint8_t)D_JIT_X86_MODRM(D_JIT_X86_MOD_REG, rf, _rm->base));
    }

    base  = _rm->base;
    index = _rm->index;

    scale_field = D_JIT_X86_SIB_SCALE_1;
    if (index >= 0) {
        switch (_rm->scale) {
            case 1: scale_field = D_JIT_X86_SIB_SCALE_1; break;
            case 2: scale_field = D_JIT_X86_SIB_SCALE_2; break;
            case 4: scale_field = D_JIT_X86_SIB_SCALE_4; break;
            case 8: scale_field = D_JIT_X86_SIB_SCALE_8; break;
            default: return -1;
        }
        if (index == D_JIT_X86_REG_ESP) { return -1; }  /* ESP cannot index */
    }

    /* no base and no index: absolute [disp32] via rm=101, mod=00 */
    if (base < 0 && index < 0) {
        if (d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_MODRM(
                D_JIT_X86_MOD_INDIRECT, rf, D_JIT_X86_RM_DISP32)) != 0) {
            return -1;
        }
        return d_jit_emit_u32(_buf, (uint32_t)_rm->disp);
    }

    /* simple [base + disp] with no index and base != ESP: no SIB byte */
    if (index < 0 && base != D_JIT_X86_REG_ESP) {
        if (_rm->disp == 0 && base != D_JIT_X86_REG_EBP) {
            return d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_MODRM(
                D_JIT_X86_MOD_INDIRECT, rf, base));
        }
        if (d_jit_x86_fits_int8(_rm->disp)) {
            if (d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_MODRM(
                    D_JIT_X86_MOD_DISP8, rf, base)) != 0) { return -1; }
            return d_jit_emit_u8(_buf, (uint8_t)(_rm->disp & 0xFF));
        }
        if (d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_MODRM(
                D_JIT_X86_MOD_DISP32, rf, base)) != 0) { return -1; }
        return d_jit_emit_u32(_buf, (uint32_t)_rm->disp);
    }

    /* a SIB byte is required (there is an index, or the base is ESP) */
    idx_field = (index < 0) ? D_JIT_X86_SIB_INDEX_NONE : (index & 7);

    /* index but no base: [index*scale + disp32], SIB base = 101, mod = 00 */
    if (base < 0) {
        if (d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_MODRM(
                D_JIT_X86_MOD_INDIRECT, rf, D_JIT_X86_RM_SIB)) != 0) {
            return -1;
        }
        if (d_jit_emit_u8(_buf, (uint8_t)D_JIT_X86_SIB(
                scale_field, idx_field, D_JIT_X86_SIB_BASE_NONE)) != 0) {
            return -1;
        }
        return d_jit_emit_u32(_buf, (uint32_t)_rm->disp);
    }

    /* base present (with optional index) */
    if (_rm->disp == 0 && base != D_JIT_X86_REG_EBP) {
        mod = D_JIT_X86_MOD_INDIRECT;
    } else if (d_jit_x86_fits_int8(_rm->disp)) {
        mod = D_JIT_X86_MOD_DISP8;
    } else {
        mod = D_JIT_X86_MOD_DISP32;
    }
    if (d_jit_emit_u8(_buf,
            (uint8_t)D_JIT_X86_MODRM(mod, rf, D_JIT_X86_RM_SIB)) != 0) {
        return -1;
    }
    if (d_jit_emit_u8(_buf,
            (uint8_t)D_JIT_X86_SIB(scale_field, idx_field, base)) != 0) {
        return -1;
    }
    if (mod == D_JIT_X86_MOD_DISP8) {
        return d_jit_emit_u8(_buf, (uint8_t)(_rm->disp & 0xFF));
    }
    if (mod == D_JIT_X86_MOD_DISP32) {
        return d_jit_emit_u32(_buf, (uint32_t)_rm->disp);
    }
    return 0;
}

// d_jit_x86_emit_0f
//   function (internal): emit a two-byte-opcode instruction (0x0F, _op2)
// with reg field _reg and r/m operand _rm.
static int d_jit_x86_emit_0f(d_jit_buffer* _buf, uint8_t _op2, int _reg,
                             const d_jit_x86_operand* _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_TWOBYTE) != 0) { return -1; }
    if (d_jit_emit_u8(_buf, _op2) != 0)                { return -1; }
    return d_jit_x86_emit_rm(_buf, _reg, _rm);
}


// ===========================================================================
// II.  DATA MOVEMENT
// ===========================================================================

int d_jit_x86_emit_mov(d_jit_buffer* _buf, d_jit_x86_operand _dst,
                       d_jit_x86_operand _src)
{
    if (_src.kind == D_JIT_X86_KIND_REG) {          /* 0x89 /r: rm <- reg */
        if (d_jit_emit_u8(_buf, D_JIT_X86_OP_MOV_RM_R) != 0) { return -1; }
        return d_jit_x86_emit_rm(_buf, _src.base, &_dst);
    }
    if (_dst.kind == D_JIT_X86_KIND_REG) {          /* 0x8B /r: reg <- rm */
        if (d_jit_emit_u8(_buf, D_JIT_X86_OP_MOV_R_RM) != 0) { return -1; }
        return d_jit_x86_emit_rm(_buf, _dst.base, &_src);
    }
    return -1;                                       /* mem <- mem: invalid */
}

int d_jit_x86_emit_mov_imm32(d_jit_buffer* _buf, d_jit_x86_operand _dst,
                             uint32_t _imm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_MOV_RM_IMM32) != 0) { return -1; }
    if (d_jit_x86_emit_rm(_buf, 0, &_dst) != 0)             { return -1; }
    return d_jit_emit_u32(_buf, _imm);
}

int d_jit_x86_emit_lea(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _mem)
{
    if (_mem.kind != D_JIT_X86_KIND_MEM)          { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_LEA) != 0) { return -1; }
    return d_jit_x86_emit_rm(_buf, _reg, &_mem);
}

int d_jit_x86_emit_movzx8(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _rm)
{
    return d_jit_x86_emit_0f(_buf, D_JIT_X86_OP2_MOVZX_B, _reg, &_rm);
}

int d_jit_x86_emit_movzx16(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _rm)
{
    return d_jit_x86_emit_0f(_buf, D_JIT_X86_OP2_MOVZX_W, _reg, &_rm);
}

int d_jit_x86_emit_movsx8(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _rm)
{
    return d_jit_x86_emit_0f(_buf, D_JIT_X86_OP2_MOVSX_B, _reg, &_rm);
}

int d_jit_x86_emit_movsx16(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _rm)
{
    return d_jit_x86_emit_0f(_buf, D_JIT_X86_OP2_MOVSX_W, _reg, &_rm);
}

int d_jit_x86_emit_xchg(d_jit_buffer* _buf, d_jit_x86_operand _rm, int _reg)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_XCHG_RM_R) != 0) { return -1; }
    return d_jit_x86_emit_rm(_buf, _reg, &_rm);
}


// ===========================================================================
// III. ARITHMETIC / LOGIC
// ===========================================================================

//   The eight ALU operations occupy fixed opcode blocks: for operation N
// (ADD=0 .. CMP=7) the r/m<-r form is 0x01+8N and the r<-r/m form is 0x03+8N,
// while the immediate forms use group-1 opcodes with /digit = N.

int d_jit_x86_emit_alu(d_jit_buffer* _buf, d_jit_x86_alu _op,
                       d_jit_x86_operand _dst, d_jit_x86_operand _src)
{
    if ((int)_op < 0 || (int)_op > 7) { return -1; }
    if (_src.kind == D_JIT_X86_KIND_REG) {          /* rm <- reg: 0x01+8N */
        if (d_jit_emit_u8(_buf, (uint8_t)((int)_op * 8 + 1)) != 0) {
            return -1;
        }
        return d_jit_x86_emit_rm(_buf, _src.base, &_dst);
    }
    if (_dst.kind == D_JIT_X86_KIND_REG) {          /* reg <- rm: 0x03+8N */
        if (d_jit_emit_u8(_buf, (uint8_t)((int)_op * 8 + 3)) != 0) {
            return -1;
        }
        return d_jit_x86_emit_rm(_buf, _dst.base, &_src);
    }
    return -1;
}

int d_jit_x86_emit_alu_imm32(d_jit_buffer* _buf, d_jit_x86_alu _op,
                             d_jit_x86_operand _dst, int32_t _imm)
{
    if ((int)_op < 0 || (int)_op > 7)                        { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP1_RM_IMM32) != 0) { return -1; }
    if (d_jit_x86_emit_rm(_buf, (int)_op, &_dst) != 0)       { return -1; }
    return d_jit_emit_u32(_buf, (uint32_t)_imm);
}

int d_jit_x86_emit_alu_imm8(d_jit_buffer* _buf, d_jit_x86_alu _op,
                            d_jit_x86_operand _dst, int8_t _imm)
{
    if ((int)_op < 0 || (int)_op > 7)                       { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP1_RM_IMM8) != 0) { return -1; }
    if (d_jit_x86_emit_rm(_buf, (int)_op, &_dst) != 0)      { return -1; }
    return d_jit_emit_u8(_buf, (uint8_t)_imm);
}

int d_jit_x86_emit_test(d_jit_buffer* _buf, d_jit_x86_operand _rm, int _reg)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_TEST_RM_R) != 0) { return -1; }
    return d_jit_x86_emit_rm(_buf, _reg, &_rm);
}

int d_jit_x86_emit_test_imm32(d_jit_buffer* _buf, d_jit_x86_operand _rm,
                              uint32_t _imm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP3) != 0)   { return -1; }  /* /0 */
    if (d_jit_x86_emit_rm(_buf, 0, &_rm) != 0)         { return -1; }
    return d_jit_emit_u32(_buf, _imm);
}

int d_jit_x86_emit_unary(d_jit_buffer* _buf, d_jit_x86_unary _op,
                         d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP3) != 0) { return -1; }
    return d_jit_x86_emit_rm(_buf, (int)_op, &_rm);
}

int d_jit_x86_emit_inc(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP5) != 0) { return -1; }  /* /0 */
    return d_jit_x86_emit_rm(_buf, 0, &_rm);
}

int d_jit_x86_emit_dec(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP5) != 0) { return -1; }  /* /1 */
    return d_jit_x86_emit_rm(_buf, 1, &_rm);
}

int d_jit_x86_emit_imul(d_jit_buffer* _buf, int _reg, d_jit_x86_operand _rm)
{
    return d_jit_x86_emit_0f(_buf, D_JIT_X86_OP2_IMUL_RM, _reg, &_rm);
}

int d_jit_x86_emit_shift(d_jit_buffer* _buf, d_jit_x86_shift _op,
                         d_jit_x86_operand _rm, uint8_t _count)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP2_IMM8) != 0) { return -1; }
    if (d_jit_x86_emit_rm(_buf, (int)_op, &_rm) != 0)     { return -1; }
    return d_jit_emit_u8(_buf, _count);
}

int d_jit_x86_emit_shift_cl(d_jit_buffer* _buf, d_jit_x86_shift _op,
                            d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP2_CL) != 0) { return -1; }
    return d_jit_x86_emit_rm(_buf, (int)_op, &_rm);
}


// ===========================================================================
// IV.  STACK
// ===========================================================================

int d_jit_x86_emit_push_reg(d_jit_buffer* _buf, int _reg)
{
    return d_jit_emit_u8(_buf, (uint8_t)(D_JIT_X86_OP_PUSH_R + (_reg & 7)));
}

int d_jit_x86_emit_pop_reg(d_jit_buffer* _buf, int _reg)
{
    return d_jit_emit_u8(_buf, (uint8_t)(D_JIT_X86_OP_POP_R + (_reg & 7)));
}

int d_jit_x86_emit_push(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP5) != 0) { return -1; }  /* /6 */
    return d_jit_x86_emit_rm(_buf, 6, &_rm);
}

int d_jit_x86_emit_pop(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_POP_RM) != 0) { return -1; }  /* /0 */
    return d_jit_x86_emit_rm(_buf, 0, &_rm);
}

int d_jit_x86_emit_push_imm32(d_jit_buffer* _buf, int32_t _imm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_PUSH_IMM32) != 0) { return -1; }
    return d_jit_emit_u32(_buf, (uint32_t)_imm);
}


// ===========================================================================
// V.   CONTROL FLOW
// ===========================================================================

// d_jit_x86_reloc_rel32
//   function (internal): patch a 4-byte pc-relative displacement at _at so
// the branch reaches _target. x86 measures rel32 from the end of the field
// (_at + 4).
static int d_jit_x86_reloc_rel32(d_jit_buffer* _buf, size_t _at,
                                 size_t _target)
{
    long long disp = (long long)_target - (long long)(_at + 4);
    return d_jit_buffer_patch(_buf, _at, (uint32_t)disp, 4);
}

int d_jit_x86_emit_jmp(d_jit_buffer* _buf, d_jit_label* _target)
{
    size_t at;
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_JMP_REL32) != 0) { return -1; }
    at = _buf->size;
    if (d_jit_emit_u32(_buf, 0) != 0)                     { return -1; }
    return d_jit_label_reference(_buf, _target, at,
                                d_jit_x86_reloc_rel32);
}

int d_jit_x86_emit_jcc(d_jit_buffer* _buf, int _cc, d_jit_label* _target)
{
    size_t at;
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_TWOBYTE) != 0)         { return -1; }
    if (d_jit_emit_u8(_buf, D_JIT_X86_JCC_REL32_OP2(_cc)) != 0) { return -1; }
    at = _buf->size;
    if (d_jit_emit_u32(_buf, 0) != 0)                          { return -1; }
    return d_jit_label_reference(_buf, _target, at,
                                d_jit_x86_reloc_rel32);
}

int d_jit_x86_emit_call(d_jit_buffer* _buf, d_jit_label* _target)
{
    size_t at;
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_CALL_REL32) != 0) { return -1; }
    at = _buf->size;
    if (d_jit_emit_u32(_buf, 0) != 0)                      { return -1; }
    return d_jit_label_reference(_buf, _target, at,
                                d_jit_x86_reloc_rel32);
}

int d_jit_x86_emit_jmp_rm(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP5) != 0) { return -1; }  /* /4 */
    return d_jit_x86_emit_rm(_buf, D_JIT_X86_GRP5_JMP_RM, &_rm);
}

int d_jit_x86_emit_call_rm(d_jit_buffer* _buf, d_jit_x86_operand _rm)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_GRP5) != 0) { return -1; }  /* /2 */
    return d_jit_x86_emit_rm(_buf, D_JIT_X86_GRP5_CALL_RM, &_rm);
}

int d_jit_x86_emit_ret_imm16(d_jit_buffer* _buf, uint16_t _bytes)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_RET_IMM16) != 0) { return -1; }
    return d_jit_emit_u16(_buf, _bytes);
}

int d_jit_x86_emit_setcc(d_jit_buffer* _buf, int _cc, d_jit_x86_operand _rm8)
{
    if (d_jit_emit_u8(_buf, D_JIT_X86_OP_TWOBYTE) != 0) { return -1; }
    if (d_jit_emit_u8(_buf,
            (uint8_t)(D_JIT_X86_OP2_SETCC_RM | (_cc & 0xF))) != 0) {
        return -1;
    }
    return d_jit_x86_emit_rm(_buf, 0, &_rm8);       /* /0 */
}

int d_jit_x86_emit_cdq(d_jit_buffer* _buf)
{
    return d_jit_emit_u8(_buf, D_JIT_X86_OP_CDQ);
}


// ===========================================================================
// VI.  FRAME + CONVENIENCE HELPERS
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
    /* compact B8+rd id form */
    uint8_t op = (uint8_t)(D_JIT_X86_OP_MOV_R_IMM32 + (_reg & 7));
    if (d_jit_emit_u8(_buf, op) != 0) { return -1; }
    return d_jit_emit_u32(_buf, _imm);
}

int d_jit_x86_emit_mov_reg_reg(d_jit_buffer* _buf, int _dst, int _src)
{
    return d_jit_x86_emit_mov(_buf, d_jit_x86_reg(_dst), d_jit_x86_reg(_src));
}

int d_jit_x86_emit_add_reg_imm32(d_jit_buffer* _buf, int _reg, int32_t _imm)
{
    return d_jit_x86_emit_alu_imm32(_buf, D_JIT_X86_ALU_ADD,
                                    d_jit_x86_reg(_reg), _imm);
}

int d_jit_x86_emit_load_arg(d_jit_buffer* _buf, int _reg, int _index)
{
    /* mov _reg, [ebp + 8 + 4*_index] */
    d_jit_x86_operand mem = d_jit_x86_mem(D_JIT_X86_REG_EBP, 8 + 4 * _index);
    return d_jit_x86_emit_mov(_buf, d_jit_x86_reg(_reg), mem);
}

const char* d_jit_x86_reg_name(int _reg)
{
    static const char* const names[8] = {
        "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
    };
    return (_reg >= 0 && _reg < 8) ? names[_reg] : "?";
}
