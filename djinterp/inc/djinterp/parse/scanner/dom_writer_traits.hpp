/******************************************************************************
* djinterp [dom]                                        dom_writer_traits.hpp
*
* DOM writer SFINAE detection traits:
*   This header provides compile-time structural traits for detecting
* whether a given arena type conforms to the "sink" interface required
* by dom_writer.  Detection is purely structural — no tagging, no
* base-class checks, no RTTI.
*
* Traits provided:
*   - has_intern_method<T>                detects T::intern(const std::string&)
*   - has_resolve_method<T>               detects T::resolve(dom_string_id)
*   - has_allocate_method<T,P>            detects T::allocate(stable_id, P&&)
*   - has_append_child_method<T>          detects T::append_child(id, id)
*   - has_add_reference_method<T>         detects T::add_reference(from, to)
*   - is_string_sink<T>                   composite: has_intern_method
*   - is_tree_sink<T,P>                   composite: allocate + append_child
*   - is_xref_sink<T>                     composite: has_add_reference_method
*   - tree_node_id<T,P>                   extracts the node id type returned
*                                         by T::allocate (SFINAE-safe, void
*                                         on failure)
*
*
* path:      /inc/cpp/dom/dom_writer_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_DOM_WRITER_TRAITS_
#define DJINTERP_DOM_WRITER_TRAITS_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include "../core/djinterp.hpp"
#include "./dom_node.hpp"


// D_KEYWORD_DOM
//   keyword: resolves to `dom`.
#ifndef D_KEYWORD_DOM
    #define D_KEYWORD_DOM               dom
#endif

// NS_DOM
//   namespace: the DOM subsystem namespace.
#ifndef NS_DOM
    #define NS_DOM                      D_NAMESPACE(D_KEYWORD_DOM)
#endif


NS_DJINTERP
NS_DOM
NS_TRAITS


// ================================================================
//  has_intern_method
// ================================================================

NS_INTERNAL

    // has_intern_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_intern_method_helper : std::false_type
    {};

    // has_intern_method_helper (success case)
    //   trait: succeeds when _Type::intern(const std::string&)
    // is a well-formed expression.
    template<typename _Type>
    struct has_intern_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().intern(
                std::declval<const std::string&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_intern_method
//   trait: detects whether _Type has an `intern` member
// accepting `const std::string&`.
template<typename _Type>
struct has_intern_method
    : internal::has_intern_method_helper<_Type>
{};

// has_intern_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_intern_method_v =
        has_intern_method<_Type>::value;
#endif


// ================================================================
//  has_resolve_method
// ================================================================

NS_INTERNAL

    // has_resolve_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_resolve_method_helper : std::false_type
    {};

    // has_resolve_method_helper (success case)
    //   trait: succeeds when _Type::resolve(dom_string_id) is a
    // well-formed expression.
    template<typename _Type>
    struct has_resolve_method_helper<
        _Type,
        void_t<decltype(
            std::declval<const _Type&>().resolve(
                std::declval<dom_string_id>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_resolve_method
//   trait: detects whether _Type has a `resolve` member
// accepting `dom_string_id`.
template<typename _Type>
struct has_resolve_method
    : internal::has_resolve_method_helper<_Type>
{};

// has_resolve_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_resolve_method_v =
        has_resolve_method<_Type>::value;
#endif


// ================================================================
//  has_allocate_method
// ================================================================

NS_INTERNAL

    // has_allocate_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename _Payload,
             typename = void>
    struct has_allocate_method_helper : std::false_type
    {};

    // has_allocate_method_helper (success case)
    //   trait: succeeds when
    // _Type::allocate(uint64_t, _Payload&&) is a well-formed
    // expression.
    template<typename _Type,
             typename _Payload>
    struct has_allocate_method_helper<
        _Type,
        _Payload,
        void_t<decltype(
            std::declval<_Type>().allocate(
                std::declval<std::uint64_t>(),
                std::declval<_Payload&&>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_allocate_method
//   trait: detects whether _Type has an `allocate` member
// accepting `(uint64_t, _Payload&&)`.
template<typename _Type,
         typename _Payload>
struct has_allocate_method
    : internal::has_allocate_method_helper<_Type, _Payload>
{};

// has_allocate_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             typename _Payload>
    constexpr bool has_allocate_method_v =
        has_allocate_method<_Type, _Payload>::value;
#endif


// ================================================================
//  has_append_child_method
// ================================================================

NS_INTERNAL

    // node_id_probe
    //   trait: resolves to the type returned by _Type::allocate
    // for payload _Payload, or void on failure.  Used internally
    // to parameterize the append_child probe.
    template<typename _Type,
             typename _Payload,
             typename = void>
    struct node_id_probe
    {
        using type = void;
    };

    template<typename _Type,
             typename _Payload>
    struct node_id_probe<
        _Type,
        _Payload,
        void_t<decltype(
            std::declval<_Type>().allocate(
                std::declval<std::uint64_t>(),
                std::declval<_Payload&&>()
            )
        )>
    >
    {
        using type = decltype(
            std::declval<_Type>().allocate(
                std::declval<std::uint64_t>(),
                std::declval<_Payload&&>()
            )
        );
    };

    // has_append_child_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename _NodeId,
             typename = void>
    struct has_append_child_method_helper : std::false_type
    {};

    // has_append_child_method_helper (success case)
    //   trait: succeeds when _Type::append_child(_NodeId, _NodeId)
    // is a well-formed expression.
    template<typename _Type,
             typename _NodeId>
    struct has_append_child_method_helper<
        _Type,
        _NodeId,
        void_t<decltype(
            std::declval<_Type>().append_child(
                std::declval<_NodeId>(),
                std::declval<_NodeId>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_append_child_method
//   trait: detects whether _Type has an `append_child` member
// accepting two values of _NodeId.
template<typename _Type,
         typename _NodeId>
struct has_append_child_method
    : internal::has_append_child_method_helper<_Type, _NodeId>
{};

// has_append_child_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             typename _NodeId>
    constexpr bool has_append_child_method_v =
        has_append_child_method<_Type, _NodeId>::value;
#endif


// ================================================================
//  has_add_reference_method
// ================================================================

NS_INTERNAL

    // has_add_reference_method_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename = void>
    struct has_add_reference_method_helper : std::false_type
    {};

    // has_add_reference_method_helper (success case)
    //   trait: succeeds when
    // _Type::add_reference(uint64_t, uint64_t) is a well-formed
    // expression.
    template<typename _Type>
    struct has_add_reference_method_helper<
        _Type,
        void_t<decltype(
            std::declval<_Type>().add_reference(
                std::declval<std::uint64_t>(),
                std::declval<std::uint64_t>()
            )
        )>
    > : std::true_type
    {};

NS_END  // internal

// has_add_reference_method
//   trait: detects whether _Type has an `add_reference` member
// accepting two `uint64_t` values.
template<typename _Type>
struct has_add_reference_method
    : internal::has_add_reference_method_helper<_Type>
{};

// has_add_reference_method_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool has_add_reference_method_v =
        has_add_reference_method<_Type>::value;
#endif


// ================================================================
//  is_string_sink
// ================================================================

// is_string_sink
//   trait: composite — _Type is a valid string-table sink if it
// can intern a std::string.
template<typename _Type>
struct is_string_sink
    : std::integral_constant<bool,
                             has_intern_method<_Type>::value>
{};

// is_string_sink_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_string_sink_v = is_string_sink<_Type>::value;
#endif


// ================================================================
//  is_tree_sink
// ================================================================

NS_INTERNAL

    // is_tree_sink_helper
    //   trait: primary template (failure case).
    template<typename _Type,
             typename _Payload,
             bool     _HasAlloc = has_allocate_method<_Type, _Payload>::value,
             typename           = void>
    struct is_tree_sink_helper : std::false_type
    {};

    // is_tree_sink_helper (success case)
    //   trait: _Type is a tree sink if it supports allocate(...)
    // for _Payload AND append_child taking the node_id type that
    // allocate returns.
    template<typename _Type,
             typename _Payload>
    struct is_tree_sink_helper<
        _Type,
        _Payload,
        true,
        typename std::enable_if<
            has_append_child_method<
                _Type,
                typename node_id_probe<_Type, _Payload>::type
            >::value
        >::type
    > : std::true_type
    {};

NS_END  // internal

// is_tree_sink
//   trait: composite — _Type is a valid DOM tree sink for
// payload _Payload when it supports `allocate(stable_id,
// _Payload&&)` and `append_child(node_id, node_id)` on the
// returned id type.
template<typename _Type,
         typename _Payload>
struct is_tree_sink
    : internal::is_tree_sink_helper<_Type, _Payload>
{};

// is_tree_sink_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type,
             typename _Payload>
    constexpr bool is_tree_sink_v =
        is_tree_sink<_Type, _Payload>::value;
#endif


// ================================================================
//  is_xref_sink
// ================================================================

// is_xref_sink
//   trait: composite — _Type is a valid cross-reference sink if
// it can accept (from_stable_id, to_stable_id) pairs via
// add_reference.
template<typename _Type>
struct is_xref_sink
    : std::integral_constant<bool,
                             has_add_reference_method<_Type>::value>
{};

// is_xref_sink_v
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    constexpr bool is_xref_sink_v = is_xref_sink<_Type>::value;
#endif


// ================================================================
//  tree_node_id
// ================================================================

// tree_node_id
//   trait: SFINAE-safe extraction of the node id type returned
// by _Type::allocate(stable_id, _Payload&&).  Produces void if
// the allocate expression is not well-formed.
template<typename _Type,
         typename _Payload>
struct tree_node_id
{
    using type = typename internal::node_id_probe<_Type, _Payload>::type;
};

// tree_node_id_t
//   type: convenience alias for tree_node_id<_Type, _Payload>::type.
template<typename _Type,
         typename _Payload>
using tree_node_id_t = typename tree_node_id<_Type, _Payload>::type;


NS_END  // traits
NS_END  // dom
NS_END  // djinterp


#endif  // DJINTERP_DOM_WRITER_TRAITS_
