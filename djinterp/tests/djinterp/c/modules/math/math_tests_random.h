/*******************************************************************************
* djinterp [c/math]                                          math_tests_random.h
*
* The math harnesses' random numbers.
*   Marsaglia's xorshift on 64 bits -- shifts of 13, 7 and 17 -- kept as two
* 32-bit halves, which every target has. ISO-strict C++98 on a 32-bit target
* has no 64-bit integer at all (dstdint.h declares none, and d_math_umax has
* 32 bits there), and the generator still draws the very numbers a 64-bit
* build draws: each harness tests the same arguments in every build.
*   Each draw steps the generator once and reads the new 64-bit word w, as the
* harnesses once read a uint64_t: w mod n, w's last bit, its first bits as a
* number, its last 32 bits, or its bits shifted, as a double's. A harness
* seeds the generator before its first draw.
*
*
* path:      /tests/djinterp/c/modules/math/math_tests_random.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.11
*                                                            revised: 2026.10.11
*******************************************************************************/

#ifndef DJINTERP_TESTS_C_MATH_MATH_TESTS_RANDOM_H
#define DJINTERP_TESTS_C_MATH_MATH_TESTS_RANDOM_H 1

// std
#include <string.h>  // memcpy
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // uint32_t


// d_tests_random_word
//   struct: a 64-bit word as two 32-bit halves, high 2^32 + low.
struct d_tests_random_word
{
    uint32_t high;  // the first 32 bits
    uint32_t low;   // the last 32 bits
};

// the generator's word: never 0 once seeded
static struct d_tests_random_word g_d_tests_random = { 0u, 1u };

// d_tests_seed
//   starts the generator at the word _high 2^32 + _low, which must not be 0.
static inline void
d_tests_seed(
    uint32_t _high,
    uint32_t _low
)
{
    g_d_tests_random.high = _high;
    g_d_tests_random.low  = _low;

    return;
}

// d_tests_step
//   one step -- w ^= w << 13, w ^= w >> 7, w ^= w << 17 -- each shift
// carrying bits from one half to the other; returns the new word.
static inline struct d_tests_random_word
d_tests_step(void)
{
    uint32_t high = g_d_tests_random.high;
    uint32_t low  = g_d_tests_random.low;

    // w ^= w << 13: the high half takes the low half's first 13 bits
    high ^= (uint32_t)((uint32_t)(high << 13) | (low >> 19));
    low  ^= (uint32_t)(low << 13);

    // w ^= w >> 7: the low half takes the high half's last 7 bits
    low  ^= (uint32_t)((low >> 7) | (uint32_t)(high << 25));
    high ^= (uint32_t)(high >> 7);

    // w ^= w << 17
    high ^= (uint32_t)((uint32_t)(high << 17) | (low >> 15));
    low  ^= (uint32_t)(low << 17);

    g_d_tests_random.high = high;
    g_d_tests_random.low  = low;

    return g_d_tests_random;
}

// d_tests_below
//   w mod _n, for _n from 1 to 2^24: the remainder taken a byte at a time,
// from the first, so that nothing passes 2^32.
static inline unsigned long
d_tests_below(
    unsigned long _n
)
{
    const struct d_tests_random_word w = d_tests_step();
    unsigned long                    r = 0u;
    int                              k = 0;

    for (k = 24; k >= 0; k -= 8)
    {
        r = ((r << 8) | (unsigned long)((w.high >> k) & 0xFFu)) % _n;
    }

    for (k = 24; k >= 0; k -= 8)
    {
        r = ((r << 8) | (unsigned long)((w.low >> k) & 0xFFu)) % _n;
    }

    return r;
}

// d_tests_bit
//   w's last bit, 0 or 1.
static inline unsigned
d_tests_bit(void)
{
    return (unsigned)(d_tests_step().low & 1u);
}

// d_tests_low32
//   w's last 32 bits.
static inline uint32_t
d_tests_low32(void)
{
    return d_tests_step().low;
}

// d_tests_top
//   w's first _bits bits, 1 to 64, as a number: w >> (64 - _bits), exact
// where long double holds _bits bits, and else rounded once, as a conversion
// of the integer would be.
static inline long double
d_tests_top(
    int _bits
)
{
    const struct d_tests_random_word w     = d_tests_step();
    const int                        shift = 64 - _bits;

    // within the first half: its first _bits bits
    if (shift >= 32)
    {
        return (long double)(w.high >> (shift - 32));
    }

    // across both: the first half, and what the shift leaves of the second
    if (shift == 0)
    {
        return ((long double)w.high * 4294967296.0L) + (long double)w.low;
    }

    return ( ((long double)(w.high >> shift) * 4294967296.0L) +
             (long double)((w.low >> shift) |
                           (uint32_t)(w.high << (32 - shift))) );
}

// d_tests_pattern_d
//   stores in *_out the double whose bits are w >> _shift, _shift from 0 to
// 31: the bytes a 64-bit integer of that value has in memory, in the order
// this target keeps one. Stored, not returned: a 32-bit x87 returns a double
// in a register, and loading a signaling NaN there quiets it.
static inline void
d_tests_pattern_d(
    double* _out,
    int     _shift
)
{
    const struct d_tests_random_word w        = d_tests_step();
    const uint32_t                   one      = 1u;
    unsigned char                    first    = 0u;
    unsigned char                    bytes[8] = { 0u };
    uint32_t                         high     = w.high;
    uint32_t                         low      = w.low;

    // the word shifted, the high half's last bits carried down
    if (_shift > 0)
    {
        low  = (uint32_t)((low >> _shift) |
                          (uint32_t)(high << (32 - _shift)));
        high = (uint32_t)(high >> _shift);
    }

    // a target that keeps an integer's last byte first keeps its last half
    // first too
    memcpy(&first, &one, 1u);

    if (first == 1u)
    {
        memcpy(&bytes[0], &low, 4u);
        memcpy(&bytes[4], &high, 4u);
    }
    else
    {
        memcpy(&bytes[0], &high, 4u);
        memcpy(&bytes[4], &low, 4u);
    }

    memcpy(_out, bytes, sizeof(*_out));

    return;
}

#endif  // DJINTERP_TESTS_C_MATH_MATH_TESTS_RANDOM_H
