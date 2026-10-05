/*******************************************************************************
* djinterp [re_std]                                          duration_values.hpp
*
* the duration_values trait:
*   Supplies the three special values -- zero, min and max -- for a
* representation type. duration::zero(), duration::min() and
* duration::max() are thin wrappers over these.
*
*   WHY THIS IS NOT JUST numeric_limits:
*   Two reasons, and the second is the one that matters.
*
*   First, min() here means the LOWEST value, not the smallest positive
* one. numeric_limits<double>::min() is a tiny positive number; a
* duration's min() must be the most negative representable duration, so
* this trait routes through numeric_limits<Rep>::lowest() instead. Using
* min() would silently make every floating-point duration's lower bound
* positive -- a bug that survives every integral test.
*
*   Second, a representation need not be a fundamental type at all.
* duration_values is the customisation point for a user-defined rep:
* specialise it, and duration's three observers follow, with no
* obligation to also specialise numeric_limits.
*
*   zero() IS Rep(0) AND NOT numeric_limits::zero:
*   The standard requires Rep(0) explicitly, and for a rep with an
* unusual identity that distinction is real.
*
*
* path:      /inc/re_std/chrono/duration_values.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_CHRONO_DURATION_VALUES_HPP
#define RE_STD_CHRONO_DURATION_VALUES_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../limits/numeric_limits.hpp"


namespace re_std
{

namespace chrono
{

    // duration_values
    //   trait: the zero, lowest and highest values of a duration
    // representation. Specialise for a user-defined rep.
    template<typename Rep>
    struct duration_values
    {
        // zero
        //   function: the additive identity, Rep(0).
        static RE_STD_CONSTEXPR Rep zero() RE_STD_NOEXCEPT
        {
            return Rep(0);
        }

        // min
        //   function: the LOWEST representable value -- lowest(), not
        // min(). See the header comment.
        static RE_STD_CONSTEXPR Rep min() RE_STD_NOEXCEPT
        {
            return numeric_limits<Rep>::lowest();
        }

        // max
        //   function: the highest representable value.
        static RE_STD_CONSTEXPR Rep max() RE_STD_NOEXCEPT
        {
            return numeric_limits<Rep>::max();
        }
    };

}  // namespace chrono

}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_CHRONO_DURATION_VALUES_HPP
