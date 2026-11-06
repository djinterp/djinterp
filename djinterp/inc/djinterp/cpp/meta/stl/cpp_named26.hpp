/*******************************************************************************
* djinterp [cpp]                                                 cpp_named26.hpp
*
* C++26 named requirement type traits:
*   Extends the C++23 named requirement traits with C++26 additions:
*   - Trivial relocatability detection (P1144/P2786)
*   - Contract-annotated constraint wrappers
*   - Relocatable compound concepts
*
* ADDITIONS OVER C++23:
*   - is_trivially_relocatable<T>                     (trait)
*   - is_trivially_relocatable_v<T>                   (value)
*   - concepts::trivially_relocatable                  (concept)
*   - concepts::relocatable                            (concept)
*   - concepts::nothrow_relocatable                    (concept)
*   - concepts::trivial_value_type                     (concept)
*   - concepts::memcpy_safe                            (concept)
*   - concepts::c_compatible                           (concept)
*   - concepts::relocatable_value                      (concept)
*   - concepts::implicit_lifetime_and_relocatable      (concept)
*   - concepts::sortable_range                         (concept, ranges)
*   - concepts::contiguous_value_range                 (concept, ranges)
*   - concepts::relocatable_range                      (concept, ranges)
*   - Deprecation notes for TrivialType and PODType
*
* NOTE ON TRIVIAL RELOCATABILITY:
*   A type is trivially relocatable if moving it and destroying the source
*   can be replaced by a simple memcpy. This is critical for container
*   optimizations (realloc, vector growth). C++26 formalizes this with
*   std::is_trivially_relocatable.
*
* FEATURE DEPENDENCIES:
*   D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY  - trivial relocation
*   D_ENV_CPP_FEATURE_LANG_CONCEPTS                - concept availability
*   D_ENV_CPP_FEATURE_LANG_CONTRACTS               - contract annotations
*
*
* path:      /inc/djinterp/cpp/meta/stl/cpp_named26.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.02.27
*                                                            revised: 2026.11.04
*******************************************************************************/

#ifndef DJINTERP_META_STL_CPP_NAMED26_HPP
#define DJINTERP_META_STL_CPP_NAMED26_HPP 1

// djinterp
#include "../../djinterp.hpp"  // framework root


// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named26.hpp can only be used in C++ compilation mode"
#endif

// include C++23 layer
#include "cpp_named23.hpp"

// C++26 additions are active for C++26 and all later standards
#if D_ENV_LANG_IS_CPP26_OR_HIGHER

// std
#include <type_traits>


NS_DJINTERP
D_NAMESPACE(stl)

// =========================================================================
// I.   TRIVIAL RELOCATABILITY TRAITS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY

// is_trivially_relocatable
//   trait: evaluates to true if T is trivially relocatable (i.e., a
// move-construct followed by source destruction can be replaced by
// memcpy).
template<typename T>
struct is_trivially_relocatable
{
    static constexpr bool value =
        std::is_trivially_relocatable_v<T>;
};

// is_trivially_relocatable_v
//   value: shorthand for is_trivially_relocatable<T>::value.
template<typename T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<T>::value;

#else  // !D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY

// is_trivially_relocatable (fallback)
//   trait: conservative fallback when compiler does not yet support
// trivial relocatability detection. Approximates by checking both
// trivially move-constructible and trivially destructible.
template<typename T>
struct is_trivially_relocatable
{
    static constexpr bool value =
        ( std::is_trivially_move_constructible_v<T> &&
          std::is_trivially_destructible_v<T> );
};

// is_trivially_relocatable_v
//   value: shorthand for is_trivially_relocatable<T>::value.
template<typename T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY


// =========================================================================
// II.  RELOCATABILITY AND COMPOUND CONCEPTS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // trivially_relocatable
    //   concept: constrains T to types that are trivially relocatable.
    template<typename T>
    concept trivially_relocatable =
        is_trivially_relocatable_v<T>;

    // relocatable
    //   concept: constrains T to types that can be relocated (moved
    // and destroyed), regardless of triviality.
    template<typename T>
    concept relocatable =
        ( move_constructible<T> &&
          destructible<T> );

    // nothrow_relocatable
    //   concept: constrains T to types that can be relocated without
    // throwing exceptions.
    template<typename T>
    concept nothrow_relocatable =
        ( nothrow_move_constructible<T> &&
          nothrow_destructible<T> );

    // trivial_value_type
    //   concept: constrains T to types satisfying all six core named
    // requirements trivially (no user-defined special member functions
    // are involved in any operation).
    template<typename T>
    concept trivial_value_type =
        ( trivially_default_constructible<T> &&
          trivially_copy_constructible<T>    &&
          trivially_move_constructible<T>    &&
          trivially_copy_assignable<T>       &&
          trivially_move_assignable<T>       &&
          trivially_destructible<T> );


    // =====================================================================
    // IV.  COMPOUND VALUE-CATEGORY CONCEPTS (C++26)
    // =====================================================================

    // hashable_key
    //   concept: constrains T to types suitable as unordered
    // container keys (EqualityComparable + hashable).
    template<typename T>
    concept hashable_key =
        ( equality_comparable<T> &&
          hashable<T> );

    // ordered_key
    //   concept: constrains T to types suitable as ordered container
    // keys (LessThanComparable + EqualityComparable).
    template<typename T>
    concept ordered_key =
        ( less_than_comparable<T> &&
          equality_comparable<T> );

    // sortable_element
    //   concept: constrains T to types that can participate in sort
    // operations (MoveConstructible + MoveAssignable + swappable +
    // LessThanComparable).
    template<typename T>
    concept sortable_element =
        ( move_constructible<T>    &&
          move_assignable<T>       &&
          swappable<T>             &&
          less_than_comparable<T> );

    // regular_value
    //   concept: constrains T to regular types (value_type +
    // EqualityComparable + swappable). Corresponds closely to the
    // std::regular concept.
    template<typename T>
    concept regular_value =
        ( value_type<T>           &&
          equality_comparable<T>  &&
          swappable<T> );

    // container_element
    //   concept: constrains T to types well-suited for standard
    // container storage (value_type + Destructible + swappable).
    template<typename T>
    concept container_element =
        ( value_type<T> &&
          swappable<T> );


    // =====================================================================
    // V.   COMPOUND CONTAINER CONCEPTS (C++26)
    // =====================================================================

    // contiguous_sequence
    //   concept: constrains C to types satisfying both
    // SequenceContainer and ContiguousContainer (e.g., std::vector,
    // std::array, std::string).
    template<typename C>
    concept contiguous_sequence =
        ( sequence_container<C> &&
          contiguous_container<C> );

    // ordered_associative
    //   concept: constrains C to types satisfying both
    // AssociativeContainer and ReversibleContainer (e.g., std::map,
    // std::set).
    template<typename C>
    concept ordered_associative =
        ( associative_container<C> &&
          reversible_container<C> );

    // allocator_aware_sequence
    //   concept: constrains C to types satisfying both
    // SequenceContainer and AllocatorAwareContainer.
    template<typename C>
    concept allocator_aware_sequence =
        ( sequence_container<C> &&
          allocator_aware_container<C> );

    // relocatable_container_element
    //   concept: constrains T to types suitable for optimized
    // container storage: a container_element that is also trivially
    // relocatable (safe for realloc-style moves).
    template<typename T>
    concept relocatable_container_element =
        ( container_element<T> &&
          trivially_relocatable<T> );


    // =====================================================================
    // VI.  COMPOUND RANGE CONCEPTS (C++26)
    // =====================================================================

#if D_ENV_CPP_FEATURE_STL_RANGES

    // sortable_range
    //   concept: constrains R to forward ranges whose elements are
    // sortable (MoveConstructible + MoveAssignable + swappable +
    // LessThanComparable).
    template<typename R>
    concept sortable_range =
        ( forward_range<R> &&
          sortable_element<std::ranges::range_value_t<R>> );

    // contiguous_value_range
    //   concept: constrains R to contiguous ranges whose elements
    // are full value types.
    template<typename R>
    concept contiguous_value_range =
        ( contiguous_range<R> &&
          value_type<std::ranges::range_value_t<R>> );

    // relocatable_range
    //   concept: constrains R to contiguous ranges whose elements
    // are trivially relocatable — ideal for bulk-move optimizations.
    template<typename R>
    concept relocatable_range =
        ( contiguous_range<R> &&
          trivially_relocatable<std::ranges::range_value_t<R>> );

#endif  // D_ENV_CPP_FEATURE_STL_RANGES

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// III. STATIC ASSERTION DIAGNOSTIC HELPERS (C++26)
// =========================================================================

// D_STATIC_ASSERT_TRIVIALLY_RELOCATABLE
//   macro: static_assert that T is trivially relocatable with a
// descriptive error message.
#define D_STATIC_ASSERT_TRIVIALLY_RELOCATABLE(T)                          \
    static_assert(                                                         \
        is_trivially_relocatable_v<T>,                                    \
        "Type " #T " is not trivially relocatable")

// D_STATIC_ASSERT_TRIVIAL_VALUE_TYPE
//   macro: static_assert that T is a trivial value type with
// individual error messages for each failed requirement.
#define D_STATIC_ASSERT_TRIVIAL_VALUE_TYPE(T)                             \
    static_assert(                                                         \
        std::is_trivially_default_constructible_v<T>,                     \
        "Type " #T " is not trivially default-constructible");            \
    static_assert(                                                         \
        std::is_trivially_copy_constructible_v<T>,                        \
        "Type " #T " is not trivially copy-constructible");               \
    static_assert(                                                         \
        std::is_trivially_move_constructible_v<T>,                        \
        "Type " #T " is not trivially move-constructible");               \
    static_assert(                                                         \
        std::is_trivially_copy_assignable_v<T>,                           \
        "Type " #T " is not trivially copy-assignable");                  \
    static_assert(                                                         \
        std::is_trivially_move_assignable_v<T>,                           \
        "Type " #T " is not trivially move-assignable");                  \
    static_assert(                                                         \
        std::is_trivially_destructible_v<T>,                              \
        "Type " #T " is not trivially destructible")


// =========================================================================
// IV.  TYPE CLASSIFICATION COMPOUND CONCEPTS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // memcpy_safe
    //   concept: constrains T to types that can be safely copied via
    // memcpy/memmove. Equivalent to TriviallyCopyable.
    // This is a semantic alias providing intent-clarity for low-level
    // code that performs raw byte copying.
    template<typename T>
    concept memcpy_safe =
        trivially_copyable<T>;

    // c_compatible
    //   concept: constrains T to types that are safe to pass across
    // C/C++ language boundaries (standard-layout + trivially copyable).
    template<typename T>
    concept c_compatible =
        ( standard_layout_type<T> &&
          trivially_copyable<T> );

    // relocatable_value
    //   concept: constrains T to types that are both a full value type
    // (all six named requirements) and trivially relocatable.
    template<typename T>
    concept relocatable_value =
        ( value_type<T> &&
          trivially_relocatable<T> );

    // implicit_lifetime_and_relocatable
    //   concept: constrains T to types that are both
    // implicit-lifetime and trivially relocatable. Such types are
    // ideal candidates for container optimizations (placement new,
    // realloc, uninitialized byte buffers).
    template<typename T>
    concept implicit_lifetime_and_relocatable =
        ( implicit_lifetime_type<T> &&
          trivially_relocatable<T> );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// V.   DEPRECATION NOTES (C++26)
// =========================================================================

// NOTE: TrivialType deprecation (C++26)
//   The TrivialType named requirement is deprecated as of C++26.
//   djinterp::re_std::is_trivial<T> remains available for backward
//   compatibility. The standard recommends checking the specific
//   properties needed (e.g., is_trivially_copyable,
//   is_trivially_default_constructible) rather than is_trivial.
//
// NOTE: PODType removal (C++26)
//   std::is_pod was removed from the standard library in C++26.
//   djinterp::re_std::is_pod<T> remains available and falls back to
//   (is_trivial && is_standard_layout) when std::is_pod is absent.
//   Prefer is_trivially_copyable + is_standard_layout for new code.


NS_END  // stl
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP26_OR_HIGHER


#endif  // DJINTERP_META_STL_CPP_NAMED26_HPP
