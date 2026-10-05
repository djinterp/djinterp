/*******************************************************************************
* djinterp [re_std]                                           is_convertible.hpp
*
* is_convertible trait header:
*   is_convertible<From, To>::value is true iff the imaginary function
* `To test() { return declval<From>(); }` is well-formed -- i.e. an implicit
* conversion from From to To exists.  Both cv void operands are convertible
* to each other and to nothing else; array and function types decay as usual.
*
*   IMPLEMENTATION:
*   The portable SFINAE probe is exact for the ordinary cases and needs no
* intrinsic: a helper taking `To` by value is called with `declval<From>()`
* inside decltype.  The void/void case is handled ahead of the probe, since a
* function parameter of type void is ill-formed.
*
*   PORTABILITY:
*   C++11 baseline (decltype + declval).  The _v spelling is C++14+.
*
*
* path:      /inc/re_std/type_traits/is_convertible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_CONVERTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_CONVERTIBLE_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_void.hpp"
#include "./void_t.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


// =============================================================================
// I.   IS_CONVERTIBLE
// =============================================================================

namespace internal
{

    // is_convertible_probe_
    //   trait: primary -- the conversion is ill-formed.
    template<typename From,
             typename To,
             typename = void>
    struct is_convertible_probe_ : false_type
    {};

    // is_convertible_probe_ (viable)
    //   function: specialization -- selected when a To-taking function may be
    // called with a From, which is precisely the standard's imaginary-return
    // formulation.
    template<typename From,
             typename To>
    struct is_convertible_probe_<From, To,
        void_t<decltype( declval<void (&)(To)>()( declval<From>() ) )> >
        : true_type
    {};

}  // internal

// is_convertible
//   trait: whether From implicitly converts to To.  Both-void is true; a
// single void operand is false; otherwise the SFINAE probe decides.
template<typename From,
         typename To>
struct is_convertible
    : integral_constant<bool,
        ( ( is_void<From>::value && is_void<To>::value ) ||
          ( !is_void<From>::value && !is_void<To>::value &&
            internal::is_convertible_probe_<From, To>::value ) )>
{};


// =============================================================================
// II.  IS_CONVERTIBLE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename From,
         typename To>
RE_STD_CONSTEXPR bool is_convertible_v = is_convertible<From, To>::value;

#endif


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_CONVERTIBLE_HPP
