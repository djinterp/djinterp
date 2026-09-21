/******************************************************************************
* djinterp [parse]                                                   program.h
*
* The instruction stream a family's driver runs, and the container that holds
* one.
*   An instruction is twelve bytes of plain data: an opcode, a flag word, and
* two operands. Nothing in it points anywhere. That one property is what the
* whole pipeline above rests on -- a program becomes memcpy-able, hashable as a
* cache key, writable to a file and readable back on another machine, handed to
* C or to a code generator without a fix-up pass, and a candidate for living in
* static storage as a literal type. An instruction carrying a string would
* forfeit every one of those at once; the variable-length operands live in the
* pool instead and the instruction carries an index.
*
*   FAMILY-PRIVATE, AND NOW CHECKABLE. A program records which family's opcode
* space it is written in. Invariant 2 said opcode spaces never collide because
* they are private; carrying the family identifier turns that from a rule
* people follow into one d_parse_program_verify enforces, which matters exactly
* when a second family arrives.
*
*   OPERANDS ARE UNTYPED HERE ON PURPOSE. `a` and `b` are int32 and this level
* does not know whether one is a branch target, a pool index, or an immediate.
* That is the family's business, and reserving it to the family is what lets a
* new family land without touching this file. What this level DOES know is how
* to verify what a family declares: see d_parse_program_verify.
*
*   Requires: c/djinterp.h, config/parse/cfg_parse.h, parse/charset.h,
*             parse/pool.h, parse/machine.h (the registry a program is
*             verified and disassembled against), parse/diagnostic.h.
*
* path:      /inc/djinterp/parse/program.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE INSTRUCTION
    ---------------
    1.  Constants
         1.  D_PARSE_NO_OPERAND
         2.  D_PARSE_KEEP
    2.  The type
         1.  d_parse_instr
         2.  Instruction flags
              a. D_PARSE_INSTR_FLAG_TARGET
              b. D_PARSE_INSTR_FLAG_FAMILY
2.  THE PROGRAM
    -----------
    1.  Operand kinds
         1.  d_parse_operand_kind
         2.  d_parse_op_shape
    2.  The container
         1.  d_parse_program
         2.  Program flags
              a. D_PARSE_PROGRAM_OWNS_CODE
              b. D_PARSE_PROGRAM_VERIFIED
3.  OPERATIONS
    ----------
    1.  Lifetime
    2.  Building
    3.  Access
    4.  Verification
    5.  Identity and transport
    6.  Disassembly
*/

#ifndef DJINTERP_PARSE_PROGRAM_
#define DJINTERP_PARSE_PROGRAM_ 1

// std
#include <stddef.h>                     // size_t, NULL
#include <stdint.h>                     // int32_t, uint16_t, uint32_t, uint64_t
// djinterp
#include "../c/djinterp.h"              // framework root
#include "../config/parse/cfg_parse.h"  // D_INTERNAL_PARSE_PROGRAM_* knobs
#include "./charset.h"                  // d_parse_charset, the class operand
#include "./diagnostic.h"               // the channel verification reports
                                        // through
#include "./machine.h"                  // d_parse_op_set, what a program is
                                        // verified and disassembled against
#include "./pool.h"                     // d_parse_pool, the variable operands


//==============================================================================
// 1.  THE INSTRUCTION
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_PARSE_NO_OPERAND
//   constant: the operand value meaning "unused". Distinct from 0, which is a
// perfectly good branch target and pool index.
#define D_PARSE_NO_OPERAND          (-1)

// 1.1.2
// D_PARSE_KEEP
//   constant: passed to d_parse_program_patch in place of an operand to leave
// that operand as it is -- so patching a forward branch target does not have
// to restate the operand beside it.
#define D_PARSE_KEEP                (-2147483647 - 1)


// 1.2    The type
//------------------------------------------------------------------------------
// 1.2.1
// d_parse_instr
//   struct: one instruction. Twelve bytes, no padding, no pointers. Fixed
// field widths rather than `int`, so the layout is the same on every target
// and a written program reads back the same everywhere.
struct d_parse_instr
{
    uint16_t op;
    uint16_t flags;
    int32_t  a;
    int32_t  b;
};

// 1.2.2
// D_PARSE_INSTR_FLAG_TARGET
//   constant: some branch in this program targets this instruction. Set by
// d_parse_program_verify, which already walks every branch. A code generator
// needs this to know where to place a label and would otherwise re-derive it
// with a scan of its own.
#define D_PARSE_INSTR_FLAG_TARGET   0x0001u

// D_PARSE_INSTR_FLAG_FAMILY
//   constant: the mask of flag bits reserved to the family. The low byte
// belongs to this level; a family numbers its own modifiers in the high byte.
//   CAUTION: prefer a new opcode to a flag whenever the difference is
// BEHAVIOURAL. A flag a handler branches on is a switch that escaped into the
// instruction, which is the thing the registry exists to avoid. Flags are for
// annotations every handler can ignore.
#define D_PARSE_INSTR_FLAG_FAMILY   0xFF00u


//==============================================================================
// 2.  THE PROGRAM
//==============================================================================


// 2.1    Operand kinds
//------------------------------------------------------------------------------
// 2.1.1
// d_parse_operand_kind
//   enum: what an operand of a given opcode means. This level stores operands
// untyped; a family declares their meaning through a shape table so that
// verification, disassembly, and any future analysis can be written once here
// instead of once per family.
enum d_parse_operand_kind
{
    D_PARSE_OPERAND_NONE    = 0,
    D_PARSE_OPERAND_IMM     = 1,
    D_PARSE_OPERAND_TARGET  = 2,
    D_PARSE_OPERAND_CHARSET = 3,
    D_PARSE_OPERAND_NAME    = 4,
    D_PARSE_OPERAND_BLOB    = 5
};

// 2.1.2
// d_parse_op_shape
//   struct: what one opcode's operands mean, indexed by opcode alongside the
// family's op_set. A family builds a static array of these; passing it to
// verify is what lets this level check branch targets and pool indices without
// knowing a single opcode.
struct d_parse_op_shape
{
    uint8_t a;
    uint8_t b;
};


// 2.2    The container
//------------------------------------------------------------------------------
// 2.2.1
// d_parse_program
//   struct: an instruction stream, the pool its operands refer into, and the
// identity of the opcode space it is written in. The pool is a member rather
// than a pointer because the two are one artifact: an index means nothing
// without the pool it indexes, so they are written, read, hashed, and released
// together.
struct d_parse_program
{
    struct d_parse_instr* code;
    uint32_t              count;
    uint32_t              capacity;
    struct d_parse_pool   pool;
    uint32_t              entry;
    uint16_t              family;
    uint16_t              format;
    uint8_t               flags;
    uint8_t               reserved[3];
};

// 2.2.2
// D_PARSE_PROGRAM_OWNS_CODE
//   constant: the instruction array was allocated by the program and is freed
// by d_parse_program_release.
#define D_PARSE_PROGRAM_OWNS_CODE   0x01u
// D_PARSE_PROGRAM_VERIFIED
//   constant: d_parse_program_verify has passed over this program since the
// last edit, so its branch targets are in range and its target flags are set.
// Cleared by any emit or patch.
#define D_PARSE_PROGRAM_VERIFIED    0x02u


//==============================================================================
// 3.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 3.1    Lifetime
//------------------------------------------------------------------------------
//   Fixed-storage init leaves the pool empty; a caller wanting variable
// operands initialises program.pool itself through the pool API. The heap form
// does both, because the two allocations always travel together.
void            d_parse_program_init(struct d_parse_program* _program,
                                     uint16_t                _family,
                                     struct d_parse_instr*   _code,
                                     uint32_t                _capacity);
#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
D_NODISCARD int d_parse_program_init_heap(struct d_parse_program* _program,
                                          uint16_t                _family,
                                          uint32_t                _capacity);
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP
void            d_parse_program_reset(struct d_parse_program* _program);
void            d_parse_program_release(struct d_parse_program* _program);

// 3.2    Building
//------------------------------------------------------------------------------
int32_t         d_parse_program_emit(struct d_parse_program* _program,
                                     int                     _op,
                                     int32_t                 _a,
                                     int32_t                 _b);
D_NODISCARD int d_parse_program_patch(struct d_parse_program* _program,
                                      int32_t                 _pc,
                                      int32_t                 _a,
                                      int32_t                 _b);
uint32_t        d_parse_program_intern_charset(
                    struct d_parse_program*       _program,
                    const struct d_parse_charset* _set);
uint32_t        d_parse_program_intern_name(struct d_parse_program* _program,
                                            const char*             _name);

// 3.3    Access
//------------------------------------------------------------------------------
/*
d_parse_program_at
  The instruction at a program counter.

Parameter(s):
  _program: the program to read; may be NULL.
  _pc:      the program counter.
Return:
  A pointer to the instruction, or NULL when the counter is out of range.
*/
D_INLINE const struct d_parse_instr*
d_parse_program_at(
    const struct d_parse_program* _program,
    int32_t                       _pc
)
{
    // reject a missing program, absent code, and a counter outside the stream
    if ( (!_program)                            ||
         (!_program->code)                      ||
         (_pc < 0)                              ||
         ((uint32_t)_pc >= _program->count)     )
    {
        return NULL;
    }

    return &_program->code[_pc];
}

/*
d_parse_program_charset
  An interned character class, by the index an operand carries.

Parameter(s):
  _program: the program whose pool to read; may be NULL.
  _index:   the index d_parse_program_intern_charset returned.
Return:
  A pointer to the class, or NULL when the index names no class-sized entry.
*/
D_INLINE const struct d_parse_charset*
d_parse_program_charset(
    const struct d_parse_program* _program,
    uint32_t                      _index
)
{
    if (!_program)
    {
        return NULL;
    }

    // a blob of the wrong size is not a class, whatever it was interned as
    if (d_parse_pool_length(&_program->pool, _index) !=
        D_PARSE_CHARSET_BYTES)
    {
        return NULL;
    }

    return (const struct d_parse_charset*)
           d_parse_pool_data(&_program->pool, _index);
}

/*
d_parse_program_name
  An interned name, by the index an operand carries.

Parameter(s):
  _program: the program whose pool to read; may be NULL.
  _index:   the index d_parse_program_intern_name returned.
Return:
  The name, or "" when the index names no entry. Never NULL.
*/
D_INLINE const char*
d_parse_program_name(
    const struct d_parse_program* _program,
    uint32_t                      _index
)
{
    if (!_program)
    {
        return "";
    }

    return d_parse_pool_string(&_program->pool, _index);
}

// 3.4    Verification
//------------------------------------------------------------------------------
D_NODISCARD int d_parse_program_verify(
                    struct d_parse_program*        _program,
                    const struct d_parse_op_set*   _ops,
                    const struct d_parse_op_shape* _shapes,
                    uint32_t                       _shapes_n,
                    struct d_parse_diag_sink*      _diag);

// 3.5    Identity and transport
//------------------------------------------------------------------------------
//   The three things the POD instruction was chosen for. hash is a cache key
// over the whole artifact; write and read move one between processes, runs, or
// machines, in a defined little-endian layout rather than as a memory image.
uint64_t        d_parse_program_hash(const struct d_parse_program* _program);
#if (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1)
size_t          d_parse_program_write(const struct d_parse_program* _program,
                                      void*                         _out,
                                      size_t                        _size);
D_NODISCARD int d_parse_program_read(struct d_parse_program*   _program,
                                     const void*               _in,
                                     size_t                    _size,
                                     struct d_parse_diag_sink* _diag);
#endif  // D_INTERNAL_PARSE_PROGRAM_TRANSPORT

// 3.6    Disassembly
//------------------------------------------------------------------------------
//   Names come from the registry and operand meanings from the shape table, so
// this is one disassembler for every family rather than a fmt() per family.
size_t          d_parse_instr_format(
                    const struct d_parse_program*  _program,
                    const struct d_parse_op_set*   _ops,
                    const struct d_parse_op_shape* _shapes,
                    uint32_t                       _shapes_n,
                    int32_t                        _pc,
                    char*                          _out,
                    size_t                         _size);
void            d_parse_program_print(
                    const struct d_parse_program*  _program,
                    const struct d_parse_op_set*   _ops,
                    const struct d_parse_op_shape* _shapes,
                    uint32_t                       _shapes_n);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_PROGRAM_
