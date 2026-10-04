/***********************************************************************
* re_std                                                      bit_xor.hpp
*
* function object: bitwise xor (^).
*
*
* path:      /inc/djinterp/re_std/functional/bit_xor.hpp
* link(s):   TBA
* author(s): re_std                                      date: 2026.05.07
***********************************************************************/

#ifndef DJINTERP_RE_STD_FUNCTIONAL_BIT_XOR_
#define DJINTERP_RE_STD_FUNCTIONAL_BIT_XOR_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// bit_xor
//   class: function object performing bitwise xor (^).
template<typename _Type
#if D_ENV_LANG_IS_CPP14_OR_HIGHER
                     = void
#endif
        >
struct bit_xor
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
        return _x ^ _y;
    }
};

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// bit_xor<void>
//   class: transparent specialization; deduces operand types and
// forwards them through the operation.
template<>
struct bit_xor<void>
{
    typedef int is_transparent;

    template<typename _T,
             typename _U>
    D_CONSTEXPR auto
    operator()(
        _T&& _x,
        _U&& _y
    ) const -> decltype(re_std::forward<_T>(_x) ^ re_std::forward<_U>(_y))
    {
        return re_std::forward<_T>(_x) ^ re_std::forward<_U>(_y);
    }
};

#endif // D_ENV_LANG_IS_CPP14_OR_HIGHER

} // namespace re_std

#endif  // DJINTERP_RE_STD_FUNCTIONAL_BIT_XOR_
