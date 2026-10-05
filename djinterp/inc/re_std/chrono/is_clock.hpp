/*******************************************************************************
* djinterp [re_std]                                                 is_clock.hpp
*
* the is_clock trait:
*   True if Type meets the Cpp17Clock requirements -- that is, if it has
* the five members every clock must have: rep, period, duration,
* time_point and a static now().
*
*   WHAT IT ACTUALLY CHECKS, AND WHAT IT CANNOT:
*   The detection is structural. It confirms the five names exist and
* that now() is callable; it cannot confirm that the epoch is sensible,
* that is_steady is truthful, or that duration is duration<rep, period>
* as the requirements demand. The standard's own wording has the same
* shape -- a trait cannot verify semantics -- so a type that satisfies
* is_clock is a plausible clock, not a proven one.
*
*   The check deliberately does NOT require is_steady. It is a Cpp17Clock
* requirement, but omitting it from the detection costs nothing and
* accepts the several real clock types in the wild that provide
* everything else.
*
*   BACK-PORTED FROM C++20 TO C++11:
*   std added is_clock in C++20. The implementation needs only void_t and
* partial specialisation, both C++11, so re_std provides it from C++11 --
* a nine-year lead. The _v spelling follows variable templates and so
* appears from C++14.
*
*   void_t IS AN ALIAS TEMPLATE AND HAS NO ::type -- it is written bare
* below. Spelling it `typename void_t<...>::type` is ill-formed, and is
* recorded as hard-won rule 9.
*
*
* path:      /inc/re_std/chrono/is_clock.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_IS_CLOCK_HPP
#define RE_STD_CHRONO_IS_CLOCK_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/true_type.hpp"
#include "../type_traits/false_type.hpp"
#include "../type_traits/void_t.hpp"


namespace re_std
{

namespace chrono
{

    // is_clock
    //   trait: true if Type has the five members a clock must have.
    // Primary template -- selected when the specialisation's
    // substitution fails.
    template<typename Type,
             typename = void>
    struct is_clock
        : false_type
    {};

    // is_clock<Type, void>
    //   trait: specialisation, selected when every required member name
    // exists and now() is callable.
    template<typename Type>
    struct is_clock<
        Type,
        void_t< typename Type::rep,
                typename Type::period,
                typename Type::duration,
                typename Type::time_point,
                decltype(Type::now()) > >
        : true_type
    {};

    // is_clock_v (C++14+)
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename Type>
    RE_STD_CONSTEXPR bool is_clock_v = is_clock<Type>::value;
#endif

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_IS_CLOCK_HPP
