/******************************************************************************
* djinterp [container]                          container_binary_concepts.hpp
*
* Container binary concepts:
*   C++20 concepts layered over container_binary_traits.hpp. These concepts
* provide readable constraints for binary-encodable / decodable containers
* without replacing the existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* container_binary_traits.hpp:
*   - container-level encode/decode/byte_size/type-info signals
*   - element-level encodable/decodable/trivial signals
*   - bulk encode/decode signals
*   - encode/decode strategy wrappers
*   - shorthand concepts over container_binary_class<T>
*
* 
* path:      /inc/djinterp/core/container/concepts/
*                container_binary_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_BINARY_CONCEPTS_
#define DJINTERP_CONTAINER_BINARY_CONCEPTS_ 1

#ifndef __cplusplus
    #error "container_binary_concepts.hpp requires C++ compilation"
#endif

//djinterp
#include "../../djinterp.hpp"
#include "../traits/container_binary_traits.hpp"


NS_DJINTERP

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

///////////////////////////////////////////////////////////////////////////////
// I.   CONTAINER-LEVEL CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// native_binary_encodable_container
//   concept: constrains containers exposing encode().
template<typename _Type>
concept native_binary_encodable_container =
    has_encode_method_v<_Type>;

// native_binary_decodable_container
//   concept: constrains containers exposing static decode(...).
template<typename _Type>
concept native_binary_decodable_container =
    has_decode_method_v<_Type>;

// byte_sized_binary_container
//   concept: constrains containers exposing byte_size().
template<typename _Type>
concept byte_sized_binary_container =
    has_byte_size_method_v<_Type>;

// resizable_binary_container
//   concept: constrains containers exposing resize(size_t).
template<typename _Type>
concept resizable_binary_container =
    has_resize_method_v<_Type>;

// type_info_integrated_binary_container
//   concept: constrains containers integrating type metadata.
template<typename _Type>
concept type_info_integrated_binary_container =
    has_type_info_integration_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// II.  ELEMENT-LEVEL CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// trivially_copyable_element_binary_container
//   concept: constrains containers whose value_type is trivially copyable.
template<typename _Type>
concept trivially_copyable_element_binary_container =
    has_trivially_copyable_elements_v<_Type>;

// encodable_element_binary_container
//   concept: constrains containers whose elements are individually encodable.
template<typename _Type>
concept encodable_element_binary_container =
    has_encodable_elements_v<_Type>;

// decodable_element_binary_container
//   concept: constrains containers whose elements are individually decodable.
template<typename _Type>
concept decodable_element_binary_container =
    has_decodable_elements_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// III. BULK PATH CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// bulk_binary_encodable_container
//   concept: constrains containers eligible for bulk binary encoding.
template<typename _Type>
concept bulk_binary_encodable_container =
    is_bulk_encodable_v<_Type>;

// bulk_binary_decodable_container
//   concept: constrains containers eligible for bulk binary decoding.
template<typename _Type>
concept bulk_binary_decodable_container =
    is_bulk_decodable_v<_Type>;


///////////////////////////////////////////////////////////////////////////////
// IV.  STRATEGY-ORIENTED CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// strategy_binary_encodable_container
//   concept: constrains containers with any supported encode strategy.
template<typename _Type>
concept strategy_binary_encodable_container =
    is_binary_encodable_v<_Type>;

// strategy_binary_decodable_container
//   concept: constrains containers with any supported decode strategy.
template<typename _Type>
concept strategy_binary_decodable_container =
    is_binary_decodable_v<_Type>;

// round_trip_binary_container
//   concept: constrains containers supporting both encode and decode.
template<typename _Type>
concept round_trip_binary_container =
    is_binary_round_trip_v<_Type>;

// native_encode_strategy_container
//   concept: constrains containers whose best encode path is native encode().
template<typename _Type>
concept native_encode_strategy_container =
    ( container_encode_strategy_v<_Type> ==
      binary_encoding_strategy::native );

// bulk_encode_strategy_container
//   concept: constrains containers whose best encode path is bulk.
template<typename _Type>
concept bulk_encode_strategy_container =
    ( container_encode_strategy_v<_Type> ==
      binary_encoding_strategy::bulk );

// element_encode_strategy_container
//   concept: constrains containers whose best encode path is per-element.
template<typename _Type>
concept element_encode_strategy_container =
    ( container_encode_strategy_v<_Type> ==
      binary_encoding_strategy::element );

// native_decode_strategy_container
//   concept: constrains containers whose best decode path is native decode().
template<typename _Type>
concept native_decode_strategy_container =
    ( container_decode_strategy_v<_Type> ==
      binary_decoding_strategy::native );

// bulk_decode_strategy_container
//   concept: constrains containers whose best decode path is bulk.
template<typename _Type>
concept bulk_decode_strategy_container =
    ( container_decode_strategy_v<_Type> ==
      binary_decoding_strategy::bulk );

// element_decode_strategy_container
//   concept: constrains containers whose best decode path is per-element.
template<typename _Type>
concept element_decode_strategy_container =
    ( container_decode_strategy_v<_Type> ==
      binary_decoding_strategy::element );


///////////////////////////////////////////////////////////////////////////////
// V.   CLASSIFICATION-BASED SHORTHAND CONCEPTS
///////////////////////////////////////////////////////////////////////////////

// classified_binary_encodable_container
//   concept: shorthand for any type recognized by container_binary_class as
// encodable.
template<typename _Type>
concept classified_binary_encodable_container =
    container_binary_class<_Type>::is_encodable;

// classified_binary_decodable_container
//   concept: shorthand for any type recognized by container_binary_class as
// decodable.
template<typename _Type>
concept classified_binary_decodable_container =
    container_binary_class<_Type>::is_decodable;

// classified_round_trip_binary_container
//   concept: shorthand for any type recognized by container_binary_class as
// round-trip capable.
template<typename _Type>
concept classified_round_trip_binary_container =
    container_binary_class<_Type>::is_round_trip;

// fully_classified_binary_container
//   concept: binary container with explicit container-level or strategy-level
// support plus element and classification metadata.
template<typename _Type>
concept fully_classified_binary_container =
    ( container_binary_class<_Type>::is_encodable ||
      container_binary_class<_Type>::is_decodable ) &&
    ( container_binary_class<_Type>::has_encode ||
      container_binary_class<_Type>::has_decode ||
      container_binary_class<_Type>::bulk_encodable ||
      container_binary_class<_Type>::bulk_decodable );

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BINARY_CONCEPTS_