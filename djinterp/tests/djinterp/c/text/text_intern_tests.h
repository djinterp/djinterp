/*******************************************************************************
* djinterp [test]                                            text_intern_tests.h
*
* The suite for c/text/text_intern.h, in DTest's C spec form: making and
* freeing a table, adding and finding names, and reading them back, one
* block each.
*
*
* path:      /tests/djinterp/c/text/text_intern_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.18
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  Lifecycle
    2.  Adding and finding
    3.  Reading
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_INTERN_TESTS_H
#define DJINTERP_TESTS_C_TEXT_INTERN_TESTS_H 1
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../inc/djinterp/c/text/text_intern.h"  // dj_text_intern
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Lifecycle
//------------------------------------------------------------------------------
bool d_tests_text_intern_new(void);
bool d_tests_text_intern_null(void);

// 1.2    Adding and finding
//------------------------------------------------------------------------------
bool d_tests_text_intern_dense(void);
bool d_tests_text_intern_counted(void);
bool d_tests_text_intern_find(void);
bool d_tests_text_intern_growth(void);

// 1.3    Reading
//------------------------------------------------------------------------------
bool d_tests_text_intern_text(void);
bool d_tests_text_intern_embedded_nul(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_text_intern_lifecycle_rows
//   constant: the lifecycle block's rows.
static const struct dj_test_case_spec d_tests_text_intern_lifecycle_rows[] =
{
    D_TEST_CASE(d_tests_text_intern_new,
                "a new table is empty, and frees with or without names"),
    D_TEST_CASE(d_tests_text_intern_null,
                "every function takes NULL: queries answer none, adds fail")
};

// d_tests_text_intern_add_rows
//   constant: the adding and finding block's rows.
static const struct dj_test_case_spec d_tests_text_intern_add_rows[] =
{
    D_TEST_CASE(d_tests_text_intern_dense,
                "symbols are dense, in first-seen order; a name keeps its own"),
    D_TEST_CASE(d_tests_text_intern_counted,
                "a name is its length's bytes: unterminated, empty or prefix"),
    D_TEST_CASE(d_tests_text_intern_find,
                "find gives an added name's symbol and adds nothing on a miss"),
    D_TEST_CASE(d_tests_text_intern_growth,
                "ten thousand names survive every rehash and arena growth")
};

// d_tests_text_intern_read_rows
//   constant: the reading block's rows.
static const struct dj_test_case_spec d_tests_text_intern_read_rows[] =
{
    D_TEST_CASE(d_tests_text_intern_text,
                "text and length give the name back, terminated; "
                "unknown is \"\""),
    D_TEST_CASE(d_tests_text_intern_embedded_nul,
                "a name holding a NUL is its own, and length counts past it")
};

// d_tests_text_intern_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_text_intern_blocks[] =
{
    { "lifecycle",
      "making and freeing a table",
      d_tests_text_intern_lifecycle_rows,
      sizeof(d_tests_text_intern_lifecycle_rows) /
          sizeof(d_tests_text_intern_lifecycle_rows[0]) },
    { "add",
      "adding names, and finding them",
      d_tests_text_intern_add_rows,
      sizeof(d_tests_text_intern_add_rows) /
          sizeof(d_tests_text_intern_add_rows[0]) },
    { "read",
      "reading a symbol's name back",
      d_tests_text_intern_read_rows,
      sizeof(d_tests_text_intern_read_rows) /
          sizeof(d_tests_text_intern_read_rows[0]) }
};

// d_tests_text_intern_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_text_intern_module =
{
    "text_intern",
    "c/text: names stood for by small integers",
    d_tests_text_intern_blocks,
    sizeof(d_tests_text_intern_blocks) / sizeof(d_tests_text_intern_blocks[0])
};

// d_tests_text_intern_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_text_intern_spec(void)
{
    return &d_tests_text_intern_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_INTERN_TESTS_H
