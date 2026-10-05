/*******************************************************************************
* djinterp [core]                                                list_traits.hpp
*
* Compile-time SFINAE detection for list-like containers:
*   This header provides the abstract-layer traits that identify any
* container satisfying the list contract — an ordered, iterator-stable
* sequence supporting O(1) splice / per-iterator erase / per-iterator
* insert given an iterator handle.
*
*   The traits are independent of any specific list implementation.
* They detect std::list and std::forward_list as readily as our
* djinterp::linked_list, and will detect skip_list /
* unrolled_linked_list when those land — anything that exposes the
* canonical list operation set.
*
*   list_traits.hpp sits one level above linked_list_traits.hpp:
* linked_list_traits answers questions about node shape and link
* topology (singly / doubly / xor / skip), while list_traits answers
* questions about which list operations are supported (splice,
* merge, sort, unique, remove).  A type that satisfies
* is_linked_list will normally also satisfy is_list_container, but
* the inverse does not hold — std::list is a list_container but is
* not a linked_list under our trait family because its node type is
* implementation-private.
*
*
*   PORTABILITY:
*   C++11 baseline.  All `_v` aliases gated on
* D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES.  C++20 concepts live in
* list_concepts.hpp.
*
*   This header has no dependency on list.hpp or linked_list.hpp —
* it is a pure trait surface usable from any module.
*
*
* path:      /inc/djinterp/core/container/list/list_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    List-method detection      (splice, unique, merge, remove,
      ----------------------------------------------------------

      remove_if, sort)

II.   Iterator-return detection  (insert/erase return iterator)
      ---------------------------------------------------------

III.  Stability-tag detection    (opt-in is_node_stable bool)
      -------------------------------------------------------

IV.   Composite list classification
      -----------------------------

V.    Capability subsets         (spliceable / mergeable / sortable / ...)
      --------------------------------------------------------------------

VI.   Aggregate snapshot         (list_class<T>)
      ------------------------------------------

VII.  SFINAE guards
      -------------
*/

#ifndef DJINTERP_CONTAINER_LIST_LIST_TRAITS_HPP
#define DJINTERP_CONTAINER_LIST_LIST_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   LIST-METHOD DETECTION
// ===========================================================================
//   Each probe looks for a specific member-function shape.  The
// probes are tagless — the trait succeeds when the call expression
// is well-formed in SFINAE context.

NS_INTERNAL

    // list_traits_dummy_unary_pred
    //   helper: stub predicate used to probe member functions accepting a
    // unary predicate (e.g. remove_if, unique with equality predicate).
    struct list_traits_dummy_unary_pred
    {
        template<typename Arg>
        bool operator()(Arg&&) const noexcept
        {
            return false;
        }
    };

    // list_traits_dummy_binary_pred
    //   helper: stub used to probe member functions accepting a binary
    // comparator (e.g. merge with comparator, sort with comparator).
    struct list_traits_dummy_binary_pred
    {
        template<typename A,
                 typename B>
        bool operator()(A&&, B&&) const noexcept
        {
            return false;
        }
    };

    // ---------------------------------------------------------------------
    // splice — std::list / linked_list signature: void splice(iterator,
    // list&) — the core list operation that makes a container "list-
    // like".  We probe the (it, list&) form; lists that only expose
    // splice_after (std::forward_list-style) are caught by a separate
    // probe below.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_splice_helper : std::false_type
    {};

    template<typename Type>
    struct has_splice_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().splice(
                std::declval<typename clean_t<Type>::const_iterator>(),
                std::declval<Type&>()))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // splice_after — std::forward_list signature.  Distinct from splice
    // because callers that work over forward-only lists must use the
    // "after" form to avoid an O(n) walk to the predecessor.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_splice_after_helper : std::false_type
    {};

    template<typename Type>
    struct has_splice_after_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().splice_after(
                std::declval<typename clean_t<Type>::const_iterator>(),
                std::declval<Type&>()))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // unique — collapses consecutive equal elements.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_unique_helper : std::false_type
    {};

    template<typename Type>
    struct has_unique_helper<
        Type,
        void_t<decltype(std::declval<Type&>().unique())>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // unique with equality predicate
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_unique_pred_helper : std::false_type
    {};

    template<typename Type>
    struct has_unique_pred_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().unique(
                list_traits_dummy_binary_pred{}))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // merge — in-place merge of two sorted lists.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_merge_helper : std::false_type
    {};

    template<typename Type>
    struct has_merge_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().merge(
                std::declval<Type&>()))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // merge with comparator
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_merge_cmp_helper : std::false_type
    {};

    template<typename Type>
    struct has_merge_cmp_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().merge(
                std::declval<Type&>(),
                list_traits_dummy_binary_pred{}))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // remove(value) — list-specific: removes ALL elements matching
    // value, distinct from std::set::erase(value) and from the
    // generic mutable-container has_erase_signal trait.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_list_remove_helper : std::false_type
    {};

    template<typename Type>
    struct has_list_remove_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().remove(
                std::declval<typename clean_t<Type>::value_type>()))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // remove_if(pred)
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_list_remove_if_helper : std::false_type
    {};

    template<typename Type>
    struct has_list_remove_if_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().remove_if(
                list_traits_dummy_unary_pred{}))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // sort() — list-specific in-place sort (distinct from std::sort,
    // which requires random access).
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_list_sort_helper : std::false_type
    {};

    template<typename Type>
    struct has_list_sort_helper<
        Type,
        void_t<decltype(std::declval<Type&>().sort())>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // sort(cmp)
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_list_sort_cmp_helper : std::false_type
    {};

    template<typename Type>
    struct has_list_sort_cmp_helper<
        Type,
        void_t<decltype(
            std::declval<Type&>().sort(
                list_traits_dummy_binary_pred{}))>
    > : std::true_type
    {};

    // ---------------------------------------------------------------------
    // reverse() — in-place reversal.  Standard on std::list and
    // std::forward_list.
    // ---------------------------------------------------------------------
    template<typename Type,
             typename = void>
    struct has_list_reverse_helper : std::false_type
    {};

    template<typename Type>
    struct has_list_reverse_helper<
        Type,
        void_t<decltype(std::declval<Type&>().reverse())>
    > : std::true_type
    {};

NS_END  // internal

// has_splice_method
//   trait: true when Type exposes splice(const_iterator, Type&).
template<typename Type>
struct has_splice_method
    : djinterp::bool_constant<
          internal::has_splice_helper<Type>::value>
{};

// has_splice_after_method
//   trait: true when Type exposes splice_after(const_iterator, Type&).
// Forward-list-style lists expose this in place of splice.
template<typename Type>
struct has_splice_after_method
    : djinterp::bool_constant<
          internal::has_splice_after_helper<Type>::value>
{};

// has_unique_method
//   trait: true when Type exposes unique() (consecutive-duplicate collapse).
template<typename Type>
struct has_unique_method
    : djinterp::bool_constant<
          internal::has_unique_helper<Type>::value>
{};

// has_unique_pred_method
//   trait: true when Type exposes unique(BinaryPred).
template<typename Type>
struct has_unique_pred_method
    : djinterp::bool_constant<
          internal::has_unique_pred_helper<Type>::value>
{};

// has_merge_method
//   trait: true when Type exposes merge(Type&).
template<typename Type>
struct has_merge_method
    : djinterp::bool_constant<
          internal::has_merge_helper<Type>::value>
{};

// has_merge_cmp_method
//   trait: true when Type exposes merge(Type&, BinaryCmp).
template<typename Type>
struct has_merge_cmp_method
    : djinterp::bool_constant<
          internal::has_merge_cmp_helper<Type>::value>
{};

// has_list_remove_method
//   trait: true when Type exposes remove(value_type) (list-style: removes ALL
// matches).
template<typename Type>
struct has_list_remove_method
    : djinterp::bool_constant<
          internal::has_list_remove_helper<Type>::value>
{};

// has_list_remove_if_method
//   trait: true when Type exposes remove_if(UnaryPred).
template<typename Type>
struct has_list_remove_if_method
    : djinterp::bool_constant<
          internal::has_list_remove_if_helper<Type>::value>
{};

// has_list_sort_method
//   trait: true when Type exposes sort() as a member.
template<typename Type>
struct has_list_sort_method
    : djinterp::bool_constant<
          internal::has_list_sort_helper<Type>::value>
{};

// has_list_sort_cmp_method
//   trait: true when Type exposes sort(BinaryCmp) as a member.
template<typename Type>
struct has_list_sort_cmp_method
    : djinterp::bool_constant<
          internal::has_list_sort_cmp_helper<Type>::value>
{};

// has_list_reverse_method
//   trait: true when Type exposes reverse() as a member.
template<typename Type>
struct has_list_reverse_method
    : djinterp::bool_constant<
          internal::has_list_reverse_helper<Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    inline constexpr bool has_splice_method_v =
        has_splice_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_splice_after_method_v =
        has_splice_after_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_unique_method_v =
        has_unique_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_unique_pred_method_v =
        has_unique_pred_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_merge_method_v =
        has_merge_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_merge_cmp_method_v =
        has_merge_cmp_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_list_remove_method_v =
        has_list_remove_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_list_remove_if_method_v =
        has_list_remove_if_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_list_sort_method_v =
        has_list_sort_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_list_sort_cmp_method_v =
        has_list_sort_cmp_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_list_reverse_method_v =
        has_list_reverse_method<Type>::value;
#endif


// ===========================================================================
// II.  ITERATOR-RETURN METHOD DETECTION
// ===========================================================================
//   Lists return iterators from insert / erase by convention,
// distinguishing them from the void-returning insert / erase used
// by hash-set-style containers.  These probes detect the iterator-
// returning shape specifically.

NS_INTERNAL

    // has_iter_erase_helper
    //   trait: detects erase(iterator) returning iterator.
    template<typename Type,
             typename = void>
    struct has_iter_erase_helper : std::false_type
    {};

    // has_iter_erase_helper<Type, typename std::enable_if<
    // std::is_convertible< decltype(std::declval<Type&>().erase(
    // std::declval<typename clean_t<Type>
    //   trait: the `typename std::enable_if< std::is_convertible<
    // decltype(std::declval<Type&>().erase( std::declval<typename
    // clean_t<Type` case; it reports true.
    template<typename Type>
    struct has_iter_erase_helper<
        Type,
        typename std::enable_if<
            std::is_convertible<
                decltype(std::declval<Type&>().erase(
                    std::declval<typename clean_t<Type>::const_iterator>())),
                typename clean_t<Type>::iterator>::value
        >::type
    > : std::true_type
    {};

    // has_iter_insert_helper
    //   trait: detects insert(const_iterator, value_type) returning iterator.
    template<typename Type,
             typename = void>
    struct has_iter_insert_helper : std::false_type
    {};

    // has_iter_insert_helper<Type, typename std::enable_if<
    // std::is_convertible< decltype(std::declval<Type&>().insert(
    // std::declval<typename clean_t<Type>
    //   trait: the `typename std::enable_if< std::is_convertible<
    // decltype(std::declval<Type&>().insert( std::declval<typename
    // clean_t<Type` case; it reports true.
    template<typename Type>
    struct has_iter_insert_helper<
        Type,
        typename std::enable_if<
            std::is_convertible<
                decltype(std::declval<Type&>().insert(
                    std::declval<typename clean_t<Type>::const_iterator>(),
                    std::declval<typename clean_t<Type>::value_type>())),
                typename clean_t<Type>::iterator>::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// has_iter_erase_method
//   trait: true when Type exposes erase(const_iterator) returning iterator.
template<typename Type>
struct has_iter_erase_method
    : djinterp::bool_constant<
          internal::has_iter_erase_helper<Type>::value>
{};

// has_iter_insert_method
//   trait: true when Type exposes insert(const_iterator, value_type)
// returning iterator.
template<typename Type>
struct has_iter_insert_method
    : djinterp::bool_constant<
          internal::has_iter_insert_helper<Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    inline constexpr bool has_iter_erase_method_v =
        has_iter_erase_method<Type>::value;
    template<typename Type>
    inline constexpr bool has_iter_insert_method_v =
        has_iter_insert_method<Type>::value;
#endif


// ===========================================================================
// III. STABILITY-TAG DETECTION
// ===========================================================================
//   Lists are conventionally iterator-stable: an insert or erase
// invalidates only the iterator pointing at the affected element,
// leaving all others valid.  Containers can opt in to this guarantee
// by exposing a `static constexpr bool is_node_stable = true;`
// member.  Detection uses the two-step probe: existence of the
// member followed by truth of the value.

NS_INTERNAL

    // has_node_stable_tag_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename = void>
    struct has_node_stable_tag_helper : std::false_type
    {};

    // has_node_stable_tag_helper (success case)
    //   trait: matches when Type exposes a static `is_node_stable` member
    // equal to true.
    template<typename Type>
    struct has_node_stable_tag_helper<
        Type,
        void_t<decltype(clean_t<Type>::is_node_stable)>
    > : djinterp::bool_constant<
            static_cast<bool>(clean_t<Type>::is_node_stable)>
    {};

NS_END  // internal

// has_node_stable_tag
//   trait: true when Type opts in to node-stability via the `is_node_stable`
// static bool member.
template<typename Type>
struct has_node_stable_tag
    : djinterp::bool_constant<
          internal::has_node_stable_tag_helper<Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    inline constexpr bool has_node_stable_tag_v =
        has_node_stable_tag<Type>::value;
#endif


// ===========================================================================
// IV.  COMPOSITE LIST CLASSIFICATION
// ===========================================================================

// is_list_container
//   trait: true when Type satisfies the list contract. A type qualifies if it
// exposes splice (or splice_after) — the operation
// that distinguishes lists from sequence containers because it requires the
// underlying node-stable storage that lists provide.
//
//   The detection is intentionally permissive on the second signal: a type
// with splice but no merge / sort still qualifies as a list_container, since
// splice alone is the defining operation. Specialized capability traits below
// (is_mergeable_list, is_sortable_list, ...) discriminate further.
template<typename Type>
struct is_list_container
    : djinterp::bool_constant<
          ( has_splice_method<Type>::value      ||
            has_splice_after_method<Type>::value )>
{};

// is_node_stable_list
//   trait: true when Type is a list-container AND advertises the
// is_node_stable static bool. Useful when callers need explicit stability
// guarantees beyond the implicit list-contract level.
template<typename Type>
struct is_node_stable_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            has_node_stable_tag<Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    inline constexpr bool is_list_container_v =
        is_list_container<Type>::value;
    template<typename Type>
    inline constexpr bool is_node_stable_list_v =
        is_node_stable_list<Type>::value;
#endif


// ===========================================================================
// V.   CAPABILITY SUBSETS
// ===========================================================================
//   Each capability trait fires when the list both qualifies as a
// list_container AND exposes the additional operation.  The base
// list_container check screens out non-lists that happen to expose
// a coincidentally-named member.

// is_spliceable_list
//   trait: list_container with either splice form.
template<typename Type>
struct is_spliceable_list
    : djinterp::bool_constant<
          is_list_container<Type>::value>
{};

// is_mergeable_list
//   trait: list_container that exposes merge.
template<typename Type>
struct is_mergeable_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            has_merge_method<Type>::value )>
{};

// is_sortable_list
//   trait: list_container that exposes sort as a member.
template<typename Type>
struct is_sortable_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            has_list_sort_method<Type>::value )>
{};

// is_unique_capable_list
//   trait: list_container that exposes unique.
template<typename Type>
struct is_unique_capable_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            has_unique_method<Type>::value )>
{};

// is_removable_list
//   trait: list_container that exposes remove or remove_if.
template<typename Type>
struct is_removable_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            ( has_list_remove_method<Type>::value ||
              has_list_remove_if_method<Type>::value ) )>
{};

// is_reversible_list
//   trait: list_container that exposes reverse as a member. This is broader
// than the iterator-level "supports reverse iteration" notion and reflects
// in-place reversal capability.
template<typename Type>
struct is_reversible_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value &&
            has_list_reverse_method<Type>::value )>
{};

// is_full_list
//   trait: list_container that exposes the complete std::list operation
// surface — splice, merge, sort, unique, remove, remove_if, reverse.
template<typename Type>
struct is_full_list
    : djinterp::bool_constant<
          ( is_list_container<Type>::value         &&
            has_merge_method<Type>::value          &&
            has_list_sort_method<Type>::value      &&
            has_unique_method<Type>::value         &&
            has_list_remove_method<Type>::value    &&
            has_list_remove_if_method<Type>::value &&
            has_list_reverse_method<Type>::value )>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    inline constexpr bool is_spliceable_list_v =
        is_spliceable_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_mergeable_list_v =
        is_mergeable_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_sortable_list_v =
        is_sortable_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_unique_capable_list_v =
        is_unique_capable_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_removable_list_v =
        is_removable_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_reversible_list_v =
        is_reversible_list<Type>::value;
    template<typename Type>
    inline constexpr bool is_full_list_v =
        is_full_list<Type>::value;
#endif


// ===========================================================================
// VI.  AGGREGATE SNAPSHOT
// ===========================================================================

// list_class
//   struct: comprehensive aggregation of a type's list-related classification.
// Mirrors linked_list_class<T> / radix_tree_class<T> /
// tree_container_class<T>.
template<typename Type>
struct list_class
{
    // primary identity
    static constexpr bool is_list      = is_list_container<Type>::value;
    static constexpr bool is_full      = is_full_list<Type>::value;
    static constexpr bool is_stable    = is_node_stable_list<Type>::value;

    // splice forms
    static constexpr bool has_splice       = has_splice_method<Type>::value;
    static constexpr bool has_splice_after =
        has_splice_after_method<Type>::value;

    // capability operations
    static constexpr bool can_unique     = has_unique_method<Type>::value;
    static constexpr bool can_unique_pred =
        has_unique_pred_method<Type>::value;
    static constexpr bool can_merge      = has_merge_method<Type>::value;
    static constexpr bool can_merge_cmp  = has_merge_cmp_method<Type>::value;
    static constexpr bool can_remove     =
        has_list_remove_method<Type>::value;
    static constexpr bool can_remove_if  =
        has_list_remove_if_method<Type>::value;
    static constexpr bool can_sort       =
        has_list_sort_method<Type>::value;
    static constexpr bool can_sort_cmp   =
        has_list_sort_cmp_method<Type>::value;
    static constexpr bool can_reverse    =
        has_list_reverse_method<Type>::value;

    // iterator-shape operations
    static constexpr bool can_iter_erase  =
        has_iter_erase_method<Type>::value;
    static constexpr bool can_iter_insert =
        has_iter_insert_method<Type>::value;

    // capability roll-ups
    static constexpr bool is_spliceable    =
        is_spliceable_list<Type>::value;
    static constexpr bool is_mergeable     =
        is_mergeable_list<Type>::value;
    static constexpr bool is_sortable      =
        is_sortable_list<Type>::value;
    static constexpr bool is_unique_capable=
        is_unique_capable_list<Type>::value;
    static constexpr bool is_removable     =
        is_removable_list<Type>::value;
    static constexpr bool is_reversible    =
        is_reversible_list<Type>::value;
};


// ===========================================================================
// VII. SFINAE GUARDS
// ===========================================================================

// enable_if_list_container
//   trait: SFINAE guard restricting templates to list-containers.
template<typename Type>
struct enable_if_list_container
{
    using type = typename std::enable_if<
        is_list_container<Type>::value>::type;
};

// enable_if_list_container_t
//   type: the carrier of enable_if_list_container -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using enable_if_list_container_t =
    typename enable_if_list_container<Type>::type;

template<typename Type>
struct enable_if_spliceable_list
{
    using type = typename std::enable_if<
        is_spliceable_list<Type>::value>::type;
};

// enable_if_spliceable_list_t
//   type: the carrier of enable_if_spliceable_list -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using enable_if_spliceable_list_t =
    typename enable_if_spliceable_list<Type>::type;

template<typename Type>
struct enable_if_mergeable_list
{
    using type = typename std::enable_if<
        is_mergeable_list<Type>::value>::type;
};

// enable_if_mergeable_list_t
//   type: the carrier of enable_if_mergeable_list -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using enable_if_mergeable_list_t =
    typename enable_if_mergeable_list<Type>::type;

template<typename Type>
struct enable_if_sortable_list
{
    using type = typename std::enable_if<
        is_sortable_list<Type>::value>::type;
};

// enable_if_sortable_list_t
//   type: the carrier of enable_if_sortable_list -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using enable_if_sortable_list_t =
    typename enable_if_sortable_list<Type>::type;

template<typename Type>
struct enable_if_node_stable_list
{
    using type = typename std::enable_if<
        is_node_stable_list<Type>::value>::type;
};

// enable_if_node_stable_list_t
//   type: the carrier of enable_if_node_stable_list -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using enable_if_node_stable_list_t =
    typename enable_if_node_stable_list<Type>::type;

template<typename Type>
struct enable_if_full_list
{
    using type = typename std::enable_if<
        is_full_list<Type>::value>::type;
};

// enable_if_full_list_t
//   type: the carrier of enable_if_full_list -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using enable_if_full_list_t =
    typename enable_if_full_list<Type>::type;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_LIST_LIST_TRAITS_HPP
