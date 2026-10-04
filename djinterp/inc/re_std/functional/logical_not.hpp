/***********************************************************************
* re_std                                                  logical_not.hpp
*
* function object: logical negation (!).
*
*
* path:      /inc/djinterp/re_std/functional/logical_not.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_LOGICAL_NOT_
#define DJINTERP_RE_STD_FUNCTIONAL_LOGICAL_NOT_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// logical_not
//   class: function object performing logical negation (!).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct logical_not
{
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    typedef _Type argument_type;
    typedef bool  result_type;
#endif

    D_CONSTEXPR bool
    operator()(
        const _Type& _x
    ) const
    {
        return !_x;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// logical_not<void>
//   class: transparent specialization; deduces the operand type and
// forwards it through the operation.
template<>
struct logical_not<void>
{
    typedef int is_transparent;

    template<typename _T>
    D_CONSTEXPR auto
    operator()(
        _T&& _x
    ) const -> decltype(!re_std::forward<_T>(_x))
    {
        return !re_std::forward<_T>(_x);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_LOGICAL_NOT_
