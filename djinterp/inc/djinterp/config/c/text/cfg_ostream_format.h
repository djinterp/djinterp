/*******************************************************************************
* djinterp [config]                                         cfg_ostream_format.h
*
*   Configuration for formatted output onto a stream (c/text/ostream_format.h):
* how a double is formatted in a locale of the caller's choosing, without
* touching the program's locale.
*
*   targets:  c/text/ostream_format.c -> D_INTERNAL_OSTREAM_FORMAT_LOCALE
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*   A LOCALE PER CALL, WHERE THE PLATFORM HAS ONE. snprintf takes its decimal
* point from the locale, and setlocale changes the locale for every thread at
* once, so a double formatted while another thread calls setlocale can come
* out in either locale. A locale object used for one call -- POSIX 2008's
* newlocale and uselocale, or the Windows CRT's _create_locale and
* _snprintf_s_l -- is unaffected by setlocale, and also lets a caller write a
* double for people in a locale the program never set (stream 2). Where the
* platform has neither, the stream formats in the program's locale and
* rewrites its decimal point to '.', as it always has: the C form is still
* right, but races with setlocale, and a named locale cannot be made.
*
*   THE KNOB CHOOSES THE MECHANISM, NOT THE LOCALE. A double for machines is
* written in the C form under every setting (the decimal-point ruling, stream
* 1): a knob that changed it would break every PDF, JSON and CSV the build
* writes. The locale for people is chosen per call, at run time, with a
* dj_ostream_locale.
*
*   AUTO IS DECIDED BY WHAT ostream_format.c SEES. It asks for POSIX 2008
* itself, and takes POSIX where <locale.h> then declares newlocale (keyed on
* LC_NUMERIC_MASK, not on the _l functions, which musl lacks), WINDOWS under
* the Microsoft CRT, and REWRITE otherwise. A pinned mechanism the target
* cannot supply is a build error: a silent downgrade would ship a binary
* without the behaviour it was pinned for.
*
*
* path:      /inc/djinterp/config/c/text/cfg_ostream_format.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.09
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Locale mechanism
         1.  D_CFG_OSTREAM_FORMAT_LOCALE_AUTO
         2.  D_CFG_OSTREAM_FORMAT_LOCALE_REWRITE
         3.  D_CFG_OSTREAM_FORMAT_LOCALE_POSIX
         4.  D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS
         5.  D_CFG_OSTREAM_FORMAT_LOCALE
2.  VALIDATION
    ----------
    1.  Literals and ranges
*/

#ifndef DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_FORMAT_H
#define DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_FORMAT_H 1

// djinterp
#include "../../cfg_common.h"  // the root, first: overrides, D_CFG_NORM,
                               // D_CFG_IS_INT_LITERAL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Locale mechanism
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_OSTREAM_FORMAT_LOCALE_AUTO
//   constant: mechanism selector: the best the target has (the default).
#define D_CFG_OSTREAM_FORMAT_LOCALE_AUTO     0

// 1.1.2
// D_CFG_OSTREAM_FORMAT_LOCALE_REWRITE
//   constant: mechanism selector: no locale objects; the program's locale,
// its decimal point rewritten to '.'. Named locales cannot be made.
#define D_CFG_OSTREAM_FORMAT_LOCALE_REWRITE  1

// 1.1.3
// D_CFG_OSTREAM_FORMAT_LOCALE_POSIX
//   constant: mechanism selector: newlocale, uselocale and freelocale (POSIX
// 2008).
#define D_CFG_OSTREAM_FORMAT_LOCALE_POSIX    2

// 1.1.4
// D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS
//   constant: mechanism selector: _create_locale, _snprintf_s_l and
// _free_locale (the Microsoft CRT).
#define D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS  3

// 1.1.5
// D_CFG_OSTREAM_FORMAT_LOCALE
//   knob: how a double is formatted in a locale; one of the selectors above.
#ifndef D_CFG_OSTREAM_FORMAT_LOCALE
    #define D_CFG_OSTREAM_FORMAT_LOCALE D_CFG_OSTREAM_FORMAT_LOCALE_AUTO
#endif  // D_CFG_OSTREAM_FORMAT_LOCALE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Literals and ranges
//------------------------------------------------------------------------------
// Whether the target supplies a pinned mechanism is checked where it is used,
// in c/text/ostream_format.c, after <locale.h>.
#if !D_CFG_IS_INT_LITERAL(D_CFG_OSTREAM_FORMAT_LOCALE)
    #error "D_CFG_OSTREAM_FORMAT_LOCALE must name one of its values"
#endif

#if ( (D_CFG_NORM(D_CFG_OSTREAM_FORMAT_LOCALE) <                               \
       D_CFG_OSTREAM_FORMAT_LOCALE_AUTO) ||                                    \
      (D_CFG_NORM(D_CFG_OSTREAM_FORMAT_LOCALE) >                               \
       D_CFG_OSTREAM_FORMAT_LOCALE_WINDOWS) )
    #error "D_CFG_OSTREAM_FORMAT_LOCALE must be a locale mechanism selector"
#endif


#endif  // DJINTERP_CONFIG_C_TEXT_CFG_OSTREAM_FORMAT_H
