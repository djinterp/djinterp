/*******************************************************************************
* djinterp [parse]                                               parse_c_tests.c
*
* The parse substrate's C suite: the diagnostic channel, the machine and its
* registry, and the instruction stream, each through the C API alone.
*   Every section runs on storage it declares, so each holds in every
* configuration, including one with no allocator at all. The one section whose
* subject a knob removes -- the transport format -- is defined and listed under
* that knob, and no section carries a conditional inside its body.
*
*
* path:      /tests/djinterp/parse/c/parse_c_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.21
*                                                            revised: 2026.10.04
*******************************************************************************/
#include "./parse_c_tests.h"  // corresponding header
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <string.h>   // strcmp
// djinterp
#include "../../../../inc/djinterp/parse/c/charset.h"     // d_parse_charset
#include "../../../../inc/djinterp/parse/c/diagnostic.h"  // the sink
#include "../../../../inc/djinterp/parse/c/machine.h"     // machine, op_set
#include "../../../../inc/djinterp/parse/c/program.h"     // program, verify
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int32_t, uint16_t,
                                                     // uint32_t, uint64_t


// TOY_FAMILY
//   constant: the identifier of the throwaway opcode space these sections use.
static const uint16_t TOY_FAMILY = 9u;

// the toy family's opcodes: one that takes a class, one that branches, and one
// that takes nothing -- every operand shape a section needs
enum d_tests_parse_c_op
{
    OP_SET = 0,
    OP_JUMP,
    OP_HALT,
    OP_COUNT
};

// TOY_SHAPES
//   constant: what each of the toy family's opcodes takes as operands.
static const struct d_parse_op_shape TOY_SHAPES[OP_COUNT] =
{
    { D_PARSE_OPERAND_CHARSET, D_PARSE_OPERAND_NONE },
    { D_PARSE_OPERAND_TARGET,  D_PARSE_OPERAND_NONE },
    { D_PARSE_OPERAND_NONE,    D_PARSE_OPERAND_NONE }
};

// TOY_NAMES
//   constant: what each of the toy family's opcodes is called in a trace.
static const char* const TOY_NAMES[OP_COUNT] = { "SET", "JUMP", "HALT" };

// d_tests_parse_c_toy
//   struct: the toy family's registry and one program for it, over storage the
// struct itself carries -- so that a section needs no allocator -- with where
// the program's parts landed.
struct d_tests_parse_c_toy
{
    struct d_parse_instr      code[8];
    char                      bytes[128];
    struct d_parse_pool_entry entries[8];
    struct d_parse_op         table[OP_COUNT];
    struct d_parse_program    program;
    struct d_parse_op_set     ops;
    uint32_t                  set;
    int32_t                   jump;
    int32_t                   halt;
};

/*
d_tests_parse_c_op_count
  Test operator: increments the int its context points at.
*/
static void
d_tests_parse_c_op_count(
    struct d_parse_machine* _machine,
    void*                   _ctx
)
{
    (void)_machine;

    (*(int*)_ctx)++;

    return;
}

/*
d_tests_parse_c_op_stop
  Test operator: halts the run successfully.
*/
static void
d_tests_parse_c_op_stop(
    struct d_parse_machine* _machine,
    void*                   _ctx
)
{
    (void)_ctx;

    d_parse_machine_halt(_machine, 1);

    return;
}

/*
d_tests_parse_c_op_nop
  Test operator: does nothing, for a registry that exists only so a program
has something to be verified against.
*/
static void
d_tests_parse_c_op_nop(
    struct d_parse_machine* _machine,
    void*                   _ctx
)
{
    (void)_machine;
    (void)_ctx;

    return;
}

/*
d_tests_parse_c_toy_build
  Binds the toy's storage, defines its three opcodes, and emits its program:
a forward jump over a class test, landing on the halt. The jump is emitted
with a placeholder and patched once its target is known.
*/
static bool
d_tests_parse_c_toy_build(
    struct d_tests_parse_c_toy* _toy
)
{
    struct d_parse_charset digits = { 0 };

    d_parse_program_init(&_toy->program, TOY_FAMILY, _toy->code, 8u);
    d_parse_pool_init(&_toy->program.pool,
                      _toy->bytes,
                      (uint32_t)sizeof(_toy->bytes),
                      _toy->entries,
                      8u);
    d_parse_op_set_init(&_toy->ops,
                        TOY_FAMILY,
                        _toy->table,
                        (uint32_t)OP_COUNT);

    // every opcode of the family, under the name a disassembly prints
    for (int code = 0; code < OP_COUNT; code++)
    {
        if (d_parse_op_set_def(&_toy->ops,
                               code,
                               TOY_NAMES[code],
                               d_tests_parse_c_op_nop,
                               NULL) != 0)
        {
            return false;
        }
    }

    if (d_parse_charset_parse(&digits, "0-9") != 0)
    {
        return false;
    }

    _toy->set  = d_parse_program_intern_charset(&_toy->program, &digits);
    _toy->jump = d_parse_program_emit(&_toy->program,
                                      OP_JUMP,
                                      D_PARSE_NO_OPERAND,
                                      D_PARSE_NO_OPERAND);

    (void)d_parse_program_emit(&_toy->program,
                               OP_SET,
                               (int32_t)_toy->set,
                               D_PARSE_NO_OPERAND);

    _toy->halt = d_parse_program_emit(&_toy->program,
                                      OP_HALT,
                                      D_PARSE_NO_OPERAND,
                                      D_PARSE_NO_OPERAND);

    return ( (_toy->set != D_PARSE_POOL_NONE)                    &&
             (_toy->halt == 2)                                   &&
             (d_parse_program_patch(&_toy->program,
                                    _toy->jump,
                                    _toy->halt,
                                    D_PARSE_KEEP) == 0)          );
}

/*
d_tests_parse_c_diagnostics_tally
  What a sink over caller storage counts.
  Tests the following:
  - a warning alone is not a failure
  - an error is, and failed() answers from the tally
  - release is safe on caller storage
*/
static bool
d_tests_parse_c_diagnostics_tally(void)
{
    struct d_parse_diagnostic items[4]  = { 0 };
    char                      text[128] = { 0 };
    struct d_parse_diag_sink  sink      = { 0 };

    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));

    (void)d_parse_diag_emit(&sink,
                            (int)D_PARSE_SEVERITY_WARNING,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                            (uint16_t)1u,
                            d_parse_span_make(3u, 2u),
                            "careful");

    if (d_parse_diag_failed(&sink))
    {
        return false;
    }

    (void)d_parse_diag_emit(&sink,
                            (int)D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                            (uint16_t)2u,
                            d_parse_span_unknown(),
                            "broken");

    const bool failed = (d_parse_diag_failed(&sink) != 0);

    d_parse_diag_sink_release(&sink);

    return failed;
}

/*
d_tests_parse_c_diagnostics_identity
  A diagnostic is identified by its domain and its code together.
  Tests the following:
  - the pair finds a stored condition, and says where it is
  - the same code under another domain is a different condition
  - message text comes back out of the arena by offset
*/
static bool
d_tests_parse_c_diagnostics_identity(void)
{
    struct d_parse_diagnostic items[4]  = { 0 };
    char                      text[128] = { 0 };
    struct d_parse_diag_sink  sink      = { 0 };

    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));

    (void)d_parse_diag_emit(&sink,
                            (int)D_PARSE_SEVERITY_WARNING,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                            (uint16_t)1u,
                            d_parse_span_make(3u, 2u),
                            "careful");
    (void)d_parse_diag_emit(&sink,
                            (int)D_PARSE_SEVERITY_ERROR,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                            (uint16_t)2u,
                            d_parse_span_unknown(),
                            "broken");

    return ( (d_parse_diag_find(&sink,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_CORE,
                                (uint16_t)2u,
                                0u) == 1)                               &&
             (d_parse_diag_find(&sink,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                                (uint16_t)2u,
                                0u) == -1)                              &&
             (strcmp(d_parse_diag_message(&sink,
                                          d_parse_diag_at(&sink, 0u)),
                     "careful") == 0)                                   );
}

/*
d_tests_parse_c_machine_dispatch
  The substrate, driven from C.
  Tests the following:
  - a C function and a context pointer are the whole of a C operator
  - a driver is a plain loop over the registry, with no switch on opcode
  - the run halts successfully where an operator halts it
*/
static bool
d_tests_parse_c_machine_dispatch(void)
{
    struct d_parse_op      table[2]  = { 0 };
    struct d_parse_op_set  ops       = { 0 };
    struct d_parse_machine machine   = { 0 };
    int                    count     = 0;
    const int              program[] = { 0, 0, 0, 1 };

    d_parse_op_set_init(&ops, TOY_FAMILY, table, 2u);
    d_parse_machine_init(&machine, NULL);

    if ( (d_parse_op_set_def(&ops,
                             0,
                             "COUNT",
                             d_tests_parse_c_op_count,
                             &count) != 0)          ||
         (d_parse_op_set_def(&ops,
                             1,
                             "STOP",
                             d_tests_parse_c_op_stop,
                             NULL) != 0)            )
    {
        return false;
    }

    // fetch, dispatch, and stop when an operator halts the run
    for (size_t i = 0; i < (sizeof(program) / sizeof(program[0])); i++)
    {
        if (!d_parse_op_set_dispatch(&machine, &ops, program[i]))
        {
            break;
        }
    }

    return ( (count == 3)       &&
             (machine.halted)   &&
             (machine.ok)       );
}

/*
d_tests_parse_c_machine_faults
  What stops a run that cannot go on.
  Tests the following:
  - an opcode with no operator halts the run and is reported, never a silent
    no-op
  - the step budget stops a program that never halts, and that is reported
*/
static bool
d_tests_parse_c_machine_faults(void)
{
    struct d_parse_op         table[2]  = { 0 };
    struct d_parse_op_set     ops       = { 0 };
    struct d_parse_diagnostic items[4]  = { 0 };
    char                      text[128] = { 0 };
    struct d_parse_diag_sink  sink      = { 0 };
    struct d_parse_machine    machine   = { 0 };
    int                       count     = 0;

    d_parse_op_set_init(&ops, TOY_FAMILY, table, 2u);
    d_parse_diag_sink_init(&sink, items, 4u, text, (uint32_t)sizeof(text));
    d_parse_machine_init(&machine, &sink);

    // opcode 1 has no operator
    if ( (d_parse_op_set_def(&ops,
                             0,
                             "COUNT",
                             d_tests_parse_c_op_count,
                             &count) != 0)                             ||
         (d_parse_op_set_dispatch(&machine, &ops, 1))                  ||
         (d_parse_diag_find(&sink,
                            (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                            (uint16_t)D_PARSE_DIAG_UNKNOWN_OP,
                            0u) < 0)                                   )
    {
        return false;
    }

    d_parse_machine_init(&machine, &sink);
    machine.step_limit = 5L;

    // the operator never halts; the budget must
    while (d_parse_op_set_dispatch(&machine, &ops, 0))
    {
        continue;
    }

    return ( (!machine.ok)                                             &&
             (d_parse_diag_find(&sink,
                                (uint16_t)D_PARSE_DIAG_DOMAIN_MACHINE,
                                (uint16_t)D_PARSE_DIAG_STEP_LIMIT,
                                0u) >= 0)                              );
}

/*
d_tests_parse_c_program_verify
  The instruction stream, built and checked from C.
  Tests the following:
  - a forward branch is emitted with a placeholder and patched later
  - verification checks operands against the family's shapes
  - it marks the branch's landing site, and no other instruction
*/
static bool
d_tests_parse_c_program_verify(void)
{
    struct d_tests_parse_c_toy toy = { 0 };

    if ( (!d_tests_parse_c_toy_build(&toy))                              ||
         (d_parse_program_verify(&toy.program,
                                 &toy.ops,
                                 TOY_SHAPES,
                                 (uint32_t)OP_COUNT,
                                 NULL) != 0)                             )
    {
        return false;
    }

    return ( ((d_parse_program_at(&toy.program, toy.halt)->flags &
               D_PARSE_INSTR_FLAG_TARGET) != 0u)                        &&
             ((d_parse_program_at(&toy.program, 1)->flags &
               D_PARSE_INSTR_FLAG_TARGET) == 0u)                        );
}

/*
d_tests_parse_c_program_identity
  What makes two operands, and two programs, the same.
  Tests the following:
  - interning is canonical: another spelling of a class is the same entry
  - the digest is stable across verification, which only sets annotations
*/
static bool
d_tests_parse_c_program_identity(void)
{
    struct d_tests_parse_c_toy toy     = { 0 };
    struct d_parse_charset     spelled = { 0 };

    if ( (!d_tests_parse_c_toy_build(&toy))                             ||
         (d_parse_charset_parse(&spelled, "0123456789") != 0)           ||
         (d_parse_program_intern_charset(&toy.program,
                                         &spelled) != toy.set)          )
    {
        return false;
    }

    const uint64_t digest = d_parse_program_hash(&toy.program);

    return ( (d_parse_program_verify(&toy.program,
                                     &toy.ops,
                                     TOY_SHAPES,
                                     (uint32_t)OP_COUNT,
                                     NULL) == 0)                        &&
             (d_parse_program_hash(&toy.program) == digest)             );
}

#if (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1)
/*
d_tests_parse_c_program_measure
  The transport format, measured.
  Tests the following:
  - a write with no buffer at all reports how many bytes a write needs
  - a write into a buffer of that size writes exactly that many
*/
static bool
d_tests_parse_c_program_measure(void)
{
    struct d_tests_parse_c_toy toy         = { 0 };
    unsigned char              buffer[512] = { 0 };

    if (!d_tests_parse_c_toy_build(&toy))
    {
        return false;
    }

    const size_t needed = d_parse_program_write(&toy.program, NULL, 0u);

    return ( (needed != 0u)                                     &&
             (needed <= sizeof(buffer))                         &&
             (d_parse_program_write(&toy.program,
                                    buffer,
                                    needed) == needed)          );
}
#endif  // D_INTERNAL_PARSE_PROGRAM_TRANSPORT

// d_tests_parse_c
//   constant: the suite -- every section this build has, the sink before the
// machine that reports into it, and the machine before the program it runs.
const struct d_tests_section d_tests_parse_c[] =
{
    { "parse_c_diagnostics_tally",    d_tests_parse_c_diagnostics_tally    },
    { "parse_c_diagnostics_identity", d_tests_parse_c_diagnostics_identity },
    { "parse_c_machine_dispatch",     d_tests_parse_c_machine_dispatch     },
    { "parse_c_machine_faults",       d_tests_parse_c_machine_faults       },
    { "parse_c_program_verify",       d_tests_parse_c_program_verify       },
#if (D_INTERNAL_PARSE_PROGRAM_TRANSPORT == 1)
    { "parse_c_program_measure",      d_tests_parse_c_program_measure      },
#endif  // D_INTERNAL_PARSE_PROGRAM_TRANSPORT
    { "parse_c_program_identity",     d_tests_parse_c_program_identity     }
};

// d_tests_parse_c_count
//   constant: how many sections the table above holds in this build.
const size_t d_tests_parse_c_count =
    sizeof(d_tests_parse_c) / sizeof(d_tests_parse_c[0]);
