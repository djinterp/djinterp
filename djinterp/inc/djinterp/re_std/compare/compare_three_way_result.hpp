/******************************************************************************
* djinterp [restd]                                  compare_three_way_result.hpp
*
* compare_three_way_result trait header:
*   Per [cmp.result]: yields the type of `t <=> u` where t and u are
* const lvalues of remove_reference_t<T> and remove_reference_t<U>
* respectively, when that expression is well-formed. Otherwise the
* trait has no `type` member (SFINAE-friendly absence).
*
*     compare_three_way_result<int>::type            -> strong_ordering
*     compare_three_way_result<double>::type         -> partial_ordering
*     compare_three_way_result<int, double>::type    -> partial_ordering
*     compare_three_way_result<void(*)()>::type      -> std::strong_ordering
*
*   PORTABILITY:
*   The trait struct is shipped on C++11+ but only has a `type`
* member on C++20+ (where the operator<=> language feature exists).
* On C++11-17, the trait is intentionally ill-formed-when-used:
* there is no <=> expression to take the decltype of. Code that
* needs the trait on lower tiers must guard with
* D_ENV_LANG_IS_CPP20_OR_HIGHER.
*
*   The detection uses the void_t / SFINAE-partial-spec idiom:
* the unconstrained primary has no `type`; the void_t-anchored
* specialisation (gated on C++20+) supplies `type` only when the
* <=> expression is well-formed.
*
*   The const-lvalue framing in the standard means that the trait
* takes the cv/ref properties of T and U into account in a specific
* way: `const remove_reference_t<T>&` is the comparison operand type.
* Rvalue inputs are converted to const-lvalues; cv-qualifiers on the
* reference are preserved.
*
*
* path:      /inc/djinterp/re_std/compare/compare_three_way_result.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_RESULT_
#define DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_RESULT_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "../type_traits/void_t.hpp"
#include "../type_traits/remove_reference.hpp"
#include "../utility/declval.hpp"


NS_RESTD


// =============================================================================
// I.   COMPARE_THREE_WAY_RESULT
// =============================================================================

NS_INTERNAL

    // ctwr_impl
    //   trait: implementation hook. The primary is unconstrained
    // (no `type` member). The partial spec, gated below on C++20+,
    // supplies `type` only when (a <=> b) is well-formed.
    template<typename _T, typename _U, typename = void>
    struct ctwr_impl
    {};

    #if D_ENV_LANG_IS_CPP20_OR_HIGHER

        // On C++20+: detect well-formedness of `a <=> b` where a and b
        // are const lvalues of T and U.
        template<typename _T, typename _U>
        struct ctwr_impl<_T, _U,
                         typename void_t<
                             decltype(
                                 restd::declval<
                                     const typename remove_reference<_T>::type&
                                 >()
                                 <=>
                                 restd::declval<
                                     const typename remove_reference<_U>::type&
                                 >()
                             )
                         >::type>
        {
            typedef decltype(
                        restd::declval<
                            const typename remove_reference<_T>::type&
                        >()
                        <=>
                        restd::declval<
                            const typename remove_reference<_U>::type&
                        >()
                    ) type;
        };

    #endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

NS_END  // internal


// compare_three_way_result
//   trait: thin facade over internal::ctwr_impl. The default for
// _U is _T per the standard (single-arg form picks the homogeneous
// comparison).
template<typename _T,
         typename _U = _T>
struct compare_three_way_result
    : internal::ctwr_impl<_T, _U>
{};


// =============================================================================
// II.  COMPARE_THREE_WAY_RESULT_T (C++14+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<typename _T,
             typename _U = _T>
    using compare_three_way_result_t
        = typename compare_three_way_result<_T, _U>::type;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_COMPARE_COMPARE_THREE_WAY_RESULT_
