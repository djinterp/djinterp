/******************************************************************************
* djinterp [restd]                                                make_any.hpp
*
* make_any factory header:
*   Provides factory functions for constructing restd::any objects
* with emplaced values. Mirrors the C++17 std::make_any interface:
*   - make_any<T>(args...)                   - forwards to T constructor
*   - make_any<T>(initializer_list, args...) - initializer_list overload
*
*   PORTABILITY:
*   Requires variadic templates (C++11+). Not available on C++98/03;
* use direct construction via the any value constructors instead.
*
*
* path:      /inc/djinterp/re_std/any/make_any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RESTD_MAKE_ANY_
#define DJINTERP_RESTD_MAKE_ANY_ 1

#include "../../core/djinterp.hpp"
#include "./any.hpp"

// gate: requires variadic templates
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES

#include <initializer_list>


NS_RESTD


// ===========================================================================
// I.   MAKE_ANY
// ===========================================================================

// make_any (forwarding)
//   function: constructs an any containing a value of type _Type,
// forwarding _args to the _Type constructor.
template<typename    _Type,
         typename... _Args>
any
make_any(
    _Args&&... _args
)
{
    any result;
    result.template emplace<_Type>(static_cast<_Args&&>(_args)...);

    return result;
}

// make_any (initializer_list)
//   function: constructs an any containing a value of type _Type,
// forwarding an initializer_list and additional _args to the _Type
// constructor.
template<typename    _Type,
         typename    _U,
         typename... _Args>
any
make_any(
    std::initializer_list<_U> _il,
    _Args&&...                _args
)
{
    any result;
    result.template emplace<_Type>(_il, static_cast<_Args&&>(_args)...);

    return result;
}


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_MAKE_ANY_
