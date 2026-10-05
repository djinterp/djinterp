/*******************************************************************************
* djinterp [re_std]                                            tuple_element.hpp
*
* tuple_element trait header:
*   Yields the type of the I-th element of a tuple-like type. Per
* [tuple.helper]:
*   - tuple_element<I, tuple<T0, T1, ...>>::type -> Ti
*   - tuple_element<I, const tuple<...>>::type   -> const Ti  (LWG 2762)
*
*     tuple_element<0, tuple<int, char> >::type        -> int
*     tuple_element<1, tuple<int, char> >::type        -> char
*     tuple_element<0, const tuple<int, char> >::type  -> const int
*
*   IMPLEMENTATION NOTE:
*   The recursive partial-specialization approach used here yields a
* compile-time error (no member `type`) if I is out of range. This
* matches the standard's "Mandates" clause.
*
*   PORTABILITY:
*   Requires variadic templates (C++11+).
*
*
* path:      /inc/re_std/tuple/tuple_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_TUPLE_ELEMENT_HPP
#define RE_STD_TUPLE_TUPLE_ELEMENT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: requires variadic templates
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// std
#include <cstddef>
// re_std
#include "../type_traits/add_const.hpp"
#include "../type_traits/add_volatile.hpp"
#include "../type_traits/add_cv.hpp"


namespace re_std
{


// =============================================================================
// I.   FORWARD DECLARATION OF TUPLE
// =============================================================================

template<typename... Types>
class tuple;


// =============================================================================
// II.  TUPLE_ELEMENT
// =============================================================================

// tuple_element
//   trait: primary template, undefined.
template<std::size_t I,
         typename    Tuple>
struct tuple_element;

// tuple_element<0, tuple<Head, Tail...>>
//   trait: head case yields the head type.
template<typename    Head,
         typename... Tail>
struct tuple_element<0, tuple<Head, Tail...> >
{
    typedef Head type;
};

// tuple_element<I, tuple<Head, Tail...>>
//   trait: recursive case strips one element and decrements the index.
template<std::size_t I,
         typename    Head,
         typename... Tail>
struct tuple_element<I, tuple<Head, Tail...> >
    : tuple_element<I - 1, tuple<Tail...> >
{};

// cv-qualified passthrough specializations (LWG 2762): the resulting
// element type carries the tuple's cv-qualification.

template<std::size_t I,
         typename    Tuple>
struct tuple_element<I, const Tuple>
{
    typedef typename add_const<
                typename tuple_element<I, Tuple>::type
            >::type type;
};

template<std::size_t I,
         typename    Tuple>
struct tuple_element<I, volatile Tuple>
{
    typedef typename add_volatile<
                typename tuple_element<I, Tuple>::type
            >::type type;
};

template<std::size_t I,
         typename    Tuple>
struct tuple_element<I, const volatile Tuple>
{
    typedef typename add_cv<
                typename tuple_element<I, Tuple>::type
            >::type type;
};


// =============================================================================
// III. TUPLE_ELEMENT_T (C++14+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // tuple_element_t
    //   alias: convenience alias for tuple_element<I, Tuple>::type.
    template<std::size_t I,
             typename    Tuple>
    using tuple_element_t = typename tuple_element<I, Tuple>::type;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TUPLE_TUPLE_ELEMENT_HPP
