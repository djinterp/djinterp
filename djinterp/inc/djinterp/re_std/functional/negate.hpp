/***********************************************************************
* restd                                                        negate.hpp
*
* function object: arithmetic negation (unary -).
*
*
* path:      /inc/restd/functional/negate.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_NEGATE_
#define RESTD_FUNCTIONAL_NEGATE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "restd/utility/forward.hpp"
#endif

namespace restd
{

// negate
//   class: function object performing arithmetic negation (unary -).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct negate
{
#if !D_ENV_LANG_IS_CPP20_OR_HIGHER
    typedef _Type argument_type;
    typedef _Type result_type;
#endif

    D_CONSTEXPR _Type
    operator()(
        const _Type& _x
    ) const
    {
        return -_x;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// negate<void>
//   class: transparent specialization; deduces the operand type and
// forwards it through the operation.
template<>
struct negate<void>
{
    typedef int is_transparent;

    template<typename _T>
    D_CONSTEXPR auto
    operator()(
        _T&& _x
    ) const -> decltype(-restd::forward<_T>(_x))
    {
        return -restd::forward<_T>(_x);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace restd

#endif // RESTD_FUNCTIONAL_NEGATE_
