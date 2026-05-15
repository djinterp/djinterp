/***********************************************************************
* restd                                                       exchange.hpp
*
* exchange(_obj, _new_val):
*   Replaces _obj's value with _new_val and returns the previous
* value of _obj. The return is move-constructed (or copy-constructed
* on C++98/03), and the assignment uses perfect forwarding.
*
*   Canonical idiom: implementing move ctors/op= cleanly.
*     T(T&& other) noexcept
*       : ptr(restd::exchange(other.ptr, nullptr))   // steal + null
*     { }
*
* tiered:
*   C++14    available, no constexpr (return needs more than one
*            statement under the C++11 constexpr rules)
*   C++20+   constexpr (std made it so)
*   restd    constexpr from C++14+ via D_CONSTEXPR_CPP14
*
*
* path:      /inc/restd/utility/exchange.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.05.09
***********************************************************************/

#ifndef RESTD_UTILITY_EXCHANGE_
#define RESTD_UTILITY_EXCHANGE_ 1

#include "djinterp.hpp"


#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    #include "../utility/move.hpp"
    #include "../utility/forward.hpp"


// D_CONSTEXPR_CPP14 -- constexpr on C++14+, empty on C++11. Slated
// for the global qualifier-macro table; locally defined for now to
// keep this file self-contained.
#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


namespace restd
{

template<typename _T, typename _U = _T>
D_CONSTEXPR_CPP14 _T exchange(_T& _obj, _U&& _new_val)
{
    // Save the old value (move if possible), assign the new, return
    // the saved old.
    _T _old = restd::move(_obj);
    _obj = restd::forward<_U>(_new_val);
    return _old;
}


}  // namespace restd

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

#endif  // RESTD_UTILITY_EXCHANGE_
