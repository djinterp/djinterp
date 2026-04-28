/******************************************************************************
* djinterp [dom]                                              cpp_dom.hpp
*
* C++ DOM Tree:
*   This header defines the complete DOM tree system for representing
* parsed C++ source code as an arena-allocated tree of cpp_dom_node
* payloads.
*
* Architecture:
*
*   arena  -->  arena_tree  -->  dom_tree_base<Node>  -->  cpp_dom
*
*   dom_tree_base<Node> is a generic template that adds string interning,
* payload-aware accessors, source location tracking, and kind-filtered
* queries on top of arena_tree.  It works with any Node type that
* inherits from dom_node.
*
*   cpp_dom is the concrete C++ DOM tree.  It fixes the payload to
* cpp_dom_node and provides language-aware building methods (add_class,
* add_method, add_parameter, ...) that construct properly-typed nodes
* and maintain cached child counts.
*
* Typical usage:
*
*   cpp_dom dom;
*   node_id root = dom.add_translation_unit("main.cpp");
*   node_id ns   = dom.add_namespace(root, "my_lib");
*   node_id cls  = dom.add_class(ns, "widget");
*   node_id meth = dom.add_method(cls, "update", "void",
*                                 lang::qualifier::virtual_);
*   dom.add_parameter(meth, "dt", "float");
*
* Contents:
*   I.    dom_tree_base  (generic template)
*   II.   cpp_dom        (concrete C++ DOM)
*
* Dependencies:
*   container/arena/arena_tree.hpp  — arena-allocated tree
*   dom/dom_node.hpp                — base payload + string table
*   dom/cpp_dom_node.hpp            — C++ payload
*
*
* path:      /inc/cpp/dom/cpp_dom.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.16
******************************************************************************/

#ifndef DJINTERP_CPP_DOM_
#define DJINTERP_CPP_DOM_ 1

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/djinterp.hpp"
#include "../container/arena/arena_tree.hpp"
#include "./dom_node.hpp"
#include "./cpp_dom_node.hpp"


NS_DJINTERP


// =============================================================================
// I.   dom_tree_base
// =============================================================================

// dom_tree_base
//   class: generic DOM tree template over any dom_node-derived
// payload.  Provides string interning, payload accessors,
// source location helpers, and kind-filtered queries.
//
//   Inherits arena_tree, so all arena/arena_tree operations
// (allocate, add_child, collect_children, visit_depth_first,
// etc.) are available directly.
//
// Template parameters:
//   _DomNode    — payload type, must inherit from dom_node
//   _LinkPolicy — tree link topology (default: full n-ary)
//   _Allocator  — STL allocator for arena storage

template<typename _DomNode,
         typename _LinkPolicy = full_nary_link_policy,
         typename _Allocator  = std::allocator<
             arena_node<_DomNode, _LinkPolicy>>>
class dom_tree_base : public arena_tree<_DomNode,
                                         _LinkPolicy,
                                         _Allocator>
{
private:
    using base_type = arena_tree<_DomNode, _LinkPolicy, _Allocator>;

public:
    using typename base_type::payload_type;
    using typename base_type::node_type;
    using typename base_type::size_type;


    // =================================================================
    //  constructors
    // =================================================================

    dom_tree_base()
        : base_type()
        , m_strings()
    {
    }

    explicit
    dom_tree_base(size_type _reserve)
        : base_type(_reserve)
        , m_strings()
    {
    }


    // =================================================================
    //  string table access
    // =================================================================

    // strings
    //   method: returns the string interning table.
    dom_string_table&
    strings()
    {
        return m_strings;
    }

    const dom_string_table&
    strings() const
    {
        return m_strings;
    }

    // intern
    //   method: convenience wrapper for string interning.
    dom_string_id
    intern(const std::string& _str)
    {
        return m_strings.intern(_str);
    }

    // resolve
    //   method: convenience wrapper for string lookup.
    const std::string&
    resolve(dom_string_id _id) const
    {
        return m_strings.resolve(_id);
    }


    // =================================================================
    //  payload accessors
    // =================================================================

    // payload
    //   method: returns a mutable reference to the payload at _id.
    _DomNode&
    payload(node_id _id)
    {
        return base_type::data(_id);
    }

    const _DomNode&
    payload(node_id _id) const
    {
        return base_type::data(_id);
    }


    // =================================================================
    //  name resolution
    // =================================================================

    // name_of
    //   method: returns the unqualified name string for _id.
    const std::string&
    name_of(node_id _id) const
    {
        return m_strings.resolve(payload(_id).name);
    }

    // type_of
    //   method: returns the type spelling string for _id.
    const std::string&
    type_of(node_id _id) const
    {
        return m_strings.resolve(payload(_id).type_spelling);
    }

    // comment_of
    //   method: returns the documentation comment for _id.
    const std::string&
    comment_of(node_id _id) const
    {
        return m_strings.resolve(payload(_id).comment);
    }


    // =================================================================
    //  annotation (set properties on existing nodes)
    // =================================================================

    // set_location
    //   method: sets the source location on an existing node.
    void
    set_location(node_id            _id,
                 const std::string& _file,
                 std::uint32_t      _line,
                 std::uint32_t      _column)
    {
        _DomNode& nd = payload(_id);
        nd.file   = m_strings.intern(_file);
        nd.line   = _line;
        nd.column = _column;

        return;
    }

    // set_comment
    //   method: sets the documentation comment on a node.
    void
    set_comment(node_id            _id,
                const std::string& _text)
    {
        payload(_id).comment = m_strings.intern(_text);

        return;
    }

    // set_qualifiers
    //   method: replaces the qualifier bitfield on a node.
    void
    set_qualifiers(node_id       _id,
                   std::uint64_t _quals)
    {
        payload(_id).qualifiers = _quals;

        return;
    }

    // add_qualifier
    //   method: ORs a qualifier flag into the existing field.
    void
    add_qualifier(node_id       _id,
                  std::uint64_t _flag)
    {
        payload(_id).qualifiers |= _flag;

        return;
    }

    // set_stable_id
    //   method: sets the persistent identity hash.
    void
    set_stable_id(node_id       _id,
                  std::uint64_t _stable)
    {
        payload(_id).stable_id = _stable;

        return;
    }

    // mark_definition
    //   method: marks a node as a definition (not just a decl).
    void
    mark_definition(node_id _id)
    {
        payload(_id).is_definition = true;

        return;
    }


    // =================================================================
    //  kind-filtered queries
    // =================================================================

    // find_by_name
    //   method: returns all node_ids whose name matches _name.
    std::vector<node_id>
    find_by_name(const std::string& _name) const
    {
        std::vector<node_id> result;
        dom_string_id        target;

        target = m_strings.resolve(D_DOM_NULL_STRING) == _name
               ? D_DOM_NULL_STRING
               : D_DOM_NULL_STRING;

        // search through the interned table first
        for (dom_string_id i = 1;
             i < static_cast<dom_string_id>(m_strings.size());
             ++i)
        {
            if (m_strings.resolve(i) == _name)
            {
                target = i;
                break;
            }
        }

        // no match in the string table
        if (target == D_DOM_NULL_STRING)
        {
            return result;
        }

        // walk the arena collecting matches
        if (base_type::has_root())
        {
            base_type::visit_depth_first(
                base_type::root(),
                [this, target, &result]
                (node_id _nid, std::size_t)
                {
                    if (this->payload(_nid).name == target)
                    {
                        result.push_back(_nid);
                    }
                });
        }

        return result;
    }

    // find_by_kind
    //   method: returns all node_ids with the given symbol_kind.
    std::vector<node_id>
    find_by_kind(std::uint16_t _kind) const
    {
        std::vector<node_id> result;

        if (base_type::has_root())
        {
            base_type::visit_depth_first(
                base_type::root(),
                [this, _kind, &result]
                (node_id _nid, std::size_t)
                {
                    if (this->payload(_nid).kind == _kind)
                    {
                        result.push_back(_nid);
                    }
                });
        }

        return result;
    }

    // find_by_stable_id
    //   method: returns the first node with the given stable_id,
    // or null_node if not found.
    node_id
    find_by_stable_id(std::uint64_t _stable) const
    {
        node_id found = null_node;

        if (base_type::has_root())
        {
            base_type::visit_depth_first(
                base_type::root(),
                [this, _stable, &found]
                (node_id _nid, std::size_t)
                {
                    if ( (found == null_node) &&
                         (this->payload(_nid).stable_id ==
                          _stable) )
                    {
                        found = _nid;
                    }
                });
        }

        return found;
    }

    // children_of_kind
    //   method: returns the immediate children of _parent
    // whose kind matches _kind.
    std::vector<node_id>
    children_of_kind(node_id       _parent,
                     std::uint16_t _kind) const
    {
        std::vector<node_id> all_children;
        std::vector<node_id> result;

        base_type::collect_children(_parent, all_children);

        for (node_id cid : all_children)
        {
            if (payload(cid).kind == _kind)
            {
                result.push_back(cid);
            }
        }

        return result;
    }


    // =================================================================
    //  clear
    // =================================================================

    // clear
    //   method: destroys all nodes and resets the string table.
    void
    clear()
    {
        base_type::clear();
        m_strings.clear();

        return;
    }


private:
    dom_string_table m_strings;
};


// =============================================================================
// II.  cpp_dom
// =============================================================================

// cpp_dom
//   class: concrete DOM tree for parsed C++ source code.
// Payload is cpp_dom_node.  Provides language-aware building
// methods that construct properly-typed nodes and maintain the
// cached child counts in the payload.
//
//   All arena_tree operations (detach, move_subtree, etc.) and
// all dom_tree_base operations (find_by_name, set_location,
// etc.) are inherited and available.

class cpp_dom : public dom_tree_base<cpp_dom_node>
{
private:
    using base_type = dom_tree_base<cpp_dom_node>;

public:
    using base_type::base_type;  // inherit constructors


    // =================================================================
    //  C++ name resolution
    // =================================================================

    // qualified_name_of
    //   method: returns the fully qualified name for _id.
    const std::string&
    qualified_name_of(node_id _id) const
    {
        dom_string_id qn = payload(_id).qualified_name;

        if (qn != D_DOM_NULL_STRING)
        {
            return resolve(qn);
        }

        return name_of(_id);
    }

    // return_type_of
    //   method: returns the return type spelling for _id.
    const std::string&
    return_type_of(node_id _id) const
    {
        return resolve(payload(_id).return_type);
    }

    // signature_of
    //   method: returns the full signature text for _id.
    const std::string&
    signature_of(node_id _id) const
    {
        return resolve(payload(_id).signature);
    }

    // underlying_type_of
    //   method: returns the underlying type for typedefs/aliases.
    const std::string&
    underlying_type_of(node_id _id) const
    {
        return resolve(payload(_id).underlying_type);
    }


    // =================================================================
    //  C++ annotation (extends base set_*)
    // =================================================================

    // set_qualified_name
    //   method: sets the fully qualified name on a node.
    void
    set_qualified_name(node_id            _id,
                       const std::string& _qname)
    {
        payload(_id).qualified_name = intern(_qname);

        return;
    }

    // set_return_type
    //   method: sets the return type on a function/method node.
    void
    set_return_type(node_id            _id,
                    const std::string& _type)
    {
        payload(_id).return_type = intern(_type);

        return;
    }

    // set_signature
    //   method: sets the full signature text.
    void
    set_signature(node_id            _id,
                  const std::string& _sig)
    {
        payload(_id).signature = intern(_sig);

        return;
    }

    // set_mangled_name
    //   method: sets the linker-mangled name.
    void
    set_mangled_name(node_id            _id,
                     const std::string& _mangled)
    {
        payload(_id).mangled_name = intern(_mangled);

        return;
    }

    // set_underlying_type
    //   method: sets the underlying type for typedefs/aliases/
    // enums.
    void
    set_underlying_type(node_id            _id,
                        const std::string& _type)
    {
        payload(_id).underlying_type = intern(_type);

        return;
    }

    // mark_deleted
    //   method: sets the = delete marker.
    void
    mark_deleted(node_id _id)
    {
        add_qualifier(_id, lang::qualifier::deleted_);

        return;
    }

    // mark_defaulted
    //   method: sets the = default marker.
    void
    mark_defaulted(node_id _id)
    {
        add_qualifier(_id, lang::qualifier::defaulted_);

        return;
    }

    // mark_deprecated
    //   method: sets the [[deprecated]] marker.
    void
    mark_deprecated(node_id _id)
    {
        add_qualifier(_id, lang::qualifier::deprecated_);

        return;
    }


    // =================================================================
    //  building API — translation unit
    // =================================================================

    // add_translation_unit
    //   method: creates the root node representing a parsed file.
    node_id
    add_translation_unit(const std::string& _filename)
    {
        cpp_dom_node nd(lang::symbol_kind::translation_unit,
                        intern(_filename));
        nd.file          = nd.name;
        nd.is_definition = true;

        return base_type::create_root(nd);
    }


    // =================================================================
    //  building API — scopes
    // =================================================================

    // add_namespace
    //   method: adds a namespace declaration as a child of
    // _parent.
    node_id
    add_namespace(node_id            _parent,
                  const std::string& _name)
    {
        cpp_dom_node nd(lang::symbol_kind::namespace_decl,
                        intern(_name));
        nd.is_definition = true;

        return base_type::add_child(_parent, nd);
    }

    // add_class
    //   method: adds a class declaration.
    node_id
    add_class(node_id            _parent,
              const std::string& _name,
              std::uint64_t      _quals  = lang::qualifier::none,
              std::uint8_t       _access =
                  lang::access_specifier::unspecified)
    {
        cpp_dom_node nd(lang::symbol_kind::class_decl,
                        intern(_name));
        nd.qualifiers = _quals;
        nd.access     = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_struct
    //   method: adds a struct declaration.
    node_id
    add_struct(node_id            _parent,
               const std::string& _name,
               std::uint64_t      _quals  = lang::qualifier::none,
               std::uint8_t       _access =
                   lang::access_specifier::unspecified)
    {
        cpp_dom_node nd(lang::symbol_kind::struct_decl,
                        intern(_name));
        nd.qualifiers = _quals;
        nd.access     = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_enum
    //   method: adds an enum declaration.
    node_id
    add_enum(node_id            _parent,
             const std::string& _name,
             std::uint8_t       _access =
                 lang::access_specifier::unspecified)
    {
        cpp_dom_node nd(lang::symbol_kind::enum_decl,
                        intern(_name));
        nd.access = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }


    // =================================================================
    //  building API — callables
    // =================================================================

    // add_function
    //   method: adds a free function declaration.
    node_id
    add_function(node_id            _parent,
                 const std::string& _name,
                 const std::string& _return_type,
                 std::uint64_t      _quals = lang::qualifier::none)
    {
        cpp_dom_node nd(lang::symbol_kind::function_decl,
                        intern(_name));
        nd.return_type = intern(_return_type);
        nd.qualifiers  = _quals;

        return base_type::add_child(_parent, nd);
    }

    // add_method
    //   method: adds a class method declaration.
    node_id
    add_method(node_id            _parent,
               const std::string& _name,
               const std::string& _return_type,
               std::uint64_t      _quals  = lang::qualifier::none,
               std::uint8_t       _access =
                   lang::access_specifier::public_)
    {
        cpp_dom_node nd(lang::symbol_kind::method_decl,
                        intern(_name));
        nd.return_type = intern(_return_type);
        nd.qualifiers  = _quals;
        nd.access      = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_constructor
    //   method: adds a constructor declaration.
    node_id
    add_constructor(node_id       _parent,
                    std::uint64_t _quals  = lang::qualifier::none,
                    std::uint8_t  _access =
                        lang::access_specifier::public_)
    {
        // use the parent's name as the constructor name
        dom_string_id ctor_name = payload(_parent).name;

        cpp_dom_node nd(lang::symbol_kind::constructor_decl,
                        ctor_name);
        nd.qualifiers = _quals;
        nd.access     = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_destructor
    //   method: adds a destructor declaration.
    node_id
    add_destructor(node_id       _parent,
                   std::uint64_t _quals  = lang::qualifier::none,
                   std::uint8_t  _access =
                       lang::access_specifier::public_)
    {
        // prefix the parent's name with ~
        std::string dtor_name =
            "~" + resolve(payload(_parent).name);

        cpp_dom_node nd(lang::symbol_kind::destructor_decl,
                        intern(dtor_name));
        nd.qualifiers = _quals;
        nd.access     = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }


    // =================================================================
    //  building API — data members
    // =================================================================

    // add_field
    //   method: adds a class/struct field (data member).
    node_id
    add_field(node_id            _parent,
              const std::string& _name,
              const std::string& _type,
              std::uint64_t      _quals  = lang::qualifier::none,
              std::uint8_t       _access =
                  lang::access_specifier::private_)
    {
        cpp_dom_node nd(lang::symbol_kind::field_decl,
                        intern(_name));
        nd.type_spelling = intern(_type);
        nd.qualifiers    = _quals;
        nd.access        = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_variable
    //   method: adds a variable declaration (namespace or local
    // scope).
    node_id
    add_variable(node_id            _parent,
                 const std::string& _name,
                 const std::string& _type,
                 std::uint64_t      _quals = lang::qualifier::none)
    {
        cpp_dom_node nd(lang::symbol_kind::variable_decl,
                        intern(_name));
        nd.type_spelling = intern(_type);
        nd.qualifiers    = _quals;

        return base_type::add_child(_parent, nd);
    }

    // add_enum_constant
    //   method: adds an enumerator as a child of an enum node.
    node_id
    add_enum_constant(node_id            _parent,
                      const std::string& _name)
    {
        cpp_dom_node nd(lang::symbol_kind::enum_constant,
                        intern(_name));

        return base_type::add_child(_parent, nd);
    }


    // =================================================================
    //  building API — subordinate nodes
    // =================================================================

    // add_parameter
    //   method: adds a function/method parameter as a child of
    // a callable node.
    node_id
    add_parameter(node_id            _parent,
                  const std::string& _name,
                  const std::string& _type)
    {
        cpp_dom_node nd(lang::symbol_kind::parameter_decl,
                        intern(_name));
        nd.type_spelling = intern(_type);

        node_id id = base_type::add_child(_parent, nd);
        payload(_parent).param_count++;

        return id;
    }

    // add_template_param
    //   method: adds a template parameter (type, non-type, or
    // template-template) as a child of a template-bearing node.
    node_id
    add_template_param(node_id            _parent,
                       const std::string& _name,
                       const std::string& _type = "typename")
    {
        cpp_dom_node nd(lang::symbol_kind::template_param,
                        intern(_name));
        nd.type_spelling = intern(_type);

        node_id id = base_type::add_child(_parent, nd);
        payload(_parent).template_param_count++;
        add_qualifier(_parent, lang::qualifier::template_);

        return id;
    }

    // add_base_specifier
    //   method: adds a base class specifier as a child of a
    // class/struct node.
    node_id
    add_base_specifier(node_id            _parent,
                       const std::string& _base_name,
                       std::uint8_t       _access =
                           lang::access_specifier::public_,
                       bool               _is_virtual = false)
    {
        cpp_dom_node nd(lang::symbol_kind::base_specifier,
                        intern(_base_name));
        nd.access = _access;

        if (_is_virtual)
        {
            nd.qualifiers |= lang::qualifier::virtual_;
        }

        node_id id = base_type::add_child(_parent, nd);
        payload(_parent).base_count++;

        return id;
    }


    // =================================================================
    //  building API — type aliases
    // =================================================================

    // add_typedef
    //   method: adds a typedef declaration.
    node_id
    add_typedef(node_id            _parent,
                const std::string& _name,
                const std::string& _underlying,
                std::uint8_t       _access =
                    lang::access_specifier::unspecified)
    {
        cpp_dom_node nd(lang::symbol_kind::typedef_decl,
                        intern(_name));
        nd.underlying_type = intern(_underlying);
        nd.access          = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_type_alias
    //   method: adds a C++ using type alias.
    node_id
    add_type_alias(node_id            _parent,
                   const std::string& _name,
                   const std::string& _underlying,
                   std::uint8_t       _access =
                       lang::access_specifier::unspecified)
    {
        cpp_dom_node nd(lang::symbol_kind::type_alias_decl,
                        intern(_name));
        nd.underlying_type = intern(_underlying);
        nd.access          = _access;

        node_id id = base_type::add_child(_parent, nd);
        m_bump_member_count(_parent);

        return id;
    }

    // add_friend
    //   method: adds a friend declaration.
    node_id
    add_friend(node_id            _parent,
               const std::string& _name)
    {
        cpp_dom_node nd(lang::symbol_kind::friend_decl,
                        intern(_name));

        return base_type::add_child(_parent, nd);
    }


    // =================================================================
    //  C++ semantic queries
    // =================================================================

    // namespaces
    //   method: returns all namespace nodes in the tree.
    std::vector<node_id>
    namespaces() const
    {
        return find_by_kind(lang::symbol_kind::namespace_decl);
    }

    // classes
    //   method: returns all class and struct nodes.
    std::vector<node_id>
    classes() const
    {
        std::vector<node_id> result;

        if (has_root())
        {
            visit_depth_first(
                root(),
                [this, &result]
                (node_id _nid, std::size_t)
                {
                    if (lang::is_class_like_kind(
                            this->payload(_nid).kind))
                    {
                        result.push_back(_nid);
                    }
                });
        }

        return result;
    }

    // functions
    //   method: returns all free function nodes (excludes
    // methods).
    std::vector<node_id>
    functions() const
    {
        return find_by_kind(lang::symbol_kind::function_decl);
    }

    // methods
    //   method: returns method children of _class_node.
    std::vector<node_id>
    methods(node_id _class_node) const
    {
        std::vector<node_id> all_children;
        std::vector<node_id> result;

        collect_children(_class_node, all_children);

        for (node_id cid : all_children)
        {
            if (lang::is_method_kind(payload(cid).kind))
            {
                result.push_back(cid);
            }
        }

        return result;
    }

    // fields
    //   method: returns field children of _class_node.
    std::vector<node_id>
    fields(node_id _class_node) const
    {
        return children_of_kind(_class_node,
                                lang::symbol_kind::field_decl);
    }

    // parameters
    //   method: returns parameter children of a callable node.
    std::vector<node_id>
    parameters(node_id _callable) const
    {
        return children_of_kind(
            _callable, lang::symbol_kind::parameter_decl);
    }

    // template_params
    //   method: returns template parameter children.
    std::vector<node_id>
    template_params(node_id _node) const
    {
        return children_of_kind(
            _node, lang::symbol_kind::template_param);
    }

    // base_specifiers
    //   method: returns base class specifier children.
    std::vector<node_id>
    base_specifiers(node_id _class_node) const
    {
        return children_of_kind(
            _class_node, lang::symbol_kind::base_specifier);
    }

    // enum_constants
    //   method: returns enum constant children of an enum node.
    std::vector<node_id>
    enum_constants(node_id _enum_node) const
    {
        return children_of_kind(
            _enum_node, lang::symbol_kind::enum_constant);
    }


    // =================================================================
    //  C++ qualified name queries
    // =================================================================

    // find_by_qualified_name
    //   method: returns the first node whose qualified_name
    // matches _qname, or null_node if not found.
    node_id
    find_by_qualified_name(const std::string& _qname) const
    {
        node_id found = null_node;

        // intern won't modify the table since we're const,
        // so we search the string table manually.
        dom_string_id target = D_DOM_NULL_STRING;

        for (dom_string_id i = 1;
             i < static_cast<dom_string_id>(strings().size());
             ++i)
        {
            if (strings().resolve(i) == _qname)
            {
                target = i;
                break;
            }
        }

        if (target == D_DOM_NULL_STRING)
        {
            return null_node;
        }

        if (has_root())
        {
            visit_depth_first(
                root(),
                [this, target, &found]
                (node_id _nid, std::size_t)
                {
                    if ( (found == null_node) &&
                         (this->payload(_nid)
                              .qualified_name == target) )
                    {
                        found = _nid;
                    }
                });
        }

        return found;
    }

    // callables
    //   method: returns all callable nodes (functions +
    // methods + constructors + destructors) in the tree.
    std::vector<node_id>
    callables() const
    {
        std::vector<node_id> result;

        if (has_root())
        {
            visit_depth_first(
                root(),
                [this, &result]
                (node_id _nid, std::size_t)
                {
                    if (lang::is_callable_kind(
                            this->payload(_nid).kind))
                    {
                        result.push_back(_nid);
                    }
                });
        }

        return result;
    }

    // members
    //   method: returns all direct member children of a
    // class/struct (fields + methods + nested types + aliases),
    // excluding subordinate nodes like parameters and base
    // specifiers.
    std::vector<node_id>
    members(node_id _class_node) const
    {
        std::vector<node_id> all_children;
        std::vector<node_id> result;

        collect_children(_class_node, all_children);

        for (node_id cid : all_children)
        {
            std::uint16_t k = payload(cid).kind;

            // exclude subordinate kinds
            if ( (k != lang::symbol_kind::parameter_decl)  &&
                 (k != lang::symbol_kind::base_specifier)  &&
                 (k != lang::symbol_kind::template_param) )
            {
                result.push_back(cid);
            }
        }

        return result;
    }


private:

    // m_bump_member_count
    //   method: increments the member_count on a class-like
    // parent.  No-op if the parent is not class-like.
    void
    m_bump_member_count(node_id _parent)
    {
        if (lang::is_class_like_kind(payload(_parent).kind))
        {
            payload(_parent).member_count++;
        }

        return;
    }
};


NS_END  // djinterp


#endif  // DJINTERP_CPP_DOM_
