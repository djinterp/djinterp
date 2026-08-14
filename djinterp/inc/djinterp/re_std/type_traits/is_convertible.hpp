/******************************************************************************
* djinterp [restd]                                          is_convertible.hpp
*
* is_convertible trait header:
*   is_convertible<_From, _To>::value is true iff the imaginary function
* `_To test() { return declval<_From>(); }` is well-formed -- i.e. an implicit
* conversion from _From to _To exists.  Both cv void operands are convertible
* to each other and to nothing else; array and function types decay as usual.
*
*   IMPLEMENTATION:
*   The portable SFINAE probe is exact for the ordinary cases and needs no
* intrinsic: a helper taking `_To` by value is called with `declval<_From>()`
* inside decltype.  The void/void case is handled ahead of the probe, since a
* function parameter of type void is ill-formed.
*
*   PORTABILITY:
*   C++11 baseline (decltype + declval).  The _v spelling is C++14+.
*
*
* path:      /inc/djinterp/restd/type_traits/is_convertible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.27
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_CONVERTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_CONVERTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_void.hpp"
#include "./void_t.hpp"
#include "../utility/declval.hpp"


NS_RESTD


// =============================================================================
// I.   IS_CONVERTIBLE
// =============================================================================

NS_INTERNAL

    // is_convertible_probe_
    //   helper: primary -- the conversion is ill-formed.
    template<typename _From,
             typename _To,
             typename = void>
    struct is_convertible_probe_ : false_type
    {};

    // is_convertible_probe_ (viable)
    //   helper: specialization -- selected when a _To-taking function may be
    // called with a _From, which is precisely the standard's imaginary-return
    // formulation.
    template<typename _From,
             typename _To>
    struct is_convertible_probe_<_From, _To,
        void_t<decltype( declval<void (&)(_To)>()( declval<_From>() ) )> >
        : true_type
    {};

NS_END  // internal

// is_convertible
//   trait: whether _From implicitly converts to _To.  Both-void is true; a
// single void operand is false; otherwise the SFINAE probe decides.
template<typename _From,
         typename _To>
struct is_convertible
    : integral_constant<bool,
        ( ( is_void<_From>::value && is_void<_To>::value ) ||
          ( !is_void<_From>::value && !is_void<_To>::value &&
            internal::is_convertible_probe_<_From, _To>::value ) )>
{};


// =============================================================================
// II.  IS_CONVERTIBLE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _From,
         typename _To>
D_CONSTEXPR bool is_convertible_v = is_convertible<_From, _To>::value;

#endif


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_CONVERTIBLE_
