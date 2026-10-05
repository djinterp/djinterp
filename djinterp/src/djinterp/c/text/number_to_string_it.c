/*******************************************************************************
* djinterp [c]                                             number_to_string_it.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_it.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_it.h"

// std
#include <string.h>

typedef struct nts_it_word
{
    const char* s;
    unsigned short len;
} nts_it_word;

#define NTS_IT_WORD(lit) { lit, (unsigned short)(sizeof(lit) - 1u) }

static const nts_it_word g_units_0_19[] =
{
    NTS_IT_WORD("zero"),
    NTS_IT_WORD("uno"),
    NTS_IT_WORD("due"),
    NTS_IT_WORD("tre"),
    NTS_IT_WORD("quattro"),
    NTS_IT_WORD("cinque"),
    NTS_IT_WORD("sei"),
    NTS_IT_WORD("sette"),
    NTS_IT_WORD("otto"),
    NTS_IT_WORD("nove"),
    NTS_IT_WORD("dieci"),
    NTS_IT_WORD("undici"),
    NTS_IT_WORD("dodici"),
    NTS_IT_WORD("tredici"),
    NTS_IT_WORD("quattordici"),
    NTS_IT_WORD("quindici"),
    NTS_IT_WORD("sedici"),
    NTS_IT_WORD("diciassette"),
    NTS_IT_WORD("diciotto"),
    NTS_IT_WORD("diciannove")
};

static const nts_it_word g_tens[] =
{
    NTS_IT_WORD(""),
    NTS_IT_WORD(""),
    NTS_IT_WORD("venti"),
    NTS_IT_WORD("trenta"),
    NTS_IT_WORD("quaranta"),
    NTS_IT_WORD("cinquanta"),
    NTS_IT_WORD("sessanta"),
    NTS_IT_WORD("settanta"),
    NTS_IT_WORD("ottanta"),
    NTS_IT_WORD("novanta")
};

static const nts_it_word g_ord_0_10[] =
{
    NTS_IT_WORD("zero"),
    NTS_IT_WORD("primo"),
    NTS_IT_WORD("secondo"),
    NTS_IT_WORD("terzo"),
    NTS_IT_WORD("quarto"),
    NTS_IT_WORD("quinto"),
    NTS_IT_WORD("sesto"),
    NTS_IT_WORD("settimo"),
    NTS_IT_WORD("ottavo"),
    NTS_IT_WORD("nono"),
    NTS_IT_WORD("decimo")
};

static const nts_it_word g_big_singular[] =
{
    NTS_IT_WORD(""),
    NTS_IT_WORD(""),
    NTS_IT_WORD("milione"),
    NTS_IT_WORD("miliardo"),
    NTS_IT_WORD("bilione"),
    NTS_IT_WORD("biliardo"),
    NTS_IT_WORD("trilione")
};

static const nts_it_word g_big_plural[] =
{
    NTS_IT_WORD(""),
    NTS_IT_WORD(""),
    NTS_IT_WORD("milioni"),
    NTS_IT_WORD("miliardi"),
    NTS_IT_WORD("bilioni"),
    NTS_IT_WORD("biliardi"),
    NTS_IT_WORD("trilioni")
};

static const nts_it_word g_big_ordinals[] =
{
    NTS_IT_WORD(""),
    NTS_IT_WORD("millesimo"),
    NTS_IT_WORD("milionesimo"),
    NTS_IT_WORD("miliardesimo"),
    NTS_IT_WORD("bilionesimo"),
    NTS_IT_WORD("biliardesimo"),
    NTS_IT_WORD("trilionesimo")
};

static void nts_it_term(char* dst, size_t dst_cap, size_t total)
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

static void nts_it_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
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

static void nts_it_put_word(char* dst, size_t dst_cap, size_t* total, nts_it_word w)
{
    nts_it_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_it_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_it_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_it_put_char(dst, dst_cap, total, ' ');
}

static uint64_t nts_it_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static unsigned nts_it_split_groups(uint64_t value, unsigned groups[7])
{
    unsigned count = 0u;

    do {
        groups[count++] = (unsigned)(value % UINT64_C(1000));
        value /= UINT64_C(1000);
    } while (value != 0u && count < 7u);

    return count;
}

static void nts_it_append_units_with_tre_accent(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    if (value == 3u) {
        nts_it_put_mem(dst, dst_cap, total, "tré", sizeof("tré") - 1u);
    } else {
        nts_it_put_word(dst, dst_cap, total, g_units_0_19[value]);
    }
}

static void nts_it_write_sub100_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned tens;
    unsigned ones;
    nts_it_word t;

    if (value < 20u) {
        nts_it_put_word(dst, dst_cap, total, g_units_0_19[value]);
        return;
    }

    tens = value / 10u;
    ones = value % 10u;
    t    = g_tens[tens];

    if (ones == 1u || ones == 8u) {
        nts_it_put_mem(dst, dst_cap, total, t.s, t.len - 1u);
    } else {
        nts_it_put_word(dst, dst_cap, total, t);
    }

    if (ones != 0u) {
        nts_it_append_units_with_tre_accent(dst, dst_cap, total, ones);
    }
}

static void nts_it_write_triplet_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned hundreds;
    unsigned rem;

    if (value == 0u) {
        return;
    }

    if (value < 100u) {
        nts_it_write_sub100_cardinal(dst, dst_cap, total, value);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    if (hundreds == 1u) {
        if (rem >= 80u && rem < 90u) {
            nts_it_put_mem(dst, dst_cap, total, "cent", 4u);
        } else {
            nts_it_put_mem(dst, dst_cap, total, "cento", 5u);
        }
    } else {
        nts_it_put_word(dst, dst_cap, total, g_units_0_19[hundreds]);
        if (rem >= 80u && rem < 90u) {
            nts_it_put_mem(dst, dst_cap, total, "cent", 4u);
        } else {
            nts_it_put_mem(dst, dst_cap, total, "cento", 5u);
        }
    }

    if (rem != 0u) {
        nts_it_write_sub100_cardinal(dst, dst_cap, total, rem);
    }
}

static void nts_it_write_under_million_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned thousands;
    unsigned rem;

    if (value == 0u) {
        return;
    }

    if (value < 1000u) {
        nts_it_write_triplet_cardinal(dst, dst_cap, total, value);
        return;
    }

    thousands = value / 1000u;
    rem       = value % 1000u;

    if (thousands == 1u) {
        nts_it_put_mem(dst, dst_cap, total, "mille", 5u);
    } else {
        nts_it_write_triplet_cardinal(dst, dst_cap, total, thousands);
        nts_it_put_mem(dst, dst_cap, total, "mila", 4u);
    }

    if (rem != 0u) {
        nts_it_write_triplet_cardinal(dst, dst_cap, total, rem);
    }
}

static size_t nts_it_build_under_million_cardinal(char* buf, size_t cap, unsigned value)
{
    size_t total = 0u;

    nts_it_write_under_million_cardinal(buf, cap, &total, value);
    nts_it_term(buf, cap, total);
    return total;
}


static size_t nts_it_trim_final_vowel_bytes(const char* s, size_t n)
{
    if (n >= 2u && (unsigned char)s[n - 2u] == 0xC3u && (unsigned char)s[n - 1u] == 0xA9u) {
        return n - 2u;
    }

    if (n != 0u) {
        char c = s[n - 1u];
        if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') {
            return n - 1u;
        }
    }

    return n;
}

static size_t nts_it_make_simple_ordinal_from_cardinal(
    char*           dst,
    size_t          dst_cap,
    const char*     cardinal,
    size_t          cardinal_len)
{
    size_t total = 0u;
    size_t base_len;

    if (cardinal_len == 0u) {
        nts_it_term(dst, dst_cap, total);
        return total;
    }

    if (cardinal_len >= 2u &&
        (unsigned char)cardinal[cardinal_len - 2u] == 0xC3u &&
        (unsigned char)cardinal[cardinal_len - 1u] == 0xA9u) {
        base_len = cardinal_len - 2u;
        nts_it_put_mem(dst, dst_cap, &total, cardinal, base_len);
        nts_it_put_mem(dst, dst_cap, &total, "eesimo", 6u);
        nts_it_term(dst, dst_cap, total);
        return total;
    }

    if (cardinal_len >= 3u &&
        memcmp(cardinal + (cardinal_len - 3u), "sei", 3u) == 0) {
        nts_it_put_mem(dst, dst_cap, &total, cardinal, cardinal_len);
        nts_it_put_mem(dst, dst_cap, &total, "esimo", 5u);
        nts_it_term(dst, dst_cap, total);
        return total;
    }

    base_len = nts_it_trim_final_vowel_bytes(cardinal, cardinal_len);
    nts_it_put_mem(dst, dst_cap, &total, cardinal, base_len);
    nts_it_put_mem(dst, dst_cap, &total, "esimo", 5u);
    nts_it_term(dst, dst_cap, total);
    return total;
}

static size_t nts_it_build_under_million_ordinal(char* buf, size_t cap, unsigned value)
{
    char cardinal[512];
    size_t n;

    if (value <= 10u) {
        size_t total = 0u;
        nts_it_put_word(buf, cap, &total, g_ord_0_10[value]);
        nts_it_term(buf, cap, total);
        return total;
    }

    if ((value % 1000u) == 0u) {
        unsigned q = value / 1000u;
        size_t total = 0u;

        if (q == 1u) {
            nts_it_put_word(buf, cap, &total, g_big_ordinals[1]);
        } else {
            n = nts_it_build_under_million_cardinal(cardinal, sizeof(cardinal), q);
            nts_it_put_mem(buf, cap, &total, cardinal, n);
            nts_it_put_word(buf, cap, &total, g_big_ordinals[1]);
        }

        nts_it_term(buf, cap, total);
        return total;
    }

    n = nts_it_build_under_million_cardinal(cardinal, sizeof(cardinal), value);
    return nts_it_make_simple_ordinal_from_cardinal(buf, cap, cardinal, n);
}

static void nts_it_write_positive_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    uint64_t    mag)
{
    unsigned groups[7];
    unsigned count;
    unsigned i;
    int first = 1;

    if (mag == 0u) {
        nts_it_put_word(dst, dst_cap, total, g_units_0_19[0]);
        return;
    }

    count = nts_it_split_groups(mag, groups);

    for (i = count; i-- > 2u;) {
        unsigned group = groups[i];

        if (group == 0u) {
            continue;
        }

        if (!first) {
            nts_it_put_space(dst, dst_cap, total);
        }
        first = 0;

        if (group == 1u) {
            nts_it_put_mem(dst, dst_cap, total, "un ", 3u);
            nts_it_put_word(dst, dst_cap, total, g_big_singular[i]);
        } else {
            nts_it_write_triplet_cardinal(dst, dst_cap, total, group);
            nts_it_put_space(dst, dst_cap, total);
            nts_it_put_word(dst, dst_cap, total, g_big_plural[i]);
        }
    }

    if (count > 1u && groups[1] != 0u) {
        if (!first) {
            nts_it_put_space(dst, dst_cap, total);
        }
        first = 0;

        if (groups[1] == 1u) {
            nts_it_put_mem(dst, dst_cap, total, "mille", 5u);
        } else {
            nts_it_write_triplet_cardinal(dst, dst_cap, total, groups[1]);
            nts_it_put_mem(dst, dst_cap, total, "mila", 4u);
        }
    }

    if (groups[0] != 0u || first) {
        if (!first && groups[0] != 0u && groups[1] == 0u) {
            nts_it_put_space(dst, dst_cap, total);
        }
        nts_it_write_triplet_cardinal(dst, dst_cap, total, groups[0]);
    }
}

/* A direct recursive ordinal writer for million-plus values. */
static void nts_it_write_positive_ordinal_rec(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    uint64_t    mag)
{
    unsigned groups[7];
    unsigned count;
    unsigned top;
    uint64_t high_value;
    uint64_t low_value;
    char tmp[512];
    size_t n;

    if (mag == 0u) {
        nts_it_put_word(dst, dst_cap, total, g_ord_0_10[0]);
        return;
    }

    if (mag < UINT64_C(1000000)) {
        n = nts_it_build_under_million_ordinal(tmp, sizeof(tmp), (unsigned)mag);
        nts_it_put_mem(dst, dst_cap, total, tmp, n);
        return;
    }

    count = nts_it_split_groups(mag, groups);
    top   = count - 1u;

    while (top > 0u && groups[top] == 0u) {
        --top;
    }

    if (top <= 1u) {
        n = nts_it_build_under_million_ordinal(tmp, sizeof(tmp), (unsigned)(mag % UINT64_C(1000000)));
        nts_it_put_mem(dst, dst_cap, total, tmp, n);
        return;
    }

    high_value = 0u;
    {
        unsigned i;
        for (i = top; i-- > 0u;) {
            high_value = high_value * UINT64_C(1000) + groups[i];
        }
        high_value = high_value / UINT64_C(1000);
    }

    low_value = mag;
    {
        uint64_t base = 1u;
        unsigned k;
        for (k = 0u; k < top; ++k) {
            base *= UINT64_C(1000);
        }
        high_value = mag / base;
        low_value  = mag % base;

        if (low_value == 0u) {
            if (top == 1u) {
                if (high_value == 1u) {
                    nts_it_put_word(dst, dst_cap, total, g_big_ordinals[1]);
                } else {
                    n = nts_it_build_under_million_cardinal(tmp, sizeof(tmp), (unsigned)high_value);
                    nts_it_put_mem(dst, dst_cap, total, tmp, n);
                    nts_it_put_word(dst, dst_cap, total, g_big_ordinals[1]);
                }
                return;
            }

            if (high_value == 1u) {
                nts_it_put_word(dst, dst_cap, total, g_big_ordinals[top]);
            } else {
                n = nts_it_build_under_million_cardinal(tmp, sizeof(tmp), (unsigned)high_value);
                nts_it_put_mem(dst, dst_cap, total, tmp, n);
                nts_it_put_word(dst, dst_cap, total, g_big_ordinals[top]);
            }
            return;
        }

        if (high_value == 1u) {
            nts_it_put_mem(dst, dst_cap, total, "un ", 3u);
            nts_it_put_word(dst, dst_cap, total, g_big_singular[top]);
        } else {
            n = nts_it_build_under_million_cardinal(tmp, sizeof(tmp), (unsigned)high_value);
            nts_it_put_mem(dst, dst_cap, total, tmp, n);
            nts_it_put_space(dst, dst_cap, total);
            nts_it_put_word(dst, dst_cap, total, g_big_plural[top]);
        }

        nts_it_put_space(dst, dst_cap, total);
        nts_it_write_positive_ordinal_rec(dst, dst_cap, total, low_value);
    }
}

size_t nts_it_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;

    (void)flags;

    if (value < 0) {
        nts_it_put_mem(dst, dst_cap, &total, "meno ", 5u);
    }

    mag = nts_it_abs_i64(value);
    nts_it_write_positive_cardinal(dst, dst_cap, &total, mag);

    nts_it_term(dst, dst_cap, total);
    return total;
}

size_t nts_it_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;

    (void)flags;

    if (value < 0) {
        nts_it_put_mem(dst, dst_cap, &total, "meno ", 5u);
    }

    mag = nts_it_abs_i64(value);
    nts_it_write_positive_ordinal_rec(dst, dst_cap, &total, mag);

    nts_it_term(dst, dst_cap, total);
    return total;
}

size_t nts_it_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    char digits[32];
    size_t n = 0u;
    size_t total = 0u;
    uint64_t mag = nts_it_abs_i64(value);

    if (value < 0) {
        nts_it_put_mem(dst, dst_cap, &total, "meno ", 5u);
    }

    do {
        digits[n++] = (char)('0' + (unsigned)(mag % 10u));
        mag /= 10u;
    } while (mag != 0u);

    while (n != 0u) {
        size_t idx = n - 1u;
        nts_it_put_char(dst, dst_cap, &total, digits[idx]);
        n = idx;

        if ((flags & NTS_IT_FLAG_GROUP_DOTS) && n != 0u && (n % 3u) == 0u) {
            nts_it_put_char(dst, dst_cap, &total, '.');
        }
    }

    nts_it_put_mem(dst, dst_cap, &total, "°", sizeof("°") - 1u);
    nts_it_term(dst, dst_cap, total);
    return total;
}
