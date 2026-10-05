/*******************************************************************************
* djinterp [re_std]                                         array_tuple_size.hpp
*
* array tuple_size specialization header:
*   Specialises re_std::tuple_size for array<Type, Size>:
*
*     tuple_size<array<Type, Size>>::value == Size
*
*   Together with array_tuple_element.hpp and array_get.hpp this
* makes array<Type, Size> a tuple-like type — usable with
* structured bindings (C++17+) and the apply / make_from_tuple
* machinery in <tuple>.
*
*   CV-QUALIFIED ARRAYS:
*   The primary tuple_size template ships cv-qualified passthrough
* specialisations per LWG 2762 (already in re_std::utility). They
* automatically forward cv-qualified array<...> to the unqualified
* specialisation here — no extra work needed.
*
*
* path:      /inc/re_std/array/array_tuple_size.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_TUPLE_SIZE_HPP
#define RE_STD_ARRAY_ARRAY_TUPLE_SIZE_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "./array.hpp"
#include "../tuple/tuple_size.hpp"


namespace re_std
{


// ===========================================================================
// I.   tuple_size<array>
// ===========================================================================

// tuple_size<array<Type, Size>>
//   trait: yields Size as a std::size_t integral_constant.
template<typename    Type,
         std::size_t Size>
struct tuple_size<array<Type, Size> >
    : re_std::integral_constant<std::size_t, Size>
{};


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_TUPLE_SIZE_HPP
