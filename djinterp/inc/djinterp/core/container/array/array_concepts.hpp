/******************************************************************************
* djinterp [container]                                      array_concepts.hpp
*
* Array-specific classification concepts.
*   C++20 concepts layered on top of array_traits.hpp.  These concepts
* provide readable `requires` constraints for array-like container
* classification, including capacity model, contiguity, circular and
* chunked layout, element properties, shift/rotation capability,
* growth policy, lifetime, iterability, and preferred bulk-operation
* strategy.
*
*   This header is intentionally thin: it does not re-implement
* detection.  Instead, each concept forwards to the corresponding
* public trait or variable template from the array trait layer.
*
*   PORTABILITY:
*   The whole header is a no-op when concepts are unavailable.
* On C++17 and earlier, callers should constrain templates with
* the underlying SFINAE traits directly (e.g. via std::enable_if
* on is_contiguous_array<_Type>::value).
*
*
* path:      /inc/djinterp/core/container/array/array_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.01
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.   feature gate
2.   baseline array concepts
3.   capacity model concepts
4.   contiguity concepts
5.   circular buffer concepts
6.   chunked array concepts
7.   element concepts
8.   shift and rotation concepts
9.   growth policy concepts
10.  lifetime concepts
11.  iterability concepts
12.  strategy concepts
*/

#ifndef DJINTERP_ARRAY_CONCEPTS_
#define DJINTERP_ARRAY_CONCEPTS_ 1

// std
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "./array_traits.hpp"


// ===========================================================================
// I.   Feature Gate
// ===========================================================================
#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_DJINTERP

// ===========================================================================
// II.  Baseline Array Concepts
// ===========================================================================

// array_like
//   concept: constrains types recognized by the array trait
// layer along any of the structural axes.
template<typename _Type>
concept array_like =
    ( ( capacity_model_of_v<clean_t<_Type>> !=
        capacity_model::none )                                     ||
      is_contiguous_array_v<clean_t<_Type>>                        ||
      is_circular_buffer_v<clean_t<_Type>>                         ||
      is_chunked_array_v<clean_t<_Type>> );

// c_array_like
//   concept: constrains raw C array types.
template<typename _Type>
concept c_array_like = is_c_array_v<clean_t<_Type>>;

// std_array_like
//   concept: constrains std::array fixed-size array types.
template<typename _Type>
concept std_array_like = is_std_array_v<clean_t<_Type>>;

// static_extent_array
//   concept: constrains arrays with compile-time extent
// information.
template<typename _Type>
concept static_extent_array =
    has_static_extent_v<clean_t<_Type>>;


// ===========================================================================
// III. Capacity Model Concepts
// ===========================================================================

// fixed_capacity_array
//   concept: constrains arrays with fixed extent and no
// reserve().
template<typename _Type>
concept fixed_capacity_array =
    is_fixed_capacity_v<clean_t<_Type>>;

// dynamic_capacity_array
//   concept: constrains arrays with dynamically growable
// capacity.
template<typename _Type>
concept dynamic_capacity_array =
    is_dynamic_capacity_v<clean_t<_Type>>;

// small_buffer_array
//   concept: constrains arrays advertising inline small-
// buffer capacity.
template<typename _Type>
concept small_buffer_array =
    is_small_buffer_optimized_v<clean_t<_Type>>;

// external_storage_array
//   concept: constrains arrays whose storage is externally
// owned.
template<typename _Type>
concept external_storage_array =
    ( capacity_model_of_v<clean_t<_Type>> ==
      capacity_model::external );

// classified_array
//   concept: constrains arrays with any recognized capacity
// model.
template<typename _Type>
concept classified_array =
    ( capacity_model_of_v<clean_t<_Type>> !=
      capacity_model::none );


// ===========================================================================
// IV.  Contiguity Concepts
// ===========================================================================

// contiguous_array
//   concept: constrains arrays storing elements
// contiguously.
template<typename _Type>
concept contiguous_array =
    is_contiguous_array_v<clean_t<_Type>>;

// non_contiguous_array
//   concept: constrains array-like types without contiguous
// storage.
template<typename _Type>
concept non_contiguous_array =
    ( array_like<_Type>                                                    &&
      !contiguous_array<_Type> );


// ===========================================================================
// V.   Circular Buffer Concepts
// ===========================================================================

// circular_buffer_array
//   concept: constrains arrays exposing ring-buffer cursor
// semantics.
template<typename _Type>
concept circular_buffer_array =
    is_circular_buffer_v<clean_t<_Type>>;

// push_front_array
//   concept: constrains arrays exposing
// push_front(value_type).
template<typename _Type>
concept push_front_array =
    has_push_front_method_v<clean_t<_Type>>;

// pop_front_array
//   concept: constrains arrays exposing pop_front().
template<typename _Type>
concept pop_front_array =
    has_pop_front_method_v<clean_t<_Type>>;


// ===========================================================================
// VI.  Chunked Array Concepts
// ===========================================================================

// chunked_array
//   concept: constrains arrays organized as fixed-size
// chunks.
template<typename _Type>
concept chunked_array =
    is_chunked_array_v<clean_t<_Type>>;

// chunk_addressable_array
//   concept: constrains arrays exposing chunk_at(size_t).
template<typename _Type>
concept chunk_addressable_array =
    has_chunk_at_method_v<clean_t<_Type>>;


// ===========================================================================
// VII. Element Concepts
// ===========================================================================

// typed_array
//   concept: constrains arrays with an identifiable element
// type.
template<typename _Type>
concept typed_array =
    ( !std::is_void_v<typename
          array_element_type_of<clean_t<_Type>>::type> );

// complete_element_array
//   concept: constrains arrays whose element size is known.
template<typename _Type>
concept complete_element_array =
    ( element_size_of_v<clean_t<_Type>> > 0 );

// aligned_element_array
//   concept: constrains arrays whose element alignment is
// known.
template<typename _Type>
concept aligned_element_array =
    ( element_alignment_of_v<clean_t<_Type>> > 0 );

// strided_array
//   concept: constrains arrays with a non-zero logical
// element stride.
template<typename _Type>
concept strided_array =
    ( element_stride_of_v<clean_t<_Type>> > 0 );

// trivially_relocatable_array
//   concept: constrains arrays whose elements may be bulk-
// moved safely.
template<typename _Type>
concept trivially_relocatable_array =
    is_trivially_relocatable_array_v<clean_t<_Type>>;


// ===========================================================================
// VIII. Shift and Rotation Concepts
// ===========================================================================

// shiftable_array
//   concept: constrains arrays that are contiguous and
// sized.
template<typename _Type>
concept shiftable_array =
    is_shiftable_array_v<clean_t<_Type>>;

// shift_left_array
//   concept: constrains arrays exposing shift_left(size_t).
template<typename _Type>
concept shift_left_array =
    has_shift_left_method_v<clean_t<_Type>>;

// shift_right_array
//   concept: constrains arrays exposing shift_right(size_t).
template<typename _Type>
concept shift_right_array =
    has_shift_right_method_v<clean_t<_Type>>;

// rotatable_array
//   concept: constrains arrays exposing rotate(size_t).
template<typename _Type>
concept rotatable_array =
    has_rotate_method_v<clean_t<_Type>>;


// ===========================================================================
// IX.  Growth Policy Concepts
// ===========================================================================

// sized_array
//   concept: constrains arrays exposing size().
template<typename _Type>
concept sized_array =
    has_size_accessor_v<clean_t<_Type>>;

// capacity_array
//   concept: constrains arrays exposing capacity().
template<typename _Type>
concept capacity_array =
    has_capacity_method_v<clean_t<_Type>>;

// reservable_array
//   concept: constrains arrays exposing reserve(size_t).
template<typename _Type>
concept reservable_array =
    has_reserve_method_v<clean_t<_Type>>;

// resizable_array
//   concept: constrains arrays exposing resize(size_t).
template<typename _Type>
concept resizable_array =
    has_resize_method_v<clean_t<_Type>>;

// shrinkable_array
//   concept: constrains arrays exposing shrink_to_fit().
template<typename _Type>
concept shrinkable_array =
    has_shrink_to_fit_method_v<clean_t<_Type>>;

// growth_factor_array
//   concept: constrains arrays exposing growth-factor
// metadata.
template<typename _Type>
concept growth_factor_array =
    ( has_growth_factor_field_v<clean_t<_Type>>                    ||
      has_growth_factor_method_v<clean_t<_Type>> );


// ===========================================================================
// X.   Lifetime Concepts
// ===========================================================================

// constexpr_array
//   concept: constrains arrays whose data is reachable at
// compile time (per has_constexpr_iteration).
template<typename _Type>
concept constexpr_array =
    is_constexpr_array_v<clean_t<_Type>>;

// immutable_array
//   concept: constrains arrays that are read-only at
// runtime (no mutation entry points, no constexpr
// iteration).
template<typename _Type>
concept immutable_array =
    is_immutable_array_v<clean_t<_Type>>;

// mutable_array
//   concept: constrains arrays that expose mutation entry
// points.
template<typename _Type>
concept mutable_array =
    is_mutable_array_v<clean_t<_Type>>;


// ===========================================================================
// XI.  Iterability Concepts
// ===========================================================================

// iterable_array
//   concept: constrains arrays with begin/end iteration.
template<typename _Type>
concept iterable_array =
    is_iterable_array_v<clean_t<_Type>>;

// non_iterable_array
//   concept: constrains arrays with data()/size() but no
// iteration.
template<typename _Type>
concept non_iterable_array =
    is_non_iterable_array_v<clean_t<_Type>>;


// ===========================================================================
// XII. Strategy Concepts
// ===========================================================================

// bulk_memcpy_array
//   concept: constrains arrays preferring bulk
// memcpy/memmove operations.
template<typename _Type>
concept bulk_memcpy_array =
    ( array_strategy_v<clean_t<_Type>> ==
      array_operations_strategy::bulk_memcpy );

// element_move_array
//   concept: constrains arrays preferring element-wise
// move operations.
template<typename _Type>
concept element_move_array =
    ( array_strategy_v<clean_t<_Type>> ==
      array_operations_strategy::element_move );

// circular_strategy_array
//   concept: constrains arrays preferring circular cursor
// operations.
template<typename _Type>
concept circular_strategy_array =
    ( array_strategy_v<clean_t<_Type>> ==
      array_operations_strategy::circular );

// chunked_strategy_array
//   concept: constrains arrays preferring per-chunk
// operations.
template<typename _Type>
concept chunked_strategy_array =
    ( array_strategy_v<clean_t<_Type>> ==
      array_operations_strategy::chunked );

// generic_strategy_array
//   concept: constrains arrays using the generic fallback
// strategy.
template<typename _Type>
concept generic_strategy_array =
    ( array_strategy_v<clean_t<_Type>> ==
      array_operations_strategy::generic );


NS_END  // djinterp


#endif  // C++20 + concepts


#endif  // DJINTERP_ARRAY_CONCEPTS_
