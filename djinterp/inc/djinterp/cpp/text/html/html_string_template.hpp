/*******************************************************************************
* djinterp [cpp]                                        html_string_template.hpp
*
*   HTML facade over `DMarkupStringTemplate<EscapePolicy>`. Binds
* `DHtmlEscapePolicy` and re-exports the templating engine under
* HTML-flavoured names. All the actual parsing, rendering, partial
* resolution, section iteration, and context machinery lives in
* `markup_string_template.hpp`; this file is a thin subclass plus a
* matching context subclass plus convenience aliases.
*
*   ZERO OVERHEAD:
*   `DHtmlStringTemplate` adds NO members beyond
* `DMarkupStringTemplate<DHtmlEscapePolicy>`, so memory layout is
* identical and slicing is harmless. Constructors are inherited via
* `using base::base`. The subclass exists for clean type names in
* error messages and to leave room for HTML-specific extensions
* later.
*
*   USAGE:
*     html::DHtmlStringTemplate t("<p>{name}</p>");
*     html::DHtmlStringTemplateContext ctx;
*     ctx.set("name", "Hello & World");
*     std::string out = t.render(ctx);   // <p>Hello &amp; World</p>
*
*
* path:      /inc/djinterp/cpp/text/html/html_string_template.hpp
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
#include "../../djinterp.hpp"
#include "../markup_string_template.hpp"
#include "./html.hpp"


NS_DJINTERP

namespace html {


///////////////////////////////////////////////////////////////////////////////
///                HTML STRING TEMPLATE FACADE                              ///
///////////////////////////////////////////////////////////////////////////////

// DHtmlStringTemplateContext
//   class: thin facade over
// `DMarkupStringTemplateContext<DHtmlEscapePolicy>`.
// Inherits every constructor and method via `using base::base`;
// adds nothing.
class DHtmlStringTemplateContext
:   public ::djinterp::DMarkupStringTemplateContext<
        ::djinterp::DHtmlEscapePolicy>
{
public:
    // base_type
    //   type: alias for the underlying template-context.
    using base_type = ::djinterp::DMarkupStringTemplateContext<
        ::djinterp::DHtmlEscapePolicy>;

    using base_type::base_type;
};


// DHtmlStringTemplate
//   class: thin facade over
// `DMarkupStringTemplate<DHtmlEscapePolicy>`. Inherits every
// constructor and method via `using base::base`; adds nothing.
class DHtmlStringTemplate
:   public ::djinterp::DMarkupStringTemplate<
        ::djinterp::DHtmlEscapePolicy>
{
public:
    // base_type
    //   type: alias for the underlying template engine.
    using base_type = ::djinterp::DMarkupStringTemplate<
        ::djinterp::DHtmlEscapePolicy>;

    using base_type::base_type;
};


///////////////////////////////////////////////////////////////////////////////
///                HTML-FLAVOURED ALIASES                                   ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases preserving the prior `html_string_template_*`
// naming for syntax policies. The underlying types are the shared
// `markup_string_template_syntax_*` structs from
// `markup_string_template.hpp`; using either name is equivalent.
using DHtmlStringTemplateSyntaxDefault      =
    ::djinterp::DMarkupStringTemplateSyntaxDefault;
using DHtmlStringTemplateSyntaxHandlebars   =
    ::djinterp::DMarkupStringTemplateSyntaxHandlebars;
using DHtmlStringTemplateSyntaxErb          =
    ::djinterp::DMarkupStringTemplateSyntaxErb;
using DHtmlStringTemplateSyntaxDollar       =
    ::djinterp::DMarkupStringTemplateSyntaxDollar;
using DHtmlStringTemplateSyntaxAngle        =
    ::djinterp::DMarkupStringTemplateSyntaxAngle;
using DHtmlStringTemplateSyntaxSquare       =
    ::djinterp::DMarkupStringTemplateSyntaxSquare;
using DHtmlStringTemplateSyntaxPhp          =
    ::djinterp::DMarkupStringTemplateSyntaxPhp;


///////////////////////////////////////////////////////////////////////////////
///                FACTORY HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// make_html_string_template
//   function: factory that constructs an HTML template from
// source text and a syntax-policy tag in a single expression.
template<typename Syntax>
inline
DHtmlStringTemplate make_html_string_template(
    const std::string& _source,
    Syntax             _syntax_tag = Syntax()
)
{
    return DHtmlStringTemplate(_source, _syntax_tag);
}


// make_shared_html_string_template
//   function: factory that returns a shared_ptr-wrapped HTML
// template, ready to be registered as a partial.
template<typename Syntax>
inline
std::shared_ptr<DHtmlStringTemplate> make_shared_html_string_template(
    const std::string& _source,
    Syntax             _syntax_tag = Syntax()
)
{
    return std::make_shared<DHtmlStringTemplate>(_source, _syntax_tag);
}


}   // namespace html
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_HTML_HTML_STRING_TEMPLATE_HPP
