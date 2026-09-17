/******************************************************************************
* djinterp [jit]                                                     jit_x86.h
*
* djinterp x86 (32-bit) JIT encoder (byte constants + instruction emitters):
*   The IA-32 half of the djinterp JIT, parallel to jit_x64.h but for 32-bit
* protected mode. There is no REX prefix; register operands are the eight
* classic 32-bit registers, and the emitters follow the cdecl convention
* (integer arguments on the stack, result in EAX). Opcode bytes are shared with
* x86-64 where the encoding is identical; this header states them independently
* so the module stands alone. Every literal was verified against `as --32`.
*
*   HOST-INDEPENDENT constants; the emitter FUNCTIONS produce 32-bit code and
* are meaningful on a 32-bit target -- consult D_ENV_JIT_TARGET_X86.
*
*   NAMING CONVENTION:
*     D_JIT_X86_OP_[MNEMONIC] / _PFX_[NAME] / _MOD_* / _SIB_* / _REG_[NAME]
*     D_JIT_X86_CC_[COND]     - condition-code (tttn) value
*     D_JIT_X86_SEQ_[NAME]    - complete, comma-separated byte sequence
*     d_jit_x86_emit_*        - write an encoded instruction into a buffer
*
*   Requires:  jit.h (the d_jit_buffer type and emit primitives).
*
* path:      /inc/djinterp/jit/jit_x86.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_X86_
#define DJINTERP_JIT_X86_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_X86_ENCODING
//   feature: 1, indicating the 32-bit x86 byte-constant table is available.
#ifndef D_JIT_X86_ENCODING
    #define D_JIT_X86_ENCODING 1
#endif


// ===========================================================================
// B.   LEGACY / MANDATORY PREFIXES
// ===========================================================================
//   As on x86-64 minus the segment-base story: in 32-bit mode all six segment
// overrides are usable. 0x66 selects 16-bit operands, 0x67 selects 16-bit
// addresses.

// D_JIT_X86_PFX_OPERAND_SIZE
//   constant: operand-size override (16-bit operands).
#define D_JIT_X86_PFX_OPERAND_SIZE  0x66

// D_JIT_X86_PFX_ADDRESS_SIZE
//   constant: address-size override (16-bit effective address).
#define D_JIT_X86_PFX_ADDRESS_SIZE  0x67

// D_JIT_X86_PFX_LOCK
//   constant: LOCK -- atomic read-modify-write on the following memory op.
#define D_JIT_X86_PFX_LOCK          0xF0

// D_JIT_X86_PFX_REPNE
//   constant: REPNE/REPNZ (also a mandatory SSE prefix).
#define D_JIT_X86_PFX_REPNE         0xF2

// D_JIT_X86_PFX_REP
//   constant: REP/REPE/REPZ (also a mandatory SSE prefix).
#define D_JIT_X86_PFX_REP           0xF3

// D_JIT_X86_PFX_SEG_CS
//   constant: CS segment override.
#define D_JIT_X86_PFX_SEG_CS        0x2E

// D_JIT_X86_PFX_SEG_SS
//   constant: SS segment override.
#define D_JIT_X86_PFX_SEG_SS        0x36

// D_JIT_X86_PFX_SEG_DS
//   constant: DS segment override.
#define D_JIT_X86_PFX_SEG_DS        0x3E

// D_JIT_X86_PFX_SEG_ES
//   constant: ES segment override.
#define D_JIT_X86_PFX_SEG_ES        0x26

// D_JIT_X86_PFX_SEG_FS
//   constant: FS segment override.
#define D_JIT_X86_PFX_SEG_FS        0x64

// D_JIT_X86_PFX_SEG_GS
//   constant: GS segment override.
#define D_JIT_X86_PFX_SEG_GS        0x65


// ===========================================================================
// C.   MODR/M AND SIB FIELDS
// ===========================================================================
//   Identical layout to x86-64 (there is no REX to extend the fields). The
// special encodings differ in one place: rm=101 with mod=00 is a plain disp32
// absolute address in 32-bit mode (x86-64 reinterprets it as RIP-relative).

// ---------------------------------------------------------------------------
// mod field (bits 7-6)
// ---------------------------------------------------------------------------

// D_JIT_X86_MOD_INDIRECT
//   constant: ModR/M.mod = [rm] -- indirect, no displacement.
#define D_JIT_X86_MOD_INDIRECT  0x00

// D_JIT_X86_MOD_DISP8
//   constant: ModR/M.mod = [rm + disp8] -- 8-bit displacement follows.
#define D_JIT_X86_MOD_DISP8     0x01

// D_JIT_X86_MOD_DISP32
//   constant: ModR/M.mod = [rm + disp32] -- 32-bit displacement follows.
#define D_JIT_X86_MOD_DISP32    0x02

// D_JIT_X86_MOD_REG
//   constant: ModR/M.mod = rm -- operand is the register itself.
#define D_JIT_X86_MOD_REG       0x03

// ---------------------------------------------------------------------------
// rm-field special encodings
// ---------------------------------------------------------------------------

// D_JIT_X86_RM_SIB
//   constant: rm=100, mod!=11: a SIB byte follows.
#define D_JIT_X86_RM_SIB        0x04

// D_JIT_X86_RM_DISP32
//   constant: rm=101, mod=00: disp32 absolute address.
#define D_JIT_X86_RM_DISP32     0x05

// D_JIT_X86_MODRM(mod, reg, rm)
//   macro: compose a ModR/M byte (reg may be a register or /digit).
#ifndef D_JIT_X86_MODRM
    #define D_JIT_X86_MODRM(mod, reg, rm)                                    \
        ( (((mod) & 3) << 6) |                                               \
          (((reg) & 7) << 3) |                                               \
          ((rm) & 7) )
#endif

// ---------------------------------------------------------------------------
// SIB scale field and special index/base
// ---------------------------------------------------------------------------

// D_JIT_X86_SIB_SCALE_1
//   constant: index * 1.
#define D_JIT_X86_SIB_SCALE_1       0x00

// D_JIT_X86_SIB_SCALE_2
//   constant: index * 2.
#define D_JIT_X86_SIB_SCALE_2       0x01

// D_JIT_X86_SIB_SCALE_4
//   constant: index * 4.
#define D_JIT_X86_SIB_SCALE_4       0x02

// D_JIT_X86_SIB_SCALE_8
//   constant: index * 8.
#define D_JIT_X86_SIB_SCALE_8       0x03

// D_JIT_X86_SIB_INDEX_NONE
//   constant: index=100: no index register.
#define D_JIT_X86_SIB_INDEX_NONE    0x04

// D_JIT_X86_SIB_BASE_NONE
//   constant: base=101 with mod=00: no base, disp32 only.
#define D_JIT_X86_SIB_BASE_NONE     0x05

// D_JIT_X86_SIB(scale, index, base)
//   macro: compose a SIB byte.
#ifndef D_JIT_X86_SIB
    #define D_JIT_X86_SIB(scale, index, base)                                \
        ( (((scale) & 3) << 6) |                                             \
          (((index) & 7) << 3) |                                             \
          ((base) & 7) )
#endif


// ===========================================================================
// D.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   The eight 32-bit general registers, in the legacy A,C,D,B,SP,BP,SI,DI
// order. The same 3-bit numbers name the 16- and 8-bit registers.

#define D_JIT_X86_REG_EAX   0
#define D_JIT_X86_REG_ECX   1
#define D_JIT_X86_REG_EDX   2
#define D_JIT_X86_REG_EBX   3
#define D_JIT_X86_REG_ESP   4
#define D_JIT_X86_REG_EBP   5
#define D_JIT_X86_REG_ESI   6
#define D_JIT_X86_REG_EDI   7


// ===========================================================================
// E.   ONE-BYTE OPCODES
// ===========================================================================
//   push/pop and the immediate-mov embed the register in the opcode (+rd);
// there is no REX, so only the eight base registers are reachable this way.

// D_JIT_X86_OP_RET
//   constant: near return.
#define D_JIT_X86_OP_RET            0xC3

// D_JIT_X86_OP_RET_IMM16
//   constant: near return, pop imm16 bytes of args (imm16 follows).
#define D_JIT_X86_OP_RET_IMM16      0xC2

// D_JIT_X86_OP_LEAVE
//   constant: tear down stack frame (mov esp,ebp ; pop ebp).
#define D_JIT_X86_OP_LEAVE          0xC9

// D_JIT_X86_OP_NOP
//   constant: one-byte no-op (also XCHG eAX,eAX).
#define D_JIT_X86_OP_NOP            0x90

// D_JIT_X86_OP_INT3
//   constant: breakpoint trap.
#define D_JIT_X86_OP_INT3           0xCC

// D_JIT_X86_OP_HLT
//   constant: halt.
#define D_JIT_X86_OP_HLT            0xF4

// D_JIT_X86_OP_CDQ
//   constant: sign-extend EAX into EDX:EAX.
#define D_JIT_X86_OP_CDQ            0x99

// D_JIT_X86_OP_CWDE
//   constant: sign-extend AX into EAX.
#define D_JIT_X86_OP_CWDE           0x98

// D_JIT_X86_OP_PUSH_R
//   constant: push r32, +rd (0x50 + reg).
#define D_JIT_X86_OP_PUSH_R         0x50

// D_JIT_X86_OP_POP_R
//   constant: pop r32,  +rd (0x58 + reg).
#define D_JIT_X86_OP_POP_R          0x58

// D_JIT_X86_OP_MOV_R_IMM32
//   constant: mov r32, imm32, +rd.
#define D_JIT_X86_OP_MOV_R_IMM32    0xB8

// D_JIT_X86_OP_LEA
//   constant: lea r, m (load effective address).
#define D_JIT_X86_OP_LEA            0x8D

// D_JIT_X86_OP_MOV_RM_R
//   constant: mov r/m, r   (store; /r).
#define D_JIT_X86_OP_MOV_RM_R       0x89

// D_JIT_X86_OP_MOV_R_RM
//   constant: mov r, r/m   (load;  /r).
#define D_JIT_X86_OP_MOV_R_RM       0x8B

// D_JIT_X86_OP_MOV_RM_IMM32
//   constant: mov r/m, imm32, group /0.
#define D_JIT_X86_OP_MOV_RM_IMM32   0xC7

// D_JIT_X86_OP_TEST_RM_R
//   constant: test r/m, r  (/r).
#define D_JIT_X86_OP_TEST_RM_R      0x85

// D_JIT_X86_OP_XCHG_RM_R
//   constant: xchg r/m, r  (/r).
#define D_JIT_X86_OP_XCHG_RM_R      0x87


// ===========================================================================
// F.   TWO-BYTE (0F-MAP) OPCODES
// ===========================================================================

// D_JIT_X86_OP_TWOBYTE
//   constant: two-byte-opcode escape (0F map).
#define D_JIT_X86_OP_TWOBYTE          0x0F

// D_JIT_X86_OP2_UD2
//   constant: 0F 0B: raise #UD.
#define D_JIT_X86_OP2_UD2             0x0B

// D_JIT_X86_OP2_IMUL_RM
//   constant: 0F AF /r: imul r, r/m.
#define D_JIT_X86_OP2_IMUL_RM         0xAF

// D_JIT_X86_OP2_NOP_RM
//   constant: 0F 1F /0: multi-byte NOP form.
#define D_JIT_X86_OP2_NOP_RM          0x1F

// D_JIT_X86_OP2_JCC_REL32
//   constant: 0F 80+cc: jcc rel32.
#define D_JIT_X86_OP2_JCC_REL32       0x80

// D_JIT_X86_OP2_SETCC_RM
//   constant: 0F 90+cc: setcc r/m8.
#define D_JIT_X86_OP2_SETCC_RM        0x90

// D_JIT_X86_OP2_MOVZX_B
//   constant: 0F B6 /r: movzx r, r/m8.
#define D_JIT_X86_OP2_MOVZX_B         0xB6

// D_JIT_X86_OP2_MOVSX_B
//   constant: 0F BE /r: movsx r, r/m8.
#define D_JIT_X86_OP2_MOVSX_B         0xBE


// ===========================================================================
// G.   ALU OPCODES (GROUP-1 IMMEDIATE + REGISTER FORMS)
// ===========================================================================
//   Same group-1 immediate encoding and register-form stride as x86-64.

// ---------------------------------------------------------------------------
// group-1 immediate opcodes (operation chosen by the /digit below)
// ---------------------------------------------------------------------------

// D_JIT_X86_OP_GRP1_RM_IMM8
//   constant: op r/m, imm8  (imm8 sign-extended).
#define D_JIT_X86_OP_GRP1_RM_IMM8     0x83

// D_JIT_X86_OP_GRP1_RM_IMM32
//   constant: op r/m, imm32.
#define D_JIT_X86_OP_GRP1_RM_IMM32    0x81

// D_JIT_X86_OP_GRP1_RM8_IMM8
//   constant: op r/m8, imm8.
#define D_JIT_X86_OP_GRP1_RM8_IMM8    0x80

// group-1 /digit operation selectors
#define D_JIT_X86_GRP1_ADD    0
#define D_JIT_X86_GRP1_OR     1
#define D_JIT_X86_GRP1_ADC    2
#define D_JIT_X86_GRP1_SBB    3
#define D_JIT_X86_GRP1_AND    4
#define D_JIT_X86_GRP1_SUB    5
#define D_JIT_X86_GRP1_XOR    6
#define D_JIT_X86_GRP1_CMP    7

// ---------------------------------------------------------------------------
// register/memory forms: r/m <- r direction (/r). +2 gives r <- r/m.
// ---------------------------------------------------------------------------

// D_JIT_X86_OP_ADD_RM_R
//   constant: add r/m, r  (r,r/m = 0x03).
#define D_JIT_X86_OP_ADD_RM_R     0x01

// D_JIT_X86_OP_OR_RM_R
//   constant: or  r/m, r  (r,r/m = 0x0B).
#define D_JIT_X86_OP_OR_RM_R      0x09

// D_JIT_X86_OP_AND_RM_R
//   constant: and r/m, r  (r,r/m = 0x23).
#define D_JIT_X86_OP_AND_RM_R     0x21

// D_JIT_X86_OP_SUB_RM_R
//   constant: sub r/m, r  (r,r/m = 0x2B).
#define D_JIT_X86_OP_SUB_RM_R     0x29

// D_JIT_X86_OP_XOR_RM_R
//   constant: xor r/m, r  (the xor r,r zero-idiom).
#define D_JIT_X86_OP_XOR_RM_R     0x31

// D_JIT_X86_OP_CMP_RM_R
//   constant: cmp r/m, r  (r,r/m = 0x3B).
#define D_JIT_X86_OP_CMP_RM_R     0x39


// ===========================================================================
// H.   CONTROL TRANSFER + CONDITION CODES
// ===========================================================================

// D_JIT_X86_OP_CALL_REL32
//   constant: call rel32 (disp32 from end of instruction).
#define D_JIT_X86_OP_CALL_REL32   0xE8

// D_JIT_X86_OP_JMP_REL32
//   constant: jmp  rel32.
#define D_JIT_X86_OP_JMP_REL32    0xE9

// D_JIT_X86_OP_JMP_REL8
//   constant: jmp  rel8  (short).
#define D_JIT_X86_OP_JMP_REL8     0xEB

// D_JIT_X86_OP_JCC_REL8
//   constant: jcc  rel8, +cc.
#define D_JIT_X86_OP_JCC_REL8     0x70

// D_JIT_X86_OP_GRP5
//   constant: group-5: indirect call/jmp/push.
#define D_JIT_X86_OP_GRP5         0xFF

// group-5 /digit selectors
// D_JIT_X86_GRP5_CALL_RM: call r/m32
#define D_JIT_X86_GRP5_CALL_RM    2
// D_JIT_X86_GRP5_JMP_RM: jmp r/m32
#define D_JIT_X86_GRP5_JMP_RM     4
// D_JIT_X86_GRP5_PUSH_RM: push r/m32
#define D_JIT_X86_GRP5_PUSH_RM    6

// ---------------------------------------------------------------------------
// condition codes (tttn) -- add to 0x70 (Jcc rel8) or 0x0F80 (Jcc rel32)
// ---------------------------------------------------------------------------

// D_JIT_X86_CC_O: overflow
#define D_JIT_X86_CC_O    0x0
// D_JIT_X86_CC_NO: no overflow
#define D_JIT_X86_CC_NO   0x1
// D_JIT_X86_CC_B: below / carry
#define D_JIT_X86_CC_B    0x2
// D_JIT_X86_CC_AE: above-or-equal / no-carry
#define D_JIT_X86_CC_AE   0x3
// D_JIT_X86_CC_E: equal / zero
#define D_JIT_X86_CC_E    0x4
// D_JIT_X86_CC_NE: not-equal / not-zero
#define D_JIT_X86_CC_NE   0x5
// D_JIT_X86_CC_BE: below-or-equal
#define D_JIT_X86_CC_BE   0x6
// D_JIT_X86_CC_A: above
#define D_JIT_X86_CC_A    0x7
// D_JIT_X86_CC_S: sign
#define D_JIT_X86_CC_S    0x8
// D_JIT_X86_CC_NS: no sign
#define D_JIT_X86_CC_NS   0x9
// D_JIT_X86_CC_P: parity-even
#define D_JIT_X86_CC_P    0xA
// D_JIT_X86_CC_NP: parity-odd
#define D_JIT_X86_CC_NP   0xB
// D_JIT_X86_CC_L: less (signed)
#define D_JIT_X86_CC_L    0xC
// D_JIT_X86_CC_GE: greater-or-equal (signed)
#define D_JIT_X86_CC_GE   0xD
// D_JIT_X86_CC_LE: less-or-equal (signed)
#define D_JIT_X86_CC_LE   0xE
// D_JIT_X86_CC_G: greater (signed)
#define D_JIT_X86_CC_G    0xF

// D_JIT_X86_JCC_REL8(cc)
//   macro: the one-byte opcode for `jcc rel8` with condition code cc.
#ifndef D_JIT_X86_JCC_REL8
    #define D_JIT_X86_JCC_REL8(cc)  (D_JIT_X86_OP_JCC_REL8 | ((cc) & 0xF))
#endif

// D_JIT_X86_JCC_REL32_OP2(cc)
//   macro: the SECOND opcode byte for `jcc rel32` (follows 0x0F) with cc.
#ifndef D_JIT_X86_JCC_REL32_OP2
    #define D_JIT_X86_JCC_REL32_OP2(cc)                                      \
        (D_JIT_X86_OP2_JCC_REL32 | ((cc) & 0xF))
#endif


// ===========================================================================
// I.   COMPLETE BYTE SEQUENCES
// ===========================================================================
//   32-bit fixed fragments. The prologue/epilogue use EBP; the first cdecl
// integer argument sits at [EBP+8] once the frame is set up.

// D_JIT_X86_SEQ_PROLOGUE
//   constant: push ebp ; mov ebp, esp   (frame setup).
#define D_JIT_X86_SEQ_PROLOGUE        0x55, 0x89, 0xE5

// D_JIT_X86_SEQ_EPILOGUE
//   constant: pop ebp ; ret             (frame teardown).
#define D_JIT_X86_SEQ_EPILOGUE        0x5D, 0xC3

// D_JIT_X86_SEQ_EPILOGUE_LEAVE
//   constant: leave ; ret.
#define D_JIT_X86_SEQ_EPILOGUE_LEAVE  0xC9, 0xC3

// D_JIT_X86_SEQ_RET
//   constant: ret.
#define D_JIT_X86_SEQ_RET             0xC3

// D_JIT_X86_SEQ_TRAP
//   constant: int3 (breakpoint / trap padding).
#define D_JIT_X86_SEQ_TRAP            0xCC

// canonical multi-byte NOPs (1-9 bytes; same encodings as 64-bit mode)
#define D_JIT_X86_SEQ_NOP1  0x90
#define D_JIT_X86_SEQ_NOP2  0x66, 0x90
#define D_JIT_X86_SEQ_NOP3  0x0F, 0x1F, 0x00
#define D_JIT_X86_SEQ_NOP4  0x0F, 0x1F, 0x40, 0x00
#define D_JIT_X86_SEQ_NOP5  0x0F, 0x1F, 0x44, 0x00, 0x00
#define D_JIT_X86_SEQ_NOP6  0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00
#define D_JIT_X86_SEQ_NOP7  0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00
#define D_JIT_X86_SEQ_NOP8  0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00
#define D_JIT_X86_SEQ_NOP9                                                   \
    0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00


// ===========================================================================
// J.   INSTRUCTION EMITTERS
// ===========================================================================
//   A minimal cdecl-oriented encoder set: write one 32-bit instruction into a
// d_jit_buffer. Arguments are D_JIT_X86_REG_* numbers. Each returns 0 on
// success or -1 if the underlying buffer emit fails.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// d_jit_x86_emit_prologue
//   function: push ebp ; mov ebp, esp  (D_JIT_X86_SEQ_PROLOGUE).
D_NODISCARD int d_jit_x86_emit_prologue(d_jit_buffer* _buf);

// d_jit_x86_emit_epilogue
//   function: pop ebp ; ret  (D_JIT_X86_SEQ_EPILOGUE).
D_NODISCARD int d_jit_x86_emit_epilogue(d_jit_buffer* _buf);

// d_jit_x86_emit_ret
//   function: ret (near return).
D_NODISCARD int d_jit_x86_emit_ret(d_jit_buffer* _buf);

// d_jit_x86_emit_nop
//   function: emit exactly _n bytes of canonical multi-byte NOP padding.
D_NODISCARD int d_jit_x86_emit_nop(d_jit_buffer* _buf, size_t _n);

// d_jit_x86_emit_mov_reg_imm32
//   function: mov r32, imm32 (B8+rd id).
D_NODISCARD int d_jit_x86_emit_mov_reg_imm32(d_jit_buffer* _buf,
                                              int _reg, uint32_t _imm);

// d_jit_x86_emit_mov_reg_reg
//   function: mov _dst, _src  (0x89 /r).
D_NODISCARD int d_jit_x86_emit_mov_reg_reg(d_jit_buffer* _buf,
                                            int _dst, int _src);

// d_jit_x86_emit_add_reg_imm32
//   function: add r32, imm32 (0x81 /0 id).
D_NODISCARD int d_jit_x86_emit_add_reg_imm32(d_jit_buffer* _buf,
                                             int _reg, int32_t _imm);

// d_jit_x86_emit_load_arg
//   function: mov _reg, [ebp + 8 + 4*_index]  -- load the _index-th cdecl
// integer argument (0-based) into a register, given a standard EBP frame
// (emit the prologue first).
D_NODISCARD int d_jit_x86_emit_load_arg(d_jit_buffer* _buf,
                                        int _reg, int _index);

// d_jit_x86_reg_name
//   function: the lowercase 32-bit name ("eax".."edi"), else "?".
const char*     d_jit_x86_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_X86_
