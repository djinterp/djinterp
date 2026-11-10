/*******************************************************************************
* djinterp [c/parse]                                      overflow_tests_sites.c
*
* The overflow suite, block "sites": each allocation site that takes a count
* from a caller, driven past what it can allocate.
*
*
* path:      /tests/djinterp/c/modules/parse/overflow/overflow_tests_sites.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/
#include "./overflow_tests.h"  // corresponding header


// D_TESTS_OVERFLOW_HALF
//   constant: a count whose double passes the size_t range.
#define D_TESTS_OVERFLOW_HALF   (((size_t)-1 / 2u) + 1u)

/*
d_tests_overflow_wide
  Whether size_t is wider than 32 bits, so that a 32-bit count times a
record's size cannot overflow it.
*/
static bool
d_tests_overflow_wide(void)
{
    return (sizeof(size_t) > 4u);
}

/*
d_tests_overflow_syntax
  The syntax module's three sites (syntax_group.c 148 and 149, syntax_item.c
2026 to 2031).
  Tests the following:
  - a count past the size_t range, or past 32 bits, is refused, with nothing
    allocated
*/
bool
d_tests_overflow_syntax(void)
{
    struct dj_token         token;
    struct dj_syntax_groups groups;
    struct dj_syntax_items  items;

    memset(&token, 0, sizeof(token));
    memset(&groups, 0, sizeof(groups));
    memset(&items, 0, sizeof(items));

    D_TEST_REQUIRE(!d_syntax_group(&token, D_TESTS_OVERFLOW_HALF, &groups));
    D_TEST_REQUIRE( (!groups.parent) && (!groups.match) );
    D_TEST_REQUIRE(!d_syntax_items_build(&token, D_TESTS_OVERFLOW_HALF,
                                         &groups, "", &items));
    D_TEST_REQUIRE( (!items.item_of) && (!items.level_of) &&
                    (!items.pp_of) );

    return true;
}

/*
d_tests_overflow_arg_grow
  The argument parser's growth (arg_internal.c 2295).
  Tests the following:
  - an element size whose product with the capacity passes the size_t range
    is refused, the array untouched
  - a sound request still grows
*/
bool
d_tests_overflow_arg_grow(void)
{
    size_t     capacity = 0u;
    void*      refused  = d_arg_internal_grow(NULL, 0u, 3u,
                                              D_TESTS_OVERFLOW_HALF,
                                              &capacity);
    void*      grown    = d_arg_internal_grow(NULL, 0u, 3u, sizeof(int),
                                              &capacity);
    const bool sound    = ( (grown) && (capacity == 4u) );

    free(grown);
    free(refused);

    D_TEST_REQUIRE(!refused);
    D_TEST_REQUIRE(sound);

    return true;
}

/*
d_tests_overflow_heap_refused
  The foundation's and parsegen's heap constructors (diagnostic.c 152,
machine.c 153, pool.c 110, program.c 117, grammar.c 153 and 162, registry.c
111; analysis.c's five take their counts from a grammar, which these bounds
keep), where size_t is 32 bits.
  Tests the following:
  - a count of 2^32 - 1 is refused cleanly
  - elsewhere the test is skipped: a 32-bit count cannot overflow a wider
    size_t
*/
bool
d_tests_overflow_heap_refused(void)
{
    struct dj_parse_diag_sink   sink;
    struct dj_parse_op_set      set;
    struct dj_parse_pool        pool;
    struct dj_parse_program     program;
    struct dj_parsegen_grammar  grammar;
    struct dj_parsegen_registry registry;
    const uint32_t              most = 0xFFFFFFFFu;

    if (d_tests_overflow_wide())
    {
        D_TEST_SKIP("a 32-bit count cannot overflow a 64-bit size_t: the "
                    "refusals run on 32-bit targets");
    }

    D_TEST_REQUIRE(d_parse_diag_sink_init_heap(&sink, most, 16u) != 0);
    D_TEST_REQUIRE(d_parse_op_set_init_heap(&set, 1u, most) != 0);
    D_TEST_REQUIRE(d_parse_pool_init_heap(&pool, 16u, most) != 0);
    D_TEST_REQUIRE(d_parse_program_init_heap(&program, 1u, most) != 0);
    D_TEST_REQUIRE(d_parsegen_grammar_init_heap(&grammar, most, 4u) != 0);
    D_TEST_REQUIRE(d_parsegen_registry_init_heap(&registry, most) != 0);

    return true;
}

/*
d_tests_overflow_heap_sound
  The same constructors on a sound count.
  Tests the following:
  - each still allocates, and releases
*/
bool
d_tests_overflow_heap_sound(void)
{
    struct dj_parse_diag_sink   sink;
    struct dj_parse_op_set      set;
    struct dj_parse_pool        pool;
    struct dj_parse_program     program;
    struct dj_parsegen_grammar  grammar;
    struct dj_parsegen_registry registry;

    const bool sink_made     = (d_parse_diag_sink_init_heap(&sink, 8u,
                                                            64u) == 0);
    const bool set_made      = (d_parse_op_set_init_heap(&set, 1u, 8u) == 0);
    const bool pool_made     = (d_parse_pool_init_heap(&pool, 64u, 8u) == 0);
    const bool program_made  = (d_parse_program_init_heap(&program, 1u,
                                                          8u) == 0);
    const bool grammar_made  = (d_parsegen_grammar_init_heap(&grammar, 8u,
                                                             4u) == 0);
    const bool registry_made = (d_parsegen_registry_init_heap(&registry,
                                                              8u) == 0);

    d_parse_diag_sink_release(&sink);
    d_parse_op_set_release(&set);
    d_parse_pool_release(&pool);
    d_parse_program_release(&program);
    d_parsegen_grammar_release(&grammar);
    d_parsegen_registry_release(&registry);

    D_TEST_REQUIRE(sink_made);
    D_TEST_REQUIRE(set_made);
    D_TEST_REQUIRE(pool_made);
    D_TEST_REQUIRE(program_made);
    D_TEST_REQUIRE(grammar_made);
    D_TEST_REQUIRE(registry_made);

    return true;
}
