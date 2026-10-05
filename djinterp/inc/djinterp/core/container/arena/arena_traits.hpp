/*******************************************************************************
* djinterp [core]                                               arena_traits.hpp
*
* Arena SFINAE detection traits:
*   This header provides compile-time structural traits for detecting
* and classifying arena types, arena nodes, link policies, and payload
* validity.  Detection is purely structural - no tagging, no base-class
* checks.
*
* Traits provided:
*   PAYLOAD VALIDATION
*   - is_arena_payload<T>            does T satisfy payload requirements?
*
*   LINK POLICY DETECTION
*   - is_link_policy<T>             does T expose link policy constants?
*   - has_link_flag<Policy, Flag>   does Policy include a specific link?
*   - link_policy_flags<T>          extracts the raw flag bitmask
*   - link_policy_num_links<T>      extracts the link count
*
*   ARENA NODE DETECTION
*   - is_arena_node<T>              is T an arena_node<...>?
*   - arena_node_payload_type<T>    extracts payload_type from a node
*   - arena_node_link_policy<T>     extracts link_policy from a node
*
*   ARENA DETECTION
*   - is_arena<T>                   is T an arena-like container?
*   - arena_payload_type<T>         extracts payload_type from an arena
*   - arena_link_policy_type<T>     extracts link_policy from an arena
*
*   CROSS-ARENA
*   - arenas_cross_referenceable<A,B>  can two arenas share stable_ids?
*
*   COMBINED CLASSIFICATION
*   - arena_class<T>                aggregate classification struct
*
*
* path:      /inc/djinterp/core/container/arena/arena_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.03.18
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARENA_ARENA_TRAITS_HPP
#define DJINTERP_CONTAINER_ARENA_ARENA_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"        // framework root
#include "../../meta/trait_detect.hpp"  // D_VOID_T
#include "./arena.hpp"                  // the arena types classified here


NS_DJINTERP

// ===========================================================================
// I.   Payload Validation
// ===========================================================================

NS_INTERNAL

    // is_arena_payload_helper
    //   trait: primary template (failure case).
    template<typename Type,
             typename = void>
    struct is_arena_payload_helper : std::false_type
    {};;

    // is_arena_payload_helper (success case)
    //   trait: succeeds when Type is destructible and either
    // move-constructible or copy-constructible.
    template<typename Type>
    struct is_arena_payload_helper<
        Type,
        typename std::enable_if<
            ( std::is_destructible<Type>::value &&
              ( std::is_move_constructible<Type>::value ||
                std::is_copy_constructible<Type>::value ) )
        >::type
    > : std::true_type
    {};;

NS_END  // internal

// is_arena_payload
//   trait: detects whether Type satisfies the minimum requirements for use as
// an arena payload.
template<typename Type>
struct is_arena_payload
    : internal::is_arena_payload_helper<Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_arena_payload_v =
        is_arena_payload<Type>::value;
#endif


// ===========================================================================
// II.  Link Policy Detection
// ===========================================================================

// is_link_policy
//   trait: detects whether Type exposes the link policy interface (num_links,
// flags).
template<typename Type,
         typename = void>
struct is_link_policy : std::false_type
{};

// is_link_policy specialization
//   helper: the detected case -- selected when the type declares the static
// members `num_links` and `flags`.
template<typename Type>
struct is_link_policy<Type,
    D_VOID_T<
        decltype(clean_t<Type>::num_links),
        decltype(clean_t<Type>::flags)
    >> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_link_policy_v =
        is_link_policy<Type>::value;
#endif

// has_link_flag
//   trait: detects whether a link policy includes a specific flag. Flag must
// be one of the tree_link constants.
template<typename Policy,
         unsigned Flag,
         typename = void>
struct has_link_flag : std::false_type
{};

// has_link_flag<Policy, Flag, typename std::enable_if< (
// is_link_policy<Policy>
//   trait: the `typename std::enable_if< ( is_link_policy<Policy` case; it
// reports true.
template<typename Policy,
         unsigned Flag>
struct has_link_flag<Policy, Flag,
    typename std::enable_if<
        ( is_link_policy<Policy>::value &&
          ((clean_t<Policy>::flags & Flag) != 0) )
    >::type
> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Policy,
             unsigned Flag>
    D_CONSTEXPR bool has_link_flag_v =
        has_link_flag<Policy, Flag>::value;
#endif

// link_policy_flags
//   trait: extracts the raw flag bitmask from a policy.
NS_INTERNAL

    template<typename Policy,
             typename = void>
    struct link_policy_flags_helper
    {
        static D_CONSTEXPR unsigned value = 0;
    };

    template<typename Policy>
    struct link_policy_flags_helper<Policy,
        D_VOID_T<decltype(clean_t<Policy>::flags)>>
    {
        static D_CONSTEXPR unsigned value =
            clean_t<Policy>::flags;
    };

NS_END  // internal

template<typename Policy>
struct link_policy_flags
{
    static D_CONSTEXPR unsigned value =
        internal::link_policy_flags_helper<Policy>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Policy>
    D_CONSTEXPR unsigned link_policy_flags_v =
        link_policy_flags<Policy>::value;
#endif

// link_policy_num_links
//   trait: extracts num_links from a policy.
NS_INTERNAL

    template<typename Policy,
             typename = void>
    struct link_policy_num_links_helper
    {
        static D_CONSTEXPR std::size_t value = 0;
    };

    template<typename Policy>
    struct link_policy_num_links_helper<Policy,
        D_VOID_T<decltype(clean_t<Policy>::num_links)>>
    {
        static D_CONSTEXPR std::size_t value =
            clean_t<Policy>::num_links;
    };

NS_END  // internal

template<typename Policy>
struct link_policy_num_links
{
    static D_CONSTEXPR std::size_t value =
        internal::link_policy_num_links_helper<
            Policy>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Policy>
    D_CONSTEXPR std::size_t link_policy_num_links_v =
        link_policy_num_links<Policy>::value;
#endif


// ===========================================================================
// III. Arena Node Detection
// ===========================================================================

// is_arena_node
//   trait: detects whether Type is an arena_node-like type (exposes
// payload_type, link_policy, num_links, stable_id field, alive field).
template<typename Type,
         typename = void>
struct is_arena_node : std::false_type
{};

// is_arena_node<Type, D_VOID_T< typename clean_t<Type>
//   trait: the `D_VOID_T< typename clean_t<Type` case; it reports true.
template<typename Type>
struct is_arena_node<Type,
    D_VOID_T<
        typename clean_t<Type>::payload_type,
        typename clean_t<Type>::link_policy,
        decltype(clean_t<Type>::num_links),
        decltype(std::declval<const Type&>().stable_id),
        decltype(std::declval<const Type&>().alive)
    >> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_arena_node_v =
        is_arena_node<Type>::value;
#endif

// arena_node_payload_type
//   trait: SFINAE-safe extraction of payload_type from an arena node.
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct arena_node_payload_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct arena_node_payload_type_helper<Type,
        D_VOID_T<typename clean_t<Type>::payload_type>>
    {
        using type = typename clean_t<Type>::payload_type;
    };

NS_END  // internal

template<typename Type>
struct arena_node_payload_type
    : internal::arena_node_payload_type_helper<Type>
{};

// arena_node_payload_type_t
//   type: the carrier of arena_node_payload_type -- its `::type`, for use
// where a type rather than a value is wanted.
template<typename Type>
using arena_node_payload_type_t =
    typename arena_node_payload_type<Type>::type;

// arena_node_link_policy
//   trait: SFINAE-safe extraction of link_policy from an arena node.
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct arena_node_link_policy_helper
    {
        using type = void;
    };

    template<typename Type>
    struct arena_node_link_policy_helper<Type,
        D_VOID_T<typename clean_t<Type>::link_policy>>
    {
        using type = typename clean_t<Type>::link_policy;
    };

NS_END  // internal

template<typename Type>
struct arena_node_link_policy
    : internal::arena_node_link_policy_helper<Type>
{};

// arena_node_link_policy_t
//   type: the carrier of arena_node_link_policy -- its `::type`, for use where
// a type rather than a value is wanted.
template<typename Type>
using arena_node_link_policy_t =
    typename arena_node_link_policy<Type>::type;


// ===========================================================================
// IV.  Arena Detection
// ===========================================================================

// is_arena
//   trait: detects whether Type is an arena-like container
// exposing payload_type, link_policy, and indexed access via
// operator[](node_id).
template<typename Type,
         typename = void>
struct is_arena : std::false_type
{};

// is_arena<Type, D_VOID_T< typename clean_t<Type>
//   trait: the `D_VOID_T< typename clean_t<Type` case; it reports true.
template<typename Type>
struct is_arena<Type,
    D_VOID_T<
        typename clean_t<Type>::payload_type,
        typename clean_t<Type>::link_policy,
        decltype(std::declval<const Type&>()[
            std::declval<node_id>()])
    >> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_arena_v = is_arena<Type>::value;
#endif

// arena_payload_type
//   trait: SFINAE-safe extraction of payload_type from an arena container.
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct arena_payload_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct arena_payload_type_helper<Type,
        D_VOID_T<typename clean_t<Type>::payload_type>>
    {
        using type = typename clean_t<Type>::payload_type;
    };

NS_END  // internal

template<typename Type>
struct arena_payload_type
    : internal::arena_payload_type_helper<Type>
{};

// arena_payload_type_t
//   type: the carrier of arena_payload_type -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using arena_payload_type_t =
    typename arena_payload_type<Type>::type;

// arena_link_policy_type
//   trait: SFINAE-safe extraction of link_policy from an arena container.
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct arena_link_policy_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct arena_link_policy_type_helper<Type,
        D_VOID_T<typename clean_t<Type>::link_policy>>
    {
        using type = typename clean_t<Type>::link_policy;
    };

NS_END  // internal

template<typename Type>
struct arena_link_policy_type
    : internal::arena_link_policy_type_helper<Type>
{};

// arena_link_policy_type_t
//   type: the carrier of arena_link_policy_type -- its `::type`, for use where
// a type rather than a value is wanted.
template<typename Type>
using arena_link_policy_type_t =
    typename arena_link_policy_type<Type>::type;


// ===========================================================================
// V.   Cross-Arena Referenceability
// ===========================================================================

// arenas_cross_referenceable
//   trait: two arenas can share stable_id cross-references when both are valid
// arenas. Payload types need not match.
template<typename A,
         typename B>
struct arenas_cross_referenceable
{
    static D_CONSTEXPR bool value =
        ( is_arena<A>::value &&
          is_arena<B>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename A,
             typename B>
    D_CONSTEXPR bool arenas_cross_referenceable_v =
        arenas_cross_referenceable<A, B>::value;
#endif


// ===========================================================================
// VI.  Combined Classification
// ===========================================================================

// arena_class
//   struct: aggregate classification of an arena type.
template<typename Type>
struct arena_class
{
    // identity
    static D_CONSTEXPR bool is_arena_type =
        is_arena<Type>::value;

    // payload
    using payload_type =
        arena_payload_type_t<Type>;

    static D_CONSTEXPR bool valid_payload =
        is_arena_payload<payload_type>::value;

    // link policy
    using policy_type =
        arena_link_policy_type_t<Type>;

    static D_CONSTEXPR bool has_policy =
        is_link_policy<policy_type>::value;

    // link capabilities (safe defaults when no policy)
    static D_CONSTEXPR bool has_first_child =
        has_link_flag<policy_type,
                      tree_link::first_child>::value;
    static D_CONSTEXPR bool has_next_sibling =
        has_link_flag<policy_type,
                      tree_link::next_sibling>::value;
    static D_CONSTEXPR bool has_parent =
        has_link_flag<policy_type,
                      tree_link::parent>::value;
    static D_CONSTEXPR bool has_prev_sibling =
        has_link_flag<policy_type,
                      tree_link::prev_sibling>::value;
    static D_CONSTEXPR bool has_last_child =
        has_link_flag<policy_type,
                      tree_link::last_child>::value;
    static D_CONSTEXPR bool has_left =
        has_link_flag<policy_type,
                      tree_link::left>::value;
    static D_CONSTEXPR bool has_right =
        has_link_flag<policy_type,
                      tree_link::right>::value;

    // derived capabilities
    static D_CONSTEXPR bool o1_detach =
        ( has_prev_sibling && has_next_sibling );
    static D_CONSTEXPR bool o1_append =
        ( has_first_child && has_last_child );
    static D_CONSTEXPR bool supports_parent_traversal =
        has_parent;
    static D_CONSTEXPR bool is_binary =
        ( has_left && has_right );
    static D_CONSTEXPR bool is_nary =
        ( has_first_child && has_next_sibling );
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARENA_ARENA_TRAITS_HPP
