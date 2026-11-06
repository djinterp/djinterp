/*******************************************************************************
* djinterp [c/parse]                                          t_parse_overflow.c
*
* The upgrade audit's allocation sites in parse and parsegen (lane A's
* addendum of 2026.11.04, 22 sites): each now computes its size with
* d_ckd_mul before the call, and refuses a count too large cleanly.
*   A site whose count is a size_t (the syntax module's, the argument
* parser's growth) is driven past the size_t range on every target. A site
* whose count is 32 bits multiplies within a 64-bit size_t whatever the
* count, so its overflow exists only where size_t is 32 bits: there it is
* driven past it, and elsewhere the section says it cannot be and checks the
* site still allocates as before. The EBNF parser's sites take their count
* from their own growth, not from a caller, and have no section.
*
*
* path:      /tests/djinterp/c/modules/parse/t_parse_overflow.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <stdbool.h>  // bool, true, false
#include <stddef.h>   // size_t, NULL
#include <stdio.h>    // printf
#include <stdlib.h>   // free
#include <string.h>   // memset
// djinterp
#include "../../../../../inc/djinterp/c/modules/parse/diagnostic.h"            // the sink
#include "../../../../../inc/djinterp/c/modules/parse/machine.h"               // the op set
#include "../../../../../inc/djinterp/c/modules/parse/pool.h"                  // the pool
#include "../../../../../inc/djinterp/c/modules/parse/program.h"               // the program
#include "../../../../../inc/djinterp/c/modules/parse/syntax/syntax_group.h"   // d_syntax_group
#include "../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"    // d_syntax_items_build
#include "../../../../../inc/djinterp/c/modules/parsegen/grammar.h"            // the grammar
#include "../../../../../inc/djinterp/c/modules/parsegen/registry.h"           // the registry
#include "../../../../../src/djinterp/c/modules/parse/cli/arg_parser/arg_internal.h"  // d_arg_internal_grow
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


// WIDE
//   constant: whether size_t is wider than 32 bits, so that a 32-bit count
// times a record's size cannot overflow it.
static const bool WIDE = (sizeof(size_t) > 4u);

/*
d_tests_overflow_expect
  Reports a check that failed, and folds it into the section's result.
*/
static bool
d_tests_overflow_expect(
    bool        _held,
    const char* _what
)
{
    if (!_held)
    {
        printf("    failed: %s\n", _what);
    }

    return _held;
}

/*
d_tests_overflow_syntax
  The syntax module's three sites (syntax_group.c 148 and 149, syntax_item.c
2026 to 2031).
  Tests the following:
  - a count past the size_t range, or past 32 bits, is refused, with nothing
    allocated
*/
static bool
d_tests_overflow_syntax(void)
{
    struct dj_token         token;
    struct dj_syntax_groups groups;
    struct dj_syntax_items  items;
    bool                    ok = true;

    memset(&token, 0, sizeof(token));
    memset(&groups, 0, sizeof(groups));
    memset(&items, 0, sizeof(items));

    ok = d_tests_overflow_expect(
             ( (!d_syntax_group(&token, ((size_t)-1 / 2u) + 1u, &groups)) &&
               (!groups.parent) && (!groups.match) ),
             "d_syntax_group refuses a count too large");
    ok = d_tests_overflow_expect(
             (ok) &&
             (!d_syntax_items_build(&token, ((size_t)-1 / 2u) + 1u, &groups,
                                    "", &items)) &&
             (!items.item_of) && (!items.level_of) && (!items.pp_of),
             "d_syntax_items_build refuses a count too large");

    return ok;
}

/*
d_tests_overflow_arg_grow
  The argument parser's growth (arg_internal.c 2295).
  Tests the following:
  - an element size whose product with the capacity passes the size_t range
    is refused, the array untouched
  - a sound request still grows
*/
static bool
d_tests_overflow_arg_grow(void)
{
    size_t capacity = 0u;
    void*  refused  = d_arg_internal_grow(NULL, 0u, 3u,
                                          ((size_t)-1 / 2u) + 1u, &capacity);
    void*  grown    = d_arg_internal_grow(NULL, 0u, 3u, sizeof(int),
                                          &capacity);
    bool   ok       = d_tests_overflow_expect((!refused) &&
                                              (grown) && (capacity == 4u),
                                              "the growth is checked");

    free(grown);

    return ok;
}

/*
d_tests_overflow_heap_inits
  The foundation's and parsegen's heap constructors (diagnostic.c 152,
machine.c 153, pool.c 110, program.c 117, grammar.c 153 and 162, registry.c
111; analysis.c's five take their counts from a grammar, which these bounds
keep).
  Tests the following:
  - where size_t is 32 bits, a count of 2^32 - 1 is refused cleanly
  - everywhere, a sound count still allocates and releases
*/
static bool
d_tests_overflow_heap_inits(void)
{
    struct dj_parse_diag_sink   sink;
    struct dj_parse_op_set      set;
    struct dj_parse_pool        pool;
    struct dj_parse_program     program;
    struct dj_parsegen_grammar  grammar;
    struct dj_parsegen_registry registry;
    const uint32_t              most = 0xFFFFFFFFu;
    bool                        ok   = true;

    // the overflow, only where it can happen
    if (!WIDE)
    {
        ok = d_tests_overflow_expect(
                 (d_parse_diag_sink_init_heap(&sink, most, 16u) != 0) &&
                 (d_parse_op_set_init_heap(&set, 1u, most) != 0) &&
                 (d_parse_pool_init_heap(&pool, 16u, most) != 0) &&
                 (d_parse_program_init_heap(&program, 1u, most) != 0) &&
                 (d_parsegen_grammar_init_heap(&grammar, most, 4u) != 0) &&
                 (d_parsegen_registry_init_heap(&registry, most) != 0),
                 "each constructor refuses a count too large");
    }
    else
    {
        printf("    (a 32-bit count cannot overflow a 64-bit size_t: the "
               "refusals run on 32-bit targets)\n");
    }

    // the sound path, unchanged
    ok = d_tests_overflow_expect(
             (ok) &&
             (d_parse_diag_sink_init_heap(&sink, 8u, 64u) == 0) &&
             (d_parse_op_set_init_heap(&set, 1u, 8u) == 0) &&
             (d_parse_pool_init_heap(&pool, 64u, 8u) == 0) &&
             (d_parse_program_init_heap(&program, 1u, 8u) == 0) &&
             (d_parsegen_grammar_init_heap(&grammar, 8u, 4u) == 0) &&
             (d_parsegen_registry_init_heap(&registry, 8u) == 0),
             "each constructor still allocates");

    d_parse_diag_sink_release(&sink);
    d_parse_op_set_release(&set);
    d_parse_pool_release(&pool);
    d_parse_program_release(&program);
    d_parsegen_grammar_release(&grammar);
    d_parsegen_registry_release(&registry);

    return ok;
}

/*
main
  Runs the three sections.
*/
int
main(void)
{
    const bool syntax = d_tests_overflow_syntax();
    const bool grow   = d_tests_overflow_arg_grow();
    const bool inits  = d_tests_overflow_heap_inits();

    printf("  [%s] overflow_syntax\n", (syntax) ? "PASS" : "FAIL");
    printf("  [%s] overflow_arg_grow\n", (grow) ? "PASS" : "FAIL");
    printf("  [%s] overflow_heap_inits\n", (inits) ? "PASS" : "FAIL");

    return ( (syntax) && (grow) && (inits) ) ? 0 : 1;
}
