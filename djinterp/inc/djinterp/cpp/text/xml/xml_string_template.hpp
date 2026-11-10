/*******************************************************************************
* djinterp [cpp]                                         xml_string_template.hpp
*
*   XML facade over `DMarkupStringTemplate<EscapePolicy>`. Binds
* `DXmlEscapePolicy` and re-exports the templating engine under
* XML-flavoured names. All the actual parsing, rendering, partial
* resolution, section iteration, and context machinery lives in
* `markup_string_template.hpp`; this file is a thin subclass plus a
* matching context subclass plus convenience aliases.
*
*   ZERO OVERHEAD:
*   `DXmlStringTemplate` adds NO members beyond
* `DMarkupStringTemplate<DXmlEscapePolicy>`, so memory layout is
* identical and slicing is harmless. Constructors are inherited via
* `using base::base`. The subclass exists for clean type names in
* error messages and to leave room for XML-specific extensions
* later.
*
*   USAGE:
*     DXmlStringTemplate t("<msg>{name}</msg>");
*     DXmlStringTemplateContext ctx;
*     ctx.set("name", "<world>");
*     std::string out = t.render(ctx);   // <msg>&lt;world&gt;</msg>
*
*
* path:      /inc/djinterp/cpp/text/xml/xml_string_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.09
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_TEXT_XML_XML_STRING_TEMPLATE_HPP
#define DJINTERP_TEXT_XML_XML_STRING_TEMPLATE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// djinterp
#include "../../djinterp.hpp"
#include "../markup_string_template.hpp"
#include "./xml.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                XML STRING TEMPLATE FACADE                               ///
///////////////////////////////////////////////////////////////////////////////

// DXmlStringTemplateContext
//   class: thin facade over
// `DMarkupStringTemplateContext<DXmlEscapePolicy>`. Inherits
// every constructor and method via `using base::base`; adds
// nothing.
class DXmlStringTemplateContext
:   public DMarkupStringTemplateContext<DXmlEscapePolicy>
{
public:
    // base_type
    //   type: alias for the underlying template-context.
    using base_type = DMarkupStringTemplateContext<DXmlEscapePolicy>;

    using base_type::base_type;
};


// DXmlStringTemplate
//   class: thin facade over
// `DMarkupStringTemplate<DXmlEscapePolicy>`. Inherits every
// constructor and method via `using base::base`; adds nothing.
class DXmlStringTemplate
:   public DMarkupStringTemplate<DXmlEscapePolicy>
{
public:
    // base_type
    //   type: alias for the underlying template engine.
    using base_type = DMarkupStringTemplate<DXmlEscapePolicy>;

    using base_type::base_type;
};


///////////////////////////////////////////////////////////////////////////////
///                XML-FLAVOURED ALIASES                                    ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases preserving the prior `xml_string_template_*`
// naming for syntax policies. The underlying types are the shared
// `markup_string_template_syntax_*` structs from
// `markup_string_template.hpp`; using either name is equivalent.
using DXmlStringTemplateSyntaxDefault         =
    DMarkupStringTemplateSyntaxDefault;
using DXmlStringTemplateSyntaxHandlebars   =
    DMarkupStringTemplateSyntaxHandlebars;
using DXmlStringTemplateSyntaxErb          =
    DMarkupStringTemplateSyntaxErb;
using DXmlStringTemplateSyntaxDollar       =
    DMarkupStringTemplateSyntaxDollar;
using DXmlStringTemplateSyntaxSquare       =
    DMarkupStringTemplateSyntaxSquare;
using DXmlStringTemplateSyntaxCurlyDollar =
    DMarkupStringTemplateSyntaxCurlyDollar;
using DXmlStringTemplateSyntaxXmlPi       =
    DMarkupStringTemplateSyntaxXmlPi;


///////////////////////////////////////////////////////////////////////////////
///                FACTORY HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// make_xml_string_template
//   function: factory that constructs an XML template from
// source text and a syntax-policy tag in a single expression.
template<typename Syntax>
inline
DXmlStringTemplate make_xml_string_template(
    const std::string& _source,
    Syntax             _syntax_tag = Syntax()
)
{
    return DXmlStringTemplate(_source, _syntax_tag);
}


// make_shared_xml_string_template
//   function: factory that returns a shared_ptr-wrapped XML
// template, ready to be registered as a partial.
template<typename Syntax>
inline
std::shared_ptr<DXmlStringTemplate> make_shared_xml_string_template(
    const std::string& _source,
    Syntax             _syntax_tag = Syntax()
)
{
    return std::make_shared<DXmlStringTemplate>(_source, _syntax_tag);
}


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_TEXT_XML_XML_STRING_TEMPLATE_HPP
