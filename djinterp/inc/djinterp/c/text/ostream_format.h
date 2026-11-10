/*******************************************************************************
* djinterp [c]                                                  ostream_format.h
*
*   Formatted output onto a stream: a double in the C locale's form whatever
* the program's locale, a double in a locale the caller names, and printf.
*
*   NUMBERS FOR MACHINES DO NOT FOLLOW THE LOCALE. printf's %f, %e and %g take
* their decimal point from LC_NUMERIC, so under a German locale 1.5 prints as
* "1,5" -- which is not a number in a PDF content stream, a CSS length or a
* JSON value, and a document that reads correctly on one machine is corrupt
* on the next. d_ostream_write_double writes the C form under every locale.
* Integers need nothing of the kind; ostream.h converts them itself.
*
*   A LOCALE PER CALL. Where the platform has locale objects -- POSIX 2008's
* newlocale and uselocale, or the Windows CRT's _create_locale and
* _snprintf_s_l -- a double is formatted in one for that call alone: the C
* locale for the C form, made once and shared, or a locale the caller made
* with d_ostream_locale_init for a double meant for people
* (d_ostream_write_double_l). The program's locale is never read or changed,
* so another thread's setlocale cannot reach the call. Elsewhere the stream
* formats in the program's locale and rewrites its decimal point, which may
* be more than one byte, to '.'; that C form races with setlocale, and a
* named locale cannot be made (d_ostream_locale_named says which). The
* mechanism is the knob D_CFG_OSTREAM_FORMAT_LOCALE's
* (config/c/text/cfg_ostream_format.h).
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
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FORMATTED OUTPUT
    ----------------
    1.  Doubles
    2.  printf
    3.  Locales
         1.  dj_ostream_locale
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


// 1.3    Locales
//------------------------------------------------------------------------------
// 1.3.1
// dj_ostream_locale
//   struct: a numeric locale a double is formatted in for one call. Made by
// d_ostream_locale_init and released by d_ostream_locale_close; the caller
// holds it, and djinterp allocates nothing (the platform's locale object is
// the platform's). Read-only once made, so threads may share one. A zeroed
// one, or one whose init failed, is no locale: a write with it is refused.
struct dj_ostream_locale
{
    void*   handle;  // the platform's locale object; NULL for the C form
    int32_t is_c;    // 1 for the C form, which needs no object of its own
};

/**
 * @brief Makes a numeric locale.
 *
 * @note Only LC_NUMERIC is taken from the named locale. "C" and "POSIX" give
 *       the C form under every mechanism; another name needs locale objects
 *       (d_ostream_locale_named).
 *
 * @param[out] _locale  receives the locale; on failure, no locale.
 * @param[in]  _name    a locale's name, as setlocale takes it
 *                      ("de_DE.UTF-8"); "" for the one the environment
 *                      names (LC_ALL, LC_NUMERIC, LANG).
 * @return 1 on success; 0 with errno set: EINVAL for a NULL argument, ENOENT
 *         for a locale the platform does not have, ENOTSUP for a named locale
 *         where the build has no locale objects, ENOMEM.
 */
int32_t d_ostream_locale_init(struct dj_ostream_locale* _locale,
                              const char*               _name);

/**
 * @brief Releases a locale's object, and leaves it no locale.
 *
 * @param[in,out] _locale  the locale; NULL, zeroed or released is fine.
 */
void d_ostream_locale_close(struct dj_ostream_locale* _locale);

/**
 * @brief Whether this build can make a named locale: whether
 *        D_CFG_OSTREAM_FORMAT_LOCALE resolved to locale objects.
 *
 * @return 1 when it can, 0 when only the C form is available.
 */
int32_t d_ostream_locale_named(void);

/**
 * @brief Writes a double in a locale: its decimal point, and nothing else of
 *        it (no grouping).
 *
 * @note Not-a-number writes as "nan" and the infinities as "inf" and "-inf",
 *       in every locale. The program's locale is neither read nor changed.
 *
 * @param[in,out] _os         the stream.
 * @param[in]     _value      the value.
 * @param[in]     _style      'g', 'f' or 'e', as printf's conversions.
 * @param[in]     _precision  as printf's precision; negative for its default
 *                            of 6.
 * @param[in]     _locale     the locale; NULL for the C form, as
 *                            d_ostream_write_double.
 * @return as d_ostream_write(); 0 as well for an unknown _style, or a
 *         _locale that is no locale, recorded as D_OSTREAM_INVALID.
 */
int32_t d_ostream_write_double_l(struct dj_ostream*              _os,
                                 double                          _value,
                                 char                            _style,
                                 int32_t                         _precision,
                                 const struct dj_ostream_locale* _locale);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEXT_OSTREAM_FORMAT_H
