/*******************************************************************************
* djinterp [core]                                          arena_tree_traits.hpp
*
* Arena Tree SFINAE detection traits:
*   This header provides compile-time structural traits specific to
* arena_tree<> and any type that satisfies the arena tree protocol
* (an arena with a root() accessor).  Detection is purely structural.
*
* Traits provided:
*   TREE IDENTITY
*   - is_arena_tree<T>             does T satisfy the arena tree protocol?
*   - is_rooted_arena<T>           alias for is_arena_tree
*
*   ROOT DETECTION
*   - has_root_method<T>           does T expose root()?
*   - has_has_root_method<T>       does T expose has_root()?
*   - has_is_root_method<T>        does T expose is_root(node_id)?
*   - has_create_root_method<T>    does T expose create_root(...)?
*
*   TREE MUTATION DETECTION
*   - has_add_child_method<T>      does T expose add_child(...)?
*   - has_remove_subtree_method<T> does T expose remove_subtree(node_id)?
*
*   TREE NAVIGATION
*   - is_parent_navigable<T>       can walk from child to root?
*   - is_sibling_navigable<T>      can walk the sibling chain?
*   - is_fully_navigable<T>        all five n-ary navigations available?
*
*   COMBINED CLASSIFICATION
*   - arena_tree_class<T>          aggregate classification struct
*
*
* path:      /inc/djinterp/core/container/arena/tree/arena_tree_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.07
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_TRAITS_HPP
#define DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../../../../djinterp.hpp"
#include "../arena.hpp"
#include "../arena_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Root Method Detection
// ===========================================================================

// has_root_method
//   trait: detects a root() method returning node_id.
template<typename Type,
         typename = void>
struct has_root_method : std::false_type
{};

// has_root_method<Type, void_t<decltype(std::declval<const
// Type&>().root())>>
//   trait: the `void_t<decltype(std::declval<const Type&>().root())>` case;
// it reports true.
template<typename Type>
struct has_root_method<Type,
    void_t<decltype(std::declval<const Type&>().root())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_root_method_v =
        has_root_method<Type>::value;
#endif

// has_has_root_method
//   trait: detects has_root() returning bool.
template<typename Type,
         typename = void>
struct has_has_root_method : std::false_type
{};

// has_has_root_method<Type, void_t<decltype(std::declval<const
// Type&>().has_root())>>
//   trait: the `void_t<decltype(std::declval<const Type&>().has_root())>`
// case; it reports true.
template<typename Type>
struct has_has_root_method<Type,
    void_t<decltype(std::declval<const Type&>().has_root())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_has_root_method_v =
        has_has_root_method<Type>::value;
#endif

// has_is_root_method
//   trait: detects is_root(node_id) returning bool.
template<typename Type,
         typename = void>
struct has_is_root_method : std::false_type
{};

// has_is_root_method<Type, void_t<decltype( std::declval<const
// Type&>().is_root( std::declval<node_id>()))>>
//   trait: the `void_t<decltype( std::declval<const Type&>().is_root(
// std::declval<node_id>()))>` case; it reports true.
template<typename Type>
struct has_is_root_method<Type,
    void_t<decltype(
        std::declval<const Type&>().is_root(
            std::declval<node_id>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_is_root_method_v =
        has_is_root_method<Type>::value;
#endif

// has_set_root_method
//   trait: detects set_root(node_id).
template<typename Type,
         typename = void>
struct has_set_root_method : std::false_type
{};

// has_set_root_method<Type, void_t<decltype( std::declval<Type&>().set_root(
// std::declval<node_id>()))>>
//   trait: the `void_t<decltype( std::declval<Type&>().set_root(
// std::declval<node_id>()))>` case; it reports true.
template<typename Type>
struct has_set_root_method<Type,
    void_t<decltype(
        std::declval<Type&>().set_root(
            std::declval<node_id>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_set_root_method_v =
        has_set_root_method<Type>::value;
#endif


// ===========================================================================
// II.  Tree Mutation Detection
// ===========================================================================

// has_create_root_method
//   trait: detects create_root(Payload).
template<typename Type,
         typename = void>
struct has_create_root_method : std::false_type
{};

// has_create_root_method<Type, void_t<decltype(
// std::declval<Type&>().create_root( std::declval<typename clean_t<Type>
//   trait: the `void_t<decltype( std::declval<Type&>().create_root(
// std::declval<typename clean_t<Type` case; it reports true.
template<typename Type>
struct has_create_root_method<Type,
    void_t<decltype(
        std::declval<Type&>().create_root(
            std::declval<typename clean_t<Type>::payload_type>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_create_root_method_v =
        has_create_root_method<Type>::value;
#endif

// has_add_child_method
//   trait: detects add_child(node_id, Payload).
template<typename Type,
         typename = void>
struct has_add_child_method : std::false_type
{};

// has_add_child_method<Type, void_t<decltype(
// std::declval<Type&>().add_child( std::declval<node_id>(),
// std::declval<typename clean_t<Type>
//   trait: the `void_t<decltype( std::declval<Type&>().add_child(
// std::declval<node_id>(), std::declval<typename clean_t<Type` case; it
// reports true.
template<typename Type>
struct has_add_child_method<Type,
    void_t<decltype(
        std::declval<Type&>().add_child(
            std::declval<node_id>(),
            std::declval<typename clean_t<Type>::payload_type>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_add_child_method_v =
        has_add_child_method<Type>::value;
#endif

// has_remove_subtree_method
//   trait: detects remove_subtree(node_id).
template<typename Type,
         typename = void>
struct has_remove_subtree_method : std::false_type
{};

// has_remove_subtree_method<Type, void_t<decltype(
// std::declval<Type&>().remove_subtree( std::declval<node_id>()))>>
//   trait: the `void_t<decltype( std::declval<Type&>().remove_subtree(
// std::declval<node_id>()))>` case; it reports true.
template<typename Type>
struct has_remove_subtree_method<Type,
    void_t<decltype(
        std::declval<Type&>().remove_subtree(
            std::declval<node_id>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_remove_subtree_method_v =
        has_remove_subtree_method<Type>::value;
#endif


// ===========================================================================
// III. Arena Tree Identity
// ===========================================================================

// is_arena_tree
//   trait: detects whether Type satisfies the arena tree protocol - an arena
// with root ownership.
template<typename Type>
struct is_arena_tree
{
    static D_CONSTEXPR bool value =
        ( is_arena<Type>::value           &&
          has_root_method<Type>::value     &&
          has_has_root_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_arena_tree_v =
        is_arena_tree<Type>::value;
#endif

// is_rooted_arena
//   trait: alias for is_arena_tree.
template<typename Type>
struct is_rooted_arena : is_arena_tree<Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_rooted_arena_v =
        is_rooted_arena<Type>::value;
#endif


// ===========================================================================
// IV.  Navigation Classification
// ===========================================================================
// These traits inspect the link policy of an arena to
// determine navigational capabilities.

NS_INTERNAL

    // safe_link_policy
    //   helper: extracts link_policy from Type, or produces a zero-link
    // policy if not available.
    template<typename Type,
             typename = void>
    struct safe_link_policy
    {
        // stub policy - all flags false
        struct type
        {
            static D_CONSTEXPR unsigned flags      = 0;
            static D_CONSTEXPR std::size_t num_links = 0;
            static D_CONSTEXPR bool has_first_child  = false;
            static D_CONSTEXPR bool has_next_sibling = false;
            static D_CONSTEXPR bool has_parent       = false;
            static D_CONSTEXPR bool has_prev_sibling = false;
            static D_CONSTEXPR bool has_last_child   = false;
            static D_CONSTEXPR bool has_left         = false;
            static D_CONSTEXPR bool has_right        = false;
        };
    };

    // safe_link_policy<Type, void_t<typename clean_t<Type>
    //   trait: the `void_t<typename clean_t<Type` case; it maps to `typename
    // clean_t<Type>::link_policy`.
    template<typename Type>
    struct safe_link_policy<Type,
        void_t<typename clean_t<Type>::link_policy>>
    {
        using type = typename clean_t<Type>::link_policy;
    };

    template<typename Type>
    using safe_link_policy_t =
        typename safe_link_policy<Type>::type;

NS_END  // internal

// is_parent_navigable
//   trait: true if the arena supports child-to-root traversal (has parent
// link).
template<typename Type>
struct is_parent_navigable
{
    using policy = internal::safe_link_policy_t<Type>;

    static D_CONSTEXPR bool value = policy::has_parent;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_parent_navigable_v =
        is_parent_navigable<Type>::value;
#endif

// is_sibling_navigable
//   trait: true if the arena supports bidirectional sibling traversal (next +
// prev).
template<typename Type>
struct is_sibling_navigable
{
    using policy = internal::safe_link_policy_t<Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_next_sibling &&
          policy::has_prev_sibling );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_sibling_navigable_v =
        is_sibling_navigable<Type>::value;
#endif

// is_fully_navigable
//   trait: true if all five n-ary navigational links are present (first_child,
// last_child, next_sibling, prev_sibling, parent).
template<typename Type>
struct is_fully_navigable
{
    using policy = internal::safe_link_policy_t<Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_first_child  &&
          policy::has_last_child   &&
          policy::has_next_sibling &&
          policy::has_prev_sibling &&
          policy::has_parent );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_fully_navigable_v =
        is_fully_navigable<Type>::value;
#endif

// is_binary_arena
//   trait: true if the arena uses a binary link layout.
template<typename Type>
struct is_binary_arena
{
    using policy = internal::safe_link_policy_t<Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_left && policy::has_right );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_binary_arena_v =
        is_binary_arena<Type>::value;
#endif

// is_nary_arena
//   trait: true if the arena uses an n-ary (LCRS-family) link layout.
template<typename Type>
struct is_nary_arena
{
    using policy = internal::safe_link_policy_t<Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_first_child &&
          policy::has_next_sibling );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_nary_arena_v =
        is_nary_arena<Type>::value;
#endif


// ===========================================================================
// V.   Combined Classification
// ===========================================================================

// arena_tree_class
//   struct: comprehensive classification of an arena tree.
template<typename Type>
struct arena_tree_class
{
    using policy = internal::safe_link_policy_t<Type>;

    // -----------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool is_arena_type =
        is_arena<Type>::value;
    static D_CONSTEXPR bool is_tree =
        is_arena_tree<Type>::value;

    // -----------------------------------------------------------------
    // Topology
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool is_binary =
        is_binary_arena<Type>::value;
    static D_CONSTEXPR bool is_nary =
        is_nary_arena<Type>::value;

    // -----------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool parent_navigable =
        is_parent_navigable<Type>::value;
    static D_CONSTEXPR bool sibling_navigable =
        is_sibling_navigable<Type>::value;
    static D_CONSTEXPR bool fully_navigable =
        is_fully_navigable<Type>::value;

    // -----------------------------------------------------------------
    // Operations
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool has_create_root =
        has_create_root_method<Type>::value;
    static D_CONSTEXPR bool has_add_child =
        has_add_child_method<Type>::value;
    static D_CONSTEXPR bool has_remove_subtree =
        has_remove_subtree_method<Type>::value;

    // -----------------------------------------------------------------
    // Complexity Guarantees
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool o1_detach =
        ( policy::has_prev_sibling &&
          policy::has_next_sibling );
    static D_CONSTEXPR bool o1_append =
        ( policy::has_first_child &&
          policy::has_last_child );

    // -----------------------------------------------------------------
    // Link Budget
    // -----------------------------------------------------------------
    static D_CONSTEXPR std::size_t num_links =
        policy::num_links;
    static D_CONSTEXPR unsigned link_flags =
        policy::flags;
};


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_ARENA_TREE_ARENA_TREE_TRAITS_HPP
