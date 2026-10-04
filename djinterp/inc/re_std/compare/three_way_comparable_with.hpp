/******************************************************************************
* djinterp [re_std]                               three_way_comparable_with.hpp
*
* the three_way_comparable_with concept:
*   The heterogeneous form: constrains TWO types to being three-way
* comparable against each other, not merely each against itself.
*
*       template<typename T, typename U>
*           requires three_way_comparable_with<T, U, weak_ordering>
*       auto cmp(const T&, const U&);
*
*   THE COMMON-REFERENCE REQUIREMENT IS THE SUBSTANTIVE PART:
*   It is not enough that T <=> U compiles. The standard also demands that
* T and U share a common reference type, and that THAT type is itself
* three_way_comparable at the same category. Without it, a mixed
* comparison could be well-formed while meaning something inconsistent
* with either operand's own ordering -- the classic failure being a pair
* of types whose cross-comparison silently converts through a third type
* with a different notion of equivalence.
*
*   This is why the concept is written over common_reference rather than
* being a simple conjunction of the two homogeneous checks. It is also
* the reason this header is the last piece of <compare> to land: it needs
* common_reference, which was itself one of the twenty-one type_traits
* headers that existed but were never included by the module umbrella.
*
*   C++20 ONLY -- no back-port. Both `concept` and `operator<=>` are
* language features. See three_way_comparable.hpp.
*
*   THE CATEGORY ARGUMENT IS A std:: TYPE, not an re_std:: one -- the
* builtin operator<=> yields std's categories and the language will never
* produce re_std's. three_way_comparable.hpp documents the reasoning.
*
*
* path:      /inc/djinterp/re_std/compare/three_way_comparable_with.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_
#define DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_ 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

// std
#include <compare>

// re_std
#include "./three_way_comparable.hpp"
#include "../concepts/common_reference_with.hpp"
#include "../type_traits/common_reference.hpp"
#include "../type_traits/remove_reference.hpp"


NS_RESTD

    // three_way_comparable_with
    //   concept: _T and _U are mutually three-way comparable at a category
    // at least as strong as _Cat, each is three_way_comparable on its own,
    // and their common reference type is too.
    template<typename _T,
             typename _U,
             typename _Cat = ::std::partial_ordering>
    concept three_way_comparable_with
        =  three_way_comparable<_T, _Cat>
        && three_way_comparable<_U, _Cat>
        && common_reference_with<
               const typename remove_reference<_T>::type&,
               const typename remove_reference<_U>::type&>
        && three_way_comparable<
               typename common_reference<
                   const typename remove_reference<_T>::type&,
                   const typename remove_reference<_U>::type&>::type,
               _Cat>
        && internal::weakly_equality_comparable_with<_T, _U>
        && internal::partially_ordered_with<_T, _U>
        && requires(const typename remove_reference<_T>::type& _t,
                    const typename remove_reference<_U>::type& _u)
           {
               { _t <=> _u } -> internal::compares_as<_Cat>;
               { _u <=> _t } -> internal::compares_as<_Cat>;
           };

NS_END  // re_std

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // DJINTERP_RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_
