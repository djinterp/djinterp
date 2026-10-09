/*******************************************************************************
* djinterp [parse]                                       parse_program_tests.cpp
*
* The program-IR suite's sections: the canonical character class, the operand
* pool, and the instruction stream they serve.
*   A SECTION IS THE UNIT A KNOB REMOVES. A program that owns its storage, a
* pool that does, and the transport format each exist only under their
* configuration knob, so each has sections of its own, defined and listed
* under that knob. No section carries a conditional inside its body: what a
* build cannot do, it does not claim to have checked.
*
*
* path:      /tests/djinterp/parse/parse_program_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parse_program_tests.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor its header has

// std
#include <cstddef>  // std::size_t
#include <cstring>  // std::strcmp, std::memcmp, std::strstr
// djinterp
#include "../../../inc/djinterp/parse/charset.hpp"     // charset
#include "../../../inc/djinterp/parse/diagnostic.hpp"  // fixed_diagnostics
#include "../../../inc/djinterp/parse/machine.hpp"     // fixed_op_set, machine
#include "../../../inc/djinterp/parse/pool.hpp"        // pool, fixed_pool
#include "../../../inc/djinterp/parse/program.hpp"     // program, instr, shape
// re_std
#include "../../../inc/re_std/cstdint/cstdint.hpp"  // re_std::int32_t,
                                                    // uint32_t, uint64_t


namespace
{

using namespace djinterp::parse;

// a throwaway family: its opcode space, its identifier, and the operand shapes
// that let this level verify and disassemble it without knowing any of it
enum
{
    P_ANY = 0,
    P_SET,
    P_CALL,
    P_JUMP,
    P_HALT,
    P_COUNT
};

enum
{
    FAMILY_TOY   = 21,
    FAMILY_OTHER = 22
};

// TOY_SHAPES
//   constant: what each of the toy family's opcodes takes as operands.
const op_shape TOY_SHAPES[P_COUNT] =
{
    shape(),                                    // P_ANY
    shape(operand::charset),                    // P_SET  class
    shape(operand::target, operand::name),      // P_CALL target, name
    shape(operand::target),                     // P_JUMP target
    shape()                                     // P_HALT
};

// nothing_op
//   struct: an operator that does nothing, for a registry that exists only so
// a program has something to be verified against.
struct nothing_op
{
    void
    operator()(
        machine& _machine
    ) const
    {
        (void)_machine;

        return;
    }
};

// toy_registry
//   function: defines every opcode of the toy family in a registry.
bool
toy_registry(
    op_set& _ops
)
{
    return ( (_ops.def(P_ANY,  "ANY",  nothing_op()))  &&
             (_ops.def(P_SET,  "SET",  nothing_op()))  &&
             (_ops.def(P_CALL, "CALL", nothing_op()))  &&
             (_ops.def(P_JUMP, "JUMP", nothing_op()))  &&
             (_ops.def(P_HALT, "HALT", nothing_op()))  );
}

/*
d_tests_parse_charset_membership
  What a class holds, and the questions an encoding decision asks of it.
  Tests the following:
  - membership, ranges, and negation
  - two spellings of one set compare equal, which is what makes interning work
  - count reports density and contiguous reports a run, with its bounds
  - disjointness, the condition an ordered choice needs to become a dispatch
*/
bool
d_tests_parse_charset_membership(void)
{
    const charset digits  = charset::of("0-9");
    const charset same    = charset::of("0123456789");
    const charset letters = charset::of("a-zA-Z");
    const charset negated = charset::of("^\n");

    // two spellings, one set -- the property the pool relies on
    if ( (digits != same)         ||
         (!digits.test('5'))      ||
         (digits.test('a'))       ||
         (digits.count() != 10u)  )
    {
        return false;
    }

    unsigned char low  = 0u;
    unsigned char high = 0u;

    // a contiguous run compiles to two compares rather than a table load
    if ( (!digits.contiguous(&low, &high))  ||
         (low != '0')                       ||
         (high != '9')                      )
    {
        return false;
    }

    // the first-set condition an ordered choice needs to become a dispatch
    if ( (!digits.disjoint(letters))  ||
         (letters.count() != 52u)     ||
         (letters.contiguous())       )
    {
        return false;
    }

    return ( (!negated.test('\n'))       &&
             (negated.test('x'))         &&
             (negated.count() == 255u)   );
}

/*
d_tests_parse_charset_render
  The canonical text of a class.
  Tests the following:
  - rendering folds runs back into ranges
  - a dense set renders negated rather than as 255 characters
  - what renders out parses back to the same set
*/
bool
d_tests_parse_charset_render(void)
{
    const charset digits  = charset::of("0-9");
    const charset negated = charset::of("^\n");
    char          text[128];

    (void)digits.render(text, sizeof(text));

    if (std::strcmp(text, "[0-9]") != 0)
    {
        return false;
    }

    (void)negated.render(text, sizeof(text));

    if (std::strcmp(text, "[^\\n]") != 0)
    {
        return false;
    }

    charset reparsed;

    // the C entry point fills the C++ object directly
    if (d_parse_charset_parse(&reparsed, "0-9") != 0)
    {
        return false;
    }

    return (reparsed == digits);
}

/*
d_tests_parse_charset_algebra
  Set algebra over classes.
  Tests the following:
  - union holds every member of both operands
  - difference removes exactly what was united, restoring the original
*/
bool
d_tests_parse_charset_algebra(void)
{
    const charset digits  = charset::of("0-9");
    const charset letters = charset::of("a-zA-Z");
    charset       alnum   = digits;

    alnum.unite(letters);

    if ( (alnum.count() != 62u)  ||
         (!alnum.test('7'))      ||
         (!alnum.test('Q'))      )
    {
        return false;
    }

    alnum.subtract(letters);

    return (alnum == digits);
}

/*
d_tests_parse_pool_intern
  The operand intern pool.
  Tests the following:
  - an identical blob interned twice yields one entry and one index
  - strings carry their terminator, so reading one back needs no copy
  - a class interns as its own bytes and comes back as a class
  - another spelling of that class finds the entry already there
*/
bool
d_tests_parse_pool_intern(void)
{
    fixed_pool<256u, 8u> fixed;

    const re_std::uint32_t first  = fixed.intern("Digit");
    const re_std::uint32_t second = fixed.intern("Digit");
    const re_std::uint32_t other  = fixed.intern("Letter");

    // interning is the whole point: one blob, one index, however often asked
    if ( (first != second)                                  ||
         (first == other)                                   ||
         (fixed.size() != 2u)                               ||
         (std::strcmp(fixed.text(first), "Digit") != 0)     ||
         (std::strcmp(fixed.text(other), "Letter") != 0)    )
    {
        return false;
    }

    const charset          digits = charset::of("0-9");
    const re_std::uint32_t set_at = fixed.intern(digits);

    // a class interns as its own 32 bytes and reads back as a class
    if ( (set_at == D_PARSE_POOL_NONE)                    ||
         (fixed.length(set_at) != D_PARSE_CHARSET_BYTES)  ||
         (!fixed.set(set_at))                             ||
         (std::memcmp(fixed.set(set_at),
                      &digits,
                      D_PARSE_CHARSET_BYTES) != 0)        )
    {
        return false;
    }

    return (fixed.intern(charset::of("0123456789")) == set_at);
}

/*
d_tests_parse_pool_refusal
  A pool over fixed storage, at its limit.
  Tests the following:
  - a blob that fits is interned
  - one that does not is refused rather than written past the caller's storage
*/
bool
d_tests_parse_pool_refusal(void)
{
    fixed_pool<8u, 1u> tiny;

    if (tiny.intern("aaaa") == D_PARSE_POOL_NONE)
    {
        return false;
    }

    return (tiny.intern("bbbbbbbbbbbb") == D_PARSE_POOL_NONE);
}

#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
/*
d_tests_parse_pool_digest
  The pool's digest, across storage.
  Tests the following:
  - the digest is over contents, so a pool that owns and grows its storage
    agrees with a fixed one holding the same blobs
*/
bool
d_tests_parse_pool_digest(void)
{
    const charset        digits = charset::of("0-9");
    fixed_pool<256u, 8u> fixed;
    pool                 grown;

    if (!grown.reserve(16u, 2u))
    {
        return false;
    }

    (void)fixed.intern("Digit");
    (void)fixed.intern("Letter");
    (void)fixed.intern(digits);

    (void)grown.intern("Digit");
    (void)grown.intern("Letter");
    (void)grown.intern(digits);

    return (grown.digest() == fixed.digest());
}
#endif  // D_INTERNAL_PARSE_POOL_HEAP

#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
// layout
//   struct: where the toy program's parts landed, for the sections sharing it.
struct layout
{
    re_std::uint32_t digits;
    re_std::uint32_t name;
    re_std::int32_t  jump;
    re_std::int32_t  halt;
};

// build
//   function: emits the toy program into a program that owns its storage -- a
// forward jump over a class test and a call, landing on the halt. The jump is
// emitted with a placeholder and patched once its target is known, which is
// the shape every generator's backpatching takes.
bool
build(
    program& _code,
    layout&  _at
)
{
    if (!_code.reserve(2u))
    {
        return false;
    }

    _at.digits = _code.intern(charset::of("0-9"));
    _at.name   = _code.intern("Number");
    _at.jump   = _code.emit(P_JUMP, D_PARSE_NO_OPERAND);

    (void)_code.emit(P_SET, static_cast<re_std::int32_t>(_at.digits));
    (void)_code.emit(P_CALL, 1, static_cast<re_std::int32_t>(_at.name));

    _at.halt = _code.emit(P_HALT);

    return ( (_at.digits != D_PARSE_POOL_NONE)   &&
             (_at.name != D_PARSE_POOL_NONE)     &&
             (_at.jump == 0)                     &&
             (_at.halt == 3)                     &&
             (_code.size() == 4u)                &&
             (_code.patch(_at.jump, _at.halt))   );
}

/*
d_tests_parse_program_build
  Building and verifying an instruction stream.
  Tests the following:
  - emit reports where an instruction landed, so a forward branch can be
    patched once its target is known
  - a valid program verifies against its family's registry and shapes
  - verification annotates branch targets, so a backend need not re-derive them
  - the digest is over what the program means, so verifying it again -- which
    sets annotation flags -- does not change it
*/
bool
d_tests_parse_program_build(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    fixed_op_set<P_COUNT>         ops(FAMILY_TOY);
    program                       code(FAMILY_TOY);
    layout                        at = { 0u, 0u, 0, 0 };

    if ( (!toy_registry(ops))                               ||
         (!build(code, at))                                 ||
         (!code.verify(ops, TOY_SHAPES, P_COUNT, &diag))    )
    {
        return false;
    }

    // the landing site is marked, and nothing else is
    if ( (diag.failed())                                                ||
         (!code.verified())                                             ||
         ((code.at(at.halt)->flags & D_PARSE_INSTR_FLAG_TARGET) == 0u)  ||
         ((code.at(2)->flags & D_PARSE_INSTR_FLAG_TARGET) != 0u)        )
    {
        return false;
    }

    const re_std::uint64_t before = code.digest();

    if (!code.verify(ops, TOY_SHAPES, P_COUNT, &diag))
    {
        return false;
    }

    return (code.digest() == before);
}

/*
d_tests_parse_program_faults
  What verification refuses.
  Tests the following:
  - a branch off the end is caught, with the instruction's index in the span
  - a failed verification leaves the program unverified
  - a program run against another family's registry is caught rather than
    documented
*/
bool
d_tests_parse_program_faults(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    fixed_op_set<P_COUNT>         ops(FAMILY_TOY);
    fixed_op_set<P_COUNT>         foreign(FAMILY_OTHER);
    program                       code(FAMILY_TOY);
    layout                        at = { 0u, 0u, 0, 0 };

    if ( (!toy_registry(ops))       ||
         (!toy_registry(foreign))   ||
         (!build(code, at))         ||
         (!code.patch(at.jump, 99)) )
    {
        return false;
    }

    // the branch now leaves the program
    if ( (code.verify(ops, TOY_SHAPES, P_COUNT, &diag))     ||
         (code.verified())                                  ||
         (!diag[0].is(D_PARSE_DIAG_DOMAIN_PROGRAM,
                      D_PARSE_DIAG_BAD_TARGET))             ||
         (diag[0].where().offset != 0u)                     )
    {
        return false;
    }

    diag.clear();

    // repaired, it is still the wrong family's program for this registry
    return ( (code.patch(at.jump, at.halt))                            &&
             (!code.verify(foreign, TOY_SHAPES, P_COUNT, &diag))       &&
             (diag[0].is(D_PARSE_DIAG_DOMAIN_PROGRAM,
                         D_PARSE_DIAG_FAMILY_MISMATCH))                );
}

/*
d_tests_parse_program_format
  One disassembler, for any family, given its shapes.
  Tests the following:
  - the mnemonic comes from the registry
  - a class operand renders as the class, and a name operand as the name
*/
bool
d_tests_parse_program_format(void)
{
    fixed_op_set<P_COUNT> ops(FAMILY_TOY);
    program               code(FAMILY_TOY);
    layout                at = { 0u, 0u, 0, 0 };
    char                  line[128];

    if ( (!toy_registry(ops))  ||
         (!build(code, at))    )
    {
        return false;
    }

    (void)d_parse_instr_format(&code,
                               &ops,
                               TOY_SHAPES,
                               P_COUNT,
                               1,
                               line,
                               sizeof(line));

    if ( (!std::strstr(line, "SET"))    ||
         (!std::strstr(line, "[0-9]"))  )
    {
        return false;
    }

    (void)d_parse_instr_format(&code,
                               &ops,
                               TOY_SHAPES,
                               P_COUNT,
                               2,
                               line,
                               sizeof(line));

    return (std::strstr(line, "Number") != nullptr);
}
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP

#if ( (D_INTERNAL_PARSE_PROGRAM_HEAP == 1) &&                                  \
      (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1) )
/*
d_tests_parse_program_transport
  The round trip the POD instruction was chosen for.
  Tests the following:
  - a write with no buffer measures
  - a program written out and read back is the same program: same digest,
    same family, same length
  - the restored pool still resolves the operands the instructions carry
*/
bool
d_tests_parse_program_transport(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    program                       code(FAMILY_TOY);
    program                       restored;
    layout                        at = { 0u, 0u, 0, 0 };
    unsigned char                 buffer[1024];

    if (!build(code, at))
    {
        return false;
    }

    const std::size_t needed = code.write(nullptr, 0u);

    if ( (needed == 0u)                                    ||
         (needed > sizeof(buffer))                         ||
         (code.write(buffer, sizeof(buffer)) != needed)    ||
         (!restored.read(buffer, needed, &diag))           )
    {
        return false;
    }

    return ( (restored.digest() == code.digest())                       &&
             (restored.space() == FAMILY_TOY)                           &&
             (restored.size() == code.size())                           &&
             (!diag.failed())                                           &&
             (std::strcmp(restored.name_at(at.name), "Number") == 0)    &&
             (restored.set_at(at.digits) != nullptr)                    );
}

/*
d_tests_parse_program_transport_refusal
  What reading refuses.
  Tests the following:
  - a buffer that is not a program is refused rather than trusted, and the
    refusal is identified by the program's domain and its own code
*/
bool
d_tests_parse_program_transport_refusal(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    program                       rejected;
    unsigned char                 garbage[64];

    // any bytes that do not begin with the format's signature
    for (std::size_t index = 0u; index < sizeof(garbage); index++)
    {
        garbage[index] = static_cast<unsigned char>(index);
    }

    return ( (!rejected.read(garbage, sizeof(garbage), &diag))      &&
             (diag.failed())                                        &&
             (diag[0].is(D_PARSE_DIAG_DOMAIN_PROGRAM,
                         D_PARSE_DIAG_BAD_FORMAT))                  );
}
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP && D_INTERNAL_PARSE_PROGRAM_TRANSPORT

/*
d_tests_parse_program_fixed
  A program with no allocator at all.
  Tests the following:
  - fixed storage and a fixed pool hold a program and verify it
  - an instruction is a twelve-byte literal type, which is the compile-time
    door: one built by make_instr is a constant expression
*/
bool
d_tests_parse_program_fixed(void)
{
    fixed_diagnostics<16u, 1024u> diag;
    fixed_op_set<P_COUNT>         ops(FAMILY_TOY);
    fixed_program<4u, 64u, 4u>    embedded(FAMILY_TOY);

    constexpr instr literal = make_instr(P_JUMP, 3);

    D_STATIC_ASSERT(sizeof(instr) == 12u, "an instruction is twelve bytes");
    D_STATIC_ASSERT(literal.op == P_JUMP, "an instruction is a constant");
    D_STATIC_ASSERT(literal.a == 3, "its operands are constants too");

    if (!toy_registry(ops))
    {
        return false;
    }

    (void)embedded.emit(P_ANY);
    (void)embedded.emit(P_HALT);

    return ( (embedded.verify(ops, TOY_SHAPES, P_COUNT, &diag))  &&
             (embedded.size() == 2u)                             &&
             (!diag.failed())                                    );
}

}  // namespace

// d_tests_parse_program
//   constant: the suite -- every section this build has, the class before the
// pool that interns it and the pool before the program that carries its
// indices.
const d_tests_section d_tests_parse_program[] =
{
    { "parse_charset_membership",  &d_tests_parse_charset_membership  },
    { "parse_charset_render",      &d_tests_parse_charset_render      },
    { "parse_charset_algebra",     &d_tests_parse_charset_algebra     },
    { "parse_pool_intern",         &d_tests_parse_pool_intern         },
    { "parse_pool_refusal",        &d_tests_parse_pool_refusal        },
#if (D_INTERNAL_PARSE_POOL_HEAP == 1)
    { "parse_pool_digest",         &d_tests_parse_pool_digest         },
#endif  // D_INTERNAL_PARSE_POOL_HEAP
#if (D_INTERNAL_PARSE_PROGRAM_HEAP == 1)
    { "parse_program_build",       &d_tests_parse_program_build       },
    { "parse_program_faults",      &d_tests_parse_program_faults      },
    { "parse_program_format",      &d_tests_parse_program_format      },
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP
#if ( (D_INTERNAL_PARSE_PROGRAM_HEAP == 1) &&                                  \
      (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1) )
    { "parse_program_transport",   &d_tests_parse_program_transport   },
    { "parse_program_transport_refusal",
      &d_tests_parse_program_transport_refusal                        },
#endif  // D_INTERNAL_PARSE_PROGRAM_HEAP && D_INTERNAL_PARSE_PROGRAM_TRANSPORT
    { "parse_program_fixed",       &d_tests_parse_program_fixed       }
};

// d_tests_parse_program_count
//   constant: how many sections the table above holds in this build.
const std::size_t d_tests_parse_program_count =
    sizeof(d_tests_parse_program) / sizeof(d_tests_parse_program[0]);

#endif  // floor, for now
