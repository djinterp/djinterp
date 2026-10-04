/******************************************************************************
* djinterp [re_std]                                                  climits.hpp
*
* the integer limit macros:
*   CHAR_BIT, the SCHAR / UCHAR / CHAR / SHRT / INT / LONG / LLONG limits
* and MB_LEN_MAX. Every one is a MACRO, so none of them can live in
* re_std:: -- macros have no namespace. This header therefore does two
* things a bare `#include <climits>` does not:
*
*     1. It states, in one place, that the typed alternative exists and is
*        usually better -- see below.
*     2. It fills in the C++11 long long limits when the platform's own
*        <climits> predates them.
*
*   THE GAP-FILL IS #ifndef, NEVER AN OVERRIDE:
*   Every definition below is guarded on the macro being absent. re_std
*   does not second-guess a platform that has already answered; it only
*   answers where the platform stayed silent. A toolchain whose <climits>
*   is complete leaves this file with no effect at all.
*
*   The gap is real rather than hypothetical: LLONG_MIN, LLONG_MAX and
* ULLONG_MAX are C++11 additions, and compilers that support long long as
* an extension on an older library ship the type without the macros. The
* values are recovered from the compiler's own __LONG_LONG_MAX__ builtin
* -- the compiler is the authority on its own type's width, so this is a
* re-derivation, not a guess. Where even that builtin is absent, nothing
* is defined and the macros stay missing: an omission the user can test
* for with #ifdef, rather than a wrong value they cannot.
*
*   LLONG_MIN IS DERIVED AS -MAX - 1, NOT WRITTEN OUT:
*   Spelling the most-negative value as a literal does not work. The token
* -9223372036854775808 is a unary minus applied to a literal too large for
* a signed type, so it may not even be signed. -LLONG_MAX - 1 stays inside
* the representable range at every step and is correct on any two's
* complement target.
*
*   PREFER numeric_limits:
*   re_std::numeric_limits<T>::max() answers the same questions with a
* type, a namespace and constexpr, and it does not depend on the caller
* having included the right C header. These macros remain the right tool
* inside #if conditionals, where no function can be called.
*
*
* path:      /inc/djinterp/re_std/climits/climits.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CLIMITS_CLIMITS_
#define DJINTERP_RE_STD_CLIMITS_CLIMITS_ 1

// djinterp
#include "../../core/djinterp.hpp"

// std
//   the macros themselves. This is the whole payload of the header; there
// is no namespaced part to add.
#include <climits>


// ===========================================================================
// I.   C++11 LONG LONG LIMITS -- FILLED ONLY IF ABSENT
// ===========================================================================

#if !defined(LLONG_MAX) && defined(__LONG_LONG_MAX__)
    // LLONG_MAX
    //   constant: largest long long, recovered from the compiler builtin.
    #define LLONG_MAX   __LONG_LONG_MAX__
#endif

#if !defined(LLONG_MIN) && defined(LLONG_MAX)
    // LLONG_MIN
    //   constant: most-negative long long. Derived rather than written as
    // a literal -- see the header comment.
    #define LLONG_MIN   (-LLONG_MAX - 1)
#endif

#if !defined(ULLONG_MAX) && defined(LLONG_MAX)
    // ULLONG_MAX
    //   constant: largest unsigned long long. Doubling LLONG_MAX and
    // adding one is done in unsigned arithmetic (the cast comes first),
    // so it cannot overflow signed.
    #define ULLONG_MAX  ((unsigned long long)LLONG_MAX * 2ULL + 1ULL)
#endif


// ===========================================================================
// II.  MB_LEN_MAX -- FILLED ONLY IF ABSENT
// ===========================================================================

#ifndef MB_LEN_MAX
    // MB_LEN_MAX
    //   constant: longest multibyte character, in bytes, across all
    // supported locales. Reached only on a freestanding library that
    // omitted it. 16 is an over-estimate of every implementation the
    // authors know of (glibc 16, MSVC 5, musl 4), and over-estimating is
    // the safe direction here: the macro's job is to size a buffer, so a
    // value that is too large wastes stack while one that is too small
    // overflows it.
    #define MB_LEN_MAX  16
#endif


#endif  // DJINTERP_RE_STD_CLIMITS_CLIMITS_
