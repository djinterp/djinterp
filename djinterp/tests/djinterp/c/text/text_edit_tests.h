/*******************************************************************************
* djinterp [test]                                              text_edit_tests.h
*
* The suite for c/text/text_edit.h, in DTest's C spec form: making a set of
* changes, recording them and refusing overlaps, and applying them, one block
* each.
*
*
* path:      /tests/djinterp/c/text/text_edit_tests.h
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
    2.  Changes
    3.  Applying
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_EDIT_TESTS_H
#define DJINTERP_TESTS_C_TEXT_EDIT_TESTS_H 1
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../inc/djinterp/c/text/text_edit.h"  // dj_text_edit
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Lifecycle
//------------------------------------------------------------------------------
bool d_tests_text_edit_new(void);
bool d_tests_text_edit_clear(void);

// 1.2    Changes
//------------------------------------------------------------------------------
bool d_tests_text_edit_overlaps(void);
bool d_tests_text_edit_touching(void);
bool d_tests_text_edit_bad_arguments(void);

// 1.3    Applying
//------------------------------------------------------------------------------
bool d_tests_text_edit_apply_none(void);
bool d_tests_text_edit_apply_kinds(void);
bool d_tests_text_edit_apply_order(void);
bool d_tests_text_edit_apply_between(void);
bool d_tests_text_edit_apply_past_end(void);
bool d_tests_text_edit_apply_again(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_text_edit_lifecycle_rows
//   constant: the lifecycle block's rows.
static const struct dj_test_case_spec d_tests_text_edit_lifecycle_rows[] =
{
    D_TEST_CASE(d_tests_text_edit_new,
                "a new set has no changes; free takes NULL"),
    D_TEST_CASE(d_tests_text_edit_clear,
                "clear forgets every change and keeps the set for more")
};

// d_tests_text_edit_change_rows
//   constant: the changes block's rows.
static const struct dj_test_case_spec d_tests_text_edit_change_rows[] =
{
    D_TEST_CASE(d_tests_text_edit_overlaps,
                "a change over another is refused, and nothing is kept"),
    D_TEST_CASE(d_tests_text_edit_touching,
                "changes that only touch, or insert at an edge, are kept"),
    D_TEST_CASE(d_tests_text_edit_bad_arguments,
                "no set, an end before the start, no text, or no "
                "change: refused as invalid")
};

// d_tests_text_edit_apply_rows
//   constant: the applying block's rows.
static const struct dj_test_case_spec d_tests_text_edit_apply_rows[] =
{
    D_TEST_CASE(d_tests_text_edit_apply_none,
                "no changes, or no set, give a terminated copy"),
    D_TEST_CASE(d_tests_text_edit_apply_kinds,
                "a replacement, an insertion and a deletion apply as made"),
    D_TEST_CASE(d_tests_text_edit_apply_order,
                "by position; at one place insertions first, as made"),
    D_TEST_CASE(d_tests_text_edit_apply_between,
                "the hook is asked at each meeting, and its byte put between"),
    D_TEST_CASE(d_tests_text_edit_apply_past_end,
                "a change past the text's end gives NULL and length 0"),
    D_TEST_CASE(d_tests_text_edit_apply_again,
                "the changes stay: applying twice gives the same text")
};

// d_tests_text_edit_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_text_edit_blocks[] =
{
    { "lifecycle",
      "making, clearing and freeing a set of changes",
      d_tests_text_edit_lifecycle_rows,
      sizeof(d_tests_text_edit_lifecycle_rows) /
          sizeof(d_tests_text_edit_lifecycle_rows[0]) },
    { "change",
      "recording changes, and refusing overlaps",
      d_tests_text_edit_change_rows,
      sizeof(d_tests_text_edit_change_rows) /
          sizeof(d_tests_text_edit_change_rows[0]) },
    { "apply",
      "applying the changes to a text",
      d_tests_text_edit_apply_rows,
      sizeof(d_tests_text_edit_apply_rows) /
          sizeof(d_tests_text_edit_apply_rows[0]) }
};

// d_tests_text_edit_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_text_edit_module =
{
    "text_edit",
    "c/text: changes to a text, kept apart until applied",
    d_tests_text_edit_blocks,
    sizeof(d_tests_text_edit_blocks) / sizeof(d_tests_text_edit_blocks[0])
};

// d_tests_text_edit_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_text_edit_spec(void)
{
    return &d_tests_text_edit_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_EDIT_TESTS_H
