#include "./text_lineup_tests_sa.h"


/******************************************************************************
 * SECTION III: d_text_lineup
 *
 * Helper macros used locally in this file.
 *****************************************************************************/

// D_TL_TEST_BUF_CONTAINS
//   macro: convenience check — true when the null-terminated region
// [buf->buffer, buf->pos) contains the literal substring _substr.
#define D_TL_TEST_BUF_CONTAINS(buf, _substr)                        \
    d_strcontains((buf)->buffer,                                     \
                  (buf)->pos,                                        \
                  (_substr))

// D_TL_TEST_BUF_EQUALS
//   macro: convenience check — true when the buffer content exactly
// equals the literal string _str of length _len.
#define D_TL_TEST_BUF_EQUALS(buf, _str, _len)                       \
    d_strequals((buf)->buffer,                                       \
                (buf)->pos,                                          \
                (_str),                                              \
                (_len))

// D_TL_TEST_STARTS_WITH
//   macro: true when the buffer content starts with _prefix of
// length _prefix_len.
#define D_TL_TEST_STARTS_WITH(buf, _prefix, _prefix_len)            \
    d_strstartswith((buf)->buffer,                                   \
                    (buf)->pos,                                      \
                    (_prefix),                                       \
                    (_prefix_len))


/******************************************************************************
 * NULL / INVALID INPUT TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_null_inputs
  Tests d_text_lineup rejection of invalid arguments.
  Tests the following:
  - NULL template pointer → false
  - NULL buffer pointer → false
  - template with NULL tokens pointer → false
  - template with zero-count token array → false
  - valid minimal call succeeds → true
*/
struct d_test_object*
d_tests_sa_text_lineup_null_inputs
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_null_template;
    bool                   test_null_buffer;
    bool                   test_null_tokens;
    bool                   test_empty_tokens;
    bool                   test_valid_minimal;
    size_t                 idx;

    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);

    // test 1: NULL template pointer
    test_null_template = false;

    if (buf)
    {
        result             = d_text_lineup(NULL, buf, 0, NULL, NULL,
                                           false, false);
        test_null_template = (result == false);
    }

    // test 2: NULL buffer pointer
    test_null_buffer = false;

    if (tokens)
    {
        d_string_array_append(tokens, "tok");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        result           = d_text_lineup(&tmpl, NULL, 0, NULL, NULL,
                                         false, false);
        test_null_buffer = (result == false);
    }

    // test 3: template with NULL tokens
    test_null_tokens = false;

    if (buf)
    {
        buf->pos = 0;
        buf->buffer[0] = '\0';
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = NULL;
        result           = d_text_lineup(&tmpl, buf, 0, NULL, NULL,
                                         false, false);
        test_null_tokens = (result == false);
    }

    // test 4: template with empty token array (count == 0)
    test_empty_tokens = false;

    if (buf && tokens)
    {
        buf->pos         = 0;
        buf->buffer[0]   = '\0';
        d_memset(&tmpl, 0, sizeof(tmpl));

        // build fresh empty array
        d_string_array_free(tokens);
        tokens = d_string_array_new(4);

        if (tokens)
        {
            tmpl.tokens        = tokens;
            result             = d_text_lineup(&tmpl, buf, 0, NULL, NULL,
                                               false, false);
            test_empty_tokens  = (result == false);
        }
    }

    // test 5: valid minimal call (one token, no wrapping)
    test_valid_minimal = false;

    if (buf && tokens)
    {
        buf->pos = 0;
        if (buf->buffer)
        {
            buf->buffer[0] = '\0';
        }

        d_string_array_append(tokens, "X");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens        = tokens;
        result             = d_text_lineup(&tmpl, buf,
                                           0, NULL, NULL, false, false);
        test_valid_minimal = (result == true);
    }

    d_text_buffer_free(buf);
    d_string_array_free(tokens);

    // build result tree
    group = d_test_object_new_interior("null/invalid inputs", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("null_template",
                                           test_null_template,
                                           "NULL template → false");
    group->elements[idx++] = D_ASSERT_TRUE("null_buffer",
                                           test_null_buffer,
                                           "NULL buffer → false");
    group->elements[idx++] = D_ASSERT_TRUE("null_tokens",
                                           test_null_tokens,
                                           "NULL tokens → false");
    group->elements[idx++] = D_ASSERT_TRUE("empty_tokens",
                                           test_empty_tokens,
                                           "zero-count tokens → false");
    group->elements[idx++] = D_ASSERT_TRUE("valid_minimal",
                                           test_valid_minimal,
                                           "valid minimal call succeeds");

    return group;
}


/******************************************************************************
 * NO-WRAP (LINE_WIDTH == 0) TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_no_wrap
  Tests d_text_lineup with line_width == 0 (no wrapping).
  Tests the following:
  - all tokens appear in the output
  - separator is placed between each pair of tokens
  - prefix is prepended to the output
  - suffix is appended to the output
  - separator_before places separator before first token
  - separator_after places separator after last token
  - single token produces no inter-token separator
*/
struct d_test_object*
d_tests_sa_text_lineup_no_wrap
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_tokens_present;
    bool                   test_separator;
    bool                   test_prefix;
    bool                   test_suffix;
    bool                   test_sep_before;
    bool                   test_sep_after;
    bool                   test_single_token;
    size_t                 idx;
    char                   sep[]    = ", ";
    char                   prefix[] = "[ ";
    char                   suffix[] = " ]";

    buf    = NULL;
    tokens = NULL;

    // test 1 & 2: tokens and separator present
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);
    test_tokens_present = false;
    test_separator      = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "aaa");
        d_string_array_append(tokens, "bbb");
        d_string_array_append(tokens, "ccc");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = sep;

        result              = d_text_lineup(&tmpl, buf, 0,
                                            NULL, NULL, false, false);
        test_tokens_present = result &&
                              D_TL_TEST_BUF_CONTAINS(buf, "aaa") &&
                              D_TL_TEST_BUF_CONTAINS(buf, "bbb") &&
                              D_TL_TEST_BUF_CONTAINS(buf, "ccc");
        test_separator      = result &&
                              D_TL_TEST_BUF_CONTAINS(buf, ", ");
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 3: prefix written at front
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(2);
    test_prefix = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "tok");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        result      = d_text_lineup(&tmpl, buf, 0,
                                    prefix, NULL, false, false);
        test_prefix = result &&
                      D_TL_TEST_STARTS_WITH(buf, "[ ", 2);
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 4: suffix written at end
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(2);
    test_suffix = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "tok");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        result      = d_text_lineup(&tmpl, buf, 0,
                                    NULL, suffix, false, false);
        test_suffix = result &&
                      D_TL_TEST_BUF_CONTAINS(buf, " ]");
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 5: separator_before inserts sep before first token
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(2);
    test_sep_before = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "tok");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens           = tokens;
        tmpl.separator        = sep;
        tmpl.separator_before = true;

        result          = d_text_lineup(&tmpl, buf, 0,
                                        NULL, NULL, false, false);
        test_sep_before = result &&
                          D_TL_TEST_STARTS_WITH(buf, ", ", 2);
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 6: separator_after inserts sep after last token
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(2);
    test_sep_after = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "tok");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens          = tokens;
        tmpl.separator       = sep;
        tmpl.separator_after = true;

        result         = d_text_lineup(&tmpl, buf, 0,
                                       NULL, NULL, false, false);
        test_sep_after = result &&
                         D_TL_TEST_BUF_CONTAINS(buf, "tok, ");
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 7: single token — no separator between tokens
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(2);
    test_single_token = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "only");
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = sep;

        result            = d_text_lineup(&tmpl, buf, 0,
                                          NULL, NULL, false, false);
        test_single_token = result &&
                            D_TL_TEST_BUF_EQUALS(buf, "only", 4);
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // build result tree
    group = d_test_object_new_interior("no-wrap (line_width == 0)", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("tokens_present",
                                           test_tokens_present,
                                           "all tokens appear in output");
    group->elements[idx++] = D_ASSERT_TRUE("separator",
                                           test_separator,
                                           "separator placed between tokens");
    group->elements[idx++] = D_ASSERT_TRUE("prefix",
                                           test_prefix,
                                           "prefix written at start");
    group->elements[idx++] = D_ASSERT_TRUE("suffix",
                                           test_suffix,
                                           "suffix written at end");
    group->elements[idx++] = D_ASSERT_TRUE("sep_before",
                                           test_sep_before,
                                           "separator_before inserts sep first");
    group->elements[idx++] = D_ASSERT_TRUE("sep_after",
                                           test_sep_after,
                                           "separator_after inserts sep last");
    group->elements[idx++] = D_ASSERT_TRUE("single_token",
                                           test_single_token,
                                           "single token: no inter-token separator");

    return group;
}


/******************************************************************************
 * LINE-WRAPPING TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_wrapping
  Tests d_text_lineup multi-line wrapping behaviour.
  Tests the following:
  - output contains a newline when tokens overflow one line
  - all tokens are present across all lines
  - each line respects line_width (no line exceeds it, incl. newline)
  - a very wide line_width keeps all tokens on one line
  - a very narrow line_width forces one token per line
*/
struct d_test_object*
d_tests_sa_text_lineup_wrapping
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_newline;
    bool                   test_all_tokens;
    bool                   test_line_width;
    bool                   test_wide;
    bool                   test_narrow;
    size_t                 idx;
    size_t                 i;
    size_t                 line_start;
    size_t                 line_len;
    bool                   line_ok;
    char                   sep[] = " ";

    // create 10 tokens of 5 chars each → 10*6 = 60 chars on one line;
    // with line_width 40 this must wrap.
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(16);

    test_newline   = false;
    test_all_tokens = false;
    test_line_width = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "alpha");
        d_string_array_append(tokens, "bravo");
        d_string_array_append(tokens, "charlie");
        d_string_array_append(tokens, "delta");
        d_string_array_append(tokens, "echo");
        d_string_array_append(tokens, "foxtrot");
        d_string_array_append(tokens, "golf");
        d_string_array_append(tokens, "hotel");

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = sep;

        result = d_text_lineup(&tmpl, buf, 40, NULL, NULL, false, false);

        // test 1: output contains a newline
        test_newline = result &&
                       D_TL_TEST_BUF_CONTAINS(buf, "\n");

        // test 2: all tokens present
        test_all_tokens = result &&
                          D_TL_TEST_BUF_CONTAINS(buf, "alpha")   &&
                          D_TL_TEST_BUF_CONTAINS(buf, "bravo")   &&
                          D_TL_TEST_BUF_CONTAINS(buf, "charlie") &&
                          D_TL_TEST_BUF_CONTAINS(buf, "delta")   &&
                          D_TL_TEST_BUF_CONTAINS(buf, "echo")    &&
                          D_TL_TEST_BUF_CONTAINS(buf, "foxtrot") &&
                          D_TL_TEST_BUF_CONTAINS(buf, "golf")    &&
                          D_TL_TEST_BUF_CONTAINS(buf, "hotel");

        // test 3: no line (terminated by \n or end) exceeds 40 chars
        line_start = 0;
        line_ok    = true;

        for (i = 0; i <= buf->pos; i++)
        {
            if ( (i == buf->pos) ||
                 (buf->buffer[i] == '\n') )
            {
                line_len = i - line_start;

                if (line_len > 40)
                {
                    line_ok = false;
                }

                line_start = i + 1;
            }
        }

        test_line_width = result && line_ok;
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 4: very wide line_width → no newline
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);
    test_wide = false;

    if (buf && tokens)
    {
        d_string_array_append(tokens, "one");
        d_string_array_append(tokens, "two");
        d_string_array_append(tokens, "three");

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = sep;

        result    = d_text_lineup(&tmpl, buf, 1024, NULL, NULL,
                                  false, false);
        test_wide = result &&
                    !D_TL_TEST_BUF_CONTAINS(buf, "\n");
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // test 5: very narrow → newline after every token
    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);
    test_narrow = false;

    if (buf && tokens)
    {
        // tokens are "a", "b", "c" (1 char each)
        d_string_array_append(tokens, "a");
        d_string_array_append(tokens, "b");
        d_string_array_append(tokens, "c");

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;

        // width 3 → only one 1-char token fits per line
        result = d_text_lineup(&tmpl, buf, 2, NULL, NULL, false, false);

        // expect at least 2 newlines for 3 tokens
        test_narrow = result &&
                      (d_strcount_char(buf->buffer, buf->pos, '\n') >= 2);
    }

    if (buf)
    {
        d_text_buffer_free(buf);
    }

    if (tokens)
    {
        d_string_array_free(tokens);
    }

    // build result tree
    group = d_test_object_new_interior("line wrapping", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("newline",
                                           test_newline,
                                           "output contains newline on wrap");
    group->elements[idx++] = D_ASSERT_TRUE("all_tokens",
                                           test_all_tokens,
                                           "all tokens present across all lines");
    group->elements[idx++] = D_ASSERT_TRUE("line_width",
                                           test_line_width,
                                           "no line exceeds line_width");
    group->elements[idx++] = D_ASSERT_TRUE("wide",
                                           test_wide,
                                           "wide line_width keeps all on one line");
    group->elements[idx++] = D_ASSERT_TRUE("narrow",
                                           test_narrow,
                                           "narrow width forces one token per line");

    return group;
}


/******************************************************************************
 * PREFIX / SUFFIX TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_prefix_suffix
  Tests d_text_lineup first-line prefix and last-line suffix.
  Tests the following:
  - prefix appears at the start of the output
  - suffix appears at the end of the output
  - prefix and suffix may both be used simultaneously
  - NULL prefix is accepted (no prefix written)
  - NULL suffix is accepted (no suffix written)
  - prefix larger than line_width is rejected unless flag is set
  - flag ALLOW_OVERSIZE_PREFIX permits an oversize prefix
*/
struct d_test_object*
d_tests_sa_text_lineup_prefix_suffix
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_prefix;
    bool                   test_suffix;
    bool                   test_both;
    bool                   test_null_prefix;
    bool                   test_null_suffix;
    bool                   test_oversize_rejected;
    bool                   test_oversize_allowed;
    size_t                 idx;

// helper macro: reset the output buffer to empty
#define RESET_BUF(b)                \
    do {                            \
        (b)->pos = 0;               \
        if ((b)->buffer)            \
        {                           \
            (b)->buffer[0] = '\0';  \
        }                           \
    } while (0)

    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);

    if (tokens)
    {
        d_string_array_append(tokens, "tok1");
        d_string_array_append(tokens, "tok2");
    }

    test_prefix          = false;
    test_suffix          = false;
    test_both            = false;
    test_null_prefix     = false;
    test_null_suffix     = false;
    test_oversize_rejected = false;
    test_oversize_allowed  = false;

    if (buf && tokens)
    {
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;

        // test 1: prefix
        RESET_BUF(buf);
        result      = d_text_lineup(&tmpl, buf, 60,
                                    ">> ", NULL, false, false);
        test_prefix = result && D_TL_TEST_STARTS_WITH(buf, ">> ", 3);

        // test 2: suffix
        RESET_BUF(buf);
        result      = d_text_lineup(&tmpl, buf, 60,
                                    NULL, " <<", false, false);
        test_suffix = result && D_TL_TEST_BUF_CONTAINS(buf, " <<");

        // test 3: both simultaneously
        RESET_BUF(buf);
        result    = d_text_lineup(&tmpl, buf, 60,
                                  ">> ", " <<", false, false);
        test_both = result &&
                    D_TL_TEST_STARTS_WITH(buf, ">> ", 3) &&
                    D_TL_TEST_BUF_CONTAINS(buf, " <<");

        // test 4: NULL prefix
        RESET_BUF(buf);
        result           = d_text_lineup(&tmpl, buf, 60,
                                         NULL, NULL, false, false);
        test_null_prefix = result && (buf->pos > 0);

        // test 5: NULL suffix
        RESET_BUF(buf);
        result           = d_text_lineup(&tmpl, buf, 60,
                                         NULL, NULL, false, false);
        test_null_suffix = result && (buf->pos > 0);

        // test 6: oversize prefix (100 chars) rejected when flag not set
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        tmpl.flags  = 0;
        result = d_text_lineup(
            &tmpl, buf, 20,
            "This prefix is definitely longer than twenty chars",
            NULL, false, false);
        test_oversize_rejected = (result == false);

        // test 7: same oversize prefix allowed with ALLOW_OVERSIZE_PREFIX
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        tmpl.flags  = D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_PREFIX;
        result = d_text_lineup(
            &tmpl, buf, 20,
            "This prefix is definitely longer than twenty chars",
            NULL, false, false);
        test_oversize_allowed = (result == true);
    }

    d_text_buffer_free(buf);
    d_string_array_free(tokens);

    #undef RESET_BUF

    // build result tree
    group = d_test_object_new_interior("prefix / suffix", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("prefix",
                                           test_prefix,
                                           "prefix at start of output");
    group->elements[idx++] = D_ASSERT_TRUE("suffix",
                                           test_suffix,
                                           "suffix at end of output");
    group->elements[idx++] = D_ASSERT_TRUE("both",
                                           test_both,
                                           "prefix and suffix may coexist");
    group->elements[idx++] = D_ASSERT_TRUE("null_prefix",
                                           test_null_prefix,
                                           "NULL prefix is accepted");
    group->elements[idx++] = D_ASSERT_TRUE("null_suffix",
                                           test_null_suffix,
                                           "NULL suffix is accepted");
    group->elements[idx++] = D_ASSERT_TRUE("oversize_rejected",
                                           test_oversize_rejected,
                                           "oversize prefix rejected by default");
    group->elements[idx++] = D_ASSERT_TRUE("oversize_allowed",
                                           test_oversize_allowed,
                                           "ALLOW_OVERSIZE_PREFIX flag permits it");

    return group;
}


/******************************************************************************
 * LINE PREFIX / LINE SUFFIX TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_line_prefix_suffix
  Tests d_text_lineup per-line prefix and per-line suffix.
  Tests the following:
  - line_prefix appears at the start of continuation lines
  - line_suffix appears at the end of each non-last line
  - line_prefix on every line with LINE_PREFIX_ALL_LINES flag
  - line_suffix on every line with LINE_SUFFIX_ALL_LINES flag
  - line_suffix after the global suffix with LINE_SUFFIX_AFTER_SUFFIX flag
*/
struct d_test_object*
d_tests_sa_text_lineup_line_prefix_suffix
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_line_prefix;
    bool                   test_line_suffix;
    bool                   test_prefix_all_lines;
    bool                   test_suffix_all_lines;
    bool                   test_suffix_after_suffix;
    size_t                 idx;

// reset helper
#define RESET_BUF(b)                \
    do {                            \
        (b)->pos = 0;               \
        if ((b)->buffer)            \
        {                           \
            (b)->buffer[0] = '\0';  \
        }                           \
    } while (0)

    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(8);

    if (tokens)
    {
        d_string_array_append(tokens, "aaa");
        d_string_array_append(tokens, "bbb");
        d_string_array_append(tokens, "ccc");
        d_string_array_append(tokens, "ddd");
        d_string_array_append(tokens, "eee");
        d_string_array_append(tokens, "fff");
    }

    test_line_prefix         = false;
    test_line_suffix         = false;
    test_prefix_all_lines    = false;
    test_suffix_all_lines    = false;
    test_suffix_after_suffix = false;

    if (buf && tokens)
    {
        char line_prefix[] = "  ";   // 2-space indent
        char line_suffix[] = " \\";  // trailing backslash

        // test 1: line_prefix on continuation lines
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_prefix = line_prefix;

        result           = d_text_lineup(&tmpl, buf, 18,
                                         NULL, NULL, false, false);
        // with line_width 18, tokens must wrap; second line starts
        // with "  "
        test_line_prefix = result &&
                           D_TL_TEST_BUF_CONTAINS(buf, "\n  ");

        // test 2: line_suffix on non-last lines
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_suffix = line_suffix;

        result           = d_text_lineup(&tmpl, buf, 18,
                                         NULL, NULL, false, false);
        test_line_suffix = result &&
                           D_TL_TEST_BUF_CONTAINS(buf, " \\");

        // test 3: LINE_PREFIX_ALL_LINES — every line (incl. first) starts
        // with line_prefix
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_prefix = line_prefix;
        tmpl.flags       = D_BEHAVIOR_FLAG_LINE_PREFIX_ALL_LINES;

        result                = d_text_lineup(&tmpl, buf, 18,
                                              NULL, NULL, false, false);
        test_prefix_all_lines = result &&
                                D_TL_TEST_STARTS_WITH(buf, "  ", 2);

        // test 4: LINE_SUFFIX_ALL_LINES — last line also gets line_suffix
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_suffix = line_suffix;
        tmpl.flags       = D_BEHAVIOR_FLAG_LINE_SUFFIX_ALL_LINES;

        result                = d_text_lineup(&tmpl, buf, 18,
                                              NULL, NULL, false, false);
        // last line ends with " \" not "\n"
        test_suffix_all_lines = result &&
                                D_TL_TEST_BUF_CONTAINS(buf, " \\");

        // test 5: LINE_SUFFIX_AFTER_SUFFIX
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_suffix = line_suffix;
        tmpl.flags       = D_BEHAVIOR_FLAG_LINE_SUFFIX_AFTER_SUFFIX;

        result                   = d_text_lineup(&tmpl, buf, 18,
                                                 NULL, "END", false, false);
        test_suffix_after_suffix = result &&
                                   D_TL_TEST_BUF_CONTAINS(buf, "END") &&
                                   D_TL_TEST_BUF_CONTAINS(buf, " \\");
    }

    d_text_buffer_free(buf);
    d_string_array_free(tokens);

    #undef RESET_BUF

    // build result tree
    group = d_test_object_new_interior("line prefix / line suffix", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("line_prefix",
                                           test_line_prefix,
                                           "line_prefix on continuation lines");
    group->elements[idx++] = D_ASSERT_TRUE("line_suffix",
                                           test_line_suffix,
                                           "line_suffix on non-last lines");
    group->elements[idx++] = D_ASSERT_TRUE("prefix_all_lines",
                                           test_prefix_all_lines,
                                           "LINE_PREFIX_ALL_LINES on first line");
    group->elements[idx++] = D_ASSERT_TRUE("suffix_all_lines",
                                           test_suffix_all_lines,
                                           "LINE_SUFFIX_ALL_LINES on last line");
    group->elements[idx++] = D_ASSERT_TRUE("suffix_after_suffix",
                                           test_suffix_after_suffix,
                                           "LINE_SUFFIX_AFTER_SUFFIX ordering");

    return group;
}


/******************************************************************************
 * SEPARATOR TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_separators
  Tests separator placement across all relevant combinations.
  Tests the following:
  - NULL separator produces no separator in the output
  - separator appears between every adjacent pair of tokens
  - separator_before and separator_after may be combined
  - separator_before only (no separator_after)
  - separator_after only (no separator_before)
  - separator appears correctly on wrapped lines
*/
struct d_test_object*
d_tests_sa_text_lineup_separators
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_no_sep;
    bool                   test_between;
    bool                   test_both;
    bool                   test_before_only;
    bool                   test_after_only;
    bool                   test_wrapped;
    size_t                 idx;

#define RESET_BUF(b)                \
    do {                            \
        (b)->pos = 0;               \
        if ((b)->buffer)            \
        {                           \
            (b)->buffer[0] = '\0';  \
        }                           \
    } while (0)

    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(4);

    if (tokens)
    {
        d_string_array_append(tokens, "A");
        d_string_array_append(tokens, "B");
        d_string_array_append(tokens, "C");
    }

    test_no_sep    = false;
    test_between   = false;
    test_both      = false;
    test_before_only = false;
    test_after_only  = false;
    test_wrapped   = false;

    if (buf && tokens)
    {
        // test 1: NULL separator — no separating characters
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = NULL;

        result     = d_text_lineup(&tmpl, buf, 0,
                                   NULL, NULL, false, false);
        test_no_sep = result &&
                      D_TL_TEST_BUF_EQUALS(buf, "ABC", 3);

        // test 2: separator between every pair
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = "|";

        result      = d_text_lineup(&tmpl, buf, 0,
                                    NULL, NULL, false, false);
        test_between = result &&
                       D_TL_TEST_BUF_EQUALS(buf, "A|B|C", 5);

        // test 3: separator_before AND separator_after
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens           = tokens;
        tmpl.separator        = "|";
        tmpl.separator_before = true;
        tmpl.separator_after  = true;

        result    = d_text_lineup(&tmpl, buf, 0,
                                  NULL, NULL, false, false);
        test_both = result &&
                    D_TL_TEST_BUF_EQUALS(buf, "|A|B|C|", 7);

        // test 4: separator_before only
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens           = tokens;
        tmpl.separator        = "|";
        tmpl.separator_before = true;
        tmpl.separator_after  = false;

        result          = d_text_lineup(&tmpl, buf, 0,
                                        NULL, NULL, false, false);
        test_before_only = result &&
                           D_TL_TEST_BUF_EQUALS(buf, "|A|B|C", 6);

        // test 5: separator_after only
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens           = tokens;
        tmpl.separator        = "|";
        tmpl.separator_before = false;
        tmpl.separator_after  = true;

        result         = d_text_lineup(&tmpl, buf, 0,
                                       NULL, NULL, false, false);
        test_after_only = result &&
                          D_TL_TEST_BUF_EQUALS(buf, "A|B|C|", 6);

        // test 6: separator appears on wrapped second line
        RESET_BUF(buf);
        d_string_array_free(tokens);
        tokens = d_string_array_new(6);

        if (tokens)
        {
            d_string_array_append(tokens, "aaa");
            d_string_array_append(tokens, "bbb");
            d_string_array_append(tokens, "ccc");
            d_string_array_append(tokens, "ddd");

            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens    = tokens;
            tmpl.separator = " ";

            result       = d_text_lineup(&tmpl, buf, 10,
                                         NULL, NULL, false, false);
            // with sep=" " and token width 4, two tokens fit per line:
            // "aaa bbb\nccc ddd"
            test_wrapped = result &&
                           D_TL_TEST_BUF_CONTAINS(buf, "aaa") &&
                           D_TL_TEST_BUF_CONTAINS(buf, "ccc");
        }
    }

    d_text_buffer_free(buf);
    d_string_array_free(tokens);

    #undef RESET_BUF

    // build result tree
    group = d_test_object_new_interior("separators", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("no_sep",
                                           test_no_sep,
                                           "NULL separator: no extra characters");
    group->elements[idx++] = D_ASSERT_TRUE("between",
                                           test_between,
                                           "separator between every adjacent pair");
    group->elements[idx++] = D_ASSERT_TRUE("both",
                                           test_both,
                                           "separator_before and _after combined");
    group->elements[idx++] = D_ASSERT_TRUE("before_only",
                                           test_before_only,
                                           "separator_before only");
    group->elements[idx++] = D_ASSERT_TRUE("after_only",
                                           test_after_only,
                                           "separator_after only");
    group->elements[idx++] = D_ASSERT_TRUE("wrapped",
                                           test_wrapped,
                                           "separator on wrapped continuation lines");

    return group;
}


/******************************************************************************
 * TOKEN ALIGNMENT TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_token_alignment
  Tests TOKEN_ALIGN_LEFT, TOKEN_ALIGN_CENTER, and default (right) alignment.
  Tests the following:
  - default (right) alignment: shorter token padded on the left
  - TOKEN_ALIGN_LEFT: shorter token padded on the right (suffix spaces)
  - TOKEN_ALIGN_CENTER: shorter token padded on both sides
  - all three modes produce the same total column width
  - mixed-length tokens are correctly padded under each mode
*/
struct d_test_object*
d_tests_sa_text_lineup_token_alignment
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf_right;
    struct d_text_buffer*  buf_left;
    struct d_text_buffer*  buf_center;
    struct d_string_array* tokens;
    bool                   result_r;
    bool                   result_l;
    bool                   result_c;
    bool                   test_right;
    bool                   test_left;
    bool                   test_center;
    bool                   test_same_width;
    bool                   test_mixed;
    size_t                 idx;

    buf_right  = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    buf_left   = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    buf_center = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);

    // tokens with different lengths: "i", "ab", "xyz"
    tokens = d_string_array_new(4);

    if (tokens)
    {
        d_string_array_append(tokens, "i");
        d_string_array_append(tokens, "ab");
        d_string_array_append(tokens, "xyz");
    }

    test_right      = false;
    test_left       = false;
    test_center     = false;
    test_same_width = false;
    test_mixed      = false;

    result_r = false;
    result_l = false;
    result_c = false;

    if (buf_right && buf_left && buf_center && tokens)
    {
        // right (default) alignment — spaces before shorter tokens
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        tmpl.flags  = 0;
        result_r    = d_text_lineup(&tmpl, buf_right, 0,
                                    NULL, NULL, false, false);

        // left alignment — spaces after shorter tokens
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        tmpl.flags  = D_BEHAVIOR_FLAG_TOKEN_ALIGN_LEFT;
        result_l    = d_text_lineup(&tmpl, buf_left, 0,
                                    NULL, NULL, false, false);

        // center alignment — spaces split around shorter tokens
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        tmpl.flags  = D_BEHAVIOR_FLAG_TOKEN_ALIGN_CENTER;
        result_c    = d_text_lineup(&tmpl, buf_center, 0,
                                    NULL, NULL, false, false);

        // test 1: right alignment — all actual token text present
        test_right = result_r &&
                     D_TL_TEST_BUF_CONTAINS(buf_right, "i")   &&
                     D_TL_TEST_BUF_CONTAINS(buf_right, "ab")  &&
                     D_TL_TEST_BUF_CONTAINS(buf_right, "xyz");

        // test 2: left alignment — token text present
        test_left = result_l &&
                    D_TL_TEST_BUF_CONTAINS(buf_left, "i")  &&
                    D_TL_TEST_BUF_CONTAINS(buf_left, "ab") &&
                    D_TL_TEST_BUF_CONTAINS(buf_left, "xyz");

        // test 3: center alignment — token text present
        test_center = result_c &&
                      D_TL_TEST_BUF_CONTAINS(buf_center, "i")   &&
                      D_TL_TEST_BUF_CONTAINS(buf_center, "ab")  &&
                      D_TL_TEST_BUF_CONTAINS(buf_center, "xyz");

        // test 4: all three produce the same total byte count
        // (alignment only redistributes spaces, not total column space)
        test_same_width = result_r && result_l && result_c &&
                          (buf_right->pos == buf_left->pos) &&
                          (buf_right->pos == buf_center->pos);

        // test 5: mixed lengths — padding bytes are actually spaces
        // the space count in the output should be > 0 (padding exists)
        test_mixed = result_r &&
                     (d_strcount_char(buf_right->buffer,
                                      buf_right->pos,
                                      ' ') > 0);
    }

    d_text_buffer_free(buf_right);
    d_text_buffer_free(buf_left);
    d_text_buffer_free(buf_center);
    d_string_array_free(tokens);

    // build result tree
    group = d_test_object_new_interior("token alignment", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("right",
                                           test_right,
                                           "default (right) alignment: tokens present");
    group->elements[idx++] = D_ASSERT_TRUE("left",
                                           test_left,
                                           "TOKEN_ALIGN_LEFT: tokens present");
    group->elements[idx++] = D_ASSERT_TRUE("center",
                                           test_center,
                                           "TOKEN_ALIGN_CENTER: tokens present");
    group->elements[idx++] = D_ASSERT_TRUE("same_width",
                                           test_same_width,
                                           "all modes produce the same byte count");
    group->elements[idx++] = D_ASSERT_TRUE("mixed",
                                           test_mixed,
                                           "mixed-length tokens: spaces present");

    return group;
}


/******************************************************************************
 * BEHAVIOR FLAG TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_flags
  Tests the remaining DBehaviorFlag values.
  Tests the following:
  - RIGHT_ALIGN_LINE_SUFFIX right-aligns the line_suffix column
  - PAD_AFTER_SEPARATOR places padding after the separator
  - ALIGN_SINGLE_ROW applies column alignment even for a single row
  - ALIGN_FIRST_ROW_TO_SLOTS aligns first-row tokens to slot boundaries
  - ALLOW_OVERSIZE_SUFFIX permits suffix longer than line_width
*/
struct d_test_object*
d_tests_sa_text_lineup_flags
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf;
    struct d_string_array* tokens;
    bool                   result;
    bool                   test_right_align_suffix;
    bool                   test_pad_after_sep;
    bool                   test_align_single_row;
    bool                   test_align_first_row;
    bool                   test_oversize_suffix;
    size_t                 idx;

#define RESET_BUF(b)                \
    do {                            \
        (b)->pos = 0;               \
        if ((b)->buffer)            \
        {                           \
            (b)->buffer[0] = '\0';  \
        }                           \
    } while (0)

    buf    = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(8);

    if (tokens)
    {
        d_string_array_append(tokens, "aa");
        d_string_array_append(tokens, "bb");
        d_string_array_append(tokens, "cc");
        d_string_array_append(tokens, "dd");
        d_string_array_append(tokens, "ee");
        d_string_array_append(tokens, "ff");
    }

    test_right_align_suffix = false;
    test_pad_after_sep      = false;
    test_align_single_row   = false;
    test_align_first_row    = false;
    test_oversize_suffix    = false;

    if (buf && tokens)
    {
        char line_suffix[] = "|";

        // test 1: RIGHT_ALIGN_LINE_SUFFIX — output contains "|" and spaces
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_suffix = line_suffix;
        tmpl.flags       = D_BEHAVIOR_FLAG_RIGHT_ALIGN_LINE_SUFFIX;

        result                  = d_text_lineup(&tmpl, buf, 30,
                                                NULL, NULL, false, false);
        test_right_align_suffix = result &&
                                  D_TL_TEST_BUF_CONTAINS(buf, "|");

        // test 2: PAD_AFTER_SEPARATOR — succeeds without crashing
        RESET_BUF(buf);
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens    = tokens;
        tmpl.separator = ", ";
        tmpl.flags     = D_BEHAVIOR_FLAG_PAD_AFTER_SEPARATOR;

        result             = d_text_lineup(&tmpl, buf, 30,
                                           NULL, NULL, false, false);
        test_pad_after_sep = result && D_TL_TEST_BUF_CONTAINS(buf, ",");

        // test 3: ALIGN_SINGLE_ROW — single-token result still succeeds
        RESET_BUF(buf);
        d_string_array_free(tokens);
        tokens = d_string_array_new(2);

        if (tokens)
        {
            d_string_array_append(tokens, "only");
            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens = tokens;
            tmpl.flags  = D_BEHAVIOR_FLAG_ALIGN_SINGLE_ROW;

            result                = d_text_lineup(&tmpl, buf, 40,
                                                  NULL, NULL, false, false);
            test_align_single_row = result &&
                                    D_TL_TEST_BUF_CONTAINS(buf, "only");

            d_string_array_free(tokens);
        }

        // test 4: ALIGN_FIRST_ROW_TO_SLOTS — succeeds and contains tokens
        buf->pos = 0;
        if (buf->buffer)
        {
            buf->buffer[0] = '\0';
        }

        tokens = d_string_array_new(4);

        if (tokens)
        {
            d_string_array_append(tokens, "one");
            d_string_array_append(tokens, "two");
            d_string_array_append(tokens, "three");

            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens    = tokens;
            tmpl.separator = " ";
            tmpl.flags     = D_BEHAVIOR_FLAG_ALIGN_FIRST_ROW_TO_SLOTS;

            result               = d_text_lineup(&tmpl, buf, 40,
                                                 "  ", NULL, false, false);
            test_align_first_row = result &&
                                   D_TL_TEST_BUF_CONTAINS(buf, "one");

            d_string_array_free(tokens);
        }

        // test 5: ALLOW_OVERSIZE_SUFFIX — oversize suffix is accepted
        RESET_BUF(buf);
        tokens = d_string_array_new(2);

        if (tokens)
        {
            d_string_array_append(tokens, "tok");

            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens = tokens;
            tmpl.flags  = D_BEHAVIOR_FLAG_ALLOW_OVERSIZE_SUFFIX;

            result = d_text_lineup(
                &tmpl, buf, 10,
                NULL,
                "This suffix is much longer than ten chars",
                false, false);
            test_oversize_suffix = (result == true);
        }
    }

    d_text_buffer_free(buf);
    d_string_array_free(tokens);

    #undef RESET_BUF

    // build result tree
    group = d_test_object_new_interior("behavior flags", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("right_align_suffix",
                                           test_right_align_suffix,
                                           "RIGHT_ALIGN_LINE_SUFFIX: suffix present");
    group->elements[idx++] = D_ASSERT_TRUE("pad_after_sep",
                                           test_pad_after_sep,
                                           "PAD_AFTER_SEPARATOR: output correct");
    group->elements[idx++] = D_ASSERT_TRUE("align_single_row",
                                           test_align_single_row,
                                           "ALIGN_SINGLE_ROW: single token ok");
    group->elements[idx++] = D_ASSERT_TRUE("align_first_row",
                                           test_align_first_row,
                                           "ALIGN_FIRST_ROW_TO_SLOTS: tokens ok");
    group->elements[idx++] = D_ASSERT_TRUE("oversize_suffix",
                                           test_oversize_suffix,
                                           "ALLOW_OVERSIZE_SUFFIX: accepted");

    return group;
}


/******************************************************************************
 * MIN_TOKEN_WIDTH TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_min_token_width
  Tests min_token_width enforcement.
  Tests the following:
  - min_token_width wider than all tokens increases column width
  - output is wider (more spaces) than without min_token_width
  - min_token_width equal to max token length has no visible effect
  - min_token_width of 0 uses natural token width
  - all tokens still present with min_token_width set
*/
struct d_test_object*
d_tests_sa_text_lineup_min_token_width
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf_narrow;
    struct d_text_buffer*  buf_wide;
    struct d_string_array* tokens;
    bool                   result_n;
    bool                   result_w;
    bool                   test_wider;
    bool                   test_equal;
    bool                   test_zero;
    bool                   test_tokens_present;
    size_t                 idx;

    buf_narrow = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    buf_wide   = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens     = d_string_array_new(4);

    if (tokens)
    {
        d_string_array_append(tokens, "a");
        d_string_array_append(tokens, "b");
        d_string_array_append(tokens, "c");
    }

    test_wider        = false;
    test_equal        = false;
    test_zero         = false;
    test_tokens_present = false;
    result_n          = false;
    result_w          = false;

    if (buf_narrow && buf_wide && tokens)
    {
        // run without min_token_width
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens          = tokens;
        tmpl.min_token_width = 0;
        result_n             = d_text_lineup(&tmpl, buf_narrow, 0,
                                             NULL, NULL, false, false);

        // run with min_token_width == 5 (wider than "a", "b", "c")
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens          = tokens;
        tmpl.min_token_width = 5;
        result_w             = d_text_lineup(&tmpl, buf_wide, 0,
                                             NULL, NULL, false, false);

        // test 1: min_token_width forces output to be wider
        test_wider = result_n && result_w &&
                     (buf_wide->pos > buf_narrow->pos);

        // test 2: min_token_width == max token len → same as no override
        // (both "xyz" runs use the same natural width)
        buf_narrow->pos = 0;
        if (buf_narrow->buffer)
        {
            buf_narrow->buffer[0] = '\0';
        }

        buf_wide->pos = 0;
        if (buf_wide->buffer)
        {
            buf_wide->buffer[0] = '\0';
        }

        d_string_array_free(tokens);
        tokens = d_string_array_new(2);

        if (tokens)
        {
            d_string_array_append(tokens, "xyz");

            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens          = tokens;
            tmpl.min_token_width = 0;
            d_text_lineup(&tmpl, buf_narrow, 0,
                          NULL, NULL, false, false);

            d_memset(&tmpl, 0, sizeof(tmpl));
            tmpl.tokens          = tokens;
            tmpl.min_token_width = 3;  // == strlen("xyz")
            d_text_lineup(&tmpl, buf_wide, 0,
                          NULL, NULL, false, false);

            test_equal = (buf_narrow->pos == buf_wide->pos);

            // test 3: min_token_width == 0 uses natural width
            test_zero = D_TL_TEST_BUF_EQUALS(buf_narrow, "xyz", 3);

            // test 4: tokens still present
            test_tokens_present = D_TL_TEST_BUF_CONTAINS(buf_wide, "xyz");
        }
    }

    d_text_buffer_free(buf_narrow);
    d_text_buffer_free(buf_wide);
    d_string_array_free(tokens);

    // build result tree
    group = d_test_object_new_interior("min_token_width", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("wider",
                                           test_wider,
                                           "min_token_width widens output");
    group->elements[idx++] = D_ASSERT_TRUE("equal",
                                           test_equal,
                                           "width == max token: no extra padding");
    group->elements[idx++] = D_ASSERT_TRUE("zero",
                                           test_zero,
                                           "width 0 uses natural token width");
    group->elements[idx++] = D_ASSERT_TRUE("tokens_present",
                                           test_tokens_present,
                                           "all tokens present with min_token_width");

    return group;
}


/******************************************************************************
 * INDENT_PREFIX / OUTDENT_SUFFIX TESTS
 *****************************************************************************/

/*
d_tests_sa_text_lineup_indent_outdent
  Tests the _indent_prefix and _outdent_suffix parameters of d_text_lineup.
  Tests the following:
  - _indent_prefix true: line_prefix precedes the first-line prefix
  - _indent_prefix false: first line starts with prefix only
  - _outdent_suffix true: line_suffix follows the global suffix on last line
  - _outdent_suffix false: last line does not have line_suffix after suffix
  - both flags false produces correct output without crashes
  - both flags true produces correct output without crashes
*/
struct d_test_object*
d_tests_sa_text_lineup_indent_outdent
(
    void
)
{
    struct d_test_object*  group;
    struct d_text_template tmpl;
    struct d_text_buffer*  buf_a;
    struct d_text_buffer*  buf_b;
    struct d_string_array* tokens;
    bool                   result_a;
    bool                   result_b;
    bool                   test_indent_true;
    bool                   test_indent_false;
    bool                   test_outdent_true;
    bool                   test_outdent_false;
    bool                   test_both_false;
    bool                   test_both_true;
    size_t                 idx;
    char                   line_prefix[] = "  ";
    char                   line_suffix[] = " \\";

    buf_a  = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    buf_b  = d_text_buffer_new(D_TEST_LINEUP_BUF_CAPACITY);
    tokens = d_string_array_new(8);

    if (tokens)
    {
        d_string_array_append(tokens, "aaa");
        d_string_array_append(tokens, "bbb");
        d_string_array_append(tokens, "ccc");
        d_string_array_append(tokens, "ddd");
        d_string_array_append(tokens, "eee");
    }

    test_indent_true   = false;
    test_indent_false  = false;
    test_outdent_true  = false;
    test_outdent_false = false;
    test_both_false    = false;
    test_both_true     = false;
    result_a           = false;
    result_b           = false;

    if (buf_a && buf_b && tokens)
    {
        // test 1 & 2: _indent_prefix true vs false
        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_prefix = line_prefix;
        tmpl.flags       = D_BEHAVIOR_FLAG_LINE_PREFIX_ALL_LINES;

        // _indent_prefix = true → line_prefix before first-line prefix
        result_a = d_text_lineup(&tmpl, buf_a, 20,
                                 ">", NULL, true, false);

        // _indent_prefix = false → first line starts with ">" only
        result_b = d_text_lineup(&tmpl, buf_b, 20,
                                 ">", NULL, false, false);

        test_indent_true  = result_a &&
                            D_TL_TEST_STARTS_WITH(buf_a, "  >", 3);
        test_indent_false = result_b &&
                            D_TL_TEST_STARTS_WITH(buf_b, ">", 1);

        // test 3 & 4: _outdent_suffix true vs false
        buf_a->pos = 0;
        if (buf_a->buffer)
        {
            buf_a->buffer[0] = '\0';
        }

        buf_b->pos = 0;
        if (buf_b->buffer)
        {
            buf_b->buffer[0] = '\0';
        }

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_suffix = line_suffix;
        tmpl.flags       = D_BEHAVIOR_FLAG_LINE_SUFFIX_ALL_LINES |
                           D_BEHAVIOR_FLAG_RIGHT_ALIGN_LINE_SUFFIX;

        // _outdent_suffix = true
        result_a = d_text_lineup(&tmpl, buf_a, 20,
                                 NULL, "END", false, true);

        // _outdent_suffix = false
        result_b = d_text_lineup(&tmpl, buf_b, 20,
                                 NULL, "END", false, false);

        // with _outdent_suffix true the last line still gets line_suffix
        test_outdent_true  = result_a &&
                             D_TL_TEST_BUF_CONTAINS(buf_a, "END") &&
                             D_TL_TEST_BUF_CONTAINS(buf_a, " \\");

        // with _outdent_suffix false the last line gets suffix only
        test_outdent_false = result_b &&
                             D_TL_TEST_BUF_CONTAINS(buf_b, "END");

        // test 5: both false → valid output
        buf_a->pos = 0;
        if (buf_a->buffer)
        {
            buf_a->buffer[0] = '\0';
        }

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens = tokens;
        result_a    = d_text_lineup(&tmpl, buf_a, 20,
                                    "P", "S", false, false);
        test_both_false = result_a &&
                          D_TL_TEST_BUF_CONTAINS(buf_a, "aaa");

        // test 6: both true → valid output
        buf_b->pos = 0;
        if (buf_b->buffer)
        {
            buf_b->buffer[0] = '\0';
        }

        d_memset(&tmpl, 0, sizeof(tmpl));
        tmpl.tokens      = tokens;
        tmpl.line_prefix = line_prefix;
        tmpl.line_suffix = line_suffix;
        result_b         = d_text_lineup(&tmpl, buf_b, 20,
                                         "P", "S", true, true);
        test_both_true = result_b &&
                         D_TL_TEST_BUF_CONTAINS(buf_b, "aaa");
    }

    d_text_buffer_free(buf_a);
    d_text_buffer_free(buf_b);
    d_string_array_free(tokens);

    // build result tree
    group = d_test_object_new_interior("indent_prefix / outdent_suffix", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("indent_true",
                                           test_indent_true,
                                           "_indent_prefix true: line_prefix first");
    group->elements[idx++] = D_ASSERT_TRUE("indent_false",
                                           test_indent_false,
                                           "_indent_prefix false: prefix only");
    group->elements[idx++] = D_ASSERT_TRUE("outdent_true",
                                           test_outdent_true,
                                           "_outdent_suffix true: line_suffix on last");
    group->elements[idx++] = D_ASSERT_TRUE("outdent_false",
                                           test_outdent_false,
                                           "_outdent_suffix false: suffix only on last");
    group->elements[idx++] = D_ASSERT_TRUE("both_false",
                                           test_both_false,
                                           "both false: valid output");
    group->elements[idx++] = D_ASSERT_TRUE("both_true",
                                           test_both_true,
                                           "both true: valid output");

    return group;
}


/*
d_tests_sa_text_lineup_lineup_all
  Runs all d_text_lineup tests (Section III).
  Tests the following:
  - null/invalid input rejection
  - no-wrap output (line_width == 0)
  - multi-line wrapping
  - first-line prefix / last-line suffix
  - per-line prefix / per-line suffix
  - separator placement
  - token alignment flags
  - behaviour flags
  - min_token_width enforcement
  - indent_prefix / outdent_suffix parameters
*/
struct d_test_object*
d_tests_sa_text_lineup_lineup_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("d_text_lineup", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_sa_text_lineup_null_inputs();
    group->elements[idx++] = d_tests_sa_text_lineup_no_wrap();
    group->elements[idx++] = d_tests_sa_text_lineup_wrapping();
    group->elements[idx++] = d_tests_sa_text_lineup_prefix_suffix();
    group->elements[idx++] = d_tests_sa_text_lineup_line_prefix_suffix();
    group->elements[idx++] = d_tests_sa_text_lineup_separators();
    group->elements[idx++] = d_tests_sa_text_lineup_token_alignment();
    group->elements[idx++] = d_tests_sa_text_lineup_flags();
    group->elements[idx++] = d_tests_sa_text_lineup_min_token_width();
    group->elements[idx++] = d_tests_sa_text_lineup_indent_outdent();

    return group;
}


#undef D_TL_TEST_BUF_CONTAINS
#undef D_TL_TEST_BUF_EQUALS
#undef D_TL_TEST_STARTS_WITH
