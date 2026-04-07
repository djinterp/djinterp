/******************************************************************************
* djinterp [container]                                 arena_tree_traits.hpp
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
* path:      /inc/container/arena/arena_tree_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_ARENA_TREE_TRAITS_
#define DJINTERP_ARENA_TREE_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../../djinterp.hpp"
#include "./arena.hpp"
#include "./arena_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =============================================================================
// I.   Root Method Detection
// =============================================================================

// has_root_method
//   trait: detects a root() method returning node_id.
template<typename _Type,
         typename = void>
struct has_root_method : std::false_type
{
};

template<typename _Type>
struct has_root_method<_Type,
    D_VOID_T<decltype(std::declval<const _Type&>().root())>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_root_method_v =
        has_root_method<_Type>::value;
#endif

// has_has_root_method
//   trait: detects has_root() returning bool.
template<typename _Type,
         typename = void>
struct has_has_root_method : std::false_type
{
};

template<typename _Type>
struct has_has_root_method<_Type,
    D_VOID_T<decltype(std::declval<const _Type&>().has_root())>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_has_root_method_v =
        has_has_root_method<_Type>::value;
#endif

// has_is_root_method
//   trait: detects is_root(node_id) returning bool.
template<typename _Type,
         typename = void>
struct has_is_root_method : std::false_type
{
};

template<typename _Type>
struct has_is_root_method<_Type,
    D_VOID_T<decltype(
        std::declval<const _Type&>().is_root(
            std::declval<node_id>()))>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_is_root_method_v =
        has_is_root_method<_Type>::value;
#endif

// has_set_root_method
//   trait: detects set_root(node_id).
template<typename _Type,
         typename = void>
struct has_set_root_method : std::false_type
{
};

template<typename _Type>
struct has_set_root_method<_Type,
    D_VOID_T<decltype(
        std::declval<_Type&>().set_root(
            std::declval<node_id>()))>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_set_root_method_v =
        has_set_root_method<_Type>::value;
#endif


// =============================================================================
// II.  Tree Mutation Detection
// =============================================================================

// has_create_root_method
//   trait: detects create_root(Payload).
template<typename _Type,
         typename = void>
struct has_create_root_method : std::false_type
{
};

template<typename _Type>
struct has_create_root_method<_Type,
    D_VOID_T<decltype(
        std::declval<_Type&>().create_root(
            std::declval<typename _Type::payload_type>()))>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_create_root_method_v =
        has_create_root_method<_Type>::value;
#endif

// has_add_child_method
//   trait: detects add_child(node_id, Payload).
template<typename _Type,
         typename = void>
struct has_add_child_method : std::false_type
{
};

template<typename _Type>
struct has_add_child_method<_Type,
    D_VOID_T<decltype(
        std::declval<_Type&>().add_child(
            std::declval<node_id>(),
            std::declval<typename _Type::payload_type>()))>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_add_child_method_v =
        has_add_child_method<_Type>::value;
#endif

// has_remove_subtree_method
//   trait: detects remove_subtree(node_id).
template<typename _Type,
         typename = void>
struct has_remove_subtree_method : std::false_type
{
};

template<typename _Type>
struct has_remove_subtree_method<_Type,
    D_VOID_T<decltype(
        std::declval<_Type&>().remove_subtree(
            std::declval<node_id>()))>>
    : std::true_type
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_remove_subtree_method_v =
        has_remove_subtree_method<_Type>::value;
#endif


// =============================================================================
// III. Arena Tree Identity
// =============================================================================

// is_arena_tree
//   trait: detects whether _Type satisfies the arena tree
// protocol — an arena with root ownership.
template<typename _Type>
struct is_arena_tree
{
    static D_CONSTEXPR bool value =
        ( is_arena<_Type>::value           &&
          has_root_method<_Type>::value     &&
          has_has_root_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_arena_tree_v =
        is_arena_tree<_Type>::value;
#endif

// is_rooted_arena
//   trait: alias for is_arena_tree.
template<typename _Type>
struct is_rooted_arena : is_arena_tree<_Type>
{
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_rooted_arena_v =
        is_rooted_arena<_Type>::value;
#endif


// =============================================================================
// IV.  Navigation Classification
// =============================================================================
// These traits inspect the link policy of an arena to
// determine navigational capabilities.

NS_INTERNAL

    // safe_link_policy
    //   helper: extracts link_policy from _Type, or
    // produces a zero-link policy if not available.
    template<typename _Type,
             typename = void>
    struct safe_link_policy
    {
        // stub policy — all flags false
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

    template<typename _Type>
    struct safe_link_policy<_Type,
        D_VOID_T<typename _Type::link_policy>>
    {
        using type = typename _Type::link_policy;
    };

    template<typename _Type>
    using safe_link_policy_t =
        typename safe_link_policy<_Type>::type;

NS_END  // internal

// is_parent_navigable
//   trait: true if the arena supports child-to-root
// traversal (has parent link).
template<typename _Type>
struct is_parent_navigable
{
    using policy = internal::safe_link_policy_t<_Type>;

    static D_CONSTEXPR bool value = policy::has_parent;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_parent_navigable_v =
        is_parent_navigable<_Type>::value;
#endif

// is_sibling_navigable
//   trait: true if the arena supports bidirectional
// sibling traversal (next + prev).
template<typename _Type>
struct is_sibling_navigable
{
    using policy = internal::safe_link_policy_t<_Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_next_sibling &&
          policy::has_prev_sibling );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_sibling_navigable_v =
        is_sibling_navigable<_Type>::value;
#endif

// is_fully_navigable
//   trait: true if all five n-ary navigational links
// are present (first_child, last_child, next_sibling,
// prev_sibling, parent).
template<typename _Type>
struct is_fully_navigable
{
    using policy = internal::safe_link_policy_t<_Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_first_child  &&
          policy::has_last_child   &&
          policy::has_next_sibling &&
          policy::has_prev_sibling &&
          policy::has_parent );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_fully_navigable_v =
        is_fully_navigable<_Type>::value;
#endif

// is_binary_arena
//   trait: true if the arena uses a binary link layout.
template<typename _Type>
struct is_binary_arena
{
    using policy = internal::safe_link_policy_t<_Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_left && policy::has_right );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_binary_arena_v =
        is_binary_arena<_Type>::value;
#endif

// is_nary_arena
//   trait: true if the arena uses an n-ary (LCRS-family)
// link layout.
template<typename _Type>
struct is_nary_arena
{
    using policy = internal::safe_link_policy_t<_Type>;

    static D_CONSTEXPR bool value =
        ( policy::has_first_child &&
          policy::has_next_sibling );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_nary_arena_v =
        is_nary_arena<_Type>::value;
#endif


// =============================================================================
// V.   Combined Classification
// =============================================================================

// arena_tree_class
//   struct: comprehensive classification of an arena tree.
template<typename _Type>
struct arena_tree_class
{
    using policy = internal::safe_link_policy_t<_Type>;

    // -----------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool is_arena_type =
        is_arena<_Type>::value;
    static D_CONSTEXPR bool is_tree =
        is_arena_tree<_Type>::value;

    // -----------------------------------------------------------------
    // Topology
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool is_binary =
        is_binary_arena<_Type>::value;
    static D_CONSTEXPR bool is_nary =
        is_nary_arena<_Type>::value;

    // -----------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool parent_navigable =
        is_parent_navigable<_Type>::value;
    static D_CONSTEXPR bool sibling_navigable =
        is_sibling_navigable<_Type>::value;
    static D_CONSTEXPR bool fully_navigable =
        is_fully_navigable<_Type>::value;

    // -----------------------------------------------------------------
    // Operations
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool has_create_root =
        has_create_root_method<_Type>::value;
    static D_CONSTEXPR bool has_add_child =
        has_add_child_method<_Type>::value;
    static D_CONSTEXPR bool has_remove_subtree =
        has_remove_subtree_method<_Type>::value;

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


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARENA_TREE_TRAITS_
