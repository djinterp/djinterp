/******************************************************************************
* djinterp [re_std]                                 common_comparison_category.hpp
*
* common_comparison_category meta-function header:
*   Per [cmp.common]: yields the weakest common comparison category
* among the input types. The "weakness" ranking is:
*     strong_ordering   <  weak_ordering   <  partial_ordering
* (where < means "stronger than").
*
*   RULES:
*   - If sizeof...(Ts) == 0: type is strong_ordering.
*   - If any T_i is not one of the three category types: type is void.
*   - Otherwise: type is the weakest among the T_i.
*
*     common_comparison_category<strong_ordering>::type
*       -> strong_ordering
*     common_comparison_category<strong_ordering, weak_ordering>::type
*       -> weak_ordering
*     common_comparison_category<weak_ordering, partial_ordering>::type
*       -> partial_ordering
*     common_comparison_category<int>::type
*       -> void  (int is not a category)
*
*   IMPLEMENTATION:
*   A recursive partial-spec pattern: the 0-arg case yields
* strong_ordering; the 1-arg case yields the input iff it is a
* category, else void; the N-arg case folds via an internal
* pick_weaker helper that returns the weaker of two categories or
* void if either is non-category.
*
*   PORTABILITY:
*   C++11+ (variadic templates + the three category classes).
*
*
* path:      /inc/djinterp/re_std/compare/common_comparison_category.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.17
******************************************************************************/

#ifndef DJINTERP_RE_STD_COMPARE_COMMON_COMPARISON_CATEGORY_
#define DJINTERP_RE_STD_COMPARE_COMMON_COMPARISON_CATEGORY_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if ( D_ENV_LANG_IS_CPP11_OR_HIGHER &&                                        \
      D_ENV_CPP_FEATURE_LANG_VARIADIC_TEMPLATES )


// djinterp
#include "./partial_ordering.hpp"
#include "./weak_ordering.hpp"
#include "./strong_ordering.hpp"
#include "../type_traits/integral_constant.hpp"
#include "../type_traits/conditional.hpp"
#include "../type_traits/is_same.hpp"


NS_RESTD


// =============================================================================
// I.   INTERNAL CATEGORY-DETECTION + WEAKNESS RANK
// =============================================================================

NS_INTERNAL

    // is_cmp_category
    //   trait: true iff _T is one of the three comparison category
    // classes. Used to gate the common_comparison_category trait —
    // non-category inputs yield void.
    template<typename _T>
    struct is_cmp_category
        : integral_constant<bool,
              is_same<_T, partial_ordering>::value ||
              is_same<_T, weak_ordering>::value    ||
              is_same<_T, strong_ordering>::value>
    {};


    // cmp_rank
    //   trait: weakness rank for a comparison category.
    //     strong_ordering  -> 0  (strongest)
    //     weak_ordering    -> 1
    //     partial_ordering -> 2  (weakest)
    // Higher rank = weaker. Not specialised for non-categories;
    // is_cmp_category is the gate.
    template<typename _T>
    struct cmp_rank;

    template<>
    struct cmp_rank<strong_ordering>
        : integral_constant<int, 0>
    {};

    template<>
    struct cmp_rank<weak_ordering>
        : integral_constant<int, 1>
    {};

    template<>
    struct cmp_rank<partial_ordering>
        : integral_constant<int, 2>
    {};


    // pick_weaker
    //   trait: returns the weaker of two categories, or void if
    // either argument is void (sticky-void propagation).
    template<typename _T, typename _U>
    struct pick_weaker
    {
        typedef typename conditional<
                              (cmp_rank<_T>::value >= cmp_rank<_U>::value),
                              _T,
                              _U
                          >::type type;
    };

    // void as either operand: result is void.
    template<typename _T>
    struct pick_weaker<void, _T>
    {
        typedef void type;
    };

    template<typename _T>
    struct pick_weaker<_T, void>
    {
        typedef void type;
    };

    template<>
    struct pick_weaker<void, void>
    {
        typedef void type;
    };

NS_END  // internal


// =============================================================================
// II.  COMMON_COMPARISON_CATEGORY
// =============================================================================

// Primary template — undefined; matched by the partial specs below.
template<typename... _Ts>
struct common_comparison_category;

// Empty pack: result is strong_ordering per [cmp.common]/3.
template<>
struct common_comparison_category<>
{
    typedef strong_ordering type;
};

// One-arg: result is _T iff _T is a category, else void.
template<typename _T>
struct common_comparison_category<_T>
{
    typedef typename conditional<
                          internal::is_cmp_category<_T>::value,
                          _T,
                          void
                      >::type type;
};

// N-arg (N >= 2): recursive fold via pick_weaker.
template<typename _T,
         typename... _Rest>
struct common_comparison_category<_T, _Rest...>
{
    typedef typename internal::pick_weaker<
                          typename common_comparison_category<_T>::type,
                          typename common_comparison_category<_Rest...>::type
                      >::type type;
};


// =============================================================================
// III. COMMON_COMPARISON_CATEGORY_T (C++14+ alias)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_ALIAS_TEMPLATES

    template<typename... _Ts>
    using common_comparison_category_t
        = typename common_comparison_category<_Ts...>::type;

#endif


NS_END  // re_std


#endif  // C++11+ && variadic templates


#endif  // DJINTERP_RE_STD_COMPARE_COMMON_COMPARISON_CATEGORY_
