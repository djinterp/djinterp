/*******************************************************************************
* djinterp [c]                                             number_to_string_la.h
*
* TBA
*
*
* path:      /inc/djinterp/c/text/number_to_string_la.h
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.10.03
*******************************************************************************/
#ifndef DJINTERP_C_TEXT_NUMBER_TO_STRING_LA_H
#define DJINTERP_C_TEXT_NUMBER_TO_STRING_LA_H 1

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
 * Flags controlling Classical Latin formatting.
 *
 * NTS_LA_FLAG_COMMAS:
 *   Insert ", " between major recursive thousand-segments in cardinal output.
 *   Disabled by default because plain spacing is the more Latin-looking style.
 *
 * NTS_LA_FLAG_ET:
 *   Insert " et " in a few optional two-part compounds where Latin permits it,
 *   e.g. "viginti et unus" / "centum et unus". The formatter keeps the usual
 *   additive descending order rather than inverting to "unus et viginti".
 */
enum
{
    NTS_LA_FLAG_COMMAS = 1u << 0,
    NTS_LA_FLAG_ET     = 1u << 1,

    NTS_LA_FLAGS_DEFAULT = 0u
};

/*
 * A comfortably safe buffer size for any signed 64-bit Classical-Latin-style
 * cardinal, ordinal word, or abbreviated ordinal produced by this module.
 */
#define NTS_LA_I64_BUFFER_SIZE 768u

/*
 * Writes the Classical Latin cardinal form of value into dst.
 *
 * Notes:
 * - 0 is rendered as "nihil" as a practical modern convenience.
 * - For values beyond the native thousand-scale, the formatter stays within
 *   classical vocabulary by recursively stacking "mille/milia" segments rather
 *   than using post-classical million/billion words.
 *
 * Returns the number of bytes that would have been written, excluding the
 * terminating '\0'. Pass dst = NULL and/or dst_cap = 0 to query length.
 */
size_t nts_la_cardinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes the Classical Latin ordinal word form of value into dst.
 *
 * Masculine nominative singular forms are used: primus, vicesimus primus,
 * centesimus primus, millesimus, bis millesimus, etc.
 *
 * Returns the number of bytes that would have been written, excluding the
 * terminating '\0'. Pass dst = NULL and/or dst_cap = 0 to query length.
 */
size_t nts_la_ordinal_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

/*
 * Writes an abbreviated Classical-Latin-style ordinal into dst.
 *
 * This function uses Roman numerals followed by a period, e.g.:
 *   1  -> I.
 *   21 -> XXI.
 *
 * 0 falls back to "0." because Classical Latin has no native zero numeral.
 *
 * Returns the number of bytes that would have been written, excluding the
 * terminating '\0'. Pass dst = NULL and/or dst_cap = 0 to query length.
 */
size_t nts_la_ordinal_abbrev_i64(
    char*       dst,
    size_t      dst_cap,
    int64_t     value,
    unsigned    flags
);

#ifdef __cplusplus
}
#endif

#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_TEXT_NUMBER_TO_STRING_LA_H
