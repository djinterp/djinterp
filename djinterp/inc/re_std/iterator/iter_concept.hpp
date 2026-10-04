/******************************************************************************
* re_std [iterator]                                             iter_concept.hpp
*
*   ITER_CONCEPT - the member-typedef pull-through that decides which iterator
* concept an iterator claims.
*
*   THE THREE-STEP FALLBACK IS THE WHOLE FACILITY, and the order matters:
*
*     1. iterator_traits<I>::iterator_concept  if the traits supply one
*     2. iterator_traits<I>::iterator_category if they supply that instead
*     3. random_access_iterator_tag            if the traits are the PRIMARY
*                                              template (i.e. not specialised)
*
*   Step 3 looks reckless and is not. It applies only when iterator_traits has
* not been specialised for I at all, which means I is being described by its
* own member typedefs - and C++20 requires such a type to satisfy the concepts
* it actually models. Assuming random access there is what lets a
* newly-written iterator opt into the strongest concept without declaring a
* legacy category tag it does not otherwise need.
*
*   EXPOSITION-ONLY IN STD, so it lives in internal:: - there is no standard
* name a user may rely on.
*
*   STD IS C++20; re_std IS C++11 (void_t-based detection).
*
* path:      /inc/djinterp/re_std/iterator/iter_concept.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef DJINTERP_RE_STD_ITERATOR_ITER_CONCEPT_
#define DJINTERP_RE_STD_ITERATOR_ITER_CONCEPT_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./iterator_traits.hpp"
#include "./iterator_tags.hpp"

NS_RESTD
NS_INTERNAL

    template<typename...> struct iter_void { typedef void type; };

    // has_iterator_concept / has_iterator_category
    template<typename _Traits, typename = void>
    struct has_iterator_concept : false_type {};
    template<typename _Traits>
    struct has_iterator_concept<
        _Traits,
        typename iter_void<typename _Traits::iterator_concept>::type>
        : true_type {};

    template<typename _Traits, typename = void>
    struct has_iterator_category : false_type {};
    template<typename _Traits>
    struct has_iterator_category<
        _Traits,
        typename iter_void<typename _Traits::iterator_category>::type>
        : true_type {};

    // iter_concept_impl
    //   trait: the three-step fallback, most specific first.
    template<typename _Iter,
             bool _HasConcept  = has_iterator_concept<
                                     iterator_traits<_Iter> >::value,
             bool _HasCategory = has_iterator_category<
                                     iterator_traits<_Iter> >::value>
    struct iter_concept_impl
    { typedef typename iterator_traits<_Iter>::iterator_concept type; };

    template<typename _Iter>
    struct iter_concept_impl<_Iter, false, true>
    { typedef typename iterator_traits<_Iter>::iterator_category type; };

    //   Neither: the traits are the primary template, so I describes itself
    // and is required to model what it claims.  See the header note.
    template<typename _Iter>
    struct iter_concept_impl<_Iter, false, false>
    { typedef random_access_iterator_tag type; };

    // iter_concept
    //   trait: ITER_CONCEPT(I).
    template<typename _Iter>
    struct iter_concept : iter_concept_impl<_Iter> {};

NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_ITERATOR_ITER_CONCEPT_
