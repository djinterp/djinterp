/******************************************************************************
* djinterp [restd]                                              make_tuple.hpp
*
* make_tuple factory header:
*   Creates a tuple object, deducing element types from the arguments
* and applying the standard's "decay + reference_wrapper unwrap"
* transformation. Per [tuple.creation]:
*
*   for each argument of type Ti, the corresponding tuple element type
*   Vi is decay_t<Ti> -- with one exception: if decay_t<Ti> is
*   reference_wrapper<X> for some X, then Vi is X&.
*
*     make_tuple(1, 'x', 3.14)
*       -> tuple<int, char, double>
*     int n; auto t = make_tuple(ref(n));
*       -> tuple<int&>          (reference_wrapper unwrap)
*
*   PORTABILITY:
*   restd does not yet implement reference_wrapper, so the unwrap path
* is currently a no-op (decay only). When restd::reference_wrapper
* lands, this header will be updated to honour the unwrap rule. The
* common case -- pass-by-value with decay -- works correctly today.
*
*
* path:      /inc/djinterp/restd/tuple/make_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RESTD_TUPLE_MAKE_TUPLE_
#define DJINTERP_RESTD_TUPLE_MAKE_TUPLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// djinterp
#include "./tuple.hpp"
#include "../type_traits/decay.hpp"


NS_RESTD


// =============================================================================
// I.   MAKE_TUPLE
// =============================================================================

NS_INTERNAL

    // make_tuple_decay
    //   helper: applies decay to _T. A reference_wrapper-aware
    // specialisation will be added when restd::reference_wrapper
    // lands; for now this is plain decay, matching the common
    // pass-by-value case.
    template<typename _T>
    struct make_tuple_decay
    {
        typedef typename decay<_T>::type type;
    };

NS_END  // internal


// make_tuple
//   function: creates a tuple from forwarded arguments. Element types
// are computed via internal::make_tuple_decay (which is currently
// equivalent to decay).
template<typename... _Types>
D_CONSTEXPR
tuple<typename internal::make_tuple_decay<_Types>::type...>
make_tuple(
    _Types&&... _args
)
{
    return tuple<typename internal::make_tuple_decay<_Types>::type...>(
        static_cast<_Types&&>(_args)...);
}


NS_END  // restd


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RESTD_TUPLE_MAKE_TUPLE_
