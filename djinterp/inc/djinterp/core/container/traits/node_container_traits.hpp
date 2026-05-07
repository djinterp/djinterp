/******************************************************************************
* djinterp [container]                              node_container_traits.hpp
*
* Node Container Traits:
*   Compile-time SFINAE-based structural detection for node-based
* containers.  Detects ownership model, entry point form, node type
* extraction, and classifies the container's ownership strategy.
*
*   These traits operate on any type that structurally resembles a
* node_container - no base class check is performed.  A type is
* recognized as a node container if it exposes the structural
* minimum: node_type alias + entry_point() or root() or head().
*
*
* path:      /inc/djinterp/core/container/traits/node_container_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Ownership Policy Detection
II.   Entry Point Detection
III.  Node Type Extraction
IV.   Ownership Model Classification
V.    Node Container Identity
VI.   Combined Classification
VII.  C++20 Concepts (feature-gated)
*/

#ifndef DJINTERP_NODE_CONTAINER_TRAITS_
#define DJINTERP_NODE_CONTAINER_TRAITS_ 1

// std
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   Ownership Policy Detection
// ===========================================================================

// has_ownership_policy
//   trait: true if _Type exposes an `ownership_policy` alias.
D_TYPE_TRAIT_TRUE(has_ownership_policy,
                  typename _Type::ownership_policy)

// has_entry_storage_type
//   trait: true if _Type exposes an `entry_storage` alias.
D_TYPE_TRAIT_TRUE(has_entry_storage_type,
                  typename _Type::entry_storage)

// has_entry_owns_constant
//   trait: true if _Type exposes `entry_owns` as a static bool.
template<typename _Type,
         typename = void>
struct has_entry_owns_constant : std::false_type
{};

template<typename _Type>
struct has_entry_owns_constant<_Type,
    std::enable_if_t<std::is_same<
        decltype(_Type::entry_owns),
        const bool>::value>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_entry_owns_constant_v =
        has_entry_owns_constant<_Type>::value;
#endif


// ===========================================================================
// II.  Entry Point Detection
// ===========================================================================
//   Detects what kind of entry point(s) a node container exposes.
// A container may have root() (trees), head() (lists), or the
// generic entry_point() from node_container.

// has_entry_point_method
//   trait: true if _Type has an entry_point() method.
D_TYPE_TRAIT_TRUE(has_entry_point_method,
    decltype(std::declval<const _Type&>().entry_point()))

// has_root_method
//   trait: true if _Type has a root() method.
D_TYPE_TRAIT_TRUE(has_root_method,
    decltype(std::declval<const _Type&>().root()))

// has_has_root_method
//   trait: true if _Type has a has_root() method.
D_TYPE_TRAIT_TRUE(has_has_root_method,
    decltype(std::declval<const _Type&>().has_root()))

// has_head_method
//   trait: true if _Type has a head() method.
D_TYPE_TRAIT_TRUE(has_head_method,
    decltype(std::declval<const _Type&>().head()))

// has_tail_method
//   trait: true if _Type has a tail() method.
D_TYPE_TRAIT_TRUE(has_tail_method,
    decltype(std::declval<const _Type&>().tail()))

// has_has_entry_method
//   trait: true if _Type has a has_entry() method.
D_TYPE_TRAIT_TRUE(has_has_entry_method,
    decltype(std::declval<const _Type&>().has_entry()))

// has_release_entry_method
//   trait: true if _Type has a release_entry() method.
D_TYPE_TRAIT_TRUE(has_release_entry_method,
    decltype(std::declval<_Type&>().release_entry()))

// has_any_entry_point
//   trait: true if _Type exposes any kind of entry point.
template<typename _Type>
struct has_any_entry_point
{
    static constexpr bool value =
        ( has_entry_point_method<_Type>::value ||
          has_root_method<_Type>::value        ||
          has_head_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_any_entry_point_v =
        has_any_entry_point<_Type>::value;
#endif


// ===========================================================================
// III. Node Type Extraction
// ===========================================================================

// has_node_type
//   trait: true if _Type has a nested `node_type` alias.
D_TYPE_TRAIT_TRUE(has_node_type,
    typename _Type::node_type)

NS_INTERNAL
    // node_type_helper
    //   trait: safely extracts `node_type` or falls back to nonesuch.
    template<typename _Type,
             bool _Has = has_node_type<_Type>::value>
    struct node_type_helper
    {
        using type = nonesuch;
    };

    template<typename _Type>
    struct node_type_helper<_Type, true>
    {
        using type = typename _Type::node_type;
    };

NS_END  // internal

// node_type_of_t
//   type: the extracted node_type of _Type, or nonesuch.
template<typename _Type>
using node_type_of_t = typename internal::node_type_helper<_Type>::type;


// ===========================================================================
// IV.  Ownership Model Classification
// ===========================================================================

// DOwnershipModel
//   enum: classifies the ownership strategy of a node container.
enum class DOwnershipModel : std::uint8_t
{
    non_owning = 0,     // raw pointer entry, no destruction
    unique     = 1,     // unique_ptr entry, exclusive ownership
    shared     = 2,     // shared_ptr entry, reference-counted
    unknown    = 3      // has ownership_policy but unrecognized
};

NS_INTERNAL

    // Forward declarations of the policy types for identity checks.
    // These must match the types defined in node_container.hpp.

    // ownership_model_impl
    //   trait: classifies the ownership model by matching the
    // ownership_policy alias against known policy types.
    template<typename _Type,
             bool _Has = has_ownership_policy<_Type>::value>
    struct ownership_model_impl
    {
        static constexpr DOwnershipModel value =
            DOwnershipModel::non_owning;
    };

    template<typename _Type>
    struct ownership_model_impl<_Type, true>
    {
        using policy = typename _Type::ownership_policy;

        static constexpr DOwnershipModel value =
            std::is_same<policy,
                         unique_owning_policy
            >::value
                ? DOwnershipModel::unique

            : std::is_same<policy,
                           shared_owning_policy
              >::value
                ? DOwnershipModel::shared

            : std::is_same<policy,
                           non_owning_policy
              >::value
                ? DOwnershipModel::non_owning

            : DOwnershipModel::unknown;
    };

NS_END  // internal

// ownership_model_of
//   trait: classifies the ownership model of a node container.
template<typename _Type>
struct ownership_model_of
    : std::integral_constant<DOwnershipModel,
                              internal::ownership_model_impl<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr DOwnershipModel ownership_model_of_v =
        ownership_model_of<_Type>::value;
#endif

// is_owning_container
//   trait: true if _Type has an ownership policy where owns == true.
template<typename _Type,
         typename = void>
struct is_owning_container : std::false_type
{};

template<typename _Type>
struct is_owning_container<_Type,
    std::enable_if_t<
        has_ownership_policy<_Type>::value &&
        _Type::ownership_policy::owns
    >>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_owning_container_v =
        is_owning_container<_Type>::value;
#endif

// is_unique_owning_container
//   trait: true if _Type uses unique_owning_policy.
template<typename _Type>
struct is_unique_owning_container
{
    static constexpr bool value =
        ( ownership_model_of<_Type>::value ==
          DOwnershipModel::unique );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_unique_owning_container_v =
        is_unique_owning_container<_Type>::value;
#endif

// is_shared_owning_container
//   trait: true if _Type uses shared_owning_policy.
template<typename _Type>
struct is_shared_owning_container
{
    static constexpr bool value =
        ( ownership_model_of<_Type>::value ==
          DOwnershipModel::shared );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_shared_owning_container_v =
        is_shared_owning_container<_Type>::value;
#endif


// ===========================================================================
// V.   Node Container Identity
// ===========================================================================

// is_node_container
//   trait: true if _Type structurally resembles a node-based container.
// Requires: node_type alias AND at least one entry point method.
template<typename _Type>
struct is_node_container
{
    static constexpr bool value =
        ( has_node_type<_Type>::value &&
          has_any_entry_point<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_node_container_v =
        is_node_container<_Type>::value;
#endif

// is_tree_shaped_container
//   trait: true if _Type is a node container with a root entry point.
template<typename _Type>
struct is_tree_shaped_container
{
    static constexpr bool value =
        ( is_node_container<_Type>::value &&
          has_root_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_tree_shaped_container_v =
        is_tree_shaped_container<_Type>::value;
#endif

// is_list_shaped_container
//   trait: true if _Type is a node container with a head entry point.
template<typename _Type>
struct is_list_shaped_container
{
    static constexpr bool value =
        ( is_node_container<_Type>::value &&
          has_head_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_list_shaped_container_v =
        is_list_shaped_container<_Type>::value;
#endif

// is_doubly_linked_container
//   trait: true if _Type is list-shaped with both head() and tail().
template<typename _Type>
struct is_doubly_linked_container
{
    static constexpr bool value =
        ( is_list_shaped_container<_Type>::value &&
          has_tail_method<_Type>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_doubly_linked_container_v =
        is_doubly_linked_container<_Type>::value;
#endif


// ===========================================================================
// VI.  Combined Classification
// ===========================================================================

// DNodeContainerShape
//   enum: topology classification for strategy dispatch.
enum class DNodeContainerShape : std::uint8_t
{
    unknown   = 0,
    tree      = 1,      // single root entry
    list      = 2,      // head (+ optional tail) entry
    graph     = 3,      // entry set
    generic   = 4       // has entry_point() but no specific shape
};

NS_INTERNAL

    template<typename _Type>
    struct shape_impl
    {
        static constexpr DNodeContainerShape value =
            is_tree_shaped_container<_Type>::value
                ? DNodeContainerShape::tree

            : is_list_shaped_container<_Type>::value
                ? DNodeContainerShape::list

            : is_node_container<_Type>::value
                ? DNodeContainerShape::generic

            : DNodeContainerShape::unknown;
    };

NS_END  // internal

// node_container_shape_of
//   trait: classifies the topology of a node container.
template<typename _Type>
struct node_container_shape_of
    : std::integral_constant<DNodeContainerShape,
                              internal::shape_impl<_Type>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr DNodeContainerShape node_container_shape_of_v =
        node_container_shape_of<_Type>::value;
#endif

// node_container_class
//   struct: comprehensive classification of a node container.
template<typename _Type>
struct node_container_class
{
    // identity
    static constexpr bool is_node_container =
        ::djinterp::is_node_container<_Type>::value;

    // shape
    static constexpr DNodeContainerShape shape =
        node_container_shape_of<_Type>::value;
    static constexpr bool is_tree_shaped =
        is_tree_shaped_container<_Type>::value;
    static constexpr bool is_list_shaped =
        is_list_shaped_container<_Type>::value;
    static constexpr bool is_doubly_linked =
        is_doubly_linked_container<_Type>::value;

    // ownership
    static constexpr bool has_ownership =
        has_ownership_policy<_Type>::value;
    static constexpr DOwnershipModel ownership =
        ownership_model_of<_Type>::value;
    static constexpr bool is_owning =
        is_owning_container<_Type>::value;
    static constexpr bool is_unique_owning =
        is_unique_owning_container<_Type>::value;
    static constexpr bool is_shared_owning =
        is_shared_owning_container<_Type>::value;

    // entry point
    static constexpr bool has_entry =
        has_any_entry_point<_Type>::value;
    static constexpr bool has_root =
        has_root_method<_Type>::value;
    static constexpr bool has_head =
        has_head_method<_Type>::value;
    static constexpr bool has_tail =
        has_tail_method<_Type>::value;
    static constexpr bool has_release =
        has_release_entry_method<_Type>::value;

    // node type
    static constexpr bool has_node =
        has_node_type<_Type>::value;
    using node = node_type_of_t<_Type>;
};


NS_END  // djinterp


#endif  // DJINTERP_NODE_CONTAINER_TRAITS_