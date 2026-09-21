/******************************************************************************
* djinterp [parse]                                                   program.c
*
*   Definitions for the non-inline declarations in program.h.
*
*
* path:      /src/djinterp/parse/program.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/program.h"  // corresponding header
// std
#include <stdio.h>   // printf, snprintf
#include <string.h>  // memset, memcpy
// djinterp
#include "../../../inc/djinterp/parse/storage.h"  // d_parse_grow, the shared
                                                  // growth policy
#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
#include <stdlib.h>  // malloc, free
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP


// D_INTERNAL_PROGRAM_MAGIC
//   macro: the four bytes a written program begins with, so a read refuses a
// buffer that is not one rather than trusting the counts inside it.
#define D_INTERNAL_PROGRAM_MAGIC    0x4A505247u

// D_INTERNAL_PROGRAM_FORMAT
//   macro: the transport format version. A read refuses anything else; the
// version travels in the header so a future format can be recognised rather
// than mis-parsed.
#define D_INTERNAL_PROGRAM_FORMAT   1u

// D_INTERNAL_PROGRAM_HEADER
//   macro: the fixed part of a written program, in bytes -- magic, format,
// family, entry, and the four counts.
#define D_INTERNAL_PROGRAM_HEADER   32u


/*
d_parse_program_init
  Initialises a program over a caller-supplied instruction array.
NOTE:
  The pool is left empty. A family whose operands are all immediates and branch
targets needs nothing more; one with classes or names initialises program.pool
through the pool API, or uses the heap form below, which does both.

Parameter(s):
  _program:  the program to initialise; ignored if NULL.
  _family:   the identifier of the opcode space this program is written in.
  _code:     storage for instructions; may be NULL for an empty program.
  _capacity: how many instructions _code holds.
Return:
  none.
*/
void
d_parse_program_init(
    struct d_parse_program* _program,
    uint16_t                _family,
    struct d_parse_instr*   _code,
    uint32_t                _capacity
)
{
    if (!_program)
    {
        return;
    }

    memset(_program, 0, sizeof(*_program));

    _program->code     = _code;
    _program->capacity = (_code != NULL) ? _capacity : 0u;
    _program->family   = _family;
    _program->format   = (uint16_t)D_INTERNAL_PROGRAM_FORMAT;

    d_parse_pool_init(&_program->pool, NULL, 0u, NULL, 0u);

    return;
}


#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)

/*
d_parse_program_init_heap
  Initialises a program over instruction storage and a pool it allocates and
owns, both of which then grow on demand.

Parameter(s):
  _program:  the program to initialise; ignored if NULL.
  _family:   the identifier of the opcode space this program is written in.
  _capacity: instructions to reserve; 0 selects D_PARSE_PROGRAM_DEFAULT_CODE.
Post-condition(s):
  - on success the program owns its code and its pool owns its storage; on
    failure the program is left valid, empty, and owning nothing.
Return:
  0 on success; -1 if _program is NULL or an allocation was refused.
*/
int
d_parse_program_init_heap(
    struct d_parse_program* _program,
    uint16_t                _family,
    uint32_t                _capacity
)
{
    if (!_program)
    {
        return -1;
    }

    d_parse_program_init(_program, _family, NULL, 0u);

    const uint32_t slots = (_capacity > 0u)
                           ? _capacity
                           : (uint32_t)D_PARSE_PROGRAM_DEFAULT_CODE;

    struct d_parse_instr* code =
        (struct d_parse_instr*)malloc((size_t)slots * sizeof(*code));

    // check if memory allocation was successful
    if (!code)
    {
        return -1;
    }

    // check if the pool's storage was obtained
    if (d_parse_pool_init_heap(&_program->pool, 0u, 0u) != 0)
    {
        free(code);

        return -1;
    }

    _program->code     = code;
    _program->capacity = slots;
    _program->flags    = (uint8_t)D_PARSE_PROGRAM_OWNS_CODE;

    return 0;
}


/*
d_parse_program_internal_grow
  Grows an owned instruction array so one more instruction fits.

Parameter(s):
  _program: the program to grow.
Return:
  0 on success; -1 when the program does not own its code or a reallocation
was refused.
*/
static int
d_parse_program_internal_grow(
    struct d_parse_program* _program
)
{
    void* const grown = d_parse_grow(_program->code,
                                     &_program->capacity,
                                     _program->count + 1u,
                                     (uint32_t)sizeof(*_program->code),
                                     (_program->flags &
                                      D_PARSE_PROGRAM_OWNS_CODE) != 0u);

    // check if memory allocation was successful
    if (!grown)
    {
        return -1;
    }

    _program->code = (struct d_parse_instr*)grown;

    return 0;
}

#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP


/*
d_parse_program_reset
  Empties a program for reuse, keeping its storage and its family.

Parameter(s):
  _program: the program to empty; ignored if NULL.
Return:
  none.
*/
void
d_parse_program_reset(
    struct d_parse_program* _program
)
{
    if (!_program)
    {
        return;
    }

    _program->count = 0u;
    _program->entry = 0u;
    _program->flags = (uint8_t)(_program->flags & ~D_PARSE_PROGRAM_VERIFIED);

    d_parse_pool_reset(&_program->pool);

    return;
}


/*
d_parse_program_release
  Releases any storage the program owns, its pool included, and leaves it
empty.

Parameter(s):
  _program: the program to release; ignored if NULL.
Return:
  none.
*/
void
d_parse_program_release(
    struct d_parse_program* _program
)
{
    if (!_program)
    {
        return;
    }

#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
    // free only what this program allocated; caller storage is never touched
    if ( (_program->code != NULL) &&
         ((_program->flags & D_PARSE_PROGRAM_OWNS_CODE) != 0u) )
    {
        free(_program->code);
    }
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP

    d_parse_pool_release(&_program->pool);

    memset(_program, 0, sizeof(*_program));

    return;
}


/*
d_parse_program_emit
  Appends one instruction and reports where it landed.
NOTE:
  The returned counter is what a generator patches later, which is why this
returns a position rather than a status: a forward branch is emitted with a
placeholder operand and fixed up once its target is known.

Parameter(s):
  _program: the program to append to; may be NULL.
  _op:      the opcode, in this program's family's private space.
  _a:       the first operand, or D_PARSE_NO_OPERAND.
  _b:       the second operand, or D_PARSE_NO_OPERAND.
Return:
  The program counter of the new instruction, or -1 when the program is
absent, the opcode does not fit the field, or storage could not be found.
*/
int32_t
d_parse_program_emit(
    struct d_parse_program* _program,
    int                     _op,
    int32_t                 _a,
    int32_t                 _b
)
{
    // reject a missing program and an opcode outside the field's range
    if ( (!_program)     ||
         (_op < 0)       ||
         (_op > 0xFFFF)  )
    {
        return -1;
    }

    // make room, or refuse
    if (_program->count >= _program->capacity)
    {
#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
        if (d_parse_program_internal_grow(_program) != 0)
        {
            return -1;
        }
#else
        return -1;
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP
    }

    const uint32_t pc = _program->count;

    _program->code[pc].op    = (uint16_t)_op;
    _program->code[pc].flags = 0u;
    _program->code[pc].a     = _a;
    _program->code[pc].b     = _b;

    _program->count++;

    // any edit invalidates the last verification
    _program->flags = (uint8_t)(_program->flags & ~D_PARSE_PROGRAM_VERIFIED);

    return (int32_t)pc;
}


/*
d_parse_program_patch
  Replaces the operands of an instruction already emitted.
NOTE:
  Pass D_PARSE_KEEP for an operand that should not change, so fixing up a
forward branch target does not have to restate the operand beside it.

Parameter(s):
  _program: the program to edit; may be NULL.
  _pc:      the counter d_parse_program_emit returned.
  _a:       the new first operand, or D_PARSE_KEEP.
  _b:       the new second operand, or D_PARSE_KEEP.
Return:
  0 on success; -1 when the program is absent or the counter is out of range.
*/
int
d_parse_program_patch(
    struct d_parse_program* _program,
    int32_t                 _pc,
    int32_t                 _a,
    int32_t                 _b
)
{
    // reject a missing program and a counter outside the stream
    if ( (!_program)                            ||
         (!_program->code)                      ||
         (_pc < 0)                              ||
         ((uint32_t)_pc >= _program->count)     )
    {
        return -1;
    }

    if (_a != D_PARSE_KEEP)
    {
        _program->code[_pc].a = _a;
    }

    if (_b != D_PARSE_KEEP)
    {
        _program->code[_pc].b = _b;
    }

    // any edit invalidates the last verification
    _program->flags = (uint8_t)(_program->flags & ~D_PARSE_PROGRAM_VERIFIED);

    return 0;
}


/*
d_parse_program_intern_charset
  Interns a character class into the program's pool.

Parameter(s):
  _program: the program whose pool to intern into; may be NULL.
  _set:     the class to store; may be NULL.
Return:
  The index to carry in an operand, or D_PARSE_POOL_NONE on failure.
*/
uint32_t
d_parse_program_intern_charset(
    struct d_parse_program*       _program,
    const struct d_parse_charset* _set
)
{
    if ( (!_program) ||
         (!_set)     )
    {
        return D_PARSE_POOL_NONE;
    }

    return d_parse_pool_intern(&_program->pool,
                               _set->bits,
                               D_PARSE_CHARSET_BYTES);
}


/*
d_parse_program_intern_name
  Interns a name into the program's pool.

Parameter(s):
  _program: the program whose pool to intern into; may be NULL.
  _name:    the name to store; may be NULL.
Return:
  The index to carry in an operand, or D_PARSE_POOL_NONE on failure.
*/
uint32_t
d_parse_program_intern_name(
    struct d_parse_program* _program,
    const char*             _name
)
{
    if (!_program)
    {
        return D_PARSE_POOL_NONE;
    }

    return d_parse_pool_intern_string(&_program->pool, _name);
}


/*
d_parse_program_internal_check_operand
  Verifies one operand against the kind its opcode declares.

Parameter(s):
  _program: the program being verified.
  _kind:    the declared operand kind.
  _value:   the operand as stored.
  _pc:      the instruction's counter, for the diagnostic.
  _which:   'a' or 'b', for the diagnostic.
  _diag:    the sink to report through; may be NULL.
Return:
  A boolean value corresponding to either:
  - 1, if the operand is usable, or
  - 0, otherwise.
*/
static int
d_parse_program_internal_check_operand(
    struct d_parse_program*   _program,
    uint8_t                   _kind,
    int32_t                   _value,
    int32_t                   _pc,
    char                      _which,
    struct d_parse_diag_sink* _diag
)
{
    const struct d_parse_span where = d_parse_span_make((uint32_t)_pc, 1u);

    switch (_kind)
    {
        case D_PARSE_OPERAND_TARGET:
            // a branch must land on an instruction of this program; one past
            // the end is not a target, it is a fall-off
            if ( (_value < 0) ||
                 ((uint32_t)_value >= _program->count) )
            {
                char message[128];

                (void)snprintf(message,
                               sizeof(message),
                               "operand %c of pc %d branches to %d, which is "
                               "outside the program",
                               _which,
                               (int)_pc,
                               (int)_value);

                (void)d_parse_diag_emit(_diag,
                                        (int)D_PARSE_SEVERITY_ERROR,
                                        (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                        (uint16_t)D_PARSE_DIAG_BAD_TARGET,
                                        where,
                                        message);

                return 0;
            }

            // record the landing site while we are here, so a code generator
            // does not have to re-derive it
            _program->code[_value].flags |= (uint16_t)
                                            D_PARSE_INSTR_FLAG_TARGET;
            break;

        case D_PARSE_OPERAND_CHARSET:
            if (!d_parse_program_charset(_program, (uint32_t)_value))
            {
                (void)d_parse_diag_emit(_diag,
                                        (int)D_PARSE_SEVERITY_ERROR,
                                        (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                        (uint16_t)D_PARSE_DIAG_BAD_OPERAND,
                                        where,
                                        "operand does not name a "
                                        "character class");

                return 0;
            }
            break;

        case D_PARSE_OPERAND_NAME:
        case D_PARSE_OPERAND_BLOB:
            if (d_parse_pool_length(&_program->pool,
                                    (uint32_t)_value) == 0u)
            {
                (void)d_parse_diag_emit(_diag,
                                        (int)D_PARSE_SEVERITY_ERROR,
                                        (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                        (uint16_t)D_PARSE_DIAG_BAD_OPERAND,
                                        where,
                                        "operand does not name a "
                                        "pool entry");

                return 0;
            }
            break;

        default:
            // NONE and IMM carry nothing this level can check
            break;
    }

    return 1;
}


/*
d_parse_program_verify
  Checks a program against the registry that will run it and the operand
shapes its family declares.
NOTE:
  This is where invariant 2 stops being a convention. A program records the
opcode space it was written in; running it against another family's registry
is the mistake a second family makes possible, and it is caught here rather
than by a wrong parse at run time.
  Verification also annotates: every instruction some branch lands on gets
D_PARSE_INSTR_FLAG_TARGET, which is exactly what a code generator needs and
would otherwise re-derive.

Parameter(s):
  _program:     the program to verify and annotate; may be NULL.
  _ops:         the registry that will run it; may be NULL to skip the opcode
                and family checks and verify structure only.
  _shapes:      the family's operand-kind table, indexed by opcode; may be NULL
                to skip operand checks.
  _shapes_n: how many opcodes _shapes describes.
  _diag:        the sink to report through; may be NULL.
Post-condition(s):
  - on success D_PARSE_PROGRAM_VERIFIED is set and every branch target carries
    D_PARSE_INSTR_FLAG_TARGET.
Return:
  0 if the program is usable; -1 otherwise, with every problem found reported
rather than only the first.
*/
int
d_parse_program_verify(
    struct d_parse_program*        _program,
    const struct d_parse_op_set*   _ops,
    const struct d_parse_op_shape* _shapes,
    uint32_t                       _shapes_n,
    struct d_parse_diag_sink*      _diag
)
{
    if (!_program)
    {
        return -1;
    }

    int ok = 1;

    // the opcode space a program is written in must be the one the registry
    // reads; a program run against the wrong family would dispatch real
    // handlers on meaningless opcodes
    if ( (_ops != NULL) &&
         (_ops->family != _program->family) )
    {
        char message[128];

        (void)snprintf(message,
                       sizeof(message),
                       "program is written for family %u but the registry "
                       "reads family %u",
                       (unsigned)_program->family,
                       (unsigned)_ops->family);

        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_FAMILY_MISMATCH,
                                d_parse_span_unknown(),
                                message);

        ok = 0;
    }

    // the entry point must be an instruction, unless the program is empty
    if ( (_program->count > 0u) &&
         (_program->entry >= _program->count) )
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_BAD_TARGET,
                                d_parse_span_unknown(),
                                "entry point is outside the program");

        ok = 0;
    }

    // clear the annotation before re-deriving it, so a target that a patch
    // removed does not keep its flag
    for (uint32_t pc = 0u; pc < _program->count; pc++)
    {
        _program->code[pc].flags = (uint16_t)
            (_program->code[pc].flags & ~D_PARSE_INSTR_FLAG_TARGET);
    }

    // walk every instruction, reporting all its problems rather than the
    // first, because a generator bug usually shows up more than once
    for (uint32_t pc = 0u; pc < _program->count; pc++)
    {
        const struct d_parse_instr* const instruction = &_program->code[pc];

        if ( (_ops != NULL) &&
             (!d_parse_op_set_find(_ops, (int)instruction->op)) )
        {
            char message[128];

            (void)snprintf(message,
                           sizeof(message),
                           "pc %d holds opcode %u, which the registry does "
                           "not implement",
                           (int)pc,
                           (unsigned)instruction->op);

            (void)d_parse_diag_emit(_diag,
                                    (int)D_PARSE_SEVERITY_ERROR,
                                    (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                    (uint16_t)D_PARSE_DIAG_UNKNOWN_OP,
                                    d_parse_span_make(pc, 1u),
                                    message);

            ok = 0;

            continue;
        }

        // without a shape table this level cannot know what the operands mean,
        // so structure is all it checks
        if ( (!_shapes) ||
             (instruction->op >= _shapes_n) )
        {
            continue;
        }

        const struct d_parse_op_shape shape = _shapes[instruction->op];

        if (!d_parse_program_internal_check_operand(_program,
                                                    shape.a,
                                                    instruction->a,
                                                    (int32_t)pc,
                                                    'a',
                                                    _diag))
        {
            ok = 0;
        }

        if (!d_parse_program_internal_check_operand(_program,
                                                    shape.b,
                                                    instruction->b,
                                                    (int32_t)pc,
                                                    'b',
                                                    _diag))
        {
            ok = 0;
        }
    }

    if (ok)
    {
        _program->flags |= (uint8_t)D_PARSE_PROGRAM_VERIFIED;
    }

    return ok ? 0 : -1;
}


/*
d_parse_program_hash
  A 64-bit digest of a whole program -- its family, entry, instructions, and
pool.
NOTE:
  FNV-1a over the fields in a defined order rather than over the memory image,
because the image carries capacity, ownership flags, and padding, none of
which are part of what the program IS. Two programs that mean the same thing
digest the same even when one grew its storage and the other did not. This is
a cache key, not a cryptographic hash.

Parameter(s):
  _program: the program to digest; may be NULL.
Return:
  The digest.
*/
uint64_t
d_parse_program_hash(
    const struct d_parse_program* _program
)
{
    uint64_t digest = 1469598103934665603ULL;

    if (!_program)
    {
        return digest;
    }

    #define D_INTERNAL_PROGRAM_FOLD(value)                                    \
        do                                                                    \
        {                                                                     \
            digest ^= (uint64_t)(value);                                      \
            digest *= 1099511628211ULL;                                       \
        } while (0)

    D_INTERNAL_PROGRAM_FOLD(_program->family);
    D_INTERNAL_PROGRAM_FOLD(_program->entry);
    D_INTERNAL_PROGRAM_FOLD(_program->count);

    // fold the instructions field by field; the annotation flags are derived
    // and are deliberately left out, so verifying a program does not change
    // its identity
    for (uint32_t pc = 0u; pc < _program->count; pc++)
    {
        D_INTERNAL_PROGRAM_FOLD(_program->code[pc].op);
        D_INTERNAL_PROGRAM_FOLD((uint32_t)_program->code[pc].a);
        D_INTERNAL_PROGRAM_FOLD((uint32_t)_program->code[pc].b);
    }

    #undef D_INTERNAL_PROGRAM_FOLD

    digest ^= d_parse_pool_hash(&_program->pool);
    digest *= 1099511628211ULL;

    return digest;
}


#if (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1)

/*
d_parse_program_internal_put32
  Writes a 32-bit value little-endian, if there is room.

Parameter(s):
  _out:    the buffer; may be NULL.
  _size:   its size in bytes.
  _cursor: the offset to write at.
  _value:  the value to write.
Return:
  none.
*/
static void
d_parse_program_internal_put32(
    unsigned char* _out,
    size_t         _size,
    size_t         _cursor,
    uint32_t       _value
)
{
    // write only when the whole field fits, so a sizing pass may pass NULL
    if ( (!_out) ||
         ((_cursor + 4u) > _size) )
    {
        return;
    }

    _out[_cursor + 0u] = (unsigned char)(_value & 0xFFu);
    _out[_cursor + 1u] = (unsigned char)((_value >> 8) & 0xFFu);
    _out[_cursor + 2u] = (unsigned char)((_value >> 16) & 0xFFu);
    _out[_cursor + 3u] = (unsigned char)((_value >> 24) & 0xFFu);

    return;
}


/*
d_parse_program_internal_get32
  Reads a 32-bit value little-endian.

Parameter(s):
  _in:     the buffer.
  _cursor: the offset to read from.
Return:
  The value.
*/
static uint32_t
d_parse_program_internal_get32(
    const unsigned char* _in,
    size_t               _cursor
)
{
    return (uint32_t)_in[_cursor + 0u]              |
           ((uint32_t)_in[_cursor + 1u] << 8)       |
           ((uint32_t)_in[_cursor + 2u] << 16)      |
           ((uint32_t)_in[_cursor + 3u] << 24);
}


/*
d_parse_program_write
  Writes a program to a buffer in the transport format.
NOTE:
  Explicitly little-endian, field by field, rather than a memory image: an
image would carry capacities, ownership flags, and padding, and would not
survive a move between targets. This is the format that makes a compiled
program a build artifact.

Parameter(s):
  _program: the program to write; may be NULL.
  _out:     the buffer; may be NULL to measure.
  _size:    the size of _out in bytes.
Return:
  The number of bytes the whole program occupies -- so a return greater than
_size means nothing was written and the caller should retry with that size.
*/
size_t
d_parse_program_write(
    const struct d_parse_program* _program,
    void*                         _out,
    size_t                        _size
)
{
    if (!_program)
    {
        return 0u;
    }

    const size_t code_bytes  = (size_t)_program->count * 12u;
    const size_t pool_bytes  = (size_t)_program->pool.used;
    const size_t entry_bytes = (size_t)_program->pool.count * 8u;
    const size_t needed      = (size_t)D_INTERNAL_PROGRAM_HEADER +
                               code_bytes + pool_bytes + entry_bytes;

    // measure-only, or too small to fill: report the size and write nothing
    if ( (!_out) ||
         (_size < needed) )
    {
        return needed;
    }

    unsigned char* const out = (unsigned char*)_out;

    d_parse_program_internal_put32(out, _size, 0u,
                                   (uint32_t)D_INTERNAL_PROGRAM_MAGIC);
    d_parse_program_internal_put32(out, _size, 4u,
                                   (uint32_t)D_INTERNAL_PROGRAM_FORMAT);
    d_parse_program_internal_put32(out, _size, 8u,
                                   (uint32_t)_program->family);
    d_parse_program_internal_put32(out, _size, 12u, _program->entry);
    d_parse_program_internal_put32(out, _size, 16u, _program->count);
    d_parse_program_internal_put32(out, _size, 20u, _program->pool.count);
    d_parse_program_internal_put32(out, _size, 24u, _program->pool.used);
    d_parse_program_internal_put32(out, _size, 28u, 0u);

    size_t cursor = (size_t)D_INTERNAL_PROGRAM_HEADER;

    // instructions, field by field, so the layout is the format's and not the
    // compiler's
    for (uint32_t pc = 0u; pc < _program->count; pc++)
    {
        d_parse_program_internal_put32(out, _size, cursor,
                                       (uint32_t)_program->code[pc].op);
        d_parse_program_internal_put32(out, _size, cursor + 4u,
                                       (uint32_t)_program->code[pc].a);
        d_parse_program_internal_put32(out, _size, cursor + 8u,
                                       (uint32_t)_program->code[pc].b);

        cursor += 12u;
    }

    // the pool's entry table, then its bytes
    for (uint32_t index = 0u; index < _program->pool.count; index++)
    {
        d_parse_program_internal_put32(out, _size, cursor,
                                       _program->pool.entries[index].offset);
        d_parse_program_internal_put32(out, _size, cursor + 4u,
                                       _program->pool.entries[index].length);

        cursor += 8u;
    }

    if (pool_bytes > 0u)
    {
        memcpy(out + cursor, _program->pool.bytes, pool_bytes);
    }

    return needed;
}


/*
d_parse_program_read
  Reads a program written by d_parse_program_write.
CAUTION:
  The buffer is untrusted. Every count is checked against the bytes actually
present before anything is copied, and every pool entry is checked to lie
inside the blob it claims to index, because a program is an instruction stream
somebody will run.

Parameter(s):
  _program: receives the program; must not be NULL. Released first.
  _in:      the buffer to read; must not be NULL.
  _size:    the size of _in in bytes.
  _diag:    the sink to report through; may be NULL.
Return:
  0 on success; -1 on a malformed buffer or an allocation failure, with the
program left valid and empty.
*/
int
d_parse_program_read(
    struct d_parse_program*   _program,
    const void*               _in,
    size_t                    _size,
    struct d_parse_diag_sink* _diag
)
{
    if ( (!_program) ||
         (!_in)      )
    {
        return -1;
    }

    d_parse_program_release(_program);

    const unsigned char* const in = (const unsigned char*)_in;

    // a buffer too short to hold a header cannot be checked field by field
    if (_size < (size_t)D_INTERNAL_PROGRAM_HEADER)
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_BAD_FORMAT,
                                d_parse_span_unknown(),
                                "buffer is too short to be a program");

        return -1;
    }

    const uint32_t magic  = d_parse_program_internal_get32(in, 0u);
    const uint32_t format = d_parse_program_internal_get32(in, 4u);

    if ( (magic != (uint32_t)D_INTERNAL_PROGRAM_MAGIC) ||
         (format != (uint32_t)D_INTERNAL_PROGRAM_FORMAT) )
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_BAD_FORMAT,
                                d_parse_span_unknown(),
                                "buffer is not a program of a known format");

        return -1;
    }

    const uint32_t family      = d_parse_program_internal_get32(in, 8u);
    const uint32_t entry       = d_parse_program_internal_get32(in, 12u);
    const uint32_t count       = d_parse_program_internal_get32(in, 16u);
    const uint32_t pool_count  = d_parse_program_internal_get32(in, 20u);
    const uint32_t pool_used   = d_parse_program_internal_get32(in, 24u);

    const size_t needed = (size_t)D_INTERNAL_PROGRAM_HEADER +
                          ((size_t)count * 12u) +
                          ((size_t)pool_count * 8u) +
                          (size_t)pool_used;

    // the counts must describe the bytes actually present, or they are a lie
    if (_size < needed)
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_BAD_FORMAT,
                                d_parse_span_unknown(),
                                "program claims more content than the "
                                "buffer holds");

        return -1;
    }

#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
    if (d_parse_program_init_heap(_program,
                                  (uint16_t)family,
                                  (count > 0u) ? count : 1u) != 0)
    {
        (void)d_parse_diag_emit(_diag,
                                (int)D_PARSE_SEVERITY_ERROR,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                (uint16_t)D_PARSE_DIAG_OUT_OF_MEMORY,
                                d_parse_span_unknown(),
                                "could not allocate the program");

        return -1;
    }
#else
    (void)family;

    (void)d_parse_diag_emit(_diag,
                            (int)D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                            (uint16_t)D_PARSE_DIAG_CAPACITY,
                            d_parse_span_unknown(),
                            "reading a program needs heap-backed storage");

    return -1;
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP

    size_t cursor = (size_t)D_INTERNAL_PROGRAM_HEADER;

    for (uint32_t pc = 0u; pc < count; pc++)
    {
        const uint32_t op = d_parse_program_internal_get32(in, cursor);
        const uint32_t a  = d_parse_program_internal_get32(in, cursor + 4u);
        const uint32_t b  = d_parse_program_internal_get32(in, cursor + 8u);

        if (d_parse_program_emit(_program,
                                 (int)(op & 0xFFFFu),
                                 (int32_t)a,
                                 (int32_t)b) < 0)
        {
            d_parse_program_release(_program);

            return -1;
        }

        cursor += 12u;
    }

    // re-intern each blob rather than copying the pool wholesale, so the
    // rebuilt pool is a pool this build produced and its invariants hold by
    // construction rather than by trust
    const size_t entries_at = cursor;
    const size_t bytes_at   = entries_at + ((size_t)pool_count * 8u);

    for (uint32_t index = 0u; index < pool_count; index++)
    {
        const size_t   entry_at = entries_at + ((size_t)index * 8u);
        const uint32_t offset =
            d_parse_program_internal_get32(in, entry_at);
        const uint32_t length =
            d_parse_program_internal_get32(in, entry_at + 4u);

        // an entry must lie inside the blob it claims to index
        if ( ((size_t)offset + (size_t)length) > (size_t)pool_used )
        {
            (void)d_parse_diag_emit(_diag,
                                    (int)D_PARSE_SEVERITY_ERROR,
                                    (uint16_t)D_PARSE_DIAG_DOMAIN_PROGRAM,
                                    (uint16_t)D_PARSE_DIAG_BAD_FORMAT,
                                    d_parse_span_unknown(),
                                    "a pool entry points outside the pool");

            d_parse_program_release(_program);

            return -1;
        }

        if (d_parse_pool_intern(&_program->pool,
                                in + bytes_at + offset,
                                length) == D_PARSE_POOL_NONE)
        {
            d_parse_program_release(_program);

            return -1;
        }
    }

    _program->entry = entry;

    return 0;
}

#endif  // D_INTERNAL_PARSE_PROGRAM_TRANSPORT


/*
d_parse_instr_format
  Renders one instruction, taking its mnemonic from the registry and its
operand meanings from the shape table.
NOTE:
  One disassembler for every family. The alternative -- a fmt() per family --
is the same function written twice, and the second copy is the one that drifts.

Parameter(s):
  _program:     the program to read; may be NULL.
  _ops:         the registry supplying mnemonics; may be NULL, which renders
                opcodes numerically.
  _shapes:      the family's operand-kind table; may be NULL, which renders
                operands numerically.
  _shapes_n: how many opcodes _shapes describes.
  _pc:          the instruction to render.
  _out:         the buffer to write into; may be NULL when _size is 0.
  _size:        the size of _out in bytes, including the terminator.
Return:
  The number of characters the full rendering would occupy, excluding the
terminator.
*/
size_t
d_parse_instr_format(
    const struct d_parse_program*  _program,
    const struct d_parse_op_set*   _ops,
    const struct d_parse_op_shape* _shapes,
    uint32_t                       _shapes_n,
    int32_t                        _pc,
    char*                          _out,
    size_t                         _size
)
{
    const struct d_parse_instr* const instruction =
        d_parse_program_at(_program, _pc);

    if (!instruction)
    {
        return 0u;
    }

    const char* const mnemonic = (_ops != NULL)
                                 ? d_parse_op_set_name(_ops,
                                                       (int)instruction->op)
                                 : "?";

    char operands[192];

    operands[0] = '\0';

    size_t filled = 0u;

    // render each operand the way its declared kind says to read it
    for (int which = 0; which < 2; which++)
    {
        const int32_t value = (which == 0) ? instruction->a : instruction->b;

        uint8_t kind = (uint8_t)D_PARSE_OPERAND_IMM;

        if ( (_shapes != NULL) &&
             (instruction->op < _shapes_n) )
        {
            kind = (which == 0) ? _shapes[instruction->op].a
                                : _shapes[instruction->op].b;
        }

        if (kind == (uint8_t)D_PARSE_OPERAND_NONE)
        {
            continue;
        }

        char piece[128];

        piece[0] = '\0';

        switch (kind)
        {
            case D_PARSE_OPERAND_TARGET:
                (void)snprintf(piece, sizeof(piece), " -> %d", (int)value);
                break;

            case D_PARSE_OPERAND_CHARSET:
            {
                const struct d_parse_charset* const set =
                    d_parse_program_charset(_program, (uint32_t)value);

                char rendered[96];

                rendered[0] = '\0';

                if (set)
                {
                    (void)d_parse_charset_render(set,
                                                 rendered,
                                                 sizeof(rendered));
                }

                (void)snprintf(piece, sizeof(piece), " %s", rendered);
                break;
            }

            case D_PARSE_OPERAND_NAME:
                (void)snprintf(piece,
                               sizeof(piece),
                               " %s",
                               d_parse_program_name(_program,
                                                    (uint32_t)value));
                break;

            case D_PARSE_OPERAND_BLOB:
                (void)snprintf(piece, sizeof(piece), " blob#%d", (int)value);
                break;

            default:
                if (value != (int32_t)D_PARSE_NO_OPERAND)
                {
                    (void)snprintf(piece, sizeof(piece), " %d", (int)value);
                }
                break;
        }

        const size_t room = sizeof(operands) - filled;
        const int    put  = snprintf(operands + filled, room, "%s", piece);

        if ( (put > 0) &&
             ((size_t)put < room) )
        {
            filled += (size_t)put;
        }
    }

    const int written = snprintf(_out,
                                 _size,
                                 "%4d%s %-10s%s",
                                 (int)_pc,
                                 ((instruction->flags &
                                   D_PARSE_INSTR_FLAG_TARGET) != 0u)
                                 ? ":"
                                 : " ",
                                 mnemonic,
                                 operands);

    return (written < 0) ? 0u : (size_t)written;
}


/*
d_parse_program_print
  Writes a whole program to stdout, one instruction per line. Diagnostics and
build checks only.

Parameter(s):
  _program:     the program to print; ignored if NULL.
  _ops:         the registry supplying mnemonics; may be NULL.
  _shapes:      the family's operand-kind table; may be NULL.
  _shapes_n: how many opcodes _shapes describes.
Return:
  none.
*/
void
d_parse_program_print(
    const struct d_parse_program*  _program,
    const struct d_parse_op_set*   _ops,
    const struct d_parse_op_shape* _shapes,
    uint32_t                       _shapes_n
)
{
    if (!_program)
    {
        return;
    }

    char line[256];

    for (uint32_t pc = 0u; pc < _program->count; pc++)
    {
        (void)d_parse_instr_format(_program,
                                   _ops,
                                   _shapes,
                                   _shapes_n,
                                   (int32_t)pc,
                                   line,
                                   sizeof(line));

        printf("%s\n", line);
    }

    return;
}
