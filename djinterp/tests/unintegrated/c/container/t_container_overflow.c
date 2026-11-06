/*******************************************************************************
* djinterp [test]                                         t_container_overflow.c
*
*   Regression tests for the two heap overflows the upgrade audit of
* 2026.11.01 confirmed, both a size computed as count * element size without
* a check: d_array_common_init_fill (SIZE_MAX / 2 + 2 elements of 2 bytes
* wrapped to 2, malloc(2) succeeded and the fill wrote past it) and
* d_vector_common_reserve (SIZE_MAX / 16 + 2 elements of 16 bytes wrapped to
* 16, realloc shrank the buffer and the vector recorded the capacity asked
* for, so the second push_back wrote past it). Each now refuses a size that
* does not fit, through d_ckd_mul (c/dckdint.h). This harness holds both to
* that, and checks that ordinary calls still work; built with
* -fsanitize=address it also proves nothing is written out of bounds. A
* standalone program, as the other tests not yet in DTest are
* (tests/unintegrated/README.md): it prints a line per failed check, ends
* with "N checks, M failed", and exits 0 only when nothing failed.
*
*
* path:      /tests/unintegrated/c/container/t_container_overflow.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <stdio.h>    // printf
#include <stdlib.h>   // free
#include <string.h>   // memcmp
// djinterp
#include "../../../../inc/djinterp/c/container/array/array_common.h"
#include "../../../../inc/djinterp/c/container/vector/vector_common.h"


// D_TESTS_OVERFLOW_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_OVERFLOW_CHECK(expr)                                           \
    d_tests_overflow_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_OVERFLOW_MAX
//   constant: the largest size_t.
#define D_TESTS_OVERFLOW_MAX    ((size_t)-1)

// the counts every check adds to
static unsigned long d_tests_overflow_checks = 0;
static unsigned long d_tests_overflow_failed = 0;

// a 16-byte element, the size the audit's vector repro used
struct d_tests_overflow_item
{
    unsigned long a;
    unsigned long b;
    char          pad[16 - (2 * sizeof(unsigned long)) % 16];
};


// d_tests_overflow_check
//   helper: counts one check, and prints it where it failed.
static void
d_tests_overflow_check(
    int         _passed,
    const char* _expression,
    int         _line
)
{
    ++d_tests_overflow_checks;

    if (!_passed)
    {
        ++d_tests_overflow_failed;
        printf("FAIL line %d: %s\n", _line, _expression);
    }

    return;
}

// d_tests_overflow_fill
//   group: d_array_common_init_fill refuses a count whose bytes wrap, leaving
// the array empty, and fills an ordinary one.
static void
d_tests_overflow_fill(void)
{
    void*          data  = NULL;
    size_t         count = 99;
    unsigned short value = 0xABCDu;
    bool           made;

    // the audit's call: SIZE_MAX / 2 + 2 elements of 2 bytes
    made = d_array_common_init_fill(&data, &count, sizeof(value),
                                    (D_TESTS_OVERFLOW_MAX / 2u) + 2u, &value);
    D_TESTS_OVERFLOW_CHECK(!made);
    D_TESTS_OVERFLOW_CHECK(data == NULL);
    D_TESTS_OVERFLOW_CHECK(count == 0);

    // the largest count, of the smallest element that can wrap
    count = 99;
    made  = d_array_common_init_fill(&data, &count, 2u,
                                     D_TESTS_OVERFLOW_MAX, &value);
    D_TESTS_OVERFLOW_CHECK(!made);
    D_TESTS_OVERFLOW_CHECK(data == NULL);
    D_TESTS_OVERFLOW_CHECK(count == 0);

    // an ordinary fill still works, element by element
    made = d_array_common_init_fill(&data, &count, sizeof(value), 5u, &value);
    D_TESTS_OVERFLOW_CHECK(made);
    D_TESTS_OVERFLOW_CHECK(count == 5u);
    D_TESTS_OVERFLOW_CHECK(data != NULL);

    if (data)
    {
        size_t i;

        for (i = 0; i < 5u; ++i)
        {
            D_TESTS_OVERFLOW_CHECK(memcmp((char*)data + (i * sizeof(value)),
                                          &value, sizeof(value)) == 0);
        }

        free(data);
    }

    return;
}

// d_tests_overflow_reserve
//   group: d_vector_common_reserve refuses a capacity whose bytes wrap,
// leaving the vector as it was, so pushing after it stays in bounds; an
// ordinary reserve still grows it.
static void
d_tests_overflow_reserve(void)
{
    void*                        elements = NULL;
    size_t                       count    = 0;
    size_t                       capacity = 0;
    struct d_tests_overflow_item value;
    void*                        before;
    bool                         reserved;

    memset(&value, 0x5A, sizeof(value));

    D_TESTS_OVERFLOW_CHECK(d_vector_common_init(&elements, &count, &capacity,
                                                sizeof(value), 4u));
    before = elements;

    // the audit's call: SIZE_MAX / 16 + 2 elements of 16 bytes
    reserved = d_vector_common_reserve(&elements, count, &capacity,
                                       sizeof(value),
                                       (D_TESTS_OVERFLOW_MAX / 16u) + 2u);
    D_TESTS_OVERFLOW_CHECK(!reserved);
    D_TESTS_OVERFLOW_CHECK(elements == before);
    D_TESTS_OVERFLOW_CHECK(capacity == 4u);

    // the largest capacity
    reserved = d_vector_common_reserve(&elements, count, &capacity,
                                       sizeof(value), D_TESTS_OVERFLOW_MAX);
    D_TESTS_OVERFLOW_CHECK(!reserved);
    D_TESTS_OVERFLOW_CHECK(capacity == 4u);

    // pushing past the old capacity grows it in bounds: the repro's second
    // push_back wrote out of bounds here
    {
        size_t i;

        for (i = 0; i < 6u; ++i)
        {
            D_TESTS_OVERFLOW_CHECK(d_vector_common_push_back(&elements,
                                                             &count,
                                                             &capacity,
                                                             sizeof(value),
                                                             &value));
        }
    }
    D_TESTS_OVERFLOW_CHECK(count == 6u);
    D_TESTS_OVERFLOW_CHECK(capacity >= 6u);

    // an ordinary reserve still works
    reserved = d_vector_common_reserve(&elements, count, &capacity,
                                       sizeof(value), 64u);
    D_TESTS_OVERFLOW_CHECK(reserved);
    D_TESTS_OVERFLOW_CHECK(capacity == 64u);

    free(elements);

    return;
}


int
main(void)
{
    d_tests_overflow_fill();
    d_tests_overflow_reserve();

    printf("container overflow: %lu checks, %lu failed\n",
           d_tests_overflow_checks,
           d_tests_overflow_failed);

    return (d_tests_overflow_failed == 0) ? 0 : 1;
}
