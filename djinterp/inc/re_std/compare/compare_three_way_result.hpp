/*******************************************************************************
* djinterp [re_std]                                 compare_three_way_result.hpp
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
* RE_STD_LANG_IS_CPP20_OR_HIGHER.
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
* path:      /inc/re_std/compare/compare_three_way_result.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_COMPARE_COMPARE_THREE_WAY_RESULT_HPP
#define RE_STD_COMPARE_COMPARE_THREE_WAY_RESULT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "../type_traits/void_t.hpp"
#include "../type_traits/remove_reference.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


// =============================================================================
// I.   COMPARE_THREE_WAY_RESULT
// =============================================================================

namespace internal
{

    // ctwr_impl
    //   trait: implementation hook. The primary is unconstrained
    // (no `type` member). The partial spec, gated below on C++20+,
    // supplies `type` only when (a <=> b) is well-formed.
    template<typename T, typename U, typename = void>
    struct ctwr_impl
    {};

    #if RE_STD_LANG_IS_CPP20_OR_HIGHER

        // On C++20+: detect well-formedness of `a <=> b` where a and b
        // are const lvalues of T and U.
        template<typename T, typename U>
        struct ctwr_impl<T, U,
                         void_t<
                             decltype(
                                 re_std::declval<
                                     const typename remove_reference<T>::type&
                                 >()
                                 <=>
                                 re_std::declval<
                                     const typename remove_reference<U>::type&
                                 >()
                             )
                         >>
        {
            typedef decltype(
                        re_std::declval<
                            const typename remove_reference<T>::type&
                        >()
                        <=>
                        re_std::declval<
                            const typename remove_reference<U>::type&
                        >()
                    ) type;
        };

    #endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

}  // internal


// compare_three_way_result
//   trait: thin facade over internal::ctwr_impl. The default for
// U is T per the standard (single-arg form picks the homogeneous
// comparison).
template<typename T,
         typename U = T>
struct compare_three_way_result
    : internal::ctwr_impl<T, U>
{};


// =============================================================================
// II.  COMPARE_THREE_WAY_RESULT_T (C++14+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    template<typename T,
             typename U = T>
    using compare_three_way_result_t
        = typename compare_three_way_result<T, U>::type;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_COMPARE_COMPARE_THREE_WAY_RESULT_HPP
