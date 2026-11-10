/*******************************************************************************
* djinterp [c/parse]                                            overflow_tests.h
*
* The upgrade audit's allocation sites in parse and parsegen (lane A's
* addendum of 2026.11.04, 22 sites) in DTest's C face (c/modules/test/
* test_suite.h): each site computes its size with d_ckd_mul before the call,
* and refuses a count too large cleanly.
*   A site whose count is a size_t (the syntax module's, the argument
* parser's growth) is driven past the size_t range on every target. A site
* whose count is 32 bits multiplies within a 64-bit size_t whatever the
* count, so its overflow exists only where size_t is 32 bits: there it is
* driven past it, and elsewhere its test is skipped, saying why, while a
* second test checks the site still allocates as before. The EBNF parser's
* sites take their count from their own growth, not from a caller, and have
* no test.
*   Two faces, chosen by DTEST_SPEC_MODE: the section unit sees the units
* under test; the runner, which defines DTEST_SPEC_MODE first, sees the tests
* and the spec provider, d_tests_parse_overflow_spec().
*
*
* TABLE OF CONTENTS
* =================
* 1.  TESTS
*     -----
* 2.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/modules/parse/overflow/overflow_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.10
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_OVERFLOW_TESTS_H
#define DJINTERP_TESTS_C_PARSE_OVERFLOW_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdbool.h>  // bool, true, false
    #include <stddef.h>   // size_t, NULL
    #include <stdlib.h>   // free
    #include <string.h>   // memset
    // djinterp
    #include "../../../../../../inc/djinterp/c/modules/parse/diagnostic.h"           // the sink
    #include "../../../../../../inc/djinterp/c/modules/parse/machine.h"              // the op set
    #include "../../../../../../inc/djinterp/c/modules/parse/pool.h"                 // the pool
    #include "../../../../../../inc/djinterp/c/modules/parse/program.h"              // the program
    #include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_group.h"  // d_syntax_group
    #include "../../../../../../inc/djinterp/c/modules/parse/syntax/syntax_item.h"   // d_syntax_items_build
    #include "../../../../../../inc/djinterp/c/modules/parsegen/grammar.h"           // the grammar
    #include "../../../../../../inc/djinterp/c/modules/parsegen/registry.h"          // the registry
    #include "../../../../../../src/djinterp/c/modules/parse/cli/arg_parser/arg_internal.h"  // d_arg_internal_grow
    // re_std
    #include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================
// Defined in overflow_tests_sites.c; each is described there.
bool d_tests_overflow_syntax(void);
bool d_tests_overflow_arg_grow(void);
bool d_tests_overflow_heap_refused(void);
bool d_tests_overflow_heap_sound(void);


//==============================================================================
// 2.  SPEC
//==============================================================================

// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_overflow_sites_rows
//   constant: the sites block's rows.
static const struct dj_test_case_spec d_tests_parse_overflow_sites_rows[] =
{
    { "d_tests_overflow_syntax",
      "the syntax module refuses a count past size_t",
      d_tests_overflow_syntax },
    { "d_tests_overflow_arg_grow",
      "the argument parser's growth refuses a size past size_t",
      d_tests_overflow_arg_grow },
    { "d_tests_overflow_heap_refused",
      "each heap constructor refuses a 32-bit count past size_t",
      d_tests_overflow_heap_refused },
    { "d_tests_overflow_heap_sound",
      "each heap constructor still allocates a sound count",
      d_tests_overflow_heap_sound }
};

// d_tests_parse_overflow_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_overflow_blocks[] =
{
    { "sites",
      "the allocation sites that take a count from a caller",
      d_tests_parse_overflow_sites_rows,
      sizeof(d_tests_parse_overflow_sites_rows) /
          sizeof(d_tests_parse_overflow_sites_rows[0]) }
};

// d_tests_parse_overflow_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_overflow_module =
{
    "parse_overflow",
    "parse and parsegen: counts too large refused",
    d_tests_parse_overflow_blocks,
    sizeof(d_tests_parse_overflow_blocks) /
        sizeof(d_tests_parse_overflow_blocks[0])
};

// d_tests_parse_overflow_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_overflow_spec(void)
{
    return &d_tests_parse_overflow_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_OVERFLOW_TESTS_H
