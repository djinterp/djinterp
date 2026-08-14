/***********************************************************************
* restd                                                          plus.hpp
*
* function object: addition (binary +).
*
*
* path:      /inc/djinterp/re_std/functional/plus.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_PLUS_
#define RESTD_FUNCTIONAL_PLUS_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "restd/utility/forward.hpp"
#endif

namespace restd
{

// plus
//   class: function object performing addition (binary +).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct plus
{
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    typedef _Type first_argument_type;
    typedef _Type second_argument_type;
    typedef _Type result_type;
#endif

    D_CONSTEXPR _Type
    operator()(
        const _Type& _x,
        const _Type& _y
    ) const
    {
        return _x + _y;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// plus<void>
//   class: transparent specialization; deduces operand types and
// forwards them through the operation.
template<>
struct plus<void>
{
    typedef int is_transparent;

    template<typename _T,
             typename _U>
    D_CONSTEXPR auto
    operator()(
        _T&& _x,
        _U&& _y
    ) const -> decltype(restd::forward<_T>(_x) + restd::forward<_U>(_y))
    {
        return restd::forward<_T>(_x) + restd::forward<_U>(_y);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace restd

#endif // RESTD_FUNCTIONAL_PLUS_
