/******************************************************************************
* djinterp [restd]                                            tuple_element.hpp
*
* tuple_element trait header:
*   Yields the type of the _I-th element of a tuple-like type. Per
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
* compile-time error (no member `type`) if _I is out of range. This
* matches the standard's "Mandates" clause.
*
*   PORTABILITY:
*   Requires variadic templates (C++11+).
*
*
* path:      /inc/djinterp/restd/tuple/tuple_element.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_TUPLE_ELEMENT_
#define DJINTERP_RESTD_TUPLE_TUPLE_ELEMENT_ 1

// djinterp
#include "../../core/djinterp.hpp"


// gate: requires variadic templates
#if D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


// std
#include <cstddef>
// djinterp
#include "../type_traits/add_const.hpp"
#include "../type_traits/add_volatile.hpp"
#include "../type_traits/add_cv.hpp"


NS_RESTD


// =============================================================================
// I.   FORWARD DECLARATION OF TUPLE
// =============================================================================

template<typename... _Types>
class tuple;


// =============================================================================
// II.  TUPLE_ELEMENT
// =============================================================================

// tuple_element
//   trait: primary template, undefined.
template<std::size_t _I,
         typename    _Tuple>
struct tuple_element;

// tuple_element<0, tuple<_Head, _Tail...>>
//   trait: head case yields the head type.
template<typename    _Head,
         typename... _Tail>
struct tuple_element<0, tuple<_Head, _Tail...> >
{
    typedef _Head type;
};

// tuple_element<_I, tuple<_Head, _Tail...>>
//   trait: recursive case strips one element and decrements the index.
template<std::size_t _I,
         typename    _Head,
         typename... _Tail>
struct tuple_element<_I, tuple<_Head, _Tail...> >
    : tuple_element<_I - 1, tuple<_Tail...> >
{};

// cv-qualified passthrough specializations (LWG 2762): the resulting
// element type carries the tuple's cv-qualification.

template<std::size_t _I,
         typename    _Tuple>
struct tuple_element<_I, const _Tuple>
{
    typedef typename add_const<
                typename tuple_element<_I, _Tuple>::type
            >::type type;
};

template<std::size_t _I,
         typename    _Tuple>
struct tuple_element<_I, volatile _Tuple>
{
    typedef typename add_volatile<
                typename tuple_element<_I, _Tuple>::type
            >::type type;
};

template<std::size_t _I,
         typename    _Tuple>
struct tuple_element<_I, const volatile _Tuple>
{
    typedef typename add_cv<
                typename tuple_element<_I, _Tuple>::type
            >::type type;
};


// =============================================================================
// III. TUPLE_ELEMENT_T (C++14+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // tuple_element_t
    //   alias: convenience alias for tuple_element<_I, _Tuple>::type.
    template<std::size_t _I,
             typename    _Tuple>
    using tuple_element_t = typename tuple_element<_I, _Tuple>::type;

#endif


NS_END  // restd


#endif  // D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES


#endif  // DJINTERP_RESTD_TUPLE_TUPLE_ELEMENT_
