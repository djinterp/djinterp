/*******************************************************************************
* djinterp [test]                                markup_string_tests_facades.cpp
*
*   The groups for the two facades over the markup string template:
* html/html_string_template.hpp, in namespace djinterp::html, and
* xml/xml_string_template.hpp, in namespace djinterp. Each is the engine
* with its escape policy bound, under its own names: the banner's example,
* the policy's apostrophe, the base's types and size, the syntax aliases, the
* inherited constructors, the factories, and a facade template as another's
* partial.
*
*
* path:      /tests/djinterp/cpp/text/markup_string_tests_facades.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markup_string_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <memory>  // std::shared_ptr
#include <string>  // std::string
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_base_of, std::is_same
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/html/html_string_template.hpp"  // html
#include "../../../../inc/djinterp/cpp/text/xml/xml_string_template.hpp"  // xml


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
d_tests_markup_string_html
  Tests the following:
  - the banner's example: "Hello & World" in "<p>{name}</p>"
  - the apostrophe is HTML's numeric reference, &#39;
  - DHtmlStringTemplate and its context derive from the engine's, with the
    HTML policy, and add nothing to their size
  - the seven syntax aliases are the engine's syntax types
  - the inherited constructors: a syntax tag, and explicit delimiters
  - the factories, by value and shared; a shared one is another template's
    partial
  - a facade context is a list item
*/
void
d_tests_markup_string_html(void)
{
    using djinterp::DHtmlEscapePolicy;
    using djinterp::DMarkupStringTemplate;
    using djinterp::DMarkupStringTemplateContext;
    using namespace djinterp::html;

    typedef DMarkupStringTemplate<DHtmlEscapePolicy>        base_template;
    typedef DMarkupStringTemplateContext<DHtmlEscapePolicy> base_context;

    const DHtmlStringTemplate banner("<p>{name}</p>");
    const DHtmlStringTemplate bars("{{name}}",
                                   DHtmlStringTemplateSyntaxHandlebars());
    const DHtmlStringTemplate angle("<<name>>", "<<", ">>");
    const DHtmlStringTemplate made =
        make_html_string_template<DHtmlStringTemplateSyntaxSquare>("[[name]]");
    DHtmlStringTemplate        page("<div>{>inner}</div>");
    const DHtmlStringTemplate  rows("{#rows}[{name}]{/rows}");
    DHtmlStringTemplateContext ctx;
    DHtmlStringTemplateContext item;
    DHtmlStringTemplateContext list;

    ctx.set("name", "Hello & World");
    D_TESTS_TEXT_CHECK(banner.render(ctx) == "<p>Hello &amp; World</p>");

    ctx.set("name", "it's");
    D_TESTS_TEXT_CHECK(banner.render(ctx) == "<p>it&#39;s</p>");

    D_TESTS_TEXT_CHECK(
        (std::is_base_of<base_template, DHtmlStringTemplate>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_base_of<base_context, DHtmlStringTemplateContext>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_same<DHtmlStringTemplate::escape_policy_t,
                      DHtmlEscapePolicy>::value));
    D_TESTS_TEXT_CHECK(sizeof(DHtmlStringTemplate) == sizeof(base_template));
    D_TESTS_TEXT_CHECK(sizeof(DHtmlStringTemplateContext) ==
                       sizeof(base_context));

    D_TESTS_TEXT_CHECK(
        (std::is_same<DHtmlStringTemplateSyntaxDefault,
                      djinterp::DMarkupStringTemplateSyntaxDefault>::value) &&
        (std::is_same<DHtmlStringTemplateSyntaxHandlebars,
                      djinterp::DMarkupStringTemplateSyntaxHandlebars>::
                          value) &&
        (std::is_same<DHtmlStringTemplateSyntaxErb,
                      djinterp::DMarkupStringTemplateSyntaxErb>::value) &&
        (std::is_same<DHtmlStringTemplateSyntaxDollar,
                      djinterp::DMarkupStringTemplateSyntaxDollar>::value) &&
        (std::is_same<DHtmlStringTemplateSyntaxAngle,
                      djinterp::DMarkupStringTemplateSyntaxAngle>::value) &&
        (std::is_same<DHtmlStringTemplateSyntaxSquare,
                      djinterp::DMarkupStringTemplateSyntaxSquare>::value) &&
        (std::is_same<DHtmlStringTemplateSyntaxPhp,
                      djinterp::DMarkupStringTemplateSyntaxPhp>::value));

    D_TESTS_TEXT_CHECK(bars.render(ctx) == "it&#39;s");
    D_TESTS_TEXT_CHECK(angle.render(ctx) == "it&#39;s");
    D_TESTS_TEXT_CHECK(made.render(ctx) == "it&#39;s");

    page.add_partial("inner",
        make_shared_html_string_template<DHtmlStringTemplateSyntaxDefault>(
            "{name}"));
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<div>it&#39;s</div>");

    item.set("name", "<i>");
    list.append_to_list("rows", item);
    D_TESTS_TEXT_CHECK(rows.render(list) == "[&lt;i&gt;]");

    return;
}


/*
d_tests_markup_string_xml
  Tests the following:
  - the banner's example: "<world>" in "<msg>{name}</msg>"
  - the apostrophe is XML's entity, &apos;
  - DXmlStringTemplate and its context derive from the engine's, with the
    XML policy, and add nothing to their size
  - the seven syntax aliases are the engine's syntax types
  - the processing-instruction syntax, through the factory
  - the inherited constructors and the shared factory; a shared template
    is another's partial
*/
void
d_tests_markup_string_xml(void)
{
    using djinterp::DMarkupStringTemplate;
    using djinterp::DMarkupStringTemplateContext;
    using djinterp::DXmlEscapePolicy;
    using djinterp::DXmlStringTemplate;
    using djinterp::DXmlStringTemplateContext;

    typedef DMarkupStringTemplate<DXmlEscapePolicy>        base_template;
    typedef DMarkupStringTemplateContext<DXmlEscapePolicy> base_context;

    const DXmlStringTemplate banner("<msg>{name}</msg>");
    const DXmlStringTemplate pi =
        djinterp::make_xml_string_template<
            djinterp::DXmlStringTemplateSyntaxXmlPi>("<r><?tpl name ?></r>");
    const DXmlStringTemplate curly(
        "${name}", djinterp::DXmlStringTemplateSyntaxCurlyDollar());
    const DXmlStringTemplate square("[[name]]", "[[", "]]");
    DXmlStringTemplate        doc("<doc>{>part}</doc>");
    DXmlStringTemplateContext ctx;

    ctx.set("name", "<world>");
    D_TESTS_TEXT_CHECK(banner.render(ctx) == "<msg>&lt;world&gt;</msg>");
    D_TESTS_TEXT_CHECK(pi.render(ctx) == "<r>&lt;world&gt;</r>");

    ctx.set("name", "it's");
    D_TESTS_TEXT_CHECK(banner.render(ctx) == "<msg>it&apos;s</msg>");

    D_TESTS_TEXT_CHECK(
        (std::is_base_of<base_template, DXmlStringTemplate>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_base_of<base_context, DXmlStringTemplateContext>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_same<DXmlStringTemplate::escape_policy_t,
                      DXmlEscapePolicy>::value));
    D_TESTS_TEXT_CHECK(sizeof(DXmlStringTemplate) == sizeof(base_template));
    D_TESTS_TEXT_CHECK(sizeof(DXmlStringTemplateContext) ==
                       sizeof(base_context));

    D_TESTS_TEXT_CHECK(
        (std::is_same<djinterp::DXmlStringTemplateSyntaxDefault,
                      djinterp::DMarkupStringTemplateSyntaxDefault>::value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxHandlebars,
                      djinterp::DMarkupStringTemplateSyntaxHandlebars>::
                          value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxErb,
                      djinterp::DMarkupStringTemplateSyntaxErb>::value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxDollar,
                      djinterp::DMarkupStringTemplateSyntaxDollar>::value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxSquare,
                      djinterp::DMarkupStringTemplateSyntaxSquare>::value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxCurlyDollar,
                      djinterp::DMarkupStringTemplateSyntaxCurlyDollar>::
                          value) &&
        (std::is_same<djinterp::DXmlStringTemplateSyntaxXmlPi,
                      djinterp::DMarkupStringTemplateSyntaxXmlPi>::value));

    D_TESTS_TEXT_CHECK(curly.render(ctx) == "it&apos;s");
    D_TESTS_TEXT_CHECK(square.render(ctx) == "it&apos;s");

    doc.add_partial("part",
        djinterp::make_shared_xml_string_template<
            djinterp::DXmlStringTemplateSyntaxDefault>("<p>{name}</p>"));
    D_TESTS_TEXT_CHECK(doc.render(ctx) == "<doc><p>it&apos;s</p></doc>");

    return;
}

#endif  // C++11
