/***********************************************************************
* restd [functional]                                is_reference_wrapper.hpp
*
*   The reference_wrapper DETECTION TRAIT, split out of
* reference_wrapper.hpp so that invoke.hpp can dispatch on it without
* pulling in the class definition.
*
*   WHY A SEPARATE HEADER:
*   invoke.hpp needs `is_reference_wrapper` to select the rw-arg INVOKE
* bullets, and reference_wrapper::operator() needs `restd::invoke`.  That
* is a cycle.  Include guards do not resolve it: they prevent infinite
* recursion, not mis-ordering -- whichever of the two files is entered
* first, reference_wrapper::operator()'s trailing return type is parsed
* before `restd::invoke` is declared, and because `restd::invoke` is a
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
* path:      /inc/restd/functional/is_reference_wrapper.hpp
* link(s):   TBA
* author(s): restd
***********************************************************************/

#ifndef RESTD_FUNCTIONAL_IS_REFERENCE_WRAPPER_
#define RESTD_FUNCTIONAL_IS_REFERENCE_WRAPPER_ 1

#include "djinterp.hpp"
#include "restd/type_traits/type_traits.hpp"

namespace restd
{

// reference_wrapper
//   class: forward declaration only -- the trait below needs the name, not
// the definition.  reference_wrapper.hpp supplies the definition.
template<typename _Type>
class reference_wrapper;

NS_INTERNAL

    // is_reference_wrapper_helper
    //   trait: primary -- false for arbitrary types.
    template<typename _Type>
    struct is_reference_wrapper_helper : false_type
    {};

    // is_reference_wrapper_helper<reference_wrapper<U>>
    //   trait: specialization -- true for reference_wrapper.
    template<typename _U>
    struct is_reference_wrapper_helper< reference_wrapper<_U> > : true_type
    {};

NS_END  // internal

// is_reference_wrapper
//   trait: detects reference_wrapper.  Cv-stripped.
template<typename _Type>
struct is_reference_wrapper
    : internal::is_reference_wrapper_helper<typename remove_cv<_Type>::type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_reference_wrapper_v
    = is_reference_wrapper<_Type>::value;

#endif

} // namespace restd

#endif // RESTD_FUNCTIONAL_IS_REFERENCE_WRAPPER_
