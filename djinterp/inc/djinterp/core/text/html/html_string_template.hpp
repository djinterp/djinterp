/*******************************************************************************
* djinterp [core]                                       html_string_template.hpp
*
*   HTML facade over `markup_string_template<EscapePolicy>`. Binds
* `html_escape_policy` and re-exports the templating engine under
* HTML-flavoured names. All the actual parsing, rendering, partial
* resolution, section iteration, and context machinery lives in
* `markup_string_template.hpp`; this file is a thin subclass plus a
* matching context subclass plus convenience aliases.
*
*   ZERO OVERHEAD:
*   `html_string_template` adds NO members beyond
* `markup_string_template<html_escape_policy>`, so memory layout is
* identical and slicing is harmless. Constructors are inherited via
* `using base::base`. The subclass exists for clean type names in
* error messages and to leave room for HTML-specific extensions
* later.
*
*   USAGE:
*     html::html_string_template t("<p>{name}</p>");
*     html::html_string_template_context ctx;
*     ctx.set("name", "Hello & World");
*     std::string out = t.render(ctx);   // <p>Hello &amp; World</p>
*
*
* path:      /inc/djinterp/core/text/html/html_string_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_TEXT_HTML_HTML_STRING_TEMPLATE_HPP
#define DJINTERP_TEXT_HTML_HTML_STRING_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../../djinterp.hpp"
#include "../markup_string_template.hpp"
#include "./html.hpp"


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                HTML STRING TEMPLATE FACADE                              ///
///////////////////////////////////////////////////////////////////////////////

// html_string_template_context
//   class: thin facade over
// `markup_string_template_context<html_escape_policy>`.
// Inherits every constructor and method via `using base::base`;
// adds nothing.
class html_string_template_context
:   public ::djinterp::markup_string_template_context<
        ::djinterp::html_escape_policy>
{
public:
    // base_type
    //   type: alias for the underlying template-context.
    using base_type = ::djinterp::markup_string_template_context<
        ::djinterp::html_escape_policy>;

    using base_type::base_type;
};


// html_string_template
//   class: thin facade over
// `markup_string_template<html_escape_policy>`. Inherits every
// constructor and method via `using base::base`; adds nothing.
class html_string_template
:   public ::djinterp::markup_string_template<
        ::djinterp::html_escape_policy>
{
public:
    // base_type
    //   type: alias for the underlying template engine.
    using base_type = ::djinterp::markup_string_template<
        ::djinterp::html_escape_policy>;

    using base_type::base_type;
};


///////////////////////////////////////////////////////////////////////////////
///                HTML-FLAVOURED ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases preserving the prior `html_string_template_*`
// naming for syntax policies. The underlying types are the shared
// `markup_string_template_syntax_*` structs from
// `markup_string_template.hpp`; using either name is equivalent.
using html_string_template_syntax_default      =
    ::djinterp::markup_string_template_syntax_default;
using html_string_template_syntax_handlebars   =
    ::djinterp::markup_string_template_syntax_handlebars;
using html_string_template_syntax_erb          =
    ::djinterp::markup_string_template_syntax_erb;
using html_string_template_syntax_dollar       =
    ::djinterp::markup_string_template_syntax_dollar;
using html_string_template_syntax_angle        =
    ::djinterp::markup_string_template_syntax_angle;
using html_string_template_syntax_square       =
    ::djinterp::markup_string_template_syntax_square;
using html_string_template_syntax_php          =
    ::djinterp::markup_string_template_syntax_php;


///////////////////////////////////////////////////////////////////////////////
///                FACTORY HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// make_html_string_template
//   function: factory that constructs an HTML template from
// source text and a syntax-policy tag in a single expression.
template<typename Syntax>
inline html_string_template
make_html_string_template(
    const std::string&  _source,
    Syntax              _syntax_tag = Syntax()
)
{
    return html_string_template(_source, _syntax_tag);
}


// make_shared_html_string_template
//   function: factory that returns a shared_ptr-wrapped HTML
// template, ready to be registered as a partial.
template<typename Syntax>
inline std::shared_ptr<html_string_template>
make_shared_html_string_template(
    const std::string&  _source,
    Syntax              _syntax_tag = Syntax()
)
{
    return std::make_shared<html_string_template>(_source, _syntax_tag);
}


}   // namespace html
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_HTML_HTML_STRING_TEMPLATE_HPP
