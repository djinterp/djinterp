/******************************************************************************
* djinterp [restd]                                                make_any.hpp
*
* make_any factory header:
*   Provides factory functions for constructing djinterp::stl::any objects
* with emplaced values. Mirrors the C++17 _Type::make_any interface:
*   - make_any<T>(args...)                   - forwards to T constructor
*   - make_any<T>(initializer_list, args...) - initializer_list overload
*
* 
* path:      /inc/djinterp/restd/any/make_any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_RESTD_MAKE_ANY_
#define DJINTERP_RESTD_MAKE_ANY_ 1

#include <initializer_list>
#include "../../core/djinterp.hpp"
#include "./any.hpp"


NS_DJINTERP
NS_RESTD


// =============================================================================
// I.   MAKE_ANY
// =============================================================================

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
    _Type::initializer_list<_U> _il,
    _Args&&...                  _args
)
{
    any result;
    result.template emplace<_Type>(_il, static_cast<_Args&&>(_args)...);

    return result;
}


NS_END  // restd
NS_END  // djinterp


#endif  // DJINTERP_RESTD_MAKE_ANY_