/*******************************************************************************
* djinterp [c]                                             number_to_string_es.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_es.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_es.h"

// std
#include <string.h>

typedef struct nts_es_word
{
    const char* s;
    unsigned char len;
} nts_es_word;

#define NTS_ES_WORD(lit) { lit, (unsigned char)(sizeof(lit) - 1u) }

static const nts_es_word g_cardinal_0_29[] =
{
    NTS_ES_WORD("cero"),
    NTS_ES_WORD("uno"),
    NTS_ES_WORD("dos"),
    NTS_ES_WORD("tres"),
    NTS_ES_WORD("cuatro"),
    NTS_ES_WORD("cinco"),
    NTS_ES_WORD("seis"),
    NTS_ES_WORD("siete"),
    NTS_ES_WORD("ocho"),
    NTS_ES_WORD("nueve"),
    NTS_ES_WORD("diez"),
    NTS_ES_WORD("once"),
    NTS_ES_WORD("doce"),
    NTS_ES_WORD("trece"),
    NTS_ES_WORD("catorce"),
    NTS_ES_WORD("quince"),
    NTS_ES_WORD("dieciséis"),
    NTS_ES_WORD("diecisiete"),
    NTS_ES_WORD("dieciocho"),
    NTS_ES_WORD("diecinueve"),
    NTS_ES_WORD("veinte"),
    NTS_ES_WORD("veintiuno"),
    NTS_ES_WORD("veintidós"),
    NTS_ES_WORD("veintitrés"),
    NTS_ES_WORD("veinticuatro"),
    NTS_ES_WORD("veinticinco"),
    NTS_ES_WORD("veintiséis"),
    NTS_ES_WORD("veintisiete"),
    NTS_ES_WORD("veintiocho"),
    NTS_ES_WORD("veintinueve")
};

static const nts_es_word g_cardinal_0_29_apocopated[] =
{
    NTS_ES_WORD("cero"),
    NTS_ES_WORD("un"),
    NTS_ES_WORD("dos"),
    NTS_ES_WORD("tres"),
    NTS_ES_WORD("cuatro"),
    NTS_ES_WORD("cinco"),
    NTS_ES_WORD("seis"),
    NTS_ES_WORD("siete"),
    NTS_ES_WORD("ocho"),
    NTS_ES_WORD("nueve"),
    NTS_ES_WORD("diez"),
    NTS_ES_WORD("once"),
    NTS_ES_WORD("doce"),
    NTS_ES_WORD("trece"),
    NTS_ES_WORD("catorce"),
    NTS_ES_WORD("quince"),
    NTS_ES_WORD("dieciséis"),
    NTS_ES_WORD("diecisiete"),
    NTS_ES_WORD("dieciocho"),
    NTS_ES_WORD("diecinueve"),
    NTS_ES_WORD("veinte"),
    NTS_ES_WORD("veintiún"),
    NTS_ES_WORD("veintidós"),
    NTS_ES_WORD("veintitrés"),
    NTS_ES_WORD("veinticuatro"),
    NTS_ES_WORD("veinticinco"),
    NTS_ES_WORD("veintiséis"),
    NTS_ES_WORD("veintisiete"),
    NTS_ES_WORD("veintiocho"),
    NTS_ES_WORD("veintinueve")
};

static const nts_es_word g_tens[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD(""),
    NTS_ES_WORD("veinte"),
    NTS_ES_WORD("treinta"),
    NTS_ES_WORD("cuarenta"),
    NTS_ES_WORD("cincuenta"),
    NTS_ES_WORD("sesenta"),
    NTS_ES_WORD("setenta"),
    NTS_ES_WORD("ochenta"),
    NTS_ES_WORD("noventa")
};

static const nts_es_word g_hundreds[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD("ciento"),
    NTS_ES_WORD("doscientos"),
    NTS_ES_WORD("trescientos"),
    NTS_ES_WORD("cuatrocientos"),
    NTS_ES_WORD("quinientos"),
    NTS_ES_WORD("seiscientos"),
    NTS_ES_WORD("setecientos"),
    NTS_ES_WORD("ochocientos"),
    NTS_ES_WORD("novecientos")
};

static const nts_es_word g_ord_0_29[] =
{
    NTS_ES_WORD("cero"),
    NTS_ES_WORD("primero"),
    NTS_ES_WORD("segundo"),
    NTS_ES_WORD("tercero"),
    NTS_ES_WORD("cuarto"),
    NTS_ES_WORD("quinto"),
    NTS_ES_WORD("sexto"),
    NTS_ES_WORD("séptimo"),
    NTS_ES_WORD("octavo"),
    NTS_ES_WORD("noveno"),
    NTS_ES_WORD("décimo"),
    NTS_ES_WORD("undécimo"),
    NTS_ES_WORD("duodécimo"),
    NTS_ES_WORD("decimotercero"),
    NTS_ES_WORD("decimocuarto"),
    NTS_ES_WORD("decimoquinto"),
    NTS_ES_WORD("decimosexto"),
    NTS_ES_WORD("decimoséptimo"),
    NTS_ES_WORD("decimoctavo"),
    NTS_ES_WORD("decimonoveno"),
    NTS_ES_WORD("vigésimo"),
    NTS_ES_WORD("vigésimo primero"),
    NTS_ES_WORD("vigésimo segundo"),
    NTS_ES_WORD("vigésimo tercero"),
    NTS_ES_WORD("vigésimo cuarto"),
    NTS_ES_WORD("vigésimo quinto"),
    NTS_ES_WORD("vigésimo sexto"),
    NTS_ES_WORD("vigésimo séptimo"),
    NTS_ES_WORD("vigésimo octavo"),
    NTS_ES_WORD("vigésimo noveno")
};

static const nts_es_word g_tens_ordinals[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD(""),
    NTS_ES_WORD("vigésimo"),
    NTS_ES_WORD("trigésimo"),
    NTS_ES_WORD("cuadragésimo"),
    NTS_ES_WORD("quincuagésimo"),
    NTS_ES_WORD("sexagésimo"),
    NTS_ES_WORD("septuagésimo"),
    NTS_ES_WORD("octogésimo"),
    NTS_ES_WORD("nonagésimo")
};

static const nts_es_word g_hundreds_ordinals[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD("centésimo"),
    NTS_ES_WORD("ducentésimo"),
    NTS_ES_WORD("tricentésimo"),
    NTS_ES_WORD("cuadringentésimo"),
    NTS_ES_WORD("quingentésimo"),
    NTS_ES_WORD("sexcentésimo"),
    NTS_ES_WORD("septingentésimo"),
    NTS_ES_WORD("octingentésimo"),
    NTS_ES_WORD("noningentésimo")
};

static const nts_es_word g_magnitude_card_singular[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD("mil"),
    NTS_ES_WORD("millón"),
    NTS_ES_WORD("mil millones"),
    NTS_ES_WORD("billón"),
    NTS_ES_WORD("mil billones"),
    NTS_ES_WORD("trillón")
};

static const nts_es_word g_magnitude_card_plural[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD("mil"),
    NTS_ES_WORD("millones"),
    NTS_ES_WORD("mil millones"),
    NTS_ES_WORD("billones"),
    NTS_ES_WORD("mil billones"),
    NTS_ES_WORD("trillones")
};

static const nts_es_word g_magnitude_ordinals[] =
{
    NTS_ES_WORD(""),
    NTS_ES_WORD("milésimo"),
    NTS_ES_WORD("millonésimo"),
    NTS_ES_WORD("milmillonésimo"),
    NTS_ES_WORD("billonésimo"),
    NTS_ES_WORD("milbillonésimo"),
    NTS_ES_WORD("trillonésimo")
};

static void nts_es_term(char* dst, size_t dst_cap, size_t total)
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

static void nts_es_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
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

static void nts_es_put_word(char* dst, size_t dst_cap, size_t* total, nts_es_word w)
{
    nts_es_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_es_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_es_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_es_put_char(dst, dst_cap, total, ' ');
}

static uint64_t nts_es_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static int nts_es_triplet_is_univerbal(unsigned value)
{
    if (value == 0u) {
        return 0;
    }

    if (value <= 29u) {
        return 1;
    }

    if (value < 100u) {
        return (value % 10u) == 0u;
    }

    return (value % 100u) == 0u;
}

static void nts_es_parse_sub100_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    int         apocope)
{
    unsigned tens;
    unsigned ones;

    if (value <= 29u) {
        nts_es_put_word(dst, dst_cap, total, apocope ? g_cardinal_0_29_apocopated[value] : g_cardinal_0_29[value]);
        return;
    }

    tens = value / 10u;
    ones = value % 10u;

    nts_es_put_word(dst, dst_cap, total, g_tens[tens]);
    if (ones == 0u) {
        return;
    }

    nts_es_put_mem(dst, dst_cap, total, " y ", 3u);
    if (apocope && ones == 1u) {
        nts_es_put_word(dst, dst_cap, total, g_cardinal_0_29_apocopated[1]);
    } else {
        nts_es_put_word(dst, dst_cap, total, g_cardinal_0_29[ones]);
    }
}

static void nts_es_parse_triplet_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    int         apocope)
{
    unsigned hundreds;
    unsigned rem;

    if (value == 0u) {
        return;
    }

    if (value == 100u) {
        nts_es_put_mem(dst, dst_cap, total, "cien", 4u);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    if (hundreds != 0u) {
        nts_es_put_word(dst, dst_cap, total, g_hundreds[hundreds]);
        if (rem != 0u) {
            nts_es_put_space(dst, dst_cap, total);
        }
    }

    if (rem != 0u) {
        nts_es_parse_sub100_cardinal(dst, dst_cap, total, rem, apocope);
    }
}

static void nts_es_parse_sub100_ordinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned tens;
    unsigned ones;

    if (value <= 29u) {
        nts_es_put_word(dst, dst_cap, total, g_ord_0_29[value]);
        return;
    }

    tens = value / 10u;
    ones = value % 10u;

    nts_es_put_word(dst, dst_cap, total, g_tens_ordinals[tens]);
    if (ones == 0u) {
        return;
    }

    nts_es_put_space(dst, dst_cap, total);
    nts_es_put_word(dst, dst_cap, total, g_ord_0_29[ones]);
}

static void nts_es_parse_triplet_ordinal(
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
        nts_es_parse_sub100_ordinal(dst, dst_cap, total, value);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    nts_es_put_word(dst, dst_cap, total, g_hundreds_ordinals[hundreds]);
    if (rem == 0u) {
        return;
    }

    nts_es_put_space(dst, dst_cap, total);
    nts_es_parse_sub100_ordinal(dst, dst_cap, total, rem);
}

static void nts_es_put_group_separator(char* dst, size_t dst_cap, size_t* total)
{
    nts_es_put_space(dst, dst_cap, total);
}

static unsigned nts_es_split_groups(uint64_t value, unsigned groups[7])
{
    unsigned count = 0u;

    do {
        groups[count++] = (unsigned)(value % UINT64_C(1000));
        value /= UINT64_C(1000);
    } while (value != 0u && count < 7u);

    return count;
}

size_t nts_es_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    unsigned groups[7];
    unsigned count;
    unsigned i;
    size_t total = 0u;
    int first = 1;
    uint64_t mag;

    (void)flags;

    if (value < 0) {
        nts_es_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    mag = nts_es_abs_i64(value);
    if (mag == 0u) {
        nts_es_put_word(dst, dst_cap, &total, g_cardinal_0_29[0]);
        nts_es_term(dst, dst_cap, total);
        return total;
    }

    count = nts_es_split_groups(mag, groups);
    for (i = count; i-- > 0u;) {
        unsigned group = groups[i];

        if (group == 0u) {
            continue;
        }

        if (!first) {
            nts_es_put_group_separator(dst, dst_cap, &total);
        }
        first = 0;

        switch (i) {
            case 0u:
                nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 0);
                break;

            case 1u:
                if (group == 1u) {
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_singular[1]);
                } else {
                    nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 1);
                    nts_es_put_space(dst, dst_cap, &total);
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_plural[1]);
                }
                break;

            case 2u:
            case 4u:
            case 6u:
                if (group == 1u) {
                    nts_es_put_mem(dst, dst_cap, &total, "un ", 3u);
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_singular[i]);
                } else {
                    nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 1);
                    nts_es_put_space(dst, dst_cap, &total);
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_plural[i]);
                }
                break;

            case 3u:
            case 5u:
                if (group == 1u) {
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_singular[i]);
                } else {
                    nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 1);
                    nts_es_put_space(dst, dst_cap, &total);
                    nts_es_put_word(dst, dst_cap, &total, g_magnitude_card_plural[i]);
                }
                break;

            default:
                break;
        }
    }

    nts_es_term(dst, dst_cap, total);
    return total;
}

size_t nts_es_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    unsigned groups[7];
    unsigned count;
    unsigned i;
    size_t total = 0u;
    int first = 1;
    uint64_t mag;

    (void)flags;

    if (value < 0) {
        nts_es_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    mag = nts_es_abs_i64(value);
    if (mag == 0u) {
        nts_es_put_word(dst, dst_cap, &total, g_ord_0_29[0]);
        nts_es_term(dst, dst_cap, total);
        return total;
    }

    count = nts_es_split_groups(mag, groups);
    for (i = count; i-- > 0u;) {
        unsigned group = groups[i];

        if (group == 0u) {
            continue;
        }

        if (!first) {
            nts_es_put_space(dst, dst_cap, &total);
        }
        first = 0;

        if (i == 0u) {
            nts_es_parse_triplet_ordinal(dst, dst_cap, &total, group);
            continue;
        }

        if (group == 1u) {
            nts_es_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
            continue;
        }

        if (nts_es_triplet_is_univerbal(group)) {
            nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 1);
            nts_es_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
        } else {
            nts_es_parse_triplet_cardinal(dst, dst_cap, &total, group, 1);
            nts_es_put_space(dst, dst_cap, &total);
            nts_es_put_word(dst, dst_cap, &total, g_magnitude_ordinals[i]);
        }
    }

    nts_es_term(dst, dst_cap, total);
    return total;
}

size_t nts_es_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    char digits[32];
    size_t n = 0u;
    size_t total = 0u;
    uint64_t mag = nts_es_abs_i64(value);

    if (value < 0) {
        nts_es_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    do {
        digits[n++] = (char)('0' + (unsigned)(mag % 10u));
        mag /= 10u;
    } while (mag != 0u);

    while (n != 0u) {
        size_t idx = n - 1u;
        nts_es_put_char(dst, dst_cap, &total, digits[idx]);
        n = idx;

        if ((flags & NTS_ES_FLAG_GROUP_DOTS) && n != 0u && (n % 3u) == 0u) {
            nts_es_put_char(dst, dst_cap, &total, '.');
        }
    }

    nts_es_put_mem(dst, dst_cap, &total, ".º", sizeof(".º") - 1u);
    nts_es_term(dst, dst_cap, total);
    return total;
}
