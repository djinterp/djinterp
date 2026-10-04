/******************************************************************************
* djinterp [re_std]                                 basic_common_reference.hpp
*
* basic_common_reference customization-point trait:
*   The user-extensible hook used by common_reference at bullet 2 of its
* fallback chain. The primary template has no `type` member; users provide
* specializations to teach common_reference about their own types.
*
*   SIGNATURE (per [meta.trans.other]):
*     template<class T, class U,
*              template<class> class TQual,
*              template<class> class UQual>
*     struct basic_common_reference {};
*
*   The TQual / UQual template-template parameters are "qualifier-applying"
* aliases that re-attach the cv- and reference-qualifiers that the original
* T1/T2 had before remove_cvref was applied. They are passed in by
* common_reference; user specializations only need to declare them and
* may use them inside the specialization body to honor the original
* qualification of the inputs.
*
*   USAGE EXAMPLE:
*   To teach common_reference that two custom smart pointers MyPtr<X> and
* MyPtr<Y> have a common reference type related to common_reference<X, Y>:
*     template<typename _X,
*              typename _Y,
*              template<typename> class _TQual,
*              template<typename> class _UQual>
*     struct re_std::basic_common_reference< MyPtr<_X>, MyPtr<_Y>,
*                                           _TQual, _UQual >
*     {
*         typedef MyPtr<typename re_std::common_reference<
*                          _TQual<_X>, _UQual<_Y> >::type> type;
*     };
*
*   Specializations are queried by common_reference only when the first
* fallback (COMMON-REF) does not yield a result. Users should specialize
* on cv-unqualified, non-reference types for T and U (the standard
* requires this; common_reference normalizes T and U via remove_cvref
* before performing the lookup).
*
*   PORTABILITY:
*   Available on C++11 and later. The trait was standardized in C++20;
* re_std backports to C++11+ since the implementation needs only template
* template parameters (a C++98 feature) and the rest of re_std's C++11
* baseline.
*
*   DEPENDENCIES:
*   None beyond the re_std core macros.
*
*
* path:      /inc/djinterp/re_std/type_traits/basic_common_reference.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_BASIC_COMMON_REFERENCE_
#define DJINTERP_RE_STD_TYPE_TRAITS_BASIC_COMMON_REFERENCE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


    // basic_common_reference
    //   trait: customization-point primary template for common_reference.
    //          Has no `type` member by default. Users specialize this
    //          template to teach common_reference about their own types.
    //          See header comment for usage and rules.
    template<typename _T,
             typename _U,
             template<typename> class _TQual,
             template<typename> class _UQual>
    struct basic_common_reference
    {};


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_BASIC_COMMON_REFERENCE_
