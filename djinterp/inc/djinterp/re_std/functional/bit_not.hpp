/***********************************************************************
* restd                                                       bit_not.hpp
*
* function object: bitwise not (~).
*
*
* path:      /inc/restd/functional/bit_not.hpp
* link(s):   TBA
* author(s): restd                                       date: 2026.05.07
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_BIT_NOT_
#define RESTD_FUNCTIONAL_BIT_NOT_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "restd/utility/forward.hpp"
#endif

namespace restd
{

// bit_not
//   class: function object performing bitwise not (~).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct bit_not
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
        return ~_x;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// bit_not<void>
//   class: transparent specialization; deduces the operand type and
// forwards it through the operation.
template<>
struct bit_not<void>
{
    typedef int is_transparent;

    template<typename _T>
    D_CONSTEXPR auto
    operator()(
        _T&& _x
    ) const -> decltype(~restd::forward<_T>(_x))
    {
        return ~restd::forward<_T>(_x);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace restd

#endif // RESTD_FUNCTIONAL_BIT_NOT_
