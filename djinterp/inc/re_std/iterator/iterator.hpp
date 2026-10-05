/*******************************************************************************
* djinterp [re_std]                                                 iterator.hpp
*
* iterator class header:
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
*
* path:      /inc/re_std/iterator/iterator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ITERATOR_ITERATOR_HPP
#define RE_STD_ITERATOR_ITERATOR_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "../type_traits/type_traits.hpp"

namespace re_std
{

// iterator
//   struct: supplies the five iterator typedefs to a derived iterator.
template<typename Category,
         typename Type,
         typename Distance  = ptrdiff_t,
         typename Pointer   = Type*,
         typename Reference = Type&>
struct iterator
{
    typedef Category  iterator_category;
    typedef Type      value_type;
    typedef Distance  difference_type;
    typedef Pointer   pointer;
    typedef Reference reference;
};

}

#endif  // floor, for now


#endif  // RE_STD_ITERATOR_ITERATOR_HPP
