/******************************************************************************
* djinterp [container]                          container_overlay_concepts.hpp
*
* Overlay container concepts:
*   C++20 concepts layered over container_overlay_traits.hpp. These concepts
* provide readable constraints for overlay-like containers without replacing
* the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_overlay_traits.hpp:
*   - overlay detection via underlying_container_type
*   - overlay chain extraction utilities
*   - overlay kind classification
*   - shorthand concepts over container_overlay_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_overlay_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_OVERLAY_CONCEPTS_
#define DJINTERP_CONTAINER_OVERLAY_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_overlay_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_overlay_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   core overlay identity concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept overlay_container_type =
    is_overlay_container_v<_Type>;

template<typename _Type>
concept fundamental_container_type =
    !is_overlay_container_v<_Type>;

template<typename _Type>
concept underlying_typed_overlay_container =
    has_underlying_container_type_v<_Type>;

///////////////////////////////////////////////////////////////////////////////
// II.  overlay-kind concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept flat_overlay_container =
    ( overlay_kind_v<_Type> == DOverlayKind::flat );

template<typename _Type>
concept nested_overlay_container =
    ( overlay_kind_v<_Type> == DOverlayKind::nested );

template<typename _Type>
concept axis_overlay_container =
    ( overlay_kind_v<_Type> == DOverlayKind::axis );

///////////////////////////////////////////////////////////////////////////////
// III. chain / structure concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept single_layer_overlay_container =
    overlay_container_type<_Type> &&
    ( overlay_depth_v<_Type> == 1 );

template<typename _Type>
concept multi_layer_overlay_container =
    overlay_container_type<_Type> &&
    ( overlay_depth_v<_Type> > 1 );

template<typename _Type>
concept peelable_overlay_container =
    overlay_container_type<_Type>;

template<typename _Type>
concept leaf_resolvable_overlay_container =
    overlay_container_type<_Type>;

///////////////////////////////////////////////////////////////////////////////
// IV.  classification-based shorthand concepts
///////////////////////////////////////////////////////////////////////////////

template<typename _Type>
concept classified_overlay_container =
    container_overlay_class<_Type>::is_overlay;

template<typename _Type>
concept classified_fundamental_container =
    container_overlay_class<_Type>::is_fundamental;

template<typename _Type>
concept classified_nested_overlay_container =
    container_overlay_class<_Type>::kind == DOverlayKind::nested;

template<typename _Type>
concept classified_axis_overlay_container =
    container_overlay_class<_Type>::kind == DOverlayKind::axis;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_OVERLAY_CONCEPTS_