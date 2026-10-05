/*******************************************************************************
* djinterp [re_std]                                      array_tuple_element.hpp
*
* array tuple_element specialization header:
*   Specialises re_std::tuple_element for array<Type, Size>:
*
*     tuple_element<Index, array<Type, Size>>::type == Type
*
*   Out-of-range indices (Index >= Size) are diagnosed at
* instantiation via static_assert (C++11+); on C++98/03 the
* specialisation simply omits the ::type member, producing a
* substitution failure at the use site.
*
*   CV-QUALIFIED ARRAYS:
*   The primary tuple_element template ships cv-qualified passthrough
* specialisations per LWG 2762 (already in re_std::utility), so
* tuple_element<_I, array<Type, Size> const>::type is
*   tuple_element<_I, array<Type, Size>>::type const  ==  Type const,
* picked up automatically.
*
*
* path:      /inc/re_std/array/array_tuple_element.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_TUPLE_ELEMENT_HPP
#define RE_STD_ARRAY_ARRAY_TUPLE_ELEMENT_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "./array.hpp"
#include "../tuple/tuple_element.hpp"


namespace re_std
{


// ===========================================================================
// I.   tuple_element<I, array>
// ===========================================================================

// tuple_element<Index, array<Type, Size>>
//   trait: yields Type as ::type. Index must be < Size.
template<std::size_t Index,
         typename    Type,
         std::size_t Size>
struct tuple_element<Index, array<Type, Size> >
{
#if RE_STD_LANG_IS_CPP11_OR_HIGHER
    static_assert(Index < Size,
        "re_std::tuple_element<I, array<T, N>>: I must be less than N");
#endif

    typedef Type type;
};


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_TUPLE_ELEMENT_HPP
