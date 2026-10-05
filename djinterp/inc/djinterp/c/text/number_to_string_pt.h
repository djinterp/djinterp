/*******************************************************************************
* djinterp [c]                                             number_to_string_pt.h
*
* TBA
*
*
* path:      /inc/djinterp/c/text/number_to_string_pt.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.03
*******************************************************************************/
#ifndef DJINTERP_C_TEXT_NUMBER_TO_STRING_PT_H
#define DJINTERP_C_TEXT_NUMBER_TO_STRING_PT_H 1

// std
#include <stddef.h>
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Flags controlling Portuguese formatting.
 *
 * This module uses Brazilian Portuguese spellings and large-number names
 * (milhão, bilhão, trilhão, ...).
 *
 * NTS_PT_FLAG_GROUP_DOTS:
 *   Insert '.' as the thousands separator in abbreviated numeric ordinals,
 *   e.g. 1234 -> 1.234.º
 */
enum
{
    NTS_PT_FLAG_GROUP_DOTS = 1u << 0,

    NTS_PT_FLAGS_DEFAULT = 0u
};

/*
 * A comfortably safe buffer size for any signed 64-bit Portuguese cardinal,
 * ordinal word, or abbreviated ordinal produced by this module.
 */
#define NTS_PT_I64_BUFFER_SIZE 768u

/*
 * Writes the Portuguese cardinal form of value into dst.
 * Returns the number of bytes that would have been written, excluding the
 * terminating '\0'. Pass dst = NULL and/or dst_cap = 0 to query length.
 */
size_t nts_pt_cardinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes the Portuguese ordinal word form of value into dst.
 * Masculine singular forms are used: primeiro, vigésimo primeiro,
 * milésimo, bilionésimo, etc.
 */
size_t nts_pt_ordinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes the abbreviated Portuguese ordinal form of value into dst.
 * Masculine singular is used: 1 -> 1.º, 21 -> 21.º.
 * NTS_PT_FLAG_GROUP_DOTS controls grouping in the numeric portion.
 */
size_t nts_pt_ordinal_abbrev_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

#ifdef __cplusplus
}
#endif

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_TEXT_NUMBER_TO_STRING_PT_H
