/***********************************************************************
* re_std                                                         less.hpp
*
* function object: less-than test (<).
*
*
* path:      /inc/djinterp/re_std/functional/less.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_LESS_
#define DJINTERP_RE_STD_FUNCTIONAL_LESS_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// less
//   class: function object performing less-than test (<).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct less
{
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    typedef _Type first_argument_type;
    typedef _Type second_argument_type;
    typedef bool  result_type;
#endif

    D_CONSTEXPR bool
    operator()(
        const _Type& _x,
        const _Type& _y
    ) const
    {
        return _x < _y;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// less<void>
//   class: transparent specialization; deduces operand types and
// forwards them through the operation.
template<>
struct less<void>
{
    typedef int is_transparent;

    template<typename _T,
             typename _U>
    D_CONSTEXPR auto
    operator()(
        _T&& _x,
        _U&& _y
    ) const -> decltype(re_std::forward<_T>(_x) < re_std::forward<_U>(_y))
    {
        return re_std::forward<_T>(_x) < re_std::forward<_U>(_y);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_LESS_
