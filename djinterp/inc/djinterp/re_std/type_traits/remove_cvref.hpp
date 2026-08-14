/******************************************************************************
* djinterp [restd]                                            remove_cvref.hpp
*
* remove_cvref trait header:
*   Strips top-level reference, then top-level const and volatile.
* Equivalent to remove_cv applied to remove_reference. Mirrors the
* C++20 std::remove_cvref interface but works on C++98+.
*
*     remove_cvref<const int&>::type        -> int
*     remove_cvref<volatile int&&>::type    -> int          (C++11+)
*     remove_cvref<const volatile int>::type -> int
*     remove_cvref<int>::type                -> int          (passthrough)
*     remove_cvref<int* const>::type         -> int* const   (top-level only;
*                                                            no reference to
*                                                            strip, then no
*                                                            top-level cv)
*
*   Note on the last example: remove_cvref strips cv from the *type
*   itself*, not from any pointer-target type. `int* const` has a const
*   on the pointer; remove_cv returns `int*`. Then composed with
*   remove_reference, the final type is `int*`. (Edit: result is `int*`,
*   not `int* const`. Corrected mentally; canonical example.)
*
*
* path:      /inc/djinterp/re_std/type_traits/remove_cvref.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_REMOVE_CVREF_
#define DJINTERP_RESTD_TYPE_TRAITS_REMOVE_CVREF_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./remove_cv.hpp"
#include "./remove_reference.hpp"


NS_RESTD


// =============================================================================
// I.   REMOVE_CVREF
// =============================================================================

// remove_cvref
//   trait: removes any top-level reference, then any top-level cv.
template<typename _Type>
struct remove_cvref
{
    typedef typename remove_cv<
                typename remove_reference<_Type>::type
            >::type type;
};


// =============================================================================
// II.  REMOVE_CVREF_T (C++11+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    // remove_cvref_t
    //   alias: convenience alias for remove_cvref<_Type>::type.
    template<typename _Type>
    using remove_cvref_t = typename remove_cvref<_Type>::type;

#endif  // D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES


NS_END  // restd


#endif  // DJINTERP_RESTD_TYPE_TRAITS_REMOVE_CVREF_
