/*******************************************************************************
* djinterp [c][text]                                               emoji_tests.h
*
* The suite for c/text/symbol/emoji/emoji.h's UTF-8 encoders, in DTest's C
* spec form: d_text_emoji_utf8_encode at both ends of every UTF-8 length and
* on each kind of value it must refuse; d_text_emoji_utf8, d_text_emoji_utf8_seq
* and their macros against the header's own literals and at the edges of
* D_EMOJI_SEQ_CAPACITY; and that a refused call leaves the last result as it
* was, and each function its own buffer.
*
*
* path:      /tests/djinterp/c/text/symbol/emoji/emoji_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.23
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_SYMBOL_EMOJI_TESTS_H
#define DJINTERP_TESTS_C_TEXT_SYMBOL_EMOJI_TESTS_H 1
// djinterp
#include "../../../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest
// tested
#include "../../../../../../inc/djinterp/c/text/symbol/emoji/emoji.h"  // emoji
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


bool d_tests_emoji_encode(void);
bool d_tests_emoji_utf8(void);
bool d_tests_emoji_seq(void);
bool d_tests_emoji_capacity(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_emoji_rows
//   constant: the block's rows.
static const struct dj_test_case_spec d_tests_emoji_rows[] =
{
    D_TEST_CASE(d_tests_emoji_encode,
                "each UTF-8 length's ends; surrogates and past U+10FFFF "
                "refused"),
    D_TEST_CASE(d_tests_emoji_utf8,
                "a code point as the header's literal; a refusal keeps the "
                "last"),
    D_TEST_CASE(d_tests_emoji_seq,
                "ZWJ sequences as the header's literals; the 35-byte kiss"),
    D_TEST_CASE(d_tests_emoji_capacity,
                "D_EMOJI_SEQ_CAPACITY - 1 bytes fit; each function its own "
                "buffer")
};

// d_tests_emoji_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_emoji_blocks[] =
{
    { "utf8",
      "code points and sequences written as UTF-8",
      d_tests_emoji_rows,
      sizeof(d_tests_emoji_rows) / sizeof(d_tests_emoji_rows[0]) }
};

// d_tests_emoji_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_emoji_module =
{
    "emoji",
    "c/text/symbol/emoji: emoji as UTF-8",
    d_tests_emoji_blocks,
    sizeof(d_tests_emoji_blocks) / sizeof(d_tests_emoji_blocks[0])
};

// d_tests_emoji_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_emoji_spec(void)
{
    return &d_tests_emoji_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_SYMBOL_EMOJI_TESTS_H
