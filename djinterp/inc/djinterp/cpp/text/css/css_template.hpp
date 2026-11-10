/*******************************************************************************
* djinterp [cpp]                                                css_template.hpp
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
* (`internal::DCssDefaultNode`) carries both rule and declaration
* state plus optional selector / property / value fields. Rules have
* `kind = DCssRuleKind::*` and may hold child nodes; declarations
* live as child nodes whose own kind tags them as declarations.
* Real backends (libcss, katana-parser, stylo) are free to use
* distinct types -- the trait layer detects them structurally.
*
*   FACADES:
*   - `DCssDeclaration<Backend>` wraps `Backend::declaration_type`
*   - `DCssRule<Backend>`        wraps `Backend::rule_type`
*   - `DCssStylesheet<Backend>`  holds a `Backend::stylesheet_type`
*     by value and exposes the render targets.
*
*   ZERO OVERHEAD:
*   The declaration/rule facades hold a single pointer to the
* backend node. They do not own the storage and do not add data
* members beyond that pointer.
*
*   RENDERING:
*   `DCssStylesheet` exposes:
*     - render_to_css           (formatted output)
*     - render_to_minified_css  (whitespace-stripped output)
*     - render_to_scss          (SCSS-flavoured output with native
*                                nesting via `&` selectors)
*
*
* path:      /inc/djinterp/cpp/text/css/css_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.10
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DEFAULT BACKEND STORAGE
      -----------------------

II.   DCssDeclaration<Backend>
      -------------------------

III.  DCssRule<Backend>
      ------------------

IV.   INTERNAL EMISSION HELPERS
      -------------------------

V.    DCssStylesheet<Backend>
      ------------------------

VI.   DCssDefaultBackend
      -------------------

VII.  FREE HELPERS / FACTORIES
      ------------------------
*/

#ifndef DJINTERP_TEXT_CSS_CSS_TEMPLATE_HPP
#define DJINTERP_TEXT_CSS_CSS_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./css.hpp"             // foundation + folded traits/concepts


NS_DJINTERP

namespace css {


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULT BACKEND STORAGE                             ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace css
NS_INTERNAL

    // DCssDefaultNode
    //   struct: bundled in-memory storage type used by the
    // default backend for stylesheets, rules, and declarations.
    // Carries the rule-kind discriminator plus the union of all
    // fields every standard kind needs. Nodes can hold child
    // nodes (other rules or declarations). A single struct keeps
    // the default backend simple and inspectable.
    struct DCssDefaultNode
    {
        // discriminator
        ::djinterp::css::DCssRuleKind          rule_kind =
            ::djinterp::css::DCssRuleKind::unknown;
        ::djinterp::css::DCssAtRuleKind       at_rule_kind =
            ::djinterp::css::DCssAtRuleKind::unknown;

        // declaration fields (when rule_kind == declaration_block
        // or when node represents a declaration entry)
        std::string                             property;
        std::string                             value;
        ::djinterp::css::DCssImportance         importance =
            ::djinterp::css::DCssImportance::normal;
        ::djinterp::css::DCssOrigin             origin =
            ::djinterp::css::DCssOrigin::author;

        // style-rule fields
        std::string                             selector;       // joined selector list
        std::vector<std::string>                selectors;      // optional split form

        // at-rule fields
        std::string                             at_keyword;     // without '@'
        std::string                             prelude;        // text after keyword

        // comment field
        std::string                             comment_text;

        // tree
        std::vector<std::unique_ptr<DCssDefaultNode>>       children;
        DCssDefaultNode*                                    parent = nullptr;
    };


    // DCssDefaultStylesheet
    //   struct: bundled in-memory storage type for stylesheets.
    // Holds the root rule list plus stylesheet-level metadata.
    struct DCssDefaultStylesheet
    {
        ::djinterp::css::DCssLevel              level =
            ::djinterp::css::DCssLevel::css_3;
        ::djinterp::css::DCssSyntaxMode        syntax_mode =
            ::djinterp::css::DCssSyntaxMode::css;
        std::unique_ptr<DCssDefaultNode>       root;
    };

NS_END  // internal
namespace css {


///////////////////////////////////////////////////////////////////////////////
///                II.   DCssDeclaration<Backend>                          ///
///////////////////////////////////////////////////////////////////////////////

// DCssDeclaration
//   class: thin facade over a backend declaration storage node.
// Holds only a raw pointer to the node; does not own the
// storage.
template<typename Backend>
class DCssDeclaration
{
public:
    // node_type
    //   type: the backend's declaration storage type.
    using node_type = typename Backend::declaration_type;


    /// constructors

    DCssDeclaration()
    :   m_node(nullptr)
    {}

    explicit
    DCssDeclaration(
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
    const DCssString& property() const
    {
        return (m_node != nullptr) ? m_node->property : s_empty();
    }

    // get_property
    //   function: getter-form alias.
    const DCssString& get_property() const
    {
        return this->property();
    }

    // set_property
    //   function: replaces the property name.
    void set_property(
        const DCssString& _name
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
    const DCssString& value() const
    {
        return (m_node != nullptr) ? m_node->value : s_empty();
    }

    // get_value
    //   function: getter-form alias.
    const DCssString& get_value() const
    {
        return this->value();
    }

    // set_value
    void set_value(
        const DCssString& _value
    )
    {
        if (m_node != nullptr)
        {
            m_node->value = _value;
        }
    }


    /// importance / origin

    // importance
    DCssImportance importance() const
    {
        return (m_node != nullptr) ? m_node->importance
                                   : DCssImportance::normal;
    }

    // is_important
    bool is_important() const
    {
        return ( this->importance() == DCssImportance::important );
    }

    // set_important
    void set_important(
        bool _important
    )
    {
        if (m_node != nullptr)
        {
            m_node->importance = _important
                ? DCssImportance::important
                : DCssImportance::normal;
        }
    }

    // origin
    DCssOrigin origin() const
    {
        return (m_node != nullptr) ? m_node->origin : DCssOrigin::author;
    }

    // is_custom_property
    //   function: true if this declaration's property name is
    // a custom property (begins with `--`).
    bool is_custom_property() const
    {
        return is_custom_property_name(this->property().c_str());
    }


private:
    static
    const DCssString& s_empty()
    {
        static const DCssString e;
        return e;
    }


    node_type*  m_node;
};


///////////////////////////////////////////////////////////////////////////////
///                III.   DCssRule<Backend>                                ///
///////////////////////////////////////////////////////////////////////////////

// DCssRule
//   class: thin facade over a backend rule storage node.
template<typename Backend>
class DCssRule
{
public:
    using node_type             = typename Backend::rule_type;
    using declaration_node_type = typename Backend::declaration_type;
    using declaration_facade    = DCssDeclaration<Backend>;


    DCssRule()
    :   m_node(nullptr)
    {}

    explicit
    DCssRule(
        node_type*  _node
    )
    :   m_node(_node)
    {}


    /// identity

    bool       valid() const          { return (m_node != nullptr); }
    node_type* backend_handle() const { return m_node; }


    /// kind

    // rule_kind
    DCssRuleKind rule_kind() const
    {
        return (m_node != nullptr) ? m_node->rule_kind
                                   : DCssRuleKind::unknown;
    }

    // get_rule_kind
    DCssRuleKind get_rule_kind() const
    {
        return this->rule_kind();
    }

    // at_rule_kind
    //   function: returns the at-rule subkind (only meaningful
    // when this rule is an at-rule).
    DCssAtRuleKind at_rule_kind() const
    {
        return (m_node != nullptr) ? m_node->at_rule_kind
                                   : DCssAtRuleKind::unknown;
    }

    // get_at_rule_kind
    DCssAtRuleKind get_at_rule_kind() const
    {
        return this->at_rule_kind();
    }

    bool is_at_rule()      const { return is_at_rule_kind(this->rule_kind()); }
    bool is_conditional()  const { return is_conditional_rule_kind(this->rule_kind()); }
    bool is_grouping()     const { return is_grouping_rule_kind(this->rule_kind()); }
    bool is_descriptor()   const { return is_descriptor_rule_kind(this->rule_kind()); }
    bool is_nestable()     const { return is_nestable_rule_kind(this->rule_kind()); }
    bool is_keyframes()    const { return is_keyframes_rule_kind(this->rule_kind()); }
    bool is_style_rule()   const { return (this->rule_kind() == DCssRuleKind::style_rule); }
    bool is_comment()      const { return (this->rule_kind() == DCssRuleKind::comment); }


    /// selector access (style rules)

    const DCssString& selector() const
    {
        return (m_node != nullptr) ? m_node->selector : s_empty();
    }

    const DCssString& get_selector() const
    {
        return this->selector();
    }

    void set_selector(
        const DCssString& _sel
    )
    {
        if (m_node != nullptr)
        {
            m_node->selector = _sel;
        }
    }

    const std::vector<DCssString>& selectors() const
    {
        return (m_node != nullptr) ? m_node->selectors : s_empty_vec();
    }


    /// at-rule access

    const DCssString& at_keyword() const
    {
        return (m_node != nullptr) ? m_node->at_keyword : s_empty();
    }

    const DCssString& get_at_keyword() const
    {
        return this->at_keyword();
    }

    void set_at_keyword(
        const DCssString& _kw
    )
    {
        if (m_node != nullptr)
        {
            m_node->at_keyword   = _kw;
            m_node->at_rule_kind = at_rule_kind_from_name(_kw.c_str());
        }
    }

    const DCssString& prelude() const
    {
        return (m_node != nullptr) ? m_node->prelude : s_empty();
    }

    const DCssString& get_prelude() const
    {
        return this->prelude();
    }

    void set_prelude(
        const DCssString& _prelude
    )
    {
        if (m_node != nullptr)
        {
            m_node->prelude = _prelude;
        }
    }


    /// declaration access

    // declaration_count
    std::size_t declaration_count() const
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
                 (c->rule_kind == DCssRuleKind::declaration_block) )
            {
                ++count;
            }
        }
        return count;
    }

    // declaration_at
    declaration_facade declaration_at(
        std::size_t _index
    ) const
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
                 (c->rule_kind == DCssRuleKind::declaration_block) )
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
    declaration_facade find_declaration(
        const DCssString& _name
    ) const
    {
        if (m_node == nullptr)
        {
            return declaration_facade();
        }
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            node_type*  c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind == DCssRuleKind::declaration_block) &&
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
    declaration_facade add_declaration(
        const DCssString& _property,
        const DCssString&   _value,
        bool                _important = false
    )
    {
        if (m_node == nullptr)
        {
            return declaration_facade();
        }
        std::unique_ptr<node_type>  d(new node_type);
        d->rule_kind  = DCssRuleKind::declaration_block;
        d->property   = _property;
        d->value      = _value;
        d->importance = _important
            ? DCssImportance::important
            : DCssImportance::normal;
        d->parent     = m_node;
        node_type* raw = d.get();
        m_node->children.push_back(std::move(d));
        return declaration_facade(raw);
    }

    // remove_declaration
    //   function: removes the first declaration with property
    // name `_name`. Returns true if a declaration was removed.
    bool remove_declaration(
        const DCssString& _name
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
                 (c->rule_kind == DCssRuleKind::declaration_block) &&
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
    void set_property(
        const DCssString& _name,
        const DCssString&   _value,
        bool                _important = false
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


    /// native color / font convenience (via the folded to_css_* bridge)

    // set_color
    //   sets the `color` property from any native color model
    // (rgb, rgba, cmyk, hsl, ...).
    template <typename Color>
    void set_color(
        const Color& _c
    )
    {
        this->set_property(props::color, to_css_color(_c));
    }

    // set_background_color
    //   sets `background-color` from any native color model.
    template <typename Color>
    void set_background_color(
        const Color& _c
    )
    {
        this->set_property(props::background_color, to_css_color(_c));
    }

    // set_color_property
    //   sets an arbitrary color-valued property (border-color,
    // outline-color, ...) from any native color model.
    template <typename Color>
    void set_color_property(
        const DCssString&   _property,
        const Color&        _c
    )
    {
        this->set_property(_property, to_css_color(_c));
    }

    // set_font
    //   sets the `font` shorthand from a native djinterp::DFont.
    template <unsigned Feat, typename ColorT>
    void set_font(
        const ::djinterp::DFont<Feat, ColorT>& _f
    )
    {
        this->set_property(props::font, to_css_font(_f));
    }

    // set_font_longhand
    //   sets font-family / font-size / font-weight / font-style as
    // four separate declarations from a native djinterp::DFont.
    template <unsigned Feat, typename ColorT>
    void set_font_longhand(
        const ::djinterp::DFont<Feat, ColorT>& _f
    )
    {
        this->set_property(props::font_family, to_css_font_family(_f));
        this->set_property(props::font_size,   to_css_font_size(_f));
        this->set_property(props::font_weight, to_css_font_weight(_f));
        this->set_property(props::font_style,  to_css_font_style(_f));
    }


    /// nested rules

    // rule_count
    //   function: returns the count of nested non-declaration
    // children (i.e. nested rules).
    std::size_t rule_count() const
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
                 (c->rule_kind != DCssRuleKind::declaration_block) )
            {
                ++count;
            }
        }
        return count;
    }

    // rule_at
    //   function: returns the nested rule at index `_index`,
    // skipping declaration entries.
    DCssRule<Backend> rule_at(
        std::size_t _index
    ) const
    {
        if (m_node == nullptr)
        {
            return DCssRule<Backend>();
        }
        std::size_t seen = 0;
        for (std::size_t i = 0; i < m_node->children.size(); ++i)
        {
            node_type*  c = m_node->children[i].get();
            if ( (c != nullptr) &&
                 (c->rule_kind != DCssRuleKind::declaration_block) )
            {
                if (seen == _index)
                {
                    return DCssRule<Backend>(c);
                }
                ++seen;
            }
        }
        return DCssRule<Backend>();
    }

    // add_rule
    //   function: appends a fresh rule of the given kind.
    DCssRule<Backend> add_rule(
        DCssRuleKind _kind = DCssRuleKind::style_rule
    )
    {
        if (m_node == nullptr)
        {
            return DCssRule<Backend>();
        }
        std::unique_ptr<node_type>  r(new node_type);
        r->rule_kind = _kind;
        r->parent    = m_node;
        node_type* raw = r.get();
        m_node->children.push_back(std::move(r));
        return DCssRule<Backend>(raw);
    }


private:
    static
    const DCssString& s_empty()
    {
        static const DCssString e;
        return e;
    }

    static
    const std::vector<DCssString>& s_empty_vec()
    {
        static const std::vector<DCssString> e;
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
    inline
    void css_emit_indent_helper(
        std::ostream& _out,
        std::size_t   _depth,
        const char*   _indent
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
    inline
    void css_emit_declaration_helper(
        std::ostream&           _out,
        const DCssDefaultNode&  _decl,
        bool                    _minified
    )
    {
        _out << _decl.property << ':';
        if (!_minified)
        {
            _out << ' ';
        }
        _out << _decl.value;
        if (_decl.importance == ::djinterp::css::DCssImportance::important)
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
    inline
    void css_emit_rule_body_helper(
        std::ostream&           _out,
        const DCssDefaultNode&  _node,
        bool                    _minified,
        std::size_t             _depth,
        const char*             _indent,
        const char*             _newline,
        bool                    _scss_nesting
    )
    {
        using K = ::djinterp::css::DCssRuleKind;

        _out << (_minified ? "{" : "{")
             << (_minified ? "" : _newline);

        for (std::size_t i = 0; i < _node.children.size(); ++i)
        {
            const DCssDefaultNode* c = _node.children[i].get();
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
    inline
    void css_emit_rule_helper(
        std::ostream&           _out,
        const DCssDefaultNode&  _node,
        bool                    _minified,
        std::size_t             _depth,
        const char*             _indent,
        const char*             _newline,
        bool                    _scss_nesting
    )
    {
        using K = ::djinterp::css::DCssRuleKind;

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
    inline
    void css_emit_stylesheet_helper(
        std::ostream&           _out,
        const DCssDefaultNode&  _root,
        bool                    _minified,
        const char*             _indent,
        const char*             _newline,
        bool                    _scss_nesting
    )
    {
        for (std::size_t i = 0; i < _root.children.size(); ++i)
        {
            const DCssDefaultNode* c = _root.children[i].get();
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
///                V.   DCssStylesheet<Backend>                            ///
///////////////////////////////////////////////////////////////////////////////

// DCssStylesheet
//   class: facade over the backend's storage stylesheet type
// plus level / syntax-mode metadata. Owns the storage by value
// and exposes the render targets.
template<typename Backend>
class DCssStylesheet
{
public:
    using backend_type    = Backend;
    using stylesheet_type = typename Backend::stylesheet_type;
    using rule_node_type  = typename Backend::rule_type;
    using rule_facade     = DCssRule<Backend>;


    /// constructors

    DCssStylesheet()
    :   m_sheet(Backend::make_stylesheet())
    {}

    explicit
    DCssStylesheet(
        DCssLevel   _level
    )
    :   m_sheet(Backend::make_stylesheet())
    {
        m_sheet.level = _level;
    }

    DCssStylesheet(
        DCssLevel           _level,
        DCssSyntaxMode      _syntax
    )
    :   m_sheet(Backend::make_stylesheet())
    {
        m_sheet.level       = _level;
        m_sheet.syntax_mode = _syntax;
    }

    explicit
    DCssStylesheet(
        stylesheet_type&&   _sheet
    )
    :   m_sheet(std::move(_sheet))
    {}


    /// metadata

    DCssLevel level() const
    {
        return m_sheet.level;
    }

    DCssLevel get_level() const
    {
        return m_sheet.level;
    }

    void set_level(
        DCssLevel _level
    )
    {
        m_sheet.level = _level;
    }

    DCssSyntaxMode syntax_mode() const
    {
        return m_sheet.syntax_mode;
    }

    DCssSyntaxMode get_syntax_mode() const
    {
        return m_sheet.syntax_mode;
    }

    void set_syntax_mode(
        DCssSyntaxMode _mode
    )
    {
        m_sheet.syntax_mode = _mode;
    }


    /// root access

    rule_facade root_rule() const
    {
        return rule_facade(m_sheet.root.get());
    }

    const stylesheet_type& backend_stylesheet() const
    {
        return m_sheet;
    }


    /// rule traversal

    // rule_count
    //   function: returns the number of top-level rules.
    std::size_t rule_count() const
    {
        if (!m_sheet.root)
        {
            return 0;
        }
        return m_sheet.root->children.size();
    }

    // rule_at
    rule_facade rule_at(
        std::size_t _index
    ) const
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
    rule_facade add_style_rule(
        const DCssString& _selector
    )
    {
        if (!m_sheet.root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type> r(new rule_node_type);
        r->rule_kind = DCssRuleKind::style_rule;
        r->selector  = _selector;
        r->parent    = m_sheet.root.get();
        rule_node_type* raw = r.get();
        m_sheet.root->children.push_back(std::move(r));
        return rule_facade(raw);
    }

    // add_at_rule
    //   function: appends a new at-rule with the given
    // keyword and prelude.
    rule_facade add_at_rule(
        const DCssString& _keyword,
        const DCssString& _prelude = DCssString()
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
    void render_to_css(
        std::ostream& _out
    ) const
    {
        if (!m_sheet.root)
        {
            return;
        }
        const bool  scss_nesting =
            ( m_sheet.syntax_mode == DCssSyntaxMode::scss ||
              m_sheet.syntax_mode == DCssSyntaxMode::css_nesting );
        ::djinterp::internal::css_emit_stylesheet_helper(
            _out, *m_sheet.root,
            /*minified*/ false,
            D_CSS_DEFAULT_INDENT,
            D_CSS_DEFAULT_NEWLINE,
            scss_nesting);
    }

    DCssString render_to_css() const
    {
        std::ostringstream  oss;
        this->render_to_css(oss);
        return oss.str();
    }


    // render_to_minified_css
    //   function: emits this stylesheet with whitespace
    // stripped.
    void render_to_minified_css(
        std::ostream& _out
    ) const
    {
        if (!m_sheet.root)
        {
            return;
        }
        const bool  scss_nesting =
            ( m_sheet.syntax_mode == DCssSyntaxMode::scss ||
              m_sheet.syntax_mode == DCssSyntaxMode::css_nesting );
        ::djinterp::internal::css_emit_stylesheet_helper(
            _out, *m_sheet.root,
            /*minified*/ true,
            "", "",
            scss_nesting);
    }

    DCssString render_to_minified_css() const
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
    void render_to_scss(
        std::ostream& _out
    ) const
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

    DCssString render_to_scss() const
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
///                VI.   DCssDefaultBackend                                ///
///////////////////////////////////////////////////////////////////////////////

// DCssDefaultBackend
//   struct: bundled in-memory backend for the CSS facades.
// Uses a single underlying storage type for stylesheets,
// rules, and declarations.
struct DCssDefaultBackend
{
    using css_backend_tag = ::djinterp::css::DCssDefaultBackendTag;

    using rule_type        = ::djinterp::internal::DCssDefaultNode;
    using declaration_type = ::djinterp::internal::DCssDefaultNode;
    using stylesheet_type  = ::djinterp::internal::DCssDefaultStylesheet;

    // make_stylesheet
    //   function: factory for a fresh empty stylesheet with a
    // root `stylesheet` node ready to receive children.
    static
    stylesheet_type make_stylesheet()
    {
        stylesheet_type     s;
        s.level       = DCssLevel::css_3;
        s.syntax_mode = DCssSyntaxMode::css;
        std::unique_ptr<rule_type>  root(new rule_type);
        root->rule_kind = DCssRuleKind::stylesheet;
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
template<typename Backend>
inline
DCssStylesheet<Backend> make_stylesheet(
    DCssLevel       _level = DCssLevel::css_3,
    DCssSyntaxMode  _syntax = DCssSyntaxMode::css
)
{
    return DCssStylesheet<Backend>(_level, _syntax);
}


// make_style_rule
//   function: returns a new freestanding style rule with the
// given selector.
template<typename Backend>
inline
DCssRule<Backend> make_style_rule(
    const DCssString& _selector
)
{
    using node_t = typename Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = DCssRuleKind::style_rule;
    n->selector  = _selector;
    return DCssRule<Backend>(n);
}


// make_at_rule
//   function: returns a new freestanding at-rule with the
// given keyword and prelude.
template<typename Backend>
inline
DCssRule<Backend> make_at_rule(
    const DCssString& _keyword,
    const DCssString& _prelude = DCssString()
)
{
    using node_t = typename Backend::rule_type;
    node_t* n = new node_t;
    n->at_rule_kind = at_rule_kind_from_name(_keyword.c_str());
    n->rule_kind    = rule_kind_from_at_rule_kind(n->at_rule_kind);
    n->at_keyword   = _keyword;
    n->prelude      = _prelude;
    return DCssRule<Backend>(n);
}


// make_media_rule
//   function: convenience for `make_at_rule(at_rules::media,
// _query)`.
template<typename Backend>
inline
DCssRule<Backend> make_media_rule(
    const DCssString& _query
)
{
    return make_at_rule<Backend>(at_rules::media, _query);
}


// make_supports_rule
//   function: convenience for `@supports`.
template<typename Backend>
inline
DCssRule<Backend> make_supports_rule(
    const DCssString& _condition
)
{
    return make_at_rule<Backend>(at_rules::supports, _condition);
}


// make_keyframes_rule
//   function: convenience for `@keyframes <name>`.
template<typename Backend>
inline
DCssRule<Backend> make_keyframes_rule(
    const DCssString& _name
)
{
    return make_at_rule<Backend>(at_rules::keyframes, _name);
}


// make_font_face_rule
//   function: convenience for `@font-face`.
template<typename Backend>
inline
DCssRule<Backend> make_font_face_rule()
{
    return make_at_rule<Backend>(at_rules::font_face);
}


// make_import_rule
//   function: convenience for `@import url(...)`.
template<typename Backend>
inline
DCssRule<Backend> make_import_rule(
    const DCssString& _url
)
{
    return make_at_rule<Backend>(at_rules::import_, _url);
}


// make_charset_rule
//   function: convenience for `@charset "encoding"`.
template<typename Backend>
inline
DCssRule<Backend> make_charset_rule(
    const DCssString& _encoding
)
{
    DCssString    quoted;
    quoted.reserve(_encoding.size() + 2);
    quoted.push_back('"');
    quoted.append(_encoding);
    quoted.push_back('"');
    return make_at_rule<Backend>(at_rules::charset, quoted);
}


// make_comment
//   function: returns a new freestanding comment rule.
template<typename Backend>
inline
DCssRule<Backend> make_comment(
    const DCssString& _text
)
{
    using node_t = typename Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind    = DCssRuleKind::comment;
    n->comment_text = _text;
    return DCssRule<Backend>(n);
}


}   // namespace css
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_CSS_CSS_TEMPLATE_HPP
