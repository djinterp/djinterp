/******************************************************************************
* djinterp [stl]                                              make_any.hpp
*
* make_any factory header:
*   Provides factory functions for constructing djinterp::stl::any objects
* with emplaced values. Mirrors the C++17 std::make_any interface:
*   - make_any<T>(args...)            — forwards to T constructor
*   - make_any<T>(initializer_list, args...) — initializer_list overload
*
* path:      \inc\cpp\stl\make_any.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.10
******************************************************************************/

#ifndef DJINTERP_MAKE_ANY_
#define DJINTERP_MAKE_ANY_ 1

#include <initializer_list>
#include ".\djinterp.hpp"
#include ".\stl\any.hpp"


NS_DJINTERP
NS_STL


// =============================================================================
// I.   MAKE_ANY
// =============================================================================

// make_any (forwarding)
//   function: constructs an any containing a value of type _T,
// forwarding _args to the _T constructor.
template<typename    _T,
         typename... _Args>
any
make_any
(
    _Args&&... _args
)
{
    any result;
    result.template emplace<_T>(static_cast<_Args&&>(_args)...);

    return result;
}

// make_any (initializer_list)
//   function: constructs an any containing a value of type _T,
// forwarding an initializer_list and additional _args to the _T
// constructor.
template<typename    _T,
         typename    _U,
         typename... _Args>
any
make_any
(
    std::initializer_list<_U> _il,
    _Args&&...                _args
)
{
    any result;
    result.template emplace<_T>(_il,
                                static_cast<_Args&&>(_args)...);

    return result;
}


NS_END  // stl
NS_END  // djinterp


#endif  // DJINTERP_MAKE_ANY_
