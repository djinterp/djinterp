/******************************************************************************
* djinterp [restd]                                         array_tuple_size.hpp
*
* array tuple_size specialization header:
*   Specialises restd::tuple_size for array<_Type, _Size>:
*
*     tuple_size<array<_Type, _Size>>::value == _Size
*
*   Together with array_tuple_element.hpp and array_get.hpp this
* makes array<_Type, _Size> a tuple-like type — usable with
* structured bindings (C++17+) and the apply / make_from_tuple
* machinery in <tuple>.
*
*   CV-QUALIFIED ARRAYS:
*   The primary tuple_size template ships cv-qualified passthrough
* specialisations per LWG 2762 (already in restd::utility). They
* automatically forward cv-qualified array<...> to the unqualified
* specialisation here — no extra work needed.
*
*
* path:      /inc/djinterp/restd/array/array_tuple_size.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_ARRAY_TUPLE_SIZE_
#define DJINTERP_RESTD_ARRAY_TUPLE_SIZE_ 1

#include <cstddef>

#include "../../core/djinterp.hpp"
#include "./array.hpp"
#include "../utility/tuple_size.hpp"


NS_RESTD


// ===========================================================================
// I.   tuple_size<array>
// ===========================================================================

// tuple_size<array<_Type, _Size>>
//   trait: yields _Size as a std::size_t integral_constant.
template<typename    _Type,
         std::size_t _Size>
struct tuple_size<array<_Type, _Size> >
    : restd::integral_constant<std::size_t, _Size>
{};


NS_END  // restd


#endif  // DJINTERP_RESTD_ARRAY_TUPLE_SIZE_
