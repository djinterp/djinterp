/*******************************************************************************
* djinterp [cpp]                                                 cpp_named11.hpp
*
* C++11 named requirement type traits:
*   Provides precise compile-time detection of fundamental named requirements
* using the C++11 <type_traits> header. This supersedes the sizeof-based
* SFINAE approach from cpp_named98.hpp with compiler-intrinsic-backed traits
* that correctly handle private/deleted special members.
*
*   C++11 introduces rvalue references, enabling MoveConstructible and
* MoveAssignable detection for the first time. All six core named
* requirements are now available, along with nothrow variants.
*
* SUPPORTED REQUIREMENTS:
*   - DefaultConstructible  (is_default_constructible)
*   - MoveConstructible     (is_move_constructible)        [NEW in C++11]
*   - CopyConstructible     (is_copy_constructible)
*   - MoveAssignable        (is_move_assignable)           [NEW in C++11]
*   - CopyAssignable        (is_copy_assignable)
*   - Destructible          (is_destructible)
*   - nothrow variants of all of the above                 [NEW in C++11]
*   - ScalarType            (is_scalar)                    [NEW in C++11]
*   - TriviallyCopyable     (is_trivially_copyable)        [NEW in C++11]
*   - TrivialType           (is_trivial)                   [NEW in C++11]
*   - StandardLayoutType    (is_standard_layout)           [NEW in C++11]
*   - PODType               (is_pod)                       [NEW in C++11]
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
*   djinterp::re_std::is_default_constructible<T>::value
*   djinterp::re_std::is_nothrow_move_constructible<T>::value
*   (etc.)
*
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named11.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED11_HPP
#define DJINTERP_META_STL_CPP_NAMED11_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named11.hpp can only be used in C++ compilation mode"
#endif

// include C++98/03 layer (active only when C++11 is not available)
#include "cpp_named03.hpp"

// C++11 trait definitions are active for C++11 and all later standards
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <chrono>
#include <functional>
#include <ios>
#include <iterator>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   NAMED REQUIREMENT TRAITS (C++11)
// =========================================================================

// is_default_constructible
//   trait: evaluates to true if T is default-constructible.
// backed by compiler intrinsics via std::is_default_constructible.
template<typename T>
struct is_default_constructible
{
    static constexpr bool value =
        std::is_default_constructible<T>::value;
};

// is_copy_constructible
//   trait: evaluates to true if T is copy-constructible from const T&.
// backed by compiler intrinsics via std::is_copy_constructible.
template<typename T>
struct is_copy_constructible
{
    static constexpr bool value =
        std::is_copy_constructible<T>::value;
};

// is_copy_assignable
//   trait: evaluates to true if T is copy-assignable from const T&.
// backed by compiler intrinsics via std::is_copy_assignable.
template<typename T>
struct is_copy_assignable
{
    static constexpr bool value =
        std::is_copy_assignable<T>::value;
};

// is_destructible
//   trait: evaluates to true if T has an accessible, non-deleted
// destructor.
// backed by compiler intrinsics via std::is_destructible.
template<typename T>
struct is_destructible
{
    static constexpr bool value =
        std::is_destructible<T>::value;
};


// =========================================================================
// II.  MOVE-AWARE TRAITS (C++11, requires rvalue references)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_move_constructible
//   trait: evaluates to true if T is move-constructible from T&&.
// requires rvalue reference support (C++11).
template<typename T>
struct is_move_constructible
{
    static constexpr bool value =
        std::is_move_constructible<T>::value;
};

// is_move_assignable
//   trait: evaluates to true if T is move-assignable from T&&.
// requires rvalue reference support (C++11).
template<typename T>
struct is_move_assignable
{
    static constexpr bool value =
        std::is_move_assignable<T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// III. NOTHROW VARIANTS (C++11)
// =========================================================================

// is_nothrow_default_constructible
//   trait: evaluates to true if T is default-constructible without
// throwing exceptions.
template<typename T>
struct is_nothrow_default_constructible
{
    static constexpr bool value =
        std::is_nothrow_default_constructible<T>::value;
};

// is_nothrow_copy_constructible
//   trait: evaluates to true if T is copy-constructible without
// throwing exceptions.
template<typename T>
struct is_nothrow_copy_constructible
{
    static constexpr bool value =
        std::is_nothrow_copy_constructible<T>::value;
};

// is_nothrow_copy_assignable
//   trait: evaluates to true if T is copy-assignable without
// throwing exceptions.
template<typename T>
struct is_nothrow_copy_assignable
{
    static constexpr bool value =
        std::is_nothrow_copy_assignable<T>::value;
};

// is_nothrow_destructible
//   trait: evaluates to true if T is destructible without throwing
// exceptions.
template<typename T>
struct is_nothrow_destructible
{
    static constexpr bool value =
        std::is_nothrow_destructible<T>::value;
};

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_nothrow_move_constructible
//   trait: evaluates to true if T is move-constructible without
// throwing exceptions.
template<typename T>
struct is_nothrow_move_constructible
{
    static constexpr bool value =
        std::is_nothrow_move_constructible<T>::value;
};

// is_nothrow_move_assignable
//   trait: evaluates to true if T is move-assignable without
// throwing exceptions.
template<typename T>
struct is_nothrow_move_assignable
{
    static constexpr bool value =
        std::is_nothrow_move_assignable<T>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES


// =========================================================================
// IV.  TYPE CLASSIFICATION TRAITS (C++11)
// =========================================================================

// is_scalar
//   trait: evaluates to true if T is a scalar type (arithmetic,
// enumeration, pointer, pointer-to-member, or std::nullptr_t).
// backed by compiler intrinsics via std::is_scalar.
template<typename T>
struct is_scalar
{
    static constexpr bool value =
        std::is_scalar<T>::value;
};

// is_trivially_copyable
//   trait: evaluates to true if T is trivially copyable (can be
// safely copied via memcpy/memmove).
template<typename T>
struct is_trivially_copyable
{
    static constexpr bool value =
        std::is_trivially_copyable<T>::value;
};

// is_trivial
//   trait: evaluates to true if T is a trivial type (trivially
// default-constructible and trivially copyable).
// note: TrivialType named requirement is deprecated in C++26.
template<typename T>
struct is_trivial
{
    static constexpr bool value =
        std::is_trivial<T>::value;
};

// is_standard_layout
//   trait: evaluates to true if T satisfies the StandardLayoutType
// named requirement.
template<typename T>
struct is_standard_layout
{
    static constexpr bool value =
        std::is_standard_layout<T>::value;
};

// is_pod
//   trait: evaluates to true if T is a POD (Plain Old Data) type,
// i.e., both trivial and standard-layout.
// note: PODType named requirement is deprecated in C++20 and
// std::is_pod is removed in C++26. Prefer is_trivially_copyable with
// is_standard_layout for new code.
template<typename T>
struct is_pod
{
#if !D_ENV_LANG_IS_CPP26_OR_HIGHER
    static constexpr bool value =
        std::is_pod<T>::value;
#else
    // std::is_pod removed in C++26; equivalent definition
    static constexpr bool value =
        ( std::is_trivial<T>::value &&
          std::is_standard_layout<T>::value );
#endif
};


// =========================================================================
// V.   COMPARISON AND BOOLEAN TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // boolean_testable_helper
    //   trait: detects if T is contextually convertible to bool and
    // supports logical negation.
    template<typename T,
             typename = void>
    struct boolean_testable_helper
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct boolean_testable_helper<T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<T>())
        ))>
    {
        static constexpr bool value = true;
    };

    // equality_comparable_helper
    //   trait: detects if T supports == and != with bool-convertible
    // results.
    template<typename T,
             typename = void>
    struct equality_comparable_helper
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct equality_comparable_helper<T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const T&>() ==
                              std::declval<const T&>()),
            static_cast<bool>(std::declval<const T&>() !=
                              std::declval<const T&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // less_than_comparable_helper
    //   trait: detects if T supports < with a bool-convertible result.
    template<typename T,
             typename = void>
    struct less_than_comparable_helper
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct less_than_comparable_helper<T,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const T&>() <
                              std::declval<const T&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // swappable_helper
    //   trait: detects if T supports ADL-visible swap via
    // `using std::swap; swap(a, b)`.
    template<typename T,
             typename = void>
    struct swappable_helper
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct swappable_helper<T,
        decltype(static_cast<void>(
            (void)(std::swap(std::declval<T&>(),
                             std::declval<T&>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // callable_helper
    //   trait: detects if F is callable with Args... .
    template<typename Void,
             typename F,
             typename... Args>
    struct callable_helper
    {
        static constexpr bool value = false;
    };

    template<typename F,
             typename... Args>
    struct callable_helper<
        decltype(static_cast<void>(
            std::declval<F>()(std::declval<Args>()...)
        )),
        F,
        Args...>
    {
        static constexpr bool value = true;
    };

    // callable_returns_bool_helper
    //   trait: detects if F(Args...) is well-formed and returns a
    // type contextually convertible to bool.
    template<typename Void,
             typename F,
             typename... Args>
    struct callable_returns_bool_helper
    {
        static constexpr bool value = false;
    };

    template<typename F,
             typename... Args>
    struct callable_returns_bool_helper<
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<F>()(std::declval<Args>()...))
        )),
        F,
        Args...>
    {
        static constexpr bool value = true;
    };

    // has_operator_call
    //   trait: detects if T has a non-overloaded operator() member.
    // note: will report false for types with overloaded or templated
    // operator(). Use is_callable for a more general test.
    template<typename T,
             typename = void>
    struct has_operator_call
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct has_operator_call<T,
        decltype(static_cast<void>(&T::operator()))>
    {
        static constexpr bool value = true;
    };

    // has_hash_specialization
    //   trait: detects if std::hash<T> is well-formed and callable
    // with const T&, returning std::size_t.
    template<typename T,
             typename = void>
    struct has_hash_specialization
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct has_hash_specialization<T,
        typename std::enable_if<
            std::is_same<
                decltype(std::declval<const std::hash<T>&>()(
                    std::declval<const T&>())),
                std::size_t
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_allocator_interface
    //   trait: detects if A has value_type, allocate(n), and
    // deallocate(p, n) members as required by Allocator.
    template<typename A,
             typename = void>
    struct has_allocator_interface
    {
        static constexpr bool value = false;
    };

    template<typename A>
    struct has_allocator_interface<A,
        decltype(static_cast<void>(
            std::declval<typename A::value_type>(),
            std::declval<A&>().deallocate(
                std::declval<A&>().allocate(std::size_t(1)),
                std::size_t(1))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_boolean_testable
//   trait: evaluates to true if T is contextually convertible to
// bool (satisfies BooleanTestable).
template<typename T>
struct is_boolean_testable
{
    static constexpr bool value =
        internal::boolean_testable_helper<T>::value;
};

// is_equality_comparable
//   trait: evaluates to true if T supports == and != with
// bool-convertible results (satisfies EqualityComparable).
template<typename T>
struct is_equality_comparable
{
    static constexpr bool value =
        internal::equality_comparable_helper<T>::value;
};

// is_less_than_comparable
//   trait: evaluates to true if T supports operator< with a
// bool-convertible result (satisfies LessThanComparable).
template<typename T>
struct is_less_than_comparable
{
    static constexpr bool value =
        internal::less_than_comparable_helper<T>::value;
};

// is_swappable
//   trait: evaluates to true if T is swappable via ADL-visible swap.
// note: from C++17, std::is_swappable; for C++11 and C++14, a basic
// detection using std::swap as fallback.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
template<typename T>
struct is_swappable
{
    static constexpr bool value =
        std::is_swappable<T>::value;
};
#else
template<typename T>
struct is_swappable
{
    static constexpr bool value =
        internal::swappable_helper<T>::value;
};
#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER

// is_value_swappable
//   trait: evaluates to true if T is an iterator-like type whose
// dereferenced value is swappable (satisfies ValueSwappable).
// requires: T is dereferenceable.
template<typename T,
         typename = void>
struct is_value_swappable
{
    static constexpr bool value = false;
};

// is_value_swappable (dereferenceable specialization)
//   trait: partial specialization for dereferenceable types.
template<typename T>
struct is_value_swappable<T,
    decltype(static_cast<void>(*std::declval<T&>()))>
{
    static constexpr bool value =
        is_swappable<decltype(*std::declval<T&>())>::value;
};

// is_nullable_pointer
//   trait: evaluates to true if T satisfies the NullablePointer named
// requirement: DefaultConstructible, CopyConstructible, CopyAssignable,
// Destructible, contextually convertible to bool, and constructible/
// comparable with std::nullptr_t.
template<typename T>
struct is_nullable_pointer
{
    static constexpr bool value =
        ( std::is_default_constructible<T>::value  &&
          std::is_copy_constructible<T>::value      &&
          std::is_copy_assignable<T>::value          &&
          std::is_destructible<T>::value             &&
          std::is_constructible<T, std::nullptr_t>::value &&
          is_boolean_testable<T>::value              &&
          is_equality_comparable<T>::value );
};

// is_hashable
//   trait: evaluates to true if std::hash<T> is a valid
// specialization that accepts const T& and returns std::size_t
// (satisfies Hash requirement for T as key type).
template<typename T>
struct is_hashable
{
    static constexpr bool value =
        internal::has_hash_specialization<T>::value;
};

// is_allocator
//   trait: evaluates to true if A satisfies the basic structural
// requirements of the Allocator named requirement: has value_type,
// allocate(n), and deallocate(p, n).
// note: does not verify all Allocator requirements (e.g., propagation
// traits, construct/destroy). For full compliance, see
// std::allocator_traits.
template<typename A>
struct is_allocator
{
    static constexpr bool value =
        ( internal::has_allocator_interface<A>::value &&
          std::is_copy_constructible<A>::value        &&
          std::is_destructible<A>::value );
};


// =========================================================================
// VI.  CALLABLE AND PREDICATE TRAITS (C++11)
// =========================================================================

// is_function_object
//   trait: evaluates to true if F has a non-overloaded operator()
// member (satisfies FunctionObject structurally).
// note: reports false for types with overloaded or templated
// operator(). Use is_callable for a general invocability test.
template<typename F>
struct is_function_object
{
    static constexpr bool value =
        internal::has_operator_call<F>::value;
};

//   is_callable, is_predicate, is_binary_predicate and is_compare: from
// C++17 with INVOKE semantics, through std::is_invocable; for C++11 and C++14,
// by direct call.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// is_callable (C++17 override)
//   trait: evaluates to true if F is invocable with Args... using
// INVOKE semantics (handles pointer-to-member, etc.).
// backed by std::is_invocable (supersedes C++11 direct-call detection).
template<typename F,
         typename... Args>
struct is_callable
{
    static constexpr bool value =
        std::is_invocable<F, Args...>::value;
};

// is_predicate (C++17 override)
//   trait: evaluates to true if F is invocable with Arg and returns
// a type convertible to bool (satisfies Predicate).
template<typename F,
         typename Arg>
struct is_predicate
{
    static constexpr bool value =
        ( std::is_invocable<F, Arg>::value &&
          std::is_convertible<
              std::invoke_result_t<F, Arg>, bool>::value );
};

// is_binary_predicate (C++17 override)
//   trait: evaluates to true if F is invocable with Arg1, Arg2 and
// returns a type convertible to bool (satisfies BinaryPredicate).
template<typename F,
         typename Arg1,
         typename Arg2>
struct is_binary_predicate
{
    static constexpr bool value =
        ( std::is_invocable<F, Arg1, Arg2>::value &&
          std::is_convertible<
              std::invoke_result_t<F, Arg1, Arg2>, bool>::value );
};

// is_compare (C++17 override)
//   trait: evaluates to true if F is a BinaryPredicate over
// (const T&, const T&) (satisfies Compare structurally).
template<typename F,
         typename T>
struct is_compare
{
    static constexpr bool value =
        is_binary_predicate<F, const T&, const T&>::value;
};

#else


// is_callable
//   trait: evaluates to true if F can be invoked with Args...
// (satisfies Callable).
// note: tests direct invocation `F(Args...)` only. Does not account
// for std::invoke semantics (pointer-to-member, etc.). See
// cpp_named17.hpp for std::is_invocable-backed version.
template<typename F,
         typename... Args>
struct is_callable
{
    static constexpr bool value =
        internal::callable_helper<void, F, Args...>::value;
};

// is_predicate
//   trait: evaluates to true if F is callable with Arg and returns
// a type contextually convertible to bool (satisfies Predicate).
template<typename F,
         typename Arg>
struct is_predicate
{
    static constexpr bool value =
        internal::callable_returns_bool_helper<void, F, Arg>::value;
};

// is_binary_predicate
//   trait: evaluates to true if F is callable with Arg1 and Arg2
// and returns a type contextually convertible to bool (satisfies
// BinaryPredicate).
template<typename F,
         typename Arg1,
         typename Arg2>
struct is_binary_predicate
{
    static constexpr bool value =
        internal::callable_returns_bool_helper<
            void, F, Arg1, Arg2>::value;
};

// is_compare
//   trait: evaluates to true if F is a BinaryPredicate over (T, T)
// (satisfies Compare structurally).
// note: cannot verify strict-weak-ordering semantics at compile time;
// this is a structural check only.
template<typename F,
         typename T>
struct is_compare
{
    static constexpr bool value =
        is_binary_predicate<F, const T&, const T&>::value;
};

#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


// =========================================================================
// VII. CONTAINER TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_container_types
    //   trait: detects required Container nested types: value_type,
    // reference, const_reference, iterator, const_iterator,
    // difference_type, size_type.
    template<typename C,
             typename = void>
    struct has_container_types
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_container_types<C,
        decltype(static_cast<void>(
            std::declval<typename C::value_type>(),
            std::declval<typename C::reference>(),
            std::declval<typename C::const_reference>(),
            std::declval<typename C::iterator>(),
            std::declval<typename C::const_iterator>(),
            std::declval<typename C::difference_type>(),
            std::declval<typename C::size_type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_container_ops
    //   trait: detects required Container member functions: begin(),
    // end(), cbegin(), cend(), size(), max_size(), empty().
    template<typename C,
             typename = void>
    struct has_container_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_container_ops<C,
        decltype(static_cast<void>(
            std::declval<const C&>().begin(),
            std::declval<const C&>().end(),
            std::declval<const C&>().cbegin(),
            std::declval<const C&>().cend(),
            std::declval<const C&>().size(),
            std::declval<const C&>().max_size(),
            static_cast<bool>(std::declval<const C&>().empty())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_reversible_ops
    //   trait: detects ReversibleContainer members: reverse_iterator,
    // const_reverse_iterator, rbegin(), rend(), crbegin(), crend().
    template<typename C,
             typename = void>
    struct has_reversible_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_reversible_ops<C,
        decltype(static_cast<void>(
            std::declval<typename C::reverse_iterator>(),
            std::declval<typename C::const_reverse_iterator>(),
            std::declval<const C&>().rbegin(),
            std::declval<const C&>().rend(),
            std::declval<const C&>().crbegin(),
            std::declval<const C&>().crend()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_allocator_aware_ops
    //   trait: detects AllocatorAwareContainer members: allocator_type,
    // get_allocator().
    template<typename C,
             typename = void>
    struct has_allocator_aware_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_allocator_aware_ops<C,
        decltype(static_cast<void>(
            std::declval<typename C::allocator_type>(),
            std::declval<const C&>().get_allocator()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_sequence_ops
    //   trait: detects core SequenceContainer members: front(),
    // emplace(), insert(pos, val), erase(pos).
    template<typename C,
             typename = void>
    struct has_sequence_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_sequence_ops<C,
        decltype(static_cast<void>(
            std::declval<C&>().front(),
            std::declval<C&>().insert(
                std::declval<typename C::const_iterator>(),
                std::declval<const typename C::value_type&>()),
            std::declval<C&>().erase(
                std::declval<typename C::const_iterator>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_associative_ops
    //   trait: detects AssociativeContainer members: key_type,
    // key_compare, value_compare, find(key), count(key), lower_bound,
    // upper_bound, equal_range.
    template<typename C,
             typename = void>
    struct has_associative_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_associative_ops<C,
        decltype(static_cast<void>(
            std::declval<typename C::key_type>(),
            std::declval<typename C::key_compare>(),
            std::declval<typename C::value_compare>(),
            std::declval<const C&>().find(
                std::declval<const typename C::key_type&>()),
            std::declval<const C&>().count(
                std::declval<const typename C::key_type&>()),
            std::declval<const C&>().lower_bound(
                std::declval<const typename C::key_type&>()),
            std::declval<const C&>().upper_bound(
                std::declval<const typename C::key_type&>()),
            std::declval<const C&>().equal_range(
                std::declval<const typename C::key_type&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_unordered_assoc_ops
    //   trait: detects UnorderedAssociativeContainer members: hasher,
    // key_equal, hash_function(), key_eq(), bucket_count(),
    // load_factor(), max_load_factor().
    template<typename C,
             typename = void>
    struct has_unordered_assoc_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_unordered_assoc_ops<C,
        decltype(static_cast<void>(
            std::declval<typename C::hasher>(),
            std::declval<typename C::key_equal>(),
            std::declval<const C&>().hash_function(),
            std::declval<const C&>().key_eq(),
            std::declval<const C&>().bucket_count(),
            std::declval<const C&>().load_factor(),
            std::declval<const C&>().max_load_factor()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_container
//   trait: evaluates to true if C satisfies the structural
// requirements of the Container named requirement: has required
// nested types and member functions.
template<typename C>
struct is_container
{
    static constexpr bool value =
        ( internal::has_container_types<C>::value &&
          internal::has_container_ops<C>::value );
};

// is_reversible_container
//   trait: evaluates to true if C satisfies the
// ReversibleContainer named requirement: is a Container with
// reverse iteration support.
template<typename C>
struct is_reversible_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_reversible_ops<C>::value );
};

// is_allocator_aware_container
//   trait: evaluates to true if C satisfies the
// AllocatorAwareContainer named requirement: is a Container with
// allocator_type and get_allocator().
template<typename C>
struct is_allocator_aware_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_allocator_aware_ops<C>::value );
};

// is_sequence_container
//   trait: evaluates to true if C satisfies the structural
// requirements of the SequenceContainer named requirement: is a
// Container with front(), insert(), and erase().
// note: does not verify all SequenceContainer requirements (e.g.,
// emplace_front, push_back are optional for some sequences).
template<typename C>
struct is_sequence_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_sequence_ops<C>::value );
};

// is_associative_container
//   trait: evaluates to true if C satisfies the structural
// requirements of the AssociativeContainer named requirement: is a
// Container with key_type, key_compare, find(), count(), and
// range-lookup members.
template<typename C>
struct is_associative_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_associative_ops<C>::value );
};

// is_unordered_associative_container
//   trait: evaluates to true if C satisfies the structural
// requirements of the UnorderedAssociativeContainer named requirement:
// is a Container with hasher, key_equal, hash_function(),
// bucket_count(), and load factor members.
template<typename C>
struct is_unordered_associative_container
{
    static constexpr bool value =
        ( is_container<C>::value &&
          internal::has_unordered_assoc_ops<C>::value );
};


// =========================================================================
// VIII. CONTAINER ELEMENT TRAITS (C++11)
// =========================================================================
//   These traits test whether an element type T can be constructed or
// destroyed in allocator-managed storage via std::allocator_traits<A>.
// They correspond to the container element named requirements:
// DefaultInsertable, CopyInsertable, MoveInsertable,
// EmplaceConstructible, and Erasable.

NS_INTERNAL

    // can_alloc_construct
    //   trait: detects if allocator_traits<A>::construct(a, p, args...)
    // is well-formed.
    template<typename Void,
             typename T,
             typename A,
             typename... Args>
    struct can_alloc_construct
    {
        static constexpr bool value = false;
    };

    template<typename T,
             typename A,
             typename... Args>
    struct can_alloc_construct<
        decltype(static_cast<void>(
            std::allocator_traits<A>::construct(
                std::declval<A&>(),
                std::declval<T*>(),
                std::declval<Args>()...)
        )),
        T,
        A,
        Args...>
    {
        static constexpr bool value = true;
    };

    // can_alloc_destroy
    //   trait: detects if allocator_traits<A>::destroy(a, p) is
    // well-formed.
    template<typename T,
             typename A,
             typename = void>
    struct can_alloc_destroy
    {
        static constexpr bool value = false;
    };

    template<typename T,
             typename A>
    struct can_alloc_destroy<T, A,
        decltype(static_cast<void>(
            std::allocator_traits<A>::destroy(
                std::declval<A&>(),
                std::declval<T*>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_default_insertable
//   trait: evaluates to true if T is DefaultInsertable into a
// container using allocator A (i.e., allocator_traits<A>::construct
// (a, p) is well-formed).
// default: A = std::allocator<T>.
template<typename T,
         typename A = std::allocator<T>>
struct is_default_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<void, T, A>::value;
};

// is_copy_insertable
//   trait: evaluates to true if T is CopyInsertable into a container
// using allocator A (i.e., allocator_traits<A>::construct(a, p, v)
// is well-formed where v is a const lvalue reference).
// default: A = std::allocator<T>.
template<typename T,
         typename A = std::allocator<T>>
struct is_copy_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<
            void, T, A, const T&>::value;
};

// is_move_insertable
//   trait: evaluates to true if T is MoveInsertable into a container
// using allocator A (i.e., allocator_traits<A>::construct(a, p, rv)
// is well-formed where rv is an rvalue reference).
// default: A = std::allocator<T>.
#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

template<typename T,
         typename A = std::allocator<T>>
struct is_move_insertable
{
    static constexpr bool value =
        internal::can_alloc_construct<void, T, A, T&&>::value;
};

#endif  // D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

// is_emplace_constructible
//   trait: evaluates to true if T is EmplaceConstructible into a
// container using allocator A from Args... (i.e.,
// allocator_traits<A>::construct(a, p, args...) is well-formed).
// default: A = std::allocator<T>.
template<typename T,
         typename A,
         typename... Args>
struct is_emplace_constructible
{
    static constexpr bool value =
        internal::can_alloc_construct<
            void, T, A, Args...>::value;
};

// is_erasable
//   trait: evaluates to true if T is Erasable from a container using
// allocator A (i.e., allocator_traits<A>::destroy(a, p) is
// well-formed).
// default: A = std::allocator<T>.
template<typename T,
         typename A = std::allocator<T>>
struct is_erasable
{
    static constexpr bool value =
        internal::can_alloc_destroy<T, A>::value;
};


// =========================================================================
// IX.  ITERATOR TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_iterator_traits
    //   trait: detects if std::iterator_traits<I> provides the five
    // required nested types: value_type, difference_type, reference,
    // pointer, iterator_category.
    template<typename I,
             typename = void>
    struct has_iterator_traits
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_iterator_traits<I,
        decltype(static_cast<void>(
            std::declval<typename std::iterator_traits<I>::value_type>(),
            std::declval<typename std::iterator_traits<I>::difference_type>(),
            std::declval<typename std::iterator_traits<I>::reference>(),
            std::declval<typename std::iterator_traits<I>::pointer>(),
            std::declval<typename std::iterator_traits<I>::iterator_category>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_legacy_iterator_ops
    //   trait: detects LegacyIterator operations: *it and ++it.
    template<typename I,
             typename = void>
    struct has_legacy_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_legacy_iterator_ops<I,
        decltype(static_cast<void>(
            *std::declval<I&>(),
            ++std::declval<I&>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_input_iterator_ops
    //   trait: detects LegacyInputIterator operations: ==, !=, it->m,
    // *it (returning reference).
    template<typename I,
             typename = void>
    struct has_input_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_input_iterator_ops<I,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<const I&>() ==
                              std::declval<const I&>()),
            static_cast<bool>(std::declval<const I&>() !=
                              std::declval<const I&>()),
            std::declval<const I&>().operator->(),
            *std::declval<I&>()++
        ))>
    {
        static constexpr bool value = true;
    };

    // has_input_iterator_ops (pointer specialization fallback)
    //   trait: pointers satisfy LegacyInputIterator trivially.
    template<typename T>
    struct has_input_iterator_ops<T*,
        decltype(static_cast<void>(0))>
    {
        static constexpr bool value = true;
    };

    // has_output_iterator_ops
    //   trait: detects LegacyOutputIterator operations: *it = v,
    // ++it, it++.
    template<typename I,
             typename = void>
    struct has_output_iterator_ops
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_output_iterator_ops<I,
        decltype(static_cast<void>(
            *std::declval<I&>() =
                std::declval<
                    typename std::iterator_traits<I>::value_type>(),
            ++std::declval<I&>(),
            std::declval<I&>()++
        ))>
    {
        static constexpr bool value = true;
    };

    // has_bidirectional_ops
    //   trait: detects LegacyBidirectionalIterator operations:
    // --it, it--, *it--.
    template<typename I,
             typename = void>
    struct has_bidirectional_ops
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_bidirectional_ops<I,
        decltype(static_cast<void>(
            --std::declval<I&>(),
            std::declval<I&>()--,
            *std::declval<I&>()--
        ))>
    {
        static constexpr bool value = true;
    };

    // has_random_access_ops
    //   trait: detects LegacyRandomAccessIterator operations:
    // it+=n, it-=n, it+n, n+it, it-n, it-it, it[n], <, >, <=, >=.
    template<typename I,
             typename = void>
    struct has_random_access_ops
    {
        static constexpr bool value = false;
    };

    template<typename I>
    struct has_random_access_ops<I,
        decltype(static_cast<void>(
            std::declval<I&>() +=
                std::declval<
                    typename std::iterator_traits<I>::difference_type>(),
            std::declval<I&>() -=
                std::declval<
                    typename std::iterator_traits<I>::difference_type>(),
            std::declval<const I&>() +
                std::declval<
                    typename std::iterator_traits<I>::difference_type>(),
            std::declval<
                typename std::iterator_traits<I>::difference_type>() +
                std::declval<const I&>(),
            std::declval<const I&>() -
                std::declval<
                    typename std::iterator_traits<I>::difference_type>(),
            std::declval<const I&>() - std::declval<const I&>(),
            std::declval<const I&>()[
                std::declval<
                    typename std::iterator_traits<I>::difference_type>()],
            static_cast<bool>(std::declval<const I&>() <
                              std::declval<const I&>()),
            static_cast<bool>(std::declval<const I&>() >
                              std::declval<const I&>()),
            static_cast<bool>(std::declval<const I&>() <=
                              std::declval<const I&>()),
            static_cast<bool>(std::declval<const I&>() >=
                              std::declval<const I&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // is_iterator_category_at_least
    //   trait: detects if iterator_traits<I>::iterator_category is
    // derived from Cat (i.e., at least as strong as Cat).
    template<typename I,
             typename Cat,
             typename = void>
    struct is_iterator_category_at_least
    {
        static constexpr bool value = false;
    };

    template<typename I,
             typename Cat>
    struct is_iterator_category_at_least<I, Cat,
        typename std::enable_if<
            std::is_base_of<
                Cat,
                typename std::iterator_traits<I>::iterator_category
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_legacy_iterator
//   trait: evaluates to true if I satisfies the LegacyIterator named
// requirement: has iterator_traits nested types, is dereferenceable,
// is pre-incrementable, and is CopyConstructible/CopyAssignable/
// Destructible.
template<typename I>
struct is_legacy_iterator
{
    static constexpr bool value =
        ( internal::has_iterator_traits<I>::value     &&
          internal::has_legacy_iterator_ops<I>::value  &&
          std::is_copy_constructible<I>::value         &&
          std::is_copy_assignable<I>::value            &&
          std::is_destructible<I>::value );
};

// is_legacy_input_iterator
//   trait: evaluates to true if I satisfies the
// LegacyInputIterator named requirement: LegacyIterator with ==, !=,
// arrow operator, and post-increment dereference.
template<typename I>
struct is_legacy_input_iterator
{
    static constexpr bool value =
        ( is_legacy_iterator<I>::value &&
          internal::has_input_iterator_ops<I>::value );
};

// is_legacy_output_iterator
//   trait: evaluates to true if I satisfies the
// LegacyOutputIterator named requirement: *it = val is well-formed
// and incrementable.
template<typename I>
struct is_legacy_output_iterator
{
    static constexpr bool value =
        ( internal::has_iterator_traits<I>::value     &&
          internal::has_output_iterator_ops<I>::value );
};

// is_legacy_forward_iterator
//   trait: evaluates to true if I satisfies the
// LegacyForwardIterator named requirement: LegacyInputIterator with
// DefaultConstructible and at least forward_iterator_tag.
// note: multi-pass guarantee cannot be verified at compile time.
template<typename I>
struct is_legacy_forward_iterator
{
    static constexpr bool value =
        ( is_legacy_input_iterator<I>::value                   &&
          std::is_default_constructible<I>::value               &&
          internal::is_iterator_category_at_least<
              I, std::forward_iterator_tag>::value );
};

// is_legacy_bidirectional_iterator
//   trait: evaluates to true if I satisfies the
// LegacyBidirectionalIterator named requirement:
// LegacyForwardIterator with --, it--, and *it--.
template<typename I>
struct is_legacy_bidirectional_iterator
{
    static constexpr bool value =
        ( is_legacy_forward_iterator<I>::value       &&
          internal::has_bidirectional_ops<I>::value   &&
          internal::is_iterator_category_at_least<
              I, std::bidirectional_iterator_tag>::value );
};

// is_legacy_random_access_iterator
//   trait: evaluates to true if I satisfies the
// LegacyRandomAccessIterator named requirement:
// LegacyBidirectionalIterator with +=, -=, +, -, [], and all
// relational operators.
template<typename I>
struct is_legacy_random_access_iterator
{
    static constexpr bool value =
        ( is_legacy_bidirectional_iterator<I>::value   &&
          internal::has_random_access_ops<I>::value    &&
          internal::is_iterator_category_at_least<
              I, std::random_access_iterator_tag>::value );
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
    //   trait: detects if S supports unformatted input stream
    // operations: get(), read(buf, n), gcount().
    template<typename S,
             typename = void>
    struct has_unformatted_input_ops
    {
        static constexpr bool value = false;
    };

    template<typename S>
    struct has_unformatted_input_ops<S,
        decltype(static_cast<void>(
            std::declval<S&>().get(),
            std::declval<S&>().read(
                std::declval<typename S::char_type*>(),
                std::declval<std::streamsize>()),
            std::declval<S&>().gcount()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_formatted_input_ops
    //   trait: detects if S supports formatted input via operator>>
    // with an int (representative arithmetic type).
    template<typename S,
             typename = void>
    struct has_formatted_input_ops
    {
        static constexpr bool value = false;
    };

    template<typename S>
    struct has_formatted_input_ops<S,
        decltype(static_cast<void>(
            std::declval<S&>() >> std::declval<int&>()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_unformatted_output_ops
    //   trait: detects if S supports unformatted output stream
    // operations: put(ch), write(buf, n).
    template<typename S,
             typename = void>
    struct has_unformatted_output_ops
    {
        static constexpr bool value = false;
    };

    template<typename S>
    struct has_unformatted_output_ops<S,
        decltype(static_cast<void>(
            std::declval<S&>().put(
                std::declval<typename S::char_type>()),
            std::declval<S&>().write(
                std::declval<const typename S::char_type*>(),
                std::declval<std::streamsize>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_formatted_output_ops
    //   trait: detects if S supports formatted output via operator<<
    // with an int (representative arithmetic type).
    template<typename S,
             typename = void>
    struct has_formatted_output_ops
    {
        static constexpr bool value = false;
    };

    template<typename S>
    struct has_formatted_output_ops<S,
        decltype(static_cast<void>(
            std::declval<S&>() << std::declval<int>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_unformatted_input_stream
//   trait: evaluates to true if S supports unformatted input stream
// operations (get, read, gcount) as required by the
// UnformattedInputFunction named requirement.
template<typename S>
struct is_unformatted_input_stream
{
    static constexpr bool value =
        internal::has_unformatted_input_ops<S>::value;
};

// is_formatted_input_stream
//   trait: evaluates to true if S supports formatted input via
// operator>> as required by the FormattedInputFunction named
// requirement.
template<typename S>
struct is_formatted_input_stream
{
    static constexpr bool value =
        internal::has_formatted_input_ops<S>::value;
};

// is_unformatted_output_stream
//   trait: evaluates to true if S supports unformatted output
// stream operations (put, write) as required by the
// UnformattedOutputFunction named requirement.
template<typename S>
struct is_unformatted_output_stream
{
    static constexpr bool value =
        internal::has_unformatted_output_ops<S>::value;
};

// is_formatted_output_stream
//   trait: evaluates to true if S supports formatted output via
// operator<< as required by the FormattedOutputFunction named
// requirement.
template<typename S>
struct is_formatted_output_stream
{
    static constexpr bool value =
        internal::has_formatted_output_ops<S>::value;
};


// =========================================================================
// XI.  RANDOM NUMBER GENERATION TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_seed_sequence_ops
    //   trait: detects SeedSequence members: result_type, size(),
    // param(out), generate(first, last).
    template<typename S,
             typename = void>
    struct has_seed_sequence_ops
    {
        static constexpr bool value = false;
    };

    template<typename S>
    struct has_seed_sequence_ops<S,
        decltype(static_cast<void>(
            std::declval<typename S::result_type>(),
            std::declval<const S&>().size(),
            std::declval<const S&>().param(
                std::declval<typename S::result_type*>()),
            std::declval<S&>().generate(
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
    template<typename G,
             typename = void>
    struct has_urbg_ops
    {
        static constexpr bool value = false;
    };

    template<typename G>
    struct has_urbg_ops<G,
        typename std::enable_if<
            std::is_unsigned<typename G::result_type>::value,
            decltype(static_cast<void>(
                std::declval<G&>()(),
                G::min(),
                G::max()
            ))
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_engine_ops
    //   trait: detects RandomNumberEngine members beyond URBG:
    // seed(), seed(val), discard(n), ==, !=.
    template<typename E,
             typename = void>
    struct has_engine_ops
    {
        static constexpr bool value = false;
    };

    template<typename E>
    struct has_engine_ops<E,
        decltype(static_cast<void>(
            std::declval<E&>().seed(),
            std::declval<E&>().seed(
                std::declval<typename E::result_type>()),
            std::declval<E&>().discard(
                std::declval<unsigned long long>()),
            static_cast<bool>(std::declval<const E&>() ==
                              std::declval<const E&>()),
            static_cast<bool>(std::declval<const E&>() !=
                              std::declval<const E&>())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_engine_adaptor_ops
    //   trait: detects RandomNumberEngineAdaptor member: base().
    template<typename E,
             typename = void>
    struct has_engine_adaptor_ops
    {
        static constexpr bool value = false;
    };

    template<typename E>
    struct has_engine_adaptor_ops<E,
        decltype(static_cast<void>(
            std::declval<const E&>().base()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_distribution_ops
    //   trait: detects RandomNumberDistribution members: result_type,
    // param_type, operator()(gen), param(), min(), max(), reset().
    template<typename D,
             typename = void>
    struct has_distribution_ops
    {
        static constexpr bool value = false;
    };

    template<typename D>
    struct has_distribution_ops<D,
        decltype(static_cast<void>(
            std::declval<typename D::result_type>(),
            std::declval<typename D::param_type>(),
            std::declval<D&>()(std::declval<std::mt19937&>()),
            std::declval<const D&>().param(),
            std::declval<const D&>().min(),
            std::declval<const D&>().max(),
            std::declval<D&>().reset()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_seed_sequence
//   trait: evaluates to true if S satisfies the structural
// requirements of the SeedSequence named requirement: has
// result_type, size(), param(), and generate().
template<typename S>
struct is_seed_sequence
{
    static constexpr bool value =
        internal::has_seed_sequence_ops<S>::value;
};

// is_uniform_random_bit_generator
//   trait: evaluates to true if G satisfies the
// UniformRandomBitGenerator named requirement: has unsigned integral
// result_type, operator(), min(), and max().
template<typename G>
struct is_uniform_random_bit_generator
{
    static constexpr bool value =
        internal::has_urbg_ops<G>::value;
};

// is_random_number_engine
//   trait: evaluates to true if E satisfies the structural
// requirements of the RandomNumberEngine named requirement: is a
// UniformRandomBitGenerator with seed(), seed(val), discard(n), and
// equality comparison.
template<typename E>
struct is_random_number_engine
{
    static constexpr bool value =
        ( is_uniform_random_bit_generator<E>::value &&
          internal::has_engine_ops<E>::value );
};

// is_random_number_engine_adaptor
//   trait: evaluates to true if E satisfies the structural
// requirements of the RandomNumberEngineAdaptor named requirement:
// is a RandomNumberEngine with base() returning the adapted engine.
template<typename E>
struct is_random_number_engine_adaptor
{
    static constexpr bool value =
        ( is_random_number_engine<E>::value &&
          internal::has_engine_adaptor_ops<E>::value );
};

// is_random_number_distribution
//   trait: evaluates to true if D satisfies the structural
// requirements of the RandomNumberDistribution named requirement:
// has result_type, param_type, operator()(gen), param(), min(),
// max(), and reset().
template<typename D>
struct is_random_number_distribution
{
    static constexpr bool value =
        internal::has_distribution_ops<D>::value;
};


// =========================================================================
// XII. CONCURRENCY TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_basic_lockable_ops
    //   trait: detects BasicLockable members: lock(), unlock().
    template<typename M,
             typename = void>
    struct has_basic_lockable_ops
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_basic_lockable_ops<M,
        decltype(static_cast<void>(
            std::declval<M&>().lock(),
            std::declval<M&>().unlock()
        ))>
    {
        static constexpr bool value = true;
    };

    // has_try_lock
    //   trait: detects Lockable member: try_lock() returning bool.
    template<typename M,
             typename = void>
    struct has_try_lock
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_try_lock<M,
        decltype(static_cast<void>(
            static_cast<bool>(std::declval<M&>().try_lock())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_timed_lock_ops
    //   trait: detects TimedLockable members: try_lock_for(dur),
    // try_lock_until(tp) returning bool.
    template<typename M,
             typename = void>
    struct has_timed_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_timed_lock_ops<M,
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<M&>().try_lock_for(
                    std::declval<std::chrono::milliseconds>())),
            static_cast<bool>(
                std::declval<M&>().try_lock_until(
                    std::declval<
                        std::chrono::steady_clock::time_point>()))
        ))>
    {
        static constexpr bool value = true;
    };

    // has_shared_lock_ops
    //   trait: detects SharedLockable members: lock_shared(),
    // unlock_shared(), try_lock_shared() returning bool.
    template<typename M,
             typename = void>
    struct has_shared_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_shared_lock_ops<M,
        decltype(static_cast<void>(
            std::declval<M&>().lock_shared(),
            std::declval<M&>().unlock_shared(),
            static_cast<bool>(
                std::declval<M&>().try_lock_shared())
        ))>
    {
        static constexpr bool value = true;
    };

    // has_shared_timed_lock_ops
    //   trait: detects SharedTimedLockable members:
    // try_lock_shared_for(dur), try_lock_shared_until(tp).
    template<typename M,
             typename = void>
    struct has_shared_timed_lock_ops
    {
        static constexpr bool value = false;
    };

    template<typename M>
    struct has_shared_timed_lock_ops<M,
        decltype(static_cast<void>(
            static_cast<bool>(
                std::declval<M&>().try_lock_shared_for(
                    std::declval<std::chrono::milliseconds>())),
            static_cast<bool>(
                std::declval<M&>().try_lock_shared_until(
                    std::declval<
                        std::chrono::steady_clock::time_point>()))
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_basic_lockable
//   trait: evaluates to true if M satisfies the BasicLockable named
// requirement: has lock() and unlock().
template<typename M>
struct is_basic_lockable
{
    static constexpr bool value =
        internal::has_basic_lockable_ops<M>::value;
};

// is_lockable
//   trait: evaluates to true if M satisfies the Lockable named
// requirement: BasicLockable with try_lock() returning bool.
template<typename M>
struct is_lockable
{
    static constexpr bool value =
        ( is_basic_lockable<M>::value &&
          internal::has_try_lock<M>::value );
};

// is_timed_lockable
//   trait: evaluates to true if M satisfies the TimedLockable named
// requirement: Lockable with try_lock_for() and try_lock_until().
template<typename M>
struct is_timed_lockable
{
    static constexpr bool value =
        ( is_lockable<M>::value &&
          internal::has_timed_lock_ops<M>::value );
};

// is_shared_lockable
//   trait: evaluates to true if M satisfies the SharedLockable
// named requirement: lock_shared(), unlock_shared(),
// try_lock_shared().
template<typename M>
struct is_shared_lockable
{
    static constexpr bool value =
        internal::has_shared_lock_ops<M>::value;
};

// is_shared_timed_lockable
//   trait: evaluates to true if M satisfies the
// SharedTimedLockable named requirement: SharedLockable with
// try_lock_shared_for() and try_lock_shared_until().
template<typename M>
struct is_shared_timed_lockable
{
    static constexpr bool value =
        ( is_shared_lockable<M>::value &&
          internal::has_shared_timed_lock_ops<M>::value );
};

// is_mutex
//   trait: evaluates to true if M satisfies the Mutex named
// requirement: Lockable, DefaultConstructible, Destructible, and
// not copyable or movable.
template<typename M>
struct is_mutex
{
    static constexpr bool value =
        ( is_lockable<M>::value                       &&
          std::is_default_constructible<M>::value      &&
          std::is_destructible<M>::value               &&
          !std::is_copy_constructible<M>::value        &&
          !std::is_move_constructible<M>::value        &&
          !std::is_copy_assignable<M>::value           &&
          !std::is_move_assignable<M>::value );
};

// is_timed_mutex
//   trait: evaluates to true if M satisfies the TimedMutex named
// requirement: Mutex with TimedLockable.
template<typename M>
struct is_timed_mutex
{
    static constexpr bool value =
        ( is_mutex<M>::value &&
          is_timed_lockable<M>::value );
};

// is_shared_mutex
//   trait: evaluates to true if M satisfies the SharedMutex named
// requirement: Mutex with SharedLockable.
// note: std::shared_mutex is C++17. This trait works for any type
// providing the correct interface.
template<typename M>
struct is_shared_mutex
{
    static constexpr bool value =
        ( is_mutex<M>::value &&
          is_shared_lockable<M>::value );
};

// is_shared_timed_mutex
//   trait: evaluates to true if M satisfies the SharedTimedMutex
// named requirement: Mutex with TimedLockable and
// SharedTimedLockable.
template<typename M>
struct is_shared_timed_mutex
{
    static constexpr bool value =
        ( is_mutex<M>::value              &&
          is_timed_lockable<M>::value     &&
          is_shared_timed_lockable<M>::value );
};


// =========================================================================
// XIII. TYPE TRAIT META-TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_value_and_type
    //   trait: detects if T has a nested ::value (convertible to
    // bool) and ::type nested typedef. Used for UnaryTypeTrait and
    // BinaryTypeTrait detection.
    template<typename T,
             typename = void>
    struct has_value_and_type
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct has_value_and_type<T,
        decltype(static_cast<void>(
            static_cast<bool>(T::value),
            std::declval<typename T::type>()
        ))>
    {
        static constexpr bool value = true;
    };

    // derives_integral_constant
    //   trait: detects if T derives from std::integral_constant.
    template<typename T,
             typename = void>
    struct derives_integral_constant
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct derives_integral_constant<T,
        typename std::enable_if<
            std::is_base_of<
                std::integral_constant<bool, true>,
                T
            >::value ||
            std::is_base_of<
                std::integral_constant<bool, false>,
                T
            >::value
        >::type>
    {
        static constexpr bool value = true;
    };

    // has_nested_type
    //   trait: detects if T has a nested ::type typedef.
    template<typename T,
             typename = void>
    struct has_nested_type
    {
        static constexpr bool value = false;
    };

    template<typename T>
    struct has_nested_type<T,
        decltype(static_cast<void>(
            std::declval<typename T::type>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_unary_type_trait
//   trait: evaluates to true if T satisfies the UnaryTypeTrait
// named requirement: has a nested ::value convertible to bool, a
// nested ::type, and derives from std::integral_constant.
// note: cannot verify the template-arity requirement (that T takes
// exactly one type parameter) at this level.
template<typename T>
struct is_unary_type_trait
{
    static constexpr bool value =
        ( internal::has_value_and_type<T>::value &&
          internal::derives_integral_constant<T>::value );
};

// is_binary_type_trait
//   trait: evaluates to true if T satisfies the BinaryTypeTrait
// named requirement: has ::value, ::type, and derives from
// integral_constant.
// note: structurally identical to UnaryTypeTrait. The distinction
// is template arity (two parameters), which cannot be verified
// without instantiation. Provided as a semantic alias.
template<typename T>
struct is_binary_type_trait
{
    static constexpr bool value =
        ( internal::has_value_and_type<T>::value &&
          internal::derives_integral_constant<T>::value );
};

// is_transformation_trait
//   trait: evaluates to true if T satisfies the
// TransformationTrait named requirement: has a nested ::type typedef.
template<typename T>
struct is_transformation_trait
{
    static constexpr bool value =
        internal::has_nested_type<T>::value;
};


// =========================================================================
// XIV. CLOCK AND TRIVIALCLOCK TRAITS (C++11)
// =========================================================================

NS_INTERNAL

    // has_clock_ops
    //   trait: detects Clock members: rep, period, duration,
    // time_point, is_steady, now().
    template<typename C,
             typename = void>
    struct has_clock_ops
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct has_clock_ops<C,
        decltype(static_cast<void>(
            std::declval<typename C::rep>(),
            std::declval<typename C::period>(),
            std::declval<typename C::duration>(),
            std::declval<typename C::time_point>(),
            static_cast<bool>(C::is_steady),
            C::now()
        ))>
    {
        static constexpr bool value = true;
    };

    // is_now_noexcept
    //   trait: detects if C::now() is noexcept.
    template<typename C,
             typename = void>
    struct is_now_noexcept
    {
        static constexpr bool value = false;
    };

    template<typename C>
    struct is_now_noexcept<C,
        typename std::enable_if<
            noexcept(C::now())
        >::type>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_clock
//   trait: evaluates to true if C satisfies the Clock named
// requirement: has rep, period, duration, time_point nested types,
// an is_steady static member, and a now() static function.
template<typename C>
struct is_clock
{
    static constexpr bool value =
        internal::has_clock_ops<C>::value;
};

// is_trivial_clock
//   trait: evaluates to true if C satisfies the TrivialClock named
// requirement: is a Clock whose now() is noexcept, and whose types
// satisfy Clock consistency requirements.
// note: some TrivialClock semantic requirements (monotonicity,
// representation fidelity) cannot be verified at compile time.
template<typename C>
struct is_trivial_clock
{
    static constexpr bool value =
        ( is_clock<C>::value &&
          internal::is_now_noexcept<C>::value );
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
    template<typename Tr,
             typename = void>
    struct has_char_traits_ops
    {
        static constexpr bool value = false;
    };

    template<typename Tr>
    struct has_char_traits_ops<Tr,
        decltype(static_cast<void>(
            std::declval<typename Tr::char_type>(),
            std::declval<typename Tr::int_type>(),
            std::declval<typename Tr::off_type>(),
            std::declval<typename Tr::pos_type>(),
            std::declval<typename Tr::state_type>(),
            static_cast<bool>(
                Tr::eq(std::declval<typename Tr::char_type>(),
                        std::declval<typename Tr::char_type>())),
            static_cast<bool>(
                Tr::lt(std::declval<typename Tr::char_type>(),
                        std::declval<typename Tr::char_type>())),
            Tr::length(
                std::declval<const typename Tr::char_type*>()),
            Tr::to_char_type(
                std::declval<typename Tr::int_type>()),
            Tr::to_int_type(
                std::declval<typename Tr::char_type>()),
            Tr::eof(),
            Tr::not_eof(
                std::declval<typename Tr::int_type>())
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_char_traits
//   trait: evaluates to true if Tr satisfies the structural
// requirements of the CharTraits named requirement: has the required
// nested types and static member functions.
template<typename Tr>
struct is_char_traits
{
    static constexpr bool value =
        internal::has_char_traits_ops<Tr>::value;
};


// =========================================================================
// XVI. BITMASKTYPE TRAIT (C++11)
// =========================================================================

NS_INTERNAL

    // has_bitmask_ops
    //   trait: detects BitmaskType operations: &, |, ^, ~, &=, |=,
    // ^=.
    template<typename B,
             typename = void>
    struct has_bitmask_ops
    {
        static constexpr bool value = false;
    };

    template<typename B>
    struct has_bitmask_ops<B,
        decltype(static_cast<void>(
            std::declval<B>() & std::declval<B>(),
            std::declval<B>() | std::declval<B>(),
            std::declval<B>() ^ std::declval<B>(),
            ~std::declval<B>(),
            std::declval<B&>() &= std::declval<B>(),
            std::declval<B&>() |= std::declval<B>(),
            std::declval<B&>() ^= std::declval<B>()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_bitmask_type
//   trait: evaluates to true if B satisfies the BitmaskType named
// requirement: supports &, |, ^, ~, &=, |=, ^= operators.
template<typename B>
struct is_bitmask_type
{
    static constexpr bool value =
        internal::has_bitmask_ops<B>::value;
};


// =========================================================================
// XVII. NUMERICTYPE TRAIT (C++11)
// =========================================================================

// is_numeric_type
//   trait: evaluates to true if T satisfies the NumericType named
// requirement: an arithmetic type, or a cv-qualified version
// thereof. For user-defined numeric types, use in conjunction with
// std::numeric_limits<T>::is_specialized.
// note: the standard defines NumericType as the set of cv-unqualified
// arithmetic types. This trait accepts cv-qualified types as well
// for practical use.
template<typename T>
struct is_numeric_type
{
    static constexpr bool value =
        std::is_arithmetic<
            typename std::remove_cv<T>::type>::value;
};


// =========================================================================
// XVIII. REGEXTRAITS TRAIT (C++11)
// =========================================================================

NS_INTERNAL

    // has_regex_traits_ops
    //   trait: detects RegexTraits members: char_type, string_type,
    // locale_type, char_class_type, length(), translate(),
    // translate_nocase(), value(), imbue(), getloc().
    template<typename Tr,
             typename = void>
    struct has_regex_traits_ops
    {
        static constexpr bool value = false;
    };

    template<typename Tr>
    struct has_regex_traits_ops<Tr,
        decltype(static_cast<void>(
            std::declval<typename Tr::char_type>(),
            std::declval<typename Tr::string_type>(),
            std::declval<typename Tr::locale_type>(),
            std::declval<typename Tr::char_class_type>(),
            Tr::length(
                std::declval<const typename Tr::char_type*>()),
            std::declval<const Tr&>().translate(
                std::declval<typename Tr::char_type>()),
            std::declval<const Tr&>().translate_nocase(
                std::declval<typename Tr::char_type>()),
            std::declval<const Tr&>().value(
                std::declval<typename Tr::char_type>(), int(10)),
            std::declval<Tr&>().imbue(
                std::declval<typename Tr::locale_type>()),
            std::declval<const Tr&>().getloc()
        ))>
    {
        static constexpr bool value = true;
    };

NS_END  // internal

// is_regex_traits
//   trait: evaluates to true if Tr satisfies the structural
// requirements of the RegexTraits named requirement: has the
// required nested types and static/member functions.
template<typename Tr>
struct is_regex_traits
{
    static constexpr bool value =
        internal::has_regex_traits_ops<Tr>::value;
};


// =========================================================================
// XIX. LITERALTYPE TRAIT (C++11)
// =========================================================================

// is_literal_type_v_
//   trait: evaluates to true if T satisfies the LiteralType named
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
template<typename T>
struct is_literal_type_
{
    static constexpr bool value =
        ( std::is_scalar<T>::value        ||
          std::is_reference<T>::value     ||
          ( std::is_class<T>::value                    &&
            std::is_trivially_destructible<T>::value   &&
            std::is_default_constructible<T>::value )  ||
          ( std::is_array<T>::value                    &&
            std::is_trivially_destructible<
                typename std::remove_all_extents<T>::type>::value ) );
};


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED11_HPP
