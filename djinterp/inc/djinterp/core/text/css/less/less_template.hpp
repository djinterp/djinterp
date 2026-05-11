/******************************************************************************
* djinterp [less]                                            less_template.hpp
*
*   Templated Less rule and stylesheet facades and the bundled
* default backend. Mirrors `sass_template.hpp` but targets Less
* semantics: `@`-prefixed variables, callable-rule mixins, `when`
* guards, `:extend()` pseudo, import option flags, namespaces,
* detached rulesets.
*
*   STORAGE MODEL:
*   For the default backend, the storage type is the CSS default
* node (`css::internal::css_default_node`). Less extension state
* fits into the same fields the CSS / Sass backends already use --
* variable name stored in `property` (with `@` prefix), variable
* value in `value`, mixin parameters in `prelude`, guard clause
* appended to `prelude`, import options in `prelude`, etc.
*
*   FACADES:
*   - `less_rule<_Backend>`       extends `css::css_rule<_Backend>`
*   - `less_stylesheet<_Backend>` extends `css::css_stylesheet<_Backend>`
*
*   ZERO ADDED MEMBERS:
*   `less_rule<B>` adds NO members beyond `css_rule<B>`.
*   `less_stylesheet<B>` adds NO members beyond `css_stylesheet<B>`.
*   Memory layout matches the CSS bases exactly.
*
*   RENDERING:
*   `less_stylesheet` exposes:
*     - render_to_less_source  (Less source)
*     - compile_to_css         (stub; real compilation belongs to
*                               adapter backends bound to less.js
*                               or less-cpp)
*
*
* path:      /inc/djinterp/core/util/less/less_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    less_rule<_Backend>
II.   INTERNAL EMISSION HELPERS
III.  less_stylesheet<_Backend>
IV.   less_default_backend
V.    FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_LESS_TEMPLATE_
#define DJINTERP_LESS_TEMPLATE_ 1

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
#include "../css/css_template.hpp"


NS_DJINTERP

namespace less {


///////////////////////////////////////////////////////////////////////////////
///                I.   less_rule<_Backend>                                 ///
///////////////////////////////////////////////////////////////////////////////

// less_rule
//   class: thin facade extending `css::css_rule<_Backend>`
// with Less-specific accessors. The Less kind discriminator is
// derived from the underlying CSS rule kind plus inspection of
// the property name (for variables) / selector shape (for
// mixins) / at-keyword (for plugin and option directives) /
// prelude content (for guards and imports).
template<typename _Backend>
class less_rule
:   public ::djinterp::css::css_rule<_Backend>
{
public:
    using base_type = ::djinterp::css::css_rule<_Backend>;
    using node_type = typename base_type::node_type;


    /// constructors

    less_rule()
    :   base_type()
    {}

    explicit
    less_rule(
        node_type*  _node
    )
    :   base_type(_node)
    {}

    explicit
    less_rule(
        const base_type&    _css
    )
    :   base_type(_css)
    {}


    /// Less kind dispatch

    // less_rule_kind
    //   function: returns the Less-specific rule kind.
    enum less_rule_kind
    less_rule_kind() const
    {
        if (!this->valid())
        {
            return less_rule_kind::unknown;
        }

        // Variable declaration: a CSS declaration whose
        // property name begins with `@`.
        if ( (this->rule_kind() ==
                ::djinterp::css::css_rule_kind::declaration_block) &&
             is_less_variable_name(this->backend_handle()->property.c_str()) )
        {
            // Detached ruleset: the value is a `{ ... }` block.
            // The default backend cannot tell statically;
            // adapter backends override.
            return less_rule_kind::variable_declaration;
        }

        // Property reference (Less 3.5+): `$name` inside a
        // value expression. Stored as a declaration with
        // property name beginning with `$`.
        if ( (this->rule_kind() ==
                ::djinterp::css::css_rule_kind::declaration_block) &&
             is_less_property_variable_name(
                 this->backend_handle()->property.c_str()) )
        {
            return less_rule_kind::property_variable;
        }

        // Style rule whose selector looks like a mixin
        // callable -- `.name` or `#name`.
        if ( (this->rule_kind() ==
                ::djinterp::css::css_rule_kind::style_rule) &&
             is_less_mixin_selector(this->selector().c_str()) )
        {
            // Distinguish definition from call by body presence
            // (call has no body when used as a statement; the
            // default backend uses children-count as the proxy).
            if (this->declaration_count() + this->rule_count() > 0)
            {
                // body present -- definition
                if (this->selector().find('(') != less_string_t::npos)
                {
                    return less_rule_kind::parametric_mixin;
                }
                return less_rule_kind::mixin_definition;
            }
            // no body -- mixin call statement (rarely useful
            // when stored as a style_rule; included for
            // completeness)
            return less_rule_kind::mixin_call;
        }

        // At-rules: classify Less-specific keywords.
        if (this->is_at_rule())
        {
            const less_at_rule_kind   ak =
                less_at_rule_kind_from_name(
                    this->at_keyword().c_str());
            if (ak == less_at_rule_kind::plugin)
            {
                return less_rule_kind::plugin_import;
            }
            if (ak == less_at_rule_kind::import_)
            {
                // Less import always carries options syntax;
                // distinguish "vanilla CSS import" from
                // "Less import" by presence of option keywords
                // in the prelude. Heuristic; adapter backends
                // can override.
                if (this->prelude().find('(') != less_string_t::npos)
                {
                    return less_rule_kind::import_with_options;
                }
            }
        }

        return less_rule_kind::unknown;
    }

    enum less_rule_kind
    get_less_rule_kind() const
    {
        return this->less_rule_kind();
    }

    bool is_variable_declaration() const
    {
        return ( this->less_rule_kind() == less_rule_kind::variable_declaration );
    }
    bool is_mixin_definition() const
    {
        const auto k = this->less_rule_kind();
        return ( (k == less_rule_kind::mixin_definition) ||
                 (k == less_rule_kind::parametric_mixin) );
    }
    bool is_mixin_call() const
    {
        return ( this->less_rule_kind() == less_rule_kind::mixin_call );
    }
    bool is_parametric_mixin() const
    {
        return ( this->less_rule_kind() == less_rule_kind::parametric_mixin );
    }
    bool is_plugin_import() const
    {
        return ( this->less_rule_kind() == less_rule_kind::plugin_import );
    }
    bool is_import_with_options() const
    {
        return ( this->less_rule_kind() == less_rule_kind::import_with_options );
    }


    /// variable accessors

    // variable_name
    //   function: returns the variable name (including the
    // leading `@`). Only meaningful for variable declarations.
    less_string_t
    variable_name() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        return this->backend_handle()->property;
    }

    less_string_t
    get_variable_name() const
    {
        return this->variable_name();
    }

    // set_variable_name
    void
    set_variable_name(
        const less_string_t&    _name
    )
    {
        if (!this->valid())
        {
            return;
        }
        this->backend_handle()->property =
            ( (!_name.empty()) && (_name[0] != '@') )
                ? less_string_t("@") + _name
                : _name;
    }


    /// mixin accessors

    // mixin_selector
    //   function: returns the mixin selector portion (before
    // any parameter list).
    less_string_t
    mixin_selector() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    sel = this->selector();
        const std::size_t       p   = sel.find('(');
        return (p == less_string_t::npos) ? sel : sel.substr(0, p);
    }

    less_string_t
    get_mixin_selector() const
    {
        return this->mixin_selector();
    }

    // parameters
    //   function: returns the parameter-list source between
    // the parentheses on a mixin definition selector.
    less_string_t
    parameters() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    sel  = this->selector();
        const std::size_t       open = sel.find('(');
        // close on the matching ')' before any subsequent `when`
        const std::size_t       close = sel.rfind(')');
        if ( (open == less_string_t::npos) ||
             (close == less_string_t::npos) ||
             (close <= open + 1) )
        {
            return less_string_t();
        }
        return sel.substr(open + 1, close - open - 1);
    }

    less_string_t
    get_parameters() const
    {
        return this->parameters();
    }

    // arguments
    //   function: alias of `parameters()` for mixin calls --
    // the syntactic form is identical.
    less_string_t
    arguments() const
    {
        return this->parameters();
    }


    /// guard accessor

    // guard_clause
    //   function: returns the guard expression for a guarded
    // mixin. For a selector like `.dark(@color) when (lightness(
    // @color) < 50%)`, returns `lightness(@color) < 50%`.
    less_string_t
    guard_clause() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    sel = this->selector();
        const std::size_t       w   = sel.find(" when ");
        if (w == less_string_t::npos)
        {
            return less_string_t();
        }
        // Skip "when ", grab the parenthesised condition.
        const std::size_t       after = w + 6;
        const std::size_t       open  = sel.find('(', after);
        const std::size_t       close = sel.rfind(')');
        if ( (open == less_string_t::npos) ||
             (close == less_string_t::npos) ||
             (close <= open + 1) )
        {
            return sel.substr(after);
        }
        return sel.substr(open + 1, close - open - 1);
    }

    less_string_t
    get_guard_clause() const
    {
        return this->guard_clause();
    }

    // is_guarded
    //   function: true if the rule carries a `when (...)`
    // guard clause.
    bool
    is_guarded() const
    {
        if (!this->valid())
        {
            return false;
        }
        return ( this->selector().find(" when ") != less_string_t::npos );
    }


    /// extend accessors

    // extend_target
    //   function: returns the target of a `&:extend(target)`
    // pseudo. Returns empty if no extend is present in the
    // selector.
    less_string_t
    extend_target() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    sel = this->selector();
        const std::size_t       e   = sel.find(":extend(");
        if (e == less_string_t::npos)
        {
            return less_string_t();
        }
        const std::size_t       open  = e + 7;
        const std::size_t       close = sel.find(')', open);
        if (close == less_string_t::npos)
        {
            return less_string_t();
        }
        less_string_t           inner = sel.substr(open + 1, close - open - 1);
        // strip trailing " all" if present
        const std::size_t       all_at = inner.rfind(" all");
        if ( (all_at != less_string_t::npos) &&
             (all_at + 4 == inner.size()) )
        {
            inner.resize(all_at);
        }
        return inner;
    }

    // extend_all
    //   function: true if `&:extend(target all)` was specified.
    bool
    extend_all() const
    {
        if (!this->valid())
        {
            return false;
        }
        const less_string_t&    sel = this->selector();
        return ( sel.find(" all)") != less_string_t::npos );
    }


    /// import accessors

    // import_url
    //   function: returns the URL string from `@import 'name'`.
    less_string_t
    import_url() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    pre = this->prelude();
        // skip leading `(options)` group if present
        std::size_t             i   = 0;
        const std::size_t       n   = pre.size();
        if ( (i < n) && (pre[i] == '(') )
        {
            const std::size_t   close = pre.find(')', i);
            if (close == less_string_t::npos)
            {
                return less_string_t();
            }
            i = close + 1;
            while ( (i < n) && ((pre[i] == ' ') || (pre[i] == '\t')) )
            {
                ++i;
            }
        }
        if (i >= n)
        {
            return less_string_t();
        }
        // strip surrounding quotes if present
        char    quote = pre[i];
        if ( (quote == '\'') || (quote == '"') )
        {
            const std::size_t   close = pre.find(quote, i + 1);
            if (close == less_string_t::npos)
            {
                return pre.substr(i + 1);
            }
            return pre.substr(i + 1, close - i - 1);
        }
        // unquoted form (url(...)?); take rest until whitespace
        const std::size_t       sp = pre.find_first_of(" \t", i);
        return (sp == less_string_t::npos) ? pre.substr(i)
                                           : pre.substr(i, sp - i);
    }

    // import_options
    //   function: returns the bitmask of `@import` option flags.
    unsigned
    import_options() const
    {
        if (!this->valid())
        {
            return lio_none;
        }
        const less_string_t&    pre = this->prelude();
        if (pre.empty() || (pre[0] != '('))
        {
            return lio_none;
        }
        const std::size_t       close = pre.find(')');
        if (close == less_string_t::npos)
        {
            return lio_none;
        }
        const less_string_t     opts = pre.substr(1, close - 1);
        unsigned                mask = lio_none;
        if (opts.find("reference") != less_string_t::npos) mask |= lio_reference;
        if (opts.find("inline")    != less_string_t::npos) mask |= lio_inline_;
        if (opts.find("less")      != less_string_t::npos) mask |= lio_less;
        if (opts.find("css")       != less_string_t::npos) mask |= lio_css;
        if (opts.find("once")      != less_string_t::npos) mask |= lio_once;
        if (opts.find("multiple")  != less_string_t::npos) mask |= lio_multiple;
        if (opts.find("optional")  != less_string_t::npos) mask |= lio_optional;
        return mask;
    }

    // plugin_url
    //   function: returns the URL from `@plugin 'name'`.
    less_string_t
    plugin_url() const
    {
        if (!this->valid())
        {
            return less_string_t();
        }
        const less_string_t&    pre = this->prelude();
        if (pre.size() < 2)
        {
            return less_string_t();
        }
        char    quote = pre[0];
        if ( (quote == '\'') || (quote == '"') )
        {
            const std::size_t   close = pre.find(quote, 1);
            if (close == less_string_t::npos)
            {
                return pre.substr(1);
            }
            return pre.substr(1, close - 1);
        }
        return pre;
    }


private:
    // (no extra members)
};


///////////////////////////////////////////////////////////////////////////////
///                II.   INTERNAL EMISSION HELPERS                          ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace less
NS_INTERNAL

    // less_emit_indent_helper
    //   function: writes `_depth` repetitions of the indent
    // string to `_out`.
    inline void
    less_emit_indent_helper(
        std::ostream&       _out,
        std::size_t         _depth
    )
    {
        for (std::size_t i = 0; i < _depth; ++i)
        {
            _out << "  ";
        }
    }


    // less_emit_rule_helper
    //   function: emits a single rule (and its descendants)
    // as Less source. Recursive for grouping rules and nested
    // style rules.
    inline void
    less_emit_rule_helper(
        std::ostream&                                                   _out,
        const ::djinterp::internal::css_default_node&                   _node,
        std::size_t                                                     _depth
    )
    {
        using K = ::djinterp::css::css_rule_kind;

        if (_node.rule_kind == K::comment)
        {
            less_emit_indent_helper(_out, _depth);
            _out << "/*" << _node.comment_text << "*/\n";
            return;
        }

        if (_node.rule_kind == K::declaration_block)
        {
            less_emit_indent_helper(_out, _depth);
            _out << _node.property << ": " << _node.value;
            if (_node.importance == ::djinterp::css::css_importance::important)
            {
                _out << " !important";
            }
            _out << ";\n";
            return;
        }

        if (_node.rule_kind == K::style_rule)
        {
            less_emit_indent_helper(_out, _depth);
            _out << _node.selector << " {\n";
            for (std::size_t i = 0; i < _node.children.size(); ++i)
            {
                if (_node.children[i])
                {
                    less_emit_rule_helper(
                        _out, *_node.children[i], _depth + 1);
                }
            }
            less_emit_indent_helper(_out, _depth);
            _out << "}\n";
            return;
        }

        if (::djinterp::css::is_at_rule_kind(_node.rule_kind))
        {
            less_emit_indent_helper(_out, _depth);
            _out << '@' << _node.at_keyword;
            if (!_node.prelude.empty())
            {
                _out << ' ' << _node.prelude;
            }
            const bool  has_body =
                ( ::djinterp::css::is_grouping_rule_kind(_node.rule_kind) ||
                  (!_node.children.empty()) );
            if (has_body)
            {
                _out << " {\n";
                for (std::size_t i = 0; i < _node.children.size(); ++i)
                {
                    if (_node.children[i])
                    {
                        less_emit_rule_helper(
                            _out, *_node.children[i], _depth + 1);
                    }
                }
                less_emit_indent_helper(_out, _depth);
                _out << "}\n";
            }
            else
            {
                _out << ";\n";
            }
            return;
        }
    }

NS_END  // internal
namespace less {


///////////////////////////////////////////////////////////////////////////////
///                III.   less_stylesheet<_Backend>                         ///
///////////////////////////////////////////////////////////////////////////////

// less_stylesheet
//   class: thin facade extending
// `css::css_stylesheet<_Backend>` with Less-specific metadata
// and emission. Adds NO data members beyond the CSS base.
template<typename _Backend>
class less_stylesheet
:   public ::djinterp::css::css_stylesheet<_Backend>
{
public:
    using base_type       = ::djinterp::css::css_stylesheet<_Backend>;
    using stylesheet_type = typename base_type::stylesheet_type;
    using rule_node_type  = typename base_type::rule_node_type;
    using rule_facade     = less_rule<_Backend>;


    /// constructors

    less_stylesheet()
    :   base_type()
    {
        this->set_syntax_mode(::djinterp::css::css_syntax_mode::css);
    }


    /// metadata

    // less_dialect
    //   function: returns the configured dialect. Default
    // backend reports `less_js`; adapter backends can override.
    enum less_dialect
    less_dialect() const
    {
        return less_dialect::less_js;
    }

    enum less_dialect
    get_less_dialect() const
    {
        return this->less_dialect();
    }


    /// rule traversal (returns Less facades)

    rule_facade
    less_rule_at(
        std::size_t     _index
    )   const
    {
        auto    css_r = this->rule_at(_index);
        return rule_facade(css_r.backend_handle());
    }


    /// Less-specific authoring helpers

    // add_variable
    //   function: appends a variable declaration to the
    // stylesheet root (`@name: value;`).
    rule_facade
    add_variable(
        const less_string_t&    _name,
        const less_string_t&    _value
    )
    {
        if (!this->backend_stylesheet().root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type>     n(new rule_node_type);
        n->rule_kind = ::djinterp::css::css_rule_kind::declaration_block;
        n->property  = ( (!_name.empty()) && (_name[0] == '@') )
            ? _name
            : less_string_t("@") + _name;
        n->value = _value;
        rule_node_type* raw = n.get();
        const_cast<typename base_type::stylesheet_type&>(
            this->backend_stylesheet()).root->children.push_back(std::move(n));
        return rule_facade(raw);
    }

    // add_mixin
    //   function: appends a mixin definition.
    rule_facade
    add_mixin(
        const less_string_t&    _selector,
        const less_string_t&    _parameters = less_string_t(),
        const less_string_t&    _guard      = less_string_t()
    )
    {
        if (!this->backend_stylesheet().root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type>     n(new rule_node_type);
        n->rule_kind = ::djinterp::css::css_rule_kind::style_rule;
        n->selector  = _selector;
        if ( (!_parameters.empty()) ||
             (_selector.find('(') == less_string_t::npos) )
        {
            // Always emit parens for mixin definitions so the
            // round-trip is unambiguous.
            n->selector += '(';
            n->selector += _parameters;
            n->selector += ')';
        }
        if (!_guard.empty())
        {
            n->selector += " when (";
            n->selector += _guard;
            n->selector += ')';
        }
        rule_node_type* raw = n.get();
        const_cast<typename base_type::stylesheet_type&>(
            this->backend_stylesheet()).root->children.push_back(std::move(n));
        return rule_facade(raw);
    }

    // add_import
    //   function: appends `@import (opts) 'url';`.
    rule_facade
    add_import(
        const less_string_t&    _url,
        unsigned                _options = lio_none
    )
    {
        less_string_t   prelude;
        if (_options != lio_none)
        {
            prelude.push_back('(');
            bool first = true;
            const auto append = [&first, &prelude](const char* name)
            {
                if (!first) { prelude.push_back(','); }
                prelude += name;
                first = false;
            };
            if (_options & lio_reference) append("reference");
            if (_options & lio_inline_)   append("inline");
            if (_options & lio_less)      append("less");
            if (_options & lio_css)       append("css");
            if (_options & lio_once)      append("once");
            if (_options & lio_multiple)  append("multiple");
            if (_options & lio_optional)  append("optional");
            prelude.push_back(')');
            prelude.push_back(' ');
        }
        prelude.push_back('\'');
        prelude += _url;
        prelude.push_back('\'');
        auto css_r = this->add_at_rule(at_keywords::import_, prelude);
        return rule_facade(css_r.backend_handle());
    }

    // add_plugin
    //   function: appends `@plugin 'url';`.
    rule_facade
    add_plugin(
        const less_string_t&    _url
    )
    {
        less_string_t   prelude = less_string_t("'") + _url + less_string_t("'");
        auto css_r = this->add_at_rule(at_keywords::plugin, prelude);
        return rule_facade(css_r.backend_handle());
    }


    /// rendering

    // render_to_less_source
    //   function: emits this stylesheet as Less source.
    void
    render_to_less_source(
        std::ostream&   _out
    )   const
    {
        const auto& doc = this->backend_stylesheet();
        if (!doc.root)
        {
            return;
        }
        for (std::size_t i = 0; i < doc.root->children.size(); ++i)
        {
            if (doc.root->children[i])
            {
                ::djinterp::internal::less_emit_rule_helper(
                    _out, *doc.root->children[i], 0);
            }
        }
    }

    less_string_t
    render_to_less_source() const
    {
        std::ostringstream  oss;
        this->render_to_less_source(oss);
        return oss.str();
    }

    // compile_to_css
    //   function: stub. Real Less compilation requires the
    // full evaluator (variable resolution, mixin expansion,
    // guard evaluation, extension graph resolution, color /
    // math operators, plugin invocation). The default backend
    // cannot do this; adapter backends bound to less.js or
    // less-cpp provide real compilation. The default falls
    // back to emitting the Less source unchanged.
    void
    compile_to_css(
        std::ostream&   _out
    )   const
    {
        this->render_to_less_source(_out);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.   less_default_backend                               ///
///////////////////////////////////////////////////////////////////////////////

// less_default_backend
//   struct: bundled in-memory backend for the Less facades.
// Reuses the CSS default backend's storage types verbatim.
// Exposes the CSS and Less backend tags so the same type
// satisfies both protocols.
struct less_default_backend
{
    using css_backend_tag  = ::djinterp::css::css_default_backend_tag;
    using less_backend_tag = ::djinterp::less::less_default_backend_tag;

    using rule_type             = ::djinterp::internal::css_default_node;
    using declaration_type      = ::djinterp::internal::css_default_node;
    using stylesheet_type       = ::djinterp::internal::css_default_stylesheet;
    using less_rule_type        = ::djinterp::internal::css_default_node;
    using less_stylesheet_type  = ::djinterp::internal::css_default_stylesheet;

    static stylesheet_type
    make_stylesheet()
    {
        return ::djinterp::css::css_default_backend::make_stylesheet();
    }

    // make_less_stylesheet
    //   function: factory matching the Less backend protocol.
    static stylesheet_type
    make_less_stylesheet()
    {
        return make_stylesheet();
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   FREE HELPERS / FACTORIES                            ///
///////////////////////////////////////////////////////////////////////////////

// make_less_stylesheet
//   function: factory returning a freshly-built Less
// stylesheet facade for the given backend.
template<typename _Backend>
inline less_stylesheet<_Backend>
make_less_stylesheet()
{
    return less_stylesheet<_Backend>();
}


// make_variable
//   function: returns a new freestanding variable declaration.
template<typename _Backend>
inline less_rule<_Backend>
make_variable(
    const less_string_t&    _name,
    const less_string_t&    _value
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = ::djinterp::css::css_rule_kind::declaration_block;
    n->property  = ( (!_name.empty()) && (_name[0] == '@') )
        ? _name
        : less_string_t("@") + _name;
    n->value = _value;
    return less_rule<_Backend>(n);
}


// make_mixin
//   function: returns a new freestanding mixin definition.
template<typename _Backend>
inline less_rule<_Backend>
make_mixin(
    const less_string_t&    _selector,
    const less_string_t&    _parameters = less_string_t(),
    const less_string_t&    _guard      = less_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = ::djinterp::css::css_rule_kind::style_rule;
    n->selector  = _selector;
    n->selector += '(';
    n->selector += _parameters;
    n->selector += ')';
    if (!_guard.empty())
    {
        n->selector += " when (";
        n->selector += _guard;
        n->selector += ')';
    }
    return less_rule<_Backend>(n);
}


// make_mixin_call
//   function: returns a new freestanding mixin call statement.
template<typename _Backend>
inline less_rule<_Backend>
make_mixin_call(
    const less_string_t&    _selector,
    const less_string_t&    _arguments = less_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = ::djinterp::css::css_rule_kind::style_rule;
    n->selector  = _selector;
    n->selector += '(';
    n->selector += _arguments;
    n->selector += ')';
    return less_rule<_Backend>(n);
}


// make_import
//   function: returns a new freestanding @import rule.
template<typename _Backend>
inline less_rule<_Backend>
make_import(
    const less_string_t&    _url,
    unsigned                _options = lio_none
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::import_;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    less_string_t&  pre = n->prelude;
    if (_options != lio_none)
    {
        pre.push_back('(');
        bool first = true;
        const auto append = [&first, &pre](const char* name)
        {
            if (!first) { pre.push_back(','); }
            pre += name;
            first = false;
        };
        if (_options & lio_reference) append("reference");
        if (_options & lio_inline_)   append("inline");
        if (_options & lio_less)      append("less");
        if (_options & lio_css)       append("css");
        if (_options & lio_once)      append("once");
        if (_options & lio_multiple)  append("multiple");
        if (_options & lio_optional)  append("optional");
        pre.push_back(')');
        pre.push_back(' ');
    }
    pre.push_back('\'');
    pre += _url;
    pre.push_back('\'');
    return less_rule<_Backend>(n);
}


// make_plugin
//   function: returns a new freestanding @plugin rule.
template<typename _Backend>
inline less_rule<_Backend>
make_plugin(
    const less_string_t&    _url
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::plugin;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = less_string_t("'") + _url + less_string_t("'");
    return less_rule<_Backend>(n);
}


}   // namespace less
NS_END  // djinterp


#endif  // DJINTERP_LESS_TEMPLATE_
