/*******************************************************************************
* djinterp [test]                                                    t_dckdint.c
*
*   The checked-arithmetic kit's harness (c/dckdint.h): d_ckd_add, d_ckd_sub
* and d_ckd_mul against a table of edge cases and a sweep of random operands,
* each held to a reference that computes the exact result in two halves, so
* that it never relies on the arithmetic under test. A standalone program, as
* the other tests not yet in DTest are (tests/unintegrated/README.md): it
* prints a line per failed check, ends with "N checks, M failed", and exits 0
* only when nothing failed.
*   One source for both languages: it compiles as C (C99 to C23) and as C++
* (C++98 to C++26), and it runs at whichever tier the build reaches. To reach
* the lower tiers on a compiler that has the higher, pre-define
* D_ENV_C_HAS_STDCKDINT_H=0 (the builtins) and also
* D_ENV_COMPILER_HAS_OVERFLOW_BUILTINS=0 (the portable test). Defining
* D_TESTS_CKD_EXPECT_TIER to 1, 2 or 3 also checks that the build reached it.
*
*
* path:      /tests/unintegrated/c/t_dckdint.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.04
*                                                            revised: 2026.11.04
*******************************************************************************/
// std
#include <stdio.h>    // printf
// djinterp
#include "../../../inc/djinterp/c/dckdint.h"


// D_TESTS_CKD_MAX
//   constant: the largest size_t.
#define D_TESTS_CKD_MAX    ((size_t)-1)

// D_TESTS_CKD_HALF_BITS
//   constant: half the bits of a size_t, the width of the reference's halves.
#define D_TESTS_CKD_HALF_BITS    (sizeof(size_t) * 4u)

// D_TESTS_CKD_SWEEP
//   constant: the random operand pairs the sweep tries, per operation.
#define D_TESTS_CKD_SWEEP    200000u

// the counts every check adds to
static unsigned long d_tests_ckd_checks = 0;
static unsigned long d_tests_ckd_failed = 0;


// d_tests_ckd_print_size
//   helper: prints a size_t in decimal without %zu, which ISO C++98's
// library does not have.
static void
d_tests_ckd_print_size(
    size_t _value
)
{
    char   digits[48];
    size_t length = 0;

    do
    {
        digits[length++] = (char)('0' + (int)(_value % 10u));
        _value          /= 10u;
    } while (_value != 0);

    while (length != 0)
    {
        putchar(digits[--length]);
    }

    return;
}

// d_tests_ckd_expect
//   helper: one check of one operation: counts it, and on a mismatch prints
// the operation, its operands, and what came against what was expected.
static void
d_tests_ckd_expect(
    const char* _operation,
    size_t      _a,
    size_t      _b,
    bool        _got_wrapped,
    size_t      _got_value,
    bool        _want_wrapped,
    size_t      _want_value,
    int         _line
)
{
    ++d_tests_ckd_checks;

    if ( (_got_wrapped == _want_wrapped) &&
         (_got_value == _want_value) )
    {
        return;
    }

    ++d_tests_ckd_failed;
    printf("FAIL line %d: %s(", _line, _operation);
    d_tests_ckd_print_size(_a);
    printf(", ");
    d_tests_ckd_print_size(_b);
    printf(") gave %s ", _got_wrapped ? "true" : "false");
    d_tests_ckd_print_size(_got_value);
    printf(", expected %s ", _want_wrapped ? "true" : "false");
    d_tests_ckd_print_size(_want_value);
    printf("\n");

    return;
}

// d_tests_ckd_ref_mul
//   reference: the product of _a and _b, computed exactly in halves of a
// size_t: its low size_t goes to _low, and the result says whether any bits
// remain above it. It uses only products that fit, so it shares nothing with
// the code under test.
static bool
d_tests_ckd_ref_mul(
    size_t  _a,
    size_t  _b,
    size_t* _low
)
{
    const size_t mask = (((size_t)1) << D_TESTS_CKD_HALF_BITS) - 1u;
    const size_t a_lo = _a & mask;
    const size_t a_hi = _a >> D_TESTS_CKD_HALF_BITS;
    const size_t b_lo = _b & mask;
    const size_t b_hi = _b >> D_TESTS_CKD_HALF_BITS;
    // each partial product of two halves fits in a size_t
    const size_t ll   = a_lo * b_lo;
    const size_t lh   = a_lo * b_hi;
    const size_t hl   = a_hi * b_lo;
    const size_t hh   = a_hi * b_hi;
    // the middle column: the cross products' low halves and ll's high half
    const size_t mid  = (ll >> D_TESTS_CKD_HALF_BITS) + (lh & mask) +
                        (hl & mask);
    const size_t high = hh + (lh >> D_TESTS_CKD_HALF_BITS) +
                        (hl >> D_TESTS_CKD_HALF_BITS) +
                        (mid >> D_TESTS_CKD_HALF_BITS);

    *_low = (ll & mask) | ((mid & mask) << D_TESTS_CKD_HALF_BITS);

    return (high != 0);
}

// d_tests_ckd_case
//   helper: runs all three operations on one pair against the reference.
static void
d_tests_ckd_case(
    size_t _a,
    size_t _b,
    int    _line
)
{
    size_t got;
    size_t want;
    bool   wrapped;

    // the sum: it wraps exactly when it comes out below an operand
    want    = _a + _b;
    got     = 0;
    wrapped = d_ckd_add(&got, _a, _b);
    d_tests_ckd_expect("d_ckd_add", _a, _b, wrapped, got,
                       (want < _a), want, _line);

    // the difference: it wraps exactly when the subtrahend is the larger
    want    = _a - _b;
    got     = 0;
    wrapped = d_ckd_sub(&got, _a, _b);
    d_tests_ckd_expect("d_ckd_sub", _a, _b, wrapped, got,
                       (_b > _a), want, _line);

    // the product, against the reference in halves
    {
        size_t low     = 0;
        bool   over    = d_tests_ckd_ref_mul(_a, _b, &low);

        got     = 0;
        wrapped = d_ckd_mul(&got, _a, _b);
        d_tests_ckd_expect("d_ckd_mul", _a, _b, wrapped, got,
                           over, low, _line);
    }

    return;
}

// d_tests_ckd_table
//   group: the edges: zero, one, the largest size, its halves and their
// neighbours, the two products the upgrade audit found wrapping in
// d_array_common_init_fill and d_vector_common_reserve, and each pair the
// other way round.
static void
d_tests_ckd_table(void)
{
    const size_t max  = D_TESTS_CKD_MAX;
    const size_t half = max / 2u;
    const size_t root = ((size_t)1) << D_TESTS_CKD_HALF_BITS;
    const size_t values[] =
    {
        0u, 1u, 2u, 3u, 16u, 255u, 256u,
        root - 1u, root, root + 1u,
        half - 1u, half, half + 1u, half + 2u,
        max / 16u, max / 16u + 1u, max / 16u + 2u,
        max - 1u, max
    };
    const size_t count = sizeof(values) / sizeof(values[0]);
    size_t       i;
    size_t       j;

    for (i = 0; i < count; ++i)
    {
        for (j = 0; j < count; ++j)
        {
            d_tests_ckd_case(values[i], values[j], __LINE__);
        }
    }

    // the audit's two: SIZE_MAX / 2 + 2 elements of 2 bytes wrap to 2, and
    // SIZE_MAX / 16 + 2 elements of 16 bytes wrap to 16
    {
        size_t bytes   = 0;
        bool   wrapped = d_ckd_mul(&bytes, half + 2u, 2u);

        d_tests_ckd_expect("d_ckd_mul", half + 2u, 2u, wrapped, bytes,
                           true, 2u, __LINE__);

        bytes   = 0;
        wrapped = d_ckd_mul(&bytes, max / 16u + 2u, 16u);
        d_tests_ckd_expect("d_ckd_mul", max / 16u + 2u, 16u, wrapped, bytes,
                           true, 16u, __LINE__);
    }

    return;
}

// d_tests_ckd_next
//   helper: xorshift, seeded the same every run, with its operands drawn
// toward the edges: a quarter of them shifted down to a random width, so
// small counts and sizes come up as often as huge ones.
static size_t
d_tests_ckd_next(
    size_t* _state
)
{
    size_t x = *_state;
    size_t value;

    x ^= x << 7;
    x ^= x >> 9;
    x ^= x << 13;
    *_state = x;
    value   = x;

    if ((x & 3u) == 0)
    {
        value >>= (size_t)((x >> 2) % (sizeof(size_t) * 8u));
    }

    return value;
}

// d_tests_ckd_sweep
//   group: D_TESTS_CKD_SWEEP random pairs, each operation against the
// reference.
static void
d_tests_ckd_sweep(void)
{
    size_t        state = (size_t)0x9E3779B9u;
    unsigned long i;

    for (i = 0; i < D_TESTS_CKD_SWEEP; ++i)
    {
        const size_t a = d_tests_ckd_next(&state);
        const size_t b = d_tests_ckd_next(&state);

        d_tests_ckd_case(a, b, __LINE__);
    }

    return;
}

// d_tests_ckd_tier
//   group: the tier the build reached, and, where the build says which it
// expects, that it reached that one.
static void
d_tests_ckd_tier(void)
{
    const char* name = (D_CKD_TIER == D_CKD_TIER_STDCKDINT) ? "stdckdint.h"
                     : (D_CKD_TIER == D_CKD_TIER_BUILTINS)  ? "builtins"
                     :                                        "portable";

    printf("dckdint: tier %d (%s)\n", (int)D_CKD_TIER, name);

#ifdef D_TESTS_CKD_EXPECT_TIER
    ++d_tests_ckd_checks;

    if (D_CKD_TIER != D_TESTS_CKD_EXPECT_TIER)
    {
        ++d_tests_ckd_failed;
        printf("FAIL: tier %d, expected %d\n",
               (int)D_CKD_TIER, (int)D_TESTS_CKD_EXPECT_TIER);
    }
#endif  // D_TESTS_CKD_EXPECT_TIER

    return;
}


int
main(void)
{
    d_tests_ckd_tier();
    d_tests_ckd_table();
    d_tests_ckd_sweep();

    printf("dckdint: %lu checks, %lu failed\n",
           d_tests_ckd_checks,
           d_tests_ckd_failed);

    return (d_tests_ckd_failed == 0) ? 0 : 1;
}
