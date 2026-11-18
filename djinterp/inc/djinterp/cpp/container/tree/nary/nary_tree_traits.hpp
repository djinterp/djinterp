/*******************************************************************************
* djinterp [cpp]                                            nary_tree_traits.hpp
*
* N-ary Tree SFINAE Detection Traits:
*   Comprehensive compile-time structural detection and classification
* for any n-ary tree implementation, regardless of memory strategy,
* node representation, or linking mechanism.  Detection is purely
* structural - no tagging, no base-class checks, no template-argument
* introspection.
* COVERED IMPLEMENTATIONS:
*   arena_tree, arena, DTreeContainer, poly_tree, DLinkedNode
*   derivatives, DDynamicNode, raw user structs with parent/child
*   fields, std:: containers with tree adapters, Qt QObject trees,
*   and any structural conformant.
* DEPENDENCIES:
*   djinterp.hpp       - namespace macros, D_CONSTEXPR, std::void_t
*   type_traits.hpp    - clean_t, void_t
*   node_container_traits.hpp, node_common.hpp, container_traits.hpp,
*   member_types.hpp   - the probes this header shares with the other trees
*                        and containers, each defined once (decision L1):
*                        root(), has_root(), is_root, remove_subtree, the
*                        parent links, version(), allocator_type
*   node_traits.hpp    - node-level structural detection (optional,
*                        for node_type extraction and handle form)
* COMPAT:
*   C++11: all traits via struct::value
*   C++14: _v variable templates where feature-gated
*   C++17: if constexpr in strategy selection (consumer-side)
*   C++20: concept wrappers available behind feature gate
*
*
* path:      /inc/djinterp/cpp/container/tree/nary/nary_tree_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.08
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.    child access model detection
      ----------------------------

2.    container-level navigation detection
      ------------------------------------

3.    container-level mutation detection
      ----------------------------------

4.    identity and versioning detection
      ---------------------------------

5.    node reference form detection
      -----------------------------

6.    memory model detection
      ----------------------

7.    node type extraction
      --------------------

8.    child access strategy enum
      --------------------------

9.    node reference strategy enum
      ----------------------------

10.   memory strategy enum
      --------------------

11.   complexity characteristics
      --------------------------

12.   n-ary tree identity
      -------------------

13.   combined classification
      -----------------------

14.   C++20 concept wrappers (in nary_tree_concepts.hpp, W5)
      ------------------------------------------------------
*/

#ifndef DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_TRAITS_HPP
#define DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#ifndef __cplusplus
    #error "nary_tree_traits.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <memory>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"
#include "../../../meta/type_traits.hpp"
#include "../../../meta/member_types.hpp"          // has_allocator_type
#include "../../traits/container_traits.hpp"       // has_version_method
#include "../../traits/node_container_traits.hpp"  // the shared tree probes
#include "../../node/node_common.hpp"              // the parent links

// the D_ENV_* level macros come from djinterp.hpp, so this test follows it
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "nary_tree_traits.hpp requires C++11 or higher"
#endif


NS_DJINTERP


// ===========================================================================
// I.   Child Access Model Detection
// ===========================================================================
// Detects HOW a type exposes its children.  A type may
// satisfy multiple models simultaneously (e.g. an arena
// node with both first_child() and edges()).

// --- LCRS model: first_child + next_sibling ---

// has_first_child_access
//   trait: true if Type exposes first_child via field or method.
template<typename Type,
         typename = void>
struct has_first_child_field : std::false_type
{};

template<typename Type>
struct has_first_child_field<Type,
    std::void_t<decltype(std::declval<Type>().first_child)>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_first_child_method : std::false_type
{};

template<typename Type>
struct has_first_child_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().first_child())>>
    : std::true_type
{};

template<typename Type>
struct has_first_child_access
{
    static D_CONSTEXPR bool value =
        ( has_first_child_field<Type>::value ||
          has_first_child_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_first_child_access_v =
        has_first_child_access<Type>::value;
#endif

// has_next_sibling_access
//   trait: true if Type exposes next_sibling via field or method.
template<typename Type,
         typename = void>
struct has_next_sibling_field : std::false_type
{};

template<typename Type>
struct has_next_sibling_field<Type,
    std::void_t<decltype(std::declval<Type>().next_sibling)>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_next_sibling_method : std::false_type
{};

template<typename Type>
struct has_next_sibling_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().next_sibling())>>
    : std::true_type
{};

template<typename Type>
struct has_next_sibling_access
{
    static D_CONSTEXPR bool value =
        ( has_next_sibling_field<Type>::value ||
          has_next_sibling_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_next_sibling_access_v =
        has_next_sibling_access<Type>::value;
#endif

// has_prev_sibling_access
//   trait: true if Type exposes prev_sibling via field or method.
template<typename Type,
         typename = void>
struct has_prev_sibling_field : std::false_type
{};

template<typename Type>
struct has_prev_sibling_field<Type,
    std::void_t<decltype(std::declval<Type>().prev_sibling)>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_prev_sibling_method : std::false_type
{};

template<typename Type>
struct has_prev_sibling_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().prev_sibling())>>
    : std::true_type
{};

template<typename Type>
struct has_prev_sibling_access
{
    static D_CONSTEXPR bool value =
        ( has_prev_sibling_field<Type>::value ||
          has_prev_sibling_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_prev_sibling_access_v =
        has_prev_sibling_access<Type>::value;
#endif

// has_last_child_access
//   trait: true if Type exposes last_child via field or method.
template<typename Type,
         typename = void>
struct has_last_child_field : std::false_type
{};

template<typename Type>
struct has_last_child_field<Type,
    std::void_t<decltype(std::declval<Type>().last_child)>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_last_child_method : std::false_type
{};

template<typename Type>
struct has_last_child_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().last_child())>>
    : std::true_type
{};

template<typename Type>
struct has_last_child_access
{
    static D_CONSTEXPR bool value =
        ( has_last_child_field<Type>::value ||
          has_last_child_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_last_child_access_v =
        has_last_child_access<Type>::value;
#endif

// --- Container model: children() returning an iterable ---

// has_children_method
//   trait: true if Type has a children() method.
template<typename Type,
         typename = void>
struct has_children_method : std::false_type
{};

// has_children_method<Type, std::void_t<decltype( std::declval<const
// Type&>().children())>>
//   trait: the `std::void_t<decltype( std::declval<const
// Type&>().children())>` case; it reports true.
template<typename Type>
struct has_children_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().children())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_children_method_v =
        has_children_method<Type>::value;
#endif

// has_iterable_children
//   trait: true if children() returns something with begin()/end().
NS_INTERNAL

    template<typename Type,
             typename = void>
    struct iterable_children_check : std::false_type
    {};

    template<typename Type>
    struct iterable_children_check<Type,
        std::void_t<
            decltype(std::begin(
                std::declval<const Type&>().children())),
            decltype(std::end(
                std::declval<const Type&>().children()))
        >> : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_iterable_children
    : internal::iterable_children_check<Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_iterable_children_v =
        has_iterable_children<Type>::value;
#endif

// --- Edge model: edges() returning an array or container ---

// has_edges_method
//   trait: true if Type has an edges() method.
template<typename Type,
         typename = void>
struct has_edges_method : std::false_type
{};

// has_edges_method<Type, std::void_t<decltype( std::declval<const
// Type&>().edges())>>
//   trait: the `std::void_t<decltype( std::declval<const Type&>().edges())>`
// case; it reports true.
template<typename Type>
struct has_edges_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().edges())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_edges_method_v =
        has_edges_method<Type>::value;
#endif

// has_edge_count_method
//   trait: true if Type has edge_count().
template<typename Type,
         typename = void>
struct has_edge_count_method : std::false_type
{};

// has_edge_count_method<Type, std::void_t<decltype( std::declval<const
// Type&>().edge_count())>>
//   trait: the `std::void_t<decltype( std::declval<const
// Type&>().edge_count())>` case; it reports true.
template<typename Type>
struct has_edge_count_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().edge_count())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_edge_count_method_v =
        has_edge_count_method<Type>::value;
#endif


// ===========================================================================
// II.  Container-Level Navigation Detection
// ===========================================================================
// These probe the container (not the node) for navigation
// methods.  Some trees expose navigation at the container
// level (arena_tree::first_child(id)) while others expose
// it at the node level (node.first_child).

// root() and has_root() on the container are node_container_traits.hpp's
// has_root_method and has_has_root_method (decisions L1 and W5).

// has_parent_field
//   trait: true if Type has a `parent` data member: node_common.hpp's
// has_parent, under the name this header gave it.
template<typename Type>
struct has_parent_field : has_parent<Type>
{};

// The parent() method and the unified field-or-method probe are
// node_common.hpp's has_parent_method and has_parent_access (decision L1).
// This header asks them of a const tree -- has_parent_access<const Type&> --
// as its own copies did.

// has_depth_method
//   trait: detects depth() on the container or node.
template<typename Type,
         typename = void>
struct has_depth_method : std::false_type
{};

// has_depth_method<Type, std::void_t<decltype( std::declval<const
// Type&>().depth())>>
//   trait: the `std::void_t<decltype( std::declval<const Type&>().depth())>`
// case; it reports true.
template<typename Type>
struct has_depth_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().depth())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_depth_method_v =
        has_depth_method<Type>::value;
#endif

// has_child_count_method
//   trait: detects child_count() on the container or node.
template<typename Type,
         typename = void>
struct has_child_count_method : std::false_type
{};

// has_child_count_method<Type, std::void_t<decltype( std::declval<const
// Type&>().child_count())>>
//   trait: the `std::void_t<decltype( std::declval<const
// Type&>().child_count())>` case; it reports true.
template<typename Type>
struct has_child_count_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().child_count())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_child_count_method_v =
        has_child_count_method<Type>::value;
#endif

// has_is_leaf_method
//   trait: detects is_leaf() method.
template<typename Type,
         typename = void>
struct has_is_leaf_method : std::false_type
{};

// has_is_leaf_method<Type, std::void_t<decltype( std::declval<const
// Type&>().is_leaf())>>
//   trait: the `std::void_t<decltype( std::declval<const
// Type&>().is_leaf())>` case; it reports true.
template<typename Type>
struct has_is_leaf_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().is_leaf())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_is_leaf_method_v =
        has_is_leaf_method<Type>::value;
#endif

// is_root() is node_container_traits.hpp's has_is_root_method, asked of a
// const tree with no arguments: has_is_root_method<const Type> (decision L1).


// ===========================================================================
// III. Container-Level Mutation Detection
// ===========================================================================

// has_append_child_method
//   trait: detects append_child(...) on the container.
template<typename Type,
         typename = void>
struct has_append_child_method : std::false_type
{};

// has_append_child_method<Type, std::void_t<decltype(
// std::declval<Type&>().append_child( std::declval<decltype(
// std::declval<const Type&>().root())>(), std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().append_child(
// std::declval<decltype( std::declval<const Type&>().root())>(),
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_append_child_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().append_child(
            std::declval<decltype(
                std::declval<const Type&>().root())>(),
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_append_child_method_v =
        has_append_child_method<Type>::value;
#endif

template<typename Type,
         typename = void>
struct has_prepend_child_method : std::false_type
{};

// has_prepend_child_method<Type, std::void_t<decltype(
// std::declval<Type&>().prepend_child( std::declval<decltype(
// std::declval<const Type&>().root())>(), std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().prepend_child(
// std::declval<decltype( std::declval<const Type&>().root())>(),
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_prepend_child_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().prepend_child(
            std::declval<decltype(
                std::declval<const Type&>().root())>(),
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_prepend_child_method_v =
        has_prepend_child_method<Type>::value;
#endif

template<typename Type,
         typename = void>
struct has_detach_method : std::false_type
{};

// has_detach_method<Type, std::void_t<decltype(
// std::declval<Type&>().detach( std::declval<decltype( std::declval<const
// Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().detach(
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_detach_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().detach(
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_detach_method_v =
        has_detach_method<Type>::value;
#endif

template<typename Type,
         typename = void>
struct has_move_subtree_method : std::false_type
{};

// has_move_subtree_method<Type, std::void_t<decltype(
// std::declval<Type&>().move_subtree( std::declval<decltype(
// std::declval<const Type&>().root())>(), std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().move_subtree(
// std::declval<decltype( std::declval<const Type&>().root())>(),
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_move_subtree_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().move_subtree(
            std::declval<decltype(
                std::declval<const Type&>().root())>(),
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_move_subtree_method_v =
        has_move_subtree_method<Type>::value;
#endif

NS_INTERNAL

    // nary_has_remove_subtree
    //   trait: true if Type has root() and remove_subtree can be called on it
    // with what root() returns -- node_container_traits.hpp's
    // has_remove_subtree_method, given an n-ary tree's argument (decision
    // L1).
    template<typename Type,
             typename = void>
    struct nary_has_remove_subtree : std::false_type
    {};

    template<typename Type>
    struct nary_has_remove_subtree<Type,
        std::void_t<decltype(std::declval<const Type&>().root())>>
        : has_remove_subtree_method<Type,
              decltype(std::declval<const Type&>().root())>
    {};

NS_END  // internal

template<typename Type,
         typename = void>
struct has_insert_after_method : std::false_type
{};

// has_insert_after_method<Type, std::void_t<decltype(
// std::declval<Type&>().insert_after( std::declval<decltype(
// std::declval<const Type&>().root())>(), std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().insert_after(
// std::declval<decltype( std::declval<const Type&>().root())>(),
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_insert_after_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().insert_after(
            std::declval<decltype(
                std::declval<const Type&>().root())>(),
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_insert_after_method_v =
        has_insert_after_method<Type>::value;
#endif

template<typename Type,
         typename = void>
struct has_insert_before_method : std::false_type
{};

// has_insert_before_method<Type, std::void_t<decltype(
// std::declval<Type&>().insert_before( std::declval<decltype(
// std::declval<const Type&>().root())>(), std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().insert_before(
// std::declval<decltype( std::declval<const Type&>().root())>(),
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_insert_before_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().insert_before(
            std::declval<decltype(
                std::declval<const Type&>().root())>(),
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_insert_before_method_v =
        has_insert_before_method<Type>::value;
#endif


// ===========================================================================
// IV.  Identity and Versioning Detection
// ===========================================================================

// has_stable_id
//   trait: detects stable_id field or method.
template<typename Type,
         typename = void>
struct has_stable_id_field : std::false_type
{};

template<typename Type>
struct has_stable_id_field<Type,
    std::void_t<decltype(std::declval<const Type&>().stable_id)>>
    : std::true_type
{};

template<typename Type,
         typename = void>
struct has_stable_id_method : std::false_type
{};

template<typename Type>
struct has_stable_id_method<Type,
    std::void_t<decltype(
        std::declval<const Type&>().stable_id())>>
    : std::true_type
{};

template<typename Type>
struct has_stable_id
{
    static D_CONSTEXPR bool value =
        ( has_stable_id_field<Type>::value ||
          has_stable_id_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_stable_id_v =
        has_stable_id<Type>::value;
#endif

// has_version
//   trait: detects version field or method.
template<typename Type,
         typename = void>
struct has_version_field : std::false_type
{};

template<typename Type>
struct has_version_field<Type,
    std::void_t<decltype(std::declval<const Type&>().version)>>
    : std::true_type
{};

// version() is container_traits.hpp's has_version_method (decision L1).

template<typename Type>
struct has_version
{
    static D_CONSTEXPR bool value =
        ( has_version_field<Type>::value ||
          has_version_method<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_version_v =
        has_version<Type>::value;
#endif

template<typename Type,
         typename = void>
struct has_bump_version_method : std::false_type
{};

// has_bump_version_method<Type, std::void_t<decltype(
// std::declval<Type&>().bump_version( std::declval<decltype(
// std::declval<const Type&>().root())>()))>>
//   trait: the `std::void_t<decltype( std::declval<Type&>().bump_version(
// std::declval<decltype( std::declval<const Type&>().root())>()))>` case; it
// reports true.
template<typename Type>
struct has_bump_version_method<Type,
    std::void_t<decltype(
        std::declval<Type&>().bump_version(
            std::declval<decltype(
                std::declval<const Type&>().root())>()))>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_bump_version_method_v =
        has_bump_version_method<Type>::value;
#endif


// ===========================================================================
// V.   Node Reference Form Detection
// ===========================================================================
// Determines how the tree references its nodes.  Probes
// the return type of root() to infer the handle form.

NS_INTERNAL

    // root_return_type_helper
    //   trait: extracts the return type of root() when available, or produces
    // void.
    template<typename Type,
             typename = void>
    struct root_return_type_helper
    {
        using type = void;
    };

    // root_return_type_helper<Type, std::void_t<decltype( std::declval<const
    // Type&>().root())>>
    //   trait: the `std::void_t<decltype( std::declval<const
    // Type&>().root())>` case; it maps to `typename std::decay<
    // decltype(std::declval<const Type&>().root()) >::type`.
    template<typename Type>
    struct root_return_type_helper<Type,
        std::void_t<decltype(
            std::declval<const Type&>().root())>>
    {
        using type = typename std::decay<
            decltype(std::declval<const Type&>().root())
        >::type;
    };

NS_END  // internal

// root_handle_type
//   trait: the decayed return type of root(). This is the node handle form
// (pointer, index, smart pointer, etc.).
template<typename Type>
struct root_handle_type
{
    using type =
        typename internal::root_return_type_helper<Type>::type;
};

// root_handle_type_t
//   type: the carrier of root_handle_type -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using root_handle_type_t =
    typename root_handle_type<Type>::type;

// DUsesIndexHandles
//   trait: true if root() returns a non-bool integral type (index into an
// arena or flat array).
template<typename Type>
struct DUsesIndexHandles
{
    using handle = root_handle_type_t<Type>;

    static D_CONSTEXPR bool value =
        ( std::is_integral<handle>::value &&
          !std::is_same<handle, bool>::value &&
          !std::is_void<handle>::value );
};

// old name, until 0.9.0-rc.1
template<typename Type>
using uses_index_handles = DUsesIndexHandles<Type>;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool uses_index_handles_v =
        DUsesIndexHandles<Type>::value;
#endif

// DUsesPointerHandles
//   trait: true if root() returns a raw pointer.
template<typename Type>
struct DUsesPointerHandles
{
    using handle = root_handle_type_t<Type>;

    static D_CONSTEXPR bool value =
        ( std::is_pointer<handle>::value );
};

// old name, until 0.9.0-rc.1
template<typename Type>
using uses_pointer_handles = DUsesPointerHandles<Type>;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool uses_pointer_handles_v =
        DUsesPointerHandles<Type>::value;
#endif

// uses_smart_pointer_handles
//   trait: true if root() returns a smart pointer type.
NS_INTERNAL

    template<typename H> struct is_any_smart_ptr : std::false_type {};
    template<typename H, typename D>
    struct is_any_smart_ptr<std::unique_ptr<H, D>> : std::true_type {};
    template<typename H>
    struct is_any_smart_ptr<std::shared_ptr<H>> : std::true_type {};
    template<typename H>
    struct is_any_smart_ptr<std::weak_ptr<H>> : std::true_type {};

NS_END  // internal

template<typename Type>
struct DUsesSmartPointerHandles
{
    using handle = root_handle_type_t<Type>;

    static D_CONSTEXPR bool value =
        internal::is_any_smart_ptr<handle>::value;
};

// old name, until 0.9.0-rc.1
template<typename Type>
using uses_smart_pointer_handles = DUsesSmartPointerHandles<Type>;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool uses_smart_pointer_handles_v =
        DUsesSmartPointerHandles<Type>::value;
#endif


// ===========================================================================
// VI.  Memory Model Detection
// ===========================================================================
// Detects how the tree manages its backing storage.

// has_allocator_type
//   member_types.hpp's, which reads the alias of the cleaned type (decision
// L1).

// has_link_policy_type
//   trait: true if Type exposes a link_policy type alias.
template<typename Type,
         typename = void>
struct has_link_policy_type : std::false_type
{};

// has_link_policy_type<Type, std::void_t<typename Type::link_policy>>
//   trait: the `std::void_t<typename Type::link_policy>` case; it reports
// true.
template<typename Type>
struct has_link_policy_type<Type,
    std::void_t<typename Type::link_policy>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_link_policy_type_v =
        has_link_policy_type<Type>::value;
#endif

// has_payload_type
//   trait: true if Type exposes payload_type.
template<typename Type,
         typename = void>
struct has_payload_type : std::false_type
{};

// has_payload_type<Type, std::void_t<typename Type::payload_type>>
//   trait: the `std::void_t<typename Type::payload_type>` case; it reports
// true.
template<typename Type>
struct has_payload_type<Type,
    std::void_t<typename Type::payload_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_payload_type_v =
        has_payload_type<Type>::value;
#endif

// has_node_type_alias
//   trait: true if Type exposes node_type.
template<typename Type,
         typename = void>
struct has_node_type_alias : std::false_type
{};

// has_node_type_alias<Type, std::void_t<typename Type::node_type>>
//   trait: the `std::void_t<typename Type::node_type>` case; it reports true.
template<typename Type>
struct has_node_type_alias<Type,
    std::void_t<typename Type::node_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_node_type_alias_v =
        has_node_type_alias<Type>::value;
#endif

// is_arena_backed
//   trait: true if the type looks like an arena (exposes payload_type,
// link_policy, and index-based access).
template<typename Type>
struct is_arena_backed
{
    static D_CONSTEXPR bool value =
        ( has_payload_type<Type>::value    &&
          has_link_policy_type<Type>::value &&
          DUsesIndexHandles<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_arena_backed_v =
        is_arena_backed<Type>::value;
#endif

// has_resource_method
//   trait: detects pool allocators' resource() method on the allocator type.
NS_INTERNAL

    template<typename Type,
             bool HasAlloc = has_allocator_type<Type>::value,
             typename = void>
    struct pool_backed_check : std::false_type
    {};

    template<typename Type>
    struct pool_backed_check<Type, true,
        std::void_t<decltype(
            std::declval<
                const typename Type::allocator_type&
            >().resource())>>
        : std::true_type
    {};

NS_END  // internal

// is_pool_backed
//   trait: true if the type's allocator is pool-backed (exposes resource()
// method).
template<typename Type>
struct is_pool_backed
    : internal::pool_backed_check<Type>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_pool_backed_v =
        is_pool_backed<Type>::value;
#endif


// ===========================================================================
// VII. Node Type Extraction
// ===========================================================================

NS_INTERNAL

    template<typename Type,
             typename = void>
    struct safe_node_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct safe_node_type_helper<Type,
        std::void_t<typename Type::node_type>>
    {
        using type = typename Type::node_type;
    };

    template<typename Type,
             typename = void>
    struct safe_payload_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct safe_payload_type_helper<Type,
        std::void_t<typename Type::payload_type>>
    {
        using type = typename Type::payload_type;
    };

    template<typename Type,
             typename = void>
    struct safe_value_type_helper
    {
        using type = void;
    };

    template<typename Type>
    struct safe_value_type_helper<Type,
        std::void_t<typename Type::value_type>>
    {
        using type = typename Type::value_type;
    };

NS_END  // internal

// nary_tree_node_type
//   trait: SFINAE-safe extraction of the node type. Prefers node_type, falls
// back to void.
template<typename Type>
struct nary_tree_node_type
    : internal::safe_node_type_helper<Type>
{};

// nary_tree_node_type_t
//   type: the carrier of nary_tree_node_type -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using nary_tree_node_type_t =
    typename nary_tree_node_type<Type>::type;

// nary_tree_data_type
//   trait: SFINAE-safe extraction of the stored data type. Prefers
// payload_type, falls back to value_type, then void.
template<typename Type>
struct nary_tree_data_type
{
    using type = typename std::conditional<
        !std::is_void<
            typename internal::safe_payload_type_helper<Type>::type
        >::value,
        typename internal::safe_payload_type_helper<Type>::type,
        typename internal::safe_value_type_helper<Type>::type
    >::type;
};

// nary_tree_data_type_t
//   type: the carrier of nary_tree_data_type -- its `::type`, for use where a
// type rather than a value is wanted.
template<typename Type>
using nary_tree_data_type_t =
    typename nary_tree_data_type<Type>::type;


// ===========================================================================
// VIII. Child Access Strategy Enum
// ===========================================================================

// DNaryChildAccess
//   enum: compile-time classification of how a tree or node exposes its
// children.
enum class DNaryChildAccess
{
    // left-child/right-sibling encoding
    lcrs,

    // children() returning an iterable container
    container,

    // edges() returning an array or container
    edges,

    // left/right (binary - not n-ary, but classified for completeness and
    // mutual exclusion)
    binary,

    // multiple models detected - consumer should
    // inspect individual traits
    hybrid,

    // no child access detected
    none
};

// old name, until 0.9.0-rc.1
using nary_child_access = DNaryChildAccess;

NS_INTERNAL

    template<typename Type>
    struct child_access_strategy_impl
    {
        static D_CONSTEXPR bool has_lcrs =
            ( has_first_child_access<Type>::value &&
              has_next_sibling_access<Type>::value );

        static D_CONSTEXPR bool has_cont =
            has_iterable_children<Type>::value;

        static D_CONSTEXPR bool has_edge =
            has_edges_method<Type>::value;

        // binary: left + right, no LCRS
        static D_CONSTEXPR bool has_bin =
            ( !has_lcrs &&
              !has_cont &&
              !has_edge );
        // (binary detection deferred to node_traits for left/right; here we
        // only mark it if nothing else applies)

        static D_CONSTEXPR int count =
            (has_lcrs ? 1 : 0) +
            (has_cont ? 1 : 0) +
            (has_edge ? 1 : 0);

        static D_CONSTEXPR DNaryChildAccess value =
            (count > 1) ? DNaryChildAccess::hybrid
            : has_lcrs  ? DNaryChildAccess::lcrs
            : has_cont  ? DNaryChildAccess::container
            : has_edge  ? DNaryChildAccess::edges
                        : DNaryChildAccess::none;
    };

NS_END  // internal

template<typename Type>
struct child_access_strategy
{
    static D_CONSTEXPR DNaryChildAccess value =
        internal::child_access_strategy_impl<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR DNaryChildAccess child_access_strategy_v =
        child_access_strategy<Type>::value;
#endif


// ===========================================================================
// IX.  Node Reference Strategy Enum
// ===========================================================================

enum class DNaryHandleForm
{
    // integer index into a flat arena
    index,

    // raw pointer to heap-allocated node
    raw_pointer,

    // std::unique_ptr / shared_ptr / weak_ptr
    smart_pointer,

    // unknown or void
    unknown
};

// old name, until 0.9.0-rc.1
using nary_handle_form = DNaryHandleForm;

template<typename Type>
struct handle_form_strategy
{
    static D_CONSTEXPR DNaryHandleForm value =
        DUsesIndexHandles<Type>::value
            ? DNaryHandleForm::index
        : DUsesPointerHandles<Type>::value
            ? DNaryHandleForm::raw_pointer
        : DUsesSmartPointerHandles<Type>::value
            ? DNaryHandleForm::smart_pointer
            : DNaryHandleForm::unknown;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR DNaryHandleForm handle_form_strategy_v =
        handle_form_strategy<Type>::value;
#endif


// ===========================================================================
// X.   Memory Strategy Enum
// ===========================================================================

enum class DNaryMemoryModel
{
    // flat arena with index-based referencing
    arena,
    // pool_allocator-backed (chunked or contiguous)
    pool,
    // standard allocator (heap)
    standard,
    // no allocator detected (node-level only, or opaque)
    undetected
};

// old name, until 0.9.0-rc.1
using nary_memory_model = DNaryMemoryModel;

template<typename Type>
struct memory_model_strategy
{
    static D_CONSTEXPR DNaryMemoryModel value =
        is_arena_backed<Type>::value
            ? DNaryMemoryModel::arena
        : is_pool_backed<Type>::value
            ? DNaryMemoryModel::pool
        : has_allocator_type<Type>::value
            ? DNaryMemoryModel::standard
            : DNaryMemoryModel::undetected;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR DNaryMemoryModel memory_model_strategy_v =
        memory_model_strategy<Type>::value;
#endif


// ===========================================================================
// XI.  Complexity Characteristics
// ===========================================================================
// Infers O(1) vs O(k) complexity for common operations
// based on which links are structurally available.

// has_o1_append
//   trait: true if O(1) last-child append is structurally possible (last_child
// link or equivalent).
template<typename Type>
struct has_o1_append
{
    static D_CONSTEXPR bool value =
        ( has_last_child_access<Type>::value &&
          has_first_child_access<Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_o1_append_v =
        has_o1_append<Type>::value;
#endif

// has_o1_detach
//   trait: true if O(1) unlink is structurally possible (prev_sibling +
// next_sibling + parent).
template<typename Type>
struct has_o1_detach
{
    static D_CONSTEXPR bool value =
        ( has_prev_sibling_access<Type>::value &&
          has_next_sibling_access<Type>::value &&
          has_parent_access<const Type&>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_o1_detach_v =
        has_o1_detach<Type>::value;
#endif

// has_o1_sibling_insert
//   trait: true if O(1) sibling insertion is possible.
template<typename Type>
struct has_o1_sibling_insert
{
    static D_CONSTEXPR bool value =
        has_next_sibling_access<Type>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool has_o1_sibling_insert_v =
        has_o1_sibling_insert<Type>::value;
#endif


// ===========================================================================
// XII. N-ary Tree Identity
// ===========================================================================
// The top-level "is this an n-ary tree?" predicate.
// Intentionally broad: anything that has a root and a way
// to iterate children qualifies.

// is_nary_tree
//   trait: true if Type has root access and at least one child access model
// (LCRS, container, or edges) that is not purely binary.
template<typename Type>
struct is_nary_tree
{
    static D_CONSTEXPR DNaryChildAccess strategy =
        child_access_strategy<Type>::value;

    static D_CONSTEXPR bool value =
        ( has_root_method<Type>::value &&
          ( strategy == DNaryChildAccess::lcrs      ||
            strategy == DNaryChildAccess::container  ||
            strategy == DNaryChildAccess::edges      ||
            strategy == DNaryChildAccess::hybrid ) );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_nary_tree_v =
        is_nary_tree<Type>::value;
#endif

// is_nary_node
//   trait: true if Type is a node type (not container) that exhibits n-ary
// child access.
template<typename Type>
struct is_nary_tree_node
{
    static D_CONSTEXPR bool has_lcrs =
        ( has_first_child_access<Type>::value &&
          has_next_sibling_access<Type>::value );

    static D_CONSTEXPR bool has_cont =
        has_iterable_children<Type>::value;

    static D_CONSTEXPR bool has_edge =
        has_edges_method<Type>::value;

    static D_CONSTEXPR bool value =
        ( has_lcrs || has_cont || has_edge );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename Type>
    D_CONSTEXPR bool is_nary_tree_node_v =
        is_nary_tree_node<Type>::value;
#endif


// ===========================================================================
// XIII. Combined Classification
// ===========================================================================

// DNaryTreeClass
//   struct: comprehensive n-ary tree classification.
// Works on any tree implementation: arena_tree, poly_tree, DTreeContainer,
// pointer-based custom trees, etc.
template<typename Type>
struct DNaryTreeClass
{
    // -----------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool is_nary =
        is_nary_tree<Type>::value;

    // -----------------------------------------------------------------
    // Type Extraction
    // -----------------------------------------------------------------
    using node_type =
        nary_tree_node_type_t<Type>;
    using data_type =
        nary_tree_data_type_t<Type>;
    using handle_type =
        root_handle_type_t<Type>;

    // -----------------------------------------------------------------
    // Child Access Model
    // -----------------------------------------------------------------
    static D_CONSTEXPR DNaryChildAccess child_model =
        child_access_strategy<Type>::value;

    static D_CONSTEXPR bool has_lcrs =
        ( has_first_child_access<Type>::value &&
          has_next_sibling_access<Type>::value );
    static D_CONSTEXPR bool has_children_container =
        has_iterable_children<Type>::value;
    static D_CONSTEXPR bool has_edges =
        has_edges_method<Type>::value;

    // -----------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool has_root =
        has_root_method<Type>::value;
    static D_CONSTEXPR bool has_parent =
        has_parent_access<const Type&>::value;
    static D_CONSTEXPR bool has_next_sibling =
        has_next_sibling_access<Type>::value;
    static D_CONSTEXPR bool has_prev_sibling =
        has_prev_sibling_access<Type>::value;
    static D_CONSTEXPR bool has_first_child =
        has_first_child_access<Type>::value;
    static D_CONSTEXPR bool has_last_child =
        has_last_child_access<Type>::value;

    static D_CONSTEXPR bool bidirectional_siblings =
        ( has_next_sibling && has_prev_sibling );
    static D_CONSTEXPR bool fully_navigable =
        ( has_first_child && has_last_child &&
          has_next_sibling && has_prev_sibling &&
          has_parent );

    // -----------------------------------------------------------------
    // Handle Form
    // -----------------------------------------------------------------
    static D_CONSTEXPR DNaryHandleForm handle_form =
        handle_form_strategy<Type>::value;

    // -----------------------------------------------------------------
    // Memory Model
    // -----------------------------------------------------------------
    static D_CONSTEXPR DNaryMemoryModel memory =
        memory_model_strategy<Type>::value;

    static D_CONSTEXPR bool arena_backed =
        is_arena_backed<Type>::value;
    static D_CONSTEXPR bool pool_backed =
        is_pool_backed<Type>::value;
    static D_CONSTEXPR bool allocator_aware =
        has_allocator_type<Type>::value;

    // -----------------------------------------------------------------
    // Mutation Capabilities
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool can_append =
        has_append_child_method<Type>::value;
    static D_CONSTEXPR bool can_prepend =
        has_prepend_child_method<Type>::value;
    static D_CONSTEXPR bool can_insert_after =
        has_insert_after_method<Type>::value;
    static D_CONSTEXPR bool can_insert_before =
        has_insert_before_method<Type>::value;
    static D_CONSTEXPR bool can_detach =
        has_detach_method<Type>::value;
    static D_CONSTEXPR bool can_move_subtree =
        has_move_subtree_method<Type>::value;
    static D_CONSTEXPR bool can_remove_subtree =
        internal::nary_has_remove_subtree<Type>::value;

    // -----------------------------------------------------------------
    // Complexity Guarantees
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool o1_append =
        has_o1_append<Type>::value;
    static D_CONSTEXPR bool o1_detach =
        has_o1_detach<Type>::value;
    static D_CONSTEXPR bool o1_sibling_insert =
        has_o1_sibling_insert<Type>::value;

    // -----------------------------------------------------------------
    // Identity / Versioning
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool has_stable_identity =
        has_stable_id<Type>::value;
    static D_CONSTEXPR bool has_versioning =
        has_version<Type>::value;
    static D_CONSTEXPR bool has_change_tracking =
        ( has_stable_identity && has_versioning );

    // -----------------------------------------------------------------
    // Structural Queries
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool has_is_leaf = has_is_leaf_method<Type>::value;
    static D_CONSTEXPR bool has_is_root_check =
        has_is_root_method<const Type>::value;
    static D_CONSTEXPR bool has_child_count = has_child_count_method<Type>::value;
    static D_CONSTEXPR bool has_depth = has_depth_method<Type>::value;

    // -----------------------------------------------------------------
    // Policy Awareness
    // -----------------------------------------------------------------
    static D_CONSTEXPR bool policy_driven = has_link_policy_type<Type>::value;
};

// old name, until 0.9.0-rc.1
template<typename Type>
using nary_tree_class = DNaryTreeClass<Type>;


// ===========================================================================
// XIV. C++20 Concept Wrappers
// ===========================================================================
//   The seven wrappers that stood here were exact twins of
// nary_tree_concepts.hpp's nary_tree_container_type, nary_tree_node_like,
// rooted_nary_tree_type, mutable_nary_tree_type, fully_navigable_nary_tree,
// pool_backed_nary_tree and change_tracked_nary_tree (decision W5). Their old
// names are aliases there, until 0.9.0-rc.1.
//   (arena_nary_tree, an arena-backed n-ary tree, went earlier: arena_tree.hpp's
// arena_nary_tree names the arena tree type, and nary_tree_concepts.hpp's
// arena_backed_nary_tree is the same constraint (decision L1).)


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_TREE_NARY_NARY_TREE_TRAITS_HPP
