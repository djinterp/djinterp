/*******************************************************************************
* djinterp [c]                                             number_to_string_la.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_la.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_la.h"

// std
#include <string.h>

typedef struct nts_la_word
{
    const char* s;
    unsigned char len;
} nts_la_word;

#define NTS_LA_WORD(lit) { lit, (unsigned char)(sizeof(lit) - 1u) }

enum
{
    NTS_LA_GENDER_MASC   = 0,
    NTS_LA_GENDER_NEUTER = 1
};

static const nts_la_word g_units_masc[] =
{
    NTS_LA_WORD("nihil"),
    NTS_LA_WORD("unus"),
    NTS_LA_WORD("duo"),
    NTS_LA_WORD("tres"),
    NTS_LA_WORD("quattuor"),
    NTS_LA_WORD("quinque"),
    NTS_LA_WORD("sex"),
    NTS_LA_WORD("septem"),
    NTS_LA_WORD("octo"),
    NTS_LA_WORD("novem")
};

static const nts_la_word g_units_neuter[] =
{
    NTS_LA_WORD("nihil"),
    NTS_LA_WORD("unum"),
    NTS_LA_WORD("duo"),
    NTS_LA_WORD("tria"),
    NTS_LA_WORD("quattuor"),
    NTS_LA_WORD("quinque"),
    NTS_LA_WORD("sex"),
    NTS_LA_WORD("septem"),
    NTS_LA_WORD("octo"),
    NTS_LA_WORD("novem")
};

static const nts_la_word g_unit_ordinals[] =
{
    NTS_LA_WORD("nullesimus"),
    NTS_LA_WORD("primus"),
    NTS_LA_WORD("secundus"),
    NTS_LA_WORD("tertius"),
    NTS_LA_WORD("quartus"),
    NTS_LA_WORD("quintus"),
    NTS_LA_WORD("sextus"),
    NTS_LA_WORD("septimus"),
    NTS_LA_WORD("octavus"),
    NTS_LA_WORD("nonus")
};

static const nts_la_word g_teens[] =
{
    NTS_LA_WORD("decem"),
    NTS_LA_WORD("undecim"),
    NTS_LA_WORD("duodecim"),
    NTS_LA_WORD("tredecim"),
    NTS_LA_WORD("quattuordecim"),
    NTS_LA_WORD("quindecim"),
    NTS_LA_WORD("sedecim"),
    NTS_LA_WORD("septendecim"),
    NTS_LA_WORD("duodeviginti"),
    NTS_LA_WORD("undeviginti")
};

static const nts_la_word g_teen_ordinals[] =
{
    NTS_LA_WORD("decimus"),
    NTS_LA_WORD("undecimus"),
    NTS_LA_WORD("duodecimus"),
    NTS_LA_WORD("tertius decimus"),
    NTS_LA_WORD("quartus decimus"),
    NTS_LA_WORD("quintus decimus"),
    NTS_LA_WORD("sextus decimus"),
    NTS_LA_WORD("septimus decimus"),
    NTS_LA_WORD("duodevicesimus"),
    NTS_LA_WORD("undevicesimus")
};

static const nts_la_word g_tens[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD(""),
    NTS_LA_WORD("viginti"),
    NTS_LA_WORD("triginta"),
    NTS_LA_WORD("quadraginta"),
    NTS_LA_WORD("quinquaginta"),
    NTS_LA_WORD("sexaginta"),
    NTS_LA_WORD("septuaginta"),
    NTS_LA_WORD("octoginta"),
    NTS_LA_WORD("nonaginta")
};

static const nts_la_word g_tens_ordinals[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD(""),
    NTS_LA_WORD("vicesimus"),
    NTS_LA_WORD("tricesimus"),
    NTS_LA_WORD("quadragesimus"),
    NTS_LA_WORD("quinquagesimus"),
    NTS_LA_WORD("sexagesimus"),
    NTS_LA_WORD("septuagesimus"),
    NTS_LA_WORD("octogesimus"),
    NTS_LA_WORD("nonagesimus")
};

static const nts_la_word g_hundreds_masc[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD("centum"),
    NTS_LA_WORD("ducenti"),
    NTS_LA_WORD("trecenti"),
    NTS_LA_WORD("quadringenti"),
    NTS_LA_WORD("quingenti"),
    NTS_LA_WORD("sescenti"),
    NTS_LA_WORD("septingenti"),
    NTS_LA_WORD("octingenti"),
    NTS_LA_WORD("nongenti")
};

static const nts_la_word g_hundreds_neuter[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD("centum"),
    NTS_LA_WORD("ducenta"),
    NTS_LA_WORD("trecenta"),
    NTS_LA_WORD("quadringenta"),
    NTS_LA_WORD("quingenta"),
    NTS_LA_WORD("sescenta"),
    NTS_LA_WORD("septingenta"),
    NTS_LA_WORD("octingenta"),
    NTS_LA_WORD("nongenta")
};

static const nts_la_word g_hundred_ordinals[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD("centesimus"),
    NTS_LA_WORD("ducentesimus"),
    NTS_LA_WORD("trecentesimus"),
    NTS_LA_WORD("quadringentesimus"),
    NTS_LA_WORD("quingentesimus"),
    NTS_LA_WORD("sescentesimus"),
    NTS_LA_WORD("septingentesimus"),
    NTS_LA_WORD("octingentesimus"),
    NTS_LA_WORD("nongentesimus")
};

static const nts_la_word g_adverb_units[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD("semel"),
    NTS_LA_WORD("bis"),
    NTS_LA_WORD("ter"),
    NTS_LA_WORD("quater"),
    NTS_LA_WORD("quinquies"),
    NTS_LA_WORD("sexies"),
    NTS_LA_WORD("septies"),
    NTS_LA_WORD("octies"),
    NTS_LA_WORD("novies")
};

static const nts_la_word g_adverb_teens[] =
{
    NTS_LA_WORD("decies"),
    NTS_LA_WORD("undecies"),
    NTS_LA_WORD("duodecies"),
    NTS_LA_WORD("tredecies"),
    NTS_LA_WORD("quattuordecies"),
    NTS_LA_WORD("quindecies"),
    NTS_LA_WORD("sedecies"),
    NTS_LA_WORD("septendecies"),
    NTS_LA_WORD("duodevicies"),
    NTS_LA_WORD("undevicies")
};

static const nts_la_word g_adverb_tens[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD(""),
    NTS_LA_WORD("vicies"),
    NTS_LA_WORD("tricies"),
    NTS_LA_WORD("quadragies"),
    NTS_LA_WORD("quinquagies"),
    NTS_LA_WORD("sexagies"),
    NTS_LA_WORD("septuagies"),
    NTS_LA_WORD("octogies"),
    NTS_LA_WORD("nonagies")
};

static const nts_la_word g_adverb_hundreds[] =
{
    NTS_LA_WORD(""),
    NTS_LA_WORD("centies"),
    NTS_LA_WORD("ducenties"),
    NTS_LA_WORD("trecenties"),
    NTS_LA_WORD("quadringenties"),
    NTS_LA_WORD("quingenties"),
    NTS_LA_WORD("sescenties"),
    NTS_LA_WORD("septingenties"),
    NTS_LA_WORD("octingenties"),
    NTS_LA_WORD("nongenties")
};

static void nts_la_term(char* dst, size_t dst_cap, size_t total)
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

static void nts_la_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        size_t avail = dst_cap - off;
        size_t copy = (n < avail) ? n : avail;
        if (copy != 0u) {
            memcpy(dst + off, s, copy);
        }
    }

    *total = off + n;
}

static void nts_la_put_word(char* dst, size_t dst_cap, size_t* total, nts_la_word w)
{
    nts_la_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_la_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_la_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_la_put_char(dst, dst_cap, total, ' ');
}

static void nts_la_put_connector(char* dst, size_t dst_cap, size_t* total, unsigned flags)
{
    if (flags & NTS_LA_FLAG_COMMAS) {
        nts_la_put_mem(dst, dst_cap, total, ", ", 2u);
    } else {
        nts_la_put_space(dst, dst_cap, total);
    }
}

static uint64_t nts_la_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static nts_la_word nts_la_unit_cardinal(unsigned value, unsigned gender)
{
    return (gender == NTS_LA_GENDER_NEUTER) ? g_units_neuter[value] : g_units_masc[value];
}

static nts_la_word nts_la_hundred_cardinal(unsigned value, unsigned gender)
{
    return (gender == NTS_LA_GENDER_NEUTER) ? g_hundreds_neuter[value] : g_hundreds_masc[value];
}

static void nts_la_format_cardinal_lt1000(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    gender,
    unsigned    flags)
{
    unsigned hundreds;
    unsigned rem;
    unsigned tens;
    unsigned ones;

    if (value == 0u) {
        nts_la_put_word(dst, dst_cap, total, nts_la_unit_cardinal(0u, gender));
        return;
    }

    if (value < 10u) {
        nts_la_put_word(dst, dst_cap, total, nts_la_unit_cardinal(value, gender));
        return;
    }

    if (value < 20u) {
        nts_la_put_word(dst, dst_cap, total, g_teens[value - 10u]);
        return;
    }

    if (value < 100u) {
        tens = value / 10u;
        ones = value % 10u;

        nts_la_put_word(dst, dst_cap, total, g_tens[tens]);
        if (ones != 0u) {
            if (flags & NTS_LA_FLAG_ET) {
                nts_la_put_mem(dst, dst_cap, total, " et ", 4u);
            } else {
                nts_la_put_space(dst, dst_cap, total);
            }
            nts_la_put_word(dst, dst_cap, total, nts_la_unit_cardinal(ones, gender));
        }
        return;
    }

    hundreds = value / 100u;
    rem = value % 100u;

    nts_la_put_word(dst, dst_cap, total, nts_la_hundred_cardinal(hundreds, gender));
    if (rem != 0u) {
        if ((flags & NTS_LA_FLAG_ET) && rem < 10u) {
            nts_la_put_mem(dst, dst_cap, total, " et ", 4u);
        } else {
            nts_la_put_space(dst, dst_cap, total);
        }
        nts_la_format_cardinal_lt1000(dst, dst_cap, total, rem, gender, flags & ~NTS_LA_FLAG_ET);
    }
}

static void nts_la_format_cardinal_recursive(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    uint64_t    value,
    unsigned    gender,
    unsigned    flags)
{
    uint64_t high;
    unsigned low;

    if (value < 1000u) {
        nts_la_format_cardinal_lt1000(dst, dst_cap, total, (unsigned)value, gender, flags);
        return;
    }

    high = value / 1000u;
    low = (unsigned)(value % 1000u);

    if (high == 1u) {
        nts_la_put_mem(dst, dst_cap, total, "mille", 5u);
    } else {
        nts_la_format_cardinal_recursive(dst, dst_cap, total, high, NTS_LA_GENDER_NEUTER, flags);
        nts_la_put_mem(dst, dst_cap, total, " milia", 6u);
    }

    if (low != 0u) {
        if ((flags & NTS_LA_FLAG_ET) && low < 10u) {
            nts_la_put_mem(dst, dst_cap, total, " et ", 4u);
        } else {
            nts_la_put_connector(dst, dst_cap, total, flags);
        }
        nts_la_format_cardinal_lt1000(dst, dst_cap, total, low, gender, flags & ~NTS_LA_FLAG_ET);
    }
}

static void nts_la_format_adverb_lt1000(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned hundreds;
    unsigned rem;
    unsigned tens;
    unsigned ones;

    if (value == 0u) {
        return;
    }

    if (value < 10u) {
        nts_la_put_word(dst, dst_cap, total, g_adverb_units[value]);
        return;
    }

    if (value < 20u) {
        nts_la_put_word(dst, dst_cap, total, g_adverb_teens[value - 10u]);
        return;
    }

    if (value < 100u) {
        tens = value / 10u;
        ones = value % 10u;

        nts_la_put_word(dst, dst_cap, total, g_adverb_tens[tens]);
        if (ones != 0u) {
            nts_la_put_space(dst, dst_cap, total);
            nts_la_put_word(dst, dst_cap, total, g_adverb_units[ones]);
        }
        return;
    }

    hundreds = value / 100u;
    rem = value % 100u;

    nts_la_put_word(dst, dst_cap, total, g_adverb_hundreds[hundreds]);
    if (rem != 0u) {
        nts_la_put_space(dst, dst_cap, total);
        nts_la_format_adverb_lt1000(dst, dst_cap, total, rem);
    }
}

static void nts_la_format_adverb_recursive(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    uint64_t    value)
{
    uint64_t high;
    unsigned low;

    if (value == 0u) {
        return;
    }

    if (value < 1000u) {
        nts_la_format_adverb_lt1000(dst, dst_cap, total, (unsigned)value);
        return;
    }

    high = value / 1000u;
    low = (unsigned)(value % 1000u);

    if (high == 1u) {
        nts_la_put_mem(dst, dst_cap, total, "millies", 7u);
    } else {
        nts_la_format_adverb_recursive(dst, dst_cap, total, high);
        nts_la_put_mem(dst, dst_cap, total, " millies", 8u);
    }

    if (low != 0u) {
        nts_la_put_space(dst, dst_cap, total);
        nts_la_format_adverb_lt1000(dst, dst_cap, total, low);
    }
}

static void nts_la_format_ordinal_lt1000(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned hundreds;
    unsigned rem;
    unsigned tens;
    unsigned ones;

    if (value == 0u) {
        nts_la_put_word(dst, dst_cap, total, g_unit_ordinals[0]);
        return;
    }

    if (value < 10u) {
        nts_la_put_word(dst, dst_cap, total, g_unit_ordinals[value]);
        return;
    }

    if (value < 20u) {
        nts_la_put_word(dst, dst_cap, total, g_teen_ordinals[value - 10u]);
        return;
    }

    if (value < 100u) {
        tens = value / 10u;
        ones = value % 10u;

        nts_la_put_word(dst, dst_cap, total, g_tens_ordinals[tens]);
        if (ones != 0u) {
            nts_la_put_space(dst, dst_cap, total);
            nts_la_put_word(dst, dst_cap, total, g_unit_ordinals[ones]);
        }
        return;
    }

    hundreds = value / 100u;
    rem = value % 100u;

    nts_la_put_word(dst, dst_cap, total, g_hundred_ordinals[hundreds]);
    if (rem != 0u) {
        nts_la_put_space(dst, dst_cap, total);
        nts_la_format_ordinal_lt1000(dst, dst_cap, total, rem);
    }
}

static void nts_la_format_ordinal_recursive(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    uint64_t    value)
{
    uint64_t thousands;
    unsigned rem;

    if (value < 1000u) {
        nts_la_format_ordinal_lt1000(dst, dst_cap, total, (unsigned)value);
        return;
    }

    thousands = value / 1000u;
    rem = (unsigned)(value % 1000u);

    if (thousands == 1u) {
        nts_la_put_mem(dst, dst_cap, total, "millesimus", 10u);
    } else {
        nts_la_format_adverb_recursive(dst, dst_cap, total, thousands);
        nts_la_put_mem(dst, dst_cap, total, " millesimus", 11u);
    }

    if (rem != 0u) {
        nts_la_put_space(dst, dst_cap, total);
        nts_la_format_ordinal_lt1000(dst, dst_cap, total, rem);
    }
}

size_t nts_la_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag = nts_la_abs_i64(value);

    if (value < 0) {
        nts_la_put_mem(dst, dst_cap, &total, "minus ", 6u);
    }

    nts_la_format_cardinal_recursive(dst, dst_cap, &total, mag, NTS_LA_GENDER_MASC, flags);
    nts_la_term(dst, dst_cap, total);
    return total;
}

size_t nts_la_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag = nts_la_abs_i64(value);
    (void)flags;

    if (value < 0) {
        nts_la_put_mem(dst, dst_cap, &total, "minus ", 6u);
    }

    nts_la_format_ordinal_recursive(dst, dst_cap, &total, mag);
    nts_la_term(dst, dst_cap, total);
    return total;
}

size_t nts_la_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    static const struct
    {
        uint64_t value;
        const char* numerals;
    } roman_map[] =
    {
        { UINT64_C(1000), "M" },
        { UINT64_C(900),  "CM" },
        { UINT64_C(500),  "D" },
        { UINT64_C(400),  "CD" },
        { UINT64_C(100),  "C" },
        { UINT64_C(90),   "XC" },
        { UINT64_C(50),   "L" },
        { UINT64_C(40),   "XL" },
        { UINT64_C(10),   "X" },
        { UINT64_C(9),    "IX" },
        { UINT64_C(5),    "V" },
        { UINT64_C(4),    "IV" },
        { UINT64_C(1),    "I" }
    };

    size_t total = 0u;
    uint64_t mag = nts_la_abs_i64(value);
    unsigned i;
    (void)flags;

    if (value < 0) {
        nts_la_put_char(dst, dst_cap, &total, '-');
    }

    if (mag == 0u) {
        nts_la_put_char(dst, dst_cap, &total, '0');
        nts_la_put_char(dst, dst_cap, &total, '.');
        nts_la_term(dst, dst_cap, total);
        return total;
    }

    for (i = 0u; i < (unsigned)(sizeof(roman_map) / sizeof(roman_map[0])); ++i) {
        size_t len = strlen(roman_map[i].numerals);
        while (mag >= roman_map[i].value) {
            nts_la_put_mem(dst, dst_cap, &total, roman_map[i].numerals, len);
            mag -= roman_map[i].value;
        }
    }

    nts_la_put_char(dst, dst_cap, &total, '.');
    nts_la_term(dst, dst_cap, total);
    return total;
}
