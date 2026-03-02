/******************************************************************************
* djinterp [core]                                              cpp_named17.h
*
* C++17 named requirement type traits:
*   Extends the C++14 named requirement traits with:
*   - inline constexpr variable templates (ODR-safe across translation units)
*   - Compound requirement traits using std::conjunction / std::disjunction
*   - Trivially-qualified variants (is_trivially_*)
*
* ADDITIONS OVER C++14:
*   - inline constexpr _v helpers (ODR-safe)
*   - is_trivially_default_constructible<_T>
*   - is_trivially_copy_constructible<_T>
*   - is_trivially_move_constructible<_T>
*   - is_trivially_copy_assignable<_T>
*   - is_trivially_move_assignable<_T>
*   - is_trivially_destructible<_T>
*   - is_fully_constructible<_T>     (compound: default + copy + move)
*   - is_fully_assignable<_T>        (compound: copy + move)
*   - is_fully_copyable<_T>          (compound: copy construct + copy assign)
*   - is_fully_movable<_T>           (compound: move construct + move assign)
*   - is_value_type<_T>              (compound: all six named requirements)
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES  - inline constexpr
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES - move trait availability
*
* path:      \inc\cpp_named17.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED17_
#define DJINTERP_CPP_NAMED17_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named17.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named17.h can only be used in C++ compilation mode"
#endif

// include C++14 layer
#include "cpp_named14.h"

// C++17 additions are active for C++17 and all later standards
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#include <type_traits>


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   TRIVIALLY-QUALIFIED VARIANTS (C++17)
// =========================================================================

// is_trivially_default_constructible
//   trait: evaluates to true if _T is trivially default-constructible
// (i.e., default construction performs no non-trivial operations).
template<typename _T>
struct is_trivially_default_constructible
{
    static constexpr bool value =
        std::is_trivially_default_constructible<_T>::value;
};

// is_trivially_copy_constructible
//   trait: evaluates to true if _T is trivially copy-constructible.
template<typename _T>
struct is_trivially_copy_constructible
{
    static constexpr bool value =
        std::is_trivially_copy_constructible<_T>::value;
};

// is_trivially_copy_assignable
//   trait: evaluates to true if _T is trivially copy-assignable.
template<typename _T>
struct is_trivially_copy_assignable
{
    static constexpr bool value =
        std::is_trivially_copy_assignable<_T>::value;
};

// is_trivially_destructible
//   trait: evaluates to true if _T is trivially destructible.
template<typename _T>
struct is_trivially_destructible
{
    static constexpr bool value =
        std::is_trivially_destructible<_T>::value;
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_trivially_move_constructible
//   trait: evaluates to true if _T is trivially move-constructible.
template<typename _T>
struct is_trivially_move_constructible
{
    static constexpr bool value =
        std::is_trivially_move_constructible<_T>::value;
};

// is_trivially_move_assignable
//   trait: evaluates to true if _T is trivially move-assignable.
template<typename _T>
struct is_trivially_move_assignable
{
    static constexpr bool value =
        std::is_trivially_move_assignable<_T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// II.  COMPOUND REQUIREMENT TRAITS (C++17)
// =========================================================================

// is_fully_copyable
//   trait: evaluates to true if _T satisfies both CopyConstructible and
// CopyAssignable.
template<typename _T>
struct is_fully_copyable
{
    static constexpr bool value =
        ( std::is_copy_constructible<_T>::value &&
          std::is_copy_assignable<_T>::value );
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_fully_movable
//   trait: evaluates to true if _T satisfies both MoveConstructible and
// MoveAssignable.
template<typename _T>
struct is_fully_movable
{
    static constexpr bool value =
        ( std::is_move_constructible<_T>::value &&
          std::is_move_assignable<_T>::value );
};

// is_fully_constructible
//   trait: evaluates to true if _T satisfies DefaultConstructible,
// CopyConstructible, and MoveConstructible.
template<typename _T>
struct is_fully_constructible
{
    static constexpr bool value =
        ( std::is_default_constructible<_T>::value &&
          std::is_copy_constructible<_T>::value    &&
          std::is_move_constructible<_T>::value );
};

// is_fully_assignable
//   trait: evaluates to true if _T satisfies both CopyAssignable and
// MoveAssignable.
template<typename _T>
struct is_fully_assignable
{
    static constexpr bool value =
        ( std::is_copy_assignable<_T>::value &&
          std::is_move_assignable<_T>::value );
};

// is_value_type
//   trait: evaluates to true if _T satisfies all six core named
// requirements: DefaultConstructible, MoveConstructible,
// CopyConstructible, MoveAssignable, CopyAssignable, and Destructible.
template<typename _T>
struct is_value_type
{
    static constexpr bool value =
        ( std::is_default_constructible<_T>::value &&
          std::is_copy_constructible<_T>::value    &&
          std::is_move_constructible<_T>::value    &&
          std::is_copy_assignable<_T>::value        &&
          std::is_move_assignable<_T>::value        &&
          std::is_destructible<_T>::value );
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. INLINE CONSTEXPR VARIABLE TEMPLATES (C++17)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// --- trivially-qualified _v helpers ---

// is_trivially_default_constructible_v
//   value: shorthand for is_trivially_default_constructible<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_default_constructible_v =
    is_trivially_default_constructible<_T>::value;

// is_trivially_copy_constructible_v
//   value: shorthand for is_trivially_copy_constructible<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_copy_constructible_v =
    is_trivially_copy_constructible<_T>::value;

// is_trivially_copy_assignable_v
//   value: shorthand for is_trivially_copy_assignable<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_copy_assignable_v =
    is_trivially_copy_assignable<_T>::value;

// is_trivially_destructible_v
//   value: shorthand for is_trivially_destructible<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_destructible_v =
    is_trivially_destructible<_T>::value;

// --- compound _v helpers ---

// is_fully_copyable_v
//   value: shorthand for is_fully_copyable<_T>::value.
template<typename _T>
inline constexpr bool is_fully_copyable_v =
    is_fully_copyable<_T>::value;

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_trivially_move_constructible_v
//   value: shorthand for is_trivially_move_constructible<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_move_constructible_v =
    is_trivially_move_constructible<_T>::value;

// is_trivially_move_assignable_v
//   value: shorthand for is_trivially_move_assignable<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_move_assignable_v =
    is_trivially_move_assignable<_T>::value;

// is_fully_movable_v
//   value: shorthand for is_fully_movable<_T>::value.
template<typename _T>
inline constexpr bool is_fully_movable_v =
    is_fully_movable<_T>::value;

// is_fully_constructible_v
//   value: shorthand for is_fully_constructible<_T>::value.
template<typename _T>
inline constexpr bool is_fully_constructible_v =
    is_fully_constructible<_T>::value;

// is_fully_assignable_v
//   value: shorthand for is_fully_assignable<_T>::value.
template<typename _T>
inline constexpr bool is_fully_assignable_v =
    is_fully_assignable<_T>::value;

// is_value_type_v
//   value: shorthand for is_value_type<_T>::value.
template<typename _T>
inline constexpr bool is_value_type_v =
    is_value_type<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// --- type classification inline constexpr _v helpers ---

// is_scalar_v (inline constexpr)
//   value: ODR-safe shorthand for is_scalar<_T>::value.
template<typename _T>
inline constexpr bool is_scalar_v =
    is_scalar<_T>::value;

// is_trivially_copyable_v (inline constexpr)
//   value: ODR-safe shorthand for is_trivially_copyable<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_copyable_v =
    is_trivially_copyable<_T>::value;

// is_trivial_v (inline constexpr)
//   value: ODR-safe shorthand for is_trivial<_T>::value.
// note: TrivialType is deprecated in C++26.
template<typename _T>
inline constexpr bool is_trivial_v =
    is_trivial<_T>::value;

// is_standard_layout_v (inline constexpr)
//   value: ODR-safe shorthand for is_standard_layout<_T>::value.
template<typename _T>
inline constexpr bool is_standard_layout_v =
    is_standard_layout<_T>::value;

// is_pod_v (inline constexpr)
//   value: ODR-safe shorthand for is_pod<_T>::value.
// note: PODType is deprecated in C++20; std::is_pod removed in C++26.
template<typename _T>
inline constexpr bool is_pod_v =
    is_pod<_T>::value;


// --- comparison, callable, and predicate inline constexpr _v helpers ---

// is_boolean_testable_v (inline constexpr)
//   value: ODR-safe shorthand for is_boolean_testable<_T>::value.
template<typename _T>
inline constexpr bool is_boolean_testable_v =
    is_boolean_testable<_T>::value;

// is_equality_comparable_v (inline constexpr)
//   value: ODR-safe shorthand for is_equality_comparable<_T>::value.
template<typename _T>
inline constexpr bool is_equality_comparable_v =
    is_equality_comparable<_T>::value;

// is_less_than_comparable_v (inline constexpr)
//   value: ODR-safe shorthand for is_less_than_comparable<_T>::value.
template<typename _T>
inline constexpr bool is_less_than_comparable_v =
    is_less_than_comparable<_T>::value;

// is_nullable_pointer_v (inline constexpr)
//   value: ODR-safe shorthand for is_nullable_pointer<_T>::value.
template<typename _T>
inline constexpr bool is_nullable_pointer_v =
    is_nullable_pointer<_T>::value;

// is_hashable_v (inline constexpr)
//   value: ODR-safe shorthand for is_hashable<_T>::value.
template<typename _T>
inline constexpr bool is_hashable_v =
    is_hashable<_T>::value;

// is_allocator_v (inline constexpr)
//   value: ODR-safe shorthand for is_allocator<_A>::value.
template<typename _A>
inline constexpr bool is_allocator_v =
    is_allocator<_A>::value;

// is_function_object_v (inline constexpr)
//   value: ODR-safe shorthand for is_function_object<_F>::value.
template<typename _F>
inline constexpr bool is_function_object_v =
    is_function_object<_F>::value;

// is_compare_v (inline constexpr)
//   value: ODR-safe shorthand for is_compare<_F, _T>::value.
template<typename _F,
         typename _T>
inline constexpr bool is_compare_v =
    is_compare<_F, _T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// IV.  STD-BACKED SWAPPABLE TRAITS (C++17)
// =========================================================================

// is_swappable (C++17 override)
//   trait: evaluates to true if _T is swappable.
// backed by std::is_swappable (supersedes C++11 ADL-based detection).
template<typename _T>
struct is_swappable
{
    static constexpr bool value =
        std::is_swappable<_T>::value;
};

// is_nothrow_swappable
//   trait: evaluates to true if _T is swappable without throwing
// exceptions.
template<typename _T>
struct is_nothrow_swappable
{
    static constexpr bool value =
        std::is_nothrow_swappable<_T>::value;
};

// is_swappable_with
//   trait: evaluates to true if _T and _U are mutually swappable.
template<typename _T,
         typename _U>
struct is_swappable_with
{
    static constexpr bool value =
        std::is_swappable_with<_T, _U>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_swappable_v (inline constexpr, C++17)
//   value: ODR-safe shorthand for is_swappable<_T>::value.
template<typename _T>
inline constexpr bool is_swappable_v =
    is_swappable<_T>::value;

// is_nothrow_swappable_v (inline constexpr)
//   value: ODR-safe shorthand for is_nothrow_swappable<_T>::value.
template<typename _T>
inline constexpr bool is_nothrow_swappable_v =
    is_nothrow_swappable<_T>::value;

// is_swappable_with_v (inline constexpr)
//   value: ODR-safe shorthand for is_swappable_with<_T, _U>::value.
template<typename _T,
         typename _U>
inline constexpr bool is_swappable_with_v =
    is_swappable_with<_T, _U>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// V.   STD-BACKED INVOCABLE / CALLABLE TRAITS (C++17)
// =========================================================================

// is_callable (C++17 override)
//   trait: evaluates to true if _F is invocable with _Args... using
// INVOKE semantics (handles pointer-to-member, etc.).
// backed by std::is_invocable (supersedes C++11 direct-call detection).
template<typename _F,
         typename... _Args>
struct is_callable
{
    static constexpr bool value =
        std::is_invocable<_F, _Args...>::value;
};

// is_predicate (C++17 override)
//   trait: evaluates to true if _F is invocable with _Arg and returns
// a type convertible to bool (satisfies Predicate).
template<typename _F,
         typename _Arg>
struct is_predicate
{
    static constexpr bool value =
        ( std::is_invocable<_F, _Arg>::value &&
          std::is_convertible<
              std::invoke_result_t<_F, _Arg>, bool>::value );
};

// is_binary_predicate (C++17 override)
//   trait: evaluates to true if _F is invocable with _Arg1, _Arg2 and
// returns a type convertible to bool (satisfies BinaryPredicate).
template<typename _F,
         typename _Arg1,
         typename _Arg2>
struct is_binary_predicate
{
    static constexpr bool value =
        ( std::is_invocable<_F, _Arg1, _Arg2>::value &&
          std::is_convertible<
              std::invoke_result_t<_F, _Arg1, _Arg2>, bool>::value );
};

// is_compare (C++17 override)
//   trait: evaluates to true if _F is a BinaryPredicate over
// (const _T&, const _T&) (satisfies Compare structurally).
template<typename _F,
         typename _T>
struct is_compare
{
    static constexpr bool value =
        is_binary_predicate<_F, const _T&, const _T&>::value;
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_callable_v (inline constexpr, C++17)
//   value: ODR-safe shorthand for is_callable<_F, _Args...>::value.
template<typename _F,
         typename... _Args>
inline constexpr bool is_callable_v =
    is_callable<_F, _Args...>::value;

// is_predicate_v (inline constexpr, C++17)
//   value: ODR-safe shorthand for is_predicate<_F, _Arg>::value.
template<typename _F,
         typename _Arg>
inline constexpr bool is_predicate_v =
    is_predicate<_F, _Arg>::value;

// is_binary_predicate_v (inline constexpr, C++17)
//   value: ODR-safe shorthand for
// is_binary_predicate<_F, _Arg1, _Arg2>::value.
template<typename _F,
         typename _Arg1,
         typename _Arg2>
inline constexpr bool is_binary_predicate_v =
    is_binary_predicate<_F, _Arg1, _Arg2>::value;

// is_compare_v (inline constexpr, C++17)
//   value: ODR-safe shorthand for is_compare<_F, _T>::value.
template<typename _F,
         typename _T>
inline constexpr bool is_compare_v =
    is_compare<_F, _T>::value;

// --- container inline constexpr _v helpers ---

// is_container_v (inline constexpr)
//   value: ODR-safe shorthand for is_container<_C>::value.
template<typename _C>
inline constexpr bool is_container_v =
    is_container<_C>::value;

// is_reversible_container_v (inline constexpr)
//   value: ODR-safe shorthand for is_reversible_container<_C>::value.
template<typename _C>
inline constexpr bool is_reversible_container_v =
    is_reversible_container<_C>::value;

// is_allocator_aware_container_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_allocator_aware_container<_C>::value.
template<typename _C>
inline constexpr bool is_allocator_aware_container_v =
    is_allocator_aware_container<_C>::value;

// is_sequence_container_v (inline constexpr)
//   value: ODR-safe shorthand for is_sequence_container<_C>::value.
template<typename _C>
inline constexpr bool is_sequence_container_v =
    is_sequence_container<_C>::value;

// is_associative_container_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_associative_container<_C>::value.
template<typename _C>
inline constexpr bool is_associative_container_v =
    is_associative_container<_C>::value;

// is_unordered_associative_container_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_unordered_associative_container<_C>::value.
template<typename _C>
inline constexpr bool is_unordered_associative_container_v =
    is_unordered_associative_container<_C>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// VI.  CONTIGUOUS CONTAINER TRAIT (C++17)
// =========================================================================

NS_INTERNAL

    // has_contiguous_ops
    //   trait: detects ContiguousContainer member: data() returning a
    // pointer to value_type.
    template<typename _C,
             typename = void>
    struct has_contiguous_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_contiguous_ops<_C,
        decltype(static_cast<void>(
            static_cast<const typename _C::value_type*>(
                std::declval<const _C&>().data())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_contiguous_container
//   trait: evaluates to true if _C satisfies the structural
// requirements of the ContiguousContainer named requirement: is a
// Container with a data() member returning a pointer to contiguous
// storage.
// note: introduced in C++17. Does not verify contiguous_iterator
// concept on iterators (that requires C++20 concepts).
template<typename _C>
struct is_contiguous_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_contiguous_ops<_C>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_contiguous_container_v (inline constexpr)
//   value: ODR-safe shorthand for is_contiguous_container<_C>::value.
template<typename _C>
inline constexpr bool is_contiguous_container_v =
    is_contiguous_container<_C>::value;

// --- container element inline constexpr _v helpers ---

// is_default_insertable_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_default_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
inline constexpr bool is_default_insertable_v =
    is_default_insertable<_T, _A>::value;

// is_copy_insertable_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_copy_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
inline constexpr bool is_copy_insertable_v =
    is_copy_insertable<_T, _A>::value;

// is_move_insertable_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_move_insertable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
inline constexpr bool is_move_insertable_v =
    is_move_insertable<_T, _A>::value;

// is_emplace_constructible_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_emplace_constructible<_T, _A, _Args...>::value.
template<typename _T,
         typename _A,
         typename... _Args>
inline constexpr bool is_emplace_constructible_v =
    is_emplace_constructible<_T, _A, _Args...>::value;

// is_erasable_v (inline constexpr)
//   value: ODR-safe shorthand for is_erasable<_T, _A>::value.
template<typename _T,
         typename _A = std::allocator<_T>>
inline constexpr bool is_erasable_v =
    is_erasable<_T, _A>::value;

// --- iterator inline constexpr _v helpers ---

// is_legacy_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for is_legacy_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_iterator_v =
    is_legacy_iterator<_I>::value;

// is_legacy_input_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for is_legacy_input_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_input_iterator_v =
    is_legacy_input_iterator<_I>::value;

// is_legacy_output_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_output_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_output_iterator_v =
    is_legacy_output_iterator<_I>::value;

// is_legacy_forward_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_forward_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_forward_iterator_v =
    is_legacy_forward_iterator<_I>::value;

// is_legacy_bidirectional_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_bidirectional_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_bidirectional_iterator_v =
    is_legacy_bidirectional_iterator<_I>::value;

// is_legacy_random_access_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_random_access_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_random_access_iterator_v =
    is_legacy_random_access_iterator<_I>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


// =========================================================================
// VII. LEGACY CONTIGUOUS ITERATOR TRAIT (C++17)
// =========================================================================

NS_INTERNAL

    // is_contiguous_iterator_tag
    //   trait: detects if iterator_traits<_I>::iterator_category is
    // or derives from contiguous_iterator_tag. Since
    // std::contiguous_iterator_tag is formally C++20, this SFINAE
    // check gracefully returns false on implementations that do not
    // provide it.
    template<typename _I,
             typename = void>
    struct is_contiguous_iterator_tag
    {
        static constexpr bool value = false;
    };

#if (__cplusplus >= 202002L)
    template<typename _I>
    struct is_contiguous_iterator_tag<_I,
        typename std::enable_if<
            std::is_base_of<
                std::contiguous_iterator_tag,
                typename std::iterator_traits<_I>::iterator_category
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };
#endif

NS_END  // internal

// is_legacy_contiguous_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyContiguousIterator named requirement:
// LegacyRandomAccessIterator whose elements are contiguous in memory.
// note: pointer types are always contiguous. For class-type iterators,
// detection relies on contiguous_iterator_tag (C++20). See
// cpp_named20.h for std::contiguous_iterator-backed version.
template<typename _I>
struct is_legacy_contiguous_iterator
{
    static constexpr bool value =
        ( is_legacy_random_access_iterator<_I>::value &&
          ( std::is_pointer<_I>::value ||
            internal::is_contiguous_iterator_tag<_I>::value ) );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// is_legacy_contiguous_iterator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_legacy_contiguous_iterator<_I>::value.
template<typename _I>
inline constexpr bool is_legacy_contiguous_iterator_v =
    is_legacy_contiguous_iterator<_I>::value;

// --- stream I/O inline constexpr _v helpers ---

// is_unformatted_input_stream_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_unformatted_input_stream<_S>::value.
template<typename _S>
inline constexpr bool is_unformatted_input_stream_v =
    is_unformatted_input_stream<_S>::value;

// is_formatted_input_stream_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_formatted_input_stream<_S>::value.
template<typename _S>
inline constexpr bool is_formatted_input_stream_v =
    is_formatted_input_stream<_S>::value;

// is_unformatted_output_stream_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_unformatted_output_stream<_S>::value.
template<typename _S>
inline constexpr bool is_unformatted_output_stream_v =
    is_unformatted_output_stream<_S>::value;

// is_formatted_output_stream_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_formatted_output_stream<_S>::value.
template<typename _S>
inline constexpr bool is_formatted_output_stream_v =
    is_formatted_output_stream<_S>::value;

// --- random number generation inline constexpr _v helpers ---

// is_seed_sequence_v (inline constexpr)
//   value: ODR-safe shorthand for is_seed_sequence<_S>::value.
template<typename _S>
inline constexpr bool is_seed_sequence_v =
    is_seed_sequence<_S>::value;

// is_uniform_random_bit_generator_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_uniform_random_bit_generator<_G>::value.
template<typename _G>
inline constexpr bool is_uniform_random_bit_generator_v =
    is_uniform_random_bit_generator<_G>::value;

// is_random_number_engine_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_random_number_engine<_E>::value.
template<typename _E>
inline constexpr bool is_random_number_engine_v =
    is_random_number_engine<_E>::value;

// is_random_number_engine_adaptor_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_random_number_engine_adaptor<_E>::value.
template<typename _E>
inline constexpr bool is_random_number_engine_adaptor_v =
    is_random_number_engine_adaptor<_E>::value;

// is_random_number_distribution_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_random_number_distribution<_D>::value.
template<typename _D>
inline constexpr bool is_random_number_distribution_v =
    is_random_number_distribution<_D>::value;

// --- concurrency inline constexpr _v helpers ---

// is_basic_lockable_v (inline constexpr)
//   value: ODR-safe shorthand for is_basic_lockable<_M>::value.
template<typename _M>
inline constexpr bool is_basic_lockable_v =
    is_basic_lockable<_M>::value;

// is_lockable_v (inline constexpr)
//   value: ODR-safe shorthand for is_lockable<_M>::value.
template<typename _M>
inline constexpr bool is_lockable_v =
    is_lockable<_M>::value;

// is_timed_lockable_v (inline constexpr)
//   value: ODR-safe shorthand for is_timed_lockable<_M>::value.
template<typename _M>
inline constexpr bool is_timed_lockable_v =
    is_timed_lockable<_M>::value;

// is_shared_lockable_v (inline constexpr)
//   value: ODR-safe shorthand for is_shared_lockable<_M>::value.
template<typename _M>
inline constexpr bool is_shared_lockable_v =
    is_shared_lockable<_M>::value;

// is_shared_timed_lockable_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_shared_timed_lockable<_M>::value.
template<typename _M>
inline constexpr bool is_shared_timed_lockable_v =
    is_shared_timed_lockable<_M>::value;

// is_mutex_v (inline constexpr)
//   value: ODR-safe shorthand for is_mutex<_M>::value.
template<typename _M>
inline constexpr bool is_mutex_v =
    is_mutex<_M>::value;

// is_timed_mutex_v (inline constexpr)
//   value: ODR-safe shorthand for is_timed_mutex<_M>::value.
template<typename _M>
inline constexpr bool is_timed_mutex_v =
    is_timed_mutex<_M>::value;

// is_shared_mutex_v (inline constexpr)
//   value: ODR-safe shorthand for is_shared_mutex<_M>::value.
template<typename _M>
inline constexpr bool is_shared_mutex_v =
    is_shared_mutex<_M>::value;

// is_shared_timed_mutex_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_shared_timed_mutex<_M>::value.
template<typename _M>
inline constexpr bool is_shared_timed_mutex_v =
    is_shared_timed_mutex<_M>::value;

// --- type trait meta-trait inline constexpr _v helpers ---

// is_unary_type_trait_v (inline constexpr)
//   value: ODR-safe shorthand for is_unary_type_trait<_T>::value.
template<typename _T>
inline constexpr bool is_unary_type_trait_v =
    is_unary_type_trait<_T>::value;

// is_binary_type_trait_v (inline constexpr)
//   value: ODR-safe shorthand for is_binary_type_trait<_T>::value.
template<typename _T>
inline constexpr bool is_binary_type_trait_v =
    is_binary_type_trait<_T>::value;

// is_transformation_trait_v (inline constexpr)
//   value: ODR-safe shorthand for
// is_transformation_trait<_T>::value.
template<typename _T>
inline constexpr bool is_transformation_trait_v =
    is_transformation_trait<_T>::value;

// --- clock, chartraits, misc inline constexpr _v helpers ---

// is_clock_v (inline constexpr)
//   value: ODR-safe shorthand for is_clock<_C>::value.
template<typename _C>
inline constexpr bool is_clock_v =
    is_clock<_C>::value;

// is_trivial_clock_v (inline constexpr)
//   value: ODR-safe shorthand for is_trivial_clock<_C>::value.
template<typename _C>
inline constexpr bool is_trivial_clock_v =
    is_trivial_clock<_C>::value;

// is_char_traits_v (inline constexpr)
//   value: ODR-safe shorthand for is_char_traits<_Tr>::value.
template<typename _Tr>
inline constexpr bool is_char_traits_v =
    is_char_traits<_Tr>::value;

// is_bitmask_type_v (inline constexpr)
//   value: ODR-safe shorthand for is_bitmask_type<_B>::value.
template<typename _B>
inline constexpr bool is_bitmask_type_v =
    is_bitmask_type<_B>::value;

// is_numeric_type_v (inline constexpr)
//   value: ODR-safe shorthand for is_numeric_type<_T>::value.
template<typename _T>
inline constexpr bool is_numeric_type_v =
    is_numeric_type<_T>::value;

// is_regex_traits_v (inline constexpr)
//   value: ODR-safe shorthand for is_regex_traits<_Tr>::value.
template<typename _Tr>
inline constexpr bool is_regex_traits_v =
    is_regex_traits<_Tr>::value;

// is_literal_type_v_ (inline constexpr)
//   value: ODR-safe shorthand for is_literal_type_<_T>::value.
template<typename _T>
inline constexpr bool is_literal_type_v_ =
    is_literal_type_<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED17_
