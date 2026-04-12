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
#include "../../meta/node_traits.hpp"


NS_DJINTERP
NS_TRAITS

    // =========================================================================
    // I.   CORE ALIAS DETECTION
    // =========================================================================

    // has_node_type
    //   trait: evaluates to true if _T has a nested `node_type` alias.
    template<typename _T,
             typename = void>
    struct has_node_type : std::false_type
    {};

    template<typename _T>
    struct has_node_type<_T, D_VOID_T<typename _T::node_type>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_node_type_v
    //   variable template: value of has_node_type<_T>.
    template<typename _T>
    D_CONSTEXPR bool has_node_type_v = has_node_type<_T>::value;
#endif

    // has_depth_type
    //   trait: evaluates to true if _T has a nested `depth_type` alias.
    template<typename _T,
             typename = void>
    struct has_depth_type : std::false_type
    {};

    template<typename _T>
    struct has_depth_type<_T, D_VOID_T<typename _T::depth_type>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_depth_type_v
    //   variable template: value of has_depth_type<_T>.
    template<typename _T>
    D_CONSTEXPR bool has_depth_type_v = has_depth_type<_T>::value;
#endif

    // has_key_compare
    //   trait: evaluates to true if _T has a nested `key_compare` alias.
    template<typename _T,
             typename = void>
    struct has_key_compare : std::false_type
    {};

    template<typename _T>
    struct has_key_compare<_T, D_VOID_T<typename _T::key_compare>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_key_compare_v
    //   variable template: value of has_key_compare<_T>.
    template<typename _T>
    D_CONSTEXPR bool has_key_compare_v = has_key_compare<_T>::value;
#endif

    NS_INTERNAL

        // tree_node_type_helper
        //   trait: safely extracts `node_type` or falls back to `nonesuch`.
        template<typename _T,
                 bool     _HasNode = has_node_type<_T>::value>
        struct tree_node_type_helper
        {
            using type = djinterp::traits::nonesuch;
        };

        template<typename _T>
        struct tree_node_type_helper<_T, true>
        {
            using type = typename _T::node_type;
        };

    NS_END  // internal

    // tree_node_type_of_t
    //   type: alias for the extracted `node_type` of _T.
    template<typename _T>
    using tree_node_type_of_t = typename internal::tree_node_type_helper<_T>::type;


    // =========================================================================
    // II.  STRUCTURAL ACCESSOR DETECTION
    // =========================================================================

    // has_root_method
    //   trait: evaluates to true if _T has a `root()` method.
    template<typename _T,
             typename = void>
    struct has_root_method : std::false_type
    {};

    template<typename _T>
    struct has_root_method<_T, D_VOID_T<decltype(std::declval<_T>().root())>>
        : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_root_method_v = has_root_method<_T>::value;
#endif

    // has_set_root_method
    //   trait: evaluates to true if _T has a `set_root(...)` method.
    template<typename _T,
             typename = void>
    struct has_set_root_method : std::false_type
    {};

    template<typename _T>
    struct has_set_root_method<_T, D_VOID_T<decltype(
        std::declval<_T>().set_root(std::declval<tree_node_type_of_t<_T>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_set_root_method_v = has_set_root_method<_T>::value;
#endif

    // has_has_root_method
    //   trait: evaluates to true if _T has a `has_root()` method.
    template<typename _T,
             typename = void>
    struct has_has_root_method : std::false_type
    {};

    template<typename _T>
    struct has_has_root_method<_T, D_VOID_T<decltype(
        std::declval<const _T>().has_root()
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_has_root_method_v = has_has_root_method<_T>::value;
#endif


    // =========================================================================
    // III. OPERATIONAL DETECTION (MUTATORS & ADVANCED TOPOLOGY)
    // =========================================================================

    // has_rotate_left_method
    //   trait: evaluates to true if _T structurally supports `rotate_left(node)`.
    template<typename _T,
             typename = void>
    struct has_rotate_left_method : std::false_type
    {};

    template<typename _T>
    struct has_rotate_left_method<_T, D_VOID_T<decltype(
        std::declval<_T>().rotate_left(std::declval<tree_node_type_of_t<_T>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_rotate_left_method_v = has_rotate_left_method<_T>::value;
#endif

    // has_rotate_right_method
    //   trait: evaluates to true if _T structurally supports `rotate_right(node)`.
    template<typename _T,
             typename = void>
    struct has_rotate_right_method : std::false_type
    {};

    template<typename _T>
    struct has_rotate_right_method<_T, D_VOID_T<decltype(
        std::declval<_T>().rotate_right(std::declval<tree_node_type_of_t<_T>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_rotate_right_method_v = has_rotate_right_method<_T>::value;
#endif

    // has_rebalance_method
    //   trait: evaluates to true if _T exposes a `rebalance(...)` method.
    template<typename _T,
             typename = void>
    struct has_rebalance_method : std::false_type
    {};

    template<typename _T>
    struct has_rebalance_method<_T, D_VOID_T<decltype(
        std::declval<_T>().rebalance(std::declval<tree_node_type_of_t<_T>*>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_rebalance_method_v = has_rebalance_method<_T>::value;
#endif

    // has_merge_method
    //   trait: evaluates to true if _T supports merging with another tree.
    template<typename _T,
             typename = void>
    struct has_merge_method : std::false_type
    {};

    template<typename _T>
    struct has_merge_method<_T, D_VOID_T<decltype(
        std::declval<_T>().merge(std::declval<_T&>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_merge_method_v = has_merge_method<_T>::value;
#endif

    // has_split_method
    //   trait: evaluates to true if _T supports splitting (e.g., treaps).
    template<typename _T,
             typename = void>
    struct has_split_method : std::false_type
    {};

    template<typename _T>
    struct has_split_method<_T, D_VOID_T<decltype(
        std::declval<_T>().split(std::declval<typename _T::key_type>())
    )>> : std::true_type
    {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool has_split_method_v = has_split_method<_T>::value;
#endif


    // =========================================================================
    // IV.  TOPOLOGY CLASSIFICATION
    // =========================================================================
    // Defers to `node_traits.hpp` using the extracted `node_type`.

    // is_binary_tree
    //   trait: evaluates to true if _T's node_type satisfies is_binary_node.
    template<typename _T>
    struct is_binary_tree
    {
        static constexpr bool value =
            is_binary_node<tree_node_type_of_t<_T>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool is_binary_tree_v = is_binary_tree<_T>::value;
#endif

    // is_nary_tree
    //   trait: evaluates to true if _T's node_type satisfies is_nary_node.
    template<typename _T>
    struct is_nary_tree
    {
        static constexpr bool value =
            is_nary_node<tree_node_type_of_t<_T>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool is_nary_tree_v = is_nary_tree<_T>::value;
#endif

    // is_parented_tree
    //   trait: evaluates to true if _T's node_type satisfies is_parented.
    template<typename _T>
    struct is_parented_tree
    {
        static constexpr bool value =
            is_parented<tree_node_type_of_t<_T>>::value;
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _T>
    D_CONSTEXPR bool is_parented_tree_v = is_parented_tree<_T>::value;
#endif


    // =========================================================================
    // V.   MASTER CLASSIFICATION STRUCT
    // =========================================================================
    // Note: Uses ::value syntax for C++11 compatibility. The _v variable
    // templates are a convenience layer that requires C++14.

    // tree_container_class
    //   struct: comprehensive aggregation of a tree container's capabilities.
    template<typename _T>
    struct tree_container_class
    {
        // -----------------------------------------------------------------
        // Core Identity
        // -----------------------------------------------------------------
        static constexpr bool is_tree =
            ( has_node_type<_T>::value &&
              has_root_method<_T>::value );

        static constexpr bool is_search_tree =
            ( is_tree &&
              has_key_compare<_T>::value );

        // -----------------------------------------------------------------
        // Structural Topology
        // -----------------------------------------------------------------
        static constexpr bool is_binary   = is_binary_tree<_T>::value;
        static constexpr bool is_nary     = is_nary_tree<_T>::value;
        static constexpr bool is_parented = is_parented_tree<_T>::value;

        // -----------------------------------------------------------------
        // Operational Capabilities
        // -----------------------------------------------------------------
        static constexpr bool is_rotatable =
            ( has_rotate_left_method<_T>::value &&
              has_rotate_right_method<_T>::value );

        static constexpr bool is_self_balancing =
            ( is_tree &&
              has_rebalance_method<_T>::value );

        static constexpr bool is_mergeable =
            ( is_tree &&
              has_merge_method<_T>::value );

        static constexpr bool is_splittable =
            ( is_tree &&
              has_split_method<_T>::value );
    };


NS_END  // traits
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_CONTAINER_TRAITS_
