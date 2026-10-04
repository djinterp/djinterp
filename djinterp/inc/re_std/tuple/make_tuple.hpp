/******************************************************************************
* djinterp [re_std]                                             make_tuple.hpp
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
*   REFERENCE_WRAPPER UNWRAP (completed 2026-08-25):
*   The unwrap rule is now honoured via re_std::unwrap_ref_decay, so
*
*       int n = 0;
*       auto t = make_tuple(re_std::ref(n), 1);
*       get<0>(t) = 42;                 // writes through to n
*
* behaves as [tuple.creation] requires. tie() remains the right tool
* for building a tuple of references from named lvalues; make_tuple +
* ref is the composable form.
*
*
* path:      /inc/djinterp/re_std/tuple/make_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TUPLE_MAKE_TUPLE_
#define DJINTERP_RE_STD_TUPLE_MAKE_TUPLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES &&                            \
      D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES )


// djinterp
#include "./tuple.hpp"
#include "../type_traits/decay.hpp"
// decay + reference_wrapper unwrap, per [tuple.creation]/p2
#include "../functional/unwrap_ref_decay.hpp"


NS_RESTD


// =============================================================================
// I.   MAKE_TUPLE
// =============================================================================

NS_INTERNAL

    // make_tuple_decay
    //   helper: the [tuple.creation] Vi computation -- decay, then
    // collapse reference_wrapper<X> to X&. Kept as a named alias
    // rather than using unwrap_ref_decay directly at the call site so
    // the standard's Vi notation stays visible in the signature.
    template<typename _T>
    struct make_tuple_decay
    {
        typedef typename re_std::unwrap_ref_decay<_T>::type type;
    };

NS_END  // internal


// make_tuple
//   function: creates a tuple from forwarded arguments. Element types
// are computed via internal::make_tuple_decay, i.e. decay followed by
// reference_wrapper unwrap.
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


NS_END  // re_std


#endif  // variadic templates && rvalue references


#endif  // DJINTERP_RE_STD_TUPLE_MAKE_TUPLE_
