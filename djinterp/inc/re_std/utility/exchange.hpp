/*******************************************************************************
* djinterp [re_std]                                                 exchange.hpp
*
* value-replacing utility:
*   Provides re_std::exchange, which replaces the value of an object
* with a new one and returns the old value. Equivalent to:
*
*     T old = std::move(obj);
*     obj   = std::forward<U>(new_value);
*     return old;
*
*   STANDARD STATUS:
*   Added in C++14. Made constexpr in C++20 (P1132R7). re_std provides
* on C++11+ (without constexpr), C++14+ (with constexpr -- the body is
* multi-statement, which requires C++14 relaxed constexpr).
*
*   Requires rvalue references; gated accordingly.
*
*
* path:      /inc/re_std/utility/exchange.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.05.02
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_EXCHANGE_HPP
#define RE_STD_UTILITY_EXCHANGE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

#include "move.hpp"
#include "forward.hpp"

namespace re_std
{

// =============================================================================
// EXCHANGE
// =============================================================================

#if RE_STD_LANG_IS_CPP14_OR_HIGHER

    // exchange (C++14+: relaxed constexpr permits multi-statement body)
    //   function: replaces obj's value with new_value and returns the
    //   previous value.
    template<typename Type, typename Other>
    RE_STD_CONSTEXPR Type exchange(Type& _obj, Other&& _new_value)
    {
        Type _old = re_std::move(_obj);
        _obj = re_std::forward<Other>(_new_value);
        return _old;
    }

#else  // C++11

    // exchange (C++11: not constexpr -- multi-statement body)
    template<typename Type, typename Other>
    Type exchange(Type& _obj, Other&& _new_value)
    {
        Type _old = re_std::move(_obj);
        _obj = re_std::forward<Other>(_new_value);
        return _old;
    }

#endif

}  // re_std

#endif  // RE_STD_LANG_HAS_RVALUE_REFERENCES

#endif  // RE_STD_UTILITY_EXCHANGE_HPP
