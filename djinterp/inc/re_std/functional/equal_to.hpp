/*******************************************************************************
* djinterp [re_std]                                                 equal_to.hpp
*
* equal_to class header:
* function object: equality test (==).
*
*
* path:      /inc/re_std/functional/equal_to.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_EQUAL_TO_HPP
#define RE_STD_FUNCTIONAL_EQUAL_TO_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// equal_to
//   class: function object performing equality test (==).
template<typename Type
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
                     = void
#endif
        >
struct equal_to
{
#if !RE_STD_LANG_IS_CPP20_OR_HIGHER
    typedef Type first_argument_type;
    typedef Type second_argument_type;
    typedef bool  result_type;
#endif

    RE_STD_CONSTEXPR bool
    operator()(
        const Type& _x,
        const Type& _y
    ) const
    {
        return _x == _y;
    }
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// equal_to<void>
//   class: transparent specialization, from C++11 (std's is C++14;
// it needs only decltype and forwarding); deduces operand types and
// forwards them through the operation.
template<>
struct equal_to<void>
{
    typedef int is_transparent;

    template<typename T,
             typename U>
    RE_STD_CONSTEXPR auto
    operator()(
        T&& _x,
        U&& _y
    ) const -> decltype(re_std::forward<T>(_x) == re_std::forward<U>(_y))
    {
        return re_std::forward<T>(_x) == re_std::forward<U>(_y);
    }
};

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

}  // re_std
#endif  // RE_STD_FUNCTIONAL_EQUAL_TO_HPP
