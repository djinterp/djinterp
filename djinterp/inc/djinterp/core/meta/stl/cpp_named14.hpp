/******************************************************************************
* djinterp [core]                                              cpp_named14.h
*
* C++14 named requirement type traits:
*   Extends the C++11 named requirement traits with variable template
* helpers (the `_v` suffix convention). These provide a more concise syntax
* for querying trait values without the `::value` member access.
*
* ADDITIONS OVER C++11:
*   - is_default_constructible_v<_T>            (variable template)
*   - is_move_constructible_v<_T>               (variable template)
*   - is_copy_constructible_v<_T>               (variable template)
*   - is_move_assignable_v<_T>                  (variable template)
*   - is_copy_assignable_v<_T>                  (variable template)
*   - is_destructible_v<_T>                     (variable template)
*   - is_nothrow_*_v<_T> variants               (variable templates)
*   - is_scalar_v<_T>                           (variable template)
*   - is_trivially_copyable_v<_T>               (variable template)
*   - is_trivial_v<_T>                          (variable template)
*   - is_standard_layout_v<_T>                  (variable template)
*   - is_pod_v<_T>                              (variable template)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES   - _v helper availability
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES    - move trait availability
*
* path:      \inc\cpp_named14.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED14_
#define DJINTERP_CPP_NAMED14_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named14.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named14.h can only be used in C++ compilation mode"
#endif

// include C++11 layer
#include "cpp_named11.h"

// C++14 additions are active for C++14 and all later standards
#if D_ENV_LANG_IS_CPP14_OR_HIGHER


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   VARIABLE TEMPLATE HELPERS (C++14)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

// is_default_constructible_v
//   value: shorthand for is_default_constructible<_T>::value.
template<typename _T>
constexpr bool is_default_constructible_v =
    is_default_constructible<_T>::value;

// is_copy_constructible_v
//   value: shorthand for is_copy_constructible<_T>::value.
template<typename _T>
constexpr bool is_copy_constructible_v =
    is_copy_constructible<_T>::value;

// is_copy_assignable_v
//   value: shorthand for is_copy_assignable<_T>::value.
template<typename _T>
constexpr bool is_copy_assignable_v =
    is_copy_assignable<_T>::value;

// is_destructible_v
//   value: shorthand for is_destructible<_T>::value.
template<typename _T>
constexpr bool is_destructible_v =
    is_destructible<_T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_constructible_v
//   value: shorthand for is_move_constructible<_T>::value.
template<typename _T>
constexpr bool is_move_constructible_v =
    is_move_constructible<_T>::value;

// is_move_assignable_v
//   value: shorthand for is_move_assignable<_T>::value.
template<typename _T>
constexpr bool is_move_assignable_v =
    is_move_assignable<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// II.  NOTHROW VARIABLE TEMPLATE HELPERS (C++14)
// =========================================================================

// is_nothrow_default_constructible_v
//   value: shorthand for is_nothrow_default_constructible<_T>::value.
template<typename _T>
constexpr bool is_nothrow_default_constructible_v =
    is_nothrow_default_constructible<_T>::value;

// is_nothrow_copy_constructible_v
//   value: shorthand for is_nothrow_copy_constructible<_T>::value.
template<typename _T>
constexpr bool is_nothrow_copy_constructible_v =
    is_nothrow_copy_constructible<_T>::value;

// is_nothrow_copy_assignable_v
//   value: shorthand for is_nothrow_copy_assignable<_T>::value.
template<typename _T>
constexpr bool is_nothrow_copy_assignable_v =
    is_nothrow_copy_assignable<_T>::value;

// is_nothrow_destructible_v
//   value: shorthand for is_nothrow_destructible<_T>::value.
template<typename _T>
constexpr bool is_nothrow_destructible_v =
    is_nothrow_destructible<_T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_nothrow_move_constructible_v
//   value: shorthand for is_nothrow_move_constructible<_T>::value.
template<typename _T>
constexpr bool is_nothrow_move_constructible_v =
    is_nothrow_move_constructible<_T>::value;

// is_nothrow_move_assignable_v
//   value: shorthand for is_nothrow_move_assignable<_T>::value.
template<typename _T>
constexpr bool is_nothrow_move_assignable_v =
    is_nothrow_move_assignable<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. TYPE CLASSIFICATION _v HELPERS (C++14)
// =========================================================================

// is_scalar_v
//   value: shorthand for is_scalar<_T>::value.
template<typename _T>
constexpr bool is_scalar_v =
    is_scalar<_T>::value;

// is_trivially_copyable_v
//   value: shorthand for is_trivially_copyable<_T>::value.
template<typename _T>
constexpr bool is_trivially_copyable_v =
    is_trivially_copyable<_T>::value;

// is_trivial_v
//   value: shorthand for is_trivial<_T>::value.
// note: TrivialType is deprecated in C++26.
template<typename _T>
constexpr bool is_trivial_v =
    is_trivial<_T>::value;

// is_standard_layout_v
//   value: shorthand for is_standard_layout<_T>::value.
template<typename _T>
constexpr bool is_standard_layout_v =
    is_standard_layout<_T>::value;

// is_pod_v
//   value: shorthand for is_pod<_T>::value.
// note: PODType is deprecated in C++20; std::is_pod removed in C++26.
template<typename _T>
constexpr bool is_pod_v =
    is_pod<_T>::value;


// =========================================================================
// IV.  COMPARISON AND BOOLEAN _v HELPERS (C++14)
// =========================================================================

// is_boolean_testable_v
//   value: shorthand for is_boolean_testable<_T>::value.
template<typename _T>
constexpr bool is_boolean_testable_v =
    is_boolean_testable<_T>::value;

// is_equality_comparable_v
//   value: shorthand for is_equality_comparable<_T>::value.
template<typename _T>
constexpr bool is_equality_comparable_v =
    is_equality_comparable<_T>::value;

// is_less_than_comparable_v
//   value: shorthand for is_less_than_comparable<_T>::value.
template<typename _T>
constexpr bool is_less_than_comparable_v =
    is_less_than_comparable<_T>::value;

// is_swappable_v
//   value: shorthand for is_swappable<_T>::value.
template<typename _T>
constexpr bool is_swappable_v =
    is_swappable<_T>::value;

// is_nullable_pointer_v
//   value: shorthand for is_nullable_pointer<_T>::value.
template<typename _T>
constexpr bool is_nullable_pointer_v =
    is_nullable_pointer<_T>::value;

// is_hashable_v
//   value: shorthand for is_hashable<_T>::value.
template<typename _T>
constexpr bool is_hashable_v =
    is_hashable<_T>::value;

// is_allocator_v
//   value: shorthand for is_allocator<_A>::value.
template<typename _A>
constexpr bool is_allocator_v =
    is_allocator<_A>::value;

// is_function_object_v
//   value: shorthand for is_function_object<_F>::value.
template<typename _F>
constexpr bool is_function_object_v =
    is_function_object<_F>::value;

// is_callable_v
//   value: shorthand for is_callable<_F, _Args...>::value.
template<typename _F,
         typename... _Args>
constexpr bool is_callable_v =
    is_callable<_F, _Args...>::value;

// is_predicate_v
//   value: shorthand for is_predicate<_F, _Arg>::value.
template<typename _F,
         typename _Arg>
constexpr bool is_predicate_v =
    is_predicate<_F, _Arg>::value;

// is_binary_predicate_v
//   value: shorthand for is_binary_predicate<_F, _Arg1, _Arg2>::value.
template<typename _F,
         typename _Arg1,
         typename _Arg2>
constexpr bool is_binary_predicate_v =
    is_binary_predicate<_F, _Arg1, _Arg2>::value;

// is_compare_v
//   value: shorthand for is_compare<_F, _T>::value.
template<typename _F,
         typename _T>
constexpr bool is_compare_v =
    is_compare<_F, _T>::value;


// =========================================================================
// V.   CONTAINER _v HELPERS (C++14)
// =========================================================================

// is_container_v
//   value: shorthand for is_container<_C>::value.
template<typename _C>
constexpr bool is_container_v =
    is_container<_C>::value;

// is_reversible_container_v
//   value: shorthand for is_reversible_container<_C>::value.
template<typename _C>
constexpr bool is_reversible_container_v =
    is_reversible_container<_C>::value;

// is_allocator_aware_container_v
//   value: shorthand for is_allocator_aware_container<_C>::value.
template<typename _C>
constexpr bool is_allocator_aware_container_v =
    is_allocator_aware_container<_C>::value;

// is_sequence_container_v
//   value: shorthand for is_sequence_container<_C>::value.
template<typename _C>
constexpr bool is_sequence_container_v =
    is_sequence_container<_C>::value;

// is_associative_container_v
//   value: shorthand for is_associative_container<_C>::value.
template<typename _C>
constexpr bool is_associative_container_v =
    is_associative_container<_C>::value;

// is_unordered_associative_container_v
//   value: shorthand for
// is_unordered_associative_container<_C>::value.
template<typename _C>
constexpr bool is_unordered_associative_container_v =
    is_unordered_associative_container<_C>::value;


// =========================================================================
// VI.  CONTAINER ELEMENT _v HELPERS (C++14)
// =========================================================================

// is_default_insertable_v
//   value: shorthand for is_default_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
constexpr bool is_default_insertable_v =
    is_default_insertable<_T, _A>::value;

// is_copy_insertable_v
//   value: shorthand for is_copy_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
constexpr bool is_copy_insertable_v =
    is_copy_insertable<_T, _A>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_insertable_v
//   value: shorthand for is_move_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
constexpr bool is_move_insertable_v =
    is_move_insertable<_T, _A>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_emplace_constructible_v
//   value: shorthand for
// is_emplace_constructible<_T, _A, _Args...>::value.
template<typename _T,
         typename _A,
         typename... _Args>
constexpr bool is_emplace_constructible_v =
    is_emplace_constructible<_T, _A, _Args...>::value;

// is_erasable_v
//   value: shorthand for is_erasable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
constexpr bool is_erasable_v =
    is_erasable<_T, _A>::value;


// =========================================================================
// VII. ITERATOR _v HELPERS (C++14)
// =========================================================================

// is_legacy_iterator_v
//   value: shorthand for is_legacy_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_iterator_v =
    is_legacy_iterator<_I>::value;

// is_legacy_input_iterator_v
//   value: shorthand for is_legacy_input_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_input_iterator_v =
    is_legacy_input_iterator<_I>::value;

// is_legacy_output_iterator_v
//   value: shorthand for is_legacy_output_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_output_iterator_v =
    is_legacy_output_iterator<_I>::value;

// is_legacy_forward_iterator_v
//   value: shorthand for is_legacy_forward_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_forward_iterator_v =
    is_legacy_forward_iterator<_I>::value;

// is_legacy_bidirectional_iterator_v
//   value: shorthand for is_legacy_bidirectional_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_bidirectional_iterator_v =
    is_legacy_bidirectional_iterator<_I>::value;

// is_legacy_random_access_iterator_v
//   value: shorthand for is_legacy_random_access_iterator<_I>::value.
template<typename _I>
constexpr bool is_legacy_random_access_iterator_v =
    is_legacy_random_access_iterator<_I>::value;


// =========================================================================
// VIII. STREAM I/O _v HELPERS (C++14)
// =========================================================================

// is_unformatted_input_stream_v
//   value: shorthand for is_unformatted_input_stream<_S>::value.
template<typename _S>
constexpr bool is_unformatted_input_stream_v =
    is_unformatted_input_stream<_S>::value;

// is_formatted_input_stream_v
//   value: shorthand for is_formatted_input_stream<_S>::value.
template<typename _S>
constexpr bool is_formatted_input_stream_v =
    is_formatted_input_stream<_S>::value;

// is_unformatted_output_stream_v
//   value: shorthand for is_unformatted_output_stream<_S>::value.
template<typename _S>
constexpr bool is_unformatted_output_stream_v =
    is_unformatted_output_stream<_S>::value;

// is_formatted_output_stream_v
//   value: shorthand for is_formatted_output_stream<_S>::value.
template<typename _S>
constexpr bool is_formatted_output_stream_v =
    is_formatted_output_stream<_S>::value;


// =========================================================================
// IX.  RANDOM NUMBER GENERATION _v HELPERS (C++14)
// =========================================================================

// is_seed_sequence_v
//   value: shorthand for is_seed_sequence<_S>::value.
template<typename _S>
constexpr bool is_seed_sequence_v =
    is_seed_sequence<_S>::value;

// is_uniform_random_bit_generator_v
//   value: shorthand for
// is_uniform_random_bit_generator<_G>::value.
template<typename _G>
constexpr bool is_uniform_random_bit_generator_v =
    is_uniform_random_bit_generator<_G>::value;

// is_random_number_engine_v
//   value: shorthand for is_random_number_engine<_E>::value.
template<typename _E>
constexpr bool is_random_number_engine_v =
    is_random_number_engine<_E>::value;

// is_random_number_engine_adaptor_v
//   value: shorthand for
// is_random_number_engine_adaptor<_E>::value.
template<typename _E>
constexpr bool is_random_number_engine_adaptor_v =
    is_random_number_engine_adaptor<_E>::value;

// is_random_number_distribution_v
//   value: shorthand for
// is_random_number_distribution<_D>::value.
template<typename _D>
constexpr bool is_random_number_distribution_v =
    is_random_number_distribution<_D>::value;


// =========================================================================
// X.   CONCURRENCY _v HELPERS (C++14)
// =========================================================================

// is_basic_lockable_v
//   value: shorthand for is_basic_lockable<_M>::value.
template<typename _M>
constexpr bool is_basic_lockable_v =
    is_basic_lockable<_M>::value;

// is_lockable_v
//   value: shorthand for is_lockable<_M>::value.
template<typename _M>
constexpr bool is_lockable_v =
    is_lockable<_M>::value;

// is_timed_lockable_v
//   value: shorthand for is_timed_lockable<_M>::value.
template<typename _M>
constexpr bool is_timed_lockable_v =
    is_timed_lockable<_M>::value;

// is_shared_lockable_v
//   value: shorthand for is_shared_lockable<_M>::value.
template<typename _M>
constexpr bool is_shared_lockable_v =
    is_shared_lockable<_M>::value;

// is_shared_timed_lockable_v
//   value: shorthand for is_shared_timed_lockable<_M>::value.
template<typename _M>
constexpr bool is_shared_timed_lockable_v =
    is_shared_timed_lockable<_M>::value;

// is_mutex_v
//   value: shorthand for is_mutex<_M>::value.
template<typename _M>
constexpr bool is_mutex_v =
    is_mutex<_M>::value;

// is_timed_mutex_v
//   value: shorthand for is_timed_mutex<_M>::value.
template<typename _M>
constexpr bool is_timed_mutex_v =
    is_timed_mutex<_M>::value;

// is_shared_mutex_v
//   value: shorthand for is_shared_mutex<_M>::value.
template<typename _M>
constexpr bool is_shared_mutex_v =
    is_shared_mutex<_M>::value;

// is_shared_timed_mutex_v
//   value: shorthand for is_shared_timed_mutex<_M>::value.
template<typename _M>
constexpr bool is_shared_timed_mutex_v =
    is_shared_timed_mutex<_M>::value;


// =========================================================================
// XI.  TYPE TRAIT META-TRAIT _v HELPERS (C++14)
// =========================================================================

// is_unary_type_trait_v
//   value: shorthand for is_unary_type_trait<_T>::value.
template<typename _T>
constexpr bool is_unary_type_trait_v =
    is_unary_type_trait<_T>::value;

// is_binary_type_trait_v
//   value: shorthand for is_binary_type_trait<_T>::value.
template<typename _T>
constexpr bool is_binary_type_trait_v =
    is_binary_type_trait<_T>::value;

// is_transformation_trait_v
//   value: shorthand for is_transformation_trait<_T>::value.
template<typename _T>
constexpr bool is_transformation_trait_v =
    is_transformation_trait<_T>::value;


// =========================================================================
// XII. CLOCK, CHARTRAITS, MISC _v HELPERS (C++14)
// =========================================================================

// is_clock_v
//   value: shorthand for is_clock<_C>::value.
template<typename _C>
constexpr bool is_clock_v =
    is_clock<_C>::value;

// is_trivial_clock_v
//   value: shorthand for is_trivial_clock<_C>::value.
template<typename _C>
constexpr bool is_trivial_clock_v =
    is_trivial_clock<_C>::value;

// is_char_traits_v
//   value: shorthand for is_char_traits<_Tr>::value.
template<typename _Tr>
constexpr bool is_char_traits_v =
    is_char_traits<_Tr>::value;

// is_bitmask_type_v
//   value: shorthand for is_bitmask_type<_B>::value.
template<typename _B>
constexpr bool is_bitmask_type_v =
    is_bitmask_type<_B>::value;

// is_numeric_type_v
//   value: shorthand for is_numeric_type<_T>::value.
template<typename _T>
constexpr bool is_numeric_type_v =
    is_numeric_type<_T>::value;

// is_regex_traits_v
//   value: shorthand for is_regex_traits<_Tr>::value.
template<typename _Tr>
constexpr bool is_regex_traits_v =
    is_regex_traits<_Tr>::value;

// is_literal_type_v_
//   value: shorthand for is_literal_type_<_T>::value.
template<typename _T>
constexpr bool is_literal_type_v_ =
    is_literal_type_<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED14_
