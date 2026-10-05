/*******************************************************************************
* djinterp [re_std]                                               tuple_size.hpp
*
* tuple_size trait header:
*   Yields the number of elements in a tuple-like type as a
* compile-time `std::size_t`. Forward-declares the primary template
* and provides the partial specialization for `tuple<Types...>` plus
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
* path:      /inc/re_std/tuple/tuple_size.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_SIZE_HPP
#define RE_STD_TUPLE_TUPLE_SIZE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: requires variadic templates
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// std
#include <cstddef>
// re_std
#include "../type_traits/integral_constant.hpp"


namespace re_std
{


// =============================================================================
// I.   FORWARD DECLARATION OF TUPLE
// =============================================================================
// Forward-declared here so tuple_size can name it in its specialization
// without depending on the full tuple definition.

template<typename... Types>
class tuple;


// =============================================================================
// II.  TUPLE_SIZE
// =============================================================================

// tuple_size
//   trait: primary template, undefined. User specializations are
// permitted for any tuple-like type.
template<typename Tuple>
struct tuple_size;

// tuple_size<tuple<Types...>>
//   trait: yields sizeof...(Types) for the re_std::tuple specialization.
template<typename... Types>
struct tuple_size<tuple<Types...> >
    : integral_constant<std::size_t, sizeof...(Types)>
{};

// cv-qualified passthrough specializations (LWG 2762).
template<typename Tuple>
struct tuple_size<const Tuple>
    : integral_constant<std::size_t, tuple_size<Tuple>::value>
{};

template<typename Tuple>
struct tuple_size<volatile Tuple>
    : integral_constant<std::size_t, tuple_size<Tuple>::value>
{};

template<typename Tuple>
struct tuple_size<const volatile Tuple>
    : integral_constant<std::size_t, tuple_size<Tuple>::value>
{};


// =============================================================================
// III. TUPLE_SIZE_V (C++17+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    // tuple_size_v
    //   variable: convenience for tuple_size<Tuple>::value.
    template<typename Tuple>
    RE_STD_CONSTEXPR std::size_t tuple_size_v = tuple_size<Tuple>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TUPLE_TUPLE_SIZE_HPP
