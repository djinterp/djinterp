/******************************************************************************
* djinterp [re_std]                               duration_compare_three_way.hpp
*
* the duration three-way comparison:
*   operator<=> for durations of mixed periods. C++20 only -- the
* spaceship operator is a language feature and has no back-port. On
* C++11 through C++17 the six explicit operators in duration_compare.hpp
* are the whole comparison surface.
*
*   THE COMPARISON IS DELEGATED TO THE REPRESENTATION:
*   Both operands convert to the common period, and the two counts are
* compared with <=>. The result type is therefore whatever the rep's own
* three-way comparison yields -- std::strong_ordering for the integral
* reps every predefined duration uses, and std::partial_ordering for a
* floating-point rep, correctly, since a NaN count is unordered.
*
*   NO synth-three-way FALLBACK:
*   The standard composes <=> from < and == for representations that lack
* their own. re_std omits that, as tuple and array already do: a rep
* without operator<=> produces a compile error at the decltype site
* rather than a silently different ordering. The constraint is expressed
* through the trailing return type, so it is a substitution failure and
* the overload simply drops out.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_compare_three_way.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_THREE_WAY_
#define DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_THREE_WAY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
//   required for the ordering types the builtin <=> yields.
#include <compare>

// djinterp
#include "./duration.hpp"
#include "./duration_common_type.hpp"
#include "../type_traits/common_type.hpp"


NS_RESTD

namespace chrono
{

    // operator<=>
    //   function: three-way comparison across periods. The result is the
    // representation's own ordering category.
    template<typename _Rep1, typename _Period1,
             typename _Rep2, typename _Period2>
    D_CONSTEXPR auto operator<=>(const duration<_Rep1, _Period1>& _lhs,
                                 const duration<_Rep2, _Period2>& _rhs)
        -> decltype(
            common_type< duration<_Rep1, _Period1>,
                         duration<_Rep2, _Period2> >::type::zero().count()
            <=>
            common_type< duration<_Rep1, _Period1>,
                         duration<_Rep2, _Period2> >::type::zero().count() )
    {
        typedef typename common_type< duration<_Rep1, _Period1>,
                                      duration<_Rep2, _Period2> >::type _CD;
        return _CD(_lhs).count() <=> _CD(_rhs).count();
    }

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_COMPARE_THREE_WAY_
