/*******************************************************************************
* djinterp [test]                                         html_tests_printer.cpp
*
*   The group for html/html_printer.hpp: DHtmlPrintPolicy's escaping and
* element forms, and a document of cpp/util/document printed through it, at
* compile time (DHtmlDocument) and boxed (html_policy). Kept apart from the
* tree's unit: html_printer.hpp includes cpp/util/document/document.hpp,
* whose DXmlDocument clashes with xml_template.hpp's, until the clash is
* settled.
*
*
* path:      /tests/djinterp/cpp/text/html_tests_printer.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "html_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <string>  // std::string
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/html/html_printer.hpp"  // policy


#if D_ENV_LANG_IS_CPP17_OR_HIGHER

/*
d_tests_html_printer_build
  Builds the same small document in _doc: a div with a quoted title, an
empty br, a p of text, an empty span and an empty param.
*/
template<typename Document>
static void
d_tests_html_printer_build(
    Document& _doc
)
{
    djinterp::DCursor root = _doc.root("div");

    root.attr("title", "a\"b");
    (void)root.open_child("br");
    (void)root.open_child("p").text("x < y & z");
    (void)root.open_child("span");
    (void)root.open_child("param");

    return;
}


/*
d_tests_html_printer
  Tests the following:
  - escape_text escapes &, < and >; escape_attribute the double quote too
  - form_of: a void element is voidlike, empty or not; any other, a custom
    one included, is paired; param, once unknown, is void
  - a document printed through the policy: <br> and <param> with no end tag
    and no slash, <span></span> paired, text and values escaped
  - html_policy(), boxed, prints the same text
*/
void
d_tests_html_printer(void)
{
    using djinterp::DElementForm;
    using djinterp::DHtmlPrintPolicy;

    djinterp::DHtmlDocument                       doc;
    djinterp::DDocument<djinterp::DBoxedPrintPolicy> boxed(
                                                  djinterp::html_policy());
    std::string                                   out;

    D_TESTS_TEXT_CHECK(DHtmlPrintPolicy::escape_text("a<b>&\"c\"") ==
                       "a&lt;b&gt;&amp;\"c\"");
    D_TESTS_TEXT_CHECK(DHtmlPrintPolicy::escape_attribute("a<b>&\"c\"") ==
                       "a&lt;b&gt;&amp;&quot;c&quot;");

    D_TESTS_TEXT_CHECK(
        (DHtmlPrintPolicy::form_of("br", true) == DElementForm::voidlike) &&
        (DHtmlPrintPolicy::form_of("br", false) == DElementForm::voidlike) &&
        (DHtmlPrintPolicy::form_of("param", true) == DElementForm::voidlike));
    D_TESTS_TEXT_CHECK(
        (DHtmlPrintPolicy::form_of("div", true) == DElementForm::paired) &&
        (DHtmlPrintPolicy::form_of("div", false) == DElementForm::paired) &&
        (DHtmlPrintPolicy::form_of("my-widget", true) ==
         DElementForm::paired));

    d_tests_html_printer_build(doc);
    out = doc.to_string();
    D_TESTS_TEXT_CHECK((out.find("<br>") != std::string::npos) &&
                       (out.find("</br>") == std::string::npos) &&
                       (out.find("<br/>") == std::string::npos));
    D_TESTS_TEXT_CHECK((out.find("<param>") != std::string::npos) &&
                       (out.find("</param>") == std::string::npos));
    D_TESTS_TEXT_CHECK(out.find("<span></span>") != std::string::npos);
    D_TESTS_TEXT_CHECK(out.find("<p>x &lt; y &amp; z</p>") !=
                       std::string::npos);
    D_TESTS_TEXT_CHECK(out.find("title=\"a&quot;b\"") != std::string::npos);

    d_tests_html_printer_build(boxed);
    D_TESTS_TEXT_CHECK(boxed.to_string() == out);

    return;
}

#endif  // C++17
