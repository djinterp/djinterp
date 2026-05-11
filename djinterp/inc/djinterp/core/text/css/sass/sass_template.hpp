/******************************************************************************
* djinterp [sass]                                            sass_template.hpp
*
*   Templated Sass / SCSS rule and stylesheet facades and the
* bundled default backend. Mirrors `xml_template.hpp` /
* `html_template.hpp` / `css_template.hpp` but extends the CSS
* facades with Sass-specific surface (variable / mixin / include /
* extend / control flow / module accessors) layered on top of the
* CSS protocol.
*
*   STORAGE MODEL:
*   For the default backend, the storage type is the CSS default
* node (`css::internal::css_default_node`) augmented with Sass
* fields stored in a sibling struct (`internal::sass_default_extra`)
* attached via the parent / child relationship. This keeps the CSS
* node compatible with the Sass facades; adapter backends (libsass,
* dart-sass IPC) are free to use distinct types.
*
*   FACADES:
*   - `sass_rule<_Backend>`       extends `css::css_rule<_Backend>`
*   - `sass_stylesheet<_Backend>` extends `css::css_stylesheet<_Backend>`
*   The Sass-specific information (variable name, mixin name,
* parameters, condition, etc.) is read from / written to optional
* fields on the same backend node. This keeps memory layout
* unchanged below the extension surface.
*
*   ZERO ADDED MEMBERS:
*   `sass_rule<B>` adds NO members beyond `css_rule<B>`.
*   `sass_stylesheet<B>` adds NO members beyond `css_stylesheet<B>`.
*   The Sass-specific fields live in the backend node itself
* (which the default backend stores in unused fields of the CSS
* default node), so the facades remain pointer-only / by-value
* with the same footprint as their CSS bases.
*
*   RENDERING:
*   `sass_stylesheet` exposes:
*     - render_to_scss_source  (SCSS source, semicolons + braces)
*     - render_to_sass_source  (Sass indented source)
*     - compile_to_css         (stub; real compilation is delegated
*                               to adapter backends bound to libsass
*                               or sass-embedded)
*
*
* path:      /inc/djinterp/core/util/sass/sass_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.10
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DEFAULT BACKEND STORAGE EXTENSION
II.   sass_rule<_Backend>
III.  INTERNAL EMISSION HELPERS
IV.   sass_stylesheet<_Backend>
V.    sass_default_backend
VI.   FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_SASS_TEMPLATE_
#define DJINTERP_SASS_TEMPLATE_ 1

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

namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                I.   DEFAULT BACKEND STORAGE EXTENSION                   ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace sass
NS_INTERNAL

    // sass_default_extra
    //   struct: optional Sass extension fields stored alongside
    // the CSS default node. Carried as an optional sidecar
    // pointer on the CSS node's parent map so that the CSS
    // node layout itself does not change. The default backend
    // simply reuses CSS-node fields where it can (selector for
    // mixin name, prelude for parameter list, value for variable
    // value, etc.) and falls back to fields here for state that
    // does not fit any CSS slot.
    //
    //   For this baseline implementation, the Sass extension
    // state fits entirely into existing CSS-node fields:
    //     - sass rule kind: derived from rule_kind + at_keyword
    //     - variable name: stored in `selector` (with $ prefix)
    //     - variable value: stored in `value`
    //     - !default flag: importance == important is reused
    //         (importance is otherwise meaningless on a Sass
    //         variable declaration)
    //     - mixin / function name + params: stored in `prelude`
    //     - include target + args: stored in `prelude`
    //     - extend target: stored in `prelude`
    //     - control flow condition / loop binding: stored in
    //         `prelude`
    //     - module url + namespace + config: stored in `prelude`
    //
    //   This sidecar struct is reserved for future use when an
    // adapter backend wants to surface richer parsed state.
    struct sass_default_extra
    {
        ::djinterp::sass::sass_rule_kind        sass_kind =
            ::djinterp::sass::sass_rule_kind::unknown;
        bool                                    is_default = false;
        bool                                    is_global  = false;
        bool                                    accepts_content_block = false;
        bool                                    extend_optional = false;
        std::string                             namespace_alias;
        std::vector<std::string>                parameters;
        std::vector<std::string>                arguments;
    };

NS_END  // internal
namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                II.   sass_rule<_Backend>                                ///
///////////////////////////////////////////////////////////////////////////////

// sass_rule
//   class: thin facade extending `css::css_rule<_Backend>`
// with Sass-specific accessors. The Sass kind discriminator
// is derived from the underlying CSS rule kind plus the
// at-keyword string -- no extra storage required for the
// default backend.
template<typename _Backend>
class sass_rule
:   public ::djinterp::css::css_rule<_Backend>
{
public:
    // base_type
    //   type: the underlying CSS rule facade.
    using base_type = ::djinterp::css::css_rule<_Backend>;

    // node_type
    //   type: the backend's rule storage type, inherited from
    // the CSS facade.
    using node_type = typename base_type::node_type;


    /// constructors

    sass_rule()
    :   base_type()
    {}

    explicit
    sass_rule(
        node_type*  _node
    )
    :   base_type(_node)
    {}

    explicit
    sass_rule(
        const base_type&    _css
    )
    :   base_type(_css)
    {}


    /// Sass kind dispatch

    // sass_rule_kind
    //   function: returns the Sass-specific rule kind. Derived
    // from the underlying CSS rule kind plus the at-keyword
    // string, so the same backend node can be classified by
    // either layer.
    sass_rule_kind
    sass_rule_kind() const
    {
        if (!this->valid())
        {
            return sass_rule_kind::unknown;
        }
        // variable declaration: a CSS declaration whose
        // property name begins with `$`.
        if ( (this->rule_kind() == ::djinterp::css::css_rule_kind::declaration_block) &&
             is_sass_variable_name(this->backend_handle()->property.c_str()) )
        {
            return sass_rule_kind::variable_declaration;
        }
        // style rule whose selector begins with `%` is a
        // placeholder.
        if ( (this->rule_kind() == ::djinterp::css::css_rule_kind::style_rule) &&
             is_sass_placeholder_name(this->selector().c_str()) )
        {
            return sass_rule_kind::placeholder_rule;
        }
        // at-rules: classify by keyword.
        if (this->is_at_rule())
        {
            return sass_rule_kind_from_at_rule_kind(
                sass_at_rule_kind_from_name(
                    this->at_keyword().c_str()));
        }
        return sass_rule_kind::unknown;
    }

    // get_sass_rule_kind
    //   function: getter-form alias.
    enum sass_rule_kind
    get_sass_rule_kind() const
    {
        return this->sass_rule_kind();
    }

    // sass_at_rule_kind
    //   function: returns the Sass-specific at-rule kind.
    enum sass_at_rule_kind
    sass_at_rule_kind() const
    {
        if (!this->valid())
        {
            return sass_at_rule_kind::unknown;
        }
        return sass_at_rule_kind_from_name(this->at_keyword().c_str());
    }

    bool is_variable_declaration() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::variable_declaration );
    }
    bool is_mixin_declaration() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::mixin_declaration );
    }
    bool is_function_declaration() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::function_declaration );
    }
    bool is_include_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::include_statement );
    }
    bool is_extend_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::extend_statement );
    }
    bool is_if_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::if_statement );
    }
    bool is_each_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::each_statement );
    }
    bool is_for_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::for_statement );
    }
    bool is_while_statement() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::while_statement );
    }
    bool is_use_rule() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::use_rule );
    }
    bool is_forward_rule() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::forward_rule );
    }
    bool is_at_root_rule() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::at_root_rule );
    }
    bool is_placeholder_rule() const
    {
        return ( this->sass_rule_kind() == sass_rule_kind::placeholder_rule );
    }


    /// variable accessors

    // variable_name
    //   function: returns the variable name (including the
    // leading `$`). Only meaningful when
    // `is_variable_declaration() == true`.
    sass_string_t
    variable_name() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        // For Sass variable declarations the default backend
        // stores the variable name in the property field.
        return this->backend_handle()->property;
    }

    sass_string_t
    get_variable_name() const
    {
        return this->variable_name();
    }

    // set_variable_name
    void
    set_variable_name(
        const sass_string_t&    _name
    )
    {
        if (!this->valid())
        {
            return;
        }
        if ( (!_name.empty()) && (_name[0] != '$') )
        {
            this->backend_handle()->property = sass_string_t("$") + _name;
        }
        else
        {
            this->backend_handle()->property = _name;
        }
    }


    /// mixin / function accessors

    // mixin_name
    //   function: returns the mixin name. For default-backend
    // storage, mixin name is encoded as the at-rule prelude
    // up to the first `(`.
    sass_string_t
    mixin_name() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        const sass_string_t&    pre = this->prelude();
        const std::size_t       p   = pre.find('(');
        return (p == sass_string_t::npos) ? pre : pre.substr(0, p);
    }

    sass_string_t
    get_mixin_name() const
    {
        return this->mixin_name();
    }

    // function_name
    //   function: alias of `mixin_name` -- @function and @mixin
    // share the same default-backend encoding.
    sass_string_t
    function_name() const
    {
        return this->mixin_name();
    }

    sass_string_t
    get_function_name() const
    {
        return this->mixin_name();
    }

    // parameters
    //   function: returns the parameter-list source between
    // the parentheses, e.g. `($x, $y: 10px)` -> `$x, $y: 10px`.
    sass_string_t
    parameters() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        const sass_string_t&    pre = this->prelude();
        const std::size_t       open  = pre.find('(');
        const std::size_t       close = pre.rfind(')');
        if ( (open == sass_string_t::npos) ||
             (close == sass_string_t::npos) ||
             (close <= open + 1) )
        {
            return sass_string_t();
        }
        return pre.substr(open + 1, close - open - 1);
    }

    sass_string_t
    get_parameters() const
    {
        return this->parameters();
    }


    /// include / extend accessors

    // include_target
    //   function: returns the mixin name being included
    // (without the argument list).
    sass_string_t
    include_target() const
    {
        return this->mixin_name();
    }

    // arguments
    //   function: alias of `parameters()` -- the syntactic form
    // is identical for default-backend storage.
    sass_string_t
    arguments() const
    {
        return this->parameters();
    }

    // extend_target
    //   function: returns the selector being extended.
    sass_string_t
    extend_target() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        // Trim any trailing `!optional` flag.
        sass_string_t   p = this->prelude();
        const std::size_t   bang = p.find("!optional");
        if (bang != sass_string_t::npos)
        {
            // strip the flag and any preceding whitespace
            std::size_t end = bang;
            while ( (end > 0) &&
                    ( (p[end - 1] == ' ') || (p[end - 1] == '\t') ) )
            {
                --end;
            }
            return p.substr(0, end);
        }
        return p;
    }

    // extend_optional
    //   function: true if the @extend statement carries
    // `!optional`.
    bool
    extend_optional() const
    {
        if (!this->valid())
        {
            return false;
        }
        return ( this->prelude().find("!optional") != sass_string_t::npos );
    }


    /// control flow accessors

    // condition
    //   function: returns the condition expression for @if /
    // @while statements.
    sass_string_t
    condition() const
    {
        return this->valid() ? this->prelude() : sass_string_t();
    }

    sass_string_t
    get_condition() const
    {
        return this->condition();
    }

    // loop_variable
    //   function: returns the loop binding for @each / @for.
    // For @each `$x in list`, returns `$x`. For @for `$i from
    // 1 through 5`, returns `$i`.
    sass_string_t
    loop_variable() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        const sass_string_t&    pre = this->prelude();
        const std::size_t       sp = pre.find(' ');
        return (sp == sass_string_t::npos) ? pre : pre.substr(0, sp);
    }


    /// module accessors

    // module_url
    //   function: returns the module URL string for @use /
    // @forward / @import.
    sass_string_t
    module_url() const
    {
        if (!this->valid())
        {
            return sass_string_t();
        }
        // Strip surrounding quotes if present.
        const sass_string_t&    pre = this->prelude();
        std::size_t             a   = 0;
        std::size_t             b   = pre.size();
        while ( (a < b) && ( (pre[a] == ' ') || (pre[a] == '\t') ) )
        {
            ++a;
        }
        // up to first whitespace / `as` keyword / `with` keyword.
        std::size_t end = a;
        while ( (end < b) &&
                (pre[end] != ' ') && (pre[end] != '\t') )
        {
            ++end;
        }
        sass_string_t   url = pre.substr(a, end - a);
        if ( (url.size() >= 2) &&
             ((url.front() == '\'') || (url.front() == '"')) &&
             (url.back() == url.front()) )
        {
            url = url.substr(1, url.size() - 2);
        }
        return url;
    }


private:
    // (no extra members -- the facade is pointer-only via the
    // CSS base class.)
};


///////////////////////////////////////////////////////////////////////////////
///                III.   INTERNAL EMISSION HELPERS                         ///
///////////////////////////////////////////////////////////////////////////////

}   // namespace sass
NS_INTERNAL

    // sass_emit_indent_helper
    //   function: writes `_depth` repetitions of the indent
    // string to `_out`. Indent is two spaces by default.
    inline void
    sass_emit_indent_helper(
        std::ostream&       _out,
        std::size_t         _depth
    )
    {
        for (std::size_t i = 0; i < _depth; ++i)
        {
            _out << "  ";
        }
    }


    // sass_emit_rule_scss_helper
    //   function: emits a single rule (and its descendants)
    // as SCSS source. Recursive for grouping rules and nested
    // style rules.
    inline void
    sass_emit_rule_scss_helper(
        std::ostream&                                                   _out,
        const ::djinterp::internal::css_default_node&                   _node,
        std::size_t                                                     _depth
    )
    {
        using K = ::djinterp::css::css_rule_kind;

        if (_node.rule_kind == K::comment)
        {
            sass_emit_indent_helper(_out, _depth);
            _out << "/*" << _node.comment_text << "*/\n";
            return;
        }

        if (_node.rule_kind == K::declaration_block)
        {
            sass_emit_indent_helper(_out, _depth);
            // Variable declaration?
            if ( (!_node.property.empty()) && (_node.property[0] == '$') )
            {
                _out << _node.property << ": " << _node.value;
                if (_node.importance ==
                        ::djinterp::css::css_importance::important)
                {
                    _out << " !default";
                }
                _out << ";\n";
                return;
            }
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
            sass_emit_indent_helper(_out, _depth);
            _out << _node.selector << " {\n";
            for (std::size_t i = 0; i < _node.children.size(); ++i)
            {
                if (_node.children[i])
                {
                    sass_emit_rule_scss_helper(
                        _out, *_node.children[i], _depth + 1);
                }
            }
            sass_emit_indent_helper(_out, _depth);
            _out << "}\n";
            return;
        }

        if (::djinterp::css::is_at_rule_kind(_node.rule_kind))
        {
            sass_emit_indent_helper(_out, _depth);
            _out << '@' << _node.at_keyword;
            if (!_node.prelude.empty())
            {
                _out << ' ' << _node.prelude;
            }
            // Body? Sass at-rules with bodies cover both CSS
            // grouping at-rules and Sass-only ones (@if, @each,
            // @for, @while, @mixin, @function, @at-root).
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
                        sass_emit_rule_scss_helper(
                            _out, *_node.children[i], _depth + 1);
                    }
                }
                sass_emit_indent_helper(_out, _depth);
                _out << "}\n";
            }
            else
            {
                _out << ";\n";
            }
            return;
        }

        // unknown / unsupported -- skip silently.
    }


    // sass_emit_rule_indented_helper
    //   function: emits a single rule (and its descendants)
    // as Sass-indented source (no braces, no semicolons; nesting
    // by indentation).
    inline void
    sass_emit_rule_indented_helper(
        std::ostream&                                                   _out,
        const ::djinterp::internal::css_default_node&                   _node,
        std::size_t                                                     _depth
    )
    {
        using K = ::djinterp::css::css_rule_kind;

        if (_node.rule_kind == K::comment)
        {
            sass_emit_indent_helper(_out, _depth);
            _out << "// " << _node.comment_text << '\n';
            return;
        }

        if (_node.rule_kind == K::declaration_block)
        {
            sass_emit_indent_helper(_out, _depth);
            _out << _node.property << ": " << _node.value;
            if (_node.importance == ::djinterp::css::css_importance::important)
            {
                if ( (!_node.property.empty()) && (_node.property[0] == '$') )
                {
                    _out << " !default";
                }
                else
                {
                    _out << " !important";
                }
            }
            _out << '\n';
            return;
        }

        if (_node.rule_kind == K::style_rule)
        {
            sass_emit_indent_helper(_out, _depth);
            _out << _node.selector << '\n';
            for (std::size_t i = 0; i < _node.children.size(); ++i)
            {
                if (_node.children[i])
                {
                    sass_emit_rule_indented_helper(
                        _out, *_node.children[i], _depth + 1);
                }
            }
            return;
        }

        if (::djinterp::css::is_at_rule_kind(_node.rule_kind))
        {
            sass_emit_indent_helper(_out, _depth);
            _out << '@' << _node.at_keyword;
            if (!_node.prelude.empty())
            {
                _out << ' ' << _node.prelude;
            }
            _out << '\n';
            for (std::size_t i = 0; i < _node.children.size(); ++i)
            {
                if (_node.children[i])
                {
                    sass_emit_rule_indented_helper(
                        _out, *_node.children[i], _depth + 1);
                }
            }
            return;
        }
    }

NS_END  // internal
namespace sass {


///////////////////////////////////////////////////////////////////////////////
///                IV.   sass_stylesheet<_Backend>                          ///
///////////////////////////////////////////////////////////////////////////////

// sass_stylesheet
//   class: thin facade extending
// `css::css_stylesheet<_Backend>` with Sass-specific
// metadata and emission. Adds NO data members beyond the CSS
// base; the Sass syntax / dialect tags are stored in unused
// fields of the inherited stylesheet storage (the CSS
// `syntax_mode` field already covers SCSS / Sass; this facade
// surfaces them under the Sass-specific enums).
template<typename _Backend>
class sass_stylesheet
:   public ::djinterp::css::css_stylesheet<_Backend>
{
public:
    using base_type       = ::djinterp::css::css_stylesheet<_Backend>;
    using stylesheet_type = typename base_type::stylesheet_type;
    using rule_node_type  = typename base_type::rule_node_type;
    using rule_facade     = sass_rule<_Backend>;


    /// constructors

    sass_stylesheet()
    :   base_type()
    {
        this->set_syntax_mode(::djinterp::css::css_syntax_mode::scss);
    }

    explicit
    sass_stylesheet(
        sass_syntax     _syntax
    )
    :   base_type()
    {
        this->set_sass_syntax(_syntax);
    }


    /// metadata

    // sass_syntax
    //   function: returns the Sass surface syntax. Derived
    // from the CSS `syntax_mode` field.
    enum sass_syntax
    sass_syntax() const
    {
        const auto m = this->syntax_mode();
        return ( m == ::djinterp::css::css_syntax_mode::sass )
                ? sass_syntax::indented
             : ( m == ::djinterp::css::css_syntax_mode::scss )
                ? sass_syntax::scss
             :    sass_syntax::unspecified;
    }

    enum sass_syntax
    get_sass_syntax() const
    {
        return this->sass_syntax();
    }

    // set_sass_syntax
    void
    set_sass_syntax(
        enum sass_syntax    _syntax
    )
    {
        switch (_syntax)
        {
            case sass_syntax::indented:
                this->set_syntax_mode(::djinterp::css::css_syntax_mode::sass);
                break;
            case sass_syntax::scss:
                this->set_syntax_mode(::djinterp::css::css_syntax_mode::scss);
                break;
            default:
                this->set_syntax_mode(::djinterp::css::css_syntax_mode::scss);
                break;
        }
    }

    // sass_dialect
    //   function: returns the configured dialect. Default
    // backend reports `dart_sass`; adapter backends can
    // override.
    enum sass_dialect
    sass_dialect() const
    {
        return sass_dialect::dart_sass;
    }


    /// rule traversal (returns Sass facades)

    // sass_rule_at
    //   function: returns a Sass facade for the rule at index
    // `_index`.
    rule_facade
    sass_rule_at(
        std::size_t     _index
    )   const
    {
        auto    css_r = this->rule_at(_index);
        return rule_facade(css_r.backend_handle());
    }


    /// Sass-specific authoring helpers

    // add_variable
    //   function: appends a variable declaration to the
    // stylesheet root.
    rule_facade
    add_variable(
        const sass_string_t&    _name,
        const sass_string_t&    _value,
        bool                    _default = false
    )
    {
        if (!this->backend_stylesheet().root)
        {
            return rule_facade();
        }
        std::unique_ptr<rule_node_type>     n(new rule_node_type);
        n->rule_kind = ::djinterp::css::css_rule_kind::declaration_block;
        n->property  = ( (!_name.empty()) && (_name[0] == '$') )
            ? _name
            : sass_string_t("$") + _name;
        n->value     = _value;
        if (_default)
        {
            n->importance = ::djinterp::css::css_importance::important;
        }
        rule_node_type* raw = n.get();
        const_cast<typename base_type::stylesheet_type&>(
            this->backend_stylesheet()).root->children.push_back(std::move(n));
        return rule_facade(raw);
    }

    // add_mixin
    //   function: appends a `@mixin name(params)` rule.
    rule_facade
    add_mixin(
        const sass_string_t&    _name,
        const sass_string_t&    _parameters = sass_string_t()
    )
    {
        sass_string_t   prelude = _name;
        if (!_parameters.empty())
        {
            prelude += '(';
            prelude += _parameters;
            prelude += ')';
        }
        return this->add_sass_at_rule_(at_keywords::mixin, prelude);
    }

    // add_include
    //   function: appends an `@include name(args)` statement.
    rule_facade
    add_include(
        const sass_string_t&    _target,
        const sass_string_t&    _arguments = sass_string_t()
    )
    {
        sass_string_t   prelude = _target;
        if (!_arguments.empty())
        {
            prelude += '(';
            prelude += _arguments;
            prelude += ')';
        }
        return this->add_sass_at_rule_(at_keywords::include, prelude);
    }

    // add_function
    //   function: appends a `@function name(params)` rule.
    rule_facade
    add_function(
        const sass_string_t&    _name,
        const sass_string_t&    _parameters = sass_string_t()
    )
    {
        sass_string_t   prelude = _name;
        if (!_parameters.empty())
        {
            prelude += '(';
            prelude += _parameters;
            prelude += ')';
        }
        return this->add_sass_at_rule_(at_keywords::function, prelude);
    }

    // add_extend
    //   function: appends an `@extend selector` statement.
    rule_facade
    add_extend(
        const sass_string_t&    _target,
        bool                    _optional = false
    )
    {
        sass_string_t   prelude = _target;
        if (_optional)
        {
            prelude += " !optional";
        }
        return this->add_sass_at_rule_(at_keywords::extend, prelude);
    }

    // add_use
    //   function: appends a `@use 'url'` rule.
    rule_facade
    add_use(
        const sass_string_t&    _url,
        const sass_string_t&    _namespace = sass_string_t()
    )
    {
        sass_string_t   prelude = sass_string_t("'") + _url + sass_string_t("'");
        if (!_namespace.empty())
        {
            prelude += " as ";
            prelude += _namespace;
        }
        return this->add_sass_at_rule_(at_keywords::use, prelude);
    }

    // add_forward
    //   function: appends a `@forward 'url'` rule.
    rule_facade
    add_forward(
        const sass_string_t&    _url
    )
    {
        sass_string_t   prelude = sass_string_t("'") + _url + sass_string_t("'");
        return this->add_sass_at_rule_(at_keywords::forward, prelude);
    }

    // add_if
    //   function: appends an `@if condition` block.
    rule_facade
    add_if(
        const sass_string_t&    _condition
    )
    {
        return this->add_sass_at_rule_(at_keywords::if_, _condition);
    }

    // add_each
    //   function: appends an `@each $var in list` block.
    rule_facade
    add_each(
        const sass_string_t&    _variable,
        const sass_string_t&    _iterable
    )
    {
        sass_string_t   prelude;
        prelude.reserve(_variable.size() + _iterable.size() + 5);
        prelude.append(_variable);
        prelude.append(" in ");
        prelude.append(_iterable);
        return this->add_sass_at_rule_(at_keywords::each, prelude);
    }


    /// rendering

    // render_to_scss_source
    //   function: emits this stylesheet as SCSS source.
    void
    render_to_scss_source(
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
                ::djinterp::internal::sass_emit_rule_scss_helper(
                    _out, *doc.root->children[i], 0);
            }
        }
    }

    sass_string_t
    render_to_scss_source() const
    {
        std::ostringstream  oss;
        this->render_to_scss_source(oss);
        return oss.str();
    }


    // render_to_sass_source
    //   function: emits this stylesheet as Sass-indented
    // source.
    void
    render_to_sass_source(
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
                ::djinterp::internal::sass_emit_rule_indented_helper(
                    _out, *doc.root->children[i], 0);
            }
        }
    }

    sass_string_t
    render_to_sass_source() const
    {
        std::ostringstream  oss;
        this->render_to_sass_source(oss);
        return oss.str();
    }


    // compile_to_css
    //   function: stub. Real Sass compilation requires the
    // full evaluator (variable resolution, mixin / function
    // expansion, control flow execution, color / math
    // operators, module loading, `@extend` resolution). The
    // default backend cannot do this; adapter backends bound
    // to libsass or sass-embedded provide real compilation.
    // The default implementation falls back to emitting the
    // SCSS source unchanged so callers wiring the emission
    // pipeline early still see output.
    void
    compile_to_css(
        std::ostream&   _out
    )   const
    {
        this->render_to_scss_source(_out);
    }


private:
    // add_sass_at_rule_
    //   function: internal helper that appends an at-rule
    // with the given Sass-specific keyword and prelude. Used
    // by the per-keyword `add_*` methods above.
    rule_facade
    add_sass_at_rule_(
        const sass_string_t&    _keyword,
        const sass_string_t&    _prelude
    )
    {
        auto    css_r = this->add_at_rule(_keyword, _prelude);
        return rule_facade(css_r.backend_handle());
    }
};


///////////////////////////////////////////////////////////////////////////////
///                V.   sass_default_backend                                ///
///////////////////////////////////////////////////////////////////////////////

// sass_default_backend
//   struct: bundled in-memory backend for the Sass facades.
// Reuses the CSS default backend's storage types verbatim --
// the Sass extension state fits entirely into existing fields
// on the CSS default node, so no parallel storage hierarchy is
// needed. Exposes BOTH the CSS backend tag and the Sass
// backend tag so the same type satisfies both protocols.
struct sass_default_backend
{
    using css_backend_tag  = ::djinterp::css::css_default_backend_tag;
    using sass_backend_tag = ::djinterp::sass::sass_default_backend_tag;

    using rule_type             = ::djinterp::internal::css_default_node;
    using declaration_type      = ::djinterp::internal::css_default_node;
    using stylesheet_type       = ::djinterp::internal::css_default_stylesheet;
    using sass_rule_type        = ::djinterp::internal::css_default_node;
    using sass_stylesheet_type  = ::djinterp::internal::css_default_stylesheet;

    // make_stylesheet
    //   function: factory matching the CSS backend protocol.
    static stylesheet_type
    make_stylesheet()
    {
        return ::djinterp::css::css_default_backend::make_stylesheet();
    }

    // make_sass_stylesheet
    //   function: factory matching the Sass backend protocol.
    // Returns a stylesheet pre-tagged with SCSS syntax mode.
    static stylesheet_type
    make_sass_stylesheet()
    {
        stylesheet_type s = make_stylesheet();
        s.syntax_mode = ::djinterp::css::css_syntax_mode::scss;
        return s;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   FREE HELPERS / FACTORIES                           ///
///////////////////////////////////////////////////////////////////////////////

// make_sass_stylesheet
//   function: factory returning a freshly-built Sass
// stylesheet facade for the given backend.
template<typename _Backend>
inline sass_stylesheet<_Backend>
make_sass_stylesheet(
    sass_syntax     _syntax = sass_syntax::scss
)
{
    return sass_stylesheet<_Backend>(_syntax);
}


// make_variable
//   function: returns a new freestanding variable declaration.
template<typename _Backend>
inline sass_rule<_Backend>
make_variable(
    const sass_string_t&    _name,
    const sass_string_t&    _value,
    bool                    _default = false
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = ::djinterp::css::css_rule_kind::declaration_block;
    n->property  = ( (!_name.empty()) && (_name[0] == '$') )
        ? _name
        : sass_string_t("$") + _name;
    n->value = _value;
    if (_default)
    {
        n->importance = ::djinterp::css::css_importance::important;
    }
    return sass_rule<_Backend>(n);
}


// make_mixin
//   function: returns a new freestanding @mixin rule.
template<typename _Backend>
inline sass_rule<_Backend>
make_mixin(
    const sass_string_t&    _name,
    const sass_string_t&    _parameters = sass_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::mixin;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = _name;
    if (!_parameters.empty())
    {
        n->prelude += '(';
        n->prelude += _parameters;
        n->prelude += ')';
    }
    return sass_rule<_Backend>(n);
}


// make_include
//   function: returns a new freestanding @include statement.
template<typename _Backend>
inline sass_rule<_Backend>
make_include(
    const sass_string_t&    _target,
    const sass_string_t&    _arguments = sass_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::include;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = _target;
    if (!_arguments.empty())
    {
        n->prelude += '(';
        n->prelude += _arguments;
        n->prelude += ')';
    }
    return sass_rule<_Backend>(n);
}


// make_extend
//   function: returns a new freestanding @extend statement.
template<typename _Backend>
inline sass_rule<_Backend>
make_extend(
    const sass_string_t&    _target,
    bool                    _optional = false
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::extend;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = _target;
    if (_optional)
    {
        n->prelude += " !optional";
    }
    return sass_rule<_Backend>(n);
}


// make_use
//   function: returns a new freestanding @use rule.
template<typename _Backend>
inline sass_rule<_Backend>
make_use(
    const sass_string_t&    _url,
    const sass_string_t&    _namespace = sass_string_t()
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::use;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = sass_string_t("'") + _url + sass_string_t("'");
    if (!_namespace.empty())
    {
        n->prelude += " as ";
        n->prelude += _namespace;
    }
    return sass_rule<_Backend>(n);
}


// make_placeholder_rule
//   function: returns a new freestanding placeholder selector
// rule (`%name { ... }`).
template<typename _Backend>
inline sass_rule<_Backend>
make_placeholder_rule(
    const sass_string_t&    _name
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->rule_kind = ::djinterp::css::css_rule_kind::style_rule;
    n->selector  = ( (!_name.empty()) && (_name[0] == '%') )
        ? _name
        : sass_string_t("%") + _name;
    return sass_rule<_Backend>(n);
}


// make_each
//   function: returns a new freestanding @each rule.
template<typename _Backend>
inline sass_rule<_Backend>
make_each(
    const sass_string_t&    _variable,
    const sass_string_t&    _iterable
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::each;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = _variable;
    n->prelude   += " in ";
    n->prelude   += _iterable;
    return sass_rule<_Backend>(n);
}


// make_if
//   function: returns a new freestanding @if rule.
template<typename _Backend>
inline sass_rule<_Backend>
make_if(
    const sass_string_t&    _condition
)
{
    using node_t = typename _Backend::rule_type;
    node_t* n = new node_t;
    n->at_keyword = at_keywords::if_;
    n->rule_kind  = ::djinterp::css::css_rule_kind::at_rule;
    n->prelude    = _condition;
    return sass_rule<_Backend>(n);
}


}   // namespace sass
NS_END  // djinterp


#endif  // DJINTERP_SASS_TEMPLATE_
