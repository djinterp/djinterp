/*******************************************************************************
* djinterp [re_std]                                                 make_any.hpp
*
* make_any factory header:
*   Provides factory functions for constructing re_std::any objects
* with emplaced values. Mirrors the C++17 std::make_any interface:
*   - make_any<T>(args...)                   - forwards to T constructor
*   - make_any<T>(initializer_list, args...) - initializer_list overload
*
*   PORTABILITY:
*   Requires variadic templates (C++11+). Not available on C++98/03;
* use direct construction via the any value constructors instead.
*
*
* path:      /inc/re_std/any/make_any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.10
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ANY_MAKE_ANY_HPP
#define RE_STD_ANY_MAKE_ANY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./any.hpp"

// gate: requires variadic templates
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES

// std
#include <initializer_list>


namespace re_std
{


// ===========================================================================
// I.   MAKE_ANY
// ===========================================================================

// make_any (forwarding)
//   function: constructs an any containing a value of type Type,
// forwarding _args to the Type constructor.
template<typename    Type,
         typename... Args>
any
make_any(
    Args&&... _args
)
{
    any result;
    result.template emplace<Type>(static_cast<Args&&>(_args)...);

    return result;
}

// make_any (initializer_list)
//   function: constructs an any containing a value of type Type,
// forwarding an initializer_list and additional _args to the Type
// constructor.
template<typename    Type,
         typename    U,
         typename... Args>
any
make_any(
    std::initializer_list<U> _il,
    Args&&...                _args
)
{
    any result;
    result.template emplace<Type>(_il, static_cast<Args&&>(_args)...);

    return result;
}


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_ANY_MAKE_ANY_HPP
