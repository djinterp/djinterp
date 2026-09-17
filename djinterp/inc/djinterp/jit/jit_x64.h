/******************************************************************************
* djinterp [jit]                                                     jit_x64.h
*
* djinterp x86-64 JIT encoder (byte constants + instruction emitters):
*   The x86-64 (AMD64 / Intel 64) half of the djinterp JIT. It owns the raw
* machine-code vocabulary -- legacy prefixes, the REX prefix and its fields,
* ModR/M and SIB layout, register numbers, one/two-byte opcodes, the ALU
* immediate group, control-transfer opcodes and condition codes, and complete
* canonical byte sequences -- and a small set of instruction emitters that
* write those bytes into a d_jit_buffer from jit.h.
*
*   These constants moved here from env/jit/env_jit_x64.h: they describe the
* ISA, not the environment, so they belong with the encoder, not the detector.
* Every literal was cross-checked against assembled GNU as / objdump output.
*
*   HOST-INDEPENDENT constants: the byte values are properties of the x86-64
* ISA and are defined unconditionally (guarded only by `#ifndef` for override).
* The emitter FUNCTIONS, by contrast, produce x86-64 code and are meaningful
* only on an x86-64 target -- consult D_ENV_JIT_TARGET_X64.
*
*   NAMING CONVENTION:
*     D_JIT_X64_OP_[MNEMONIC]   - an opcode byte (or opcode-group byte)
*     D_JIT_X64_PFX_[NAME]      - a prefix byte
*     D_JIT_X64_REX[_...]       - REX prefix bytes / field bit masks
*     D_JIT_X64_MOD_* / _RM_* / _SIB_* - ModR/M and SIB field values
*     D_JIT_X64_REG_[NAME]      - a register operand number (0-15)
*     D_JIT_X64_CC_[COND]       - a condition-code (tttn) value
*     D_JIT_X64_SEQ_[NAME]      - a complete, comma-separated byte sequence
*     d_jit_x64_emit_*          - write an encoded instruction into a buffer
*
*   Requires:  jit.h (the d_jit_buffer type and emit primitives).
*
* path:      /inc/djinterp/jit/jit_x64.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.16
******************************************************************************/

#ifndef DJINTERP_JIT_X64_
#define DJINTERP_JIT_X64_ 1

// djinterp
#include "jit.h"


// ===========================================================================
// A.   MODULE MARKER
// ===========================================================================
// D_JIT_X64_ENCODING
//   feature: 1, indicating the x86-64 byte-constant table is available.
#ifndef D_JIT_X64_ENCODING
    #define D_JIT_X64_ENCODING 1
#endif


// ===========================================================================
// B.   LEGACY / MANDATORY PREFIXES
// ===========================================================================
//   Single-byte prefixes that precede the opcode. The operand-size prefix
// (0x66) selects 16-bit operands in 64-bit mode and is also the mandatory
// prefix for parts of the SSE2 map; the address-size prefix (0x67) selects
// 32-bit effective addresses. LOCK, REP/REPE, and REPNE are the group-1
// prefixes; the six segment overrides are group 2 (rarely needed in flat
// 64-bit code, where only FS and GS carry a base).

// D_JIT_X64_PFX_OPERAND_SIZE
//   constant: operand-size override (16-bit operands; SSE2 mandatory pfx).
#define D_JIT_X64_PFX_OPERAND_SIZE  0x66

// D_JIT_X64_PFX_ADDRESS_SIZE
//   constant: address-size override (32-bit effective address).
#define D_JIT_X64_PFX_ADDRESS_SIZE  0x67

// D_JIT_X64_PFX_LOCK
//   constant: LOCK -- atomic read-modify-write on the following memory op.
#define D_JIT_X64_PFX_LOCK          0xF0

// D_JIT_X64_PFX_REPNE
//   constant: REPNE/REPNZ (also a mandatory SSE prefix).
#define D_JIT_X64_PFX_REPNE         0xF2

// D_JIT_X64_PFX_REP
//   constant: REP/REPE/REPZ (also a mandatory SSE prefix).
#define D_JIT_X64_PFX_REP           0xF3

// D_JIT_X64_PFX_SEG_CS
//   constant: CS segment override (also branch-not-taken hint).
#define D_JIT_X64_PFX_SEG_CS        0x2E

// D_JIT_X64_PFX_SEG_SS
//   constant: SS segment override.
#define D_JIT_X64_PFX_SEG_SS        0x36

// D_JIT_X64_PFX_SEG_DS
//   constant: DS segment override (also branch-taken hint).
#define D_JIT_X64_PFX_SEG_DS        0x3E

// D_JIT_X64_PFX_SEG_ES
//   constant: ES segment override.
#define D_JIT_X64_PFX_SEG_ES        0x26

// D_JIT_X64_PFX_SEG_FS
//   constant: FS segment override.
#define D_JIT_X64_PFX_SEG_FS        0x64

// D_JIT_X64_PFX_SEG_GS
//   constant: GS segment override.
#define D_JIT_X64_PFX_SEG_GS        0x65


// ===========================================================================
// C.   REX PREFIX
// ===========================================================================
//   REX is a single optional byte 0100WRXB that must immediately precede the
// opcode (after any legacy prefixes). W promotes the operation to 64-bit;
// R/X/B are the high (4th) bits of the ModR/M.reg, SIB.index, and ModR/M.rm
// (or SIB.base / opcode-embedded) register fields respectively. A bare REX
// (0x40) is also what makes the SPL/BPL/SIL/DIL byte registers addressable.

// D_JIT_X64_REX_BASE
//   constant: the REX prefix with no bits set (0100_0000).
#define D_JIT_X64_REX_BASE      0x40

// D_JIT_X64_REX_W
//   constant: REX bit mask -- operand-size = 64-bit.
#define D_JIT_X64_REX_W      0x08

// D_JIT_X64_REX_R
//   constant: REX bit mask -- extends ModR/M.reg to 4 bits.
#define D_JIT_X64_REX_R      0x04

// D_JIT_X64_REX_X
//   constant: REX bit mask -- extends SIB.index to 4 bits.
#define D_JIT_X64_REX_X      0x02

// D_JIT_X64_REX_B
//   constant: REX bit mask -- extends rm / base / opcode-reg field to 4 bits.
#define D_JIT_X64_REX_B      0x01

// D_JIT_X64_REX(w, r, x, b)
//   macro: compose a REX prefix byte from its four 0/1 bit arguments.
#ifndef D_JIT_X64_REX
    #define D_JIT_X64_REX(w, r, x, b)                                        \
        ( D_JIT_X64_REX_BASE       |                                         \
          (((w) & 1) << 3) |                                                 \
          (((r) & 1) << 2) |                                                 \
          (((x) & 1) << 1) |                                                 \
          ((b) & 1) )
#endif

// common concrete REX bytes (for the frequent operand-size-64 cases)
// D_JIT_X64_REX_W_BYTE
//   constant: REX.W                 -- 64-bit operands.
#define D_JIT_X64_REX_W_BYTE     0x48

// D_JIT_X64_REX_B_BYTE
//   constant: REX.B                 -- rm/base/opcode-reg high bit.
#define D_JIT_X64_REX_B_BYTE     0x41

// D_JIT_X64_REX_X_BYTE
//   constant: REX.X                 -- index high bit.
#define D_JIT_X64_REX_X_BYTE     0x42

// D_JIT_X64_REX_R_BYTE
//   constant: REX.R                 -- reg high bit.
#define D_JIT_X64_REX_R_BYTE     0x44

// D_JIT_X64_REX_WB_BYTE
//   constant: REX.W|B.
#define D_JIT_X64_REX_WB_BYTE    0x49

// D_JIT_X64_REX_WR_BYTE
//   constant: REX.W|R.
#define D_JIT_X64_REX_WR_BYTE    0x4C

// D_JIT_X64_REX_WX_BYTE
//   constant: REX.W|X.
#define D_JIT_X64_REX_WX_BYTE    0x4A

// D_JIT_X64_REX_WRB_BYTE
//   constant: REX.W|R|B.
#define D_JIT_X64_REX_WRB_BYTE   0x4D

// D_JIT_X64_REX_WRXB_BYTE
//   constant: REX.W|R|X|B (all bits set).
#define D_JIT_X64_REX_WRXB_BYTE  0x4F


// ===========================================================================
// D.   MODR/M AND SIB FIELDS
// ===========================================================================
//   The ModR/M byte is mod(2) reg(3) rm(3); SIB is scale(2) index(3) base(3).
// `reg` is either a register operand or an opcode extension digit (the `/n`
// in Intel syntax). Three encodings are special: rm=100 with mod!=11 means
// "a SIB byte follows"; rm=101 with mod=00 means RIP-relative disp32 (there is
// no [rbp] base without a displacement); in the SIB byte index=100 means "no
// index" and base=101 with mod=00 means "no base, disp32 only".

// ---------------------------------------------------------------------------
// mod field (bits 7-6)
// ---------------------------------------------------------------------------

// D_JIT_X64_MOD_INDIRECT
//   constant: ModR/M.mod = [rm] -- indirect, no displacement.
#define D_JIT_X64_MOD_INDIRECT  0x00

// D_JIT_X64_MOD_DISP8
//   constant: ModR/M.mod = [rm + disp8] -- 8-bit displacement follows.
#define D_JIT_X64_MOD_DISP8     0x01

// D_JIT_X64_MOD_DISP32
//   constant: ModR/M.mod = [rm + disp32] -- 32-bit displacement follows.
#define D_JIT_X64_MOD_DISP32    0x02

// D_JIT_X64_MOD_REG
//   constant: ModR/M.mod = rm -- operand is the register itself.
#define D_JIT_X64_MOD_REG       0x03

// ---------------------------------------------------------------------------
// rm-field special encodings
// ---------------------------------------------------------------------------

// D_JIT_X64_RM_SIB
//   constant: rm=100, mod!=11: a SIB byte follows.
#define D_JIT_X64_RM_SIB            0x04

// D_JIT_X64_RM_RIP_DISP32
//   constant: rm=101, mod=00: RIP-relative disp32.
#define D_JIT_X64_RM_RIP_DISP32     0x05

// D_JIT_X64_MODRM(mod, reg, rm)
//   macro: compose a ModR/M byte. `reg` may be a register number or an
// opcode-extension digit; only the low 3 bits of reg/rm are used (their 4th
// bits live in REX.R / REX.B).
#ifndef D_JIT_X64_MODRM
    #define D_JIT_X64_MODRM(mod, reg, rm)                                    \
        ( (((mod) & 3) << 6) |                                               \
          (((reg) & 7) << 3) |                                               \
          ((rm) & 7) )
#endif

// ---------------------------------------------------------------------------
// SIB scale field (bits 7-6) and special index/base
// ---------------------------------------------------------------------------

// D_JIT_X64_SIB_SCALE_1
//   constant: index * 1.
#define D_JIT_X64_SIB_SCALE_1       0x00

// D_JIT_X64_SIB_SCALE_2
//   constant: index * 2.
#define D_JIT_X64_SIB_SCALE_2       0x01

// D_JIT_X64_SIB_SCALE_4
//   constant: index * 4.
#define D_JIT_X64_SIB_SCALE_4       0x02

// D_JIT_X64_SIB_SCALE_8
//   constant: index * 8.
#define D_JIT_X64_SIB_SCALE_8       0x03

// D_JIT_X64_SIB_INDEX_NONE
//   constant: index=100: no index register.
#define D_JIT_X64_SIB_INDEX_NONE    0x04

// D_JIT_X64_SIB_BASE_NONE
//   constant: base=101 with mod=00: no base, disp32 only.
#define D_JIT_X64_SIB_BASE_NONE     0x05

// D_JIT_X64_SIB(scale, index, base)
//   macro: compose a SIB byte from the scale field (use D_JIT_X64_SIB_SCALE_*)
// and the low 3 bits of the index and base register numbers.
#ifndef D_JIT_X64_SIB
    #define D_JIT_X64_SIB(scale, index, base)                                \
        ( (((scale) & 3) << 6) |                                             \
          (((index) & 7) << 3) |                                             \
          ((base) & 7) )
#endif


// ===========================================================================
// E.   REGISTER OPERAND NUMBERS
// ===========================================================================
//   The 4-bit encoding number for each 64-bit general register. The low 3
// bits go into a ModR/M or SIB field; bit 3 (r8-r15) is supplied by a REX
// R/X/B bit. The 32/16/8-bit registers share these same numbers. Note the
// legacy order is A, C, D, B, SP, BP, SI, DI -- not alphabetical.

#define D_JIT_X64_REG_RAX   0
#define D_JIT_X64_REG_RCX   1
#define D_JIT_X64_REG_RDX   2
#define D_JIT_X64_REG_RBX   3
#define D_JIT_X64_REG_RSP   4
#define D_JIT_X64_REG_RBP   5
#define D_JIT_X64_REG_RSI   6
#define D_JIT_X64_REG_RDI   7
#define D_JIT_X64_REG_R8    8
#define D_JIT_X64_REG_R9    9
#define D_JIT_X64_REG_R10   10
#define D_JIT_X64_REG_R11   11
#define D_JIT_X64_REG_R12   12
#define D_JIT_X64_REG_R13   13
#define D_JIT_X64_REG_R14   14
#define D_JIT_X64_REG_R15   15


// ===========================================================================
// F.   ONE-BYTE OPCODES
// ===========================================================================
//   Opcodes that are a single byte. The push/pop and immediate-mov forms
// embed the low 3 bits of the register in the opcode itself ("+rd"); add the
// register number and supply REX.B for r8-r15. CQO and a 64-bit CWDE-family
// sign-extension need a REX.W prefix (see the notes on each).

// D_JIT_X64_OP_RET
//   constant: near return.
#define D_JIT_X64_OP_RET            0xC3

// D_JIT_X64_OP_RET_IMM16
//   constant: near return, pop imm16 bytes of args (imm16 follows).
#define D_JIT_X64_OP_RET_IMM16      0xC2

// D_JIT_X64_OP_LEAVE
//   constant: tear down stack frame (mov rsp,rbp ; pop rbp).
#define D_JIT_X64_OP_LEAVE          0xC9

// D_JIT_X64_OP_NOP
//   constant: one-byte no-op (also XCHG eAX,eAX).
#define D_JIT_X64_OP_NOP            0x90

// D_JIT_X64_OP_INT3
//   constant: breakpoint trap (ideal padding for guard bytes).
#define D_JIT_X64_OP_INT3           0xCC

// D_JIT_X64_OP_HLT
//   constant: halt.
#define D_JIT_X64_OP_HLT            0xF4

// D_JIT_X64_OP_CDQ
//   constant: sign-extend EAX into EDX:EAX.
#define D_JIT_X64_OP_CDQ            0x99

// D_JIT_X64_OP_CQO
//   constant: with REX.W: sign-extend RAX into RDX:RAX.
#define D_JIT_X64_OP_CQO            0x99

// D_JIT_X64_OP_CWDE
//   constant: sign-extend AX into EAX (CDQE with REX.W).
#define D_JIT_X64_OP_CWDE           0x98

// D_JIT_X64_OP_PUSH_R
//   constant: push r64, +rd (0x50 + reg&7; REX.B for r8-r15).
#define D_JIT_X64_OP_PUSH_R         0x50

// D_JIT_X64_OP_POP_R
//   constant: pop r64,  +rd (0x58 + reg&7; REX.B for r8-r15).
#define D_JIT_X64_OP_POP_R          0x58

// D_JIT_X64_OP_MOV_R_IMM32
//   constant: mov r32, imm32, +rd (with REX.W: mov r64, imm64).
#define D_JIT_X64_OP_MOV_R_IMM32    0xB8

// D_JIT_X64_OP_MOVSXD
//   constant: movsxd r64, r/m32 (sign-extend; needs REX.W for 64-bit dst).
#define D_JIT_X64_OP_MOVSXD         0x63

// D_JIT_X64_OP_LEA
//   constant: lea r, m (load effective address).
#define D_JIT_X64_OP_LEA            0x8D

// D_JIT_X64_OP_MOV_RM_R
//   constant: mov r/m, r   (store; /r).
#define D_JIT_X64_OP_MOV_RM_R       0x89

// D_JIT_X64_OP_MOV_R_RM
//   constant: mov r, r/m   (load;  /r).
#define D_JIT_X64_OP_MOV_R_RM       0x8B

// D_JIT_X64_OP_MOV_RM_IMM32
//   constant: mov r/m, imm32, group /0 (sign-extended to 64 w/ REX.W).
#define D_JIT_X64_OP_MOV_RM_IMM32   0xC7

// D_JIT_X64_OP_TEST_RM_R
//   constant: test r/m, r  (/r).
#define D_JIT_X64_OP_TEST_RM_R      0x85

// D_JIT_X64_OP_XCHG_RM_R
//   constant: xchg r/m, r  (/r).
#define D_JIT_X64_OP_XCHG_RM_R      0x87


// ===========================================================================
// G.   TWO-BYTE (0F-MAP) OPCODES + MULTI-BYTE NOPS
// ===========================================================================
//   Opcodes introduced by the 0x0F escape byte, plus the canonical multi-byte
// NOP forms (0F 1F ...) an assembler uses to pad to alignment with a single
// instruction. The NOP1..NOP9 sequences appear as complete byte lists in the
// SEQUENCES section below; the opcode roots are given here.

// D_JIT_X64_OP_TWOBYTE
//   constant: two-byte-opcode escape (0F map).
#define D_JIT_X64_OP_TWOBYTE          0x0F

// D_JIT_X64_OP2_UD2
//   constant: 0F 0B: raise #UD (guaranteed-undefined trap).
#define D_JIT_X64_OP2_UD2             0x0B

// D_JIT_X64_OP2_IMUL_RM
//   constant: 0F AF /r: imul r, r/m (two-operand signed multiply).
#define D_JIT_X64_OP2_IMUL_RM         0xAF

// D_JIT_X64_OP2_NOP_RM
//   constant: 0F 1F /0: the multi-byte NOP (hint-nop) form.
#define D_JIT_X64_OP2_NOP_RM          0x1F

// D_JIT_X64_OP2_JCC_REL32
//   constant: 0F 80+cc: jcc rel32 (add a condition code; see section I).
#define D_JIT_X64_OP2_JCC_REL32       0x80

// D_JIT_X64_OP2_SETCC_RM
//   constant: 0F 90+cc: setcc r/m8 (add a condition code).
#define D_JIT_X64_OP2_SETCC_RM        0x90

// D_JIT_X64_OP2_CMOVCC_R_RM
//   constant: 0F 40+cc: cmovcc r, r/m (add a condition code).
#define D_JIT_X64_OP2_CMOVCC_R_RM     0x40

// D_JIT_X64_OP2_MOVZX_B
//   constant: 0F B6 /r: movzx r, r/m8 (zero-extend byte).
#define D_JIT_X64_OP2_MOVZX_B         0xB6

// D_JIT_X64_OP2_MOVZX_W
//   constant: 0F B7 /r: movzx r, r/m16 (zero-extend word).
#define D_JIT_X64_OP2_MOVZX_W         0xB7

// D_JIT_X64_OP2_MOVSX_B
//   constant: 0F BE /r: movsx r, r/m8 (sign-extend byte).
#define D_JIT_X64_OP2_MOVSX_B         0xBE

// D_JIT_X64_OP2_MOVSX_W
//   constant: 0F BF /r: movsx r, r/m16 (sign-extend word).
#define D_JIT_X64_OP2_MOVSX_W         0xBF


// ===========================================================================
// H.   ALU OPCODES (GROUP-1 IMMEDIATE + REGISTER FORMS)
// ===========================================================================
//   The eight basic ALU operations share a group-1 immediate encoding whose
// operation is selected by the ModR/M.reg extension digit (0-7), and a family
// of two-operand register/memory opcodes at a fixed stride. For the register
// forms, the r/m<-r direction and the r<-r/m direction differ by +2, and the
// 8-bit variant is one below the 32/64-bit variant.

// ---------------------------------------------------------------------------
// group-1 immediate opcodes (operation chosen by the /digit below)
// ---------------------------------------------------------------------------

// D_JIT_X64_OP_GRP1_RM_IMM8
//   constant: op r/m, imm8  (imm8 sign-extended to operand size).
#define D_JIT_X64_OP_GRP1_RM_IMM8     0x83

// D_JIT_X64_OP_GRP1_RM_IMM32
//   constant: op r/m, imm32 (imm32 sign-extended w/ REX.W).
#define D_JIT_X64_OP_GRP1_RM_IMM32    0x81

// D_JIT_X64_OP_GRP1_RM8_IMM8
//   constant: op r/m8, imm8.
#define D_JIT_X64_OP_GRP1_RM8_IMM8    0x80

// group-1 /digit operation selectors (ModR/M.reg extension for 0x80/81/83)
#define D_JIT_X64_GRP1_ADD    0
#define D_JIT_X64_GRP1_OR     1
#define D_JIT_X64_GRP1_ADC    2
#define D_JIT_X64_GRP1_SBB    3
#define D_JIT_X64_GRP1_AND    4
#define D_JIT_X64_GRP1_SUB    5
#define D_JIT_X64_GRP1_XOR    6
#define D_JIT_X64_GRP1_CMP    7

// ---------------------------------------------------------------------------
// register/memory forms: r/m <- r direction (/r). +2 gives r <- r/m.
// ---------------------------------------------------------------------------

// D_JIT_X64_OP_ADD_RM_R
//   constant: add r/m, r  (r,r/m = 0x03; r/m8,r8 = 0x00).
#define D_JIT_X64_OP_ADD_RM_R     0x01

// D_JIT_X64_OP_OR_RM_R
//   constant: or  r/m, r  (r,r/m = 0x0B; r/m8,r8 = 0x08).
#define D_JIT_X64_OP_OR_RM_R      0x09

// D_JIT_X64_OP_ADC_RM_R
//   constant: adc r/m, r  (r,r/m = 0x13; r/m8,r8 = 0x10).
#define D_JIT_X64_OP_ADC_RM_R     0x11

// D_JIT_X64_OP_SBB_RM_R
//   constant: sbb r/m, r  (r,r/m = 0x1B; r/m8,r8 = 0x18).
#define D_JIT_X64_OP_SBB_RM_R     0x19

// D_JIT_X64_OP_AND_RM_R
//   constant: and r/m, r  (r,r/m = 0x23; r/m8,r8 = 0x20).
#define D_JIT_X64_OP_AND_RM_R     0x21

// D_JIT_X64_OP_SUB_RM_R
//   constant: sub r/m, r  (r,r/m = 0x2B; r/m8,r8 = 0x28).
#define D_JIT_X64_OP_SUB_RM_R     0x29

// D_JIT_X64_OP_XOR_RM_R
//   constant: xor r/m, r  (r,r/m = 0x33; the xor r,r zero-idiom).
#define D_JIT_X64_OP_XOR_RM_R     0x31

// D_JIT_X64_OP_CMP_RM_R
//   constant: cmp r/m, r  (r,r/m = 0x3B; r/m8,r8 = 0x38).
#define D_JIT_X64_OP_CMP_RM_R     0x39


// ===========================================================================
// I.   CONTROL TRANSFER + CONDITION CODES
// ===========================================================================
//   Relative branches take a signed displacement measured from the END of the
// branch instruction. Jcc has a 2-byte rel8 form (0x70+cc) and a 6-byte rel32
// form (0F 80+cc). Indirect call/jmp through a register or memory use the
// group-5 opcode 0xFF with reg-extension /2 (call) or /4 (jmp). The condition
// codes (tttn) below index every conditional; aliases share a value.

// D_JIT_X64_OP_CALL_REL32
//   constant: call rel32 (disp32 from end of instruction).
#define D_JIT_X64_OP_CALL_REL32   0xE8

// D_JIT_X64_OP_JMP_REL32
//   constant: jmp  rel32.
#define D_JIT_X64_OP_JMP_REL32    0xE9

// D_JIT_X64_OP_JMP_REL8
//   constant: jmp  rel8  (short).
#define D_JIT_X64_OP_JMP_REL8     0xEB

// D_JIT_X64_OP_JCC_REL8
//   constant: jcc  rel8, +cc (0x70 + condition code).
#define D_JIT_X64_OP_JCC_REL8     0x70

// D_JIT_X64_OP_GRP5
//   constant: group-5: indirect call/jmp/push (see /digits).
#define D_JIT_X64_OP_GRP5         0xFF

// group-5 /digit selectors (ModR/M.reg extension for 0xFF)
// D_JIT_X64_GRP5_CALL_RM: call r/m64 (near, indirect)
#define D_JIT_X64_GRP5_CALL_RM    2
// D_JIT_X64_GRP5_JMP_RM: jmp  r/m64 (near, indirect)
#define D_JIT_X64_GRP5_JMP_RM     4
// D_JIT_X64_GRP5_PUSH_RM: push r/m64
#define D_JIT_X64_GRP5_PUSH_RM    6

// ---------------------------------------------------------------------------
// condition codes (tttn) -- add to 0x70 (Jcc rel8), 0x0F80 (Jcc rel32),
// 0x0F90 (SETcc), or 0x0F40 (CMOVcc). Aliased mnemonics share a value.
// ---------------------------------------------------------------------------

// D_JIT_X64_CC_O: overflow (OF=1)
#define D_JIT_X64_CC_O    0x0
// D_JIT_X64_CC_NO: no overflow
#define D_JIT_X64_CC_NO   0x1
// D_JIT_X64_CC_B: below / carry / not-above-or-equal (CF=1)
#define D_JIT_X64_CC_B    0x2
// D_JIT_X64_CC_AE: above-or-equal / not-below / no-carry
#define D_JIT_X64_CC_AE   0x3
// D_JIT_X64_CC_E: equal / zero (ZF=1)
#define D_JIT_X64_CC_E    0x4
// D_JIT_X64_CC_NE: not-equal / not-zero
#define D_JIT_X64_CC_NE   0x5
// D_JIT_X64_CC_BE: below-or-equal / not-above
#define D_JIT_X64_CC_BE   0x6
// D_JIT_X64_CC_A: above / not-below-or-equal
#define D_JIT_X64_CC_A    0x7
// D_JIT_X64_CC_S: sign (SF=1)
#define D_JIT_X64_CC_S    0x8
// D_JIT_X64_CC_NS: no sign
#define D_JIT_X64_CC_NS   0x9
// D_JIT_X64_CC_P: parity / parity-even (PF=1)
#define D_JIT_X64_CC_P    0xA
// D_JIT_X64_CC_NP: no parity / parity-odd
#define D_JIT_X64_CC_NP   0xB
// D_JIT_X64_CC_L: less / not-greater-or-equal (signed)
#define D_JIT_X64_CC_L    0xC
// D_JIT_X64_CC_GE: greater-or-equal / not-less (signed)
#define D_JIT_X64_CC_GE   0xD
// D_JIT_X64_CC_LE: less-or-equal / not-greater (signed)
#define D_JIT_X64_CC_LE   0xE
// D_JIT_X64_CC_G: greater / not-less-or-equal (signed)
#define D_JIT_X64_CC_G    0xF

// D_JIT_X64_JCC_REL8(cc)
//   macro: the one-byte opcode for `jcc rel8` with condition code cc.
#ifndef D_JIT_X64_JCC_REL8
    #define D_JIT_X64_JCC_REL8(cc)  (D_JIT_X64_OP_JCC_REL8 | ((cc) & 0xF))
#endif

// D_JIT_X64_JCC_REL32_OP2(cc)
//   macro: the SECOND opcode byte for `jcc rel32` (follows 0x0F) with cc.
#ifndef D_JIT_X64_JCC_REL32_OP2
    #define D_JIT_X64_JCC_REL32_OP2(cc)                                      \
        (D_JIT_X64_OP2_JCC_REL32 | ((cc) & 0xF))
#endif

// D_JIT_X64_SETCC_OP2(cc)
//   macro: the SECOND opcode byte for `setcc r/m8` (follows 0x0F) with cc.
#ifndef D_JIT_X64_SETCC_OP2
    #define D_JIT_X64_SETCC_OP2(cc)  (D_JIT_X64_OP2_SETCC_RM | ((cc) & 0xF))
#endif


// ===========================================================================
// J.   COMPLETE BYTE SEQUENCES
// ===========================================================================
//   Ready-to-copy, comma-separated byte lists for the handful of fixed
// fragments a code generator emits verbatim. Each expands to a comma list
// suitable for a C array initializer, e.g.:
//     static const unsigned char pro[] = { D_JIT_X64_SEQ_PROLOGUE };
// The NOPn lists are the assembler's canonical n-byte alignment padding.

// D_JIT_X64_SEQ_PROLOGUE
//   constant: push rbp ; mov rbp, rsp   (standard frame setup).
#define D_JIT_X64_SEQ_PROLOGUE      0x55, 0x48, 0x89, 0xE5

// D_JIT_X64_SEQ_EPILOGUE
//   constant: leave ; ret               (tear down frame and return).
#define D_JIT_X64_SEQ_EPILOGUE      0xC9, 0xC3

// D_JIT_X64_SEQ_EPILOGUE_POP
//   constant: pop rbp ; ret             (epilogue without leave).
#define D_JIT_X64_SEQ_EPILOGUE_POP  0x5D, 0xC3

// D_JIT_X64_SEQ_RET
//   constant: ret.
#define D_JIT_X64_SEQ_RET           0xC3

// D_JIT_X64_SEQ_TRAP
//   constant: int3 (single-byte breakpoint / trap padding).
#define D_JIT_X64_SEQ_TRAP          0xCC

// canonical multi-byte NOPs (single-instruction alignment padding, 1-9 bytes)
#define D_JIT_X64_SEQ_NOP1  0x90
#define D_JIT_X64_SEQ_NOP2  0x66, 0x90
#define D_JIT_X64_SEQ_NOP3  0x0F, 0x1F, 0x00
#define D_JIT_X64_SEQ_NOP4  0x0F, 0x1F, 0x40, 0x00
#define D_JIT_X64_SEQ_NOP5  0x0F, 0x1F, 0x44, 0x00, 0x00
#define D_JIT_X64_SEQ_NOP6  0x66, 0x0F, 0x1F, 0x44, 0x00, 0x00
#define D_JIT_X64_SEQ_NOP7  0x0F, 0x1F, 0x80, 0x00, 0x00, 0x00, 0x00
#define D_JIT_X64_SEQ_NOP8  0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00
#define D_JIT_X64_SEQ_NOP9                                                   \
    0x66, 0x0F, 0x1F, 0x84, 0x00, 0x00, 0x00, 0x00, 0x00


// ===========================================================================
// K.   INSTRUCTION EMITTERS
// ===========================================================================
//   A small, deliberately minimal set of encoders that write a single x86-64
// instruction into a d_jit_buffer using the constants above. This is a
// starting vocabulary -- enough to assemble simple leaf functions -- not a
// full assembler. Register arguments are D_JIT_X64_REG_* numbers; extended
// registers (r8-r15) get their REX.B automatically. Each returns 0 on success
// or -1 if the underlying buffer emit fails.

//   C linkage for everything below, so a C++ translation unit can consume this
// header and link against the C archive. Both spellings expand to nothing
// under a C compiler, so a C-only build sees no trace of them.
D_EXTERN_C_BEGIN

// d_jit_x64_emit_prologue
//   function: push rbp ; mov rbp, rsp  (D_JIT_X64_SEQ_PROLOGUE).
D_NODISCARD int d_jit_x64_emit_prologue(d_jit_buffer* _buf);

// d_jit_x64_emit_epilogue
//   function: leave ; ret  (D_JIT_X64_SEQ_EPILOGUE).
D_NODISCARD int d_jit_x64_emit_epilogue(d_jit_buffer* _buf);

// d_jit_x64_emit_ret
//   function: ret (near return, no frame teardown).
D_NODISCARD int d_jit_x64_emit_ret(d_jit_buffer* _buf);

// d_jit_x64_emit_nop
//   function: emit exactly _n bytes of padding using the canonical multi-byte
// NOP forms (chaining the 9-byte form for larger runs).
D_NODISCARD int d_jit_x64_emit_nop(d_jit_buffer* _buf, size_t _n);

// d_jit_x64_emit_mov_reg_imm32
//   function: mov r32, imm32 (B8+rd). Writing the 32-bit form zero-extends the
// value into the full 64-bit register, which is the common way to load a
// small constant.
D_NODISCARD int d_jit_x64_emit_mov_reg_imm32(d_jit_buffer* _buf,
                                              int _reg, uint32_t _imm);

// d_jit_x64_emit_mov_reg_reg
//   function: mov _dst, _src  (64-bit; REX.W + 0x89 /r).
D_NODISCARD int d_jit_x64_emit_mov_reg_reg(d_jit_buffer* _buf,
                                            int _dst, int _src);

// d_jit_x64_emit_add_reg_imm32
//   function: add r64, imm32 (REX.W + 0x81 /0 id).
D_NODISCARD int d_jit_x64_emit_add_reg_imm32(d_jit_buffer* _buf,
                                             int _reg, int32_t _imm);

// d_jit_x64_emit_call_reg
//   function: call r64 (indirect; 0xFF /2).
D_NODISCARD int d_jit_x64_emit_call_reg(d_jit_buffer* _buf, int _reg);

// d_jit_x64_reg_name
//   function: the lowercase 64-bit name ("rax".."r15") for a register number,
// or "?" if out of range.
const char*     d_jit_x64_reg_name(int _reg);

D_EXTERN_C_END


#endif  // DJINTERP_JIT_X64_
