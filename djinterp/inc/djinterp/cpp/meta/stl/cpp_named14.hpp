/*******************************************************************************
* djinterp [cpp]                                                 cpp_named14.hpp
*
* C++14 named requirement type traits:
*   Extends the C++11 named requirement traits with variable template
* helpers (the `_v` suffix convention). These provide a more concise syntax
* for querying trait values without the `::value` member access.
*
* ADDITIONS OVER C++11:
*   - is_default_constructible_v<T>            (variable template)
*   - is_move_constructible_v<T>               (variable template)
*   - is_copy_constructible_v<T>               (variable template)
*   - is_move_assignable_v<T>                  (variable template)
*   - is_copy_assignable_v<T>                  (variable template)
*   - is_destructible_v<T>                     (variable template)
*   - is_nothrow_*_v<T> variants               (variable templates)
*   - is_scalar_v<T>                           (variable template)
*   - is_trivially_copyable_v<T>               (variable template)
*   - is_trivial_v<T>                          (variable template)
*   - is_standard_layout_v<T>                  (variable template)
*   - is_pod_v<T>                              (variable template)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES   - _v helper availability
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES    - move trait availability
*
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named14.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED14_HPP
#define DJINTERP_META_STL_CPP_NAMED14_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named14.hpp can only be used in C++ compilation mode"
#endif

// include C++11 layer
#include "cpp_named11.hpp"

// C++14 additions are active for C++14 and all later standards
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   VARIABLE TEMPLATE HELPERS (C++14)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_default_constructible_v
//   value: shorthand for is_default_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_default_constructible_v =
    is_default_constructible<T>::value;

// is_copy_constructible_v
//   value: shorthand for is_copy_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_copy_constructible_v =
    is_copy_constructible<T>::value;

// is_copy_assignable_v
//   value: shorthand for is_copy_assignable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_copy_assignable_v =
    is_copy_assignable<T>::value;

// is_destructible_v
//   value: shorthand for is_destructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_destructible_v =
    is_destructible<T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_constructible_v
//   value: shorthand for is_move_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_move_constructible_v =
    is_move_constructible<T>::value;

// is_move_assignable_v
//   value: shorthand for is_move_assignable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_move_assignable_v =
    is_move_assignable<T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// II.  NOTHROW VARIABLE TEMPLATE HELPERS (C++14)
// =========================================================================

// is_nothrow_default_constructible_v
//   value: shorthand for is_nothrow_default_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_default_constructible_v =
    is_nothrow_default_constructible<T>::value;

// is_nothrow_copy_constructible_v
//   value: shorthand for is_nothrow_copy_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_copy_constructible_v =
    is_nothrow_copy_constructible<T>::value;

// is_nothrow_copy_assignable_v
//   value: shorthand for is_nothrow_copy_assignable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_copy_assignable_v =
    is_nothrow_copy_assignable<T>::value;

// is_nothrow_destructible_v
//   value: shorthand for is_nothrow_destructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_destructible_v =
    is_nothrow_destructible<T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_nothrow_move_constructible_v
//   value: shorthand for is_nothrow_move_constructible<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_move_constructible_v =
    is_nothrow_move_constructible<T>::value;

// is_nothrow_move_assignable_v
//   value: shorthand for is_nothrow_move_assignable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nothrow_move_assignable_v =
    is_nothrow_move_assignable<T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. TYPE CLASSIFICATION _v HELPERS (C++14)
// =========================================================================

// is_scalar_v
//   value: shorthand for is_scalar<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_scalar_v =
    is_scalar<T>::value;

// is_trivially_copyable_v
//   value: shorthand for is_trivially_copyable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_trivially_copyable_v =
    is_trivially_copyable<T>::value;

// is_trivial_v
//   value: shorthand for is_trivial<T>::value.
// note: TrivialType is deprecated in C++26.
template<typename T>
D_INLINE_VAR constexpr bool is_trivial_v =
    is_trivial<T>::value;

// is_standard_layout_v
//   value: shorthand for is_standard_layout<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_standard_layout_v =
    is_standard_layout<T>::value;

// is_pod_v
//   value: shorthand for is_pod<T>::value.
// note: PODType is deprecated in C++20; std::is_pod removed in C++26.
template<typename T>
D_INLINE_VAR constexpr bool is_pod_v =
    is_pod<T>::value;


// =========================================================================
// IV.  COMPARISON AND BOOLEAN _v HELPERS (C++14)
// =========================================================================

// is_boolean_testable_v
//   value: shorthand for is_boolean_testable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_boolean_testable_v =
    is_boolean_testable<T>::value;

// is_equality_comparable_v
//   value: shorthand for is_equality_comparable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_equality_comparable_v =
    is_equality_comparable<T>::value;

// is_less_than_comparable_v
//   value: shorthand for is_less_than_comparable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_less_than_comparable_v =
    is_less_than_comparable<T>::value;

// is_swappable_v
//   value: shorthand for is_swappable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_swappable_v =
    is_swappable<T>::value;

// is_nullable_pointer_v
//   value: shorthand for is_nullable_pointer<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_nullable_pointer_v =
    is_nullable_pointer<T>::value;

// is_hashable_v
//   value: shorthand for is_hashable<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_hashable_v =
    is_hashable<T>::value;

// is_allocator_v
//   value: shorthand for is_allocator<A>::value.
template<typename A>
D_INLINE_VAR constexpr bool is_allocator_v =
    is_allocator<A>::value;

// is_function_object_v
//   value: shorthand for is_function_object<F>::value.
template<typename F>
D_INLINE_VAR constexpr bool is_function_object_v =
    is_function_object<F>::value;

// is_callable_v
//   value: shorthand for is_callable<F, Args...>::value.
template<typename F,
         typename... Args>
D_INLINE_VAR constexpr bool is_callable_v =
    is_callable<F, Args...>::value;

// is_predicate_v
//   value: shorthand for is_predicate<F, Arg>::value.
template<typename F,
         typename Arg>
D_INLINE_VAR constexpr bool is_predicate_v =
    is_predicate<F, Arg>::value;

// is_binary_predicate_v
//   value: shorthand for is_binary_predicate<F, Arg1, Arg2>::value.
template<typename F,
         typename Arg1,
         typename Arg2>
D_INLINE_VAR constexpr bool is_binary_predicate_v =
    is_binary_predicate<F, Arg1, Arg2>::value;

// is_compare_v
//   value: shorthand for is_compare<F, T>::value.
template<typename F,
         typename T>
D_INLINE_VAR constexpr bool is_compare_v =
    is_compare<F, T>::value;


// =========================================================================
// V.   CONTAINER _v HELPERS (C++14)
// =========================================================================

// is_container_v
//   value: shorthand for is_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_container_v =
    is_container<C>::value;

// is_reversible_container_v
//   value: shorthand for is_reversible_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_reversible_container_v =
    is_reversible_container<C>::value;

// is_allocator_aware_container_v
//   value: shorthand for is_allocator_aware_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_allocator_aware_container_v =
    is_allocator_aware_container<C>::value;

// is_sequence_container_v
//   value: shorthand for is_sequence_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_sequence_container_v =
    is_sequence_container<C>::value;

// is_associative_container_v
//   value: shorthand for is_associative_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_associative_container_v =
    is_associative_container<C>::value;

// is_unordered_associative_container_v
//   value: shorthand for
// is_unordered_associative_container<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_unordered_associative_container_v =
    is_unordered_associative_container<C>::value;


// =========================================================================
// VI.  CONTAINER ELEMENT _v HELPERS (C++14)
// =========================================================================

// is_default_insertable_v
//   value: shorthand for is_default_insertable<T, A>::value.
template<typename T,
         typename A = std::allocator<T>>
D_INLINE_VAR constexpr bool is_default_insertable_v =
    is_default_insertable<T, A>::value;

// is_copy_insertable_v
//   value: shorthand for is_copy_insertable<T, A>::value.
template<typename T,
         typename A = std::allocator<T>>
D_INLINE_VAR constexpr bool is_copy_insertable_v =
    is_copy_insertable<T, A>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_insertable_v
//   value: shorthand for is_move_insertable<T, A>::value.
template<typename T,
         typename A = std::allocator<T>>
D_INLINE_VAR constexpr bool is_move_insertable_v =
    is_move_insertable<T, A>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_emplace_constructible_v
//   value: shorthand for
// is_emplace_constructible<T, A, Args...>::value.
template<typename T,
         typename A,
         typename... Args>
D_INLINE_VAR constexpr bool is_emplace_constructible_v =
    is_emplace_constructible<T, A, Args...>::value;

// is_erasable_v
//   value: shorthand for is_erasable<T, A>::value.
template<typename T,
         typename A = std::allocator<T>>
D_INLINE_VAR constexpr bool is_erasable_v =
    is_erasable<T, A>::value;


// =========================================================================
// VII. ITERATOR _v HELPERS (C++14)
// =========================================================================

// is_legacy_iterator_v
//   value: shorthand for is_legacy_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_iterator_v =
    is_legacy_iterator<I>::value;

// is_legacy_input_iterator_v
//   value: shorthand for is_legacy_input_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_input_iterator_v =
    is_legacy_input_iterator<I>::value;

// is_legacy_output_iterator_v
//   value: shorthand for is_legacy_output_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_output_iterator_v =
    is_legacy_output_iterator<I>::value;

// is_legacy_forward_iterator_v
//   value: shorthand for is_legacy_forward_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_forward_iterator_v =
    is_legacy_forward_iterator<I>::value;

// is_legacy_bidirectional_iterator_v
//   value: shorthand for is_legacy_bidirectional_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_bidirectional_iterator_v =
    is_legacy_bidirectional_iterator<I>::value;

// is_legacy_random_access_iterator_v
//   value: shorthand for is_legacy_random_access_iterator<I>::value.
template<typename I>
D_INLINE_VAR constexpr bool is_legacy_random_access_iterator_v =
    is_legacy_random_access_iterator<I>::value;


// =========================================================================
// VIII. STREAM I/O _v HELPERS (C++14)
// =========================================================================

// is_unformatted_input_stream_v
//   value: shorthand for is_unformatted_input_stream<S>::value.
template<typename S>
D_INLINE_VAR constexpr bool is_unformatted_input_stream_v =
    is_unformatted_input_stream<S>::value;

// is_formatted_input_stream_v
//   value: shorthand for is_formatted_input_stream<S>::value.
template<typename S>
D_INLINE_VAR constexpr bool is_formatted_input_stream_v =
    is_formatted_input_stream<S>::value;

// is_unformatted_output_stream_v
//   value: shorthand for is_unformatted_output_stream<S>::value.
template<typename S>
D_INLINE_VAR constexpr bool is_unformatted_output_stream_v =
    is_unformatted_output_stream<S>::value;

// is_formatted_output_stream_v
//   value: shorthand for is_formatted_output_stream<S>::value.
template<typename S>
D_INLINE_VAR constexpr bool is_formatted_output_stream_v =
    is_formatted_output_stream<S>::value;


// =========================================================================
// IX.  RANDOM NUMBER GENERATION _v HELPERS (C++14)
// =========================================================================

// is_seed_sequence_v
//   value: shorthand for is_seed_sequence<S>::value.
template<typename S>
D_INLINE_VAR constexpr bool is_seed_sequence_v =
    is_seed_sequence<S>::value;

// is_uniform_random_bit_generator_v
//   value: shorthand for
// is_uniform_random_bit_generator<G>::value.
template<typename G>
D_INLINE_VAR constexpr bool is_uniform_random_bit_generator_v =
    is_uniform_random_bit_generator<G>::value;

// is_random_number_engine_v
//   value: shorthand for is_random_number_engine<E>::value.
template<typename E>
D_INLINE_VAR constexpr bool is_random_number_engine_v =
    is_random_number_engine<E>::value;

// is_random_number_engine_adaptor_v
//   value: shorthand for
// is_random_number_engine_adaptor<E>::value.
template<typename E>
D_INLINE_VAR constexpr bool is_random_number_engine_adaptor_v =
    is_random_number_engine_adaptor<E>::value;

// is_random_number_distribution_v
//   value: shorthand for
// is_random_number_distribution<D>::value.
template<typename D>
D_INLINE_VAR constexpr bool is_random_number_distribution_v =
    is_random_number_distribution<D>::value;


// =========================================================================
// X.   CONCURRENCY _v HELPERS (C++14)
// =========================================================================

// is_basic_lockable_v
//   value: shorthand for is_basic_lockable<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_basic_lockable_v =
    is_basic_lockable<M>::value;

// is_lockable_v
//   value: shorthand for is_lockable<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_lockable_v =
    is_lockable<M>::value;

// is_timed_lockable_v
//   value: shorthand for is_timed_lockable<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_timed_lockable_v =
    is_timed_lockable<M>::value;

// is_shared_lockable_v
//   value: shorthand for is_shared_lockable<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_shared_lockable_v =
    is_shared_lockable<M>::value;

// is_shared_timed_lockable_v
//   value: shorthand for is_shared_timed_lockable<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_shared_timed_lockable_v =
    is_shared_timed_lockable<M>::value;

// is_mutex_v
//   value: shorthand for is_mutex<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_mutex_v =
    is_mutex<M>::value;

// is_timed_mutex_v
//   value: shorthand for is_timed_mutex<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_timed_mutex_v =
    is_timed_mutex<M>::value;

// is_shared_mutex_v
//   value: shorthand for is_shared_mutex<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_shared_mutex_v =
    is_shared_mutex<M>::value;

// is_shared_timed_mutex_v
//   value: shorthand for is_shared_timed_mutex<M>::value.
template<typename M>
D_INLINE_VAR constexpr bool is_shared_timed_mutex_v =
    is_shared_timed_mutex<M>::value;


// =========================================================================
// XI.  TYPE TRAIT META-TRAIT _v HELPERS (C++14)
// =========================================================================

// is_unary_type_trait_v
//   value: shorthand for is_unary_type_trait<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_unary_type_trait_v =
    is_unary_type_trait<T>::value;

// is_binary_type_trait_v
//   value: shorthand for is_binary_type_trait<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_binary_type_trait_v =
    is_binary_type_trait<T>::value;

// is_transformation_trait_v
//   value: shorthand for is_transformation_trait<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_transformation_trait_v =
    is_transformation_trait<T>::value;


// =========================================================================
// XII. CLOCK, CHARTRAITS, MISC _v HELPERS (C++14)
// =========================================================================

// is_clock_v
//   value: shorthand for is_clock<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_clock_v =
    is_clock<C>::value;

// is_trivial_clock_v
//   value: shorthand for is_trivial_clock<C>::value.
template<typename C>
D_INLINE_VAR constexpr bool is_trivial_clock_v =
    is_trivial_clock<C>::value;

// is_char_traits_v
//   value: shorthand for is_char_traits<Tr>::value.
template<typename Tr>
D_INLINE_VAR constexpr bool is_char_traits_v =
    is_char_traits<Tr>::value;

// is_bitmask_type_v
//   value: shorthand for is_bitmask_type<B>::value.
template<typename B>
D_INLINE_VAR constexpr bool is_bitmask_type_v =
    is_bitmask_type<B>::value;

// is_numeric_type_v
//   value: shorthand for is_numeric_type<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_numeric_type_v =
    is_numeric_type<T>::value;

// is_regex_traits_v
//   value: shorthand for is_regex_traits<Tr>::value.
template<typename Tr>
D_INLINE_VAR constexpr bool is_regex_traits_v =
    is_regex_traits<Tr>::value;

// is_literal_type_v_
//   value: shorthand for is_literal_type_<T>::value.
template<typename T>
D_INLINE_VAR constexpr bool is_literal_type_v_ =
    is_literal_type_<T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED14_HPP
