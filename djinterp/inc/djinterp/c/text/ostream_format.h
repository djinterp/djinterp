/*******************************************************************************
* djinterp [c]                                                  ostream_format.h
*
*   Formatted output onto a stream: a double in the C locale's form whatever
* the program's locale, and printf.
*
*   NUMBERS FOR MACHINES DO NOT FOLLOW THE LOCALE. printf's %f, %e and %g take
* their decimal point from LC_NUMERIC, so under a German locale 1.5 prints as
* "1,5" -- which is not a number in a PDF content stream, a CSS length or a
* JSON value, and a document that reads correctly on one machine is corrupt
* on the next. d_ostream_write_double formats with snprintf, then rewrites
* the locale's decimal point, which may be more than one byte, to '.': the
* only part of the locale those conversions use. Integers need nothing of
* the kind; ostream.h converts them itself.
*   The rewrite reads localeconv(), so a thread calling setlocale meanwhile
* races with it, as it would with any reader of the C locale. A per-call
* locale (_snprintf_l, snprintf_l, uselocale) would remove that; it is not
* used yet.
*
*   PRINTF IS FOR PEOPLE, AND WRITES IN PLACE. d_ostream_printf follows the
* locale exactly as printf does. It formats straight into the stream's window
* when the window has room, so most calls copy nothing; otherwise into a stack
* buffer, and it allocates only for output larger than both. Its va_copy is
* C99's, which is why formatting lives on the C side: C++98 has neither
* va_copy nor std::vsnprintf, so a C++98 caller hands its va_list once to
* d_ostream_vprintf.
*
*
* path:      /inc/djinterp/c/text/ostream_format.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FORMATTED OUTPUT
    ----------------
    1.  Doubles
    2.  printf
*/

#ifndef DJINTERP_C_TEXT_OSTREAM_FORMAT_H
#define DJINTERP_C_TEXT_OSTREAM_FORMAT_H 1

// std
#include <stdarg.h>  // va_list
// djinterp
#include "../djinterp.h"              // framework root
#include "../util/ostream/ostream.h"  // struct dj_ostream
// re_std
#include "../../../re_std/c/cstdint/dstdint.h"  // int32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FORMATTED OUTPUT
//==============================================================================


// 1.1    Doubles
//------------------------------------------------------------------------------
/**
 * @brief Writes a double in the C locale's form, whatever the locale.
 *
 * @note Not-a-number writes as "nan" and the infinities as "inf" and "-inf",
 *       on every platform. A precision above 64 is taken as 64.
 *
 * @param[in,out] _os         the stream.
 * @param[in]     _value      the value.
 * @param[in]     _style      'g', 'f' or 'e', as printf's conversions.
 * @param[in]     _precision  as printf's precision; negative for its default
 *                            of 6.
 * @return as d_ostream_write(); 0 as well for an unknown _style, which is
 *         recorded as D_OSTREAM_INVALID.
 */
int32_t d_ostream_write_double(struct dj_ostream* _os,
                               double             _value,
                               char               _style,
                               int32_t            _precision);

// 1.2    printf
//------------------------------------------------------------------------------
/**
 * @brief Writes printf-formatted text.
 *
 * @note Follows the locale, as printf does. A formatting failure is recorded
 *       as D_OSTREAM_FAILED, with errno; so is an allocation that fails for
 *       output larger than the window and the stack buffer.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _format  the format.
 * @param[in]     ...      the values to format.
 * @return as d_ostream_write().
 */
int32_t d_ostream_printf(struct dj_ostream* _os,
                         const char*        _format,
                         ...);
/**
 * @brief Writes printf-formatted text from a va_list.
 *
 * @param[in,out] _os      the stream.
 * @param[in]     _format  the format.
 * @param[in]     _args    the values to format; used once.
 * @return as d_ostream_write().
 */
int32_t d_ostream_vprintf(struct dj_ostream* _os,
                          const char*        _format,
                          va_list            _args);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_OSTREAM_FORMAT_H
