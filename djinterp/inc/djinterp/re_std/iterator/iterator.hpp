/******************************************************************************
* re_std [iterator]                                              iterator.hpp
*
*   the `iterator` base class template - DEPRECATED IN C++17, shipped anyway.
*
*   WHY SHIP A DEPRECATED FACILITY.
*   Because re_std's job is compiling existing code. Iterators written before
* C++17 routinely derive from std::iterator to pick up the five member
* typedefs, and that code does not stop existing when the standard deprecates
* the base. Omitting it would make re_std unusable for exactly the C++98-era
* codebases it is aimed at.
*
*   WHY IT WAS DEPRECATED, so the note is useful rather than just a warning:
* deriving publicly to obtain typedefs is fragile. The base is not a real
* interface, its presence perturbs overload resolution and traits like
* is_base_of, and an iterator that inherits five typedefs it does not
* deliberately declare is easy to get subtly wrong - a mutable iterator that
* forgets to override `reference`, for instance. Declaring the five typedefs
* directly is clearer and is what iterator_traits actually reads.
*
*   NOT MARKED [[deprecated]]. The attribute would fire on every use in the
* legacy code this exists to serve, which is noise rather than information -
* the user already cannot change that code, or they would not need re_std.
*
*   STD WAS C++98 (deprecated C++17); re_std IS C++98.
*
* path:      /inc/djinterp/re_std/iterator/iterator.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_ITERATOR_
#define DJINTERP_RE_STD_ITERATOR_ITERATOR_ 1

#include "../../core/djinterp.hpp"
#include "../type_traits/type_traits.hpp"

NS_RESTD

// iterator
//   struct: supplies the five iterator typedefs to a derived iterator.
template<typename _Category,
         typename _Type,
         typename _Distance  = ptrdiff_t,
         typename _Pointer   = _Type*,
         typename _Reference = _Type&>
struct iterator
{
    typedef _Category  iterator_category;
    typedef _Type      value_type;
    typedef _Distance  difference_type;
    typedef _Pointer   pointer;
    typedef _Reference reference;
};

NS_END

#endif  // DJINTERP_RE_STD_ITERATOR_ITERATOR_
