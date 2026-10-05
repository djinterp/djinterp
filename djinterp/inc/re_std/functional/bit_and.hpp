/*******************************************************************************
* djinterp [re_std]                                                  bit_and.hpp
*
* bit_and class header:
* function object: bitwise and (&).
*
*
* path:      /inc/re_std/functional/bit_and.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_BIT_AND_HPP
#define RE_STD_FUNCTIONAL_BIT_AND_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// bit_and
//   class: function object performing bitwise and (&).
template<typename Type
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
                     = void
#endif
        >
struct bit_and
{
#if !RE_STD_LANG_IS_CPP20_OR_HIGHER
    typedef Type first_argument_type;
    typedef Type second_argument_type;
    typedef Type result_type;
#endif

    RE_STD_CONSTEXPR Type
    operator()(
        const Type& _x,
        const Type& _y
    ) const
    {
        return _x & _y;
    }
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// bit_and<void>
//   class: transparent specialization, from C++11 (std's is C++14;
// it needs only decltype and forwarding); deduces operand types and
// forwards them through the operation.
template<>
struct bit_and<void>
{
    typedef int is_transparent;

    template<typename T,
             typename U>
    RE_STD_CONSTEXPR auto
    operator()(
        T&& _x,
        U&& _y
    ) const -> decltype(re_std::forward<T>(_x) & re_std::forward<U>(_y))
    {
        return re_std::forward<T>(_x) & re_std::forward<U>(_y);
    }
};

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

}  // re_std
#endif  // RE_STD_FUNCTIONAL_BIT_AND_HPP
