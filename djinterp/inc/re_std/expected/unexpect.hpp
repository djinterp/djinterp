/******************************************************************************
* djinterp [re_std]                                                 unexpect.hpp
*
* unexpect tag header:
*   Provides unexpect_t and the unexpect constant — the tag-dispatch
* hook for expected<T, E>'s in-place error constructor:
*
*     expected<int, error_code> e(unexpect, error_code{EINVAL});
*
*   Distinct from in_place_t (which constructs the value), distinct
* from unexpected<E> (which is a typed wrapper). The trio
*   { in_place_t, unexpect_t, unexpected<E> }
* lets a user disambiguate every ctor of expected from every other
* without overload-set guesswork.
*
*   PORTABILITY:
*   Tag type is trivial — empty struct with an explicit default
* ctor and a single named instance. Works on every tier.
*
*
* path:      /inc/djinterp/re_std/expected/unexpect.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_UNEXPECT_
#define DJINTERP_RE_STD_UNEXPECT_ 1

#include "../../core/djinterp.hpp"

// gate: the entire <expected> module is C++11+. unexpect_t is the
// tag for expected's error-construction overload; it has no
// meaningful use without expected, so we gate consistently.
#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


// ===========================================================================
// I.   UNEXPECT_T
// ===========================================================================

// unexpect_t
//   tag: passed to expected's in-place error constructor to select
// the unexpected-construction overload.
struct unexpect_t
{
    // explicit default ctor — prevents accidental implicit construction
    // from {} in contexts where a default-constructible parameter is
    // also valid (matches std::in_place_t's defensive design). constexpr
    // so the type is a literal, eligible for inline constexpr instance.
    D_CONSTEXPR explicit unexpect_t() {}
};


// ===========================================================================
// II.  UNEXPECT CONSTANT
// ===========================================================================

// unexpect
//   constant: the singleton instance of unexpect_t. Usage:
//     expected<int, std::error_code> e(re_std::unexpect, ec);
//
// note: defined inline-static when C++17 inline-variables are available;
// otherwise a plain extern declaration paired with the definition the
// project's linkage convention emits (typically one TU defines it).
// For now: extern declaration only, definition expected in a paired
// .cpp file or via the inline-variable path below.
#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
inline D_CONSTEXPR unexpect_t unexpect{};
#else
// Pre-C++17: provide as a static-const-instance shim. Header-only;
// safe under ODR because unexpect_t is empty and the address-of is
// rarely taken.
namespace internal
{
    template<typename _Dummy>
    struct unexpect_holder
    {
        static const unexpect_t value;
    };
    template<typename _Dummy>
    const unexpect_t unexpect_holder<_Dummy>::value = unexpect_t();
}
static const unexpect_t& unexpect = internal::unexpect_holder<void>::value;
#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_UNEXPECT_
