/******************************************************************************
* djinterp [css]                                              css_template.hpp
*
*   Templated CSS declaration / rule / stylesheet facades and the
* bundled default backend. Mirrors the templating pattern of
* `xml_template.hpp`, `html_template.hpp`, and `markdown_template.hpp`
* but targets CSS's rule-tree model (stylesheet contains rules; style
* rules carry a selector list and a declaration block; at-rules carry
* an at-keyword, a prelude, and either a declaration block or nested
* rules).
*
*   STORAGE MODEL:
*   For the default backend, a single underlying storage type
* (`internal::css_default_node`) carries both rule and declaration
* state plus optional selector / property / value fields. Rules have
* `kind = css_rule_kind::*` and may hold child nodes; declarations
* live as child nodes whose own kind tags them as declarations.
* Real backends (libcss, katana-parser, stylo) are free to use
* distinct types -- the trait layer detects them structurally.
*
*   FACADES:
*   - `css_declaration<_Backend>` wraps `_Backend::declaration_type`
*   - `css_rule<_Backend>`        wraps `_Backend::rule_type`
*   - `css_stylesheet<_Backend>`  holds a `_Backend::stylesheet_type`
*     by value and exposes the render targets.
*
*   ZERO OVERHEAD:
*   The declaration/rule facades hold a single pointer to the
* backend node. They do not own the storage and do not add data
* members beyond that pointer.
*
*   RENDERING:
*   `css_stylesheet` exposes:
*     - render_to_css           (formatted output)
*     - render_to_minified_css  (whitespace-stripped output)
*     - render_to_scss          (SCSS-flavoured output with native
*                                nesting via `&` selectors)
*
*
* path:      /inc/djinterp/core/util/css/css_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DEFAULT BACKEND STORAGE
II.   css_declaration<_Backend>
III.  css_rule<_Backend>
IV.   INTERNAL EMISSION HELPERS
V.    css_stylesheet<_Backend>
VI.   css_default_backend
VII.  FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_CSS_TEMPLATE_
#define DJINTERP_CSS_TEMPLATE_ 1

// std
#include <cstddef>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"


NS_DJINTERP

namespace css {


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULT BACKEND STORAGE                             ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace css
NS_INTERNAL

    // css_default_node
    //   struct: bundled in-memory storage type used by the
    // default backend for stylesheets, rules, and declarations.
    // Carries the rule-kind discriminator plus the union of all
    // fields every standard kind needs. Nodes can hold child
    // nodes (other rules or declarations). A single struct keeps
    // the default backend simple and inspectable.
    struct css_default_node
    {
        // discriminator
        ::djinterp::css::css_rule_kind          rule_kind =
            ::djinterp::css::css_rule_kind::unknown;
        ::djinterp::css::css_at_rule_kind       at_rule_kind =
            ::djinterp::css::css_at_rule_kind::unknown;

        // declaration fields (when rule_kind == declaration_block
        // or when node represents a declaration entry)
        std::string                             property;
        std::string                             value;
        ::djinterp::css::css_importance         importance =
            ::djinterp::css::css_importance::normal;
        ::djinterp::css::css_origin             origin =
            ::djinterp::css::css_origin::author;

        // style-rule fields
        std::string                             selector;       // joined selector list
        std::vector<std::string>                selectors;      // optional split form

        // at-rule fields
        std::string                             at_keyword;     // without '@'
        std::string                             prelude;        // text after keyword

        // comment field
        std::string                             comment_text;

        // tree
        std::vector<std::unique_ptr<css_default_node>>      children;
        css_default_node*                                   parent = nullptr;
    };


    // css_default_stylesheet
    //   struct: bundled in-memory storage type for stylesheets.
    // Holds the root rule list plus stylesheet-level metadata.
    struct css_default_stylesheet
    {
        ::djinterp::css::css_level              level =
            ::djinterp::css::css_level::css_3;
        ::djinterp::css::css_syntax_mode        syntax_mode =
            ::djinterp::css::css_syntax_mode::css;
        std::unique_ptr<css_default_node>       root;
    };

NS_END  // internal
namespace css {


///////////////////////////////////////////////////////////////////////////////
///                II.   css_declaration<_Backend>                          ///
///////////////////////////////////////////////////////////////////////////////

// css_declaration
//   class: thin facade over a backend declaration storage node.
// Holds only a raw pointer to the node; does not own the
// storage.
template<typename _Backend>
class css_declaration
{
public:
    // node_type
    //   type: the backend's declaration storage type.
    using node_type = typename _Backend::declaration_type;


    /// constructors

    css_declaration()
    :   m_node(nullptr)
    {}

    explicit
    css_declaration(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    bool       valid() const          { return (m_node != nullptr); }
    node_type* backend_handle() const { return m_node; }


    /// property

    // property
    //   function: returns this declaration's property name.
    const css_string_t&
    property() const
    {
        return (m_node != nullptr) ? m_node->property : s_empty();
    }

    // get_property
    //   function: getter-form alias.
    const css_string_t&
    get_property() const
    {
        return this->property();
    }

    // set_property
    //   function: replaces the property name.
    void
    set_property(
        const css_string_t&     _name
    )
    {
        if (m_node != nullptr)
        {
            m_node->property = _name;
        }
    }


    /// value

    // value
    //   function: returns this declaration's value (as a single
    // joined string -- the default backend does not split
    // tokens).
    const css_string_t&
    value() const
    {
        return (m_node != nullptr) ? m_node->value : s_empty();
    }

    // get_value
    //   function: getter-form alias.
    const css_string_t&
    get_value() const
    {
        return this->value();
    }

    // set_value
    void
    set_value(
        const css_string_t&     _value
    )
    {
        if (m_node != nullptr)
        {
            m_node->value = _value;
        }
    }


    /// importance / origin

    // importance
    css_importance
    importance() const
    {
        return (m_node != nullptr) ? m_node->importance
                                   : css_importance::normal;
    }

    // is_important
    bool
    is_important() const
    {
        return ( this->importance() == css_importance::important );
    }

    // set_important
    void
    set_important(
        bool    _important
    )
    {
        if (m_node != nullptr)
        {
            m_node->importance = _important
                ? css_importance::important
                : css_importance::normal;
        }
    }

    // origin
    css_origin
    origin() const
    {
        return (m_node != nullptr) ? m_node->origin : css_origin::author;
    }

    // is_custom_property
    //   function: true if this declaration's property name is
    // a custom property (begins with `--`).
    bool
    is_custom_property() const
    {
        return is_custom_property_name(this->property().c_str());
    }


private:
    static const css_string_t&
    s_empty()
    {
        static const css_string_t e;
        return e;
    }


    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                III.   css_rule<_Backend>                                ///
///////////////////////////////////////////////////////////////////////////////

// css_rule
//   class: thin facade over a backend rule storage node.
template<typename _Backend>
class css_rule
{
public:
    using node_type             = typename _Backend::rule_type;
    using declaration_node_type = typename _Backend::declaration_type;
    using declaration_facade    = css_declaration<_Backend>;


    css_rule()
    :   m_node(nullptr)
    {}

    explicit
    css_rule(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    bool       valid() const          { return (m_node != nullptr); }
    node_type* backend_handle() const { return m_node; }


    /// kind

    // rule_kind
    css_rule_kind
    rule_kind() const
    {
        return (m_node != nullptr) ? m_node->rule_kind
                                   : css_rule_kind::unknown;
    }

    // get_rule_kind
    css_rule_kind
    get_rule_kind() const
    {
        return this->rule_kind();
    }

    // at_rule_kind
    //   function: returns the at-rule subkind (only meaningful
    // when this rule is an at-rule).
    css_at_rule_kind
    at_rule_kind() const
    {
        return (m_node != nullptr) ? m_node->at_rule_kind
                                   : css_at_rule_kind::unknown;
    }

    // get_at_rule_kind
    css_at_rule_kind
    get_at_rule_kind() const
    {
        return this->at_rule_kind();
    }

    bool is_at_rule()      const { return is_at_rule_kind(this->rule_kind()); }
    bool is_conditional()  const { return is_conditional_rule_kind(this->rule_kind()); }
    bool is_grouping()     const { return is_grouping_rule_kind(this->rule_kind()); }
    bool is_descriptor()   const { return is_descriptor_rule_kind(this->rule_kind()); }
    bool is_nestable()     const { return is_nestable_rule_kind(this->rule_kind()); }
    bool is_keyframes()    const { return is_keyframes_rule_kind(this->rule_kind()); }
    bool is_style_rule()   const { return (this->rule_kind() == css_rule_kind::style_rule); }
    bool is_comment()      const { return (this->rule_kind() == css_rule_kind::comment); }


    /// selector access (style rules)

    const css_string_t&
    selector() const
    {
        return (m_node != nullptr) ? m_node->selector : s_empty();
    }

    const css_string_t&
    get_selector() const
    {
        return this->selector();
    }

    void
    set_selector(
        const css_string_t&     _sel
    )
    {
        if (m_node != nullptr)
        {
            m_node->selector = _sel;
        }
    }

    const std::vector<css_string_t>&
    selectors() const
    {
        return (m_node != nullptr) ? m_node->selectors : s_empty_vec();
    }


    /// at-rule access

    const css_string_t&
    at_keyword() const
    {
        return (m_node != nullptr) ? m_node->at_keyword : s_empty();
    }

    const css_string_t&
    get_at_keyword() const
    {
        return this->at_keyword();
    }

    void
    set_at_keyword(
        const css_string_t&     _kw
    )
    {
        if (m_node != nullptr)
        {
            m_node->at_keyword   = _kw;
            m_node->at_rule_kind = at_rule_kind_from_name(_kw.c_str());
        }
    }

    const css_string_t&
    prelude() const
    {
        return (m_node != nullptr) ? m_node->prelude : s_empty();
    }

    const css_string_t&
    get_prelude() const
    {
        return this->prelude();
    }

    void
    set_prelude(
        const css_string_t&     _prelude
    )
    {
        if (m_node != nullptr)
        {
            m_node->prelude = _prelude;
        }
    }


    /// declaration access

    // declaration_count
    std::size_t
    declaration_count() const
    {
        if (m_node == nullptr)
        {
            return 0;
        }
        std::size_t count = 0;
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            const node_type*    c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind == css_rule_kind::declaration_block) )
            {
                ++count;
            }
        }
        return count;
    }

    // declaration_at
    declaration_facade
    declaration_at(
        std::size_t     _index
    )   const
    {
        if (m_node == nullptr)
        {
            return declaration_facade();
        }
        std::size_t seen = 0;
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            node_type*  c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind == css_rule_kind::declaration_block) )
            {
                if (seen == _index)
                {
                    return declaration_facade(c);
                }
                ++seen;
            }
        }
        return declaration_facade();
    }

    // find_declaration
    //   function: returns the first declaration with property
    // name `_name`, or a null facade if absent.
    declaration_facade
    find_declaration(
        const css_string_t&     _name
    )   const
    {
        if (m_node == nullptr)
        {
            return declaration_facade();
        }
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            node_type*  c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind == css_rule_kind::declaration_block) &&
                 (c->property == _name) )
            {
                return declaration_facade(c);
            }
        }
        return declaration_facade();
    }

    // add_declaration
    //   function: appends a declaration to this rule's
    // declaration block. Returns a facade for the new entry.
    declaration_facade
    add_declaration(
        const css_string_t&     _property,
        const css_string_t&     _value,
        bool                    _important = false
    )
    {
        if (m_node == nullptr)
        {
            return declaration_facade();
        }
        std::unique_ptr<node_type>  d(new node_type);
        d->rule_kind  = css_rule_kind::declaration_block;
        d->property   = _property;
        d->value      = _value;
        d->importance = _important
            ? css_importance::important
            : css_importance::normal;
        d->parent     = m_node;
        node_type* raw = d.get();
        m_node->children.push_back(std::move(d));
        return declaration_facade(raw);
    }

    // remove_declaration
    //   function: removes the first declaration with property
    // name `_name`. Returns true if a declaration was removed.
    bool
    remove_declaration(
        const css_string_t&     _name
    )
    {
        if (m_node == nullptr)
        {
            return false;
        }
        for (auto it = m_node->children.begin();
             it != m_node->children.end();
             ++it)
        {
            node_type*  c = it->get();
            if ( (c != nullptr) &&
                 (c->rule_kind == css_rule_kind::declaration_block) &&
                 (c->property == _name) )
            {
                m_node->children.erase(it);
                return true;
            }
        }
        return false;
    }

    // set_property
    //   function: convenience -- finds the named declaration
    // and updates its value, or adds it if absent.
    void
    set_property(
        const css_string_t&     _name,
        const css_string_t&     _value,
        bool                    _important = false
    )
    {
        declaration_facade  d = this->find_declaration(_name);
        if (d.valid())
        {
            d.set_value(_value);
            d.set_important(_important);
            return;
        }
        this->add_declaration(_name, _value, _important);
    }


    /// nested rules

    // rule_count
    //   function: returns the count of nested non-declaration
    // children (i.e. nested rules).
    std::size_t
    rule_count() const
    {
        if (m_node == nullptr)
        {
            return 0;
        }
        std::size_t count = 0;
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            const node_type*    c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind != css_rule_kind::declaration_block) )
            {
                ++count;
            }
        }
        return count;
    }

    // rule_at
    //   function: returns the nested rule at index `_index`,
    // skipping declaration entries.
    css_rule<_Backend>
    rule_at(
        std::size_t     _index
    )   const
    {
        if (m_node == nullptr)
        {
            return css_rule<_Backend>();
        }
        std::size_t seen = 0;
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            node_type*  c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind != css_rule_kind::declaration_block) )
            {
                if (seen == _index)
                {
                    return css_rule<_Backend>(c);
                }
                ++seen;
            }
        }
        return css_rule<_Backend>();
    }

    // add_rule
    //   function: appends a fresh rule of the given kind.
    css_rule<_Backend>
    add_rule(
        css_rule_kind   _kind = css_rule_kind::style_rule
    )
    {
        if (m_node == nullptr)
        {
            return css_rule<_Backend>();
        }
        std::unique_ptr<node_type>  r(new node_type);
        r->rule_kind = _kind;
        r->parent    = m_node;
        node_type* raw = r.get();
        m_node->children.push_back(std::move(r));
        return css_rule<_Backend>(raw);
    }


private:
    static const css_string_t&
    s_empty()
    {
        static const css_string_t e;
        return e;
    }

    static const std::vector<css_string_t>&
    s_empty_vec()
    {
        static const std::vector<css_string_t> e;
        return e;
    }


    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   INTERNAL EMISSION HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace css
NS_INTERNAL

    // css_emit_indent_helper
    //   function: writes `_depth` repetitions of `_indent` to
    // `_out`.
    inline void
    css_emit_indent_helper(
        std::ostream&       _out,
        std::size_t         _depth,
        const char*         _indent
    )
    {
        for (std::size_t i = 0; i < _depth; ++i)
        {
            _out << _indent;
        }
    }


    // css_emit_declaration_helper
    //   function: emits a single declaration as `prop: value;`
    // (with optional `!important`). Does not include leading
    // whitespace.
    inline void
    css_emit_declaration_helper(
        std::ostream&               _out,
        const css_default_node&     _decl,
        bool                        _minified
    )
    {
        _out << _decl.property << ':';
        if (!_minified)
        {
            _out << ' ';
        }
        _out << _decl.value;
        if (_decl.importance == ::djinterp::css::css_importance::important)
        {
            _out << (_minified ? "!important" : " !important");
        }
        _out << ';';
    }


    // css_emit_rule_body_helper
    //   function: emits the `{ ... }` body of a rule. Walks
    // children and emits declarations and nested rules. Does
    // not include the leading selector / at-keyword (the caller
    // is responsible for that).
    inline void
    css_emit_rule_body_helper(
        std::ostream&               _out,
        const css_default_node&     _node,
        bool                        _minified,
        std::size_t                 _depth,
        const char*                 _indent,
        const char*                 _newline,
        bool                        _scss_nesting
    )
    {
        using K = ::djinterp::css::css_rule_kind;

        _out << (_minified ? "{" : "{")
             << (_minified ? "" : _newline);

        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            const css_default_node* c = _node.children[i].get();
            if (c == nullptr)
            {
                continue;
            }

            if (c->rule_kind == K::declaration_block)
            {
                if (!_minified)
                {
                    css_emit_indent_helper(_out, _depth + 1, _indent);
                }
                css_emit_declaration_helper(_out, *c, _minified);
                if (!_minified)
                {
                    _out << _newline;
                }
                continue;
            }

            if (c->rule_kind == K::comment)
            {
                if (!_minified)
                {
                    css_emit_indent_helper(_out, _depth + 1, _indent);
                    _out << "/*" << c->comment_text << "*/" << _newline;
                }
                continue;
            }

            // nested rule (CSS Nesting / SCSS)
            if (!_minified)
            {
                css_emit_indent_helper(_out, _depth + 1, _indent);
            }

            if (c->rule_kind == K::style_rule)
            {
                _out << c->selector;
                if (!_minified)
                {
                    _out << ' ';
                }
                css_emit_rule_body_helper(
                    _out, *c, _minified, _depth + 1, _indent, _newline,
                    _scss_nesting);
                if (!_minified)
                {
                    _out << _newline;
                }
            }
            else if (::djinterp::css::is_at_rule_kind(c->rule_kind))
            {
                _out << '@' << c->at_keyword;
                if (!c->prelude.empty())
                {
                    _out << ' ' << c->prelude;
                }
                if ( ::djinterp::css::is_grouping_rule_kind(c->rule_kind) ||
                     ::djinterp::css::is_descriptor_rule_kind(c->rule_kind) )
                {
                    if (!_minified)
                    {
                        _out << ' ';
                    }
                    css_emit_rule_body_helper(
                        _out, *c, _minified, _depth + 1, _indent, _newline,
                        _scss_nesting);
                }
                else
                {
                    _out << ';';
                }
                if (!_minified)
                {
                    _out << _newline;
                }
            }
        }

        if (!_minified)
        {
            css_emit_indent_helper(_out, _depth, _indent);
        }
        _out << '}';
    }


    // css_emit_rule_helper
    //   function: top-level rule emission. Dispatches on rule
    // kind and routes to the body helper.
    inline void
    css_emit_rule_helper(
        std::ostream&               _out,
        const css_default_node&     _node,
        bool                        _minified,
        std::size_t                 _depth,
        const char*                 _indent,
        const char*                 _newline,
        bool                        _scss_nesting
    )
    {
        using K = ::djinterp::css::css_rule_kind;

        if (_node.rule_kind == K::comment)
        {
            if (!_minified)
            {
                css_emit_indent_helper(_out, _depth, _indent);
                _out << "/*" << _node.comment_text << "*/" << _newline;
            }
            return;
        }

        if (!_minified)
        {
            css_emit_indent_helper(_out, _depth, _indent);
        }

        if (_node.rule_kind == K::style_rule)
        {
            _out << _node.selector;
            if (!_minified)
            {
                _out << ' ';
            }
            css_emit_rule_body_helper(
                _out, _node, _minified, _depth, _indent, _newline,
                _scss_nesting);
            if (!_minified)
            {
                _out << _newline;
            }
            return;
        }

        if (::djinterp::css::is_at_rule_kind(_node.rule_kind))
        {
            _out << '@' << _node.at_keyword;
            if (!_node.prelude.empty())
            {
                _out << ' ' << _node.prelude;
            }
            if ( ::djinterp::css::is_grouping_rule_kind(_node.rule_kind) ||
                 ::djinterp::css::is_descriptor_rule_kind(_node.rule_kind) )
            {
                if (!_minified)
                {
                    _out << ' ';
                }
                css_emit_rule_body_helper(
                    _out, _node, _minified, _depth, _indent, _newline,
                    _scss_nesting);
            }
            else
            {
                _out << ';';
            }
            if (!_minified)
            {
                _out << _newline;
            }
            return;
        }

        // unknown / unsupported -- emit nothing
    }


    // css_emit_stylesheet_helper
    //   function: emits the entire stylesheet by walking the
    // root node's children.
    inline void
    css_emit_stylesheet_helper(
        std::ostream&               _out,
        const css_default_node&     _root,
        bool                        _minified,
        const char*                 _indent,
        const char*                 _newline,
        bool                        _scss_nesting
    )
    {
        for (std::size_t i = 0; i < _root.children.size(); ++i)
        {
            const css_default_node* c = _root.children[i].get();
            if (c == nullptr)
            {
                continue;
            }
            css_emit_rule_helper(
                _out, *c, _minified, 0, _indent, _newline, _scss_nesting);
        }
    }

NS_END  // internal
namespace css {


///////////////////////////////////////////////////////////////////////////////
///                V.   css_stylesheet<_Backend>                            ///
///////////////////////////////////////////////////////////////////////////////

// css_stylesheet
//   class: facade over the backend's storage stylesheet type
// plus level / syntax-mode metadata. Owns the storage by value
// and exposes the render targets.
template<typename _Backend>
class css_stylesheet
{
public:
    using backend_type    = _Backend;
    using stylesheet_type = typename _Backend::stylesheet_type;
    using rule_node_type  = typename _Backend::rule_type;
    using rule_facade     = css_rule<_Backend>;


    /// constructors

    css_stylesheet()
    :   m_sheet(_Backend::make_stylesheet())
    {}

    explicit
    css_stylesheet(
        css_level   _level
    )
    :   m_sheet(_Backend::make_stylesheet())
    {
        m_sheet.level = _level;
    }

    css_stylesheet(
        css_level           _level,
        css_syntax_mode     _syntax
    )
    :   m_sheet(_Backend::make_stylesheet())
    {
        m_sheet.level       = _level;
        m_sheet.syntax_mode = _syntax;
    }

    explicit
    css_stylesheet(
        stylesheet_type&&   _sheet
    )
    :   m_sheet(std::move(_sheet))
    {}


    /// metadata

    css_level
    level() const
    {
        return m_sheet.level;
    }

    css_level
    get_level() const
    {
        return m_sheet.level;
    }

    void
    set_level(
        css_level   _level
    )
    {
        m_sheet.level = _level;
    }

    css_syntax_mode
    syntax_mode() const
    {
        return m_sheet.syntax_mode;
    }

    css_syntax_mode
    get_syntax_mode() const
    {
        return m_sheet.syntax_mode;
    }

    void
    set_syntax_mode(
        css_syntax_mode     _mode
    )
    {
        m_sheet.syntax_mode = _mode;
    }


    /// root access

    rule_facade
    root_rule() const
    {
        return rule_facade(m_sheet.root.get());
    }

    const stylesheet_type&
    backend_stylesheet() const
    {
        return m_sheet;
    }


    /// rule traversal

    // rule_count
    //   function: returns the number of top-level rules.
    std::size_t
    rule_count() const
    {
        if (!m_sheet.root)
        {
            return 0;
        }
        return m_sheet.root->children.size();
    }

    // rule_at
    rule_facade
    rule_at(
        std::size_t     _index
    )   const
    {
        if ( (!m_sheet.root) ||
             (_index >= m_sheet.root->children.size()) )
        {
            return rule_facade();
        }
        return rule_facade(m_sheet.root->children[_index].get());
    }

    // add_style_rule
    //   function: appends a new style rule with the given
    // selector and returns a facade for it.
    rule_facade
    add_style_rule(
        const css_string_t&     _selector
    )
    {
        if (!m_sheet.root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type> r(new rule_node_type);
        r->rule_kind = css_rule_kind::style_rule;
        r->selector  = _selector;
        r->parent    = m_sheet.root.get();
        rule_node_type* raw = r.get();
        m_sheet.root->children.push_back(std::move(r));
        return rule_facade(raw);
    }

    // add_at_rule
    //   function: appends a new at-rule with the given
    // keyword and prelude.
    rule_facade
    add_at_rule(
        const css_string_t&     _keyword,
        const css_string_t&     _prelude = css_string_t()
    )
    {
        if (!m_sheet.root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type> r(new rule_node_type);
        r->at_rule_kind = at_rule_kind_from_name(_keyword.c_str());
        r->rule_kind    = rule_kind_from_at_rule_kind(r->at_rule_kind);
        r->at_keyword   = _keyword;
        r->prelude      = _prelude;
        r->parent       = m_sheet.root.get();
        rule_node_type* raw = r.get();
        m_sheet.root->children.push_back(std::move(r));
        return rule_facade(raw);
    }


    /// rendering

    // render_to_css
    //   function: emits this stylesheet as formatted CSS source.
    void
    render_to_css(
        std::ostream&   _out
    )   const
    {
        if (!m_sheet.root)
        {
            return;
        }
        const bool  scss_nesting =
            ( m_sheet.syntax_mode == css_syntax_mode::scss ||
              m_sheet.syntax_mode == css_syntax_mode::css_nesting );
        ::djinterp::internal::css_emit_stylesheet_helper(
            _out, *m_sheet.root,
            /*minified*/ false,
            D_CSS_DEFAULT_INDENT,
            D_CSS_DEFAULT_NEWLINE,
            scss_nesting);
    }

    css_string_t
    render_to_css() const
    {
        std::ostringstream  oss;
        this->render_to_css(oss);
        return oss.str();
    }


    // render_to_minified_css
    //   function: emits this stylesheet with whitespace
    // stripped.
    void
    render_to_minified_css(
        std::ostream&   _out
    )   const
    {
        if (!m_sheet.root)
        {
            return;
        }
        const bool  scss_nesting =
            ( m_sheet.syntax_mode == css_syntax_mode::scss ||
              m_sheet.syntax_mode == css_syntax_mode::css_nesting );
        ::djinterp::internal::css_emit_stylesheet_helper(
            _out, *m_sheet.root,
            /*minified*/ true,
            "", "",
            scss_nesting);
    }

    css_string_t
    render_to_minified_css() const
    {
        std::ostringstream  oss;
        this->render_to_minified_css(oss);
        return oss.str();
    }


    // render_to_scss
    //   function: emits this stylesheet as SCSS-flavoured
    // source. The bundled emitter routes through the same
    // formatter as render_to_css with SCSS nesting enabled;
    // this stub gives adapter backends a clear hook to override
    // with full SCSS semantics (variables, mixins, @use,
    // partials).
    void
    render_to_scss(
        std::ostream&   _out
    )   const
    {
        if (!m_sheet.root)
        {
            return;
        }
        ::djinterp::internal::css_emit_stylesheet_helper(
            _out, *m_sheet.root,
            /*minified*/ false,
            D_CSS_DEFAULT_INDENT,
            D_CSS_DEFAULT_NEWLINE,
            /*scss_nesting*/ true);
    }

    css_string_t
    render_to_scss() const
    {
        std::ostringstream  oss;
        this->render_to_scss(oss);
        return oss.str();
    }


private:
    // m_sheet
    //   field: backend storage stylesheet, held by value.
    stylesheet_type     m_sheet;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   css_default_backend                                ///
///////////////////////////////////////////////////////////////////////////////

// css_default_backend
//   struct: bundled in-memory backend for the CSS facades.
// Uses a single underlying storage type for stylesheets,
// rules, and declarations.
struct css_default_backend
{
    using css_backend_tag = ::djinterp::css::css_default_backend_tag;

    using rule_type        = ::djinterp::internal::css_default_node;
    using declaration_type = ::djinterp::internal::css_default_node;
    using stylesheet_type  = ::djinterp::internal::css_default_stylesheet;

    // make_stylesheet
    //   function: factory for a fresh empty stylesheet with a
    // root `stylesheet` node ready to receive children.
    static stylesheet_type
    make_stylesheet()
    {
        stylesheet_type     s;
        s.level       = css_level::css_3;
        s.syntax_mode = css_syntax_mode::css;
        std::unique_ptr<rule_type>  root(new rule_type);
        root->rule_kind = css_rule_kind::stylesheet;
        s.root          = std::move(root);
        return s;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                VII.   FREE HELPERS / FACTORIES                          ///
///////////////////////////////////////////////////////////////////////////////

// make_stylesheet
//   function: factory returning a freshly-built stylesheet
// facade for the given backend.
template<typename _Backend>
inline css_stylesheet<_Backend>
make_stylesheet(
    css_level           _level  = css_level::css_3,
    css_syntax_mode     _syntax = css_syntax_mode::css
)
{
    return css_stylesheet<_Backend>(_level, _syntax);
}


// make_style_rule
//   function: returns a new freestanding style rule with the
// given selector.
template<typename _Backend>
inline css_rule<_Backend>
make_style_rule(
    const css_string_t&     _selector
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = css_rule_kind::style_rule;
    n->selector  = _selector;
    return css_rule<_Backend>(n);
}


// make_at_rule
//   function: returns a new freestanding at-rule with the
// given keyword and prelude.
template<typename _Backend>
inline css_rule<_Backend>
make_at_rule(
    const css_string_t&     _keyword,
    const css_string_t&     _prelude = css_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_rule_kind = at_rule_kind_from_name(_keyword.c_str());
    n->rule_kind    = rule_kind_from_at_rule_kind(n->at_rule_kind);
    n->at_keyword   = _keyword;
    n->prelude      = _prelude;
    return css_rule<_Backend>(n);
}


// make_media_rule
//   function: convenience for `make_at_rule(at_rules::media,
// _query)`.
template<typename _Backend>
inline css_rule<_Backend>
make_media_rule(
    const css_string_t&     _query
)
{
    return make_at_rule<_Backend>(at_rules::media, _query);
}


// make_supports_rule
//   function: convenience for `@supports`.
template<typename _Backend>
inline css_rule<_Backend>
make_supports_rule(
    const css_string_t&     _condition
)
{
    return make_at_rule<_Backend>(at_rules::supports, _condition);
}


// make_keyframes_rule
//   function: convenience for `@keyframes <name>`.
template<typename _Backend>
inline css_rule<_Backend>
make_keyframes_rule(
    const css_string_t&     _name
)
{
    return make_at_rule<_Backend>(at_rules::keyframes, _name);
}


// make_font_face_rule
//   function: convenience for `@font-face`.
template<typename _Backend>
inline css_rule<_Backend>
make_font_face_rule()
{
    return make_at_rule<_Backend>(at_rules::font_face);
}


// make_import_rule
//   function: convenience for `@import url(...)`.
template<typename _Backend>
inline css_rule<_Backend>
make_import_rule(
    const css_string_t&     _url
)
{
    return make_at_rule<_Backend>(at_rules::import_, _url);
}


// make_charset_rule
//   function: convenience for `@charset "encoding"`.
template<typename _Backend>
inline css_rule<_Backend>
make_charset_rule(
    const css_string_t&     _encoding
)
{
    css_string_t    quoted;
    quoted.reserve(_encoding.size() + 2);
    quoted.push_back('"');
    quoted.append(_encoding);
    quoted.push_back('"');
    return make_at_rule<_Backend>(at_rules::charset, quoted);
}


// make_comment
//   function: returns a new freestanding comment rule.
template<typename _Backend>
inline css_rule<_Backend>
make_comment(
    const css_string_t&     _text
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind    = css_rule_kind::comment;
    n->comment_text = _text;
    return css_rule<_Backend>(n);
}


}   // namespace css
NS_END  // djinterp


#endif  // DJINTERP_CSS_TEMPLATE_
