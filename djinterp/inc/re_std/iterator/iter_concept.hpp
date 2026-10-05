/*******************************************************************************
* djinterp [re_std]                                             iter_concept.hpp
*
* iter_concept trait header:
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
*
* path:      /inc/re_std/iterator/iter_concept.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ITERATOR_ITER_CONCEPT_HPP
#define RE_STD_ITERATOR_ITER_CONCEPT_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "./iterator_traits.hpp"

namespace re_std
{
namespace internal
{

    template<typename...> struct iter_void { typedef void type; };

    // has_iterator_concept / has_iterator_category
    template<typename Traits, typename = void>
    struct has_iterator_concept : false_type {};
    template<typename Traits>
    struct has_iterator_concept<
        Traits,
        typename iter_void<typename Traits::iterator_concept>::type>
        : true_type {};

    template<typename Traits, typename = void>
    struct has_iterator_category : false_type {};
    template<typename Traits>
    struct has_iterator_category<
        Traits,
        typename iter_void<typename Traits::iterator_category>::type>
        : true_type {};

    // iter_concept_impl
    //   trait: the three-step fallback, most specific first.
    template<typename Iter,
             bool HasConcept  = has_iterator_concept<
                                     iterator_traits<Iter> >::value,
             bool HasCategory = has_iterator_category<
                                     iterator_traits<Iter> >::value>
    struct iter_concept_impl
    { typedef typename iterator_traits<Iter>::iterator_concept type; };

    template<typename Iter>
    struct iter_concept_impl<Iter, false, true>
    { typedef typename iterator_traits<Iter>::iterator_category type; };

    //   Neither: the traits are the primary template, so I describes itself
    // and is required to model what it claims.  See the header note.
    template<typename Iter>
    struct iter_concept_impl<Iter, false, false>
    { typedef random_access_iterator_tag type; };

    // iter_concept
    //   trait: ITER_CONCEPT(I).
    template<typename Iter>
    struct iter_concept : iter_concept_impl<Iter> {};

}
}

#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_ITERATOR_ITER_CONCEPT_HPP
