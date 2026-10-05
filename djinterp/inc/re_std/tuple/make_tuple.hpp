/*******************************************************************************
* djinterp [re_std]                                               make_tuple.hpp
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
* path:      /inc/re_std/tuple/make_tuple.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TUPLE_MAKE_TUPLE_HPP
#define RE_STD_TUPLE_MAKE_TUPLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if ( RE_STD_LANG_HAS_VARIADIC_TEMPLATES &&                            \
      RE_STD_LANG_HAS_RVALUE_REFERENCES )


// re_std
#include "./tuple.hpp"
#include "../type_traits/decay.hpp"
// decay + reference_wrapper unwrap, per [tuple.creation]/p2
#include "../functional/unwrap_ref_decay.hpp"


namespace re_std
{


// =============================================================================
// I.   MAKE_TUPLE
// =============================================================================

namespace internal
{

    // make_tuple_decay
    //   trait: the [tuple.creation] Vi computation -- decay, then
    // collapse reference_wrapper<X> to X&. Kept as a named alias
    // rather than using unwrap_ref_decay directly at the call site so
    // the standard's Vi notation stays visible in the signature.
    template<typename T>
    struct make_tuple_decay
    {
        typedef typename re_std::unwrap_ref_decay<T>::type type;
    };

}  // internal


// make_tuple
//   function: creates a tuple from forwarded arguments. Element types
// are computed via internal::make_tuple_decay, i.e. decay followed by
// reference_wrapper unwrap.
template<typename... Types>
RE_STD_CONSTEXPR
tuple<typename internal::make_tuple_decay<Types>::type...>
make_tuple(
    Types&&... _args
)
{
    return tuple<typename internal::make_tuple_decay<Types>::type...>(
        static_cast<Types&&>(_args)...);
}


}  // re_std


#endif  // variadic templates && rvalue references


#endif  // RE_STD_TUPLE_MAKE_TUPLE_HPP
