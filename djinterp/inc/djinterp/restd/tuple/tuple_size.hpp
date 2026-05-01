/******************************************************************************
* djinterp [restd]                                                tuple_size.hpp
*
* tuple_size trait header:
*   Yields the number of elements in a tuple-like type as a
* compile-time `std::size_t`. Forward-declares the primary template
* and provides the partial specialization for `tuple<_Types...>` plus
* cv-qualified variants (a defect report - LWG 2762 - clarified that
* tuple_size of cv-qualified tuple-likes should match the unqualified).
*
*     tuple_size<tuple<>>::value                 -> 0
*     tuple_size<tuple<int>>::value              -> 1
*     tuple_size<tuple<int, char>>::value        -> 2
*     tuple_size<const tuple<int, char>>::value  -> 2  (LWG 2762)
*
*   PORTABILITY:
*   Requires variadic templates (C++11+). The trait may be specialized
* by user code for any tuple-like type that supports structured
* bindings.
*
*
* path:      /inc/djinterp/restd/tuple/tuple_size.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_TUPLE_SIZE_
#define DJINTERP_RESTD_TUPLE_TUPLE_SIZE_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: requires variadic templates
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// std
#include <cstddef>
// djinterp
#include "../type_traits/integral_constant.hpp"


NS_RESTD


// =============================================================================
// I.   FORWARD DECLARATION OF TUPLE
// =============================================================================
// Forward-declared here so tuple_size can name it in its specialization
// without depending on the full tuple definition.

template<typename... _Types>
class tuple;


// =============================================================================
// II.  TUPLE_SIZE
// =============================================================================

// tuple_size
//   trait: primary template, undefined. User specializations are
// permitted for any tuple-like type.
template<typename _Tuple>
struct tuple_size;

// tuple_size<tuple<_Types...>>
//   trait: yields sizeof...(_Types) for the restd::tuple specialization.
template<typename... _Types>
struct tuple_size<tuple<_Types...> >
    : integral_constant<std::size_t, sizeof...(_Types)>
{};

// cv-qualified passthrough specializations (LWG 2762).
template<typename _Tuple>
struct tuple_size<const _Tuple>
    : integral_constant<std::size_t, tuple_size<_Tuple>::value>
{};

template<typename _Tuple>
struct tuple_size<volatile _Tuple>
    : integral_constant<std::size_t, tuple_size<_Tuple>::value>
{};

template<typename _Tuple>
struct tuple_size<const volatile _Tuple>
    : integral_constant<std::size_t, tuple_size<_Tuple>::value>
{};


// =============================================================================
// III. TUPLE_SIZE_V (C++17+ variable template)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // tuple_size_v
    //   variable: convenience for tuple_size<_Tuple>::value.
    template<typename _Tuple>
    D_CONSTEXPR std::size_t tuple_size_v = tuple_size<_Tuple>::value;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TUPLE_TUPLE_SIZE_
