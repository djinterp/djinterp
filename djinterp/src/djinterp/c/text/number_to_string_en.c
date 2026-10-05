/*******************************************************************************
* djinterp [c]                                             number_to_string_en.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_en.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.03
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_en.h"  // corresponding header
// std
#include <string.h>
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // int64_t, uint64_t,
                                                     // UINT64_C

typedef struct nts_word
{
    const char* s;
    unsigned char len;
} nts_word;

#define NTS_WORD(lit) { lit, (unsigned char)(sizeof(lit) - 1u) }

static const nts_word g_units[] =
{
    NTS_WORD("zero"),
    NTS_WORD("one"),
    NTS_WORD("two"),
    NTS_WORD("three"),
    NTS_WORD("four"),
    NTS_WORD("five"),
    NTS_WORD("six"),
    NTS_WORD("seven"),
    NTS_WORD("eight"),
    NTS_WORD("nine")
};

static const nts_word g_unit_ordinals[] =
{
    NTS_WORD("zeroth"),
    NTS_WORD("first"),
    NTS_WORD("second"),
    NTS_WORD("third"),
    NTS_WORD("fourth"),
    NTS_WORD("fifth"),
    NTS_WORD("sixth"),
    NTS_WORD("seventh"),
    NTS_WORD("eighth"),
    NTS_WORD("ninth")
};

static const nts_word g_teens[] =
{
    NTS_WORD("ten"),
    NTS_WORD("eleven"),
    NTS_WORD("twelve"),
    NTS_WORD("thirteen"),
    NTS_WORD("fourteen"),
    NTS_WORD("fifteen"),
    NTS_WORD("sixteen"),
    NTS_WORD("seventeen"),
    NTS_WORD("eighteen"),
    NTS_WORD("nineteen")
};

static const nts_word g_teen_ordinals[] =
{
    NTS_WORD("tenth"),
    NTS_WORD("eleventh"),
    NTS_WORD("twelfth"),
    NTS_WORD("thirteenth"),
    NTS_WORD("fourteenth"),
    NTS_WORD("fifteenth"),
    NTS_WORD("sixteenth"),
    NTS_WORD("seventeenth"),
    NTS_WORD("eighteenth"),
    NTS_WORD("nineteenth")
};

static const nts_word g_tens[] =
{
    NTS_WORD(""),
    NTS_WORD(""),
    NTS_WORD("twenty"),
    NTS_WORD("thirty"),
    NTS_WORD("forty"),
    NTS_WORD("fifty"),
    NTS_WORD("sixty"),
    NTS_WORD("seventy"),
    NTS_WORD("eighty"),
    NTS_WORD("ninety")
};

static const nts_word g_tens_ordinals[] =
{
    NTS_WORD(""),
    NTS_WORD(""),
    NTS_WORD("twentieth"),
    NTS_WORD("thirtieth"),
    NTS_WORD("fortieth"),
    NTS_WORD("fiftieth"),
    NTS_WORD("sixtieth"),
    NTS_WORD("seventieth"),
    NTS_WORD("eightieth"),
    NTS_WORD("ninetieth")
};

static const nts_word g_magnitudes[] =
{
    NTS_WORD(""),
    NTS_WORD("thousand"),
    NTS_WORD("million"),
    NTS_WORD("billion"),
    NTS_WORD("trillion"),
    NTS_WORD("quadrillion"),
    NTS_WORD("quintillion")
};

static const nts_word g_magnitude_ordinals[] =
{
    NTS_WORD(""),
    NTS_WORD("thousandth"),
    NTS_WORD("millionth"),
    NTS_WORD("billionth"),
    NTS_WORD("trillionth"),
    NTS_WORD("quadrillionth"),
    NTS_WORD("quintillionth")
};

static const uint64_t g_pow1000[] =
{
    UINT64_C(1),
    UINT64_C(1000),
    UINT64_C(1000000),
    UINT64_C(1000000000),
    UINT64_C(1000000000000),
    UINT64_C(1000000000000000),
    UINT64_C(1000000000000000000)
};

static void nts_term(char* dst, size_t dst_cap, size_t total)
{
    if (dst_cap == 0u) {
        return;
    }

    if (total < dst_cap) {
        dst[total] = '\0';
    } else {
        dst[dst_cap - 1u] = '\0';
    }
}

static void nts_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        size_t avail = dst_cap - off;
        size_t copy  = (n < avail) ? n : avail;
        if (copy != 0u) {
            memcpy(dst + off, s, copy);
        }
    }

    *total = off + n;
}

static void nts_put_word(char* dst, size_t dst_cap, size_t* total, nts_word w)
{
    nts_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_put_char(dst, dst_cap, total, ' ');
}

static void nts_put_dash(char* dst, size_t dst_cap, size_t* total, unsigned flags)
{
    nts_put_char(dst, dst_cap, total, (flags & NTS_FLAG_HYPHENS) ? '-' : ' ');
}

static void nts_put_and_connector(char* dst, size_t dst_cap, size_t* total)
{
    nts_put_mem(dst, dst_cap, total, " and ", 5u);
}

static void nts_put_comma_connector(char* dst, size_t dst_cap, size_t* total)
{
    nts_put_mem(dst, dst_cap, total, ", ", 2u);
}

static uint64_t nts_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static void nts_parse_triplet_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned hundreds;
    unsigned rem;
    unsigned tens;
    unsigned ones;

    if (value == 0u) {
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    if (hundreds != 0u) {
        nts_put_word(dst, dst_cap, total, g_units[hundreds]);
        nts_put_mem(dst, dst_cap, total, " hundred", 8u);

        if (rem != 0u) {
            if (flags & NTS_FLAG_AND) {
                nts_put_and_connector(dst, dst_cap, total);
            } else {
                nts_put_space(dst, dst_cap, total);
            }
        }
    }

    if (rem == 0u) {
        return;
    }

    if (rem < 10u) {
        nts_put_word(dst, dst_cap, total, g_units[rem]);
        return;
    }

    if (rem < 20u) {
        nts_put_word(dst, dst_cap, total, g_teens[rem - 10u]);
        return;
    }

    tens = rem / 10u;
    ones = rem % 10u;

    nts_put_word(dst, dst_cap, total, g_tens[tens]);
    if (ones != 0u) {
        nts_put_dash(dst, dst_cap, total, flags);
        nts_put_word(dst, dst_cap, total, g_units[ones]);
    }
}

static void nts_parse_triplet_ordinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned hundreds;
    unsigned rem;
    unsigned tens;
    unsigned ones;

    if (value < 10u) {
        nts_put_word(dst, dst_cap, total, g_unit_ordinals[value]);
        return;
    }

    if (value < 20u) {
        nts_put_word(dst, dst_cap, total, g_teen_ordinals[value - 10u]);
        return;
    }

    if (value < 100u) {
        tens = value / 10u;
        ones = value % 10u;

        if (ones == 0u) {
            nts_put_word(dst, dst_cap, total, g_tens_ordinals[tens]);
            return;
        }

        nts_put_word(dst, dst_cap, total, g_tens[tens]);
        nts_put_dash(dst, dst_cap, total, flags);
        nts_put_word(dst, dst_cap, total, g_unit_ordinals[ones]);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    nts_put_word(dst, dst_cap, total, g_units[hundreds]);
    nts_put_mem(dst, dst_cap, total, " hundred", 8u);

    if (rem == 0u) {
        nts_put_mem(dst, dst_cap, total, "th", 2u);
        return;
    }

    if (flags & NTS_FLAG_AND) {
        nts_put_and_connector(dst, dst_cap, total);
    } else {
        nts_put_space(dst, dst_cap, total);
    }

    nts_parse_triplet_ordinal(dst, dst_cap, total, rem, flags);
}

static void nts_put_segment_connector(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    flags,
    uint64_t    remaining)
{
    if ((flags & NTS_FLAG_AND) && remaining < 100u) {
        nts_put_and_connector(dst, dst_cap, total);
        return;
    }

    if (flags & NTS_FLAG_COMMAS) {
        nts_put_comma_connector(dst, dst_cap, total);
        return;
    }

    nts_put_space(dst, dst_cap, total);
}

size_t nts_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;
    unsigned triplets[7];
    int hi = 0;
    int i;
    int wrote = 0;
    uint64_t remaining;

    if (value == 0) {
        nts_put_word(dst, dst_cap, &total, g_units[0]);
        nts_term(dst, dst_cap, total);
        return total;
    }

    mag = nts_abs_i64(value);
    if (value < 0) {
        nts_put_mem(dst, dst_cap, &total, "negative ", 9u);
    }

    {
        uint64_t work = mag;
        for (i = 0; i < 7; ++i) {
            triplets[i] = (unsigned)(work % 1000u);
            work /= 1000u;
        }
    }

    for (hi = 6; hi > 0 && triplets[hi] == 0u; --hi) {
    }

    remaining = mag;

    for (i = hi; i >= 0; --i) {
        unsigned part = triplets[i];
        if (part == 0u) {
            continue;
        }

        if (wrote) {
            nts_put_segment_connector(dst, dst_cap, &total, flags, remaining);
        }

        nts_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
        if (i != 0) {
            nts_put_space(dst, dst_cap, &total);
            nts_put_word(dst, dst_cap, &total, g_magnitudes[i]);
        }

        wrote = 1;
        remaining %= g_pow1000[i];
    }

    nts_term(dst, dst_cap, total);
    return total;
}

size_t nts_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;
    unsigned triplets[7];
    int hi = 0;
    int target = 0;
    int i;
    int wrote = 0;
    uint64_t remaining;

    if (value == 0) {
        nts_put_word(dst, dst_cap, &total, g_unit_ordinals[0]);
        nts_term(dst, dst_cap, total);
        return total;
    }

    mag = nts_abs_i64(value);
    if (value < 0) {
        nts_put_mem(dst, dst_cap, &total, "negative ", 9u);
    }

    {
        uint64_t work = mag;
        for (i = 0; i < 7; ++i) {
            triplets[i] = (unsigned)(work % 1000u);
            work /= 1000u;
        }
    }

    for (hi = 6; hi > 0 && triplets[hi] == 0u; --hi) {
    }

    for (target = 0; target < 7 && triplets[target] == 0u; ++target) {
    }

    remaining = mag;

    for (i = hi; i >= 0; --i) {
        unsigned part = triplets[i];
        if (part == 0u) {
            continue;
        }

        if (wrote) {
            nts_put_segment_connector(dst, dst_cap, &total, flags, remaining);
        }

        if (i == target) {
            if (i == 0) {
                nts_parse_triplet_ordinal(dst, dst_cap, &total, part, flags);
            } else {
                nts_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                nts_put_space(dst, dst_cap, &total);
                nts_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
            }
        } else {
            nts_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
            if (i != 0) {
                nts_put_space(dst, dst_cap, &total);
                nts_put_word(dst, dst_cap, &total, g_magnitudes[i]);
            }
        }

        wrote = 1;
        remaining %= g_pow1000[i];
    }

    nts_term(dst, dst_cap, total);
    return total;
}

static unsigned nts_ordinal_suffix(uint64_t mag)
{
    unsigned last2 = (unsigned)(mag % 100u);
    if (last2 >= 11u && last2 <= 13u) {
        return 0u;
    }

    switch ((unsigned)(mag % 10u)) {
        case 1u: return 1u;
        case 2u: return 2u;
        case 3u: return 3u;
        default: return 0u;
    }
}

size_t nts_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag = nts_abs_i64(value);
    char tmp[32];
    unsigned digits = 0u;
    unsigned suffix;
    unsigned i;

    if (value < 0) {
        nts_put_char(dst, dst_cap, &total, '-');
    }

    do {
        tmp[digits++] = (char)('0' + (mag % 10u));
        mag /= 10u;
    } while (mag != 0u);

    for (i = digits; i-- > 0u;) {
        if ((flags & NTS_FLAG_COMMAS) && i != (digits - 1u) && ((i + 1u) % 3u) == 0u) {
            nts_put_char(dst, dst_cap, &total, ',');
        }
        nts_put_char(dst, dst_cap, &total, tmp[i]);
    }

    mag = nts_abs_i64(value);
    suffix = nts_ordinal_suffix(mag);
    switch (suffix) {
        case 1u:
            nts_put_mem(dst, dst_cap, &total, "st", 2u);
            break;
        case 2u:
            nts_put_mem(dst, dst_cap, &total, "nd", 2u);
            break;
        case 3u:
            nts_put_mem(dst, dst_cap, &total, "rd", 2u);
            break;
        default:
            nts_put_mem(dst, dst_cap, &total, "th", 2u);
            break;
    }

    nts_term(dst, dst_cap, total);
    return total;
}
