/*******************************************************************************
* djinterp [text]                                            text_lineup_tests.c
*
*
* path:      /tests/djinterp/c/text/text_lineup_tests.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./text_lineup_tests.h"
#include "../../../../inc/djinterp/c/modules/test/test_suite.h"  // DTest's
                                                                // C face


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_text_lineup_setup
  Sets up the test environment for text_lineup tests.
Return:
  true (no setup actions currently required; present for extensibility).
*/
bool d_tests_text_lineup_setup(void)
{
    // no specific setup required for text_lineup tests;
    // function retained for consistency and future use.

    return true;
}

/*
d_tests_text_lineup_teardown
  Cleans up the test environment.
Return:
  true (no teardown actions currently required).
*/
bool d_tests_text_lineup_teardown(void)
{
    // no specific teardown required;
    // retained for consistency and future use.

    return true;
}

/*
d_tests_text_lineup_make_buffer
  Creates a dj_text_buffer for use in tests, wrapping d_text_buffer_new.

Parameter(s):
  _capacity: initial byte capacity to request.
Return:
  A pointer to the newly allocated buffer, or NULL on allocation failure.
*/
struct dj_text_buffer* d_tests_text_lineup_make_buffer(
    size_t _capacity
)
{
    return d_text_buffer_new(_capacity);
}

/*
d_tests_text_lineup_make_token_array
  Allocates a dj_string_array and appends the given variable-argument
strings to it.

Parameter(s):
  _count: number of string arguments that follow.
  ...:    const char* strings to append.
Return:
  A pointer to the populated string array, or NULL on failure.
*/
struct dj_string_array* d_tests_text_lineup_make_token_array(
    size_t _count,
    ...
)
{
    struct dj_string_array* array;
    va_list                args;
    const char*            str;
    size_t                 i;

    array = d_string_array_new(_count);

    if (!array)
    {
        return NULL;
    }

    va_start(args, _count);

    for (i = 0; i < _count; i++)
    {
        str = va_arg(args, const char*);

        if (!d_string_array_append(array, str))
        {
            va_end(args);
            d_string_array_free(array);

            return NULL;
        }
    }

    va_end(args);

    return array;
}

/*
d_tests_text_lineup_make_template
  Populates and returns a dj_text_template value from individual fields.
All pointer fields are assigned directly (no copies); callers retain
ownership of the pointed-to data.

Parameter(s):
  _tokens:          token array (must not be NULL for valid use).
  _separator:       inter-token separator string, or NULL.
  _sep_before:      whether separator precedes the first token.
  _sep_after:       whether separator follows the last token.
  _line_prefix:     per-line prefix string, or NULL.
  _line_suffix:     per-line suffix string, or NULL.
  _min_token_width: minimum column width applied to every token slot.
  _flags:           bitmask of dj_text_lineup_flag values.
Return:
  A fully initialised dj_text_lineup_template value.
*/
struct dj_text_lineup_template d_tests_text_lineup_make_template(
    struct dj_string_array* _tokens,
    const char*             _separator,
    bool                    _sep_before,
    bool                    _sep_after,
    const char*             _line_prefix,
    const char*             _line_suffix,
    size_t                  _min_token_width,
    uint32_t                _flags
)
{
    struct dj_text_lineup_template tmpl;

    tmpl.tokens           = _tokens;
    tmpl.separator        = (char*)_separator;
    tmpl.separator_before = _sep_before;
    tmpl.separator_after  = _sep_after;
    tmpl.line_prefix      = (char*)_line_prefix;
    tmpl.line_suffix      = (char*)_line_suffix;
    tmpl.min_token_width  = _min_token_width;
    tmpl.flags            = _flags;

    return tmpl;
}


/******************************************************************************
 * MASTER TEST RUNNER
 *****************************************************************************/

/*
d_tests_text_lineup_run_all
  Master test runner for all text_lineup module tests.
  Tests the following:
  - dj_string_array operations (Section II)
  - d_text_lineup formatting (Section III)
*/
bool d_tests_text_lineup_run_all(struct dj_test_suite* _suite)
{
    bool                  setup_ok;

    // initialise test environment
    setup_ok = d_tests_text_lineup_setup();

    if (!setup_ok)
    {
        printf("Failed to set up text_lineup test environment\n");

        return false;
    }

    // create master group
    d_test_suite_block(_suite, "text_lineup Module Tests");

    // run all sections
    d_tests_text_lineup_string_array_all(_suite);
    d_tests_text_lineup_lineup_all(_suite);

    // cleanup
    d_tests_text_lineup_teardown();

    return true;
}
