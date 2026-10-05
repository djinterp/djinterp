/*******************************************************************************
* djinterp [re_std]                                three_way_comparable_with.hpp
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
* path:      /inc/re_std/compare/three_way_comparable_with.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_HPP
#define RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_HPP 1

// re_std — the language-tier probe, and nothing else, before the gate
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// std
#include <compare>

// re_std
#include "./three_way_comparable.hpp"
#include "../concepts/common_reference_with.hpp"
#include "../type_traits/common_reference.hpp"
#include "../type_traits/remove_reference.hpp"


namespace re_std
{

    // three_way_comparable_with
    //   concept: T and U are mutually three-way comparable at a category
    // at least as strong as Cat, each is three_way_comparable on its own,
    // and their common reference type is too.
    template<typename T,
             typename U,
             typename Cat = ::std::partial_ordering>
    concept three_way_comparable_with
        =  three_way_comparable<T, Cat>
        && three_way_comparable<U, Cat>
        && common_reference_with<
               const typename remove_reference<T>::type&,
               const typename remove_reference<U>::type&>
        && three_way_comparable<
               typename common_reference<
                   const typename remove_reference<T>::type&,
                   const typename remove_reference<U>::type&>::type,
               Cat>
        && internal::weakly_equality_comparable_with<T, U>
        && internal::partially_ordered_with<T, U>
        && requires(const typename remove_reference<T>::type& _t,
                    const typename remove_reference<U>::type& _u)
           {
               { _t <=> _u } -> internal::compares_as<Cat>;
               { _u <=> _t } -> internal::compares_as<Cat>;
           };

}  // re_std

#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_COMPARE_THREE_WAY_COMPARABLE_WITH_HPP
