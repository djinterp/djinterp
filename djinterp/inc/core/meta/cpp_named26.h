/******************************************************************************
* djinterp [core]                                              cpp_named26.h
*
* C++26 named requirement type traits:
*   Extends the C++23 named requirement traits with C++26 additions:
*   - Trivial relocatability detection (P1144/P2786)
*   - Contract-annotated constraint wrappers
*   - Relocatable compound concepts
*
* ADDITIONS OVER C++23:
*   - is_trivially_relocatable<_T>                     (trait)
*   - is_trivially_relocatable_v<_T>                   (value)
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
* path:      \inc\cpp_named26.h
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.02.27
******************************************************************************/

#ifndef DJINTERP_CPP_NAMED26_
#define DJINTERP_CPP_NAMED26_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "cpp_named26.h requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "cpp_named26.h can only be used in C++ compilation mode"
#endif

// include C++23 layer
#include "cpp_named23.h"

// C++26 standard value (202600L is provisional; update when finalized)
#ifndef D_ENV_LANG_CPP_STANDARD_CPP26
    #define D_ENV_LANG_CPP_STANDARD_CPP26 202600L
#endif

// provide C++26 detection if env.h does not yet define it
#ifndef D_ENV_LANG_IS_CPP26_OR_HIGHER
    #ifdef D_ENV_LANG_CPP_STANDARD
        #define D_ENV_LANG_IS_CPP26_OR_HIGHER  \
            (D_ENV_LANG_CPP_STANDARD >= D_ENV_LANG_CPP_STANDARD_CPP26)
    #else
        #define D_ENV_LANG_IS_CPP26_OR_HIGHER 0
    #endif
#endif

// C++26 additions are active for C++26 and all later standards
#if D_ENV_LANG_IS_CPP26_OR_HIGHER

#include <type_traits>


NS_DJINTERP
NS_TRAITS

// =========================================================================
// I.   TRIVIAL RELOCATABILITY TRAITS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY

// is_trivially_relocatable
//   trait: evaluates to true if _T is trivially relocatable (i.e., a
// move-construct followed by source destruction can be replaced by
// memcpy).
template<typename _T>
struct is_trivially_relocatable
{
    static constexpr bool value =
        std::is_trivially_relocatable_v<_T>;
};

// is_trivially_relocatable_v
//   value: shorthand for is_trivially_relocatable<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<_T>::value;

#else  // !D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY

// is_trivially_relocatable (fallback)
//   trait: conservative fallback when compiler does not yet support
// trivial relocatability detection. Approximates by checking both
// trivially move-constructible and trivially destructible.
template<typename _T>
struct is_trivially_relocatable
{
    static constexpr bool value =
        ( std::is_trivially_move_constructible_v<_T> &&
          std::is_trivially_destructible_v<_T> );
};

// is_trivially_relocatable_v
//   value: shorthand for is_trivially_relocatable<_T>::value.
template<typename _T>
inline constexpr bool is_trivially_relocatable_v =
    is_trivially_relocatable<_T>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_TRIVIAL_RELOCATABILITY


// =========================================================================
// II.  RELOCATABILITY AND COMPOUND CONCEPTS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // trivially_relocatable
    //   concept: constrains _T to types that are trivially relocatable.
    template<typename _T>
    concept trivially_relocatable =
        is_trivially_relocatable_v<_T>;

    // relocatable
    //   concept: constrains _T to types that can be relocated (moved
    // and destroyed), regardless of triviality.
    template<typename _T>
    concept relocatable =
        ( move_constructible<_T> &&
          destructible<_T> );

    // nothrow_relocatable
    //   concept: constrains _T to types that can be relocated without
    // throwing exceptions.
    template<typename _T>
    concept nothrow_relocatable =
        ( nothrow_move_constructible<_T> &&
          nothrow_destructible<_T> );

    // trivial_value_type
    //   concept: constrains _T to types satisfying all six core named
    // requirements trivially (no user-defined special member functions
    // are involved in any operation).
    template<typename _T>
    concept trivial_value_type =
        ( trivially_default_constructible<_T> &&
          trivially_copy_constructible<_T>    &&
          trivially_move_constructible<_T>    &&
          trivially_copy_assignable<_T>       &&
          trivially_move_assignable<_T>       &&
          trivially_destructible<_T> );


    // =====================================================================
    // IV.  COMPOUND VALUE-CATEGORY CONCEPTS (C++26)
    // =====================================================================

    // hashable_key
    //   concept: constrains _T to types suitable as unordered
    // container keys (EqualityComparable + hashable).
    template<typename _T>
    concept hashable_key =
        ( equality_comparable<_T> &&
          hashable<_T> );

    // ordered_key
    //   concept: constrains _T to types suitable as ordered container
    // keys (LessThanComparable + EqualityComparable).
    template<typename _T>
    concept ordered_key =
        ( less_than_comparable<_T> &&
          equality_comparable<_T> );

    // sortable_element
    //   concept: constrains _T to types that can participate in sort
    // operations (MoveConstructible + MoveAssignable + swappable +
    // LessThanComparable).
    template<typename _T>
    concept sortable_element =
        ( move_constructible<_T>    &&
          move_assignable<_T>       &&
          swappable<_T>             &&
          less_than_comparable<_T> );

    // regular_value
    //   concept: constrains _T to regular types (value_type +
    // EqualityComparable + swappable). Corresponds closely to the
    // std::regular concept.
    template<typename _T>
    concept regular_value =
        ( value_type<_T>           &&
          equality_comparable<_T>  &&
          swappable<_T> );

    // container_element
    //   concept: constrains _T to types well-suited for standard
    // container storage (value_type + Destructible + swappable).
    template<typename _T>
    concept container_element =
        ( value_type<_T> &&
          swappable<_T> );


    // =====================================================================
    // V.   COMPOUND CONTAINER CONCEPTS (C++26)
    // =====================================================================

    // contiguous_sequence
    //   concept: constrains _C to types satisfying both
    // SequenceContainer and ContiguousContainer (e.g., std::vector,
    // std::array, std::string).
    template<typename _C>
    concept contiguous_sequence =
        ( sequence_container<_C> &&
          contiguous_container<_C> );

    // ordered_associative
    //   concept: constrains _C to types satisfying both
    // AssociativeContainer and ReversibleContainer (e.g., std::map,
    // std::set).
    template<typename _C>
    concept ordered_associative =
        ( associative_container<_C> &&
          reversible_container<_C> );

    // allocator_aware_sequence
    //   concept: constrains _C to types satisfying both
    // SequenceContainer and AllocatorAwareContainer.
    template<typename _C>
    concept allocator_aware_sequence =
        ( sequence_container<_C> &&
          allocator_aware_container<_C> );

    // relocatable_container_element
    //   concept: constrains _T to types suitable for optimized
    // container storage: a container_element that is also trivially
    // relocatable (safe for realloc-style moves).
    template<typename _T>
    concept relocatable_container_element =
        ( container_element<_T> &&
          trivially_relocatable<_T> );


    // =====================================================================
    // VI.  COMPOUND RANGE CONCEPTS (C++26)
    // =====================================================================

#if defined(__cpp_lib_ranges)

    // sortable_range
    //   concept: constrains _R to forward ranges whose elements are
    // sortable (MoveConstructible + MoveAssignable + swappable +
    // LessThanComparable).
    template<typename _R>
    concept sortable_range =
        ( forward_range<_R> &&
          sortable_element<std::ranges::range_value_t<_R>> );

    // contiguous_value_range
    //   concept: constrains _R to contiguous ranges whose elements
    // are full value types.
    template<typename _R>
    concept contiguous_value_range =
        ( contiguous_range<_R> &&
          value_type<std::ranges::range_value_t<_R>> );

    // relocatable_range
    //   concept: constrains _R to contiguous ranges whose elements
    // are trivially relocatable — ideal for bulk-move optimizations.
    template<typename _R>
    concept relocatable_range =
        ( contiguous_range<_R> &&
          trivially_relocatable<std::ranges::range_value_t<_R>> );

#endif  // __cpp_lib_ranges

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// III. STATIC ASSERTION DIAGNOSTIC HELPERS (C++26)
// =========================================================================

// D_STATIC_ASSERT_TRIVIALLY_RELOCATABLE
//   macro: static_assert that _T is trivially relocatable with a
// descriptive error message.
#define D_STATIC_ASSERT_TRIVIALLY_RELOCATABLE(_T)                          \
    static_assert(                                                         \
        is_trivially_relocatable_v<_T>,                                    \
        "Type " #_T " is not trivially relocatable")

// D_STATIC_ASSERT_TRIVIAL_VALUE_TYPE
//   macro: static_assert that _T is a trivial value type with
// individual error messages for each failed requirement.
#define D_STATIC_ASSERT_TRIVIAL_VALUE_TYPE(_T)                             \
    static_assert(                                                         \
        std::is_trivially_default_constructible_v<_T>,                     \
        "Type " #_T " is not trivially default-constructible");            \
    static_assert(                                                         \
        std::is_trivially_copy_constructible_v<_T>,                        \
        "Type " #_T " is not trivially copy-constructible");               \
    static_assert(                                                         \
        std::is_trivially_move_constructible_v<_T>,                        \
        "Type " #_T " is not trivially move-constructible");               \
    static_assert(                                                         \
        std::is_trivially_copy_assignable_v<_T>,                           \
        "Type " #_T " is not trivially copy-assignable");                  \
    static_assert(                                                         \
        std::is_trivially_move_assignable_v<_T>,                           \
        "Type " #_T " is not trivially move-assignable");                  \
    static_assert(                                                         \
        std::is_trivially_destructible_v<_T>,                              \
        "Type " #_T " is not trivially destructible")


// =========================================================================
// IV.  TYPE CLASSIFICATION COMPOUND CONCEPTS (C++26)
// =========================================================================

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

D_NAMESPACE(concepts)

    // memcpy_safe
    //   concept: constrains _T to types that can be safely copied via
    // memcpy/memmove. Equivalent to TriviallyCopyable.
    // This is a semantic alias providing intent-clarity for low-level
    // code that performs raw byte copying.
    template<typename _T>
    concept memcpy_safe =
        trivially_copyable<_T>;

    // c_compatible
    //   concept: constrains _T to types that are safe to pass across
    // C/C++ language boundaries (standard-layout + trivially copyable).
    template<typename _T>
    concept c_compatible =
        ( standard_layout_type<_T> &&
          trivially_copyable<_T> );

    // relocatable_value
    //   concept: constrains _T to types that are both a full value type
    // (all six named requirements) and trivially relocatable.
    template<typename _T>
    concept relocatable_value =
        ( value_type<_T> &&
          trivially_relocatable<_T> );

    // implicit_lifetime_and_relocatable
    //   concept: constrains _T to types that are both
    // implicit-lifetime and trivially relocatable. Such types are
    // ideal candidates for container optimizations (placement new,
    // realloc, uninitialized byte buffers).
    template<typename _T>
    concept implicit_lifetime_and_relocatable =
        ( implicit_lifetime_type<_T> &&
          trivially_relocatable<_T> );

NS_END  // concepts

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


// =========================================================================
// V.   DEPRECATION NOTES (C++26)
// =========================================================================

// NOTE: TrivialType deprecation (C++26)
//   The TrivialType named requirement is deprecated as of C++26.
//   djinterp::traits::is_trivial<_T> remains available for backward
//   compatibility. The standard recommends checking the specific
//   properties needed (e.g., is_trivially_copyable,
//   is_trivially_default_constructible) rather than is_trivial.
//
// NOTE: PODType removal (C++26)
//   std::is_pod was removed from the standard library in C++26.
//   djinterp::traits::is_pod<_T> remains available and falls back to
//   (is_trivial && is_standard_layout) when std::is_pod is absent.
//   Prefer is_trivially_copyable + is_standard_layout for new code.


NS_END  // traits
NS_END  // djinterp

#endif  // D_ENV_LANG_IS_CPP26_OR_HIGHER


#endif  // DJINTERP_CPP_NAMED26_
