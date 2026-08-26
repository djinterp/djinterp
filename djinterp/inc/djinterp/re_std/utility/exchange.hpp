/***********************************************************************
* re_std                                                     exchange.hpp
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
* path:      /inc/djinterp/re_std/utility/exchange.hpp
* link(s):   TBA
* author(s): re_std team                                 date: 2026.05.02
***********************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_EXCHANGE_
#define DJINTERP_RE_STD_UTILITY_EXCHANGE_ 1

#include "djinterp.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#include "../utility/move.hpp"
#include "../utility/forward.hpp"

NS_RESTD

// =============================================================================
// EXCHANGE
// =============================================================================

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

    // exchange (C++14+: relaxed constexpr permits multi-statement body)
    //   function: replaces obj's value with new_value and returns the
    //   previous value.
    template<typename _Type, typename _Other>
    D_CONSTEXPR _Type exchange(_Type& _obj, _Other&& _new_value)
    {
        _Type _old = re_std::move(_obj);
        _obj = re_std::forward<_Other>(_new_value);
        return _old;
    }

#else  // C++11

    // exchange (C++11: not constexpr -- multi-statement body)
    template<typename _Type, typename _Other>
    _Type exchange(_Type& _obj, _Other&& _new_value)
    {
        _Type _old = re_std::move(_obj);
        _obj = re_std::forward<_Other>(_new_value);
        return _old;
    }

#endif

NS_END  // re_std

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // DJINTERP_RE_STD_UTILITY_EXCHANGE_
