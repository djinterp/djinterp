/*******************************************************************************
* djinterp [re_std]                                     is_reference_wrapper.hpp
*
* is_reference_wrapper trait header:
*   The reference_wrapper DETECTION TRAIT, split out of
* reference_wrapper.hpp so that invoke.hpp can dispatch on it without
* pulling in the class definition.
*
*   WHY A SEPARATE HEADER:
*   invoke.hpp needs `is_reference_wrapper` to select the rw-arg INVOKE
* bullets, and reference_wrapper::operator() needs `re_std::invoke`.  That
* is a cycle.  Include guards do not resolve it: they prevent infinite
* recursion, not mis-ordering -- whichever of the two files is entered
* first, reference_wrapper::operator()'s trailing return type is parsed
* before `re_std::invoke` is declared, and because `re_std::invoke` is a
* qualified-id whose nested-name-specifier does not depend on a template
* parameter, it is looked up at template DEFINITION time.
*
*   Splitting the trait breaks the cycle at its narrowest point: this
* header needs only a forward declaration of reference_wrapper, invoke.hpp
* includes only this, and reference_wrapper.hpp includes invoke.hpp
* normally, at the top.
*
*   Min standard: C++11.
*
*
* path:      /inc/re_std/functional/is_reference_wrapper.hpp
* link(s):   TBA
* author(s): re_std                                                 created: TBA
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_FUNCTIONAL_IS_REFERENCE_WRAPPER_HPP
#define RE_STD_FUNCTIONAL_IS_REFERENCE_WRAPPER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "../type_traits/false_type.hpp"  // false_type
#include "../type_traits/remove_cv.hpp"   // remove_cv
#include "../type_traits/true_type.hpp"   // true_type

namespace re_std
{

// reference_wrapper
//   class: forward declaration only -- the trait below needs the name, not
// the definition.  reference_wrapper.hpp supplies the definition.
template<typename Type>
class reference_wrapper;

namespace internal
{

    // is_reference_wrapper_helper
    //   trait: primary -- false for arbitrary types.
    template<typename Type>
    struct is_reference_wrapper_helper : false_type
    {};

    // is_reference_wrapper_helper<reference_wrapper<U>>
    //   trait: specialization -- true for reference_wrapper.
    template<typename U>
    struct is_reference_wrapper_helper< reference_wrapper<U> > : true_type
    {};

}  // internal

// is_reference_wrapper
//   trait: detects reference_wrapper.  Cv-stripped.
template<typename Type>
struct is_reference_wrapper
    : internal::is_reference_wrapper_helper<typename remove_cv<Type>::type>
{};

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_reference_wrapper_v
    = is_reference_wrapper<Type>::value;

#endif

}  // re_std
#endif  // RE_STD_FUNCTIONAL_IS_REFERENCE_WRAPPER_HPP
