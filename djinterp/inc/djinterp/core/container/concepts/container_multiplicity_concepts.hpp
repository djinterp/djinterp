/******************************************************************************
* djinterp [container]                     container_multiplicity_concepts.hpp
*
* Multiplicity-axis concepts:
*   C++20 concepts layered over container_multiplicity_traits.hpp. These
* concepts provide readable constraints for uniqueness / multiplicity
* classification without replacing the existing SFINAE trait surface.
*   The concepts mirror the verified public trait surface from
* container_multiplicity_traits.hpp:
*   - structural unique / multi insert-return detection
*   - opt-in max_multiplicity constant detection
*   - resolved max_multiplicity extractor
*   - container_multiplicity_kind wrappers
*   - shorthand concepts over container_multiplicity_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts
*                container_multiplicity_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_
#define DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_multiplicity_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_multiplicity_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

template<typename _Type>
concept unique_insert_signaled_container =
    has_unique_insert_signal_v<_Type>;

template<typename _Type>
concept multi_insert_signaled_container =
    has_multi_insert_signal_v<_Type>;

template<typename _Type>
concept max_multiplicity_constant_container =
    has_max_multiplicity_constant_v<_Type>;

template<typename _Type>
concept known_multiplicity_container =
    ( max_multiplicity_v<_Type> != std::size_t{0} );

template<typename _Type>
concept single_multiplicity_container =
    ( max_multiplicity_v<_Type> == std::size_t{1} );

template<typename _Type>
concept bounded_multi_multiplicity_container =
    ( max_multiplicity_v<_Type> > std::size_t{1} &&
      max_multiplicity_v<_Type> <
        std::numeric_limits<std::size_t>::max() );

template<typename _Type>
concept unbounded_multi_multiplicity_container =
    ( max_multiplicity_v<_Type> ==
      std::numeric_limits<std::size_t>::max() );

template<typename _Type>
concept unique_multiplicity_container =
    ( multiplicity_kind_v<_Type> == container_multiplicity_kind::unique );

template<typename _Type>
concept bounded_multi_kind_container =
    ( multiplicity_kind_v<_Type> == container_multiplicity_kind::bounded_multi );

template<typename _Type>
concept unbounded_multi_kind_container =
    ( multiplicity_kind_v<_Type> == container_multiplicity_kind::unbounded_multi );

template<typename _Type>
concept unknown_multiplicity_container =
    ( multiplicity_kind_v<_Type> == container_multiplicity_kind::none );

template<typename _Type>
concept classified_unique_multiplicity_container =
    container_multiplicity_class<_Type>::is_unique;

template<typename _Type>
concept classified_bounded_multi_multiplicity_container =
    container_multiplicity_class<_Type>::is_bounded_multi;

template<typename _Type>
concept classified_unbounded_multi_multiplicity_container =
    container_multiplicity_class<_Type>::is_unbounded_multi;

template<typename _Type>
concept classified_known_multiplicity_container =
    !container_multiplicity_class<_Type>::is_none;

template<typename _Type>
concept structurally_multiplicity_signaled_container =
    has_unique_insert_signal_v<_Type>          ||
    has_multi_insert_signal_v<_Type>           ||
    has_max_multiplicity_constant_v<_Type>;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_MULTIPLICITY_CONCEPTS_