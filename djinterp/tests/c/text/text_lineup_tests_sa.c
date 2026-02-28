#include "./text_lineup_tests_sa.h"


/******************************************************************************
 * TEST UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_setup
  Sets up the test environment for text_lineup tests.
Return:
  true (no setup actions currently required; present for extensibility).
*/
bool
d_tests_sa_text_lineup_setup
(
    void
)
{
    // no specific setup required for text_lineup tests;
    // function retained for consistency and future use.

    return true;
}

/*
d_tests_sa_text_lineup_teardown
  Cleans up the test environment.
Return:
  true (no teardown actions currently required).
*/
bool
d_tests_sa_text_lineup_teardown
(
    void
)
{
    // no specific teardown required;
    // retained for consistency and future use.

    return true;
}

/*
d_tests_sa_text_lineup_make_buffer
  Creates a d_text_buffer for use in tests, wrapping d_text_buffer_new.

Parameter(s):
  _capacity: initial byte capacity to request.
Return:
  A pointer to the newly allocated buffer, or NULL on allocation failure.
*/
struct d_text_buffer*
d_tests_sa_text_lineup_make_buffer
(
    size_t _capacity
)
{
    return d_text_buffer_new(_capacity);
}

/*
d_tests_sa_text_lineup_make_token_array
  Allocates a d_string_array and appends the given variable-argument
strings to it.

Parameter(s):
  _count: number of string arguments that follow.
  ...:    const char* strings to append.
Return:
  A pointer to the populated string array, or NULL on failure.
*/
struct d_string_array*
d_tests_sa_text_lineup_make_token_array
(
    size_t _count,
    ...
)
{
    struct d_string_array* array;
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
d_tests_sa_text_lineup_make_template
  Populates and returns a d_text_template value from individual fields.
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
  _flags:           bitmask of DBehaviorFlag values.
Return:
  A fully initialised d_text_template value.
*/
struct d_text_template
d_tests_sa_text_lineup_make_template
(
    struct d_string_array* _tokens,
    const char*            _separator,
    bool                   _sep_before,
    bool                   _sep_after,
    const char*            _line_prefix,
    const char*            _line_suffix,
    size_t                 _min_token_width,
    uint32_t               _flags
)
{
    struct d_text_template tmpl;

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
d_tests_sa_text_lineup_run_all
  Master test runner for all text_lineup module tests.
  Tests the following:
  - d_text_buffer operations (Section I)
  - d_string_array operations (Section II)
  - d_text_lineup formatting (Section III)
*/
struct d_test_object*
d_tests_sa_text_lineup_run_all
(
    void
)
{
    struct d_test_object* group;
    bool                  setup_ok;
    size_t                idx;

    // initialise test environment
    setup_ok = d_tests_sa_text_lineup_setup();

    if (!setup_ok)
    {
        printf("Failed to set up text_lineup test environment\n");

        return NULL;
    }

    // create master group
    group = d_test_object_new_interior("text_lineup Module Tests", 3);

    if (!group)
    {
        d_tests_sa_text_lineup_teardown();

        return NULL;
    }

    // run all sections
    idx = 0;
    group->elements[idx++] = d_tests_sa_text_lineup_text_buffer_all();
    group->elements[idx++] = d_tests_sa_text_lineup_string_array_all();
    group->elements[idx++] = d_tests_sa_text_lineup_lineup_all();

    // cleanup
    d_tests_sa_text_lineup_teardown();

    return group;
}
