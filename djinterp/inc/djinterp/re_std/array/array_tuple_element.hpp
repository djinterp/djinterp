/******************************************************************************
* djinterp [re_std]                                     array_tuple_element.hpp
*
* array tuple_element specialization header:
*   Specialises re_std::tuple_element for array<_Type, _Size>:
*
*     tuple_element<_Index, array<_Type, _Size>>::type == _Type
*
*   Out-of-range indices (_Index >= _Size) are diagnosed at
* instantiation via static_assert (C++11+); on C++98/03 the
* specialisation simply omits the ::type member, producing a
* substitution failure at the use site.
*
*   CV-QUALIFIED ARRAYS:
*   The primary tuple_element template ships cv-qualified passthrough
* specialisations per LWG 2762 (already in re_std::utility), so
* tuple_element<_I, array<_Type, _Size> const>::type is
*   tuple_element<_I, array<_Type, _Size>>::type const  ==  _Type const,
* picked up automatically.
*
*
* path:      /inc/djinterp/re_std/array/array_tuple_element.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RE_STD_ARRAY_TUPLE_ELEMENT_
#define DJINTERP_RE_STD_ARRAY_TUPLE_ELEMENT_ 1

#include <cstddef>

#include "../../core/djinterp.hpp"
#include "./array.hpp"
#include "../tuple/tuple_element.hpp"


NS_RESTD


// ===========================================================================
// I.   tuple_element<I, array>
// ===========================================================================

// tuple_element<_Index, array<_Type, _Size>>
//   trait: yields _Type as ::type. _Index must be < _Size.
template<std::size_t _Index,
         typename    _Type,
         std::size_t _Size>
struct tuple_element<_Index, array<_Type, _Size> >
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    static_assert(_Index < _Size,
        "re_std::tuple_element<I, array<T, N>>: I must be less than N");
#endif

    typedef _Type type;
};


NS_END  // re_std


#endif  // DJINTERP_RE_STD_ARRAY_TUPLE_ELEMENT_
