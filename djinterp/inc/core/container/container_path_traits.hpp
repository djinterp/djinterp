/******************************************************************************
* djinterp [container]                               container_path_traits.hpp
*
* Path policy and capability traits for the djinterp container framework.
*   Detects whether a type satisfies the container_path_policy concept
* (const member functions for hierarchical navigation and typed
* component access), and classifies the path operations that the
* policy supports.
*
*   A container_path_policy provides six const member methods:
*
*     null_index()                            → index sentinel
*     is_null(index)                          → bool
*     parent(container, index)                → index
*     first_child(container, index)           → index
*     next_sibling(container, index)          → index
*     component(container, index)             → component_type
*
*   and three type aliases:
*
*     container_type    — the container being navigated
*     index_type        — the element address type
*     component_type    — the path component type
*
*   All detection is purely structural: no tag types are required.
* Policies declare capabilities through their public interface.
*
* DEPENDENCIES:
*   container_traits.hpp   — hierarchical container detection
*   type_traits.hpp        — clean_t, void_t, D_TYPE_TRAIT_TRUE
*
* TABLE OF CONTENTS
* =================
* I.      Policy Type Alias Detection
* II.     Policy Method Detection
* III.    Policy Concept Satisfaction
* IV.     Path Capability Classification
* V.      Convenience Predicates
* VI.     Combined Classification
*
*
* path:      /inc/cpp/container/meta/container_path_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2025.06.01
******************************************************************************/

#ifndef DJINTERP_CONTAINER_PATH_TRAITS_
#define DJINTERP_CONTAINER_PATH_TRAITS_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../type_traits.hpp"
#include "./container_traits.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// =============================================================================
// I.   Policy Type Alias Detection
// =============================================================================
// Detects whether a type exposes the container_type, index_type,
// and component_type aliases expected by the container_path_policy
// concept.

// has_path_container_type
//   type trait: true if the type has a container_type alias.
D_TYPE_TRAIT_TRUE(has_path_container_type,
    typename _Type::container_type)

// has_path_index_type
//   type trait: true if the type has an index_type alias.
D_TYPE_TRAIT_TRUE(has_path_index_type,
    typename _Type::index_type)

// has_path_component_type
//   type trait: true if the type has a component_type alias.
D_TYPE_TRAIT_TRUE(has_path_component_type,
    typename _Type::component_type)

// has_path_type_aliases
//   type trait: true if the type has all three required aliases:
// container_type, index_type, and component_type.
template<typename _Type>
struct has_path_type_aliases
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_path_container_type_v<clean_type> &&
          has_path_index_type_v<clean_type>     &&
          has_path_component_type_v<clean_type> );
};

template<typename _Type>
inline constexpr bool has_path_type_aliases_v =
    has_path_type_aliases<_Type>::value;


// =============================================================================
// II.  Policy Method Detection
// =============================================================================
// Detects each const member method required by the
// container_path_policy concept.  Each trait guards on
// the existence of the relevant type aliases before
// probing the method signature.

NS_INTERNAL

    // safe type extractors: yield void when the alias is
    // absent, avoiding hard errors during SFINAE probing.

    // safe_container_type
    //   trait: extracts container_type or yields void.
    template<typename _Type,
             typename = void>
    struct safe_container_type
    {
        using type = void;
    };

    template<typename _Type>
    struct safe_container_type<_Type,
        std::void_t<typename _Type::container_type>>
    {
        using type = typename _Type::container_type;
    };

    // safe_index_type
    //   trait: extracts index_type or yields void.
    template<typename _Type,
             typename = void>
    struct safe_index_type
    {
        using type = void;
    };

    template<typename _Type>
    struct safe_index_type<_Type,
        std::void_t<typename _Type::index_type>>
    {
        using type = typename _Type::index_type;
    };

    // --------------------------------------------------------
    //  per-method checks
    // --------------------------------------------------------
    // Each check is guarded by requiring the relevant type
    // aliases within the void_t expression, so it returns
    // false_type when the policy lacks type aliases.

    // has_null_index_check
    //   trait: detects null_index() const.
    template<typename _Policy,
             typename = void>
    struct has_null_index_check : std::false_type
    {};

    template<typename _Policy>
    struct has_null_index_check<_Policy,
        std::void_t<
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .null_index())
        >> : std::true_type
    {};

    // has_is_null_check
    //   trait: detects is_null(index_type) const.
    template<typename _Policy,
             typename = void>
    struct has_is_null_check : std::false_type
    {};

    template<typename _Policy>
    struct has_is_null_check<_Policy,
        std::void_t<
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .is_null(
                        std::declval<
                            typename _Policy::index_type
                        >()))
        >> : std::true_type
    {};

    // has_parent_check
    //   trait: detects parent(container, index) const.
    template<typename _Policy,
             typename = void>
    struct has_parent_check : std::false_type
    {};

    template<typename _Policy>
    struct has_parent_check<_Policy,
        std::void_t<
            typename _Policy::container_type,
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .parent(
                        std::declval<
                            const typename _Policy::
                                container_type&>(),
                        std::declval<
                            typename _Policy::
                                index_type>()))
        >> : std::true_type
    {};

    // has_first_child_check
    //   trait: detects first_child(container, index) const.
    template<typename _Policy,
             typename = void>
    struct has_first_child_check : std::false_type
    {};

    template<typename _Policy>
    struct has_first_child_check<_Policy,
        std::void_t<
            typename _Policy::container_type,
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .first_child(
                        std::declval<
                            const typename _Policy::
                                container_type&>(),
                        std::declval<
                            typename _Policy::
                                index_type>()))
        >> : std::true_type
    {};

    // has_next_sibling_check
    //   trait: detects next_sibling(container, index) const.
    template<typename _Policy,
             typename = void>
    struct has_next_sibling_check : std::false_type
    {};

    template<typename _Policy>
    struct has_next_sibling_check<_Policy,
        std::void_t<
            typename _Policy::container_type,
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .next_sibling(
                        std::declval<
                            const typename _Policy::
                                container_type&>(),
                        std::declval<
                            typename _Policy::
                                index_type>()))
        >> : std::true_type
    {};

    // has_component_check
    //   trait: detects component(container, index) const.
    template<typename _Policy,
             typename = void>
    struct has_component_check : std::false_type
    {};

    template<typename _Policy>
    struct has_component_check<_Policy,
        std::void_t<
            typename _Policy::container_type,
            typename _Policy::index_type,
            decltype(
                std::declval<const _Policy&>()
                    .component(
                        std::declval<
                            const typename _Policy::
                                container_type&>(),
                        std::declval<
                            typename _Policy::
                                index_type>()))
        >> : std::true_type
    {};

NS_END  // internal


// =============================================================================
// III. Policy Concept Satisfaction
// =============================================================================
// Aggregate traits that classify how much of the
// container_path_policy concept a type satisfies.

// has_path_null_index
//   type trait: true if the type has null_index() const.
template<typename _Type>
struct has_path_null_index
{
    static constexpr bool value =
        internal::has_null_index_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_null_index_v =
    has_path_null_index<_Type>::value;

// has_path_is_null
//   type trait: true if the type has is_null(index) const.
template<typename _Type>
struct has_path_is_null
{
    static constexpr bool value =
        internal::has_is_null_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_is_null_v =
    has_path_is_null<_Type>::value;

// has_path_parent
//   type trait: true if the type has parent(container,
// index) const.
template<typename _Type>
struct has_path_parent
{
    static constexpr bool value =
        internal::has_parent_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_parent_v =
    has_path_parent<_Type>::value;

// has_path_first_child
//   type trait: true if the type has first_child(container,
// index) const.
template<typename _Type>
struct has_path_first_child
{
    static constexpr bool value =
        internal::has_first_child_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_first_child_v =
    has_path_first_child<_Type>::value;

// has_path_next_sibling
//   type trait: true if the type has next_sibling(container,
// index) const.
template<typename _Type>
struct has_path_next_sibling
{
    static constexpr bool value =
        internal::has_next_sibling_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_next_sibling_v =
    has_path_next_sibling<_Type>::value;

// has_path_component
//   type trait: true if the type has component(container,
// index) const.
template<typename _Type>
struct has_path_component
{
    static constexpr bool value =
        internal::has_component_check<
            clean_t<_Type>>::value;
};

template<typename _Type>
inline constexpr bool has_path_component_v =
    has_path_component<_Type>::value;

// is_navigation_policy
//   type trait: true if the type provides all navigation
// primitives (parent, first_child, next_sibling, is_null,
// null_index) but not necessarily component.  Sufficient for
// depth, ancestors, lca, and is_ancestor operations.
template<typename _Type>
struct is_navigation_policy
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_path_null_index_v<clean_type>    &&
          has_path_is_null_v<clean_type>       &&
          has_path_parent_v<clean_type>        &&
          has_path_first_child_v<clean_type>   &&
          has_path_next_sibling_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_navigation_policy_v =
    is_navigation_policy<_Type>::value;

// is_component_policy
//   type trait: true if the policy provides navigation
// primitives plus the component accessor.  Sufficient for
// collect, resolve, and relative operations.
template<typename _Type>
struct is_component_policy
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( is_navigation_policy_v<clean_type> &&
          has_path_component_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_component_policy_v =
    is_component_policy<_Type>::value;

// is_path_policy
//   type trait: true if the type satisfies the full
// container_path_policy concept — all three type aliases
// plus all six const member methods.
template<typename _Type>
struct is_path_policy
{
    using clean_type = clean_t<_Type>;

    static constexpr bool value =
        ( has_path_type_aliases_v<clean_type> &&
          is_component_policy_v<clean_type> );
};

template<typename _Type>
inline constexpr bool is_path_policy_v =
    is_path_policy<_Type>::value;


// =============================================================================
// IV.  Path Capability Classification
// =============================================================================
// Classifies which container_path operations a policy
// supports based on its detected interface.
//
// Capability levels (each includes the previous):
//
//   ancestry    — parent + is_null + null_index
//                 enables: depth, ancestors, ancestor_chain,
//                          lca, is_ancestor
//
//   component   — ancestry + first_child + next_sibling
//                          + component
//                 enables: resolve, collect, relative
//
//   full        — component + type aliases
//                 enables: all operations with full type safety

// path_capability
//   enum: compile-time path operation support level.
enum class path_capability
{
    // no path operations available
    none,

    // parent chain traversal only — depth, ancestors,
    // lca, is_ancestor
    ancestry,

    // ancestry + component access — resolve, collect,
    // relative
    component,

    // full concept satisfaction — all type aliases present
    full
};

NS_INTERNAL

    template<typename _Type>
    struct path_capability_impl
    {
        using C = clean_t<_Type>;

        // does the policy support parent-chain walking?
        static constexpr bool has_ancestry =
            ( has_path_null_index_v<C> &&
              has_path_is_null_v<C>    &&
              has_path_parent_v<C> );

        // does the policy support component access?
        static constexpr bool has_components =
            ( has_ancestry &&
              has_path_first_child_v<C>   &&
              has_path_next_sibling_v<C>  &&
              has_path_component_v<C> );

        // does the policy have full type aliases?
        static constexpr bool has_aliases =
            ( has_components &&
              has_path_type_aliases_v<C> );

        static constexpr path_capability value =
            has_aliases
                ? path_capability::full

            : has_components
                ? path_capability::component

            : has_ancestry
                ? path_capability::ancestry

            : path_capability::none;
    };

NS_END  // internal

// policy_path_capability
//   type trait: classifies the path capability level of
// a policy type.
template<typename _Type>
struct policy_path_capability
{
    static constexpr path_capability value =
        internal::path_capability_impl<_Type>::value;
};

template<typename _Type>
inline constexpr path_capability
    policy_path_capability_v =
        policy_path_capability<_Type>::value;


// =============================================================================
// V.   Convenience Predicates
// =============================================================================
// Simple boolean traits for common path capability queries.

// can_path_resolve
//   type trait: true if the policy supports resolve() and
// collect().
template<typename _Type>
struct can_path_resolve
{
    static constexpr bool value =
        ( policy_path_capability_v<_Type> >=
          path_capability::component );
};

template<typename _Type>
inline constexpr bool can_path_resolve_v =
    can_path_resolve<_Type>::value;

// can_path_collect
//   type trait: true if the policy supports collect().
template<typename _Type>
struct can_path_collect
{
    static constexpr bool value =
        ( policy_path_capability_v<_Type> >=
          path_capability::component );
};

template<typename _Type>
inline constexpr bool can_path_collect_v =
    can_path_collect<_Type>::value;

// can_path_depth
//   type trait: true if the policy supports depth(),
// ancestors(), lca(), and is_ancestor().
template<typename _Type>
struct can_path_depth
{
    static constexpr bool value =
        ( policy_path_capability_v<_Type> >=
          path_capability::ancestry );
};

template<typename _Type>
inline constexpr bool can_path_depth_v =
    can_path_depth<_Type>::value;

// has_path_capability
//   type trait: true if the policy supports any path
// operations at all.
template<typename _Type>
struct has_path_capability
{
    static constexpr bool value =
        ( policy_path_capability_v<_Type> !=
          path_capability::none );
};

template<typename _Type>
inline constexpr bool has_path_capability_v =
    has_path_capability<_Type>::value;


// =============================================================================
// VI.  Combined Classification
// =============================================================================

// path_policy_class
//   struct: complete classification of a path policy type.
// All classification is compile-time using static constexpr
// members.
template<typename _Type>
struct path_policy_class
{
    // type aliases
    static constexpr bool has_container_type =
        has_path_container_type_v<_Type>;
    static constexpr bool has_index_type =
        has_path_index_type_v<_Type>;
    static constexpr bool has_component_type =
        has_path_component_type_v<_Type>;
    static constexpr bool has_type_aliases =
        has_path_type_aliases_v<_Type>;

    // per-method detection
    static constexpr bool has_null_index =
        has_path_null_index_v<_Type>;
    static constexpr bool has_is_null =
        has_path_is_null_v<_Type>;
    static constexpr bool has_parent =
        has_path_parent_v<_Type>;
    static constexpr bool has_first_child =
        has_path_first_child_v<_Type>;
    static constexpr bool has_next_sibling =
        has_path_next_sibling_v<_Type>;
    static constexpr bool has_component =
        has_path_component_v<_Type>;

    // concept satisfaction
    static constexpr bool is_navigation =
        is_navigation_policy_v<_Type>;
    static constexpr bool is_component =
        is_component_policy_v<_Type>;
    static constexpr bool is_full_policy =
        is_path_policy_v<_Type>;

    // capability level
    static constexpr path_capability capability =
        policy_path_capability_v<_Type>;

    // operation availability
    static constexpr bool can_resolve =
        can_path_resolve_v<_Type>;
    static constexpr bool can_collect =
        can_path_collect_v<_Type>;
    static constexpr bool can_depth =
        can_path_depth_v<_Type>;
};


NS_END  // traits
NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_PATH_TRAITS_
