/******************************************************************************
* djinterp [core]                                            node_traits.hpp
*
* Portable node structural traits:
* Provides compile-time SFINAE-based structural detection for node types.
* All traits take the node type itself as their primary template parameter,
* not the container. This answers concrete, falsifiable questions about a
* node's interface: "does it have a `next` member?", "does it have
* `left` and `right`?", "is it polymorphic?", and so on.
*
* No classification taxonomy or tag types are imposed. The traits are
* purely structural: they detect what exists and leave policy decisions
* to the consumer.
*
* NODE MEMBER DETECTION:
* - has_next<N>               — N has a `next` member
* - has_prev<N>               — N has a `prev` member
* - has_parent<N>             — N has a `parent` member
* - has_left<N>               — N has a `left` member
* - has_right<N>              — N has a `right` member
* - has_data<N>               — N has a `data` member
* - has_children_type<N>      — N has a `children_type` alias
* - has_adjacency_type<N>     — N has an `adjacency_type` alias
* - has_value_type<N>         — N has a `value_type` alias
* - has_key_type<N>           — N has a `key_type` alias
*
* NODE HANDLE FORM (classifies a node reference/handle type):
* - node_is_raw_pointer<H>    — H is _T*
* - node_is_unique_pointer<H> — H is std::unique_ptr<_T>
* - node_is_shared_pointer<H> — H is std::shared_ptr<_T>
* - node_is_weak_pointer<H>   — H is std::weak_ptr<_T>
* - node_is_smart_pointer<H>  — unique || shared || weak
* - node_is_any_pointer<H>    — raw || smart
* - node_is_index<H>          — H is integral (non-bool)
* - node_is_handle<H>         — H is a class with element_type
*
* COMPOSITE STRUCTURAL QUERIES:
* - is_singly_linked<N>          — has next, not prev
* - is_doubly_linked<N>          — has both next and prev
* - is_binary_node<N>            — has left and right
* - is_parented<N>               — has parent
* - is_parented_binary_node<N>   — has parent, left, and right
* - is_nary_node<N>              — has parent and children_type
* - is_graph_node<N>             — has adjacency_type
* - is_keyed_node<N>             — has key_type and data
* - is_leaf_capable<N>           — has data but no children links
*
* POLYMORPHISM:
* - node_is_polymorphic<N>       — N has at least one virtual method
* - node_derives_from<N, Base>   — N derives from Base
*
* SELF-REFERENTIAL:
* - node_is_self<N>             — N is the djinterp::self marker
*
* TOPOLOGY DETECTION:
* - has_edges<N>                — N has an `edges()` member
* - is_dynamic_node<N>          — N relies on dynamic containers for edges
* - is_heterogeneous_node<N>    — N exposes an `edge_tuple_type` alias
* - node_link_count<N>          — extracts compile-time link count
*
* NAMING CONVENTIONS:
* djinterp::traits::has_next<my_node>::value
* djinterp::traits::is_doubly_linked_v<my_node>
*
* path:      \inc\meta\node_traits.hpp
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.30
******************************************************************************/

#ifndef DJINTERP_NODE_TRAITS_
#define DJINTERP_NODE_TRAITS_ 1

// require env.h to be included first
#ifndef DJINTERP_ENVIRONMENT_
    #error "node_traits.hpp requires env.h to be included first"
#endif

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "node_traits.hpp can only be used in C++ compilation mode"
#endif

// requires C++11 or higher for type_traits, decltype, SFINAE
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "node_traits.hpp requires C++11 or higher"
#endif

#include <cstddef>
#include <type_traits>
#include <memory>
#include "../../djinterp.hpp"


NS_DJINTERP
NS_TRAITS


// =========================================================================
// I.   NODE MEMBER DETECTION
// =========================================================================
// Each trait probes for the existence of a specific member or nested
// type alias on the node type _N. Detection is purely syntactic; no
// semantic verification is performed. Every trait evaluates to
// std::true_type or std::false_type.

// has_next
//   trait: evaluates to true if _N has a `next` member (any type).
template<typename _N,
         typename = void>
struct has_next : std::false_type
{};

template<typename _N>
struct has_next<_N, std::void_t<decltype(std::declval<_N>().next)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_v
    //   variable template: value of has_next<_N>.
    template<typename _N>
    constexpr bool has_next_v = has_next<_N>::value;
#endif

// has_prev
//   trait: evaluates to true if _N has a `prev` member (any type).
template<typename _N,
         typename = void>
struct has_prev : std::false_type
{};

template<typename _N>
struct has_prev<_N,
    D_VOID_T<decltype(std::declval<_N>().prev)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_v
    //   variable template: value of has_prev<_N>.
    template<typename _N>
    constexpr bool has_prev_v = has_prev<_N>::value;
#endif

// has_parent
//   trait: evaluates to true if _N has a `parent` member
// (any type).
template<typename _N,
         typename = void>
struct has_parent : std::false_type
{};

template<typename _N>
struct has_parent<_N,
    D_VOID_T<decltype(std::declval<_N>().parent)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_v
    //   variable template: value of has_parent<_N>.
    template<typename _N>
    constexpr bool has_parent_v = has_parent<_N>::value;
#endif

// has_left
//   trait: evaluates to true if _N has a `left` member (any type).
template<typename _N,
         typename = void>
struct has_left : std::false_type
{};

template<typename _N>
struct has_left<_N,
    D_VOID_T<decltype(std::declval<_N>().left)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_v
    //   variable template: value of has_left<_N>.
    template<typename _N>
    constexpr bool has_left_v = has_left<_N>::value;
#endif

// has_right
//   trait: evaluates to true if _N has a `right` member (any type).
template<typename _N,
         typename = void>
struct has_right : std::false_type
{};

template<typename _N>
struct has_right<_N,
    D_VOID_T<decltype(std::declval<_N>().right)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_v
    //   variable template: value of has_right<_N>.
    template<typename _N>
    constexpr bool has_right_v = has_right<_N>::value;
#endif

// has_data
//   trait: evaluates to true if _N has a `data` member (any type).
template<typename _N,
         typename = void>
struct has_data : std::false_type
{};

template<typename _N>
struct has_data<_N,
    D_VOID_T<decltype(std::declval<_N>().data)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_data_v
    //   variable template: value of has_data<_N>.
    template<typename _N>
    constexpr bool has_data_v = has_data<_N>::value;
#endif

// has_children_type
//   trait: evaluates to true if _N has a nested `children_type`
// type alias.
template<typename _N,
         typename = void>
struct has_children_type : std::false_type
{};

template<typename _N>
struct has_children_type<_N,
    D_VOID_T<typename _N::children_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_children_type_v
    //   variable template: value of has_children_type<_N>.
    template<typename _N>
    constexpr bool has_children_type_v =
        has_children_type<_N>::value;
#endif

// has_adjacency_type
//   trait: evaluates to true if _N has a nested `adjacency_type`
// type alias.
template<typename _N,
         typename = void>
struct has_adjacency_type : std::false_type
{};

template<typename _N>
struct has_adjacency_type<_N,
    D_VOID_T<typename _N::adjacency_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_adjacency_type_v
    //   variable template: value of has_adjacency_type<_N>.
    template<typename _N>
    constexpr bool has_adjacency_type_v =
        has_adjacency_type<_N>::value;
#endif

// has_value_type
//   trait: evaluates to true if _N has a nested `value_type`
// type alias.
template<typename _N,
         typename = void>
struct has_value_type : std::false_type
{};

template<typename _N>
struct has_value_type<_N,
    D_VOID_T<typename _N::value_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_value_type_v
    //   variable template: value of has_value_type<_N>.
    template<typename _N>
    constexpr bool has_value_type_v = has_value_type<_N>::value;
#endif

// has_key_type
//   trait: evaluates to true if _N has a nested `key_type`
// type alias.
template<typename _N,
         typename = void>
struct has_key_type : std::false_type
{};

template<typename _N>
struct has_key_type<_N,
    D_VOID_T<typename _N::key_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_key_type_v
    //   variable template: value of has_key_type<_N>.
    template<typename _N>
    constexpr bool has_key_type_v = has_key_type<_N>::value;
#endif


// =========================================================================
// II.  NODE HANDLE FORM DETECTION
// =========================================================================
// Classifies the structural form of a type _H used as a node
// handle — the thing a container or another node holds to refer
// to a node. This is the type of the `next`, `left`, `parent`,
// etc. members, or the container's own `node_type` alias.
//
// The handle may be a raw pointer, smart pointer, integral index,
// or a user-defined handle class (e.g. a slot-map key).

NS_INTERNAL

    // is_unique_ptr
    //   trait: detects std::unique_ptr<...> (any deleter).
    template<typename _T>
    struct is_unique_ptr : std::false_type
    {
    };

    template<typename _T,
             typename _D>
    struct is_unique_ptr<std::unique_ptr<_T, _D>> : std::true_type
    {
    };

    // is_shared_ptr
    //   trait: detects std::shared_ptr<...>.
    template<typename _T>
    struct is_shared_ptr : std::false_type
    {
    };

    template<typename _T>
    struct is_shared_ptr<std::shared_ptr<_T>> : std::true_type
    {
    };

    // is_weak_ptr
    //   trait: detects std::weak_ptr<...>.
    template<typename _T>
    struct is_weak_ptr : std::false_type
    {
    };

    template<typename _T>
    struct is_weak_ptr<std::weak_ptr<_T>> : std::true_type
    {
    };

NS_END  // internal

// node_is_raw_pointer
//   trait: evaluates to true if _H is a raw pointer type.
template<typename _H>
struct node_is_raw_pointer
    : std::is_pointer<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_raw_pointer_v
    //   variable template: value of node_is_raw_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_raw_pointer_v =
        node_is_raw_pointer<_H>::value;
#endif

// node_is_unique_pointer
//   trait: evaluates to true if _H is a std::unique_ptr
// instantiation.
template<typename _H>
struct node_is_unique_pointer
    : internal::is_unique_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_unique_pointer_v
    //   variable template: value of node_is_unique_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_unique_pointer_v =
        node_is_unique_pointer<_H>::value;
#endif

// node_is_shared_pointer
//   trait: evaluates to true if _H is a std::shared_ptr
// instantiation.
template<typename _H>
struct node_is_shared_pointer
    : internal::is_shared_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_shared_pointer_v
    //   variable template: value of node_is_shared_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_shared_pointer_v =
        node_is_shared_pointer<_H>::value;
#endif

// node_is_weak_pointer
//   trait: evaluates to true if _H is a std::weak_ptr
// instantiation.
template<typename _H>
struct node_is_weak_pointer
    : internal::is_weak_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_weak_pointer_v
    //   variable template: value of node_is_weak_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_weak_pointer_v =
        node_is_weak_pointer<_H>::value;
#endif

// node_is_smart_pointer
//   trait: evaluates to true if _H is any of unique_ptr,
// shared_ptr, or weak_ptr.
template<typename _H>
struct node_is_smart_pointer
{
    static constexpr bool value =
        ( internal::is_unique_ptr<_H>::value ||
          internal::is_shared_ptr<_H>::value ||
          internal::is_weak_ptr<_H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_smart_pointer_v
    //   variable template: value of node_is_smart_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_smart_pointer_v =
        node_is_smart_pointer<_H>::value;
#endif

// node_is_any_pointer
//   trait: evaluates to true if _H is a raw pointer or any
// standard smart pointer.
template<typename _H>
struct node_is_any_pointer
{
    static constexpr bool value =
        ( std::is_pointer<_H>::value ||
          node_is_smart_pointer<_H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_any_pointer_v
    //   variable template: value of node_is_any_pointer<_H>.
    template<typename _H>
    constexpr bool node_is_any_pointer_v =
        node_is_any_pointer<_H>::value;
#endif

// node_is_index
//   trait: evaluates to true if _H is a non-bool integral type,
// indicating index-based node access into a backing store.
template<typename _H>
struct node_is_index
{
    static constexpr bool value =
        ( std::is_integral<_H>::value &&
          !std::is_same<_H, bool>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_index_v
    //   variable template: value of node_is_index<_H>.
    template<typename _H>
    constexpr bool node_is_index_v = node_is_index<_H>::value;
#endif

// node_is_handle
//   trait: evaluates to true if _H is a class type with an
// `element_type` member alias (the convention for smart-pointer-
// like handle types, e.g. slot-map keys, arena handles).
// Standard smart pointers also satisfy this, but are better
// identified by the specific traits above.
template<typename _H,
         typename = void>
struct node_is_handle : std::false_type
{};

template<typename _H>
struct node_is_handle<_H,
    D_VOID_T<typename _H::element_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_handle_v
    //   variable template: value of node_is_handle<_H>.
    template<typename _H>
    constexpr bool node_is_handle_v = node_is_handle<_H>::value;
#endif


// =========================================================================
// III. COMPOSITE STRUCTURAL QUERIES
// =========================================================================
// Higher-level traits combining multiple member detections to answer
// common structural questions about a node type. Conservative:
// require specific combinations and prefer false negatives over
// false positives.

// is_singly_linked
//   trait: evaluates to true if _N has `next` but not `prev`.
template<typename _N>
struct is_singly_linked
{
    static constexpr bool value =
        ( has_next<_N>::value &&
          !has_prev<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_singly_linked_v
    //   variable template: value of is_singly_linked<_N>.
    template<typename _N>
    constexpr bool is_singly_linked_v = is_singly_linked<_N>::value;
#endif

// is_doubly_linked
//   trait: evaluates to true if _N has both `next` and `prev`
// members.
template<typename _N>
struct is_doubly_linked
{
    static constexpr bool value =
        ( has_next<_N>::value &&
          has_prev<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_doubly_linked_v
    //   variable template: value of is_doubly_linked<_N>.
    template<typename _N>
    constexpr bool is_doubly_linked_v = is_doubly_linked<_N>::value;
#endif

// is_binary_node
//   trait: evaluates to true if _N has `left` and `right`
// members. Does not require `parent`.
template<typename _N>
struct is_binary_node
{
    static constexpr bool value =
        ( has_left<_N>::value &&
          has_right<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_binary_node_v
    //   variable template: value of is_binary_node<_N>.
    template<typename _N>
    constexpr bool is_binary_node_v = is_binary_node<_N>::value;
#endif

// is_parented
//   trait: evaluates to true if _N has a `parent` member.
template<typename _N>
struct is_parented
{
    static constexpr bool value = has_parent<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_v
    //   variable template: value of is_parented<_N>.
    template<typename _N>
    constexpr bool is_parented_v = is_parented<_N>::value;
#endif

// is_parented_binary_node
//   trait: evaluates to true if _N has `parent`, `left`, and
// `right` members.
template<typename _N>
struct is_parented_binary_node
{
    static constexpr bool value =
        ( is_binary_node<_N>::value &&
          has_parent<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_binary_node_v
    //   variable template: value of is_parented_binary_node<_N>.
    template<typename _N>
    constexpr bool is_parented_binary_node_v =
        is_parented_binary_node<_N>::value;
#endif

// is_nary_node
//   trait: evaluates to true if _N has a `parent` member and a
// `children_type` nested type alias.
template<typename _N>
struct is_nary_node
{
    static constexpr bool value =
        ( has_parent<_N>::value &&
          has_children_type<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nary_node_v
    //   variable template: value of is_nary_node<_N>.
    template<typename _N>
    constexpr bool is_nary_node_v = is_nary_node<_N>::value;
#endif

// is_graph_node
//   trait: evaluates to true if _N has an `adjacency_type`
// nested type alias.
template<typename _N>
struct is_graph_node
{
    static constexpr bool value = has_adjacency_type<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_graph_node_v
    //   variable template: value of is_graph_node<_N>.
    template<typename _N>
    constexpr bool is_graph_node_v = is_graph_node<_N>::value;
#endif

// is_keyed_node
//   trait: evaluates to true if _N has both a `key_type` alias
// and a `data` member. Common in associative-container nodes
// where the key and payload are separate concerns.
template<typename _N>
struct is_keyed_node
{
    static constexpr bool value =
        ( has_key_type<_N>::value &&
          has_data<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_keyed_node_v
    //   variable template: value of is_keyed_node<_N>.
    template<typename _N>
    constexpr bool is_keyed_node_v = is_keyed_node<_N>::value;
#endif

// is_leaf_capable
//   trait: evaluates to true if _N carries payload (has `data`
// or `value_type`) but has no child links (no `left`, `right`,
// `children_type`, or `adjacency_type`). Structural indicator
// that _N can only be a leaf node.
template<typename _N>
struct is_leaf_capable
{
    static constexpr bool value =
        ( ( has_data<_N>::value ||
            has_value_type<_N>::value )       &&
          !has_left<_N>::value                &&
          !has_right<_N>::value               &&
          !has_children_type<_N>::value       &&
          !has_adjacency_type<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_leaf_capable_v
    //   variable template: value of is_leaf_capable<_N>.
    template<typename _N>
    constexpr bool is_leaf_capable_v = is_leaf_capable<_N>::value;
#endif


// =========================================================================
// IV.  POLYMORPHISM DETECTION
// =========================================================================

// node_is_polymorphic
//   trait: evaluates to true if _N has at least one virtual
// method (i.e. std::is_polymorphic). Indicates the node
// participates in a type hierarchy where different concrete
// node types (e.g. leaf vs. interior) share a common base.
template<typename _N>
struct node_is_polymorphic
{
    static constexpr bool value = std::is_polymorphic<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_polymorphic_v
    //   variable template: value of node_is_polymorphic<_N>.
    template<typename _N>
    constexpr bool node_is_polymorphic_v =
        node_is_polymorphic<_N>::value;
#endif

// node_derives_from
//   trait: evaluates to true if _N derives from _Base.
// Two-parameter trait for verifying a node against a known
// base class (e.g. node_base, tree_node_base).
template<typename _N,
         typename _Base>
struct node_derives_from
{
    static constexpr bool value =
        ( std::is_base_of<_Base, _N>::value &&
          !std::is_same<_Base, _N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_derives_from_v
    //   variable template: value of node_derives_from<_N, _Base>.
    template<typename _N,
             typename _Base>
    constexpr bool node_derives_from_v =
        node_derives_from<_N, _Base>::value;
#endif


// =========================================================================
// V.   SELF-REFERENTIAL NODE DETECTION
// =========================================================================
// Integration with djinterp::self / djinterp::resolve_self.

// node_is_self
//   trait: evaluates to true if _N is the djinterp::self marker
// type, indicating the node type is a placeholder that resolves
// to the owning type via resolve_self.
template<typename _N>
struct node_is_self
{
    static constexpr bool value = djinterp::is_self<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_self_v
    //   variable template: value of node_is_self<_N>.
    template<typename _N>
    constexpr bool node_is_self_v = node_is_self<_N>::value;
#endif


// =========================================================================
// VI.  TOPOLOGY DETECTION
// =========================================================================
// Probes for node structural types (dynamic containers, heterogeneous
// tuples) to power constexpr traversal logic.

// has_edges
//   trait: evaluates to true if _N has an `edges()` member.
template<typename _N,
         typename = void>
struct has_edges : std::false_type
{};

template<typename _N>
struct has_edges<_N,
    D_VOID_T<decltype(std::declval<_N>().edges())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_edges_v
    //   variable template: value of has_edges<_N>.
    template<typename _N>
    constexpr bool has_edges_v = has_edges<_N>::value;
#endif

// is_dynamic_node
//   trait: evaluates to true if _N relies on dynamic containers for edges.
template<typename _N>
struct is_dynamic_node : has_edges<_N>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_dynamic_node_v
    //   variable template: value of is_dynamic_node<_N>.
    template<typename _N>
    constexpr bool is_dynamic_node_v = is_dynamic_node<_N>::value;
#endif

// is_heterogeneous_node
//   trait: evaluates to true if _N exposes an `edge_tuple_type` alias.
template<typename _N,
         typename = void>
struct is_heterogeneous_node : std::false_type
{};

template<typename _N>
struct is_heterogeneous_node<_N,
    D_VOID_T<typename _N::edge_tuple_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_heterogeneous_node_v
    //   variable template: value of is_heterogeneous_node<_N>.
    template<typename _N>
    constexpr bool is_heterogeneous_node_v =
        is_heterogeneous_node<_N>::value;
#endif

// dynamic_link_extent
//   constant: represents an unbounded/dynamic link count.
D_STATIC_CONSTEXPR std::size_t dynamic_link_extent = static_cast<std::size_t>(-1);

// node_link_count
//   trait: extracts the compile-time link count of _N.
// Resolves to dynamic_link_extent for dynamically sized nodes.
template<typename _N,
         typename = void>
struct node_link_count
    : std::integral_constant<std::size_t, dynamic_link_extent>
{};

template<typename _N>
struct node_link_count<_N,
    D_VOID_T<decltype(_N::num_links)>>
    : std::integral_constant<std::size_t, _N::num_links>
{};

template<typename _N>
struct node_link_count<_N,
    D_VOID_T<decltype(_N::edge_groups)>>
    : std::integral_constant<std::size_t, _N::edge_groups>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_link_count_v
    //   variable template: value of node_link_count<_N>.
    template<typename _N>
    constexpr std::size_t node_link_count_v = node_link_count<_N>::value;
#endif


NS_END  // traits
NS_END  // djinterp


#endif  // DJINTERP_NODE_TRAITS_