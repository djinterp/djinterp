/******************************************************************************
* djinterp [re_std]                                       duration_cast_impl.hpp
*
* the internal duration conversion arithmetic:
*   The four-way dispatch that duration_cast and duration's converting
* constructor both run on. Nothing here is a standard name.
*
*   THE CONVERSION IS ONE MULTIPLICATION AND ONE DIVISION:
*   Converting a duration<Rep, Period> to duration<ToRep, ToPeriod> means
* scaling the count by CF = Period / ToPeriod. Done naively that is
*
*       count * CF::num / CF::den
*
*   in a common arithmetic type wide enough to hold the intermediate.
*
*   WHY FOUR SPECIALISATIONS AND NOT ONE EXPRESSION:
*   Because in the overwhelmingly common cases CF::num or CF::den is 1,
* and multiplying or dividing by a literal 1 is not free -- not at
* run time on a rep whose operator* is a real function, and not at
* compile time either, where each operation is one more step against the
* constexpr evaluation limit. More importantly, the identity operations
* can OVERFLOW: promoting a seconds count to the common type with
* intmax_t just to multiply it by 1 can push a value out of range that
* the direct conversion would have carried through untouched. So each
* combination gets exactly the arithmetic it needs and no more:
*
*       num == 1, den == 1    convert the rep, no arithmetic
*       num == 1, den != 1    divide only
*       num != 1, den == 1    multiply only
*       otherwise             multiply then divide
*
*   THE COMMON TYPE INCLUDES intmax_t DELIBERATELY:
*   CF::num and CF::den are intmax_t, so the arithmetic type is
* common_type<ToRep, Rep, intmax_t>. Without intmax_t in that set, a
* conversion between two narrow integral reps would do its multiplication
* in a narrow type and overflow -- microseconds from a
* duration<short, ratio<3600>> being the easy example.
*
*   WHAT IS STILL NOT PROTECTED:
*   Overflow of the common type itself. duration_cast<nanoseconds> of a
* large hours value overflows int64 and the standard does not require a
* diagnostic. re_std does not add one, because the check would cost every
* conversion and would have to invent an error channel the interface has
* no room for. It is called out here so it is a known limitation rather
* than a surprise.
*
*
* path:      /inc/djinterp/re_std/chrono/duration_cast_impl.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CHRONO_DURATION_CAST_IMPL_
#define DJINTERP_RE_STD_CHRONO_DURATION_CAST_IMPL_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "./duration_fwd.hpp"


NS_RESTD

namespace chrono
{

NS_INTERNAL

    // duration_cast_helper
    //   struct: primary template. _CF is the conversion factor ratio,
    // _CR the arithmetic type. The two bools select the specialisation;
    // the primary is the general multiply-then-divide case.
    template<typename _ToDur,
             typename _CF,
             typename _CR,
             bool     _NumIsOne = false,
             bool     _DenIsOne = false>
    struct duration_cast_helper
    {
        template<typename _Rep,
                 typename _Period>
        static D_CONSTEXPR _ToDur cast(const duration<_Rep, _Period>& _d)
        {
            return _ToDur(static_cast<typename _ToDur::rep>(
                static_cast<_CR>(_d.count())
                    * static_cast<_CR>(_CF::num)
                    / static_cast<_CR>(_CF::den)));
        }
    };

    // duration_cast_helper<..., true, true>
    //   struct: periods are identical -- only the representation changes,
    // so no arithmetic runs at all.
    template<typename _ToDur,
             typename _CF,
             typename _CR>
    struct duration_cast_helper<_ToDur, _CF, _CR, true, true>
    {
        template<typename _Rep,
                 typename _Period>
        static D_CONSTEXPR _ToDur cast(const duration<_Rep, _Period>& _d)
        {
            return _ToDur(static_cast<typename _ToDur::rep>(_d.count()));
        }
    };

    // duration_cast_helper<..., true, false>
    //   struct: numerator is 1 -- divide only. This is the coarsening
    // direction (milliseconds to seconds), where integral reps truncate
    // toward zero.
    template<typename _ToDur,
             typename _CF,
             typename _CR>
    struct duration_cast_helper<_ToDur, _CF, _CR, true, false>
    {
        template<typename _Rep,
                 typename _Period>
        static D_CONSTEXPR _ToDur cast(const duration<_Rep, _Period>& _d)
        {
            return _ToDur(static_cast<typename _ToDur::rep>(
                static_cast<_CR>(_d.count()) / static_cast<_CR>(_CF::den)));
        }
    };

    // duration_cast_helper<..., false, true>
    //   struct: denominator is 1 -- multiply only. The refining direction
    // (seconds to milliseconds), which is exact and therefore the case
    // duration's converting constructor allows implicitly.
    template<typename _ToDur,
             typename _CF,
             typename _CR>
    struct duration_cast_helper<_ToDur, _CF, _CR, false, true>
    {
        template<typename _Rep,
                 typename _Period>
        static D_CONSTEXPR _ToDur cast(const duration<_Rep, _Period>& _d)
        {
            return _ToDur(static_cast<typename _ToDur::rep>(
                static_cast<_CR>(_d.count()) * static_cast<_CR>(_CF::num)));
        }
    };

NS_END  // internal

}  // namespace chrono

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CHRONO_DURATION_CAST_IMPL_
