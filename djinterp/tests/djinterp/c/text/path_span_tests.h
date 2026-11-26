/*******************************************************************************
* djinterp [c][text]                                           path_span_tests.h
*
* The suite for c/text/path_span.h, in DTest's C spec form. The generic path
* is read through four grammars a caller might write, none of which ships
* with the type: a POSIX filesystem's, a Windows filesystem's, a URL's and an
* n-ary tree's. A block for the POSIX grammar, one for the other three, and
* one for empty paths.
*
*
* path:      /tests/djinterp/c/text/path_span_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.21
*                                                            revised: 2026.11.23
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  A POSIX path
    2.  Other grammars
    3.  Nothing
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_PATH_SPAN_TESTS_H
#define DJINTERP_TESTS_C_TEXT_PATH_SPAN_TESTS_H 1
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../inc/djinterp/c/text/path_span.h"  // dj_path_span
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    A POSIX path
//------------------------------------------------------------------------------
bool d_tests_path_span_read(void);
bool d_tests_path_span_fold(void);
bool d_tests_path_span_order(void);
bool d_tests_path_span_join(void);
bool d_tests_path_span_render(void);

// 1.2    Other grammars
//------------------------------------------------------------------------------
bool d_tests_path_span_windows(void);
bool d_tests_path_span_url(void);
bool d_tests_path_span_tree(void);

// 1.3    Nothing
//------------------------------------------------------------------------------
bool d_tests_path_span_empty(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_path_span_posix_rows
//   constant: the POSIX block's rows.
static const struct dj_test_case_spec d_tests_path_span_posix_rows[] =
{
    D_TEST_CASE(d_tests_path_span_read,
                "the anchor, the labels one by one, the last, the level"),
    D_TEST_CASE(d_tests_path_span_fold,
                "dot labels folded; a relative path keeps the .. it can't"),
    D_TEST_CASE(d_tests_path_span_order,
                "the meet of two paths, and prefixes by label, not byte"),
    D_TEST_CASE(d_tests_path_span_join,
                "a reference taken from a base: .. folds, an anchor stands"),
    D_TEST_CASE(d_tests_path_span_render,
                "as snprintf: cut short, terminated, the length it needed")
};

// d_tests_path_span_grammar_rows
//   constant: the other grammars' rows.
static const struct dj_test_case_spec d_tests_path_span_grammar_rows[] =
{
    D_TEST_CASE(d_tests_path_span_windows,
                "a drive anchors; either separator; case folded"),
    D_TEST_CASE(d_tests_path_span_url,
                "the authority anchors; the path ends at ? or #"),
    D_TEST_CASE(d_tests_path_span_tree,
                "child indices as labels; siblings meet at their parent")
};

// d_tests_path_span_empty_rows
//   constant: the empty block's rows.
static const struct dj_test_case_spec d_tests_path_span_empty_rows[] =
{
    D_TEST_CASE(d_tests_path_span_empty,
                "no text, or no span: no labels, no anchor, nothing written")
};

// d_tests_path_span_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_path_span_blocks[] =
{
    { "posix",
      "a POSIX filesystem's grammar",
      d_tests_path_span_posix_rows,
      sizeof(d_tests_path_span_posix_rows) /
          sizeof(d_tests_path_span_posix_rows[0]) },
    { "grammars",
      "a Windows filesystem's, a URL's and an n-ary tree's grammars",
      d_tests_path_span_grammar_rows,
      sizeof(d_tests_path_span_grammar_rows) /
          sizeof(d_tests_path_span_grammar_rows[0]) },
    { "empty",
      "a path with no text, and no path at all",
      d_tests_path_span_empty_rows,
      sizeof(d_tests_path_span_empty_rows) /
          sizeof(d_tests_path_span_empty_rows[0]) }
};

// d_tests_path_span_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_path_span_module =
{
    "path_span",
    "c/text: a path read through a caller's grammar",
    d_tests_path_span_blocks,
    sizeof(d_tests_path_span_blocks) / sizeof(d_tests_path_span_blocks[0])
};

// d_tests_path_span_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_path_span_spec(void)
{
    return &d_tests_path_span_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_PATH_SPAN_TESTS_H
