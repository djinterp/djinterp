/*******************************************************************************
* djinterp [c]                                             number_to_string_pt.c
*
* TBA
*
*
* path:      /src/djinterp/c/text/number_to_string_pt.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.20
*******************************************************************************/
#include "../../../../inc/djinterp/c/text/number_to_string_pt.h"

// std
#include <string.h>

typedef struct nts_pt_word
{
    const char* s;
    unsigned short len;
} nts_pt_word;

#define NTS_PT_WORD(lit) { lit, (unsigned short)(sizeof(lit) - 1u) }

static const nts_pt_word g_card_0_19[] =
{
    NTS_PT_WORD("zero"),
    NTS_PT_WORD("um"),
    NTS_PT_WORD("dois"),
    NTS_PT_WORD("três"),
    NTS_PT_WORD("quatro"),
    NTS_PT_WORD("cinco"),
    NTS_PT_WORD("seis"),
    NTS_PT_WORD("sete"),
    NTS_PT_WORD("oito"),
    NTS_PT_WORD("nove"),
    NTS_PT_WORD("dez"),
    NTS_PT_WORD("onze"),
    NTS_PT_WORD("doze"),
    NTS_PT_WORD("treze"),
    NTS_PT_WORD("catorze"),
    NTS_PT_WORD("quinze"),
    NTS_PT_WORD("dezesseis"),
    NTS_PT_WORD("dezessete"),
    NTS_PT_WORD("dezoito"),
    NTS_PT_WORD("dezenove")
};

static const nts_pt_word g_tens[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD(""),
    NTS_PT_WORD("vinte"),
    NTS_PT_WORD("trinta"),
    NTS_PT_WORD("quarenta"),
    NTS_PT_WORD("cinquenta"),
    NTS_PT_WORD("sessenta"),
    NTS_PT_WORD("setenta"),
    NTS_PT_WORD("oitenta"),
    NTS_PT_WORD("noventa")
};

static const nts_pt_word g_hundreds[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD("cento"),
    NTS_PT_WORD("duzentos"),
    NTS_PT_WORD("trezentos"),
    NTS_PT_WORD("quatrocentos"),
    NTS_PT_WORD("quinhentos"),
    NTS_PT_WORD("seiscentos"),
    NTS_PT_WORD("setecentos"),
    NTS_PT_WORD("oitocentos"),
    NTS_PT_WORD("novecentos")
};

static const nts_pt_word g_ord_0_19[] =
{
    NTS_PT_WORD("zero"),
    NTS_PT_WORD("primeiro"),
    NTS_PT_WORD("segundo"),
    NTS_PT_WORD("terceiro"),
    NTS_PT_WORD("quarto"),
    NTS_PT_WORD("quinto"),
    NTS_PT_WORD("sexto"),
    NTS_PT_WORD("sétimo"),
    NTS_PT_WORD("oitavo"),
    NTS_PT_WORD("nono"),
    NTS_PT_WORD("décimo"),
    NTS_PT_WORD("décimo primeiro"),
    NTS_PT_WORD("décimo segundo"),
    NTS_PT_WORD("décimo terceiro"),
    NTS_PT_WORD("décimo quarto"),
    NTS_PT_WORD("décimo quinto"),
    NTS_PT_WORD("décimo sexto"),
    NTS_PT_WORD("décimo sétimo"),
    NTS_PT_WORD("décimo oitavo"),
    NTS_PT_WORD("décimo nono")
};

static const nts_pt_word g_tens_ord[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD(""),
    NTS_PT_WORD("vigésimo"),
    NTS_PT_WORD("trigésimo"),
    NTS_PT_WORD("quadragésimo"),
    NTS_PT_WORD("quinquagésimo"),
    NTS_PT_WORD("sexagésimo"),
    NTS_PT_WORD("septuagésimo"),
    NTS_PT_WORD("octogésimo"),
    NTS_PT_WORD("nonagésimo")
};

static const nts_pt_word g_hundreds_ord[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD("centésimo"),
    NTS_PT_WORD("ducentésimo"),
    NTS_PT_WORD("trecentésimo"),
    NTS_PT_WORD("quadringentésimo"),
    NTS_PT_WORD("quingentésimo"),
    NTS_PT_WORD("sexcentésimo"),
    NTS_PT_WORD("septingentésimo"),
    NTS_PT_WORD("octingentésimo"),
    NTS_PT_WORD("nongentésimo")
};

static const nts_pt_word g_mag_singular[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD("mil"),
    NTS_PT_WORD("milhão"),
    NTS_PT_WORD("bilhão"),
    NTS_PT_WORD("trilhão"),
    NTS_PT_WORD("quadrilhão"),
    NTS_PT_WORD("quintilhão")
};

static const nts_pt_word g_mag_plural[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD("mil"),
    NTS_PT_WORD("milhões"),
    NTS_PT_WORD("bilhões"),
    NTS_PT_WORD("trilhões"),
    NTS_PT_WORD("quadrilhões"),
    NTS_PT_WORD("quintilhões")
};

static const nts_pt_word g_mag_ord[] =
{
    NTS_PT_WORD(""),
    NTS_PT_WORD("milésimo"),
    NTS_PT_WORD("milionésimo"),
    NTS_PT_WORD("bilionésimo"),
    NTS_PT_WORD("trilionésimo"),
    NTS_PT_WORD("quadrilionésimo"),
    NTS_PT_WORD("quintilionésimo")
};

static void nts_pt_term(char* dst, size_t dst_cap, size_t total)
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

static void nts_pt_put_mem(char* dst, size_t dst_cap, size_t* total, const char* s, size_t n)
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

static void nts_pt_put_word(char* dst, size_t dst_cap, size_t* total, nts_pt_word w)
{
    nts_pt_put_mem(dst, dst_cap, total, w.s, w.len);
}

static void nts_pt_put_char(char* dst, size_t dst_cap, size_t* total, char c)
{
    size_t off = *total;

    if (dst != NULL && off < dst_cap) {
        dst[off] = c;
    }

    *total = off + 1u;
}

static void nts_pt_put_space(char* dst, size_t dst_cap, size_t* total)
{
    nts_pt_put_char(dst, dst_cap, total, ' ');
}

static uint64_t nts_pt_abs_i64(int64_t value)
{
    uint64_t u = (uint64_t)value;
    return (value < 0) ? (UINT64_C(0) - u) : u;
}

static unsigned nts_pt_split_groups(uint64_t value, unsigned groups[7])
{
    unsigned count = 0u;

    do {
        groups[count++] = (unsigned)(value % UINT64_C(1000));
        value /= UINT64_C(1000);
    } while (value != 0u && count < 7u);

    return count;
}

static void nts_pt_write_sub100_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned tens;
    unsigned ones;

    if (value < 20u) {
        nts_pt_put_word(dst, dst_cap, total, g_card_0_19[value]);
        return;
    }

    tens = value / 10u;
    ones = value % 10u;

    nts_pt_put_word(dst, dst_cap, total, g_tens[tens]);
    if (ones == 0u) {
        return;
    }

    nts_pt_put_mem(dst, dst_cap, total, " e ", 3u);
    nts_pt_put_word(dst, dst_cap, total, g_card_0_19[ones]);
}

static void nts_pt_write_triplet_cardinal(
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

    if (value == 100u) {
        nts_pt_put_mem(dst, dst_cap, total, "cem", 3u);
        return;
    }

    if (value < 100u) {
        nts_pt_write_sub100_cardinal(dst, dst_cap, total, value);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    nts_pt_put_word(dst, dst_cap, total, g_hundreds[hundreds]);
    if (rem != 0u) {
        nts_pt_put_mem(dst, dst_cap, total, " e ", 3u);
        nts_pt_write_sub100_cardinal(dst, dst_cap, total, rem);
    }
}

static void nts_pt_write_sub100_ordinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value)
{
    unsigned tens;
    unsigned ones;

    if (value < 20u) {
        nts_pt_put_word(dst, dst_cap, total, g_ord_0_19[value]);
        return;
    }

    tens = value / 10u;
    ones = value % 10u;

    nts_pt_put_word(dst, dst_cap, total, g_tens_ord[tens]);
    if (ones != 0u) {
        nts_pt_put_space(dst, dst_cap, total);
        nts_pt_put_word(dst, dst_cap, total, g_ord_0_19[ones]);
    }
}

static void nts_pt_write_triplet_ordinal(
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
        nts_pt_write_sub100_ordinal(dst, dst_cap, total, value);
        return;
    }

    hundreds = value / 100u;
    rem      = value % 100u;

    nts_pt_put_word(dst, dst_cap, total, g_hundreds_ord[hundreds]);
    if (rem != 0u) {
        nts_pt_put_space(dst, dst_cap, total);
        nts_pt_write_sub100_ordinal(dst, dst_cap, total, rem);
    }
}

static int nts_pt_has_lower_nonzero(const unsigned groups[7], unsigned idx)
{
    unsigned i;

    for (i = 0u; i < idx; ++i) {
        if (groups[i] != 0u) {
            return 1;
        }
    }

    return 0;
}

static int nts_pt_use_e_connector(const unsigned groups[7], unsigned idx)
{
    unsigned value = groups[idx];

    if (value == 0u) {
        return 0;
    }

    if (idx == 0u) {
        return (value < 100u) || ((value % 100u) == 0u);
    }

    if (!nts_pt_has_lower_nonzero(groups, idx)) {
        return (value < 100u) || ((value % 100u) == 0u);
    }

    return 0;
}

static void nts_pt_write_group_cardinal(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    idx)
{
    if (idx == 0u) {
        nts_pt_write_triplet_cardinal(dst, dst_cap, total, value);
        return;
    }

    if (idx == 1u) {
        if (value == 1u) {
            nts_pt_put_mem(dst, dst_cap, total, "mil", 3u);
        } else {
            nts_pt_write_triplet_cardinal(dst, dst_cap, total, value);
            nts_pt_put_mem(dst, dst_cap, total, " mil", 4u);
        }
        return;
    }

    if (value == 1u) {
        nts_pt_put_mem(dst, dst_cap, total, "um ", 3u);
        nts_pt_put_word(dst, dst_cap, total, g_mag_singular[idx]);
    } else {
        nts_pt_write_triplet_cardinal(dst, dst_cap, total, value);
        nts_pt_put_space(dst, dst_cap, total);
        nts_pt_put_word(dst, dst_cap, total, g_mag_plural[idx]);
    }
}

static void nts_pt_write_group_ordinal_mixed(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    idx)
{
    if (idx == 0u) {
        nts_pt_write_triplet_ordinal(dst, dst_cap, total, value);
        return;
    }

    if (value == 1u) {
        nts_pt_put_word(dst, dst_cap, total, g_mag_ord[idx]);
        return;
    }

    nts_pt_write_triplet_ordinal(dst, dst_cap, total, value);
    nts_pt_put_space(dst, dst_cap, total);
    nts_pt_put_word(dst, dst_cap, total, g_mag_ord[idx]);
}

static void nts_pt_write_group_ordinal_exact(
    char*       dst,
    size_t      dst_cap,
    size_t*     total,
    unsigned    value,
    unsigned    idx)
{
    if (idx == 0u) {
        nts_pt_write_triplet_ordinal(dst, dst_cap, total, value);
        return;
    }

    if (value == 1u) {
        nts_pt_put_word(dst, dst_cap, total, g_mag_ord[idx]);
        return;
    }

    nts_pt_write_triplet_cardinal(dst, dst_cap, total, value);
    nts_pt_put_space(dst, dst_cap, total);
    nts_pt_put_word(dst, dst_cap, total, g_mag_ord[idx]);
}

static size_t nts_pt_write_grouped_uint(
    char*       dst,
    size_t      dst_cap,
    uint64_t    value,
    int         group_dots)
{
    char rev[32];
    size_t n = 0u;
    size_t total = 0u;
    unsigned digits_in_group = 0u;

    if (value == 0u) {
        if (dst != NULL && dst_cap != 0u) {
            dst[0] = '0';
        }
        nts_pt_term(dst, dst_cap, 1u);
        return 1u;
    }

    while (value != 0u) {
        if (group_dots && digits_in_group == 3u) {
            rev[n++] = '.';
            digits_in_group = 0u;
        }

        rev[n++] = (char)('0' + (value % UINT64_C(10)));
        value /= UINT64_C(10);
        ++digits_in_group;
    }

    while (n != 0u) {
        nts_pt_put_char(dst, dst_cap, &total, rev[--n]);
    }

    nts_pt_term(dst, dst_cap, total);
    return total;
}

size_t nts_pt_cardinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    unsigned groups[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    unsigned count;
    unsigned idx;
    size_t total = 0u;
    int first = 1;

    (void)flags;

    if (value < 0) {
        nts_pt_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    count = nts_pt_split_groups(nts_pt_abs_i64(value), groups);

    if (count == 1u && groups[0] == 0u) {
        nts_pt_put_word(dst, dst_cap, &total, g_card_0_19[0]);
        nts_pt_term(dst, dst_cap, total);
        return total;
    }

    for (idx = count; idx-- != 0u;) {
        if (groups[idx] == 0u) {
            continue;
        }

        if (!first) {
            if (nts_pt_use_e_connector(groups, idx)) {
                nts_pt_put_mem(dst, dst_cap, &total, " e ", 3u);
            } else {
                nts_pt_put_space(dst, dst_cap, &total);
            }
        }

        nts_pt_write_group_cardinal(dst, dst_cap, &total, groups[idx], idx);
        first = 0;
    }

    nts_pt_term(dst, dst_cap, total);
    return total;
}

size_t nts_pt_ordinal_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    unsigned groups[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
    unsigned count;
    unsigned idx;
    unsigned nonzero = 0u;
    unsigned top_idx = 0u;
    size_t total = 0u;
    int first = 1;

    (void)flags;

    if (value < 0) {
        nts_pt_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    count = nts_pt_split_groups(nts_pt_abs_i64(value), groups);

    if (count == 1u && groups[0] == 0u) {
        nts_pt_put_word(dst, dst_cap, &total, g_ord_0_19[0]);
        nts_pt_term(dst, dst_cap, total);
        return total;
    }

    for (idx = count; idx-- != 0u;) {
        if (groups[idx] != 0u) {
            ++nonzero;
            if (idx > top_idx) {
                top_idx = idx;
            }
        }
    }

    if (nonzero == 1u) {
        nts_pt_write_group_ordinal_exact(dst, dst_cap, &total, groups[top_idx], top_idx);
        nts_pt_term(dst, dst_cap, total);
        return total;
    }

    for (idx = top_idx + 1u; idx-- != 0u;) {
        if (groups[idx] == 0u) {
            continue;
        }

        if (!first) {
            nts_pt_put_space(dst, dst_cap, &total);
        }

        nts_pt_write_group_ordinal_mixed(dst, dst_cap, &total, groups[idx], idx);
        first = 0;
    }

    nts_pt_term(dst, dst_cap, total);
    return total;
}

size_t nts_pt_ordinal_abbrev_i64(char* dst, size_t dst_cap, int64_t value, unsigned flags)
{
    size_t total = 0u;

    if (value < 0) {
        nts_pt_put_mem(dst, dst_cap, &total, "menos ", 6u);
    }

    total += nts_pt_write_grouped_uint(
        (dst != NULL) ? (dst + total) : NULL,
        (dst_cap > total) ? (dst_cap - total) : 0u,
        nts_pt_abs_i64(value),
        (flags & NTS_PT_FLAG_GROUP_DOTS) != 0u);

    nts_pt_put_mem(dst, dst_cap, &total, ".º", sizeof(".º") - 1u);
    nts_pt_term(dst, dst_cap, total);
    return total;
}
