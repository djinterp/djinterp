/*******************************************************************************
* djinterp [re_std]                                                   negate.hpp
*
* negate class header:
* function object: arithmetic negation (unary -).
*
*
* path:      /inc/re_std/functional/negate.hpp
* link(s):   TBA
* author(s): re_std                                          created: 2026.05.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_NEGATE_HPP
#define RE_STD_FUNCTIONAL_NEGATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "re_std/utility/forward.hpp"
#endif

namespace re_std
{

// negate
//   class: function object performing arithmetic negation (unary -).
template<typename Type
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
                     = void
#endif
        >
struct negate
{
#if !RE_STD_LANG_IS_CPP20_OR_HIGHER
    typedef Type argument_type;
    typedef Type result_type;
#endif

    RE_STD_CONSTEXPR Type
    operator()(
        const Type& _x
    ) const
    {
        return -_x;
    }
};

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// negate<void>
//   class: transparent specialization, from C++11 (std's is C++14;
// it needs only decltype and forwarding); deduces the operand type and
// forwards it through the operation.
template<>
struct negate<void>
{
    typedef int is_transparent;

    template<typename T>
    RE_STD_CONSTEXPR auto
    operator()(
        T&& _x
    ) const -> decltype(-re_std::forward<T>(_x))
    {
        return -re_std::forward<T>(_x);
    }
};

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

}  // re_std
#endif  // RE_STD_FUNCTIONAL_NEGATE_HPP
