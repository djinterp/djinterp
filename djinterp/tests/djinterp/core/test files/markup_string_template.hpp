/******************************************************************************
* djinterp [util]                                       markup_string_template.hpp
*
*   Delimiter-agnostic string-templating engine for producing markup
* (XML, HTML, or any other format) via variable interpolation,
* partial inclusion, and simple sectioning. Inspired by handlebars /
* mustache but with the delimiter strings made fully configurable so
* the same engine can drive any syntactic style:
*
*     default:     "Hello {name}!"
*     handlebars:  "Hello {{name}}!"
*     erb:         "Hello <%= name %>!"
*     dollar:      "Hello $name$!"
*     angle:       "Hello <<name>>!"
*     square:      "Hello [[name]]!"
*     custom:      "Hello |START| name |END|!"
*
*   The engine is the SHARED CORE for both `xml_string_template` and
* `html_string_template`. Those types are thin facades layered on top
* of `markup_string_template<_EscapePolicy>` -- they bind a specific
* escape policy and inherit the rest. The split exists because the
* only meaningful difference between XML and HTML interpolation is
* the entity reference for the apostrophe (`&apos;` for XML,
* `&#39;` for HTML, since older HTML versions did not define
* `&apos;`).
*
*   ESCAPE POLICY:
*   The class template's first parameter is a struct exposing a
* single static method:
*
*     struct my_escape_policy {
*         static void escape(std::ostream& _out,
*                            const std::string& _in);
*     };
*
*   Three policies are provided here:
*     - `xml_escape_policy`  -- canonical XML entity references
*     - `html_escape_policy` -- HTML-compatible entity references
*     - `null_escape_policy` -- no escaping (raw substitution)
*
*   FEATURES:
*   - variable interpolation        {name}
*   - partial inclusion             {>partial_name}
*   - section open / close          {#section_name} ... {/section_name}
*   - comments                      {!ignored content}
*   - raw (un-escaped) variable     {&name}
*   - context with scalar / list / partial bindings
*   - arbitrary nesting depth via partials calling partials
*
*   ZERO COUPLING TO MARKUP ASTs:
*   The engine produces a `std::string`. That string can be written
* to disk, sent over the wire, set as the text content of a markup
* node, or fed back into a parser. This header has no dependency on
* `xml_template.hpp` or `html_template.hpp`, so those headers may
* include this one (directly or via the facade headers) freely.
*
*
* path:      /inc/djinterp/core/util/markup_string_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.05.09
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SYNTAX POLICY STRUCTS
II.   ESCAPE POLICY STRUCTS
III.  INTERNAL TOKEN TYPES & PARSE HELPERS
IV.   markup_string_template_context<_EscapePolicy>
V.    markup_string_template<_EscapePolicy>
VI.   FREE HELPERS / FACTORIES
*/

#ifndef DJINTERP_MARKUP_STRING_TEMPLATE_
#define DJINTERP_MARKUP_STRING_TEMPLATE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../djinterp.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                I.   SYNTAX POLICY STRUCTS                               ///
///////////////////////////////////////////////////////////////////////////////

// markup_string_template_syntax_tag
//   struct: empty base for all syntax policy tags. Lets traits
// detect a syntax-policy type via inheritance.
struct markup_string_template_syntax_tag
{};


// markup_string_template_syntax_default
//   struct: bundled default delimiter set -- single curly braces.
//   Variable    : {name}
//   Raw var     : {&name}
//   Partial     : {>partial_name}
//   Section open: {#section_name}
//   Section end : {/section_name}
//   Comment     : {!commented out}
struct markup_string_template_syntax_default
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "{";
    D_STATIC_CONSTEXPR const char*  close_tag            = "}";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_handlebars
//   struct: handlebars-style double-curly delimiters.
struct markup_string_template_syntax_handlebars
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "{{";
    D_STATIC_CONSTEXPR const char*  close_tag            = "}}";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_erb
//   struct: ERB-style angle-percent delimiters.
struct markup_string_template_syntax_erb
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "<%=";
    D_STATIC_CONSTEXPR const char*  close_tag            = "%>";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "==";
    D_STATIC_CONSTEXPR const char*  partial_marker       = "include ";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "begin ";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "end ";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "#";
};


// markup_string_template_syntax_dollar
//   struct: dollar-delimited (open and close are the same).
struct markup_string_template_syntax_dollar
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "$";
    D_STATIC_CONSTEXPR const char*  close_tag            = "$";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_angle
//   struct: double-angle delimiters (avoids tag conflicts).
struct markup_string_template_syntax_angle
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "<<";
    D_STATIC_CONSTEXPR const char*  close_tag            = ">>";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_square
//   struct: double-square delimiters.
struct markup_string_template_syntax_square
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "[[";
    D_STATIC_CONSTEXPR const char*  close_tag            = "]]";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_php
//   struct: PHP-style short-tag delimiters.
struct markup_string_template_syntax_php
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "<?=";
    D_STATIC_CONSTEXPR const char*  close_tag            = "?>";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = "include ";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "if ";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "endif ";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "//";
};


// markup_string_template_syntax_curly_dollar
//   struct: shell-style `${name}` delimiters. Convenient when
// the source contains literal `{` / `}` but `${...}` is rare.
struct markup_string_template_syntax_curly_dollar
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "${";
    D_STATIC_CONSTEXPR const char*  close_tag            = "}";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "&";
    D_STATIC_CONSTEXPR const char*  partial_marker       = ">";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "#";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "/";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "!";
};


// markup_string_template_syntax_xml_pi
//   struct: XML-processing-instruction-style delimiters --
// `<?tpl name ?>`. The templated source remains valid XML even
// before substitution (PIs are syntactically legal anywhere
// text content is), so half-rendered templates still parse.
struct markup_string_template_syntax_xml_pi
:   markup_string_template_syntax_tag
{
    D_STATIC_CONSTEXPR const char*  open_tag             = "<?tpl";
    D_STATIC_CONSTEXPR const char*  close_tag            = "?>";
    D_STATIC_CONSTEXPR const char*  raw_marker           = "raw ";
    D_STATIC_CONSTEXPR const char*  partial_marker       = "include ";
    D_STATIC_CONSTEXPR const char*  section_open_marker  = "begin ";
    D_STATIC_CONSTEXPR const char*  section_close_marker = "end ";
    D_STATIC_CONSTEXPR const char*  comment_marker       = "rem ";
};


///////////////////////////////////////////////////////////////////////////////
///                II.   ESCAPE POLICY STRUCTS                              ///
///////////////////////////////////////////////////////////////////////////////

// markup_escape_policy_tag
//   struct: empty base for all escape policy tags. Lets traits
// detect an escape-policy type via inheritance.
struct markup_escape_policy_tag
{};


// xml_escape_policy
//   struct: escape policy producing the five XML-canonical
// entity references:
//   &amp; &lt; &gt; &quot; &apos;
struct xml_escape_policy
:   markup_escape_policy_tag
{
    static void
    escape(
        std::ostream&       _out,
        const std::string&  _in
    )
    {
        const std::size_t   n = _in.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const char c = _in[i];
            switch (c)
            {
                case '&':   _out << "&amp;";   break;
                case '<':   _out << "&lt;";    break;
                case '>':   _out << "&gt;";    break;
                case '"':   _out << "&quot;";  break;
                case '\'':  _out << "&apos;";  break;
                default:    _out << c;         break;
            }
        }
    }
};


// html_escape_policy
//   struct: escape policy producing HTML-compatible entity
// references. Matches `xml_escape_policy` exactly except that
// the apostrophe becomes `&#39;` (a numeric character
// reference) rather than `&apos;`, since `&apos;` is undefined
// in HTML4 / pre-HTML5 documents.
struct html_escape_policy
:   markup_escape_policy_tag
{
    static void
    escape(
        std::ostream&       _out,
        const std::string&  _in
    )
    {
        const std::size_t   n = _in.size();
        for (std::size_t i = 0; i < n; ++i)
        {
            const char c = _in[i];
            switch (c)
            {
                case '&':   _out << "&amp;";   break;
                case '<':   _out << "&lt;";    break;
                case '>':   _out << "&gt;";    break;
                case '"':   _out << "&quot;";  break;
                case '\'':  _out << "&#39;";   break;
                default:    _out << c;         break;
            }
        }
    }
};


// null_escape_policy
//   struct: escape policy that performs no escaping. Useful
// for non-markup content (config files, code generation, plain
// text emails) where variable values should pass through
// verbatim.
struct null_escape_policy
:   markup_escape_policy_tag
{
    static void
    escape(
        std::ostream&       _out,
        const std::string&  _in
    )
    {
        _out << _in;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III.   INTERNAL TOKEN TYPES & PARSE HELPERS              ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // markup_string_template_token_kind
    //   enum: discriminator for parsed template tokens.
    enum class markup_string_template_token_kind : std::uint8_t
    {
        literal,            // plain text emitted verbatim
        variable_escaped,   // {name} -- escaped on output
        variable_raw,       // {&name} -- emitted as-is
        partial_include,    // {>name} -- recursive partial render
        section,            // {#name}...{/name} -- conditional / loop
        comment             // {!...} -- consumed, no output
    };


    // markup_string_template_token
    //   struct: a single parsed token. For sections, `children`
    // holds the body; for everything else `children` is empty.
    struct markup_string_template_token
    {
        markup_string_template_token_kind           kind =
            markup_string_template_token_kind::literal;
        std::string                                 text;
        std::vector<markup_string_template_token>   children;
    };


    // markup_string_template_trim_helper
    //   function: returns `_in` with leading and trailing ASCII
    // whitespace stripped.
    inline std::string
    markup_string_template_trim_helper(
        const std::string&  _in
    )
    {
        const std::size_t   n = _in.size();
        std::size_t         a = 0;
        while ( (a < n) && ( (_in[a] == ' ')  || (_in[a] == '\t') ||
                             (_in[a] == '\n') || (_in[a] == '\r') ) )
        {
            ++a;
        }
        std::size_t         b = n;
        while ( (b > a) && ( (_in[b - 1] == ' ')  || (_in[b - 1] == '\t') ||
                             (_in[b - 1] == '\n') || (_in[b - 1] == '\r') ) )
        {
            --b;
        }
        return _in.substr(a, b - a);
    }


    // markup_string_template_starts_with_helper
    //   function: true if `_s` begins with `_prefix`.
    inline bool
    markup_string_template_starts_with_helper(
        const std::string&  _s,
        const std::string&  _prefix
    )
    {
        if (_prefix.empty())
        {
            return false;
        }
        if (_s.size() < _prefix.size())
        {
            return false;
        }
        return (_s.compare(0, _prefix.size(), _prefix) == 0);
    }

NS_END  // internal


// (forward declaration -- defined below)
template<typename _EscapePolicy>
class markup_string_template;


///////////////////////////////////////////////////////////////////////////////
///                IV.   markup_string_template_context                     ///
///////////////////////////////////////////////////////////////////////////////

// markup_string_template_context
//   class: variable bindings, list bindings, and (optional)
// per-context partial overrides for a single render call.
// Contexts are value types; create child contexts for nested
// scopes (e.g. iterating over a list).
//
// Parameterised on `_EscapePolicy` because the partial map
// holds shared pointers to templates with that same policy --
// templates with different escape policies are distinct types.
template<typename _EscapePolicy>
class markup_string_template_context
{
public:
    // template_t
    //   type: the template type the partial map holds.
    using template_t    = markup_string_template<_EscapePolicy>;

    // partial_ptr_t
    //   type: shared-ownership handle for a partial template.
    using partial_ptr_t = std::shared_ptr<template_t>;

    // value_map_t
    //   type: scalar variable bindings.
    using value_map_t   = std::map<std::string, std::string>;

    // list_map_t
    //   type: list bindings -- each list entry is a child context
    // used when rendering a section that names this list.
    using list_map_t    = std::map<std::string,
                                   std::vector<markup_string_template_context>>;

    // partial_map_t
    //   type: per-context partial overrides. Resolved BEFORE the
    // template's own partial registry.
    using partial_map_t = std::map<std::string, partial_ptr_t>;


    /// scalar bindings

    // set
    //   function: binds `_key` to a string value.
    void
    set(
        const std::string&  _key,
        const std::string&  _value
    )
    {
        m_values[_key] = _value;
    }

    // set
    //   function: binds `_key` to an integer value (formatted via
    // std::to_string).
    void
    set(
        const std::string&  _key,
        long long           _value
    )
    {
        m_values[_key] = std::to_string(_value);
    }

    // set_bool
    //   function: binds `_key` to a boolean. Stored as "true" /
    // "false" but additionally usable as a section condition.
    void
    set_bool(
        const std::string&  _key,
        bool                _value
    )
    {
        m_values[_key] = _value ? "true" : "false";
    }

    // has
    //   function: true if `_key` has a scalar binding.
    bool
    has(
        const std::string&  _key
    )   const
    {
        return (m_values.find(_key) != m_values.end());
    }

    // get
    //   function: returns the bound value for `_key`, or the
    // empty string if absent.
    const std::string&
    get(
        const std::string&  _key
    )   const
    {
        typename value_map_t::const_iterator    it = m_values.find(_key);
        if (it == m_values.end())
        {
            return s_empty();
        }
        return it->second;
    }

    // is_truthy
    //   function: true if `_key` is bound to a value that should
    // gate a section on. False for missing, empty, "false", or
    // "0"; true for everything else.
    bool
    is_truthy(
        const std::string&  _key
    )   const
    {
        if (!this->has(_key))
        {
            return false;
        }
        const std::string&  v = this->get(_key);
        return ( (!v.empty()) && (v != "false") && (v != "0") );
    }


    /// list bindings

    // set_list
    //   function: binds `_key` to a list of child contexts.
    // Sections named `_key` will iterate, rendering their body
    // once per entry.
    void
    set_list(
        const std::string&                                          _key,
        const std::vector<markup_string_template_context>&          _items
    )
    {
        m_lists[_key] = _items;
    }

    // append_to_list
    //   function: appends a single child context to the list
    // bound at `_key`, creating the list if absent.
    void
    append_to_list(
        const std::string&                          _key,
        const markup_string_template_context&       _item
    )
    {
        m_lists[_key].push_back(_item);
    }

    // has_list
    //   function: true if `_key` has a list binding.
    bool
    has_list(
        const std::string&  _key
    )   const
    {
        return (m_lists.find(_key) != m_lists.end());
    }

    // get_list
    //   function: returns the list bound at `_key`. Returns an
    // empty vector reference if absent.
    const std::vector<markup_string_template_context>&
    get_list(
        const std::string&  _key
    )   const
    {
        typename list_map_t::const_iterator     it = m_lists.find(_key);
        if (it == m_lists.end())
        {
            return s_empty_list();
        }
        return it->second;
    }


    /// partial overrides

    // set_partial
    //   function: registers (or overrides) a partial template
    // for this render. Takes shared ownership.
    void
    set_partial(
        const std::string&      _name,
        const partial_ptr_t&    _tpl
    )
    {
        m_partials[_name] = _tpl;
    }

    // find_partial
    //   function: looks up a partial by name. Returns an empty
    // shared_ptr if the context does not override that name.
    const partial_ptr_t&
    find_partial(
        const std::string&  _name
    )   const
    {
        typename partial_map_t::const_iterator  it = m_partials.find(_name);
        if (it == m_partials.end())
        {
            return s_empty_partial();
        }
        return it->second;
    }


private:
    // s_empty
    //   function: holds the canonical empty-string return for
    // `get` on a missing key.
    static const std::string&
    s_empty()
    {
        static const std::string e;
        return e;
    }

    // s_empty_list
    //   function: holds the canonical empty-list return for
    // `get_list` on a missing key.
    static const std::vector<markup_string_template_context>&
    s_empty_list()
    {
        static const std::vector<markup_string_template_context> e;
        return e;
    }

    // s_empty_partial
    //   function: holds the canonical empty-shared_ptr return
    // for `find_partial` on a missing key.
    static const partial_ptr_t&
    s_empty_partial()
    {
        static const partial_ptr_t e;
        return e;
    }


    // m_values
    //   field: scalar variable bindings.
    value_map_t     m_values;

    // m_lists
    //   field: list bindings used by section iteration.
    list_map_t      m_lists;

    // m_partials
    //   field: per-context partial overrides.
    partial_map_t   m_partials;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   markup_string_template                              ///
///////////////////////////////////////////////////////////////////////////////

// markup_string_template
//   class: a parsed string template. Owns its source string,
// its delimiter configuration, and (optionally) a registry of
// named partial sub-templates. Renders to a `std::string` (or
// streams to an `std::ostream`) given a context of variable /
// list bindings.
//
// Parameterised on `_EscapePolicy` -- the only behavioural
// knob between formats. The bundled policies are
// `xml_escape_policy`, `html_escape_policy`, and
// `null_escape_policy`; user code can supply any struct with a
// matching `static void escape(std::ostream&, const std::string&)`
// signature.
//
// The class is otherwise non-templated: the syntax policy only
// seeds initial delimiter values, after which everything lives
// at runtime. Heterogeneous templates (handlebars-style and
// angle-style) can sit in the same container without type
// erasure, provided they share the escape policy.
template<typename _EscapePolicy>
class markup_string_template
{
public:
    // escape_policy_t
    //   type: alias for the escape policy this template uses.
    using escape_policy_t = _EscapePolicy;

    // context_t
    //   type: alias for the matching context type.
    using context_t       = markup_string_template_context<_EscapePolicy>;

    // partial_ptr_t
    //   type: shared-ownership handle for a partial template.
    using partial_ptr_t   = std::shared_ptr<markup_string_template>;

    // partial_map_t
    //   type: name -> partial registry.
    using partial_map_t   = std::map<std::string, partial_ptr_t>;


    /// constructors

    // markup_string_template (default)
    //   ctor: empty source, default-syntax delimiters.
    markup_string_template()
    :   m_open_tag(markup_string_template_syntax_default::open_tag),
        m_close_tag(markup_string_template_syntax_default::close_tag),
        m_raw_marker(markup_string_template_syntax_default::raw_marker),
        m_partial_marker(
            markup_string_template_syntax_default::partial_marker),
        m_section_open_marker(
            markup_string_template_syntax_default::section_open_marker),
        m_section_close_marker(
            markup_string_template_syntax_default::section_close_marker),
        m_comment_marker(
            markup_string_template_syntax_default::comment_marker),
        m_source(),
        m_partials(),
        m_tokens(),
        m_dirty(true)
    {}

    // markup_string_template (from source, default syntax)
    //   ctor: takes source text; uses default delimiters.
    explicit
    markup_string_template(
        const std::string&  _source
    )
    :   markup_string_template()
    {
        m_source = _source;
    }

    // markup_string_template (from source + syntax policy)
    //   ctor: takes source text and a syntax policy struct.
    // The policy's `D_STATIC_CONSTEXPR const char*` constants
    // are copied into the instance's runtime delimiter strings.
    template<typename _Syntax>
    markup_string_template(
        const std::string&  _source,
        _Syntax             /*syntax_tag*/
    )
    :   m_open_tag(_Syntax::open_tag),
        m_close_tag(_Syntax::close_tag),
        m_raw_marker(_Syntax::raw_marker),
        m_partial_marker(_Syntax::partial_marker),
        m_section_open_marker(_Syntax::section_open_marker),
        m_section_close_marker(_Syntax::section_close_marker),
        m_comment_marker(_Syntax::comment_marker),
        m_source(_source),
        m_partials(),
        m_tokens(),
        m_dirty(true)
    {}

    // markup_string_template (from source + explicit delimiters)
    //   ctor: takes source text and explicit open/close
    // delimiter strings. All other markers stay at default.
    markup_string_template(
        const std::string&  _source,
        const std::string&  _open_tag,
        const std::string&  _close_tag
    )
    :   markup_string_template()
    {
        m_source    = _source;
        m_open_tag  = _open_tag;
        m_close_tag = _close_tag;
    }


    /// configuration

    // set_source
    //   function: replaces the template source. Marks the
    // parsed-token cache dirty.
    void
    set_source(
        const std::string&  _source
    )
    {
        m_source = _source;
        m_dirty  = true;
    }

    // source
    //   function: returns the current source string.
    const std::string&
    source() const
    {
        return m_source;
    }

    // set_delimiters
    //   function: overrides the open / close delimiter strings.
    // Either may be empty -- empty open_tag disables
    // interpolation entirely (the template renders verbatim).
    void
    set_delimiters(
        const std::string&  _open_tag,
        const std::string&  _close_tag
    )
    {
        m_open_tag  = _open_tag;
        m_close_tag = _close_tag;
        m_dirty     = true;
    }

    // set_markers
    //   function: overrides the directive-classification
    // markers. Pass empty string to disable a directive class.
    void
    set_markers(
        const std::string&  _raw_marker,
        const std::string&  _partial_marker,
        const std::string&  _section_open_marker,
        const std::string&  _section_close_marker,
        const std::string&  _comment_marker
    )
    {
        m_raw_marker           = _raw_marker;
        m_partial_marker       = _partial_marker;
        m_section_open_marker  = _section_open_marker;
        m_section_close_marker = _section_close_marker;
        m_comment_marker       = _comment_marker;
        m_dirty                = true;
    }


    /// partial registry

    // add_partial
    //   function: registers a partial template under `_name`.
    // The partial is shared-owned; the same partial may be
    // registered under multiple names or in multiple parents.
    void
    add_partial(
        const std::string&      _name,
        const partial_ptr_t&    _partial
    )
    {
        m_partials[_name] = _partial;
    }

    // add_partial
    //   function: registers a partial by value (copies into a
    // new shared handle).
    void
    add_partial(
        const std::string&              _name,
        const markup_string_template&   _partial
    )
    {
        m_partials[_name] =
            std::make_shared<markup_string_template>(_partial);
    }

    // remove_partial
    //   function: removes the partial registered under `_name`.
    void
    remove_partial(
        const std::string&  _name
    )
    {
        m_partials.erase(_name);
    }

    // partial_count
    //   function: number of registered partials.
    std::size_t
    partial_count() const
    {
        return m_partials.size();
    }


    /// rendering

    // render
    //   function: renders the template against `_ctx`, returning
    // the produced string.
    std::string
    render(
        const context_t&    _ctx
    )   const
    {
        std::ostringstream  oss;
        this->render_to(oss, _ctx);
        return oss.str();
    }

    // render
    //   function: convenience overload that renders against an
    // empty context.
    std::string
    render() const
    {
        context_t   empty_ctx;
        return this->render(empty_ctx);
    }

    // render_to
    //   function: streams the rendered output to `_out`.
    void
    render_to(
        std::ostream&       _out,
        const context_t&    _ctx
    )   const
    {
        if (m_dirty)
        {
            this->parse_now();
        }
        this->render_token_list_to(_out, m_tokens, _ctx);
    }


private:
    /// parsing

    // parse_now
    //   function: re-parses the source into m_tokens and clears
    // the dirty flag. `mutable` because the parsed cache is
    // logically const (lazily derived from public state).
    void
    parse_now() const
    {
        m_tokens.clear();
        if (!m_open_tag.empty())
        {
            std::size_t pos = 0;
            m_tokens = this->parse_token_list(pos, std::string());
        }
        else
        {
            // no interpolation -- entire source is one literal.
            internal::markup_string_template_token  t;
            t.kind = internal::markup_string_template_token_kind::literal;
            t.text = m_source;
            m_tokens.push_back(std::move(t));
        }
        m_dirty = false;
    }

    // parse_token_list
    //   function: recursive descent over the source. Reads
    // tokens until either end-of-source or a section_close
    // matching `_section_terminator`. On match it consumes the
    // close directive and returns. `_pos` is updated by
    // reference so callers can pick up where parsing stopped.
    std::vector<internal::markup_string_template_token>
    parse_token_list(
        std::size_t&        _pos,
        const std::string&  _section_terminator
    )   const
    {
        std::vector<internal::markup_string_template_token>     out;
        const std::size_t   n = m_source.size();

        while (_pos < n)
        {
            const std::size_t   open_pos = m_source.find(m_open_tag, _pos);
            if (open_pos == std::string::npos)
            {
                // remainder is literal text.
                this->push_literal(out, m_source.substr(_pos));
                _pos = n;
                break;
            }

            // emit literal prefix (if any).
            if (open_pos > _pos)
            {
                this->push_literal(out,
                    m_source.substr(_pos, open_pos - _pos));
            }

            // find matching close delimiter.
            const std::size_t   inner_start = open_pos + m_open_tag.size();
            const std::size_t   close_pos   =
                m_source.find(m_close_tag, inner_start);
            if (close_pos == std::string::npos)
            {
                // unterminated; treat the rest (including the
                // open delimiter) as literal text and stop.
                this->push_literal(out, m_source.substr(open_pos));
                _pos = n;
                break;
            }

            // extract & trim the directive body.
            std::string         directive =
                internal::markup_string_template_trim_helper(
                    m_source.substr(inner_start, close_pos - inner_start));
            const std::size_t   after_close = close_pos + m_close_tag.size();

            // section close?
            if ( (!m_section_close_marker.empty()) &&
                 internal::markup_string_template_starts_with_helper(
                    directive, m_section_close_marker) )
            {
                std::string name =
                    internal::markup_string_template_trim_helper(
                        directive.substr(m_section_close_marker.size()));
                if (name == _section_terminator)
                {
                    _pos = after_close;
                    return out;
                }
                // mismatched close -- keep parsing as if literal.
                this->push_literal(out,
                    m_source.substr(open_pos, after_close - open_pos));
                _pos = after_close;
                continue;
            }

            // section open?
            if ( (!m_section_open_marker.empty()) &&
                 internal::markup_string_template_starts_with_helper(
                    directive, m_section_open_marker) )
            {
                std::string name =
                    internal::markup_string_template_trim_helper(
                        directive.substr(m_section_open_marker.size()));
                internal::markup_string_template_token  t;
                t.kind = internal::markup_string_template_token_kind::section;
                t.text = name;
                _pos   = after_close;
                t.children = this->parse_token_list(_pos, name);
                out.push_back(std::move(t));
                continue;
            }

            // partial include?
            if ( (!m_partial_marker.empty()) &&
                 internal::markup_string_template_starts_with_helper(
                    directive, m_partial_marker) )
            {
                internal::markup_string_template_token  t;
                t.kind =
                    internal::markup_string_template_token_kind::partial_include;
                t.text = internal::markup_string_template_trim_helper(
                    directive.substr(m_partial_marker.size()));
                out.push_back(std::move(t));
                _pos = after_close;
                continue;
            }

            // comment?
            if ( (!m_comment_marker.empty()) &&
                 internal::markup_string_template_starts_with_helper(
                    directive, m_comment_marker) )
            {
                internal::markup_string_template_token  t;
                t.kind = internal::markup_string_template_token_kind::comment;
                // text intentionally discarded
                out.push_back(std::move(t));
                _pos = after_close;
                continue;
            }

            // raw variable?
            if ( (!m_raw_marker.empty()) &&
                 internal::markup_string_template_starts_with_helper(
                    directive, m_raw_marker) )
            {
                internal::markup_string_template_token  t;
                t.kind =
                    internal::markup_string_template_token_kind::variable_raw;
                t.text = internal::markup_string_template_trim_helper(
                    directive.substr(m_raw_marker.size()));
                out.push_back(std::move(t));
                _pos = after_close;
                continue;
            }

            // default: escaped variable.
            internal::markup_string_template_token  t;
            t.kind =
                internal::markup_string_template_token_kind::variable_escaped;
            t.text = directive;
            out.push_back(std::move(t));
            _pos = after_close;
        }

        return out;
    }

    // push_literal
    //   function: appends a literal token, merging with the
    // preceding token if it is also a literal (keeps the token
    // stream compact).
    void
    push_literal(
        std::vector<internal::markup_string_template_token>&    _out,
        const std::string&                                      _text
    )   const
    {
        if (_text.empty())
        {
            return;
        }
        if ( (!_out.empty()) &&
             (_out.back().kind ==
                internal::markup_string_template_token_kind::literal) )
        {
            _out.back().text += _text;
            return;
        }
        internal::markup_string_template_token  t;
        t.kind = internal::markup_string_template_token_kind::literal;
        t.text = _text;
        _out.push_back(std::move(t));
    }


    /// rendering

    // render_token_list_to
    //   function: walks a parsed token list and writes the
    // rendered output to `_out`. Recursive for sections and
    // partials.
    void
    render_token_list_to(
        std::ostream&                                                   _out,
        const std::vector<internal::markup_string_template_token>&      _tokens,
        const context_t&                                                _ctx
    )   const
    {
        for (std::size_t i = 0; i < _tokens.size(); ++i)
        {
            this->render_token_to(_out, _tokens[i], _ctx);
        }
    }

    // render_token_to
    //   function: dispatches a single token. Variable escaping
    // is delegated to `_EscapePolicy::escape`.
    void
    render_token_to(
        std::ostream&                                       _out,
        const internal::markup_string_template_token&       _tok,
        const context_t&                                    _ctx
    )   const
    {
        using kind_t = internal::markup_string_template_token_kind;

        switch (_tok.kind)
        {
            case kind_t::literal:
            {
                _out << _tok.text;
                break;
            }
            case kind_t::variable_escaped:
            {
                if (_ctx.has(_tok.text))
                {
                    _EscapePolicy::escape(_out, _ctx.get(_tok.text));
                }
                break;
            }
            case kind_t::variable_raw:
            {
                if (_ctx.has(_tok.text))
                {
                    _out << _ctx.get(_tok.text);
                }
                break;
            }
            case kind_t::partial_include:
            {
                this->render_partial_to(_out, _tok.text, _ctx);
                break;
            }
            case kind_t::section:
            {
                this->render_section_to(_out, _tok, _ctx);
                break;
            }
            case kind_t::comment:
            {
                // intentionally empty
                break;
            }
        }
    }

    // render_partial_to
    //   function: looks up `_name` first in the context (for
    // per-render overrides), then in this template's own
    // registry. Renders the resolved partial against `_ctx`. If
    // `_name` resolves to no partial, emits nothing.
    void
    render_partial_to(
        std::ostream&       _out,
        const std::string&  _name,
        const context_t&    _ctx
    )   const
    {
        const partial_ptr_t&    ctx_p = _ctx.find_partial(_name);
        if (ctx_p)
        {
            ctx_p->render_to(_out, _ctx);
            return;
        }
        typename partial_map_t::const_iterator  it = m_partials.find(_name);
        if (it != m_partials.end() && it->second)
        {
            it->second->render_to(_out, _ctx);
        }
    }

    // render_section_to
    //   function: renders a section. Three modes, in priority
    // order: (1) if the name resolves to a list, iterate and
    // render the body once per list entry with the entry as the
    // context; (2) if it resolves to a truthy scalar, render
    // the body once with the parent context; (3) otherwise
    // render nothing.
    void
    render_section_to(
        std::ostream&                                   _out,
        const internal::markup_string_template_token&   _tok,
        const context_t&                                _ctx
    )   const
    {
        if (_ctx.has_list(_tok.text))
        {
            const std::vector<context_t>&   items = _ctx.get_list(_tok.text);
            for (std::size_t i = 0; i < items.size(); ++i)
            {
                this->render_token_list_to(_out, _tok.children, items[i]);
            }
            return;
        }
        if (_ctx.is_truthy(_tok.text))
        {
            this->render_token_list_to(_out, _tok.children, _ctx);
        }
    }


    /// state

    // m_open_tag, m_close_tag
    //   field: delimiter strings. Either may be empty -- empty
    // open_tag disables interpolation entirely.
    std::string     m_open_tag;
    std::string     m_close_tag;

    // m_raw_marker, m_partial_marker, m_section_open_marker,
    // m_section_close_marker, m_comment_marker
    //   field: directive-classification markers. Each is matched
    // against the start of the trimmed directive body.
    std::string     m_raw_marker;
    std::string     m_partial_marker;
    std::string     m_section_open_marker;
    std::string     m_section_close_marker;
    std::string     m_comment_marker;

    // m_source
    //   field: the raw template text.
    std::string     m_source;

    // m_partials
    //   field: name -> partial registry consulted during render.
    partial_map_t   m_partials;

    // m_tokens
    //   field: cached parsed token tree. Mutable because the
    // cache is rebuilt lazily on the first render after any
    // public-state change.
    mutable std::vector<internal::markup_string_template_token>     m_tokens;

    // m_dirty
    //   field: true if m_tokens needs re-parsing before next
    // render. Mutable for the same reason as m_tokens.
    mutable bool    m_dirty;
};


///////////////////////////////////////////////////////////////////////////////
///                VI.   FREE HELPERS / FACTORIES                           ///
///////////////////////////////////////////////////////////////////////////////

// make_markup_string_template
//   function: factory that constructs a template from source
// text and a syntax-policy tag in a single expression. The
// escape policy must be supplied as the leading explicit
// template argument.
template<typename _EscapePolicy,
         typename _Syntax>
inline markup_string_template<_EscapePolicy>
make_markup_string_template(
    const std::string&  _source,
    _Syntax             _syntax_tag = _Syntax()
)
{
    return markup_string_template<_EscapePolicy>(_source, _syntax_tag);
}


// make_shared_markup_string_template
//   function: factory that returns a shared_ptr-wrapped
// template, ready to be registered as a partial.
template<typename _EscapePolicy,
         typename _Syntax>
inline std::shared_ptr<markup_string_template<_EscapePolicy>>
make_shared_markup_string_template(
    const std::string&  _source,
    _Syntax             _syntax_tag = _Syntax()
)
{
    return std::make_shared<markup_string_template<_EscapePolicy>>(
        _source, _syntax_tag);
}


NS_END  // djinterp


#endif  // DJINTERP_MARKUP_STRING_TEMPLATE_