/*******************************************************************************
* djinterp [test]                                               t_dstring_grow.c
*
*   A regression test for d_string_internal_grow (c/dstring.c), which doubled
* a string's capacity in a loop until it held the request: for a request
* above SIZE_MAX / 2 the doubling wrapped to 0 and never ended. A capacity
* that cannot double now takes the request exactly, through d_ckd_mul
* (c/dckdint.h), so such a request is tried once and refused by the
* allocator, leaving the string as it was. Every public growth reaches the
* function; d_string_reserve is the plainest.
*   Run it with the allocator allowed to return NULL for a huge request:
* under AddressSanitizer, ASAN_OPTIONS=allocator_may_return_null=1. A
* standalone program, as the other tests not yet in DTest are
* (tests/unintegrated/README.md): a line per failed check, then
* "N checks, M failed", and exit status 0 only when nothing failed. Lane B
* owns dstring's suites; this folds into them.
*
*
* path:      /tests/unintegrated/c/t_dstring_grow.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <stdio.h>   // printf
#include <string.h>  // strcmp
// djinterp
#include "../../../inc/djinterp/c/dstring.h"  // dj_string, d_string_reserve


// D_TESTS_DSTRING_GROW_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_DSTRING_GROW_CHECK(expr)                                       \
    d_tests_dstring_grow_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_DSTRING_GROW_MAX
//   constant: the largest size_t.
#define D_TESTS_DSTRING_GROW_MAX    ((size_t)-1)

// the counts every check adds to
static unsigned long d_tests_dstring_grow_checks = 0;
static unsigned long d_tests_dstring_grow_failed = 0;


/*
d_tests_dstring_grow_check
  Counts one check, and prints it where it failed.
*/
static void
d_tests_dstring_grow_check(
    int         _passed,
    const char* _expression,
    int         _line
)
{
    ++d_tests_dstring_grow_checks;

    if (!_passed)
    {
        ++d_tests_dstring_grow_failed;
        printf("FAIL line %d: %s\n", _line, _expression);
    }

    return;
}

/*
d_tests_dstring_grow_huge
  Tests the following:
  - a request just above SIZE_MAX / 2 returns (the loop ends) and fails,
    leaving the string's text and capacity as they were
  - a request of SIZE_MAX does the same
  - an ordinary request still grows by doubling, keeping the text
*/
static void
d_tests_dstring_grow_huge(void)
{
    struct dj_string* const string = d_string_new_from_cstr("abc");

    D_TESTS_DSTRING_GROW_CHECK(string != NULL);

    if (string == NULL)
    {
        return;
    }

    const size_t capacity = string->capacity;

    D_TESTS_DSTRING_GROW_CHECK(
        !d_string_reserve(string, (D_TESTS_DSTRING_GROW_MAX / 2u) + 2u));
    D_TESTS_DSTRING_GROW_CHECK(string->capacity == capacity);
    D_TESTS_DSTRING_GROW_CHECK(strcmp(string->text, "abc") == 0);

    D_TESTS_DSTRING_GROW_CHECK(
        !d_string_reserve(string, D_TESTS_DSTRING_GROW_MAX));
    D_TESTS_DSTRING_GROW_CHECK(string->capacity == capacity);
    D_TESTS_DSTRING_GROW_CHECK(strcmp(string->text, "abc") == 0);

    // an ordinary growth doubles past the request
    D_TESTS_DSTRING_GROW_CHECK(d_string_reserve(string, capacity + 1u));
    D_TESTS_DSTRING_GROW_CHECK(string->capacity >= capacity + 1u);
    D_TESTS_DSTRING_GROW_CHECK(strcmp(string->text, "abc") == 0);

    d_string_free(string);

    return;
}


/*
main
  Every group, then the count.
*/
int
main(void)
{
    printf("t_dstring_grow\n");

    d_tests_dstring_grow_huge();

    printf("%lu checks, %lu failed\n",
           d_tests_dstring_grow_checks,
           d_tests_dstring_grow_failed);

    return (d_tests_dstring_grow_failed == 0u) ? 0 : 1;
}
