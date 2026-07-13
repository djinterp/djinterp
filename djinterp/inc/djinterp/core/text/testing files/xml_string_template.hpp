/******************************************************************************
* djinterp [xml]                                        xml_string_template.hpp
*
*   XML facade over `markup_string_template<_EscapePolicy>`. Binds
* `xml_escape_policy` and re-exports the templating engine under
* XML-flavoured names. All the actual parsing, rendering, partial
* resolution, section iteration, and context machinery lives in
* `markup_string_template.hpp`; this file is a thin subclass plus a
* matching context subclass plus convenience aliases.
*
*   ZERO OVERHEAD:
*   `xml_string_template` adds NO members beyond
* `markup_string_template<xml_escape_policy>`, so memory layout is
* identical and slicing is harmless. Constructors are inherited via
* `using base::base`. The subclass exists for clean type names in
* error messages and to leave room for XML-specific extensions
* later.
*
*   USAGE:
*     xml_string_template t("<msg>{name}</msg>");
*     xml_string_template_context ctx;
*     ctx.set("name", "<world>");
*     std::string out = t.render(ctx);   // <msg>&lt;world&gt;</msg>
*
*
* path:      /inc/djinterp/core/util/xml/xml_string_template.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.05.09
******************************************************************************/

#ifndef DJINTERP_XML_STRING_TEMPLATE_
#define DJINTERP_XML_STRING_TEMPLATE_ 1

// djinterp
#include "../../../djinterp.hpp"
#include "../markup_string_template.hpp"
#include "./xml.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                XML STRING TEMPLATE FACADE                               ///
///////////////////////////////////////////////////////////////////////////////

// xml_string_template_context
//   class: thin facade over
// `markup_string_template_context<xml_escape_policy>`. Inherits
// every constructor and method via `using base::base`; adds
// nothing.
class xml_string_template_context
:   public markup_string_template_context<xml_escape_policy>
{
public:
    // base_type
    //   type: alias for the underlying template-context.
    using base_type = markup_string_template_context<xml_escape_policy>;

    using base_type::base_type;
};


// xml_string_template
//   class: thin facade over
// `markup_string_template<xml_escape_policy>`. Inherits every
// constructor and method via `using base::base`; adds nothing.
class xml_string_template
:   public markup_string_template<xml_escape_policy>
{
public:
    // base_type
    //   type: alias for the underlying template engine.
    using base_type = markup_string_template<xml_escape_policy>;

    using base_type::base_type;
};


///////////////////////////////////////////////////////////////////////////////
///                XML-FLAVOURED ALIASES                                    ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases preserving the prior `xml_string_template_*`
// naming for syntax policies. The underlying types are the shared
// `markup_string_template_syntax_*` structs from
// `markup_string_template.hpp`; using either name is equivalent.
using xml_string_template_syntax_default      =
    markup_string_template_syntax_default;
using xml_string_template_syntax_handlebars   =
    markup_string_template_syntax_handlebars;
using xml_string_template_syntax_erb          =
    markup_string_template_syntax_erb;
using xml_string_template_syntax_dollar       =
    markup_string_template_syntax_dollar;
using xml_string_template_syntax_square       =
    markup_string_template_syntax_square;
using xml_string_template_syntax_curly_dollar =
    markup_string_template_syntax_curly_dollar;
using xml_string_template_syntax_xml_pi       =
    markup_string_template_syntax_xml_pi;


///////////////////////////////////////////////////////////////////////////////
///                FACTORY HELPERS                                          ///
///////////////////////////////////////////////////////////////////////////////

// make_xml_string_template
//   function: factory that constructs an XML template from
// source text and a syntax-policy tag in a single expression.
template<typename _Syntax>
inline xml_string_template
make_xml_string_template(
    const std::string&  _source,
    _Syntax             _syntax_tag = _Syntax()
)
{
    return xml_string_template(_source, _syntax_tag);
}


// make_shared_xml_string_template
//   function: factory that returns a shared_ptr-wrapped XML
// template, ready to be registered as a partial.
template<typename _Syntax>
inline std::shared_ptr<xml_string_template>
make_shared_xml_string_template(
    const std::string&  _source,
    _Syntax             _syntax_tag = _Syntax()
)
{
    return std::make_shared<xml_string_template>(_source, _syntax_tag);
}


NS_END  // djinterp


#endif  // DJINTERP_XML_STRING_TEMPLATE_
