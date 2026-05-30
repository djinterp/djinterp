/******************************************************************************
* djinterp [core]                                          file_tree_traits.hpp
*
* File Tree Traits:
*   Provides compile-time SFINAE-based structural detection for file-tree
* containers.  Determines whether a type exposes the file_tree access surface
* (scan / resolve / name / full_path / traversal) and what payload it carries,
* strictly through structural inspection - no tag types.
*
*   The traits answer concrete, falsifiable questions: "does it have a
* `scan(const char*)`?", "does it expose a `resolve(...)`?", "does it carry a
* `file_entry`-shaped payload (name_offset / name_length / type / size)?".
* Policy decisions are left to the consumer; classification is aggregated into
* file_tree_class at the bottom of the header.
*
* Detection axes:
*   - core alias detection      (node_type)
*   - scanning surface          (scan)
*   - resolution surface        (resolve)
*   - naming surface            (name / name_str / full_path)
*   - traversal surface         (visit_depth_first / visit_breadth_first)
*   - mutation surface          (add_child / clear)
*   - payload shape             (file_entry-like: offset/length/type/size)
*   - aggregate classification  (file_tree_class)
*
*
* path:      /inc/cpp/fs/file_tree_traits.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.22
******************************************************************************/

#ifndef DJINTERP_FS_FILE_TREE_TRAITS_
#define DJINTERP_FS_FILE_TREE_TRAITS_ 1

// only meaningful in C++ mode
#ifndef __cplusplus
    #error "file_tree_traits.hpp can only be used in C++ compilation mode"
#endif

// std
#include <cstddef>
#include <cstdint>
#include <type_traits>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP
NS_FS


// =========================================================================
// I.   CORE ALIAS DETECTION
// =========================================================================

// has_node_type
//   trait: evaluates to true if _Type exposes a nested `node_type`
// alias.
template<typename _Type,
         typename = void>
struct has_node_type : std::false_type
{};

template<typename _Type>
struct has_node_type<_Type,
    void_t<typename _Type::node_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_node_type_v
    //   variable template: value of has_node_type<_Type>.
    template<typename _Type>
    D_CONSTEXPR bool has_node_type_v = has_node_type<_Type>::value;
#endif


NS_INTERNAL

    // file_node_type_helper
    //   trait: safely extracts `node_type` or falls back to void.
    template<typename _Type,
             bool     _HasNode = has_node_type<_Type>::value>
    struct file_node_type_helper
    {
        using type = void;
    };

    template<typename _Type>
    struct file_node_type_helper<_Type, true>
    {
        using type = typename _Type::node_type;
    };

NS_END  // internal

// file_node_type_of_t
//   type: alias for the extracted `node_type` of _Type, or void
// if absent.
template<typename _Type>
using file_node_type_of_t =
    typename internal::file_node_type_helper<_Type>::type;


NS_INTERNAL

    // file_payload_type_helper
    //   trait: extracts the payload type from a node wrapper by
    // inspecting its `.data` member (arena_node stores the
    // file_entry there).  Falls back to the node type itself when
    // no `.data` member is present, so payloads that *are* the node
    // still classify.
    template<typename _Node,
             typename = void>
    struct file_payload_type_helper
    {
        using type = _Node;
    };

    template<typename _Node>
    struct file_payload_type_helper<_Node,
        void_t<decltype(std::declval<_Node>().data)>>
    {
        using type = typename std::decay<
            decltype(std::declval<_Node>().data)>::type;
    };

NS_END  // internal

// file_payload_type_of_t
//   type: alias for the payload carried by _Type's node_type.
template<typename _Type>
using file_payload_type_of_t =
    typename internal::file_payload_type_helper<
        file_node_type_of_t<_Type>>::type;


// =========================================================================
// II.  SCANNING SURFACE DETECTION
// =========================================================================

// has_scan_method
//   trait: evaluates to true if _Type exposes a `scan(const char*)`
// method - the defining capability of a populatable file tree.
template<typename _Type,
         typename = void>
struct has_scan_method : std::false_type
{};

template<typename _Type>
struct has_scan_method<_Type, void_t<decltype(
    std::declval<_Type&>().scan(std::declval<const char*>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_scan_method_v = has_scan_method<_Type>::value;
#endif


// =========================================================================
// III. RESOLUTION SURFACE DETECTION
// =========================================================================

// has_resolve_method
//   trait: evaluates to true if _Type exposes a
// `resolve(const char*)` method for path resolution.
template<typename _Type,
         typename = void>
struct has_resolve_method : std::false_type
{};

template<typename _Type>
struct has_resolve_method<_Type, void_t<decltype(
    std::declval<const _Type&>().resolve(std::declval<const char*>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_resolve_method_v = has_resolve_method<_Type>::value;
#endif


// =========================================================================
// IV.  NAMING SURFACE DETECTION
// =========================================================================

// has_name_method
//   trait: evaluates to true if _Type exposes a `name(node_id, ...)`
// accessor returning a pooled name pointer.  Probed via a single
// node_id-like argument (the length out-param defaults).
template<typename _Type,
         typename = void>
struct has_name_method : std::false_type
{};

template<typename _Type>
struct has_name_method<_Type, void_t<decltype(
    std::declval<const _Type&>().name(std::declval<node_id>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_name_method_v = has_name_method<_Type>::value;
#endif

// has_name_str_method
//   trait: evaluates to true if _Type exposes a
// `name_str(node_id)` accessor returning a std::string.
template<typename _Type,
         typename = void>
struct has_name_str_method : std::false_type
{};

template<typename _Type>
struct has_name_str_method<_Type, void_t<decltype(
    std::declval<const _Type&>().name_str(std::declval<node_id>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_name_str_method_v =
        has_name_str_method<_Type>::value;
#endif

// has_full_path_method
//   trait: evaluates to true if _Type exposes a
// `full_path(node_id, ...)` accessor reconstructing a path string.
template<typename _Type,
         typename = void>
struct has_full_path_method : std::false_type
{};

template<typename _Type>
struct has_full_path_method<_Type, void_t<decltype(
    std::declval<const _Type&>().full_path(std::declval<node_id>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_full_path_method_v =
        has_full_path_method<_Type>::value;
#endif


// =========================================================================
// V.   TRAVERSAL SURFACE DETECTION
// =========================================================================
// Traversal methods are templates on the visitor, so detection probes
// with a trivial no-op callable.

NS_INTERNAL

    // file_tree_noop_visitor
    //   helper: a callable matching the _fn(node_id, depth) shape
    // expected by visit_depth_first / visit_breadth_first.
    struct file_tree_noop_visitor
    {
        void operator()(node_id, std::size_t) const {}
    };

NS_END  // internal

// has_depth_first_method
//   trait: evaluates to true if _Type exposes a
// `visit_depth_first(node_id, fn)` traversal.
template<typename _Type,
         typename = void>
struct has_depth_first_method : std::false_type
{};

template<typename _Type>
struct has_depth_first_method<_Type, void_t<decltype(
    std::declval<const _Type&>().visit_depth_first(
        std::declval<node_id>(),
        std::declval<internal::file_tree_noop_visitor>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_depth_first_method_v =
        has_depth_first_method<_Type>::value;
#endif

// has_breadth_first_method
//   trait: evaluates to true if _Type exposes a
// `visit_breadth_first(node_id, fn)` traversal.
template<typename _Type,
         typename = void>
struct has_breadth_first_method : std::false_type
{};

template<typename _Type>
struct has_breadth_first_method<_Type, void_t<decltype(
    std::declval<const _Type&>().visit_breadth_first(
        std::declval<node_id>(),
        std::declval<internal::file_tree_noop_visitor>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_breadth_first_method_v =
        has_breadth_first_method<_Type>::value;
#endif


// =========================================================================
// VI.  MUTATION SURFACE DETECTION
// =========================================================================

// has_add_child_method
//   trait: evaluates to true if _Type exposes an
// `add_child(node_id, const char*, file_type)` mutator.
template<typename _Type,
         typename = void>
struct has_add_child_method : std::false_type
{};

template<typename _Type>
struct has_add_child_method<_Type, void_t<decltype(
    std::declval<_Type&>().add_child(
        std::declval<node_id>(),
        std::declval<const char*>(),
        std::declval<file_type>())
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_add_child_method_v =
        has_add_child_method<_Type>::value;
#endif

// has_clear_method
//   trait: evaluates to true if _Type exposes a `clear()` mutator.
template<typename _Type,
         typename = void>
struct has_clear_method : std::false_type
{};

template<typename _Type>
struct has_clear_method<_Type, void_t<decltype(
    std::declval<_Type&>().clear()
)>> : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Type>
    D_CONSTEXPR bool has_clear_method_v = has_clear_method<_Type>::value;
#endif


// =========================================================================
// VII. PAYLOAD SHAPE DETECTION
// =========================================================================
// A file_entry-shaped payload carries a pooled-name reference
// (offset + length), a typed kind, and a size.  These traits probe
// the payload type directly so that alternate trees carrying
// equivalently-shaped payloads classify correctly.

// payload_has_name_offset
//   trait: evaluates to true if _Payload has a `name_offset` member.
template<typename _Payload,
         typename = void>
struct payload_has_name_offset : std::false_type
{};

template<typename _Payload>
struct payload_has_name_offset<_Payload,
    void_t<decltype(std::declval<_Payload>().name_offset)>>
    : std::true_type
{};

// payload_has_name_length
//   trait: evaluates to true if _Payload has a `name_length` member.
template<typename _Payload,
         typename = void>
struct payload_has_name_length : std::false_type
{};

template<typename _Payload>
struct payload_has_name_length<_Payload,
    void_t<decltype(std::declval<_Payload>().name_length)>>
    : std::true_type
{};

// payload_has_type
//   trait: evaluates to true if _Payload has a `type` member.
template<typename _Payload,
         typename = void>
struct payload_has_type : std::false_type
{};

template<typename _Payload>
struct payload_has_type<_Payload,
    void_t<decltype(std::declval<_Payload>().type)>>
    : std::true_type
{};

// payload_has_size
//   trait: evaluates to true if _Payload has a `size` member.
template<typename _Payload,
         typename = void>
struct payload_has_size : std::false_type
{};

template<typename _Payload>
struct payload_has_size<_Payload,
    void_t<decltype(std::declval<_Payload>().size)>>
    : std::true_type
{};

// is_file_entry_payload
//   trait: evaluates to true if _Payload carries the full
// file_entry shape (pooled name + type + size).
template<typename _Payload>
struct is_file_entry_payload
{
    static constexpr bool value =
        ( payload_has_name_offset<_Payload>::value &&
          payload_has_name_length<_Payload>::value &&
          payload_has_type<_Payload>::value        &&
          payload_has_size<_Payload>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Payload>
    D_CONSTEXPR bool is_file_entry_payload_v =
        is_file_entry_payload<_Payload>::value;
#endif


// =========================================================================
// VIII. MASTER CLASSIFICATION STRUCT
// =========================================================================
// Note: uses ::value syntax for C++11 compatibility.  The _v variable
// templates above are a convenience layer that requires C++14.

// file_tree_class
//   struct: comprehensive aggregation of a file-tree container's
// detected capabilities.
template<typename _Type>
struct file_tree_class
{
    // -----------------------------------------------------------------
    // Core Identity
    // -----------------------------------------------------------------

    // is_file_tree
    //   the minimum structural surface: a node_type alias plus the
    // scan and resolve methods that define a populatable, queryable
    // file tree.
    static constexpr bool is_file_tree =
        ( has_node_type<_Type>::value    &&
          has_scan_method<_Type>::value  &&
          has_resolve_method<_Type>::value );

    // -----------------------------------------------------------------
    // Naming Surface
    // -----------------------------------------------------------------
    static constexpr bool is_named =
        ( has_name_method<_Type>::value     ||
          has_name_str_method<_Type>::value );

    static constexpr bool is_path_addressable =
        ( is_file_tree &&
          has_full_path_method<_Type>::value );

    // -----------------------------------------------------------------
    // Traversal Surface
    // -----------------------------------------------------------------
    static constexpr bool is_dfs_traversable =
        has_depth_first_method<_Type>::value;

    static constexpr bool is_bfs_traversable =
        has_breadth_first_method<_Type>::value;

    static constexpr bool is_traversable =
        ( is_dfs_traversable || is_bfs_traversable );

    // -----------------------------------------------------------------
    // Mutation Surface
    // -----------------------------------------------------------------
    static constexpr bool is_mutable =
        ( has_add_child_method<_Type>::value &&
          has_clear_method<_Type>::value );

    // -----------------------------------------------------------------
    // Payload Shape
    // -----------------------------------------------------------------
    static constexpr bool carries_file_entry =
        is_file_entry_payload<
            file_payload_type_of_t<_Type>>::value;
};


NS_END  // fs
NS_END  // djinterp


#endif  // DJINTERP_FS_FILE_TREE_TRAITS_
