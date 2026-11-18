/*******************************************************************************
* djinterp [test]                                            text_source_tests.h
*
* The suite for c/text/text_source.h, in DTest's C spec form: making a
* source, its lines, telling an offset as a line and column, and reading and
* writing whole files, one block each.
*   The file tests write in the working directory, which CTest makes the
* build directory, and remove what they write.
*
*
* path:      /tests/djinterp/c/text/text_source_tests.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
/*
TABLE OF CONTENTS
=================
1.  TESTS
    -----
    1.  Lifecycle
    2.  Lines
    3.  Locating
    4.  Files
2.  SPEC
    ----
    1.  The provider
*/
#ifndef DJINTERP_TESTS_C_TEXT_SOURCE_TESTS_H
#define DJINTERP_TESTS_C_TEXT_SOURCE_TESTS_H 1
// djinterp
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // D_TEST_*
// tested
#include "../../../../inc/djinterp/c/text/text_source.h"  // dj_text_source
D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Lifecycle
//------------------------------------------------------------------------------
bool d_tests_text_source_new(void);
bool d_tests_text_source_borrow(void);
bool d_tests_text_source_null(void);

// 1.2    Lines
//------------------------------------------------------------------------------
bool d_tests_text_source_line_count(void);
bool d_tests_text_source_line_spans(void);
bool d_tests_text_source_line_crlf(void);

// 1.3    Locating
//------------------------------------------------------------------------------
bool d_tests_text_source_locate(void);
bool d_tests_text_source_locate_end(void);

// 1.4    Files
//------------------------------------------------------------------------------
bool d_tests_text_source_write_read(void);
bool d_tests_text_source_write_replaces(void);
bool d_tests_text_source_read_missing(void);
bool d_tests_text_source_write_fails(void);


//==============================================================================
// 2.  SPEC
//==============================================================================


// 2.1    The provider
//------------------------------------------------------------------------------
#ifdef DTEST_SPEC_MODE
// d_tests_text_source_lifecycle_rows
//   constant: the lifecycle block's rows.
static const struct dj_test_case_spec d_tests_text_source_lifecycle_rows[] =
{
    D_TEST_CASE(d_tests_text_source_new,
                "new copies the text, terminated; the caller's may change"),
    D_TEST_CASE(d_tests_text_source_borrow,
                "borrow keeps the caller's text, copying nothing"),
    D_TEST_CASE(d_tests_text_source_null,
                "NULL text with a length is refused; NULL is taken elsewhere")
};

// d_tests_text_source_line_rows
//   constant: the lines block's rows.
static const struct dj_test_case_spec d_tests_text_source_line_rows[] =
{
    D_TEST_CASE(d_tests_text_source_line_count,
                "lines run to a newline or the end; no empty line after the last"),
    D_TEST_CASE(d_tests_text_source_line_spans,
                "each line's span excludes its newline; past the last is false"),
    D_TEST_CASE(d_tests_text_source_line_crlf,
                "a CR before a newline stays in the line")
};

// d_tests_text_source_locate_rows
//   constant: the locating block's rows.
static const struct dj_test_case_spec d_tests_text_source_locate_rows[] =
{
    D_TEST_CASE(d_tests_text_source_locate,
                "every offset is its line and byte column, from 1"),
    D_TEST_CASE(d_tests_text_source_locate_end,
                "the end is after the last byte, or opens the next line")
};

// d_tests_text_source_file_rows
//   constant: the files block's rows.
static const struct dj_test_case_spec d_tests_text_source_file_rows[] =
{
    D_TEST_CASE(d_tests_text_source_write_read,
                "a written file reads back byte for byte, CR and NUL kept"),
    D_TEST_CASE(d_tests_text_source_write_replaces,
                "writing over a file replaces it whole, leaving no part file"),
    D_TEST_CASE(d_tests_text_source_read_missing,
                "a file that is not there reads as NULL, length 0"),
    D_TEST_CASE(d_tests_text_source_write_fails,
                "a write that cannot be made says why and leaves nothing")
};

// d_tests_text_source_blocks
//   constant: the module's blocks.
static const struct dj_test_block_spec d_tests_text_source_blocks[] =
{
    { "lifecycle",
      "making and freeing a source",
      d_tests_text_source_lifecycle_rows,
      sizeof(d_tests_text_source_lifecycle_rows) /
          sizeof(d_tests_text_source_lifecycle_rows[0]) },
    { "line",
      "a source's lines and their spans",
      d_tests_text_source_line_rows,
      sizeof(d_tests_text_source_line_rows) /
          sizeof(d_tests_text_source_line_rows[0]) },
    { "locate",
      "an offset told as a line and column",
      d_tests_text_source_locate_rows,
      sizeof(d_tests_text_source_locate_rows) /
          sizeof(d_tests_text_source_locate_rows[0]) },
    { "file",
      "reading and writing whole files",
      d_tests_text_source_file_rows,
      sizeof(d_tests_text_source_file_rows) /
          sizeof(d_tests_text_source_file_rows[0]) }
};

// d_tests_text_source_module
//   constant: the module spec.
static const struct dj_test_module_spec d_tests_text_source_module =
{
    "text_source",
    "c/text: a text as read, by offset and by line, and whole files",
    d_tests_text_source_blocks,
    sizeof(d_tests_text_source_blocks) / sizeof(d_tests_text_source_blocks[0])
};

// d_tests_text_source_spec
//   function: the suite's provider.
D_STATIC_INLINE
const struct dj_test_module_spec*
d_tests_text_source_spec(void)
{
    return &d_tests_text_source_module;
}

#endif  // DTEST_SPEC_MODE


D_EXTERN_C_END


#endif  // DJINTERP_TESTS_C_TEXT_SOURCE_TESTS_H
