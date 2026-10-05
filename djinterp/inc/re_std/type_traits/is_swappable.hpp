/*******************************************************************************
* djinterp [re_std]                                             is_swappable.hpp
*
* is_swappable trait:
*   true_type if objects of type Type can be swapped with each other via
* the standard swap protocol, false_type otherwise. Equivalent (per the C++
* standard) to is_swappable_with<Type&, Type&> for referenceable types,
* and false_type for non-referenceable types (cv void, cv- or ref-qualified
* function types).
*
*   IMPLEMENTATION:
*   This trait simply forwards add_lvalue_reference<Type>::type on both
* sides of is_swappable_with. The forwarding handles non-referenceable types
* automatically: add_lvalue_reference leaves cv void and cv-qualified
* function types as-is (per [meta.trans.ref]), and is_swappable_with<void,
* void> is false_type because swap() cannot be called with void arguments
* (SFINAE rejects). This means is_swappable<void>, is_swappable<int() const>,
* etc. all correctly evaluate to false_type without any explicit gating.
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait, matching
* is_swappable_with.
*
*   DEPENDENCIES:
*   is_swappable_with (transitive: requires re_std::swap, re_std::declval).
*
*
* path:      /inc/re_std/type_traits/is_swappable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_SWAPPABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_SWAPPABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./is_swappable_with.hpp"
#include "./add_lvalue_reference.hpp"


namespace re_std
{


    // is_swappable
    //   trait: true_type if Type is swappable with itself (lvalue-to-lvalue),
    //          false_type otherwise. Non-referenceable types (cv void,
    //          cv/ref-qualified functions) yield false_type via SFINAE
    //          inside is_swappable_with.
    template<typename Type>
    struct is_swappable
        : is_swappable_with<
              typename add_lvalue_reference<Type>::type,
              typename add_lvalue_reference<Type>::type >
    {};


    // is_swappable_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename Type>
        RE_STD_CONSTEXPR bool is_swappable_v = is_swappable<Type>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_SWAPPABLE_HPP
