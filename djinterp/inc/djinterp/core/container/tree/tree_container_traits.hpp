/******************************************************************************
* djinterp [container]                               tree_container_traits.hpp
*
* Tree Container Traits:
* Provides compile-time SFINAE-based structural detection for tree containers.
* Determines tree topologies (binary, n-ary, parented) and operational
* capabilities (rebalancing, rotations, merging) strictly through structural
* inspection, avoiding tag types entirely.
*
* path:      /inc/container/tree_container_traits.hpp
* link(s):   TBA
* author(s): djinterp AI Agent                                date: 2026.03.31
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TREE_CONTAINER_TRAITS_
#define DJINTERP_CONTAINER_TREE_CONTAINER_TRAITS_ 1

#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../node/node_traits.hpp"


NS_DJINTERP

    // =========================================================================
    // I.   CORE ALIAS DETECTION
    // =========================================================================

    // has_node_type
    //   trait: evaluates to true if _Type has a nested `node_type` alias.
    template<typename _Type,
             typename = void>
    struct has_node_type : std::false_type
    {};

    template<typename _Type>
    struct has_node_type<_Type, D_VOID_T<typename _Type::node_type>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_node_type_v
    //   variable template: value of has_node_type<_Type>.
    template<typename _Type>
    D_CONSTEXPR bool has_node_type_v = has_node_type<_Type>::value;
#endif

    // has_depth_type
    //   trait: evaluates to true if _Type has a nested `depth_type` alias.
    template<typename _Type,
             typename = void>
    struct has_depth_type : std::false_type
    {};

    template<typename _Type>
    struct has_depth_type<_Type, D_VOID_T<typename _Type::depth_type>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_depth_type_v
    //   variable template: value of has_depth_type<_Type>.
    template<typename _Type>
    D_CONSTEXPR bool has_depth_type_v = has_depth_type<_Type>::value;
#endif

    // has_key_compare
    //   trait: evaluates to true if _Type has a nested `key_compare` alias.
    template<typename _Type,
             typename = void>
    struct has_key_compare : std::false_type
    {};

    template<typename _Type>
    struct has_key_compare<_Type, D_VOID_T<typename _Type::key_compare>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_key_compare_v
    //   variable template: value of has_key_compare<_Type>.
    template<typename _Type>
    D_CONSTEXPR bool has_key_compare_v = has_key_compare<_Type>::value;
#endif

    NS_INTERNAL

        // tree_node_type_helper
        //   trait: safely extracts `node_type` or falls back to `nonesuch`.
        template<typename _Type,
                 bool     _HasNode = has_node_type<_Type>::value>
        struct tree_node_type_helper
        {
            using type = djinterp::nonesuch;
        };

        template<typename _Type>
        struct tree_node_type_helper<_Type, true>
        {
            using type = typename _Type::node_type;
        };

    NS_END  // internal

    // tree_node_type_of_t
    //   type: alias for the extracted `node_type` of _Type.
    template<typename _Type>
    using tree_node_type_of_t = typename internal::tree_node_type_helper<_Type>::type;


    // =========================================================================
    // II.  STRUCTURAL ACCESSOR DETECTION
    // =========================================================================

    // has_root_method
    //   trait: evaluates to true if _Type has a `root()` method.
    template<typename _Type,
             typename = void>
    struct has_root_method : std::false_type
    {};

    template<typename _Type>
    struct has_root_method<_Type, D_VOID_T<decltype(std::declval<_Type>().root())>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_root_method_v = has_root_method<_Type>::value;
#endif

    // has_set_root_method
    //   trait: evaluates to true if _Type has a `set_root(...)` method.
    template<typename _Type,
             typename = void>
    struct has_set_root_method : std::false_type
    {};

    template<typename _Type>
    struct has_set_root_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().set_root(std::declval<tree_node_type_of_t<_Type>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_set_root_method_v = has_set_root_method<_Type>::value;
#endif

    // has_has_root_method
    //   trait: evaluates to true if _Type has a `has_root()` method.
    template<typename _Type,
             typename = void>
    struct has_has_root_method : std::false_type
    {};

    template<typename _Type>
    struct has_has_root_method<_Type, D_VOID_T<decltype(
        std::declval<const _Type>().has_root()
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_has_root_method_v = has_has_root_method<_Type>::value;
#endif


    // =========================================================================
    // III. OPERATIONAL DETECTION (MUTATORS & ADVANCED TOPOLOGY)
    // =========================================================================

    // has_rotate_left_method
    //   trait: evaluates to true if _Type structurally supports `rotate_left(node)`.
    template<typename _Type,
             typename = void>
    struct has_rotate_left_method : std::false_type
    {};

    template<typename _Type>
    struct has_rotate_left_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().rotate_left(std::declval<tree_node_type_of_t<_Type>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_rotate_left_method_v = has_rotate_left_method<_Type>::value;
#endif

    // has_rotate_right_method
    //   trait: evaluates to true if _Type structurally supports `rotate_right(node)`.
    template<typename _Type,
             typename = void>
    struct has_rotate_right_method : std::false_type
    {};

    template<typename _Type>
    struct has_rotate_right_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().rotate_right(std::declval<tree_node_type_of_t<_Type>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_rotate_right_method_v = has_rotate_right_method<_Type>::value;
#endif

    // has_rebalance_method
    //   trait: evaluates to true if _Type exposes a `rebalance(...)` method.
    template<typename _Type,
             typename = void>
    struct has_rebalance_method : std::false_type
    {};

    template<typename _Type>
    struct has_rebalance_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().rebalance(std::declval<tree_node_type_of_t<_Type>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_rebalance_method_v = has_rebalance_method<_Type>::value;
#endif

    // has_merge_method
    //   trait: evaluates to true if _Type supports merging with another tree.
    template<typename _Type,
             typename = void>
    struct has_merge_method : std::false_type
    {};

    template<typename _Type>
    struct has_merge_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().merge(std::declval<_Type&>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_merge_method_v = has_merge_method<_Type>::value;
#endif

    // has_split_method
    //   trait: evaluates to true if _Type supports splitting (e.g., treaps).
    template<typename _Type,
             typename = void>
    struct has_split_method : std::false_type
    {};

    template<typename _Type>
    struct has_split_method<_Type, D_VOID_T<decltype(
        std::declval<_Type>().split(std::declval<typename _Type::key_type>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_split_method_v = has_split_method<_Type>::value;
#endif


    // =========================================================================
    // IV.  TOPOLOGY CLASSIFICATION
    // =========================================================================
    // Defers to `node_traits.hpp` using the extracted `node_type`.

    // is_binary_tree
    //   trait: evaluates to true if _Type's node_type satisfies is_binary_node.
    template<typename _Type>
    struct is_binary_tree
    {
        static constexpr bool value =
            is_binary_node<tree_node_type_of_t<_Type>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_binary_tree_v = is_binary_tree<_Type>::value;
#endif

    // is_nary_tree
    //   trait: evaluates to true if _Type's node_type satisfies is_nary_node.
    template<typename _Type>
    struct is_nary_tree
    {
        static constexpr bool value =
            is_nary_node<tree_node_type_of_t<_Type>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_nary_tree_v = is_nary_tree<_Type>::value;
#endif

    // is_parented_tree
    //   trait: evaluates to true if _Type's node_type satisfies is_parented.
    template<typename _Type>
    struct is_parented_tree
    {
        static constexpr bool value =
            is_parented<tree_node_type_of_t<_Type>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool is_parented_tree_v = is_parented_tree<_Type>::value;
#endif


    // =========================================================================
    // V.   MASTER CLASSIFICATION STRUCT
    // =========================================================================
    // Note: Uses ::value syntax for C++11 compatibility. The _v variable
    // templates are a convenience layer that requires C++14.

    // tree_container_class
    //   struct: comprehensive aggregation of a tree container's capabilities.
    template<typename _Type>
    struct tree_container_class
    {
        // -----------------------------------------------------------------
        // Core Identity
        // -----------------------------------------------------------------
        static constexpr bool is_tree =
            ( has_node_type<_Type>::value &&
              has_root_method<_Type>::value );

        static constexpr bool is_search_tree =
            ( is_tree &&
              has_key_compare<_Type>::value );

        // -----------------------------------------------------------------
        // Structural Topology
        // -----------------------------------------------------------------
        static constexpr bool is_binary   = is_binary_tree<_Type>::value;
        static constexpr bool is_nary     = is_nary_tree<_Type>::value;
        static constexpr bool is_parented = is_parented_tree<_Type>::value;

        // -----------------------------------------------------------------
        // Operational Capabilities
        // -----------------------------------------------------------------
        static constexpr bool is_rotatable =
            ( has_rotate_left_method<_Type>::value &&
              has_rotate_right_method<_Type>::value );

        static constexpr bool is_self_balancing =
            ( is_tree &&
              has_rebalance_method<_Type>::value );

        static constexpr bool is_mergeable =
            ( is_tree &&
              has_merge_method<_Type>::value );

        static constexpr bool is_splittable =
            ( is_tree &&
              has_split_method<_Type>::value );
    };


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_CONTAINER_TRAITS_