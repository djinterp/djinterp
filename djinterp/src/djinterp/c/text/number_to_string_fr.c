/*******************************************************************************
* djinterp [c]                                             number_to_string_fr.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_fr.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_fr.h"

// std
#include <string.h>

typedef struct nts_fr_word
{
    const char* s;
    unsigned char len;
} nts_fr_word;

#define NTS_FR_WORD(lit) { lit, (unsigned char)(sizeof(lit) - 1u) }

static const nts_fr_word g_units[] =
{
    NTS_FR_WORD("zéro"),
    NTS_FR_WORD("un"),
    NTS_FR_WORD("deux"),
    NTS_FR_WORD("trois"),
    NTS_FR_WORD("quatre"),
    NTS_FR_WORD("cinq"),
    NTS_FR_WORD("six"),
    NTS_FR_WORD("sept"),
    NTS_FR_WORD("huit"),
    NTS_FR_WORD("neuf")
};

static const nts_fr_word g_cardinal_0_16[] =
{
    NTS_FR_WORD("zéro"),
    NTS_FR_WORD("un"),
    NTS_FR_WORD("deux"),
    NTS_FR_WORD("trois"),
    NTS_FR_WORD("quatre"),
    NTS_FR_WORD("cinq"),
    NTS_FR_WORD("six"),
    NTS_FR_WORD("sept"),
    NTS_FR_WORD("huit"),
    NTS_FR_WORD("neuf"),
    NTS_FR_WORD("dix"),
    NTS_FR_WORD("onze"),
    NTS_FR_WORD("douze"),
    NTS_FR_WORD("treize"),
    NTS_FR_WORD("quatorze"),
    NTS_FR_WORD("quinze"),
    NTS_FR_WORD("seize")
};

static const nts_fr_word g_ordinal_0_16[] =
{
    NTS_FR_WORD("zéroième"),
    NTS_FR_WORD("unième"),
    NTS_FR_WORD("deuxième"),
    NTS_FR_WORD("troisième"),
    NTS_FR_WORD("quatrième"),
    NTS_FR_WORD("cinquième"),
    NTS_FR_WORD("sixième"),
    NTS_FR_WORD("septième"),
    NTS_FR_WORD("huitième"),
    NTS_FR_WORD("neuvième"),
    NTS_FR_WORD("dixième"),
    NTS_FR_WORD("onzième"),
    NTS_FR_WORD("douzième"),
    NTS_FR_WORD("treizième"),
    NTS_FR_WORD("quatorzième"),
    NTS_FR_WORD("quinzième"),
    NTS_FR_WORD("seizième")
};

static const nts_fr_word g_tens[] =
{
    NTS_FR_WORD(""),
    NTS_FR_WORD(""),
    NTS_FR_WORD("vingt"),
    NTS_FR_WORD("trente"),
    NTS_FR_WORD("quarante"),
    NTS_FR_WORD("cinquante"),
    NTS_FR_WORD("soixante")
};

static const nts_fr_word g_tens_ordinals[] =
{
    NTS_FR_WORD(""),
    NTS_FR_WORD(""),
    NTS_FR_WORD("vingtième"),
    NTS_FR_WORD("trentième"),
    NTS_FR_WORD("quarantième"),
    NTS_FR_WORD("cinquantième"),
    NTS_FR_WORD("soixantième")
};

static const nts_fr_word g_magnitudes[] =
{
    NTS_FR_WORD(""),
    NTS_FR_WORD("mille"),
    NTS_FR_WORD("million"),
    NTS_FR_WORD("milliard"),
    NTS_FR_WORD("billion"),
    NTS_FR_WORD("billiard"),
    NTS_FR_WORD("trillion")
};

static const nts_fr_word g_magnitude_ordinals[] =
{
    NTS_FR_WORD(""),
    NTS_FR_WORD("millième"),
    NTS_FR_WORD("millionième"),
    NTS_FR_WORD("milliardième"),
    NTS_FR_WORD("billionième"),
    NTS_FR_WORD("billiardième"),
    NTS_FR_WORD("trillionième")
};

static void nts_fr_term(char* dst, size_t dst_cap, size_t total)
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

static void nts_fr_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
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

static void nts_fr_put_word(char* dst, size_t dst_cap, size_t* total, nts_fr_word w)
{
    nts_fr_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_fr_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_fr_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_fr_put_char(dst, dst_cap, total, ' ');
}

static void nts_fr_put_joiner(char* dst, size_t dst_cap, size_t* total, unsigned flags)
{
    nts_fr_put_char(dst, dst_cap, total, (flags & NTS_FR_FLAG_HYPHENS) ? '-' : ' ');
}

static void nts_fr_put_et(char* dst, size_t dst_cap, size_t* total)
{
    nts_fr_put_mem(dst, dst_cap, total, " et ", 4u);
}

static void nts_fr_put_group_connector(char* dst, size_t dst_cap, size_t* total, unsigned flags)
{
    if (flags & NTS_FR_FLAG_COMMAS) {
        nts_fr_put_mem(dst, dst_cap, total, ", ", 2u);
        return;
    }

    nts_fr_put_space(dst, dst_cap, total);
}

static void nts_fr_put_quatre_vingt_base(char* dst, size_t dst_cap, size_t* total, unsigned flags)
{
    nts_fr_put_word(dst, dst_cap, total, g_units[4]);
    nts_fr_put_joiner(dst, dst_cap, total, flags);
    nts_fr_put_mem(dst, dst_cap, total, "vingt", 5u);
}

static uint64_t nts_fr_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static void nts_fr_parse_sub100_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned tens;
    unsigned ones;

    if (value <= 16u) {
        nts_fr_put_word(dst, dst_cap, total, g_cardinal_0_16[value]);
        return;
    }

    if (value < 20u) {
        nts_fr_put_word(dst, dst_cap, total, g_cardinal_0_16[10u]);
        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_put_word(dst, dst_cap, total, g_units[value - 10u]);
        return;
    }

    if (value < 70u) {
        tens = value / 10u;
        ones = value % 10u;

        nts_fr_put_word(dst, dst_cap, total, g_tens[tens]);
        if (ones == 0u) {
            return;
        }

        if (ones == 1u) {
            nts_fr_put_et(dst, dst_cap, total);
            nts_fr_put_word(dst, dst_cap, total, g_units[1]);
            return;
        }

        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_put_word(dst, dst_cap, total, g_units[ones]);
        return;
    }

    if (value < 80u) {
        nts_fr_put_word(dst, dst_cap, total, g_tens[6]);
        if (value == 71u) {
            nts_fr_put_et(dst, dst_cap, total);
            nts_fr_put_word(dst, dst_cap, total, g_cardinal_0_16[11]);
            return;
        }

        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_parse_sub100_cardinal(dst, dst_cap, total, value - 60u, flags);
        return;
    }

    nts_fr_put_quatre_vingt_base(dst, dst_cap, total, flags);
    if (value == 80u) {
        nts_fr_put_char(dst, dst_cap, total, 's');
        return;
    }

    nts_fr_put_joiner(dst, dst_cap, total, flags);
    nts_fr_parse_sub100_cardinal(dst, dst_cap, total, value - 80u, flags);
}

static void nts_fr_parse_sub100_ordinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned tens;
    unsigned ones;

    if (value <= 16u) {
        nts_fr_put_word(dst, dst_cap, total, g_ordinal_0_16[value]);
        return;
    }

    if (value < 20u) {
        nts_fr_put_word(dst, dst_cap, total, g_cardinal_0_16[10u]);
        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_put_word(dst, dst_cap, total, g_ordinal_0_16[value - 10u]);
        return;
    }

    if (value < 70u) {
        tens = value / 10u;
        ones = value % 10u;

        if (ones == 0u) {
            nts_fr_put_word(dst, dst_cap, total, g_tens_ordinals[tens]);
            return;
        }

        nts_fr_put_word(dst, dst_cap, total, g_tens[tens]);
        if (ones == 1u) {
            nts_fr_put_et(dst, dst_cap, total);
            nts_fr_put_mem(dst, dst_cap, total, "unième", 8u);
            return;
        }

        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_put_word(dst, dst_cap, total, g_ordinal_0_16[ones]);
        return;
    }

    if (value < 80u) {
        if (value == 70u) {
            nts_fr_put_word(dst, dst_cap, total, g_tens[6]);
            nts_fr_put_joiner(dst, dst_cap, total, flags);
            nts_fr_put_word(dst, dst_cap, total, g_ordinal_0_16[10]);
            return;
        }

        nts_fr_put_word(dst, dst_cap, total, g_tens[6]);
        if (value == 71u) {
            nts_fr_put_et(dst, dst_cap, total);
            nts_fr_put_word(dst, dst_cap, total, g_ordinal_0_16[11]);
            return;
        }

        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_parse_sub100_ordinal(dst, dst_cap, total, value - 60u, flags);
        return;
    }

    if (value == 80u) {
        nts_fr_put_word(dst, dst_cap, total, g_units[4]);
        nts_fr_put_joiner(dst, dst_cap, total, flags);
        nts_fr_put_mem(dst, dst_cap, total, "vingtième", 10u);
        return;
    }

    nts_fr_put_quatre_vingt_base(dst, dst_cap, total, flags);
    nts_fr_put_joiner(dst, dst_cap, total, flags);
    nts_fr_parse_sub100_ordinal(dst, dst_cap, total, value - 80u, flags);
}

static void nts_fr_parse_triplet_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned hundreds;
    unsigned rem;

    if (value == 0u) {
        return;
    }

    if (value < 100u) {
        nts_fr_parse_sub100_cardinal(dst, dst_cap, total, value, flags);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    if (hundreds == 1u) {
        nts_fr_put_mem(dst, dst_cap, total, "cent", 4u);
    } else {
        nts_fr_put_word(dst, dst_cap, total, g_units[hundreds]);
        nts_fr_put_space(dst, dst_cap, total);
        nts_fr_put_mem(dst, dst_cap, total, "cent", 4u);
    }

    if (rem == 0u) {
        if (hundreds > 1u) {
            nts_fr_put_char(dst, dst_cap, total, 's');
        }
        return;
    }

    nts_fr_put_space(dst, dst_cap, total);
    nts_fr_parse_sub100_cardinal(dst, dst_cap, total, rem, flags);
}

static void nts_fr_parse_triplet_ordinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    flags)
{
    unsigned hundreds;
    unsigned rem;

    if (value < 100u) {
        nts_fr_parse_sub100_ordinal(dst, dst_cap, total, value, flags);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    if (rem == 0u) {
        if (hundreds == 1u) {
            nts_fr_put_mem(dst, dst_cap, total, "centième", 9u);
        } else {
            nts_fr_put_word(dst, dst_cap, total, g_units[hundreds]);
            nts_fr_put_space(dst, dst_cap, total);
            nts_fr_put_mem(dst, dst_cap, total, "centième", 9u);
        }
        return;
    }

    if (hundreds == 1u) {
        nts_fr_put_mem(dst, dst_cap, total, "cent", 4u);
    } else {
        nts_fr_put_word(dst, dst_cap, total, g_units[hundreds]);
        nts_fr_put_space(dst, dst_cap, total);
        nts_fr_put_mem(dst, dst_cap, total, "cent", 4u);
    }

    nts_fr_put_space(dst, dst_cap, total);
    nts_fr_parse_sub100_ordinal(dst, dst_cap, total, rem, flags);
}

size_t nts_fr_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;
    unsigned triplets[7];
    int hi = 0;
    int i;
    int wrote = 0;

    if (value == 0) {
        nts_fr_put_word(dst, dst_cap, &total, g_units[0]);
        nts_fr_term(dst, dst_cap, total);
        return total;
    }

    mag = nts_fr_abs_i64(value);
    if (value < 0) {
        nts_fr_put_mem(dst, dst_cap, &total, "moins ", 6u);
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

    for (i = hi; i >= 0; --i) {
        unsigned part = triplets[i];
        if (part == 0u) {
            continue;
        }

        if (wrote) {
            nts_fr_put_group_connector(dst, dst_cap, &total, flags);
        }

        if (i == 0) {
            nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
        } else if (i == 1) {
            if (part == 1u) {
                nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[1]);
            } else {
                nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                nts_fr_put_space(dst, dst_cap, &total);
                nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[1]);
            }
        } else {
            if (part == 1u) {
                nts_fr_put_word(dst, dst_cap, &total, g_units[1]);
                nts_fr_put_space(dst, dst_cap, &total);
                nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[i]);
            } else {
                nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                nts_fr_put_space(dst, dst_cap, &total);
                nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[i]);
                nts_fr_put_char(dst, dst_cap, &total, 's');
            }
        }

        wrote = 1;
    }

    nts_fr_term(dst, dst_cap, total);
    return total;
}

size_t nts_fr_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag;
    unsigned triplets[7];
    int hi = 0;
    int target = 0;
    int i;
    int wrote = 0;

    if (value == 0) {
        nts_fr_put_word(dst, dst_cap, &total, g_ordinal_0_16[0]);
        nts_fr_term(dst, dst_cap, total);
        return total;
    }

    if (value == 1) {
        nts_fr_put_mem(dst, dst_cap, &total, "premier", 7u);
        nts_fr_term(dst, dst_cap, total);
        return total;
    }

    if (value == -1) {
        nts_fr_put_mem(dst, dst_cap, &total, "moins premier", 13u);
        nts_fr_term(dst, dst_cap, total);
        return total;
    }

    mag = nts_fr_abs_i64(value);
    if (value < 0) {
        nts_fr_put_mem(dst, dst_cap, &total, "moins ", 6u);
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

    for (i = hi; i >= 0; --i) {
        unsigned part = triplets[i];
        if (part == 0u) {
            continue;
        }

        if (wrote) {
            nts_fr_put_group_connector(dst, dst_cap, &total, flags);
        }

        if (i == target) {
            if (i == 0) {
                nts_fr_parse_triplet_ordinal(dst, dst_cap, &total, part, flags);
            } else if (i == 1) {
                if (part == 1u) {
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitude_ordinals[1]);
                } else {
                    nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                    nts_fr_put_space(dst, dst_cap, &total);
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitude_ordinals[1]);
                }
            } else {
                if (part == 1u) {
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
                } else {
                    nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                    nts_fr_put_space(dst, dst_cap, &total);
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
                }
            }
        } else {
            if (i == 0) {
                nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
            } else if (i == 1) {
                if (part == 1u) {
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[1]);
                } else {
                    nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                    nts_fr_put_space(dst, dst_cap, &total);
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[1]);
                }
            } else {
                if (part == 1u) {
                    nts_fr_put_word(dst, dst_cap, &total, g_units[1]);
                    nts_fr_put_space(dst, dst_cap, &total);
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[i]);
                } else {
                    nts_fr_parse_triplet_cardinal(dst, dst_cap, &total, part, flags);
                    nts_fr_put_space(dst, dst_cap, &total);
                    nts_fr_put_word(dst, dst_cap, &total, g_magnitudes[i]);
                    nts_fr_put_char(dst, dst_cap, &total, 's');
                }
            }
        }

        wrote = 1;
    }

    nts_fr_term(dst, dst_cap, total);
    return total;
}

size_t nts_fr_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;
    uint64_t mag = nts_fr_abs_i64(value);
    char tmp[32];
    unsigned digits = 0u;
    unsigned i;

    if (value < 0) {
        nts_fr_put_char(dst, dst_cap, &total, '-');
    }

    do {
        tmp[digits++] = (char)('0' + (mag % 10u));
        mag /= 10u;
    } while (mag != 0u);

    for (i = digits; i-- > 0u;) {
        if ((flags & NTS_FR_FLAG_COMMAS) && i != (digits - 1u) && ((i + 1u) % 3u) == 0u) {
            nts_fr_put_char(dst, dst_cap, &total, ',');
        }
        nts_fr_put_char(dst, dst_cap, &total, tmp[i]);
    }

    if (nts_fr_abs_i64(value) == 1u) {
        nts_fr_put_mem(dst, dst_cap, &total, "er", 2u);
    } else {
        nts_fr_put_char(dst, dst_cap, &total, 'e');
    }

    nts_fr_term(dst, dst_cap, total);
    return total;
}
