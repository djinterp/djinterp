/*******************************************************************************
* djinterp [c][parse]                                          parse_log_tests.h
*
* Parse's log unit (parse_log.h) in DTest's C face (c/modules/test/
* test_suite.h): its two tables, checked as the log checks a module's; the
* records the subscriber writes, each line as the log's text writer writes
* it at a fixed time; its notes, its own words and what it leaves out; the
* log's body and parse's text, written from one template by two writers,
* agreeing for every row; and the switch parse.log.
*   Each test sets the log back to its defaults, writing to a buffer of its
* own at a fixed time. Where the unit is compiled out (D_CFG_PARSE_LOG 0, or
* a build below the log's 64-bit floor), each test is skipped, and says why.
*   Two faces, chosen by DTEST_SPEC_MODE: the section unit sees the units
* under test; the runner, which defines DTEST_SPEC_MODE first, sees the tests
* and the spec provider, d_tests_parse_log_spec().
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
* path:      /tests/djinterp/c/modules/parse/log/parse_log_tests.h
* link(s):   /docs/djinterp/log/log_design.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.23
*                                                            revised: 2026.11.23
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_PARSE_LOG_TESTS_H
#define DJINTERP_TESTS_C_PARSE_LOG_TESTS_H 1

// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*, specs
#ifndef DTEST_SPEC_MODE
    // std
    #include <stdbool.h>  // bool, true, false
    #include <stddef.h>   // size_t, NULL
    #include <stdio.h>    // printf
    #include <string.h>   // memset, strcmp, strstr
    // djinterp
    #include "../../../../../../inc/djinterp/c/modules/parse/parse_log.h"  // the unit
    #include "../../../../../../inc/djinterp/c/modules/parse/settings.h"   // parse.log
#endif  // DTEST_SPEC_MODE


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================
bool d_tests_parse_log_tables(void);
bool d_tests_parse_log_records(void);
bool d_tests_parse_log_notes(void);
bool d_tests_parse_log_left_out(void);
bool d_tests_parse_log_agree(void);
bool d_tests_parse_log_switch(void);


//==============================================================================
// 2.  SPEC
//==============================================================================

// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE

// d_tests_parse_log_rows
//   constant: the one block's rows.
static const struct dj_test_case_spec d_tests_parse_log_rows[] =
{
    { "d_tests_parse_log_tables",
      "parse.expr's and parse.core's tables, checked and registered",
      d_tests_parse_log_tables },
    { "d_tests_parse_log_records",
      "a diagnostic as one record: its code, its body, its facts",
      d_tests_parse_log_records },
    { "d_tests_parse_log_notes",
      "a note as a level; several as a level of levels",
      d_tests_parse_log_notes },
    { "d_tests_parse_log_left_out",
      "words of its own; what is not written, and the log's own levels",
      d_tests_parse_log_left_out },
    { "d_tests_parse_log_agree",
      "for every row, the log's body is the text parse stores",
      d_tests_parse_log_agree },
    { "d_tests_parse_log_switch",
      "parse.log off attaches nothing; detaching",
      d_tests_parse_log_switch }
};

// d_tests_parse_log_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_parse_log_blocks[] =
{
    { "subscriber",
      "parse's tables and its records in the log",
      d_tests_parse_log_rows,
      sizeof(d_tests_parse_log_rows) / sizeof(d_tests_parse_log_rows[0]) }
};

// d_tests_parse_log_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_parse_log_module =
{
    "parse_log",
    "parse/parse_log.h: parse's diagnostics as the log's records",
    d_tests_parse_log_blocks,
    sizeof(d_tests_parse_log_blocks) / sizeof(d_tests_parse_log_blocks[0])
};

// d_tests_parse_log_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_parse_log_spec(void)
{
    return &d_tests_parse_log_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_PARSE_LOG_TESTS_H
