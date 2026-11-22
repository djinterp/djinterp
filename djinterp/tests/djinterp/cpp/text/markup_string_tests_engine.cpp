/*******************************************************************************
* djinterp [cpp][text]                            markup_string_tests_engine.cpp
*
*   The groups for markup_string_template.hpp: the escape policies, variables,
* sections, literals, partials, the bundled syntaxes, and a template's life
* from construction to copy. The template under test escapes for XML unless a
* group says otherwise, so that escaping shows in what it renders.
*
*
* path:      /tests/djinterp/cpp/text/markup_string_tests_engine.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "markup_string_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <memory>   // std::make_shared, std::shared_ptr
#include <sstream>  // std::ostringstream
#include <string>   // std::string
#include <vector>   // std::vector
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <type_traits>  // std::is_base_of
#endif
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"  // framework root
#include "../../../../inc/djinterp/cpp/text/markup_string_template.hpp"  // engine


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// xml_template, xml_context
//   the template under test, escaping for XML, and its context.
typedef djinterp::DMarkupStringTemplate<djinterp::DXmlEscapePolicy>
    xml_template;
typedef xml_template::context_t xml_context;


/*
d_tests_markup_string_escaped
  What an escape policy writes for _in.
*/
template<typename Policy>
static std::string
d_tests_markup_string_escaped(
    const std::string& _in
)
{
    std::ostringstream out;

    Policy::escape(out, _in);

    return out.str();
}


/*
d_tests_markup_string_escape
  Tests the following:
  - DXmlEscapePolicy writes &amp; &lt; &gt; &quot; &apos;
  - DHtmlEscapePolicy writes the same, but &#39; for the apostrophe
  - DNullEscapePolicy writes its input
  - every other byte passes through, UTF-8 included, and nothing is
    unescaped first: "&amp;" becomes "&amp;amp;"
  - the three are escape policies by their tag
*/
void
d_tests_markup_string_escape(void)
{
    using djinterp::DHtmlEscapePolicy;
    using djinterp::DMarkupEscapePolicyTag;
    using djinterp::DNullEscapePolicy;
    using djinterp::DXmlEscapePolicy;

    const std::string all   = "a&b<c>d\"e'f";
    const std::string other = "plain text, 100% \xC3\xA9";

    D_TESTS_TEXT_CHECK(d_tests_markup_string_escaped<DXmlEscapePolicy>(all) ==
                       "a&amp;b&lt;c&gt;d&quot;e&apos;f");
    D_TESTS_TEXT_CHECK(d_tests_markup_string_escaped<DHtmlEscapePolicy>(all) ==
                       "a&amp;b&lt;c&gt;d&quot;e&#39;f");
    D_TESTS_TEXT_CHECK(d_tests_markup_string_escaped<DNullEscapePolicy>(all) ==
                       all);

    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_escaped<DXmlEscapePolicy>(other) == other);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_escaped<DHtmlEscapePolicy>(other) == other);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_escaped<DXmlEscapePolicy>("&amp;") ==
        "&amp;amp;");
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_escaped<DXmlEscapePolicy>("").empty());

    D_TESTS_TEXT_CHECK(
        (std::is_base_of<DMarkupEscapePolicyTag, DXmlEscapePolicy>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_base_of<DMarkupEscapePolicyTag, DHtmlEscapePolicy>::value));
    D_TESTS_TEXT_CHECK(
        (std::is_base_of<DMarkupEscapePolicyTag, DNullEscapePolicy>::value));

    return;
}


/*
d_tests_markup_string_variables
  Tests the following:
  - {name} is escaped and {&name} is not
  - blanks inside the delimiters are trimmed, after the raw marker too
  - a name with no binding renders nothing, escaped or raw
  - integers (negative too) and booleans bind as text
  - has and get, with get's empty string for a missing name
  - a later set replaces a value
*/
void
d_tests_markup_string_variables(void)
{
    const xml_template both("<p>{name}</p><q>{&name}</q>");
    const xml_template padded("[{ name }][{& name }]");
    const xml_template missing("[{nobody}][{&nobody}]");
    const xml_template typed("{n}|{neg}|{yes}|{no}");
    xml_context        ctx;
    xml_context        values;

    ctx.set("name", "Tom & <Jerry>");
    values.set("n", 42);
    values.set("neg", -7);
    values.set_bool("yes", true);
    values.set_bool("no", false);

    D_TESTS_TEXT_CHECK(both.render(ctx) ==
                       "<p>Tom &amp; &lt;Jerry&gt;</p><q>Tom & <Jerry></q>");
    D_TESTS_TEXT_CHECK(padded.render(ctx) ==
                       "[Tom &amp; &lt;Jerry&gt;][Tom & <Jerry>]");
    D_TESTS_TEXT_CHECK(missing.render(ctx) == "[][]");
    D_TESTS_TEXT_CHECK(typed.render(values) == "42|-7|true|false");

    D_TESTS_TEXT_CHECK(ctx.has("name") && !ctx.has("nobody"));
    D_TESTS_TEXT_CHECK(ctx.get("name") == "Tom & <Jerry>");
    D_TESTS_TEXT_CHECK(ctx.get("nobody").empty());

    ctx.set("name", "x");
    D_TESTS_TEXT_CHECK(both.render(ctx) == "<p>x</p><q>x</q>");

    return;
}


/*
d_tests_markup_string_sections
  Tests the following:
  - a section renders its body, with the parent's context, when its name
    is bound to a truthy value; "false", "0", "" and no binding are not
    truthy
  - a list renders the body once per item, each item as the context
  - an empty list renders nothing, even beside a truthy scalar of the
    same name: the list comes first
  - a list item's body sees the item's bindings only, not the parent's
  - sections nest
*/
void
d_tests_markup_string_sections(void)
{
    const xml_template gate("[{#show}on {what}{/show}]");
    const xml_template list("<ul>{#items}<li>{name}</li>{/items}</ul>");
    const xml_template scope("{#items}{name}/{title};{/items}");
    const xml_template nested("{#outer}({#inner}x{/inner}){/outer}");
    xml_context        on;
    xml_context        off_false;
    xml_context        off_zero;
    xml_context        off_empty;
    xml_context        unbound;
    xml_context        first;
    xml_context        second;
    xml_context        items;
    xml_context        both;
    xml_context        parent;
    xml_context        nest;

    on.set("show", "yes");
    on.set("what", "air");
    off_false.set("show", "false");
    off_zero.set("show", "0");
    off_empty.set("show", "");

    D_TESTS_TEXT_CHECK(gate.render(on) == "[on air]");
    D_TESTS_TEXT_CHECK((gate.render(off_false) == "[]") &&
                       (gate.render(off_zero) == "[]"));
    D_TESTS_TEXT_CHECK((gate.render(off_empty) == "[]") &&
                       (gate.render(unbound) == "[]"));
    D_TESTS_TEXT_CHECK(on.is_truthy("show") && !off_false.is_truthy("show") &&
                       !unbound.is_truthy("show"));

    first.set("name", "a&b");
    second.set("name", "c");
    items.append_to_list("items", first);
    items.append_to_list("items", second);
    D_TESTS_TEXT_CHECK(items.has_list("items") &&
                       (items.get_list("items").size() == 2));
    D_TESTS_TEXT_CHECK(list.render(items) ==
                       "<ul><li>a&amp;b</li><li>c</li></ul>");

    both.set_list("items", std::vector<xml_context>());
    both.set("items", "yes");
    D_TESTS_TEXT_CHECK(list.render(both) == "<ul></ul>");
    D_TESTS_TEXT_CHECK(!unbound.has_list("items") &&
                       unbound.get_list("items").empty());

    parent.set("title", "T");
    parent.append_to_list("items", second);
    D_TESTS_TEXT_CHECK(scope.render(parent) == "c/;");

    nest.set_bool("outer", true);
    nest.set_bool("inner", true);
    D_TESTS_TEXT_CHECK(nested.render(nest) == "(x)");
    nest.set_bool("inner", false);
    D_TESTS_TEXT_CHECK(nested.render(nest) == "()");

    return;
}


/*
d_tests_markup_string_literals
  Tests the following:
  - a comment renders nothing
  - an unterminated tag is literal, with all that follows it
  - a close that closes nothing is literal: at the top level, where "{/}"
    once ended the template, and inside a section that it doesn't close
  - a section with no close runs to the end of the source
  - a section named "" is closed by "{/}"
  - text outside the tags is written as it is, not escaped
*/
void
d_tests_markup_string_literals(void)
{
    xml_context ctx;
    xml_context on;
    xml_context blank;

    ctx.set("a", "1");
    on.set_bool("s", true);
    blank.set_bool("", true);

    D_TESTS_TEXT_CHECK(xml_template("x{! a note }y").render(ctx) == "xy");

    D_TESTS_TEXT_CHECK(xml_template("x{a").render(ctx) == "x{a");
    D_TESTS_TEXT_CHECK(xml_template("{a}{b").render(ctx) == "1{b");

    D_TESTS_TEXT_CHECK(xml_template("x{/s}y").render(ctx) == "x{/s}y");
    D_TESTS_TEXT_CHECK(xml_template("x{/}y{a}").render(ctx) == "x{/}y1");
    D_TESTS_TEXT_CHECK(xml_template("{#s}a{/t}b{/s}c").render(on) ==
                       "a{/t}bc");

    D_TESTS_TEXT_CHECK(xml_template("{#s}open").render(on) == "open");
    D_TESTS_TEXT_CHECK(xml_template("{#}in{/}out").render(blank) == "inout");

    D_TESTS_TEXT_CHECK(xml_template("no tags & <raw>").render(ctx) ==
                       "no tags & <raw>");

    return;
}


/*
d_tests_markup_string_partials
  Tests the following:
  - a registered partial renders with the including template's context,
    and a missing one renders nothing
  - one registered by value is a copy: later changes to the original
    don't reach it
  - one registered by pointer is shared: they do; registering a name again
    replaces its partial
  - a partial in the context overrides the template's
  - partials include partials
  - remove_partial, and partial_count throughout
*/
void
d_tests_markup_string_partials(void)
{
    xml_template                  page("<h1>{title}</h1>{>body}{>missing}");
    xml_template                  body("<p>{text}</p>");
    std::shared_ptr<xml_template> shared =
                                      std::make_shared<xml_template>(
                                          "<i>{text}</i>");
    xml_template                  outer("({>mid})");
    xml_template                  mid("<{>leaf}>");
    const xml_template            leaf("{text}");
    xml_context                   ctx;
    xml_context                   over;

    ctx.set("title", "T");
    ctx.set("text", "a<b");

    page.add_partial("body", body);
    D_TESTS_TEXT_CHECK(page.partial_count() == 1);
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1><p>a&lt;b</p>");

    body.set_source("changed");
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1><p>a&lt;b</p>");

    page.add_partial("body", shared);
    D_TESTS_TEXT_CHECK(page.partial_count() == 1);
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1><i>a&lt;b</i>");
    shared->set_source("<b>{text}</b>");
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1><b>a&lt;b</b>");

    over = ctx;
    over.set_partial("body", std::make_shared<xml_template>("[{text}]"));
    D_TESTS_TEXT_CHECK(page.render(over) == "<h1>T</h1>[a&lt;b]");
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1><b>a&lt;b</b>");

    mid.add_partial("leaf", leaf);
    outer.add_partial("mid", mid);
    D_TESTS_TEXT_CHECK(outer.render(ctx) == "(<a&lt;b>)");

    page.remove_partial("body");
    D_TESTS_TEXT_CHECK(page.partial_count() == 0);
    D_TESTS_TEXT_CHECK(page.render(ctx) == "<h1>T</h1>");

    return;
}


/*
d_tests_markup_string_syntax_one
  The output of one syntax's source, with h bound to "<b>", s truthy and a
partial p of "[{h}]" registered.
*/
template<typename Syntax>
static std::string
d_tests_markup_string_syntax_one(
    const std::string& _source
)
{
    xml_template      tpl(_source, Syntax());
    const xml_template partial("[{&h}]");
    xml_context       ctx;

    ctx.set("h", "<b>");
    ctx.set_bool("s", true);
    tpl.add_partial("p", partial);

    return tpl.render(ctx);
}


/*
d_tests_markup_string_syntax
  Tests the following:
  - each of the nine bundled syntaxes reads an escaped variable, a raw one,
    a comment, a section and a partial in its own delimiters and markers
  - every syntax is a syntax by its tag
  - the explicit-delimiter constructor changes only the delimiters
  - empty delimiters render the source as it is
  - set_markers with an empty marker turns that directive class off
*/
void
d_tests_markup_string_syntax(void)
{
    using namespace djinterp;

    const std::string expect = "&lt;b&gt;|<b>||in|[<b>]";
    xml_template      verbatim("{h}");
    xml_template      unsectioned("{#s}in{/s}");
    xml_context       ctx;

    ctx.set("h", "<b>");
    ctx.set_bool("s", true);

    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxDefault>(
            "{h}|{&h}|{!c}|{#s}in{/s}|{>p}") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<
            DMarkupStringTemplateSyntaxHandlebars>(
            "{{h}}|{{&h}}|{{!c}}|{{#s}}in{{/s}}|{{>p}}") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxErb>(
            "<%= h %>|<%=== h %>|<%=# c %>|<%= begin s %>in<%= end s %>|"
            "<%= include p %>") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxDollar>(
            "$h$|$&h$|$!c$|$#s$in$/s$|$>p$") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxAngle>(
            "<<h>>|<<&h>>|<<!c>>|<<#s>>in<</s>>|<<>p>>") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxSquare>(
            "[[h]]|[[&h]]|[[!c]]|[[#s]]in[[/s]]|[[>p]]") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxPhp>(
            "<?= h ?>|<?= &h ?>|<?= // c ?>|<?= if s ?>in<?= endif s ?>|"
            "<?= include p ?>") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<
            DMarkupStringTemplateSyntaxCurlyDollar>(
            "${h}|${&h}|${!c}|${#s}in${/s}|${>p}") == expect);
    D_TESTS_TEXT_CHECK(
        d_tests_markup_string_syntax_one<DMarkupStringTemplateSyntaxXmlPi>(
            "<?tpl h ?>|<?tpl raw h ?>|<?tpl rem c ?>|<?tpl begin s ?>in"
            "<?tpl end s ?>|<?tpl include p ?>") == expect);

    D_TESTS_TEXT_CHECK((std::is_base_of<DMarkupStringTemplateSyntaxTag,
                                        DMarkupStringTemplateSyntaxDefault>::
                            value) &&
                       (std::is_base_of<DMarkupStringTemplateSyntaxTag,
                                        DMarkupStringTemplateSyntaxXmlPi>::
                            value));

    D_TESTS_TEXT_CHECK(xml_template("<<h>> {h}", "<<", ">>").render(ctx) ==
                       "&lt;b&gt; {h}");

    verbatim.set_delimiters("", "");
    D_TESTS_TEXT_CHECK(verbatim.render(ctx) == "{h}");

    unsectioned.set_markers("&", ">", "", "", "!");
    D_TESTS_TEXT_CHECK(unsectioned.render(ctx) == "in");

    return;
}


/*
d_tests_markup_string_lifecycle
  Tests the following:
  - source returns the source; render with no context renders the
    literals only
  - set_source replaces the source, and the next render reads it
  - render_to writes after what the stream holds
  - a copy renders as its original did, and is its own
  - a default template renders nothing
  - the factories, by value and shared
  - rendering leaves a template as it was: twice gives the same
*/
void
d_tests_markup_string_lifecycle(void)
{
    using namespace djinterp;

    xml_template                        changing("a{x}b");
    const xml_template                  unset;
    const xml_template                  made =
        make_markup_string_template<DXmlEscapePolicy,
                                    DMarkupStringTemplateSyntaxSquare>("[[x]]");
    const std::shared_ptr<xml_template> shared =
        make_shared_markup_string_template<DXmlEscapePolicy,
                                           DMarkupStringTemplateSyntaxAngle>(
            "<<x>>");
    const xml_template                  twice("{x}{x}");
    xml_context                         ctx;
    std::ostringstream                  out;

    ctx.set("x", "1");

    D_TESTS_TEXT_CHECK(changing.source() == "a{x}b");
    D_TESTS_TEXT_CHECK(changing.render() == "ab");

    changing.set_source("[{x}]");
    D_TESTS_TEXT_CHECK((changing.source() == "[{x}]") &&
                       (changing.render(ctx) == "[1]"));

    out << ">";
    changing.render_to(out, ctx);
    D_TESTS_TEXT_CHECK(out.str() == ">[1]");

    {
        const xml_template copy(changing);

        changing.set_source("other");
        D_TESTS_TEXT_CHECK(copy.render(ctx) == "[1]");
        D_TESTS_TEXT_CHECK(changing.render(ctx) == "other");
    }

    D_TESTS_TEXT_CHECK(unset.source().empty() && unset.render().empty());

    D_TESTS_TEXT_CHECK(made.render(ctx) == "1");
    D_TESTS_TEXT_CHECK(shared && (shared->render(ctx) == "1"));

    D_TESTS_TEXT_CHECK((twice.render(ctx) == "11") &&
                       (twice.render(ctx) == "11"));

    return;
}

#endif  // C++11
