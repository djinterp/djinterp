#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * VALIDATION TESTS
 *****************************************************************************/

/*
d_tests_string_fn_str_is_valid
  Tests d_str_is_valid for string validity checking.
  Tests the following:
  - returns true for normal string
  - returns false for NULL
  - returns false for embedded null
  - handles empty string (length 0)
*/
struct d_test_object*
d_tests_string_fn_str_is_valid
(
    void
)
{
    struct d_test_object* group;
    bool                  test_normal;
    bool                  test_null;
    bool                  test_embedded_null;
    bool                  test_empty;
    size_t                idx;

    // test 1: normal string
    test_normal = d_str_is_valid("Hello", 5);

    // test 2: NULL pointer
    test_null = !d_str_is_valid(NULL, 5);

    // test 3: embedded null
    char buf[] = "He\0lo";
    test_embedded_null = !d_str_is_valid(buf, 5);

    // test 4: empty (length 0 is vacuously true)
    test_empty = d_str_is_valid("anything", 0);

    // build result tree
    group = d_test_object_new_interior("d_str_is_valid", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normal",
                                           test_normal,
                                           "returns true for normal string");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns false for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("embedded_null",
                                           test_embedded_null,
                                           "returns false for embedded null");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");

    return group;
}

/*
d_tests_string_fn_str_is_ascii
  Tests d_str_is_ascii for ASCII validation.
  Tests the following:
  - returns true for pure ASCII
  - returns false for non-ASCII
  - returns false for NULL
  - handles empty string
*/
struct d_test_object*
d_tests_string_fn_str_is_ascii
(
    void
)
{
    struct d_test_object* group;
    bool                  test_ascii;
    bool                  test_non_ascii;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    // test 1: pure ASCII
    test_ascii = d_str_is_ascii("Hello 123!@#", 12);

    // test 2: non-ASCII (high byte)
    char non_ascii[] = { 'H', 'i', (char)0x80, '\0' };
    test_non_ascii = !d_str_is_ascii(non_ascii, 3);

    // test 3: NULL
    test_null = !d_str_is_ascii(NULL, 5);

    // test 4: empty
    test_empty = d_str_is_ascii("", 0);

    // build result tree
    group = d_test_object_new_interior("d_str_is_ascii", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("ascii",
                                           test_ascii,
                                           "returns true for pure ASCII");
    group->elements[idx++] = D_ASSERT_TRUE("non_ascii",
                                           test_non_ascii,
                                           "returns false for non-ASCII");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns false for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");

    return group;
}

/*
d_tests_string_fn_str_is_numeric
  Tests d_str_is_numeric for numeric validation.
  Tests the following:
  - returns true for all digits
  - returns false for letters
  - returns false for mixed
  - returns false for empty
  - returns false for NULL
*/
struct d_test_object*
d_tests_string_fn_str_is_numeric
(
    void
)
{
    struct d_test_object* group;
    bool                  test_digits;
    bool                  test_letters;
    bool                  test_mixed;
    bool                  test_empty;
    bool                  test_null;
    size_t                idx;

    test_digits  = d_str_is_numeric("1234567890", 10);
    test_letters = !d_str_is_numeric("abcdef", 6);
    test_mixed   = !d_str_is_numeric("abc123", 6);
    test_empty   = !d_str_is_numeric("", 0);
    test_null    = !d_str_is_numeric(NULL, 5);

    // build result tree
    group = d_test_object_new_interior("d_str_is_numeric", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("digits",
                                           test_digits,
                                           "returns true for all digits");
    group->elements[idx++] = D_ASSERT_TRUE("letters",
                                           test_letters,
                                           "returns false for letters");
    group->elements[idx++] = D_ASSERT_TRUE("mixed",
                                           test_mixed,
                                           "returns false for mixed");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "returns false for empty");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "returns false for NULL");

    return group;
}

/*
d_tests_string_fn_str_is_alpha
  Tests d_str_is_alpha for alphabetic validation.
  Tests the following:
  - returns true for all letters
  - returns false for digits
  - returns false for mixed
  - returns false for empty
  - handles both cases
*/
struct d_test_object*
d_tests_string_fn_str_is_alpha
(
    void
)
{
    struct d_test_object* group;
    bool                  test_alpha;
    bool                  test_digits;
    bool                  test_mixed;
    bool                  test_empty;
    bool                  test_both_cases;
    size_t                idx;

    test_alpha      = d_str_is_alpha("abcdef", 6);
    test_digits     = !d_str_is_alpha("123456", 6);
    test_mixed      = !d_str_is_alpha("abc123", 6);
    test_empty      = !d_str_is_alpha("", 0);
    test_both_cases = d_str_is_alpha("ABCdef", 6);

    // build result tree
    group = d_test_object_new_interior("d_str_is_alpha", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("alpha",
                                           test_alpha,
                                           "returns true for all letters");
    group->elements[idx++] = D_ASSERT_TRUE("digits",
                                           test_digits,
                                           "returns false for digits");
    group->elements[idx++] = D_ASSERT_TRUE("mixed",
                                           test_mixed,
                                           "returns false for mixed");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "returns false for empty");
    group->elements[idx++] = D_ASSERT_TRUE("both_cases",
                                           test_both_cases,
                                           "handles both cases");

    return group;
}

/*
d_tests_string_fn_str_is_alnum
  Tests d_str_is_alnum for alphanumeric validation.
  Tests the following:
  - returns true for letters and digits
  - returns false for special characters
  - returns false for empty
  - handles pure digits
  - handles pure letters
*/
struct d_test_object*
d_tests_string_fn_str_is_alnum
(
    void
)
{
    struct d_test_object* group;
    bool                  test_alnum;
    bool                  test_special;
    bool                  test_empty;
    bool                  test_digits_only;
    bool                  test_alpha_only;
    size_t                idx;

    test_alnum       = d_str_is_alnum("abc123DEF", 9);
    test_special     = !d_str_is_alnum("abc!@#", 6);
    test_empty       = !d_str_is_alnum("", 0);
    test_digits_only = d_str_is_alnum("12345", 5);
    test_alpha_only  = d_str_is_alnum("abcXYZ", 6);

    // build result tree
    group = d_test_object_new_interior("d_str_is_alnum", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("alnum",
                                           test_alnum,
                                           "returns true for alphanumeric");
    group->elements[idx++] = D_ASSERT_TRUE("special",
                                           test_special,
                                           "returns false for special chars");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "returns false for empty");
    group->elements[idx++] = D_ASSERT_TRUE("digits_only",
                                           test_digits_only,
                                           "handles pure digits");
    group->elements[idx++] = D_ASSERT_TRUE("alpha_only",
                                           test_alpha_only,
                                           "handles pure letters");

    return group;
}

/*
d_tests_string_fn_str_is_whitespace
  Tests d_str_is_whitespace for whitespace validation.
  Tests the following:
  - returns true for all whitespace
  - returns false for non-whitespace
  - returns false for empty
  - handles mixed whitespace types
*/
struct d_test_object*
d_tests_string_fn_str_is_whitespace
(
    void
)
{
    struct d_test_object* group;
    bool                  test_whitespace;
    bool                  test_non_ws;
    bool                  test_empty;
    bool                  test_mixed_ws;
    size_t                idx;

    test_whitespace = d_str_is_whitespace("   ", 3);
    test_non_ws     = !d_str_is_whitespace("hello", 5);
    test_empty      = !d_str_is_whitespace("", 0);
    test_mixed_ws   = d_str_is_whitespace(" \t\n\r ", 5);

    // build result tree
    group = d_test_object_new_interior("d_str_is_whitespace", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("whitespace",
                                           test_whitespace,
                                           "returns true for all whitespace");
    group->elements[idx++] = D_ASSERT_TRUE("non_ws",
                                           test_non_ws,
                                           "returns false for non-whitespace");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "returns false for empty");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_ws",
                                           test_mixed_ws,
                                           "handles mixed whitespace types");

    return group;
}

/*
d_tests_string_fn_validation_all
  Runs all validation tests.
  Tests the following:
  - d_str_is_valid
  - d_str_is_ascii
  - d_str_is_numeric
  - d_str_is_alpha
  - d_str_is_alnum
  - d_str_is_whitespace
*/
struct d_test_object*
d_tests_string_fn_validation_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Validation", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_str_is_valid();
    group->elements[idx++] = d_tests_string_fn_str_is_ascii();
    group->elements[idx++] = d_tests_string_fn_str_is_numeric();
    group->elements[idx++] = d_tests_string_fn_str_is_alpha();
    group->elements[idx++] = d_tests_string_fn_str_is_alnum();
    group->elements[idx++] = d_tests_string_fn_str_is_whitespace();

    return group;
}
