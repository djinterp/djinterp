/******************************************************************************
* djinterp [core]                                              cpp_named11.h
*
* C++11 named requirement type traits:
*   Provides precise compile-time detection of fundamental named requirements
* using the C++11 <type_traits> header. This supersedes the sizeof-based
* SFINAE approach from cpp_named98.h with compiler-intrinsic-backed traits
* that correctly handle private/deleted special members.
*
*   C++11 introduces rvalue references, enabling MoveConstructible and
* MoveAssignable detection for the first time. All six core named
* requirements are now available, along with nothrow variants.
*
* SUPPORTED REQUIREMENTS:
*   - DefaultConstructible  (is_default_constructible)
*   - MoveConstructible     (is_move_constructible)       [NEW in C++11]
*   - CopyConstructible     (is_copy_constructible)
*   - MoveAssignable        (is_move_assignable)          [NEW in C++11]
*   - CopyAssignable        (is_copy_assignable)
*   - Destructible          (is_destructible)
*   - nothrow variants of all of the above                [NEW in C++11]
*   - ScalarType            (is_scalar)                   [NEW in C++11]
*   - TriviallyCopyable     (is_trivially_copyable)       [NEW in C++11]
*   - TrivialType           (is_trivial)                  [NEW in C++11]
*   - StandardLayoutType    (is_standard_layout)          [NEW in C++11]
*   - PODType               (is_pod)                      [NEW in C++11]
*   - BooleanTestable       (is_boolean_testable)          [NEW in C++11]
*   - EqualityComparable    (is_equality_comparable)       [NEW in C++11]
*   - LessThanComparable    (is_less_than_comparable)      [NEW in C++11]
*   - Swappable             (is_swappable)                 [NEW in C++11]
*   - ValueSwappable        (is_value_swappable)           [NEW in C++11]
*   - NullablePointer       (is_nullable_pointer)          [NEW in C++11]
*   - Hash                  (is_hashable)                  [NEW in C++11]
*   - Allocator             (is_allocator)                 [NEW in C++11]
*   - FunctionObject        (is_function_object)           [NEW in C++11]
*   - Callable              (is_callable)                  [NEW in C++11]
*   - Predicate             (is_predicate)                 [NEW in C++11]
*   - BinaryPredicate       (is_binary_predicate)          [NEW in C++11]
*   - Compare               (is_compare)                   [NEW in C++11]
*   - Container             (is_container)                 [NEW in C++11]
*   - ReversibleContainer   (is_reversible_container)      [NEW in C++11]
*   - AllocatorAwareContainer (is_allocator_aware_container) [NEW in C++11]
*   - SequenceContainer     (is_sequence_container)        [NEW in C++11]
*   - AssociativeContainer  (is_associative_container)     [NEW in C++11]
*   - UnorderedAssociativeContainer
*                           (is_unordered_associative_container) [NEW]
*   - DefaultInsertable     (is_default_insertable)        [NEW in C++11]
*   - CopyInsertable        (is_copy_insertable)           [NEW in C++11]
*   - MoveInsertable        (is_move_insertable)           [NEW in C++11]
*   - EmplaceConstructible  (is_emplace_constructible)     [NEW in C++11]
*   - Erasable              (is_erasable)                  [NEW in C++11]
*   - LegacyIterator        (is_legacy_iterator)           [NEW in C++11]
*   - LegacyInputIterator   (is_legacy_input_iterator)     [NEW in C++11]
*   - LegacyOutputIterator  (is_legacy_output_iterator)    [NEW in C++11]
*   - LegacyForwardIterator (is_legacy_forward_iterator)   [NEW in C++11]
*   - LegacyBidirectionalIterator
*                           (is_legacy_bidirectional_iterator) [NEW]
*   - LegacyRandomAccessIterator
*                           (is_legacy_random_access_iterator) [NEW]
*   - Stream I/O traits     (is_unformatted_input_stream, etc.) [NEW]
*   - SeedSequence          (is_seed_sequence)                 [NEW in C++11]
*   - UniformRandomBitGenerator (is_uniform_random_bit_generator) [NEW]
*   - RandomNumberEngine    (is_random_number_engine)          [NEW in C++11]
*   - RandomNumberEngineAdaptor
*                           (is_random_number_engine_adaptor)  [NEW]
*   - RandomNumberDistribution
*                           (is_random_number_distribution)    [NEW]
*   - BasicLockable through SharedTimedMutex                   [NEW in C++11]
*   - UnaryTypeTrait         (is_unary_type_trait)             [NEW in C++11]
*   - BinaryTypeTrait        (is_binary_type_trait)            [NEW in C++11]
*   - TransformationTrait    (is_transformation_trait)         [NEW in C++11]
*   - Clock                  (is_clock)                        [NEW in C++11]
*   - TrivialClock           (is_trivial_clock)                [NEW in C++11]
*   - CharTraits             (is_char_traits)                  [NEW in C++11]
*   - BitmaskType            (is_bitmask_type)                 [NEW in C++11]
*   - NumericType            (is_numeric_type)                 [NEW in C++11]
*   - RegexTraits            (is_regex_traits)                 [NEW in C++11]
*   - LiteralType            (is_literal_type_)                [NEW in C++11]
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES  - move trait availability
*
* NAMING CONVENTIONS:
*   djinterp::traits::is_default_constructible<_T>::value
*   djinterp::traits::is_nothrow_move_constructible<_T>::value
*   (etc.)
*
* path:      \inc\cpp_named11.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED11_
#define DJINTERP_CPP_NAMED11_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named11.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named11.h can only be used in C++ compilation mode"
#endif

// include C++98/03 layer (active only when C++11 is not available)
#include "cpp_named03.h"

// C++11 trait definitions are active for C++11 and all later standards
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include <type_traits>
#include <functional>
#include <utility>
#include <memory>
#include <iterator>
#include <ios>
#include <random>
#include <chrono>


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   NAMED REQUIREMENT TRAITS (C++11)
// =========================================================================

// is_default_constructible
//   trait: evaluates to true if _T is default-constructible.
// backed by compiler intrinsics via std::is_default_constructible.
template<typename _T>
struct is_default_constructible
{
    static constexpr bool value =
        std::is_default_constructible<_T>::value;
};

// is_copy_constructible
//   trait: evaluates to true if _T is copy-constructible from const _T&.
// backed by compiler intrinsics via std::is_copy_constructible.
template<typename _T>
struct is_copy_constructible
{
    static constexpr bool value =
        std::is_copy_constructible<_T>::value;
};

// is_copy_assignable
//   trait: evaluates to true if _T is copy-assignable from const _T&.
// backed by compiler intrinsics via std::is_copy_assignable.
template<typename _T>
struct is_copy_assignable
{
    static constexpr bool value =
        std::is_copy_assignable<_T>::value;
};

// is_destructible
//   trait: evaluates to true if _T has an accessible, non-deleted
// destructor.
// backed by compiler intrinsics via std::is_destructible.
template<typename _T>
struct is_destructible
{
    static constexpr bool value =
        std::is_destructible<_T>::value;
};


// =========================================================================
// II.  MOVE-AWARE TRAITS (C++11, requires rvalue references)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_constructible
//   trait: evaluates to true if _T is move-constructible from _T&&.
// requires rvalue reference support (C++11).
template<typename _T>
struct is_move_constructible
{
    static constexpr bool value =
        std::is_move_constructible<_T>::value;
};

// is_move_assignable
//   trait: evaluates to true if _T is move-assignable from _T&&.
// requires rvalue reference support (C++11).
template<typename _T>
struct is_move_assignable
{
    static constexpr bool value =
        std::is_move_assignable<_T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. NOTHROW VARIANTS (C++11)
// =========================================================================

// is_nothrow_default_constructible
//   trait: evaluates to true if _T is default-constructible without
// throwing exceptions.
template<typename _T>
struct is_nothrow_default_constructible
{
    static constexpr bool value =
        std::is_nothrow_default_constructible<_T>::value;
};

// is_nothrow_copy_constructible
//   trait: evaluates to true if _T is copy-constructible without
// throwing exceptions.
template<typename _T>
struct is_nothrow_copy_constructible
{
    static constexpr bool value =
        std::is_nothrow_copy_constructible<_T>::value;
};

// is_nothrow_copy_assignable
//   trait: evaluates to true if _T is copy-assignable without
// throwing exceptions.
template<typename _T>
struct is_nothrow_copy_assignable
{
    static constexpr bool value =
        std::is_nothrow_copy_assignable<_T>::value;
};

// is_nothrow_destructible
//   trait: evaluates to true if _T is destructible without throwing
// exceptions.
template<typename _T>
struct is_nothrow_destructible
{
    static constexpr bool value =
        std::is_nothrow_destructible<_T>::value;
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_nothrow_move_constructible
//   trait: evaluates to true if _T is move-constructible without
// throwing exceptions.
template<typename _T>
struct is_nothrow_move_constructible
{
    static constexpr bool value =
        std::is_nothrow_move_constructible<_T>::value;
};

// is_nothrow_move_assignable
//   trait: evaluates to true if _T is move-assignable without
// throwing exceptions.
template<typename _T>
struct is_nothrow_move_assignable
{
    static constexpr bool value =
        std::is_nothrow_move_assignable<_T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// IV.  TYPE CLASSIFICATION TRAITS (C++11)
// =========================================================================

// is_scalar
//   trait: evaluates to true if _T is a scalar type (arithmetic,
// enumeration, pointer, pointer-to-member, or std::nullptr_t).
// backed by compiler intrinsics via std::is_scalar.
template<typename _T>
struct is_scalar
{
    static constexpr bool value =
        std::is_scalar<_T>::value;
};

// is_trivially_copyable
//   trait: evaluates to true if _T is trivially copyable (can be
// safely copied via memcpy/memmove).
template<typename _T>
struct is_trivially_copyable
{
    static constexpr bool value =
        std::is_trivially_copyable<_T>::value;
};

// is_trivial
//   trait: evaluates to true if _T is a trivial type (trivially
// default-constructible and trivially copyable).
// note: TrivialType named requirement is deprecated in C++26.
template<typename _T>
struct is_trivial
{
    static constexpr bool value =
        std::is_trivial<_T>::value;
};

// is_standard_layout
//   trait: evaluates to true if _T satisfies the StandardLayoutType
// named requirement.
template<typename _T>
struct is_standard_layout
{
    static constexpr bool value =
        std::is_standard_layout<_T>::value;
};

// is_pod
//   trait: evaluates to true if _T is a POD (Plain Old Data) type,
// i.e., both trivial and standard-layout.
// note: PODType named requirement is deprecated in C++20 and
// std::is_pod is removed in C++26. Prefer is_trivially_copyable with
// is_standard_layout for new code.
template<typename _T>
struct is_pod
{
#if (__cplusplus < 202600L)
    static constexpr bool value =
        std::is_pod<_T>::value;
#else
    // std::is_pod removed in C++26; equivalent definition
    static constexpr bool value =
        ( std::is_trivial<_T>::value &&
          std::is_standard_layout<_T>::value );
#endif
};


// =========================================================================
// V.   COMPARISON AND BOOLEAN TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // boolean_testable_helper
    //   trait: detects if _T is contextually convertible to bool and
    // supports logical negation.
    template<typename _T,
             typename = void>
    struct boolean_testable_helper
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct boolean_testable_helper<_T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<_T>())
        ))>
    {
        static constexpr bool value = true;
    };

    // equality_comparable_helper
    //   trait: detects if _T supports == and != with bool-convertible
    // results.
    template<typename _T,
             typename = void>
    struct equality_comparable_helper
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct equality_comparable_helper<_T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const _T&>() ==
                              std::declval<const _T&>()),
            static_cast<bool>(std::declval<const _T&>() !=
                              std::declval<const _T&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // less_than_comparable_helper
    //   trait: detects if _T supports < with a bool-convertible result.
    template<typename _T,
             typename = void>
    struct less_than_comparable_helper
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct less_than_comparable_helper<_T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const _T&>() <
                              std::declval<const _T&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // swappable_helper
    //   trait: detects if _T supports ADL-visible swap via
    // `using std::swap; swap(a, b)`.
    template<typename _T,
             typename = void>
    struct swappable_helper
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct swappable_helper<_T,
        decltype(static_cast<void>(
            (void)(std::swap(std::declval<_T&>(),
                             std::declval<_T&>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // callable_helper
    //   trait: detects if _F is callable with _Args... .
    template<typename _Void,
             typename _F,
             typename... _Args>
    struct callable_helper
    {
        static constexpr bool value = false;
    };

    template<typename _F,
             typename... _Args>
    struct callable_helper<
        decltype(static_cast<void>(
            std::declval<_F>()(std::declval<_Args>()...)
        )),
        _F,
        _Args...>
    {
        static constexpr bool value = true;
    };

    // callable_returns_bool_helper
    //   trait: detects if _F(_Args...) is well-formed and returns a
    // type contextually convertible to bool.
    template<typename _Void,
             typename _F,
             typename... _Args>
    struct callable_returns_bool_helper
    {
        static constexpr bool value = false;
    };

    template<typename _F,
             typename... _Args>
    struct callable_returns_bool_helper<
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<_F>()(std::declval<_Args>()...))
        )),
        _F,
        _Args...>
    {
        static constexpr bool value = true;
    };

    // has_operator_call
    //   trait: detects if _T has a non-overloaded operator() member.
    // note: will report false for types with overloaded or templated
    // operator(). Use is_callable for a more general test.
    template<typename _T,
             typename = void>
    struct has_operator_call
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct has_operator_call<_T,
        decltype(static_cast<void>(&_T::operator()))>
    {
        static constexpr bool value = true;
    };

    // has_hash_specialization
    //   trait: detects if std::hash<_T> is well-formed and callable
    // with const _T&, returning std::size_t.
    template<typename _T,
             typename = void>
    struct has_hash_specialization
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct has_hash_specialization<_T,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const std::hash<_T>&>()(
                    std::declval<const _T&>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_allocator_interface
    //   trait: detects if _A has value_type, allocate(n), and
    // deallocate(p, n) members as required by Allocator.
    template<typename _A,
             typename = void>
    struct has_allocator_interface
    {
        static constexpr bool value = false;
    };

    template<typename _A>
    struct has_allocator_interface<_A,
        decltype(static_cast<void>(
            std::declval<typename _A::value_type>(),
            std::declval<_A&>().deallocate(
                std::declval<_A&>().allocate(std::size_t(1)),
                std::size_t(1))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_boolean_testable
//   trait: evaluates to true if _T is contextually convertible to
// bool (satisfies BooleanTestable).
template<typename _T>
struct is_boolean_testable
{
    static constexpr bool value =
        internal::boolean_testable_helper<_T>::value;
};

// is_equality_comparable
//   trait: evaluates to true if _T supports == and != with
// bool-convertible results (satisfies EqualityComparable).
template<typename _T>
struct is_equality_comparable
{
    static constexpr bool value =
        internal::equality_comparable_helper<_T>::value;
};

// is_less_than_comparable
//   trait: evaluates to true if _T supports operator< with a
// bool-convertible result (satisfies LessThanComparable).
template<typename _T>
struct is_less_than_comparable
{
    static constexpr bool value =
        internal::less_than_comparable_helper<_T>::value;
};

// is_swappable
//   trait: evaluates to true if _T is swappable via ADL-visible swap.
// note: this is a basic detection using std::swap as fallback. See
// cpp_named17.h for std::is_swappable-backed version.
template<typename _T>
struct is_swappable
{
    static constexpr bool value =
        internal::swappable_helper<_T>::value;
};

// is_value_swappable
//   trait: evaluates to true if _T is an iterator-like type whose
// dereferenced value is swappable (satisfies ValueSwappable).
// requires: _T is dereferenceable.
template<typename _T,
         typename = void>
struct is_value_swappable
{
    static constexpr bool value = false;
};

// is_value_swappable (dereferenceable specialization)
//   trait: partial specialization for dereferenceable types.
template<typename _T>
struct is_value_swappable<_T,
    decltype(static_cast<void>(*std::declval<_T&>()))>
{
    static constexpr bool value =
        is_swappable<decltype(*std::declval<_T&>())>::value;
};

// is_nullable_pointer
//   trait: evaluates to true if _T satisfies the NullablePointer named
// requirement: DefaultConstructible, CopyConstructible, CopyAssignable,
// Destructible, contextually convertible to bool, and constructible/
// comparable with std::nullptr_t.
template<typename _T>
struct is_nullable_pointer
{
    static constexpr bool value =
        ( std::is_default_constructible<_T>::value  &&
          std::is_copy_constructible<_T>::value      &&
          std::is_copy_assignable<_T>::value          &&
          std::is_destructible<_T>::value             &&
          std::is_constructible<_T, std::nullptr_t>::value &&
          is_boolean_testable<_T>::value              &&
          is_equality_comparable<_T>::value );
};

// is_hashable
//   trait: evaluates to true if std::hash<_T> is a valid
// specialization that accepts const _T& and returns std::size_t
// (satisfies Hash requirement for _T as key type).
template<typename _T>
struct is_hashable
{
    static constexpr bool value =
        internal::has_hash_specialization<_T>::value;
};

// is_allocator
//   trait: evaluates to true if _A satisfies the basic structural
// requirements of the Allocator named requirement: has value_type,
// allocate(n), and deallocate(p, n).
// note: does not verify all Allocator requirements (e.g., propagation
// traits, construct/destroy). For full compliance, see
// std::allocator_traits.
template<typename _A>
struct is_allocator
{
    static constexpr bool value =
        ( internal::has_allocator_interface<_A>::value &&
          std::is_copy_constructible<_A>::value        &&
          std::is_destructible<_A>::value );
};


// =========================================================================
// VI.  CALLABLE AND PREDICATE TRAITS (C++11)
// =========================================================================

// is_function_object
//   trait: evaluates to true if _F has a non-overloaded operator()
// member (satisfies FunctionObject structurally).
// note: reports false for types with overloaded or templated
// operator(). Use is_callable for a general invocability test.
template<typename _F>
struct is_function_object
{
    static constexpr bool value =
        internal::has_operator_call<_F>::value;
};

// is_callable
//   trait: evaluates to true if _F can be invoked with _Args...
// (satisfies Callable).
// note: tests direct invocation `_F(_Args...)` only. Does not account
// for std::invoke semantics (pointer-to-member, etc.). See
// cpp_named17.h for std::is_invocable-backed version.
template<typename _F,
         typename... _Args>
struct is_callable
{
    static constexpr bool value =
        internal::callable_helper<void, _F, _Args...>::value;
};

// is_predicate
//   trait: evaluates to true if _F is callable with _Arg and returns
// a type contextually convertible to bool (satisfies Predicate).
template<typename _F,
         typename _Arg>
struct is_predicate
{
    static constexpr bool value =
        internal::callable_returns_bool_helper<void, _F, _Arg>::value;
};

// is_binary_predicate
//   trait: evaluates to true if _F is callable with _Arg1 and _Arg2
// and returns a type contextually convertible to bool (satisfies
// BinaryPredicate).
template<typename _F,
         typename _Arg1,
         typename _Arg2>
struct is_binary_predicate
{
    static constexpr bool value =
        internal::callable_returns_bool_helper<
            void, _F, _Arg1, _Arg2>::value;
};

// is_compare
//   trait: evaluates to true if _F is a BinaryPredicate over (_T, _T)
// (satisfies Compare structurally).
// note: cannot verify strict-weak-ordering semantics at compile time;
// this is a structural check only.
template<typename _F,
         typename _T>
struct is_compare
{
    static constexpr bool value =
        is_binary_predicate<_F, const _T&, const _T&>::value;
};


// =========================================================================
// VII. CONTAINER TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_container_types
    //   trait: detects required Container nested types: value_type,
    // reference, const_reference, iterator, const_iterator,
    // difference_type, size_type.
    template<typename _C,
             typename = void>
    struct has_container_types
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_container_types<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::value_type>(),
            std::declval<typename _C::reference>(),
            std::declval<typename _C::const_reference>(),
            std::declval<typename _C::iterator>(),
            std::declval<typename _C::const_iterator>(),
            std::declval<typename _C::difference_type>(),
            std::declval<typename _C::size_type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_container_ops
    //   trait: detects required Container member functions: begin(),
    // end(), cbegin(), cend(), size(), max_size(), empty().
    template<typename _C,
             typename = void>
    struct has_container_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_container_ops<_C,
        decltype(static_cast<void>(
            std::declval<const _C&>().begin(),
            std::declval<const _C&>().end(),
            std::declval<const _C&>().cbegin(),
            std::declval<const _C&>().cend(),
            std::declval<const _C&>().size(),
            std::declval<const _C&>().max_size(),
            static_cast<bool>(std::declval<const _C&>().empty())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_reversible_ops
    //   trait: detects ReversibleContainer members: reverse_iterator,
    // const_reverse_iterator, rbegin(), rend(), crbegin(), crend().
    template<typename _C,
             typename = void>
    struct has_reversible_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_reversible_ops<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::reverse_iterator>(),
            std::declval<typename _C::const_reverse_iterator>(),
            std::declval<const _C&>().rbegin(),
            std::declval<const _C&>().rend(),
            std::declval<const _C&>().crbegin(),
            std::declval<const _C&>().crend()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_allocator_aware_ops
    //   trait: detects AllocatorAwareContainer members: allocator_type,
    // get_allocator().
    template<typename _C,
             typename = void>
    struct has_allocator_aware_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_allocator_aware_ops<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::allocator_type>(),
            std::declval<const _C&>().get_allocator()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_sequence_ops
    //   trait: detects core SequenceContainer members: front(),
    // emplace(), insert(pos, val), erase(pos).
    template<typename _C,
             typename = void>
    struct has_sequence_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_sequence_ops<_C,
        decltype(static_cast<void>(
            std::declval<_C&>().front(),
            std::declval<_C&>().insert(
                std::declval<typename _C::const_iterator>(),
                std::declval<const typename _C::value_type&>()),
            std::declval<_C&>().erase(
                std::declval<typename _C::const_iterator>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_associative_ops
    //   trait: detects AssociativeContainer members: key_type,
    // key_compare, value_compare, find(key), count(key), lower_bound,
    // upper_bound, equal_range.
    template<typename _C,
             typename = void>
    struct has_associative_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_associative_ops<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::key_type>(),
            std::declval<typename _C::key_compare>(),
            std::declval<typename _C::value_compare>(),
            std::declval<const _C&>().find(
                std::declval<const typename _C::key_type&>()),
            std::declval<const _C&>().count(
                std::declval<const typename _C::key_type&>()),
            std::declval<const _C&>().lower_bound(
                std::declval<const typename _C::key_type&>()),
            std::declval<const _C&>().upper_bound(
                std::declval<const typename _C::key_type&>()),
            std::declval<const _C&>().equal_range(
                std::declval<const typename _C::key_type&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_unordered_assoc_ops
    //   trait: detects UnorderedAssociativeContainer members: hasher,
    // key_equal, hash_function(), key_eq(), bucket_count(),
    // load_factor(), max_load_factor().
    template<typename _C,
             typename = void>
    struct has_unordered_assoc_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_unordered_assoc_ops<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::hasher>(),
            std::declval<typename _C::key_equal>(),
            std::declval<const _C&>().hash_function(),
            std::declval<const _C&>().key_eq(),
            std::declval<const _C&>().bucket_count(),
            std::declval<const _C&>().load_factor(),
            std::declval<const _C&>().max_load_factor()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_container
//   trait: evaluates to true if _C satisfies the structural
// requirements of the Container named requirement: has required
// nested types and member functions.
template<typename _C>
struct is_container
{
    static constexpr bool value =
        ( internal::has_container_types<_C>::value &&
          internal::has_container_ops<_C>::value );
};

// is_reversible_container
//   trait: evaluates to true if _C satisfies the
// ReversibleContainer named requirement: is a Container with
// reverse iteration support.
template<typename _C>
struct is_reversible_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_reversible_ops<_C>::value );
};

// is_allocator_aware_container
//   trait: evaluates to true if _C satisfies the
// AllocatorAwareContainer named requirement: is a Container with
// allocator_type and get_allocator().
template<typename _C>
struct is_allocator_aware_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_allocator_aware_ops<_C>::value );
};

// is_sequence_container
//   trait: evaluates to true if _C satisfies the structural
// requirements of the SequenceContainer named requirement: is a
// Container with front(), insert(), and erase().
// note: does not verify all SequenceContainer requirements (e.g.,
// emplace_front, push_back are optional for some sequences).
template<typename _C>
struct is_sequence_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_sequence_ops<_C>::value );
};

// is_associative_container
//   trait: evaluates to true if _C satisfies the structural
// requirements of the AssociativeContainer named requirement: is a
// Container with key_type, key_compare, find(), count(), and
// range-lookup members.
template<typename _C>
struct is_associative_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_associative_ops<_C>::value );
};

// is_unordered_associative_container
//   trait: evaluates to true if _C satisfies the structural
// requirements of the UnorderedAssociativeContainer named requirement:
// is a Container with hasher, key_equal, hash_function(),
// bucket_count(), and load factor members.
template<typename _C>
struct is_unordered_associative_container
{
    static constexpr bool value =
        ( is_container<_C>::value &&
          internal::has_unordered_assoc_ops<_C>::value );
};


// =========================================================================
// VIII. CONTAINER ELEMENT TRAITS (C++11)
// =========================================================================
//   These traits test whether an element type _T can be constructed or
// destroyed in allocator-managed storage via std::allocator_traits<_A>.
// They correspond to the container element named requirements:
// DefaultInsertable, CopyInsertable, MoveInsertable,
// EmplaceConstructible, and Erasable.

NS_INTERNAL

    // can_alloc_construct
    //   trait: detects if allocator_traits<_A>::construct(a, p, args...)
    // is well-formed.
    template<typename _Void,
             typename _T,
             typename _A,
             typename... _Args>
    struct can_alloc_construct
    {
        static constexpr bool value = false;
    };

    template<typename _T,
             typename _A,
             typename... _Args>
    struct can_alloc_construct<
        decltype(static_cast<void>(
            std::allocator_traits<_A>::construct(
                std::declval<_A&>(),
                std::declval<_T*>(),
                std::declval<_Args>()...)
        )),
        _T,
        _A,
        _Args...>
    {
        static constexpr bool value = true;
    };

    // can_alloc_destroy
    //   trait: detects if allocator_traits<_A>::destroy(a, p) is
    // well-formed.
    template<typename _T,
             typename _A,
             typename = void>
    struct can_alloc_destroy
    {
        static constexpr bool value = false;
    };

    template<typename _T,
             typename _A>
    struct can_alloc_destroy<_T, _A,
        decltype(static_cast<void>(
            std::allocator_traits<_A>::destroy(
                std::declval<_A&>(),
                std::declval<_T*>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_default_insertable
//   trait: evaluates to true if _T is DefaultInsertable into a
// container using allocator _A (i.e., allocator_traits<_A>::construct
// (a, p) is well-formed).
// default: _A = std::allocator<_T>.
template<typename _T,
         typename _A = std::allocator<_T>>
struct is_default_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<void, _T, _A>::value;
};

// is_copy_insertable
//   trait: evaluates to true if _T is CopyInsertable into a container
// using allocator _A (i.e., allocator_traits<_A>::construct(a, p, v)
// is well-formed where v is a const lvalue reference).
// default: _A = std::allocator<_T>.
template<typename _T,
         typename _A = std::allocator<_T>>
struct is_copy_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<
            void, _T, _A, const _T&>::value;
};

// is_move_insertable
//   trait: evaluates to true if _T is MoveInsertable into a container
// using allocator _A (i.e., allocator_traits<_A>::construct(a, p, rv)
// is well-formed where rv is an rvalue reference).
// default: _A = std::allocator<_T>.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

template<typename _T,
         typename _A = std::allocator<_T>>
struct is_move_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<void, _T, _A, _T&&>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_emplace_constructible
//   trait: evaluates to true if _T is EmplaceConstructible into a
// container using allocator _A from _Args... (i.e.,
// allocator_traits<_A>::construct(a, p, args...) is well-formed).
// default: _A = std::allocator<_T>.
template<typename _T,
         typename _A,
         typename... _Args>
struct is_emplace_constructible
{
    static constexpr bool value =
        internal::can_alloc_construct<
            void, _T, _A, _Args...>::value;
};

// is_erasable
//   trait: evaluates to true if _T is Erasable from a container using
// allocator _A (i.e., allocator_traits<_A>::destroy(a, p) is
// well-formed).
// default: _A = std::allocator<_T>.
template<typename _T,
         typename _A = std::allocator<_T>>
struct is_erasable
{
    static constexpr bool value =
        internal::can_alloc_destroy<_T, _A>::value;
};


// =========================================================================
// IX.  ITERATOR TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_iterator_traits
    //   trait: detects if std::iterator_traits<_I> provides the five
    // required nested types: value_type, difference_type, reference,
    // pointer, iterator_category.
    template<typename _I,
             typename = void>
    struct has_iterator_traits
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_iterator_traits<_I,
        decltype(static_cast<void>(
            std::declval<typename std::iterator_traits<_I>::value_type>(),
            std::declval<typename std::iterator_traits<_I>::difference_type>(),
            std::declval<typename std::iterator_traits<_I>::reference>(),
            std::declval<typename std::iterator_traits<_I>::pointer>(),
            std::declval<typename std::iterator_traits<_I>::iterator_category>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_legacy_iterator_ops
    //   trait: detects LegacyIterator operations: *it and ++it.
    template<typename _I,
             typename = void>
    struct has_legacy_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_legacy_iterator_ops<_I,
        decltype(static_cast<void>(
            *std::declval<_I&>(),
            ++std::declval<_I&>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_input_iterator_ops
    //   trait: detects LegacyInputIterator operations: ==, !=, it->m,
    // *it (returning reference).
    template<typename _I,
             typename = void>
    struct has_input_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_input_iterator_ops<_I,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const _I&>() ==
                              std::declval<const _I&>()),
            static_cast<bool>(std::declval<const _I&>() !=
                              std::declval<const _I&>()),
            std::declval<const _I&>().operator->(),
            *std::declval<_I&>()++
        ))>
    {
        static constexpr bool value = true;
    };

    // has_input_iterator_ops (pointer specialization fallback)
    //   trait: pointers satisfy LegacyInputIterator trivially.
    template<typename _T>
    struct has_input_iterator_ops<_T*,
        decltype(static_cast<void>(0))>
    {
        static constexpr bool value = true;
    };

    // has_output_iterator_ops
    //   trait: detects LegacyOutputIterator operations: *it = v,
    // ++it, it++.
    template<typename _I,
             typename = void>
    struct has_output_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_output_iterator_ops<_I,
        decltype(static_cast<void>(
            *std::declval<_I&>() =
                std::declval<
                    typename std::iterator_traits<_I>::value_type>(),
            ++std::declval<_I&>(),
            std::declval<_I&>()++
        ))>
    {
        static constexpr bool value = true;
    };

    // has_bidirectional_ops
    //   trait: detects LegacyBidirectionalIterator operations:
    // --it, it--, *it--.
    template<typename _I,
             typename = void>
    struct has_bidirectional_ops
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_bidirectional_ops<_I,
        decltype(static_cast<void>(
            --std::declval<_I&>(),
            std::declval<_I&>()--,
            *std::declval<_I&>()--
        ))>
    {
        static constexpr bool value = true;
    };

    // has_random_access_ops
    //   trait: detects LegacyRandomAccessIterator operations:
    // it+=n, it-=n, it+n, n+it, it-n, it-it, it[n], <, >, <=, >=.
    template<typename _I,
             typename = void>
    struct has_random_access_ops
    {
        static constexpr bool value = false;
    };

    template<typename _I>
    struct has_random_access_ops<_I,
        decltype(static_cast<void>(
            std::declval<_I&>() +=
                std::declval<
                    typename std::iterator_traits<_I>::difference_type>(),
            std::declval<_I&>() -=
                std::declval<
                    typename std::iterator_traits<_I>::difference_type>(),
            std::declval<const _I&>() +
                std::declval<
                    typename std::iterator_traits<_I>::difference_type>(),
            std::declval<
                typename std::iterator_traits<_I>::difference_type>() +
                std::declval<const _I&>(),
            std::declval<const _I&>() -
                std::declval<
                    typename std::iterator_traits<_I>::difference_type>(),
            std::declval<const _I&>() - std::declval<const _I&>(),
            std::declval<const _I&>()[
                std::declval<
                    typename std::iterator_traits<_I>::difference_type>()],
            static_cast<bool>(std::declval<const _I&>() <
                              std::declval<const _I&>()),
            static_cast<bool>(std::declval<const _I&>() >
                              std::declval<const _I&>()),
            static_cast<bool>(std::declval<const _I&>() <=
                              std::declval<const _I&>()),
            static_cast<bool>(std::declval<const _I&>() >=
                              std::declval<const _I&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // is_iterator_category_at_least
    //   trait: detects if iterator_traits<_I>::iterator_category is
    // derived from _Cat (i.e., at least as strong as _Cat).
    template<typename _I,
             typename _Cat,
             typename = void>
    struct is_iterator_category_at_least
    {
        static constexpr bool value = false;
    };

    template<typename _I,
             typename _Cat>
    struct is_iterator_category_at_least<_I, _Cat,
        typename std::enable_if<
            std::is_base_of<
                _Cat,
                typename std::iterator_traits<_I>::iterator_category
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_legacy_iterator
//   trait: evaluates to true if _I satisfies the LegacyIterator named
// requirement: has iterator_traits nested types, is dereferenceable,
// is pre-incrementable, and is CopyConstructible/CopyAssignable/
// Destructible.
template<typename _I>
struct is_legacy_iterator
{
    static constexpr bool value =
        ( internal::has_iterator_traits<_I>::value     &&
          internal::has_legacy_iterator_ops<_I>::value  &&
          std::is_copy_constructible<_I>::value         &&
          std::is_copy_assignable<_I>::value            &&
          std::is_destructible<_I>::value );
};

// is_legacy_input_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyInputIterator named requirement: LegacyIterator with ==, !=,
// arrow operator, and post-increment dereference.
template<typename _I>
struct is_legacy_input_iterator
{
    static constexpr bool value =
        ( is_legacy_iterator<_I>::value &&
          internal::has_input_iterator_ops<_I>::value );
};

// is_legacy_output_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyOutputIterator named requirement: *it = val is well-formed
// and incrementable.
template<typename _I>
struct is_legacy_output_iterator
{
    static constexpr bool value =
        ( internal::has_iterator_traits<_I>::value     &&
          internal::has_output_iterator_ops<_I>::value );
};

// is_legacy_forward_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyForwardIterator named requirement: LegacyInputIterator with
// DefaultConstructible and at least forward_iterator_tag.
// note: multi-pass guarantee cannot be verified at compile time.
template<typename _I>
struct is_legacy_forward_iterator
{
    static constexpr bool value =
        ( is_legacy_input_iterator<_I>::value                   &&
          std::is_default_constructible<_I>::value               &&
          internal::is_iterator_category_at_least<
              _I, std::forward_iterator_tag>::value );
};

// is_legacy_bidirectional_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyBidirectionalIterator named requirement:
// LegacyForwardIterator with --, it--, and *it--.
template<typename _I>
struct is_legacy_bidirectional_iterator
{
    static constexpr bool value =
        ( is_legacy_forward_iterator<_I>::value       &&
          internal::has_bidirectional_ops<_I>::value   &&
          internal::is_iterator_category_at_least<
              _I, std::bidirectional_iterator_tag>::value );
};

// is_legacy_random_access_iterator
//   trait: evaluates to true if _I satisfies the
// LegacyRandomAccessIterator named requirement:
// LegacyBidirectionalIterator with +=, -=, +, -, [], and all
// relational operators.
template<typename _I>
struct is_legacy_random_access_iterator
{
    static constexpr bool value =
        ( is_legacy_bidirectional_iterator<_I>::value   &&
          internal::has_random_access_ops<_I>::value    &&
          internal::is_iterator_category_at_least<
              _I, std::random_access_iterator_tag>::value );
};


// =========================================================================
// X.   STREAM I/O TRAITS (C++11)
// =========================================================================
//   These traits detect whether a type supports the stream interfaces
// associated with the stream I/O function named requirements. The
// named requirements themselves apply to functions (behavioral
// contracts), so these traits check structural compatibility.

NS_INTERNAL

    // has_unformatted_input_ops
    //   trait: detects if _S supports unformatted input stream
    // operations: get(), read(buf, n), gcount().
    template<typename _S,
             typename = void>
    struct has_unformatted_input_ops
    {
        static constexpr bool value = false;
    };

    template<typename _S>
    struct has_unformatted_input_ops<_S,
        decltype(static_cast<void>(
            std::declval<_S&>().get(),
            std::declval<_S&>().read(
                std::declval<typename _S::char_type*>(),
                std::declval<std::streamsize>()),
            std::declval<_S&>().gcount()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_formatted_input_ops
    //   trait: detects if _S supports formatted input via operator>>
    // with an int (representative arithmetic type).
    template<typename _S,
             typename = void>
    struct has_formatted_input_ops
    {
        static constexpr bool value = false;
    };

    template<typename _S>
    struct has_formatted_input_ops<_S,
        decltype(static_cast<void>(
            std::declval<_S&>() >> std::declval<int&>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_unformatted_output_ops
    //   trait: detects if _S supports unformatted output stream
    // operations: put(ch), write(buf, n).
    template<typename _S,
             typename = void>
    struct has_unformatted_output_ops
    {
        static constexpr bool value = false;
    };

    template<typename _S>
    struct has_unformatted_output_ops<_S,
        decltype(static_cast<void>(
            std::declval<_S&>().put(
                std::declval<typename _S::char_type>()),
            std::declval<_S&>().write(
                std::declval<const typename _S::char_type*>(),
                std::declval<std::streamsize>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_formatted_output_ops
    //   trait: detects if _S supports formatted output via operator<<
    // with an int (representative arithmetic type).
    template<typename _S,
             typename = void>
    struct has_formatted_output_ops
    {
        static constexpr bool value = false;
    };

    template<typename _S>
    struct has_formatted_output_ops<_S,
        decltype(static_cast<void>(
            std::declval<_S&>() << std::declval<int>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_unformatted_input_stream
//   trait: evaluates to true if _S supports unformatted input stream
// operations (get, read, gcount) as required by the
// UnformattedInputFunction named requirement.
template<typename _S>
struct is_unformatted_input_stream
{
    static constexpr bool value =
        internal::has_unformatted_input_ops<_S>::value;
};

// is_formatted_input_stream
//   trait: evaluates to true if _S supports formatted input via
// operator>> as required by the FormattedInputFunction named
// requirement.
template<typename _S>
struct is_formatted_input_stream
{
    static constexpr bool value =
        internal::has_formatted_input_ops<_S>::value;
};

// is_unformatted_output_stream
//   trait: evaluates to true if _S supports unformatted output
// stream operations (put, write) as required by the
// UnformattedOutputFunction named requirement.
template<typename _S>
struct is_unformatted_output_stream
{
    static constexpr bool value =
        internal::has_unformatted_output_ops<_S>::value;
};

// is_formatted_output_stream
//   trait: evaluates to true if _S supports formatted output via
// operator<< as required by the FormattedOutputFunction named
// requirement.
template<typename _S>
struct is_formatted_output_stream
{
    static constexpr bool value =
        internal::has_formatted_output_ops<_S>::value;
};


// =========================================================================
// XI.  RANDOM NUMBER GENERATION TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_seed_sequence_ops
    //   trait: detects SeedSequence members: result_type, size(),
    // param(out), generate(first, last).
    template<typename _S,
             typename = void>
    struct has_seed_sequence_ops
    {
        static constexpr bool value = false;
    };

    template<typename _S>
    struct has_seed_sequence_ops<_S,
        decltype(static_cast<void>(
            std::declval<typename _S::result_type>(),
            std::declval<const _S&>().size(),
            std::declval<const _S&>().param(
                std::declval<typename _S::result_type*>()),
            std::declval<_S&>().generate(
                std::declval<unsigned int*>(),
                std::declval<unsigned int*>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_urbg_ops
    //   trait: detects UniformRandomBitGenerator members: result_type,
    // operator(), min(), max(). Verifies result_type is unsigned
    // integral.
    template<typename _G,
             typename = void>
    struct has_urbg_ops
    {
        static constexpr bool value = false;
    };

    template<typename _G>
    struct has_urbg_ops<_G,
        typename std::enable_if<
            std::is_unsigned<typename _G::result_type>::value,
            decltype(static_cast<void>(
                std::declval<_G&>()(),
                _G::min(),
                _G::max()
            ))
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_engine_ops
    //   trait: detects RandomNumberEngine members beyond URBG:
    // seed(), seed(val), discard(n), ==, !=.
    template<typename _E,
             typename = void>
    struct has_engine_ops
    {
        static constexpr bool value = false;
    };

    template<typename _E>
    struct has_engine_ops<_E,
        decltype(static_cast<void>(
            std::declval<_E&>().seed(),
            std::declval<_E&>().seed(
                std::declval<typename _E::result_type>()),
            std::declval<_E&>().discard(
                std::declval<unsigned long long>()),
            static_cast<bool>(std::declval<const _E&>() ==
                              std::declval<const _E&>()),
            static_cast<bool>(std::declval<const _E&>() !=
                              std::declval<const _E&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_engine_adaptor_ops
    //   trait: detects RandomNumberEngineAdaptor member: base().
    template<typename _E,
             typename = void>
    struct has_engine_adaptor_ops
    {
        static constexpr bool value = false;
    };

    template<typename _E>
    struct has_engine_adaptor_ops<_E,
        decltype(static_cast<void>(
            std::declval<const _E&>().base()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_distribution_ops
    //   trait: detects RandomNumberDistribution members: result_type,
    // param_type, operator()(gen), param(), min(), max(), reset().
    template<typename _D,
             typename = void>
    struct has_distribution_ops
    {
        static constexpr bool value = false;
    };

    template<typename _D>
    struct has_distribution_ops<_D,
        decltype(static_cast<void>(
            std::declval<typename _D::result_type>(),
            std::declval<typename _D::param_type>(),
            std::declval<_D&>()(std::declval<std::mt19937&>()),
            std::declval<const _D&>().param(),
            std::declval<const _D&>().min(),
            std::declval<const _D&>().max(),
            std::declval<_D&>().reset()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_seed_sequence
//   trait: evaluates to true if _S satisfies the structural
// requirements of the SeedSequence named requirement: has
// result_type, size(), param(), and generate().
template<typename _S>
struct is_seed_sequence
{
    static constexpr bool value =
        internal::has_seed_sequence_ops<_S>::value;
};

// is_uniform_random_bit_generator
//   trait: evaluates to true if _G satisfies the
// UniformRandomBitGenerator named requirement: has unsigned integral
// result_type, operator(), min(), and max().
template<typename _G>
struct is_uniform_random_bit_generator
{
    static constexpr bool value =
        internal::has_urbg_ops<_G>::value;
};

// is_random_number_engine
//   trait: evaluates to true if _E satisfies the structural
// requirements of the RandomNumberEngine named requirement: is a
// UniformRandomBitGenerator with seed(), seed(val), discard(n), and
// equality comparison.
template<typename _E>
struct is_random_number_engine
{
    static constexpr bool value =
        ( is_uniform_random_bit_generator<_E>::value &&
          internal::has_engine_ops<_E>::value );
};

// is_random_number_engine_adaptor
//   trait: evaluates to true if _E satisfies the structural
// requirements of the RandomNumberEngineAdaptor named requirement:
// is a RandomNumberEngine with base() returning the adapted engine.
template<typename _E>
struct is_random_number_engine_adaptor
{
    static constexpr bool value =
        ( is_random_number_engine<_E>::value &&
          internal::has_engine_adaptor_ops<_E>::value );
};

// is_random_number_distribution
//   trait: evaluates to true if _D satisfies the structural
// requirements of the RandomNumberDistribution named requirement:
// has result_type, param_type, operator()(gen), param(), min(),
// max(), and reset().
template<typename _D>
struct is_random_number_distribution
{
    static constexpr bool value =
        internal::has_distribution_ops<_D>::value;
};


// =========================================================================
// XII. CONCURRENCY TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_basic_lockable_ops
    //   trait: detects BasicLockable members: lock(), unlock().
    template<typename _M,
             typename = void>
    struct has_basic_lockable_ops
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_basic_lockable_ops<_M,
        decltype(static_cast<void>(
            std::declval<_M&>().lock(),
            std::declval<_M&>().unlock()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_try_lock
    //   trait: detects Lockable member: try_lock() returning bool.
    template<typename _M,
             typename = void>
    struct has_try_lock
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_try_lock<_M,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<_M&>().try_lock())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_timed_lock_ops
    //   trait: detects TimedLockable members: try_lock_for(dur),
    // try_lock_until(tp) returning bool.
    template<typename _M,
             typename = void>
    struct has_timed_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_timed_lock_ops<_M,
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<_M&>().try_lock_for(
                    std::declval<std::chrono::milliseconds>())),
            static_cast<bool>(
                std::declval<_M&>().try_lock_until(
                    std::declval<
                        std::chrono::steady_clock::time_point>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // has_shared_lock_ops
    //   trait: detects SharedLockable members: lock_shared(),
    // unlock_shared(), try_lock_shared() returning bool.
    template<typename _M,
             typename = void>
    struct has_shared_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_shared_lock_ops<_M,
        decltype(static_cast<void>(
            std::declval<_M&>().lock_shared(),
            std::declval<_M&>().unlock_shared(),
            static_cast<bool>(
                std::declval<_M&>().try_lock_shared())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_shared_timed_lock_ops
    //   trait: detects SharedTimedLockable members:
    // try_lock_shared_for(dur), try_lock_shared_until(tp).
    template<typename _M,
             typename = void>
    struct has_shared_timed_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename _M>
    struct has_shared_timed_lock_ops<_M,
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<_M&>().try_lock_shared_for(
                    std::declval<std::chrono::milliseconds>())),
            static_cast<bool>(
                std::declval<_M&>().try_lock_shared_until(
                    std::declval<
                        std::chrono::steady_clock::time_point>()))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_basic_lockable
//   trait: evaluates to true if _M satisfies the BasicLockable named
// requirement: has lock() and unlock().
template<typename _M>
struct is_basic_lockable
{
    static constexpr bool value =
        internal::has_basic_lockable_ops<_M>::value;
};

// is_lockable
//   trait: evaluates to true if _M satisfies the Lockable named
// requirement: BasicLockable with try_lock() returning bool.
template<typename _M>
struct is_lockable
{
    static constexpr bool value =
        ( is_basic_lockable<_M>::value &&
          internal::has_try_lock<_M>::value );
};

// is_timed_lockable
//   trait: evaluates to true if _M satisfies the TimedLockable named
// requirement: Lockable with try_lock_for() and try_lock_until().
template<typename _M>
struct is_timed_lockable
{
    static constexpr bool value =
        ( is_lockable<_M>::value &&
          internal::has_timed_lock_ops<_M>::value );
};

// is_shared_lockable
//   trait: evaluates to true if _M satisfies the SharedLockable
// named requirement: lock_shared(), unlock_shared(),
// try_lock_shared().
template<typename _M>
struct is_shared_lockable
{
    static constexpr bool value =
        internal::has_shared_lock_ops<_M>::value;
};

// is_shared_timed_lockable
//   trait: evaluates to true if _M satisfies the
// SharedTimedLockable named requirement: SharedLockable with
// try_lock_shared_for() and try_lock_shared_until().
template<typename _M>
struct is_shared_timed_lockable
{
    static constexpr bool value =
        ( is_shared_lockable<_M>::value &&
          internal::has_shared_timed_lock_ops<_M>::value );
};

// is_mutex
//   trait: evaluates to true if _M satisfies the Mutex named
// requirement: Lockable, DefaultConstructible, Destructible, and
// not copyable or movable.
template<typename _M>
struct is_mutex
{
    static constexpr bool value =
        ( is_lockable<_M>::value                       &&
          std::is_default_constructible<_M>::value      &&
          std::is_destructible<_M>::value               &&
          !std::is_copy_constructible<_M>::value        &&
          !std::is_move_constructible<_M>::value        &&
          !std::is_copy_assignable<_M>::value           &&
          !std::is_move_assignable<_M>::value );
};

// is_timed_mutex
//   trait: evaluates to true if _M satisfies the TimedMutex named
// requirement: Mutex with TimedLockable.
template<typename _M>
struct is_timed_mutex
{
    static constexpr bool value =
        ( is_mutex<_M>::value &&
          is_timed_lockable<_M>::value );
};

// is_shared_mutex
//   trait: evaluates to true if _M satisfies the SharedMutex named
// requirement: Mutex with SharedLockable.
// note: std::shared_mutex is C++17. This trait works for any type
// providing the correct interface.
template<typename _M>
struct is_shared_mutex
{
    static constexpr bool value =
        ( is_mutex<_M>::value &&
          is_shared_lockable<_M>::value );
};

// is_shared_timed_mutex
//   trait: evaluates to true if _M satisfies the SharedTimedMutex
// named requirement: Mutex with TimedLockable and
// SharedTimedLockable.
template<typename _M>
struct is_shared_timed_mutex
{
    static constexpr bool value =
        ( is_mutex<_M>::value              &&
          is_timed_lockable<_M>::value     &&
          is_shared_timed_lockable<_M>::value );
};


// =========================================================================
// XIII. TYPE TRAIT META-TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_value_and_type
    //   trait: detects if _T has a nested ::value (convertible to
    // bool) and ::type nested typedef. Used for UnaryTypeTrait and
    // BinaryTypeTrait detection.
    template<typename _T,
             typename = void>
    struct has_value_and_type
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct has_value_and_type<_T,
        decltype(static_cast<void>(
            static_cast<bool>(_T::value),
            std::declval<typename _T::type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // derives_integral_constant
    //   trait: detects if _T derives from std::integral_constant.
    template<typename _T,
             typename = void>
    struct derives_integral_constant
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct derives_integral_constant<_T,
        typename std::enable_if<
            std::is_base_of<
                std::integral_constant<bool, true>,
                _T
            >::value ||
            std::is_base_of<
                std::integral_constant<bool, false>,
                _T
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_nested_type
    //   trait: detects if _T has a nested ::type typedef.
    template<typename _T,
             typename = void>
    struct has_nested_type
    {
        static constexpr bool value = false;
    };

    template<typename _T>
    struct has_nested_type<_T,
        decltype(static_cast<void>(
            std::declval<typename _T::type>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_unary_type_trait
//   trait: evaluates to true if _T satisfies the UnaryTypeTrait
// named requirement: has a nested ::value convertible to bool, a
// nested ::type, and derives from std::integral_constant.
// note: cannot verify the template-arity requirement (that _T takes
// exactly one type parameter) at this level.
template<typename _T>
struct is_unary_type_trait
{
    static constexpr bool value =
        ( internal::has_value_and_type<_T>::value &&
          internal::derives_integral_constant<_T>::value );
};

// is_binary_type_trait
//   trait: evaluates to true if _T satisfies the BinaryTypeTrait
// named requirement: has ::value, ::type, and derives from
// integral_constant.
// note: structurally identical to UnaryTypeTrait. The distinction
// is template arity (two parameters), which cannot be verified
// without instantiation. Provided as a semantic alias.
template<typename _T>
struct is_binary_type_trait
{
    static constexpr bool value =
        ( internal::has_value_and_type<_T>::value &&
          internal::derives_integral_constant<_T>::value );
};

// is_transformation_trait
//   trait: evaluates to true if _T satisfies the
// TransformationTrait named requirement: has a nested ::type typedef.
template<typename _T>
struct is_transformation_trait
{
    static constexpr bool value =
        internal::has_nested_type<_T>::value;
};


// =========================================================================
// XIV. CLOCK AND TRIVIALCLOCK TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_clock_ops
    //   trait: detects Clock members: rep, period, duration,
    // time_point, is_steady, now().
    template<typename _C,
             typename = void>
    struct has_clock_ops
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct has_clock_ops<_C,
        decltype(static_cast<void>(
            std::declval<typename _C::rep>(),
            std::declval<typename _C::period>(),
            std::declval<typename _C::duration>(),
            std::declval<typename _C::time_point>(),
            static_cast<bool>(_C::is_steady),
            _C::now()
        ))>
    {
        static constexpr bool value = true;
    };

    // is_now_noexcept
    //   trait: detects if _C::now() is noexcept.
    template<typename _C,
             typename = void>
    struct is_now_noexcept
    {
        static constexpr bool value = false;
    };

    template<typename _C>
    struct is_now_noexcept<_C,
        typename std::enable_if<
            noexcept(_C::now())
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_clock
//   trait: evaluates to true if _C satisfies the Clock named
// requirement: has rep, period, duration, time_point nested types,
// an is_steady static member, and a now() static function.
template<typename _C>
struct is_clock
{
    static constexpr bool value =
        internal::has_clock_ops<_C>::value;
};

// is_trivial_clock
//   trait: evaluates to true if _C satisfies the TrivialClock named
// requirement: is a Clock whose now() is noexcept, and whose types
// satisfy Clock consistency requirements.
// note: some TrivialClock semantic requirements (monotonicity,
// representation fidelity) cannot be verified at compile time.
template<typename _C>
struct is_trivial_clock
{
    static constexpr bool value =
        ( is_clock<_C>::value &&
          internal::is_now_noexcept<_C>::value );
};


// =========================================================================
// XV.  CHARTRAITS TRAIT (C++11)
// =========================================================================

NS_INTERNAL

    // has_char_traits_ops
    //   trait: detects CharTraits members: char_type, int_type,
    // off_type, pos_type, state_type, eq(), lt(), length(), move(),
    // copy(), assign(), not_eof(), to_char_type(), to_int_type(),
    // eof().
    template<typename _Tr,
             typename = void>
    struct has_char_traits_ops
    {
        static constexpr bool value = false;
    };

    template<typename _Tr>
    struct has_char_traits_ops<_Tr,
        decltype(static_cast<void>(
            std::declval<typename _Tr::char_type>(),
            std::declval<typename _Tr::int_type>(),
            std::declval<typename _Tr::off_type>(),
            std::declval<typename _Tr::pos_type>(),
            std::declval<typename _Tr::state_type>(),
            static_cast<bool>(
                _Tr::eq(std::declval<typename _Tr::char_type>(),
                        std::declval<typename _Tr::char_type>())),
            static_cast<bool>(
                _Tr::lt(std::declval<typename _Tr::char_type>(),
                        std::declval<typename _Tr::char_type>())),
            _Tr::length(
                std::declval<const typename _Tr::char_type*>()),
            _Tr::to_char_type(
                std::declval<typename _Tr::int_type>()),
            _Tr::to_int_type(
                std::declval<typename _Tr::char_type>()),
            _Tr::eof(),
            _Tr::not_eof(
                std::declval<typename _Tr::int_type>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_char_traits
//   trait: evaluates to true if _Tr satisfies the structural
// requirements of the CharTraits named requirement: has the required
// nested types and static member functions.
template<typename _Tr>
struct is_char_traits
{
    static constexpr bool value =
        internal::has_char_traits_ops<_Tr>::value;
};


// =========================================================================
// XVI. BITMASKTYPE TRAIT (C++11)
// =========================================================================

NS_INTERNAL

    // has_bitmask_ops
    //   trait: detects BitmaskType operations: &, |, ^, ~, &=, |=,
    // ^=.
    template<typename _B,
             typename = void>
    struct has_bitmask_ops
    {
        static constexpr bool value = false;
    };

    template<typename _B>
    struct has_bitmask_ops<_B,
        decltype(static_cast<void>(
            std::declval<_B>() & std::declval<_B>(),
            std::declval<_B>() | std::declval<_B>(),
            std::declval<_B>() ^ std::declval<_B>(),
            ~std::declval<_B>(),
            std::declval<_B&>() &= std::declval<_B>(),
            std::declval<_B&>() |= std::declval<_B>(),
            std::declval<_B&>() ^= std::declval<_B>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_bitmask_type
//   trait: evaluates to true if _B satisfies the BitmaskType named
// requirement: supports &, |, ^, ~, &=, |=, ^= operators.
template<typename _B>
struct is_bitmask_type
{
    static constexpr bool value =
        internal::has_bitmask_ops<_B>::value;
};


// =========================================================================
// XVII. NUMERICTYPE TRAIT (C++11)
// =========================================================================

// is_numeric_type
//   trait: evaluates to true if _T satisfies the NumericType named
// requirement: an arithmetic type, or a cv-qualified version
// thereof. For user-defined numeric types, use in conjunction with
// std::numeric_limits<_T>::is_specialized.
// note: the standard defines NumericType as the set of cv-unqualified
// arithmetic types. This trait accepts cv-qualified types as well
// for practical use.
template<typename _T>
struct is_numeric_type
{
    static constexpr bool value =
        std::is_arithmetic<
            typename std::remove_cv<_T>::type>::value;
};


// =========================================================================
// XVIII. REGEXTRAITS TRAIT (C++11)
// =========================================================================

NS_INTERNAL

    // has_regex_traits_ops
    //   trait: detects RegexTraits members: char_type, string_type,
    // locale_type, char_class_type, length(), translate(),
    // translate_nocase(), value(), imbue(), getloc().
    template<typename _Tr,
             typename = void>
    struct has_regex_traits_ops
    {
        static constexpr bool value = false;
    };

    template<typename _Tr>
    struct has_regex_traits_ops<_Tr,
        decltype(static_cast<void>(
            std::declval<typename _Tr::char_type>(),
            std::declval<typename _Tr::string_type>(),
            std::declval<typename _Tr::locale_type>(),
            std::declval<typename _Tr::char_class_type>(),
            _Tr::length(
                std::declval<const typename _Tr::char_type*>()),
            std::declval<const _Tr&>().translate(
                std::declval<typename _Tr::char_type>()),
            std::declval<const _Tr&>().translate_nocase(
                std::declval<typename _Tr::char_type>()),
            std::declval<const _Tr&>().value(
                std::declval<typename _Tr::char_type>(), int(10)),
            std::declval<_Tr&>().imbue(
                std::declval<typename _Tr::locale_type>()),
            std::declval<const _Tr&>().getloc()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_regex_traits
//   trait: evaluates to true if _Tr satisfies the structural
// requirements of the RegexTraits named requirement: has the
// required nested types and static/member functions.
template<typename _Tr>
struct is_regex_traits
{
    static constexpr bool value =
        internal::has_regex_traits_ops<_Tr>::value;
};


// =========================================================================
// XIX. LITERALTYPE TRAIT (C++11)
// =========================================================================

// is_literal_type_v_
//   trait: evaluates to true if _T satisfies the LiteralType named
// requirement. A literal type is:
//   - a scalar type, or
//   - a reference type, or
//   - a class type with a trivial destructor and at least one
//     constexpr constructor (not a copy/move constructor), or
//   - an array of literal type.
// note: in C++17, std::is_literal_type was deprecated. In C++20 it
// was removed. This implementation uses a conservative structural
// approximation: scalar || reference || (trivially destructible &&
// default constructible) || array-of-literal.
// note: named with trailing underscore to avoid collision with the
// deprecated std::is_literal_type.
template<typename _T>
struct is_literal_type_
{
    static constexpr bool value =
        ( std::is_scalar<_T>::value        ||
          std::is_reference<_T>::value     ||
          ( std::is_class<_T>::value                    &&
            std::is_trivially_destructible<_T>::value   &&
            std::is_default_constructible<_T>::value )  ||
          ( std::is_array<_T>::value                    &&
            std::is_trivially_destructible<
                typename std::remove_all_extents<_T>::type>::value ) );
};


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED11_
