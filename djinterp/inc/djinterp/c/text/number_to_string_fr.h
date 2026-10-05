/*******************************************************************************
* djinterp [c]                                             number_to_string_fr.h
*
* TBA
*
*
* path:      /inc/djinterp/c/text/number_to_string_fr.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.03
*******************************************************************************/
#ifndef DJINTERP_C_TEXT_NUMBER_TO_STRING_FR_H
#define DJINTERP_C_TEXT_NUMBER_TO_STRING_FR_H 1

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
 * Flags controlling French formatting.
 *
 * NTS_FR_FLAG_HYPHENS:
 *   Use traditional internal hyphenation for compound forms such as
 *   "vingt-trois" and "quatre-vingt-un". The fixed "et" forms remain
 *   space-separated: "vingt et un", "soixante et onze".
 *
 * NTS_FR_FLAG_COMMAS:
 *   Insert ", " between major word segments and "," in abbreviated numeric
 *   ordinals. Disabled by default because standard French usually prefers
 *   simple spacing in these positions.
 */
enum
{
    NTS_FR_FLAG_HYPHENS = 1u << 0,
    NTS_FR_FLAG_COMMAS  = 1u << 1,

    NTS_FR_FLAGS_DEFAULT = NTS_FR_FLAG_HYPHENS
};

/*
 * A comfortably safe buffer size for any signed 64-bit French cardinal,
 * ordinal word, or abbreviated ordinal produced by this module.
 */
#define NTS_FR_I64_BUFFER_SIZE 384u

/*
 * Writes the French cardinal form of value into dst.
 * Returns the number of bytes that would have been written, excluding the
 * terminating '\0'. Pass dst = NULL and/or dst_cap = 0 to query length.
 */
size_t nts_fr_cardinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes the French ordinal word form of value into dst.
 * Masculine singular is used for 1: "premier".
 * Example: 21 -> "vingt et unième"
 */
size_t nts_fr_ordinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes the abbreviated French ordinal form of value into dst.
 * Masculine singular is used for 1: 1 -> "1er".
 * All other values use the common "e" suffix: 2 -> "2e", 21 -> "21e".
 */
size_t nts_fr_ordinal_abbrev_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

#ifdef __cplusplus
}
#endif

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_TEXT_NUMBER_TO_STRING_FR_H
