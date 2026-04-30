/******************************************************************************
* djinterp [container]                         sequential_container_traits.hpp
*
* Sequential container compile-time classification:
*   Detects whether a type preserves insertion order (sequential)
* and classifies its storage model.  Sequential containers
* include arrays, vectors, deques, lists, strings, and any
* user-defined ordered container - but NOT unordered
* associative containers or cyclic graphs.
*   Detection is purely structural: a type is sequential if it
* is iterable and does NOT expose a `hasher` type alias (which
* would indicate an unordered associative container).
*   This header is the traits-only counterpart to
* sequential_container.hpp (the CRTP base).  Include this file
* when you need the classification without the CRTP algorithms.
* Contents:
*   Detection (namespace internal)
*     - has_hasher_check        hasher alias detection
*     - has_c_str_check         string-like detection
*     - sequential_kind_helper  storage model classification
*   Public traits
*     - is_sequential_container     true if type is sequential
*     - DSequentialKind             storage model enum
*     - sequential_kind             storage model classifier
*
*
* path:      /inc/djinterp/core/container/traits/
*                sequential_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.09
******************************************************************************/

#ifndef DJINTERP_SEQUENTIAL_CONTAINER_TRAITS_
#define DJINTERP_SEQUENTIAL_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "container_traits.hpp"
#include "../iterator/iterator_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Internal Detection Helpers
// ===========================================================================

NS_INTERNAL

    // has_hasher_check
    //   trait: detects whether a type exposes a `hasher` member
    // alias, indicating an unordered associative container.
    // Primary template (failure case).
    template<typename _Container,
             typename = void>
    struct has_hasher_check : std::false_type
    {};

    // has_hasher_check (success case)
    //   trait: partial specialization when `hasher` alias
    // exists.
    template<typename _Container>
    struct has_hasher_check<
        _Container,
        std::void_t<typename _Container::hasher>
    > : std::true_type
    {};

    // has_c_str_check
    //   trait: detects whether a type exposes c_str(),
    // indicating a string-like container.
    // Primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_c_str_check : std::false_type
    {};

    // has_c_str_check (success case)
    //   trait: partial specialization when c_str() is
    // well-formed.
    template<typename _Type>
    struct has_c_str_check<
        _Type,
        std::void_t<decltype(
            std::declval<const _Type&>().c_str())>
    > : std::true_type
    {};

    // sequential_kind_helper
    //   trait: classifies the storage model of a sequential
    // container.  Priority: string --> array --> list -->
    // forward_list --> deque --> none.
    template<typename _Type>
    struct sequential_kind_helper
    {
        using clean_type = clean_t<_Type>;

        static constexpr DSequentialKind value =
            // string-like (has c_str())
            has_c_str_check<clean_type>::value
                ? DSequentialKind::string_like

            // contiguous + random-access
            : ( has_data_accessor_v<clean_type> &&
                is_random_access_iterable_v<clean_type> )
                ? DSequentialKind::array_like

            // bidirectional but not contiguous
            : ( is_bidirectional_iterable_v<clean_type> &&
                !has_data_accessor_v<clean_type> )
                ? DSequentialKind::list_like

            // forward-only
            : ( is_forward_iterable_v<clean_type> &&
                !is_bidirectional_iterable_v<clean_type> )
                ? DSequentialKind::forward_list_like

            // random-access but not contiguous (deque)
            : ( is_random_access_iterable_v<clean_type> &&
                !has_data_accessor_v<clean_type> )
                ? DSequentialKind::deque_like

            : DSequentialKind::none;
    };

NS_END  // internal


// ===========================================================================
// II.  Public Sequential Classification
// ===========================================================================

// is_sequential_container
//   trait: true if the container preserves insertion order.
// Structurally: iterable AND NOT unordered associative (no
// hasher/key_equal).
//
//   This is broader than the STL SequenceContainer named
// requirement.  A forward_list is sequential but does not
// satisfy the full SequenceContainer concept.
template<typename _Type>
struct is_sequential_container
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_iterable_container_v<clean_type>              &&
          !internal::has_hasher_check<clean_type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES
    template<typename _Type>
    inline constexpr bool is_sequential_container_v =
        is_sequential_container<_Type>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_INLINE_VARIABLES

// ===========================================================================
// III. Sequential Kind Classification
// ===========================================================================

// DSequentialKind
//   enum: classifies the sequential storage model.
enum class DSequentialKind
{
    // unknown or non-sequential
    none,

    // contiguous random-access (array, vector)
    array_like,

    // node-based bidirectional (list)
    list_like,

    // node-based forward-only (forward_list, slist)
    forward_list_like,

    // double-ended contiguous segments (deque)
    deque_like,

    // string / text buffer
    string_like
};

// sequential_kind
//   trait: resolves the DSequentialKind for a given type.
template<typename _Type>
struct sequential_kind
{
    static constexpr DSequentialKind value =
        internal::sequential_kind_helper<_Type>::value;
};

template<typename _Type>
inline constexpr DSequentialKind sequential_kind_v =
    sequential_kind<_Type>::value;


NS_END  // djinterp


#endif  // DJINTERP_SEQUENTIAL_CONTAINER_TRAITS_