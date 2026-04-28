/******************************************************************************
* djinterp [dom]                                          cpp_dom_node.hpp
*
* C++ DOM Node Payload:
*   Extends dom_node with fields specific to C++ declarations: qualified
* names, return types, full signatures, mangled names, and parameter/
* template-parameter counts.
*
*   The structural relationships (parameters are children, base classes
* are children, template parameters are children) are expressed by tree
* topology, not vectors in the payload.  This keeps the node fixed-size
* and the arena happy.  The cached counts are for O(1) queries; if the
* tree is modified, they should be bumped.
*
* Dependencies:
*   dom_node.hpp   — base DOM node payload
*   lang/cpp.hpp   — C++-specific symbol_kind and qualifier extensions
*
*
* path:      /inc/cpp/dom/cpp_dom_node.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.16
******************************************************************************/

#ifndef DJINTERP_CPP_DOM_NODE_
#define DJINTERP_CPP_DOM_NODE_ 1

#include <cstdint>
#include "../core/djinterp.hpp"
#include "../lang/cpp.hpp"
#include "./dom_node.hpp"


NS_DJINTERP


// =============================================================================
// I.   cpp_dom_node
// =============================================================================

// cpp_dom_node
//   struct: C++ DOM node payload.  Inherits all common fields
// from dom_node and adds C++-specific identity and signature
// data.
//
//   Relationships that are variable-length (parameter lists,
// base class lists, template parameter lists) are represented
// as child nodes in the arena_tree — not as vectors here.  The
// uint16_t counts below are cached hints that the tree builder
// maintains; they are not authoritative (walk children to be
// certain).
struct cpp_dom_node : public dom_node
{
    // ---- C++ identity ----

    // qualified_name
    //   field: fully qualified name including namespaces and
    // enclosing classes (e.g. "ns::my_class::do_work").
    dom_string_id   qualified_name;

    // return_type
    //   field: return type spelling for functions / methods.
    // D_DOM_NULL_STRING for non-callable nodes.
    dom_string_id   return_type;

    // signature
    //   field: full declaration text (type + name + params)
    // suitable for display or diffing.
    dom_string_id   signature;

    // mangled_name
    //   field: Itanium / MSVC mangled linker name.
    // D_DOM_NULL_STRING if unavailable.
    dom_string_id   mangled_name;

    // underlying_type
    //   field: for typedefs and type aliases, the aliased type.
    // For enums, the underlying integer type.
    // D_DOM_NULL_STRING otherwise.
    dom_string_id   underlying_type;


    // ---- cached child counts (hints) ----

    // param_count
    //   field: number of parameter_decl children.
    std::uint16_t   param_count;

    // template_param_count
    //   field: number of template_param children.
    std::uint16_t   template_param_count;

    // base_count
    //   field: number of base_specifier children.
    std::uint16_t   base_count;

    // member_count
    //   field: number of direct member children (fields +
    // methods + nested types).
    std::uint16_t   member_count;


    // ============================================================
    //  construction
    // ============================================================

    cpp_dom_node()
        : dom_node()
        , qualified_name(D_DOM_NULL_STRING)
        , return_type(D_DOM_NULL_STRING)
        , signature(D_DOM_NULL_STRING)
        , mangled_name(D_DOM_NULL_STRING)
        , underlying_type(D_DOM_NULL_STRING)
        , param_count(0)
        , template_param_count(0)
        , base_count(0)
        , member_count(0)
    {
    }

    cpp_dom_node(std::uint16_t  _kind,
                 dom_string_id  _name)
        : dom_node(_kind, _name)
        , qualified_name(D_DOM_NULL_STRING)
        , return_type(D_DOM_NULL_STRING)
        , signature(D_DOM_NULL_STRING)
        , mangled_name(D_DOM_NULL_STRING)
        , underlying_type(D_DOM_NULL_STRING)
        , param_count(0)
        , template_param_count(0)
        , base_count(0)
        , member_count(0)
    {
    }

    cpp_dom_node(std::uint16_t  _kind,
                 dom_string_id  _name,
                 dom_string_id  _qualified_name)
        : dom_node(_kind, _name)
        , qualified_name(_qualified_name)
        , return_type(D_DOM_NULL_STRING)
        , signature(D_DOM_NULL_STRING)
        , mangled_name(D_DOM_NULL_STRING)
        , underlying_type(D_DOM_NULL_STRING)
        , param_count(0)
        , template_param_count(0)
        , base_count(0)
        , member_count(0)
    {
    }
};


NS_END  // djinterp


#endif  // DJINTERP_CPP_DOM_NODE_
