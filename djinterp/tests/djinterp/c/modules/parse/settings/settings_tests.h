/*******************************************************************************
* djinterp [c/parse]                                            settings_tests.h
*
* Parse's settings group (settings.h) in DTest's C face (c/modules/test/
* test_suite.h), by lane E's checklist (knob_and_option.md): the defaults
* from the knobs, each source, the standing, a forced value, the named depth,
* a misspelling's suggestion, and the parse that reads the rows.
*   Each test registers the djin in a registry of its own and takes it off
* again, so the program's registry is never touched. Where the group is
* compiled out (D_CFG_PARSE_SETTINGS 0), each test is skipped, and says why.
*   Two faces, chosen by DTEST_SPEC_MODE: the section unit sees the units
* under test and the fixtures; the runner, which defines DTEST_SPEC_MODE
* first, sees the tests and the spec provider, d_tests_parse_settings_spec().
*
*
* TABLE OF CONTENTS
* =================
* 1.  FIXTURES
*     --------
* 2.  TESTS
*     -----
* 3.  SPEC
*     ----
*     1.  The provider
*
*
* path:      /tests/djinterp/c/modules/parse/settings/settings_tests.h
* link(s):   /docs/djinterp/option/knob_and_option.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.12
*                                                            revised: 2026.11.12
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_SETTINGS_TESTS_H
#define DJINTERP_TESTS_C_PARSE_SETTINGS_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdbool.h>  // bool, true, false
    #include <stddef.h>   // size_t, NULL
    #include <stdlib.h>   // setenv, _putenv
    #include <string.h>   // memset, strcmp, strlen
    // djinterp
    #include "../../../../../../inc/djinterp/c/modules/parse/settings.h"     // the group
    #include "../../../../../../inc/djinterp/c/modules/parse/expr/expr.h"    // the core
    #include "../../../../../../inc/djinterp/c/option/option_env_registry.h" // registries
    // re_std
    #include "../../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FIXTURES
//==============================================================================
// Defined in settings_tests_rows.c; each is described there.
#ifndef DTEST_SPEC_MODE

bool d_tests_settings_open(struct dj_option_env_registry* _registry);
void d_tests_settings_close(void);
int  d_tests_settings_depth(uint32_t _negations, uint32_t _max_depth);

#endif  // DTEST_SPEC_MODE


//==============================================================================
// 2.  TESTS
//==============================================================================
bool d_tests_settings_defaults(void);
bool d_tests_settings_sources(void);
bool d_tests_settings_standing(void);
bool d_tests_settings_named(void);
bool d_tests_settings_suggest(void);
bool d_tests_settings_parse(void);
bool d_tests_settings_unregister(void);


//==============================================================================
// 3.  SPEC
//==============================================================================

// 3.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_settings_rows
//   constant: the rows block's rows.
static const struct dj_test_case_spec d_tests_parse_settings_rows[] =
{
    { "d_tests_settings_defaults",
      "unbound rows read their knobs, and say so",
      d_tests_settings_defaults },
    { "d_tests_settings_sources",
      "a file, the environment and the command line bind rows",
      d_tests_settings_sources },
    { "d_tests_settings_standing",
      "the higher standing holds; a forced value, then given back",
      d_tests_settings_standing },
    { "d_tests_settings_named",
      "unbounded by name; out of range refused",
      d_tests_settings_named },
    { "d_tests_settings_suggest",
      "a misspelt row's nearest name",
      d_tests_settings_suggest }
};

// d_tests_parse_settings_core_rows
//   constant: the core block's rows.
static const struct dj_test_case_spec d_tests_parse_settings_core_rows[] =
{
    { "d_tests_settings_parse",
      "an option of 0 reads the row; a nonzero option holds over it",
      d_tests_settings_parse },
    { "d_tests_settings_unregister",
      "off the registry, the core is back on its knobs",
      d_tests_settings_unregister }
};

// d_tests_parse_settings_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_settings_blocks[] =
{
    { "rows",
      "the djin's rows and where their values come from",
      d_tests_parse_settings_rows,
      sizeof(d_tests_parse_settings_rows) /
          sizeof(d_tests_parse_settings_rows[0]) },
    { "core",
      "expression core v2 reading the rows",
      d_tests_parse_settings_core_rows,
      sizeof(d_tests_parse_settings_core_rows) /
          sizeof(d_tests_parse_settings_core_rows[0]) }
};

// d_tests_parse_settings_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_settings_module =
{
    "parse_settings",
    "parse/settings.h: the djin parse",
    d_tests_parse_settings_blocks,
    sizeof(d_tests_parse_settings_blocks) /
        sizeof(d_tests_parse_settings_blocks[0])
};

// d_tests_parse_settings_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_settings_spec(void)
{
    return &d_tests_parse_settings_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_SETTINGS_TESTS_H
